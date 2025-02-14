/* === S Y N F I G ========================================================= */
/*!	\file colorcorrect.cpp
**	\brief Implementation of the "Color Correct" layer
**
**	$Id$
**
**	\legal
**	Copyright (c) 2002-2005 Robert B. Quattlebaum Jr., Adrian Bentley
**	Copyright (c) 2012-2013 Carlos López
**
**	This package is free software; you can redistribute it and/or
**	modify it under the terms of the GNU General Public License as
**	published by the Free Software Foundation; either version 2 of
**	the License, or (at your option) any later version.
**
**	This package is distributed in the hope that it will be useful,
**	but WITHOUT ANY WARRANTY; without even the implied warranty of
**	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
**	General Public License for more details.
**	\endlegal
*/
/* ========================================================================= */

/* === H E A D E R S ======================================================= */

#ifdef USING_PCH
#	include "pch.h"
#else
#ifdef HAVE_CONFIG_H
#	include <config.h>
#endif

#include <synfig/localization.h>
#include <synfig/general.h>

#include "colorcorrect.h"
#include <synfig/string.h>
#include <synfig/time.h>
#include <synfig/context.h>
#include <synfig/paramdesc.h>
#include <synfig/renddesc.h>
#include <synfig/surface.h>
#include <synfig/value.h>
#include <synfig/valuenode.h>
#include <synfig/cairo_renddesc.h>

#include <synfig/rendering/software/surfacesw.h>

#endif

/* === U S I N G =========================================================== */

using namespace etl;
using namespace std;
using namespace synfig;
using namespace modules;
using namespace mod_filter;

/* === G L O B A L S ======================================================= */

SYNFIG_LAYER_INIT(Layer_ColorCorrect);
SYNFIG_LAYER_SET_NAME(Layer_ColorCorrect,"colorcorrect");
SYNFIG_LAYER_SET_LOCAL_NAME(Layer_ColorCorrect,N_("Color Correct"));
SYNFIG_LAYER_SET_CATEGORY(Layer_ColorCorrect,N_("Filters"));
SYNFIG_LAYER_SET_VERSION(Layer_ColorCorrect,"0.1");
SYNFIG_LAYER_SET_CVS_ID(Layer_ColorCorrect,"$Id$");

/* === P R O C E D U R E S ================================================= */

/* === M E T H O D S ======================================================= */

/* === E N T R Y P O I N T ================================================= */

void
TaskColorCorrectSW::split(const RectInt &sub_target_rect)
{
	trunc_target_rect(sub_target_rect);
	if (valid_target() && sub_task() && sub_task()->valid_target())
	{
		sub_task() = sub_task()->clone();
		sub_task()->trunc_target_rect(
			get_target_rect()
			- get_target_offset()
			- get_offset() );
	}
}

void
TaskColorCorrectSW::correct_pixel(Color &dst, const Color &src, const Angle &hue_adjust, ColorReal shift, ColorReal amplifier, const Gamma &gamma) const
{
	static const double precision = 1e-8;

	dst = src;

	if (fabs(gamma.get_gamma_r() - 1.0) > precision)
	{
		if (dst.get_r() < 0)
			dst.set_r(-gamma.r_F32_to_F32(-dst.get_r()));
		else
			dst.set_r(gamma.r_F32_to_F32(dst.get_r()));
	}

	if (fabs(gamma.get_gamma_g() - 1.0) > precision)
	{
		if (dst.get_g() < 0)
			dst.set_g(-gamma.g_F32_to_F32(-dst.get_g()));
		else
			dst.set_g(gamma.g_F32_to_F32(dst.get_g()));
	}

	if (fabs(gamma.get_gamma_b() - 1.0) > precision)
	{
		if (dst.get_b() < 0)
			dst.set_b(-gamma.b_F32_to_F32(-dst.get_b()));
		else
			dst.set_b(gamma.b_F32_to_F32(dst.get_b()));
	}

	assert(!isnan(dst.get_r()));
	assert(!isnan(dst.get_g()));
	assert(!isnan(dst.get_b()));

	if (fabs(amplifier - 1.0) > precision)
	{
		dst.set_r(dst.get_r()*amplifier);
		dst.set_g(dst.get_g()*amplifier);
		dst.set_b(dst.get_b()*amplifier);
	}

	if (fabs(shift) > precision)
	{
		// Adjust R Channel Brightness
		if (dst.get_r() > -shift)
			dst.set_r(dst.get_r() + shift);
		else
		if(dst.get_r() < shift)
			dst.set_r(dst.get_r() - shift);
		else
			dst.set_r(0);

		// Adjust G Channel Brightness
		if (dst.get_g() > -shift)
			dst.set_g(dst.get_g() + shift);
		else
		if(dst.get_g() < shift)
			dst.set_g(dst.get_g() - shift);
		else
			dst.set_g(0);

		// Adjust B Channel Brightness
		if (dst.get_b() > -shift)
			dst.set_b(dst.get_b() + shift);
		else
		if(dst.get_b() < shift)
			dst.set_b(dst.get_b() - shift);
		else
			dst.set_b(0);
	}

	// Return the color, adjusting the hue if necessary
	if (!!hue_adjust)
		dst.rotate_uv(hue_adjust);
}

bool
TaskColorCorrectSW::run(RunParams & /* params */) const
{
	const synfig::Surface &a =
		rendering::SurfaceSW::Handle::cast_dynamic( sub_task()->target_surface )->get_surface();
	synfig::Surface &c =
		rendering::SurfaceSW::Handle::cast_dynamic( target_surface )->get_surface();

	//debug::DebugSurface::save_to_file(a, "TaskClampSW__run__a");

	RectInt r = get_target_rect();
	if (r.valid())
	{
		VectorInt offset = get_offset();
		RectInt ra = sub_task()->get_target_rect() + r.get_min() + offset;
		if (ra.valid())
		{
			etl::set_intersect(ra, ra, r);
			if (ra.valid())
			{
				ColorReal amplifier = (ColorReal)(contrast*exp(exposure));
				ColorReal shift = (ColorReal)((brightness - 0.5)*contrast + 0.5);
				Gamma g(fabs(gamma) < 1e-8 ? 1.0 : 1.0/gamma);

				synfig::Surface::pen pc = c.get_pen(ra.minx, ra.maxx);
				synfig::Surface::pen pa = c.get_pen(ra.minx, ra.maxx);
				for(int y = ra.miny; y < ra.maxy; ++y)
				{
					const Color *ca = &a[y - r.miny - offset[1]][ra.minx - r.minx - offset[0]];
					Color *cc = &c[y][ra.minx];
					for(int x = ra.minx; x < ra.maxx; ++x, ++ca, ++cc)
						correct_pixel(*cc, *ca, hue_adjust, shift, amplifier, g);
				}
			}
		}
	}

	//debug::DebugSurface::save_to_file(c, "TaskClampSW__run__c");

	return true;
}


void
OptimizerColorCorrectSW::run(const RunParams& params) const
{
	TaskColorCorrect::Handle color_correct = TaskColorCorrect::Handle::cast_dynamic(params.ref_task);
	if ( color_correct
	  && color_correct->target_surface
	  && color_correct.type_equal<TaskColorCorrect>() )
	{
		TaskColorCorrectSW::Handle color_correct_sw;
		init_and_assign_all<rendering::SurfaceSW>(color_correct_sw, color_correct);

		// TODO: Are we really need to check 'is_temporary' flag?
		if ( color_correct_sw->sub_task()->target_surface->is_temporary )
		{
			color_correct_sw->sub_task()->target_surface = color_correct_sw->target_surface;
			color_correct_sw->sub_task()->move_target_rect(
				color_correct_sw->get_target_offset() );
		}
		else
		{
			color_correct_sw->sub_task()->set_target_origin( VectorInt::zero() );
			color_correct_sw->sub_task()->target_surface->set_size(
				color_correct_sw->sub_task()->get_target_rect().maxx,
				color_correct_sw->sub_task()->get_target_rect().maxy );
		}
		assert( color_correct_sw->sub_task()->check() );

		apply(params, color_correct_sw);
	}
}


Layer_ColorCorrect::Layer_ColorCorrect():
	param_hue_adjust(ValueBase(Angle::zero())),
	param_brightness(ValueBase(Real(0))),
	param_contrast(ValueBase(Real(1.0))),
	param_exposure(ValueBase(Real(0.0))),
	param_gamma(ValueBase(Real(1.0)))
{
	SET_INTERPOLATION_DEFAULTS();
	SET_STATIC_DEFAULTS();
}

inline Color
Layer_ColorCorrect::correct_color(const Color &in)const
{
	Angle hue_adjust=param_hue_adjust.get(Angle());
	Real _brightness=param_brightness.get(Real());
	Real contrast=param_contrast.get(Real());
	Real exposure=param_exposure.get(Real());
	
	Color ret(in);
	Real brightness((_brightness-0.5)*contrast+0.5);

	if(gamma.get_gamma_r()!=1.0)
	{
		if(ret.get_r() < 0)
		{
			ret.set_r(-gamma.r_F32_to_F32(-ret.get_r()));
		}else
		{
			ret.set_r(gamma.r_F32_to_F32(ret.get_r()));
		}
	}
	if(gamma.get_gamma_g()!=1.0)
	{
		if(ret.get_g() < 0)
		{
			ret.set_g(-gamma.g_F32_to_F32(-ret.get_g()));
		}else
		{
			ret.set_g(gamma.g_F32_to_F32(ret.get_g()));
		}
	}
	if(gamma.get_gamma_b()!=1.0)
	{
		if(ret.get_b() < 0)
		{
			ret.set_b(-gamma.b_F32_to_F32(-ret.get_b()));
		}else
		{
			ret.set_b(gamma.b_F32_to_F32(ret.get_b()));
		}
	}

	assert(!std::isnan(ret.get_r()));
	assert(!std::isnan(ret.get_g()));
	assert(!std::isnan(ret.get_b()));

	if(exposure!=0.0)
	{
		const float factor(exp(exposure));
		ret.set_r(ret.get_r()*factor);
		ret.set_g(ret.get_g()*factor);
		ret.set_b(ret.get_b()*factor);
	}

	// Adjust Contrast
	if(contrast!=1.0)
	{
		ret.set_r(ret.get_r()*contrast);
		ret.set_g(ret.get_g()*contrast);
		ret.set_b(ret.get_b()*contrast);
	}

	if(brightness)
	{
		// Adjust R Channel Brightness
		if(ret.get_r()>-brightness)
			ret.set_r(ret.get_r()+brightness);
		else if(ret.get_r()<brightness)
			ret.set_r(ret.get_r()-brightness);
		else
			ret.set_r(0);

		// Adjust G Channel Brightness
		if(ret.get_g()>-brightness)
			ret.set_g(ret.get_g()+brightness);
		else if(ret.get_g()<brightness)
			ret.set_g(ret.get_g()-brightness);
		else
			ret.set_g(0);

		// Adjust B Channel Brightness
		if(ret.get_b()>-brightness)
			ret.set_b(ret.get_b()+brightness);
		else if(ret.get_b()<brightness)
			ret.set_b(ret.get_b()-brightness);
		else
			ret.set_b(0);
	}

	// Return the color, adjusting the hue if necessary
	if(!!hue_adjust)
		return ret.rotate_uv(hue_adjust);
	else
		return ret;
}

bool
Layer_ColorCorrect::set_param(const String & param, const ValueBase &value)
{
	IMPORT_VALUE(param_hue_adjust);
	IMPORT_VALUE(param_brightness);
	IMPORT_VALUE(param_contrast);
	IMPORT_VALUE(param_exposure);

	IMPORT_VALUE_PLUS(param_gamma,
		{
			gamma.set_gamma(1.0/param_gamma.get(Real()));
			return true;
		});
	return false;
}

ValueBase
Layer_ColorCorrect::get_param(const String &param)const
{
	EXPORT_VALUE(param_hue_adjust);
	EXPORT_VALUE(param_brightness);
	EXPORT_VALUE(param_contrast);
	EXPORT_VALUE(param_exposure);

	if(param=="gamma")
	{
		ValueBase ret=param_gamma;
		ret.set(1.0/gamma.get_gamma());
		return ret;
	}

	EXPORT_NAME();
	EXPORT_VERSION();

	return ValueBase();
}

Layer::Vocab
Layer_ColorCorrect::get_param_vocab()const
{
	Layer::Vocab ret;

	ret.push_back(ParamDesc("hue_adjust")
		.set_local_name(_("Hue Adjust"))
	);

	ret.push_back(ParamDesc("brightness")
		.set_local_name(_("Brightness"))
	);

	ret.push_back(ParamDesc("contrast")
		.set_local_name(_("Contrast"))
	);

	ret.push_back(ParamDesc("exposure")
		.set_local_name(_("Exposure Adjust"))
	);

	ret.push_back(ParamDesc("gamma")
		.set_local_name(_("Gamma Adjustment"))
	);

	return ret;
}

Color
Layer_ColorCorrect::get_color(Context context, const Point &pos)const
{
	return correct_color(context.get_color(pos));
}

bool
Layer_ColorCorrect::accelerated_render(Context context,Surface *surface,int quality, const RendDesc &renddesc, ProgressCallback *cb)const
{
	RENDER_TRANSFORMED_IF_NEED(__FILE__, __LINE__)

	SuperCallback supercb(cb,0,9500,10000);

	if(!context.accelerated_render(surface,quality,renddesc,&supercb))
		return false;

	int x,y;

	Surface::pen pen(surface->begin());

	for(y=0;y<renddesc.get_h();y++,pen.inc_y(),pen.dec_x(x))
		for(x=0;x<renddesc.get_w();x++,pen.inc_x())
			pen.put_value(correct_color(pen.get_value()));

	// Mark our progress as finished
	if(cb && !cb->amount_complete(10000,10000))
		return false;

	return true;
}

bool
Layer_ColorCorrect::accelerated_cairorender(Context context, cairo_t *cr, int quality, const RendDesc &renddesc_, ProgressCallback *cb)const
{
	RendDesc	renddesc(renddesc_);
	
	// Untransform the render desc
	if(!cairo_renddesc_untransform(cr, renddesc))
		return false;
	
	const Real pw(renddesc.get_pw()),ph(renddesc.get_ph());
	const Point tl(renddesc.get_tl());
	const int w(renddesc.get_w());
	const int h(renddesc.get_h());

	int x,y;

	SuperCallback supercb(cb,0,9500,10000);

	cairo_surface_t *surface;
	
	surface=cairo_surface_create_similar(cairo_get_target(cr), CAIRO_CONTENT_COLOR_ALPHA, w, h);
	cairo_t* subcr=cairo_create(surface);
	cairo_scale(subcr, 1/pw, 1/ph);
	cairo_translate(subcr, -tl[0], -tl[1]);
	
	if(!context.accelerated_cairorender(subcr,quality,renddesc,&supercb))
		return false;

	cairo_destroy(subcr);
	
	CairoSurface csurface(surface);
	csurface.map_cairo_image();
	
	CairoSurface::pen pen(csurface.begin());
	
	for(y=0;y<renddesc.get_h();y++,pen.inc_y(),pen.dec_x(x))
		for(x=0;x<renddesc.get_w();x++,pen.inc_x())
			pen.put_value(CairoColor(correct_color(Color(pen.get_value().demult_alpha())).clamped()).premult_alpha());
	
	csurface.unmap_cairo_image();
	
	// paint surface on cr
	cairo_save(cr);
	cairo_translate(cr, tl[0], tl[1]);
	cairo_scale(cr, pw, ph);
	cairo_set_source_surface(cr, surface, 0, 0);
	cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
	cairo_paint(cr);
	cairo_restore(cr);
	
	cairo_surface_destroy(surface);
	// Mark our progress as finished
	if(cb && !cb->amount_complete(10000,10000))
		return false;
	
	return true;
}

Rect
Layer_ColorCorrect::get_full_bounding_rect(Context context)const
{
	return context.get_full_bounding_rect();
}

rendering::Task::Handle
Layer_ColorCorrect::build_rendering_task_vfunc(Context context)const
{
	TaskColorCorrect::Handle task_color_correct(new TaskColorCorrect());
	task_color_correct->hue_adjust = param_hue_adjust.get(Angle());
	task_color_correct->brightness = param_brightness.get(Real());
	task_color_correct->contrast = param_contrast.get(Real());
	task_color_correct->exposure = param_exposure.get(Real());
	task_color_correct->gamma = param_gamma.get(Real());
	task_color_correct->sub_task() = context.build_rendering_task();
	return task_color_correct;
}

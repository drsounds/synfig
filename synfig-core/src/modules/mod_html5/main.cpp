/* === S Y N F I G ========================================================= */
/*!	\file mod_html5/main.cpp
**	\brief Template Header
**
**	$Id$
**
**	\legal
**	Copyright (c) 2016 Alexander Forselius
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
**
** === N O T E S ===========================================================
**
** ========================================================================= */

/* === H E A D E R S ======================================================= */

#define SYNFIG_MODULE

#ifdef USING_PCH
#	include "pch.h"
#else
#ifdef HAVE_CONFIG_H
#	include <config.h>
#endif

#include <synfig/localization.h>
#include <synfig/general.h>

#include <synfig/module.h>
#include "mptr_html5.h"
#include "trgt_html5.h"
#endif

/* === E N T R Y P O I N T ================================================= */

MODULE_DESC_BEGIN(mod_html5)
	MODULE_NAME("HTML5 Module")
	MODULE_DESCRIPTION("Provides ability to export animation as html5")
	MODULE_AUTHOR("Alexander Forselius")
	MODULE_VERSION("1.0")
	MODULE_COPYRIGHT(SYNFIG_COPYRIGHT)
MODULE_DESC_END

MODULE_INVENTORY_BEGIN(mod_html5)
	BEGIN_TARGETS
		TARGET(ffmpeg_trgt)
		TARGET_EXT(ffmpeg_trgt,"")
	END_TARGETS
	BEGIN_IMPORTERS
	END_IMPORTERS
MODULE_INVENTORY_END

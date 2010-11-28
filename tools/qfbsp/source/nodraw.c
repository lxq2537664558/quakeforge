/*
	Copyright (C) 1996-1997  Id Software, Inc.

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA

	See file, 'COPYING', for details.
*/
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

static __attribute__ ((used)) const char rcsid[] =
	"$Id$";

#include "draw.h"

/**	\addtogroup qfbsp_draw
*/
//@{


void
Draw_ClearBounds (void)
{
}

void
Draw_AddToBounds (const vec3_t v)
{
}

void
Draw_DrawFace (const struct visfacet_s *f)
{
}

void
Draw_ClearWindow (void)
{
}

void
Draw_SetRed (void)
{
}

void
Draw_SetGrey (void)
{
}

void
Draw_SetBlack (void)
{
}

void
DrawPoint (const vec3_t v)
{
}

void
DrawLeaf (const struct node_s *l, int color)
{
}

void
DrawBrush (const struct brush_s *b)
{
}

void
DrawWinding (const struct winding_s *w)
{
}

void
DrawTri (const vec3_t p1, const vec3_t p2, const vec3_t p3)
{
}

void
DrawPortal (const struct portal_s *portal)
{
}

//@}

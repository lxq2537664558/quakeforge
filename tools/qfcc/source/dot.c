/*
	dot.c

	General dot output support

	Copyright (C) 2012 Bill Currie <bill@taniwha.org>

	Author: Bill Currie <bill@taniwha.org>
	Date: 2012/11/07

	This program is free software; you can redistribute it and/or
	modify it under the terms of the GNU General Public License
	as published by the Free Software Foundation; either version 2
	of the License, or (at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

	See the GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to:

		Free Software Foundation, Inc.
		59 Temple Place - Suite 330
		Boston, MA  02111-1307, USA

*/
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdlib.h>

#include "QF/va.h"

#include "dot.h"
#include "function.h"
#include "qfcc.h"
#include "strpool.h"

void
dump_dot (const char *stage, void *data,
		  void (*dump_func) (void *data, const char *fname))
{
	char       *fname;

	fname = nva ("%s.%s.%s.dot", GETSTR (pr.source_file), current_func->name,
				 stage);
	dump_func (data, fname);
	free (fname);
}

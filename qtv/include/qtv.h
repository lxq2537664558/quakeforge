/*
	#FILENAME#

	#DESCRIPTION#

	Copyright (C) 2004 #AUTHOR#

	Author: #AUTHOR#
	Date: #DATE#

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

	$Id$
*/

#ifndef __qtv_h
#define __qtv_h

#define PORT_QTV 27501

typedef enum {
	RD_NONE,
	RD_CLIENT,
	RD_PACKET,
} redirect_t;

extern double realtime;

extern struct cbuf_s *qtv_cbuf;
extern struct cbuf_args_s *qtv_args;

struct client_s;

void qtv_printf (const char *fmt, ...) __attribute__((format(printf,1,2)));
void qtv_begin_redirect (redirect_t rd, struct client_s *cl);
void qtv_end_redirect (void);
void qtv_flush_redirect (void);
void qtv_connectionless_packet (void);

#endif//__qtv_h
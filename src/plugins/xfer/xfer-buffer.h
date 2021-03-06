/*
 * Copyright (c) 2003-2009 by FlashCode <flashcode@flashtux.org>
 * See README for License detail, AUTHORS for developers list.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


#ifndef __WEECHAT_XFER_DISPLAY_H
#define __WEECHAT_XFER_DISPLAY_H 1

extern struct t_gui_buffer *xfer_buffer;
extern int xfer_buffer_selected_line;

extern void xfer_buffer_refresh (const char *hotlist);
extern int xfer_buffer_input_cb (void *data, struct t_gui_buffer *buffer,
                                 const char *input_data);
extern int xfer_buffer_close_cb (void *data, struct t_gui_buffer *buffer);
extern void xfer_buffer_open ();

#endif /* xfer-buffer.h */

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


#ifndef __WEECHAT_IRC_DISPLAY_H
#define __WEECHAT_IRC_DISPLAY_H 1

extern void irc_display_hide_password (char *string, int look_for_nickserv);
extern void irc_display_away (struct t_irc_server *server, const char *string1,
                              const char *string2);
extern void irc_display_server (struct t_irc_server *server, int with_detail);

#endif /* irc-display.h */

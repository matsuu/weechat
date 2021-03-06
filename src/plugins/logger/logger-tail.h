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


#ifndef __WEECHAT_LOGGER_TAIL_H
#define __WEECHAT_LOGGER_TAIL_H 1

struct t_logger_line
{
    char *data;                        /* line content                      */
    struct t_logger_line *next_line;   /* link to next line                 */
};

extern struct t_logger_line *logger_tail_file (const char *filename,
                                               int n_lines);
extern void logger_tail_free (struct t_logger_line *lines);

#endif /* logger-tail.h */

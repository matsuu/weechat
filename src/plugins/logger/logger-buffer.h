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


#ifndef __WEECHAT_LOGGER_BUFFER_H
#define __WEECHAT_LOGGER_BUFFER_H 1

struct t_infolist;

struct t_logger_buffer
{
    struct t_gui_buffer *buffer;          /* pointer to buffer              */
    char *log_filename;                   /* log filename                   */
    FILE *log_file;                       /* log file                       */
    int log_enabled;                      /* log enabled ?                  */
    int log_level;                        /* log level (0..9)               */
    struct t_logger_buffer *prev_buffer;  /* link to previous buffer        */
    struct t_logger_buffer *next_buffer;  /* link to next buffer            */
};

extern struct t_logger_buffer *logger_buffers;
extern struct t_logger_buffer *last_logger_buffer;

extern int logger_buffer_valid (struct t_logger_buffer *logger_buffer);
extern struct t_logger_buffer *logger_buffer_add (struct t_gui_buffer *,
                                                  int log_level);
extern struct t_logger_buffer *logger_buffer_search_buffer (struct t_gui_buffer *buffer);
extern struct t_logger_buffer *logger_buffer_search_log_filename (const char *log_filename);
extern void logger_buffer_free (struct t_logger_buffer *logger_buffer);
extern int logger_buffer_add_to_infolist (struct t_infolist *infolist,
                                          struct t_logger_buffer *logger_buffer);

#endif /* logger-buffer.h */

/*
 * Copyright (c) 2003-2006 by FlashCode <flashcode@flashtux.org>
 * See README for License detail, AUTHORS for developers list.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

/* gui-infobar.c: infobar display functions for Curses GUI */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <string.h>

#ifdef HAVE_NCURSESW_CURSES_H
#include <ncursesw/ncurses.h>
#else
#include <ncurses.h>
#endif

#include "../../common/weechat.h"
#include "../gui.h"
#include "../../common/hotlist.h"
#include "../../common/weeconfig.h"
#include "gui-curses.h"


/*
 * gui_infobar_draw_time: draw time in infobar window
 */

void
gui_infobar_draw_time (t_gui_buffer *buffer)
{
    t_gui_window *ptr_win;
    time_t time_seconds;
    struct tm *local_time;
    
    /* make gcc happy */
    (void) buffer;
    
    if (!gui_ok)
        return;
    
    for (ptr_win = gui_windows; ptr_win; ptr_win = ptr_win->next_window)
    {        
        time_seconds = time (NULL);
        local_time = localtime (&time_seconds);
        if (local_time)
        {
            gui_window_set_weechat_color (ptr_win->win_infobar, COLOR_WIN_INFOBAR);
            mvwprintw (ptr_win->win_infobar,
                       0, 1,
                       "%02d:%02d",
                       local_time->tm_hour, local_time->tm_min);
            if (cfg_look_infobar_seconds)
                wprintw (ptr_win->win_infobar,
                         ":%02d",
                         local_time->tm_sec);
        }
        wnoutrefresh (ptr_win->win_infobar);
    }
}

/*
 * gui_infobar_draw: draw infobar window for a buffer
 */

void
gui_infobar_draw (t_gui_buffer *buffer, int erase)
{
    t_gui_window *ptr_win;
    time_t time_seconds;
    struct tm *local_time;
    char text_time[1024 + 1];
    
    /* make gcc happy */
    (void) buffer;
    
    if (!gui_ok)
        return;
    
    for (ptr_win = gui_windows; ptr_win; ptr_win = ptr_win->next_window)
    {
        if (erase)
            gui_window_curses_clear (ptr_win->win_infobar, COLOR_WIN_INFOBAR);
        
        gui_window_set_weechat_color (ptr_win->win_infobar, COLOR_WIN_INFOBAR);
        
        time_seconds = time (NULL);
        local_time = localtime (&time_seconds);
        if (local_time)
        {
            strftime (text_time, 1024, cfg_look_infobar_timestamp, local_time);
            gui_window_set_weechat_color (ptr_win->win_infobar, COLOR_WIN_INFOBAR_DELIMITERS);
            wprintw (ptr_win->win_infobar, "[");
            gui_window_set_weechat_color (ptr_win->win_infobar, COLOR_WIN_INFOBAR);
            wprintw (ptr_win->win_infobar,
                     "%02d:%02d",
                     local_time->tm_hour, local_time->tm_min);
            if (cfg_look_infobar_seconds)
                wprintw (ptr_win->win_infobar,
                         ":%02d",
                         local_time->tm_sec);
            gui_window_set_weechat_color (ptr_win->win_infobar, COLOR_WIN_INFOBAR_DELIMITERS);
            wprintw (ptr_win->win_infobar, "]");
            gui_window_set_weechat_color (ptr_win->win_infobar, COLOR_WIN_INFOBAR);
            wprintw (ptr_win->win_infobar,
                     " %s", text_time);
        }
        if (gui_infobar)
        {
            gui_window_set_weechat_color (ptr_win->win_infobar, COLOR_WIN_INFOBAR_DELIMITERS);
            wprintw (ptr_win->win_infobar, " | ");
            gui_window_set_weechat_color (ptr_win->win_infobar, gui_infobar->color);
            wprintw (ptr_win->win_infobar, "%s", gui_infobar->text);
        }
        
        wnoutrefresh (ptr_win->win_infobar);
        refresh ();
    }
}
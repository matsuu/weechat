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

/* gui-window.c: window display functions for Curses GUI */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <libgen.h>

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
 * gui_window_curses_clear: clear a Curses window
 */

void
gui_window_curses_clear (WINDOW *window, int num_color)
{
    if (!gui_ok)
        return;
    
    wbkgdset(window, ' ' | COLOR_PAIR (gui_color_get_pair (num_color)));
    werase (window);
    wmove (window, 0, 0);
}

/*
 * gui_window_set_weechat_color: set WeeChat color for window
 */

void
gui_window_set_weechat_color (WINDOW *window, int num_color)
{
    if ((num_color >= 0) && (num_color <= GUI_NUM_COLORS - 1))
    {
        wattroff (window, A_BOLD | A_UNDERLINE | A_REVERSE);
        wattron (window, COLOR_PAIR(gui_color_get_pair (num_color)) |
                 gui_color[num_color]->attributes);
    }
}

/*
 * gui_window_calculate_pos_size: calculate position and size for a buffer & subwindows
 *                                return 1 if pos/size changed, 0 if no change
 */

int
gui_window_calculate_pos_size (t_gui_window *window, int force_calculate)
{
    int max_length, max_height, lines;
    int num_nicks, num_op, num_halfop, num_voice, num_normal;
    
    if (!gui_ok)
        return 0;
    
    /* init chat & nicklist settings */
    if (cfg_look_nicklist && BUFFER_IS_CHANNEL(window->buffer))
    {
        max_length = nick_get_max_length (CHANNEL(window->buffer));
        
        lines = 0;
        
        if ((cfg_look_nicklist_position == CFG_LOOK_NICKLIST_LEFT) ||
            (cfg_look_nicklist_position == CFG_LOOK_NICKLIST_RIGHT))
        {
            if ((cfg_look_nicklist_min_size > 0)
                && (max_length < cfg_look_nicklist_min_size))
                max_length = cfg_look_nicklist_min_size;
            else if ((cfg_look_nicklist_max_size > 0)
                     && (max_length > cfg_look_nicklist_max_size))
                max_length = cfg_look_nicklist_max_size;
            if (!force_calculate && (window->win_nick_width == max_length + 2))
                return 0;
        }
        else
        {
            nick_count (CHANNEL(window->buffer), &num_nicks, &num_op,
                        &num_halfop, &num_voice, &num_normal);
            if (((max_length + 2) * num_nicks) % window->win_width == 0)
                lines = ((max_length + 2) * num_nicks) / window->win_width;
            else
                lines = (((max_length + 2) * num_nicks) / window->win_width) + 1;
            if ((cfg_look_nicklist_max_size > 0) && (lines > cfg_look_nicklist_max_size))
                lines = cfg_look_nicklist_max_size;
            if ((cfg_look_nicklist_min_size > 0) && (lines < cfg_look_nicklist_min_size))
                lines = cfg_look_nicklist_min_size;
            max_height = (cfg_look_infobar) ?
                window->win_height - 3 - 4 : window->win_height - 2 - 4;
            if (lines > max_height)
                lines = max_height;
            if (!force_calculate && (window->win_nick_height == lines + 1))
                return 0;
        }
        
        switch (cfg_look_nicklist_position)
        {
            case CFG_LOOK_NICKLIST_LEFT:
                window->win_chat_x = window->win_x + max_length + 2;
                window->win_chat_y = window->win_y + 1;
                window->win_chat_width = window->win_width - max_length - 2;
                window->win_nick_x = window->win_x + 0;
                window->win_nick_y = window->win_y + 1;
                window->win_nick_width = max_length + 2;
                if (cfg_look_infobar)
                {
                    window->win_chat_height = window->win_height - 4;
                    window->win_nick_height = window->win_height - 4;
                }
                else
                {
                    window->win_chat_height = window->win_height - 3;
                    window->win_nick_height = window->win_height - 3;
                }
                window->win_nick_num_max = window->win_nick_height;
                break;
            case CFG_LOOK_NICKLIST_RIGHT:
                window->win_chat_x = window->win_x;
                window->win_chat_y = window->win_y + 1;
                window->win_chat_width = window->win_width - max_length - 2;
                window->win_nick_x = window->win_x + window->win_width - max_length - 2;
                window->win_nick_y = window->win_y + 1;
                window->win_nick_width = max_length + 2;
                if (cfg_look_infobar)
                {
                    window->win_chat_height = window->win_height - 4;
                    window->win_nick_height = window->win_height - 4;
                }
                else
                {
                    window->win_chat_height = window->win_height - 3;
                    window->win_nick_height = window->win_height - 3;
                }
                window->win_nick_num_max = window->win_nick_height;
                break;
            case CFG_LOOK_NICKLIST_TOP:
                window->win_chat_x = window->win_x;
                window->win_chat_y = window->win_y + 1 + (lines + 1);
                window->win_chat_width = window->win_width;
                if (cfg_look_infobar)
                    window->win_chat_height = window->win_height - 3 - (lines + 1) - 1;
                else
                    window->win_chat_height = window->win_height - 3 - (lines + 1);
                window->win_nick_x = window->win_x;
                window->win_nick_y = window->win_y + 1;
                window->win_nick_width = window->win_width;
                window->win_nick_height = lines + 1;
                window->win_nick_num_max = lines * (window->win_nick_width / (max_length + 2));
                break;
            case CFG_LOOK_NICKLIST_BOTTOM:
                window->win_chat_x = window->win_x;
                window->win_chat_y = window->win_y + 1;
                window->win_chat_width = window->win_width;
                if (cfg_look_infobar)
                    window->win_chat_height = window->win_height - 3 - (lines + 1) - 1;
                else
                    window->win_chat_height = window->win_height - 3 - (lines + 1);
                window->win_nick_x = window->win_x;
                if (cfg_look_infobar)
                    window->win_nick_y = window->win_y + window->win_height - 2 - (lines + 1) - 1;
                else
                    window->win_nick_y = window->win_y + window->win_height - 2 - (lines + 1);
                window->win_nick_width = window->win_width;
                window->win_nick_height = lines + 1;
                window->win_nick_num_max = lines * (window->win_nick_width / (max_length + 2));
                break;
        }
        
        window->win_chat_cursor_x = window->win_x;
        window->win_chat_cursor_y = window->win_y;
    }
    else
    {
        window->win_chat_x = window->win_x;
        window->win_chat_y = window->win_y + 1;
        window->win_chat_width = window->win_width;
        if (cfg_look_infobar)
            window->win_chat_height = window->win_height - 4;
        else
            window->win_chat_height = window->win_height - 3;
        window->win_chat_cursor_x = window->win_x;
        window->win_chat_cursor_y = window->win_y;
        window->win_nick_x = -1;
        window->win_nick_y = -1;
        window->win_nick_width = -1;
        window->win_nick_height = -1;
        window->win_nick_num_max = -1;
    }
    
    return 1;
}

/*
 * gui_window_draw_separator: draw window separation
 */

void
gui_window_draw_separator (t_gui_window *window)
{
    if (window->win_separator)
        delwin (window->win_separator);
    
    if (window->win_x > 0)
    {
        window->win_separator = newwin (window->win_height,
                                        1,
                                        window->win_y,
                                        window->win_x - 1);
        gui_window_set_weechat_color (window->win_separator, COLOR_WIN_SEPARATOR);
        wborder (window->win_separator, ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ');
        wnoutrefresh (window->win_separator);
        refresh ();
    }
}

/*
 * gui_window_redraw_buffer: redraw a buffer
 */

void
gui_window_redraw_buffer (t_gui_buffer *buffer)
{
    t_gui_window *ptr_win;
    
    if (!gui_ok)
        return;
    
    for (ptr_win = gui_windows; ptr_win; ptr_win = ptr_win->next_window)
    {
        if (ptr_win->buffer == buffer)
        {
            gui_chat_draw_title (buffer, 1);
            gui_chat_draw (buffer, 1);
            if (ptr_win->win_nick)
                gui_nicklist_draw (buffer, 1);
            gui_status_draw (buffer, 1);
            if (cfg_look_infobar)
                gui_infobar_draw (buffer, 1);
            gui_input_draw (buffer, 1);
        }
    }
}

/*
 * gui_window_switch_to_buffer: switch to another buffer
 */

void
gui_window_switch_to_buffer (t_gui_window *window, t_gui_buffer *buffer)
{
    if (!gui_ok)
        return;
    
    if (window->buffer->num_displayed > 0)
        window->buffer->num_displayed--;
    
    if (window->buffer != buffer)
    {
        window->buffer->last_read_line = window->buffer->last_line;
        if (buffer->last_read_line == buffer->last_line)
            buffer->last_read_line = NULL;
    }
    
    window->buffer = buffer;
    window->win_nick_start = 0;
    gui_window_calculate_pos_size (window, 1);
    
    /* destroy Curses windows */
    if (window->win_title)
    {
        delwin (window->win_title);
        window->win_title = NULL;
    }
    if (window->win_nick)
    {
        delwin (window->win_nick);
        window->win_nick = NULL;
    }
    if (window->win_status)
    {
        delwin (window->win_status);
        window->win_status = NULL;
    }
    if (window->win_infobar)
    {
        delwin (window->win_infobar);
        window->win_infobar = NULL;
    }
    if (window->win_input)
    {
        delwin (window->win_input);
        window->win_input = NULL;
    }
        
    /* create Curses windows */
    window->win_title = newwin (1,
                                window->win_width,
                                window->win_y,
                                window->win_x);
    window->win_input = newwin (1,
                                window->win_width,
                                window->win_y + window->win_height - 1,
                                window->win_x);
    if (BUFFER_IS_CHANNEL(buffer))
    {
        if (window->win_chat)
            delwin (window->win_chat);
        window->win_chat = newwin (window->win_chat_height,
                                   window->win_chat_width,
                                   window->win_chat_y,
                                   window->win_chat_x);
        if (cfg_look_nicklist)
            window->win_nick = newwin (window->win_nick_height,
                                       window->win_nick_width,
                                       window->win_nick_y,
                                       window->win_nick_x);
        else
            window->win_nick = NULL;
    }
    if (!(BUFFER_IS_CHANNEL(buffer)))
    {
        if (window->win_chat)
            delwin (window->win_chat);
        window->win_chat = newwin (window->win_chat_height,
                                   window->win_chat_width,
                                   window->win_chat_y,
                                   window->win_chat_x);
    }
    
    /* create status/infobar windows */
    if (cfg_look_infobar)
    {
        window->win_infobar = newwin (1, window->win_width,
                                      window->win_y + window->win_height - 2,
                                      window->win_x);
        window->win_status = newwin (1, window->win_width,
                                     window->win_y + window->win_height - 3,
                                     window->win_x);
    }
    else
        window->win_status = newwin (1, window->win_width,
                                     window->win_y + window->win_height - 2,
                                     window->win_x);
    
    window->start_line = NULL;
    window->start_line_pos = 0;
    
    buffer->num_displayed++;
    
    hotlist_remove_buffer (buffer);
}

/*
 * gui_window_page_up: display previous page on buffer
 */

void
gui_window_page_up (t_gui_window *window)
{
    if (!gui_ok)
        return;
    
    if (!window->first_line_displayed)
    {
        gui_chat_calculate_line_diff (window, &window->start_line,
                                      &window->start_line_pos,
                                      (window->start_line) ?
                                      (-1) * (window->win_chat_height - 1) :
                                      (-1) * ((window->win_chat_height - 1) * 2));
        gui_chat_draw (window->buffer, 0);
        gui_status_draw (window->buffer, 0);
    }
}

/*
 * gui_window_page_down: display next page on buffer
 */

void
gui_window_page_down (t_gui_window *window)
{
    t_gui_line *ptr_line;
    int line_pos;
    
    if (!gui_ok)
        return;
    
    if (window->start_line)
    {
        gui_chat_calculate_line_diff (window, &window->start_line,
                                      &window->start_line_pos,
                                      window->win_chat_height - 1);
        
        /* check if we can display all */
        ptr_line = window->start_line;
        line_pos = window->start_line_pos;
        gui_chat_calculate_line_diff (window, &ptr_line,
                                      &line_pos,
                                      window->win_chat_height - 1);
        if (!ptr_line)
        {
            window->start_line = NULL;
            window->start_line_pos = 0;
        }
        
        gui_chat_draw (window->buffer, 0);
        gui_status_draw (window->buffer, 0);
    }
}

/*
 * gui_window_scroll_up: display previous few lines in buffer
 */

void
gui_window_scroll_up (t_gui_window *window)
{
    if (!gui_ok)
        return;
    
    if (!window->first_line_displayed)
    {
        gui_chat_calculate_line_diff (window, &window->start_line,
                                      &window->start_line_pos,
                                      (window->start_line) ?
                                      (-1) * cfg_look_scroll_amount :
                                      (-1) * ( (window->win_chat_height - 1) + cfg_look_scroll_amount));
        gui_chat_draw (window->buffer, 0);
        gui_status_draw (window->buffer, 0);
    }
}

/*
 * gui_window_scroll_down: display next few lines in buffer
 */

void
gui_window_scroll_down (t_gui_window *window)
{
    t_gui_line *ptr_line;
    int line_pos;
    
    if (!gui_ok)
        return;
    
    if (window->start_line)
    {
        gui_chat_calculate_line_diff (window, &window->start_line,
                                      &window->start_line_pos,
                                      cfg_look_scroll_amount);
        
        /* check if we can display all */
        ptr_line = window->start_line;
        line_pos = window->start_line_pos;
        gui_chat_calculate_line_diff (window, &ptr_line,
                                      &line_pos,
                                      window->win_chat_height - 1);
        
        if (!ptr_line)
        {
            window->start_line = NULL;
            window->start_line_pos = 0;
        }
        
        gui_chat_draw (window->buffer, 0);
        gui_status_draw (window->buffer, 0);
    }
}

/*
 * gui_window_scroll_top: scroll to top of buffer
 */

void
gui_window_scroll_top (t_gui_window *window)
{
    if (!gui_ok)
        return;
    
    if (!window->first_line_displayed)
    {
        window->start_line = window->buffer->lines;
        window->start_line_pos = 0;
        gui_chat_draw (window->buffer, 0);
        gui_status_draw (window->buffer, 0);
    }
}

/*
 * gui_window_scroll_bottom: scroll to bottom of buffer
 */

void
gui_window_scroll_bottom (t_gui_window *window)
{
    if (!gui_ok)
        return;
    
    if (window->start_line)
    {
        window->start_line = NULL;
        window->start_line_pos = 0;
        gui_chat_draw (window->buffer, 0);
        gui_status_draw (window->buffer, 0);
    }
}

/*
 * gui_window_nick_beginning: go to beginning of nicklist
 */

void
gui_window_nick_beginning (t_gui_window *window)
{
    if (!gui_ok)
        return;
    
    if (BUFFER_HAS_NICKLIST(window->buffer))
    {
        if (window->win_nick_start > 0)
        {
            window->win_nick_start = 0;
            gui_nicklist_draw (window->buffer, 1);
        }
    }
}

/*
 * gui_window_nick_end: go to the end of nicklist
 */

void
gui_window_nick_end (t_gui_window *window)
{
    int new_start;
    
    if (!gui_ok)
        return;
    
    if (BUFFER_HAS_NICKLIST(window->buffer))
    {
        new_start =
            CHANNEL(window->buffer)->nicks_count - window->win_nick_num_max;
        if (new_start < 0)
            new_start = 0;
        else if (new_start >= 1)
            new_start++;
        
        if (new_start != window->win_nick_start)
        {
            window->win_nick_start = new_start;
            gui_nicklist_draw (window->buffer, 1);
        }
    }
}

/*
 * gui_window_nick_page_up: scroll one page up in nicklist
 */

void
gui_window_nick_page_up (t_gui_window *window)
{
    if (!gui_ok)
        return;
    
    if (BUFFER_HAS_NICKLIST(window->buffer))
    {
        if (window->win_nick_start > 0)
        {
            window->win_nick_start -= (window->win_nick_num_max - 1);
            if (window->win_nick_start <= 1)
                window->win_nick_start = 0;
            gui_nicklist_draw (window->buffer, 1);
        }
    }
}

/*
 * gui_window_nick_page_down: scroll one page down in nicklist
 */

void
gui_window_nick_page_down (t_gui_window *window)
{
    if (!gui_ok)
        return;
    
    if (BUFFER_HAS_NICKLIST(window->buffer))
    {
        if ((CHANNEL(window->buffer)->nicks_count > window->win_nick_num_max)
            && (window->win_nick_start + window->win_nick_num_max - 1
                < CHANNEL(window->buffer)->nicks_count))
        {
            if (window->win_nick_start == 0)
                window->win_nick_start += (window->win_nick_num_max - 1);
            else
                window->win_nick_start += (window->win_nick_num_max - 2);
            gui_nicklist_draw (window->buffer, 1);
        }
    }
}

/*
 * gui_window_init_subwindows: init subviews for a WeeChat window
 */

void
gui_window_init_subwindows (t_gui_window *window)
{
    window->win_title = NULL;
    window->win_chat = NULL;
    window->win_nick = NULL;
    window->win_status = NULL;
    window->win_infobar = NULL;
    window->win_input = NULL;
}

/*
 * gui_window_auto_resize: auto-resize all windows, according to % of global size
 *                         This function is called after a terminal resize.
 *                         Returns 0 if ok, -1 if all window should be merged
 *                         (not enough space according to windows %)
 */

int
gui_window_auto_resize (t_gui_window_tree *tree,
                        int x, int y, int width, int height,
                        int simulate)
{
    int size1, size2;
    
    if (tree)
    {
        if (tree->window)
        {
            if ((width < WINDOW_MIN_WIDTH) || (height < WINDOW_MIN_HEIGHT))
                return -1;
            if (!simulate)
            {
                tree->window->win_x = x;
                tree->window->win_y = y;
                tree->window->win_width = width;
                tree->window->win_height = height;
            }
        }
        else
        {
            if (tree->split_horiz)
            {
                size1 = (height * tree->split_pct) / 100;
                size2 = height - size1;
                if (gui_window_auto_resize (tree->child1, x, y + size1,
                                            width, size2, simulate) < 0)
                    return -1;
                if (gui_window_auto_resize (tree->child2, x, y,
                                            width, size1, simulate) < 0)
                    return -1;
            }
            else
            {
                size1 = (width * tree->split_pct) / 100;
                size2 = width - size1 - 1;
                if (gui_window_auto_resize (tree->child1, x, y,
                                            size1, height, simulate) < 0)
                    return -1;
                if (gui_window_auto_resize (tree->child2, x + size1 + 1, y,
                                            size2, height, simulate) < 0)
                    return -1;
            }
        }
    }
    return 0;
}

/*
 * gui_window_refresh_windows: auto resize and refresh all windows
 */

void
gui_window_refresh_windows ()
{
    t_gui_window *ptr_win, *old_current_window;
    
    if (gui_ok)
    {
        old_current_window = gui_current_window;
        
        if (gui_window_auto_resize (gui_windows_tree, 0, 0, COLS, LINES, 0) < 0)
            gui_window_merge_all (gui_current_window);
    
        for (ptr_win = gui_windows; ptr_win; ptr_win = ptr_win->next_window)
        {
            gui_window_switch_to_buffer (ptr_win, ptr_win->buffer);
            gui_window_redraw_buffer (ptr_win->buffer);
            gui_window_draw_separator (ptr_win);
        }
        
        gui_current_window = old_current_window;
        gui_window_switch_to_buffer (gui_current_window, gui_current_window->buffer);
        gui_window_redraw_buffer (gui_current_window->buffer);
    }
}

/*
 * gui_window_split_horiz: split a window horizontally
 */

void
gui_window_split_horiz (t_gui_window *window, int pourcentage)
{
    t_gui_window *new_window;
    int height1, height2;
    
    if (!gui_ok)
        return;
    
    height1 = (window->win_height * pourcentage) / 100;
    height2 = window->win_height - height1;
    
    if ((height1 >= WINDOW_MIN_HEIGHT) && (height2 >= WINDOW_MIN_HEIGHT)
        && (pourcentage > 0) && (pourcentage <= 100))
    {
        if ((new_window = gui_window_new (window,
                                          window->win_x, window->win_y,
                                          window->win_width, height1,
                                          100, pourcentage)))
        {
            /* reduce old window height (bottom window) */
            window->win_y = new_window->win_y + new_window->win_height;
            window->win_height = height2;
            window->win_height_pct = 100 - pourcentage;
            
            /* assign same buffer for new window (top window) */
            new_window->buffer = window->buffer;
            new_window->buffer->num_displayed++;
            
            gui_window_switch_to_buffer (window, window->buffer);
            
            gui_current_window = new_window;
            gui_window_switch_to_buffer (gui_current_window, gui_current_window->buffer);
            gui_window_redraw_buffer (gui_current_window->buffer);
        }
    }
}

/*
 * gui_window_split_vertic: split a window vertically
 */

void
gui_window_split_vertic (t_gui_window *window, int pourcentage)
{
    t_gui_window *new_window;
    int width1, width2;
    
    if (!gui_ok)
        return;
    
    width1 = (window->win_width * pourcentage) / 100;
    width2 = window->win_width - width1 - 1;
    
    if ((width1 >= WINDOW_MIN_WIDTH) && (width2 >= WINDOW_MIN_WIDTH)
        && (pourcentage > 0) && (pourcentage <= 100))
    {
        if ((new_window = gui_window_new (window,
                                          window->win_x + width1 + 1, window->win_y,
                                          width2, window->win_height,
                                          pourcentage, 100)))
        {
            /* reduce old window height (left window) */
            window->win_width = width1;
            window->win_width_pct = 100 - pourcentage;
            
            /* assign same buffer for new window (right window) */
            new_window->buffer = window->buffer;
            new_window->buffer->num_displayed++;
            
            gui_window_switch_to_buffer (window, window->buffer);
            
            gui_current_window = new_window;
            gui_window_switch_to_buffer (gui_current_window, gui_current_window->buffer);
            gui_window_redraw_buffer (gui_current_window->buffer);
            
            /* create & draw separator */
            gui_window_draw_separator (gui_current_window);
        }
    }
}

/*
 * gui_window_resize: resize window
 */

void
gui_window_resize (t_gui_window *window, int pourcentage)
{
    t_gui_window_tree *parent;
    int old_split_pct;
    
    parent = window->ptr_tree->parent_node;
    if (parent)
    {
        old_split_pct = parent->split_pct;
        if (((parent->split_horiz) && (window->ptr_tree == parent->child2))
            || ((!(parent->split_horiz)) && (window->ptr_tree == parent->child1)))
            parent->split_pct = pourcentage;
        else
            parent->split_pct = 100 - pourcentage;
        if (gui_window_auto_resize (gui_windows_tree, 0, 0, COLS, LINES, 1) < 0)
            parent->split_pct = old_split_pct;
        else
            gui_window_refresh_windows ();
    }
}

/*
 * gui_window_merge: merge window with its sister
 */

int
gui_window_merge (t_gui_window *window)
{
    t_gui_window_tree *parent, *sister;
    
    parent = window->ptr_tree->parent_node;
    if (parent)
    {
        sister = (parent->child1->window == window) ?
            parent->child2 : parent->child1;
        
        if (!(sister->window))
            return 0;
        
        if (window->win_y == sister->window->win_y)
        {
            /* horizontal merge */
            window->win_width += sister->window->win_width + 1;
            window->win_width_pct += sister->window->win_width_pct;
        }
        else
        {
            /* vertical merge */
            window->win_height += sister->window->win_height;
            window->win_height_pct += sister->window->win_height_pct;
        }
        if (sister->window->win_x < window->win_x)
            window->win_x = sister->window->win_x;
        if (sister->window->win_y < window->win_y)
            window->win_y = sister->window->win_y;
        
        gui_window_free (sister->window);
        gui_window_tree_node_to_leaf (parent, window);
        
        gui_window_switch_to_buffer (window, window->buffer);
        gui_window_redraw_buffer (window->buffer);
        return 1;
    }
    return 0;
}

/*
 * gui_window_merge_all: merge all windows into only one
 */

void
gui_window_merge_all (t_gui_window *window)
{
    while (gui_windows->next_window)
    {
        gui_window_free ((gui_windows == window) ? gui_windows->next_window : gui_windows);
    }
    gui_window_tree_free (&gui_windows_tree);
    gui_window_tree_init (window);
    window->ptr_tree = gui_windows_tree;
    window->win_x = 0;
    window->win_y = 0;
    window->win_width = COLS;
    window->win_height = LINES;
    window->win_width_pct = 100;
    window->win_height_pct = 100;
    gui_window_switch_to_buffer (window, window->buffer);
    gui_window_redraw_buffer (window->buffer);
}

/*
 * gui_window_side_by_side: return a code about position of 2 windows:
 *                          0 = they're not side by side
 *                          1 = side by side (win2 is over the win1)
 *                          2 = side by side (win2 on the right)
 *                          3 = side by side (win2 below win1)
 *                          4 = side by side (win2 on the left)
 */

int
gui_window_side_by_side (t_gui_window *win1, t_gui_window *win2)
{
    /* win2 over win1 ? */
    if (win2->win_y + win2->win_height == win1->win_y)
    {
        if (win2->win_x >= win1->win_x + win1->win_width)
            return 0;
        if (win2->win_x + win2->win_width <= win1->win_x)
            return 0;
        return 1;
    }

    /* win2 on the right ? */
    if (win2->win_x == win1->win_x + win1->win_width + 1)
    {
        if (win2->win_y >= win1->win_y + win1->win_height)
            return 0;
        if (win2->win_y + win2->win_height <= win1->win_y)
            return 0;
        return 2;
    }

    /* win2 below win1 ? */
    if (win2->win_y == win1->win_y + win1->win_height)
    {
        if (win2->win_x >= win1->win_x + win1->win_width)
            return 0;
        if (win2->win_x + win2->win_width <= win1->win_x)
            return 0;
        return 3;
    }
    
    /* win2 on the left ? */
    if (win2->win_x + win2->win_width + 1 == win1->win_x)
    {
        if (win2->win_y >= win1->win_y + win1->win_height)
            return 0;
        if (win2->win_y + win2->win_height <= win1->win_y)
            return 0;
        return 4;
    }

    return 0;
}

/*
 * gui_window_switch_up: search and switch to a window over current window
 */

void
gui_window_switch_up (t_gui_window *window)
{
    t_gui_window *ptr_win;
    
    for (ptr_win = gui_windows; ptr_win;
         ptr_win = ptr_win->next_window)
    {
        if ((ptr_win != window) &&
            (gui_window_side_by_side (window, ptr_win) == 1))
        {
            gui_current_window = ptr_win;
            gui_window_switch_to_buffer (gui_current_window, gui_current_window->buffer);
            gui_window_redraw_buffer (gui_current_window->buffer);
            return;
        }
    }
}

/*
 * gui_window_switch_down: search and switch to a window below current window
 */

void
gui_window_switch_down (t_gui_window *window)
{
    t_gui_window *ptr_win;
    
    for (ptr_win = gui_windows; ptr_win;
         ptr_win = ptr_win->next_window)
    {
        if ((ptr_win != window) &&
            (gui_window_side_by_side (window, ptr_win) == 3))
        {
            gui_current_window = ptr_win;
            gui_window_switch_to_buffer (gui_current_window, gui_current_window->buffer);
            gui_window_redraw_buffer (gui_current_window->buffer);
            return;
        }
    }
}

/*
 * gui_window_switch_left: search and switch to a window on the left of current window
 */

void
gui_window_switch_left (t_gui_window *window)
{
    t_gui_window *ptr_win;
    
    for (ptr_win = gui_windows; ptr_win;
         ptr_win = ptr_win->next_window)
    {
        if ((ptr_win != window) &&
            (gui_window_side_by_side (window, ptr_win) == 4))
        {
            gui_current_window = ptr_win;
            gui_window_switch_to_buffer (gui_current_window, gui_current_window->buffer);
            gui_window_redraw_buffer (gui_current_window->buffer);
            return;
        }
    }
}

/*
 * gui_window_switch_right: search and switch to a window on the right of current window
 */

void
gui_window_switch_right (t_gui_window *window)
{
    t_gui_window *ptr_win;
    
    for (ptr_win = gui_windows; ptr_win;
         ptr_win = ptr_win->next_window)
    {
        if ((ptr_win != window) &&
            (gui_window_side_by_side (window, ptr_win) == 2))
        {
            gui_current_window = ptr_win;
            gui_window_switch_to_buffer (gui_current_window, gui_current_window->buffer);
            gui_window_redraw_buffer (gui_current_window->buffer);
            return;
        }
    }
}

/*
 * gui_window_refresh_screen: called when term size is modified
 */

void
gui_window_refresh_screen ()
{
    int new_height, new_width;
    
    endwin ();
    refresh ();
    
    getmaxyx (stdscr, new_height, new_width);
    
    gui_ok = ((new_width > WINDOW_MIN_WIDTH) && (new_height > WINDOW_MIN_HEIGHT));
    
    if (gui_ok)
        gui_window_refresh_windows ();
}

/*
 * gui_window_refresh_screen_sigwinch: called when signal SIGWINCH is received
 */

void
gui_window_refresh_screen_sigwinch ()
{
    gui_window_refresh_screen ();
    signal (SIGWINCH, gui_window_refresh_screen_sigwinch);
}

/*
 * gui_window_set_title: set terminal title
 */

void
gui_window_set_title ()
{
    char *envterm = getenv ("TERM");
    
    if (envterm)
    {
	if (strcmp( envterm, "sun-cmd") == 0)
	    printf ("\033]l%s %s\033\\", PACKAGE_NAME, PACKAGE_VERSION);
	else if (strcmp(envterm, "hpterm") == 0)
	    printf ("\033&f0k%dD%s %s", strlen(PACKAGE_NAME) + 
		    strlen(PACKAGE_VERSION) + 1,
		    PACKAGE_NAME, PACKAGE_VERSION);
	/* the following term supports the xterm excapes */
	else if (strncmp (envterm, "xterm", 5) == 0
		 || strncmp (envterm, "rxvt", 4) == 0
		 || strcmp (envterm, "Eterm") == 0
		 || strcmp (envterm, "aixterm") == 0
		 || strcmp (envterm, "iris-ansi") == 0
		 || strcmp (envterm, "dtterm") == 0)
	    printf ("\33]0;%s %s\7", PACKAGE_NAME, PACKAGE_VERSION);
	else if (strcmp (envterm, "screen") == 0)
	{
	    printf ("\033k%s %s\033\\", PACKAGE_NAME, PACKAGE_VERSION);
	    /* tryning to set the title of a backgrounded xterm like terminal */
	    printf ("\33]0;%s %s\7", PACKAGE_NAME, PACKAGE_VERSION);
	}
    }
}

/*
 * gui_window_reset_title: reset terminal title
 */

void
gui_window_reset_title ()
{
    char *envterm = getenv ("TERM");
    char *envshell = getenv ("SHELL");

    if (envterm)
    {
	if (strcmp( envterm, "sun-cmd") == 0)
	    printf ("\033]l%s\033\\", "Terminal");
	else if (strcmp( envterm, "hpterm") == 0)
	    printf ("\033&f0k%dD%s", strlen("Terminal"), "Terminal");
	/* the following term supports the xterm excapes */
	else if (strncmp (envterm, "xterm", 5) == 0
		 || strncmp (envterm, "rxvt", 4) == 0
		 || strcmp (envterm, "Eterm") == 0
		 || strcmp( envterm, "aixterm") == 0
		 || strcmp( envterm, "iris-ansi") == 0
		 || strcmp( envterm, "dtterm") == 0)
	    printf ("\33]0;%s\7", "Terminal");
	else if (strcmp (envterm, "screen") == 0)
	{
	    char *shell, *shellname;
	    if (envshell)
	    {
		shell  = strdup (envterm);
		shellname = basename(shell);
		if (shell)
		{
		    printf ("\033k%s\033\\", shellname);
		    free (shell);
		}
		else
		    printf ("\033k%s\033\\", envterm);
	    }
	    else
		printf ("\033k%s\033\\", envterm);
	    /* tryning to reset the title of a backgrounded xterm like terminal */
	    printf ("\33]0;%s\7", "Terminal");
	}
    }
}
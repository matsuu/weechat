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

/* gui-curses-main.c: main loop for Curses GUI */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>

#include "../../common/weechat.h"
#include "../gui.h"
#include "../../common/fifo.h"
#include "../../common/utf8.h"
#include "../../common/weeconfig.h"
#include "gui-curses.h"

#ifdef PLUGINS
#include "../../plugins/plugins.h"
#endif


/*
 * gui_main_loop: main loop for WeeChat with ncurses GUI
 */

void
gui_main_loop ()
{
    fd_set read_fd;
    static struct timeval timeout, tv;
    static struct timezone tz;
    t_irc_server *ptr_server;
    t_gui_buffer *ptr_buffer;
    int old_day, old_min, old_sec, diff;
    char text_time[1024];
    time_t new_time;
    struct tm *local_time;
    
    quit_weechat = 0;

    new_time = time (NULL);
    gui_last_activity_time = new_time;
    local_time = localtime (&new_time);
    old_day = local_time->tm_mday;
    
    old_min = -1;
    old_sec = -1;
    check_away = 0;
    while (!quit_weechat)
    {
        /* refresh needed ? */
        if (gui_refresh_screen_needed)
            gui_window_refresh_screen ();
        
        new_time = time (NULL);
        local_time = localtime (&new_time);
        
        /* minute has changed ? => redraw infobar */
        if (local_time->tm_min != old_min)
        {
            old_min = local_time->tm_min;
            gui_infobar_draw (gui_current_window->buffer, 1);
            
            if (cfg_look_day_change
                && (local_time->tm_mday != old_day))
            {
                strftime (text_time, sizeof (text_time),
                          cfg_look_day_change_timestamp, local_time);
                gui_add_hotlist = 0;
                for (ptr_buffer = gui_buffers; ptr_buffer;
                     ptr_buffer = ptr_buffer->next_buffer)
                {
                    if (ptr_buffer->type == BUFFER_TYPE_STANDARD)
                        gui_printf_nolog_notime (ptr_buffer,
                                                 _("Day changed to %s\n"),
                                                 text_time);
                }
                gui_add_hotlist = 1;
            }
            old_day = local_time->tm_mday;
        }
        
        /* second has changed ? */
        if (local_time->tm_sec != old_sec)
        {
            old_sec = local_time->tm_sec;
            
            if (cfg_look_infobar_seconds)
            {
                gui_infobar_draw_time (gui_current_window->buffer);
                wmove (GUI_CURSES(gui_current_window)->win_input,
                       0, gui_current_window->win_input_x);
                wrefresh (GUI_CURSES(gui_current_window)->win_input);
            }
            
            /* infobar count down */
            if (gui_infobar && gui_infobar->remaining_time > 0)
            {
                gui_infobar->remaining_time--;
                if (gui_infobar->remaining_time == 0)
                {
                    gui_infobar_remove ();
                    gui_infobar_draw (gui_current_window->buffer, 1);
                }
            }
            
            /* away check */
            if (cfg_irc_away_check != 0)
            {
                check_away++;
                if (check_away >= (cfg_irc_away_check * 60))
                {
                    check_away = 0;
                    server_check_away ();
                }
            }

#ifdef PLUGINS            
            /* call timer handlers */
            plugin_timer_handler_exec ();
#endif
        }
        
        /* read keyboard */
        
        FD_ZERO (&read_fd);
        timeout.tv_sec = 0;
        timeout.tv_usec = 8000;
        
        FD_SET (STDIN_FILENO, &read_fd);
        
        if (select (FD_SETSIZE, &read_fd, NULL, NULL, &timeout) > 0)
        {
            if (FD_ISSET (STDIN_FILENO, &read_fd))
            {
                gui_keyboard_read ();
            }
        }
        
        /* read sockets (servers, child process when connecting, FIFO pipe) */
        
        FD_ZERO (&read_fd);
        
        timeout.tv_sec = 0;
        timeout.tv_usec = 2000;
        
        if (weechat_fifo != -1)
            FD_SET (weechat_fifo, &read_fd);
        
        for (ptr_server = irc_servers; ptr_server;
             ptr_server = ptr_server->next_server)
        {
            /* check if reconnection is pending */
            if ((!ptr_server->is_connected)
                && (ptr_server->reconnect_start > 0)
                && (new_time >= (ptr_server->reconnect_start + ptr_server->autoreconnect_delay)))
                server_reconnect (ptr_server);
            else
            {
                if (ptr_server->is_connected)
                {
                    /* check for lag */
                    if ((ptr_server->lag_check_time.tv_sec == 0)
                        && (new_time >= ptr_server->lag_next_check))
                    {
                        server_sendf (ptr_server, "PING %s\r\n", ptr_server->address);
                        gettimeofday (&(ptr_server->lag_check_time), &tz);
                    }
                    
                    /* lag timeout => disconnect */
                    if ((ptr_server->lag_check_time.tv_sec != 0)
                        && (cfg_irc_lag_disconnect > 0))
                    {
                        gettimeofday (&tv, &tz);
                        diff = (int) get_timeval_diff (&(ptr_server->lag_check_time), &tv);
                        if (diff / 1000 > cfg_irc_lag_disconnect * 60)
                        {
                            irc_display_prefix (ptr_server, ptr_server->buffer, PREFIX_ERROR);
                            gui_printf (ptr_server->buffer,
                                        _("%s lag is high, disconnecting from server...\n"),
                                        WEECHAT_WARNING);
                            server_disconnect (ptr_server, 1);
                            continue;
                        }
                    }
                }
            
                if (!ptr_server->is_connected && (ptr_server->child_pid > 0))
                    FD_SET (ptr_server->child_read, &read_fd);
                else
                {
                    if (ptr_server->sock >= 0)
                        FD_SET (ptr_server->sock, &read_fd);
                }
            }
        }
        
        if (select (FD_SETSIZE, &read_fd, NULL, NULL, &timeout) > 0)
        {
            if ((weechat_fifo != -1) && (FD_ISSET (weechat_fifo, &read_fd)))
            {
                fifo_read ();
            }
            for (ptr_server = irc_servers; ptr_server;
                 ptr_server = ptr_server->next_server)
            {
                if (!ptr_server->is_connected && (ptr_server->child_pid > 0))
                {
                    if (FD_ISSET (ptr_server->child_read, &read_fd))
                        server_child_read (ptr_server);
                }
                else
                {
                    if ((ptr_server->sock >= 0) &&
                        (FD_ISSET (ptr_server->sock, &read_fd)))
                        server_recv (ptr_server);
                }
            }
        }
        
        /* manages active DCC */
        dcc_handle ();
    }
}

/*
 * gui_main_pre_init: pre-initialize GUI (called before gui_init)
 */

void
gui_main_pre_init (int *argc, char **argv[])
{
    /* nothing for Curses interface */
    (void) argc;
    (void) argv;
}

/*
 * gui_main_init: init GUI
 */

void
gui_main_init ()
{
    initscr ();
    
    curs_set (1);
    noecho ();
    nodelay (stdscr, TRUE);

    gui_color_init ();
    
    gui_infobar = NULL;
    
    gui_ok = ((COLS > 5) && (LINES > 5));

    refresh ();
    
    /* init clipboard buffer */
    gui_input_clipboard = NULL;

    /* create new window/buffer */
    if (gui_window_new (NULL, 0, 0, COLS, LINES, 100, 100))
    {
        gui_current_window = gui_windows;
        gui_buffer_new (gui_windows, NULL, NULL, BUFFER_TYPE_STANDARD, 1);
        
        if (cfg_look_set_title)
            gui_window_set_title ();
        
        gui_init_ok = 1;
        
        signal (SIGWINCH, gui_window_refresh_screen_sigwinch);
    }
}

/*
 * gui_main_end: GUI end
 */

void
gui_main_end ()
{
    /* free clipboard buffer */
    if (gui_input_clipboard)
        free (gui_input_clipboard);

    /* delete all panels */
    while (gui_panels)
        gui_panel_free (gui_panels);

    /* delete all buffers */
    gui_window_merge_all (gui_current_window);
    while (gui_buffers)
      gui_buffer_free (gui_buffers, 0);
    
    /* delete all windows */
    while (gui_windows)
        gui_window_free (gui_windows);
    gui_window_tree_free (&gui_windows_tree);
    
    /* delete global history */
    history_global_free ();
    
    /* delete infobar messages */
    while (gui_infobar)
        gui_infobar_remove ();

    /* reset title */
    if (cfg_look_set_title)
	gui_window_reset_title ();
    
    /* end of Curses output */
    refresh ();
    endwin ();
}
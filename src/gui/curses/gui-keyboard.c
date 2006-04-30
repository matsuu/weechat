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

/* gui-keyboard.c: keyboard functions for Curses GUI */


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
#include "../../common/utf8.h"

#ifdef PLUGINS
#include "../../plugins/plugins.h"
#endif


/*
 * gui_keyboard__default_bindings: create default key bindings
 */

void
gui_keyboard_default_bindings ()
{
    int i;
    char key_str[32], command[32];
    
    /* keys binded with internal functions */
    gui_key_bind ( /* RC          */ "ctrl-M",             "return");
    gui_key_bind ( /* RC          */ "ctrl-J",             "return");
    gui_key_bind ( /* tab         */ "ctrl-I",             "tab");
    gui_key_bind ( /* basckp      */ "ctrl-H",             "backspace");
    gui_key_bind ( /* basckp      */ "ctrl-?",             "backspace");
    gui_key_bind ( /* del         */ "meta2-3~",           "delete");
    gui_key_bind ( /* ^K          */ "ctrl-K",             "delete_end_line");
    gui_key_bind ( /* ^U          */ "ctrl-U",             "delete_beginning_line");
    gui_key_bind ( /* ^W          */ "ctrl-W",             "delete_previous_word");
    gui_key_bind ( /* ^Y          */ "ctrl-Y",             "clipboard_paste");
    gui_key_bind ( /* ^T          */ "ctrl-T",             "transpose_chars");
    gui_key_bind ( /* home        */ "meta2-1~",           "home");
    gui_key_bind ( /* home        */ "meta2-H",            "home");
    gui_key_bind ( /* home        */ "meta2-7~",           "home");
    gui_key_bind ( /* ^A          */ "ctrl-A",             "home");
    gui_key_bind ( /* end         */ "meta2-4~",           "end");
    gui_key_bind ( /* end         */ "meta2-F",            "end");
    gui_key_bind ( /* end         */ "meta2-8~",           "end");
    gui_key_bind ( /* ^E          */ "ctrl-E",             "end");
    gui_key_bind ( /* left        */ "meta2-D",            "left");
    gui_key_bind ( /* right       */ "meta2-C",            "right");
    gui_key_bind ( /* up          */ "meta2-A",            "up");
    gui_key_bind ( /* ^up         */ "meta-Oa",            "up_global");
    gui_key_bind ( /* down        */ "meta2-B",            "down");
    gui_key_bind ( /* ^down       */ "meta-Ob",            "down_global");
    gui_key_bind ( /* pgup        */ "meta2-5~",           "page_up");
    gui_key_bind ( /* pgdn        */ "meta2-6~",           "page_down");
    gui_key_bind ( /* m-pgup      */ "meta-meta2-5~",      "scroll_up");
    gui_key_bind ( /* m-pgdn      */ "meta-meta2-6~",      "scroll_down");
    gui_key_bind ( /* m-home      */ "meta-meta2-1~",      "scroll_top");
    gui_key_bind ( /* m-end       */ "meta-meta2-4~",      "scroll_bottom");
    gui_key_bind ( /* F10         */ "meta2-21~",          "infobar_clear");
    gui_key_bind ( /* F11         */ "meta2-23~",          "nick_page_up");
    gui_key_bind ( /* F12         */ "meta2-24~",          "nick_page_down");
    gui_key_bind ( /* m-F11       */ "meta-meta2-23~",     "nick_beginning");
    gui_key_bind ( /* m-F12       */ "meta-meta2-24~",     "nick_end");
    gui_key_bind ( /* ^L          */ "ctrl-L",             "refresh");
    gui_key_bind ( /* m-a         */ "meta-a",             "jump_smart");
    gui_key_bind ( /* m-b         */ "meta-b",             "previous_word");
    gui_key_bind ( /* ^left       */ "meta-Od",            "previous_word");
    gui_key_bind ( /* m-d         */ "meta-d",             "delete_next_word");
    gui_key_bind ( /* m-f         */ "meta-f",             "next_word");
    gui_key_bind ( /* ^right      */ "meta-Oc",            "next_word");
    gui_key_bind ( /* m-h         */ "meta-h",             "hotlist_clear");
    gui_key_bind ( /* m-j,m-d     */ "meta-jmeta-d",       "jump_dcc");
    gui_key_bind ( /* m-j,m-l     */ "meta-jmeta-l",       "jump_last_buffer");
    gui_key_bind ( /* m-j,m-s     */ "meta-jmeta-s",       "jump_server");
    gui_key_bind ( /* m-j,m-x     */ "meta-jmeta-x",       "jump_next_server");
    gui_key_bind ( /* m-j,m-r     */ "meta-jmeta-r",       "jump_raw_data");
    gui_key_bind ( /* m-k         */ "meta-k",             "grab_key");
    gui_key_bind ( /* m-n         */ "meta-n",             "scroll_next_highlight");
    gui_key_bind ( /* m-p         */ "meta-p",             "scroll_previous_highlight");
    gui_key_bind ( /* m-r         */ "meta-r",             "delete_line");
    gui_key_bind ( /* m-s         */ "meta-s",             "switch_server");
    gui_key_bind ( /* m-u         */ "meta-u",             "scroll_unread");
    
    /* keys binded with commands */
    gui_key_bind ( /* m-left      */ "meta-meta2-D",       "/buffer -1");
    gui_key_bind ( /* F5          */ "meta2-15~",          "/buffer -1");
    gui_key_bind ( /* m-right     */ "meta-meta2-C",       "/buffer +1");
    gui_key_bind ( /* F6          */ "meta2-17~",          "/buffer +1");
    gui_key_bind ( /* F7          */ "meta2-18~",          "/window -1");
    gui_key_bind ( /* F8          */ "meta2-19~",          "/window +1");
    gui_key_bind ( /* m-w,m-up    */ "meta-wmeta-meta2-A", "/window up");
    gui_key_bind ( /* m-w,m-down  */ "meta-wmeta-meta2-B", "/window down");
    gui_key_bind ( /* m-w,m-left  */ "meta-wmeta-meta2-D", "/window left");
    gui_key_bind ( /* m-w,m-right */ "meta-wmeta-meta2-C", "/window right");
    gui_key_bind ( /* m-0         */ "meta-0",             "/buffer 10");
    gui_key_bind ( /* m-1         */ "meta-1",             "/buffer 1");
    gui_key_bind ( /* m-2         */ "meta-2",             "/buffer 2");
    gui_key_bind ( /* m-3         */ "meta-3",             "/buffer 3");
    gui_key_bind ( /* m-4         */ "meta-4",             "/buffer 4");
    gui_key_bind ( /* m-5         */ "meta-5",             "/buffer 5");
    gui_key_bind ( /* m-6         */ "meta-6",             "/buffer 6");
    gui_key_bind ( /* m-7         */ "meta-7",             "/buffer 7");
    gui_key_bind ( /* m-8         */ "meta-8",             "/buffer 8");
    gui_key_bind ( /* m-9         */ "meta-9",             "/buffer 9");
    
    /* bind meta-j + {01..99} to switch to buffers # > 10 */
    for (i = 1; i < 100; i++)
    {
        sprintf (key_str, "meta-j%02d", i);
        sprintf (command, "/buffer %d", i);
        gui_key_bind (key_str, command);
    }
}

/*
 * gui_keyboard_grab_end: insert grabbed key in input buffer
 */

void
gui_keyboard_grab_end ()
{
    char *expanded_key, *expanded_key2;
    int length;
    char *buffer_before_key;

    /* get expanded name (for example: ^U => ctrl-u) */
    expanded_key = gui_key_get_expanded_name (gui_key_buffer);
    
    if (expanded_key)
    {
        if (gui_current_window->buffer->has_input)
        {
            buffer_before_key =
                (gui_current_window->buffer->input_buffer) ?
                strdup (gui_current_window->buffer->input_buffer) : strdup ("");
            gui_insert_string_input (gui_current_window, expanded_key, -1);
            gui_current_window->buffer->input_buffer_pos += utf8_strlen (expanded_key);
            gui_input_draw (gui_current_window->buffer, 1);
            gui_current_window->buffer->completion.position = -1;
#ifdef PLUGINS
            length = strlen (expanded_key) + 1 + 1;
            expanded_key2 = (char *) malloc (length);
            if (expanded_key2)
            {
                snprintf (expanded_key2, length, "*%s", expanded_key);
                (void) plugin_keyboard_handler_exec (expanded_key2,
                                                     buffer_before_key,
                                                     gui_current_window->buffer->input_buffer);
                free (expanded_key2);
            }
#endif
            if (buffer_before_key)
                free (buffer_before_key);
        }
        free (expanded_key);
    }
    
    /* end grab mode */
    gui_key_grab = 0;
    gui_key_grab_count = 0;
    gui_key_buffer[0] = '\0';
}

/*
 * gui_keyboard_read: read keyboard chars
 */

void
gui_keyboard_read ()
{
    int key, i, insert_ok;
    char key_str[32], key_str2[33];
    char *buffer_before_key;
    
    i = 0;
    /* do not loop too much here (for example when big paste was made),
       to read also socket & co */
    while (i < 8)
    {
        if (gui_key_grab && (gui_key_grab_count > 10))
            gui_keyboard_grab_end ();
        
        key = getch ();
        insert_ok = 1;
        
        if (key == ERR)
        {
            if (gui_key_grab && (gui_key_grab_count > 0))
                gui_keyboard_grab_end ();
            break;
        }
        
        if (key == KEY_RESIZE)
        {
            gui_window_refresh_screen ();
            continue;
        }
        
        gui_last_activity_time = time (NULL);
                
        if (key < 32)
        {
            insert_ok = 0;
            key_str[0] = '^';
            key_str[1] = (char) key + '@';
            key_str[2] = '\0';
        }
        else if (key == 127)
        {
            key_str[0] = '^';
            key_str[1] = '?';
            key_str[2] = '\0';
        }
        else
        {
            if (local_utf8)
            {
                /* 1 char: 0vvvvvvv */
                if (key < 0x80)
                {
                    key_str[0] = (char) key;
                    key_str[1] = '\0';
                }
                /* 2 chars: 110vvvvv 10vvvvvv */
                else if ((key & 0xE0) == 0xC0)
                {
                    key_str[0] = (char) key;
                    key_str[1] = (char) (getch ());
                    key_str[2] = '\0';
                }
                 /* 3 chars: 1110vvvv 10vvvvvv 10vvvvvv */
                else if ((key & 0xF0) == 0xE0)
                {
                    key_str[0] = (char) key;
                    key_str[1] = (char) (getch ());
                    key_str[2] = (char) (getch ());
                    key_str[3] = '\0';
                }
                /* 4 chars: 11110vvv 10vvvvvv 10vvvvvv 10vvvvvv */
                else if ((key & 0xF8) == 0xF0)
                {
                    key_str[0] = (char) key;
                    key_str[1] = (char) (getch ());
                    key_str[2] = (char) (getch ());
                    key_str[3] = (char) (getch ());
                    key_str[4] = '\0';
                }
            }
            else
            {
                key_str[0] = (char) key;
                key_str[1] = '\0';
            }
        }
        
        if (strcmp (key_str, "^") == 0)
        {
            key_str[1] = '^';
            key_str[2] = '\0';
        }
        
        /*gui_printf (gui_current_window->buffer, "gui_input_read: key = %s (%d)\n", key_str, key);*/
        
        if ((gui_key_pressed (key_str) != 0) && (insert_ok))
        {
            if (strcmp (key_str, "^^") == 0)
                key_str[1] = '\0';
            
            switch (gui_current_window->buffer->type)
            {
                case BUFFER_TYPE_STANDARD:
                    buffer_before_key =
                        (gui_current_window->buffer->input_buffer) ?
                        strdup (gui_current_window->buffer->input_buffer) : strdup ("");
                    gui_insert_string_input (gui_current_window, key_str, -1);
                    gui_current_window->buffer->input_buffer_pos += utf8_strlen (key_str);
                    gui_input_draw (gui_current_window->buffer, 0);
                    gui_current_window->buffer->completion.position = -1;
#ifdef PLUGINS
                    snprintf (key_str2, sizeof (key_str2), "*%s", key_str);
                    (void) plugin_keyboard_handler_exec (key_str2,
                                                         buffer_before_key,
                                                         gui_current_window->buffer->input_buffer);
#endif
                    break;
                case BUFFER_TYPE_DCC:
                    gui_exec_action_dcc (gui_current_window, key_str);
                    break;
                case BUFFER_TYPE_RAW_DATA:
                    gui_exec_action_raw_data (gui_current_window, key_str);
                    break;
            }
        }
        i++;
    }
}
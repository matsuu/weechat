/*
 * Copyright (c) 2003-2005 by FlashCode <flashcode@flashtux.org>
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/* gui-keyboard: keyboard functions (GUI independant) */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "../common/weechat.h"
#include "gui.h"
#include "../common/command.h"


t_gui_key *gui_keys = NULL;
t_gui_key *last_gui_key = NULL;

char gui_key_buffer[128];
int gui_key_grab = 0;
int gui_key_grab_count = 0;

t_gui_key_function gui_key_functions[] =
{ { "return",                gui_input_return,
    N_("terminate line") },
  { "tab",                   gui_input_tab,
    N_("complete word") },
  { "backspace",             gui_input_backspace,
    N_("delete previous char") },
  { "delete",                gui_input_delete,
    N_("delete next char") },
  { "delete_end_line",       gui_input_delete_end_of_line,
    N_("delete until end of line") },
  { "delete_beginning_line", gui_input_delete_begin_of_line,
    N_("delete until beginning of line") },
  { "delete_line",           gui_input_delete_line,
    N_("delete entire line") },
  { "delete_previous_word",  gui_input_delete_previous_word,
    N_("delete previous word") },
  { "delete_next_word",      gui_input_delete_next_word,
    N_("delete next word") },
  { "clipboard_paste",       gui_input_clipboard_paste,
    N_("paste current clipboard content") },
  { "transpose_chars",       gui_input_transpose_chars,
    N_("transpose chars") },
  { "home",                  gui_input_home,
    N_("go to beginning of line") },
  { "end",                   gui_input_end,
    N_("go to end of line") },
  { "left",                  gui_input_left,
    N_("move one char left") },
  { "previous_word",         gui_input_previous_word,
    N_("move to previous word") },
  { "right",                 gui_input_right,
    N_("move one char right") },
  { "next_word",             gui_input_next_word,
    N_("move to next word") },
  { "up",                    gui_input_up,
    N_("call previous command in history") },
  { "down",                  gui_input_down,
    N_("call next command in history") },
  { "page_up",               gui_input_page_up,
    N_("scroll one page up") },
  { "page_down",             gui_input_page_down,
    N_("scroll one page down") },
  { "infobar_clear",         gui_input_infobar_clear,
    N_("clear infobar") },
  { "nick_page_up",          gui_input_nick_page_up,
    N_("scroll nicklist one page up") },
  { "nick_page_down",        gui_input_nick_page_down,
    N_("scroll nicklist one page down") },
  { "nick_beginning",        gui_input_nick_beginning,
    N_("display beginning of nicklist") },
  { "nick_end",              gui_input_nick_end,
    N_("display end of nicklist") },
  { "refresh",               gui_curses_resize_handler,
    N_("refresh screen") },
  { "jump_smart",            gui_input_jump_smart,
    N_("jump to buffer with activity") },
  { "jump_dcc",              gui_input_jump_dcc,
    N_("jump to DCC buffer") },
  { "jump_last_buffer",      gui_input_jump_last_buffer,
    N_("jump to last buffer") },
  { "jump_server",           gui_input_jump_server,
    N_("jump to server buffer") },
  { "jump_next_server",      gui_input_jump_next_server,
    N_("jump to next server") },
  { "hotlist_clear",         gui_input_hotlist_clear,
    N_("clear hotlist") },
  { "grab_key",              gui_input_grab_key,
    N_("grab a key") },
  { NULL, NULL, NULL }
};


/*
 * gui_key_init: init keyboard (create default key bindings)
 */

void
gui_key_init ()
{
    gui_key_buffer[0] = '\0';
    gui_key_grab = 0;
    gui_key_grab_count = 0;
    
    gui_input_default_key_bindings ();
}

/*
 * gui_key_init_show: init "show mode"
 */

void
gui_key_init_grab ()
{
    gui_key_grab = 1;
    gui_key_grab_count = 0;
}

/*
 * gui_key_get_internal_code: get internal code from user key name
 *                            for example: return "^R" for "ctrl-R"
 */

char *
gui_key_get_internal_code (char *key)
{
    char *result;
    
    if ((result = (char *) malloc (strlen (key) + 1)))
    {
        result[0] = '\0';
        while (key[0])
        {
            if (strncasecmp (key, "meta2-", 6) == 0)
            {
                strcat (result, "^[[");
                key += 6;
            }
            if (strncasecmp (key, "meta-", 5) == 0)
            {
                strcat (result, "^[");
                key += 5;
            }
            else if (strncasecmp (key, "ctrl-", 5) == 0)
            {
                strcat (result, "^");
                key += 5;
            }
            else
            {
                strncat (result, key, 1);
                key++;
            }
        }
    }
    else
        return NULL;

    return result;
}

/*
 * gui_key_get_expanded_name: get expanded name from internal key code
 *                            for example: return "ctrl-R" for "^R"
 */

char *
gui_key_get_expanded_name (char *key)
{
    char *result;
    
    if ((result = (char *) malloc ((strlen (key) * 5) + 1)))
    {
        result[0] = '\0';
        while (key[0])
        {
            if (strncasecmp (key, "^[[", 3) == 0)
            {
                strcat (result, "meta2-");
                key += 3;
            }
            if (strncasecmp (key, "^[", 2) == 0)
            {
                strcat (result, "meta-");
                key += 2;
            }
            else if ((key[0] == '^') && (key[1]))
            {
                strcat (result, "ctrl-");
                key++;
            }
            else
            {
                strncat (result, key, 1);
                key++;
            }
        }
    }
    else
        return NULL;

    return result;
}

/*
 * gui_key_find_pos: find position for a key (for sorting keys list)
 */

t_gui_key *
gui_key_find_pos (t_gui_key *key)
{
    t_gui_key *ptr_key;
    
    for (ptr_key = gui_keys; ptr_key; ptr_key = ptr_key->next_key)
    {
        if (strcasecmp (key->key, ptr_key->key) < 0)
            return ptr_key;
    }
    return NULL;
}

/*
 * gui_key_insert_sorted: insert key into sorted list
 */

void
gui_key_insert_sorted (t_gui_key *key)
{
    t_gui_key *pos_key;
    
    if (gui_keys)
    {
        pos_key = gui_key_find_pos (key);
        
        if (pos_key)
        {
            /* insert key into the list (before key found) */
            key->prev_key = pos_key->prev_key;
            key->next_key = pos_key;
            if (pos_key->prev_key)
                pos_key->prev_key->next_key = key;
            else
                gui_keys = key;
            pos_key->prev_key = key;
        }
        else
        {
            /* add key to the end */
            key->prev_key = last_gui_key;
            key->next_key = NULL;
            last_gui_key->next_key = key;
            last_gui_key = key;
        }
    }
    else
    {
        key->prev_key = NULL;
        key->next_key = NULL;
        gui_keys = key;
        last_gui_key = key;
    }
}

/*
 * gui_key_new: add a new key in keys list
 */

t_gui_key *
gui_key_new (char *key, char *command, void *function)
{
    t_gui_key *new_key;
    char *internal_code;
    
    if ((new_key = (t_gui_key *) malloc (sizeof (t_gui_key))))
    {
        internal_code = gui_key_get_internal_code (key);
        new_key->key = (internal_code) ? strdup (internal_code) : strdup (key);
        if (internal_code)
            free (internal_code);
        new_key->command = (command) ? strdup (command) : NULL;
        new_key->function = function;
        gui_key_insert_sorted (new_key);
    }
    else
        return NULL;
    
    return new_key;
}

/*
 * gui_key_search: search a key
 */

t_gui_key *
gui_key_search (char *key)
{
    t_gui_key *ptr_key;

    for (ptr_key = gui_keys; ptr_key; ptr_key = ptr_key->next_key)
    {
        if (strcasecmp (ptr_key->key, key) == 0)
            return ptr_key;
    }
    
    /* key not found */
    return NULL;
}

/*
 * gui_key_cmp: compares 2 keys
 */

int
gui_key_cmp (char *key, char *search)
{
    while (search[0])
    {
        if (toupper(key[0]) != toupper(search[0]))
            return search[0] - key[0];
        key++;
        search++;
    }
    
    return 0;
}

/*
 * gui_key_search_part: search a key (maybe part of string)
 */

t_gui_key *
gui_key_search_part (char *key)
{
    t_gui_key *ptr_key;
    
    for (ptr_key = gui_keys; ptr_key; ptr_key = ptr_key->next_key)
    {
        if (gui_key_cmp (ptr_key->key, key) == 0)
            return ptr_key;
    }
    
    /* key not found */
    return NULL;
}

/*
 * gui_key_function_search_by_name: search a function by name
 */

void *
gui_key_function_search_by_name (char *name)
{
    int i;
    
    i = 0;
    while (gui_key_functions[i].function_name)
    {
        if (strcasecmp (gui_key_functions[i].function_name, name) == 0)
            return gui_key_functions[i].function;
        i++;
    }

    /* function not found */
    return NULL;
}

/*
 * gui_key_function_search_by_ptr: search a function by pointer
 */

char *
gui_key_function_search_by_ptr (void *function)
{
    int i;
    
    i = 0;
    while (gui_key_functions[i].function_name)
    {
        if (gui_key_functions[i].function == function)
            return gui_key_functions[i].function_name;
        i++;
    }

    /* function not found */
    return NULL;
}

/*
 * gui_key_bind: bind a key to a function (command or special function)
 */

t_gui_key *
gui_key_bind (char *key, char *command)
{
    t_gui_key_function *ptr_function;
    t_gui_key *new_key;
    
    if (!key || !command)
    {
        wee_log_printf (_("%s unable to bind key \"%s\"\n"),
                        WEECHAT_ERROR, key);
        return NULL;
    }
    
    ptr_function = NULL;
    if (command[0] != '/')
    {
        ptr_function = gui_key_function_search_by_name (command);
        if (!ptr_function)
        {
            wee_log_printf (_("%s unable to bind key \"%s\" (invalid function name: \"%s\")\n"),
                            WEECHAT_ERROR, key, command);
            return NULL;
        }
    }
    
    gui_key_unbind (key);
    
    new_key = gui_key_new (key,
                           (ptr_function) ? NULL : command,
                           ptr_function);
    if (!new_key)
    {
        wee_log_printf (_("%s not enough memory for key binding\n"),
                        WEECHAT_ERROR);
        return NULL;
    }
    
    return new_key;
}

/*
 * gui_key_unbind: remove a key binding
 */

int
gui_key_unbind (char *key)
{
    t_gui_key *ptr_key;
    char *internal_code;
    
    internal_code = gui_key_get_internal_code (key);
    
    ptr_key = gui_key_search ((internal_code) ? internal_code : key);
    if (ptr_key)
        gui_key_free (ptr_key);
    
    if (internal_code)
        free (internal_code);
    
    return (ptr_key != NULL);
}

/*
 * gui_key_pressed: treat new key pressed
 *                  return: 1 if key should be added to input buffer
 *                          0 otherwise
 */

int
gui_key_pressed (char *key_str)
{
    int first_key;
    t_gui_key *ptr_key;

    /* add key to buffer */
    first_key = (gui_key_buffer[0] == '\0');
    strcat (gui_key_buffer, key_str);
    
    /* if we are in "show mode", increase counter and return */
    if (gui_key_grab)
    {
        gui_key_grab_count++;
        return 0;
    }
    
    /* look for key combo in key table */
    ptr_key = gui_key_search_part (gui_key_buffer);
    if (ptr_key)
    {
        if (strcasecmp (ptr_key->key, gui_key_buffer) == 0)
        {
            /* exact combo found => execute function or command */
            gui_key_buffer[0] = '\0';
            if (ptr_key->command)
                user_command (SERVER(gui_current_window->buffer),
                              gui_current_window->buffer,
                              ptr_key->command);
            else
                (void)(ptr_key->function)();
        }
        return 0;
    }
    
    gui_key_buffer[0] = '\0';

    /* if this is first key and not found (even partial) => return 1
       else return 0 (= silently discard sequence of bad keys) */
    return first_key;
}

/*
 * key_free: delete a key binding
 */

void
gui_key_free (t_gui_key *key)
{
    /* free memory */
    if (key->key)
        free (key->key);
    if (key->command)
        free (key->command);
    
    /* remove key from keys list */
    if (key->prev_key)
        key->prev_key->next_key = key->next_key;
    if (key->next_key)
        key->next_key->prev_key = key->prev_key;
    if (gui_keys == key)
        gui_keys = key->next_key;
    if (last_gui_key == key)
        last_gui_key = key->prev_key;
    
    free (key);
}

/*
 * gui_key_free_all: delete all key bindings
 */

void
gui_key_free_all ()
{
    while (gui_keys)
        gui_key_free (gui_keys);
}
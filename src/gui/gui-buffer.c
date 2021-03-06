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

/* gui-buffer.c: buffer functions, used by all GUI */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <ctype.h>

#include "../core/weechat.h"
#include "../core/wee-config.h"
#include "../core/wee-hook.h"
#include "../core/wee-infolist.h"
#include "../core/wee-log.h"
#include "../core/wee-string.h"
#include "../core/wee-utf8.h"
#include "../plugins/plugin.h"
#include "gui-buffer.h"
#include "gui-chat.h"
#include "gui-color.h"
#include "gui-completion.h"
#include "gui-history.h"
#include "gui-hotlist.h"
#include "gui-input.h"
#include "gui-keyboard.h"
#include "gui-layout.h"
#include "gui-line.h"
#include "gui-main.h"
#include "gui-nicklist.h"
#include "gui-window.h"


struct t_gui_buffer *gui_buffers = NULL;           /* first buffer          */
struct t_gui_buffer *last_gui_buffer = NULL;       /* last buffer           */

/* history of last visited buffers */
struct t_gui_buffer_visited *gui_buffers_visited = NULL;
struct t_gui_buffer_visited *last_gui_buffer_visited = NULL;
int gui_buffers_visited_index = -1;             /* index of pointer in list */
int gui_buffers_visited_count = 0;              /* number of visited buffers*/
int gui_buffers_visited_frozen = 0;             /* 1 to forbid list updates */

char *gui_buffer_notify_string[GUI_BUFFER_NUM_NOTIFY] =
{ "none", "highlight", "message", "all" };


/*
 * gui_buffer_find_pos: find position for buffer in list
 */

struct t_gui_buffer *
gui_buffer_find_pos (struct t_gui_buffer *buffer)
{
    struct t_gui_buffer *ptr_buffer;
    
    /* if no number is asked by layout, then add to the end by default */
    if (buffer->layout_number < 1)
        return NULL;
    
    for (ptr_buffer = gui_buffers; ptr_buffer;
         ptr_buffer = ptr_buffer->next_buffer)
    {
        if (buffer->layout_number <= ptr_buffer->number)
            return ptr_buffer;
    }
    
    /* position not found, add to the end */
    return NULL;
}

/*
 * gui_buffer_local_var_search: search a local variable with name
 */

struct t_gui_buffer_local_var *
gui_buffer_local_var_search (struct t_gui_buffer *buffer, const char *name)
{
    struct t_gui_buffer_local_var *ptr_local_var;
    
    if (!buffer || !name)
        return NULL;
    
    for (ptr_local_var = buffer->local_variables; ptr_local_var;
         ptr_local_var = ptr_local_var->next_var)
    {
        if (strcmp (ptr_local_var->name, name) == 0)
            return ptr_local_var;
    }
    
    /* local variable not found */
    return NULL;
}

/*
 * gui_buffer_local_var_add: add a new local variable to a buffer
 */

struct t_gui_buffer_local_var *
gui_buffer_local_var_add (struct t_gui_buffer *buffer, const char *name,
                          const char *value)
{
    struct t_gui_buffer_local_var *new_local_var;
    
    if (!buffer || !name || !value)
        return NULL;
    
    new_local_var = gui_buffer_local_var_search (buffer, name);
    if (new_local_var)
    {
        if (new_local_var->name)
            free (new_local_var->name);
        if (new_local_var->value)
            free (new_local_var->value);
        new_local_var->name = strdup (name);
        new_local_var->value = strdup (value);
        
        hook_signal_send ("buffer_localvar_changed",
                          WEECHAT_HOOK_SIGNAL_POINTER, buffer);
    }
    else
    {
        new_local_var = malloc (sizeof (*new_local_var));
        if (new_local_var)
        {
            new_local_var->name = strdup (name);
            new_local_var->value = strdup (value);
            
            new_local_var->prev_var = buffer->last_local_var;
            new_local_var->next_var = NULL;
            if (buffer->local_variables)
                buffer->last_local_var->next_var = new_local_var;
            else
                buffer->local_variables = new_local_var;
            buffer->last_local_var = new_local_var;
            
            hook_signal_send ("buffer_localvar_added",
                              WEECHAT_HOOK_SIGNAL_POINTER, buffer);
        }
    }
    
    return new_local_var;
}

/*
 * gui_buffer_local_var_remove: remove a local variable in a buffer
 */

void
gui_buffer_local_var_remove (struct t_gui_buffer *buffer,
                             struct t_gui_buffer_local_var *local_var)
{
    if (!buffer || !local_var)
        return;
    
    /* free data */
    if (local_var->name)
        free (local_var->name);
    if (local_var->value)
        free (local_var->value);
    
    /* remove local variable from list */
    if (local_var->prev_var)
        (local_var->prev_var)->next_var = local_var->next_var;
    if (local_var->next_var)
        (local_var->next_var)->prev_var = local_var->prev_var;
    if (buffer->local_variables == local_var)
        buffer->local_variables = local_var->next_var;
    if (buffer->last_local_var == local_var)
        buffer->last_local_var = local_var->prev_var;
    
    free (local_var);

    hook_signal_send ("buffer_localvar_removed",
                      WEECHAT_HOOK_SIGNAL_POINTER, buffer);
}

/*
 * gui_buffer_local_var_remove_all: remove all local variables in a buffer
 */

void
gui_buffer_local_var_remove_all (struct t_gui_buffer *buffer)
{
    if (buffer)
    {
        while (buffer->local_variables)
        {
            gui_buffer_local_var_remove (buffer, buffer->local_variables);
        }
    }
}

/*
 * gui_buffer_notify_get: read a notify level in config file
 *                        we first try with all arguments, then remove one by one
 *                        to find notify level (from specific to general notify)
 */

int
gui_buffer_notify_get (struct t_gui_buffer *buffer)
{
    const char *plugin_name;
    char *option_name, *ptr_end;
    int length;
    struct t_config_option *ptr_option;
    
    plugin_name = plugin_get_name (buffer->plugin);
    length = strlen (plugin_name) + 1 + strlen (buffer->name) + 1;
    option_name = malloc (length);
    if (option_name)
    {
        snprintf (option_name, length, "%s.%s", plugin_name, buffer->name);
        
        ptr_end = option_name + strlen (option_name);
        while (ptr_end >= option_name)
        {
            ptr_option = config_file_search_option (weechat_config_file,
                                                    weechat_config_section_notify,
                                                    option_name);
            if (ptr_option)
            {
                free (option_name);
                return CONFIG_INTEGER(ptr_option);
            }
            ptr_end--;
            while ((ptr_end >= option_name) && (ptr_end[0] != '.'))
            {
                ptr_end--;
            }
            if ((ptr_end >= option_name) && (ptr_end[0] == '.'))
                ptr_end[0] = '\0';
        }
        ptr_option = config_file_search_option (weechat_config_file,
                                                weechat_config_section_notify,
                                                option_name);
        
        free (option_name);
        
        if (ptr_option)
            return CONFIG_INTEGER(ptr_option);
    }
    
    /* notify level not found */
    return CONFIG_INTEGER(config_look_buffer_notify_default);
}

/*
 * gui_buffer_notify_set: set notify value on a buffer
 */

void
gui_buffer_notify_set (struct t_gui_buffer *buffer)
{
    int old_notify, new_notify;
    
    old_notify = buffer->notify;
    new_notify = gui_buffer_notify_get (buffer);
    
    if (new_notify != old_notify)
    {
        buffer->notify = new_notify;
        gui_chat_printf (NULL,
                         _("Notify changed for \"%s%s.%s%s\": \"%s%s%s\" to \"%s%s%s\""),
                         GUI_COLOR(GUI_COLOR_CHAT_BUFFER),
                         plugin_get_name (buffer->plugin),
                         buffer->name,
                         GUI_COLOR(GUI_COLOR_CHAT),
                         GUI_COLOR(GUI_COLOR_CHAT_VALUE),
                         gui_buffer_notify_string[old_notify],
                         GUI_COLOR(GUI_COLOR_CHAT),
                         GUI_COLOR(GUI_COLOR_CHAT_VALUE),
                         gui_buffer_notify_string[buffer->notify],
                         GUI_COLOR(GUI_COLOR_CHAT));
    }
}

/*
 * gui_buffer_notify_set_all: set notify values on all opened buffers
 */

void
gui_buffer_notify_set_all ()
{
    struct t_gui_buffer *ptr_buffer;
    
    for (ptr_buffer = gui_buffers; ptr_buffer;
         ptr_buffer = ptr_buffer->next_buffer)
    {
        gui_buffer_notify_set (ptr_buffer);
    }
}

/*
 * gui_buffer_insert: insert buffer in good position in list of buffers
 */

void
gui_buffer_insert (struct t_gui_buffer *buffer)
{
    struct t_gui_buffer *pos, *ptr_buffer;
    
    pos = gui_buffer_find_pos (buffer);
    if (pos)
    {
        /* add buffer into the list (before position found) */
        buffer->number = pos->number;
        buffer->prev_buffer = pos->prev_buffer;
        buffer->next_buffer = pos;
        if (pos->prev_buffer)
            (pos->prev_buffer)->next_buffer = buffer;
        else
            gui_buffers = buffer;
        pos->prev_buffer = buffer;
        for (ptr_buffer = pos; ptr_buffer;
             ptr_buffer = ptr_buffer->next_buffer)
        {
            ptr_buffer->number++;
        }
    }
    else
    {
        /* add buffer to the end */
        buffer->number = (last_gui_buffer) ? last_gui_buffer->number + 1 : 1;
        buffer->prev_buffer = last_gui_buffer;
        buffer->next_buffer = NULL;
        if (gui_buffers)
            last_gui_buffer->next_buffer = buffer;
        else
            gui_buffers = buffer;
        last_gui_buffer = buffer;
    }
}

/*
 * gui_buffer_new: create a new buffer in current window
 */

struct t_gui_buffer *
gui_buffer_new (struct t_weechat_plugin *plugin,
                const char *name,
                int (*input_callback)(void *data,
                                      struct t_gui_buffer *buffer,
                                      const char *input_data),
                void *input_callback_data,
                int (*close_callback)(void *data,
                                      struct t_gui_buffer *buffer),
                void *close_callback_data)
{
    struct t_gui_buffer *new_buffer;
    struct t_gui_completion *new_completion;
    int first_buffer_creation;
    
    if (!name)
        return NULL;
    
    if (gui_buffer_search_by_name (plugin_get_name (plugin), name))
    {
        gui_chat_printf (NULL,
                         _("%sError: a buffer with same name (%s) already "
                           "exists"),
                         gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                         name);
        return NULL;
    }
    
    /* create new buffer */
    new_buffer = malloc (sizeof (*new_buffer));
    if (new_buffer)
    {
        /* init buffer */
        new_buffer->plugin = plugin;
        new_buffer->plugin_name_for_upgrade = NULL;
        new_buffer->merge_for_upgrade = NULL;
        
        /* number will be set later (when inserting buffer in list) */
        new_buffer->layout_number = gui_layout_buffer_get_number (gui_layout_buffers,
                                                                  plugin_get_name (plugin),
                                                                  name);
        new_buffer->name = strdup (name);
        new_buffer->short_name = strdup (name);
        new_buffer->type = GUI_BUFFER_TYPE_FORMATTED;
        new_buffer->notify = CONFIG_INTEGER(config_look_buffer_notify_default);
        new_buffer->num_displayed = 0;
        new_buffer->active = 1;
        new_buffer->print_hooks_enabled = 1;
        
        /* close callback */
        new_buffer->close_callback = close_callback;
        new_buffer->close_callback_data = close_callback_data;
        
        /* title */
        new_buffer->title = NULL;
        
        /* chat content */
        new_buffer->own_lines = gui_lines_alloc ();
        new_buffer->mixed_lines = NULL;
        new_buffer->lines = new_buffer->own_lines;
        new_buffer->time_for_each_line = 1;
        new_buffer->chat_refresh_needed = 2;
        
        /* nicklist */
        new_buffer->nicklist = 0;
        new_buffer->nicklist_case_sensitive = 0;
        new_buffer->nicklist_root = NULL;
        new_buffer->nicklist_max_length = 0;
        new_buffer->nicklist_display_groups = 1;
        new_buffer->nicklist_visible_count = 0;
        gui_nicklist_add_group (new_buffer, NULL, "root", NULL, 0);
        
        /* input */
        new_buffer->input = 1;
        new_buffer->input_callback = input_callback;
        new_buffer->input_callback_data = input_callback_data;
        new_buffer->input_get_unknown_commands = 0;
        new_buffer->input_buffer_alloc = GUI_BUFFER_INPUT_BLOCK_SIZE;
        new_buffer->input_buffer = malloc (GUI_BUFFER_INPUT_BLOCK_SIZE);
        new_buffer->input_buffer[0] = '\0';
        new_buffer->input_buffer_size = 0;
        new_buffer->input_buffer_length = 0;
        new_buffer->input_buffer_pos = 0;
        new_buffer->input_buffer_1st_display = 0;
        
        /* init completion */
        new_completion = malloc (sizeof (*new_completion));
        if (new_completion)
        {
            new_buffer->completion = new_completion;
            gui_completion_buffer_init (new_completion, new_buffer);
        }
        
        /* init history */
        new_buffer->history = NULL;
        new_buffer->last_history = NULL;
        new_buffer->ptr_history = NULL;
        new_buffer->num_history = 0;
        
        /* text search */
        new_buffer->text_search = GUI_TEXT_SEARCH_DISABLED;
        new_buffer->text_search_exact = 0;
        new_buffer->text_search_found = 0;
        new_buffer->text_search_input = NULL;
        
        /* highlight */
        new_buffer->highlight_words = NULL;
        new_buffer->highlight_tags = NULL;
        new_buffer->highlight_tags_count = 0;
        new_buffer->highlight_tags_array = NULL;
        
        /* keys */
        new_buffer->keys = NULL;
        new_buffer->last_key = NULL;
        
        /* local variables */
        new_buffer->local_variables = NULL;
        new_buffer->last_local_var = NULL;
        gui_buffer_local_var_add (new_buffer, "plugin", plugin_get_name (plugin));
        gui_buffer_local_var_add (new_buffer, "name", name);
        
        /* add buffer to buffers list */
        first_buffer_creation = (gui_buffers == NULL);
        gui_buffer_insert (new_buffer);
        
        /* set notify level */
        new_buffer->notify = gui_buffer_notify_get (new_buffer);
        
        /* check if this buffer should be assigned to a window,
           according to windows layout saved */
        gui_layout_window_check_buffer (new_buffer);
        
        if (first_buffer_creation)
        {
            gui_buffer_visited_add (new_buffer);
        }
        else
        {
            hook_signal_send ("buffer_opened",
                              WEECHAT_HOOK_SIGNAL_POINTER, new_buffer);
        }
    }
    else
        return NULL;
    
    return new_buffer;
}

/*
 * gui_buffer_valid: check if a buffer pointer exists
 *                   return 1 if buffer exists
 *                          0 if buffer is not found
 */

int
gui_buffer_valid (struct t_gui_buffer *buffer)
{
    struct t_gui_buffer *ptr_buffer;
    
    /* NULL buffer is valid (it's for printing on first buffer) */
    if (!buffer)
        return 1;
    
    for (ptr_buffer = gui_buffers; ptr_buffer;
         ptr_buffer = ptr_buffer->next_buffer)
    {
        if (ptr_buffer == buffer)
            return 1;
    }
    
    /* buffer not found */
    return 0;
}

/*
 * gui_buffer_string_replace_local_var: replace local variables ($var) in a
 *                                      string, using value of local variables
 */

char *
gui_buffer_string_replace_local_var (struct t_gui_buffer *buffer,
                                     const char *string)
{
    int length, length_var, index_string, index_result;
    char *result, *local_var;
    const char *pos_end_name;
    struct t_gui_buffer_local_var *ptr_local_var;
    
    length = strlen (string) + 1;
    result = malloc (length);
    if (result)
    {
        index_string = 0;
        index_result = 0;
        while (string[index_string])
        {
            if ((string[index_string] == '$')
                && ((index_string == 0) || (string[index_string - 1] != '\\')))
            {
                pos_end_name = string + index_string + 1;
                while (pos_end_name[0])
                {
                    if (isalnum (pos_end_name[0])
                        && (pos_end_name[0] != '_')
                        && (pos_end_name[0] != '$'))
                        pos_end_name++;
                    else
                        break;
                }
                if (pos_end_name > string + index_string + 1)
                {
                    local_var = string_strndup (string + index_string + 1,
                                                pos_end_name - (string + index_string + 1));
                    if (local_var)
                    {
                        ptr_local_var = gui_buffer_local_var_search (buffer,
                                                                     local_var);
                        if (ptr_local_var)
                        {
                            length_var = strlen (ptr_local_var->value);
                            length += length_var;
                            result = realloc (result, length);
                            if (!result)
                            {
                                free (local_var);
                                return NULL;
                            }
                            strcpy (result + index_result, ptr_local_var->value);
                            index_result += length_var;
                            index_string += strlen (local_var) + 1;
                        }
                        else
                            result[index_result++] = string[index_string++];
                        
                        free (local_var);
                    }
                    else
                        result[index_result++] = string[index_string++];
                }
                else
                    result[index_result++] = string[index_string++];
            }
            else
                result[index_result++] = string[index_string++];
        }
        result[index_result] = '\0';
    }
    
    return result;
}

/*
 * gui_buffer_set_plugin_for_upgrade: set plugin pointer for buffers with a
 *                                    given name (used after /upgrade)
 */

void
gui_buffer_set_plugin_for_upgrade (char *name, struct t_weechat_plugin *plugin)
{
    struct t_gui_buffer *ptr_buffer;
    
    for (ptr_buffer = gui_buffers; ptr_buffer;
         ptr_buffer = ptr_buffer->next_buffer)
    {
        if (ptr_buffer->plugin_name_for_upgrade
            && (strcmp (ptr_buffer->plugin_name_for_upgrade, name) == 0))
        {
            free (ptr_buffer->plugin_name_for_upgrade);
            ptr_buffer->plugin_name_for_upgrade = NULL;
            
            ptr_buffer->plugin = plugin;
        }
    }
}

/*
 * gui_buffer_get_integer: get a buffer property as integer
 */

int
gui_buffer_get_integer (struct t_gui_buffer *buffer, const char *property)
{
    if (buffer && property)
    {
        if (string_strcasecmp (property, "number") == 0)
            return buffer->number;
        else if (string_strcasecmp (property, "num_displayed") == 0)
            return buffer->num_displayed;
        else if (string_strcasecmp (property, "print_hooks_enabled") == 0)
            return buffer->print_hooks_enabled;
        else if (string_strcasecmp (property, "notify") == 0)
            return buffer->notify;
        else if (string_strcasecmp (property, "lines_hidden") == 0)
            return buffer->lines->lines_hidden;
        else if (string_strcasecmp (property, "prefix_max_length") == 0)
            return buffer->lines->prefix_max_length;
        else if (string_strcasecmp (property, "time_for_each_line") == 0)
            return buffer->time_for_each_line;
        else if (string_strcasecmp (property, "text_search") == 0)
            return buffer->text_search;
        else if (string_strcasecmp (property, "text_search_exact") == 0)
            return buffer->text_search_exact;
        else if (string_strcasecmp (property, "text_search_found") == 0)
            return buffer->text_search_found;
    }
    
    return 0;
}

/*
 * gui_buffer_get_string: get a buffer property as string
 */

const char *
gui_buffer_get_string (struct t_gui_buffer *buffer, const char *property)
{
    struct t_gui_buffer_local_var *ptr_local_var;
    
    if (buffer && property)
    {
        if (string_strcasecmp (property, "plugin") == 0)
            return plugin_get_name (buffer->plugin);
        else if (string_strcasecmp (property, "name") == 0)
            return buffer->name;
        else if (string_strcasecmp (property, "short_name") == 0)
            return buffer->short_name;
        else if (string_strcasecmp (property, "title") == 0)
            return buffer->title;
        else if (string_strcasecmp (property, "input") == 0)
            return buffer->input_buffer;
        else if (string_strncasecmp (property, "localvar_", 9) == 0)
        {
            ptr_local_var = gui_buffer_local_var_search (buffer, property + 9);
            if (ptr_local_var)
                return ptr_local_var->value;
        }
    }
    
    return NULL;
}

/*
 * gui_buffer_get_pointer: get a buffer property as pointer
 */

void *
gui_buffer_get_pointer (struct t_gui_buffer *buffer, const char *property)
{
    if (buffer && property)
    {
        if (string_strcasecmp (property, "plugin") == 0)
            return buffer->plugin;
    }
    
    return NULL;
}

/*
 * gui_buffer_ask_chat_refresh: set "chat_refresh_needed" flag
 */

void
gui_buffer_ask_chat_refresh (struct t_gui_buffer *buffer, int refresh)
{
    if (refresh > buffer->chat_refresh_needed)
        buffer->chat_refresh_needed = refresh;
}

/*
 * gui_buffer_set_name: set name for a buffer
 */

void
gui_buffer_set_name (struct t_gui_buffer *buffer, const char *name)
{
    if (name && name[0])
    {
        if (buffer->name)
            free (buffer->name);
        buffer->name = strdup (name);
        
        gui_buffer_local_var_add (buffer, "name", name);
        
        hook_signal_send ("buffer_renamed",
                          WEECHAT_HOOK_SIGNAL_POINTER, buffer);
    }
}

/*
 * gui_buffer_set_short_name: set short name for a buffer
 */

void
gui_buffer_set_short_name (struct t_gui_buffer *buffer, const char *short_name)
{
    if (short_name && short_name[0])
    {
        if (buffer->short_name)
            free (buffer->short_name);
        buffer->short_name = strdup (short_name);
        
        hook_signal_send ("buffer_renamed",
                          WEECHAT_HOOK_SIGNAL_POINTER, buffer);
    }
}

/*
 * gui_buffer_set_type: set buffer type
 */

void
gui_buffer_set_type (struct t_gui_buffer *buffer, enum t_gui_buffer_type type)
{
    if (buffer->type == type)
        return;
    
    gui_line_free_all (buffer);
    
    buffer->type = type;
    gui_buffer_ask_chat_refresh (buffer, 2);

    hook_signal_send ("buffer_type_changed",
                      WEECHAT_HOOK_SIGNAL_POINTER, buffer);
}

/*
 * gui_buffer_set_title: set title for a buffer
 */

void
gui_buffer_set_title (struct t_gui_buffer *buffer, const char *new_title)
{
    if (buffer->title)
        free (buffer->title);
    buffer->title = (new_title && new_title[0]) ? strdup (new_title) : NULL;
    
    hook_signal_send ("buffer_title_changed",
                      WEECHAT_HOOK_SIGNAL_POINTER, buffer);
}

/*
 * gui_buffer_set_time_for_each_line: set flag "time for each line" for a buffer
 */

void
gui_buffer_set_time_for_each_line (struct t_gui_buffer *buffer,
                                   int time_for_each_line)
{
    buffer->time_for_each_line = (time_for_each_line) ? 1 : 0;
    gui_buffer_ask_chat_refresh (buffer, 2);
}

/*
 * gui_buffer_set_nicklist: set nicklist for a buffer
 */

void
gui_buffer_set_nicklist (struct t_gui_buffer *buffer, int nicklist)
{
    buffer->nicklist = (nicklist) ? 1 : 0;
    gui_window_refresh_windows ();
}

/*
 * gui_buffer_set_nicklist_case_sensitive: set case_sensitive flag for a buffer
 */

void
gui_buffer_set_nicklist_case_sensitive (struct t_gui_buffer *buffer,
                                        int case_sensitive)
{
    buffer->nicklist_case_sensitive = (case_sensitive) ? 1 : 0;
}

/*
 * gui_buffer_set_nicklist_display_groups: set display_groups flag for a buffer
 */

void
gui_buffer_set_nicklist_display_groups (struct t_gui_buffer *buffer,
                                        int display_groups)
{
    buffer->nicklist_display_groups = (display_groups) ? 1 : 0;
    buffer->nicklist_visible_count = 0;
    gui_nicklist_compute_visible_count (buffer, buffer->nicklist_root);
}

/*
 * gui_buffer_set_highlight_words: set highlight words for a buffer
 */

void
gui_buffer_set_highlight_words (struct t_gui_buffer *buffer,
                                const char *new_highlight_words)
{
    if (buffer->highlight_words)
        free (buffer->highlight_words);
    buffer->highlight_words = (new_highlight_words && new_highlight_words[0]) ?
        strdup (new_highlight_words) : NULL;
}

/*
 * gui_buffer_set_highlight_tags: set highlight tags for a buffer
 */

void
gui_buffer_set_highlight_tags (struct t_gui_buffer *buffer,
                               const char *new_highlight_tags)
{
    if (buffer->highlight_tags)
        free (buffer->highlight_tags);
    if (buffer->highlight_tags_array)
        string_free_split (buffer->highlight_tags_array);
    
    if (new_highlight_tags)
    {
        buffer->highlight_tags = strdup (new_highlight_tags);
        if (buffer->highlight_tags)
        {
            buffer->highlight_tags_array = string_split (new_highlight_tags,
                                                         ",", 0, 0,
                                                         &buffer->highlight_tags_count);
        }
    }
    else
    {
        buffer->highlight_tags = NULL;
        buffer->highlight_tags_count = 0;
        buffer->highlight_tags_array = NULL;
    }
}

/*
 * gui_buffer_set_input_get_unknown_commands: set "input_get_unknown_commands"
 *                                            flag for a buffer
 */

void
gui_buffer_set_input_get_unknown_commands (struct t_gui_buffer *buffer,
                                           int input_get_unknown_commands)
{
    buffer->input_get_unknown_commands = (input_get_unknown_commands) ? 1 : 0;
}

/*
 * gui_buffer_set_unread: set unread marker for a buffer
 */

void
gui_buffer_set_unread (struct t_gui_buffer *buffer)
{
    int refresh;

    if (buffer->type == GUI_BUFFER_TYPE_FORMATTED)
    {
        refresh = ((buffer->lines->last_read_line != NULL)
                   && (buffer->lines->last_read_line != buffer->lines->last_line));
        
        buffer->lines->last_read_line = buffer->lines->last_line;
        buffer->lines->first_line_not_read = (buffer->lines->last_read_line) ? 0 : 1;
        
        if (refresh)
            gui_buffer_ask_chat_refresh (buffer, 2);
    }
}

/*
 * gui_buffer_set: set a buffer property (string)
 */

void
gui_buffer_set (struct t_gui_buffer *buffer, const char *property,
                const char *value)
{
    long number;
    char *error;
    struct t_gui_buffer_local_var *ptr_local_var;
    
    if (!property || !value)
        return;
    
    /* properties that does NOT need a buffer */
    if (string_strcasecmp (property, "hotlist") == 0)
    {
        if (strcmp (value, "-") == 0)
            gui_add_hotlist = 0;
        else if (strcmp (value, "+") == 0)
            gui_add_hotlist = 1;
        else
        {
            error = NULL;
            number = strtol (value, &error, 10);
            if (error && !error[0])
                gui_hotlist_add (buffer, number, NULL, 1);
        }
    }
    
    if (!buffer)
        return;
    
    /* properties that need a buffer */
    if (string_strcasecmp (property, "unread") == 0)
    {
        gui_buffer_set_unread (buffer);
    }
    else if (string_strcasecmp (property, "display") == 0)
    {
        /* if it is auto-switch to a buffer, then we don't set read marker,
           otherwise we reset it (if current buffer is not displayed) after
           switch */
        gui_window_switch_to_buffer (gui_current_window, buffer,
                                     (string_strcasecmp (value, "auto") == 0) ?
                                     0 : 1);
    }
    else if (string_strcasecmp (property, "print_hooks_enabled") == 0)
    {
        error = NULL;
        number = strtol (value, &error, 10);
        if (error && !error[0])
            buffer->print_hooks_enabled = (number) ? 1 : 0;
    }
    else if (string_strcasecmp (property, "number") == 0)
    {
        error = NULL;
        number = strtol (value, &error, 10);
        if (error && !error[0] && (number >= 1))
            gui_buffer_move_to_number (buffer, number);
    }
    else if (string_strcasecmp (property, "name") == 0)
    {
        gui_buffer_set_name (buffer, value);
    }
    else if (string_strcasecmp (property, "short_name") == 0)
    {
        gui_buffer_set_short_name (buffer, value);
    }
    else if (string_strcasecmp (property, "type") == 0)
    {
        if (string_strcasecmp (value, "formatted") == 0)
            gui_buffer_set_type (buffer, GUI_BUFFER_TYPE_FORMATTED);
        else if (string_strcasecmp (value, "free") == 0)
            gui_buffer_set_type (buffer, GUI_BUFFER_TYPE_FREE);
    }
    else if (string_strcasecmp (property, "notify") == 0)
    {
        error = NULL;
        number = strtol (value, &error, 10);
        if (error && !error[0]
            && (number < GUI_BUFFER_NUM_NOTIFY))
        {
            if (number < 0)
                buffer->notify = CONFIG_INTEGER(config_look_buffer_notify_default);
            else
                buffer->notify = number;
        }
    }
    else if (string_strcasecmp (property, "title") == 0)
    {
        gui_buffer_set_title (buffer, value);
    }
    else if (string_strcasecmp (property, "time_for_each_line") == 0)
    {
        error = NULL;
        number = strtol (value, &error, 10);
        if (error && !error[0])
            gui_buffer_set_time_for_each_line (buffer, number);
    }
    else if (string_strcasecmp (property, "nicklist") == 0)
    {
        error = NULL;
        number = strtol (value, &error, 10);
        if (error && !error[0])
            gui_buffer_set_nicklist (buffer, number);
    }
    else if (string_strcasecmp (property, "nicklist_case_sensitive") == 0)
    {
        error = NULL;
        number = strtol (value, &error, 10);
        if (error && !error[0])
            gui_buffer_set_nicklist_case_sensitive (buffer, number);
    }
    else if (string_strcasecmp (property, "nicklist_display_groups") == 0)
    {
        error = NULL;
        number = strtol (value, &error, 10);
        if (error && !error[0])
            gui_buffer_set_nicklist_display_groups (buffer, number);
    }
    else if (string_strcasecmp (property, "highlight_words") == 0)
    {
        gui_buffer_set_highlight_words (buffer, value);
    }
    else if (string_strcasecmp (property, "highlight_tags") == 0)
    {
        gui_buffer_set_highlight_tags (buffer, value);
    }
    else if (string_strncasecmp (property, "key_bind_", 9) == 0)
    {
        gui_keyboard_bind (buffer, property + 9, value);
    }
    else if (string_strncasecmp (property, "key_unbind_", 11) == 0)
    {
        if (strcmp (property + 11, "*") == 0)
            gui_keyboard_free_all (&buffer->keys, &buffer->last_key);
        else
            gui_keyboard_unbind (buffer, property + 11, 1);
    }
    else if (string_strcasecmp (property, "input") == 0)
    {
        gui_input_delete_line (buffer);
        gui_input_insert_string (buffer, value, 0);
        gui_input_text_changed_modifier_and_signal (buffer);
    }
    else if (string_strcasecmp (property, "input_get_unknown_commands") == 0)
    {
        error = NULL;
        number = strtol (value, &error, 10);
        if (error && !error[0])
            gui_buffer_set_input_get_unknown_commands (buffer, number);
    }
    else if (string_strncasecmp (property, "localvar_set_", 13) == 0)
    {
        if (value)
            gui_buffer_local_var_add (buffer, property + 13, value);
    }
    else if (string_strncasecmp (property, "localvar_del_", 13) == 0)
    {
        ptr_local_var = gui_buffer_local_var_search (buffer, property + 13);
        if (ptr_local_var)
            gui_buffer_local_var_remove (buffer, ptr_local_var);
    }
}

/*
 * gui_buffer_set: set a buffer property (pointer)
 */

void
gui_buffer_set_pointer (struct t_gui_buffer *buffer, const char *property,
                        void *pointer)
{
    if (!buffer || !property)
        return;
    
    if (string_strcasecmp (property, "close_callback") == 0)
    {
        buffer->close_callback = pointer;
    }
    else if (string_strcasecmp (property, "close_callback_data") == 0)
    {
        buffer->close_callback_data = pointer;
    }
    else if (string_strcasecmp (property, "input_callback") == 0)
    {
        buffer->input_callback = pointer;
    }
    else if (string_strcasecmp (property, "input_callback_data") == 0)
    {
        buffer->input_callback_data = pointer;
    }
}

/*
 * gui_buffer_compute_num_displayed: compute "num_displayed" for all buffers
 */

void
gui_buffer_compute_num_displayed ()
{
    struct t_gui_buffer *ptr_buffer;
    struct t_gui_window *ptr_window;
    
    for (ptr_buffer = gui_buffers; ptr_buffer;
         ptr_buffer = ptr_buffer->next_buffer)
    {
        ptr_buffer->num_displayed = 0;
    }
    
    for (ptr_window = gui_windows; ptr_window;
         ptr_window = ptr_window->next_window)
    {
        if (ptr_window->buffer)
            ptr_window->buffer->num_displayed++;
    }
}

/*
 * gui_buffer_add_value_num_displayed: add value to "num_displayed" variable
 *                                     for a buffer (value can be negative)
 */

void
gui_buffer_add_value_num_displayed (struct t_gui_buffer *buffer, int value)
{
    struct t_gui_buffer *ptr_buffer;
    
    for (ptr_buffer = gui_buffers; ptr_buffer;
         ptr_buffer = ptr_buffer->next_buffer)
    {
        if (ptr_buffer->number == buffer->number)
        {
            ptr_buffer->num_displayed += value;
            if (ptr_buffer->num_displayed < 0)
                ptr_buffer->num_displayed = 0;
        }
    }
}

/*
 * gui_buffer_search_main: get main buffer (weechat one, created at startup)
 *                         return first buffer if not found
 */

struct t_gui_buffer *
gui_buffer_search_main ()
{
    struct t_gui_buffer *ptr_buffer;
    
    for (ptr_buffer = gui_buffers; ptr_buffer;
         ptr_buffer = ptr_buffer->next_buffer)
    {
        if (!ptr_buffer->plugin)
            return ptr_buffer;
    }
    
    /* buffer not found (should never occur!) */
    return NULL;
}

/*
 * gui_buffer_search_by_name: search a buffer by name
 */

struct t_gui_buffer *
gui_buffer_search_by_name (const char *plugin, const char *name)
{
    struct t_gui_buffer *ptr_buffer;
    int plugin_match;
    
    if (!name || !name[0])
        return gui_current_window->buffer;
    
    for (ptr_buffer = gui_buffers; ptr_buffer;
         ptr_buffer = ptr_buffer->next_buffer)
    {
        if (ptr_buffer->name)
        {
            plugin_match = 1;
            if (plugin && plugin[0])
            {
                if (ptr_buffer->plugin_name_for_upgrade)
                {
                    if (strcmp (plugin, ptr_buffer->plugin_name_for_upgrade) != 0)
                        plugin_match = 0;
                }
                else
                {
                    if (ptr_buffer->plugin)
                    {
                        if (strcmp (plugin, ptr_buffer->plugin->name) != 0)
                            plugin_match = 0;
                    }
                    else
                    {
                        if (strcmp (plugin, PLUGIN_CORE) != 0)
                            plugin_match = 0;
                    }
                }
            }
            if (plugin_match && (strcmp (ptr_buffer->name, name) == 0))
            {
                return ptr_buffer;
            }
        }
    }
    
    /* buffer not found */
    return NULL;
}

/*
 * gui_buffer_search_by_partial_name: search a buffer by name (may be partial)
 */

struct t_gui_buffer *
gui_buffer_search_by_partial_name (const char *plugin, const char *name)
{
    struct t_gui_buffer *ptr_start_buffer, *ptr_buffer, *buffer_partial_match[3];
    int plugin_match, length_name;
    const char *pos;
    
    if (!name || !name[0])
        return gui_current_window->buffer;
    
    /* 0: mathces beginning of buffer name, 1: in the middle, 2: the end */
    buffer_partial_match[0] = NULL;
    buffer_partial_match[1] = NULL;
    buffer_partial_match[2] = NULL;
    
    length_name = strlen (name);

    ptr_buffer = gui_current_window->buffer->next_buffer;
    if (!ptr_buffer)
        ptr_buffer = gui_buffers;
    ptr_start_buffer = ptr_buffer;
    
    while (ptr_buffer)
    {
        if (ptr_buffer->name)
        {
            plugin_match = 1;
            if (plugin && plugin[0])
            {
                if (ptr_buffer->plugin)
                {
                    if (strcmp (plugin, ptr_buffer->plugin->name) != 0)
                        plugin_match = 0;
                }
                else
                {
                    if (strcmp (plugin, PLUGIN_CORE) != 0)
                        plugin_match = 0;
                }
            }
            if (plugin_match)
            {
                pos = strstr (ptr_buffer->name, name);
                if (pos)
                {
                    if (pos == ptr_buffer->name)
                    {
                        if (!pos[length_name])
                        {
                            /* matches full name, return it immediately */
                            return ptr_buffer;
                        }
                        /* matches beginning of name */
                        if (!buffer_partial_match[0])
                            buffer_partial_match[0] = ptr_buffer;
                    }
                    else
                    {
                        if (pos[length_name])
                        {
                            /* matches middle of buffer name */
                            if (!buffer_partial_match[1])
                                buffer_partial_match[1] = ptr_buffer;
                        }
                        else
                        {
                            /* matches end of buffer name */
                            if (!buffer_partial_match[2])
                                buffer_partial_match[2] = ptr_buffer;
                        }
                    }
                }
            }
        }
        ptr_buffer = ptr_buffer->next_buffer;
        if (!ptr_buffer)
            ptr_buffer = gui_buffers;
        if (ptr_buffer == ptr_start_buffer)
            break;
    }
    
    /* matches end of name? */
    if (buffer_partial_match[2])
        return buffer_partial_match[2];
    
    /* matches beginning of name? */
    if (buffer_partial_match[0])
        return buffer_partial_match[0];
    
    /* return buffer partially matching in name
       (may be NULL if no buffer was found) */
    return buffer_partial_match[1];
}

/*
 * gui_buffer_search_by_number: search a buffer by number
 */

struct t_gui_buffer *
gui_buffer_search_by_number (int number)
{
    struct t_gui_buffer *ptr_buffer;
    
    for (ptr_buffer = gui_buffers; ptr_buffer;
         ptr_buffer = ptr_buffer->next_buffer)
    {
        if (ptr_buffer->number == number)
            return ptr_buffer;
    }
    
    /* buffer not found */
    return NULL;
}

/*
 * gui_buffer_count_merged_buffers: return number of merged buffers (buffers
 *                                  with same number)
 */

int
gui_buffer_count_merged_buffers (int number)
{
    struct t_gui_buffer *ptr_buffer;
    int count;
    
    count = 0;
    
    for (ptr_buffer = gui_buffers; ptr_buffer;
         ptr_buffer = ptr_buffer->next_buffer)
    {
        if (ptr_buffer->number == number)
            count++;
    }
    
    return count;
}

/*
 * gui_buffer_is_scrolled: return 1 if all windows displaying buffer are scrolled
 *                         (user doesn't see end of buffer)
 *                         return 0 if at least one window is NOT scrolled
 */

int
gui_buffer_is_scrolled (struct t_gui_buffer *buffer)
{
    struct t_gui_window *ptr_win;
    int buffer_found;

    if (!buffer)
        return 0;
    
    buffer_found = 0;
    for (ptr_win = gui_windows; ptr_win; ptr_win = ptr_win->next_window)
    {
        if (ptr_win->buffer == buffer)
        {
            buffer_found = 1;
            /* buffer found and not scrolled, exit immediately */
            if (!ptr_win->scroll)
                return 0;
        }
    }
    
    /* buffer found, and all windows were scrolled */
    if (buffer_found)
        return 1;
    
    /* buffer not found */
    return 0;
}

/*
 * gui_buffer_clear: clear buffer content
 */

void
gui_buffer_clear (struct t_gui_buffer *buffer)
{
    struct t_gui_window *ptr_win;
    
    if (!buffer)
        return;
    
    /* remove all lines */
    gui_line_free_all (buffer);
    
    /* remove any scroll for buffer */
    for (ptr_win = gui_windows; ptr_win; ptr_win = ptr_win->next_window)
    {
        if (ptr_win->buffer == buffer)
        {
            ptr_win->first_line_displayed = 1;
            ptr_win->start_line = NULL;
            ptr_win->start_line_pos = 0;
        }
    }
    
    gui_hotlist_remove_buffer (buffer);
    
    gui_buffer_ask_chat_refresh (buffer, 2);
}

/*
 * gui_buffer_clear_all: clear all buffers content
 */

void
gui_buffer_clear_all ()
{
    struct t_gui_buffer *ptr_buffer;
    
    for (ptr_buffer = gui_buffers; ptr_buffer;
         ptr_buffer = ptr_buffer->next_buffer)
    {
        if (ptr_buffer->type == GUI_BUFFER_TYPE_FORMATTED)
            gui_buffer_clear (ptr_buffer);
    }
}

/*
 * gui_buffer_close: close a buffer
 */

void
gui_buffer_close (struct t_gui_buffer *buffer)
{
    struct t_gui_window *ptr_window;
    struct t_gui_buffer *ptr_buffer, *ptr_back_to_buffer;
    int index;
    struct t_gui_buffer_visited *ptr_buffer_visited;
    
    hook_signal_send ("buffer_closing",
                      WEECHAT_HOOK_SIGNAL_POINTER, buffer);
    
    if (buffer->close_callback)
    {
        (void)(buffer->close_callback) (buffer->close_callback_data, buffer);
    }
    
    ptr_back_to_buffer = NULL;
    
    /* first unmerge buffer if it is merged to at least one other buffer */
    if (gui_buffer_count_merged_buffers (buffer->number) > 1)
    {
        ptr_back_to_buffer = gui_buffer_get_next_active_buffer (buffer);
        gui_buffer_unmerge (buffer, -1);
    }
    
    if (!weechat_quit)
    {
        /* find other buffer to display: previously visited buffer if current
           window is displaying buffer, or buffer # - 1 */
        ptr_buffer_visited = NULL;
        if (CONFIG_BOOLEAN(config_look_jump_previous_buffer_when_closing)
            && gui_current_window && (gui_current_window->buffer == buffer))
        {
            index = gui_buffer_visited_get_index_previous ();
            if (index >= 0)
            {
                ptr_buffer_visited = gui_buffer_visited_search_by_number (index);
                if (ptr_buffer_visited->buffer == buffer)
                    ptr_buffer_visited = NULL;
            }
        }
        
        /* switch to another buffer in each window that displays buffer */
        for (ptr_window = gui_windows; ptr_window;
             ptr_window = ptr_window->next_window)
        {
            if ((buffer == ptr_window->buffer) &&
                ((buffer->next_buffer) || (buffer->prev_buffer)))
            {
                /* switch to previous buffer */
                if (gui_buffers != last_gui_buffer)
                {
                    if (ptr_back_to_buffer)
                    {
                        gui_window_switch_to_buffer (ptr_window,
                                                     ptr_back_to_buffer,
                                                     1);
                    }
                    else if (ptr_buffer_visited)
                    {
                        gui_window_switch_to_buffer (ptr_window,
                                                     ptr_buffer_visited->buffer,
                                                     1);
                    }
                    else if (ptr_window->buffer->prev_buffer)
                    {
                        gui_window_switch_to_buffer (ptr_window,
                                                     ptr_window->buffer->prev_buffer,
                                                     1);
                    }
                    else
                    {
                        gui_window_switch_to_buffer (ptr_window,
                                                     last_gui_buffer,
                                                     1);
                    }
                }
            }
        }
    }
    
    gui_hotlist_remove_buffer (buffer);
    if (gui_hotlist_initial_buffer == buffer)
        gui_hotlist_initial_buffer = NULL;
    
    gui_buffer_visited_remove_by_buffer (buffer);
    
    /* decrease buffer number for all next buffers */
    for (ptr_buffer = buffer->next_buffer; ptr_buffer;
         ptr_buffer = ptr_buffer->next_buffer)
    {
        ptr_buffer->number--;
    }
    
    /* free all lines */
    gui_line_free_all (buffer);
    if (buffer->own_lines)
        free (buffer->own_lines);
    if (buffer->mixed_lines)
        free (buffer->mixed_lines);
    
    /* free some data */
    if (buffer->title)
        free (buffer->title);
    if (buffer->name)
        free (buffer->name);
    if (buffer->short_name)
        free (buffer->short_name);
    if (buffer->input_buffer)
        free (buffer->input_buffer);
    if (buffer->completion)
        gui_completion_free (buffer->completion);
    gui_history_buffer_free (buffer);
    if (buffer->text_search_input)
        free (buffer->text_search_input);
    gui_nicklist_remove_all (buffer);
    gui_nicklist_remove_group (buffer, buffer->nicklist_root);
    if (buffer->highlight_words)
        free (buffer->highlight_words);
    if (buffer->highlight_tags)
        free (buffer->highlight_tags);
    if (buffer->highlight_tags_array)
        string_free_split (buffer->highlight_tags_array);
    gui_keyboard_free_all (&buffer->keys, &buffer->last_key);
    gui_buffer_local_var_remove_all (buffer);
    
    /* remove buffer from buffers list */
    if (buffer->prev_buffer)
        (buffer->prev_buffer)->next_buffer = buffer->next_buffer;
    if (buffer->next_buffer)
        (buffer->next_buffer)->prev_buffer = buffer->prev_buffer;
    if (gui_buffers == buffer)
        gui_buffers = buffer->next_buffer;
    if (last_gui_buffer == buffer)
        last_gui_buffer = buffer->prev_buffer;
    
    for (ptr_window = gui_windows; ptr_window;
         ptr_window = ptr_window->next_window)
    {
        if (ptr_window->buffer == buffer)
            ptr_window->buffer = NULL;
    }
    
    hook_signal_send ("buffer_closed",
                      WEECHAT_HOOK_SIGNAL_POINTER, buffer);
    
    free (buffer);
}

/*
 * gui_buffer_switch_by_number: switch to another buffer with number
 */

void
gui_buffer_switch_by_number (struct t_gui_window *window, int number)
{
    struct t_gui_buffer *ptr_buffer;
    
    /* invalid buffer */
    if ((number < 0) || (number == window->buffer->number))
        return;
    
    /* search for buffer in the list */
    for (ptr_buffer = gui_buffers; ptr_buffer; ptr_buffer = ptr_buffer->next_buffer)
    {
        if ((ptr_buffer != window->buffer) && (number == ptr_buffer->number)
            && ptr_buffer->active)
        {
            gui_window_switch_to_buffer (window, ptr_buffer, 1);
            return;
        }
    }
}

/*
 * gui_buffer_set_active_buffer: set active buffer (when many buffers are
 *                               merged)
 */

void
gui_buffer_set_active_buffer (struct t_gui_buffer *buffer)
{
    struct t_gui_buffer *ptr_buffer;
    
    for (ptr_buffer = gui_buffers; ptr_buffer;
         ptr_buffer = ptr_buffer->next_buffer)
    {
        if (ptr_buffer->number == buffer->number)
            ptr_buffer->active = (ptr_buffer == buffer) ? 1 : 0;
    }
}

/*
 * gui_buffer_get_next_active_buffer: get next active buffer (when many buffers
 *                                    are merged)
 */

struct t_gui_buffer *
gui_buffer_get_next_active_buffer (struct t_gui_buffer *buffer)
{
    struct t_gui_buffer *ptr_buffer;
    
    if (buffer->next_buffer
        && (buffer->next_buffer->number == buffer->number))
        return buffer->next_buffer;
    else
    {
        for (ptr_buffer = gui_buffers; ptr_buffer;
             ptr_buffer = ptr_buffer->next_buffer)
        {
            if ((ptr_buffer != buffer)
                && (ptr_buffer->number == buffer->number))
            {
                return ptr_buffer;
            }
        }
    }
    return buffer;
}

/*
 * gui_buffer_move_to_number: move a buffer to another number
 */

void
gui_buffer_move_to_number (struct t_gui_buffer *buffer, int number)
{
    struct t_gui_buffer *ptr_first_buffer, *ptr_last_buffer, *ptr_buffer;
    struct t_gui_buffer *ptr_buffer_pos;
    char buf1_str[16], buf2_str[16], *argv[2] = { NULL, NULL };
    
    /* if only one buffer then return */
    if (gui_buffers == last_gui_buffer)
        return;
    
    if (number < 1)
        number = 1;
    
    /* buffer number is already ok ? */
    if (number == buffer->number)
        return;
    
    ptr_first_buffer = NULL;
    ptr_last_buffer = NULL;
    for (ptr_buffer = gui_buffers; ptr_buffer;
         ptr_buffer = ptr_buffer->next_buffer)
    {
        if (ptr_buffer->number == buffer->number)
        {
            if (!ptr_first_buffer)
                ptr_first_buffer = ptr_buffer;
            ptr_last_buffer = ptr_buffer;
        }
    }
    
    /* error when looking for buffers */
    if (!ptr_first_buffer || !ptr_last_buffer)
        return;
    
    /* if group of buffers found is all buffers, then we can't move buffers! */
    if ((ptr_first_buffer == gui_buffers) && (ptr_last_buffer == last_gui_buffer))
        return;
    
    snprintf (buf2_str, sizeof (buf2_str) - 1, "%d", buffer->number);
    
    /* remove buffer(s) from list */
    if (ptr_first_buffer == gui_buffers)
    {
        gui_buffers = ptr_last_buffer->next_buffer;
        gui_buffers->prev_buffer = NULL;
    }
    else if (ptr_last_buffer == last_gui_buffer)
    {
        last_gui_buffer = ptr_first_buffer->prev_buffer;
        last_gui_buffer->next_buffer = NULL;
    }
    if (ptr_first_buffer->prev_buffer)
        (ptr_first_buffer->prev_buffer)->next_buffer = ptr_last_buffer->next_buffer;
    if (ptr_last_buffer->next_buffer)
        (ptr_last_buffer->next_buffer)->prev_buffer = ptr_first_buffer->prev_buffer;
    
    /* compute "number - 1" for all buffers after buffer(s) */
    for (ptr_buffer = ptr_last_buffer->next_buffer; ptr_buffer;
         ptr_buffer = ptr_buffer->next_buffer)
    {
        ptr_buffer->number--;
    }
    
    if (number == 1)
    {
        for (ptr_buffer = ptr_first_buffer; ptr_buffer;
             ptr_buffer = ptr_buffer->next_buffer)
        {
            ptr_buffer->number = 1;
            if (ptr_buffer == ptr_last_buffer)
                break;
        }
        gui_buffers->prev_buffer = ptr_last_buffer;
        ptr_first_buffer->prev_buffer = NULL;
        ptr_last_buffer->next_buffer = gui_buffers;
        gui_buffers = ptr_first_buffer;
    }
    else
    {
        /* search for new position in the list */
        for (ptr_buffer_pos = gui_buffers; ptr_buffer_pos;
             ptr_buffer_pos = ptr_buffer_pos->next_buffer)
        {
            if (ptr_buffer_pos->number >= number)
                break;
        }
        if (ptr_buffer_pos)
        {
            /* insert before buffer found */
            for (ptr_buffer = ptr_first_buffer; ptr_buffer;
                 ptr_buffer = ptr_buffer->next_buffer)
            {
                ptr_buffer->number = ptr_buffer_pos->number;
                if (ptr_buffer == ptr_last_buffer)
                    break;
            }
            ptr_first_buffer->prev_buffer = ptr_buffer_pos->prev_buffer;
            ptr_last_buffer->next_buffer = ptr_buffer_pos;
            if (ptr_buffer_pos->prev_buffer)
                (ptr_buffer_pos->prev_buffer)->next_buffer = ptr_first_buffer;
            ptr_buffer_pos->prev_buffer = ptr_last_buffer;
        }
        else
        {
            /* number not found (too big)? => add to end */
            for (ptr_buffer = ptr_first_buffer; ptr_buffer;
                 ptr_buffer = ptr_buffer->next_buffer)
            {
                ptr_buffer->number = last_gui_buffer->number + 1;
                if (ptr_buffer == ptr_last_buffer)
                    break;
            }
            ptr_first_buffer->prev_buffer = last_gui_buffer;
            ptr_last_buffer->next_buffer = NULL;
            last_gui_buffer->next_buffer = ptr_first_buffer;
            last_gui_buffer = ptr_last_buffer;
        }
    }
    
    /* compute "number + 1" for all buffers after buffer(s) */
    for (ptr_buffer = ptr_last_buffer->next_buffer; ptr_buffer;
         ptr_buffer = ptr_buffer->next_buffer)
    {
        ptr_buffer->number++;
    }
    
    snprintf (buf1_str, sizeof (buf1_str) - 1, "%d", buffer->number);
    argv[0] = buf1_str;
    argv[1] = buf2_str;
    
    hook_signal_send ("buffer_moved",
                      WEECHAT_HOOK_SIGNAL_POINTER, buffer);
}

/*
 * gui_buffer_merge: merge a buffer to another buffer
 */

void
gui_buffer_merge (struct t_gui_buffer *buffer,
                  struct t_gui_buffer *target_buffer)
{
    struct t_gui_buffer *ptr_buffer;
    int target_number;
    
    /* if only one buffer then return */
    if (gui_buffers == last_gui_buffer)
        return;
    
    if ((buffer == target_buffer) || (buffer->number == target_buffer->number))
        return;
    
    if (gui_buffer_count_merged_buffers (buffer->number) > 1)
        gui_buffer_unmerge (buffer, -1);
    
    /* check if current buffer and target buffers are type "formatted" */
    if ((buffer->type != GUI_BUFFER_TYPE_FORMATTED)
        || (target_buffer->type != GUI_BUFFER_TYPE_FORMATTED))
    {
        gui_chat_printf (NULL,
                         _("%sError: it is only possible to merge buffers "
                           "with formatted content"),
                         gui_chat_prefix[GUI_CHAT_PREFIX_ERROR]);
        return;
    }
    
    /* move buffer immediately after number we want to merge to */
    target_number = (buffer->number < target_buffer->number) ?
        target_buffer->number : target_buffer->number + 1;
    if (buffer->number != target_number)
        gui_buffer_move_to_number (buffer, target_number);
    
    /* change number */
    buffer->number--;
    
    /* mix lines */
    gui_line_mix_buffers (buffer);
    
    /* set buffer as active in merged buffers group */
    gui_buffer_set_active_buffer (buffer);
    
    /* compute "number - 1" for next buffers */
    for (ptr_buffer = buffer->next_buffer; ptr_buffer;
         ptr_buffer = ptr_buffer->next_buffer)
    {
        ptr_buffer->number--;
    }
    
    gui_buffer_compute_num_displayed ();
    
    gui_window_ask_refresh (1);
    
    hook_signal_send ("buffer_merged",
                      WEECHAT_HOOK_SIGNAL_POINTER, buffer);
}

/*
 * gui_buffer_unmerge: unmerge a buffer from group of merged buffers
 *                     if number >= 1, then buffer is moved to this number,
 *                     otherwise it is moved to buffer->number + 1
 */

void
gui_buffer_unmerge (struct t_gui_buffer *buffer, int number)
{
    int num_merged;
    struct t_gui_buffer *ptr_buffer, *ptr_new_active_buffer;
    
    /* if only one buffer then return */
    if (gui_buffers == last_gui_buffer)
        return;
    
    ptr_new_active_buffer = NULL;
    
    num_merged = gui_buffer_count_merged_buffers (buffer->number);
    
    /* can't unmerge if buffer is not merged to at least one buffer */
    if (num_merged < 2)
        return;
    
    /* by default, we move buffer to buffer->number + 1 */
    if ((number < 1)  || (number == buffer->number))
    {
        number = buffer->number + 1;
    }
    else
    {
        if (number > last_gui_buffer->number + 1)
            number = last_gui_buffer->number + 1;
    }
    
    if (num_merged == 2)
    {
        /* only one buffer will remain, so it will not be merged any more */
        gui_line_mixed_free_all (buffer);
        gui_lines_free (buffer->mixed_lines);
        for (ptr_buffer = gui_buffers; ptr_buffer;
             ptr_buffer = ptr_buffer->next_buffer)
        {
            if (ptr_buffer->number == buffer->number)
            {
                ptr_buffer->active = 1;
                ptr_buffer->mixed_lines = NULL;
                ptr_buffer->lines = ptr_buffer->own_lines;
            }
        }
    }
    else
    {
        /* remove this buffer from mixed_lines, but keep other buffers merged */
        ptr_new_active_buffer = gui_buffer_get_next_active_buffer (buffer);
        if (ptr_new_active_buffer)
            gui_buffer_set_active_buffer (ptr_new_active_buffer);
        gui_line_mixed_free_buffer (buffer);
        buffer->mixed_lines = NULL;
        buffer->lines = buffer->own_lines;
    }
    
    /* remove buffer from list */
    if (buffer->prev_buffer)
        (buffer->prev_buffer)->next_buffer = buffer->next_buffer;
    if (buffer->next_buffer)
        (buffer->next_buffer)->prev_buffer = buffer->prev_buffer;
    if (gui_buffers == buffer)
        gui_buffers = buffer->next_buffer;
    if (last_gui_buffer == buffer)
        last_gui_buffer = buffer->prev_buffer;
    
    /* move buffer */
    for (ptr_buffer = gui_buffers; ptr_buffer;
         ptr_buffer = ptr_buffer->next_buffer)
    {
        if (ptr_buffer->number >= number)
            break;
    }
    if (ptr_buffer)
    {
        /* insert buffer into the list (before buffer found) */
        buffer->prev_buffer = ptr_buffer->prev_buffer;
        buffer->next_buffer = ptr_buffer;
        if (ptr_buffer->prev_buffer)
            (ptr_buffer->prev_buffer)->next_buffer = buffer;
        else
            gui_buffers = buffer;
        ptr_buffer->prev_buffer = buffer;
    }
    else
    {
        /* add buffer to the end */
        buffer->prev_buffer = last_gui_buffer;
        buffer->next_buffer = NULL;
        last_gui_buffer->next_buffer = buffer;
        last_gui_buffer = buffer;
    }
    buffer->active = 1;
    buffer->number = number;
    for (ptr_buffer = buffer->next_buffer; ptr_buffer;
         ptr_buffer = ptr_buffer->next_buffer)
    {
        ptr_buffer->number++;
    }
    
    gui_buffer_compute_num_displayed ();

    if (ptr_new_active_buffer)
    {
        gui_line_compute_prefix_max_length (ptr_new_active_buffer->mixed_lines);
        gui_line_compute_buffer_max_length (ptr_new_active_buffer,
                                            ptr_new_active_buffer->mixed_lines);
    }
    
    gui_window_ask_refresh (1);
    
    hook_signal_send ("buffer_unmerged",
                      WEECHAT_HOOK_SIGNAL_POINTER, buffer);
}

/*
 * gui_buffer_visited_search: search a visited buffer in list of visited buffers
 */

struct t_gui_buffer_visited *
gui_buffer_visited_search (struct t_gui_buffer *buffer)
{
    struct t_gui_buffer_visited *ptr_buffer_visited;
    
    if (!buffer)
        return NULL;
    
    for (ptr_buffer_visited = gui_buffers_visited; ptr_buffer_visited;
         ptr_buffer_visited = ptr_buffer_visited->next_buffer)
    {
        if (ptr_buffer_visited->buffer == buffer)
            return ptr_buffer_visited;
    }
    
    /* visited buffer not found */
    return NULL;
}

/*
 * gui_buffer_visited_search_by_number: search a visited buffer in list of
 *                                      visited buffers
 */

struct t_gui_buffer_visited *
gui_buffer_visited_search_by_number (int number)
{
    struct t_gui_buffer_visited *ptr_buffer_visited;
    int i;
    
    if ((number < 0) || (number >= gui_buffers_visited_count))
        return NULL;
    
    i = 0;
    for (ptr_buffer_visited = gui_buffers_visited; ptr_buffer_visited;
         ptr_buffer_visited = ptr_buffer_visited->next_buffer)
    {
        if (i == number)
            return ptr_buffer_visited;
        i++;
    }
    
    /* visited buffer not found (should never be reached) */
    return NULL;
}

/*
 * gui_buffer_visited_remove: remove a visited buffer from list of visited buffers
 */

void
gui_buffer_visited_remove (struct t_gui_buffer_visited *buffer_visited)
{
    if (!buffer_visited)
        return;
    
    /* remove visited buffer from list */
    if (buffer_visited->prev_buffer)
        (buffer_visited->prev_buffer)->next_buffer = buffer_visited->next_buffer;
    if (buffer_visited->next_buffer)
        (buffer_visited->next_buffer)->prev_buffer = buffer_visited->prev_buffer;
    if (gui_buffers_visited == buffer_visited)
        gui_buffers_visited = buffer_visited->next_buffer;
    if (last_gui_buffer_visited == buffer_visited)
        last_gui_buffer_visited = buffer_visited->prev_buffer;
    
    free (buffer_visited);
    
    if (gui_buffers_visited_count > 0)
        gui_buffers_visited_count--;
    
    if (gui_buffers_visited_index >= gui_buffers_visited_count)
        gui_buffers_visited_index = -1;
}

/*
 * gui_buffer_visited_remove_by_buffer: remove a visited buffer from list of
 *                                      visited buffers
 */

void
gui_buffer_visited_remove_by_buffer (struct t_gui_buffer *buffer)
{
    struct t_gui_buffer_visited *buffer_visited;
    
    if (!buffer)
        return;

    buffer_visited = gui_buffer_visited_search (buffer);
    if (buffer_visited)
        gui_buffer_visited_remove (buffer_visited);
}

/*
 * gui_buffer_visited_remove_all: remove all visited buffers from list
 */

void
gui_buffer_visited_remove_all ()
{
    while (gui_buffers_visited)
    {
        gui_buffer_visited_remove (gui_buffers_visited);
    }
}

/*
 * gui_buffer_visited_add: add a visited buffer to list of visited buffers
 */

struct t_gui_buffer_visited *
gui_buffer_visited_add (struct t_gui_buffer *buffer)
{
    struct t_gui_buffer_visited *new_buffer_visited;
    
    if (!buffer)
        return NULL;
    
    new_buffer_visited = gui_buffer_visited_search (buffer);
    if (new_buffer_visited)
        gui_buffer_visited_remove (new_buffer_visited);
    
    /* remove old buffer(s) visited if list is too long */
    while (gui_buffers_visited_count > CONFIG_INTEGER(config_history_max_visited_buffers))
    {
        gui_buffer_visited_remove (gui_buffers_visited);
    }
    
    new_buffer_visited = malloc (sizeof (*new_buffer_visited));
    if (new_buffer_visited)
    {
        new_buffer_visited->buffer = buffer;
        
        new_buffer_visited->prev_buffer = last_gui_buffer_visited;
        new_buffer_visited->next_buffer = NULL;
        if (gui_buffers_visited)
            last_gui_buffer_visited->next_buffer = new_buffer_visited;
        else
            gui_buffers_visited = new_buffer_visited;
        last_gui_buffer_visited = new_buffer_visited;
        
        gui_buffers_visited_count++;
        gui_buffers_visited_index = -1;
    }
    
    return new_buffer_visited;
}

/*
 * gui_buffer_visited_get_index_previous: get index for previously visited buffer
 *                                        return -1 if there's no previously buffer
 *                                        in history, starting from current index
 */

int
gui_buffer_visited_get_index_previous ()
{
    if ((gui_buffers_visited_count < 2) || (gui_buffers_visited_index == 0))
        return -1;
    
    if (gui_buffers_visited_index < 0)
        return gui_buffers_visited_count - 2;
    else
        return gui_buffers_visited_index - 1;
}

/*
 * gui_buffer_visited_get_index_next: get index for next visited buffer
 *                                    return -1 if there's no next buffer
 *                                    in history, starting from current index
 */

int
gui_buffer_visited_get_index_next ()
{
    if ((gui_buffers_visited_count < 2)
        || (gui_buffers_visited_index >= gui_buffers_visited_count - 1))
        return -1;
    
    return gui_buffers_visited_index + 1;
}

/*
 * gui_buffer_add_to_infolist: add a buffer in an infolist
 *                             return 1 if ok, 0 if error
 */

int
gui_buffer_add_to_infolist (struct t_infolist *infolist,
                            struct t_gui_buffer *buffer)
{
    struct t_infolist_item *ptr_item;
    struct t_gui_key *ptr_key;
    struct t_gui_buffer_local_var *ptr_local_var;
    char option_name[64];
    int i;
    
    if (!infolist || !buffer)
        return 0;
    
    ptr_item = infolist_new_item (infolist);
    if (!ptr_item)
        return 0;

    if (!infolist_new_var_pointer (ptr_item, "pointer", buffer))
        return 0;
    if (!infolist_new_var_integer (ptr_item, "current_buffer",
                                   (gui_current_window->buffer == buffer) ? 1 : 0))
        return 0;
    if (!infolist_new_var_pointer (ptr_item, "plugin", buffer->plugin))
        return 0;
    if (!infolist_new_var_string (ptr_item, "plugin_name",
                                  plugin_get_name (buffer->plugin)))
        return 0;
    if (!infolist_new_var_integer (ptr_item, "number", buffer->number))
        return 0;
    if (!infolist_new_var_string (ptr_item, "name", buffer->name))
        return 0;
    if (!infolist_new_var_string (ptr_item, "short_name", buffer->short_name))
        return 0;
    if (!infolist_new_var_integer (ptr_item, "type", buffer->type))
        return 0;
    if (!infolist_new_var_integer (ptr_item, "notify", buffer->notify))
        return 0;
    if (!infolist_new_var_integer (ptr_item, "num_displayed", buffer->num_displayed))
        return 0;
    if (!infolist_new_var_integer (ptr_item, "active", buffer->active))
        return 0;
    if (!infolist_new_var_integer (ptr_item, "print_hooks_enabled", buffer->print_hooks_enabled))
        return 0;
    if (!infolist_new_var_integer (ptr_item, "first_line_not_read", buffer->lines->first_line_not_read))
        return 0;
    if (!infolist_new_var_integer (ptr_item, "lines_hidden", buffer->lines->lines_hidden))
        return 0;
    if (!infolist_new_var_integer (ptr_item, "prefix_max_length", buffer->lines->prefix_max_length))
        return 0;
    if (!infolist_new_var_integer (ptr_item, "time_for_each_line", buffer->time_for_each_line))
        return 0;
    if (!infolist_new_var_integer (ptr_item, "nicklist_case_sensitive", buffer->nicklist_case_sensitive))
        return 0;
    if (!infolist_new_var_integer (ptr_item, "nicklist_display_groups", buffer->nicklist_display_groups))
        return 0;
    if (!infolist_new_var_string (ptr_item, "title", buffer->title))
        return 0;
    if (!infolist_new_var_integer (ptr_item, "input", buffer->input))
        return 0;
    if (!infolist_new_var_integer (ptr_item, "input_get_unknown_commands", buffer->input_get_unknown_commands))
        return 0;
    if (!infolist_new_var_string (ptr_item, "input_buffer", buffer->input_buffer))
        return 0;
    if (!infolist_new_var_integer (ptr_item, "input_buffer_alloc", buffer->input_buffer_alloc))
        return 0;
    if (!infolist_new_var_integer (ptr_item, "input_buffer_size", buffer->input_buffer_size))
        return 0;
    if (!infolist_new_var_integer (ptr_item, "input_buffer_length", buffer->input_buffer_length))
        return 0;
    if (!infolist_new_var_integer (ptr_item, "input_buffer_pos", buffer->input_buffer_pos))
        return 0;
    if (!infolist_new_var_integer (ptr_item, "input_buffer_1st_display", buffer->input_buffer_1st_display))
        return 0;
    if (!infolist_new_var_integer (ptr_item, "text_search", buffer->text_search))
        return 0;
    if (!infolist_new_var_integer (ptr_item, "text_search_exact", buffer->text_search_exact))
        return 0;
    if (!infolist_new_var_integer (ptr_item, "text_search_found", buffer->text_search_found))
        return 0;
    if (!infolist_new_var_string (ptr_item, "text_search_input", buffer->text_search_input))
        return 0;
    if (!infolist_new_var_string (ptr_item, "highlight_words", buffer->highlight_words))
        return 0;
    if (!infolist_new_var_string (ptr_item, "highlight_tags", buffer->highlight_tags))
        return 0;
    i = 0;
    for (ptr_key = buffer->keys; ptr_key; ptr_key = ptr_key->next_key)
    {
        snprintf (option_name, sizeof (option_name), "key_%05d", i);
        if (!infolist_new_var_string (ptr_item, option_name, ptr_key->key))
            return 0;
        snprintf (option_name, sizeof (option_name), "key_command_%05d", i);
        if (!infolist_new_var_string (ptr_item, option_name, ptr_key->command))
            return 0;
        i++;
    }
    i = 0;
    for (ptr_local_var = buffer->local_variables; ptr_local_var;
         ptr_local_var = ptr_local_var->next_var)
    {
        snprintf (option_name, sizeof (option_name), "localvar_name_%05d", i);
        if (!infolist_new_var_string (ptr_item, option_name,
                                      ptr_local_var->name))
            return 0;
        snprintf (option_name, sizeof (option_name), "localvar_value_%05d", i);
        if (!infolist_new_var_string (ptr_item, option_name,
                                      ptr_local_var->value))
            return 0;
        i++;
    }
    
    return 1;
}

/*
 * gui_buffer_dump_hexa: dump content of buffer as hexa data in log file
 */

void
gui_buffer_dump_hexa (struct t_gui_buffer *buffer)
{
    struct t_gui_line *ptr_line;
    int num_line, msg_pos;
    char *message_without_colors, *tags;
    char hexa[(16 * 3) + 1], ascii[(16 * 2) + 1];
    int hexa_pos, ascii_pos;
    
    log_printf ("[buffer dump hexa (addr:0x%lx)]", buffer);
    num_line = 1;
    for (ptr_line = buffer->lines->first_line; ptr_line;
         ptr_line = ptr_line->next_line)
    {
        /* display line without colors */
        message_without_colors = (ptr_line->data->message) ?
            gui_color_decode (ptr_line->data->message, NULL) : NULL;
        log_printf ("");
        log_printf ("  line %d: %s",
                    num_line,
                    (message_without_colors) ?
                    message_without_colors : "(null)");
        if (message_without_colors)
            free (message_without_colors);
        tags = string_build_with_split_string ((const char **)ptr_line->data->tags_array,
                                               ",");
        log_printf ("  tags: %s", (tags) ? tags : "(none)");
        if (tags)
            free (tags);
        
        /* display raw message for line */
        if (ptr_line->data->message)
        {
            log_printf ("");
            log_printf ("  raw data for line %d (with color codes):",
                        num_line);
            msg_pos = 0;
            hexa_pos = 0;
            ascii_pos = 0;
            while (ptr_line->data->message[msg_pos])
            {
                snprintf (hexa + hexa_pos, 4, "%02X ",
                          (unsigned char)(ptr_line->data->message[msg_pos]));
                hexa_pos += 3;
                snprintf (ascii + ascii_pos, 3, "%c ",
                          ((((unsigned char)ptr_line->data->message[msg_pos]) < 32)
                           || (((unsigned char)ptr_line->data->message[msg_pos]) > 127)) ?
                          '.' : (unsigned char)(ptr_line->data->message[msg_pos]));
                ascii_pos += 2;
                if (ascii_pos == 32)
                {
                    log_printf ("    %-48s  %s", hexa, ascii);
                    hexa_pos = 0;
                    ascii_pos = 0;
                }
                msg_pos++;
            }
            if (ascii_pos > 0)
                log_printf ("    %-48s  %s", hexa, ascii);
        }
        
        num_line++;
    }
}

/*
 * gui_buffer_print_log: print buffer infos in log (usually for crash dump)
 */

void
gui_buffer_print_log ()
{
    struct t_gui_buffer *ptr_buffer;
    struct t_gui_buffer_local_var *ptr_local_var;
    struct t_gui_line *ptr_line;
    struct t_gui_buffer_visited *ptr_buffer_visited;
    char *tags;
    int num;
    
    for (ptr_buffer = gui_buffers; ptr_buffer;
         ptr_buffer = ptr_buffer->next_buffer)
    {
        log_printf ("");
        log_printf ("[buffer (addr:0x%lx)]", ptr_buffer);
        log_printf ("  plugin . . . . . . . . : 0x%lx ('%s')",
                    ptr_buffer->plugin, plugin_get_name (ptr_buffer->plugin));
        log_printf ("  plugin_name_for_upgrade: '%s'",  ptr_buffer->plugin_name_for_upgrade);
        log_printf ("  number . . . . . . . . : %d",    ptr_buffer->number);
        log_printf ("  layout_number. . . . . : %d",    ptr_buffer->layout_number);
        log_printf ("  name . . . . . . . . . : '%s'",  ptr_buffer->name);
        log_printf ("  short_name . . . . . . : '%s'",  ptr_buffer->short_name);
        log_printf ("  type . . . . . . . . . : %d",    ptr_buffer->type);
        log_printf ("  notify . . . . . . . . : %d",    ptr_buffer->notify);
        log_printf ("  num_displayed. . . . . : %d",    ptr_buffer->num_displayed);
        log_printf ("  active . . . . . . . . : %d",    ptr_buffer->active);
        log_printf ("  print_hooks_enabled. . : %d",    ptr_buffer->print_hooks_enabled);
        log_printf ("  close_callback . . . . : 0x%lx", ptr_buffer->close_callback);
        log_printf ("  close_callback_data. . : 0x%lx", ptr_buffer->close_callback_data);
        log_printf ("  title. . . . . . . . . : '%s'",  ptr_buffer->title);
        log_printf ("  own_lines. . . . . . . : 0x%lx", ptr_buffer->own_lines);
        gui_lines_print_log (ptr_buffer->own_lines);
        log_printf ("  mixed_lines. . . . . . : 0x%lx", ptr_buffer->mixed_lines);
        gui_lines_print_log (ptr_buffer->mixed_lines);
        log_printf ("  lines. . . . . . . . . : 0x%lx", ptr_buffer->lines);
        log_printf ("  time_for_each_line . . : %d",    ptr_buffer->time_for_each_line);
        log_printf ("  chat_refresh_needed. . : %d",    ptr_buffer->chat_refresh_needed);
        log_printf ("  nicklist . . . . . . . : %d",    ptr_buffer->nicklist);
        log_printf ("  nicklist_case_sensitive: %d",    ptr_buffer->nicklist_case_sensitive);
        log_printf ("  nicklist_root. . . . . : 0x%lx", ptr_buffer->nicklist_root);
        log_printf ("  nicklist_max_length. . : %d",    ptr_buffer->nicklist_max_length);
        log_printf ("  nicklist_display_groups: %d",    ptr_buffer->nicklist_display_groups);
        log_printf ("  nicklist_visible_count.: %d",    ptr_buffer->nicklist_visible_count);
        log_printf ("  input. . . . . . . . . : %d",    ptr_buffer->input);
        log_printf ("  input_callback . . . . : 0x%lx", ptr_buffer->input_callback);
        log_printf ("  input_callback_data. . : 0x%lx", ptr_buffer->input_callback_data);
        log_printf ("  input_get_unknown_cmd. : %d",    ptr_buffer->input_get_unknown_commands);
        log_printf ("  input_buffer . . . . . : '%s'",  ptr_buffer->input_buffer);
        log_printf ("  input_buffer_alloc . . : %d",    ptr_buffer->input_buffer_alloc);
        log_printf ("  input_buffer_size. . . : %d",    ptr_buffer->input_buffer_size);
        log_printf ("  input_buffer_length. . : %d",    ptr_buffer->input_buffer_length);
        log_printf ("  input_buffer_pos . . . : %d",    ptr_buffer->input_buffer_pos);
        log_printf ("  input_buffer_1st_disp. : %d",    ptr_buffer->input_buffer_1st_display);
        log_printf ("  completion . . . . . . : 0x%lx", ptr_buffer->completion);
        log_printf ("  history. . . . . . . . : 0x%lx", ptr_buffer->history);
        log_printf ("  last_history . . . . . : 0x%lx", ptr_buffer->last_history);
        log_printf ("  ptr_history. . . . . . : 0x%lx", ptr_buffer->ptr_history);
        log_printf ("  num_history. . . . . . : %d",    ptr_buffer->num_history);
        log_printf ("  text_search. . . . . . : %d",    ptr_buffer->text_search);
        log_printf ("  text_search_exact. . . : %d",    ptr_buffer->text_search_exact);
        log_printf ("  text_search_found. . . : %d",    ptr_buffer->text_search_found);
        log_printf ("  text_search_input. . . : '%s'",  ptr_buffer->text_search_input);
        log_printf ("  highlight_words. . . . : '%s'",  ptr_buffer->highlight_words);
        log_printf ("  highlight_tags . . . . : '%s'",  ptr_buffer->highlight_tags);
        log_printf ("  highlight_tags_count . : %d",    ptr_buffer->highlight_tags_count);
        log_printf ("  highlight_tags_array . : 0x%lx", ptr_buffer->highlight_tags_array);
        log_printf ("  keys . . . . . . . . . : 0x%lx", ptr_buffer->keys);
        log_printf ("  last_key . . . . . . . : 0x%lx", ptr_buffer->last_key);
        log_printf ("  local_variables. . . . : 0x%lx", ptr_buffer->local_variables);
        log_printf ("  last_local_var . . . . : 0x%lx", ptr_buffer->last_local_var);
        log_printf ("  prev_buffer. . . . . . : 0x%lx", ptr_buffer->prev_buffer);
        log_printf ("  next_buffer. . . . . . : 0x%lx", ptr_buffer->next_buffer);
        
        if (ptr_buffer->keys)
        {
            log_printf ("");
            log_printf ("  => keys = 0x%lx, last_key = 0x%lx:",
                        ptr_buffer->keys, ptr_buffer->last_key);
            gui_keyboard_print_log (ptr_buffer);
        }

        if (ptr_buffer->local_variables)
        {
            log_printf ("");
            log_printf ("  => local_variables = 0x%lx, last_local_var = 0x%lx:",
                        ptr_buffer->local_variables, ptr_buffer->last_local_var);
            for (ptr_local_var = ptr_buffer->local_variables; ptr_local_var;
                 ptr_local_var = ptr_local_var->next_var)
            {
                log_printf ("");
                log_printf ("    [local variable (addr:0x%lx)]",
                            ptr_local_var);
                log_printf ("      name . . . . . . . : '%s'", ptr_local_var->name);
                log_printf ("      value. . . . . . . : '%s'", ptr_local_var->value);
            }
        }
        
        log_printf ("");
        log_printf ("  => nicklist_root (addr:0x%lx):", ptr_buffer->nicklist_root);
        gui_nicklist_print_log (ptr_buffer->nicklist_root, 0);
        
        log_printf ("");
        log_printf ("  => last 100 lines:");
        num = 0;
        ptr_line = ptr_buffer->lines->last_line;
        while (ptr_line && (num < 100))
        {
            num++;
            ptr_line = ptr_line->prev_line;
        }
        if (!ptr_line)
            ptr_line = ptr_buffer->lines->first_line;
        else
            ptr_line = ptr_line->next_line;
        
        while (ptr_line)
        {
            num--;
            tags = string_build_with_split_string ((const char **)ptr_line->data->tags_array,
                                                   ",");
            log_printf ("       line N-%05d: y:%d, str_time:'%s', tags:'%s', "
                        "displayed:%d, highlight:%d, refresh_needed:%d, prefix:'%s'",
                        num, ptr_line->data->y, ptr_line->data->str_time,
                        (tags) ? tags  : "",
                        (int)(ptr_line->data->displayed),
                        (int)(ptr_line->data->highlight),
                        (int)(ptr_line->data->refresh_needed),
                        ptr_line->data->prefix);
            log_printf ("                     data: '%s'",
                        ptr_line->data->message);
            if (tags)
                free (tags);
            
            ptr_line = ptr_line->next_line;
        }
        
        if (ptr_buffer->completion)
        {
            log_printf ("");
            gui_completion_print_log (ptr_buffer->completion);
        }
    }
    
    log_printf ("");
    log_printf ("[visited buffers]");
    num = 1;
    for (ptr_buffer_visited = gui_buffers_visited; ptr_buffer_visited;
         ptr_buffer_visited = ptr_buffer_visited->next_buffer)
    {
        log_printf ("  #%d:", num);
        log_printf ("    buffer . . . . . . . . : 0x%lx", ptr_buffer_visited->buffer);
        log_printf ("    prev_buffer. . . . . . : 0x%lx", ptr_buffer_visited->prev_buffer);
        log_printf ("    next_buffer. . . . . . : 0x%lx", ptr_buffer_visited->next_buffer);
        num++;
    }
}

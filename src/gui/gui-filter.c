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

/* gui-filter.c: filter functions, used by all GUI */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <string.h>
#include <regex.h>

#include "../core/weechat.h"
#include "../core/wee-hook.h"
#include "../core/wee-infolist.h"
#include "../core/wee-log.h"
#include "../core/wee-string.h"
#include "../plugins/plugin.h"
#include "gui-filter.h"
#include "gui-buffer.h"
#include "gui-line.h"


struct t_gui_filter *gui_filters = NULL;           /* first filter          */
struct t_gui_filter *last_gui_filter = NULL;       /* last filter           */
int gui_filters_enabled = 1;                       /* filters enabled?      */


/*
 * gui_filter_line_has_tag_no_filter: return 1 if line has tag "no_filter",
 *                                    which means that line should never
 *                                    been filtered (always displayed)
 */

int
gui_filter_line_has_tag_no_filter (struct t_gui_line *line)
{
    int i;
    
    for (i = 0; i < line->data->tags_count; i++)
    {
        if (strcmp (line->data->tags_array[i], GUI_FILTER_TAG_NO_FILTER) == 0)
            return 1;
    }
    
    /* tag not found, line may be filtered */
    return 0;
}

/*
 * gui_filter_check_line: return 1 if a line should be displayed, or
 *                        0 if line is hidden (tag or regex found)
 */

int
gui_filter_check_line (struct t_gui_buffer *buffer, struct t_gui_line *line)
{
    struct t_gui_filter *ptr_filter;
    const char *buffer_plugin_name;
    
    /* line is always displayed if filters are disabled */
    if (!gui_filters_enabled)
        return 1;
    
    if (gui_filter_line_has_tag_no_filter (line))
        return 1;

    buffer_plugin_name = (!buffer->plugin && buffer->plugin_name_for_upgrade) ?
        buffer->plugin_name_for_upgrade : plugin_get_name (buffer->plugin);
    
    for (ptr_filter = gui_filters; ptr_filter;
         ptr_filter = ptr_filter->next_filter)
    {
        if (ptr_filter->enabled)
        {
            /* check plugin and buffer names */
            if ((!ptr_filter->plugin_name
                 || (string_strcasecmp (buffer_plugin_name, ptr_filter->plugin_name) == 0))
                && string_match (buffer->name, ptr_filter->buffer_name, 0))
            {
                if ((strcmp (ptr_filter->tags, "*") == 0)
                    || (gui_line_match_tags (line,
                                             ptr_filter->tags_count,
                                             ptr_filter->tags_array)))
                {
                    /* check line with regex */
                    if (!ptr_filter->regex_prefix && !ptr_filter->regex_message)
                        return 0;
                    
                    if (gui_line_match_regex (line,
                                              ptr_filter->regex_prefix,
                                              ptr_filter->regex_message))
                        return 0;
                }
            }
        }
    }
    
    /* no tag or regex matching, then line is displayed */
    return 1;
}

/*
 * gui_filter_buffer: filter a buffer, using message filters
 */

void
gui_filter_buffer (struct t_gui_buffer *buffer)
{
    struct t_gui_line *ptr_line;
    int line_displayed, lines_hidden;
    
    lines_hidden = 0;
    
    buffer->lines->prefix_max_length = 0;
    
    for (ptr_line = buffer->lines->first_line; ptr_line;
         ptr_line = ptr_line->next_line)
    {
        line_displayed = gui_filter_check_line (buffer, ptr_line);
        
        if (line_displayed
            && (ptr_line->data->prefix_length > buffer->lines->prefix_max_length))
        {
            buffer->lines->prefix_max_length = ptr_line->data->prefix_length;
        }
        
        /* force chat refresh if at least one line changed */
        if (ptr_line->data->displayed != line_displayed)
            gui_buffer_ask_chat_refresh (buffer, 2);
        
        ptr_line->data->displayed = line_displayed;
        
        if (!line_displayed)
            lines_hidden = 1;
    }
    
    if (buffer->lines->lines_hidden != lines_hidden)
    {
        buffer->lines->lines_hidden = lines_hidden;
        hook_signal_send ("buffer_lines_hidden",
                          WEECHAT_HOOK_SIGNAL_POINTER, buffer);
    }
}

/*
 * gui_filter_all_buffers: filter all buffers, using message filters
 */

void
gui_filter_all_buffers ()
{
    struct t_gui_buffer *ptr_buffer;
    
    for (ptr_buffer = gui_buffers; ptr_buffer;
         ptr_buffer = ptr_buffer->next_buffer)
    {
        gui_filter_buffer (ptr_buffer);
    }
}

/*
 * gui_filter_global_enable: enable message filtering
 */

void
gui_filter_global_enable ()
{
    if (!gui_filters_enabled)
    {
        gui_filters_enabled = 1;
        gui_filter_all_buffers ();
        hook_signal_send ("filters_enabled",
                          WEECHAT_HOOK_SIGNAL_STRING, NULL);
    }
}

/*
 * gui_filter_global_disable: disable message filtering
 */

void
gui_filter_global_disable ()
{
    if (gui_filters_enabled)
    {
        gui_filters_enabled = 0;
        gui_filter_all_buffers ();
        hook_signal_send ("filters_disabled",
                          WEECHAT_HOOK_SIGNAL_STRING, NULL);
    }
}

/*
 * gui_filter_enable: enable a filter
 */

void
gui_filter_enable (struct t_gui_filter *filter)
{
    if (filter && !filter->enabled)
    {
        filter->enabled = 1;
        gui_filter_all_buffers ();
    }
}

/*
 * gui_filter_disable: disable a filter
 */

void
gui_filter_disable (struct t_gui_filter *filter)
{
    if (filter && filter->enabled)
    {
        filter->enabled = 0;
        gui_filter_all_buffers ();
    }
}

/*
 * gui_filter_search: search a filter
 */

struct t_gui_filter *
gui_filter_search (const char *buffer_name, const char *tags, const char *regex)
{
    struct t_gui_filter *ptr_filter;
    
    for (ptr_filter = gui_filters; ptr_filter;
         ptr_filter = ptr_filter->next_filter)
    {
        if ((strcmp (ptr_filter->buffer_name, buffer_name) == 0)
            && (strcmp (ptr_filter->tags, tags) == 0)
            && (strcmp (ptr_filter->regex, regex) == 0))
            return ptr_filter;
    }
    
    /* filter not found */
    return NULL;
}

/*
 * gui_filter_search_by_name: search a filter by name
 */

struct t_gui_filter *
gui_filter_search_by_name (const char *name)
{
    struct t_gui_filter *ptr_filter;
    
    for (ptr_filter = gui_filters; ptr_filter;
         ptr_filter = ptr_filter->next_filter)
    {
        if (strcmp (ptr_filter->name, name) == 0)
            return ptr_filter;
    }
    
    /* filter not found */
    return NULL;
}

/*
 * gui_filter_new: create a new filter
 */

struct t_gui_filter *
gui_filter_new (int enabled, const char *name, const char *buffer_name,
                const char *tags, const char *regex)
{
    struct t_gui_filter *new_filter;
    regex_t *regex1, *regex2;
    char *pos_tab, *pos_point, *regex_prefix;
    const char *pos_regex_message;

    if (!name || !buffer_name || !tags || !regex)
        return NULL;
    
    if (gui_filter_search_by_name (name))
        return NULL;
    
    regex1 = NULL;
    regex2 = NULL;
    if (strcmp (regex, "*") != 0)
    {
        pos_tab = strstr (regex, "\\t");
        if (pos_tab)
        {
            regex_prefix = string_strndup (regex, pos_tab - regex);
            pos_regex_message = pos_tab + 2;
        }
        else
        {
            regex_prefix = NULL;
            pos_regex_message = regex;
        }
        
        if (regex_prefix)
        {
            regex1 = malloc (sizeof (*regex1));
            if (regex1)
            {
                if (regcomp (regex1, regex_prefix,
                             REG_NOSUB | REG_ICASE) != 0)
                {
                    free (regex_prefix);
                    free (regex1);
                    return NULL;
                }
            }
        }
        
        regex2 = malloc (sizeof (*regex2));
        if (regex2)
        {
            if (regcomp (regex2, pos_regex_message,
                         REG_NOSUB | REG_ICASE) != 0)
            {
                if (regex_prefix)
                    free (regex_prefix);
                if (regex1)
                    free (regex1);
                free (regex2);
                return NULL;
            }
        }
        
        if (regex_prefix)
            free (regex_prefix);
    }
    
    /* create new filter */
    new_filter = malloc (sizeof (*new_filter));
    if (new_filter)
    {
        /* init filter */
        new_filter->enabled = enabled;
        new_filter->name = strdup (name);
        new_filter->plugin_name = NULL;
        if (buffer_name)
        {
            pos_point = strchr (buffer_name, '.');
            if (pos_point)
            {
                new_filter->plugin_name = string_strndup (buffer_name, pos_point - buffer_name);
                new_filter->buffer_name = strdup (pos_point + 1);
            }
            else
                new_filter->buffer_name = strdup (buffer_name);
        }
        else
        {
            new_filter->buffer_name = strdup ("*");
        }
        if (tags)
        {
            new_filter->tags = (tags) ? strdup (tags) : NULL;
            new_filter->tags_array = string_split (tags, ",", 0, 0,
                                                   &new_filter->tags_count);
        }
        else
        {
            new_filter->tags = NULL;
            new_filter->tags_count = 0;
            new_filter->tags_array = NULL;
        }
        new_filter->regex = strdup (regex);
        new_filter->regex_prefix = regex1;
        new_filter->regex_message = regex2;
        
        /* add filter to filters list */
        new_filter->prev_filter = last_gui_filter;
        if (gui_filters)
            last_gui_filter->next_filter = new_filter;
        else
            gui_filters = new_filter;
        last_gui_filter = new_filter;
        new_filter->next_filter = NULL;
        
        gui_filter_all_buffers ();
        
        hook_signal_send ("filter_added",
                          WEECHAT_HOOK_SIGNAL_POINTER, new_filter);
    }
    
    return new_filter;
}

/*
 * gui_filter_rename: rename a filter
 */

int
gui_filter_rename (struct t_gui_filter *filter, const char *new_name)
{
    if (!filter || !new_name)
        return 0;
    
    if (gui_filter_search_by_name (new_name))
        return 0;
    
    free (filter->name);
    filter->name = strdup (new_name);
    
    return 1;
}

/*
 * gui_filter_free: remove a filter
 */

void
gui_filter_free (struct t_gui_filter *filter)
{
    hook_signal_send ("filter_removing",
                      WEECHAT_HOOK_SIGNAL_POINTER, filter);
    
    /* free data */
    if (filter->name)
        free (filter->name);
    if (filter->plugin_name)
        free (filter->plugin_name);
    if (filter->buffer_name)
        free (filter->buffer_name);
    if (filter->tags)
        free (filter->tags);
    if (filter->tags_array)
        string_free_split (filter->tags_array);
    if (filter->regex)
        free (filter->regex);
    if (filter->regex_prefix)
    {
        regfree (filter->regex_prefix);
	free (filter->regex_prefix);
    }
    if (filter->regex_message)
    {
        regfree (filter->regex_message);
	free (filter->regex_message);
    }
    
    /* remove filter from filters list */
    if (filter->prev_filter)
        (filter->prev_filter)->next_filter = filter->next_filter;
    if (filter->next_filter)
        (filter->next_filter)->prev_filter = filter->prev_filter;
    if (gui_filters == filter)
        gui_filters = filter->next_filter;
    if (last_gui_filter == filter)
        last_gui_filter = filter->prev_filter;
    
    free (filter);
    
    gui_filter_all_buffers ();
    
    hook_signal_send ("filter_removed", WEECHAT_HOOK_SIGNAL_STRING, NULL);
}

/*
 * gui_filter_free_all: remove all filters
 */

void
gui_filter_free_all ()
{
    while (gui_filters)
    {
        gui_filter_free (gui_filters);
    }
}

/*
 * gui_filter_add_to_infolist: add a filter in an infolist
 *                             return 1 if ok, 0 if error
 */

int
gui_filter_add_to_infolist (struct t_infolist *infolist,
                            struct t_gui_filter *filter)
{
    struct t_infolist_item *ptr_item;
    char option_name[64];
    int i;
    
    if (!infolist || !filter)
        return 0;
    
    ptr_item = infolist_new_item (infolist);
    if (!ptr_item)
        return 0;
    
    if (!infolist_new_var_integer (ptr_item, "enabled", filter->enabled))
        return 0;
    if (!infolist_new_var_string (ptr_item, "name", filter->name))
        return 0;
    if (!infolist_new_var_string (ptr_item, "plugin_name", filter->plugin_name))
        return 0;
    if (!infolist_new_var_string (ptr_item, "buffer_name", filter->buffer_name))
        return 0;
    if (!infolist_new_var_string (ptr_item, "tags", filter->tags))
        return 0;
    if (!infolist_new_var_integer (ptr_item, "tags_count", filter->tags_count))
        return 0;
    for (i = 0; i < filter->tags_count; i++)
    {
        snprintf (option_name, sizeof (option_name), "tag_%05d", i + 1);
        if (!infolist_new_var_string (ptr_item, option_name,
                                      filter->tags_array[i]))
            return 0;
    }
    if (!infolist_new_var_string (ptr_item, "regex", filter->regex))
        return 0;
    
    return 1;
}

/*
 * gui_filter_print_log: print filter infos in log (usually for crash dump)
 */

void
gui_filter_print_log ()
{
    struct t_gui_filter *ptr_filter;
    
    log_printf ("");
    log_printf ("gui_filters_enabled = %d", gui_filters_enabled);
    
    for (ptr_filter = gui_filters; ptr_filter;
         ptr_filter = ptr_filter->next_filter)
    {
        log_printf ("");
        log_printf ("[filter (addr:0x%lx)]", ptr_filter);
        log_printf ("  enabled. . . . . . . . : %d",    ptr_filter->enabled);
        log_printf ("  name . . . . . . . . . : '%s'",  ptr_filter->name);
        log_printf ("  plugin_name. . . . . . : '%s'",  ptr_filter->plugin_name);
        log_printf ("  buffer_name. . . . . . : '%s'",  ptr_filter->buffer_name);
        log_printf ("  tags . . . . . . . . . : '%s'",  ptr_filter->tags);
        log_printf ("  regex. . . . . . . . . : '%s'",  ptr_filter->regex);
        log_printf ("  regex_prefix . . . . . : 0x%lx", ptr_filter->regex_prefix);
        log_printf ("  regex_message. . . . . : 0x%lx", ptr_filter->regex_message);
        log_printf ("  prev_filter. . . . . . : 0x%lx", ptr_filter->prev_filter);
        log_printf ("  next_filter. . . . . . : 0x%lx", ptr_filter->next_filter);
    }
}

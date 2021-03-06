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


#ifndef __WEECHAT_CONFIG_H
#define __WEECHAT_CONFIG_H 1

struct t_gui_buffer;

#include "wee-config-file.h"

#define WEECHAT_CONFIG_NAME "weechat"

enum t_config_look_nicklist
{
    CONFIG_LOOK_NICKLIST_LEFT = 0,
    CONFIG_LOOK_NICKLIST_RIGHT,
    CONFIG_LOOK_NICKLIST_TOP,
    CONFIG_LOOK_NICKLIST_BOTTOM,
};

enum t_config_look_prefix_align
{
    CONFIG_LOOK_PREFIX_ALIGN_NONE = 0,
    CONFIG_LOOK_PREFIX_ALIGN_LEFT,
    CONFIG_LOOK_PREFIX_ALIGN_RIGHT,
};

enum t_config_look_prefix_buffer_align
{
    CONFIG_LOOK_PREFIX_BUFFER_ALIGN_NONE = 0,
    CONFIG_LOOK_PREFIX_BUFFER_ALIGN_LEFT,
    CONFIG_LOOK_PREFIX_BUFFER_ALIGN_RIGHT,
};

enum t_config_look_hotlist_sort
{
    CONFIG_LOOK_HOTLIST_SORT_GROUP_TIME_ASC = 0,
    CONFIG_LOOK_HOTLIST_SORT_GROUP_TIME_DESC,
    CONFIG_LOOK_HOTLIST_SORT_GROUP_NUMBER_ASC,
    CONFIG_LOOK_HOTLIST_SORT_GROUP_NUMBER_DESC,
    CONFIG_LOOK_HOTLIST_SORT_NUMBER_ASC,
    CONFIG_LOOK_HOTLIST_SORT_NUMBER_DESC,
};

enum t_config_look_read_marker
{
    CONFIG_LOOK_READ_MARKER_NONE = 0,
    CONFIG_LOOK_READ_MARKER_LINE,
    CONFIG_LOOK_READ_MARKER_DOTTED_LINE,
    CONFIG_LOOK_READ_MARKER_CHAR,
};

enum t_config_look_save_layout_on_exit
{
    CONFIG_LOOK_SAVE_LAYOUT_ON_EXIT_NONE = 0,
    CONFIG_LOOK_SAVE_LAYOUT_ON_EXIT_BUFFERS,
    CONFIG_LOOK_SAVE_LAYOUT_ON_EXIT_WINDOWS,
    CONFIG_LOOK_SAVE_LAYOUT_ON_EXIT_ALL,
};

extern struct t_config_file *weechat_config_file;
extern struct t_config_section *weechat_config_section_proxy;
extern struct t_config_section *weechat_config_section_bar;
extern struct t_config_section *weechat_config_section_notify;

extern struct t_config_option *config_startup_command_after_plugins;
extern struct t_config_option *config_startup_command_before_plugins;
extern struct t_config_option *config_startup_display_logo;
extern struct t_config_option *config_startup_display_version;
extern struct t_config_option *config_startup_weechat_slogan;

extern struct t_config_option *config_look_buffer_notify_default;
extern struct t_config_option *config_look_buffer_time_format;
extern struct t_config_option *config_look_color_nicks_number;
extern struct t_config_option *config_look_color_real_white;
extern struct t_config_option *config_look_day_change;
extern struct t_config_option *config_look_day_change_time_format;
extern struct t_config_option *config_look_highlight;
extern struct t_config_option *config_look_hline_char;
extern struct t_config_option *config_look_hotlist_names_count;
extern struct t_config_option *config_look_hotlist_names_length;
extern struct t_config_option *config_look_hotlist_names_level;
extern struct t_config_option *config_look_hotlist_names_merged_buffers;
extern struct t_config_option *config_look_hotlist_short_names;
extern struct t_config_option *config_look_hotlist_sort;
extern struct t_config_option *config_look_item_time_format;
extern struct t_config_option *config_look_jump_current_to_previous_buffer;
extern struct t_config_option *config_look_jump_previous_buffer_when_closing;
extern struct t_config_option *config_look_nickmode;
extern struct t_config_option *config_look_nickmode_empty;
extern struct t_config_option *config_look_paste_max_lines;
extern struct t_config_option *config_look_prefix[];
extern struct t_config_option *config_look_prefix_align;
extern struct t_config_option *config_look_prefix_align_max;
extern struct t_config_option *config_look_prefix_align_more;
extern struct t_config_option *config_look_prefix_buffer_align;
extern struct t_config_option *config_look_prefix_buffer_align_max;
extern struct t_config_option *config_look_prefix_buffer_align_more;
extern struct t_config_option *config_look_prefix_suffix;
extern struct t_config_option *config_look_read_marker;
extern struct t_config_option *config_look_save_config_on_exit;
extern struct t_config_option *config_look_save_layout_on_exit;
extern struct t_config_option *config_look_scroll_amount;
extern struct t_config_option *config_look_scroll_page_percent;
extern struct t_config_option *config_look_search_text_not_found_alert;
extern struct t_config_option *config_look_set_title;

extern struct t_config_option *config_color_separator;
extern struct t_config_option *config_color_bar_more;
extern struct t_config_option *config_color_chat;
extern struct t_config_option *config_color_chat_bg;
extern struct t_config_option *config_color_chat_time;
extern struct t_config_option *config_color_chat_time_delimiters;
extern struct t_config_option *config_color_chat_prefix_buffer;
extern struct t_config_option *config_color_chat_prefix[];
extern struct t_config_option *config_color_chat_prefix_more;
extern struct t_config_option *config_color_chat_prefix_suffix;
extern struct t_config_option *config_color_chat_buffer;
extern struct t_config_option *config_color_chat_server;
extern struct t_config_option *config_color_chat_channel;
extern struct t_config_option *config_color_chat_nick;
extern struct t_config_option *config_color_chat_nick_self;
extern struct t_config_option *config_color_chat_nick_other;
extern struct t_config_option *config_color_chat_nick_colors[];
extern struct t_config_option *config_color_chat_host;
extern struct t_config_option *config_color_chat_delimiters;
extern struct t_config_option *config_color_chat_highlight;
extern struct t_config_option *config_color_chat_highlight_bg;
extern struct t_config_option *config_color_chat_read_marker;
extern struct t_config_option *config_color_chat_read_marker_bg;
extern struct t_config_option *config_color_chat_text_found;
extern struct t_config_option *config_color_chat_text_found_bg;
extern struct t_config_option *config_color_chat_value;
extern struct t_config_option *config_color_status_number;
extern struct t_config_option *config_color_status_name;
extern struct t_config_option *config_color_status_filter;
extern struct t_config_option *config_color_status_data_msg;
extern struct t_config_option *config_color_status_data_private;
extern struct t_config_option *config_color_status_data_highlight;
extern struct t_config_option *config_color_status_data_other;
extern struct t_config_option *config_color_status_more;
extern struct t_config_option *config_color_input_text_not_found;
extern struct t_config_option *config_color_input_actions;
extern struct t_config_option *config_color_nicklist_group;
extern struct t_config_option *config_color_nicklist_away;
extern struct t_config_option *config_color_nicklist_prefix1;
extern struct t_config_option *config_color_nicklist_prefix2;
extern struct t_config_option *config_color_nicklist_prefix3;
extern struct t_config_option *config_color_nicklist_prefix4;
extern struct t_config_option *config_color_nicklist_prefix5;
extern struct t_config_option *config_color_nicklist_more;

extern struct t_config_option *config_completion_default_template;
extern struct t_config_option *config_completion_nick_add_space;
extern struct t_config_option *config_completion_nick_completer;
extern struct t_config_option *config_completion_nick_first_only;
extern struct t_config_option *config_completion_nick_ignore_chars;
extern struct t_config_option *config_completion_partial_completion_alert;
extern struct t_config_option *config_completion_partial_completion_command;
extern struct t_config_option *config_completion_partial_completion_command_arg;
extern struct t_config_option *config_completion_partial_completion_other;
extern struct t_config_option *config_completion_partial_completion_count;

extern struct t_config_option *config_history_max_lines;
extern struct t_config_option *config_history_max_commands;
extern struct t_config_option *config_history_max_visited_buffers;
extern struct t_config_option *config_history_display_default;

extern struct t_config_option *config_network_gnutls_dh_prime_bits;

extern struct t_config_option *config_plugin_autoload;
extern struct t_config_option *config_plugin_debug;
extern struct t_config_option *config_plugin_extension;
extern struct t_config_option *config_plugin_path;
extern struct t_config_option *config_plugin_save_config_on_unload;


extern struct t_config_option *config_weechat_debug_get (const char *plugin_name);
extern int config_weechat_debug_set (const char *plugin_name,
                                     const char *value);
extern void config_weechat_debug_set_all ();
extern int config_weechat_notify_set (struct t_gui_buffer *buffer,
                                      const char *notify);
extern int config_weechat_init ();
extern int config_weechat_read ();
extern int config_weechat_write ();

#endif /* wee-config.h */

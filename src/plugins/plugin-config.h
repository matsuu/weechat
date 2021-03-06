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


#ifndef __WEECHAT_PLUGIN_CONFIG_H
#define __WEECHAT_PLUGIN_CONFIG_H 1

#define PLUGIN_CONFIG_NAME "plugins"

extern struct t_config_file *plugin_config;
extern struct t_config_option *plugin_options;

extern struct t_config_option *plugin_config_search (const char *plugin_name,
                                                     const char *option_name);
extern int plugin_config_set (const char *plugin_name, const char *option_name,
                              const char *value);
extern void plugin_config_init ();
extern int plugin_config_read ();
extern int plugin_config_write ();
extern void plugin_config_end ();

#endif /* plugin-config.h */

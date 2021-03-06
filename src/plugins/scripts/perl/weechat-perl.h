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


#ifndef __WEECHAT_PERL_H
#define __WEECHAT_PERL_H 1

#define weechat_plugin weechat_perl_plugin
#define PERL_PLUGIN_NAME "perl"

#define PERL_CURRENT_SCRIPT_NAME ((perl_current_script) ? perl_current_script->name : "-")

extern struct t_weechat_plugin *weechat_perl_plugin;

extern int perl_quiet;
extern struct t_plugin_script *perl_scripts;
extern struct t_plugin_script *last_perl_script;
extern struct t_plugin_script *perl_current_script;
extern const char *perl_current_script_filename;

extern void *weechat_perl_exec (struct t_plugin_script *script,
                                int ret_type, const char *function,
                                char **argv);

#endif /* weechat-perl.h */

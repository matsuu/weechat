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

/* irc-display.c: display functions for IRC plugin */


#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include "../weechat-plugin.h"
#include "irc.h"
#include "irc-channel.h"
#include "irc-command.h"
#include "irc-config.h"
#include "irc-server.h"
#include "irc-nick.h"


/*
 * irc_display_hide_password: hide IRC password(s) in a string
 */

void
irc_display_hide_password (char *string, int look_for_nickserv)
{
    char *pos_nickserv, *pos, *pos_pwd, *pos_space;
    int char_size;
    
    pos = string;
    while (pos)
    {
        if (look_for_nickserv)
        {
            pos_nickserv = strstr (pos, "nickserv ");
            if (!pos_nickserv)
                return;
            pos = pos_nickserv + 9;
            while (pos[0] == ' ')
            {
                pos++;
            }
            if (strncmp (pos, "identify ", 9) == 0)
            {
                pos_pwd = pos + 9;
                pos_space = strchr (pos_pwd, ' ');
                if (pos_space)
                {
                    pos_pwd = pos_space + 1;
                    while (pos_pwd[0] == ' ')
                    {
                        pos_pwd++;
                    }
                }
            }
            else if (strncmp (pos, "register ", 9) == 0)
                pos_pwd = pos + 9;
            else
                pos_pwd = NULL;
        }
        else
        {
            pos_pwd = strstr (pos, "identify ");
            if (pos_pwd)
            {
                pos_pwd += 9;
                pos_space = strchr (pos_pwd, ' ');
                if (pos_space)
                {
                    pos_pwd = pos_space + 1;
                    while (pos_pwd[0] == ' ')
                    {
                        pos_pwd++;
                    }
                }
            }
            else
            {
                pos_pwd = strstr (pos, "register ");
                if (pos_pwd)
                    pos_pwd += 9;
            }
            if (!pos_pwd)
                return;
        }

        if (pos_pwd)
        {
            while (pos_pwd[0] == ' ')
            {
                pos_pwd++;
            }
            
            while (pos_pwd && pos_pwd[0] && (pos_pwd[0] != ' '))
            {
                char_size = weechat_utf8_char_size (pos_pwd);
                if (char_size > 0)
                    memset (pos_pwd, '*', char_size);
                pos_pwd = weechat_utf8_next_char (pos_pwd);
            }
            pos = pos_pwd;
        }
    }
}

/*
 * irc_display_away: display away on all channels of all servers
 */

void
irc_display_away (struct t_irc_server *server, const char *string1,
                  const char *string2)
{
    struct t_irc_channel *ptr_channel;
    
    for (ptr_channel = server->channels; ptr_channel;
         ptr_channel = ptr_channel->next_channel)
    {
        if (ptr_channel->type == IRC_CHANNEL_TYPE_CHANNEL)
        {
            weechat_printf_tags (ptr_channel->buffer,
                                 "away_info",
                                 "%s[%s%s%s %s: %s%s]",
                                 IRC_COLOR_CHAT_DELIMITERS,
                                 IRC_COLOR_CHAT_NICK,
                                 server->nick,
                                 IRC_COLOR_CHAT,
                                 string1,
                                 string2,
                                 IRC_COLOR_CHAT_DELIMITERS);
        }
    }
}

/*
 * irc_display_server: display server infos
 */

void
irc_display_server (struct t_irc_server *server, int with_detail)
{
    char *string;
    int num_channels, num_pv;

    if (with_detail)
    {
        weechat_printf (NULL, "");
        weechat_printf (NULL, _("%sServer: %s%s %s[%s%s%s]%s%s"),
                        IRC_COLOR_CHAT,
                        IRC_COLOR_CHAT_SERVER,
                        server->name,
                        IRC_COLOR_CHAT_DELIMITERS,
                        IRC_COLOR_CHAT,
                        (server->is_connected) ?
                        _("connected") : _("not connected"),
                        IRC_COLOR_CHAT_DELIMITERS,
                        IRC_COLOR_CHAT,
                        (server->temp_server) ? _(" (temporary)") : "");
        
        if (weechat_config_option_is_null (server->options[IRC_SERVER_OPTION_ADDRESSES]))
            weechat_printf (NULL, "  addresses. . . . . . :   ('%s')",
                            IRC_SERVER_OPTION_STRING(server, IRC_SERVER_OPTION_ADDRESSES));
        else
            weechat_printf (NULL, "  addresses. . . . . . : %s'%s'",
                            IRC_COLOR_CHAT_VALUE,
                            weechat_config_string (server->options[IRC_SERVER_OPTION_ADDRESSES]));
        if (weechat_config_option_is_null (server->options[IRC_SERVER_OPTION_PROXY]))
            weechat_printf (NULL, "  proxy. . . . . . . . :   ('%s')",
                            IRC_SERVER_OPTION_STRING(server, IRC_SERVER_OPTION_PROXY));
        else
            weechat_printf (NULL, "  proxy. . . . . . . . : %s'%s'",
                            IRC_COLOR_CHAT_VALUE,
                            weechat_config_string (server->options[IRC_SERVER_OPTION_PROXY]));
        if (weechat_config_option_is_null (server->options[IRC_SERVER_OPTION_IPV6]))
            weechat_printf (NULL, "  ipv6 . . . . . . . . :   (%s)",
                            (IRC_SERVER_OPTION_BOOLEAN(server, IRC_SERVER_OPTION_IPV6)) ?
                            _("on") : _("off"));
        else
            weechat_printf (NULL, "  ipv6 . . . . . . . . : %s%s",
                            IRC_COLOR_CHAT_VALUE,
                            weechat_config_boolean (server->options[IRC_SERVER_OPTION_IPV6]) ?
                            _("on") : _("off"));
        if (weechat_config_option_is_null (server->options[IRC_SERVER_OPTION_SSL]))
            weechat_printf (NULL, "  ssl. . . . . . . . . :   (%s)",
                            (IRC_SERVER_OPTION_BOOLEAN(server, IRC_SERVER_OPTION_SSL)) ?
                            _("on") : _("off"));
        else
            weechat_printf (NULL, "  ssl. . . . . . . . . : %s%s",
                            IRC_COLOR_CHAT_VALUE,
                            weechat_config_boolean (server->options[IRC_SERVER_OPTION_SSL]) ?
                            _("on") : _("off"));
        if (weechat_config_option_is_null (server->options[IRC_SERVER_OPTION_PASSWORD]))
            weechat_printf (NULL, "  password . . . . . . :   %s",
                            _("(hidden)"));
        else
            weechat_printf (NULL, "  password . . . . . . : %s%s",
                            IRC_COLOR_CHAT_VALUE,
                            _("(hidden)"));
        if (weechat_config_option_is_null (server->options[IRC_SERVER_OPTION_AUTOCONNECT]))
            weechat_printf (NULL, "  autoconnect. . . . . :   (%s)",
                            (IRC_SERVER_OPTION_BOOLEAN(server, IRC_SERVER_OPTION_AUTOCONNECT)) ?
                            _("on") : _("off"));
        else
            weechat_printf (NULL, "  autoconnect. . . . . : %s%s",
                            IRC_COLOR_CHAT_VALUE,
                            weechat_config_boolean (server->options[IRC_SERVER_OPTION_AUTOCONNECT]) ?
                            _("on") : _("off"));
        if (weechat_config_option_is_null (server->options[IRC_SERVER_OPTION_AUTORECONNECT]))
            weechat_printf (NULL, "  autoreconnect. . . . :   (%s)",
                            (IRC_SERVER_OPTION_BOOLEAN(server, IRC_SERVER_OPTION_AUTORECONNECT)) ?
                            _("on") : _("off"));
        else
            weechat_printf (NULL, "  autoreconnect. . . . : %s%s",
                            IRC_COLOR_CHAT_VALUE,
                            weechat_config_boolean (server->options[IRC_SERVER_OPTION_AUTORECONNECT]) ?
                            _("on") : _("off"));
        if (weechat_config_option_is_null (server->options[IRC_SERVER_OPTION_AUTORECONNECT_DELAY]))
            weechat_printf (NULL, "  autoreconnect_delay. :   (%d %s)",
                            IRC_SERVER_OPTION_INTEGER(server, IRC_SERVER_OPTION_AUTORECONNECT_DELAY),
                            NG_("second", "seconds", IRC_SERVER_OPTION_INTEGER(server, IRC_SERVER_OPTION_AUTORECONNECT_DELAY)));
        else
            weechat_printf (NULL, "  autoreconnect_delay. : %s%d %s",
                            IRC_COLOR_CHAT_VALUE,
                            weechat_config_integer (server->options[IRC_SERVER_OPTION_AUTORECONNECT_DELAY]),
                            NG_("second", "seconds", weechat_config_integer (server->options[IRC_SERVER_OPTION_AUTORECONNECT_DELAY])));
        if (weechat_config_option_is_null (server->options[IRC_SERVER_OPTION_NICKS]))
            weechat_printf (NULL, "  nicks. . . . . . . . :   ('%s')",
                            IRC_SERVER_OPTION_STRING(server, IRC_SERVER_OPTION_NICKS));
        else
            weechat_printf (NULL, "  nicks. . . . . . . . : %s'%s'",
                            IRC_COLOR_CHAT_VALUE,
                            weechat_config_string (server->options[IRC_SERVER_OPTION_NICKS]));
        if (weechat_config_option_is_null (server->options[IRC_SERVER_OPTION_USERNAME]))
            weechat_printf (NULL, "  username . . . . . . :   ('%s')",
                            IRC_SERVER_OPTION_STRING(server, IRC_SERVER_OPTION_USERNAME));
        else
            weechat_printf (NULL, "  username . . . . . . : %s'%s'",
                            IRC_COLOR_CHAT_VALUE,
                            weechat_config_string (server->options[IRC_SERVER_OPTION_USERNAME]));
        if (weechat_config_option_is_null (server->options[IRC_SERVER_OPTION_REALNAME]))
            weechat_printf (NULL, "  realname . . . . . . :   ('%s')",
                            IRC_SERVER_OPTION_STRING(server, IRC_SERVER_OPTION_REALNAME));
        else
            weechat_printf (NULL, "  realname . . . . . . : %s'%s'",
                            IRC_COLOR_CHAT_VALUE,
                            weechat_config_string (server->options[IRC_SERVER_OPTION_REALNAME]));
        if (weechat_config_option_is_null (server->options[IRC_SERVER_OPTION_LOCAL_HOSTNAME]))
            weechat_printf (NULL, "  local_hostname . . . :   ('%s')",
                            IRC_SERVER_OPTION_STRING(server, IRC_SERVER_OPTION_LOCAL_HOSTNAME));
        else
            weechat_printf (NULL, "  local_hostname . . . : %s'%s'",
                            IRC_COLOR_CHAT_VALUE,
                            weechat_config_string (server->options[IRC_SERVER_OPTION_LOCAL_HOSTNAME]));
        if (weechat_config_option_is_null (server->options[IRC_SERVER_OPTION_COMMAND]))
        {
            string = strdup (IRC_SERVER_OPTION_STRING(server, IRC_SERVER_OPTION_COMMAND));
            if (string && weechat_config_boolean (irc_config_look_hide_nickserv_pwd))
                irc_display_hide_password (string, 1);
            weechat_printf (NULL, "  command. . . . . . . :   ('%s')",
                            (string) ? string : IRC_SERVER_OPTION_STRING(server, IRC_SERVER_OPTION_COMMAND));
            if (string)
                free (string);
        }
        else
        {
            string = strdup (weechat_config_string (server->options[IRC_SERVER_OPTION_COMMAND]));
            if (string && weechat_config_boolean (irc_config_look_hide_nickserv_pwd))
                irc_display_hide_password (string, 1);
            weechat_printf (NULL, "  command. . . . . . . : %s'%s'",
                            IRC_COLOR_CHAT_VALUE,
                            (string) ? string : weechat_config_string (server->options[IRC_SERVER_OPTION_COMMAND]));
            if (string)
                free (string);
        }
        if (weechat_config_option_is_null (server->options[IRC_SERVER_OPTION_COMMAND_DELAY]))
            weechat_printf (NULL, "  command_delay. . . . :   (%d %s)",
                            IRC_SERVER_OPTION_INTEGER(server, IRC_SERVER_OPTION_COMMAND_DELAY),
                            NG_("second", "seconds", IRC_SERVER_OPTION_INTEGER(server, IRC_SERVER_OPTION_COMMAND_DELAY)));
        else
            weechat_printf (NULL, "  command_delay. . . . : %s%d %s",
                            IRC_COLOR_CHAT_VALUE,
                            weechat_config_integer (server->options[IRC_SERVER_OPTION_COMMAND_DELAY]),
                            NG_("second", "seconds", weechat_config_integer (server->options[IRC_SERVER_OPTION_COMMAND_DELAY])));
        if (weechat_config_option_is_null (server->options[IRC_SERVER_OPTION_AUTOJOIN]))
            weechat_printf (NULL, "  autojoin . . . . . . :   ('%s')",
                            IRC_SERVER_OPTION_STRING(server, IRC_SERVER_OPTION_AUTOJOIN));
        else
            weechat_printf (NULL, "  autojoin . . . . . . : %s'%s'",
                            IRC_COLOR_CHAT_VALUE,
                            weechat_config_string (server->options[IRC_SERVER_OPTION_AUTOJOIN]));
        if (weechat_config_option_is_null (server->options[IRC_SERVER_OPTION_AUTOREJOIN]))
            weechat_printf (NULL, "  autorejoin . . . . . :   (%s)",
                            (IRC_SERVER_OPTION_BOOLEAN(server, IRC_SERVER_OPTION_AUTOREJOIN)) ?
                            _("on") : _("off"));
        else
            weechat_printf (NULL, "  autorejoin . . . . . : %s%s",
                            IRC_COLOR_CHAT_VALUE,
                            weechat_config_boolean (server->options[IRC_SERVER_OPTION_AUTOREJOIN]) ?
                            _("on") : _("off"));
    }
    else
    {
        if (server->is_connected)
        {
            num_channels = irc_server_get_channel_count (server);
            num_pv = irc_server_get_pv_count (server);
            weechat_printf (NULL, " %s %s%s %s[%s%s%s]%s%s, %d %s, %d pv",
                            (server->is_connected) ? "*" : " ",
                            IRC_COLOR_CHAT_SERVER,
                            server->name,
                            IRC_COLOR_CHAT_DELIMITERS,
                            IRC_COLOR_CHAT,
                            (server->is_connected) ?
                            _("connected") : _("not connected"),
                            IRC_COLOR_CHAT_DELIMITERS,
                            IRC_COLOR_CHAT,
                            (server->temp_server) ? _(" (temporary)") : "",
                            num_channels,
                            NG_("channel", "channels", num_channels),
                            num_pv);
        }
        else
        {
            weechat_printf (NULL, "   %s%s%s%s",
                            IRC_COLOR_CHAT_SERVER,
                            server->name,
                            IRC_COLOR_CHAT,
                            (server->temp_server) ? _(" (temporary)") : "");
        }
    }
}

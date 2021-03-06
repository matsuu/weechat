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

/* irc-command.c: IRC commands */


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <sys/time.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "../weechat-plugin.h"
#include "irc.h"
#include "irc-command.h"
#include "irc-buffer.h"
#include "irc-color.h"
#include "irc-config.h"
#include "irc-input.h"
#include "irc-server.h"
#include "irc-channel.h"
#include "irc-nick.h"
#include "irc-display.h"
#include "irc-ignore.h"
#include "irc-raw.h"


/*
 * irc_command_admin: find information about the administrator of the server
 */

int
irc_command_admin (void *data, struct t_gui_buffer *buffer, int argc,
                   char **argv, char **argv_eol)
{
    IRC_GET_SERVER(buffer);
    IRC_COMMAND_CHECK_SERVER("admin", 1);

    /* make C compiler happy */
    (void) data;
    (void) argv;
    
    if (argc > 1)
        irc_server_sendf (ptr_server, 0, "ADMIN %s", argv_eol[1]);
    else
        irc_server_sendf (ptr_server, 0, "ADMIN");
    
    return WEECHAT_RC_OK;
}

/*
 * irc_command_me_channel: send a ctcp action to a channel
 */

void
irc_command_me_channel (struct t_irc_server *server,
                        struct t_irc_channel *channel,
                        const char *arguments)
{
    char *string;
    
    irc_server_sendf (server, 1, "PRIVMSG %s :\01ACTION %s\01",
                      channel->name,
                      (arguments && arguments[0]) ? arguments : "");
    string = (arguments && arguments[0]) ?
        irc_color_decode (arguments,
                          weechat_config_boolean (irc_config_network_colors_receive)) : NULL;
    weechat_printf (channel->buffer,
                    "%s%s%s %s%s",
                    weechat_prefix ("action"),
                    IRC_COLOR_CHAT_NICK_SELF,
                    server->nick,
                    IRC_COLOR_CHAT,
                    (string) ? string : "");
    if (string)
        free (string);
}

/*
 * irc_command_me_all_channels: send a ctcp action to all channels of a server
 */

void
irc_command_me_all_channels (struct t_irc_server *server, const char *arguments)
{
    struct t_irc_channel *ptr_channel;
    
    for (ptr_channel = server->channels; ptr_channel;
         ptr_channel = ptr_channel->next_channel)
    {
        if (ptr_channel->type == IRC_CHANNEL_TYPE_CHANNEL)
            irc_command_me_channel (server, ptr_channel, arguments);
    }
}

/*
 * irc_command_mode_nicks: send mode change for many nicks on a channel
 */

void
irc_command_mode_nicks (struct t_irc_server *server, const char *channel,
                        const char *set, const char *mode, int argc, char **argv)
{
    int i, length;
    char *command;
    
    length = 0;
    for (i = 1; i < argc; i++)
        length += strlen (argv[i]) + 1;
    length += strlen (channel) + (argc * strlen (mode)) + 32;
    command = malloc (length);
    if (command)
    {
        snprintf (command, length, "MODE %s %s", channel, set);
        for (i = 1; i < argc; i++)
            strcat (command, mode);
        for (i = 1; i < argc; i++)
        {
            strcat (command, " ");
            strcat (command, argv[i]);
        }
        irc_server_sendf (server, 0, "%s", command);
        free (command);
    }
}

/*
 * irc_command_ame: send a ctcp action to all channels of all connected servers
 */

int
irc_command_ame (void *data, struct t_gui_buffer *buffer, int argc,
                 char **argv, char **argv_eol)
{
    struct t_irc_server *ptr_server;
    struct t_irc_channel *ptr_channel;
    
    /* make C compiler happy */
    (void) data;
    (void) buffer;
    (void) argv;
    
    if (argc > 1)
    {
        weechat_buffer_set (NULL, "hotlist", "-");
        for (ptr_server = irc_servers; ptr_server;
             ptr_server = ptr_server->next_server)
        {
            if (ptr_server->is_connected)
            {
                for (ptr_channel = ptr_server->channels; ptr_channel;
                     ptr_channel = ptr_channel->next_channel)
                {
                    if (ptr_channel->type == IRC_CHANNEL_TYPE_CHANNEL)
                        irc_command_me_channel (ptr_server, ptr_channel,
                                                argv_eol[1]);
                }
            }
        }
        weechat_buffer_set (NULL, "hotlist", "+");
    }
    return WEECHAT_RC_OK;
}

/*
 * irc_command_amsg: send message to all channels of all connected servers
 */

int
irc_command_amsg (void *data, struct t_gui_buffer *buffer, int argc,
                  char **argv, char **argv_eol)
{
    struct t_irc_server *ptr_server;
    struct t_irc_channel *ptr_channel;
    struct t_irc_nick *ptr_nick;
    char *string;
    
    /* make C compiler happy */
    (void) data;
    (void) buffer;
    (void) argv;
    
    if (argc > 1)
    {
        weechat_buffer_set (NULL, "hotlist", "-");
        for (ptr_server = irc_servers; ptr_server;
             ptr_server = ptr_server->next_server)
        {
            if (ptr_server->is_connected)
            {
                for (ptr_channel = ptr_server->channels; ptr_channel;
                     ptr_channel = ptr_channel->next_channel)
                {
                    if (ptr_channel->type == IRC_CHANNEL_TYPE_CHANNEL)
                    {
                        irc_server_sendf (ptr_server, 1, "PRIVMSG %s :%s",
                                          ptr_channel->name, argv_eol[1]);
                        ptr_nick = irc_nick_search (ptr_channel,
                                                    ptr_server->nick);
                        if (ptr_nick)
                        {
                            string = irc_color_decode (argv_eol[1],
                                                       weechat_config_boolean (irc_config_network_colors_receive));
                            weechat_printf (ptr_channel->buffer, "%s%s",
                                            irc_nick_as_prefix (ptr_nick,
                                                                NULL, NULL),
                                            (string) ? string : argv_eol[1]);
                            if (string)
                                free (string);
                        }
                        else
                        {
                            weechat_printf (ptr_server->buffer,
                                            _("%s%s: cannot find nick for "
                                              "sending message"),
                                            weechat_prefix ("error"),
                                            IRC_PLUGIN_NAME);
                        }
                    }
                }
            }
        }
        weechat_buffer_set (NULL, "hotlist", "+");
    }
    else
        return WEECHAT_RC_ERROR;
    
    return WEECHAT_RC_OK;
}

/*
 * irc_command_away_server: toggle away status for one server
 */

void
irc_command_away_server (struct t_irc_server *server, const char *arguments,
                         int reset_unread_marker)
{
    char *string, buffer[4096];
    time_t time_now, elapsed;
    struct t_irc_channel *ptr_channel;
    
    if (!server)
        return;
    
    if (arguments)
    {
        if (server->away_message)
            free (server->away_message);
        server->away_message = strdup (arguments);
        
        /* if server is connected, send away command now */
        if (server->is_connected)
        {
            server->is_away = 1;
            server->away_time = time (NULL);
            irc_server_sendf (server, 1, "AWAY :%s", arguments);
            if (weechat_config_integer (irc_config_look_display_away) != IRC_CONFIG_DISPLAY_AWAY_OFF)
            {
                string = irc_color_decode (arguments,
                                           weechat_config_boolean (irc_config_network_colors_receive));
                if (weechat_config_integer (irc_config_look_display_away) == IRC_CONFIG_DISPLAY_AWAY_LOCAL)
                    irc_display_away (server, "away",
                                      (string) ? string : arguments);
                else
                {
                    snprintf (buffer, sizeof (buffer), "is away: %s",
                              (string) ? string : arguments);
                    irc_command_me_all_channels (server, buffer);
                }
                if (string)
                    free (string);
            }
            irc_server_set_away (server, server->nick, 1);
            
            /* reset "unread" indicator on server and channels/pv buffers */
            if (reset_unread_marker)
            {
                if (weechat_buffer_get_integer (server->buffer, "num_displayed") > 0)
                    weechat_buffer_set (server->buffer, "unread", "");
                for (ptr_channel = server->channels; ptr_channel;
                     ptr_channel = ptr_channel->next_channel)
                {
                    if (weechat_buffer_get_integer (ptr_channel->buffer, "num_displayed") > 0)
                        weechat_buffer_set (ptr_channel->buffer, "unread", "");
                }
            }
            
            /* ask refresh for "away" item */
            weechat_bar_item_update ("away");
        }
        else
        {
            /* server not connected, store away for future usage
               (when connecting to server) */
            string = irc_color_decode (arguments,
                                       weechat_config_boolean (irc_config_network_colors_receive));
            weechat_printf (server->buffer,
                            _("%s: future away: %s"),
                            IRC_PLUGIN_NAME,
                            (string) ? string : arguments);
            if (string)
                free (string);
        }
    }
    else
    {
        if (server->away_message)
        {
            free (server->away_message);
            server->away_message = NULL;
        }
        
        /* if server is connected, send away command now */
        if (server->is_connected)
        {
            irc_server_sendf (server, 1, "AWAY");
            server->is_away = 0;
            if (server->away_time != 0)
            {
                time_now = time (NULL);
                elapsed = (time_now >= server->away_time) ?
                    time_now - server->away_time : 0;
                server->away_time = 0;
                if (weechat_config_integer (irc_config_look_display_away) != IRC_CONFIG_DISPLAY_AWAY_OFF)
                {
                    if (weechat_config_integer (irc_config_look_display_away) == IRC_CONFIG_DISPLAY_AWAY_LOCAL)
                    {
                        snprintf (buffer, sizeof (buffer),
                                  "gone %.2ld:%.2ld:%.2ld",
                                  (long int)(elapsed / 3600),
                                  (long int)((elapsed / 60) % 60),
                                  (long int)(elapsed % 60));
                        irc_display_away (server, "back", buffer);
                    }
                    else
                    {
                        snprintf (buffer, sizeof (buffer),
                                  "is back (gone %.2ld:%.2ld:%.2ld)",
                                  (long int)(elapsed / 3600),
                                  (long int)((elapsed / 60) % 60),
                                  (long int)(elapsed % 60));
                        irc_command_me_all_channels (server, buffer);
                    }
                }
            }
            irc_server_set_away (server, server->nick, 0);
            
            /* ask refresh for "away" item */
            weechat_bar_item_update ("away");
        }
        else
        {
            /* server not connected, remove away message but do not send
               anything */
            weechat_printf (server->buffer,
                            _("%s: future away removed"),
                            IRC_PLUGIN_NAME);
        }
    }
}

/*
 * irc_command_away: toggle away status
 */

int
irc_command_away (void *data, struct t_gui_buffer *buffer, int argc,
                  char **argv, char **argv_eol)
{
    IRC_GET_SERVER(buffer);
    
    /* make C compiler happy */
    (void) data;
    
    if ((argc >= 2) && (weechat_strcasecmp (argv[1], "-all") == 0))
    {
        weechat_buffer_set (NULL, "hotlist", "-");
        for (ptr_server = irc_servers; ptr_server;
             ptr_server = ptr_server->next_server)
        {
            if (ptr_server->is_connected)
                irc_command_away_server (ptr_server,
                                         (argc > 2) ? argv_eol[2] : NULL,
                                         1);
        }
        weechat_buffer_set (NULL, "hotlist", "+");
    }
    else
    {
        IRC_COMMAND_CHECK_SERVER("away", 0);
        
        weechat_buffer_set (NULL, "hotlist", "-");
        irc_command_away_server (ptr_server, argv_eol[1], 1);
        weechat_buffer_set (NULL, "hotlist", "+");
    }
    
    return WEECHAT_RC_OK;
}

/*
 * irc_command_ban: bans nicks or hosts
 */

int
irc_command_ban (void *data, struct t_gui_buffer *buffer, int argc,
                 char **argv, char **argv_eol)
{
    char *pos_channel;
    int pos_args;
    
    IRC_GET_SERVER_CHANNEL(buffer);
    IRC_COMMAND_CHECK_SERVER("ban", 1);
    
    /* make C compiler happy */
    (void) data;
    (void) argv_eol;
    
    if (argc > 1)
    {
        if (irc_channel_is_channel (argv[1]))
        {
            pos_channel = argv[1];
            pos_args = 2;
        }
        else
        {
            pos_channel = NULL;
            pos_args = 1;
        }
        
        /* channel not given, use default buffer */
        if (!pos_channel)
        {
            if (ptr_channel && (ptr_channel->type == IRC_CHANNEL_TYPE_CHANNEL))
                pos_channel = ptr_channel->name;
            else
            {
                weechat_printf (ptr_server->buffer,
                                _("%s%s: \"%s\" command can only be "
                                  "executed in a channel buffer"),
                                weechat_prefix ("error"), IRC_PLUGIN_NAME,
                                "ban");
                return WEECHAT_RC_OK;
            }
        }
        
        /* loop on users */
        while (argv[pos_args])
        {
            irc_server_sendf (ptr_server, 0, "MODE %s +b %s",
                              pos_channel, argv[pos_args]);
            pos_args++;
        }
    }
    else
    {
        if (!ptr_channel)
        {
            weechat_printf (ptr_server->buffer,
                            _("%s%s: \"%s\" command can only be "
                              "executed in a channel buffer"),
                            weechat_prefix ("error"), IRC_PLUGIN_NAME, "ban");
            return WEECHAT_RC_OK;
        }
        irc_server_sendf (ptr_server, 0, "MODE %s +b", ptr_channel->name);
    }
    
    return WEECHAT_RC_OK;
}

/*
 * irc_command_connect_one_server: connect to one server
 *                                 return 0 if error, 1 if ok
 */

int
irc_command_connect_one_server (struct t_irc_server *server, int no_join)
{
    if (!server)
        return 0;
    
    if (server->is_connected)
    {
        weechat_printf (NULL,
                        _("%s%s: already connected to server "
                          "\"%s\"!"),
                        weechat_prefix ("error"), IRC_PLUGIN_NAME,
                        server->name);
        return 0;
    }
    if (server->hook_connect)
    {
        weechat_printf (NULL,
                        _("%s%s: currently connecting to server "
                          "\"%s\"!"),
                        weechat_prefix ("error"), IRC_PLUGIN_NAME,
                        server->name);
        return 0;
    }
    server->disable_autojoin = no_join;
    if (irc_server_connect (server))
    {
        server->reconnect_start = 0;
        server->reconnect_join = (server->channels) ? 1 : 0;
    }
    
    /* connect ok */
    return 1;
}

/*
 * irc_command_connect: connect to server(s)
 */

int
irc_command_connect (void *data, struct t_gui_buffer *buffer, int argc,
                     char **argv, char **argv_eol)
{
    int i, nb_connect, connect_ok, all_servers, no_join, port, ipv6, ssl;
    char *name, *error;
    long number;
    
    IRC_GET_SERVER(buffer);
    
    /* make C compiler happy */
    (void) data;
    (void) argv_eol;
    
    nb_connect = 0;
    connect_ok = 1;
    port = IRC_SERVER_DEFAULT_PORT;
    ipv6 = 0;
    ssl = 0;
    
    all_servers = 0;
    no_join = 0;
    for (i = 1; i < argc; i++)
    {
        if (weechat_strcasecmp (argv[i], "-all") == 0)
            all_servers = 1;
        if (weechat_strcasecmp (argv[i], "-nojoin") == 0)
            no_join = 1;
        if (weechat_strcasecmp (argv[i], "-ipv6") == 0)
            ipv6 = 1;
        if (weechat_strcasecmp (argv[i], "-ssl") == 0)
            ssl = 1;
        if (weechat_strcasecmp (argv[i], "-port") == 0)
        {
            if (i == (argc - 1))
            {
                weechat_printf (NULL,
                                _("%s%s: missing argument for \"%s\" "
                                  "option"),
                                weechat_prefix ("error"), IRC_PLUGIN_NAME,
                                "-port");
                return WEECHAT_RC_OK;
            }
            error = NULL;
            number = strtol (argv[++i], &error, 10);
            if (error && !error[0])
                port = number;
        }
    }
    
    if (all_servers)
    {
        for (ptr_server = irc_servers; ptr_server;
             ptr_server = ptr_server->next_server)
        {
            nb_connect++;
            if (!ptr_server->is_connected && (!ptr_server->hook_connect))
            {
                if (!irc_command_connect_one_server (ptr_server, no_join))
                    connect_ok = 0;
            }
        }
    }
    else
    {
        for (i = 1; i < argc; i++)
        {
            if (argv[i][0] != '-')
            {
                nb_connect++;
                ptr_server = irc_server_search (argv[i]);
                if (ptr_server)
                {
                    if (!irc_command_connect_one_server (ptr_server, no_join))
                        connect_ok = 0;
                }
                else
                {
                    name = irc_server_get_name_without_port (argv[i]);
                    ptr_server = irc_server_alloc ((name) ? name : argv[i]);
                    if (name)
                        free (name);
                    if (ptr_server)
                    {
                        ptr_server->temp_server = 1;
                        weechat_config_option_set (ptr_server->options[IRC_SERVER_OPTION_ADDRESSES],
                                                   argv[i], 1);
                        weechat_printf (NULL,
                                        _("%s: server %s%s%s created (temporary server, NOT SAVED!)"),
                                        IRC_PLUGIN_NAME,
                                        IRC_COLOR_CHAT_SERVER,
                                        ptr_server->name,
                                        IRC_COLOR_CHAT);
                        if (!irc_command_connect_one_server (ptr_server, 0))
                            connect_ok = 0;
                    }
                    else
                    {
                        weechat_printf (NULL,
                                        _("%s%s: unable to create server "
                                          "\"%s\""),
                                        weechat_prefix ("error"),
                                        IRC_PLUGIN_NAME, argv[i]);
                    }
                }
            }
            else
            {
                if (weechat_strcasecmp (argv[i], "-port") == 0)
                    i++;
            }
        }
    }
    
    if (nb_connect == 0)
        connect_ok = irc_command_connect_one_server (ptr_server, no_join);
    
    if (!connect_ok)
        return WEECHAT_RC_ERROR;
    
    return WEECHAT_RC_OK;
}

/*
 * irc_command_ctcp: send a ctcp message
 */

int
irc_command_ctcp (void *data, struct t_gui_buffer *buffer, int argc,
                  char **argv, char **argv_eol)
{
    char *pos, *irc_cmd, str_time[512];
    struct timeval tv;
    
    IRC_GET_SERVER(buffer);
    IRC_COMMAND_CHECK_SERVER("ctcp", 1);
    
    /* make C compiler happy */
    (void) data;

    if (argc > 2)
    {
        irc_cmd = strdup (argv[2]);
        if (!irc_cmd)
            return WEECHAT_RC_ERROR;
        
        pos = irc_cmd;
        while (pos[0])
        {
            pos[0] = toupper (pos[0]);
            pos++;
        }
        
        if ((weechat_strcasecmp (argv[2], "ping") == 0) && !argv_eol[3])
        {
            gettimeofday (&tv, NULL);
            snprintf (str_time, sizeof (str_time), "%ld %ld",
                      tv.tv_sec, tv.tv_usec);
            irc_server_sendf (ptr_server, 1, "PRIVMSG %s :\01PING %s\01",
                              argv[1], str_time);
            weechat_printf (ptr_server->buffer,
                            _("%sCTCP query to %s%s%s: %s%s%s%s%s"),
                            weechat_prefix ("network"),
                            IRC_COLOR_CHAT_NICK,
                            argv[1],
                            IRC_COLOR_CHAT,
                            IRC_COLOR_CHAT_CHANNEL,
                            irc_cmd,
                            IRC_COLOR_CHAT,
                            " ",
                            str_time);
        }
        else
        {
            irc_server_sendf (ptr_server, 1, "PRIVMSG %s :\01%s%s%s\01",
                              argv[1],
                              irc_cmd,
                              (argv_eol[3]) ? " " : "",
                              (argv_eol[3]) ? argv_eol[3] : "");
            weechat_printf (ptr_server->buffer,
                            _("%sCTCP query to %s%s%s: %s%s%s%s%s"),
                            weechat_prefix ("network"),
                            IRC_COLOR_CHAT_NICK,
                            argv[1],
                            IRC_COLOR_CHAT,
                            IRC_COLOR_CHAT_CHANNEL,
                            irc_cmd,
                            IRC_COLOR_CHAT,
                            (argv_eol[3]) ? " " : "",
                            (argv_eol[3]) ? argv_eol[3] : "");
        }
        
        free (irc_cmd);
    }
    
    return WEECHAT_RC_OK;
}
    
/*
 * irc_command_cycle: leave and rejoin a channel
 */

int
irc_command_cycle (void *data, struct t_gui_buffer *buffer, int argc,
                   char **argv, char **argv_eol)
{
    char *channel_name, *pos_args, *buf;
    const char *version, *ptr_arg;
    char **channels;
    int i, num_channels;
    
    IRC_GET_SERVER_CHANNEL(buffer);
    IRC_COMMAND_CHECK_SERVER("cycle", 1);
    
    /* make C compiler happy */
    (void) data;
    (void) argv;
    
    if (argc > 1)
    {
        if (irc_channel_is_channel (argv[1]))
        {
            channel_name = argv[1];
            pos_args = argv_eol[2];
            channels = weechat_string_split (channel_name, ",", 0, 0,
                                             &num_channels);
            if (channels)
            {
                for (i = 0; i < num_channels; i++)
                {
                    ptr_channel = irc_channel_search (ptr_server, channels[i]);
                    /* mark channel as cycling */
                    if (ptr_channel &&
                        (ptr_channel->type == IRC_CHANNEL_TYPE_CHANNEL))
                        ptr_channel->cycle = 1;
                }
                weechat_string_free_split (channels);
            }
        }
        else
        {
            if (!ptr_channel)
            {
                weechat_printf (ptr_server->buffer,
                                _("%s%s: \"%s\" command can not be executed "
                                  "on a server buffer"),
                                weechat_prefix ("error"), IRC_PLUGIN_NAME,
                                "cycle");
                return WEECHAT_RC_OK;
            }
            
            /* does nothing on private buffer (cycle has no sense!) */
            if (ptr_channel->type != IRC_CHANNEL_TYPE_CHANNEL)
                return WEECHAT_RC_OK;
            
            channel_name = ptr_channel->name;
            pos_args = argv_eol[1];
            ptr_channel->cycle = 1;
        }
    }
    else
    {
        if (!ptr_channel)
        {
            weechat_printf (ptr_server->buffer,
                            _("%s%s: \"%s\" command can not be executed on "
                              "a server buffer"),
                            weechat_prefix ("error"), IRC_PLUGIN_NAME, "part");
            return WEECHAT_RC_OK;
        }
        
        /* does nothing on private buffer (cycle has no sense!) */
        if (ptr_channel->type != IRC_CHANNEL_TYPE_CHANNEL)
            return WEECHAT_RC_OK;
        
        channel_name = ptr_channel->name;
        pos_args = NULL;
        ptr_channel->cycle = 1;
    }
    
    ptr_arg = (pos_args) ? pos_args :
        (weechat_config_string (irc_config_network_default_msg_part)
         && weechat_config_string (irc_config_network_default_msg_part)[0]) ?
        weechat_config_string (irc_config_network_default_msg_part) : NULL;
    
    if (ptr_arg)
    {
        version = weechat_info_get ("version", "");
        buf = weechat_string_replace (ptr_arg, "%v", (version) ? version : "");
        irc_server_sendf (ptr_server, 0, "PART %s :%s", channel_name,
                          (buf) ? buf : ptr_arg);
        if (buf)
            free (buf);
    }
    else
        irc_server_sendf (ptr_server, 0, "PART %s", channel_name);
    
    return WEECHAT_RC_OK;
}

/*
 * irc_command_dcc: DCC control (file or chat)
 */

int
irc_command_dcc (void *data, struct t_gui_buffer *buffer, int argc,
                 char **argv, char **argv_eol)
{
    struct sockaddr_in addr;
    socklen_t length;
    unsigned long address;
    struct t_infolist *infolist;
    struct t_infolist_item *item;
    char plugin_id[128], str_address[128], charset_modifier[256];
    
    IRC_GET_SERVER_CHANNEL(buffer);
    IRC_COMMAND_CHECK_SERVER("dcc", 1);
    
    /* make C compiler happy */
    (void) data;
    
    if (argc > 1)
    {
        /* use the local interface, from the server socket */
        memset (&addr, 0, sizeof (struct sockaddr_in));
        length = sizeof (addr);
        getsockname (ptr_server->sock, (struct sockaddr *) &addr, &length);
        addr.sin_family = AF_INET;
        address = ntohl (addr.sin_addr.s_addr);
        
        /* DCC SEND file */
        if (weechat_strcasecmp (argv[1], "send") == 0)
        {
            if (argc < 4)
            {
                IRC_COMMAND_TOO_FEW_ARGUMENTS(ptr_server->buffer, "dcc send");
            }
            infolist = weechat_infolist_new ();
            if (infolist)
            {
                item = weechat_infolist_new_item (infolist);
                if (item)
                {
                    weechat_infolist_new_var_string (item, "plugin_name", weechat_plugin->name);
                    snprintf (plugin_id, sizeof (plugin_id),
                              "%lx", (long unsigned int)ptr_server);
                    weechat_infolist_new_var_string (item, "plugin_id", plugin_id);
                    weechat_infolist_new_var_string (item, "type", "file_send");
                    weechat_infolist_new_var_string (item, "protocol", "dcc");
                    weechat_infolist_new_var_string (item, "remote_nick", argv[2]);
                    weechat_infolist_new_var_string (item, "local_nick", ptr_server->nick);
                    weechat_infolist_new_var_string (item, "filename", argv_eol[3]);
                    snprintf (str_address, sizeof (str_address),
                              "%lu", address);
                    weechat_infolist_new_var_string (item, "address", str_address);
                    weechat_infolist_new_var_integer (item, "socket", ptr_server->sock);
                    weechat_hook_signal_send ("xfer_add",
                                              WEECHAT_HOOK_SIGNAL_POINTER,
                                              infolist);
                }
                weechat_infolist_free (infolist);
            }
        }
        /* DCC CHAT */
        else if (weechat_strcasecmp (argv[1], "chat") == 0)
        {
            if (argc < 3)
            {
                IRC_COMMAND_TOO_FEW_ARGUMENTS(ptr_server->buffer, "dcc chat");
            }
            infolist = weechat_infolist_new ();
            if (infolist)
            {
                item = weechat_infolist_new_item (infolist);
                if (item)
                {
                    weechat_infolist_new_var_string (item, "plugin_name", weechat_plugin->name);
                    snprintf (plugin_id, sizeof (plugin_id),
                              "%lx", (long unsigned int)ptr_server);
                    weechat_infolist_new_var_string (item, "plugin_id", plugin_id);
                    weechat_infolist_new_var_string (item, "type", "chat_send");
                    weechat_infolist_new_var_string (item, "remote_nick", argv[2]);
                    weechat_infolist_new_var_string (item, "local_nick", ptr_server->nick);
                    snprintf (charset_modifier, sizeof (charset_modifier),
                              "irc.%s.%s", ptr_server->name, argv[2]);
                    weechat_infolist_new_var_string (item, "charset_modifier", charset_modifier);
                    snprintf (str_address, sizeof (str_address),
                              "%lu", address);
                    weechat_infolist_new_var_string (item, "address", str_address);
                    weechat_hook_signal_send ("xfer_add",
                                              WEECHAT_HOOK_SIGNAL_POINTER,
                                              infolist);
                }
                weechat_infolist_free (infolist);
            }
        }
        /* unknown DCC action */
        else
        {
            weechat_printf (ptr_server->buffer,
                            _("%s%s: wrong arguments for \"%s\" command"),
                            weechat_prefix ("error"), IRC_PLUGIN_NAME, "dcc");
            return WEECHAT_RC_OK;
        }
    }
    else
    {
        IRC_COMMAND_TOO_FEW_ARGUMENTS(ptr_server->buffer, "dcc");
    }
    
    return WEECHAT_RC_OK;
}

/*
 * irc_command_dehalfop: remove half operator privileges from nickname(s)
 */

int
irc_command_dehalfop (void *data, struct t_gui_buffer *buffer, int argc,
                      char **argv, char **argv_eol)
{
    IRC_GET_SERVER_CHANNEL(buffer);
    IRC_COMMAND_CHECK_SERVER("dehalfop", 1);
    
    /* make C compiler happy */
    (void) data;
    (void) argv_eol;
    
    if (ptr_channel && (ptr_channel->type == IRC_CHANNEL_TYPE_CHANNEL))
    {
        if (argc < 2)
            irc_server_sendf (ptr_server, 0, "MODE %s -h %s",
                              ptr_channel->name,
                              ptr_server->nick);
        else
            irc_command_mode_nicks (ptr_server, ptr_channel->name,
                                    "-", "h", argc, argv);
    }
    else
    {
        weechat_printf (ptr_server->buffer,
                        _("%s%s: \"%s\" command can only be executed in "
                          "a channel buffer"),
                        weechat_prefix ("error"), IRC_PLUGIN_NAME, "dehalfop");
        return WEECHAT_RC_OK;
    }
    return WEECHAT_RC_OK;
}

/*
 * irc_command_deop: remove operator privileges from nickname(s)
 */

int
irc_command_deop (void *data, struct t_gui_buffer *buffer, int argc,
                  char **argv, char **argv_eol)
{
    IRC_GET_SERVER_CHANNEL(buffer);
    IRC_COMMAND_CHECK_SERVER("deop", 1);
    
    /* make C compiler happy */
    (void) data;
    (void) argv_eol;
    
    if (ptr_channel && (ptr_channel->type == IRC_CHANNEL_TYPE_CHANNEL))
    {
        if (argc < 2)
            irc_server_sendf (ptr_server, 0, "MODE %s -o %s",
                              ptr_channel->name,
                              ptr_server->nick);
        else
            irc_command_mode_nicks (ptr_server, ptr_channel->name,
                                    "-", "o", argc, argv);
    }
    else
    {
        weechat_printf (ptr_server->buffer,
                        _("%s%s: \"%s\" command can only be executed in "
                          "a channel buffer"),
                        weechat_prefix ("error"), IRC_PLUGIN_NAME, "deop");
        return WEECHAT_RC_OK;
    }
    return WEECHAT_RC_OK;
}

/*
 * irc_command_devoice: remove voice from nickname(s)
 */

int
irc_command_devoice (void *data, struct t_gui_buffer *buffer, int argc,
                     char **argv, char **argv_eol)
{
    IRC_GET_SERVER_CHANNEL(buffer);
    IRC_COMMAND_CHECK_SERVER("devoice", 1);
    
    /* make C compiler happy */
    (void) data;
    (void) argv_eol;
    
    if (ptr_channel && (ptr_channel->type == IRC_CHANNEL_TYPE_CHANNEL))
    {
        if (argc < 2)
            irc_server_sendf (ptr_server, 0, "MODE %s -v %s",
                              ptr_channel->name,
                              ptr_server->nick);
        else
            irc_command_mode_nicks (ptr_server, ptr_channel->name,
                                    "-", "v", argc, argv);
    }
    else
    {
        weechat_printf (ptr_server->buffer,
                        _("%s%s: \"%s\" command can only be "
                          "executed in a channel buffer"),
                        weechat_prefix ("error"), IRC_PLUGIN_NAME, "devoice");
        return WEECHAT_RC_OK;
    }
    return WEECHAT_RC_OK;
}

/*
 * irc_command_die: shotdown the server
 */

int
irc_command_die (void *data, struct t_gui_buffer *buffer, int argc,
                 char **argv, char **argv_eol)
{
    IRC_GET_SERVER(buffer);
    IRC_COMMAND_CHECK_SERVER("die", 1);
    
    /* make C compiler happy */
    (void) data;
    (void) argc;
    (void) argv;
    (void) argv_eol;
    
    irc_server_sendf (ptr_server, 0, "DIE");
    
    return WEECHAT_RC_OK;
}

/*
 * irc_command_quit_server: send QUIT to a server
 */

void
irc_command_quit_server (struct t_irc_server *server, const char *arguments)
{
    const char *ptr_arg, *version;
    char *buf;
    
    if (!server)
        return;
    
    if (server->is_connected)
    {
        ptr_arg = (arguments) ? arguments :
            (weechat_config_string (irc_config_network_default_msg_quit)
             && weechat_config_string (irc_config_network_default_msg_quit)[0]) ?
            weechat_config_string (irc_config_network_default_msg_quit) : NULL;
        
        if (ptr_arg)
        {
            version = weechat_info_get ("version", "");
            buf = weechat_string_replace (ptr_arg, "%v",
                                          (version) ? version : "");
            irc_server_sendf (server, 0, "QUIT :%s",
                              (buf) ? buf : ptr_arg);
            if (buf)
                free (buf);
        }
        else
            irc_server_sendf (server, 0, "QUIT");
    }
}

/*
 * irc_command_disconnect_one_server: disconnect from a server
 *                                    return 0 if error, 1 if ok
 */

int
irc_command_disconnect_one_server (struct t_irc_server *server)
{
    if (!server)
        return 0;
    
    if ((!server->is_connected) && (!server->hook_connect)
        && (server->reconnect_start == 0))
    {
        weechat_printf (server->buffer,
                        _("%s%s: not connected to server \"%s\"!"),
                        weechat_prefix ("error"), IRC_PLUGIN_NAME,
                        server->name);
        return 0;
    }
    if (server->reconnect_start > 0)
    {
        weechat_printf (server->buffer,
                        _("%s: auto-reconnection is cancelled"),
                        IRC_PLUGIN_NAME);
    }
    irc_command_quit_server (server, NULL);
    irc_server_disconnect (server, 0);
    
    /* disconnect ok */
    return 1;
}

/*
 * irc_command_disconnect: disconnect from server(s)
 */

int
irc_command_disconnect (void *data, struct t_gui_buffer *buffer, int argc,
                        char **argv, char **argv_eol)
{
    int i, disconnect_ok;
    
    IRC_GET_SERVER(buffer);
    
    /* make C compiler happy */
    (void) data;
    (void) argv_eol;
    
    if (argc < 2)
        disconnect_ok = irc_command_disconnect_one_server (ptr_server);
    else
    {
        disconnect_ok = 1;
        
        if (weechat_strcasecmp (argv[1], "-all") == 0)
        {
            for (ptr_server = irc_servers; ptr_server;
                 ptr_server = ptr_server->next_server)
            {
                if ((ptr_server->is_connected) || (ptr_server->hook_connect)
                    || (ptr_server->reconnect_start != 0))
                {
                    if (!irc_command_disconnect_one_server (ptr_server))
                        disconnect_ok = 0;
                }
            }
        }
        else
        {
            for (i = 1; i < argc; i++)
            {
                ptr_server = irc_server_search (argv[i]);
                if (ptr_server)
                {
                    if (!irc_command_disconnect_one_server (ptr_server))
                        disconnect_ok = 0;
                }
                else
                {
                    weechat_printf (NULL,
                                    _("%s%s: server \"%s\" not found"),
                                    weechat_prefix ("error"), IRC_PLUGIN_NAME,
                                    argv[i]);
                    disconnect_ok = 0;
                }
            }
        }
    }
    
    if (!disconnect_ok)
        return WEECHAT_RC_ERROR;
    
    return WEECHAT_RC_OK;
}

/*
 * irc_command_halfop: give half operator privileges to nickname(s)
 */

int
irc_command_halfop (void *data, struct t_gui_buffer *buffer, int argc,
                    char **argv, char **argv_eol)
{
    IRC_GET_SERVER_CHANNEL(buffer);
    IRC_COMMAND_CHECK_SERVER("halfop", 1);
    
    /* make C compiler happy */
    (void) data;
    (void) argv_eol;
    
    if (ptr_channel && (ptr_channel->type == IRC_CHANNEL_TYPE_CHANNEL))
    {
        if (argc < 2)
            irc_server_sendf (ptr_server, 0, "MODE %s +h %s",
                              ptr_channel->name,
                              ptr_server->nick);
        else
            irc_command_mode_nicks (ptr_server, ptr_channel->name,
                                    "+", "h", argc, argv);
    }
    else
    {
        weechat_printf (ptr_server->buffer,
                        _("%s%s: \"%s\" command can only be "
                          "executed in a channel buffer"),
                        weechat_prefix ("error"), IRC_PLUGIN_NAME, "halfop");
        return WEECHAT_RC_OK;
    }
    return WEECHAT_RC_OK;
}

/*
 * irc_command_ignore_display: display a ignore
 */

void
irc_command_ignore_display (struct t_irc_ignore *ignore)
{
    char *mask;
    
    mask = weechat_strndup (ignore->mask + 1, strlen (ignore->mask) - 2);
    
    weechat_printf (NULL,
                    _("  %s[%s%d%s]%s mask: %s / server: %s / channel: %s"),
                    IRC_COLOR_CHAT_DELIMITERS,
                    IRC_COLOR_CHAT,
                    ignore->number,
                    IRC_COLOR_CHAT_DELIMITERS,
                    IRC_COLOR_CHAT,
                    (mask) ? mask : ignore->mask,
                    (ignore->server) ?
                    ignore->server : "*",
                    (ignore->channel) ?
                    ignore->channel : "*");
    
    if (mask)
        free (mask);
}

/*
 * irc_command_ignore: add or remove ignore
 */

int
irc_command_ignore (void *data, struct t_gui_buffer *buffer, int argc,
                    char **argv, char **argv_eol)
{
    struct t_irc_ignore *ptr_ignore;
    char *mask, *regex, *ptr_regex, *server, *channel, *error;
    long number;
    
    /* make C compiler happy */
    (void) data;
    (void) buffer;
    (void) argv_eol;
    
    if ((argc == 1)
        || ((argc == 2) && (weechat_strcasecmp (argv[1], "list") == 0)))
    {
        /* display all key bindings */
        if (irc_ignore_list)
        {
            weechat_printf (NULL, "");
            weechat_printf (NULL, _("%s: ignore list:"), IRC_PLUGIN_NAME);
            for (ptr_ignore = irc_ignore_list; ptr_ignore;
                 ptr_ignore = ptr_ignore->next_ignore)
            {
                irc_command_ignore_display (ptr_ignore);
            }
        }
        else
            weechat_printf (NULL, _("%s: no ignore in list"), IRC_PLUGIN_NAME);
        
        return WEECHAT_RC_OK;
    }
    
    /* add ignore */
    if (weechat_strcasecmp (argv[1], "add") == 0)
    {
        if (argc < 3)
        {
            weechat_printf (NULL,
                            _("%s%s: missing arguments for \"%s\" "
                              "command"),
                            weechat_prefix ("error"), IRC_PLUGIN_NAME,
                            "ignore add");
            return WEECHAT_RC_OK;
        }
        
        mask = argv[2];
        server = (argc > 3) ? argv[3] : NULL;
        channel = (argc > 4) ? argv[4] : NULL;
        
        if (strncmp (mask, "re:", 3) == 0)
        {
            regex = NULL;
            ptr_regex = mask + 3;
        }
        else
        {
            regex = weechat_string_mask_to_regex (mask);
            ptr_regex = (regex) ? regex : mask;
        }
        
        if (irc_ignore_search (ptr_regex, server, channel))
        {
            weechat_printf (NULL,
                            _("%s%s: ignore already exists"),
                            weechat_prefix ("error"), IRC_PLUGIN_NAME);
            return WEECHAT_RC_OK;
        }
        
        ptr_ignore = irc_ignore_new (ptr_regex, server, channel);
        
        if (regex)
            free (regex);
        
        if (ptr_ignore)
        {
            weechat_printf (NULL, "");
            weechat_printf (NULL, _("%s: ignore added:"), IRC_PLUGIN_NAME);
            irc_command_ignore_display (ptr_ignore);
        }
        else
        {
            weechat_printf (NULL, _("%s%s: error adding ignore"),
                            weechat_prefix ("error"), IRC_PLUGIN_NAME);
        }
        
        return WEECHAT_RC_OK;
    }
    
    /* delete ignore */
    if (weechat_strcasecmp (argv[1], "del") == 0)
    {
        if (argc < 3)
        {
            weechat_printf (NULL,
                            _("%s%s: missing arguments for \"%s\" "
                              "command"),
                            weechat_prefix ("error"), IRC_PLUGIN_NAME,
                            "ignore del");
            return WEECHAT_RC_OK;
        }
        
        if (weechat_strcasecmp (argv[2], "-all") == 0)
        {
            if (irc_ignore_list)
            {
                irc_ignore_free_all ();
                weechat_printf (NULL, _("%s: all ignores deleted"),
                                IRC_PLUGIN_NAME);
            }
            else
            {
                weechat_printf (NULL, _("%s: no ignore in list"),
                                IRC_PLUGIN_NAME);
            }
        }
        else
        {
            error = NULL;
            number = strtol (argv[2], &error, 10);
            if (error && !error[0])
            {
                ptr_ignore = irc_ignore_search_by_number (number);
                if (ptr_ignore)
                {
                    irc_ignore_free (ptr_ignore);
                    weechat_printf (NULL, _("%s: ignore deleted"),
                                    IRC_PLUGIN_NAME);
                }
                else
                {
                    weechat_printf (NULL,
                                    _("%s%s: ignore not found"),
                                    weechat_prefix ("error"), IRC_PLUGIN_NAME);
                    return WEECHAT_RC_OK;
                }
            }
            else
            {
                weechat_printf (NULL,
                                _("%s%s: wrong ignore number"),
                                weechat_prefix ("error"), IRC_PLUGIN_NAME);
                return WEECHAT_RC_OK;
            }
        }
        
        return WEECHAT_RC_OK;
    }
    
    weechat_printf (NULL,
                    _("%s%s: unknown option for \"%s\" "
                      "command"),
                    weechat_prefix ("error"), IRC_PLUGIN_NAME, "ignore");
    return WEECHAT_RC_OK;
}

/*
 * irc_command_info: get information describing the server
 */

int
irc_command_info (void *data, struct t_gui_buffer *buffer, int argc,
                  char **argv, char **argv_eol)
{
    IRC_GET_SERVER(buffer);
    IRC_COMMAND_CHECK_SERVER("info", 1);
    
    /* make C compiler happy */
    (void) data;
    (void) argv;
    
    if (argc > 1)
        irc_server_sendf (ptr_server, 0, "INFO %s", argv_eol[1]);
    else
        irc_server_sendf (ptr_server, 0, "INFO");
    
    return WEECHAT_RC_OK;
}

/*
 * irc_command_invite: invite a nick on a channel
 */

int
irc_command_invite (void *data, struct t_gui_buffer *buffer, int argc,
                    char **argv, char **argv_eol)
{
    IRC_GET_SERVER_CHANNEL(buffer);
    IRC_COMMAND_CHECK_SERVER("invite", 1);
    
    /* make C compiler happy */
    (void) data;
    (void) argv_eol;
    
    if (argc > 2)
        irc_server_sendf (ptr_server, 0, "INVITE %s %s", argv[1], argv[2]);
    else
    {
        if (ptr_channel && (ptr_channel->type == IRC_CHANNEL_TYPE_CHANNEL))
            irc_server_sendf (ptr_server, 0, "INVITE %s %s",
                              argv[1], ptr_channel->name);
        else
        {
            weechat_printf (ptr_server->buffer,
                            _("%s%s: \"%s\" command can only be "
                              "executed in a channel buffer"),
                            weechat_prefix ("error"), IRC_PLUGIN_NAME,
                            "invite");
            return WEECHAT_RC_OK;
        }

    }
    
    return WEECHAT_RC_OK;
}

/*
 * irc_command_ison: check if a nickname is currently on IRC
 */

int
irc_command_ison (void *data, struct t_gui_buffer *buffer, int argc,
                  char **argv, char **argv_eol)
{
    IRC_GET_SERVER(buffer);
    IRC_COMMAND_CHECK_SERVER("ison", 1);
    
    /* make C compiler happy */
    (void) data;
    (void) argv;
    
    if (argc > 1)
        irc_server_sendf (ptr_server, 0, "ISON %s", argv_eol[1]);
    else
    {
        IRC_COMMAND_TOO_FEW_ARGUMENTS(ptr_server->buffer, "ison");
    }
    
    return WEECHAT_RC_OK;
}

/*
 * irc_command_join_server: send JOIN command on a server
 */

void
irc_command_join_server (struct t_irc_server *server, const char *arguments)
{
    if (irc_channel_is_channel (arguments))
        irc_server_sendf (server, 0, "JOIN %s", arguments);
    else
        irc_server_sendf (server, 0, "JOIN #%s", arguments);
}

/*
 * irc_command_join: join a new channel
 */

int
irc_command_join (void *data, struct t_gui_buffer *buffer, int argc,
                  char **argv, char **argv_eol)
{
    IRC_GET_SERVER_CHANNEL(buffer);
    IRC_COMMAND_CHECK_SERVER("join", 1);
    
    /* make C compiler happy */
    (void) data;
    (void) argv;
    
    if (argc > 1)
        irc_command_join_server (ptr_server, argv_eol[1]);
    else
    {
        if (ptr_channel && (ptr_channel->type == IRC_CHANNEL_TYPE_CHANNEL)
            && !ptr_channel->nicks)
        {
            irc_command_join_server (ptr_server, ptr_channel->name);
        }
        else
        {
            IRC_COMMAND_TOO_FEW_ARGUMENTS(ptr_server->buffer, "join");
        }
    }
    
    return WEECHAT_RC_OK;
}

/*
 * irc_command_kick: forcibly remove a user from a channel
 */

int
irc_command_kick (void *data, struct t_gui_buffer *buffer, int argc,
                  char **argv, char **argv_eol)
{
    char *pos_channel, *pos_nick, *pos_comment;
    
    IRC_GET_SERVER_CHANNEL(buffer);
    IRC_COMMAND_CHECK_SERVER("kick", 1);
    
    /* make C compiler happy */
    (void) data;
    
    if (argc > 1)
    {
        if (irc_channel_is_channel (argv[1]))
        {
            if (argc < 3)
            {
                weechat_printf (ptr_server->buffer,
                                _("%s%s: wrong arguments for \"%s\" "
                                  "command"),
                                weechat_prefix ("error"), IRC_PLUGIN_NAME,
                                "kick");
                return WEECHAT_RC_OK;
            }
            pos_channel = argv[1];
            pos_nick = argv[2];
            pos_comment = argv_eol[3];
        }
        else
        {
            if (ptr_channel && (ptr_channel->type == IRC_CHANNEL_TYPE_CHANNEL))
            {
                pos_channel = ptr_channel->name;
                pos_nick = argv[1];
                pos_comment = argv_eol[2];
            }
            else
            {
                weechat_printf (ptr_server->buffer,
                                _("%s%s: \"%s\" command can only be "
                                  "executed in a channel buffer"),
                                weechat_prefix ("error"), IRC_PLUGIN_NAME,
                                "kick");
                return WEECHAT_RC_OK;
            }
        }
        
        if (pos_comment)
            irc_server_sendf (ptr_server, 0, "KICK %s %s :%s",
                              pos_channel, pos_nick, pos_comment);
        else
            irc_server_sendf (ptr_server, 0, "KICK %s %s",
                              pos_channel, pos_nick);
    }
    else
    {
        IRC_COMMAND_TOO_FEW_ARGUMENTS(ptr_server->buffer, "kick");
    }
    
    return WEECHAT_RC_OK;
}

/*
 * irc_command_kickban: forcibly remove a user from a channel and ban it
 */

int
irc_command_kickban (void *data, struct t_gui_buffer *buffer, int argc,
                     char **argv, char **argv_eol)
{
    char *pos_channel, *pos_nick, *nick_only, *pos_comment, *pos;
    
    IRC_GET_SERVER_CHANNEL(buffer);
    IRC_COMMAND_CHECK_SERVER("kickban", 1);
    
    /* make C compiler happy */
    (void) data;

    if (argc > 1)
    {
        if (irc_channel_is_channel (argv[1]))
        {
            if (argc < 3)
            {
                weechat_printf (ptr_server->buffer,
                                _("%s%s: wrong arguments for \"%s\" "
                                  "command"),
                                weechat_prefix ("error"), IRC_PLUGIN_NAME,
                                "kickban");
                return WEECHAT_RC_OK;
            }
            pos_channel = argv[1];
            pos_nick = argv[2];
            pos_comment = argv_eol[3];
        }
        else
        {
            if (ptr_channel && (ptr_channel->type == IRC_CHANNEL_TYPE_CHANNEL))
            {
                pos_channel = ptr_channel->name;
                pos_nick = argv[1];
                pos_comment = argv_eol[2];
            }
            else
            {
                weechat_printf (ptr_server->buffer,
                                _("%s%s: \"%s\" command can only be "
                                  "executed in a channel buffer"),
                                weechat_prefix ("error"), IRC_PLUGIN_NAME,
                                "kickban");
                return WEECHAT_RC_OK;
            }
        }

        /* set ban for nick(+host) on channel */
        irc_server_sendf (ptr_server, 0, "MODE %s +b %s",
                          pos_channel, pos_nick);
        
        /* kick nick from channel */
        nick_only = strdup (pos_nick);
        if (nick_only)
        {
            pos = strchr (nick_only, '@');
            if (pos)
                pos[0] = '\0';
            pos = strchr (nick_only, '!');
            if (pos)
                pos[0] = '\0';
            irc_server_sendf (ptr_server, 0, "KICK %s %s%s%s",
                              pos_channel,
                              nick_only,
                              (pos_comment) ? " :" : "",
                              (pos_comment) ? pos_comment : "");
            free (nick_only);
        }
    }
    else
    {
        IRC_COMMAND_TOO_FEW_ARGUMENTS(ptr_server->buffer, "kickban");
    }
    
    return WEECHAT_RC_OK;
}

/*
 * irc_command_kill: close client-server connection
 */

int
irc_command_kill (void *data, struct t_gui_buffer *buffer, int argc,
                  char **argv, char **argv_eol)
{
    IRC_GET_SERVER(buffer);
    IRC_COMMAND_CHECK_SERVER("kill", 1);
    
    /* make C compiler happy */
    (void) data;
    
    if (argc > 2)
    {
        irc_server_sendf (ptr_server, 0, "KILL %s :%s",
                          argv[1], argv_eol[2]);
    }
    else
    {
        IRC_COMMAND_TOO_FEW_ARGUMENTS(ptr_server->buffer, "kill");
    }
    
    return WEECHAT_RC_OK;
}

/*
 * irc_command_links: list all servernames which are known by the server
 *                    answering the query
 */

int
irc_command_links (void *data, struct t_gui_buffer *buffer, int argc,
                   char **argv, char **argv_eol)
{
    IRC_GET_SERVER(buffer);
    IRC_COMMAND_CHECK_SERVER("links", 1);
    
    /* make C compiler happy */
    (void) data;
    (void) argv;
    
    if (argc > 1)
        irc_server_sendf (ptr_server, 0, "LINKS %s", argv_eol[1]);
    else
        irc_server_sendf (ptr_server, 0, "LINKS");
    
    return WEECHAT_RC_OK;
}

/*
 * irc_command_list: close client-server connection
 */

int
irc_command_list (void *data, struct t_gui_buffer *buffer, int argc,
                  char **argv, char **argv_eol)
{
    char buf[512];
    int ret;
    
    IRC_GET_SERVER(buffer);
    IRC_COMMAND_CHECK_SERVER("list", 1);
    
    /* make C compiler happy */
    (void) data;
    (void) argv;
    
    if (ptr_server->cmd_list_regexp)
    {
	regfree (ptr_server->cmd_list_regexp);
	free (ptr_server->cmd_list_regexp);
	ptr_server->cmd_list_regexp = NULL;
    }
    
    if (argc > 1)
    {
	ptr_server->cmd_list_regexp = malloc (sizeof (*ptr_server->cmd_list_regexp));
	if (ptr_server->cmd_list_regexp)
	{
	    if ((ret = regcomp (ptr_server->cmd_list_regexp,
                                argv_eol[1],
                                REG_NOSUB | REG_ICASE)) != 0)
	    {
		regerror (ret, ptr_server->cmd_list_regexp,
                          buf, sizeof(buf));
		weechat_printf (ptr_server->buffer,
                                _("%s%s: \"%s\" is not a valid regular "
                                  "expression (%s)"),
                                weechat_prefix ("error"), IRC_PLUGIN_NAME,
                                argv_eol, buf);
                return WEECHAT_RC_OK;
	    }
	    else
		irc_server_sendf (ptr_server, 0, "LIST");
	}
	else
	{
	    weechat_printf (ptr_server->buffer,
                            _("%s%s: not enough memory for regular "
                              "expression"),
                            weechat_prefix ("error"), IRC_PLUGIN_NAME);
            return WEECHAT_RC_OK;
	}
    }
    else
	irc_server_sendf (ptr_server, 0, "LIST");
    
    return WEECHAT_RC_OK;
}

/*
 * irc_command_lusers: get statistics about ths size of the IRC network
 */

int
irc_command_lusers (void *data, struct t_gui_buffer *buffer, int argc,
                    char **argv, char **argv_eol)
{
    IRC_GET_SERVER(buffer);
    IRC_COMMAND_CHECK_SERVER("lusers", 1);
    
    /* make C compiler happy */
    (void) data;
    (void) argv;
    
    if (argc > 1)
        irc_server_sendf (ptr_server, 0, "LUSERS %s", argv_eol[1]);
    else
        irc_server_sendf (ptr_server, 0, "LUSERS");
    
    return WEECHAT_RC_OK;
}

/*
 * irc_command_me: send a ctcp action to the current channel
 */

int
irc_command_me (void *data, struct t_gui_buffer *buffer, int argc, char **argv,
                char **argv_eol)
{
    IRC_GET_SERVER_CHANNEL(buffer);
    IRC_COMMAND_CHECK_SERVER("me", 1);
    
    /* make C compiler happy */
    (void) data;
    (void) argv;
    
    if (!ptr_channel)
    {
        weechat_printf (ptr_server->buffer,
                        _("%s%s: \"%s\" command can not be executed "
                          "on a server buffer"),
                        weechat_prefix ("error"), IRC_PLUGIN_NAME, "me");
        return WEECHAT_RC_OK;
    }
    irc_command_me_channel (ptr_server, ptr_channel,
                            (argc > 1) ? argv_eol[1] : NULL);
    
    return WEECHAT_RC_OK;
}

/*
 * irc_command_mode_server: send MODE command on a server
 */

void
irc_command_mode_server (struct t_irc_server *server,
                         struct t_irc_channel *channel, const char *arguments)
{
    if (server && (channel || arguments))
    {
        if (channel && arguments)
        {
            irc_server_sendf (server, 0, "MODE %s %s",
                              channel->name, arguments);
        }
        else
        {
            irc_server_sendf (server, 0, "MODE %s",
                              (channel) ? channel->name : arguments);
        }
    }
}

/*
 * irc_command_mode: change mode for channel/nickname
 */

int
irc_command_mode (void *data, struct t_gui_buffer *buffer, int argc,
                  char **argv, char **argv_eol)
{
    IRC_GET_SERVER_CHANNEL(buffer);
    IRC_COMMAND_CHECK_SERVER("mode", 1);
    
    /* make C compiler happy */
    (void) data;
    
    if (argc > 1)
    {
        if ((argv[1][0] == '+') || (argv[1][0] == '-'))
        {
            /* channel not specified, check we are on channel and use it */
            if (!ptr_channel)
            {
                weechat_printf (ptr_server->buffer,
                                _("%s%s: you must specify channel for \"%s\" "
                                  "command if you're not in a channel"),
                                weechat_prefix ("error"), IRC_PLUGIN_NAME,
                                "mode");
                return WEECHAT_RC_OK;
            }
            irc_command_mode_server (ptr_server, ptr_channel, argv_eol[1]);
        }
        else
        {
            /* user gives channel, use arguments as-is */
            irc_command_mode_server (ptr_server, NULL, argv_eol[1]);
        }
    }
    else
    {
        IRC_COMMAND_TOO_FEW_ARGUMENTS(ptr_server->buffer, "mode");
    }
    
    return WEECHAT_RC_OK;
}

/*
 * irc_command_motd: get the "Message Of The Day"
 */

int
irc_command_motd (void *data, struct t_gui_buffer *buffer, int argc,
                  char **argv, char **argv_eol)
{
    IRC_GET_SERVER(buffer);
    IRC_COMMAND_CHECK_SERVER("motd", 1);
    
    /* make C compiler happy */
    (void) data;
    (void) argv;
    
    if (argc > 1)
        irc_server_sendf (ptr_server, 0, "MOTD %s", argv_eol[1]);
    else
        irc_server_sendf (ptr_server, 0, "MOTD");
    
    return WEECHAT_RC_OK;
}

/*
 * irc_command_msg: send a message to a nick or channel
 */

int
irc_command_msg (void *data, struct t_gui_buffer *buffer, int argc,
                 char **argv, char **argv_eol)
{
    char **targets;
    int num_targets, i, arg_target, arg_text;
    char *msg_pwd_hidden;
    struct t_irc_nick *ptr_nick;
    char *string;
    
    IRC_GET_SERVER_CHANNEL(buffer);
    
    /* make C compiler happy */
    (void) data;
    
    if (argc <= 2)
    {
        IRC_COMMAND_TOO_FEW_ARGUMENTS((ptr_server) ? ptr_server->buffer : NULL,
                                      "msg");
    }
    
    arg_target = 1;
    arg_text = 2;
    
    if ((argc >= 5) && (weechat_strcasecmp (argv[1], "-server") == 0))
    {
        ptr_server = irc_server_search (argv[2]);
        ptr_channel = NULL;
        arg_target = 3;
        arg_text = 4;
    }
    
    IRC_COMMAND_CHECK_SERVER("msg", 1);
    
    targets = weechat_string_split (argv[arg_target], ",", 0, 0,
                                    &num_targets);
    if (targets)
    {
        for (i = 0; i < num_targets; i++)
        {
            if (strcmp (targets[i], "*") == 0)
            {
                if (!ptr_channel
                    || ((ptr_channel->type != IRC_CHANNEL_TYPE_CHANNEL)
                        && (ptr_channel->type != IRC_CHANNEL_TYPE_PRIVATE)))
                {
                    weechat_printf (ptr_server->buffer,
                                    _("%s%s: \"%s\" command can only be "
                                      "executed in a channel or private "
                                      "buffer"),
                                    weechat_prefix ("error"),
                                    IRC_PLUGIN_NAME, "msg *");
                    return WEECHAT_RC_OK;
                }
                if (ptr_channel->type == IRC_CHANNEL_TYPE_CHANNEL)
                    ptr_nick = irc_nick_search (ptr_channel, ptr_server->nick);
                else
                    ptr_nick = NULL;
                string = irc_color_decode (argv_eol[arg_text],
                                           weechat_config_boolean (irc_config_network_colors_receive));
                irc_input_user_message_display (ptr_channel->buffer,
                                                (string) ? string : argv_eol[arg_text]);
                if (string)
                    free (string);
                
                irc_server_sendf (ptr_server, 1, "PRIVMSG %s :%s",
                                  ptr_channel->name, argv_eol[arg_text]);
            }
            else
            {
                if (irc_channel_is_channel (targets[i]))
                {
                    ptr_channel = irc_channel_search (ptr_server,
                                                      targets[i]);
                    if (ptr_channel)
                    {
                        if (ptr_channel->type == IRC_CHANNEL_TYPE_CHANNEL)
                            ptr_nick = irc_nick_search (ptr_channel, ptr_server->nick);
                        else
                            ptr_nick = NULL;
                        string = irc_color_decode (argv_eol[arg_text],
                                                   weechat_config_boolean (irc_config_network_colors_receive));
                        irc_input_user_message_display (ptr_channel->buffer,
                                                        (string) ? string : argv_eol[arg_text]);
                        if (string)
                            free (string);
                    }
                    irc_server_sendf (ptr_server, 1, "PRIVMSG %s :%s",
                                      targets[i], argv_eol[arg_text]);
                }
                else
                {
                    /* message to nickserv with identify ? */
                    if (weechat_strcasecmp (targets[i], "nickserv") == 0)
                    {
                        msg_pwd_hidden = strdup (argv_eol[arg_text]);
                        if (msg_pwd_hidden
                            && (weechat_config_boolean (irc_config_look_hide_nickserv_pwd)))
                            irc_display_hide_password (msg_pwd_hidden, 0);
                        string = irc_color_decode (
                            (msg_pwd_hidden) ? msg_pwd_hidden : argv_eol[arg_text],
                            weechat_config_boolean (irc_config_network_colors_receive));
                        weechat_printf (ptr_server->buffer,
                                        "%s%s-%s%s%s- %s%s",
                                        weechat_prefix ("network"),
                                        IRC_COLOR_CHAT_DELIMITERS,
                                        IRC_COLOR_CHAT_NICK,
                                        targets[i],
                                        IRC_COLOR_CHAT_DELIMITERS,
                                        IRC_COLOR_CHAT,
                                        (string) ?
                                        string : ((msg_pwd_hidden) ?
                                                  msg_pwd_hidden : argv_eol[arg_text]));
                        if (string)
                            free (string);
                        if (msg_pwd_hidden)
                            free (msg_pwd_hidden);
                    }
                    else
                    {
                        string = irc_color_decode (argv_eol[arg_text],
                                                   weechat_config_boolean (irc_config_network_colors_receive));
                        ptr_channel = irc_channel_search (ptr_server,
                                                          targets[i]);
                        if (ptr_channel)
                        {
                            irc_input_user_message_display (ptr_channel->buffer,
                                                            (string) ? string : argv_eol[arg_text]);
                        }
                        else
                        {
                            weechat_printf (ptr_server->buffer,
                                            "%sMSG%s(%s%s%s)%s: %s",
                                            weechat_prefix ("network"),
                                            IRC_COLOR_CHAT_DELIMITERS,
                                            IRC_COLOR_CHAT_NICK,
                                            targets[i],
                                            IRC_COLOR_CHAT_DELIMITERS,
                                            IRC_COLOR_CHAT,
                                            (string) ? string : argv_eol[arg_text]);
                        }
                        if (string)
                            free (string);
                    }
                    irc_server_sendf (ptr_server, 1, "PRIVMSG %s :%s",
                                      targets[i], argv_eol[arg_text]);
                }
            }
        }
        weechat_string_free_split (targets);
    }
    
    return WEECHAT_RC_OK;
}

/*
 * irc_command_names: list nicknames on channels
 */

int
irc_command_names (void *data, struct t_gui_buffer *buffer, int argc,
                   char **argv, char **argv_eol)
{
    IRC_GET_SERVER_CHANNEL(buffer);
    IRC_COMMAND_CHECK_SERVER("names", 1);
    
    /* make C compiler happy */
    (void) data;
    (void) argv;
    
    if (argc > 1)
        irc_server_sendf (ptr_server, 0, "NAMES %s", argv_eol[1]);
    else
    {
        if (ptr_channel && (ptr_channel->type == IRC_CHANNEL_TYPE_CHANNEL))
            irc_server_sendf (ptr_server, 0, "NAMES %s",
                              ptr_channel->name);
        else
        {
            weechat_printf (ptr_server->buffer,
                            _("%s%s: \"%s\" command can only be "
                              "executed in a channel buffer"),
                            weechat_prefix ("error"), IRC_PLUGIN_NAME,
                            "names");
            return WEECHAT_RC_OK;
        }
    }
    
    return WEECHAT_RC_OK;
}

/*
 * irc_send_nick_server: change nickname on a server
 */

void
irc_send_nick_server (struct t_irc_server *server, const char *nickname)
{
    if (!server)
        return;
    
    if (server->is_connected)
        irc_server_sendf (server, 0, "NICK %s", nickname);
    else
        irc_server_set_nick (server, nickname);
}

/*
 * irc_command_nick: change nickname
 */

int
irc_command_nick (void *data, struct t_gui_buffer *buffer, int argc,
                  char **argv, char **argv_eol)
{
    IRC_GET_SERVER(buffer);
    IRC_COMMAND_CHECK_SERVER("nick", 0);
    
    /* make C compiler happy */
    (void) data;
    (void) argv_eol;
    
    if (argc > 2)
    {
        if (weechat_strcasecmp (argv[1], "-all") != 0)
        {
            weechat_printf (ptr_server->buffer,
                            _("%s%s: wrong arguments for \"%s\" command"),
                            weechat_prefix ("error"), IRC_PLUGIN_NAME, "nick");
            return WEECHAT_RC_OK;
        }
        for (ptr_server = irc_servers; ptr_server;
             ptr_server = ptr_server->next_server)
        {
            irc_send_nick_server (ptr_server, argv[2]);
        }
    }
    else
    {
        if (argc > 1)
            irc_send_nick_server (ptr_server, argv[1]);
        else
        {
            IRC_COMMAND_TOO_FEW_ARGUMENTS(ptr_server->buffer, "nick");
        }
    }
    
    return WEECHAT_RC_OK;
}

/*
 * irc_command_notice: send notice message
 */

int
irc_command_notice (void *data, struct t_gui_buffer *buffer, int argc,
                    char **argv, char **argv_eol)
{
    char *string;
    int arg_nick, arg_text;
    
    IRC_GET_SERVER(buffer);
    
    /* make C compiler happy */
    (void) data;
    
    if (argc > 2)
    {
        arg_nick = 1;
        arg_text = 2;
        if ((argc >= 5) && (weechat_strcasecmp (argv[1], "-server") == 0))
        {
            ptr_server = irc_server_search (argv[2]);
            arg_nick = 3;
            arg_text = 4;
        }
        
        IRC_COMMAND_CHECK_SERVER("notice", 1);
        
        string = irc_color_decode (argv_eol[arg_text],
                                   weechat_config_boolean (irc_config_network_colors_receive));
        weechat_printf (ptr_server->buffer,
                        "Notice -> %s%s%s: %s",
                        IRC_COLOR_CHAT_NICK,
                        argv[arg_nick],
                        IRC_COLOR_CHAT,
                        (string) ? string : argv_eol[arg_text]);
        if (string)
            free (string);
        irc_server_sendf (ptr_server, 1, "NOTICE %s :%s",
                          argv[arg_nick], argv_eol[arg_text]);
    }
    else
    {
        IRC_COMMAND_TOO_FEW_ARGUMENTS((ptr_server) ? ptr_server->buffer : NULL,
                                      "notice");
    }
    
    return WEECHAT_RC_OK;
}

/*
 * irc_command_op: give operator privileges to nickname(s)
 */

int
irc_command_op (void *data, struct t_gui_buffer *buffer, int argc, char **argv,
                char **argv_eol)
{
    IRC_GET_SERVER_CHANNEL(buffer);
    IRC_COMMAND_CHECK_SERVER("op", 1);
    
    /* make C compiler happy */
    (void) data;
    (void) argv_eol;
    
    if (ptr_channel && (ptr_channel->type == IRC_CHANNEL_TYPE_CHANNEL))
    {
        if (argc < 2)
            irc_server_sendf (ptr_server, 0, "MODE %s +o %s",
                              ptr_channel->name,
                              ptr_server->nick);
        else
            irc_command_mode_nicks (ptr_server, ptr_channel->name,
                                    "+", "o", argc, argv);
    }
    else
    {
        weechat_printf (ptr_server->buffer,
                        _("%s%s: \"%s\" command can only be "
                          "executed in a channel buffer"),
                        weechat_prefix ("error"), IRC_PLUGIN_NAME, "op");
        return WEECHAT_RC_OK;
    }
    return WEECHAT_RC_OK;
}

/*
 * irc_command_oper: get oper privileges
 */

int
irc_command_oper (void *data, struct t_gui_buffer *buffer, int argc,
                  char **argv, char **argv_eol)
{
    IRC_GET_SERVER(buffer);
    IRC_COMMAND_CHECK_SERVER("oper", 1);
    
    /* make C compiler happy */
    (void) data;
    (void) argv;

    if (argc > 2)
        irc_server_sendf (ptr_server, 0, "OPER %s", argv_eol[1]);
    else
    {
        IRC_COMMAND_TOO_FEW_ARGUMENTS(ptr_server->buffer, "oper");
    }
    
    return WEECHAT_RC_OK;
}

/*
 * irc_command_part_channel: send a part message for a channel
 */

void
irc_command_part_channel (struct t_irc_server *server, const char *channel_name,
                          const char *part_message)
{
    const char *ptr_arg;
    char *buf;
    const char *version;
    
    ptr_arg = (part_message) ? part_message :
        (weechat_config_string (irc_config_network_default_msg_part)
         && weechat_config_string (irc_config_network_default_msg_part)[0]) ?
        weechat_config_string (irc_config_network_default_msg_part) : NULL;
    
    if (ptr_arg)
    {
        version = weechat_info_get ("version", "");
        buf = weechat_string_replace (ptr_arg, "%v", (version) ? version : "");
        irc_server_sendf (server, 0, "PART %s :%s",
                          channel_name,
                          (buf) ? buf : ptr_arg);
        if (buf)
            free (buf);
    }
    else
        irc_server_sendf (server, 0, "PART %s", channel_name);
}

/*
 * irc_command_part: leave a channel or close a private window
 */

int
irc_command_part (void *data, struct t_gui_buffer *buffer, int argc,
                  char **argv, char **argv_eol)
{
    char *channel_name, *pos_args;

    IRC_GET_SERVER_CHANNEL(buffer);
    IRC_COMMAND_CHECK_SERVER("part", 1);
    
    /* make C compiler happy */
    (void) data;
    
    if (argc > 1)
    {
        if (irc_channel_is_channel (argv[1]))
        {
            channel_name = argv[1];
            pos_args = argv_eol[2];
        }
        else
        {
            if (!ptr_channel)
            {
                weechat_printf (ptr_server->buffer,
                                _("%s%s: \"%s\" command can only be "
                                  "executed in a channel or "
                                  "private buffer"),
                                weechat_prefix ("error"), IRC_PLUGIN_NAME,
                                "part");
                return WEECHAT_RC_OK;
            }
            channel_name = ptr_channel->name;
            pos_args = argv_eol[1];
        }
    }
    else
    {
        if (!ptr_channel)
        {
            weechat_printf (ptr_server->buffer,
                            _("%s%s: \"%s\" command can only be "
                              "executed in a channel or private "
                              "buffer"),
                            weechat_prefix ("error"), IRC_PLUGIN_NAME, "part");
            return WEECHAT_RC_OK;
        }
        if (!ptr_channel->nicks)
        {
            weechat_buffer_close (ptr_channel->buffer);
            return WEECHAT_RC_OK;
        }
        channel_name = ptr_channel->name;
        pos_args = NULL;
    }
    
    irc_command_part_channel (ptr_server, channel_name, pos_args);
    
    return WEECHAT_RC_OK;
}

/*
 * irc_command_ping: ping a server
 */

int
irc_command_ping (void *data, struct t_gui_buffer *buffer, int argc,
                  char **argv, char **argv_eol)
{
    IRC_GET_SERVER(buffer);
    IRC_COMMAND_CHECK_SERVER("ping", 1);
    
    /* make C compiler happy */
    (void) data;
    (void) argv;

    if (argc > 1)
        irc_server_sendf (ptr_server, 0, "PING %s", argv_eol[1]);
    else
    {
        IRC_COMMAND_TOO_FEW_ARGUMENTS(ptr_server->buffer, "ping");
    }
    
    return WEECHAT_RC_OK;
}

/*
 * irc_command_pong: send pong answer to a daemon
 */

int
irc_command_pong (void *data, struct t_gui_buffer *buffer, int argc,
                  char **argv, char **argv_eol)
{
    IRC_GET_SERVER(buffer);
    IRC_COMMAND_CHECK_SERVER("pong", 0);
    
    /* make C compiler happy */
    (void) data;
    (void) argv;
    
    if (argc > 1)
        irc_server_sendf (ptr_server, 0, "PONG %s", argv_eol[1]);
    else
    {
        IRC_COMMAND_TOO_FEW_ARGUMENTS(ptr_server->buffer, "pong");
    }
    
    return WEECHAT_RC_OK;
}

/*
 * irc_command_query: start private conversation with a nick
 */

int
irc_command_query (void *data, struct t_gui_buffer *buffer, int argc,
                   char **argv, char **argv_eol)
{
    char *string;
    int arg_nick, arg_text;
    
    IRC_GET_SERVER_CHANNEL(buffer);
    
    /* make C compiler happy */
    (void) data;
    
    if (argc > 1)
    {
        arg_nick = 1;
        arg_text = 2;
        if ((argc >= 4) && (weechat_strcasecmp (argv[1], "-server") == 0))
        {
            ptr_server = irc_server_search (argv[2]);
            arg_nick = 3;
            arg_text = 4;
        }
        
        IRC_COMMAND_CHECK_SERVER("query", 1);
        
        /* create private window if not already opened */
        ptr_channel = irc_channel_search (ptr_server, argv[arg_nick]);
        if (!ptr_channel)
        {
            ptr_channel = irc_channel_new (ptr_server,
                                           IRC_CHANNEL_TYPE_PRIVATE,
                                           argv[arg_nick], 1, 0);
            if (!ptr_channel)
            {
                weechat_printf (ptr_server->buffer,
                                _("%s%s: cannot create new private "
                                  "buffer \"%s\""),
                                weechat_prefix ("error"), IRC_PLUGIN_NAME,
                                argv[arg_nick]);
                return WEECHAT_RC_OK;
            }
        }
        weechat_buffer_set (ptr_channel->buffer, "display", "1");
        
        /* display text if given */
        if (argv_eol[arg_text])
        {
            string = irc_color_decode (argv_eol[arg_text],
                                       weechat_config_boolean (irc_config_network_colors_receive));
            irc_input_user_message_display (ptr_channel->buffer,
                                            (string) ? string : argv_eol[arg_text]);
            if (string)
                free (string);
            irc_server_sendf (ptr_server, 1, "PRIVMSG %s :%s",
                              argv[arg_nick], argv_eol[arg_text]);
        }
    }
    else
    {
        IRC_COMMAND_TOO_FEW_ARGUMENTS((ptr_server) ? ptr_server->buffer : NULL,
                                      "query");
    }
    
    return WEECHAT_RC_OK;
}

/*
 * irc_command_quote: send raw data to server
 */

int
irc_command_quote (void *data, struct t_gui_buffer *buffer, int argc,
                   char **argv, char **argv_eol)
{
    IRC_GET_SERVER(buffer);
    
    /* make C compiler happy */
    (void) data;
    
    if (argc > 1)
    {
        if ((argc >= 4) && (weechat_strcasecmp (argv[1], "-server") == 0))
        {
            ptr_server = irc_server_search (argv[2]);
            if (!ptr_server || (ptr_server->sock < 0))
                return WEECHAT_RC_ERROR;
            irc_server_sendf (ptr_server, 1, "%s", argv_eol[3]);
        }
        else
        {
            if (!ptr_server || (ptr_server->sock < 0))
                return WEECHAT_RC_ERROR;
            irc_server_sendf (ptr_server, 1, "%s", argv_eol[1]);
        }
    }
    else
    {
        IRC_COMMAND_TOO_FEW_ARGUMENTS((ptr_server) ? ptr_server->buffer : NULL,
                                      "quote");
    }
    
    return WEECHAT_RC_OK;
}

/*
 * irc_command_reconnect_one_server: reconnect to a server
 *                               return 0 if error, 1 if ok
 */

int
irc_command_reconnect_one_server (struct t_irc_server *server, int no_join)
{
    if (!server)
        return 0;
    
    if ((!server->is_connected) && (!server->hook_connect))
    {
        weechat_printf (server->buffer,
                        _("%s%s: not connected to server \"%s\"!"),
                        weechat_prefix ("error"), IRC_PLUGIN_NAME,
                        server->name);
        return 0;
    }
    irc_command_quit_server (server, NULL);
    irc_server_disconnect (server, 0);
    server->disable_autojoin = no_join;
    if (irc_server_connect (server))
    {
        server->reconnect_start = 0;
        server->reconnect_join = (server->channels) ? 1 : 0;    
    }
    
    /* reconnect ok */
    return 1;
}

/*
 * irc_command_reconnect: reconnect to server(s)
 */

int
irc_command_reconnect (void *data, struct t_gui_buffer *buffer, int argc,
                       char **argv, char **argv_eol)
{
    int i, nb_reconnect, reconnect_ok, all_servers, no_join;

    IRC_GET_SERVER(buffer);
    
    /* make C compiler happy */
    (void) data;
    (void) argv_eol;
    
    nb_reconnect = 0;
    reconnect_ok = 1;
    
    all_servers = 0;
    no_join = 0;
    for (i = 1; i < argc; i++)
    {
        if (weechat_strcasecmp (argv[i], "-all") == 0)
            all_servers = 1;
        if (weechat_strcasecmp (argv[i], "-nojoin") == 0)
            no_join = 1;
    }
    
    if (all_servers)
    {
        for (ptr_server = irc_servers; ptr_server;
             ptr_server = ptr_server->next_server)
        {
            nb_reconnect++;
            if ((ptr_server->is_connected) || (ptr_server->hook_connect))
            {
                if (!irc_command_reconnect_one_server (ptr_server, no_join))
                    reconnect_ok = 0;
            }
        }
    }
    else
    {
        for (i = 1; i < argc; i++)
        {
            if (argv[i][0] != '-')
            {
                nb_reconnect++;
                ptr_server = irc_server_search (argv[i]);
                if (ptr_server)
                {
                    if (!irc_command_reconnect_one_server (ptr_server, no_join))
                        reconnect_ok = 0;
                }
                else
                {
                    weechat_printf (NULL,
                                    _("%s%s: server \"%s\" not found"),
                                    weechat_prefix ("error"), IRC_PLUGIN_NAME,
                                    argv[i]);
                    reconnect_ok = 0;
                }
            }
        }
    }
    
    if (nb_reconnect == 0)
        reconnect_ok = irc_command_reconnect_one_server (ptr_server, no_join);
    
    if (!reconnect_ok)
        return WEECHAT_RC_ERROR;
    
    return WEECHAT_RC_OK;
}

/*
 * irc_command_rehash: tell the server to reload its config file
 */

int
irc_command_rehash (void *data, struct t_gui_buffer *buffer, int argc,
                    char **argv, char **argv_eol)
{
    IRC_GET_SERVER(buffer);
    IRC_COMMAND_CHECK_SERVER("rehash", 1);
    
    /* make C compiler happy */
    (void) data;
    (void) argc;
    (void) argv;
    (void) argv_eol;
    
    irc_server_sendf (ptr_server, 0, "REHASH");
    
    return WEECHAT_RC_OK;
}

/*
 * irc_command_restart: tell the server to restart itself
 */

int
irc_command_restart (void *data, struct t_gui_buffer *buffer, int argc,
                     char **argv, char **argv_eol)
{
    IRC_GET_SERVER(buffer);
    IRC_COMMAND_CHECK_SERVER("restart", 1);
    
    /* make C compiler happy */
    (void) data;
    (void) argc;
    (void) argv;
    (void) argv_eol;
    
    irc_server_sendf (ptr_server, 0, "RESTART");
    
    return WEECHAT_RC_OK;
}

/*
 * irc_command_sajoin: forces a user to join channel(s)
 */

int
irc_command_sajoin (void *data, struct t_gui_buffer *buffer, int argc,
                    char **argv, char **argv_eol)
{
    IRC_GET_SERVER_CHANNEL(buffer);
    IRC_COMMAND_CHECK_SERVER("sajoin", 1);
    
    /* make C compiler happy */
    (void) data;
    
    if (argc > 2)
    {
        irc_server_sendf (ptr_server, 0, "SAJOIN %s %s", argv[1], argv_eol[2]);
    }
    else
    {
        IRC_COMMAND_TOO_FEW_ARGUMENTS(ptr_server->buffer, "sajoin");
    }
    
    return WEECHAT_RC_OK;
}

/*
 * irc_command_samode: change mode on channel, without having operator status
 */

int
irc_command_samode (void *data, struct t_gui_buffer *buffer, int argc,
                    char **argv, char **argv_eol)
{
    IRC_GET_SERVER_CHANNEL(buffer);
    IRC_COMMAND_CHECK_SERVER("samode", 1);
    
    /* make C compiler happy */
    (void) data;
    
    if (argc > 2)
    {
        irc_server_sendf (ptr_server, 0, "SAMODE %s %s", argv[1], argv_eol[2]);
    }
    else
    {
        IRC_COMMAND_TOO_FEW_ARGUMENTS(ptr_server->buffer, "samode");
    }
    
    return WEECHAT_RC_OK;
}

/*
 * irc_command_sanick: forces a user to use another nick
 */

int
irc_command_sanick (void *data, struct t_gui_buffer *buffer, int argc,
                    char **argv, char **argv_eol)
{
    IRC_GET_SERVER_CHANNEL(buffer);
    IRC_COMMAND_CHECK_SERVER("sanick", 1);
    
    /* make C compiler happy */
    (void) data;
    
    if (argc > 2)
    {
        irc_server_sendf (ptr_server, 0, "SANICK %s %s", argv[1], argv_eol[2]);
    }
    else
    {
        IRC_COMMAND_TOO_FEW_ARGUMENTS(ptr_server->buffer, "sanick");
    }
    
    return WEECHAT_RC_OK;
}

/*
 * irc_command_sapart: forces a user to leave channel(s)
 */

int
irc_command_sapart (void *data, struct t_gui_buffer *buffer, int argc,
                    char **argv, char **argv_eol)
{
    IRC_GET_SERVER_CHANNEL(buffer);
    IRC_COMMAND_CHECK_SERVER("sapart", 1);
    
    /* make C compiler happy */
    (void) data;
    
    if (argc > 2)
    {
        irc_server_sendf (ptr_server, 0, "SAPART %s %s", argv[1], argv_eol[2]);
    }
    else
    {
        IRC_COMMAND_TOO_FEW_ARGUMENTS(ptr_server->buffer, "sapart");
    }
    
    return WEECHAT_RC_OK;
}

/*
 * irc_command_saquit: forces a user to quit server with a reason
 */

int
irc_command_saquit (void *data, struct t_gui_buffer *buffer, int argc,
                    char **argv, char **argv_eol)
{
    IRC_GET_SERVER_CHANNEL(buffer);
    IRC_COMMAND_CHECK_SERVER("saquit", 1);
    
    /* make C compiler happy */
    (void) data;
    
    if (argc > 2)
    {
        irc_server_sendf (ptr_server, 0, "SAQUIT %s :%s", argv[1], argv_eol[2]);
    }
    else
    {
        IRC_COMMAND_TOO_FEW_ARGUMENTS(ptr_server->buffer, "saquit");
    }
    
    return WEECHAT_RC_OK;
}

/*
 * irc_command_server: manage IRC servers
 */

int
irc_command_server (void *data, struct t_gui_buffer *buffer, int argc,
                    char **argv, char **argv_eol)
{
    int i, detailed_list, one_server_found;
    struct t_irc_server *ptr_server2, *server_found, *new_server;
    char *server_name;
    
    IRC_GET_SERVER_CHANNEL(buffer);
    
    /* make C compiler happy */
    (void) data;
    (void) buffer;
    (void) argv_eol;
    
    if ((argc == 1)
        || (weechat_strcasecmp (argv[1], "list") == 0)
        || (weechat_strcasecmp (argv[1], "listfull") == 0))
    {
        /* list servers */
        server_name = NULL;
        detailed_list = 0;
        for (i = 1; i < argc; i++)
        {
            if (weechat_strcasecmp (argv[i], "list") == 0)
                continue;
            if (weechat_strcasecmp (argv[i], "listfull") == 0)
            {
                detailed_list = 1;
                continue;
            }
            if (!server_name)
                server_name = argv[i];
        }
        if (!server_name)
        {
            if (irc_servers)
            {
                weechat_printf (NULL, "");
                weechat_printf (NULL, _("All servers:"));
                for (ptr_server2 = irc_servers; ptr_server2;
                     ptr_server2 = ptr_server2->next_server)
                {
                    irc_display_server (ptr_server2, detailed_list);
                }
            }
            else
                weechat_printf (NULL, _("No server"));
        }
        else
        {
            one_server_found = 0;
            for (ptr_server2 = irc_servers; ptr_server2;
                 ptr_server2 = ptr_server2->next_server)
            {
                if (weechat_strcasestr (ptr_server2->name, server_name))
                {
                    if (!one_server_found)
                    {
                        weechat_printf (NULL, "");
                        weechat_printf (NULL,
                                        _("Servers with \"%s\":"),
                                        server_name);
                    }
                    one_server_found = 1;
                    irc_display_server (ptr_server2, detailed_list);
                }
            }
            if (!one_server_found)
                weechat_printf (NULL,
                                _("No server found with \"%s\""),
                                server_name);
        }
        
        return WEECHAT_RC_OK;
    }
    
    if (weechat_strcasecmp (argv[1], "add") == 0)
    {
        if (argc < 4)
        {
            IRC_COMMAND_TOO_FEW_ARGUMENTS(NULL, "server add");
        }
        if (irc_server_search (argv[2]))
        {
            weechat_printf (NULL,
                            _("%s%s: server \"%s\" already exists, "
                              "can't create it!"),
                            weechat_prefix ("error"), IRC_PLUGIN_NAME,
                            argv[2]);
            return WEECHAT_RC_OK;
        }
        
        new_server = irc_server_alloc (argv[2]);
        if (!new_server)
        {
            weechat_printf (NULL,
                            _("%s%s: unable to create server"),
                            weechat_prefix ("error"), IRC_PLUGIN_NAME);
            return WEECHAT_RC_OK;
        }
        
        weechat_config_option_set (new_server->options[IRC_SERVER_OPTION_ADDRESSES],
                                   argv[3], 1);
        
        /* parse arguments */
        for (i = 4; i < argc; i++)
        {
            if (argv[i][0] == '-')
            {
                if (weechat_strcasecmp (argv[i], "-auto") == 0)
                {
                    weechat_config_option_set (new_server->options[IRC_SERVER_OPTION_AUTOCONNECT],
                                               "on", 1);
                }
                if (weechat_strcasecmp (argv[i], "-noauto") == 0)
                {
                    weechat_config_option_set (new_server->options[IRC_SERVER_OPTION_AUTOCONNECT],
                                               "off", 1);
                }
                if (weechat_strcasecmp (argv[i], "-ipv6") == 0)
                {
                    weechat_config_option_set (new_server->options[IRC_SERVER_OPTION_IPV6],
                                               "on", 1);
                }
                if (weechat_strcasecmp (argv[i], "-ssl") == 0)
                {
                    weechat_config_option_set (new_server->options[IRC_SERVER_OPTION_SSL],
                                               "on", 1);
                }
            }
        }
        
        weechat_printf (NULL,
                        _("%s: server %s%s%s created"),
                        IRC_PLUGIN_NAME,
                        IRC_COLOR_CHAT_SERVER,
                        new_server->name,
                        IRC_COLOR_CHAT);
        
        if (IRC_SERVER_OPTION_BOOLEAN(new_server, IRC_SERVER_OPTION_AUTOCONNECT))
            irc_server_connect (new_server);
        
        return WEECHAT_RC_OK;
    }
    
    if (weechat_strcasecmp (argv[1], "copy") == 0)
    {
        if (argc < 4)
        {
            IRC_COMMAND_TOO_FEW_ARGUMENTS(NULL, "server copy");
        }
        
        /* look for server by name */
        server_found = irc_server_search (argv[2]);
        if (!server_found)
        {
            weechat_printf (NULL,
                            _("%s%s: server \"%s\" not found for "
                              "\"%s\" command"),
                            weechat_prefix ("error"), IRC_PLUGIN_NAME,
                            argv[2], "server copy");
            return WEECHAT_RC_OK;
        }
        
        /* check if target name already exists */
        if (irc_server_search (argv[3]))
        {
            weechat_printf (NULL,
                            _("%s%s: server \"%s\" already exists for "
                              "\"%s\" command"),
                            weechat_prefix ("error"), IRC_PLUGIN_NAME,
                            argv[3], "server copy");
            return WEECHAT_RC_OK;
        }
        
        /* copy server */
        new_server = irc_server_copy (server_found, argv[3]);
        if (new_server)
        {
            weechat_printf (NULL,
                            _("%s: server %s%s%s has been copied to "
                              "%s%s%s"),
                            IRC_PLUGIN_NAME,
                            IRC_COLOR_CHAT_SERVER,
                            argv[2],
                            IRC_COLOR_CHAT,
                            IRC_COLOR_CHAT_SERVER,
                            argv[3],
                            IRC_COLOR_CHAT);
            return WEECHAT_RC_OK;
        }
        
        return WEECHAT_RC_ERROR;
    }

    if (weechat_strcasecmp (argv[1], "rename") == 0)
    {
        if (argc < 4)
        {
            IRC_COMMAND_TOO_FEW_ARGUMENTS(NULL, "server rename");
        }
        
        /* look for server by name */
        server_found = irc_server_search (argv[2]);
        if (!server_found)
        {
            weechat_printf (NULL,
                            _("%s%s: server \"%s\" not found for "
                              "\"%s\" command"),
                            weechat_prefix ("error"), IRC_PLUGIN_NAME,
                            argv[2], "server rename");
            return WEECHAT_RC_OK;
        }
        
        /* check if target name already exists */
        if (irc_server_search (argv[3]))
        {
            weechat_printf (NULL,
                            _("%s%s: server \"%s\" already exists for "
                              "\"%s\" command"),
                            weechat_prefix ("error"), IRC_PLUGIN_NAME,
                            argv[3], "server rename");
            return WEECHAT_RC_OK;
        }
        
        /* rename server */
        if (irc_server_rename (server_found, argv[3]))
        {
            weechat_printf (NULL,
                            _("%s: server %s%s%s has been renamed to "
                              "%s%s%s"),
                            IRC_PLUGIN_NAME,
                            IRC_COLOR_CHAT_SERVER,
                            argv[2],
                            IRC_COLOR_CHAT,
                            IRC_COLOR_CHAT_SERVER,
                            argv[3],
                            IRC_COLOR_CHAT);
            return WEECHAT_RC_OK;
        }
        
        return WEECHAT_RC_ERROR;
    }
    
    if (weechat_strcasecmp (argv[1], "keep") == 0)
    {
        if (argc < 3)
        {
            IRC_COMMAND_TOO_FEW_ARGUMENTS(NULL, "server keep");
        }
        
        /* look for server by name */
        server_found = irc_server_search (argv[2]);
        if (!server_found)
        {
            weechat_printf (NULL,
                            _("%s%s: server \"%s\" not found for "
                              "\"%s\" command"),
                            weechat_prefix ("error"), IRC_PLUGIN_NAME,
                            argv[2], "server keep");
            return WEECHAT_RC_OK;
        }
        
        /* check that is it temporary server */
        if (!server_found->temp_server)
        {
            weechat_printf (NULL,
                            _("%s%s: server \"%s\" is not a temporary server"),
                            weechat_prefix ("error"), IRC_PLUGIN_NAME,
                            argv[2], "server keep");
            return WEECHAT_RC_OK;
        }
        
        /* remove temporary flag on server */
        server_found->temp_server = 0;
        
        weechat_printf (NULL,
                        _("%s: server %s%s%s is not temporary any more"),
                        IRC_PLUGIN_NAME,
                        IRC_COLOR_CHAT_SERVER,
                        argv[2],
                        IRC_COLOR_CHAT);
        
        return WEECHAT_RC_OK;
    }
    
    if (weechat_strcasecmp (argv[1], "del") == 0)
    {
        if (argc < 3)
        {
            IRC_COMMAND_TOO_FEW_ARGUMENTS(NULL, "server del");
        }
        
        /* look for server by name */
        server_found = irc_server_search (argv[2]);
        if (!server_found)
        {
            weechat_printf (NULL,
                            _("%s%s: server \"%s\" not found for "
                              "\"%s\" command"),
                            weechat_prefix ("error"), IRC_PLUGIN_NAME,
                            argv[2], "server del");
            return WEECHAT_RC_OK;
        }
        if (server_found->is_connected)
        {
            weechat_printf (NULL,
                            _("%s%s: you can not delete server \"%s\" "
                              "because you are connected to. "
                              "Try \"/disconnect %s\" before."),
                            weechat_prefix ("error"), IRC_PLUGIN_NAME,
                            argv[2], argv[2]);
            return WEECHAT_RC_OK;
        }
        
        server_name = strdup (server_found->name);
        irc_server_free (server_found);
        weechat_printf (NULL,
                        _("%s: Server %s%s%s has been deleted"),
                        IRC_PLUGIN_NAME,
                        IRC_COLOR_CHAT_SERVER,
                        (server_name) ? server_name : "???",
                        IRC_COLOR_CHAT);
        if (server_name)
            free (server_name);
        
        return WEECHAT_RC_OK;
    }
    
    if (weechat_strcasecmp (argv[1], "deloutq") == 0)
    {
        for (ptr_server2 = irc_servers; ptr_server2;
             ptr_server2 = ptr_server2->next_server)
        {
            irc_server_outqueue_free_all (ptr_server2);
        }
        weechat_printf (NULL,
                        _("%s: messages outqueue DELETED for all "
                          "servers. Some messages from you or "
                          "WeeChat may have been lost!"),
                        IRC_PLUGIN_NAME);
        return WEECHAT_RC_OK;
    }
    
    if (weechat_strcasecmp (argv[1], "raw") == 0)
    {
        irc_raw_open (1);
        return WEECHAT_RC_OK;
    }
    
    if (weechat_strcasecmp (argv[1], "jump") == 0)
    {
        if (ptr_server && ptr_server->buffer)
            weechat_buffer_set (ptr_server->buffer, "display", "1");
        return WEECHAT_RC_OK;
    }
    
    weechat_printf (NULL,
                    _("%s%s: unknown option for \"%s\" command"),
                    weechat_prefix ("error"), IRC_PLUGIN_NAME, "server");
    
    return WEECHAT_RC_OK;
}

/*
 * irc_command_service: register a new service
 */

int
irc_command_service (void *data, struct t_gui_buffer *buffer, int argc,
                     char **argv, char **argv_eol)
{
    IRC_GET_SERVER(buffer);
    IRC_COMMAND_CHECK_SERVER("service", 1);
    
    /* make C compiler happy */
    (void) data;
    (void) argv;

    if (argc > 1)
        irc_server_sendf (ptr_server, 0, "SERVICE %s", argv_eol[1]);
    else
    {
        IRC_COMMAND_TOO_FEW_ARGUMENTS(ptr_server->buffer, "service");
    }
    
    return WEECHAT_RC_OK;
}

/*
 * irc_command_servlist: list services currently connected to the network
 */

int
irc_command_servlist (void *data, struct t_gui_buffer *buffer, int argc,
                      char **argv, char **argv_eol)
{
    IRC_GET_SERVER(buffer);
    IRC_COMMAND_CHECK_SERVER("servlist", 1);
    
    /* make C compiler happy */
    (void) data;
    (void) argv;
    
    if (argc > 1)
        irc_server_sendf (ptr_server, 0, "SERVLIST %s", argv_eol[1]);
    else
        irc_server_sendf (ptr_server, 0, "SERVLIST");
    
    return WEECHAT_RC_OK;
}

/*
 * irc_command_squery: deliver a message to a service
 */

int
irc_command_squery (void *data, struct t_gui_buffer *buffer, int argc,
                    char **argv, char **argv_eol)
{
    IRC_GET_SERVER(buffer);
    IRC_COMMAND_CHECK_SERVER("squery", 1);
    
    /* make C compiler happy */
    (void) data;
    
    if (argc > 1)
    {
        if (argc > 2)
            irc_server_sendf (ptr_server, 1, "SQUERY %s :%s",
                              argv[1], argv_eol[2]);
        else
            irc_server_sendf (ptr_server, 1, "SQUERY %s", argv_eol[1]);
    }
    else
    {
        IRC_COMMAND_TOO_FEW_ARGUMENTS(ptr_server->buffer, "squery");
    }
    
    return WEECHAT_RC_OK;
}

/*
 * irc_command_squit: disconnect server links
 */

int
irc_command_squit (void *data, struct t_gui_buffer *buffer, int argc,
                   char **argv, char **argv_eol)
{
    IRC_GET_SERVER(buffer);
    IRC_COMMAND_CHECK_SERVER("squit", 1);
    
    /* make C compiler happy */
    (void) data;
    (void) argv;
    
    if (argc > 1)
        irc_server_sendf (ptr_server, 0, "SQUIT %s", argv_eol[1]);
    else
    {
        IRC_COMMAND_TOO_FEW_ARGUMENTS(ptr_server->buffer, "squit");
    }
    
    return WEECHAT_RC_OK;
}

/*
 * irc_command_stats: query statistics about server
 */

int
irc_command_stats (void *data, struct t_gui_buffer *buffer, int argc,
                   char **argv, char **argv_eol)
{
    IRC_GET_SERVER(buffer);
    IRC_COMMAND_CHECK_SERVER("stats", 1);
    
    /* make C compiler happy */
    (void) data;
    (void) argv;
    
    if (argc > 1)
        irc_server_sendf (ptr_server, 0, "STATS %s", argv_eol[1]);
    else
        irc_server_sendf (ptr_server, 0, "STATS");
    
    return WEECHAT_RC_OK;
}

/*
 * irc_command_summon: give users who are on a host running an IRC server
 *                      a message asking them to please join IRC
 */

int
irc_command_summon (void *data, struct t_gui_buffer *buffer, int argc,
                    char **argv, char **argv_eol)
{
    IRC_GET_SERVER(buffer);
    IRC_COMMAND_CHECK_SERVER("summon", 1);
    
    /* make C compiler happy */
    (void) data;
    (void) argv;
    
    if (argc > 1)
        irc_server_sendf (ptr_server, 0, "SUMMON %s", argv_eol[1]);
    else
    {
        IRC_COMMAND_TOO_FEW_ARGUMENTS(ptr_server->buffer, "summon");
    }
    
    return WEECHAT_RC_OK;
}

/*
 * irc_command_time: query local time from server
 */

int
irc_command_time (void *data, struct t_gui_buffer *buffer, int argc,
                  char **argv, char **argv_eol)
{
    IRC_GET_SERVER(buffer);
    IRC_COMMAND_CHECK_SERVER("time", 1);
    
    /* make C compiler happy */
    (void) data;
    (void) argv;
    
    if (argc > 1)
        irc_server_sendf (ptr_server, 0, "TIME %s", argv_eol[1]);
    else
        irc_server_sendf (ptr_server, 0, "TIME");
    
    return WEECHAT_RC_OK;
}

/*
 * irc_command_topic: get/set topic for a channel
 */

int
irc_command_topic (void *data, struct t_gui_buffer *buffer, int argc,
                   char **argv, char **argv_eol)
{
    char *channel_name, *new_topic, *new_topic_color;
    
    IRC_GET_SERVER_CHANNEL(buffer);
    IRC_COMMAND_CHECK_SERVER("topic", 1);
    
    /* make C compiler happy */
    (void) data;
    (void) argv;
    
    channel_name = NULL;
    new_topic = NULL;
    
    if (argc > 1)
    {
        if (irc_channel_is_channel (argv[1]))
        {
            channel_name = argv[1];
            new_topic = argv_eol[2];
        }
        else
            new_topic = argv_eol[1];
    }
    
    /* look for current channel if not specified */
    if (!channel_name)
    {
        if (ptr_channel && (ptr_channel->type == IRC_CHANNEL_TYPE_CHANNEL))
            channel_name = ptr_channel->name;
        else
        {
            weechat_printf (ptr_server->buffer,
                            _("%s%s: \"%s\" command can only be "
                              "executed in a channel buffer"),
                            weechat_prefix ("error"), IRC_PLUGIN_NAME,
                            "topic");
            return WEECHAT_RC_OK;
        }
    }
    
    if (new_topic)
    {
        if (weechat_strcasecmp (new_topic, "-delete") == 0)
            irc_server_sendf (ptr_server, 1, "TOPIC %s :",
                              channel_name);
        else
        {
            new_topic_color = irc_color_encode (new_topic,
                                                weechat_config_boolean (irc_config_network_colors_send));
            irc_server_sendf (ptr_server, 1, "TOPIC %s :%s",
                              channel_name,
                              (new_topic_color) ? new_topic_color : new_topic);
            if (new_topic_color)
                free (new_topic_color);
        }
    }
    else
        irc_server_sendf (ptr_server, 1, "TOPIC %s", channel_name);
    
    return WEECHAT_RC_OK;
}

/*
 * irc_command_trace: find the route to specific server
 */

int
irc_command_trace (void *data, struct t_gui_buffer *buffer, int argc,
                   char **argv, char **argv_eol)
{
    IRC_GET_SERVER(buffer);
    IRC_COMMAND_CHECK_SERVER("trace", 1);
    
    /* make C compiler happy */
    (void) data;
    (void) argv;
    
    if (argc > 1)
        irc_server_sendf (ptr_server, 0, "TRACE %s", argv_eol[1]);
    else
        irc_server_sendf (ptr_server, 0, "TRACE");
    
    return WEECHAT_RC_OK;
}

/*
 * irc_command_unban: unbans nicks or hosts
 */

int
irc_command_unban (void *data, struct t_gui_buffer *buffer, int argc,
                   char **argv, char **argv_eol)
{
    char *pos_channel;
    int pos_args;
    
    IRC_GET_SERVER_CHANNEL(buffer);
    IRC_COMMAND_CHECK_SERVER("unban", 1);
    
    /* make C compiler happy */
    (void) data;
    (void) argv_eol;
    
    if (argc > 1)
    {
        if (irc_channel_is_channel (argv[1]))
        {
            pos_channel = argv[1];
            pos_args = 2;
        }
        else
        {
            pos_channel = NULL;
            pos_args = 1;
        }
        
        /* channel not given, use default buffer */
        if (!pos_channel)
        {
            if (ptr_channel && (ptr_channel->type == IRC_CHANNEL_TYPE_CHANNEL))
                pos_channel = ptr_channel->name;
            else
            {
                weechat_printf (ptr_server->buffer,
                                _("%s%s: \"%s\" command can only be "
                                  "executed in a channel buffer"),
                                weechat_prefix ("error"), IRC_PLUGIN_NAME,
                                "unban");
                return WEECHAT_RC_OK;
            }
        }
        
        /* loop on users */
        while (argv[pos_args])
        {
            irc_server_sendf (ptr_server, 0, "MODE %s -b %s",
                              pos_channel, argv[pos_args]);
            pos_args++;
        }
    }
    else
    {
        weechat_printf (ptr_server->buffer,
                        _("%s%s: wrong argument count for \"%s\" command"),
                        weechat_prefix ("error"), IRC_PLUGIN_NAME, "unban");
        return WEECHAT_RC_OK;
    }
    
    return WEECHAT_RC_OK;
}

/*
 * irc_command_userhost: return a list of information about nicknames
 */

int
irc_command_userhost (void *data, struct t_gui_buffer *buffer, int argc,
                      char **argv, char **argv_eol)
{
    IRC_GET_SERVER(buffer);
    IRC_COMMAND_CHECK_SERVER("userhost", 1);
    
    /* make C compiler happy */
    (void) data;
    (void) argv;
    
    if (argc > 1)
        irc_server_sendf (ptr_server, 0, "USERHOST %s", argv_eol[1]);
    else
    {
        IRC_COMMAND_TOO_FEW_ARGUMENTS(ptr_server->buffer, "userhost");
    }
    
    return WEECHAT_RC_OK;
}

/*
 * irc_command_users: list of users logged into the server
 */

int
irc_command_users (void *data, struct t_gui_buffer *buffer, int argc,
                   char **argv, char **argv_eol)
{
    IRC_GET_SERVER(buffer);
    IRC_COMMAND_CHECK_SERVER("users", 1);
    
    /* make C compiler happy */
    (void) data;
    (void) argv;
    
    if (argc > 1)
        irc_server_sendf (ptr_server, 0, "USERS %s", argv_eol[1]);
    else
        irc_server_sendf (ptr_server, 0, "USERS");
    
    return WEECHAT_RC_OK;
}

/*
 * irc_command_version: gives the version info of nick or server (current or specified)
 */

int
irc_command_version (void *data, struct t_gui_buffer *buffer, int argc,
                     char **argv, char **argv_eol)
{
    IRC_GET_SERVER_CHANNEL(buffer);
    IRC_COMMAND_CHECK_SERVER("version", 1);
    
    /* make C compiler happy */
    (void) data;
    (void) argv_eol;
    
    if (argc > 1)
    {
        if (ptr_channel && (ptr_channel->type == IRC_CHANNEL_TYPE_CHANNEL)
            && irc_nick_search (ptr_channel, argv[1]))
            irc_server_sendf (ptr_server, 1, "PRIVMSG %s :\01VERSION\01",
                              argv[1]);
        else
            irc_server_sendf (ptr_server, 1, "VERSION %s",
                              argv[1]);
    }
    else
        irc_server_sendf (ptr_server, 1, "VERSION");
    
    return WEECHAT_RC_OK;
}

/*
 * irc_command_voice: give voice to nickname(s)
 */

int
irc_command_voice (void *data, struct t_gui_buffer *buffer, int argc,
                   char **argv, char **argv_eol)
{
    IRC_GET_SERVER_CHANNEL(buffer);
    IRC_COMMAND_CHECK_SERVER("voice", 1);
    
    /* make C compiler happy */
    (void) data;
    (void) argv_eol;
    
    if (ptr_channel && (ptr_channel->type == IRC_CHANNEL_TYPE_CHANNEL))
    {
        if (argc < 2)
            irc_server_sendf (ptr_server, 0, "MODE %s +v %s",
                              ptr_channel->name,
                              ptr_server->nick);
        else
            irc_command_mode_nicks (ptr_server, ptr_channel->name,
                                    "+", "v", argc, argv);
    }
    else
    {
        weechat_printf (ptr_server->buffer,
                        _("%s%s: \"%s\" command can only be "
                          "executed in a channel buffer"),
                        weechat_prefix ("error"), IRC_PLUGIN_NAME, "voice");
        return WEECHAT_RC_OK;
    }
    
    return WEECHAT_RC_OK;
}

/*
 * irc_command_wallops: send a message to all currently connected users who
 *                       have set the 'w' user mode for themselves
 */

int
irc_command_wallops (void *data, struct t_gui_buffer *buffer, int argc,
                     char **argv, char **argv_eol)
{
    IRC_GET_SERVER(buffer);
    IRC_COMMAND_CHECK_SERVER("wallops", 1);
    
    /* make C compiler happy */
    (void) data;
    (void) argv;
    
    if (argc > 1)
        irc_server_sendf (ptr_server, 1, "WALLOPS :%s", argv_eol[1]);
    else
    {
        IRC_COMMAND_TOO_FEW_ARGUMENTS(ptr_server->buffer, "wallops");
    }
    
    return WEECHAT_RC_OK;
}

/*
 * irc_command_who: generate a query which returns a list of information
 */

int
irc_command_who (void *data, struct t_gui_buffer *buffer, int argc,
                 char **argv, char **argv_eol)
{
    IRC_GET_SERVER(buffer);
    IRC_COMMAND_CHECK_SERVER("who", 1);
    
    /* make C compiler happy */
    (void) data;
    (void) argv;
    
    if (argc > 1)
        irc_server_sendf (ptr_server, 0, "WHO %s", argv_eol[1]);
    else
        irc_server_sendf (ptr_server, 0, "WHO");
    
    return WEECHAT_RC_OK;
}

/*
 * irc_command_whois: query information about user(s)
 */

int
irc_command_whois (void *data, struct t_gui_buffer *buffer, int argc,
                   char **argv, char **argv_eol)
{
    IRC_GET_SERVER_CHANNEL(buffer);
    IRC_COMMAND_CHECK_SERVER("whois", 1);
    
    /* make C compiler happy */
    (void) data;
    (void) argv;
    
    if (argc > 1)
        irc_server_sendf (ptr_server, 0, "WHOIS %s", argv_eol[1]);
    else
    {
        if (ptr_channel
            && (ptr_channel->type == IRC_CHANNEL_TYPE_PRIVATE))
        {
            irc_server_sendf (ptr_server, 0, "WHOIS %s", ptr_channel->name);
        }
        else
        {
            IRC_COMMAND_TOO_FEW_ARGUMENTS(ptr_server->buffer, "whois");
        }
    }
    
    return WEECHAT_RC_OK;
}

/*
 * irc_command_whowas: ask for information about a nickname which no longer exists
 */

int
irc_command_whowas (void *data, struct t_gui_buffer *buffer, int argc,
                    char **argv, char **argv_eol)
{
    IRC_GET_SERVER(buffer);
    IRC_COMMAND_CHECK_SERVER("whowas", 1);
    
    /* make C compiler happy */
    (void) data;
    (void) argv;
    
    if (argc > 1)
        irc_server_sendf (ptr_server, 0, "WHOWAS %s", argv_eol[1]);
    else
    {
        IRC_COMMAND_TOO_FEW_ARGUMENTS(ptr_server->buffer, "whowas");
    }
    
    return WEECHAT_RC_OK;
}

/*
 * irc_command_init: init IRC commands (create hooks)
 */

void
irc_command_init ()
{
    weechat_hook_command ("admin",
                          N_("find information about the administrator of the "
                             "server"),
                          N_("[target]"),
                          N_("target: server"),
                          NULL, &irc_command_admin, NULL);
    weechat_hook_command ("ame",
                          N_("send a CTCP action to all channels of all "
                             "connected servers"),
                          N_("message"),
                          N_("message: message to send"),
                          NULL, &irc_command_ame, NULL);
    weechat_hook_command ("amsg",
                          N_("send message to all channels of all connected "
                             "servers"),
                          N_("text"),
                          N_("text: text to send"),
                          NULL, &irc_command_amsg, NULL);
    weechat_hook_command ("away",
                          N_("toggle away status"),
                          N_("[-all] [message]"),
                          N_("   -all: toggle away status on all connected "
                             "servers\n"
                             "message: message for away (if no message is "
                             "given, away status is removed)"),
                          "-all", &irc_command_away, NULL);
    weechat_hook_command ("ban",
                          N_("ban nicks or hosts"),
                          N_("[channel] [nickname [nickname ...]]"),
                          N_(" channel: channel for ban\n"
                             "nickname: user or host to ban"),
                          "%(irc_channel_nicks_hosts)", &irc_command_ban, NULL);
    weechat_hook_command ("connect",
                          N_("connect to IRC server(s)"),
                          N_("[-all [-nojoin] | servername [servername ...] "
                             "[-nojoin] | hostname [-port port] [-ipv6] "
                             "[-ssl]]"),
                          N_("      -all: connect to all servers\n"
                             "servername: internal server name to connect\n"
                             "   -nojoin: do not join any channel (even if "
                             "autojoin is enabled on server)\n"
                             "  hostname: hostname to connect\n"
                             "      port: port for server (integer, default "
                             "is 6667)\n"
                             "      ipv6: use IPv6 protocol\n"
                             "       ssl: use SSL protocol"),
                          "-all -nojoin"
                          " || %(irc_servers)|%*",
                          &irc_command_connect, NULL);
    weechat_hook_command ("ctcp",
                          N_("send a CTCP message (Client-To-Client Protocol)"),
                          N_("receiver type [arguments]"),
                          N_(" receiver: nick or channel to send CTCP to\n"
                             "     type: CTCP type (examples: \"version\", "
                             "\"ping\", ..)\n"
                             "arguments: arguments for CTCP"),
                          "%(irc_channel)|%(nicks) "
                          "action|clientinfo|finger|ping|source|time|userinfo|"
                          "version",
                          &irc_command_ctcp, NULL);
    weechat_hook_command ("cycle",
                          N_("leave and rejoin a channel"),
                          N_("[channel[,channel]] [part_message]"),
                          N_("     channel: channel name for cycle\n"
                             "part_message: part message (displayed to other "
                             "users)"),
                          "%(irc_msg_part)", &irc_command_cycle, NULL);
    weechat_hook_command ("dcc",
                          N_("start DCC (file or chat)"),
                          N_("action [nickname [file]]"),
                          N_("  action: 'send' (file) or 'chat'\n"
                             "nickname: nickname to send file or chat\n"
                             "    file: filename (on local host)"),
                          "chat %(nicks)"
                          " || send %(nicks) %(filename)",
                          &irc_command_dcc, NULL);
    weechat_hook_command ("dehalfop",
                          N_("remove half channel operator status from "
                             "nickname(s)"),
                          N_("[nickname [nickname]]"),
                          "",
                          "%(nicks)", &irc_command_dehalfop, NULL);
    weechat_hook_command ("deop",
                          N_("remove channel operator status from "
                             "nickname(s)"),
                          N_("[nickname [nickname]]"),
                          "",
                          "%(nicks)|%*", &irc_command_deop, NULL);
    weechat_hook_command ("devoice",
                          N_("remove voice from nickname(s)"),
                          N_("[nickname [nickname]]"),
                          "",
                          "%(nicks)|%*", &irc_command_devoice, NULL);
    weechat_hook_command ("die",
                          N_("shutdown the server"),
                          "",
                          "",
                          NULL, &irc_command_die, NULL);
    weechat_hook_command ("disconnect",
                          N_("disconnect from IRC server(s)"),
                          N_("[-all | servername [servername ...]]"),
                          N_("      -all: disconnect from all servers\n"
                             "servername: server name to disconnect"),
                          "-all"
                          " || %(irc_servers)|%*",
                          &irc_command_disconnect, NULL);
    weechat_hook_command ("halfop",
                          N_("give half channel operator status to "
                             "nickname(s)"),
                          N_("[nickname [nickname]]"),
                          "",
                          "%(nicks)", &irc_command_halfop, NULL);
    weechat_hook_command ("ignore",
                          N_("ignore nicks/hosts from servers or channels"),
                          N_("[list] | [add [re:]nick/host [server [channel]]] | "
                             "[del number|-all]"),
                          N_("     list: list all ignores\n"
                             "      add: add a ignore\n"
                             "      del: del a ignore\n"
                             "   number: number of ignore to delete (look at "
                             "list to find it)\n"
                             "     -all: delete all ignores\n"
                             "nick/host: nick or host to ignore: syntax is "
                             "\"re:regex\" or \"mask\" (a mask is a string with "
                             "some \"*\" to replace one or more chars)\n"
                             "   server: internal server name where ignore "
                             "is working\n"
                             "  channel: channel name where ignore is "
                             "working\n\n"
                             "Examples:\n"
                             "  ignore nick \"toto\" everywhere:\n"
                             "    /ignore add toto\n"
                             "  ignore host \"toto@domain.com\" on freenode server:\n"
                             "    /ignore add toto@domain.com freenode\n"
                             "  ignore host \"toto*@*.domain.com\" on freenode/#weechat:\n"
                             "    /ignore add toto*@*.domain.com freenode #weechat"),
                          "list"
                          " || add %(irc_channel_nicks_hosts) %(irc_servers) %(irc_channels) %-"
                          " || del -all|%(irc_ignores_numbers) %-",
                          &irc_command_ignore, NULL);
    weechat_hook_command ("info",
                          N_("get information describing the server"),
                          N_("[target]"),
                          N_("target: server name"),
                          NULL, &irc_command_info, NULL);
    weechat_hook_command ("invite",
                          N_("invite a nick on a channel"),
                          N_("nickname channel"),
                          N_("nickname: nick to invite\n"
                             " channel: channel to invite"),
                          "%(nicks) %(irc_server_channels)", &irc_command_invite, NULL);
    weechat_hook_command ("ison",
                          N_("check if a nickname is currently on IRC"),
                          N_("nickname [nickname ...]"),
                          N_("nickname: nickname"),
                          "%(nicks)|%*", &irc_command_ison, NULL);
    weechat_hook_command ("join",
                          N_("join a channel"),
                          N_("[channel[,channel] [key[,key]]]"),
                          N_("channel: channel name to join\n"
                             "    key: key to join the channel"),
                          "%(irc_channels)", &irc_command_join, NULL);
    weechat_hook_command ("kick",
                          N_("forcibly remove a user from a channel"),
                          N_("[channel] nickname [comment]"),
                          N_(" channel: channel where user is\n"
                             "nickname: nickname to kick\n"
                             " comment: comment for kick"),
                          "%(nicks) %-", &irc_command_kick, NULL);
    weechat_hook_command ("kickban",
                          N_("kicks and bans a nick from a channel"),
                          N_("[channel] nickname [comment]"),
                          N_(" channel: channel where user is\n"
                             "nickname: nickname to kick and ban\n"
                             " comment: comment for kick"),
                          "%(irc_channel_nicks_hosts) %-", &irc_command_kickban, NULL);
    weechat_hook_command ("kill",
                          N_("close client-server connection"),
                          N_("nickname comment"),
                          N_("nickname: nickname\n"
                             " comment: comment for kill"),
                          "%(nicks) %-", &irc_command_kill, NULL);
    weechat_hook_command ("links",
                          N_("list all servernames which are known by the "
                             "server answering the query"),
                          N_("[[server] server_mask]"),
                          N_("     server: this server should answer the "
                             "query\n"
                             "server_mask: list of servers must match this "
                             "mask"),
                          NULL, &irc_command_links, NULL);
    weechat_hook_command ("list",
                          N_("list channels and their topic"),
                          N_("[channel[,channel] [server]]"),
                          N_("channel: channel to list (a regexp is allowed)\n"
                             "server: server name"),
                          NULL, &irc_command_list, NULL);
    weechat_hook_command ("lusers",
                          N_("get statistics about the size of the IRC "
                             "network"),
                          N_("[mask [target]]"),
                          N_("  mask: servers matching the mask only\n"
                             "target: server for forwarding request"),
                          NULL, &irc_command_lusers, NULL);
    weechat_hook_command ("me",
                          N_("send a CTCP action to the current channel"),
                          N_("message"),
                          N_("message: message to send"),
                          NULL, &irc_command_me, NULL);
    weechat_hook_command ("mode",
                          N_("change channel or user mode"),
                          N_("{ [channel] {[+|-]|o|p|s|i|t|n|b|v} [limit] "
                             "[user] [ban mask] } | { nickname "
                             "{[+|-]|i|w|s|o} }"),
                          N_("channel modes:\n"
                             "  channel: channel name to modify (default is "
                             "current one)\n"
                             "  o: give/take channel operator privileges\n"
                             "  p: private channel flag\n"
                             "  s: secret channel flag\n"
                             "  i: invite-only channel flag\n"
                             "  t: topic settable by channel operator only "
                             "flag\n"
                             "  n: no messages to channel from clients on the "
                             "outside\n"
                             "  m: moderated channel\n"
                             "  l: set the user limit to channel\n"
                             "  b: set a ban mask to keep users out\n"
                             "  e: set exception mask\n"
                             "  v: give/take the ability to speak on a "
                             "moderated channel\n"
                             "  k: set a channel key (password)\n"
                             "user modes:\n"
                             "  nickname: nickname to modify\n"
                             "  i: mark a user as invisible\n"
                             "  s: mark a user for receive server notices\n"
                             "  w: user receives wallops\n"
                             "  o: operator flag"),
                          "%(irc_channel)|%(irc_server_nick)", &irc_command_mode, NULL);
    weechat_hook_command ("motd",
                          N_("get the \"Message Of The Day\""),
                          N_("[target]"),
                          N_("target: server name"),
                          NULL, &irc_command_motd, NULL);
    weechat_hook_command ("msg",
                          N_("send message to a nick or channel"),
                          N_("[-server server] receiver[,receiver] text"),
                          N_("  server: send to this server (internal name)\n"
                             "receiver: nick or channel (may be mask, '*' = "
                             "current channel)\n"
                             "    text: text to send"),
                          "-server %(irc_servers)"
                          " || %(nicks) %-",
                          &irc_command_msg, NULL);
    weechat_hook_command ("names",
                          N_("list nicknames on channels"),
                          N_("[channel[,channel]]"),
                          N_("channel: channel name"),
                          "%(irc_channels)", &irc_command_names, NULL);
    weechat_hook_command ("nick",
                          N_("change current nickname"),
                          N_("[-all] nickname"),
                          N_("    -all: set new nickname for all connected "
                             "servers\n"
                             "nickname: new nickname"),
                          "-all %(irc_server_nick)"
                          " || %(irc_server_nick)",
                          &irc_command_nick, NULL);
    weechat_hook_command ("notice",
                          N_("send notice message to user"),
                          N_("[-server server] nickname text"),
                          N_("  server: send to this server (internal name)\n"
                             "nickname: user to send notice to\n"
                             "    text: text to send"),
                          "%(nicks) %-", &irc_command_notice, NULL);
    weechat_hook_command ("op",
                          N_("give channel operator status to nickname(s)"),
                          N_("nickname [nickname]"),
                          "",
                          "%(nicks)|%*", &irc_command_op, NULL);
    weechat_hook_command ("oper",
                          N_("get operator privileges"),
                          N_("user password"),
                          N_("user/password: used to get privileges on "
                             "current IRC server"),
                          NULL, &irc_command_oper, NULL);
    weechat_hook_command ("part",
                          N_("leave a channel"),
                          N_("[channel[,channel]] [part_message]"),
                          N_("     channel: channel name to leave\n"
                             "part_message: part message (displayed to other "
                             "users)"),
                          "%(irc_msg_part)", &irc_command_part, NULL);
    weechat_hook_command ("ping",
                          N_("ping server"),
                          N_("server1 [server2]"),
                          N_("server1: server to ping\n"
                             "server2: forward ping to this server"),
                          NULL, &irc_command_ping, NULL);
    weechat_hook_command ("pong",
                          N_("answer to a ping message"),
                          N_("daemon [daemon2]"),
                          N_(" daemon: daemon who has responded to Ping "
                             "message\n"
                             "daemon2: forward message to this daemon"),
                          NULL, &irc_command_pong, NULL);
    weechat_hook_command ("query",
                          N_("send a private message to a nick"),
                          N_("[-server server] nickname [text]"),
                          N_("  server: send to this server (internal name)\n"
                             "nickname: nickname for private conversation\n"
                             "    text: text to send"),
                          "%(nicks) %-", &irc_command_query, NULL);
    weechat_hook_command ("quote",
                          N_("send raw data to server without parsing"),
                          N_("[-server server] data"),
                          N_("server: send to this server (internal name)\n"
                             "data: raw data to send"),
                          NULL, &irc_command_quote, NULL);
    weechat_hook_command ("reconnect",
                          N_("reconnect to server(s)"),
                          N_("[-all [-nojoin] | servername [servername ...] "
                             "[-nojoin]]"),
                          N_("      -all: reconnect to all servers\n"
                             "servername: server name to reconnect\n"
                             "   -nojoin: do not join any channel (even if "
                             "autojoin is enabled on server)"),
                          "-all -nojoin"
                          " || %(irc_servers)|-nojoin|%*",
                          &irc_command_reconnect, NULL);
    weechat_hook_command ("rehash",
                          N_("tell the server to reload its config file"),
                          "",
                          "",
                          NULL, &irc_command_rehash, NULL);
    weechat_hook_command ("restart",
                          N_("tell the server to restart itself"),
                          "",
                          "",
                          NULL, &irc_command_restart, NULL);
    weechat_hook_command ("sajoin",
                          N_("forces a user to join channel(s)"),
                          N_("nickname channel[,channel]"),
                          N_("nickname: nickname\n"
                             " channel: channel name"),
                          "%(nicks) %(irc_server_channels)", &irc_command_sajoin, NULL);
    weechat_hook_command ("samode",
                          N_("change mode on channel, without having operator "
                             "status"),
                          /* TRANSLATORS: "channel" and "mode" are arguments
                             for command, translate them separately */
                          N_("channel mode"),
                          N_("channel: channel name\n"
                             "   mode: mode for channel"),
                          "%(irc_server_channels)", &irc_command_samode, NULL);
    weechat_hook_command ("sanick",
                          N_("forces a user to use another nick"),
                          N_("nickname new_nickname"),
                          N_("    nickname: nickname\n"
                             "new_nickname: new nickname"),
                          "%(nicks) %(nicks)", &irc_command_sanick, NULL);
    weechat_hook_command ("sapart",
                          N_("forces a user to leave channel(s)"),
                          N_("nickname channel[,channel]"),
                          N_("nickname: nickname\n"
                             " channel: channel name"),
                          "%(nicks) %(irc_server_channels)", &irc_command_sapart, NULL);
    weechat_hook_command ("saquit",
                          N_("forces a user to quit server with a reason"),
                          N_("nickname reason"),
                          N_("nickname: nickname\n"
                             "  reason: reason"),
                          "%(nicks)", &irc_command_saquit, NULL);
    weechat_hook_command ("service",
                          N_("register a new service"),
                          N_("nickname reserved distribution type reserved "
                             "info"),
                          N_("distribution: visibility of service\n"
                             "        type: reserved for future usage"),
                          NULL, &irc_command_service, NULL);
    weechat_hook_command ("server",
                          N_("list, add or remove IRC servers"),
                          N_("[list [servername]] | [listfull [servername]] | "
                             "[add servername hostname[/port] "
                             "[-auto | -noauto] [-ipv6] [-ssl]] | "
                             "[copy servername newservername] | "
                             "[rename servername newservername] | "
                             "[keep servername] | [del servername] | "
                             "[deloutq] | [jump] | [raw]"),
                          N_("      list: list servers (no parameter implies "
                             "this list)\n"
                             "  listfull: list servers with detailed info for "
                             "each server\n"
                             "       add: create a new server\n"
                             "servername: server name, for internal and "
                             "display use\n"
                             "  hostname: name or IP address of server, with "
                             "optional port (default: 6667)\n"
                             "      auto: automatically connect to server "
                             "when WeeChat starts\n"
                             "    noauto: do not connect to server when "
                             "WeeChat starts (default)\n"
                             "      ipv6: use IPv6 protocol\n"
                             "       ssl: use SSL protocol\n"
                             "      copy: duplicate a server\n"
                             "    rename: rename a server\n"
                             "      keep: keep server in config file (for "
                             "temporary servers only)\n"
                             "       del: delete a server\n"
                             "   deloutq: delete messages out queue for all "
                             "servers (all messages WeeChat is currently "
                             "sending)\n"
                             "      jump: jump to server buffer\n"
                             "       raw: open buffer with raw IRC data\n\n"
                             "Examples:\n"
                             "  /server listfull\n"
                             "  /server add oftc irc.oftc.net/6697 -ssl\n"
                             "  /server add oftc6 irc6.oftc.net/6697 -ipv6 -ssl\n"
                             "  /server add freenode2 chat.eu.freenode.net/6667,"
                             "chat.us.freenode.net/6667\n"
                             "  /server copy oftc oftcbis\n"
                             "  /server rename oftc newoftc\n"
                             "  /server del freenode\n"
                             "  /server deloutq"),
                          "add %(irc_servers)"
                          " || copy %(irc_servers) %(irc_servers)"
                          " || rename %(irc_servers) %(irc_servers)"
                          " || keep %(irc_servers)"
                          " || del %(irc_servers)"
                          " || deloutq"
                          " || jump"
                          " || list %(irc_servers)"
                          " || listfull %(irc_servers)"
                          " || raw",
                          &irc_command_server, NULL);
    weechat_hook_command ("servlist",
                          N_("list services currently connected to the "
                             "network"),
                          N_("[mask [type]]"),
                          N_("mask: list only services matching this mask\n"
                             "type: list only services of this type"),
                          NULL, &irc_command_servlist, NULL);
    weechat_hook_command ("squery",
                          N_("deliver a message to a service"),
                          N_("service text"),
                          N_("service: name of service\n"
                             "text: text to send"),
                          NULL, &irc_command_squery, NULL);
    weechat_hook_command ("squit",
                          N_("disconnect server links"),
                          N_("server comment"),
                          N_( "server: server name\n"
                              "comment: comment for quit"),
                          NULL, &irc_command_squit, NULL);
    weechat_hook_command ("stats",
                          N_("query statistics about server"),
                          N_("[query [server]]"),
                          N_(" query: c/h/i/k/l/m/o/y/u (see RFC1459)\n"
                             "server: server name"),
                          NULL, &irc_command_stats, NULL);
    weechat_hook_command ("summon",
                          N_("give users who are on a host running an IRC "
                             "server a message asking them to please join "
                             "IRC"),
                          N_("user [target [channel]]"),
                          N_("   user: username\n"
                             "target: server name\n"
                             "channel: channel name"),
                          NULL, &irc_command_summon, NULL);
    weechat_hook_command ("time",
                          N_("query local time from server"),
                          N_("[target]"),
                          N_("target: query time from specified server"),
                          NULL, &irc_command_time, NULL);
    weechat_hook_command ("topic",
                          N_("get/set channel topic"),
                          N_("[channel] [topic]"),
                          N_("channel: channel name\n"
                             "topic: new topic for "
                             "channel (if topic is \"-delete\" then topic "
                             "is deleted)"),
                          "%(irc_channel_topic)|-delete %-", &irc_command_topic, NULL);
    weechat_hook_command ("trace",
                          N_("find the route to specific server"),
                          N_("[target]"),
                          N_("target: server"),
                          NULL, &irc_command_trace, NULL);
    weechat_hook_command ("unban",
                          N_("unban nicks or hosts"),
                          N_("[channel] nickname [nickname ...]"),
                          N_(" channel: channel for unban\n"
                             "nickname: user or host to unban"),
                          NULL, &irc_command_unban, NULL);
    weechat_hook_command ("userhost",
                          N_("return a list of information about nicknames"),
                          N_("nickname [nickname ...]"),
                          N_("nickname: nickname"),
                          "%(nicks)", &irc_command_userhost, NULL);
    weechat_hook_command ("users",
                          N_("list of users logged into the server"),
                          N_("[target]"),
                          N_("target: server"),
                          NULL, &irc_command_users, NULL);
    weechat_hook_command ("version",
                          N_("give the version info of nick or server "
                             "(current or specified)"),
                          N_("[server | nickname]"),
                          N_("  server: server name\n"
                             "nickname: nickname"),
                          "%(nicks)", &irc_command_version, NULL);
    weechat_hook_command ("voice",
                          N_("give voice to nickname(s)"),
                          N_("[nickname [nickname]]"),
                          "",
                          "%(nicks)|%*", &irc_command_voice, NULL);
    weechat_hook_command ("wallops",
                          N_("send a message to all currently connected users "
                             "who have set the 'w' user mode for themselves"),
                          N_("text"),
                          N_("text to send"),
                          NULL, &irc_command_wallops, NULL);
    weechat_hook_command ("who",
                          N_("generate a query which returns a list of "
                             "information"),
                          N_("[mask [\"o\"]]"),
                          N_("mask: only information which match this mask\n"
                             "   o: only operators are returned according to "
                             "the mask supplied"),
                          "%(irc_channels)", &irc_command_who, NULL);
    weechat_hook_command ("whois",
                          N_("query information about user(s)"),
                          N_("[server] nickname[,nickname]"),
                          N_("  server: server name\n"
                             "nickname: nickname (may be a mask)"),
                          "%(nicks)", &irc_command_whois, NULL);
    weechat_hook_command ("whowas",
                          N_("ask for information about a nickname which no "
                             "longer exists"),
                          N_("nickname [,nickname [,nickname ...]] [count "
                             "[target]]"),
                          N_("nickname: nickname to search\n"
                             "   count: number of replies to return "
                             "(full search if negative number)\n"
                             "  target: reply should match this mask"),
                          "%(nicks)", &irc_command_whowas, NULL);
}

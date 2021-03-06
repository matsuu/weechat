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

/* xfer-command.c: xfer command */


#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "../weechat-plugin.h"
#include "xfer.h"
#include "xfer-buffer.h"
#include "xfer-config.h"


/*
 * xfer_command_xfer_list: list xfer
 */

void
xfer_command_xfer_list (int full)
{
    struct t_xfer *ptr_xfer;
    int i;
    char date[128];
    unsigned long pct_complete;
    struct tm *date_tmp;
    
    if (xfer_list)
    {
        weechat_printf (NULL, "");
        weechat_printf (NULL, _("Xfer list:"));
        i = 1;
        for (ptr_xfer = xfer_list; ptr_xfer; ptr_xfer = ptr_xfer->next_xfer)
        {
            /* xfer info */
            if (XFER_IS_FILE(ptr_xfer->type))
            {
                if (ptr_xfer->size == 0)
                {
                    if (ptr_xfer->status == XFER_STATUS_DONE)
                        pct_complete = 100;
                    else
                        pct_complete = 0;
                }
                else
                    pct_complete = (unsigned long)(((float)(ptr_xfer->pos)/(float)(ptr_xfer->size)) * 100);
                
                weechat_printf (NULL,
                                _("%3d. %s (%s), file: \"%s\" (local: "
                                  "\"%s\"), %s %s, status: %s%s%s "
                                  "(%lu %%)"),
                                i,
                                xfer_type_string[ptr_xfer->type],
                                xfer_protocol_string[ptr_xfer->protocol],
                                ptr_xfer->filename,
                                ptr_xfer->local_filename,
                                (XFER_IS_SEND(ptr_xfer->type)) ?
                                _("sent to") : _("received from"),
                                ptr_xfer->remote_nick,
                                weechat_color (
                                    weechat_config_string (
                                        xfer_config_color_status[ptr_xfer->status])),
                                _(xfer_status_string[ptr_xfer->status]),
                                weechat_color ("chat"),
                                pct_complete);
            }
            else
            {
                date_tmp = localtime (&(ptr_xfer->start_time));
                strftime (date, sizeof (date),
                          "%a, %d %b %Y %H:%M:%S", date_tmp);
                weechat_printf (NULL,
                                _("%3d. %s, chat with %s (local nick: %s), "
                                  "started on %s, status: %s%s"),
                                i,
                                xfer_type_string[ptr_xfer->type],
                                ptr_xfer->remote_nick,
                                ptr_xfer->local_nick,
                                date,
                                weechat_color(
                                    weechat_config_string(
                                        xfer_config_color_status[ptr_xfer->status])),
                                _(xfer_status_string[ptr_xfer->status]));
            }
            
            if (full)
            {
                /* second line of xfer info */
                if (XFER_IS_FILE(ptr_xfer->type))
                {
                    weechat_printf (NULL,
                                    _("     plugin: %s (id: %s), file: %lu "
                                      "bytes (position: %lu), address: "
                                      "%d.%d.%d.%d (port %d)"),
                                    ptr_xfer->plugin_name,
                                    ptr_xfer->plugin_id,
                                    ptr_xfer->size,
                                    ptr_xfer->pos,
                                    ptr_xfer->address >> 24,
                                    (ptr_xfer->address >> 16) & 0xff,
                                    (ptr_xfer->address >> 8) & 0xff,
                                    ptr_xfer->address & 0xff,
                                    ptr_xfer->port);
                    date_tmp = localtime (&(ptr_xfer->start_transfer));
                    strftime (date, sizeof (date),
                              "%a, %d %b %Y %H:%M:%S", date_tmp);
                    weechat_printf (NULL,
                                    _("     fast_send: %s, blocksize: %d, "
                                      "started on %s"),
                                    (ptr_xfer->fast_send) ? _("yes") : _("no"),
                                    ptr_xfer->blocksize,
                                    date);
                }
            }
            i++;
        }
    }
    else
        weechat_printf (NULL, _("No xfer"));
}

/*
 * xfer_command_xfer: command /xfer
 */

int
xfer_command_xfer (void *data, struct t_gui_buffer *buffer, int argc,
                   char **argv, char **argv_eol)
{
    /* make C compiler happy */
    (void) data;
    (void) buffer;
    (void) argv_eol;

    if ((argc > 1) && (weechat_strcasecmp (argv[1], "list") == 0))
    {
        xfer_command_xfer_list (0);
        return WEECHAT_RC_OK;
    }
    
    if ((argc > 1) && (weechat_strcasecmp (argv[1], "listfull") == 0))
    {
        xfer_command_xfer_list (1);
        return WEECHAT_RC_OK;
    }
    
    if (!xfer_buffer)
        xfer_buffer_open ();
    
    if (xfer_buffer)
    {
        weechat_buffer_set (xfer_buffer, "display", "1");
        
        if (argc > 1)
        {
            if (strcmp (argv[1], "up") == 0)
            {
                if (xfer_buffer_selected_line > 0)
                    xfer_buffer_selected_line--;
            }
            else if (strcmp (argv[1], "down") == 0)
            {
                if (xfer_buffer_selected_line < xfer_count - 1)
                    xfer_buffer_selected_line++;
            }
        }
    }
    
    xfer_buffer_refresh (NULL);
    
    return WEECHAT_RC_OK;
}

/*
 * xfer_command_init: add /xfer command
 */

void
xfer_command_init ()
{
    weechat_hook_command ("xfer",
                          N_("xfer control"),
                          "[list | listfull]",
                          N_("    list: list xfer\n"
                             "listfull: list xfer (verbose)\n\n"
                             "Without argument, this command opens buffer "
                             "with xfer list."),
                          "list|listfull", &xfer_command_xfer, NULL);
}

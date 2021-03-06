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


#ifndef __WEECHAT_XFER_CONFIG_H
#define __WEECHAT_XFER_CONFIG_H 1

#define XFER_CONFIG_NAME "xfer"

#define XFER_CONFIG_PROGRESS_BAR_MAX_SIZE 256

extern struct t_config_file *xfer_config;

extern struct t_config_option *xfer_config_look_auto_open_buffer;
extern struct t_config_option *xfer_config_look_progress_bar_size;

extern struct t_config_option *xfer_config_color_text;
extern struct t_config_option *xfer_config_color_text_bg;
extern struct t_config_option *xfer_config_color_text_selected;
extern struct t_config_option *xfer_config_color_status[];

extern struct t_config_option *xfer_config_network_timeout;
extern struct t_config_option *xfer_config_network_blocksize;
extern struct t_config_option *xfer_config_network_fast_send;
extern struct t_config_option *xfer_config_network_port_range;
extern struct t_config_option *xfer_config_network_own_ip;
extern struct t_config_option *xfer_config_network_speed_limit;

extern struct t_config_option *xfer_config_file_download_path;
extern struct t_config_option *xfer_config_file_upload_path;
extern struct t_config_option *xfer_config_file_use_nick_in_filename;
extern struct t_config_option *xfer_config_file_convert_spaces;
extern struct t_config_option *xfer_config_file_auto_rename;
extern struct t_config_option *xfer_config_file_auto_resume;
extern struct t_config_option *xfer_config_file_auto_accept_files;
extern struct t_config_option *xfer_config_file_auto_accept_chats;

extern int xfer_config_init ();
extern int xfer_config_read ();
extern int xfer_config_write ();

#endif /* xfer-config.h */

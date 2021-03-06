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


#ifndef __WEECHAT_XFER_H
#define __WEECHAT_XFER_H 1

#define weechat_plugin weechat_xfer_plugin
#define XFER_PLUGIN_NAME "xfer"

/* xfer types */

enum t_xfer_type
{
    XFER_TYPE_FILE_RECV = 0,
    XFER_TYPE_FILE_SEND,
    XFER_TYPE_CHAT_RECV,
    XFER_TYPE_CHAT_SEND,
    /* number of xfer types */
    XFER_NUM_TYPES,
};

/* xfer protocol (for file transfer) */

enum t_xfer_protocol
{
    XFER_NO_PROTOCOL = 0,
    XFER_PROTOCOL_DCC,
    /* number of xfer protocols */
    XFER_NUM_PROTOCOLS,
};

/* xfer status */

enum t_xfer_status
{
    XFER_STATUS_WAITING = 0,           /* waiting for host answer           */
    XFER_STATUS_CONNECTING,            /* connecting to host                */
    XFER_STATUS_ACTIVE,                /* sending/receiving data            */
    XFER_STATUS_DONE,                  /* transfer done                     */
    XFER_STATUS_FAILED,                /* transfer failed                   */
    XFER_STATUS_ABORTED,               /* transfer aborded by user          */
    /* number of xfer status */
    XFER_NUM_STATUS,
};
    
/* xfer errors */

enum t_xfer_error
{
    XFER_NO_ERROR = 0,                 /* no error to report, all ok!       */
    XFER_ERROR_READ_LOCAL,             /* unable to read local file         */
    XFER_ERROR_SEND_BLOCK,             /* unable to send block to receiver  */
    XFER_ERROR_READ_ACK,               /* unable to read ACK from receiver  */
    XFER_ERROR_CONNECT_SENDER,         /* unable to connect to sender       */
    XFER_ERROR_RECV_BLOCK,             /* unable to recv block from sender  */
    XFER_ERROR_WRITE_LOCAL,            /* unable to write to local file     */
    /* number of errors */
    XFER_NUM_ERRORS,
};

/* xfer blocksize */

#define XFER_BLOCKSIZE_MIN    1024     /* min blocksize when sending file   */
#define XFER_BLOCKSIZE_MAX  102400     /* max blocksize when sending file   */

/* separator in filenames */

#ifdef _WIN32
    #define DIR_SEPARATOR       "\\"
    #define DIR_SEPARATOR_CHAR  '\\'
#else
    #define DIR_SEPARATOR       "/"
    #define DIR_SEPARATOR_CHAR  '/'
#endif

/* macros for type/status */

#define XFER_IS_FILE(type) ((type == XFER_TYPE_FILE_RECV) ||    \
                            (type == XFER_TYPE_FILE_SEND))
#define XFER_IS_CHAT(type) ((type == XFER_TYPE_CHAT_RECV) ||    \
                            (type == XFER_TYPE_CHAT_SEND))
#define XFER_IS_RECV(type) ((type == XFER_TYPE_FILE_RECV) ||    \
                            (type == XFER_TYPE_CHAT_RECV))
#define XFER_IS_SEND(type) ((type == XFER_TYPE_FILE_SEND) ||    \
                            (type == XFER_TYPE_CHAT_SEND))

#define XFER_HAS_ENDED(status) ((status == XFER_STATUS_DONE) ||      \
                                (status == XFER_STATUS_FAILED) ||    \
                                (status == XFER_STATUS_ABORTED))

struct t_xfer
{
    /* data received by xfer to initiate a transfer */
    char *plugin_name;                 /* plugin name                       */
    char *plugin_id;                   /* id used by plugin                 */
    enum t_xfer_type type;             /* xfer type (send/recv file)        */
    enum t_xfer_protocol protocol;     /* xfer protocol (for file transfer) */
    char *remote_nick;                 /* remote nick                       */
    char *local_nick;                  /* local nick                        */
    char *charset_modifier;            /* string for charset modifier_data  */
    char *filename;                    /* filename                          */
    unsigned long size;                /* file size                         */
    char *proxy;                       /* proxy to use (optional)           */
    unsigned long address;             /* local or remote IP address        */
    int port;                          /* remote port                       */
    
    /* internal data */
    enum t_xfer_status status;         /* xfer status (waiting, sending,..) */
    struct t_gui_buffer *buffer;       /* buffer (for chat only)            */
    int fast_send;                     /* fast send file: does not wait ACK */
    int blocksize;                     /* block size for sending file       */
    time_t start_time;                 /* time when xfer started            */
    time_t start_transfer;             /* time when xfer transfer started   */
    int sock;                          /* socket for connection             */
    pid_t child_pid;                   /* pid of child process (send/recv)  */
    int child_read;                    /* to read into child pipe           */
    int child_write;                   /* to write into child pipe          */
    struct t_hook *hook_fd;            /* hook for socket or child pipe     */
    struct t_hook *hook_timer;         /* timeout for recever accept        */
    char *unterminated_message;        /* beginning of a message            */
    int file;                          /* local file (read or write)        */
    char *local_filename;              /* local filename (with path)        */
    int filename_suffix;               /* suffix (like .1) if renaming file */
    unsigned long pos;                 /* number of bytes received/sent     */
    unsigned long ack;                 /* number of bytes received OK       */
    unsigned long start_resume;        /* start of resume (in bytes)        */
    time_t last_check_time;            /* last time we checked bytes snt/rcv*/
    unsigned long last_check_pos;      /* bytes sent/recv at last check     */
    time_t last_activity;              /* time of last byte received/sent   */
    unsigned long bytes_per_sec;       /* bytes per second                  */
    unsigned long eta;                 /* estimated time of arrival         */
    struct t_xfer *prev_xfer;          /* link to previous xfer             */
    struct t_xfer *next_xfer;          /* link to next xfer                 */
};

extern struct t_weechat_plugin *weechat_xfer_plugin;
extern char *xfer_type_string[];
extern char *xfer_protocol_string[];
extern char *xfer_status_string[];
extern struct t_xfer *xfer_list, *last_xfer;
extern int xfer_count;

extern int xfer_valid (struct t_xfer *xfer);
extern struct t_xfer *xfer_search_by_number (int number);
extern struct t_xfer *xfer_search_by_buffer (struct t_gui_buffer *buffer);
extern void xfer_close (struct t_xfer *xfer, enum t_xfer_status status);
extern void xfer_send_signal (struct t_xfer *xfer, const char *signal);
extern void xfer_free (struct t_xfer *xfer);
extern int xfer_add_to_infolist (struct t_infolist *infolist,
                                 struct t_xfer *xfer);

#endif /* xfer.h */

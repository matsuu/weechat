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

/* wee-log.c: WeeChat log file */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>

#ifdef HAVE_FLOCK
#include <sys/file.h>
#endif

#include <sys/types.h>
#include <time.h>

#include "weechat.h"
#include "wee-log.h"
#include "wee-debug.h"
#include "wee-string.h"


char *weechat_log_filename = NULL; /* log name (~/.weechat/weechat.log)     */
FILE *weechat_log_file = NULL;     /* WeeChat log file                      */
int weechat_log_use_time = 1;      /* 0 to temporary disable time in log,   */
                                   /* for example when dumping data         */


/*
 * log_open: initialize log file
 */

int
log_open (const char *filename, const char *mode)
{
    int filename_length;

    /* exit if log already opened */
    if (weechat_log_file)
        return 0;

    if (filename)
        weechat_log_filename = strdup (filename);
    else
    {
        filename_length = strlen (weechat_home) + 64;
        weechat_log_filename = malloc (filename_length);
        snprintf (weechat_log_filename, filename_length,
                  "%s/%s", weechat_home, WEECHAT_LOG_NAME);
    }
    
    weechat_log_file = fopen (weechat_log_filename, mode);
    if (!weechat_log_file)
    {
        free (weechat_log_filename);
        weechat_log_filename = NULL;
        return 0;
    }

#ifdef HAVE_FLOCK
    if ((flock (fileno (weechat_log_file), LOCK_EX | LOCK_NB) != 0))
    {
        if (errno == EWOULDBLOCK)
        {
            fclose (weechat_log_file);
            weechat_log_file = NULL;
            free (weechat_log_filename);
            weechat_log_filename = NULL;
            return 0;
        }
    }
#endif

    return 1;
}

/*
 * log_init: initialize log file
 */

void
log_init ()
{
    if (!log_open (NULL, "w"))
    {
        string_iconv_fprintf (stderr,
                              _("Error: unable to create/append to log file (weechat.log)\n"
                                "If another WeeChat process is using this file, try to run WeeChat\n"
                                "with another home using \"--dir\" command line option.\n"));
        exit (1);
    }
    log_printf ("%s (%s %s %s)",
                PACKAGE_STRING, _("compiled on"), __DATE__, __TIME__);
}

/*
 * log_printf: write a message in WeeChat log (<weechat_home>/weechat.log)
 */

void
log_printf (const char *message, ...)
{
    static char buffer[4096];
    char *ptr_buffer;
    va_list argptr;
    static time_t seconds;
    struct tm *date_tmp;
    
    if (!weechat_log_file)
        return;
    
    va_start (argptr, message);
    vsnprintf (buffer, sizeof (buffer) - 1, message, argptr);
    va_end (argptr);
    
    /* keep only valid chars */
    ptr_buffer = buffer;
    while (ptr_buffer[0])
    {
        if ((ptr_buffer[0] != '\n')
            && (ptr_buffer[0] != '\r')
            && ((unsigned char)(ptr_buffer[0]) < 32))
            ptr_buffer[0] = '.';
        ptr_buffer++;
    }

    if (!weechat_log_use_time)
        string_iconv_fprintf (weechat_log_file, "%s\n", buffer);
    else
    {
        seconds = time (NULL);
        date_tmp = localtime (&seconds);
        if (date_tmp)
            string_iconv_fprintf (weechat_log_file,
                                  "[%04d-%02d-%02d %02d:%02d:%02d] %s\n",
                                  date_tmp->tm_year + 1900, date_tmp->tm_mon + 1,
                                  date_tmp->tm_mday, date_tmp->tm_hour,
                                  date_tmp->tm_min, date_tmp->tm_sec,
                                  buffer);
        else
            string_iconv_fprintf (weechat_log_file, "%s\n", buffer);
    }
    
    fflush (weechat_log_file);
}

/*
 * log_close: close log file
 */

void
log_close ()
{
    /* close log file */
    if (weechat_log_file)
    {
#ifdef HAVE_FLOCK
        flock (fileno (weechat_log_file), LOCK_UN);
#endif
        fclose (weechat_log_file);
        weechat_log_file = NULL;
    }

    /* free filename */
    if (weechat_log_filename)
    {
        free (weechat_log_filename);
        weechat_log_filename = NULL;
    }
}

/*
 * log_crash_rename: rename log file when crashing
 */

int
log_crash_rename ()
{
    char *old_name, *new_name;
    int length;
    time_t time_now;
    struct tm *local_time;

    if (!weechat_log_filename)
        return 0;
    
    old_name = strdup (weechat_log_filename);
    if (!old_name)
        return 0;

    log_close ();
    
    length = strlen (weechat_home) + 128;
    new_name = malloc (length);
    if (new_name)
    {
        time_now = time (NULL);
        local_time = localtime (&time_now);
        snprintf (new_name, length,
                  "%s/weechat_crash_%04d%02d%02d_%d.log",
                  weechat_home,
                  local_time->tm_year + 1900,
                  local_time->tm_mon + 1,
                  local_time->tm_mday,
                  getpid());
        if (rename (old_name, new_name) == 0)
        {
            string_iconv_fprintf (stderr, "*** Full crash dump was saved to %s file.\n",
                                  new_name);
            log_open (new_name, "a");
            free (old_name);
            free (new_name);
            return 1;
        }
        free (new_name);
    }

    free (old_name);
    log_open (NULL, "a");
    return 0;
}

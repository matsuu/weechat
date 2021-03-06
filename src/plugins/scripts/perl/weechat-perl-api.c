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

/* weechat-perl-api.c: Perl API functions */


#undef _

#include <EXTERN.h>
#include <perl.h>
#include <XSUB.h>
#include <time.h>

#include "../../weechat-plugin.h"
#include "../script.h"
#include "../script-api.h"
#include "../script-callback.h"
#include "weechat-perl.h"


#define PERL_RETURN_OK XSRETURN_YES
#define PERL_RETURN_ERROR XSRETURN_NO
#define PERL_RETURN_EMPTY XSRETURN_EMPTY
#define PERL_RETURN_STRING(__string)              \
    if (__string)                                 \
    {                                             \
        XST_mPV (0, __string);                    \
        XSRETURN (1);                             \
    }                                             \
    XST_mPV (0, "");                              \
    XSRETURN (1)
#define PERL_RETURN_STRING_FREE(__string)         \
    if (__string)                                 \
    {                                             \
        XST_mPV (0, __string);                    \
        free (__string);                          \
        XSRETURN (1);                             \
    }                                             \
    XST_mPV (0, "");                              \
    XSRETURN (1)
#define PERL_RETURN_INT(__int)                  \
    XST_mIV (0, __int);                         \
    XSRETURN (1);


extern void boot_DynaLoader (pTHX_ CV* cv);


/*
 * weechat::register: startup function for all WeeChat Perl scripts
 */

XS (XS_weechat_api_register)
{
    char *name, *author, *version, *license, *description, *shutdown_func;
    char *charset;
    dXSARGS;
    
    /* make C compiler happy */
    (void) items;
    (void) cv;
    
    perl_current_script = NULL;
    
    if (items < 7)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(perl_current_script_filename, "register");
        PERL_RETURN_ERROR;
    }
    
    name = SvPV (ST (0), PL_na);
    author = SvPV (ST (1), PL_na);
    version = SvPV (ST (2), PL_na);
    license = SvPV (ST (3), PL_na);
    description = SvPV (ST (4), PL_na);
    shutdown_func = SvPV (ST (5), PL_na);
    charset = SvPV (ST (6), PL_na);
    
    if (script_search (weechat_perl_plugin, perl_scripts, name))
    {
        /* error: another script already exists with this name! */
        weechat_printf (NULL,
                        weechat_gettext ("%s%s: unable to register script "
                                         "\"%s\" (another script already "
                                         "exists with this name)"),
                        weechat_prefix ("error"), PERL_PLUGIN_NAME, name);
        PERL_RETURN_ERROR;
    }
    
    /* register script */
    perl_current_script = script_add (weechat_perl_plugin,
                                      &perl_scripts, &last_perl_script,
                                      (perl_current_script_filename) ?
                                      perl_current_script_filename : "",
                                      name, author, version, license,
                                      description, shutdown_func, charset);
    if (perl_current_script)
    {
        if ((weechat_perl_plugin->debug >= 1) || !perl_quiet)
        {
            weechat_printf (NULL,
                            weechat_gettext ("%s: registered script \"%s\", "
                                             "version %s (%s)"),
                            PERL_PLUGIN_NAME, name, version, description);
        }
    }
    else
    {
        PERL_RETURN_ERROR;
    }
    
    PERL_RETURN_OK;
}

/*
 * weechat::plugin_get_name: get name of plugin (return "core" for WeeChat core)
 */

XS (XS_weechat_api_plugin_get_name)
{
    const char *result;
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PERL_CURRENT_SCRIPT_NAME, "plugin_get_name");
        PERL_RETURN_EMPTY;
    }
    
    if (items < 1)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PERL_CURRENT_SCRIPT_NAME, "plugin_get_name");
        PERL_RETURN_EMPTY;
    }
    
    result = weechat_plugin_get_name (script_str2ptr (SvPV (ST (0), PL_na))); /* plugin */
    
    PERL_RETURN_STRING(result);
}

/*
 * weechat::charser_set: set script charset
 */

XS (XS_weechat_api_charset_set)
{
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PERL_CURRENT_SCRIPT_NAME, "charset_set");
        PERL_RETURN_ERROR;
    }
    
    if (items < 1)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PERL_CURRENT_SCRIPT_NAME, "charset_set");
        PERL_RETURN_ERROR;
    }
    
    script_api_charset_set (perl_current_script,
                            SvPV (ST (0), PL_na)); /* charset */
    
    PERL_RETURN_OK;
}

/*
 * weechat::iconv_to_internal: convert string to internal WeeChat charset
 */

XS (XS_weechat_api_iconv_to_internal)
{
    char *result, *charset, *string;
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PERL_CURRENT_SCRIPT_NAME, "iconv_to_internal");
        PERL_RETURN_EMPTY;
    }
    
    if (items < 2)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PERL_CURRENT_SCRIPT_NAME, "iconv_to_internal");
        PERL_RETURN_EMPTY;
    }
    
    charset = SvPV (ST (0), PL_na);
    string = SvPV (ST (1), PL_na);
    
    result = weechat_iconv_to_internal (charset, string);
    
    PERL_RETURN_STRING_FREE(result);
}

/*
 * weechat::iconv_from_internal: convert string from WeeChat inernal charset
 *                               to another one
 */

XS (XS_weechat_api_iconv_from_internal)
{
    char *result, *charset, *string;
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PERL_CURRENT_SCRIPT_NAME, "iconv_from_internal");
        PERL_RETURN_EMPTY;
    }
    
    if (items < 2)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PERL_CURRENT_SCRIPT_NAME, "iconv_from_internal");
        PERL_RETURN_EMPTY;
    }
    
    charset = SvPV (ST (0), PL_na);
    string = SvPV (ST (1), PL_na);
    
    result = weechat_iconv_from_internal (charset, string);
    
    PERL_RETURN_STRING_FREE(result);
}

/*
 * weechat::gettext: get translated string
 */

XS (XS_weechat_api_gettext)
{
    const char *result;
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PERL_CURRENT_SCRIPT_NAME, "gettext");
        PERL_RETURN_EMPTY;
    }
    
    if (items < 1)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PERL_CURRENT_SCRIPT_NAME, "gettext");
        PERL_RETURN_EMPTY;
    }
    
    result = weechat_gettext (SvPV (ST (0), PL_na)); /* string */
    
    PERL_RETURN_STRING(result);
}

/*
 * weechat::ngettext: get translated string with plural form
 */

XS (XS_weechat_api_ngettext)
{
    char *single, *plural;
    const char *result;
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PERL_CURRENT_SCRIPT_NAME, "ngettext");
        PERL_RETURN_EMPTY;
    }
    
    if (items < 3)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PERL_CURRENT_SCRIPT_NAME, "ngettext");
        PERL_RETURN_EMPTY;
    }
    
    single = SvPV (ST (0), PL_na);
    plural = SvPV (ST (1), PL_na);
    
    result = weechat_ngettext (single, plural,
                               SvIV (ST (2))); /* count */
    
    PERL_RETURN_STRING(result);
}

/*
 * weechat::string_remove_color: remove WeeChat color codes from string
 */

XS (XS_weechat_api_string_remove_color)
{
    char *result, *string, *replacement;
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PERL_CURRENT_SCRIPT_NAME, "string_remove_color");
        PERL_RETURN_EMPTY;
    }
    
    if (items < 2)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PERL_CURRENT_SCRIPT_NAME, "string_remove_color");
        PERL_RETURN_EMPTY;
    }
    
    string = SvPV (ST (0), PL_na);
    replacement = SvPV (ST (1), PL_na);
    
    result = weechat_string_remove_color (string, replacement);
    
    PERL_RETURN_STRING_FREE(result);
}

/*
 * weechat::mkdir_home: create a directory in WeeChat home
 */

XS (XS_weechat_api_mkdir_home)
{
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PERL_CURRENT_SCRIPT_NAME, "mkdir_home");
        PERL_RETURN_ERROR;
    }
    
    if (items < 2)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PERL_CURRENT_SCRIPT_NAME, "mkdir_home");
        PERL_RETURN_ERROR;
    }
    
    if (weechat_mkdir_home (SvPV (ST (0), PL_na), /* directory */
                            SvIV (ST (1)))) /* mode */
        PERL_RETURN_OK;
    
    PERL_RETURN_ERROR;
}

/*
 * weechat::mkdir: create a directory
 */

XS (XS_weechat_api_mkdir)
{
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PERL_CURRENT_SCRIPT_NAME, "mkdir");
        PERL_RETURN_ERROR;
    }
    
    if (items < 2)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PERL_CURRENT_SCRIPT_NAME, "mkdir");
        PERL_RETURN_ERROR;
    }
    
    if (weechat_mkdir (SvPV (ST (0), PL_na), /* directory */
                       SvIV (ST (1)))) /* mode */
        PERL_RETURN_OK;
    
    PERL_RETURN_ERROR;
}

/*
 * weechat::mkdir_parents: create a directory and make parent directories as
 *                         needed
 */

XS (XS_weechat_api_mkdir_parents)
{
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PERL_CURRENT_SCRIPT_NAME, "mkdir_parents");
        PERL_RETURN_ERROR;
    }
    
    if (items < 2)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PERL_CURRENT_SCRIPT_NAME, "mkdir_parents");
        PERL_RETURN_ERROR;
    }
    
    if (weechat_mkdir_parents (SvPV (ST (0), PL_na), /* directory */
                               SvIV (ST (1)))) /* mode */
        PERL_RETURN_OK;
    
    PERL_RETURN_ERROR;
}

/*
 * weechat::list_new: create a new list
 */

XS (XS_weechat_api_list_new)
{
    char *result;
    dXSARGS;
    
    /* make C compiler happy */
    (void) items;
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PERL_CURRENT_SCRIPT_NAME, "list_new");
	PERL_RETURN_EMPTY;
    }
    
    result = script_ptr2str (weechat_list_new ());
    
    PERL_RETURN_STRING_FREE(result);
}

/*
 * weechat::list_add: add a string to list
 */

XS (XS_weechat_api_list_add)
{
    char *result, *weelist, *data, *where, *user_data;
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PERL_CURRENT_SCRIPT_NAME, "list_add");
	PERL_RETURN_EMPTY;
    }
    
    if (items < 4)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PERL_CURRENT_SCRIPT_NAME, "list_add");
        PERL_RETURN_EMPTY;
    }
    
    weelist = SvPV (ST (0), PL_na);
    data = SvPV (ST (1), PL_na);
    where = SvPV (ST (2), PL_na);
    user_data = SvPV (ST (3), PL_na);
    
    result = script_ptr2str (weechat_list_add (script_str2ptr (weelist),
                                               data,
                                               where,
                                               script_str2ptr (user_data)));
    
    PERL_RETURN_STRING_FREE(result);
}

/*
 * weechat::list_search: search a string in list
 */

XS (XS_weechat_api_list_search)
{
    char *result, *weelist, *data;
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PERL_CURRENT_SCRIPT_NAME, "list_search");
	PERL_RETURN_EMPTY;
    }
    
    if (items < 2)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PERL_CURRENT_SCRIPT_NAME, "list_search");
        PERL_RETURN_EMPTY;
    }
    
    weelist = SvPV (ST (0), PL_na);
    data = SvPV (ST (1), PL_na);
    
    result = script_ptr2str (weechat_list_search (script_str2ptr (weelist),
                                                  data));
    
    PERL_RETURN_STRING_FREE(result);
}

/*
 * weechat::list_casesearch: search a string in list (ignore case)
 */

XS (XS_weechat_api_list_casesearch)
{
    char *result, *weelist, *data;
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PERL_CURRENT_SCRIPT_NAME, "list_casesearch");
	PERL_RETURN_EMPTY;
    }
    
    if (items < 2)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PERL_CURRENT_SCRIPT_NAME, "list_casesearch");
        PERL_RETURN_EMPTY;
    }
    
    weelist = SvPV (ST (0), PL_na);
    data = SvPV (ST (1), PL_na);
    
    result = script_ptr2str (weechat_list_casesearch (script_str2ptr (weelist),
                                                      data));
    
    PERL_RETURN_STRING_FREE(result);
}

/*
 * weechat::list_get: get item by position
 */

XS (XS_weechat_api_list_get)
{
    char *result;
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PERL_CURRENT_SCRIPT_NAME, "list_get");
	PERL_RETURN_EMPTY;
    }
    
    if (items < 2)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PERL_CURRENT_SCRIPT_NAME, "list_get");
        PERL_RETURN_EMPTY;
    }
    
    result = script_ptr2str (weechat_list_get (script_str2ptr (SvPV (ST (0), PL_na)), /* weelist */
                                               SvIV (ST (1)))); /* position */
    
    PERL_RETURN_STRING_FREE(result);
}

/*
 * weechat::list_set: set new value for item
 */

XS (XS_weechat_api_list_set)
{
    char *item, *new_value;
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PERL_CURRENT_SCRIPT_NAME, "list_set");
	PERL_RETURN_ERROR;
    }
    
    if (items < 2)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PERL_CURRENT_SCRIPT_NAME, "list_set");
        PERL_RETURN_ERROR;
    }
    
    item = SvPV (ST (0), PL_na);
    new_value = SvPV (ST (1), PL_na);
    
    weechat_list_set (script_str2ptr (item), new_value);
    
    PERL_RETURN_OK;
}

/*
 * weechat::list_next: get next item
 */

XS (XS_weechat_api_list_next)
{
    char *result;
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PERL_CURRENT_SCRIPT_NAME, "list_next");
	PERL_RETURN_EMPTY;
    }
    
    if (items < 1)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PERL_CURRENT_SCRIPT_NAME, "list_next");
        PERL_RETURN_EMPTY;
    }
    
    result = script_ptr2str (weechat_list_next (script_str2ptr (SvPV (ST (0), PL_na)))); /* item */
    
    PERL_RETURN_STRING_FREE(result);
}

/*
 * weechat::list_prev: get previous item
 */

XS (XS_weechat_api_list_prev)
{
    char *result;
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PERL_CURRENT_SCRIPT_NAME, "list_prev");
	PERL_RETURN_EMPTY;
    }
    
    if (items < 1)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PERL_CURRENT_SCRIPT_NAME, "list_prev");
        PERL_RETURN_EMPTY;
    }
    
    result = script_ptr2str (weechat_list_prev (script_str2ptr (SvPV (ST (0), PL_na)))); /* item */
    
    PERL_RETURN_STRING_FREE(result);
}

/*
 * weechat::list_string: get string value of item
 */

XS (XS_weechat_api_list_string)
{
    const char *result;
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PERL_CURRENT_SCRIPT_NAME, "list_string");
	PERL_RETURN_EMPTY;
    }
    
    if (items < 1)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PERL_CURRENT_SCRIPT_NAME, "list_string");
        PERL_RETURN_EMPTY;
    }
    
    result = weechat_list_string (script_str2ptr (SvPV (ST (0), PL_na))); /* item */
    
    PERL_RETURN_STRING(result);
}

/*
 * weechat::list_size: get number of elements in list
 */

XS (XS_weechat_api_list_size)
{
    int size;
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PERL_CURRENT_SCRIPT_NAME, "list_size");
	PERL_RETURN_INT(0);
    }
    
    if (items < 1)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PERL_CURRENT_SCRIPT_NAME, "list_size");
        PERL_RETURN_INT(0);
    }
    
    size = weechat_list_size (script_str2ptr (SvPV (ST (0), PL_na))); /* weelist */
    
    PERL_RETURN_INT(size);
}

/*
 * weechat::list_remove: remove item from list
 */

XS (XS_weechat_api_list_remove)
{
    char *weelist, *item;
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PERL_CURRENT_SCRIPT_NAME, "list_remove");
	PERL_RETURN_ERROR;
    }
    
    if (items < 2)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PERL_CURRENT_SCRIPT_NAME, "list_remove");
        PERL_RETURN_ERROR;
    }
    
    weelist = SvPV (ST (0), PL_na);
    item = SvPV (ST (1), PL_na);
    
    weechat_list_remove (script_str2ptr (weelist), script_str2ptr (item));
    
    PERL_RETURN_OK;
}

/*
 * weechat::list_remove_all: remove all items from list
 */

XS (XS_weechat_api_list_remove_all)
{
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PERL_CURRENT_SCRIPT_NAME, "list_remove_all");
	PERL_RETURN_ERROR;
    }
    
    if (items < 1)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PERL_CURRENT_SCRIPT_NAME, "list_remove_all");
        PERL_RETURN_ERROR;
    }
    
    weechat_list_remove_all (script_str2ptr (SvPV (ST (0), PL_na))); /* weelist */
    
    PERL_RETURN_OK;
}

/*
 * weechat::list_free: free list
 */

XS (XS_weechat_api_list_free)
{
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PERL_CURRENT_SCRIPT_NAME, "list_free");
	PERL_RETURN_ERROR;
    }
    
    if (items < 1)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PERL_CURRENT_SCRIPT_NAME, "list_free");
        PERL_RETURN_ERROR;
    }
    
    weechat_list_free (script_str2ptr (SvPV (ST (0), PL_na))); /* weelist */
    
    PERL_RETURN_OK;
}

/*
 * weechat_perl_api_config_reload_cb: callback for config reload
 */

int
weechat_perl_api_config_reload_cb (void *data,
                                   struct t_config_file *config_file)
{
    struct t_script_callback *script_callback;
    char *perl_argv[3], empty_arg[1] = { '\0' };
    int *rc, ret;
    
    script_callback = (struct t_script_callback *)data;

    if (script_callback && script_callback->function && script_callback->function[0])
    {
        perl_argv[0] = (script_callback->data) ? script_callback->data : empty_arg;
        perl_argv[1] = script_ptr2str (config_file);
        perl_argv[2] = NULL;
        
        rc = (int *) weechat_perl_exec (script_callback->script,
                                        WEECHAT_SCRIPT_EXEC_INT,
                                        script_callback->function,
                                        perl_argv);
        
        if (!rc)
            ret = WEECHAT_CONFIG_READ_FILE_NOT_FOUND;
        else
        {
            ret = *rc;
            free (rc);
        }
        if (perl_argv[1])
            free (perl_argv[1]);

        return ret;
    }
    
    return WEECHAT_CONFIG_READ_FILE_NOT_FOUND;
}

/*
 * weechat::config_new: create a new configuration file
 */

XS (XS_weechat_api_config_new)
{
    char *result, *name, *function, *data;
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PERL_CURRENT_SCRIPT_NAME, "config_new");
	PERL_RETURN_EMPTY;
    }
    
    if (items < 3)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PERL_CURRENT_SCRIPT_NAME, "config_new");
        PERL_RETURN_EMPTY;
    }
    
    name = SvPV (ST (0), PL_na);
    function = SvPV (ST (1), PL_na);
    data = SvPV (ST (2), PL_na);
    
    result = script_ptr2str (script_api_config_new (weechat_perl_plugin,
                                                    perl_current_script,
                                                    name,
                                                    &weechat_perl_api_config_reload_cb,
                                                    function,
                                                    data));
    
    PERL_RETURN_STRING_FREE(result);
}

/*
 * weechat_perl_api_config_section_read_cb: callback for reading option in section
 */

int
weechat_perl_api_config_section_read_cb (void *data,
                                         struct t_config_file *config_file,
                                         struct t_config_section *section,
                                         const char *option_name,
                                         const char *value)
{
    struct t_script_callback *script_callback;
    char *perl_argv[6], empty_arg[1] = { '\0' };
    int *rc, ret;
    
    script_callback = (struct t_script_callback *)data;

    if (script_callback && script_callback->function && script_callback->function[0])
    {
        perl_argv[0] = (script_callback->data) ? script_callback->data : empty_arg;
        perl_argv[1] = script_ptr2str (config_file);
        perl_argv[2] = script_ptr2str (section);
        perl_argv[3] = (option_name) ? (char *)option_name : empty_arg;
        perl_argv[4] = (value) ? (char *)value : empty_arg;
        perl_argv[5] = NULL;
        
        rc = (int *) weechat_perl_exec (script_callback->script,
                                        WEECHAT_SCRIPT_EXEC_INT,
                                        script_callback->function,
                                        perl_argv);
        
        if (!rc)
            ret = WEECHAT_CONFIG_OPTION_SET_ERROR;
        else
        {
            ret = *rc;
            free (rc);
        }
        if (perl_argv[1])
            free (perl_argv[1]);
        if (perl_argv[2])
            free (perl_argv[2]);
        
        return ret;
    }
    
    return WEECHAT_CONFIG_OPTION_SET_ERROR;
}

/*
 * weechat_perl_api_config_section_write_cb: callback for writing section
 */

void
weechat_perl_api_config_section_write_cb (void *data,
                                          struct t_config_file *config_file,
                                          const char *section_name)
{
    struct t_script_callback *script_callback;
    char *perl_argv[4], empty_arg[1] = { '\0' };
    int *rc;
    
    script_callback = (struct t_script_callback *)data;

    if (script_callback && script_callback->function && script_callback->function[0])
    {
        perl_argv[0] = (script_callback->data) ? script_callback->data : empty_arg;
        perl_argv[1] = script_ptr2str (config_file);
        perl_argv[2] = (section_name) ? (char *)section_name : empty_arg;
        perl_argv[3] = NULL;
        
        rc = (int *) weechat_perl_exec (script_callback->script,
                                        WEECHAT_SCRIPT_EXEC_INT,
                                        script_callback->function,
                                        perl_argv);

        if (rc)
            free (rc);
        if (perl_argv[1])
            free (perl_argv[1]);
    }
}

/*
 * weechat_perl_api_config_section_write_default_cb: callback for writing
 *                                                   default values for section
 */

void
weechat_perl_api_config_section_write_default_cb (void *data,
                                                  struct t_config_file *config_file,
                                                  const char *section_name)
{
    struct t_script_callback *script_callback;
    char *perl_argv[4], empty_arg[1] = { '\0' };
    int *rc;
    
    script_callback = (struct t_script_callback *)data;

    if (script_callback && script_callback->function && script_callback->function[0])
    {
        perl_argv[0] = (script_callback->data) ? script_callback->data : empty_arg;
        perl_argv[1] = script_ptr2str (config_file);
        perl_argv[2] = (section_name) ? (char *)section_name : empty_arg;
        perl_argv[3] = NULL;
        
        rc = (int *) weechat_perl_exec (script_callback->script,
                                        WEECHAT_SCRIPT_EXEC_INT,
                                        script_callback->function,
                                        perl_argv);
        
        if (rc)
            free (rc);
        if (perl_argv[1])
            free (perl_argv[1]);
    }
}

/*
 * weechat_perl_api_config_section_create_option_cb: callback to create an option
 */

int
weechat_perl_api_config_section_create_option_cb (void *data,
                                                  struct t_config_file *config_file,
                                                  struct t_config_section *section,
                                                  const char *option_name,
                                                  const char *value)
{
    struct t_script_callback *script_callback;
    char *perl_argv[6], empty_arg[1] = { '\0' };
    int *rc, ret;
    
    script_callback = (struct t_script_callback *)data;
    
    if (script_callback && script_callback->function && script_callback->function[0])
    {
        perl_argv[0] = (script_callback->data) ? script_callback->data : empty_arg;
        perl_argv[1] = script_ptr2str (config_file);
        perl_argv[2] = script_ptr2str (section);
        perl_argv[3] = (option_name) ? (char *)option_name : empty_arg;
        perl_argv[4] = (value) ? (char *)value : empty_arg;
        perl_argv[5] = NULL;
        
        rc = (int *) weechat_perl_exec (script_callback->script,
                                        WEECHAT_SCRIPT_EXEC_INT,
                                        script_callback->function,
                                        perl_argv);
        
        if (!rc)
            ret = WEECHAT_CONFIG_OPTION_SET_ERROR;
        else
        {
            ret = *rc;
            free (rc);
        }
        if (perl_argv[1])
            free (perl_argv[1]);
        if (perl_argv[2])
            free (perl_argv[2]);
        
        return ret;
    }
    
    return WEECHAT_CONFIG_OPTION_SET_ERROR;
}

/*
 * weechat_perl_api_config_section_delete_option_cb: callback to delete an option
 */

int
weechat_perl_api_config_section_delete_option_cb (void *data,
                                                  struct t_config_file *config_file,
                                                  struct t_config_section *section,
                                                  struct t_config_option *option)
{
    struct t_script_callback *script_callback;
    char *perl_argv[5], empty_arg[1] = { '\0' };
    int *rc, ret;
    
    script_callback = (struct t_script_callback *)data;
    
    if (script_callback && script_callback->function && script_callback->function[0])
    {
        perl_argv[0] = (script_callback->data) ? script_callback->data : empty_arg;
        perl_argv[1] = script_ptr2str (config_file);
        perl_argv[2] = script_ptr2str (section);
        perl_argv[3] = script_ptr2str (option);
        perl_argv[4] = NULL;
        
        rc = (int *) weechat_perl_exec (script_callback->script,
                                        WEECHAT_SCRIPT_EXEC_INT,
                                        script_callback->function,
                                        perl_argv);
        
        if (!rc)
            ret = WEECHAT_CONFIG_OPTION_UNSET_ERROR;
        else
        {
            ret = *rc;
            free (rc);
        }
        if (perl_argv[1])
            free (perl_argv[1]);
        if (perl_argv[2])
            free (perl_argv[2]);
        if (perl_argv[3])
            free (perl_argv[3]);
        
        return ret;
    }
    
    return WEECHAT_CONFIG_OPTION_UNSET_ERROR;
}

/*
 * weechat::config_new_section: create a new section in configuration file
 */

XS (XS_weechat_api_config_new_section)
{
    char *result, *cfg_file, *name, *function_read, *data_read;
    char *function_write, *data_write, *function_write_default;
    char *data_write_default, *function_create_option, *data_create_option;
    char *function_delete_option, *data_delete_option;
    
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PERL_CURRENT_SCRIPT_NAME, "config_new_section");
	PERL_RETURN_EMPTY;
    }
    
    if (items < 14)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PERL_CURRENT_SCRIPT_NAME, "config_new_section");
        PERL_RETURN_EMPTY;
    }
    
    cfg_file = SvPV (ST (0), PL_na);
    name = SvPV (ST (1), PL_na);
    function_read = SvPV (ST (4), PL_na);
    data_read = SvPV (ST (5), PL_na);
    function_write = SvPV (ST (6), PL_na);
    data_write = SvPV (ST (7), PL_na);
    function_write_default = SvPV (ST (8), PL_na);
    data_write_default = SvPV (ST (9), PL_na);
    function_create_option = SvPV (ST (10), PL_na);
    data_create_option = SvPV (ST (11), PL_na);
    function_delete_option = SvPV (ST (12), PL_na);
    data_delete_option = SvPV (ST (13), PL_na);
    
    result = script_ptr2str (script_api_config_new_section (weechat_perl_plugin,
                                                            perl_current_script,
                                                            script_str2ptr (cfg_file),
                                                            name,
                                                            SvIV (ST (2)), /* user_can_add_options */
                                                            SvIV (ST (3)), /* user_can_delete_options */
                                                            &weechat_perl_api_config_section_read_cb,
                                                            function_read,
                                                            data_read,
                                                            &weechat_perl_api_config_section_write_cb,
                                                            function_write,
                                                            data_write,
                                                            &weechat_perl_api_config_section_write_default_cb,
                                                            function_write_default,
                                                            data_write_default,
                                                            &weechat_perl_api_config_section_create_option_cb,
                                                            function_create_option,
                                                            data_create_option,
                                                            &weechat_perl_api_config_section_delete_option_cb,
                                                            function_delete_option,
                                                            data_delete_option));
    
    PERL_RETURN_STRING_FREE(result);
}

/*
 * weechat::config_search_section: search section in configuration file
 */

XS (XS_weechat_api_config_search_section)
{
    char *result, *config_file, *section_name;
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PERL_CURRENT_SCRIPT_NAME, "config_search_section");
	PERL_RETURN_EMPTY;
    }
    
    if (items < 2)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PERL_CURRENT_SCRIPT_NAME, "config_search_section");
        PERL_RETURN_EMPTY;
    }
    
    config_file = SvPV (ST (0), PL_na);
    section_name = SvPV (ST (1), PL_na);
    
    result = script_ptr2str (weechat_config_search_section (script_str2ptr (config_file),
                                                            section_name));
    
    PERL_RETURN_STRING_FREE(result);
}

/*
 * weechat_perl_api_config_option_check_value_cb: callback for checking new
 *                                                value for option
 */

int
weechat_perl_api_config_option_check_value_cb (void *data,
                                               struct t_config_option *option,
                                               const char *value)
{
    struct t_script_callback *script_callback;
    char *perl_argv[4], empty_arg[1] = { '\0' };
    int *rc, ret;
    
    script_callback = (struct t_script_callback *)data;
    
    if (script_callback && script_callback->function && script_callback->function[0])
    {
        perl_argv[0] = (script_callback->data) ? script_callback->data : empty_arg;
        perl_argv[1] = script_ptr2str (option);
        perl_argv[2] = (value) ? (char *)value : empty_arg;
        perl_argv[3] = NULL;
        
        rc = (int *) weechat_perl_exec (script_callback->script,
                                        WEECHAT_SCRIPT_EXEC_INT,
                                        script_callback->function,
                                        perl_argv);
        
        if (!rc)
            ret = 0;
        else
        {
            ret = *rc;
            free (rc);
        }
        if (perl_argv[1])
            free (perl_argv[1]);
        
        return ret;
    }
    
    return 0;
}

/*
 * weechat_perl_api_config_option_change_cb: callback for option changed
 */

void
weechat_perl_api_config_option_change_cb (void *data,
                                          struct t_config_option *option)
{
    struct t_script_callback *script_callback;
    char *perl_argv[3], empty_arg[1] = { '\0' };
    int *rc;
    
    script_callback = (struct t_script_callback *)data;
    
    if (script_callback && script_callback->function && script_callback->function[0])
    {
        perl_argv[0] = (script_callback->data) ? script_callback->data : empty_arg;
        perl_argv[1] = script_ptr2str (option);
        perl_argv[2] = NULL;
        
        rc = (int *) weechat_perl_exec (script_callback->script,
                                        WEECHAT_SCRIPT_EXEC_INT,
                                        script_callback->function,
                                        perl_argv);
        
        if (perl_argv[1])
            free (perl_argv[1]);
        
        if (rc)
            free (rc);
    }
}

/*
 * weechat_perl_api_config_option_delete_cb: callback when option is deleted
 */

void
weechat_perl_api_config_option_delete_cb (void *data,
                                          struct t_config_option *option)
{
    struct t_script_callback *script_callback;
    char *perl_argv[3], empty_arg[1] = { '\0' };
    int *rc;
    
    script_callback = (struct t_script_callback *)data;
    
    if (script_callback && script_callback->function && script_callback->function[0])
    {
        perl_argv[0] = (script_callback->data) ? script_callback->data : empty_arg;
        perl_argv[1] = script_ptr2str (option);
        perl_argv[2] = NULL;
        
        rc = (int *) weechat_perl_exec (script_callback->script,
                                        WEECHAT_SCRIPT_EXEC_INT,
                                        script_callback->function,
                                        perl_argv);
        
        if (perl_argv[1])
            free (perl_argv[1]);
        
        if (rc)
            free (rc);
    }
}

/*
 * weechat::config_new_option: create a new option in section
 */

XS (XS_weechat_api_config_new_option)
{
    char *result, *config_file, *section, *name, *type;
    char *description, *string_values, *default_value, *value;
    char *function_check_value, *data_check_value, *function_change;
    char *data_change, *function_delete, *data_delete;
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PERL_CURRENT_SCRIPT_NAME, "config_new_option");
	PERL_RETURN_EMPTY;
    }
    
    if (items < 17)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PERL_CURRENT_SCRIPT_NAME, "config_new_option");
        PERL_RETURN_EMPTY;
    }
    
    config_file = SvPV (ST (0), PL_na);
    section = SvPV (ST (1), PL_na);
    name = SvPV (ST (2), PL_na);
    type = SvPV (ST (3), PL_na);
    description = SvPV (ST (4), PL_na);
    string_values = SvPV (ST (5), PL_na);
    default_value = SvPV (ST (8), PL_na);
    value = SvPV (ST (9), PL_na);
    function_check_value = SvPV (ST (11), PL_na);
    data_check_value = SvPV (ST (12), PL_na);
    function_change = SvPV (ST (13), PL_na);
    data_change = SvPV (ST (14), PL_na);
    function_delete = SvPV (ST (15), PL_na);
    data_delete = SvPV (ST (16), PL_na);
    
    result = script_ptr2str (script_api_config_new_option (weechat_perl_plugin,
                                                           perl_current_script,
                                                           script_str2ptr (config_file),
                                                           script_str2ptr (section),
                                                           name,
                                                           type,
                                                           description,
                                                           string_values,
                                                           SvIV (ST (6)), /* min */
                                                           SvIV (ST (7)), /* max */
                                                           default_value,
                                                           value,
                                                           SvIV (ST (10)), /* null_value_allowed */
                                                           &weechat_perl_api_config_option_check_value_cb,
                                                           function_check_value,
                                                           data_check_value,
                                                           &weechat_perl_api_config_option_change_cb,
                                                           function_change,
                                                           data_change,
                                                           &weechat_perl_api_config_option_delete_cb,
                                                           function_delete,
                                                           data_delete));
    
    PERL_RETURN_STRING_FREE(result);
}

/*
 * weechat::config_search_option: search option in configuration file or section
 */

XS (XS_weechat_api_config_search_option)
{
    char *result, *config_file, *section, *option_name;
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PERL_CURRENT_SCRIPT_NAME, "config_search_option");
	PERL_RETURN_EMPTY;
    }
    
    if (items < 3)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PERL_CURRENT_SCRIPT_NAME, "config_search_option");
        PERL_RETURN_EMPTY;
    }
    
    config_file = SvPV (ST (0), PL_na);
    section = SvPV (ST (1), PL_na);
    option_name = SvPV (ST (2), PL_na);
    
    result = script_ptr2str (weechat_config_search_option (script_str2ptr (config_file),
                                                           script_str2ptr (section),
                                                           option_name));
    
    PERL_RETURN_STRING_FREE(result);
}

/*
 * weechat::config_string_to_boolean: return boolean value of a string
 */

XS (XS_weechat_api_config_string_to_boolean)
{
    int value;
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PERL_CURRENT_SCRIPT_NAME, "config_string_to_boolean");
	PERL_RETURN_INT(0);
    }
    
    if (items < 1)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PERL_CURRENT_SCRIPT_NAME, "config_string_to_boolean");
        PERL_RETURN_INT(0);
    }
    
    value = weechat_config_string_to_boolean (SvPV (ST (0), PL_na)); /* text */
    
    PERL_RETURN_INT(value);
}

/*
 * weechat::config_option_reset: reset an option with default value
 */

XS (XS_weechat_api_config_option_reset)
{
    int rc;
    char *option;
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PERL_CURRENT_SCRIPT_NAME, "config_option_reset");
	PERL_RETURN_INT(0);
    }
    
    if (items < 2)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PERL_CURRENT_SCRIPT_NAME, "config_option_reset");
        PERL_RETURN_INT(0);
    }
    
    option = SvPV (ST (0), PL_na);
    
    rc = weechat_config_option_reset (script_str2ptr (option),
                                      SvIV (ST (1))); /* run_callback */
    
    PERL_RETURN_INT(rc);
}

/*
 * weechat::config_option_set: set new value for option
 */

XS (XS_weechat_api_config_option_set)
{
    int rc;
    char *option, *new_value;
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PERL_CURRENT_SCRIPT_NAME, "config_option_set");
	PERL_RETURN_INT(WEECHAT_CONFIG_OPTION_SET_ERROR);
    }
    
    if (items < 3)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PERL_CURRENT_SCRIPT_NAME, "config_option_set");
        PERL_RETURN_INT(WEECHAT_CONFIG_OPTION_SET_ERROR);
    }
    
    option = SvPV (ST (0), PL_na);
    new_value = SvPV (ST (1), PL_na);
    
    rc = weechat_config_option_set (script_str2ptr (option),
                                    new_value,
                                    SvIV (ST (2))); /* run_callback */
    
    PERL_RETURN_INT(rc);
}

/*
 * weechat::config_option_set_null: set null (undefined) value for option
 */

XS (XS_weechat_api_config_option_set_null)
{
    int rc;
    char *option;
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PERL_CURRENT_SCRIPT_NAME, "config_option_set_null");
	PERL_RETURN_INT(WEECHAT_CONFIG_OPTION_SET_ERROR);
    }
    
    if (items < 2)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PERL_CURRENT_SCRIPT_NAME, "config_option_set_null");
        PERL_RETURN_INT(WEECHAT_CONFIG_OPTION_SET_ERROR);
    }
    
    option = SvPV (ST (0), PL_na);
    
    rc = weechat_config_option_set_null (script_str2ptr (option),
                                         SvIV (ST (1))); /* run_callback */
    
    PERL_RETURN_INT(rc);
}

/*
 * weechat::config_option_unset: unset an option
 */

XS (XS_weechat_api_config_option_unset)
{
    int rc;
    char *option;
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PERL_CURRENT_SCRIPT_NAME, "config_option_unset");
	PERL_RETURN_INT(WEECHAT_CONFIG_OPTION_UNSET_ERROR);
    }
    
    if (items < 1)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PERL_CURRENT_SCRIPT_NAME, "config_option_unset");
        PERL_RETURN_INT(WEECHAT_CONFIG_OPTION_UNSET_ERROR);
    }
    
    option = SvPV (ST (0), PL_na);
    
    rc = weechat_config_option_unset (script_str2ptr (option));
    
    PERL_RETURN_INT(rc);
}

/*
 * weechat::config_option_rename: rename an option
 */

XS (XS_weechat_api_config_option_rename)
{
    char *option, *new_name;
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PERL_CURRENT_SCRIPT_NAME, "config_option_rename");
	PERL_RETURN_ERROR;
    }
    
    if (items < 2)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PERL_CURRENT_SCRIPT_NAME, "config_option_rename");
        PERL_RETURN_ERROR;
    }
    
    option = SvPV (ST (0), PL_na);
    new_name = SvPV (ST (1), PL_na);
    
    weechat_config_option_rename (script_str2ptr (option),
                                  new_name);
    
    PERL_RETURN_OK;
}

/*
 * weechat::config_option_is_null: return 1 if value of option is null
 */

XS (XS_weechat_api_config_option_is_null)
{
    int value;
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PERL_CURRENT_SCRIPT_NAME, "config_option_is_null");
	PERL_RETURN_INT(1);
    }
    
    if (items < 1)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PERL_CURRENT_SCRIPT_NAME, "config_option_is_null");
        PERL_RETURN_INT(1);
    }
    
    value = weechat_config_option_is_null (script_str2ptr (SvPV (ST (0), PL_na))); /* option */
    
    PERL_RETURN_INT(value);
}

/*
 * weechat::config_option_default_is_null: return 1 if default value of option
 *                                         is null
 */

XS (XS_weechat_api_config_option_default_is_null)
{
    int value;
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PERL_CURRENT_SCRIPT_NAME, "config_option_default_is_null");
	PERL_RETURN_INT(1);
    }
    
    if (items < 1)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PERL_CURRENT_SCRIPT_NAME, "config_option_default_is_null");
        PERL_RETURN_INT(1);
    }
    
    value = weechat_config_option_default_is_null (script_str2ptr (SvPV (ST (0), PL_na))); /* option */
    
    PERL_RETURN_INT(value);
}

/*
 * weechat::config_boolean: return boolean value of option
 */

XS (XS_weechat_api_config_boolean)
{
    int value;
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PERL_CURRENT_SCRIPT_NAME, "config_boolean");
	PERL_RETURN_INT(0);
    }
    
    if (items < 1)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PERL_CURRENT_SCRIPT_NAME, "config_boolean");
        PERL_RETURN_INT(0);
    }
    
    value = weechat_config_boolean (script_str2ptr (SvPV (ST (0), PL_na))); /* option */
    
    PERL_RETURN_INT(value);
}

/*
 * weechat::config_boolean_default: return default boolean value of option
 */

XS (XS_weechat_api_config_boolean_default)
{
    int value;
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PERL_CURRENT_SCRIPT_NAME, "config_boolean_default");
	PERL_RETURN_INT(0);
    }
    
    if (items < 1)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PERL_CURRENT_SCRIPT_NAME, "config_boolean_default");
        PERL_RETURN_INT(0);
    }
    
    value = weechat_config_boolean_default (script_str2ptr (SvPV (ST (0), PL_na))); /* option */
    
    PERL_RETURN_INT(value);
}

/*
 * weechat::config_integer: return integer value of option
 */

XS (XS_weechat_api_config_integer)
{
    int value;
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PERL_CURRENT_SCRIPT_NAME, "config_integer");
	PERL_RETURN_INT(0);
    }
    
    if (items < 1)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PERL_CURRENT_SCRIPT_NAME, "config_integer");
        PERL_RETURN_INT(0);
    }
    
    value = weechat_config_integer (script_str2ptr (SvPV (ST (0), PL_na))); /* option */
    
    PERL_RETURN_INT(value);
}

/*
 * weechat::config_integer_default: return default integer value of option
 */

XS (XS_weechat_api_config_integer_default)
{
    int value;
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PERL_CURRENT_SCRIPT_NAME, "config_integer_default");
	PERL_RETURN_INT(0);
    }
    
    if (items < 1)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PERL_CURRENT_SCRIPT_NAME, "config_integer_default");
        PERL_RETURN_INT(0);
    }
    
    value = weechat_config_integer_default (script_str2ptr (SvPV (ST (0), PL_na))); /* option */
    
    PERL_RETURN_INT(value);
}

/*
 * weechat::config_string: return string value of option
 */

XS (XS_weechat_api_config_string)
{
    const char *result;
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PERL_CURRENT_SCRIPT_NAME, "config_string");
	PERL_RETURN_EMPTY;
    }
    
    if (items < 1)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PERL_CURRENT_SCRIPT_NAME, "config_string");
        PERL_RETURN_EMPTY;
    }
    
    result = weechat_config_string (script_str2ptr (SvPV (ST (0), PL_na))); /* option */
    
    PERL_RETURN_STRING(result);
}

/*
 * weechat::config_string_default: return default string value of option
 */

XS (XS_weechat_api_config_string_default)
{
    const char *result;
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PERL_CURRENT_SCRIPT_NAME, "config_string_default");
	PERL_RETURN_EMPTY;
    }
    
    if (items < 1)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PERL_CURRENT_SCRIPT_NAME, "config_string_default");
        PERL_RETURN_EMPTY;
    }
    
    result = weechat_config_string_default (script_str2ptr (SvPV (ST (0), PL_na))); /* option */
    
    PERL_RETURN_STRING(result);
}

/*
 * weechat::config_color: return color value of option
 */

XS (XS_weechat_api_config_color)
{
    const char *result;
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PERL_CURRENT_SCRIPT_NAME, "config_color");
	PERL_RETURN_INT(0);
    }
    
    if (items < 1)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PERL_CURRENT_SCRIPT_NAME, "config_color");
        PERL_RETURN_INT(0);
    }
    
    result = weechat_config_color (script_str2ptr (SvPV (ST (0), PL_na))); /* option */
    
    PERL_RETURN_STRING(result);
}

/*
 * weechat::config_color_default: return default color value of option
 */

XS (XS_weechat_api_config_color_default)
{
    const char *result;
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PERL_CURRENT_SCRIPT_NAME, "config_color_default");
	PERL_RETURN_INT(0);
    }
    
    if (items < 1)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PERL_CURRENT_SCRIPT_NAME, "config_color_default");
        PERL_RETURN_INT(0);
    }
    
    result = weechat_config_color_default (script_str2ptr (SvPV (ST (0), PL_na))); /* option */
    
    PERL_RETURN_STRING(result);
}

/*
 * weechat::config_write_option: write an option in configuration file
 */

XS (XS_weechat_api_config_write_option)
{
    char *config_file, *option;
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PERL_CURRENT_SCRIPT_NAME, "config_write_option");
	PERL_RETURN_ERROR;
    }
    
    if (items < 2)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PERL_CURRENT_SCRIPT_NAME, "config_write_option");
        PERL_RETURN_ERROR;
    }
    
    config_file = SvPV (ST (0), PL_na);
    option = SvPV (ST (1), PL_na);
    
    weechat_config_write_option (script_str2ptr (config_file),
                                 script_str2ptr (option));
    
    PERL_RETURN_OK;
}

/*
 * weechat::config_write_line: write a line in configuration file
 */

XS (XS_weechat_api_config_write_line)
{
    char *config_file, *option_name, *value;
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PERL_CURRENT_SCRIPT_NAME, "config_write_line");
	PERL_RETURN_ERROR;
    }
    
    if (items < 3)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PERL_CURRENT_SCRIPT_NAME, "config_write_line");
        PERL_RETURN_ERROR;
    }
    
    config_file = SvPV (ST (0), PL_na);
    option_name = SvPV (ST (1), PL_na);
    value = SvPV (ST (2), PL_na);
    
    weechat_config_write_line (script_str2ptr (config_file), option_name,
                               "%s", value);
    
    PERL_RETURN_OK;
}

/*
 * weechat::config_write: write configuration file
 */

XS (XS_weechat_api_config_write)
{
    int rc;
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PERL_CURRENT_SCRIPT_NAME, "config_write");
	PERL_RETURN_INT(-1);
    }
    
    if (items < 1)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PERL_CURRENT_SCRIPT_NAME, "config_write");
        PERL_RETURN_INT(-1);
    }
    
    rc = weechat_config_write (script_str2ptr (SvPV (ST (0), PL_na))); /* config_file */
    
    PERL_RETURN_INT(rc);
}

/*
 * weechat::config_read: read configuration file
 */

XS (XS_weechat_api_config_read)
{
    int rc;
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PERL_CURRENT_SCRIPT_NAME, "config_read");
	PERL_RETURN_INT(-1);
    }
    
    if (items < 1)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PERL_CURRENT_SCRIPT_NAME, "config_read");
        PERL_RETURN_INT(-1);
    }
    
    rc = weechat_config_read (script_str2ptr (SvPV (ST (0), PL_na))); /* config_file */
    
    PERL_RETURN_INT(rc);
}

/*
 * weechat::config_reload: reload configuration file
 */

XS (XS_weechat_api_config_reload)
{
    int rc;
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PERL_CURRENT_SCRIPT_NAME, "config_reload");
	PERL_RETURN_INT(-1);
    }
    
    if (items < 1)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PERL_CURRENT_SCRIPT_NAME, "config_reload");
        PERL_RETURN_INT(-1);
    }
    
    rc = weechat_config_reload (script_str2ptr (SvPV (ST (0), PL_na))); /* config_file */
    
    PERL_RETURN_INT(rc);
}

/*
 * weechat::config_option_free: free an option in configuration file
 */

XS (XS_weechat_api_config_option_free)
{
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PERL_CURRENT_SCRIPT_NAME, "config_option_free");
	PERL_RETURN_ERROR;
    }
    
    if (items < 1)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PERL_CURRENT_SCRIPT_NAME, "config_option_free");
        PERL_RETURN_ERROR;
    }
    
    script_api_config_option_free (weechat_perl_plugin,
                                   perl_current_script,
                                   script_str2ptr (SvPV (ST (0), PL_na))); /* option */
    
    PERL_RETURN_OK;
}

/*
 * weechat::config_section_free_options: free options of a section in
 *                                       configuration file
 */

XS (XS_weechat_api_config_section_free_options)
{
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PERL_CURRENT_SCRIPT_NAME, "config_section_free_options");
	PERL_RETURN_ERROR;
    }
    
    if (items < 1)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PERL_CURRENT_SCRIPT_NAME, "config_section_free_options");
        PERL_RETURN_ERROR;
    }
    
    script_api_config_section_free_options (weechat_perl_plugin,
                                            perl_current_script,
                                            script_str2ptr (SvPV (ST (0), PL_na))); /* section */
    
    PERL_RETURN_OK;
}

/*
 * weechat::config_section_free: free section in configuration file
 */

XS (XS_weechat_api_config_section_free)
{
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PERL_CURRENT_SCRIPT_NAME, "config_section_free");
	PERL_RETURN_ERROR;
    }
    
    if (items < 1)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PERL_CURRENT_SCRIPT_NAME, "config_section_free");
        PERL_RETURN_ERROR;
    }
    
    script_api_config_section_free (weechat_perl_plugin,
                                    perl_current_script,
                                    script_str2ptr (SvPV (ST (0), PL_na))); /* section */
    
    PERL_RETURN_OK;
}

/*
 * weechat::config_free: free configuration file
 */

XS (XS_weechat_api_config_free)
{
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PERL_CURRENT_SCRIPT_NAME, "config_free");
	PERL_RETURN_ERROR;
    }
    
    if (items < 1)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PERL_CURRENT_SCRIPT_NAME, "config_free");
        PERL_RETURN_ERROR;
    }
    
    script_api_config_free (weechat_perl_plugin,
                            perl_current_script,
                            script_str2ptr (SvPV (ST (0), PL_na))); /* config_file */
    
    PERL_RETURN_OK;
}

/*
 * weechat::config_get: get config option
 */

XS (XS_weechat_api_config_get)
{
    char *result;
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PERL_CURRENT_SCRIPT_NAME, "config_get");
	PERL_RETURN_EMPTY;
    }
    
    if (items < 1)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PERL_CURRENT_SCRIPT_NAME, "config_get");
        PERL_RETURN_EMPTY;
    }
    
    result = script_ptr2str (weechat_config_get (SvPV (ST (0), PL_na)));
    
    PERL_RETURN_STRING_FREE(result);
}

/*
 * weechat::config_get_plugin: get value of a plugin option
 */

XS (XS_weechat_api_config_get_plugin)
{
    const char *result;
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PERL_CURRENT_SCRIPT_NAME, "config_get_plugin");
	PERL_RETURN_EMPTY;
    }
    
    if (items < 1)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PERL_CURRENT_SCRIPT_NAME, "config_get_plugin");
        PERL_RETURN_EMPTY;
    }
    
    result = script_api_config_get_plugin (weechat_perl_plugin,
                                           perl_current_script,
                                           SvPV (ST (0), PL_na));
    
    PERL_RETURN_STRING(result);
}

/*
 * weechat::config_is_set_plugin: check if a plugin option is set
 */

XS (XS_weechat_api_config_is_set_plugin)
{
    char *option;
    int rc;
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PERL_CURRENT_SCRIPT_NAME, "config_is_set_plugin");
        PERL_RETURN_INT(0);
    }
    
    if (items < 1)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PERL_CURRENT_SCRIPT_NAME, "config_is_set_plugin");
        PERL_RETURN_INT(0);
    }
    
    option = SvPV (ST (0), PL_na);
    
    rc = script_api_config_is_set_plugin (weechat_perl_plugin,
                                          perl_current_script,
                                          option);
    
    PERL_RETURN_INT(rc);
}

/*
 * weechat::config_set_plugin: set value of a plugin option
 */

XS (XS_weechat_api_config_set_plugin)
{
    char *option, *value;
    int rc;
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PERL_CURRENT_SCRIPT_NAME, "config_set_plugin");
	PERL_RETURN_INT(WEECHAT_CONFIG_OPTION_SET_ERROR);
    }
    
    if (items < 2)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PERL_CURRENT_SCRIPT_NAME, "config_set_plugin");
        PERL_RETURN_INT(WEECHAT_CONFIG_OPTION_SET_ERROR);
    }
    
    option = SvPV (ST (0), PL_na);
    value = SvPV (ST (1), PL_na);
    
    rc = script_api_config_set_plugin (weechat_perl_plugin,
                                       perl_current_script,
                                       option,
                                       value);
    
    PERL_RETURN_INT(rc);
}

/*
 * weechat::config_unset_plugin: unset a plugin option
 */

XS (XS_weechat_api_config_unset_plugin)
{
    char *option;
    int rc;
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PERL_CURRENT_SCRIPT_NAME, "config_unset_plugin");
	PERL_RETURN_INT(WEECHAT_CONFIG_OPTION_UNSET_ERROR);
    }
    
    if (items < 1)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PERL_CURRENT_SCRIPT_NAME, "config_unset_plugin");
        PERL_RETURN_INT(WEECHAT_CONFIG_OPTION_UNSET_ERROR);
    }
    
    option = SvPV (ST (0), PL_na);
    
    rc = script_api_config_unset_plugin (weechat_perl_plugin,
                                         perl_current_script,
                                         option);

    PERL_RETURN_INT(rc);
}

/*
 * weechat::prefix: get a prefix, used for display
 */

XS (XS_weechat_api_prefix)
{
    const char *result;
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PERL_CURRENT_SCRIPT_NAME, "prefix");
        PERL_RETURN_EMPTY;
    }
    
    if (items < 1)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PERL_CURRENT_SCRIPT_NAME, "prefix");
        PERL_RETURN_EMPTY;
    }
    
    result = weechat_prefix (SvPV (ST (0), PL_na)); /* prefix */
    
    PERL_RETURN_STRING(result);
}

/*
 * weechat::color: get a color code, used for display
 */

XS (XS_weechat_api_color)
{
    const char *result;
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PERL_CURRENT_SCRIPT_NAME, "color");
        PERL_RETURN_EMPTY;
    }
    
    if (items < 1)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PERL_CURRENT_SCRIPT_NAME, "color");
        PERL_RETURN_EMPTY;
    }
    
    result = weechat_color (SvPV (ST (0), PL_na)); /* color */
    
    PERL_RETURN_STRING(result);
}

/*
 * weechat::print: print message in a buffer
 */

XS (XS_weechat_api_print)
{
    char *buffer, *message;
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (items < 2)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PERL_CURRENT_SCRIPT_NAME, "print");
        PERL_RETURN_ERROR;
    }
    
    buffer = SvPV (ST (0), PL_na);
    message = SvPV (ST (1), PL_na);
    
    script_api_printf (weechat_perl_plugin,
                       perl_current_script,
                       script_str2ptr (buffer),
                       "%s", message);
    
    PERL_RETURN_OK;
}

/*
 * weechat::print_date_tags: print message in a buffer with optional date and
 *                           tags
 */

XS (XS_weechat_api_print_date_tags)
{
    char *buffer, *tags, *message;
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PERL_CURRENT_SCRIPT_NAME, "print_date_tags");
        PERL_RETURN_ERROR;
    }
    
    if (items < 4)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PERL_CURRENT_SCRIPT_NAME, "print_date_tags");
        PERL_RETURN_ERROR;
    }
    
    buffer = SvPV (ST (0), PL_na);
    tags = SvPV (ST (2), PL_na);
    message = SvPV (ST (3), PL_na);
    
    script_api_printf_date_tags (weechat_perl_plugin,
                                 perl_current_script,
                                 script_str2ptr (buffer),
                                 SvIV (ST (1)),
                                 tags,
                                 "%s", message);
    
    PERL_RETURN_OK;
}

/*
 * weechat::print_y: print message in a buffer with free content
 */

XS (XS_weechat_api_print_y)
{
    char *buffer, *message;
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PERL_CURRENT_SCRIPT_NAME, "print_y");
        PERL_RETURN_ERROR;
    }
    
    if (items < 3)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PERL_CURRENT_SCRIPT_NAME, "print_y");
        PERL_RETURN_ERROR;
    }
    
    buffer = SvPV (ST (0), PL_na);
    message = SvPV (ST (2), PL_na);
    
    script_api_printf_y (weechat_perl_plugin,
                         perl_current_script,
                         script_str2ptr (buffer),
                         SvIV (ST (1)),
                         "%s", message);
    
    PERL_RETURN_OK;
}

/*
 * weechat::log_print: print message in WeeChat log file
 */

XS (XS_weechat_api_log_print)
{
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;

    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PERL_CURRENT_SCRIPT_NAME, "log_print");
        PERL_RETURN_ERROR;
    }

    if (items < 1)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PERL_CURRENT_SCRIPT_NAME, "log_print");
        PERL_RETURN_ERROR;
    }
    
    script_api_log_printf (weechat_perl_plugin,
                           perl_current_script,
                           "%s", SvPV (ST (0), PL_na)); /* message */
    
    PERL_RETURN_OK;
}

/*
 * weechat_perl_api_hook_command_cb: callback for command hooked
 */

int
weechat_perl_api_hook_command_cb (void *data, struct t_gui_buffer *buffer,
                                  int argc, char **argv, char **argv_eol)
{
    struct t_script_callback *script_callback;
    char *perl_argv[4], empty_arg[1] = { '\0' };
    int *rc, ret;
    
    /* make C compiler happy */
    (void) argv;
    
    script_callback = (struct t_script_callback *)data;
    
    if (script_callback && script_callback->function && script_callback->function[0])
    {
        perl_argv[0] = (script_callback->data) ? script_callback->data : empty_arg;
        perl_argv[1] = script_ptr2str (buffer);
        perl_argv[2] = (argc > 1) ? argv_eol[1] : empty_arg;
        perl_argv[3] = NULL;
        
        rc = (int *) weechat_perl_exec (script_callback->script,
                                        WEECHAT_SCRIPT_EXEC_INT,
                                        script_callback->function,
                                        perl_argv);
        
        if (!rc)
            ret = WEECHAT_RC_ERROR;
        else
        {
            ret = *rc;
            free (rc);
        }
        if (perl_argv[1])
            free (perl_argv[1]);
        
        return ret;
    }
    
    return WEECHAT_RC_ERROR;
}

/*
 * weechat::hook_command: hook a command
 */

XS (XS_weechat_api_hook_command)
{
    char *result, *command, *description, *args, *args_description;
    char *completion, *function, *data;
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PERL_CURRENT_SCRIPT_NAME, "hook_command");
	PERL_RETURN_EMPTY;
    }
    
    if (items < 7)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PERL_CURRENT_SCRIPT_NAME, "hook_command");
        PERL_RETURN_EMPTY;
    }
    
    command = SvPV (ST (0), PL_na);
    description = SvPV (ST (1), PL_na);
    args = SvPV (ST (2), PL_na);
    args_description = SvPV (ST (3), PL_na);
    completion = SvPV (ST (4), PL_na);
    function = SvPV (ST (5), PL_na);
    data = SvPV (ST (6), PL_na);
    
    result = script_ptr2str (script_api_hook_command (weechat_perl_plugin,
                                                      perl_current_script,
                                                      command,
                                                      description,
                                                      args,
                                                      args_description,
                                                      completion,
                                                      &weechat_perl_api_hook_command_cb,
                                                      function,
                                                      data));
    
    PERL_RETURN_STRING_FREE(result);
}

/*
 * weechat_perl_api_hook_command_run_cb: callback for command_run hooked
 */

int
weechat_perl_api_hook_command_run_cb (void *data, struct t_gui_buffer *buffer,
                                      const char *command)
{
    struct t_script_callback *script_callback;
    char *perl_argv[4], empty_arg[1] = { '\0' };
    int *rc, ret;
    
    script_callback = (struct t_script_callback *)data;
    
    if (script_callback && script_callback->function && script_callback->function[0])
    {
        perl_argv[0] = (script_callback->data) ? script_callback->data : empty_arg;
        perl_argv[1] = script_ptr2str (buffer);
        perl_argv[2] = (command) ? (char *)command : empty_arg;
        perl_argv[3] = NULL;
        
        rc = (int *) weechat_perl_exec (script_callback->script,
                                        WEECHAT_SCRIPT_EXEC_INT,
                                        script_callback->function,
                                        perl_argv);
        
        if (!rc)
            ret = WEECHAT_RC_ERROR;
        else
        {
            ret = *rc;
            free (rc);
        }
        if (perl_argv[1])
            free (perl_argv[1]);
        
        return ret;
    }
    
    return WEECHAT_RC_ERROR;
}

/*
 * weechat::hook_command_run: hook a command_run
 */

XS (XS_weechat_api_hook_command_run)
{
    char *result, *command, *function, *data;
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PERL_CURRENT_SCRIPT_NAME, "hook_command_run");
	PERL_RETURN_EMPTY;
    }
    
    if (items < 3)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PERL_CURRENT_SCRIPT_NAME, "hook_command_run");
        PERL_RETURN_EMPTY;
    }
    
    command = SvPV (ST (0), PL_na);
    function = SvPV (ST (1), PL_na);
    data = SvPV (ST (2), PL_na);
    
    result = script_ptr2str (script_api_hook_command_run (weechat_perl_plugin,
                                                          perl_current_script,
                                                          command,
                                                          &weechat_perl_api_hook_command_run_cb,
                                                          function,
                                                          data));
    
    PERL_RETURN_STRING_FREE(result);
}

/*
 * weechat_perl_api_hook_timer_cb: callback for timer hooked
 */

int
weechat_perl_api_hook_timer_cb (void *data, int remaining_calls)
{
    struct t_script_callback *script_callback;
    char *perl_argv[3], str_remaining_calls[32], empty_arg[1] = { '\0' };
    int *rc, ret;
    
    script_callback = (struct t_script_callback *)data;

    if (script_callback && script_callback->function && script_callback->function[0])
    {
        snprintf (str_remaining_calls, sizeof (str_remaining_calls),
                  "%d", remaining_calls);
        
        perl_argv[0] = (script_callback->data) ? script_callback->data : empty_arg;
        perl_argv[1] = str_remaining_calls;
        perl_argv[2] = NULL;
        
        rc = (int *) weechat_perl_exec (script_callback->script,
                                        WEECHAT_SCRIPT_EXEC_INT,
                                        script_callback->function,
                                        perl_argv);
        
        if (!rc)
            ret = WEECHAT_RC_ERROR;
        else
        {
            ret = *rc;
            free (rc);
        }
        
        return ret;
    }
    
    return WEECHAT_RC_ERROR;
}

/*
 * weechat::hook_timer: hook a timer
 */

XS (XS_weechat_api_hook_timer)
{
    char *result;
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PERL_CURRENT_SCRIPT_NAME, "hook_timer");
	PERL_RETURN_EMPTY;
    }
    
    if (items < 5)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PERL_CURRENT_SCRIPT_NAME, "hook_timer");
        PERL_RETURN_EMPTY;
    }
    
    result = script_ptr2str (script_api_hook_timer (weechat_perl_plugin,
                                                    perl_current_script,
                                                    SvIV (ST (0)), /* interval */
                                                    SvIV (ST (1)), /* align_second */
                                                    SvIV (ST (2)), /* max_calls */
                                                    &weechat_perl_api_hook_timer_cb,
                                                    SvPV (ST (3), PL_na), /* perl function */
                                                    SvPV (ST (4), PL_na))); /* data */
    
    PERL_RETURN_STRING_FREE(result);
}

/*
 * weechat_perl_api_hook_fd_cb: callback for fd hooked
 */

int
weechat_perl_api_hook_fd_cb (void *data, int fd)
{
    struct t_script_callback *script_callback;
    char *perl_argv[3], str_fd[32], empty_arg[1] = { '\0' };
    int *rc, ret;
    
    script_callback = (struct t_script_callback *)data;
    
    if (script_callback && script_callback->function && script_callback->function[0])
    {
        snprintf (str_fd, sizeof (str_fd), "%d", fd);
        
        perl_argv[0] = (script_callback->data) ? script_callback->data : empty_arg;
        perl_argv[1] = str_fd;
        perl_argv[2] = NULL;
        
        rc = (int *) weechat_perl_exec (script_callback->script,
                                        WEECHAT_SCRIPT_EXEC_INT,
                                        script_callback->function,
                                        perl_argv);
        
        if (!rc)
            ret = WEECHAT_RC_ERROR;
        else
        {
            ret = *rc;
            free (rc);
        }
        
        return ret;
    }
    
    return WEECHAT_RC_ERROR;
}

/*
 * weechat::hook_fd: hook a fd
 */

XS (XS_weechat_api_hook_fd)
{
    char *result;
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PERL_CURRENT_SCRIPT_NAME, "hook_fd");
	PERL_RETURN_EMPTY;
    }
    
    if (items < 6)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PERL_CURRENT_SCRIPT_NAME, "hook_fd");
        PERL_RETURN_EMPTY;
    }
    
    result = script_ptr2str (script_api_hook_fd (weechat_perl_plugin,
                                                 perl_current_script,
                                                 SvIV (ST (0)), /* fd */
                                                 SvIV (ST (1)), /* read */
                                                 SvIV (ST (2)), /* write */
                                                 SvIV (ST (3)), /* exception */
                                                 &weechat_perl_api_hook_fd_cb,
                                                 SvPV (ST (4), PL_na), /* perl function */
                                                 SvPV (ST (5), PL_na))); /* data */
    
    PERL_RETURN_STRING_FREE(result);
}

/*
 * weechat_perl_api_hook_process_cb: callback for process hooked
 */

int
weechat_perl_api_hook_process_cb (void *data,
                                  const char *command, int return_code,
                                  const char *out, const char *err)
{
    struct t_script_callback *script_callback;
    char *perl_argv[6], str_rc[32], empty_arg[1] = { '\0' };
    int *rc, ret;
    
    script_callback = (struct t_script_callback *)data;
    
    if (script_callback && script_callback->function && script_callback->function[0])
    {
        snprintf (str_rc, sizeof (str_rc), "%d", return_code);
        
        perl_argv[0] = (script_callback->data) ? script_callback->data : empty_arg;
        perl_argv[1] = (command) ? (char *)command : empty_arg;
        perl_argv[2] = str_rc;
        perl_argv[3] = (out) ? (char *)out : empty_arg;
        perl_argv[4] = (err) ? (char *)err : empty_arg;
        perl_argv[5] = NULL;
        
        rc = (int *) weechat_perl_exec (script_callback->script,
                                        WEECHAT_SCRIPT_EXEC_INT,
                                        script_callback->function,
                                        perl_argv);
        
        if (!rc)
            ret = WEECHAT_RC_ERROR;
        else
        {
            ret = *rc;
            free (rc);
        }
        
        return ret;
    }
    
    return WEECHAT_RC_ERROR;
}

/*
 * weechat::hook_process: hook a process
 */

XS (XS_weechat_api_hook_process)
{
    char *command, *function, *data, *result;
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PERL_CURRENT_SCRIPT_NAME, "hook_process");
        PERL_RETURN_EMPTY;
    }
    
    if (items < 4)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PERL_CURRENT_SCRIPT_NAME, "hook_process");
        PERL_RETURN_EMPTY;
    }
    
    command = SvPV (ST (0), PL_na);
    function = SvPV (ST (2), PL_na);
    data = SvPV (ST (3), PL_na);
    
    result = script_ptr2str (script_api_hook_process (weechat_perl_plugin,
                                                      perl_current_script,
                                                      command,
                                                      SvIV (ST (1)), /* timeout */
                                                      &weechat_perl_api_hook_process_cb,
                                                      function,
                                                      data));
    
    PERL_RETURN_STRING_FREE(result);
}

/*
 * weechat_perl_api_hook_connect_cb: callback for connect hooked
 */

int
weechat_perl_api_hook_connect_cb (void *data, int status,
                                  const char *error, const char *ip_address)
{
    struct t_script_callback *script_callback;
    char *perl_argv[5], str_status[32], empty_arg[1] = { '\0' };
    int *rc, ret;
    
    script_callback = (struct t_script_callback *)data;

    if (script_callback && script_callback->function && script_callback->function[0])
    {
        snprintf (str_status, sizeof (str_status), "%d", status);
        
        perl_argv[0] = (script_callback->data) ? script_callback->data : empty_arg;
        perl_argv[1] = str_status;
        perl_argv[2] = (ip_address) ? (char *)ip_address : empty_arg;
        perl_argv[3] = (error) ? (char *)error : empty_arg;
        perl_argv[4] = NULL;
        
        rc = (int *) weechat_perl_exec (script_callback->script,
                                        WEECHAT_SCRIPT_EXEC_INT,
                                        script_callback->function,
                                        perl_argv);
        
        if (!rc)
            ret = WEECHAT_RC_ERROR;
        else
        {
            ret = *rc;
            free (rc);
        }
        
        return ret;
    }
    
    return WEECHAT_RC_ERROR;
}

/*
 * weechat::hook_connect: hook a connection
 */

XS (XS_weechat_api_hook_connect)
{
    char *proxy, *address, *local_hostname, *function, *data, *result;
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PERL_CURRENT_SCRIPT_NAME, "hook_connect");
        PERL_RETURN_EMPTY;
    }
    
    if (items < 8)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PERL_CURRENT_SCRIPT_NAME, "hook_connect");
        PERL_RETURN_EMPTY;
    }
    
    proxy = SvPV (ST (0), PL_na);
    address = SvPV (ST (1), PL_na);
    local_hostname = SvPV (ST (5), PL_na);
    function = SvPV (ST (6), PL_na);
    data = SvPV (ST (7), PL_na);
    
    result = script_ptr2str (script_api_hook_connect (weechat_perl_plugin,
                                                      perl_current_script,
                                                      proxy,
                                                      address,
                                                      SvIV (ST (2)), /* port */
                                                      SvIV (ST (3)), /* sock */
                                                      SvIV (ST (4)), /* ipv6 */
                                                      NULL, /* gnutls session */
                                                      local_hostname,
                                                      &weechat_perl_api_hook_connect_cb,
                                                      function,
                                                      data));
    
    PERL_RETURN_STRING_FREE(result);
}

/*
 * weechat_perl_api_hook_print_cb: callback for print hooked
 */

int
weechat_perl_api_hook_print_cb (void *data, struct t_gui_buffer *buffer,
                                time_t date,
                                int tags_count, const char **tags,
                                int displayed, int highlight,
                                const char *prefix, const char *message)
{
    struct t_script_callback *script_callback;
    char *perl_argv[9], empty_arg[1] = { '\0' };
    static char timebuffer[64];
    int *rc, ret;
    
    /* make C compiler happy */
    (void) tags_count;
    
    script_callback = (struct t_script_callback *)data;

    if (script_callback && script_callback->function && script_callback->function[0])
    {
        snprintf (timebuffer, sizeof (timebuffer) - 1, "%ld", (long int)date);
        
        perl_argv[0] = (script_callback->data) ? script_callback->data : empty_arg;
        perl_argv[1] = script_ptr2str (buffer);
        perl_argv[2] = timebuffer;
        perl_argv[3] = weechat_string_build_with_split_string (tags, ",");
        if (!perl_argv[3])
            perl_argv[3] = strdup ("");
        perl_argv[4] = (displayed) ? strdup ("1") : strdup ("0");
        perl_argv[5] = (highlight) ? strdup ("1") : strdup ("0");
        perl_argv[6] = (prefix) ? (char *)prefix : empty_arg;
        perl_argv[7] = (message) ? (char *)message : empty_arg;
        perl_argv[8] = NULL;
        
        rc = (int *) weechat_perl_exec (script_callback->script,
                                        WEECHAT_SCRIPT_EXEC_INT,
                                        script_callback->function,
                                        perl_argv);
        
        if (!rc)
            ret = WEECHAT_RC_ERROR;
        else
        {
            ret = *rc;
            free (rc);
        }
        if (perl_argv[1])
            free (perl_argv[1]);
        if (perl_argv[3])
            free (perl_argv[3]);
        if (perl_argv[4])
            free (perl_argv[4]);
        if (perl_argv[5])
            free (perl_argv[5]);
        
        return ret;
    }
    
    return WEECHAT_RC_ERROR;
}

/*
 * weechat::hook_print: hook a print
 */

XS (XS_weechat_api_hook_print)
{
    char *result, *buffer, *tags, *message, *function, *data;
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PERL_CURRENT_SCRIPT_NAME, "hook_print");
	PERL_RETURN_EMPTY;
    }
    
    if (items < 6)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PERL_CURRENT_SCRIPT_NAME, "hook_print");
        PERL_RETURN_EMPTY;
    }
 
    buffer = SvPV (ST (0), PL_na);
    tags = SvPV (ST (1), PL_na);
    message = SvPV (ST (2), PL_na);
    function = SvPV (ST (4), PL_na);
    data = SvPV (ST (5), PL_na);
    
    result = script_ptr2str (script_api_hook_print (weechat_perl_plugin,
                                                    perl_current_script,
                                                    script_str2ptr (buffer),
                                                    tags,
                                                    message,
                                                    SvIV (ST (3)), /* strip_colors */
                                                    &weechat_perl_api_hook_print_cb,
                                                    function,
                                                    data));
    
    PERL_RETURN_STRING_FREE(result);
}

/*
 * weechat_perl_api_hook_signal_cb: callback for signal hooked
 */

int
weechat_perl_api_hook_signal_cb (void *data, const char *signal, const char *type_data,
                                 void *signal_data)
{
    struct t_script_callback *script_callback;
    char *perl_argv[4], empty_arg[1] = { '\0' };
    static char value_str[64];
    int *rc, ret, free_needed;
    
    script_callback = (struct t_script_callback *)data;
    
    if (script_callback && script_callback->function && script_callback->function[0])
    {
        perl_argv[0] = (script_callback->data) ? script_callback->data : empty_arg;
        perl_argv[1] = (signal) ? (char *)signal : empty_arg;
        free_needed = 0;
        if (strcmp (type_data, WEECHAT_HOOK_SIGNAL_STRING) == 0)
        {
            perl_argv[2] = (signal_data) ? (char *)signal_data : empty_arg;
        }
        else if (strcmp (type_data, WEECHAT_HOOK_SIGNAL_INT) == 0)
        {
            snprintf (value_str, sizeof (value_str) - 1,
                      "%d", *((int *)signal_data));
            perl_argv[2] = value_str;
        }
        else if (strcmp (type_data, WEECHAT_HOOK_SIGNAL_POINTER) == 0)
        {
            perl_argv[2] = script_ptr2str (signal_data);
            free_needed = 1;
        }
        else
            perl_argv[2] = empty_arg;
        perl_argv[3] = NULL;
        
        rc = (int *) weechat_perl_exec (script_callback->script,
                                        WEECHAT_SCRIPT_EXEC_INT,
                                        script_callback->function,
                                        perl_argv);
        
        if (!rc)
            ret = WEECHAT_RC_ERROR;
        else
        {
            ret = *rc;
            free (rc);
        }
        if (free_needed && perl_argv[2])
            free (perl_argv[2]);
        
        return ret;
    }
    
    return WEECHAT_RC_ERROR;
}

/*
 * weechat::hook_signal: hook a signal
 */

XS (XS_weechat_api_hook_signal)
{
    char *result, *signal, *function, *data;
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PERL_CURRENT_SCRIPT_NAME, "hook_signal");
	PERL_RETURN_EMPTY;
    }
    
    if (items < 3)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PERL_CURRENT_SCRIPT_NAME, "hook_signal");
        PERL_RETURN_EMPTY;
    }
    
    signal = SvPV (ST (0), PL_na);
    function = SvPV (ST (1), PL_na);
    data = SvPV (ST (2), PL_na);
    
    result = script_ptr2str (script_api_hook_signal (weechat_perl_plugin,
                                                     perl_current_script,
                                                     signal,
                                                     &weechat_perl_api_hook_signal_cb,
                                                     function,
                                                     data));
    
    PERL_RETURN_STRING_FREE(result);
}

/*
 * weechat::hook_signal_send: send a signal
 */

XS (XS_weechat_api_hook_signal_send)
{
    char *signal, *type_data;
    int number;
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PERL_CURRENT_SCRIPT_NAME, "hook_signal_send");
	PERL_RETURN_ERROR;
    }
    
    if (items < 3)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PERL_CURRENT_SCRIPT_NAME, "hook_signal_send");
        PERL_RETURN_ERROR;
    }
    
    signal = SvPV (ST (0), PL_na);
    type_data = SvPV (ST (1), PL_na);
    if (strcmp (type_data, WEECHAT_HOOK_SIGNAL_STRING) == 0)
    {
        weechat_hook_signal_send (signal,
                                  type_data,
                                  SvPV (ST (2), PL_na)); /* signal_data */
        PERL_RETURN_OK;
    }
    else if (strcmp (type_data, WEECHAT_HOOK_SIGNAL_INT) == 0)
    {
        number = SvIV(ST (2));
        weechat_hook_signal_send (signal,
                                  type_data,
                                  &number); /* signal_data */
        PERL_RETURN_OK;
    }
    else if (strcmp (type_data, WEECHAT_HOOK_SIGNAL_POINTER) == 0)
    {
        weechat_hook_signal_send (signal,
                                  type_data,
                                  script_str2ptr (SvPV (ST (2), PL_na))); /* signal_data */
        PERL_RETURN_OK;
    }
    
    PERL_RETURN_ERROR;
}

/*
 * weechat_perl_api_hook_config_cb: callback for config option hooked
 */

int
weechat_perl_api_hook_config_cb (void *data, const char *option, const char *value)
{
    struct t_script_callback *script_callback;
    char *perl_argv[4], empty_arg[1] = { '\0' };
    int *rc, ret;
    
    script_callback = (struct t_script_callback *)data;
    
    if (script_callback && script_callback->function && script_callback->function[0])
    {
        perl_argv[0] = (script_callback->data) ? script_callback->data : empty_arg;
        perl_argv[1] = (option) ? (char *)option : empty_arg;
        perl_argv[2] = (value) ? (char *)value : empty_arg;
        perl_argv[3] = NULL;
        
        rc = (int *) weechat_perl_exec (script_callback->script,
                                        WEECHAT_SCRIPT_EXEC_INT,
                                        script_callback->function,
                                        perl_argv);
        
        if (!rc)
            ret = WEECHAT_RC_ERROR;
        else
        {
            ret = *rc;
            free (rc);
        }
        
        return ret;
    }
    
    return WEECHAT_RC_ERROR;
}

/*
 * weechat::hook_config: hook a config option
 */

XS (XS_weechat_api_hook_config)
{
    char *result, *option, *function, *data;
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PERL_CURRENT_SCRIPT_NAME, "hook_config");
	PERL_RETURN_EMPTY;
    }
    
    if (items < 3)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PERL_CURRENT_SCRIPT_NAME, "hook_config");
        PERL_RETURN_EMPTY;
    }
    
    option = SvPV (ST (0), PL_na);
    function = SvPV (ST (1), PL_na);
    data = SvPV (ST (2), PL_na);
    
    result = script_ptr2str (script_api_hook_config (weechat_perl_plugin,
                                                     perl_current_script,
                                                     option,
                                                     &weechat_perl_api_hook_config_cb,
                                                     function,
                                                     data));
    
    PERL_RETURN_STRING_FREE(result);
}

/*
 * weechat_perl_api_hook_completion_cb: callback for completion hooked
 */

int
weechat_perl_api_hook_completion_cb (void *data, const char *completion_item,
                                     struct t_gui_buffer *buffer,
                                     struct t_gui_completion *completion)
{
    struct t_script_callback *script_callback;
    char *perl_argv[5], empty_arg[1] = { '\0' };
    int *rc, ret;
    
    script_callback = (struct t_script_callback *)data;
    
    if (script_callback && script_callback->function && script_callback->function[0])
    {
        perl_argv[0] = (script_callback->data) ? script_callback->data : empty_arg;
        perl_argv[1] = (completion_item) ? (char *)completion_item : empty_arg;
        perl_argv[2] = script_ptr2str (buffer);
        perl_argv[3] = script_ptr2str (completion);
        perl_argv[4] = NULL;
        
        rc = (int *) weechat_perl_exec (script_callback->script,
                                        WEECHAT_SCRIPT_EXEC_INT,
                                        script_callback->function,
                                        perl_argv);
        
        if (!rc)
            ret = WEECHAT_RC_ERROR;
        else
        {
            ret = *rc;
            free (rc);
        }
        if (perl_argv[2])
            free (perl_argv[2]);
        if (perl_argv[3])
            free (perl_argv[3]);
        
        return ret;
    }
    
    return WEECHAT_RC_ERROR;
}

/*
 * weechat::hook_completion: hook a completion
 */

XS (XS_weechat_api_hook_completion)
{
    char *result, *completion, *description, *function, *data;
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PERL_CURRENT_SCRIPT_NAME, "hook_completion");
	PERL_RETURN_EMPTY;
    }
    
    if (items < 4)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PERL_CURRENT_SCRIPT_NAME, "hook_completion");
        PERL_RETURN_EMPTY;
    }
    
    completion = SvPV (ST (0), PL_na);
    description = SvPV (ST (1), PL_na);
    function = SvPV (ST (2), PL_na);
    data = SvPV (ST (3), PL_na);
    
    result = script_ptr2str (script_api_hook_completion (weechat_perl_plugin,
                                                         perl_current_script,
                                                         completion,
                                                         description,
                                                         &weechat_perl_api_hook_completion_cb,
                                                         function,
                                                         data));
    
    PERL_RETURN_STRING_FREE(result);
}

/*
 * weechat::hook_completion_list_add: add a word to list for a completion
 */

XS (XS_weechat_api_hook_completion_list_add)
{
    char *completion, *word, *where;
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PERL_CURRENT_SCRIPT_NAME, "hook_completion_list_add");
	PERL_RETURN_ERROR;
    }
    
    if (items < 4)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PERL_CURRENT_SCRIPT_NAME, "hook_completion_list_add");
        PERL_RETURN_ERROR;
    }
    
    completion = SvPV (ST (0), PL_na);
    word = SvPV (ST (1), PL_na);
    where = SvPV (ST (3), PL_na);
    
    weechat_hook_completion_list_add (script_str2ptr (completion),
                                      word,
                                      SvIV (ST (2)), /* nick_completion */
                                      where);
    
    PERL_RETURN_OK;
}

/*
 * weechat_perl_api_hook_modifier_cb: callback for modifier hooked
 */

char *
weechat_perl_api_hook_modifier_cb (void *data, const char *modifier,
                                   const char *modifier_data, const char *string)
{
    struct t_script_callback *script_callback;
    char *perl_argv[5], empty_arg[1] = { '\0' };
    
    script_callback = (struct t_script_callback *)data;
    
    if (script_callback && script_callback->function && script_callback->function[0])
    {
        perl_argv[0] = (script_callback->data) ? script_callback->data : empty_arg;
        perl_argv[1] = (modifier) ? (char *)modifier : empty_arg;
        perl_argv[2] = (modifier_data) ? (char *)modifier_data : empty_arg;
        perl_argv[3] = (string) ? (char *)string : empty_arg;
        perl_argv[4] = NULL;
        
        return (char *)weechat_perl_exec (script_callback->script,
                                          WEECHAT_SCRIPT_EXEC_STRING,
                                          script_callback->function,
                                          perl_argv);
    }
    
    return NULL;
}

/*
 * weechat::hook_modifier: hook a modifier
 */

XS (XS_weechat_api_hook_modifier)
{
    char *result, *modifier, *function, *data;
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PERL_CURRENT_SCRIPT_NAME, "hook_modifier");
	PERL_RETURN_EMPTY;
    }
    
    if (items < 3)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PERL_CURRENT_SCRIPT_NAME, "hook_modifier");
        PERL_RETURN_EMPTY;
    }
    
    modifier = SvPV (ST (0), PL_na);
    function = SvPV (ST (1), PL_na);
    data = SvPV (ST (2), PL_na);
    
    result = script_ptr2str (script_api_hook_modifier (weechat_perl_plugin,
                                                       perl_current_script,
                                                       modifier,
                                                       &weechat_perl_api_hook_modifier_cb,
                                                       function,
                                                       data));
    
    PERL_RETURN_STRING_FREE(result);
}

/*
 * weechat::hook_modifier_exec: execute a modifier hook
 */

XS (XS_weechat_api_hook_modifier_exec)
{
    char *result, *modifier, *modifier_data, *string;
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PERL_CURRENT_SCRIPT_NAME, "hook_modifier_exec");
	PERL_RETURN_EMPTY;
    }
    
    if (items < 3)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PERL_CURRENT_SCRIPT_NAME, "hook_modifier_exec");
        PERL_RETURN_EMPTY;
    }
    
    modifier = SvPV (ST (0), PL_na);
    modifier_data = SvPV (ST (1), PL_na);
    string = SvPV (ST (2), PL_na);
    
    result = weechat_hook_modifier_exec (modifier, modifier_data, string);
    
    PERL_RETURN_STRING_FREE(result);
}

/*
 * weechat_perl_api_hook_info_cb: callback for info hooked
 */

const char *
weechat_perl_api_hook_info_cb (void *data, const char *info_name,
                               const char *arguments)
{
    struct t_script_callback *script_callback;
    char *perl_argv[4], empty_arg[1] = { '\0' };
    
    script_callback = (struct t_script_callback *)data;
    
    if (script_callback && script_callback->function && script_callback->function[0])
    {
        perl_argv[0] = (script_callback->data) ? script_callback->data : empty_arg;
        perl_argv[1] = (info_name) ? (char *)info_name : empty_arg;
        perl_argv[2] = (arguments) ? (char *)arguments : empty_arg;
        perl_argv[3] = NULL;
        
        return (const char *)weechat_perl_exec (script_callback->script,
                                                WEECHAT_SCRIPT_EXEC_STRING,
                                                script_callback->function,
                                                perl_argv);
    }
    
    return NULL;
}

/*
 * weechat::hook_info: hook an info
 */

XS (XS_weechat_api_hook_info)
{
    char *result, *info_name, *description, *function, *data;
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PERL_CURRENT_SCRIPT_NAME, "hook_info");
	PERL_RETURN_EMPTY;
    }
    
    if (items < 4)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PERL_CURRENT_SCRIPT_NAME, "hook_info");
        PERL_RETURN_EMPTY;
    }
    
    info_name = SvPV (ST (0), PL_na);
    description = SvPV (ST (1), PL_na);
    function = SvPV (ST (2), PL_na);
    data = SvPV (ST (3), PL_na);
    
    result = script_ptr2str (script_api_hook_info (weechat_perl_plugin,
                                                   perl_current_script,
                                                   info_name,
                                                   description,
                                                   &weechat_perl_api_hook_info_cb,
                                                   function,
                                                   data));
    
    PERL_RETURN_STRING_FREE(result);
}

/*
 * weechat_perl_api_hook_infolist_cb: callback for infolist hooked
 */

struct t_infolist *
weechat_perl_api_hook_infolist_cb (void *data, const char *infolist_name,
                                   void *pointer, const char *arguments)
{
    struct t_script_callback *script_callback;
    char *perl_argv[5], empty_arg[1] = { '\0' };
    struct t_infolist *result;
    
    script_callback = (struct t_script_callback *)data;
    
    if (script_callback && script_callback->function && script_callback->function[0])
    {
        perl_argv[0] = (script_callback->data) ? script_callback->data : empty_arg;
        perl_argv[1] = (infolist_name) ? (char *)infolist_name : empty_arg;
        perl_argv[2] = script_ptr2str (pointer);
        perl_argv[3] = (arguments) ? (char *)arguments : empty_arg;
        perl_argv[4] = NULL;
        
        result = (struct t_infolist *)weechat_perl_exec (script_callback->script,
                                                         WEECHAT_SCRIPT_EXEC_STRING,
                                                         script_callback->function,
                                                         perl_argv);
        
        if (perl_argv[2])
            free (perl_argv[2]);
        
        return result;
    }
    
    return NULL;
}

/*
 * weechat::hook_infolist: hook an infolist
 */

XS (XS_weechat_api_hook_infolist)
{
    char *result, *infolist_name, *description, *function, *data;
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PERL_CURRENT_SCRIPT_NAME, "hook_infolist");
	PERL_RETURN_EMPTY;
    }
    
    if (items < 4)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PERL_CURRENT_SCRIPT_NAME, "hook_infolist");
        PERL_RETURN_EMPTY;
    }
    
    infolist_name = SvPV (ST (0), PL_na);
    description = SvPV (ST (1), PL_na);
    function = SvPV (ST (2), PL_na);
    data = SvPV (ST (3), PL_na);
    
    result = script_ptr2str (script_api_hook_infolist (weechat_perl_plugin,
                                                       perl_current_script,
                                                       infolist_name,
                                                       description,
                                                       &weechat_perl_api_hook_infolist_cb,
                                                       function,
                                                       data));
    
    PERL_RETURN_STRING_FREE(result);
}

/*
 * weechat::unhook: unhook something
 */

XS (XS_weechat_api_unhook)
{
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PERL_CURRENT_SCRIPT_NAME, "unhook");
	PERL_RETURN_ERROR;
    }
    
    if (items < 1)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PERL_CURRENT_SCRIPT_NAME, "unhook");
        PERL_RETURN_ERROR;
    }
    
    script_api_unhook (weechat_perl_plugin,
                       perl_current_script,
                       script_str2ptr (SvPV (ST (0), PL_na))); /* hook */
    
    PERL_RETURN_OK;
}

/*
 * weechat::unhook_all: unhook all for script
 */

XS (XS_weechat_api_unhook_all)
{
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    (void) items;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PERL_CURRENT_SCRIPT_NAME, "unhook_all");
	PERL_RETURN_ERROR;
    }
    
    script_api_unhook_all (perl_current_script);
    
    PERL_RETURN_OK;
}

/*
 * weechat_perl_api_buffer_input_data_cb: callback for input data in a buffer
 */

int
weechat_perl_api_buffer_input_data_cb (void *data, struct t_gui_buffer *buffer,
                                       const char *input_data)
{
    struct t_script_callback *script_callback;
    char *perl_argv[4], empty_arg[1] = { '\0' };
    int *rc, ret;
    
    script_callback = (struct t_script_callback *)data;
    
    if (script_callback && script_callback->function && script_callback->function[0])
    {
        perl_argv[0] = (script_callback->data) ? script_callback->data : empty_arg;
        perl_argv[1] = script_ptr2str (buffer);
        perl_argv[2] = (input_data) ? (char *)input_data : empty_arg;
        perl_argv[3] = NULL;
        
        rc = (int *) weechat_perl_exec (script_callback->script,
                                        WEECHAT_SCRIPT_EXEC_INT,
                                        script_callback->function,
                                        perl_argv);
        if (!rc)
            ret = WEECHAT_RC_ERROR;
        else
        {
            ret = *rc;
            free (rc);
        }
        if (perl_argv[1])
            free (perl_argv[1]);
        
        return ret;
    }
    
    return WEECHAT_RC_ERROR;
}

/*
 * weechat_perl_api_buffer_close_cb: callback for buffer closed
 */

int
weechat_perl_api_buffer_close_cb (void *data, struct t_gui_buffer *buffer)
{
    struct t_script_callback *script_callback;
    char *perl_argv[3], empty_arg[1] = { '\0' };
    int *rc, ret;
    
    script_callback = (struct t_script_callback *)data;

    if (script_callback && script_callback->function && script_callback->function[0])
    {
        perl_argv[0] = (script_callback->data) ? script_callback->data : empty_arg;
        perl_argv[1] = script_ptr2str (buffer);
        perl_argv[2] = NULL;
        
        rc = (int *) weechat_perl_exec (script_callback->script,
                                        WEECHAT_SCRIPT_EXEC_INT,
                                        script_callback->function,
                                        perl_argv);
        if (!rc)
            ret = WEECHAT_RC_ERROR;
        else
        {
            ret = *rc;
            free (rc);
        }
        if (perl_argv[1])
            free (perl_argv[1]);
        
        return ret;
    }
    
    return WEECHAT_RC_ERROR;
}

/*
 * weechat::buffer_new: create a new buffer
 */

XS (XS_weechat_api_buffer_new)
{
    char *result, *name, *function_input, *data_input, *function_close;
    char *data_close;
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PERL_CURRENT_SCRIPT_NAME, "buffer_new");
        PERL_RETURN_EMPTY;
    }
    
    if (items < 5)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PERL_CURRENT_SCRIPT_NAME, "buffer_new");
        PERL_RETURN_EMPTY;
    }
    
    name = SvPV (ST (0), PL_na);
    function_input = SvPV (ST (1), PL_na);
    data_input = SvPV (ST (2), PL_na);
    function_close = SvPV (ST (3), PL_na);
    data_close = SvPV (ST (4), PL_na);
    
    result = script_ptr2str (script_api_buffer_new (weechat_perl_plugin,
                                                    perl_current_script,
                                                    name,
                                                    &weechat_perl_api_buffer_input_data_cb,
                                                    function_input,
                                                    data_input,
                                                    &weechat_perl_api_buffer_close_cb,
                                                    function_close,
                                                    data_close));
    
    PERL_RETURN_STRING_FREE(result);
}

/*
 * weechat::buffer_search: search a buffer
 */

XS (XS_weechat_api_buffer_search)
{
    char *result, *plugin, *name;
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PERL_CURRENT_SCRIPT_NAME, "buffer_search");
	PERL_RETURN_EMPTY;
    }
    
    if (items < 2)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PERL_CURRENT_SCRIPT_NAME, "buffer_search");
        PERL_RETURN_EMPTY;
    }
    
    plugin = SvPV (ST (0), PL_na);
    name = SvPV (ST (1), PL_na);
    
    result = script_ptr2str (weechat_buffer_search (plugin, name));
    
    PERL_RETURN_STRING_FREE(result);
}

/*
 * weechat::buffer_search_main: search main buffer (WeeChat core buffer)
 */

XS (XS_weechat_api_buffer_search_main)
{
    char *result;
    dXSARGS;
    
    /* make C compiler happy */
    (void) items;
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PERL_CURRENT_SCRIPT_NAME, "buffer_search_main");
	PERL_RETURN_EMPTY;
    }
    
    result = script_ptr2str (weechat_buffer_search_main ());
    
    PERL_RETURN_STRING_FREE(result);
}

/*
 * weechat::current_buffer: get current buffer
 */

XS (XS_weechat_api_current_buffer)
{
    char *result;
    dXSARGS;
    
    /* make C compiler happy */
    (void) items;
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PERL_CURRENT_SCRIPT_NAME, "current_buffer");
	PERL_RETURN_EMPTY;
    }
    
    result = script_ptr2str (weechat_current_buffer ());
    
    PERL_RETURN_STRING_FREE(result);
}

/*
 * weechat::buffer_clear: clear a buffer
 */

XS (XS_weechat_api_buffer_clear)
{
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PERL_CURRENT_SCRIPT_NAME, "buffer_clear");
	PERL_RETURN_ERROR;
    }
    
    if (items < 1)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PERL_CURRENT_SCRIPT_NAME, "buffer_clear");
        PERL_RETURN_ERROR;
    }
    
    weechat_buffer_clear (script_str2ptr (SvPV (ST (0), PL_na))); /* buffer */
    
    PERL_RETURN_OK;
}

/*
 * weechat::buffer_close: close a buffer
 */

XS (XS_weechat_api_buffer_close)
{
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PERL_CURRENT_SCRIPT_NAME, "buffer_close");
	PERL_RETURN_ERROR;
    }
    
    if (items < 1)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PERL_CURRENT_SCRIPT_NAME, "buffer_close");
        PERL_RETURN_ERROR;
    }
    
    script_api_buffer_close (weechat_perl_plugin,
                             perl_current_script,
                             script_str2ptr (SvPV (ST (0), PL_na))); /* buffer */
    
    PERL_RETURN_OK;
}

/*
 * weechat::buffer_merge: merge a buffer to another buffer
 */

XS (XS_weechat_api_buffer_merge)
{
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PERL_CURRENT_SCRIPT_NAME, "buffer_merge");
	PERL_RETURN_ERROR;
    }
    
    if (items < 2)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PERL_CURRENT_SCRIPT_NAME, "buffer_merge");
        PERL_RETURN_ERROR;
    }
    
    weechat_buffer_merge (script_str2ptr (SvPV (ST (0), PL_na)), /* buffer */
                          script_str2ptr (SvPV (ST (1), PL_na))); /* target_buffer */
    
    PERL_RETURN_OK;
}

/*
 * weechat::buffer_unmerge: unmerge a buffer from group of merged buffers
 */

XS (XS_weechat_api_buffer_unmerge)
{
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PERL_CURRENT_SCRIPT_NAME, "buffer_unmerge");
	PERL_RETURN_ERROR;
    }
    
    if (items < 2)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PERL_CURRENT_SCRIPT_NAME, "buffer_unmerge");
        PERL_RETURN_ERROR;
    }
    
    weechat_buffer_unmerge (script_str2ptr (SvPV (ST (0), PL_na)), /* buffer */
                            SvIV (ST (1))); /* number */
    
    PERL_RETURN_OK;
}

/*
 * weechat::buffer_get_integer: get a buffer property as integer
 */

XS (XS_weechat_api_buffer_get_integer)
{
    char *buffer, *property;
    int value;
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PERL_CURRENT_SCRIPT_NAME, "buffer_get_integer");
	PERL_RETURN_INT(-1);
    }
    
    if (items < 2)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PERL_CURRENT_SCRIPT_NAME, "buffer_get_integer");
        PERL_RETURN_INT(-1);
    }
    
    buffer = SvPV (ST (0), PL_na);
    property = SvPV (ST (1), PL_na);
    
    value = weechat_buffer_get_integer (script_str2ptr (buffer), property);
    
    PERL_RETURN_INT(value);
}

/*
 * weechat::buffer_get_string: get a buffer property as string
 */

XS (XS_weechat_api_buffer_get_string)
{
    char *buffer, *property;
    const char *result;
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PERL_CURRENT_SCRIPT_NAME, "buffer_get_string");
	PERL_RETURN_EMPTY;
    }
    
    if (items < 2)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PERL_CURRENT_SCRIPT_NAME, "buffer_get_string");
        PERL_RETURN_EMPTY;
    }
    
    buffer = SvPV (ST (0), PL_na);
    property = SvPV (ST (1), PL_na);
    
    result = weechat_buffer_get_string (script_str2ptr (buffer), property);
    
    PERL_RETURN_STRING(result);
}

/*
 * weechat::buffer_get_pointer: get a buffer property as pointer
 */

XS (XS_weechat_api_buffer_get_pointer)
{
    char *result, *buffer, *property;
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PERL_CURRENT_SCRIPT_NAME, "buffer_get_pointer");
	PERL_RETURN_EMPTY;
    }
    
    if (items < 2)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PERL_CURRENT_SCRIPT_NAME, "buffer_get_pointer");
        PERL_RETURN_EMPTY;
    }
    
    buffer = SvPV (ST (0), PL_na);
    property = SvPV (ST (1), PL_na);
    
    result = script_ptr2str (weechat_buffer_get_pointer (script_str2ptr (buffer),
                                                         property));
    
    PERL_RETURN_STRING_FREE(result);
}

/*
 * weechat::buffer_set: set a buffer property
 */

XS (XS_weechat_api_buffer_set)
{
    char *buffer, *property, *value;
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PERL_CURRENT_SCRIPT_NAME, "buffer_set");
        PERL_RETURN_ERROR;
    }
    
    if (items < 3)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PERL_CURRENT_SCRIPT_NAME, "buffer_set");
        PERL_RETURN_ERROR;
    }
    
    buffer = SvPV (ST (0), PL_na);
    property = SvPV (ST (1), PL_na);
    value = SvPV (ST (2), PL_na);
    
    weechat_buffer_set (script_str2ptr (buffer), property, value);
    
    PERL_RETURN_OK;
}

/*
 * weechat::current_window: get current window
 */

XS (XS_weechat_api_current_window)
{
    char *result;
    dXSARGS;
    
    /* make C compiler happy */
    (void) items;
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PERL_CURRENT_SCRIPT_NAME, "current_window");
	PERL_RETURN_EMPTY;
    }
    
    result = script_ptr2str (weechat_current_window ());
    
    PERL_RETURN_STRING_FREE(result);
}

/*
 * weechat::window_get_integer: get a window property as integer
 */

XS (XS_weechat_api_window_get_integer)
{
    char *window, *property;
    int value;
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PERL_CURRENT_SCRIPT_NAME, "window_get_integer");
	PERL_RETURN_INT(-1);
    }
    
    if (items < 2)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PERL_CURRENT_SCRIPT_NAME, "window_get_integer");
        PERL_RETURN_INT(-1);
    }
    
    window = SvPV (ST (0), PL_na);
    property = SvPV (ST (1), PL_na);
    
    value = weechat_window_get_integer (script_str2ptr (window), property);
    
    PERL_RETURN_INT(value);
}

/*
 * weechat::window_get_string: get a window property as string
 */

XS (XS_weechat_api_window_get_string)
{
    char *window, *property;
    const char *result;
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PERL_CURRENT_SCRIPT_NAME, "window_get_string");
	PERL_RETURN_EMPTY;
    }
    
    if (items < 2)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PERL_CURRENT_SCRIPT_NAME, "window_get_string");
        PERL_RETURN_EMPTY;
    }
    
    window = SvPV (ST (0), PL_na);
    property = SvPV (ST (1), PL_na);
    
    result = weechat_window_get_string (script_str2ptr (window), property);
    
    PERL_RETURN_STRING(result);
}

/*
 * weechat::window_get_pointer: get a window property as pointer
 */

XS (XS_weechat_api_window_get_pointer)
{
    char *result, *window, *property;
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PERL_CURRENT_SCRIPT_NAME, "window_get_pointer");
	PERL_RETURN_EMPTY;
    }
    
    if (items < 2)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PERL_CURRENT_SCRIPT_NAME, "window_get_pointer");
        PERL_RETURN_EMPTY;
    }
    
    window = SvPV (ST (0), PL_na);
    property = SvPV (ST (1), PL_na);
    
    result = script_ptr2str (weechat_window_get_pointer (script_str2ptr (window),
                                                         property));
    
    PERL_RETURN_STRING_FREE(result);
}

/*
 * weechat::window_set_title: set window title
 */

XS (XS_weechat_api_window_set_title)
{
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PERL_CURRENT_SCRIPT_NAME, "window_set_title");
	PERL_RETURN_ERROR;
    }
    
    if (items < 1)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PERL_CURRENT_SCRIPT_NAME, "window_set_title");
        PERL_RETURN_ERROR;
    }
    
    weechat_window_set_title (SvPV (ST (0), PL_na)); /* title */
    
    PERL_RETURN_OK;
}

/*
 * weechat::nicklist_add_group: add a group in nicklist
 */

XS (XS_weechat_api_nicklist_add_group)
{
    char *result, *buffer, *parent_group, *name, *color;
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PERL_CURRENT_SCRIPT_NAME, "nicklist_add_group");
	PERL_RETURN_EMPTY;
    }
    
    if (items < 5)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PERL_CURRENT_SCRIPT_NAME, "nicklist_add_group");
        PERL_RETURN_EMPTY;
    }
    
    buffer = SvPV (ST (0), PL_na);
    parent_group = SvPV (ST (1), PL_na);
    name = SvPV (ST (2), PL_na);
    color = SvPV (ST (3), PL_na);
    
    result = script_ptr2str (weechat_nicklist_add_group (script_str2ptr (buffer),
                                                         script_str2ptr (parent_group),
                                                         name,
                                                         color,
                                                         SvIV (ST (4)))); /* visible */
    
    PERL_RETURN_STRING_FREE(result);
}

/*
 * weechat::nicklist_search_group: search a group in nicklist
 */

XS (XS_weechat_api_nicklist_search_group)
{
    char *result, *buffer, *from_group, *name;
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PERL_CURRENT_SCRIPT_NAME, "nicklist_search_group");
	PERL_RETURN_EMPTY;
    }
    
    if (items < 3)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PERL_CURRENT_SCRIPT_NAME, "nicklist_search_group");
        PERL_RETURN_EMPTY;
    }

    buffer = SvPV (ST (0), PL_na);
    from_group = SvPV (ST (1), PL_na);
    name = SvPV (ST (2), PL_na);
    
    result = script_ptr2str (weechat_nicklist_search_group (script_str2ptr (buffer),
                                                            script_str2ptr (from_group),
                                                            name));
    
    PERL_RETURN_STRING_FREE(result);
}

/*
 * weechat::nicklist_add_nick: add a nick in nicklist
 */

XS (XS_weechat_api_nicklist_add_nick)
{
    char *result, *buffer, *group, *name, *color, *prefix, *prefix_color;
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PERL_CURRENT_SCRIPT_NAME, "nicklist_add_nick");
	PERL_RETURN_EMPTY;
    }
    
    if (items < 7)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PERL_CURRENT_SCRIPT_NAME, "nicklist_add_nick");
        PERL_RETURN_EMPTY;
    }
    
    buffer = SvPV (ST (0), PL_na);
    group = SvPV (ST (1), PL_na);
    name = SvPV (ST (2), PL_na);
    color = SvPV (ST (3), PL_na);
    prefix = SvPV(ST (4), PL_na);
    prefix_color = SvPV (ST (5), PL_na);
    
    result = script_ptr2str (weechat_nicklist_add_nick (script_str2ptr (buffer),
                                                        script_str2ptr (group),
                                                        name,
                                                        color,
                                                        prefix,
                                                        prefix_color,
                                                        SvIV (ST (6)))); /* visible */
    
    PERL_RETURN_STRING_FREE(result);
}

/*
 * weechat::nicklist_search_nick: search a nick in nicklist
 */

XS (XS_weechat_api_nicklist_search_nick)
{
    char *result, *buffer, *from_group, *name;
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PERL_CURRENT_SCRIPT_NAME, "nicklist_search_nick");
	PERL_RETURN_EMPTY;
    }
    
    if (items < 3)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PERL_CURRENT_SCRIPT_NAME, "nicklist_search_nick");
        PERL_RETURN_EMPTY;
    }
    
    buffer = SvPV (ST (0), PL_na);
    from_group = SvPV (ST (1), PL_na);
    name = SvPV (ST (2), PL_na);
    
    result = script_ptr2str (weechat_nicklist_search_nick (script_str2ptr (buffer),
                                                           script_str2ptr (from_group),
                                                           name));
    
    PERL_RETURN_STRING_FREE(result);
}

/*
 * weechat::nicklist_remove_group: remove a group from nicklist
 */

XS (XS_weechat_api_nicklist_remove_group)
{
    char *buffer, *group;
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PERL_CURRENT_SCRIPT_NAME, "nicklist_remove_group");
	PERL_RETURN_ERROR;
    }
    
    if (items < 2)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PERL_CURRENT_SCRIPT_NAME, "nicklist_remove_group");
        PERL_RETURN_ERROR;
    }
    
    buffer = SvPV (ST (0), PL_na);
    group = SvPV (ST (1), PL_na);
    
    weechat_nicklist_remove_group (script_str2ptr (buffer), 
                                   script_str2ptr (group));
    
    PERL_RETURN_OK;
}

/*
 * weechat::nicklist_remove_nick: remove a nick from nicklist
 */

XS (XS_weechat_api_nicklist_remove_nick)
{
    char *buffer, *nick;
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PERL_CURRENT_SCRIPT_NAME, "nicklist_remove_nick");
	PERL_RETURN_ERROR;
    }
    
    if (items < 2)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PERL_CURRENT_SCRIPT_NAME, "nicklist_remove_nick");
        PERL_RETURN_ERROR;
    }
    
    buffer = SvPV (ST (0), PL_na);
    nick = SvPV (ST (1), PL_na);
    
    weechat_nicklist_remove_nick (script_str2ptr (buffer),
                                  script_str2ptr (nick));
    
    PERL_RETURN_OK;
}

/*
 * weechat::nicklist_remove_all: remove all groups/nicks from nicklist
 */

XS (XS_weechat_api_nicklist_remove_all)
{
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PERL_CURRENT_SCRIPT_NAME, "nicklist_remove_all");
	PERL_RETURN_ERROR;
    }
    
    if (items < 2)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PERL_CURRENT_SCRIPT_NAME, "nicklist_remove_all");
        PERL_RETURN_ERROR;
    }
    
    weechat_nicklist_remove_all (script_str2ptr (SvPV (ST (0), PL_na))); /* buffer */
    
    PERL_RETURN_OK;
}

/*
 * weechat::bar_item_search: search a bar item
 */

XS (XS_weechat_api_bar_item_search)
{
    char *result;
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PERL_CURRENT_SCRIPT_NAME, "bar_item_search");
	PERL_RETURN_EMPTY;
    }
    
    if (items < 1)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PERL_CURRENT_SCRIPT_NAME, "bar_item_search");
        PERL_RETURN_EMPTY;
    }
    
    result = script_ptr2str (weechat_bar_item_search (SvPV (ST (0), PL_na))); /* name */
    
    PERL_RETURN_STRING_FREE(result);
}

/*
 * weechat_perl_api_bar_item_build_cb: callback for building bar item
 */

char *
weechat_perl_api_bar_item_build_cb (void *data, struct t_gui_bar_item *item,
                                    struct t_gui_window *window)
{
    struct t_script_callback *script_callback;
    char *perl_argv[4], empty_arg[1] = { '\0' }, *ret;
    
    script_callback = (struct t_script_callback *)data;

    if (script_callback && script_callback->function && script_callback->function[0])
    {
        perl_argv[0] = (script_callback->data) ? script_callback->data : empty_arg;
        perl_argv[1] = script_ptr2str (item);
        perl_argv[2] = script_ptr2str (window);
        perl_argv[3] = NULL;
        
        ret = (char *)weechat_perl_exec (script_callback->script,
                                         WEECHAT_SCRIPT_EXEC_STRING,
                                         script_callback->function,
                                         perl_argv);
        
        if (perl_argv[1])
            free (perl_argv[1]);
        if (perl_argv[2])
            free (perl_argv[2]);
        
        return ret;
    }
    
    return NULL;
}

/*
 * weechat::bar_item_new: add a new bar item
 */

XS (XS_weechat_api_bar_item_new)
{
    char *result, *name, *function, *data;
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PERL_CURRENT_SCRIPT_NAME, "bar_item_new");
	PERL_RETURN_EMPTY;
    }
    
    if (items < 3)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PERL_CURRENT_SCRIPT_NAME, "bar_item_new");
        PERL_RETURN_EMPTY;
    }
    
    name = SvPV (ST (0), PL_na);
    function = SvPV (ST (1), PL_na);
    data = SvPV (ST (2), PL_na);
    
    result = script_ptr2str (script_api_bar_item_new (weechat_perl_plugin,
                                                      perl_current_script,
                                                      name,
                                                      &weechat_perl_api_bar_item_build_cb,
                                                      function,
                                                      data));
    
    PERL_RETURN_STRING_FREE(result);
}

/*
 * weechat::bar_item_update: update a bar item on screen
 */

XS (XS_weechat_api_bar_item_update)
{
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PERL_CURRENT_SCRIPT_NAME, "bar_item_update");
	PERL_RETURN_ERROR;
    }
    
    if (items < 1)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PERL_CURRENT_SCRIPT_NAME, "bar_item_update");
        PERL_RETURN_ERROR;
    }
    
    weechat_bar_item_update (SvPV (ST (0), PL_na)); /* name */
    
    PERL_RETURN_OK;
}

/*
 * weechat::bar_item_remove: remove a bar item
 */

XS (XS_weechat_api_bar_item_remove)
{
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PERL_CURRENT_SCRIPT_NAME, "bar_item_remove");
	PERL_RETURN_ERROR;
    }
    
    if (items < 1)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PERL_CURRENT_SCRIPT_NAME, "bar_item_remove");
        PERL_RETURN_ERROR;
    }
    
    script_api_bar_item_remove (weechat_perl_plugin,
                                perl_current_script,
                                script_str2ptr (SvPV (ST (0), PL_na))); /* item */
    
    PERL_RETURN_OK;
}

/*
 * weechat::bar_search: search a bar
 */

XS (XS_weechat_api_bar_search)
{
    char *result;
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PERL_CURRENT_SCRIPT_NAME, "bar_search");
	PERL_RETURN_EMPTY;
    }
    
    if (items < 1)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PERL_CURRENT_SCRIPT_NAME, "bar_search");
        PERL_RETURN_EMPTY;
    }
    
    result = script_ptr2str (weechat_bar_search (SvPV (ST (0), PL_na))); /* name */
    
    PERL_RETURN_STRING_FREE(result);
}

/*
 * weechat::bar_new: add a new bar
 */

XS (XS_weechat_api_bar_new)
{
    char *result, *name, *hidden, *priority, *type, *conditions, *position;
    char *filling_top_bottom, *filling_left_right, *size, *size_max, *color_fg;
    char *color_delim, *color_bg, *separator, *bar_items;
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PERL_CURRENT_SCRIPT_NAME, "bar_new");
	PERL_RETURN_EMPTY;
    }
    
    if (items < 15)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PERL_CURRENT_SCRIPT_NAME, "bar_new");
        PERL_RETURN_EMPTY;
    }
    
    name = SvPV (ST (0), PL_na);
    hidden = SvPV (ST (1), PL_na);
    priority = SvPV (ST (2), PL_na);
    type = SvPV (ST (3), PL_na);
    conditions = SvPV (ST (4), PL_na);
    position = SvPV (ST (5), PL_na);
    filling_top_bottom = SvPV (ST (6), PL_na);
    filling_left_right = SvPV (ST (7), PL_na);
    size = SvPV (ST (8), PL_na);
    size_max = SvPV (ST (9), PL_na);
    color_fg = SvPV (ST (10), PL_na);
    color_delim = SvPV (ST (11), PL_na);
    color_bg = SvPV (ST (12), PL_na);
    separator = SvPV (ST (13), PL_na);
    bar_items = SvPV (ST (14), PL_na);
    
    result = script_ptr2str (weechat_bar_new (name,
                                              hidden,
                                              priority,
                                              type,
                                              conditions,
                                              position,
                                              filling_top_bottom,
                                              filling_left_right,
                                              size,
                                              size_max,
                                              color_fg,
                                              color_delim,
                                              color_bg,
                                              separator,
                                              bar_items));
    
    PERL_RETURN_STRING_FREE(result);
}

/*
 * weechat::bar_set: set a bar property
 */

XS (XS_weechat_api_bar_set)
{
    char *bar, *property, *value;
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PERL_CURRENT_SCRIPT_NAME, "bar_set");
	PERL_RETURN_ERROR;
    }
    
    if (items < 3)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PERL_CURRENT_SCRIPT_NAME, "bar_set");
        PERL_RETURN_ERROR;
    }
    
    bar = SvPV (ST (0), PL_na);
    property = SvPV (ST (1), PL_na);
    value = SvPV (ST (2), PL_na);
    
    weechat_bar_set (script_str2ptr (bar), property, value);
    
    PERL_RETURN_OK;
}

/*
 * weechat::bar_update: update a bar on screen
 */

XS (XS_weechat_api_bar_update)
{
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PERL_CURRENT_SCRIPT_NAME, "bar_update");
	PERL_RETURN_ERROR;
    }
    
    if (items < 1)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PERL_CURRENT_SCRIPT_NAME, "bar_update");
        PERL_RETURN_ERROR;
    }
    
    weechat_bar_update (SvPV (ST (0), PL_na)); /* name */
    
    PERL_RETURN_OK;
}

/*
 * weechat::bar_remove: remove a bar
 */

XS (XS_weechat_api_bar_remove)
{
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PERL_CURRENT_SCRIPT_NAME, "bar_remove");
	PERL_RETURN_ERROR;
    }
    
    if (items < 1)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PERL_CURRENT_SCRIPT_NAME, "bar_remove");
        PERL_RETURN_ERROR;
    }
    
    weechat_bar_remove (script_str2ptr (SvPV (ST (0), PL_na))); /* bar */
    
    PERL_RETURN_OK;
}

/*
 * weechat::command: execute a command on a buffer
 */

XS (XS_weechat_api_command)
{
    char *buffer, *command;
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PERL_CURRENT_SCRIPT_NAME, "command");
        PERL_RETURN_ERROR;
    }
    
    if (items < 2)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PERL_CURRENT_SCRIPT_NAME, "command");
        PERL_RETURN_ERROR;
    }
    
    buffer = SvPV (ST (0), PL_na);
    command = SvPV (ST (1), PL_na);
    
    script_api_command (weechat_perl_plugin,
                        perl_current_script,
                        script_str2ptr (buffer),
                        command);
    
    PERL_RETURN_OK;
}

/*
 * weechat::info_get: get info about WeeChat
 */

XS (XS_weechat_api_info_get)
{
    char *info_name, *arguments;
    const char *result;
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PERL_CURRENT_SCRIPT_NAME, "info_get");
	PERL_RETURN_EMPTY;
    }
    
    if (items < 2)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PERL_CURRENT_SCRIPT_NAME, "info_get");
        PERL_RETURN_EMPTY;
    }
    
    info_name = SvPV (ST (0), PL_na);
    arguments = SvPV (ST (1), PL_na);
    
    result = weechat_info_get (info_name, arguments);
    
    PERL_RETURN_STRING(result);
}

/*
 * weechat::infolist_new: create new infolist
 */

XS (XS_weechat_api_infolist_new)
{
    char *result;
    dXSARGS;
    
    /* make C compiler happy */
    (void) items;
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PERL_CURRENT_SCRIPT_NAME, "infolist_new");
	PERL_RETURN_EMPTY;
    }
    
    result = script_ptr2str (weechat_infolist_new ());
    
    PERL_RETURN_STRING_FREE(result);
}

/*
 * weechat::infolist_new_var_integer: create new integer variable in infolist
 */

XS (XS_weechat_api_infolist_new_var_integer)
{
    char *infolist, *name, *result;
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PERL_CURRENT_SCRIPT_NAME, "infolist_new_var_integer");
	PERL_RETURN_EMPTY;
    }
    
    if (items < 3)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PERL_CURRENT_SCRIPT_NAME, "infolist_new_var_integer");
        PERL_RETURN_EMPTY;
    }
    
    infolist = SvPV (ST (0), PL_na);
    name = SvPV (ST (1), PL_na);
    
    result = script_ptr2str (weechat_infolist_new_var_integer (script_str2ptr (infolist),
                                                               name,
                                                               SvIV (ST (2)))); /* value */
    
    PERL_RETURN_STRING_FREE(result);
}

/*
 * weechat::infolist_new_var_string: create new string variable in infolist
 */

XS (XS_weechat_api_infolist_new_var_string)
{
    char *infolist, *name, *value, *result;
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PERL_CURRENT_SCRIPT_NAME, "infolist_new_var_string");
	PERL_RETURN_EMPTY;
    }
    
    if (items < 3)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PERL_CURRENT_SCRIPT_NAME, "infolist_new_var_string");
        PERL_RETURN_EMPTY;
    }
    
    infolist = SvPV (ST (0), PL_na);
    name = SvPV (ST (1), PL_na);
    value = SvPV (ST (2), PL_na);
    
    result = script_ptr2str (weechat_infolist_new_var_string (script_str2ptr (infolist),
                                                              name,
                                                              value));
    
    PERL_RETURN_STRING_FREE(result);
}

/*
 * weechat::infolist_new_var_pointer: create new pointer variable in infolist
 */

XS (XS_weechat_api_infolist_new_var_pointer)
{
    char *infolist, *name, *value, *result;
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PERL_CURRENT_SCRIPT_NAME, "infolist_new_var_pointer");
	PERL_RETURN_EMPTY;
    }
    
    if (items < 3)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PERL_CURRENT_SCRIPT_NAME, "infolist_new_var_pointer");
        PERL_RETURN_EMPTY;
    }
    
    infolist = SvPV (ST (0), PL_na);
    name = SvPV (ST (1), PL_na);
    value = SvPV (ST (2), PL_na);
    
    result = script_ptr2str (weechat_infolist_new_var_pointer (script_str2ptr (infolist),
                                                               name,
                                                               script_str2ptr (value)));
    
    PERL_RETURN_STRING_FREE(result);
}

/*
 * weechat::infolist_new_var_time: create new time variable in infolist
 */

XS (XS_weechat_api_infolist_new_var_time)
{
    char *infolist, *name, *result;
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PERL_CURRENT_SCRIPT_NAME, "infolist_new_var_time");
	PERL_RETURN_EMPTY;
    }
    
    if (items < 3)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PERL_CURRENT_SCRIPT_NAME, "infolist_new_var_time");
        PERL_RETURN_EMPTY;
    }
    
    infolist = SvPV (ST (0), PL_na);
    name = SvPV (ST (1), PL_na);
    
    result = script_ptr2str (weechat_infolist_new_var_time (script_str2ptr (infolist),
                                                            name,
                                                            SvIV (ST (2)))); /* value */
    
    PERL_RETURN_STRING_FREE(result);
}

/*
 * weechat::infolist_get: get list with infos
 */

XS (XS_weechat_api_infolist_get)
{
    char *result, *name, *pointer, *arguments;
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PERL_CURRENT_SCRIPT_NAME, "infolist_get");
	PERL_RETURN_EMPTY;
    }
    
    if (items < 3)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PERL_CURRENT_SCRIPT_NAME, "infolist_get");
        PERL_RETURN_EMPTY;
    }
    
    name = SvPV (ST (0), PL_na);
    pointer = SvPV (ST (1), PL_na);
    arguments = SvPV (ST (2), PL_na);
    
    result = script_ptr2str (weechat_infolist_get (name,
                                                   script_str2ptr (pointer),
                                                   arguments));
    
    PERL_RETURN_STRING_FREE(result);
}

/*
 * weechat::infolist_next: move item pointer to next item in infolist
 */

XS (XS_weechat_api_infolist_next)
{
    int value;
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PERL_CURRENT_SCRIPT_NAME, "infolist_next");
	PERL_RETURN_INT(0);
    }
    
    if (items < 1)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PERL_CURRENT_SCRIPT_NAME, "infolist_next");
        PERL_RETURN_INT(0);
    }
    
    value = weechat_infolist_next (script_str2ptr (SvPV (ST (0), PL_na))); /* infolist */
    
    PERL_RETURN_INT(value);
}

/*
 * weechat::infolist_prev: move item pointer to previous item in infolist
 */

XS (XS_weechat_api_infolist_prev)
{
    int value;
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PERL_CURRENT_SCRIPT_NAME, "infolist_prev");
	PERL_RETURN_INT(0);
    }
    
    if (items < 1)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PERL_CURRENT_SCRIPT_NAME, "infolist_prev");
        PERL_RETURN_INT(0);
    }
    
    value = weechat_infolist_prev (script_str2ptr (SvPV (ST (0), PL_na))); /* infolist */
    
    PERL_RETURN_INT(value);
}

/*
 * weechat::infolist_fields: get list of fields for current item of infolist
 */

XS (XS_weechat_api_infolist_fields)
{
    const char *result;
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PERL_CURRENT_SCRIPT_NAME, "infolist_fields");
	PERL_RETURN_EMPTY;
    }
    
    if (items < 1)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PERL_CURRENT_SCRIPT_NAME, "infolist_fields");
        PERL_RETURN_EMPTY;
    }
    
    result = weechat_infolist_fields (script_str2ptr (SvPV (ST (0), PL_na))); /* infolist */
    
    PERL_RETURN_STRING(result);
}

/*
 * weechat::infolist_integer: get integer value of a variable in infolist
 */

XS (XS_weechat_api_infolist_integer)
{
    char *infolist, *variable;
    int value;
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PERL_CURRENT_SCRIPT_NAME, "infolist_integer");
	PERL_RETURN_INT(0);
    }
    
    if (items < 2)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PERL_CURRENT_SCRIPT_NAME, "infolist_integer");
        PERL_RETURN_INT(0);
    }
    
    infolist = SvPV (ST (0), PL_na);
    variable = SvPV (ST (1), PL_na);
    
    value = weechat_infolist_integer (script_str2ptr (infolist), variable);
    
    PERL_RETURN_INT(value);
}

/*
 * weechat::infolist_string: get string value of a variable in infolist
 */

XS (XS_weechat_api_infolist_string)
{
    char *infolist, *variable;
    const char *result;
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PERL_CURRENT_SCRIPT_NAME, "infolist_string");
	PERL_RETURN_EMPTY;
    }
    
    if (items < 2)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PERL_CURRENT_SCRIPT_NAME, "infolist_string");
        PERL_RETURN_EMPTY;
    }
    
    infolist = SvPV (ST (0), PL_na);
    variable = SvPV (ST (1), PL_na);
    
    result = weechat_infolist_string (script_str2ptr (infolist), variable);
    
    PERL_RETURN_STRING(result);
}

/*
 * weechat::infolist_pointer: get pointer value of a variable in infolist
 */

XS (XS_weechat_api_infolist_pointer)
{
    char *infolist, *variable;
    char *result;
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PERL_CURRENT_SCRIPT_NAME, "infolist_pointer");
	PERL_RETURN_EMPTY;
    }
    
    if (items < 2)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PERL_CURRENT_SCRIPT_NAME, "infolist_pointer");
        PERL_RETURN_EMPTY;
    }
    
    infolist = SvPV (ST (0), PL_na);
    variable = SvPV (ST (1), PL_na);
    
    result = script_ptr2str (weechat_infolist_pointer (script_str2ptr (infolist), variable));
    
    PERL_RETURN_STRING_FREE(result);
}

/*
 * weechat::infolist_time: get time value of a variable in infolist
 */

XS (XS_weechat_api_infolist_time)
{
    time_t time;
    char timebuffer[64], *result, *infolist, *variable;
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PERL_CURRENT_SCRIPT_NAME, "infolist_time");
	PERL_RETURN_EMPTY;
    }
    
    if (items < 2)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PERL_CURRENT_SCRIPT_NAME, "infolist_time");
        PERL_RETURN_EMPTY;
    }
    
    infolist = SvPV (ST (0), PL_na);
    variable = SvPV (ST (1), PL_na);
    time = weechat_infolist_time (script_str2ptr (infolist), variable);
    strftime (timebuffer, sizeof (timebuffer), "%F %T", localtime (&time));
    result = strdup (timebuffer);
    
    PERL_RETURN_STRING_FREE(result);
}

/*
 * weechat::infolist_free: free infolist
 */

XS (XS_weechat_api_infolist_free)
{
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PERL_CURRENT_SCRIPT_NAME, "infolist_free");
	PERL_RETURN_ERROR;
    }
    
    if (items < 1)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PERL_CURRENT_SCRIPT_NAME, "infolist_free");
        PERL_RETURN_ERROR;
    }
    
    weechat_infolist_free (script_str2ptr (SvPV (ST (0), PL_na))); /* infolist */
    
    PERL_RETURN_OK;
}

/*
 * weechat::upgrade_new: create an upgrade file
 */

XS (XS_weechat_api_upgrade_new)
{
    char *result, *filename;
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PERL_CURRENT_SCRIPT_NAME, "upgrade_new");
        PERL_RETURN_EMPTY;
    }
    
    if (items < 2)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PERL_CURRENT_SCRIPT_NAME, "upgrade_new");
        PERL_RETURN_EMPTY;
    }
    
    filename = SvPV (ST (0), PL_na);
    
    result = script_ptr2str (weechat_upgrade_new (filename,
                                                  SvIV (ST (1)))); /* write */
    
    PERL_RETURN_STRING_FREE(result);
}

/*
 * weechat::upgrade_write_object: write object in upgrade file
 */

XS (XS_weechat_api_upgrade_write_object)
{
    char *upgrade_file, *infolist;
    int rc;
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PERL_CURRENT_SCRIPT_NAME, "upgrade_write_object");
        PERL_RETURN_INT(0);
    }
    
    if (items < 3)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PERL_CURRENT_SCRIPT_NAME, "upgrade_write_object");
        PERL_RETURN_INT(0);
    }

    upgrade_file = SvPV (ST (0), PL_na);
    infolist = SvPV (ST (2), PL_na);
    
    rc = weechat_upgrade_write_object (script_str2ptr (upgrade_file),
                                       SvIV (ST (1)), /* object_id */
                                       script_str2ptr (infolist));
    
    PERL_RETURN_INT(rc);
}

/*
 * weechat_perl_api_upgrade_read_cb: callback for reading object in upgrade file
 */

int
weechat_perl_api_upgrade_read_cb (void *data,
                                  struct t_upgrade_file *upgrade_file,
                                  int object_id,
                                  struct t_infolist *infolist)
{
    struct t_script_callback *script_callback;
    char *perl_argv[5], empty_arg[1] = { '\0' }, str_object_id[32];
    int *rc, ret;
    
    script_callback = (struct t_script_callback *)data;
    
    if (script_callback && script_callback->function && script_callback->function[0])
    {
        snprintf (str_object_id, sizeof (str_object_id), "%d", object_id);
        
        perl_argv[0] = (script_callback->data) ? script_callback->data : empty_arg;
        perl_argv[1] = script_ptr2str (upgrade_file);
        perl_argv[2] = str_object_id;
        perl_argv[3] = script_ptr2str (infolist);
        perl_argv[4] = NULL;
        
        rc = (int *) weechat_perl_exec (script_callback->script,
                                        WEECHAT_SCRIPT_EXEC_INT,
                                        script_callback->function,
                                        perl_argv);
        
        if (!rc)
            ret = WEECHAT_RC_ERROR;
        else
        {
            ret = *rc;
            free (rc);
        }
        if (perl_argv[1])
            free (perl_argv[1]);
        if (perl_argv[3])
            free (perl_argv[3]);
        
        return ret;
    }
    
    return WEECHAT_RC_ERROR;
}

/*
 * weechat::config_upgrade_read: read upgrade file
 */

XS (XS_weechat_api_upgrade_read)
{
    char *upgrade_file, *function, *data;
    int rc;
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PERL_CURRENT_SCRIPT_NAME, "upgrade_read");
        PERL_RETURN_INT(0);
    }
    
    if (items < 3)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PERL_CURRENT_SCRIPT_NAME, "upgrade_read");
        PERL_RETURN_INT(0);
    }
    
    upgrade_file = SvPV (ST (0), PL_na);
    function = SvPV (ST (1), PL_na);
    data = SvPV (ST (2), PL_na);
    
    rc = script_api_upgrade_read (weechat_perl_plugin,
                                  perl_current_script,
                                  script_str2ptr (upgrade_file),
                                  &weechat_perl_api_upgrade_read_cb,
                                  function,
                                  data);
    
    PERL_RETURN_INT(rc);
}

/*
 * weechat::upgrade_close: close upgrade file
 */

XS (XS_weechat_api_upgrade_close)
{
    char *upgrade_file;
    dXSARGS;
    
    /* make C compiler happy */
    (void) cv;
    
    if (!perl_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PERL_CURRENT_SCRIPT_NAME, "upgrade_close");
        PERL_RETURN_ERROR;
    }
    
    if (items < 1)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PERL_CURRENT_SCRIPT_NAME, "upgrade_close");
        PERL_RETURN_ERROR;
    }
    
    upgrade_file = SvPV (ST (0), PL_na);
    
    weechat_upgrade_close (script_str2ptr (upgrade_file));
    
    PERL_RETURN_OK;
}

/*
 * weechat_perl_api_init: initialize subroutines
 */

void
weechat_perl_api_init (pTHX)
{
    HV *stash;
    
    newXS ("DynaLoader::boot_DynaLoader", boot_DynaLoader, __FILE__);
    
    /* interface functions */
    newXS ("weechat::register", XS_weechat_api_register, "weechat");
    newXS ("weechat::plugin_get_name", XS_weechat_api_plugin_get_name, "weechat");
    newXS ("weechat::charset_set", XS_weechat_api_charset_set, "weechat");
    newXS ("weechat::iconv_to_internal", XS_weechat_api_iconv_to_internal, "weechat");
    newXS ("weechat::iconv_from_internal", XS_weechat_api_iconv_from_internal, "weechat");
    newXS ("weechat::gettext", XS_weechat_api_gettext, "weechat");
    newXS ("weechat::ngettext", XS_weechat_api_ngettext, "weechat");
    newXS ("weechat::string_remove_color", XS_weechat_api_string_remove_color, "weechat");
    newXS ("weechat::mkdir_home", XS_weechat_api_mkdir_home, "weechat");
    newXS ("weechat::mkdir", XS_weechat_api_mkdir, "weechat");
    newXS ("weechat::mkdir_parents", XS_weechat_api_mkdir_parents, "weechat");
    newXS ("weechat::list_new", XS_weechat_api_list_new, "weechat");
    newXS ("weechat::list_add", XS_weechat_api_list_add, "weechat");
    newXS ("weechat::list_search", XS_weechat_api_list_search, "weechat");
    newXS ("weechat::list_casesearch", XS_weechat_api_list_casesearch, "weechat");
    newXS ("weechat::list_get", XS_weechat_api_list_get, "weechat");
    newXS ("weechat::list_set", XS_weechat_api_list_set, "weechat");
    newXS ("weechat::list_next", XS_weechat_api_list_next, "weechat");
    newXS ("weechat::list_prev", XS_weechat_api_list_prev, "weechat");
    newXS ("weechat::list_string", XS_weechat_api_list_string, "weechat");
    newXS ("weechat::list_size", XS_weechat_api_list_size, "weechat");
    newXS ("weechat::list_remove", XS_weechat_api_list_remove, "weechat");
    newXS ("weechat::list_remove_all", XS_weechat_api_list_remove_all, "weechat");
    newXS ("weechat::list_free", XS_weechat_api_list_free, "weechat");
    newXS ("weechat::config_new", XS_weechat_api_config_new, "weechat");
    newXS ("weechat::config_new_section", XS_weechat_api_config_new_section, "weechat");
    newXS ("weechat::config_search_section", XS_weechat_api_config_search_section, "weechat");
    newXS ("weechat::config_new_option", XS_weechat_api_config_new_option, "weechat");
    newXS ("weechat::config_search_option", XS_weechat_api_config_search_option, "weechat");
    newXS ("weechat::config_string_to_boolean", XS_weechat_api_config_string_to_boolean, "weechat");
    newXS ("weechat::config_option_reset", XS_weechat_api_config_option_reset, "weechat");
    newXS ("weechat::config_option_set", XS_weechat_api_config_option_set, "weechat");
    newXS ("weechat::config_option_set_null", XS_weechat_api_config_option_set_null, "weechat");
    newXS ("weechat::config_option_unset", XS_weechat_api_config_option_unset, "weechat");
    newXS ("weechat::config_option_rename", XS_weechat_api_config_option_rename, "weechat");
    newXS ("weechat::config_option_is_null", XS_weechat_api_config_option_is_null, "weechat");
    newXS ("weechat::config_option_default_is_null", XS_weechat_api_config_option_default_is_null, "weechat");
    newXS ("weechat::config_boolean", XS_weechat_api_config_boolean, "weechat");
    newXS ("weechat::config_boolean_default", XS_weechat_api_config_boolean_default, "weechat");
    newXS ("weechat::config_integer", XS_weechat_api_config_integer, "weechat");
    newXS ("weechat::config_integer_default", XS_weechat_api_config_integer_default, "weechat");
    newXS ("weechat::config_string", XS_weechat_api_config_string, "weechat");
    newXS ("weechat::config_string_default", XS_weechat_api_config_string_default, "weechat");
    newXS ("weechat::config_color", XS_weechat_api_config_color, "weechat");
    newXS ("weechat::config_color_default", XS_weechat_api_config_color_default, "weechat");
    newXS ("weechat::config_write_option", XS_weechat_api_config_write_option, "weechat");
    newXS ("weechat::config_write_line", XS_weechat_api_config_write_line, "weechat");
    newXS ("weechat::config_write", XS_weechat_api_config_write, "weechat");
    newXS ("weechat::config_read", XS_weechat_api_config_read, "weechat");
    newXS ("weechat::config_reload", XS_weechat_api_config_reload, "weechat");
    newXS ("weechat::config_option_free", XS_weechat_api_config_option_free, "weechat");
    newXS ("weechat::config_section_free_options", XS_weechat_api_config_section_free_options, "weechat");
    newXS ("weechat::config_section_free", XS_weechat_api_config_section_free, "weechat");
    newXS ("weechat::config_free", XS_weechat_api_config_free, "weechat");
    newXS ("weechat::config_get", XS_weechat_api_config_get, "weechat");
    newXS ("weechat::config_get_plugin", XS_weechat_api_config_get_plugin, "weechat");
    newXS ("weechat::config_is_set_plugin", XS_weechat_api_config_is_set_plugin, "weechat");
    newXS ("weechat::config_set_plugin", XS_weechat_api_config_set_plugin, "weechat");
    newXS ("weechat::config_unset_plugin", XS_weechat_api_config_unset_plugin, "weechat");
    newXS ("weechat::prefix", XS_weechat_api_prefix, "weechat");
    newXS ("weechat::color", XS_weechat_api_color, "weechat");
    newXS ("weechat::print", XS_weechat_api_print, "weechat");
    newXS ("weechat::print_date_tags", XS_weechat_api_print_date_tags, "weechat");
    newXS ("weechat::print_y", XS_weechat_api_print_y, "weechat");
    newXS ("weechat::log_print", XS_weechat_api_log_print, "weechat");
    newXS ("weechat::hook_command", XS_weechat_api_hook_command, "weechat");
    newXS ("weechat::hook_command_run", XS_weechat_api_hook_command_run, "weechat");
    newXS ("weechat::hook_timer", XS_weechat_api_hook_timer, "weechat");
    newXS ("weechat::hook_fd", XS_weechat_api_hook_fd, "weechat");
    newXS ("weechat::hook_process", XS_weechat_api_hook_process, "weechat");
    newXS ("weechat::hook_connect", XS_weechat_api_hook_connect, "weechat");
    newXS ("weechat::hook_print", XS_weechat_api_hook_print, "weechat");
    newXS ("weechat::hook_signal", XS_weechat_api_hook_signal, "weechat");
    newXS ("weechat::hook_signal_send", XS_weechat_api_hook_signal_send, "weechat");
    newXS ("weechat::hook_config", XS_weechat_api_hook_config, "weechat");
    newXS ("weechat::hook_completion", XS_weechat_api_hook_completion, "weechat");
    newXS ("weechat::hook_completion_list_add", XS_weechat_api_hook_completion_list_add, "weechat");
    newXS ("weechat::hook_modifier", XS_weechat_api_hook_modifier, "weechat");
    newXS ("weechat::hook_modifier_exec", XS_weechat_api_hook_modifier_exec, "weechat");
    newXS ("weechat::hook_info", XS_weechat_api_hook_info, "weechat");
    newXS ("weechat::hook_infolist", XS_weechat_api_hook_infolist, "weechat");
    newXS ("weechat::unhook", XS_weechat_api_unhook, "weechat");
    newXS ("weechat::unhook_all", XS_weechat_api_unhook_all, "weechat");
    newXS ("weechat::buffer_new", XS_weechat_api_buffer_new, "weechat");
    newXS ("weechat::buffer_search", XS_weechat_api_buffer_search, "weechat");
    newXS ("weechat::buffer_search_main", XS_weechat_api_buffer_search_main, "weechat");
    newXS ("weechat::current_buffer", XS_weechat_api_current_buffer, "weechat");
    newXS ("weechat::buffer_clear", XS_weechat_api_buffer_clear, "weechat");
    newXS ("weechat::buffer_close", XS_weechat_api_buffer_close, "weechat");
    newXS ("weechat::buffer_merge", XS_weechat_api_buffer_merge, "weechat");
    newXS ("weechat::buffer_unmerge", XS_weechat_api_buffer_unmerge, "weechat");
    newXS ("weechat::buffer_get_integer", XS_weechat_api_buffer_get_integer, "weechat");
    newXS ("weechat::buffer_get_string", XS_weechat_api_buffer_get_string, "weechat");
    newXS ("weechat::buffer_get_pointer", XS_weechat_api_buffer_get_pointer, "weechat");
    newXS ("weechat::buffer_set", XS_weechat_api_buffer_set, "weechat");
    newXS ("weechat::current_window", XS_weechat_api_current_window, "weechat");
    newXS ("weechat::window_get_integer", XS_weechat_api_window_get_integer, "weechat");
    newXS ("weechat::window_get_string", XS_weechat_api_window_get_string, "weechat");
    newXS ("weechat::window_get_pointer", XS_weechat_api_window_get_pointer, "weechat");
    newXS ("weechat::window_set_title", XS_weechat_api_window_set_title, "weechat");
    newXS ("weechat::nicklist_add_group", XS_weechat_api_nicklist_add_group, "weechat");
    newXS ("weechat::nicklist_search_group", XS_weechat_api_nicklist_search_group, "weechat");
    newXS ("weechat::nicklist_add_nick", XS_weechat_api_nicklist_add_nick, "weechat");
    newXS ("weechat::nicklist_search_nick", XS_weechat_api_nicklist_search_nick, "weechat");
    newXS ("weechat::nicklist_remove_group", XS_weechat_api_nicklist_remove_group, "weechat");
    newXS ("weechat::nicklist_remove_nick", XS_weechat_api_nicklist_remove_nick, "weechat");
    newXS ("weechat::nicklist_remove_all", XS_weechat_api_nicklist_remove_all, "weechat");
    newXS ("weechat::bar_item_search", XS_weechat_api_bar_item_search, "weechat");
    newXS ("weechat::bar_item_new", XS_weechat_api_bar_item_new, "weechat");
    newXS ("weechat::bar_item_update", XS_weechat_api_bar_item_update, "weechat");
    newXS ("weechat::bar_item_remove", XS_weechat_api_bar_item_remove, "weechat");
    newXS ("weechat::bar_search", XS_weechat_api_bar_search, "weechat");
    newXS ("weechat::bar_new", XS_weechat_api_bar_new, "weechat");
    newXS ("weechat::bar_set", XS_weechat_api_bar_set, "weechat");
    newXS ("weechat::bar_update", XS_weechat_api_bar_update, "weechat");
    newXS ("weechat::bar_remove", XS_weechat_api_bar_remove, "weechat");
    newXS ("weechat::command", XS_weechat_api_command, "weechat");
    newXS ("weechat::info_get", XS_weechat_api_info_get, "weechat");
    newXS ("weechat::infolist_new", XS_weechat_api_infolist_new, "weechat");
    newXS ("weechat::infolist_new_var_integer", XS_weechat_api_infolist_new_var_integer, "weechat");
    newXS ("weechat::infolist_new_var_string", XS_weechat_api_infolist_new_var_string, "weechat");
    newXS ("weechat::infolist_new_var_pointer", XS_weechat_api_infolist_new_var_pointer, "weechat");
    newXS ("weechat::infolist_new_var_time", XS_weechat_api_infolist_new_var_time, "weechat");
    newXS ("weechat::infolist_get", XS_weechat_api_infolist_get, "weechat");
    newXS ("weechat::infolist_next", XS_weechat_api_infolist_next, "weechat");
    newXS ("weechat::infolist_prev", XS_weechat_api_infolist_prev, "weechat");
    newXS ("weechat::infolist_fields", XS_weechat_api_infolist_fields, "weechat");
    newXS ("weechat::infolist_integer", XS_weechat_api_infolist_integer, "weechat");
    newXS ("weechat::infolist_string", XS_weechat_api_infolist_string, "weechat");
    newXS ("weechat::infolist_pointer", XS_weechat_api_infolist_pointer, "weechat");
    newXS ("weechat::infolist_time", XS_weechat_api_infolist_time, "weechat");
    newXS ("weechat::infolist_free", XS_weechat_api_infolist_free, "weechat");
    newXS ("weechat::upgrade_new", XS_weechat_api_upgrade_new, "weechat");
    newXS ("weechat::upgrade_write_object", XS_weechat_api_upgrade_write_object, "weechat");
    newXS ("weechat::upgrade_read", XS_weechat_api_upgrade_read, "weechat");
    newXS ("weechat::upgrade_close", XS_weechat_api_upgrade_close, "weechat");
    
    /* interface constants */
    stash = gv_stashpv ("weechat", TRUE);
    newCONSTSUB (stash, "weechat::WEECHAT_RC_OK", newSViv (WEECHAT_RC_OK));
    newCONSTSUB (stash, "weechat::WEECHAT_RC_OK_EAT", newSViv (WEECHAT_RC_OK_EAT));
    newCONSTSUB (stash, "weechat::WEECHAT_RC_ERROR", newSViv (WEECHAT_RC_ERROR));
    
    newCONSTSUB (stash, "weechat::WEECHAT_CONFIG_READ_OK", newSViv (WEECHAT_CONFIG_READ_OK));
    newCONSTSUB (stash, "weechat::WEECHAT_CONFIG_READ_MEMORY_ERROR", newSViv (WEECHAT_CONFIG_READ_MEMORY_ERROR));
    newCONSTSUB (stash, "weechat::WEECHAT_CONFIG_READ_FILE_NOT_FOUND", newSViv (WEECHAT_CONFIG_READ_FILE_NOT_FOUND));
    newCONSTSUB (stash, "weechat::WEECHAT_CONFIG_WRITE_OK", newSViv (WEECHAT_CONFIG_WRITE_OK));
    newCONSTSUB (stash, "weechat::WEECHAT_CONFIG_WRITE_ERROR", newSViv (WEECHAT_CONFIG_WRITE_ERROR));
    newCONSTSUB (stash, "weechat::WEECHAT_CONFIG_WRITE_MEMORY_ERROR", newSViv (WEECHAT_CONFIG_WRITE_MEMORY_ERROR));
    newCONSTSUB (stash, "weechat::WEECHAT_CONFIG_OPTION_SET_OK_CHANGED", newSViv (WEECHAT_CONFIG_OPTION_SET_OK_CHANGED));
    newCONSTSUB (stash, "weechat::WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE", newSViv (WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE));
    newCONSTSUB (stash, "weechat::WEECHAT_CONFIG_OPTION_SET_ERROR", newSViv (WEECHAT_CONFIG_OPTION_SET_ERROR));
    newCONSTSUB (stash, "weechat::WEECHAT_CONFIG_OPTION_SET_OPTION_NOT_FOUND", newSViv (WEECHAT_CONFIG_OPTION_SET_OPTION_NOT_FOUND));
    newCONSTSUB (stash, "weechat::WEECHAT_CONFIG_OPTION_UNSET_OK_NO_RESET", newSViv (WEECHAT_CONFIG_OPTION_UNSET_OK_NO_RESET));
    newCONSTSUB (stash, "weechat::WEECHAT_CONFIG_OPTION_UNSET_OK_RESET", newSViv (WEECHAT_CONFIG_OPTION_UNSET_OK_RESET));
    newCONSTSUB (stash, "weechat::WEECHAT_CONFIG_OPTION_UNSET_OK_REMOVED", newSViv (WEECHAT_CONFIG_OPTION_UNSET_OK_REMOVED));
    newCONSTSUB (stash, "weechat::WEECHAT_CONFIG_OPTION_UNSET_ERROR", newSViv (WEECHAT_CONFIG_OPTION_UNSET_ERROR));
    
    newCONSTSUB (stash, "weechat::WEECHAT_LIST_POS_SORT", newSVpv (WEECHAT_LIST_POS_SORT, PL_na));
    newCONSTSUB (stash, "weechat::WEECHAT_LIST_POS_BEGINNING", newSVpv (WEECHAT_LIST_POS_BEGINNING, PL_na));
    newCONSTSUB (stash, "weechat::WEECHAT_LIST_POS_END", newSVpv (WEECHAT_LIST_POS_END, PL_na));
    
    newCONSTSUB (stash, "weechat::WEECHAT_HOTLIST_LOW", newSVpv (WEECHAT_HOTLIST_LOW, PL_na));
    newCONSTSUB (stash, "weechat::WEECHAT_HOTLIST_MESSAGE", newSVpv (WEECHAT_HOTLIST_MESSAGE, PL_na));
    newCONSTSUB (stash, "weechat::WEECHAT_HOTLIST_PRIVATE", newSVpv (WEECHAT_HOTLIST_PRIVATE, PL_na));
    newCONSTSUB (stash, "weechat::WEECHAT_HOTLIST_HIGHLIGHT", newSVpv (WEECHAT_HOTLIST_HIGHLIGHT, PL_na));
    
    newCONSTSUB (stash, "weechat::WEECHAT_HOOK_PROCESS_RUNNING", newSViv (WEECHAT_HOOK_PROCESS_RUNNING));
    newCONSTSUB (stash, "weechat::WEECHAT_HOOK_PROCESS_ERROR", newSViv (WEECHAT_HOOK_PROCESS_ERROR));
    
    newCONSTSUB (stash, "weechat::WEECHAT_HOOK_CONNECT_OK", newSViv (WEECHAT_HOOK_CONNECT_OK));
    newCONSTSUB (stash, "weechat::WEECHAT_HOOK_CONNECT_ADDRESS_NOT_FOUND", newSViv (WEECHAT_HOOK_CONNECT_ADDRESS_NOT_FOUND));
    newCONSTSUB (stash, "weechat::WEECHAT_HOOK_CONNECT_IP_ADDRESS_NOT_FOUND", newSViv (WEECHAT_HOOK_CONNECT_IP_ADDRESS_NOT_FOUND));
    newCONSTSUB (stash, "weechat::WEECHAT_HOOK_CONNECT_CONNECTION_REFUSED", newSViv (WEECHAT_HOOK_CONNECT_CONNECTION_REFUSED));
    newCONSTSUB (stash, "weechat::WEECHAT_HOOK_CONNECT_PROXY_ERROR", newSViv (WEECHAT_HOOK_CONNECT_PROXY_ERROR));
    newCONSTSUB (stash, "weechat::WEECHAT_HOOK_CONNECT_LOCAL_HOSTNAME_ERROR", newSViv (WEECHAT_HOOK_CONNECT_LOCAL_HOSTNAME_ERROR));
    newCONSTSUB (stash, "weechat::WEECHAT_HOOK_CONNECT_GNUTLS_INIT_ERROR", newSViv (WEECHAT_HOOK_CONNECT_GNUTLS_INIT_ERROR));
    newCONSTSUB (stash, "weechat::WEECHAT_HOOK_CONNECT_GNUTLS_HANDSHAKE_ERROR", newSViv (WEECHAT_HOOK_CONNECT_GNUTLS_HANDSHAKE_ERROR));
    newCONSTSUB (stash, "weechat::WEECHAT_HOOK_CONNECT_MEMORY_ERROR", newSViv (WEECHAT_HOOK_CONNECT_MEMORY_ERROR));
    
    newCONSTSUB (stash, "weechat::WEECHAT_HOOK_SIGNAL_STRING", newSVpv (WEECHAT_HOOK_SIGNAL_STRING, PL_na));
    newCONSTSUB (stash, "weechat::WEECHAT_HOOK_SIGNAL_INT", newSVpv (WEECHAT_HOOK_SIGNAL_INT, PL_na));
    newCONSTSUB (stash, "weechat::WEECHAT_HOOK_SIGNAL_POINTER", newSVpv (WEECHAT_HOOK_SIGNAL_POINTER, PL_na));
}

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

/* weechat-lua-api.c: Lua API functions */


#undef _

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "../../weechat-plugin.h"
#include "../script.h"
#include "../script-api.h"
#include "../script-callback.h"
#include "weechat-lua.h"


#define LUA_RETURN_OK return 1
#define LUA_RETURN_ERROR return 0
#define LUA_RETURN_EMPTY                                \
    lua_pushstring (lua_current_interpreter, "");       \
    return 0
#define LUA_RETURN_STRING(__string)                     \
    lua_pushstring (lua_current_interpreter,            \
                    (__string) ? __string : "");        \
    return 1;
#define LUA_RETURN_STRING_FREE(__string)                \
    lua_pushstring (lua_current_interpreter,            \
                    (__string) ? __string : "");        \
    if (__string)                                       \
        free (__string);                                \
    return 1;
#define LUA_RETURN_INT(__int)                           \
    lua_pushnumber (lua_current_interpreter, __int);    \
    return 1;


/*
 * weechat_lua_api_register: startup function for all WeeChat Lua scripts
 */

static int
weechat_lua_api_register (lua_State *L)
{
    const char *name, *author, *version, *license, *description;
    const char *shutdown_func, *charset;
    int n;
    
    /* make C compiler happy */
    (void) L;
    
    lua_current_script = NULL;
    
    name = NULL;
    author = NULL;
    version = NULL;
    license = NULL;
    description = NULL;
    shutdown_func = NULL;
    charset = NULL;
    
    n = lua_gettop (lua_current_interpreter);

    if (n < 7)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(lua_current_script_filename, "register");
        LUA_RETURN_ERROR;
    }
    
    name = lua_tostring (lua_current_interpreter, -7);
    author = lua_tostring (lua_current_interpreter, -6);
    version = lua_tostring (lua_current_interpreter, -5);
    license = lua_tostring (lua_current_interpreter, -4);
    description = lua_tostring (lua_current_interpreter, -3);
    shutdown_func = lua_tostring (lua_current_interpreter, -2);
    charset = lua_tostring (lua_current_interpreter, -1);
    
    if (script_search (weechat_lua_plugin, lua_scripts, name))
    {
        weechat_printf (NULL,
                        weechat_gettext ("%s%s: unable to register script "
                                         "\"%s\" (another script already "
                                         "exists with this name)"),
                        weechat_prefix ("error"), LUA_PLUGIN_NAME, name);
        LUA_RETURN_ERROR;
    }
    
    /* register script */
    lua_current_script = script_add (weechat_lua_plugin,
                                     &lua_scripts, &last_lua_script,
                                     (lua_current_script_filename) ?
                                     lua_current_script_filename : "",
                                     name,
                                     author,
                                     version,
                                     license,
                                     description,
                                     shutdown_func,
                                     charset);
    if (lua_current_script)
    {
        if ((weechat_lua_plugin->debug >= 1) || !lua_quiet)
        {
            weechat_printf (NULL,
                            weechat_gettext ("%s: registered script \"%s\", "
                                             "version %s (%s)"),
                            LUA_PLUGIN_NAME, name, version, description);
        }
    }
    else
    {
        LUA_RETURN_ERROR;
    }
    
    LUA_RETURN_OK;
}

/*
 * weechat_lua_api_plugin_get_name: get name of plugin (return "core" for
 *                                  WeeChat core)
 */

static int
weechat_lua_api_plugin_get_name (lua_State *L)
{
    const char *plugin, *result;
    int n;
    
    /* make C compiler happy */
    (void) L;
    
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(LUA_CURRENT_SCRIPT_NAME, "plugin_get_name");
        LUA_RETURN_EMPTY;
    }
    
    plugin = NULL;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 1)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(LUA_CURRENT_SCRIPT_NAME, "plugin_get_name");
        LUA_RETURN_EMPTY;
    }
    
    plugin = lua_tostring (lua_current_interpreter, -1);
    
    result = weechat_plugin_get_name (script_str2ptr (plugin));
    
    LUA_RETURN_STRING(result);
}

/*
 * weechat_lua_api_charset_set: set script charset
 */

static int
weechat_lua_api_charset_set (lua_State *L)
{
    const char *charset;
    int n;
    
    /* make C compiler happy */
    (void) L;
     
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(LUA_CURRENT_SCRIPT_NAME, "charset_set");
        LUA_RETURN_ERROR;
    }
    
    charset = NULL;
 
    n = lua_gettop (lua_current_interpreter);

    if (n < 1)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(LUA_CURRENT_SCRIPT_NAME, "charset_set");
        LUA_RETURN_ERROR;
    }
    
    charset = lua_tostring (lua_current_interpreter, -1);
    
    script_api_charset_set (lua_current_script,
                            charset);

    LUA_RETURN_OK;
}

/*
 * weechat_lua_api_iconv_to_internal: convert string to internal WeeChat charset
 */

static int
weechat_lua_api_iconv_to_internal (lua_State *L)
{
    const char *charset, *string;
    char *result;
    int n;
    
    /* make C compiler happy */
    (void) L;
    
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(LUA_CURRENT_SCRIPT_NAME, "iconv_to_internal");
        LUA_RETURN_EMPTY;
    }
    
    charset = NULL;
    string = NULL;
    
    n = lua_gettop (lua_current_interpreter);

    if (n < 2)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(LUA_CURRENT_SCRIPT_NAME, "iconv_to_internal");
        LUA_RETURN_EMPTY;
    }
    
    charset = lua_tostring (lua_current_interpreter, -2);
    string = lua_tostring (lua_current_interpreter, -1);
    
    result = weechat_iconv_to_internal (charset, string);
    
    LUA_RETURN_STRING_FREE(result);
}

/*
 * weechat_lua_api_iconv_from_internal: convert string from WeeChat internal
 *                                      charset to another one
 */

static int
weechat_lua_api_iconv_from_internal (lua_State *L)
{
    const char *charset, *string;
    char *result;
    int n;
    
    /* make C compiler happy */
    (void) L;
    
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(LUA_CURRENT_SCRIPT_NAME, "iconv_from_internal");
        LUA_RETURN_EMPTY;
    }
    
    charset = NULL;
    string = NULL;
    
    n = lua_gettop (lua_current_interpreter);

    if (n < 2)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(LUA_CURRENT_SCRIPT_NAME, "iconv_from_internal");
        LUA_RETURN_EMPTY;
    }
    
    charset = lua_tostring (lua_current_interpreter, -2);
    string = lua_tostring (lua_current_interpreter, -1);
    
    result = weechat_iconv_from_internal (charset, string);
    
    LUA_RETURN_STRING_FREE(result);
}

/*
 * weechat_lua_api_gettext: get translated string
 */

static int
weechat_lua_api_gettext (lua_State *L)
{
    const char *string, *result;
    int n;
    
    /* make C compiler happy */
    (void) L;
    
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(LUA_CURRENT_SCRIPT_NAME, "gettext");
        LUA_RETURN_EMPTY;
    }
    
    string = NULL;
    
    n = lua_gettop (lua_current_interpreter);

    if (n < 1)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(LUA_CURRENT_SCRIPT_NAME, "gettext");
        LUA_RETURN_EMPTY;
    }
    
    string = lua_tostring (lua_current_interpreter, -1);
    
    result = weechat_gettext (string);
    
    LUA_RETURN_STRING(result);
}

/*
 * weechat_lua_api_ngettext: get translated string with plural form
 */

static int
weechat_lua_api_ngettext (lua_State *L)
{
    const char *single, *plural, *result;
    int n, count;
    
    /* make C compiler happy */
    (void) L;
    
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(LUA_CURRENT_SCRIPT_NAME, "ngettext");
        LUA_RETURN_EMPTY;
    }
    
    single = NULL;
    plural = NULL;
    count = 0;
    
    n = lua_gettop (lua_current_interpreter);

    if (n < 3)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(LUA_CURRENT_SCRIPT_NAME, "ngettext");
        LUA_RETURN_EMPTY;
    }
    
    single = lua_tostring (lua_current_interpreter, -3);
    plural = lua_tostring (lua_current_interpreter, -2);
    count = lua_tonumber (lua_current_interpreter, -1);
    
    result = weechat_ngettext (single, plural, count);
    
    LUA_RETURN_STRING(result);
}

/*
 * weechat_lua_api_string_remove_color: remove WeeChat color codes from string
 */

static int
weechat_lua_api_string_remove_color (lua_State *L)
{
    const char *string, *replacement;
    char *result;
    int n;
    
    /* make C compiler happy */
    (void) L;
    
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(LUA_CURRENT_SCRIPT_NAME, "string_remove_color");
        LUA_RETURN_EMPTY;
    }
    
    string = NULL;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 2)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(LUA_CURRENT_SCRIPT_NAME, "string_remove_color");
        LUA_RETURN_EMPTY;
    }
    
    string = lua_tostring (lua_current_interpreter, -2);
    replacement = lua_tostring (lua_current_interpreter, -1);
    
    result = weechat_string_remove_color (string, replacement);
    
    LUA_RETURN_STRING_FREE(result);
}

/*
 * weechat_lua_api_mkdir_home: create a directory in WeeChat home
 */

static int
weechat_lua_api_mkdir_home (lua_State *L)
{
    const char *directory;
    int mode, n;
    
    /* make C compiler happy */
    (void) L;
     
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(LUA_CURRENT_SCRIPT_NAME, "mkdir_home");
        LUA_RETURN_ERROR;
    }
    
    directory = NULL;
    mode = 0;
    
    n = lua_gettop (lua_current_interpreter);

    if (n < 2)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(LUA_CURRENT_SCRIPT_NAME, "mkdir_home");
        LUA_RETURN_ERROR;
    }
    
    directory = lua_tostring (lua_current_interpreter, -2);
    mode = lua_tonumber (lua_current_interpreter, -1);
    
    if (weechat_mkdir_home (directory, mode))
        LUA_RETURN_OK;
    
    LUA_RETURN_OK;
}

/*
 * weechat_lua_api_mkdir: create a directory
 */

static int
weechat_lua_api_mkdir (lua_State *L)
{
    const char *directory;
    int mode, n;
    
    /* make C compiler happy */
    (void) L;
     
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(LUA_CURRENT_SCRIPT_NAME, "mkdir");
        LUA_RETURN_ERROR;
    }
    
    directory = NULL;
    mode = 0;
    
    n = lua_gettop (lua_current_interpreter);

    if (n < 2)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(LUA_CURRENT_SCRIPT_NAME, "mkdir");
        LUA_RETURN_ERROR;
    }
    
    directory = lua_tostring (lua_current_interpreter, -2);
    mode = lua_tonumber (lua_current_interpreter, -1);
    
    if (weechat_mkdir (directory, mode))
        LUA_RETURN_OK;
    
    LUA_RETURN_OK;
}

/*
 * weechat_lua_api_mkdir_parents: create a directory and make parent
 *                                directories as needed
 */

static int
weechat_lua_api_mkdir_parents (lua_State *L)
{
    const char *directory;
    int mode, n;
    
    /* make C compiler happy */
    (void) L;
     
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(LUA_CURRENT_SCRIPT_NAME, "mkdir_parents");
        LUA_RETURN_ERROR;
    }
    
    directory = NULL;
    mode = 0;
    
    n = lua_gettop (lua_current_interpreter);

    if (n < 2)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(LUA_CURRENT_SCRIPT_NAME, "mkdir_parents");
        LUA_RETURN_ERROR;
    }
    
    directory = lua_tostring (lua_current_interpreter, -2);
    mode = lua_tonumber (lua_current_interpreter, -1);
    
    if (weechat_mkdir_parents (directory, mode))
        LUA_RETURN_OK;
    
    LUA_RETURN_OK;
}

/*
 * weechat_lua_api_list_new: create a new list
 */

static int
weechat_lua_api_list_new (lua_State *L)
{
    char *result;
    
    /* make C compiler happy */
    (void) L;
    
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(LUA_CURRENT_SCRIPT_NAME, "list_new");
        LUA_RETURN_EMPTY;
    }
    
    result = script_ptr2str (weechat_list_new ());
    
    LUA_RETURN_STRING_FREE(result);
}

/*
 * weechat_lua_api_list_add: add a string to list
 */

static int
weechat_lua_api_list_add (lua_State *L)
{
    const char *weelist, *data, *where, *user_data;
    char *result;
    int n;
    
    /* make C compiler happy */
    (void) L;
    
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(LUA_CURRENT_SCRIPT_NAME, "list_add");
        LUA_RETURN_EMPTY;
    }
    
    weelist = NULL;
    data = NULL;
    where = NULL;
    user_data = NULL;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 4)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(LUA_CURRENT_SCRIPT_NAME, "list_add");
        LUA_RETURN_EMPTY;
    }
    
    weelist = lua_tostring (lua_current_interpreter, -4);
    data = lua_tostring (lua_current_interpreter, -3);
    where = lua_tostring (lua_current_interpreter, -2);
    user_data = lua_tostring (lua_current_interpreter, -1);
    
    result = script_ptr2str (weechat_list_add (script_str2ptr (weelist),
                                               data,
                                               where,
                                               script_str2ptr (user_data)));
    
    LUA_RETURN_STRING_FREE(result);
}

/*
 * weechat_lua_api_list_search: search a string in list
 */

static int
weechat_lua_api_list_search (lua_State *L)
{
    const char *weelist, *data;
    char *result;
    int n;
    
    /* make C compiler happy */
    (void) L;
    
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(LUA_CURRENT_SCRIPT_NAME, "list_search");
        LUA_RETURN_EMPTY;
    }
    
    weelist = NULL;
    data = NULL;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 2)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(LUA_CURRENT_SCRIPT_NAME, "list_search");
        LUA_RETURN_EMPTY;
    }
    
    weelist = lua_tostring (lua_current_interpreter, -2);
    data = lua_tostring (lua_current_interpreter, -1);
    
    result = script_ptr2str (weechat_list_search (script_str2ptr (weelist),
                                                  data));
    
    LUA_RETURN_STRING_FREE(result);
}

/*
 * weechat_lua_api_list_casesearch: search a string in list (ignore case)
 */

static int
weechat_lua_api_list_casesearch (lua_State *L)
{
    const char *weelist, *data;
    char *result;
    int n;
    
    /* make C compiler happy */
    (void) L;
    
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(LUA_CURRENT_SCRIPT_NAME, "list_casesearch");
        LUA_RETURN_EMPTY;
    }
    
    weelist = NULL;
    data = NULL;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 2)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(LUA_CURRENT_SCRIPT_NAME, "list_casesearch");
        LUA_RETURN_EMPTY;
    }
    
    weelist = lua_tostring (lua_current_interpreter, -2);
    data = lua_tostring (lua_current_interpreter, -1);
    
    result = script_ptr2str (weechat_list_casesearch (script_str2ptr (weelist),
                                                      data));
    
    LUA_RETURN_STRING_FREE(result);
}

/*
 * weechat_lua_api_list_get: get item by position
 */

static int
weechat_lua_api_list_get (lua_State *L)
{
    const char *weelist;
    char *result;
    int position, n;
    
    /* make C compiler happy */
    (void) L;
    
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(LUA_CURRENT_SCRIPT_NAME, "list_get");
        LUA_RETURN_EMPTY;
    }
    
    weelist = NULL;
    position = 0;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 2)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(LUA_CURRENT_SCRIPT_NAME, "list_get");
        LUA_RETURN_EMPTY;
    }
    
    weelist = lua_tostring (lua_current_interpreter, -2);
    position = lua_tonumber (lua_current_interpreter, -1);
    
    result = script_ptr2str (weechat_list_get (script_str2ptr (weelist),
                                               position));
    
    LUA_RETURN_STRING_FREE(result);
}

/*
 * weechat_lua_api_list_set: set new value for item
 */

static int
weechat_lua_api_list_set (lua_State *L)
{
    const char *item, *new_value;
    int n;
    
    /* make C compiler happy */
    (void) L;
    
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(LUA_CURRENT_SCRIPT_NAME, "list_set");
        LUA_RETURN_ERROR;
    }
    
    item = NULL;
    new_value = NULL;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 2)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(LUA_CURRENT_SCRIPT_NAME, "list_set");
        LUA_RETURN_ERROR;
    }
    
    item = lua_tostring (lua_current_interpreter, -2);
    new_value = lua_tostring (lua_current_interpreter, -1);
    
    weechat_list_set (script_str2ptr (item),
                      new_value);
    
    LUA_RETURN_OK;
}

/*
 * weechat_lua_api_list_next: get next item
 */

static int
weechat_lua_api_list_next (lua_State *L)
{
    const char *item;
    char *result;
    int n;
    
    /* make C compiler happy */
    (void) L;
    
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(LUA_CURRENT_SCRIPT_NAME, "list_next");
        LUA_RETURN_EMPTY;
    }
    
    item = NULL;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 1)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(LUA_CURRENT_SCRIPT_NAME, "list_next");
        LUA_RETURN_EMPTY;
    }
    
    item = lua_tostring (lua_current_interpreter, -1);
    
    result = script_ptr2str (weechat_list_next (script_str2ptr (item)));
    
    LUA_RETURN_STRING_FREE(result);
}

/*
 * weechat_lua_api_list_prev: get previous item
 */

static int
weechat_lua_api_list_prev (lua_State *L)
{
    const char *item;
    char *result;
    int n;
    
    /* make C compiler happy */
    (void) L;
    
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(LUA_CURRENT_SCRIPT_NAME, "list_prev");
        LUA_RETURN_EMPTY;
    }
    
    item = NULL;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 1)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(LUA_CURRENT_SCRIPT_NAME, "list_prev");
        LUA_RETURN_EMPTY;
    }
    
    item = lua_tostring (lua_current_interpreter, -1);
    
    result = script_ptr2str (weechat_list_prev (script_str2ptr (item)));
    
    LUA_RETURN_STRING_FREE(result);
}

/*
 * weechat_lua_api_list_string: get string value of item
 */

static int
weechat_lua_api_list_string (lua_State *L)
{
    const char *item, *result;
    int n;
    
    /* make C compiler happy */
    (void) L;
    
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(LUA_CURRENT_SCRIPT_NAME, "list_string");
        LUA_RETURN_EMPTY;
    }
    
    item = NULL;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 1)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(LUA_CURRENT_SCRIPT_NAME, "list_string");
        LUA_RETURN_EMPTY;
    }
    
    item = lua_tostring (lua_current_interpreter, -1);
    
    result = weechat_list_string (script_str2ptr (item));
    
    LUA_RETURN_STRING(result);
}

/*
 * weechat_lua_api_list_size: get number of elements in list
 */

static int
weechat_lua_api_list_size (lua_State *L)
{
    const char *weelist;
    int n, size;
    
    /* make C compiler happy */
    (void) L;
    
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(LUA_CURRENT_SCRIPT_NAME, "list_size");
        LUA_RETURN_INT(0);
    }
    
    weelist = NULL;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 1)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(LUA_CURRENT_SCRIPT_NAME, "list_size");
        LUA_RETURN_INT(0);
    }
    
    weelist = lua_tostring (lua_current_interpreter, -1);
    
    size = weechat_list_size (script_str2ptr (weelist));
    
    LUA_RETURN_INT(size);
}

/*
 * weechat_lua_api_list_remove: remove item from list
 */

static int
weechat_lua_api_list_remove (lua_State *L)
{
    const char *weelist, *item;
    int n;
    
    /* make C compiler happy */
    (void) L;
    
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(LUA_CURRENT_SCRIPT_NAME, "list_remove");
        LUA_RETURN_ERROR;
    }
    
    weelist = NULL;
    item = NULL;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 2)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(LUA_CURRENT_SCRIPT_NAME, "list_remove");
        LUA_RETURN_ERROR;
    }
    
    weelist = lua_tostring (lua_current_interpreter, -2);
    item = lua_tostring (lua_current_interpreter, -1);
    
    weechat_list_remove (script_str2ptr (weelist),
                         script_str2ptr (item));
    
    LUA_RETURN_OK;
}

/*
 * weechat_lua_api_list_remove_all: remove all items from list
 */

static int
weechat_lua_api_list_remove_all (lua_State *L)
{
    const char *weelist;
    int n;
    
    /* make C compiler happy */
    (void) L;
    
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(LUA_CURRENT_SCRIPT_NAME, "list_remove_all");
        LUA_RETURN_ERROR;
    }
    
    weelist = NULL;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 1)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(LUA_CURRENT_SCRIPT_NAME, "list_remove_all");
        LUA_RETURN_ERROR;
    }
    
    weelist = lua_tostring (lua_current_interpreter, -1);
    
    weechat_list_remove_all (script_str2ptr (weelist));
    
    LUA_RETURN_OK;
}

/*
 * weechat_lua_api_list_free: free list
 */

static int
weechat_lua_api_list_free (lua_State *L)
{
    const char *weelist;
    int n;
    
    /* make C compiler happy */
    (void) L;
    
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(LUA_CURRENT_SCRIPT_NAME, "list_free");
        LUA_RETURN_ERROR;
    }
    
    weelist = NULL;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 1)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(LUA_CURRENT_SCRIPT_NAME, "list_free");
        LUA_RETURN_ERROR;
    }
    
    weelist = lua_tostring (lua_current_interpreter, -1);
    
    weechat_list_free (script_str2ptr (weelist));
    
    LUA_RETURN_OK;
}

/*
 * weechat_lua_api_config_reload_cb: callback for ccnfig reload
 */

int
weechat_lua_api_config_reload_cb (void *data,
                                  struct t_config_file *config_file)
{
    struct t_script_callback *script_callback;
    char *lua_argv[3], empty_arg[1] = { '\0' };
    int *rc, ret;
    
    script_callback = (struct t_script_callback *)data;
    
    if (script_callback && script_callback->function && script_callback->function[0])
    {
        lua_argv[0] = (script_callback->data) ? script_callback->data : empty_arg;
        lua_argv[1] = script_ptr2str (config_file);
        lua_argv[2] = NULL;
        
        rc = (int *) weechat_lua_exec (script_callback->script,
                                       WEECHAT_SCRIPT_EXEC_INT,
                                       script_callback->function,
                                       lua_argv);
        
        if (!rc)
            ret = WEECHAT_CONFIG_READ_FILE_NOT_FOUND;
        else
        {
            ret = *rc;
            free (rc);
        }
        if (lua_argv[1])
            free (lua_argv[1]);
        
        return ret;
    }
    
    return WEECHAT_CONFIG_READ_FILE_NOT_FOUND;
}

/*
 * weechat_lua_api_config_new: create a new configuration file
 */

static int
weechat_lua_api_config_new (lua_State *L)
{
    const char *name, *function, *data;
    char *result;
    int n;
    
    /* make C compiler happy */
    (void) L;
    
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(LUA_CURRENT_SCRIPT_NAME, "config_new");
        LUA_RETURN_EMPTY;
    }
    
    name = NULL;
    function = NULL;
    data = NULL;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 3)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(LUA_CURRENT_SCRIPT_NAME, "config_new");
        LUA_RETURN_EMPTY;
    }
    
    name = lua_tostring (lua_current_interpreter, -3);
    function = lua_tostring (lua_current_interpreter, -2);
    data = lua_tostring (lua_current_interpreter, -1);
    
    result = script_ptr2str (script_api_config_new (weechat_lua_plugin,
                                                    lua_current_script,
                                                    name,
                                                    &weechat_lua_api_config_reload_cb,
                                                    function,
                                                    data));
    
    LUA_RETURN_STRING_FREE(result);
}

/*
 * weechat_lua_api_config_read_cb: callback for reading option in section
 */

int
weechat_lua_api_config_read_cb (void *data,
                                struct t_config_file *config_file,
                                struct t_config_section *section,
                                const char *option_name, const char *value)
{
    struct t_script_callback *script_callback;
    char *lua_argv[6], empty_arg[1] = { '\0' };
    int *rc, ret;
    
    script_callback = (struct t_script_callback *)data;
    
    if (script_callback && script_callback->function && script_callback->function[0])
    {
        lua_argv[0] = (script_callback->data) ? script_callback->data : empty_arg;
        lua_argv[1] = script_ptr2str (config_file);
        lua_argv[2] = script_ptr2str (section);
        lua_argv[3] = (option_name) ? (char *)option_name : empty_arg;
        lua_argv[4] = (value) ? (char *)value : empty_arg;
        lua_argv[5] = NULL;
        
        rc = (int *) weechat_lua_exec (script_callback->script,
                                       WEECHAT_SCRIPT_EXEC_INT,
                                       script_callback->function,
                                       lua_argv);
        
        if (!rc)
            ret = WEECHAT_CONFIG_OPTION_SET_ERROR;
        else
        {
            ret = *rc;
            free (rc);
        }
        if (lua_argv[1])
            free (lua_argv[1]);
        if (lua_argv[2])
            free (lua_argv[2]);
        
        return ret;
    }
    
    return WEECHAT_CONFIG_OPTION_SET_ERROR;
}

/*
 * weechat_lua_api_config_section_write_cb: callback for writing section
 */

void
weechat_lua_api_config_section_write_cb (void *data,
                                         struct t_config_file *config_file,
                                         const char *section_name)
{
    struct t_script_callback *script_callback;
    char *lua_argv[4], empty_arg[1] = { '\0' };
    int *rc;
    
    script_callback = (struct t_script_callback *)data;
    
    if (script_callback && script_callback->function && script_callback->function[0])
    {
        lua_argv[0] = (script_callback->data) ? script_callback->data : empty_arg;
        lua_argv[1] = script_ptr2str (config_file);
        lua_argv[2] = (section_name) ? (char *)section_name : empty_arg;
        lua_argv[3] = NULL;
        
        rc = (int *) weechat_lua_exec (script_callback->script,
                                       WEECHAT_SCRIPT_EXEC_INT,
                                       script_callback->function,
                                       lua_argv);
        
        if (rc)
            free (rc);
        if (lua_argv[1])
            free (lua_argv[1]);
    }
}

/*
 * weechat_lua_api_config_section_write_default_cb: callback for writing
 *                                                  default values for section
 */

void
weechat_lua_api_config_section_write_default_cb (void *data,
                                                 struct t_config_file *config_file,
                                                 const char *section_name)
{
    struct t_script_callback *script_callback;
    char *lua_argv[4], empty_arg[1] = { '\0' };
    int *rc;
    
    script_callback = (struct t_script_callback *)data;
    
    if (script_callback && script_callback->function && script_callback->function[0])
    {
        lua_argv[0] = (script_callback->data) ? script_callback->data : empty_arg;
        lua_argv[1] = script_ptr2str (config_file);
        lua_argv[2] = (section_name) ? (char *)section_name : empty_arg;
        lua_argv[3] = NULL;
        
        rc = (int *) weechat_lua_exec (script_callback->script,
                                       WEECHAT_SCRIPT_EXEC_INT,
                                       script_callback->function,
                                       lua_argv);
        
        if (rc)
            free (rc);
        if (lua_argv[1])
            free (lua_argv[1]);
    }
}

/*
 * weechat_lua_api_config_section_create_option_cb: callback to create an option
 */

int
weechat_lua_api_config_section_create_option_cb (void *data,
                                                 struct t_config_file *config_file,
                                                 struct t_config_section *section,
                                                 const char *option_name,
                                                 const char *value)
{
    struct t_script_callback *script_callback;
    char *lua_argv[6], empty_arg[1] = { '\0' };
    int *rc, ret;
    
    script_callback = (struct t_script_callback *)data;
    
    if (script_callback && script_callback->function && script_callback->function[0])
    {
        lua_argv[0] = (script_callback->data) ? script_callback->data : empty_arg;
        lua_argv[1] = script_ptr2str (config_file);
        lua_argv[2] = script_ptr2str (section);
        lua_argv[3] = (option_name) ? (char *)option_name : empty_arg;
        lua_argv[4] = (value) ? (char *)value : empty_arg;
        lua_argv[5] = NULL;
        
        rc = (int *) weechat_lua_exec (script_callback->script,
                                       WEECHAT_SCRIPT_EXEC_INT,
                                       script_callback->function,
                                       lua_argv);
        
        if (!rc)
            ret = WEECHAT_CONFIG_OPTION_SET_ERROR;
        else
        {
            ret = *rc;
            free (rc);
        }
        if (lua_argv[1])
            free (lua_argv[1]);
        if (lua_argv[2])
            free (lua_argv[2]);
        
        return ret;
    }
    
    return WEECHAT_CONFIG_OPTION_SET_ERROR;
}

/*
 * weechat_lua_api_config_section_delete_option_cb: callback to delete an option
 */

int
weechat_lua_api_config_section_delete_option_cb (void *data,
                                                 struct t_config_file *config_file,
                                                 struct t_config_section *section,
                                                 struct t_config_option *option)
{
    struct t_script_callback *script_callback;
    char *lua_argv[5], empty_arg[1] = { '\0' };
    int *rc, ret;
    
    script_callback = (struct t_script_callback *)data;
    
    if (script_callback && script_callback->function && script_callback->function[0])
    {
        lua_argv[0] = (script_callback->data) ? script_callback->data : empty_arg;
        lua_argv[1] = script_ptr2str (config_file);
        lua_argv[2] = script_ptr2str (section);
        lua_argv[3] = script_ptr2str (option);
        lua_argv[4] = NULL;
        
        rc = (int *) weechat_lua_exec (script_callback->script,
                                       WEECHAT_SCRIPT_EXEC_INT,
                                       script_callback->function,
                                       lua_argv);
        
        if (!rc)
            ret = WEECHAT_CONFIG_OPTION_UNSET_ERROR;
        else
        {
            ret = *rc;
            free (rc);
        }
        if (lua_argv[1])
            free (lua_argv[1]);
        if (lua_argv[2])
            free (lua_argv[2]);
        if (lua_argv[3])
            free (lua_argv[3]);
        
        return ret;
    }
    
    return WEECHAT_CONFIG_OPTION_UNSET_ERROR;
}

/*
 * weechat_lua_api_config_new_section: create a new section in configuration file
 */

static int
weechat_lua_api_config_new_section (lua_State *L)
{
    const char *config_file, *name, *function_read, *data_read;
    const char *function_write, *data_write, *function_write_default;
    const char *data_write_default, *function_create_option;
    const char *data_create_option, *function_delete_option;
    const char *data_delete_option;
    char *result;
    int n, user_can_add_options, user_can_delete_options;
    
    /* make C compiler happy */
    (void) L;
    
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(LUA_CURRENT_SCRIPT_NAME, "config_new_section");
        LUA_RETURN_EMPTY;
    }
    
    config_file = NULL;
    name = NULL;
    user_can_add_options = 0;
    user_can_delete_options = 0;
    function_read = NULL;
    data_read = NULL;
    function_write = NULL;
    data_write = NULL;
    function_write_default = NULL;
    data_write_default = NULL;
    function_create_option = NULL;
    data_create_option = NULL;
    function_delete_option = NULL;
    data_delete_option = NULL;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 14)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(LUA_CURRENT_SCRIPT_NAME, "config_new_section");
        LUA_RETURN_EMPTY;
    }
    
    config_file = lua_tostring (lua_current_interpreter, -14);
    name = lua_tostring (lua_current_interpreter, -13);
    user_can_add_options = lua_tonumber (lua_current_interpreter, -12);
    user_can_delete_options = lua_tonumber (lua_current_interpreter, -11);
    function_read = lua_tostring (lua_current_interpreter, -10);
    data_read = lua_tostring (lua_current_interpreter, -9);
    function_write = lua_tostring (lua_current_interpreter, -8);
    data_write = lua_tostring (lua_current_interpreter, -7);
    function_write_default = lua_tostring (lua_current_interpreter, -6);
    data_write_default = lua_tostring (lua_current_interpreter, -5);
    function_create_option = lua_tostring (lua_current_interpreter, -4);
    data_create_option = lua_tostring (lua_current_interpreter, -3);
    function_delete_option = lua_tostring (lua_current_interpreter, -2);
    data_delete_option = lua_tostring (lua_current_interpreter, -1);
    
    result = script_ptr2str (script_api_config_new_section (weechat_lua_plugin,
                                                            lua_current_script,
                                                            script_str2ptr (config_file),
                                                            name,
                                                            user_can_add_options,
                                                            user_can_delete_options,
                                                            &weechat_lua_api_config_read_cb,
                                                            function_read,
                                                            data_read,
                                                            &weechat_lua_api_config_section_write_cb,
                                                            function_write,
                                                            data_write,
                                                            &weechat_lua_api_config_section_write_default_cb,
                                                            function_write_default,
                                                            data_write_default,
                                                            &weechat_lua_api_config_section_create_option_cb,
                                                            function_create_option,
                                                            data_create_option,
                                                            &weechat_lua_api_config_section_delete_option_cb,
                                                            function_delete_option,
                                                            data_delete_option));
    
    LUA_RETURN_STRING_FREE(result);
}

/*
 * weechat_lua_api_config_search_section: search a section in configuration file
 */

static int
weechat_lua_api_config_search_section (lua_State *L)
{
    const char *config_file, *section_name;
    char *result;
    int n;
    
    /* make C compiler happy */
    (void) L;
    
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(LUA_CURRENT_SCRIPT_NAME, "config_search_section");
        LUA_RETURN_EMPTY;
    }
    
    config_file = NULL;
    section_name = NULL;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 2)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(LUA_CURRENT_SCRIPT_NAME, "config_search_section");
        LUA_RETURN_EMPTY;
    }
    
    config_file = lua_tostring (lua_current_interpreter, -2);
    section_name = lua_tostring (lua_current_interpreter, -1);
    
    result = script_ptr2str (weechat_config_search_section (script_str2ptr (config_file),
                                                            section_name));
    
    LUA_RETURN_STRING_FREE(result);
}

/*
 * weechat_lua_api_config_option_check_value_cb: callback for checking new
 *                                               value for option
 */

int
weechat_lua_api_config_option_check_value_cb (void *data,
                                              struct t_config_option *option,
                                              const char *value)
{
    struct t_script_callback *script_callback;
    char *lua_argv[4], empty_arg[1] = { '\0' };
    int *rc, ret;
    
    script_callback = (struct t_script_callback *)data;
    
    if (script_callback && script_callback->function && script_callback->function[0])
    {
        lua_argv[0] = (script_callback->data) ? script_callback->data : empty_arg;
        lua_argv[1] = script_ptr2str (option);
        lua_argv[2] = (value) ? (char *)value : empty_arg;
        lua_argv[3] = NULL;
        
        rc = (int *) weechat_lua_exec (script_callback->script,
                                       WEECHAT_SCRIPT_EXEC_INT,
                                       script_callback->function,
                                       lua_argv);
        
        if (!rc)
            ret = 0;
        else
        {
            ret = *rc;
            free (rc);
        }
        if (lua_argv[1])
            free (lua_argv[1]);
        
        return ret;
    }
    
    return 0;
}

/*
 * weechat_lua_api_config_option_change_cb: callback for option changed
 */

void
weechat_lua_api_config_option_change_cb (void *data,
                                         struct t_config_option *option)
{
    struct t_script_callback *script_callback;
    char *lua_argv[3], empty_arg[1] = { '\0' };
    int *rc;
    
    script_callback = (struct t_script_callback *)data;
    
    if (script_callback && script_callback->function && script_callback->function[0])
    {
        lua_argv[0] = (script_callback->data) ? script_callback->data : empty_arg;
        lua_argv[1] = script_ptr2str (option);
        lua_argv[2] = NULL;
        
        rc = (int *) weechat_lua_exec (script_callback->script,
                                       WEECHAT_SCRIPT_EXEC_INT,
                                       script_callback->function,
                                       lua_argv);
        
        if (lua_argv[1])
            free (lua_argv[1]);
        
        if (rc)
            free (rc);
    }
}

/*
 * weechat_lua_api_config_option_delete_cb: callback when option is deleted
 */

void
weechat_lua_api_config_option_delete_cb (void *data,
                                         struct t_config_option *option)
{
    struct t_script_callback *script_callback;
    char *lua_argv[3], empty_arg[1] = { '\0' };
    int *rc;
    
    script_callback = (struct t_script_callback *)data;
    
    if (script_callback && script_callback->function && script_callback->function[0])
    {
        lua_argv[0] = (script_callback->data) ? script_callback->data : empty_arg;
        lua_argv[1] = script_ptr2str (option);
        lua_argv[2] = NULL;
        
        rc = (int *) weechat_lua_exec (script_callback->script,
                                       WEECHAT_SCRIPT_EXEC_INT,
                                       script_callback->function,
                                       lua_argv);
        
        if (lua_argv[1])
            free (lua_argv[1]);
        
        if (rc)
            free (rc);
    }
}

/*
 * weechat_lua_api_config_new_option: create a new option in section
 */

static int
weechat_lua_api_config_new_option (lua_State *L)
{
    const char *config_file, *section, *name, *type, *description;
    const char *string_values, *default_value, *value;
    const char *function_check_value, *data_check_value, *function_change;
    const char *data_change, *function_delete, *data_delete;
    char *result;
    int n, min, max, null_value_allowed;
    
    /* make C compiler happy */
    (void) L;
    
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(LUA_CURRENT_SCRIPT_NAME, "config_new_option");
        LUA_RETURN_EMPTY;
    }
    
    config_file = NULL;
    section = NULL;
    name = NULL;
    type = NULL;
    description = NULL;
    string_values = NULL;
    min = 0;
    max = 0;
    default_value = NULL;
    value = NULL;
    null_value_allowed = 0;
    function_check_value = NULL;
    data_check_value = NULL;
    function_change = NULL;
    data_change = NULL;
    function_delete = NULL;
    data_delete = NULL;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 17)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(LUA_CURRENT_SCRIPT_NAME, "config_new_option");
        LUA_RETURN_EMPTY;
    }
    
    config_file = lua_tostring (lua_current_interpreter, -17);
    section = lua_tostring (lua_current_interpreter, -16);
    name = lua_tostring (lua_current_interpreter, -15);
    type = lua_tostring (lua_current_interpreter, -14);
    description = lua_tostring (lua_current_interpreter, -13);
    string_values = lua_tostring (lua_current_interpreter, -12);
    min = lua_tonumber (lua_current_interpreter, -11);
    max = lua_tonumber (lua_current_interpreter, -10);
    default_value = lua_tostring (lua_current_interpreter, -9);
    value = lua_tostring (lua_current_interpreter, -8);
    null_value_allowed = lua_tonumber (lua_current_interpreter, -7);
    function_check_value = lua_tostring (lua_current_interpreter, -6);
    data_check_value = lua_tostring (lua_current_interpreter, -5);
    function_change = lua_tostring (lua_current_interpreter, -4);
    data_change = lua_tostring (lua_current_interpreter, -3);
    function_delete = lua_tostring (lua_current_interpreter, -2);
    data_delete = lua_tostring (lua_current_interpreter, -1);
    
    result = script_ptr2str (script_api_config_new_option (weechat_lua_plugin,
                                                           lua_current_script,
                                                           script_str2ptr (config_file),
                                                           script_str2ptr (section),
                                                           name,
                                                           type,
                                                           description,
                                                           string_values,
                                                           min,
                                                           max,
                                                           default_value,
                                                           value,
                                                           null_value_allowed,
                                                           &weechat_lua_api_config_option_check_value_cb,
                                                           function_check_value,
                                                           data_check_value,
                                                           &weechat_lua_api_config_option_change_cb,
                                                           function_change,
                                                           data_change,
                                                           &weechat_lua_api_config_option_delete_cb,
                                                           function_delete,
                                                           data_delete));
    
    LUA_RETURN_STRING_FREE(result);
}

/*
 * weechat_lua_api_config_search_option: search option in configuration file or section
 */

static int
weechat_lua_api_config_search_option (lua_State *L)
{
    const char *config_file, *section, *option_name;
    char *result;
    int n;
    
    /* make C compiler happy */
    (void) L;
    
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(LUA_CURRENT_SCRIPT_NAME, "config_search_option");
        LUA_RETURN_EMPTY;
    }
    
    config_file = NULL;
    section = NULL;
    option_name = NULL;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 3)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(LUA_CURRENT_SCRIPT_NAME, "config_search_option");
        LUA_RETURN_EMPTY;
    }
    
    config_file = lua_tostring (lua_current_interpreter, -3);
    section = lua_tostring (lua_current_interpreter, -2);
    option_name = lua_tostring (lua_current_interpreter, -1);
    
    result = script_ptr2str (weechat_config_search_option (script_str2ptr (config_file),
                                                           script_str2ptr (section),
                                                           option_name));
    
    LUA_RETURN_STRING_FREE(result);
}

/*
 * weechat_lua_api_config_string_to_boolean: return boolean value of a string
 */

static int
weechat_lua_api_config_string_to_boolean (lua_State *L)
{
    const char *text;
    int n, value;
    
    /* make C compiler happy */
    (void) L;
    
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(LUA_CURRENT_SCRIPT_NAME, "config_string_to_boolean");
        LUA_RETURN_INT(0);
    }
    
    text = NULL;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 1)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(LUA_CURRENT_SCRIPT_NAME, "config_string_to_boolean");
        LUA_RETURN_INT(0);
    }
    
    text = lua_tostring (lua_current_interpreter, -1);
    
    value = weechat_config_string_to_boolean (text);
    
    LUA_RETURN_INT(value);
}

/*
 * weechat_lua_api_config_option_reset: reset option with default value
 */

static int
weechat_lua_api_config_option_reset (lua_State *L)
{
    const char *option;
    int n, run_callback, rc;
    
    /* make C compiler happy */
    (void) L;
    
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(LUA_CURRENT_SCRIPT_NAME, "config_option_reset");
        LUA_RETURN_INT(0);
    }
    
    option = NULL;
    run_callback = 0;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 2)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(LUA_CURRENT_SCRIPT_NAME, "config_option_reset");
        LUA_RETURN_INT(0);
    }
    
    option = lua_tostring (lua_current_interpreter, -2);
    run_callback = lua_tonumber (lua_current_interpreter, -1);
    
    rc = weechat_config_option_reset (script_str2ptr (option),
                                      run_callback);
    
    LUA_RETURN_INT(rc);
}

/*
 * weechat_lua_api_config_option_set: set new value for option
 */

static int
weechat_lua_api_config_option_set (lua_State *L)
{
    const char *option, *new_value;
    int n, run_callback, rc;
    
    /* make C compiler happy */
    (void) L;
    
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(LUA_CURRENT_SCRIPT_NAME, "config_option_set");
        LUA_RETURN_INT(WEECHAT_CONFIG_OPTION_SET_ERROR);
    }
    
    option = NULL;
    new_value = NULL;
    run_callback = 0;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 3)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(LUA_CURRENT_SCRIPT_NAME, "config_option_set");
        LUA_RETURN_INT(WEECHAT_CONFIG_OPTION_SET_ERROR);
    }
    
    option = lua_tostring (lua_current_interpreter, -3);
    new_value = lua_tostring (lua_current_interpreter, -2);
    run_callback = lua_tonumber (lua_current_interpreter, -1);
    
    rc = weechat_config_option_set (script_str2ptr (option),
                                    new_value,
                                    run_callback);
    
    LUA_RETURN_INT(rc);
}

/*
 * weechat_lua_api_config_option_set_null: set null (undefined) value for
 *                                         option
 */

static int
weechat_lua_api_config_option_set_null (lua_State *L)
{
    const char *option;
    int n, run_callback, rc;
    
    /* make C compiler happy */
    (void) L;
    
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(LUA_CURRENT_SCRIPT_NAME, "config_option_set_null");
        LUA_RETURN_INT(WEECHAT_CONFIG_OPTION_SET_ERROR);
    }
    
    option = NULL;
    run_callback = 0;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 2)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(LUA_CURRENT_SCRIPT_NAME, "config_option_set_null");
        LUA_RETURN_INT(WEECHAT_CONFIG_OPTION_SET_ERROR);
    }
    
    option = lua_tostring (lua_current_interpreter, -2);
    run_callback = lua_tonumber (lua_current_interpreter, -1);
    
    rc = weechat_config_option_set_null (script_str2ptr (option),
                                         run_callback);
    
    LUA_RETURN_INT(rc);
}

/*
 * weechat_lua_api_config_option_unset: unset an option
 */

static int
weechat_lua_api_config_option_unset (lua_State *L)
{
    const char *option;
    int n, rc;
    
    /* make C compiler happy */
    (void) L;
    
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(LUA_CURRENT_SCRIPT_NAME, "config_option_unset");
        LUA_RETURN_INT(WEECHAT_CONFIG_OPTION_UNSET_ERROR);
    }
    
    option = NULL;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 1)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(LUA_CURRENT_SCRIPT_NAME, "config_option_unset");
        LUA_RETURN_INT(WEECHAT_CONFIG_OPTION_UNSET_ERROR);
    }
    
    option = lua_tostring (lua_current_interpreter, -1);
    
    rc = weechat_config_option_unset (script_str2ptr (option));
    
    LUA_RETURN_INT(rc);
}

/*
 * weechat_lua_api_config_option_rename: rename an option
 */

static int
weechat_lua_api_config_option_rename (lua_State *L)
{
    const char *option, *new_name;
    int n;
    
    /* make C compiler happy */
    (void) L;
    
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(LUA_CURRENT_SCRIPT_NAME, "config_option_rename");
        LUA_RETURN_ERROR;
    }
    
    option = NULL;
    new_name = NULL;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 2)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(LUA_CURRENT_SCRIPT_NAME, "config_option_rename");
        LUA_RETURN_ERROR;
    }
    
    option = lua_tostring (lua_current_interpreter, -2);
    new_name = lua_tostring (lua_current_interpreter, -1);
    
    weechat_config_option_rename (script_str2ptr (option),
                                  new_name);
    
    LUA_RETURN_OK;
}

/*
 * weechat_lua_api_config_option_is_null: return 1 if value of option is null
 */

static int
weechat_lua_api_config_option_is_null (lua_State *L)
{
    const char *option;
    int n, value;
    
    /* make C compiler happy */
    (void) L;
    
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(LUA_CURRENT_SCRIPT_NAME, "config_option_is_null");
        LUA_RETURN_INT(1);
    }
    
    option = NULL;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 1)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(LUA_CURRENT_SCRIPT_NAME, "config_option_is_null");
        LUA_RETURN_INT(1);
    }
    
    option = lua_tostring (lua_current_interpreter, -1);
    
    value = weechat_config_option_is_null (script_str2ptr (option));
    
    LUA_RETURN_INT(value);
}

/*
 * weechat_lua_api_config_option_default_is_null: return 1 if default value of
 *                                                option is null
 */

static int
weechat_lua_api_config_option_default_is_null (lua_State *L)
{
    const char *option;
    int n, value;
    
    /* make C compiler happy */
    (void) L;
    
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(LUA_CURRENT_SCRIPT_NAME, "config_option_default_is_null");
        LUA_RETURN_INT(1);
    }
    
    option = NULL;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 1)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(LUA_CURRENT_SCRIPT_NAME, "config_option_default_is_null");
        LUA_RETURN_INT(1);
    }
    
    option = lua_tostring (lua_current_interpreter, -1);
    
    value = weechat_config_option_default_is_null (script_str2ptr (option));
    
    LUA_RETURN_INT(value);
}

/*
 * weechat_lua_api_config_boolean: return boolean value of option
 */

static int
weechat_lua_api_config_boolean (lua_State *L)
{
    const char *option;
    int n, value;
    
    /* make C compiler happy */
    (void) L;
    
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(LUA_CURRENT_SCRIPT_NAME, "config_boolean");
        LUA_RETURN_INT(0);
    }
    
    option = NULL;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 1)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(LUA_CURRENT_SCRIPT_NAME, "config_boolean");
        LUA_RETURN_INT(0);
    }
    
    option = lua_tostring (lua_current_interpreter, -1);
    
    value = weechat_config_boolean (script_str2ptr (option));
    
    LUA_RETURN_INT(value);
}

/*
 * weechat_lua_api_config_boolean_default: return default boolean value of option
 */

static int
weechat_lua_api_config_boolean_default (lua_State *L)
{
    const char *option;
    int n, value;
    
    /* make C compiler happy */
    (void) L;
    
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(LUA_CURRENT_SCRIPT_NAME, "config_boolean_default");
        LUA_RETURN_INT(0);
    }
    
    option = NULL;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 1)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(LUA_CURRENT_SCRIPT_NAME, "config_boolean_default");
        LUA_RETURN_INT(0);
    }
    
    option = lua_tostring (lua_current_interpreter, -1);
    
    value = weechat_config_boolean_default (script_str2ptr (option));
    
    LUA_RETURN_INT(value);
}

/*
 * weechat_lua_api_config_integer: return integer value of option
 */

static int
weechat_lua_api_config_integer (lua_State *L)
{
    const char *option;
    int n, value;
    
    /* make C compiler happy */
    (void) L;
    
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(LUA_CURRENT_SCRIPT_NAME, "config_integer");
        LUA_RETURN_INT(0);
    }
    
    option = NULL;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 1)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(LUA_CURRENT_SCRIPT_NAME, "config_integer");
        LUA_RETURN_INT(0);
    }
    
    option = lua_tostring (lua_current_interpreter, -1);
    
    value = weechat_config_integer (script_str2ptr (option));
    
    LUA_RETURN_INT(value);
}

/*
 * weechat_lua_api_config_integer_default: return default integer value of option
 */

static int
weechat_lua_api_config_integer_default (lua_State *L)
{
    const char *option;
    int n, value;
    
    /* make C compiler happy */
    (void) L;
    
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(LUA_CURRENT_SCRIPT_NAME, "config_integer_default");
        LUA_RETURN_INT(0);
    }
    
    option = NULL;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 1)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(LUA_CURRENT_SCRIPT_NAME, "config_integer_default");
        LUA_RETURN_INT(0);
    }
    
    option = lua_tostring (lua_current_interpreter, -1);
    
    value = weechat_config_integer_default (script_str2ptr (option));
    
    LUA_RETURN_INT(value);
}

/*
 * weechat_lua_api_config_string: return string value of option
 */

static int
weechat_lua_api_config_string (lua_State *L)
{
    const char *option, *result;
    int n;
    
    /* make C compiler happy */
    (void) L;
    
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(LUA_CURRENT_SCRIPT_NAME, "config_string");
        LUA_RETURN_EMPTY;
    }
    
    option = NULL;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 1)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(LUA_CURRENT_SCRIPT_NAME, "config_string");
        LUA_RETURN_INT(0);
    }
    
    option = lua_tostring (lua_current_interpreter, -1);
    
    result = weechat_config_string (script_str2ptr (option));
    
    LUA_RETURN_STRING(result);
}

/*
 * weechat_lua_api_config_string_default: return default string value of option
 */

static int
weechat_lua_api_config_string_default (lua_State *L)
{
    const char *option, *result;
    int n;
    
    /* make C compiler happy */
    (void) L;
    
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(LUA_CURRENT_SCRIPT_NAME, "config_string_default");
        LUA_RETURN_EMPTY;
    }
    
    option = NULL;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 1)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(LUA_CURRENT_SCRIPT_NAME, "config_string_default");
        LUA_RETURN_INT(0);
    }
    
    option = lua_tostring (lua_current_interpreter, -1);
    
    result = weechat_config_string_default (script_str2ptr (option));
    
    LUA_RETURN_STRING(result);
}

/*
 * weechat_lua_api_config_color: return color value of option
 */

static int
weechat_lua_api_config_color (lua_State *L)
{
    const char *option, *result;
    int n;
    
    /* make C compiler happy */
    (void) L;
    
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(LUA_CURRENT_SCRIPT_NAME, "config_color");
        LUA_RETURN_INT(0);
    }
    
    option = NULL;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 1)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(LUA_CURRENT_SCRIPT_NAME, "config_color");
        LUA_RETURN_INT(0);
    }
    
    option = lua_tostring (lua_current_interpreter, -1);
    
    result = weechat_config_color (script_str2ptr (option));
    
    LUA_RETURN_STRING(result);
}

/*
 * weechat_lua_api_config_color_default: return default color value of option
 */

static int
weechat_lua_api_config_color_default (lua_State *L)
{
    const char *option, *result;
    int n;
    
    /* make C compiler happy */
    (void) L;
    
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(LUA_CURRENT_SCRIPT_NAME, "config_color_default");
        LUA_RETURN_INT(0);
    }
    
    option = NULL;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 1)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(LUA_CURRENT_SCRIPT_NAME, "config_color_default");
        LUA_RETURN_INT(0);
    }
    
    option = lua_tostring (lua_current_interpreter, -1);
    
    result = weechat_config_color_default (script_str2ptr (option));
    
    LUA_RETURN_STRING(result);
}

/*
 * weechat_lua_api_config_write_option: write an option in configuration file
 */

static int
weechat_lua_api_config_write_option (lua_State *L)
{
    const char *config_file, *option;
    int n;
    
    /* make C compiler happy */
    (void) L;
    
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(LUA_CURRENT_SCRIPT_NAME, "config_write_option");
        LUA_RETURN_ERROR;
    }
    
    config_file = NULL;
    option = NULL;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 2)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(LUA_CURRENT_SCRIPT_NAME, "config_write_option");
        LUA_RETURN_ERROR;
    }
    
    config_file = lua_tostring (lua_current_interpreter, -2);
    option = lua_tostring (lua_current_interpreter, -1);
    
    weechat_config_write_option (script_str2ptr (config_file),
                                 script_str2ptr (option));
    
    LUA_RETURN_OK;
}

/*
 * weechat_lua_api_config_write_line: write a line in configuration file
 */

static int
weechat_lua_api_config_write_line (lua_State *L)
{
    const char *config_file, *option_name, *value;
    int n;
    
    /* make C compiler happy */
    (void) L;
    
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(LUA_CURRENT_SCRIPT_NAME, "config_write_line");
        LUA_RETURN_ERROR;
    }
    
    config_file = NULL;
    option_name = NULL;
    value = NULL;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 3)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(LUA_CURRENT_SCRIPT_NAME, "config_write_line");
        LUA_RETURN_ERROR;
    }
    
    config_file = lua_tostring (lua_current_interpreter, -3);
    option_name = lua_tostring (lua_current_interpreter, -2);
    value = lua_tostring (lua_current_interpreter, -1);
    
    weechat_config_write_line (script_str2ptr (config_file),
                               option_name,
                               "%s",
                               value);
    
    LUA_RETURN_OK;
}

/*
 * weechat_lua_api_config_write: write configuration file
 */

static int
weechat_lua_api_config_write (lua_State *L)
{
    const char *config_file;
    int n, rc;
    
    /* make C compiler happy */
    (void) L;
    
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(LUA_CURRENT_SCRIPT_NAME, "config_write");
        LUA_RETURN_INT(-1);
    }
    
    config_file = NULL;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 1)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(LUA_CURRENT_SCRIPT_NAME, "config_write");
        LUA_RETURN_INT(-1);
    }
    
    config_file = lua_tostring (lua_current_interpreter, -1);
    
    rc = weechat_config_write (script_str2ptr (config_file));
    
    LUA_RETURN_INT(rc);
}

/*
 * weechat_lua_api_config_read: read configuration file
 */

static int
weechat_lua_api_config_read (lua_State *L)
{
    const char *config_file;
    int n, rc;
    
    /* make C compiler happy */
    (void) L;
    
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(LUA_CURRENT_SCRIPT_NAME, "config_read");
        LUA_RETURN_INT(-1);
    }
    
    config_file = NULL;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 1)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(LUA_CURRENT_SCRIPT_NAME, "config_read");
        LUA_RETURN_INT(-1);
    }
    
    config_file = lua_tostring (lua_current_interpreter, -1);
    
    rc = weechat_config_read (script_str2ptr (config_file));
    
    LUA_RETURN_INT(rc);
}

/*
 * weechat_lua_api_config_reload: reload configuration file
 */

static int
weechat_lua_api_config_reload (lua_State *L)
{
    const char *config_file;
    int n, rc;
    
    /* make C compiler happy */
    (void) L;
    
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(LUA_CURRENT_SCRIPT_NAME, "config_reload");
        LUA_RETURN_INT(-1);
    }
    
    config_file = NULL;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 1)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(LUA_CURRENT_SCRIPT_NAME, "config_reload");
        LUA_RETURN_INT(-1);
    }
    
    config_file = lua_tostring (lua_current_interpreter, -1);
    
    rc = weechat_config_reload (script_str2ptr (config_file));
    
    LUA_RETURN_INT(rc);
}

/*
 * weechat_lua_api_config_option_free: free an option in configuration file
 */

static int
weechat_lua_api_config_option_free (lua_State *L)
{
    const char *option;
    int n;
    
    /* make C compiler happy */
    (void) L;
    
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(LUA_CURRENT_SCRIPT_NAME, "config_option_free");
        LUA_RETURN_ERROR;
    }
    
    option = NULL;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 1)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(LUA_CURRENT_SCRIPT_NAME, "config_option_free");
        LUA_RETURN_ERROR;
    }
    
    option = lua_tostring (lua_current_interpreter, -1);
    
    script_api_config_option_free (weechat_lua_plugin,
                                   lua_current_script,
                                   script_str2ptr (option));
    
    LUA_RETURN_OK;
}

/*
 * weechat_lua_api_config_section_free_options: free all options of a section
 *                                              in configuration file
 */

static int
weechat_lua_api_config_section_free_options (lua_State *L)
{
    const char *section;
    int n;
    
    /* make C compiler happy */
    (void) L;
    
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(LUA_CURRENT_SCRIPT_NAME, "config_section_free_options");
        LUA_RETURN_ERROR;
    }
    
    section = NULL;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 1)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(LUA_CURRENT_SCRIPT_NAME, "config_section_free_options");
        LUA_RETURN_ERROR;
    }
    
    section = lua_tostring (lua_current_interpreter, -1);
    
    script_api_config_section_free_options (weechat_lua_plugin,
                                            lua_current_script,
                                            script_str2ptr (section));
    
    LUA_RETURN_OK;
}

/*
 * weechat_lua_api_config_section_free: free section in configuration file
 */

static int
weechat_lua_api_config_section_free (lua_State *L)
{
    const char *section;
    int n;
    
    /* make C compiler happy */
    (void) L;
    
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(LUA_CURRENT_SCRIPT_NAME, "config_section_free");
        LUA_RETURN_ERROR;
    }
    
    section = NULL;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 1)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(LUA_CURRENT_SCRIPT_NAME, "config_section_free");
        LUA_RETURN_ERROR;
    }
    
    section = lua_tostring (lua_current_interpreter, -1);
    
    script_api_config_section_free (weechat_lua_plugin,
                                    lua_current_script,
                                    script_str2ptr (section));
    
    LUA_RETURN_OK;
}

/*
 * weechat_lua_api_config_free: free configuration file
 */

static int
weechat_lua_api_config_free (lua_State *L)
{
    const char *config_file;
    int n;
    
    /* make C compiler happy */
    (void) L;
    
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(LUA_CURRENT_SCRIPT_NAME, "config_free");
        LUA_RETURN_ERROR;
    }
    
    config_file = NULL;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 1)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(LUA_CURRENT_SCRIPT_NAME, "config_free");
        LUA_RETURN_ERROR;
    }
    
    config_file = lua_tostring (lua_current_interpreter, -1);
    
    script_api_config_free (weechat_lua_plugin,
                            lua_current_script,
                            script_str2ptr (config_file));
    
    LUA_RETURN_OK;
}

/*
 * weechat_lua_api_config_get: get config option
 */

static int
weechat_lua_api_config_get (lua_State *L)
{
    const char *option;
    char *result;
    int n;
    
    /* make C compiler happy */
    (void) L;
    
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(LUA_CURRENT_SCRIPT_NAME, "config_get");
        LUA_RETURN_EMPTY;
    }
    
    option = NULL;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 1)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(LUA_CURRENT_SCRIPT_NAME, "config_get");
        LUA_RETURN_EMPTY;
    }
    
    option = lua_tostring (lua_current_interpreter, -1);
    
    result = script_ptr2str (weechat_config_get (option));
    
    LUA_RETURN_STRING_FREE(result);
}

/*
 * weechat_lua_api_config_get_plugin: get value of a plugin option
 */

static int
weechat_lua_api_config_get_plugin (lua_State *L)
{
    const char *option, *result;
    int n;
    
    /* make C compiler happy */
    (void) L;
    
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(LUA_CURRENT_SCRIPT_NAME, "config_get_plugin");
        LUA_RETURN_EMPTY;
    }
    
    option = NULL;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 1)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(LUA_CURRENT_SCRIPT_NAME, "config_get_plugin");
        LUA_RETURN_EMPTY;
    }
    
    option = lua_tostring (lua_current_interpreter, -1);
    
    result = script_api_config_get_plugin (weechat_lua_plugin,
                                           lua_current_script,
                                           option);
    
    LUA_RETURN_STRING(result);
}

/*
 * weechat_lua_api_config_is_set_plugin: check if a plugin option is set
 */

static int
weechat_lua_api_config_is_set_plugin (lua_State *L)
{
    const char *option;
    int n, rc;
    
    /* make C compiler happy */
    (void) L;
    
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(LUA_CURRENT_SCRIPT_NAME, "config_is_set_plugin");
        LUA_RETURN_INT(0);
    }
    
    option = NULL;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 1)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(LUA_CURRENT_SCRIPT_NAME, "config_is_set_plugin");
        LUA_RETURN_INT(0);
    }
    
    option = lua_tostring (lua_current_interpreter, -1);
    
    rc = script_api_config_is_set_plugin (weechat_lua_plugin,
                                          lua_current_script,
                                          option);
    
    LUA_RETURN_INT(rc);
}

/*
 * weechat_lua_api_config_set_plugin: set value of a plugin option
 */

static int
weechat_lua_api_config_set_plugin (lua_State *L)
{
    const char *option, *value;
    int n, rc;
    
    /* make C compiler happy */
    (void) L;
    
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(LUA_CURRENT_SCRIPT_NAME, "config_set_plugin");
        LUA_RETURN_INT(WEECHAT_CONFIG_OPTION_SET_ERROR);
    }
    
    option = NULL;
    value = NULL;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 2)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(LUA_CURRENT_SCRIPT_NAME, "config_set_plugin");
        LUA_RETURN_INT(WEECHAT_CONFIG_OPTION_SET_ERROR);
    }
    
    option = lua_tostring (lua_current_interpreter, -2);
    value = lua_tostring (lua_current_interpreter, -1);
    
    rc = script_api_config_set_plugin (weechat_lua_plugin,
                                       lua_current_script,
                                       option,
                                       value);
    
    LUA_RETURN_INT(rc);
}

/*
 * weechat_lua_api_config_unset_plugin: unset plugin option
 */

static int
weechat_lua_api_config_unset_plugin (lua_State *L)
{
    const char *option;
    int n, rc;
    
    /* make C compiler happy */
    (void) L;
    
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(LUA_CURRENT_SCRIPT_NAME, "config_unset_plugin");
        LUA_RETURN_INT(WEECHAT_CONFIG_OPTION_UNSET_ERROR);
    }
    
    option = NULL;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 1)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(LUA_CURRENT_SCRIPT_NAME, "config_unset_plugin");
        LUA_RETURN_INT(WEECHAT_CONFIG_OPTION_UNSET_ERROR);
    }
    
    option = lua_tostring (lua_current_interpreter, -1);
    
    rc = script_api_config_unset_plugin (weechat_lua_plugin,
                                         lua_current_script,
                                         option);
    
    LUA_RETURN_INT(rc);
}

/*
 * weechat_lua_api_prefix: get a prefix, used for display
 */

static int
weechat_lua_api_prefix (lua_State *L)
{
    const char *prefix, *result;
    int n;
    
    /* make C compiler happy */
    (void) L;
    
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(LUA_CURRENT_SCRIPT_NAME, "prefix");
        LUA_RETURN_EMPTY;
    }
    
    prefix = NULL;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 1)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(LUA_CURRENT_SCRIPT_NAME, "prefix");
        LUA_RETURN_EMPTY;
    }
    
    prefix = lua_tostring (lua_current_interpreter, -1);
    
    result = weechat_prefix (prefix);
    
    LUA_RETURN_STRING(result);
}

/*
 * weechat_lua_api_color: get a color code, used for display
 */

static int
weechat_lua_api_color (lua_State *L)
{
    const char *color, *result;
    int n;
    
    /* make C compiler happy */
    (void) L;
    
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(LUA_CURRENT_SCRIPT_NAME, "color");
        LUA_RETURN_EMPTY;
    }
    
    color = NULL;
    
    n = lua_gettop (lua_current_interpreter);

    if (n < 1)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(LUA_CURRENT_SCRIPT_NAME, "color");
        LUA_RETURN_EMPTY;
    }
    
    color = lua_tostring (lua_current_interpreter, -1);
    
    result = weechat_prefix (color);
    
    LUA_RETURN_STRING(result);
}

/*
 * weechat_lua_api_print: print message in a buffer
 */

static int
weechat_lua_api_print (lua_State *L)
{
    const char *buffer, *message;
    int n;
    
    /* make C compiler happy */
    (void) L;
    
    buffer = NULL;
    message = NULL;
    
    n = lua_gettop (lua_current_interpreter);

    if (n < 2)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(LUA_CURRENT_SCRIPT_NAME, "print");
        LUA_RETURN_ERROR;
    }
    
    buffer = lua_tostring (lua_current_interpreter, -2);
    message = lua_tostring (lua_current_interpreter, -1);
    
    script_api_printf (weechat_lua_plugin,
                       lua_current_script,
                       script_str2ptr (buffer),
                       "%s", message);
    
    LUA_RETURN_OK;
}

/*
 * weechat_lua_api_print_date_tags: print message in a buffer with optional
 *                                  date and tags
 */

static int
weechat_lua_api_print_date_tags (lua_State *L)
{
    const char *buffer, *tags, *message;
    int n, date;
    
    /* make C compiler happy */
    (void) L;
    
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(LUA_CURRENT_SCRIPT_NAME, "print_date_tags");
        LUA_RETURN_ERROR;
    }
    
    buffer = NULL;
    date = 0;
    tags = NULL;
    message = NULL;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 4)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(LUA_CURRENT_SCRIPT_NAME, "print_date_tags");
        LUA_RETURN_ERROR;
    }
    
    buffer = lua_tostring (lua_current_interpreter, -4);
    date = lua_tonumber (lua_current_interpreter, -3);
    tags = lua_tostring (lua_current_interpreter, -2);
    message = lua_tostring (lua_current_interpreter, -1);
    
    script_api_printf_date_tags (weechat_lua_plugin,
                                 lua_current_script,
                                 script_str2ptr (buffer),
                                 date,
                                 tags,
                                 "%s", message);
    
    LUA_RETURN_OK;
}

/*
 * weechat_lua_api_print_y: print message in a buffer with free content
 */

static int
weechat_lua_api_print_y (lua_State *L)
{
    const char *buffer, *message;
    int n, y;
    
    /* make C compiler happy */
    (void) L;
    
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(LUA_CURRENT_SCRIPT_NAME, "print_y");
        LUA_RETURN_ERROR;
    }
    
    buffer = NULL;
    y = 0;
    message = NULL;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 3)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(LUA_CURRENT_SCRIPT_NAME, "print_y");
        LUA_RETURN_ERROR;
    }
    
    buffer = lua_tostring (lua_current_interpreter, -3);
    y = lua_tonumber (lua_current_interpreter, -2);
    message = lua_tostring (lua_current_interpreter, -1);
    
    script_api_printf_y (weechat_lua_plugin,
                         lua_current_script,
                         script_str2ptr (buffer),
                         y,
                         "%s", message);
    
    LUA_RETURN_OK;
}

/*
 * weechat_lua_api_log_print: print message in WeeChat log file
 */

static int
weechat_lua_api_log_print (lua_State *L)
{
    const char *message;
    int n;
    
    /* make C compiler happy */
    (void) L;
    
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(LUA_CURRENT_SCRIPT_NAME, "log_print");
        LUA_RETURN_ERROR;
    }
    
    message = NULL;
    
    n = lua_gettop (lua_current_interpreter);

    if (n < 1)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(LUA_CURRENT_SCRIPT_NAME, "log_print");
        LUA_RETURN_ERROR;
    }

    message = lua_tostring (lua_current_interpreter, -1);
    
    script_api_log_printf (weechat_lua_plugin,
                           lua_current_script,
                           "%s", message);

    LUA_RETURN_OK;
}

/*
 * weechat_lua_api_hook_command_cb: callback for command hooked
 */

int
weechat_lua_api_hook_command_cb (void *data, struct t_gui_buffer *buffer,
                                 int argc, char **argv, char **argv_eol)
{
    struct t_script_callback *script_callback;
    char *lua_argv[4], empty_arg[1] = { '\0' };
    int *rc, ret;

    /* make C compiler happy */
    (void) argv;
    
    script_callback = (struct t_script_callback *)data;

    if (script_callback && script_callback->function && script_callback->function[0])
    {
        lua_argv[0] = (script_callback->data) ? script_callback->data : empty_arg;
        lua_argv[1] = script_ptr2str (buffer);
        lua_argv[2] = (argc > 1) ? argv_eol[1] : empty_arg;
        lua_argv[3] = NULL;
        
        rc = (int *) weechat_lua_exec (script_callback->script,
                                       WEECHAT_SCRIPT_EXEC_INT,
                                       script_callback->function,
                                       lua_argv);
        
        if (!rc)
            ret = WEECHAT_RC_ERROR;
        else
        {
            ret = *rc;
            free (rc);
        }
        if (lua_argv[1])
            free (lua_argv[1]);
        
        return ret;
    }
    
    return WEECHAT_RC_ERROR;
}

/*
 * weechat_lua_api_hook_command: hook a command
 */

static int
weechat_lua_api_hook_command (lua_State *L)
{
    const char *command, *description, *args, *args_description, *completion;
    const char *function, *data;
    char *result;
    int n;
    
    /* make C compiler happy */
    (void) L;
        
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(LUA_CURRENT_SCRIPT_NAME, "hook_command");
        LUA_RETURN_EMPTY;
    }
    
    command = NULL;
    description = NULL;
    args = NULL;
    args_description = NULL;
    completion = NULL;
    function = NULL;
    data = NULL;
    
    n = lua_gettop (lua_current_interpreter);

    if (n < 7)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(LUA_CURRENT_SCRIPT_NAME, "hook_command");
        LUA_RETURN_EMPTY;
    }
    
    command = lua_tostring (lua_current_interpreter, -7);
    description = lua_tostring (lua_current_interpreter, -6);
    args = lua_tostring (lua_current_interpreter, -5);
    args_description = lua_tostring (lua_current_interpreter, -4);
    completion = lua_tostring (lua_current_interpreter, -3);
    function = lua_tostring (lua_current_interpreter, -2);
    data = lua_tostring (lua_current_interpreter, -1);
    
    result = script_ptr2str (script_api_hook_command (weechat_lua_plugin,
                                                      lua_current_script,
                                                      command,
                                                      description,
                                                      args,
                                                      args_description,
                                                      completion,
                                                      &weechat_lua_api_hook_command_cb,
                                                      function,
                                                      data));
    
    LUA_RETURN_STRING_FREE(result);
}

/*
 * weechat_lua_api_hook_command_run_cb: callback for command_run hooked
 */

int
weechat_lua_api_hook_command_run_cb (void *data, struct t_gui_buffer *buffer,
                                     const char *command)
{
    struct t_script_callback *script_callback;
    char *lua_argv[4], empty_arg[1] = { '\0' };
    int *rc, ret;
    
    script_callback = (struct t_script_callback *)data;
    
    if (script_callback && script_callback->function && script_callback->function[0])
    {
        lua_argv[0] = (script_callback->data) ? script_callback->data : empty_arg;
        lua_argv[1] = script_ptr2str (buffer);
        lua_argv[2] = (command) ? (char *)command : empty_arg;
        lua_argv[3] = NULL;
        
        rc = (int *) weechat_lua_exec (script_callback->script,
                                       WEECHAT_SCRIPT_EXEC_INT,
                                       script_callback->function,
                                       lua_argv);
        
        if (!rc)
            ret = WEECHAT_RC_ERROR;
        else
        {
            ret = *rc;
            free (rc);
        }
        if (lua_argv[1])
            free (lua_argv[1]);
        
        return ret;
    }
    
    return WEECHAT_RC_ERROR;
}

/*
 * weechat_lua_api_hook_command_run: hook a command_run
 */

static int
weechat_lua_api_hook_command_run (lua_State *L)
{
    const char *command, *function, *data;
    char *result;
    int n;
    
    /* make C compiler happy */
    (void) L;
        
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(LUA_CURRENT_SCRIPT_NAME, "hook_command_run");
        LUA_RETURN_EMPTY;
    }
    
    command = NULL;
    function = NULL;
    data = NULL;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 3)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(LUA_CURRENT_SCRIPT_NAME, "hook_command_run");
        LUA_RETURN_EMPTY;
    }
    
    command = lua_tostring (lua_current_interpreter, -3);
    function = lua_tostring (lua_current_interpreter, -2);
    data = lua_tostring (lua_current_interpreter, -1);
    
    result = script_ptr2str (script_api_hook_command_run (weechat_lua_plugin,
                                                          lua_current_script,
                                                          command,
                                                          &weechat_lua_api_hook_command_run_cb,
                                                          function,
                                                          data));
    
    LUA_RETURN_STRING_FREE(result);
}

/*
 * weechat_lua_api_hook_timer_cb: callback for timer hooked
 */

int
weechat_lua_api_hook_timer_cb (void *data, int remaining_calls)
{
    struct t_script_callback *script_callback;
    char *lua_argv[3], str_remaining_calls[32], empty_arg[1] = { '\0' };
    int *rc, ret;
    
    script_callback = (struct t_script_callback *)data;
    
    if (script_callback && script_callback->function && script_callback->function[0])
    {
        snprintf (str_remaining_calls, sizeof (str_remaining_calls),
                  "%d", remaining_calls);
        
        lua_argv[0] = (script_callback->data) ? script_callback->data : empty_arg;
        lua_argv[1] = str_remaining_calls;
        lua_argv[2] = NULL;
        
        rc = (int *) weechat_lua_exec (script_callback->script,
                                       WEECHAT_SCRIPT_EXEC_INT,
                                       script_callback->function,
                                       lua_argv);
        
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
 * weechat_lua_api_hook_timer: hook a timer
 */

static int
weechat_lua_api_hook_timer (lua_State *L)
{
    int n, interval, align_second, max_calls;
    const char *function, *data;
    char *result;
    
    /* make C compiler happy */
    (void) L;
        
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(LUA_CURRENT_SCRIPT_NAME, "hook_timer");
        LUA_RETURN_EMPTY;
    }
    
    interval = 10;
    align_second = 0;
    max_calls = 0;
    function = NULL;
    data = NULL;
    
    n = lua_gettop (lua_current_interpreter);

    if (n < 5)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(LUA_CURRENT_SCRIPT_NAME, "hook_timer");
        LUA_RETURN_EMPTY;
    }
    
    interval = lua_tonumber (lua_current_interpreter, -5);
    align_second = lua_tonumber (lua_current_interpreter, -4);
    max_calls = lua_tonumber (lua_current_interpreter, -3);
    function = lua_tostring (lua_current_interpreter, -2);
    data = lua_tostring (lua_current_interpreter, -1);
    
    result = script_ptr2str (script_api_hook_timer (weechat_lua_plugin,
                                                    lua_current_script,
                                                    interval,
                                                    align_second,
                                                    max_calls,
                                                    &weechat_lua_api_hook_timer_cb,
                                                    function,
                                                    data));
    
    LUA_RETURN_STRING_FREE(result);
}

/*
 * weechat_lua_api_hook_fd_cb: callback for fd hooked
 */

int
weechat_lua_api_hook_fd_cb (void *data, int fd)
{
    struct t_script_callback *script_callback;
    char *lua_argv[3], str_fd[32], empty_arg[1] = { '\0' };
    int *rc, ret;
    
    script_callback = (struct t_script_callback *)data;

    if (script_callback && script_callback->function && script_callback->function[0])
    {
        snprintf (str_fd, sizeof (str_fd), "%d", fd);
        
        lua_argv[0] = (script_callback->data) ? script_callback->data : empty_arg;
        lua_argv[1] = str_fd;
        lua_argv[2] = NULL;
        
        rc = (int *) weechat_lua_exec (script_callback->script,
                                       WEECHAT_SCRIPT_EXEC_INT,
                                       script_callback->function,
                                       lua_argv);
        
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
 * weechat_lua_api_hook_fd: hook a fd
 */

static int
weechat_lua_api_hook_fd (lua_State *L)
{
    int n, fd, read, write, exception;
    const char *function, *data;
    char *result;
    
    /* make C compiler happy */
    (void) L;
        
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(LUA_CURRENT_SCRIPT_NAME, "hook_fd");
        LUA_RETURN_EMPTY;
    }
    
    fd = 0;
    read = 0;
    write = 0;
    exception = 0;
    function = NULL;
    data = NULL;
    
    n = lua_gettop (lua_current_interpreter);

    if (n < 6)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(LUA_CURRENT_SCRIPT_NAME, "hook_fd");
        LUA_RETURN_EMPTY;
    }
    
    fd = lua_tonumber (lua_current_interpreter, -6);
    read = lua_tonumber (lua_current_interpreter, -5);
    write = lua_tonumber (lua_current_interpreter, -4);
    exception = lua_tonumber (lua_current_interpreter, -3);
    function = lua_tostring (lua_current_interpreter, -2);
    data = lua_tostring (lua_current_interpreter, -1);
    
    result = script_ptr2str (script_api_hook_fd (weechat_lua_plugin,
                                                 lua_current_script,
                                                 fd,
                                                 read,
                                                 write,
                                                 exception,
                                                 &weechat_lua_api_hook_fd_cb,
                                                 function,
                                                 data));
    
    LUA_RETURN_STRING_FREE(result);
}

/*
 * weechat_lua_api_hook_process_cb: callback for process hooked
 */

int
weechat_lua_api_hook_process_cb (void *data,
                                 const char *command, int return_code,
                                 const char *out, const char *err)
{
    struct t_script_callback *script_callback;
    char *lua_argv[6], str_rc[32], empty_arg[1] = { '\0' };
    int *rc, ret;
    
    script_callback = (struct t_script_callback *)data;
    
    if (script_callback && script_callback->function && script_callback->function[0])
    {
        snprintf (str_rc, sizeof (str_rc), "%d", return_code);
        
        lua_argv[0] = (script_callback->data) ? script_callback->data : empty_arg;
        lua_argv[1] = (command) ? (char *)command : empty_arg;
        lua_argv[2] = str_rc;
        lua_argv[3] = (out) ? (char *)out : empty_arg;
        lua_argv[4] = (err) ? (char *)err : empty_arg;
        lua_argv[5] = NULL;
        
        rc = (int *) weechat_lua_exec (script_callback->script,
                                       WEECHAT_SCRIPT_EXEC_INT,
                                       script_callback->function,
                                       lua_argv);
        
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
 * weechat_lua_api_hook_process: hook a process
 */

static int
weechat_lua_api_hook_process (lua_State *L)
{
    const char *command, *function, *data;
    int n, timeout;
    char *result;
    
    /* make C compiler happy */
    (void) L;
        
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(LUA_CURRENT_SCRIPT_NAME, "hook_process");
        LUA_RETURN_EMPTY;
    }
    
    command = NULL;
    timeout = 0;
    function = NULL;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 4)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(LUA_CURRENT_SCRIPT_NAME, "hook_process");
        LUA_RETURN_EMPTY;
    }
    
    command = lua_tostring (lua_current_interpreter, -4);
    timeout = lua_tonumber (lua_current_interpreter, -3);
    function = lua_tostring (lua_current_interpreter, -2);
    data = lua_tostring (lua_current_interpreter, -1);
    
    result = script_ptr2str (script_api_hook_process (weechat_lua_plugin,
                                                      lua_current_script,
                                                      command,
                                                      timeout,
                                                      &weechat_lua_api_hook_process_cb,
                                                      function,
                                                      data));
    
    LUA_RETURN_STRING_FREE(result);
}

/*
 * weechat_lua_api_hook_connect_cb: callback for connect hooked
 */

int
weechat_lua_api_hook_connect_cb (void *data, int status,
                                 const char *error, const char *ip_address)
{
    struct t_script_callback *script_callback;
    char *lua_argv[5], str_status[32], empty_arg[1] = { '\0' };
    int *rc, ret;
    
    script_callback = (struct t_script_callback *)data;

    if (script_callback && script_callback->function && script_callback->function[0])
    {
        snprintf (str_status, sizeof (str_status), "%d", status);
        
        lua_argv[0] = (script_callback->data) ? script_callback->data : empty_arg;
        lua_argv[1] = str_status;
        lua_argv[2] = (ip_address) ? (char *)ip_address : empty_arg;
        lua_argv[3] = (error) ? (char *)error : empty_arg;
        lua_argv[4] = NULL;
        
        rc = (int *) weechat_lua_exec (script_callback->script,
                                       WEECHAT_SCRIPT_EXEC_INT,
                                       script_callback->function,
                                       lua_argv);
        
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
 * weechat_lua_api_hook_connect: hook a connection
 */

static int
weechat_lua_api_hook_connect (lua_State *L)
{
    const char *proxy, *address, *local_hostname, *function, *data;
    int n, port, sock, ipv6;
    char *result;
    
    /* make C compiler happy */
    (void) L;
        
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(LUA_CURRENT_SCRIPT_NAME, "hook_connect");
        LUA_RETURN_EMPTY;
    }
    
    proxy = NULL;
    address = NULL;
    port = 0;
    sock = 0;
    ipv6 = 0;
    local_hostname = NULL;
    function = NULL;
    data = NULL;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 8)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(LUA_CURRENT_SCRIPT_NAME, "hook_connect");
        LUA_RETURN_EMPTY;
    }
    
    proxy = lua_tostring (lua_current_interpreter, -8);
    address = lua_tostring (lua_current_interpreter, -7);
    port = lua_tonumber (lua_current_interpreter, -6);
    sock = lua_tonumber (lua_current_interpreter, -5);
    ipv6 = lua_tonumber (lua_current_interpreter, -4);
    local_hostname = lua_tostring (lua_current_interpreter, -3);
    function = lua_tostring (lua_current_interpreter, -2);
    data = lua_tostring (lua_current_interpreter, -1);
    
    result = script_ptr2str (script_api_hook_connect (weechat_lua_plugin,
                                                      lua_current_script,
                                                      proxy,
                                                      address,
                                                      port,
                                                      sock,
                                                      ipv6,
                                                      NULL, /* gnutls session */
                                                      local_hostname,
                                                      &weechat_lua_api_hook_connect_cb,
                                                      function,
                                                      data));
    
    LUA_RETURN_STRING_FREE(result);
}

/*
 * weechat_lua_api_hook_print_cb: callback for print hooked
 */

int
weechat_lua_api_hook_print_cb (void *data, struct t_gui_buffer *buffer,
                               time_t date,
                               int tags_count, const char **tags,
                               int displayed, int highlight,
                               const char *prefix, const char *message)
{
    struct t_script_callback *script_callback;
    char *lua_argv[9], empty_arg[1] = { '\0' };
    static char timebuffer[64];
    int *rc, ret;
    
    /* make C compiler happy */
    (void) tags_count;
    
    script_callback = (struct t_script_callback *)data;

    if (script_callback && script_callback->function && script_callback->function[0])
    {
        snprintf (timebuffer, sizeof (timebuffer) - 1, "%ld", (long int)date);
        
        lua_argv[0] = (script_callback->data) ? script_callback->data : empty_arg;
        lua_argv[1] = script_ptr2str (buffer);
        lua_argv[2] = timebuffer;
        lua_argv[3] = weechat_string_build_with_split_string (tags, ",");
        if (!lua_argv[3])
            lua_argv[3] = strdup ("");
        lua_argv[4] = (displayed) ? strdup ("1") : strdup ("0");
        lua_argv[5] = (highlight) ? strdup ("1") : strdup ("0");
        lua_argv[6] = (prefix) ? (char *)prefix : empty_arg;
        lua_argv[7] = (message) ? (char *)message : empty_arg;
        lua_argv[8] = NULL;
        
        rc = (int *) weechat_lua_exec (script_callback->script,
                                       WEECHAT_SCRIPT_EXEC_INT,
                                       script_callback->function,
                                       lua_argv);
        
        if (!rc)
            ret = WEECHAT_RC_ERROR;
        else
        {
            ret = *rc;
            free (rc);
        }
        if (lua_argv[1])
            free (lua_argv[1]);
        if (lua_argv[3])
            free (lua_argv[3]);
        if (lua_argv[4])
            free (lua_argv[4]);
        if (lua_argv[5])
            free (lua_argv[5]);
        
        return ret;
    }
    
    return WEECHAT_RC_ERROR;
}

/*
 * weechat_lua_api_hook_print: hook a print
 */

static int
weechat_lua_api_hook_print (lua_State *L)
{
    const char *buffer, *tags, *message, *function, *data;
    char *result;
    int n, strip_colors;
    
    /* make C compiler happy */
    (void) L;
        
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(LUA_CURRENT_SCRIPT_NAME, "hook_print");
        LUA_RETURN_EMPTY;
    }
    
    buffer = NULL;
    tags = NULL;
    message = NULL;
    strip_colors = 0;
    function = NULL;
    data = NULL;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 6)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(LUA_CURRENT_SCRIPT_NAME, "hook_print");
        LUA_RETURN_EMPTY;
    }
    
    buffer = lua_tostring (lua_current_interpreter, -6);
    tags = lua_tostring (lua_current_interpreter, -5);
    message = lua_tostring (lua_current_interpreter, -4);
    strip_colors = lua_tonumber (lua_current_interpreter, -3);
    function = lua_tostring (lua_current_interpreter, -2);
    data = lua_tostring (lua_current_interpreter, -1);
    
    result = script_ptr2str (script_api_hook_print (weechat_lua_plugin,
                                                    lua_current_script,
                                                    script_str2ptr (buffer),
                                                    tags,
                                                    message,
                                                    strip_colors,
                                                    &weechat_lua_api_hook_print_cb,
                                                    function,
                                                    data));
    
    LUA_RETURN_STRING_FREE(result);
}

/*
 * weechat_lua_api_hook_signal_cb: callback for signal hooked
 */

int
weechat_lua_api_hook_signal_cb (void *data, const char *signal,
                                const char *type_data, void *signal_data)
{
    struct t_script_callback *script_callback;
    char *lua_argv[4], empty_arg[1] = { '\0' };
    static char value_str[64];
    int *rc, ret, free_needed;
    
    script_callback = (struct t_script_callback *)data;

    if (script_callback && script_callback->function && script_callback->function[0])
    {
        lua_argv[0] = (script_callback->data) ? script_callback->data : empty_arg;
        lua_argv[1] = (signal) ? (char *)signal : empty_arg;
        free_needed = 0;
        if (strcmp (type_data, WEECHAT_HOOK_SIGNAL_STRING) == 0)
        {
            lua_argv[2] = (signal_data) ? (char *)signal_data : empty_arg;
        }
        else if (strcmp (type_data, WEECHAT_HOOK_SIGNAL_INT) == 0)
        {
            snprintf (value_str, sizeof (value_str) - 1,
                      "%d", *((int *)signal_data));
            lua_argv[2] = value_str;
        }
        else if (strcmp (type_data, WEECHAT_HOOK_SIGNAL_POINTER) == 0)
        {
            lua_argv[2] = script_ptr2str (signal_data);
            free_needed = 1;
        }
        else
            lua_argv[2] = empty_arg;
        lua_argv[3] = NULL;
        
        rc = (int *) weechat_lua_exec (script_callback->script,
                                       WEECHAT_SCRIPT_EXEC_INT,
                                       script_callback->function,
                                       lua_argv);
        
        if (!rc)
            ret = WEECHAT_RC_ERROR;
        else
        {
            ret = *rc;
            free (rc);
        }
        if (free_needed && lua_argv[2])
            free (lua_argv[2]);
        
        return ret;
    }
    
    return WEECHAT_RC_ERROR;
}

/*
 * weechat_lua_api_hook_signal: hook a signal
 */

static int
weechat_lua_api_hook_signal (lua_State *L)
{
    const char *signal, *function, *data;
    char *result;
    int n;
    
    /* make C compiler happy */
    (void) L;
        
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(LUA_CURRENT_SCRIPT_NAME, "hook_signal");
        LUA_RETURN_EMPTY;
    }
    
    signal = NULL;
    function = NULL;
    data = NULL;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 3)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(LUA_CURRENT_SCRIPT_NAME, "hook_signal");
        LUA_RETURN_EMPTY;
    }
    
    signal = lua_tostring (lua_current_interpreter, -3);
    function = lua_tostring (lua_current_interpreter, -2);
    data = lua_tostring (lua_current_interpreter, -1);
    
    result = script_ptr2str (script_api_hook_signal (weechat_lua_plugin,
                                                     lua_current_script,
                                                     signal,
                                                     &weechat_lua_api_hook_signal_cb,
                                                     function,
                                                     data));
    
    LUA_RETURN_STRING_FREE(result);
}

/*
 * weechat_lua_api_hook_signal_send: send a signal
 */

static int
weechat_lua_api_hook_signal_send (lua_State *L)
{
    const char *signal, *type_data, *signal_data;
    int n, number;
    
    /* make C compiler happy */
    (void) L;
        
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(LUA_CURRENT_SCRIPT_NAME, "hook_signal_send");
        LUA_RETURN_ERROR;
    }
    
    signal = NULL;
    type_data = NULL;
    signal_data = NULL;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 3)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(LUA_CURRENT_SCRIPT_NAME, "hook_signal_send");
        LUA_RETURN_ERROR;
    }
    
    signal = lua_tostring (lua_current_interpreter, -3);
    type_data = lua_tostring (lua_current_interpreter, -2);

    if (strcmp (type_data, WEECHAT_HOOK_SIGNAL_STRING) == 0)
    {
        signal_data = lua_tostring (lua_current_interpreter, -1);
        weechat_hook_signal_send (signal, type_data, (void *)signal_data);
        LUA_RETURN_OK;
    }
    else if (strcmp (type_data, WEECHAT_HOOK_SIGNAL_INT) == 0)
    {
        number = lua_tonumber (lua_current_interpreter, -1);
        weechat_hook_signal_send (signal, type_data, &number);
        LUA_RETURN_OK;
    }
    else if (strcmp (type_data, WEECHAT_HOOK_SIGNAL_POINTER) == 0)
    {
        signal_data = lua_tostring (lua_current_interpreter, -1);
        weechat_hook_signal_send (signal, type_data,
                                  script_str2ptr (signal_data));
        LUA_RETURN_OK;
    }
    
    LUA_RETURN_ERROR;
}

/*
 * weechat_lua_api_hook_config_cb: callback for config option hooked
 */

int
weechat_lua_api_hook_config_cb (void *data, const char *option,
                                const char *value)
{
    struct t_script_callback *script_callback;
    char *lua_argv[4], empty_arg[1] = { '\0' };
    int *rc, ret;
    
    script_callback = (struct t_script_callback *)data;
    
    if (script_callback && script_callback->function && script_callback->function[0])
    {
        lua_argv[0] = (script_callback->data) ? script_callback->data : empty_arg;
        lua_argv[1] = (option) ? (char *)option : empty_arg;
        lua_argv[2] = (value) ? (char *)value : empty_arg;
        lua_argv[3] = NULL;
        
        rc = (int *) weechat_lua_exec (script_callback->script,
                                       WEECHAT_SCRIPT_EXEC_INT,
                                       script_callback->function,
                                       lua_argv);
        
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
 * weechat_lua_api_hook_config: hook a config option
 */

static int
weechat_lua_api_hook_config (lua_State *L)
{
    const char *type, *option, *function, *data;
    char *result;
    int n;
    
    /* make C compiler happy */
    (void) L;
    
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(LUA_CURRENT_SCRIPT_NAME, "hook_config");
        LUA_RETURN_EMPTY;
    }
    
    type = NULL;
    option = NULL;
    function = NULL;
    data = NULL;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 3)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(LUA_CURRENT_SCRIPT_NAME, "hook_config");
        LUA_RETURN_EMPTY;
    }
    
    option = lua_tostring (lua_current_interpreter, -3);
    function = lua_tostring (lua_current_interpreter, -2);
    data = lua_tostring (lua_current_interpreter, -1);
    
    result = script_ptr2str (script_api_hook_config (weechat_lua_plugin,
                                                     lua_current_script,
                                                     option,
                                                     &weechat_lua_api_hook_config_cb,
                                                     function,
                                                     data));
    
    LUA_RETURN_STRING_FREE(result);
}

/*
 * weechat_lua_api_hook_completion_cb: callback for completion hooked
 */

int
weechat_lua_api_hook_completion_cb (void *data, const char *completion_item,
                                    struct t_gui_buffer *buffer,
                                    struct t_gui_completion *completion)
{
    struct t_script_callback *script_callback;
    char *lua_argv[5], empty_arg[1] = { '\0' };
    int *rc, ret;
    
    script_callback = (struct t_script_callback *)data;

    if (script_callback && script_callback->function && script_callback->function[0])
    {
        lua_argv[0] = (script_callback->data) ? script_callback->data : empty_arg;
        lua_argv[1] = (completion_item) ? (char *)completion_item : empty_arg;
        lua_argv[2] = script_ptr2str (buffer);
        lua_argv[3] = script_ptr2str (completion);
        lua_argv[4] = NULL;
        
        rc = (int *) weechat_lua_exec (script_callback->script,
                                       WEECHAT_SCRIPT_EXEC_INT,
                                       script_callback->function,
                                       lua_argv);
        
        if (!rc)
            ret = WEECHAT_RC_ERROR;
        else
        {
            ret = *rc;
            free (rc);
        }
        if (lua_argv[2])
            free (lua_argv[2]);
        if (lua_argv[3])
            free (lua_argv[3]);
        
        return ret;
    }
    
    return WEECHAT_RC_ERROR;
}

/*
 * weechat_lua_api_hook_completion: hook a completion
 */

static int
weechat_lua_api_hook_completion (lua_State *L)
{
    const char *completion, *description, *function, *data;
    char *result;
    int n;
    
    /* make C compiler happy */
    (void) L;
        
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(LUA_CURRENT_SCRIPT_NAME, "hook_completion");
        LUA_RETURN_EMPTY;
    }
    
    completion = NULL;
    description = NULL;
    function = NULL;
    data = NULL;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 4)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(LUA_CURRENT_SCRIPT_NAME, "hook_completion");
        LUA_RETURN_EMPTY;
    }
    
    completion = lua_tostring (lua_current_interpreter, -4);
    description = lua_tostring (lua_current_interpreter, -3);
    function = lua_tostring (lua_current_interpreter, -2);
    data = lua_tostring (lua_current_interpreter, -1);
    
    result = script_ptr2str (script_api_hook_completion (weechat_lua_plugin,
                                                         lua_current_script,
                                                         completion,
                                                         description,
                                                         &weechat_lua_api_hook_completion_cb,
                                                         function,
                                                         data));
    
    LUA_RETURN_STRING_FREE(result);
}

/*
 * weechat_lua_api_hook_completion_list_add: add a word to list for a completion
 */

static int
weechat_lua_api_hook_completion_list_add (lua_State *L)
{
    const char *completion, *word, *where;
    int n, nick_completion;
    
    /* make C compiler happy */
    (void) L;
        
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(LUA_CURRENT_SCRIPT_NAME, "hook_completion_list_add");
        LUA_RETURN_ERROR;
    }
    
    completion = NULL;
    word = NULL;
    nick_completion = 0;
    where = NULL;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 4)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(LUA_CURRENT_SCRIPT_NAME, "hook_completion_list_add");
        LUA_RETURN_ERROR;
    }
    
    completion = lua_tostring (lua_current_interpreter, -4);
    word = lua_tostring (lua_current_interpreter, -3);
    nick_completion = lua_tonumber (lua_current_interpreter, -2);
    where = lua_tostring (lua_current_interpreter, -1);
    
    weechat_hook_completion_list_add (script_str2ptr (completion),
                                      word,
                                      nick_completion,
                                      where);
    
    LUA_RETURN_OK;
}

/*
 * weechat_lua_api_hook_modifier_cb: callback for modifier hooked
 */

char *
weechat_lua_api_hook_modifier_cb (void *data, const char *modifier,
                                  const char *modifier_data,
                                  const char *string)
{
    struct t_script_callback *script_callback;
    char *lua_argv[5], empty_arg[1] = { '\0' };
    
    script_callback = (struct t_script_callback *)data;

    if (script_callback && script_callback->function && script_callback->function[0])
    {
        lua_argv[0] = (script_callback->data) ? script_callback->data : empty_arg;
        lua_argv[1] = (modifier) ? (char *)modifier : empty_arg;
        lua_argv[2] = (modifier_data) ? (char *)modifier_data : empty_arg;
        lua_argv[3] = (string) ? (char *)string : empty_arg;
        lua_argv[4] = NULL;
        
        return (char *)weechat_lua_exec (script_callback->script,
                                         WEECHAT_SCRIPT_EXEC_STRING,
                                         script_callback->function,
                                         lua_argv);
    }
    
    return NULL;
}

/*
 * weechat_lua_api_hook_modifier: hook a modifier
 */

static int
weechat_lua_api_hook_modifier (lua_State *L)
{
    const char *modifier, *function, *data;
    char *result;
    int n;
    
    /* make C compiler happy */
    (void) L;
        
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(LUA_CURRENT_SCRIPT_NAME, "hook_modifier");
        LUA_RETURN_EMPTY;
    }
    
    modifier = NULL;
    function = NULL;
    data = NULL;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 3)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(LUA_CURRENT_SCRIPT_NAME, "hook_modifier");
        LUA_RETURN_EMPTY;
    }
    
    modifier = lua_tostring (lua_current_interpreter, -3);
    function = lua_tostring (lua_current_interpreter, -2);
    data = lua_tostring (lua_current_interpreter, -1);
    
    result = script_ptr2str (script_api_hook_modifier (weechat_lua_plugin,
                                                       lua_current_script,
                                                       modifier,
                                                       &weechat_lua_api_hook_modifier_cb,
                                                       function,
                                                       data));
    
    LUA_RETURN_STRING_FREE(result);
}

/*
 * weechat_lua_api_hook_modifier_exec: execute a modifier hook
 */

static int
weechat_lua_api_hook_modifier_exec (lua_State *L)
{
    const char *modifier, *modifier_data, *string;
    char *result;
    int n;
    
    /* make C compiler happy */
    (void) L;
        
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(LUA_CURRENT_SCRIPT_NAME, "hook_modifier_exec");
        LUA_RETURN_EMPTY;
    }
    
    modifier = NULL;
    modifier_data = NULL;
    string = NULL;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 3)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(LUA_CURRENT_SCRIPT_NAME, "hook_modifier_exec");
        LUA_RETURN_ERROR;
    }
    
    modifier = lua_tostring (lua_current_interpreter, -3);
    modifier_data = lua_tostring (lua_current_interpreter, -2);
    string = lua_tostring (lua_current_interpreter, -1);
    
    result = weechat_hook_modifier_exec (modifier, modifier_data, string);
    
    LUA_RETURN_STRING_FREE(result);
}

/*
 * weechat_lua_api_hook_info_cb: callback for info hooked
 */

const char *
weechat_lua_api_hook_info_cb (void *data, const char *info_name,
                              const char *arguments)
{
    struct t_script_callback *script_callback;
    char *lua_argv[4], empty_arg[1] = { '\0' };
    
    script_callback = (struct t_script_callback *)data;

    if (script_callback && script_callback->function && script_callback->function[0])
    {
        lua_argv[0] = (script_callback->data) ? script_callback->data : empty_arg;
        lua_argv[1] = (info_name) ? (char *)info_name : empty_arg;
        lua_argv[2] = (arguments) ? (char *)arguments : empty_arg;
        lua_argv[3] = NULL;
        
        return (const char *)weechat_lua_exec (script_callback->script,
                                               WEECHAT_SCRIPT_EXEC_STRING,
                                               script_callback->function,
                                               lua_argv);
    }
    
    return NULL;
}

/*
 * weechat_lua_api_hook_info: hook an info
 */

static int
weechat_lua_api_hook_info (lua_State *L)
{
    const char *info_name, *description, *function, *data;
    char *result;
    int n;
    
    /* make C compiler happy */
    (void) L;
        
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(LUA_CURRENT_SCRIPT_NAME, "hook_info");
        LUA_RETURN_EMPTY;
    }
    
    info_name = NULL;
    function = NULL;
    data = NULL;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 4)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(LUA_CURRENT_SCRIPT_NAME, "hook_info");
        LUA_RETURN_EMPTY;
    }
    
    info_name = lua_tostring (lua_current_interpreter, -4);
    description = lua_tostring (lua_current_interpreter, -3);
    function = lua_tostring (lua_current_interpreter, -2);
    data = lua_tostring (lua_current_interpreter, -1);
    
    result = script_ptr2str (script_api_hook_info (weechat_lua_plugin,
                                                   lua_current_script,
                                                   info_name,
                                                   description,
                                                   &weechat_lua_api_hook_info_cb,
                                                   function,
                                                   data));
    
    LUA_RETURN_STRING_FREE(result);
}

/*
 * weechat_lua_api_hook_infolist_cb: callback for infolist hooked
 */

struct t_infolist *
weechat_lua_api_hook_infolist_cb (void *data, const char *info_name,
                                  void *pointer, const char *arguments)
{
    struct t_script_callback *script_callback;
    char *lua_argv[5], empty_arg[1] = { '\0' };
    struct t_infolist *result;
    
    script_callback = (struct t_script_callback *)data;

    if (script_callback && script_callback->function && script_callback->function[0])
    {
        lua_argv[0] = (script_callback->data) ? script_callback->data : empty_arg;
        lua_argv[1] = (info_name) ? (char *)info_name : empty_arg;
        lua_argv[2] = script_ptr2str (pointer);
        lua_argv[3] = (arguments) ? (char *)arguments : empty_arg;
        lua_argv[4] = NULL;
        
        result = (struct t_infolist *)weechat_lua_exec (script_callback->script,
                                                        WEECHAT_SCRIPT_EXEC_STRING,
                                                        script_callback->function,
                                                        lua_argv);
        
        if (lua_argv[2])
            free (lua_argv[2]);
        
        return result;
    }
    
    return NULL;
}

/*
 * weechat_lua_api_hook_infolist: hook an infolist
 */

static int
weechat_lua_api_hook_infolist (lua_State *L)
{
    const char *infolist_name, *description, *function, *data;
    char *result;
    int n;
    
    /* make C compiler happy */
    (void) L;
        
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(LUA_CURRENT_SCRIPT_NAME, "hook_infolist");
        LUA_RETURN_EMPTY;
    }
    
    infolist_name = NULL;
    function = NULL;
    data = NULL;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 4)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(LUA_CURRENT_SCRIPT_NAME, "hook_infolist");
        LUA_RETURN_EMPTY;
    }
    
    infolist_name = lua_tostring (lua_current_interpreter, -4);
    description = lua_tostring (lua_current_interpreter, -3);
    function = lua_tostring (lua_current_interpreter, -2);
    data = lua_tostring (lua_current_interpreter, -1);
    
    result = script_ptr2str (script_api_hook_infolist (weechat_lua_plugin,
                                                       lua_current_script,
                                                       infolist_name,
                                                       description,
                                                       &weechat_lua_api_hook_infolist_cb,
                                                       function,
                                                       data));
    
    LUA_RETURN_STRING_FREE(result);
}

/*
 * weechat_lua_api_unhook: unhook something
 */

static int
weechat_lua_api_unhook (lua_State *L)
{
    const char *hook;
    int n;
    
    /* make C compiler happy */
    (void) L;
        
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(LUA_CURRENT_SCRIPT_NAME, "unhook");
        LUA_RETURN_ERROR;
    }
    
    hook = NULL;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 1)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(LUA_CURRENT_SCRIPT_NAME, "unhook");
        LUA_RETURN_ERROR;
    }
    
    hook = lua_tostring (lua_current_interpreter, -1);
    
    script_api_unhook (weechat_lua_plugin,
                       lua_current_script,
                       script_str2ptr (hook));
    
    LUA_RETURN_OK;
}

/*
 * weechat_lua_api_unhook_all: unhook all for script
 */

static int
weechat_lua_api_unhook_all (lua_State *L)
{
    /* make C compiler happy */
    (void) L;
    
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(LUA_CURRENT_SCRIPT_NAME, "unhook_all");
        LUA_RETURN_ERROR;
    }
    
    script_api_unhook_all (lua_current_script);
    
    LUA_RETURN_OK;
}

/*
 * weechat_lua_api_buffer_input_data_cb: callback for input data in a buffer
 */

int
weechat_lua_api_buffer_input_data_cb (void *data, struct t_gui_buffer *buffer,
                                      const char *input_data)
{
    struct t_script_callback *script_callback;
    char *lua_argv[4], empty_arg[1] = { '\0' };
    int *rc, ret;
    
    script_callback = (struct t_script_callback *)data;
    
    if (script_callback && script_callback->function && script_callback->function[0])
    {
        lua_argv[0] = (script_callback->data) ? script_callback->data : empty_arg;
        lua_argv[1] = script_ptr2str (buffer);
        lua_argv[2] = (input_data) ? (char *)input_data : empty_arg;
        lua_argv[3] = NULL;
        
        rc = (int *) weechat_lua_exec (script_callback->script,
                                       WEECHAT_SCRIPT_EXEC_INT,
                                       script_callback->function,
                                       lua_argv);
        
        if (!rc)
            ret = WEECHAT_RC_ERROR;
        else
        {
            ret = *rc;
            free (rc);
        }
        if (lua_argv[1])
            free (lua_argv[1]);
        
        return ret;
    }
    
    return WEECHAT_RC_ERROR;
}

/*
 * weechat_lua_api_buffer_close_cb: callback for buffer closed
 */

int
weechat_lua_api_buffer_close_cb (void *data, struct t_gui_buffer *buffer)
{
    struct t_script_callback *script_callback;
    char *lua_argv[3], empty_arg[1] = { '\0' };
    int *rc, ret;
    
    script_callback = (struct t_script_callback *)data;

    if (script_callback && script_callback->function && script_callback->function[0])
    {
        lua_argv[0] = (script_callback->data) ? script_callback->data : empty_arg;
        lua_argv[1] = script_ptr2str (buffer);
        lua_argv[2] = NULL;
        
        rc = (int *) weechat_lua_exec (script_callback->script,
                                       WEECHAT_SCRIPT_EXEC_INT,
                                       script_callback->function,
                                       lua_argv);
        
        if (!rc)
            ret = WEECHAT_RC_ERROR;
        else
        {
            ret = *rc;
            free (rc);
        }
        if (lua_argv[1])
            free (lua_argv[1]);
        
        return ret;
    }
    
    return WEECHAT_RC_ERROR;
}

/*
 * weechat_lua_api_buffer_new: create a new buffer
 */

static int
weechat_lua_api_buffer_new (lua_State *L)
{
    const char *name, *function_input, *data_input, *function_close;
    const char *data_close;
    char *result;
    int n;
    
    /* make C compiler happy */
    (void) L;
        
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(LUA_CURRENT_SCRIPT_NAME, "buffer_new");
        LUA_RETURN_EMPTY;
    }
    
    name = NULL;
    function_input = NULL;
    data_input = NULL;
    function_close = NULL;
    data_close = NULL;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 5)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(LUA_CURRENT_SCRIPT_NAME, "buffer_new");
        LUA_RETURN_EMPTY;
    }
    
    name = lua_tostring (lua_current_interpreter, -5);
    function_input = lua_tostring (lua_current_interpreter, -4);
    data_input = lua_tostring (lua_current_interpreter, -3);
    function_close = lua_tostring (lua_current_interpreter, -2);
    data_close = lua_tostring (lua_current_interpreter, -1);
    
    result = script_ptr2str (script_api_buffer_new (weechat_lua_plugin,
                                                    lua_current_script,
                                                    name,
                                                    &weechat_lua_api_buffer_input_data_cb,
                                                    function_input,
                                                    data_input,
                                                    &weechat_lua_api_buffer_close_cb,
                                                    function_close,
                                                    data_close));
    
    LUA_RETURN_STRING_FREE(result);
}

/*
 * weechat_lua_api_buffer_search: search a buffer
 */

static int
weechat_lua_api_buffer_search (lua_State *L)
{
    const char *plugin, *name;
    char *result;
    int n;
    
    /* make C compiler happy */
    (void) L;
    
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(LUA_CURRENT_SCRIPT_NAME, "buffer_search");
        LUA_RETURN_EMPTY;
    }
    
    plugin = NULL;
    name = NULL;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 2)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(LUA_CURRENT_SCRIPT_NAME, "buffer_search");
        LUA_RETURN_EMPTY;
    }
    
    plugin = lua_tostring (lua_current_interpreter, -2);
    name = lua_tostring (lua_current_interpreter, -1);
    
    result = script_ptr2str (weechat_buffer_search (plugin, name));
    
    LUA_RETURN_STRING_FREE(result);
}

/*
 * weechat_lua_api_buffer_search_main: search main buffer (WeeChat core buffer)
 */

static int
weechat_lua_api_buffer_search_main (lua_State *L)
{
    char *result;
    
    /* make C compiler happy */
    (void) L;
    
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(LUA_CURRENT_SCRIPT_NAME, "buffer_search_main");
        LUA_RETURN_EMPTY;
    }
    
    result = script_ptr2str (weechat_buffer_search_main ());
    
    LUA_RETURN_STRING_FREE(result);
}

/*
 * weechat_lua_api_current_buffer: get current buffer
 */

static int
weechat_lua_api_current_buffer (lua_State *L)
{
    char *result;
    
    /* make C compiler happy */
    (void) L;
    
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(LUA_CURRENT_SCRIPT_NAME, "current_buffer");
        LUA_RETURN_EMPTY;
    }
    
    result = script_ptr2str (weechat_current_buffer ());
    
    LUA_RETURN_STRING_FREE(result);
}

/*
 * weechat_lua_api_buffer_clear: clear a buffer
 */

static int
weechat_lua_api_buffer_clear (lua_State *L)
{
    const char *buffer;
    int n;
    
    /* make C compiler happy */
    (void) L;
    
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(LUA_CURRENT_SCRIPT_NAME, "buffer_clear");
        LUA_RETURN_ERROR;
    }
    
    buffer = NULL;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 1)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(LUA_CURRENT_SCRIPT_NAME, "buffer_clear");
        LUA_RETURN_ERROR;
    }
    
    buffer = lua_tostring (lua_current_interpreter, -1);
    
    weechat_buffer_clear (script_str2ptr (buffer));
    
    LUA_RETURN_OK;
}

/*
 * weechat_lua_api_buffer_close: close a buffer
 */

static int
weechat_lua_api_buffer_close (lua_State *L)
{
    const char *buffer;
    int n;
    
    /* make C compiler happy */
    (void) L;
        
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(LUA_CURRENT_SCRIPT_NAME, "buffer_close");
        LUA_RETURN_ERROR;
    }
    
    buffer = NULL;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 1)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(LUA_CURRENT_SCRIPT_NAME, "buffer_close");
        LUA_RETURN_ERROR;
    }
    
    buffer = lua_tostring (lua_current_interpreter, -1);
    
    script_api_buffer_close (weechat_lua_plugin,
                             lua_current_script,
                             script_str2ptr (buffer));
    
    LUA_RETURN_OK;
}

/*
 * weechat_lua_api_buffer_merge: merge a buffer to another buffer
 */

static int
weechat_lua_api_buffer_merge (lua_State *L)
{
    const char *buffer, *target_buffer;
    int n;
    
    /* make C compiler happy */
    (void) L;
    
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(LUA_CURRENT_SCRIPT_NAME, "buffer_merge");
        LUA_RETURN_ERROR;
    }
    
    buffer = NULL;
    target_buffer = NULL;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 2)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(LUA_CURRENT_SCRIPT_NAME, "buffer_merge");
        LUA_RETURN_ERROR;
    }
    
    buffer = lua_tostring (lua_current_interpreter, -2);
    target_buffer = lua_tostring (lua_current_interpreter, -1);
    
    weechat_buffer_merge (script_str2ptr (buffer),
                          script_str2ptr (target_buffer));
    
    LUA_RETURN_OK;
}

/*
 * weechat_lua_api_buffer_unmerge: unmerge a buffer from a group of merged
 *                                 buffers
 */

static int
weechat_lua_api_buffer_unmerge (lua_State *L)
{
    const char *buffer;
    int n, number;
    
    /* make C compiler happy */
    (void) L;
    
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(LUA_CURRENT_SCRIPT_NAME, "buffer_unmerge");
        LUA_RETURN_ERROR;
    }
    
    buffer = NULL;
    number = 0;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 2)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(LUA_CURRENT_SCRIPT_NAME, "buffer_unmerge");
        LUA_RETURN_ERROR;
    }
    
    buffer = lua_tostring (lua_current_interpreter, -2);
    number = lua_tonumber (lua_current_interpreter, -1);
    
    weechat_buffer_unmerge (script_str2ptr (buffer), number);
    
    LUA_RETURN_OK;
}

/*
 * weechat_lua_api_buffer_get_integer: get a buffer property as integer
 */

static int
weechat_lua_api_buffer_get_integer (lua_State *L)
{
    const char *buffer, *property;
    int n, value;
    
    /* make C compiler happy */
    (void) L;
        
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(LUA_CURRENT_SCRIPT_NAME, "buffer_get_integer");
        LUA_RETURN_INT(-1);
    }
    
    buffer = NULL;
    property = NULL;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 2)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(LUA_CURRENT_SCRIPT_NAME, "buffer_get_integer");
        LUA_RETURN_INT(-1);
    }
    
    buffer = lua_tostring (lua_current_interpreter, -2);
    property = lua_tostring (lua_current_interpreter, -1);
    
    value = weechat_buffer_get_integer (script_str2ptr (buffer),
                                        property);
    
    LUA_RETURN_INT(value);
}

/*
 * weechat_lua_api_buffer_get_string: get a buffer property as string
 */

static int
weechat_lua_api_buffer_get_string (lua_State *L)
{
    const char *buffer, *property, *result;
    int n;
    
    /* make C compiler happy */
    (void) L;
        
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(LUA_CURRENT_SCRIPT_NAME, "buffer_get_string");
        LUA_RETURN_EMPTY;
    }
    
    buffer = NULL;
    property = NULL;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 2)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(LUA_CURRENT_SCRIPT_NAME, "buffer_get_string");
        LUA_RETURN_EMPTY;
    }
    
    buffer = lua_tostring (lua_current_interpreter, -2);
    property = lua_tostring (lua_current_interpreter, -1);
    
    result = weechat_buffer_get_string (script_str2ptr (buffer),
                                        property);
    
    LUA_RETURN_STRING(result);
}

/*
 * weechat_lua_api_buffer_get_pointer: get a buffer property as pointer
 */

static int
weechat_lua_api_buffer_get_pointer (lua_State *L)
{
    const char *buffer, *property;
    char *result;
    int n;
    
    /* make C compiler happy */
    (void) L;
        
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(LUA_CURRENT_SCRIPT_NAME, "buffer_get_pointer");
        LUA_RETURN_EMPTY;
    }
    
    buffer = NULL;
    property = NULL;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 2)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(LUA_CURRENT_SCRIPT_NAME, "buffer_get_pointer");
        LUA_RETURN_EMPTY;
    }
    
    buffer = lua_tostring (lua_current_interpreter, -2);
    property = lua_tostring (lua_current_interpreter, -1);
    
    result = script_ptr2str (weechat_buffer_get_pointer (script_str2ptr (buffer),
                                                         property));
    
    LUA_RETURN_STRING_FREE(result);
}

/*
 * weechat_lua_api_buffer_set: set a buffer property
 */

static int
weechat_lua_api_buffer_set (lua_State *L)
{
    const char *buffer, *property, *value;
    int n;
    
    /* make C compiler happy */
    (void) L;
        
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(LUA_CURRENT_SCRIPT_NAME, "buffer_set");
        LUA_RETURN_ERROR;
    }
    
    buffer = NULL;
    property = NULL;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 3)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(LUA_CURRENT_SCRIPT_NAME, "buffer_set");
        LUA_RETURN_ERROR;
    }
    
    buffer = lua_tostring (lua_current_interpreter, -3);
    property = lua_tostring (lua_current_interpreter, -2);
    value = lua_tostring (lua_current_interpreter, -1);
    
    weechat_buffer_set (script_str2ptr (buffer), property, value);
    
    LUA_RETURN_OK;
}

/*
 * weechat_lua_api_current_window: get current window
 */

static int
weechat_lua_api_current_window (lua_State *L)
{
    char *result;
    
    /* make C compiler happy */
    (void) L;
    
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(LUA_CURRENT_SCRIPT_NAME, "current_window");
        LUA_RETURN_EMPTY;
    }
    
    result = script_ptr2str (weechat_current_window ());
    
    LUA_RETURN_STRING_FREE(result);
}

/*
 * weechat_lua_api_window_get_integer: get a window property as integer
 */

static int
weechat_lua_api_window_get_integer (lua_State *L)
{
    const char *window, *property;
    int n, value;
    
    /* make C compiler happy */
    (void) L;
        
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(LUA_CURRENT_SCRIPT_NAME, "window_get_integer");
        LUA_RETURN_INT(-1);
    }
    
    window = NULL;
    property = NULL;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 2)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(LUA_CURRENT_SCRIPT_NAME, "window_get_integer");
        LUA_RETURN_INT(-1);
    }
    
    window = lua_tostring (lua_current_interpreter, -2);
    property = lua_tostring (lua_current_interpreter, -1);
    
    value = weechat_window_get_integer (script_str2ptr (window),
                                        property);
    
    LUA_RETURN_INT(value);
}

/*
 * weechat_lua_api_window_get_string: get a window property as string
 */

static int
weechat_lua_api_window_get_string (lua_State *L)
{
    const char *window, *property, *result;
    int n;
    
    /* make C compiler happy */
    (void) L;
        
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(LUA_CURRENT_SCRIPT_NAME, "window_get_string");
        LUA_RETURN_EMPTY;
    }
    
    window = NULL;
    property = NULL;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 2)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(LUA_CURRENT_SCRIPT_NAME, "window_get_string");
        LUA_RETURN_EMPTY;
    }
    
    window = lua_tostring (lua_current_interpreter, -2);
    property = lua_tostring (lua_current_interpreter, -1);
    
    result = weechat_window_get_string (script_str2ptr (window),
                                        property);
    
    LUA_RETURN_STRING(result);
}

/*
 * weechat_lua_api_window_get_pointer: get a window property as pointer
 */

static int
weechat_lua_api_window_get_pointer (lua_State *L)
{
    const char *window, *property;
    char *result;
    int n;
    
    /* make C compiler happy */
    (void) L;
        
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(LUA_CURRENT_SCRIPT_NAME, "window_get_pointer");
        LUA_RETURN_EMPTY;
    }
    
    window = NULL;
    property = NULL;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 2)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(LUA_CURRENT_SCRIPT_NAME, "window_get_pointer");
        LUA_RETURN_EMPTY;
    }
    
    window = lua_tostring (lua_current_interpreter, -2);
    property = lua_tostring (lua_current_interpreter, -1);
    
    result = script_ptr2str (weechat_window_get_pointer (script_str2ptr (window),
                                                         property));
    
    LUA_RETURN_STRING_FREE(result);
}

/*
 * weechat_lua_api_window_set_title: set window title
 */

static int
weechat_lua_api_window_set_title (lua_State *L)
{
    const char *title;
    int n;
    
    /* make C compiler happy */
    (void) L;
        
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(LUA_CURRENT_SCRIPT_NAME, "window_set_title");
        LUA_RETURN_ERROR;
    }
    
    title = NULL;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 1)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(LUA_CURRENT_SCRIPT_NAME, "window_set_title");
        LUA_RETURN_ERROR;
    }
    
    title = lua_tostring (lua_current_interpreter, -1);
    
    weechat_window_set_title (title);
    
    LUA_RETURN_OK;
}

/*
 * weechat_lua_api_nicklist_add_group: add a group in nicklist
 */

static int
weechat_lua_api_nicklist_add_group (lua_State *L)
{
    const char *buffer, *parent_group, *name, *color;
    char *result;
    int n, visible;
    
    /* make C compiler happy */
    (void) L;
        
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(LUA_CURRENT_SCRIPT_NAME, "nicklist_add_group");
        LUA_RETURN_EMPTY;
    }
    
    buffer = NULL;
    parent_group = NULL;
    name = NULL;
    color = NULL;
    visible = 0;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 5)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(LUA_CURRENT_SCRIPT_NAME, "nicklist_add_group");
        LUA_RETURN_EMPTY;
    }
    
    buffer = lua_tostring (lua_current_interpreter, -5);
    parent_group = lua_tostring (lua_current_interpreter, -4);
    name = lua_tostring (lua_current_interpreter, -3);
    color = lua_tostring (lua_current_interpreter, -2);
    visible = lua_tonumber (lua_current_interpreter, -1);
    
    result = script_ptr2str (weechat_nicklist_add_group (script_str2ptr (buffer),
                                                         script_str2ptr (parent_group),
                                                         name,
                                                         color,
                                                         visible));
    
    LUA_RETURN_STRING_FREE(result);
}

/*
 * weechat_lua_api_nicklist_search_group: search a group in nicklist
 */

static int
weechat_lua_api_nicklist_search_group (lua_State *L)
{
    const char *buffer, *from_group, *name;
    char *result;
    int n;
    
    /* make C compiler happy */
    (void) L;
        
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(LUA_CURRENT_SCRIPT_NAME, "nicklist_search_group");
        LUA_RETURN_EMPTY;
    }
    
    buffer = NULL;
    from_group = NULL;
    name = NULL;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 3)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(LUA_CURRENT_SCRIPT_NAME, "nicklist_search_group");
        LUA_RETURN_EMPTY;
    }
    
    buffer = lua_tostring (lua_current_interpreter, -3);
    from_group = lua_tostring (lua_current_interpreter, -2);
    name = lua_tostring (lua_current_interpreter, -1);
    
    result = script_ptr2str (weechat_nicklist_search_group (script_str2ptr (buffer),
                                                            script_str2ptr (from_group),
                                                            name));
    
    LUA_RETURN_STRING_FREE(result);
}

/*
 * weechat_lua_api_nicklist_add_nick: add a nick in nicklist
 */

static int
weechat_lua_api_nicklist_add_nick (lua_State *L)
{
    const char *buffer, *group, *name, *color, *prefix, *prefix_color;
    char *result;
    int n, visible;
    
    /* make C compiler happy */
    (void) L;
        
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(LUA_CURRENT_SCRIPT_NAME, "nicklist_add_nick");
        LUA_RETURN_EMPTY;
    }
    
    buffer = NULL;
    group = NULL;
    name = NULL;
    color = NULL;
    prefix = NULL;
    prefix_color = NULL;
    visible = 0;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 7)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(LUA_CURRENT_SCRIPT_NAME, "nicklist_add_nick");
        LUA_RETURN_EMPTY;
    }
    
    buffer = lua_tostring (lua_current_interpreter, -7);
    group = lua_tostring (lua_current_interpreter, -6);
    name = lua_tostring (lua_current_interpreter, -5);
    color = lua_tostring (lua_current_interpreter, -4);
    prefix = lua_tostring (lua_current_interpreter, -3);
    prefix_color = lua_tostring (lua_current_interpreter, -2);
    visible = lua_tonumber (lua_current_interpreter, -1);
    
    result = script_ptr2str (weechat_nicklist_add_nick (script_str2ptr (buffer),
                                                        script_str2ptr (group),
                                                        name,
                                                        color,
                                                        prefix,
                                                        prefix_color,
                                                        visible));
    
    LUA_RETURN_STRING_FREE(result);
}

/*
 * weechat_lua_api_nicklist_search_nick: search a nick in nicklist
 */

static int
weechat_lua_api_nicklist_search_nick (lua_State *L)
{
    const char *buffer, *from_group, *name;
    char *result;
    int n;
    
    /* make C compiler happy */
    (void) L;
        
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(LUA_CURRENT_SCRIPT_NAME, "nicklist_search_nick");
        LUA_RETURN_EMPTY;
    }
    
    buffer = NULL;
    from_group = NULL;
    name = NULL;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 3)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(LUA_CURRENT_SCRIPT_NAME, "nicklist_search_nick");
        LUA_RETURN_EMPTY;
    }
    
    buffer = lua_tostring (lua_current_interpreter, -3);
    from_group = lua_tostring (lua_current_interpreter, -2);
    name = lua_tostring (lua_current_interpreter, -1);
    
    result = script_ptr2str (weechat_nicklist_search_nick (script_str2ptr (buffer),
                                                           script_str2ptr (from_group),
                                                           name));
    
    LUA_RETURN_STRING_FREE(result);
}

/*
 * weechat_lua_api_nicklist_remove_group: remove a group from nicklist
 */

static int
weechat_lua_api_nicklist_remove_group (lua_State *L)
{
    const char *buffer, *group;
    int n;
    
    /* make C compiler happy */
    (void) L;
        
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(LUA_CURRENT_SCRIPT_NAME, "nicklist_remove_group");
        LUA_RETURN_ERROR;
    }
    
    buffer = NULL;
    group = NULL;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 2)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(LUA_CURRENT_SCRIPT_NAME, "nicklist_remove_group");
        LUA_RETURN_ERROR;
    }
    
    buffer = lua_tostring (lua_current_interpreter, -3);
    group = lua_tostring (lua_current_interpreter, -2);
    
    weechat_nicklist_remove_group (script_str2ptr (buffer),
                                   script_str2ptr (group));
    
    LUA_RETURN_OK;
}

/*
 * weechat_lua_api_nicklist_remove_nick: remove a nick from nicklist
 */

static int
weechat_lua_api_nicklist_remove_nick (lua_State *L)
{
    const char *buffer, *nick;
    int n;
    
    /* make C compiler happy */
    (void) L;
        
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(LUA_CURRENT_SCRIPT_NAME, "nicklist_remove_nick");
        LUA_RETURN_ERROR;
    }
    
    buffer = NULL;
    nick = NULL;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 2)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(LUA_CURRENT_SCRIPT_NAME, "nicklist_remove_nick");
        LUA_RETURN_ERROR;
    }
    
    buffer = lua_tostring (lua_current_interpreter, -3);
    nick = lua_tostring (lua_current_interpreter, -2);
    
    weechat_nicklist_remove_nick (script_str2ptr (buffer),
                                  script_str2ptr (nick));
    
    LUA_RETURN_OK;
}

/*
 * weechat_lua_api_nicklist_remove_all: remove all groups/nicks from nicklist
 */

static int
weechat_lua_api_nicklist_remove_all (lua_State *L)
{
    const char *buffer;
    int n;
    
    /* make C compiler happy */
    (void) L;
        
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(LUA_CURRENT_SCRIPT_NAME, "nicklist_remove_all");
        LUA_RETURN_ERROR;
    }
    
    buffer = NULL;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 1)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(LUA_CURRENT_SCRIPT_NAME, "nicklist_remove_all");
        LUA_RETURN_ERROR;
    }
    
    buffer = lua_tostring (lua_current_interpreter, -3);
    
    weechat_nicklist_remove_all (script_str2ptr (buffer));
    
    LUA_RETURN_OK;
}

/*
 * weechat_lua_api_bar_item_search: search a bar item
 */

static int
weechat_lua_api_bar_item_search (lua_State *L)
{
    const char *name;
    char *result;
    int n;
    
    /* make C compiler happy */
    (void) L;
        
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(LUA_CURRENT_SCRIPT_NAME, "bar_item_search");
        LUA_RETURN_EMPTY;
    }
    
    name = NULL;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 1)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(LUA_CURRENT_SCRIPT_NAME, "bar_item_search");
        LUA_RETURN_EMPTY;
    }
    
    name = lua_tostring (lua_current_interpreter, -1);
    
    result = script_ptr2str (weechat_bar_item_search (name));
    
    LUA_RETURN_STRING_FREE(result);
}

/*
 * weechat_lua_api_bar_item_build_cb: callback for building bar item
 */

char *
weechat_lua_api_bar_item_build_cb (void *data, struct t_gui_bar_item *item,
                                   struct t_gui_window *window)
{
    struct t_script_callback *script_callback;
    char *lua_argv[4], empty_arg[1] = { '\0' }, *ret;
    
    script_callback = (struct t_script_callback *)data;

    if (script_callback && script_callback->function && script_callback->function[0])
    {
        lua_argv[0] = (script_callback->data) ? script_callback->data : empty_arg;
        lua_argv[1] = script_ptr2str (item);
        lua_argv[2] = script_ptr2str (window);
        lua_argv[3] = NULL;
        
        ret = (char *)weechat_lua_exec (script_callback->script,
                                        WEECHAT_SCRIPT_EXEC_STRING,
                                        script_callback->function,
                                        lua_argv);
        
        if (lua_argv[1])
            free (lua_argv[1]);
        if (lua_argv[2])
            free (lua_argv[2]);
        
        return ret;
    }
    
    return NULL;
}

/*
 * weechat_lua_api_bar_item_new: add a new bar item
 */

static int
weechat_lua_api_bar_item_new (lua_State *L)
{
    const char *name, *function, *data;
    char *result;
    int n;
    
    /* make C compiler happy */
    (void) L;
        
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(LUA_CURRENT_SCRIPT_NAME, "bar_item_new");
        LUA_RETURN_EMPTY;
    }
    
    name = NULL;
    function = NULL;
    data = NULL;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 3)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(LUA_CURRENT_SCRIPT_NAME, "bar_item_new");
        LUA_RETURN_EMPTY;
    }
    
    name = lua_tostring (lua_current_interpreter, -3);
    function = lua_tostring (lua_current_interpreter, -2);
    data = lua_tostring (lua_current_interpreter, -1);
    
    result = script_ptr2str (script_api_bar_item_new (weechat_lua_plugin,
                                                      lua_current_script,
                                                      name,
                                                      &weechat_lua_api_bar_item_build_cb,
                                                      function,
                                                      data));
    
    LUA_RETURN_STRING_FREE(result);
}

/*
 * weechat_lua_api_bar_item_update: update a bar item on screen
 */

static int
weechat_lua_api_bar_item_update (lua_State *L)
{
    const char *name;
    int n;
    
    /* make C compiler happy */
    (void) L;
    
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(LUA_CURRENT_SCRIPT_NAME, "bar_item_update");
        LUA_RETURN_ERROR;
    }
    
    name = NULL;
    
    n = lua_gettop (lua_current_interpreter);

    if (n < 1)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(LUA_CURRENT_SCRIPT_NAME, "bar_item_update");
        LUA_RETURN_ERROR;
    }
    
    name = lua_tostring (lua_current_interpreter, -1);
    
    weechat_bar_item_update (name);
    
    LUA_RETURN_OK;
}

/*
 * weechat_lua_api_bar_item_remove: remove a bar item
 */

static int
weechat_lua_api_bar_item_remove (lua_State *L)
{
    const char *item;
    int n;
    
    /* make C compiler happy */
    (void) L;
    
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(LUA_CURRENT_SCRIPT_NAME, "bar_item_remove");
        LUA_RETURN_ERROR;
    }
    
    item = NULL;
    
    n = lua_gettop (lua_current_interpreter);

    if (n < 1)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(LUA_CURRENT_SCRIPT_NAME, "bar_item_remove");
        LUA_RETURN_ERROR;
    }
    
    item = lua_tostring (lua_current_interpreter, -1);
    
    script_api_bar_item_remove (weechat_lua_plugin,
                                lua_current_script,
                                script_str2ptr (item));
    
    LUA_RETURN_OK;
}

/*
 * weechat_lua_api_bar_search: search a bar
 */

static int
weechat_lua_api_bar_search (lua_State *L)
{
    const char *name;
    char *result;
    int n;
    
    /* make C compiler happy */
    (void) L;
        
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(LUA_CURRENT_SCRIPT_NAME, "bar_search");
        LUA_RETURN_EMPTY;
    }
    
    name = NULL;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 1)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(LUA_CURRENT_SCRIPT_NAME, "bar_search");
        LUA_RETURN_EMPTY;
    }
    
    name = lua_tostring (lua_current_interpreter, -1);
    
    result = script_ptr2str (weechat_bar_search (name));
    
    LUA_RETURN_STRING_FREE(result);
}

/*
 * weechat_lua_api_bar_new: add a new bar
 */

static int
weechat_lua_api_bar_new (lua_State *L)
{
    const char *name, *hidden, *priority, *type, *conditions, *position;
    const char *filling_top_bottom, *filling_left_right, *size, *size_max;
    const char *color_fg, *color_delim, *color_bg, *separator, *items;
    char *result;
    int n;
    
    /* make C compiler happy */
    (void) L;
        
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(LUA_CURRENT_SCRIPT_NAME, "bar_new");
        LUA_RETURN_EMPTY;
    }
    
    name = NULL;
    hidden = NULL;
    priority = NULL;
    type = NULL;
    conditions = NULL;
    position = NULL;
    filling_top_bottom = NULL;
    filling_left_right = NULL;
    size = NULL;
    size_max = NULL;
    color_fg = NULL;
    color_delim = NULL;
    color_bg = NULL;
    separator = NULL;
    items = NULL;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 15)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(LUA_CURRENT_SCRIPT_NAME, "bar_new");
        LUA_RETURN_EMPTY;
    }
    
    name = lua_tostring (lua_current_interpreter, -15);
    hidden = lua_tostring (lua_current_interpreter, -14);
    priority = lua_tostring (lua_current_interpreter, -13);
    type = lua_tostring (lua_current_interpreter, -12);
    conditions = lua_tostring (lua_current_interpreter, -11);
    position = lua_tostring (lua_current_interpreter, -10);
    filling_top_bottom = lua_tostring (lua_current_interpreter, -9);
    filling_left_right = lua_tostring (lua_current_interpreter, -8);
    size = lua_tostring (lua_current_interpreter, -7);
    size_max = lua_tostring (lua_current_interpreter, -6);
    color_fg = lua_tostring (lua_current_interpreter, -5);
    color_delim = lua_tostring (lua_current_interpreter, -4);
    color_bg = lua_tostring (lua_current_interpreter, -3);
    separator = lua_tostring (lua_current_interpreter, -2);
    items = lua_tostring (lua_current_interpreter, -1);
    
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
                                              items));
    
    LUA_RETURN_STRING_FREE(result);
}

/*
 * weechat_lua_api_bar_set: set a bar property
 */

static int
weechat_lua_api_bar_set (lua_State *L)
{
    const char *bar, *property, *value;
    int n;
    
    /* make C compiler happy */
    (void) L;
        
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(LUA_CURRENT_SCRIPT_NAME, "bar_set");
        LUA_RETURN_ERROR;
    }
    
    bar = NULL;
    property = NULL;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 3)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(LUA_CURRENT_SCRIPT_NAME, "bar_set");
        LUA_RETURN_ERROR;
    }
    
    bar = lua_tostring (lua_current_interpreter, -3);
    property = lua_tostring (lua_current_interpreter, -2);
    value = lua_tostring (lua_current_interpreter, -1);
    
    weechat_bar_set (script_str2ptr (bar),
                     property,
                     value);
    
    LUA_RETURN_OK;
}

/*
 * weechat_lua_api_bar_update: update a bar on screen
 */

static int
weechat_lua_api_bar_update (lua_State *L)
{
    const char *name;
    int n;
    
    /* make C compiler happy */
    (void) L;
    
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(LUA_CURRENT_SCRIPT_NAME, "bar_update");
        LUA_RETURN_ERROR;
    }
    
    name = NULL;
    
    n = lua_gettop (lua_current_interpreter);

    if (n < 1)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(LUA_CURRENT_SCRIPT_NAME, "bar_update");
        LUA_RETURN_ERROR;
    }
    
    name = lua_tostring (lua_current_interpreter, -1);
    
    weechat_bar_update (name);
    
    LUA_RETURN_OK;
}

/*
 * weechat_lua_api_bar_remove: remove a bar
 */

static int
weechat_lua_api_bar_remove (lua_State *L)
{
    const char *bar;
    int n;
    
    /* make C compiler happy */
    (void) L;
    
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(LUA_CURRENT_SCRIPT_NAME, "bar_remove");
        LUA_RETURN_ERROR;
    }
    
    bar = NULL;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 1)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(LUA_CURRENT_SCRIPT_NAME, "bar_remove");
        LUA_RETURN_ERROR;
    }
    
    bar = lua_tostring (lua_current_interpreter, -1);
    
    weechat_bar_remove (script_str2ptr (bar));
    
    LUA_RETURN_OK;
}

/*
 * weechat_lua_api_command: send command to server
 */

static int
weechat_lua_api_command (lua_State *L)
{
    const char *buffer, *command;
    int n;
    
    /* make C compiler happy */
    (void) L;
    
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(LUA_CURRENT_SCRIPT_NAME, "command");
        LUA_RETURN_ERROR;
    }
    
    buffer = NULL;
    command = NULL;
    
    n = lua_gettop (lua_current_interpreter);

    if (n < 2)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(LUA_CURRENT_SCRIPT_NAME, "command");
        LUA_RETURN_ERROR;
    }
    
    buffer = lua_tostring (lua_current_interpreter, -2);
    command = lua_tostring (lua_current_interpreter, -1);
    
    script_api_command (weechat_lua_plugin,
                        lua_current_script,
                        script_str2ptr (buffer),
                        command);

    LUA_RETURN_OK;
}

/*
 * weechat_lua_api_info_get: get info about WeeChat
 */

static int
weechat_lua_api_info_get (lua_State *L)
{
    const char *info_name, *arguments, *result;
    int n;
    
    /* make C compiler happy */
    (void) L;
    
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(LUA_CURRENT_SCRIPT_NAME, "info_get");
        LUA_RETURN_EMPTY;
    }
    
    info_name = NULL;
    arguments = NULL;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 2)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(LUA_CURRENT_SCRIPT_NAME, "info_get");
        LUA_RETURN_EMPTY;
    }
    
    info_name = lua_tostring (lua_current_interpreter, -2);
    arguments = lua_tostring (lua_current_interpreter, -1);
    
    result = weechat_info_get (info_name, arguments);
    
    LUA_RETURN_STRING(result);
}

/*
 * weechat_lua_api_infolist_new: create new infolist
 */

static int
weechat_lua_api_infolist_new (lua_State *L)
{
    char *result;
    
    /* make C compiler happy */
    (void) L;
    
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(LUA_CURRENT_SCRIPT_NAME, "infolist_new");
        LUA_RETURN_EMPTY;
    }
    
    result = script_ptr2str (weechat_infolist_new ());
    
    LUA_RETURN_STRING_FREE(result);
}

/*
 * weechat_lua_api_infolist_new_var_integer: create new integer variable in
 *                                           infolist
 */

static int
weechat_lua_api_infolist_new_var_integer (lua_State *L)
{
    const char *infolist, *name;
    char *result;
    int n, value;
    
    /* make C compiler happy */
    (void) L;
    
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(LUA_CURRENT_SCRIPT_NAME, "infolist_new_var_integer");
        LUA_RETURN_EMPTY;
    }
    
    infolist = NULL;
    name = NULL;
    value = 0;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 3)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(LUA_CURRENT_SCRIPT_NAME, "infolist_new_var_integer");
        LUA_RETURN_EMPTY;
    }
    
    infolist = lua_tostring (lua_current_interpreter, -3);
    name = lua_tostring (lua_current_interpreter, -2);
    value = lua_tonumber (lua_current_interpreter, -1);
    
    result = script_ptr2str (weechat_infolist_new_var_integer (script_str2ptr (infolist),
                                                               name,
                                                               value));
    
    LUA_RETURN_STRING_FREE(result);
}

/*
 * weechat_lua_api_infolist_new_var_string: create new string variable in
 *                                          infolist
 */

static int
weechat_lua_api_infolist_new_var_string (lua_State *L)
{
    const char *infolist, *name, *value;
    char *result;
    int n;
    
    /* make C compiler happy */
    (void) L;
    
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(LUA_CURRENT_SCRIPT_NAME, "infolist_new_var_string");
        LUA_RETURN_EMPTY;
    }
    
    infolist = NULL;
    name = NULL;
    value = NULL;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 3)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(LUA_CURRENT_SCRIPT_NAME, "infolist_new_var_string");
        LUA_RETURN_EMPTY;
    }
    
    infolist = lua_tostring (lua_current_interpreter, -3);
    name = lua_tostring (lua_current_interpreter, -2);
    value = lua_tostring (lua_current_interpreter, -1);
    
    result = script_ptr2str (weechat_infolist_new_var_string (script_str2ptr (infolist),
                                                              name,
                                                              value));
    
    LUA_RETURN_STRING_FREE(result);
}

/*
 * weechat_lua_api_infolist_new_var_pointer: create new pointer variable in
 *                                           infolist
 */

static int
weechat_lua_api_infolist_new_var_pointer (lua_State *L)
{
    const char *infolist, *name, *value;
    char *result;
    int n;
    
    /* make C compiler happy */
    (void) L;
    
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(LUA_CURRENT_SCRIPT_NAME, "infolist_new_var_pointer");
        LUA_RETURN_EMPTY;
    }
    
    infolist = NULL;
    name = NULL;
    value = NULL;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 3)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(LUA_CURRENT_SCRIPT_NAME, "infolist_new_var_pointer");
        LUA_RETURN_EMPTY;
    }
    
    infolist = lua_tostring (lua_current_interpreter, -3);
    name = lua_tostring (lua_current_interpreter, -2);
    value = lua_tostring (lua_current_interpreter, -1);
    
    result = script_ptr2str (weechat_infolist_new_var_pointer (script_str2ptr (infolist),
                                                               name,
                                                               script_str2ptr (value)));
    
    LUA_RETURN_STRING_FREE(result);
}

/*
 * weechat_lua_api_infolist_new_var_time: create new time variable in infolist
 */

static int
weechat_lua_api_infolist_new_var_time (lua_State *L)
{
    const char *infolist, *name;
    char *result;
    int n, value;
    
    /* make C compiler happy */
    (void) L;
    
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(LUA_CURRENT_SCRIPT_NAME, "infolist_new_var_time");
        LUA_RETURN_EMPTY;
    }
    
    infolist = NULL;
    name = NULL;
    value = 0;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 3)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(LUA_CURRENT_SCRIPT_NAME, "infolist_new_var_time");
        LUA_RETURN_EMPTY;
    }
    
    infolist = lua_tostring (lua_current_interpreter, -3);
    name = lua_tostring (lua_current_interpreter, -2);
    value = lua_tonumber (lua_current_interpreter, -1);
    
    result = script_ptr2str (weechat_infolist_new_var_time (script_str2ptr (infolist),
                                                            name,
                                                            value));
    
    LUA_RETURN_STRING_FREE(result);
}

/*
 * weechat_lua_api_infolist_get: get list with infos
 */

static int
weechat_lua_api_infolist_get (lua_State *L)
{
    const char *name, *pointer, *arguments;
    char *result;
    int n;
    
    /* make C compiler happy */
    (void) L;
    
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(LUA_CURRENT_SCRIPT_NAME, "infolist_get");
        LUA_RETURN_EMPTY;
    }
    
    name = NULL;
    pointer = NULL;
    arguments = NULL;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 3)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(LUA_CURRENT_SCRIPT_NAME, "infolist_get");
        LUA_RETURN_EMPTY;
    }
    
    name = lua_tostring (lua_current_interpreter, -3);
    pointer = lua_tostring (lua_current_interpreter, -2);
    arguments = lua_tostring (lua_current_interpreter, -1);
    
    result = script_ptr2str (weechat_infolist_get (name,
                                                   script_str2ptr (pointer),
                                                   arguments));
    
    LUA_RETURN_STRING_FREE(result);
}

/*
 * weechat_lua_api_infolist_next: move item pointer to next item in infolist
 */

static int
weechat_lua_api_infolist_next (lua_State *L)
{
    const char *infolist;
    int n, value;
    
    /* make C compiler happy */
    (void) L;
    
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(LUA_CURRENT_SCRIPT_NAME, "infolist_next");
        LUA_RETURN_INT(0);
    }
    
    infolist = NULL;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 1)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(LUA_CURRENT_SCRIPT_NAME, "infolist_next");
        LUA_RETURN_INT(0);
    }
    
    infolist = lua_tostring (lua_current_interpreter, -1);
    
    value = weechat_infolist_next (script_str2ptr (infolist));
    
    LUA_RETURN_INT(value);
}

/*
 * weechat_lua_api_infolist_prev: move item pointer to previous item in infolist
 */

static int
weechat_lua_api_infolist_prev (lua_State *L)
{
    const char *infolist;
    int n, value;
    
    /* make C compiler happy */
    (void) L;
    
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(LUA_CURRENT_SCRIPT_NAME, "infolist_prev");
        LUA_RETURN_INT(0);
    }
    
    infolist = NULL;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 1)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(LUA_CURRENT_SCRIPT_NAME, "infolist_prev");
        LUA_RETURN_INT(0);
    }
    
    infolist = lua_tostring (lua_current_interpreter, -1);
    
    value = weechat_infolist_prev (script_str2ptr (infolist));
    
    LUA_RETURN_INT(value);
}

/*
 * weechat_lua_api_infolist_fields: get list of fields for current item of infolist
 */

static int
weechat_lua_api_infolist_fields (lua_State *L)
{
    const char *infolist, *result;
    int n;
    
    /* make C compiler happy */
    (void) L;
    
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(LUA_CURRENT_SCRIPT_NAME, "infolist_fields");
        LUA_RETURN_EMPTY;
    }
    
    infolist = NULL;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 1)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(LUA_CURRENT_SCRIPT_NAME, "infolist_fields");
        LUA_RETURN_EMPTY;
    }
    
    infolist = lua_tostring (lua_current_interpreter, -1);
    
    result = weechat_infolist_fields (script_str2ptr (infolist));
    
    LUA_RETURN_STRING(result);
}

/*
 * weechat_lua_api_infolist_integer: get integer value of a variable in infolist
 */

static int
weechat_lua_api_infolist_integer (lua_State *L)
{
    const char *infolist, *variable;
    int n, value;
    
    /* make C compiler happy */
    (void) L;
    
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(LUA_CURRENT_SCRIPT_NAME, "infolist_integer");
        LUA_RETURN_INT(0);
    }
    
    infolist = NULL;
    variable = NULL;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 2)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(LUA_CURRENT_SCRIPT_NAME, "infolist_integer");
        LUA_RETURN_INT(0);
    }
    
    infolist = lua_tostring (lua_current_interpreter, -2);
    variable = lua_tostring (lua_current_interpreter, -1);
    
    value = weechat_infolist_integer (script_str2ptr (infolist),
                                      variable);
    
    LUA_RETURN_INT(value);
}

/*
 * weechat_lua_api_infolist_string: get string value of a variable in infolist
 */

static int
weechat_lua_api_infolist_string (lua_State *L)
{
    const char *infolist, *variable, *result;
    int n;
    
    /* make C compiler happy */
    (void) L;
    
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(LUA_CURRENT_SCRIPT_NAME, "infolist_string");
        LUA_RETURN_EMPTY;
    }
    
    infolist = NULL;
    variable = NULL;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 2)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(LUA_CURRENT_SCRIPT_NAME, "infolist_string");
        LUA_RETURN_EMPTY;
    }
    
    infolist = lua_tostring (lua_current_interpreter, -2);
    variable = lua_tostring (lua_current_interpreter, -1);
    
    result = weechat_infolist_string (script_str2ptr (infolist),
                                     variable);
    
    LUA_RETURN_STRING(result);
}

/*
 * weechat_lua_api_infolist_pointer: get pointer value of a variable in infolist
 */

static int
weechat_lua_api_infolist_pointer (lua_State *L)
{
    const char *infolist, *variable;
    char *result;
    int n;
    
    /* make C compiler happy */
    (void) L;
    
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(LUA_CURRENT_SCRIPT_NAME, "infolist_pointer");
        LUA_RETURN_EMPTY;
    }
    
    infolist = NULL;
    variable = NULL;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 2)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(LUA_CURRENT_SCRIPT_NAME, "infolist_pointer");
        LUA_RETURN_EMPTY;
    }
    
    infolist = lua_tostring (lua_current_interpreter, -2);
    variable = lua_tostring (lua_current_interpreter, -1);
    
    result = script_ptr2str (weechat_infolist_pointer (script_str2ptr (infolist),
                                                       variable));
    
    LUA_RETURN_STRING_FREE(result);
}

/*
 * weechat_lua_api_infolist_time: get time value of a variable in infolist
 */

static int
weechat_lua_api_infolist_time (lua_State *L)
{
    const char *infolist, *variable;
    time_t time;
    char timebuffer[64], *result;
    int n;
    
    /* make C compiler happy */
    (void) L;
    
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(LUA_CURRENT_SCRIPT_NAME, "infolist_time");
        LUA_RETURN_EMPTY;
    }
    
    infolist = NULL;
    variable = NULL;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 2)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(LUA_CURRENT_SCRIPT_NAME, "infolist_time");
        LUA_RETURN_EMPTY;
    }
    
    infolist = lua_tostring (lua_current_interpreter, -2);
    variable = lua_tostring (lua_current_interpreter, -1);
    
    time = weechat_infolist_time (script_str2ptr (infolist),
                                  variable);
    strftime (timebuffer, sizeof (timebuffer), "%F %T", localtime (&time));
    result = strdup (timebuffer);
    
    LUA_RETURN_STRING_FREE(result);
}

/*
 * weechat_lua_api_infolist_free: free infolist
 */

static int
weechat_lua_api_infolist_free (lua_State *L)
{
    const char *infolist;
    int n;
    
    /* make C compiler happy */
    (void) L;
    
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(LUA_CURRENT_SCRIPT_NAME, "infolist_free");
        LUA_RETURN_ERROR;
    }
    
    infolist = NULL;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 1)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(LUA_CURRENT_SCRIPT_NAME, "infolist_free");
        LUA_RETURN_ERROR;
    }
    
    infolist = lua_tostring (lua_current_interpreter, -1);
    
    weechat_infolist_free (script_str2ptr (infolist));
    
    LUA_RETURN_OK;
}

/*
 * weechat_lua_api_config_new: create a new configuration file
 */

static int
weechat_lua_api_upgrade_new (lua_State *L)
{
    const char *filename;
    char *result;
    int n, write;
    
    /* make C compiler happy */
    (void) L;
    
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(LUA_CURRENT_SCRIPT_NAME, "upgrade_new");
        LUA_RETURN_EMPTY;
    }
    
    filename = NULL;
    write = 0;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 2)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(LUA_CURRENT_SCRIPT_NAME, "upgrade_new");
        LUA_RETURN_EMPTY;
    }
    
    filename = lua_tostring (lua_current_interpreter, -2);
    write = lua_tonumber (lua_current_interpreter, -1);
    
    result = script_ptr2str (weechat_upgrade_new (filename, write));
    
    LUA_RETURN_STRING_FREE(result);
}

/*
 * weechat_lua_api_upgrade_write_object: write object in upgrade file
 */

static int
weechat_lua_api_upgrade_write_object (lua_State *L)
{
    const char *upgrade_file, *infolist;
    int n, object_id, rc;
    
    /* make C compiler happy */
    (void) L;
    
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(LUA_CURRENT_SCRIPT_NAME, "upgrade_write_object");
        LUA_RETURN_INT(0);
    }
    
    upgrade_file = NULL;
    object_id = 0;
    infolist = NULL;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 3)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(LUA_CURRENT_SCRIPT_NAME, "upgrade_write_object");
        LUA_RETURN_INT(0);
    }
    
    upgrade_file = lua_tostring (lua_current_interpreter, -3);
    object_id = lua_tonumber (lua_current_interpreter, -2);
    infolist = lua_tostring (lua_current_interpreter, -1);
    
    rc = weechat_upgrade_write_object (script_str2ptr (upgrade_file),
                                       object_id,
                                       script_str2ptr (infolist));
    
    LUA_RETURN_INT(rc);
}

/*
 * weechat_lua_api_upgrade_read_cb: callback for reading object in upgrade file
 */

int
weechat_lua_api_upgrade_read_cb (void *data,
                                 struct t_upgrade_file *upgrade_file,
                                 int object_id,
                                 struct t_infolist *infolist)
{
    struct t_script_callback *script_callback;
    char *lua_argv[5], empty_arg[1] = { '\0' }, str_object_id[32];
    int *rc, ret;
    
    script_callback = (struct t_script_callback *)data;
    
    if (script_callback && script_callback->function && script_callback->function[0])
    {
        snprintf (str_object_id, sizeof (str_object_id), "%d", object_id);
        
        lua_argv[0] = (script_callback->data) ? script_callback->data : empty_arg;
        lua_argv[1] = script_ptr2str (upgrade_file);
        lua_argv[2] = str_object_id;
        lua_argv[3] = script_ptr2str (infolist);
        lua_argv[4] = NULL;
        
        rc = (int *) weechat_lua_exec (script_callback->script,
                                       WEECHAT_SCRIPT_EXEC_INT,
                                       script_callback->function,
                                       lua_argv);
        
        if (!rc)
            ret = WEECHAT_RC_ERROR;
        else
        {
            ret = *rc;
            free (rc);
        }
        if (lua_argv[1])
            free (lua_argv[1]);
        if (lua_argv[3])
            free (lua_argv[3]);
        
        return ret;
    }
    
    return WEECHAT_RC_ERROR;
}

/*
 * weechat_lua_api_upgrade_read: read upgrade file
 */

static int
weechat_lua_api_upgrade_read (lua_State *L)
{
    const char *upgrade_file, *function, *data;
    int n, rc;
    
    /* make C compiler happy */
    (void) L;
    
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(LUA_CURRENT_SCRIPT_NAME, "upgrade_read");
        LUA_RETURN_EMPTY;
    }
    
    upgrade_file = NULL;
    function = NULL;
    data = NULL;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 3)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(LUA_CURRENT_SCRIPT_NAME, "upgrade_read");
        LUA_RETURN_EMPTY;
    }
    
    upgrade_file = lua_tostring (lua_current_interpreter, -3);
    function = lua_tostring (lua_current_interpreter, -2);
    data = lua_tostring (lua_current_interpreter, -1);
    
    rc = script_api_upgrade_read (weechat_lua_plugin,
                                  lua_current_script,
                                  script_str2ptr (upgrade_file),
                                  &weechat_lua_api_upgrade_read_cb,
                                  function,
                                  data);
    
    LUA_RETURN_INT(rc);
}

/*
 * weechat_lua_api_upgrade_close: close upgrade file
 */

static int
weechat_lua_api_upgrade_close (lua_State *L)
{
    const char *upgrade_file;
    int n;
    
    /* make C compiler happy */
    (void) L;
    
    if (!lua_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(LUA_CURRENT_SCRIPT_NAME, "upgrade_close");
        LUA_RETURN_ERROR;
    }
    
    upgrade_file = NULL;
    
    n = lua_gettop (lua_current_interpreter);
    
    if (n < 1)
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(LUA_CURRENT_SCRIPT_NAME, "upgrade_close");
        LUA_RETURN_INT(0);
    }
    
    upgrade_file = lua_tostring (lua_current_interpreter, -1);
    
    weechat_upgrade_close (script_str2ptr (upgrade_file));
    
    LUA_RETURN_OK;
}

/*
 * Lua constant as functions
 */

static int
weechat_lua_api_constant_weechat_rc_ok (lua_State *L)
{
    /* make C compiler happy */
    (void) L;
    
    lua_pushnumber (lua_current_interpreter, WEECHAT_RC_OK);
    return 1;
}

static int
weechat_lua_api_constant_weechat_rc_ok_eat (lua_State *L)
{
    /* make C compiler happy */
    (void) L;
    
    lua_pushnumber (lua_current_interpreter, WEECHAT_RC_OK_EAT);
    return 1;
}

static int
weechat_lua_api_constant_weechat_rc_error (lua_State *L)
{
    /* make C compiler happy */
    (void) L;
    
    lua_pushnumber (lua_current_interpreter, WEECHAT_RC_ERROR);
    return 1;
}

static int
weechat_lua_api_constant_weechat_config_read_ok (lua_State *L)
{
    /* make C compiler happy */
    (void) L;
    
    lua_pushnumber (lua_current_interpreter, WEECHAT_CONFIG_READ_OK);
    return 1;
}

static int
weechat_lua_api_constant_weechat_config_read_memory_error (lua_State *L)
{
    /* make C compiler happy */
    (void) L;
    
    lua_pushnumber (lua_current_interpreter, WEECHAT_CONFIG_READ_MEMORY_ERROR);
    return 1;
}

static int
weechat_lua_api_constant_weechat_config_read_file_not_found (lua_State *L)
{
    /* make C compiler happy */
    (void) L;
    
    lua_pushnumber (lua_current_interpreter, WEECHAT_CONFIG_READ_FILE_NOT_FOUND);
    return 1;
}

static int
weechat_lua_api_constant_weechat_config_write_ok (lua_State *L)
{
    /* make C compiler happy */
    (void) L;
    
    lua_pushnumber (lua_current_interpreter, WEECHAT_CONFIG_WRITE_OK);
    return 1;
}

static int
weechat_lua_api_constant_weechat_config_write_error (lua_State *L)
{
    /* make C compiler happy */
    (void) L;
    
    lua_pushnumber (lua_current_interpreter, WEECHAT_CONFIG_WRITE_ERROR);
    return 1;
}

static int
weechat_lua_api_constant_weechat_config_write_memory_error (lua_State *L)
{
    /* make C compiler happy */
    (void) L;
    
    lua_pushnumber (lua_current_interpreter, WEECHAT_CONFIG_WRITE_MEMORY_ERROR);
    return 1;
}

static int
weechat_lua_api_constant_weechat_config_option_set_ok_changed (lua_State *L)
{
    /* make C compiler happy */
    (void) L;
    
    lua_pushnumber (lua_current_interpreter, WEECHAT_CONFIG_OPTION_SET_OK_CHANGED);
    return 1;
}

static int
weechat_lua_api_constant_weechat_config_option_set_ok_same_value (lua_State *L)
{
    /* make C compiler happy */
    (void) L;
    
    lua_pushnumber (lua_current_interpreter, WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE);
    return 1;
}

static int
weechat_lua_api_constant_weechat_config_option_set_error (lua_State *L)
{
    /* make C compiler happy */
    (void) L;
    
    lua_pushnumber (lua_current_interpreter, WEECHAT_CONFIG_OPTION_SET_ERROR);
    return 1;
}

static int
weechat_lua_api_constant_weechat_config_option_set_option_not_found (lua_State *L)
{
    /* make C compiler happy */
    (void) L;
    
    lua_pushnumber (lua_current_interpreter, WEECHAT_CONFIG_OPTION_SET_OPTION_NOT_FOUND);
    return 1;
}

static int
weechat_lua_api_constant_weechat_config_option_unset_ok_no_reset (lua_State *L)
{
    /* make C compiler happy */
    (void) L;
    
    lua_pushnumber (lua_current_interpreter, WEECHAT_CONFIG_OPTION_UNSET_OK_NO_RESET);
    return 1;
}

static int
weechat_lua_api_constant_weechat_config_option_unset_ok_reset (lua_State *L)
{
    /* make C compiler happy */
    (void) L;
    
    lua_pushnumber (lua_current_interpreter, WEECHAT_CONFIG_OPTION_UNSET_OK_RESET);
    return 1;
}

static int
weechat_lua_api_constant_weechat_config_option_unset_ok_removed (lua_State *L)
{
    /* make C compiler happy */
    (void) L;
    
    lua_pushnumber (lua_current_interpreter, WEECHAT_CONFIG_OPTION_UNSET_OK_REMOVED);
    return 1;
}

static int
weechat_lua_api_constant_weechat_config_option_unset_error (lua_State *L)
{
    /* make C compiler happy */
    (void) L;
    
    lua_pushnumber (lua_current_interpreter, WEECHAT_CONFIG_OPTION_UNSET_ERROR);
    return 1;
}

static int
weechat_lua_api_constant_weechat_list_pos_sort (lua_State *L)
{
    /* make C compiler happy */
    (void) L;
    
    lua_pushstring (lua_current_interpreter, WEECHAT_LIST_POS_SORT);
    return 1;
}

static int
weechat_lua_api_constant_weechat_list_pos_beginning (lua_State *L)
{
    /* make C compiler happy */
    (void) L;
    
    lua_pushstring (lua_current_interpreter, WEECHAT_LIST_POS_BEGINNING);
    return 1;
}

static int
weechat_lua_api_constant_weechat_list_pos_end (lua_State *L)
{
    /* make C compiler happy */
    (void) L;
    
    lua_pushstring (lua_current_interpreter, WEECHAT_LIST_POS_END);
    return 1;
}

static int
weechat_lua_api_constant_weechat_hotlist_low (lua_State *L)
{
    /* make C compiler happy */
    (void) L;
    
    lua_pushstring (lua_current_interpreter, WEECHAT_HOTLIST_LOW);
    return 1;
}

static int
weechat_lua_api_constant_weechat_hotlist_message (lua_State *L)
{
    /* make C compiler happy */
    (void) L;
    
    lua_pushstring (lua_current_interpreter, WEECHAT_HOTLIST_MESSAGE);
    return 1;
}

static int
weechat_lua_api_constant_weechat_hotlist_private (lua_State *L)
{
    /* make C compiler happy */
    (void) L;
    
    lua_pushstring (lua_current_interpreter, WEECHAT_HOTLIST_PRIVATE);
    return 1;
}

static int
weechat_lua_api_constant_weechat_hotlist_highlight (lua_State *L)
{
    /* make C compiler happy */
    (void) L;
    
    lua_pushstring (lua_current_interpreter, WEECHAT_HOTLIST_HIGHLIGHT);
    return 1;
}

static int
weechat_lua_api_constant_weechat_hook_process_running (lua_State *L)
{
    /* make C compiler happy */
    (void) L;
    
    lua_pushnumber (lua_current_interpreter, WEECHAT_HOOK_PROCESS_RUNNING);
    return 1;
}

static int
weechat_lua_api_constant_weechat_hook_process_error (lua_State *L)
{
    /* make C compiler happy */
    (void) L;
    
    lua_pushnumber (lua_current_interpreter, WEECHAT_HOOK_PROCESS_ERROR);
    return 1;
}

static int
weechat_lua_api_constant_weechat_hook_connect_ok (lua_State *L)
{
    /* make C compiler happy */
    (void) L;
    
    lua_pushnumber (lua_current_interpreter, WEECHAT_HOOK_CONNECT_OK);
    return 1;
}

static int
weechat_lua_api_constant_weechat_hook_connect_address_not_found (lua_State *L)
{
    /* make C compiler happy */
    (void) L;
    
    lua_pushnumber (lua_current_interpreter, WEECHAT_HOOK_CONNECT_ADDRESS_NOT_FOUND);
    return 1;
}

static int
weechat_lua_api_constant_weechat_hook_connect_ip_address_not_found (lua_State *L)
{
    /* make C compiler happy */
    (void) L;
    
    lua_pushnumber (lua_current_interpreter, WEECHAT_HOOK_CONNECT_IP_ADDRESS_NOT_FOUND);
    return 1;
}

static int
weechat_lua_api_constant_weechat_hook_connect_connection_refused (lua_State *L)
{
    /* make C compiler happy */
    (void) L;
    
    lua_pushnumber (lua_current_interpreter, WEECHAT_HOOK_CONNECT_CONNECTION_REFUSED);
    return 1;
}

static int
weechat_lua_api_constant_weechat_hook_connect_proxy_error (lua_State *L)
{
    /* make C compiler happy */
    (void) L;
    
    lua_pushnumber (lua_current_interpreter, WEECHAT_HOOK_CONNECT_PROXY_ERROR);
    return 1;
}

static int
weechat_lua_api_constant_weechat_hook_connect_local_hostname_error (lua_State *L)
{
    /* make C compiler happy */
    (void) L;
    
    lua_pushnumber (lua_current_interpreter, WEECHAT_HOOK_CONNECT_LOCAL_HOSTNAME_ERROR);
    return 1;
}

static int
weechat_lua_api_constant_weechat_hook_connect_gnutls_init_error (lua_State *L)
{
    /* make C compiler happy */
    (void) L;
    
    lua_pushnumber (lua_current_interpreter, WEECHAT_HOOK_CONNECT_GNUTLS_INIT_ERROR);
    return 1;
}

static int
weechat_lua_api_constant_weechat_hook_connect_gnutls_handshake_error (lua_State *L)
{
    /* make C compiler happy */
    (void) L;
    
    lua_pushnumber (lua_current_interpreter, WEECHAT_HOOK_CONNECT_GNUTLS_HANDSHAKE_ERROR);
    return 1;
}

static int
weechat_lua_api_constant_weechat_hook_connect_memory_error (lua_State *L)
{
    /* make C compiler happy */
    (void) L;
    
    lua_pushnumber (lua_current_interpreter, WEECHAT_HOOK_CONNECT_MEMORY_ERROR);
    return 1;
}

static int
weechat_lua_api_constant_weechat_hook_signal_string (lua_State *L)
{
    /* make C compiler happy */
    (void) L;
    
    lua_pushstring (lua_current_interpreter, WEECHAT_HOOK_SIGNAL_STRING);
    return 1;
}

static int
weechat_lua_api_constant_weechat_hook_signal_int (lua_State *L)
{
    /* make C compiler happy */
    (void) L;
    
    lua_pushstring (lua_current_interpreter, WEECHAT_HOOK_SIGNAL_INT);
    return 1;
}

static int
weechat_lua_api_constant_weechat_hook_signal_pointer (lua_State *L)
{
    /* make C compiler happy */
    (void) L;
    
    lua_pushstring (lua_current_interpreter, WEECHAT_HOOK_SIGNAL_POINTER);
    return 1;
}

/*
 * Lua subroutines
 */

const struct luaL_reg weechat_lua_api_funcs[] = {
    { "register", &weechat_lua_api_register },
    { "plugin_get_name", &weechat_lua_api_plugin_get_name },
    { "charset_set", &weechat_lua_api_charset_set },
    { "iconv_to_internal", &weechat_lua_api_iconv_to_internal },
    { "iconv_from_internal", &weechat_lua_api_iconv_from_internal },
    { "gettext", &weechat_lua_api_gettext },
    { "ngettext", &weechat_lua_api_ngettext },
    { "string_remove_color", &weechat_lua_api_string_remove_color },
    { "mkdir_home", &weechat_lua_api_mkdir_home },
    { "mkdir", &weechat_lua_api_mkdir },
    { "mkdir_parents", &weechat_lua_api_mkdir_parents },
    { "list_new", &weechat_lua_api_list_new },
    { "list_add", &weechat_lua_api_list_add },
    { "list_search", &weechat_lua_api_list_search },
    { "list_casesearch", &weechat_lua_api_list_casesearch },
    { "list_get", &weechat_lua_api_list_get },
    { "list_set", &weechat_lua_api_list_set },
    { "list_next", &weechat_lua_api_list_next },
    { "list_prev", &weechat_lua_api_list_prev },
    { "list_string", &weechat_lua_api_list_string },
    { "list_size", &weechat_lua_api_list_size },
    { "list_remove", &weechat_lua_api_list_remove },
    { "list_remove_all", &weechat_lua_api_list_remove_all },
    { "list_free", &weechat_lua_api_list_free },
    { "config_new", &weechat_lua_api_config_new },
    { "config_new_section", &weechat_lua_api_config_new_section },
    { "config_search_section", &weechat_lua_api_config_search_section },
    { "config_new_option", &weechat_lua_api_config_new_option },
    { "config_search_option", &weechat_lua_api_config_search_option },
    { "config_string_to_boolean", &weechat_lua_api_config_string_to_boolean },
    { "config_option_reset", &weechat_lua_api_config_option_reset },
    { "config_option_set", &weechat_lua_api_config_option_set },
    { "config_option_set_null", &weechat_lua_api_config_option_set_null },
    { "config_option_unset", &weechat_lua_api_config_option_unset },
    { "config_option_rename", &weechat_lua_api_config_option_rename },
    { "config_option_is_null", &weechat_lua_api_config_option_is_null },
    { "config_option_default_is_null", &weechat_lua_api_config_option_default_is_null },
    { "config_boolean", &weechat_lua_api_config_boolean },
    { "config_boolean_default", &weechat_lua_api_config_boolean_default },
    { "config_integer", &weechat_lua_api_config_integer },
    { "config_integer_default", &weechat_lua_api_config_integer_default },
    { "config_string", &weechat_lua_api_config_string },
    { "config_string_default", &weechat_lua_api_config_string_default },
    { "config_color", &weechat_lua_api_config_color },
    { "config_color_default", &weechat_lua_api_config_color_default },
    { "config_write_option", &weechat_lua_api_config_write_option },
    { "config_write_line", &weechat_lua_api_config_write_line },
    { "config_write", &weechat_lua_api_config_write },
    { "config_read", &weechat_lua_api_config_read },
    { "config_reload", &weechat_lua_api_config_reload },
    { "config_option_free", &weechat_lua_api_config_option_free },
    { "config_section_free_options", &weechat_lua_api_config_section_free_options },
    { "config_section_free", &weechat_lua_api_config_section_free },
    { "config_free", &weechat_lua_api_config_free },
    { "config_get", &weechat_lua_api_config_get },
    { "config_get_plugin", &weechat_lua_api_config_get_plugin },
    { "config_is_set_plugin", &weechat_lua_api_config_is_set_plugin },
    { "config_set_plugin", &weechat_lua_api_config_set_plugin },
    { "config_unset_plugin", &weechat_lua_api_config_unset_plugin },
    { "prefix", &weechat_lua_api_prefix },
    { "color", &weechat_lua_api_color },
    { "print", &weechat_lua_api_print },
    { "print_date_tags", &weechat_lua_api_print_date_tags },
    { "print_y", &weechat_lua_api_print_y },
    { "log_print", &weechat_lua_api_log_print },
    { "hook_command", &weechat_lua_api_hook_command },
    { "hook_command_run", &weechat_lua_api_hook_command_run },
    { "hook_timer", &weechat_lua_api_hook_timer },
    { "hook_fd", &weechat_lua_api_hook_fd },
    { "hook_process", &weechat_lua_api_hook_process },
    { "hook_connect", &weechat_lua_api_hook_connect },
    { "hook_print", &weechat_lua_api_hook_print },
    { "hook_signal", &weechat_lua_api_hook_signal },
    { "hook_signal_send", &weechat_lua_api_hook_signal_send },
    { "hook_config", &weechat_lua_api_hook_config },
    { "hook_completion", &weechat_lua_api_hook_completion },
    { "hook_completion_list_add", &weechat_lua_api_hook_completion_list_add },
    { "hook_modifier", &weechat_lua_api_hook_modifier },
    { "hook_modifier_exec", &weechat_lua_api_hook_modifier_exec },
    { "hook_info", &weechat_lua_api_hook_info },
    { "hook_infolist", &weechat_lua_api_hook_infolist },
    { "unhook", &weechat_lua_api_unhook },
    { "unhook_all", &weechat_lua_api_unhook_all },
    { "buffer_new", &weechat_lua_api_buffer_new },
    { "buffer_search", &weechat_lua_api_buffer_search },
    { "buffer_search_main", &weechat_lua_api_buffer_search_main },
    { "current_buffer", &weechat_lua_api_current_buffer },
    { "buffer_clear", &weechat_lua_api_buffer_clear },
    { "buffer_close", &weechat_lua_api_buffer_close },
    { "buffer_merge", &weechat_lua_api_buffer_merge },
    { "buffer_unmerge", &weechat_lua_api_buffer_unmerge },
    { "buffer_get_integer", &weechat_lua_api_buffer_get_integer },
    { "buffer_get_string", &weechat_lua_api_buffer_get_string },
    { "buffer_get_pointer", &weechat_lua_api_buffer_get_pointer },
    { "buffer_set", &weechat_lua_api_buffer_set },
    { "current_window", &weechat_lua_api_current_window },
    { "window_get_integer", &weechat_lua_api_window_get_integer },
    { "window_get_string", &weechat_lua_api_window_get_string },
    { "window_get_pointer", &weechat_lua_api_window_get_pointer },
    { "window_set_title", &weechat_lua_api_window_set_title },
    { "nicklist_add_group", &weechat_lua_api_nicklist_add_group },
    { "nicklist_search_group", &weechat_lua_api_nicklist_search_group },
    { "nicklist_add_nick", &weechat_lua_api_nicklist_add_nick },
    { "nicklist_search_nick", &weechat_lua_api_nicklist_search_nick },
    { "nicklist_remove_group", &weechat_lua_api_nicklist_remove_group },
    { "nicklist_remove_nick", &weechat_lua_api_nicklist_remove_nick },
    { "nicklist_remove_all", &weechat_lua_api_nicklist_remove_all },
    { "bar_item_search", &weechat_lua_api_bar_item_search },
    { "bar_item_new", &weechat_lua_api_bar_item_new },
    { "bar_item_update", &weechat_lua_api_bar_item_update },
    { "bar_item_remove", &weechat_lua_api_bar_item_remove },
    { "bar_search", &weechat_lua_api_bar_search },
    { "bar_new", &weechat_lua_api_bar_new },
    { "bar_set", &weechat_lua_api_bar_set },
    { "bar_update", &weechat_lua_api_bar_update },
    { "bar_remove", &weechat_lua_api_bar_remove },
    { "command", &weechat_lua_api_command },
    { "info_get", &weechat_lua_api_info_get },
    { "infolist_new", &weechat_lua_api_infolist_new },
    { "infolist_new_var_integer", &weechat_lua_api_infolist_new_var_integer },
    { "infolist_new_var_string", &weechat_lua_api_infolist_new_var_string },
    { "infolist_new_var_pointer", &weechat_lua_api_infolist_new_var_pointer },
    { "infolist_new_var_time", &weechat_lua_api_infolist_new_var_time },
    { "infolist_get", &weechat_lua_api_infolist_get },
    { "infolist_next", &weechat_lua_api_infolist_next },
    { "infolist_prev", &weechat_lua_api_infolist_prev },
    { "infolist_fields", &weechat_lua_api_infolist_fields },
    { "infolist_integer", &weechat_lua_api_infolist_integer },
    { "infolist_string", &weechat_lua_api_infolist_string },
    { "infolist_pointer", &weechat_lua_api_infolist_pointer },
    { "infolist_time", &weechat_lua_api_infolist_time },
    { "infolist_free", &weechat_lua_api_infolist_free },
    { "upgrade_new", &weechat_lua_api_upgrade_new },
    { "upgrade_write_object", &weechat_lua_api_upgrade_write_object },
    { "upgrade_read", &weechat_lua_api_upgrade_read },
    { "upgrade_close", &weechat_lua_api_upgrade_close },
    
    /* define constants as function which returns values */
    
    { "WEECHAT_RC_OK", &weechat_lua_api_constant_weechat_rc_ok },
    { "WEECHAT_RC_OK_EAT", &weechat_lua_api_constant_weechat_rc_ok_eat },
    { "WEECHAT_RC_ERROR", &weechat_lua_api_constant_weechat_rc_error },
    
    { "WEECHAT_CONFIG_READ_OK", &weechat_lua_api_constant_weechat_config_read_ok },
    { "WEECHAT_CONFIG_READ_MEMORY_ERROR", &weechat_lua_api_constant_weechat_config_read_memory_error },
    { "WEECHAT_CONFIG_READ_FILE_NOT_FOUND", &weechat_lua_api_constant_weechat_config_read_file_not_found },
    { "WEECHAT_CONFIG_WRITE_OK", &weechat_lua_api_constant_weechat_config_write_ok },
    { "WEECHAT_CONFIG_WRITE_ERROR", &weechat_lua_api_constant_weechat_config_write_error },
    { "WEECHAT_CONFIG_WRITE_MEMORY_ERROR", &weechat_lua_api_constant_weechat_config_write_memory_error },
    { "WEECHAT_CONFIG_OPTION_SET_OK_CHANGED", &weechat_lua_api_constant_weechat_config_option_set_ok_changed },
    { "WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE", &weechat_lua_api_constant_weechat_config_option_set_ok_same_value },
    { "WEECHAT_CONFIG_OPTION_SET_ERROR", &weechat_lua_api_constant_weechat_config_option_set_error },
    { "WEECHAT_CONFIG_OPTION_SET_OPTION_NOT_FOUND", &weechat_lua_api_constant_weechat_config_option_set_option_not_found },
    { "WEECHAT_CONFIG_OPTION_UNSET_OK_NO_RESET", &weechat_lua_api_constant_weechat_config_option_unset_ok_no_reset },
    { "WEECHAT_CONFIG_OPTION_UNSET_OK_RESET", &weechat_lua_api_constant_weechat_config_option_unset_ok_reset },
    { "WEECHAT_CONFIG_OPTION_UNSET_OK_REMOVED", &weechat_lua_api_constant_weechat_config_option_unset_ok_removed },
    { "WEECHAT_CONFIG_OPTION_UNSET_ERROR", &weechat_lua_api_constant_weechat_config_option_unset_error },
    
    { "WEECHAT_LIST_POS_SORT", &weechat_lua_api_constant_weechat_list_pos_sort },
    { "WEECHAT_LIST_POS_BEGINNING", &weechat_lua_api_constant_weechat_list_pos_beginning },
    { "WEECHAT_LIST_POS_END", &weechat_lua_api_constant_weechat_list_pos_end },
    
    { "WEECHAT_HOTLIST_LOW", &weechat_lua_api_constant_weechat_hotlist_low },
    { "WEECHAT_HOTLIST_MESSAGE", &weechat_lua_api_constant_weechat_hotlist_message },
    { "WEECHAT_HOTLIST_PRIVATE", &weechat_lua_api_constant_weechat_hotlist_private },
    { "WEECHAT_HOTLIST_HIGHLIGHT", &weechat_lua_api_constant_weechat_hotlist_highlight },
    
    { "WEECHAT_HOOK_PROCESS_RUNNING", &weechat_lua_api_constant_weechat_hook_process_running },
    { "WEECHAT_HOOK_PROCESS_ERROR", &weechat_lua_api_constant_weechat_hook_process_error },
    
    { "WEECHAT_HOOK_CONNECT_OK", &weechat_lua_api_constant_weechat_hook_connect_ok },
    { "WEECHAT_HOOK_CONNECT_ADDRESS_NOT_FOUND", &weechat_lua_api_constant_weechat_hook_connect_address_not_found },
    { "WEECHAT_HOOK_CONNECT_IP_ADDRESS_NOT_FOUND", &weechat_lua_api_constant_weechat_hook_connect_ip_address_not_found },
    { "WEECHAT_HOOK_CONNECT_CONNECTION_REFUSED", &weechat_lua_api_constant_weechat_hook_connect_connection_refused },
    { "WEECHAT_HOOK_CONNECT_PROXY_ERROR", &weechat_lua_api_constant_weechat_hook_connect_proxy_error },
    { "WEECHAT_HOOK_CONNECT_LOCAL_HOSTNAME_ERROR", &weechat_lua_api_constant_weechat_hook_connect_local_hostname_error },
    { "WEECHAT_HOOK_CONNECT_GNUTLS_INIT_ERROR", &weechat_lua_api_constant_weechat_hook_connect_gnutls_init_error },
    { "WEECHAT_HOOK_CONNECT_GNUTLS_HANDSHAKE_ERROR", &weechat_lua_api_constant_weechat_hook_connect_gnutls_handshake_error },
    { "WEECHAT_HOOK_CONNECT_MEMORY_ERROR", &weechat_lua_api_constant_weechat_hook_connect_memory_error },
    
    { "WEECHAT_HOOK_SIGNAL_STRING", &weechat_lua_api_constant_weechat_hook_signal_string },
    { "WEECHAT_HOOK_SIGNAL_INT", &weechat_lua_api_constant_weechat_hook_signal_int },
    { "WEECHAT_HOOK_SIGNAL_POINTER", &weechat_lua_api_constant_weechat_hook_signal_pointer },
    
    { NULL, NULL }
};

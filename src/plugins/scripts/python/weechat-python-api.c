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

/* weechat-python-api.c: Python API functions */


#undef _

#include <Python.h>
#include <time.h>

#include "../../weechat-plugin.h"
#include "../script.h"
#include "../script-api.h"
#include "../script-callback.h"
#include "weechat-python.h"


#define PYTHON_RETURN_OK return Py_BuildValue ("i", 1);
#define PYTHON_RETURN_ERROR return Py_BuildValue ("i", 0);
#define PYTHON_RETURN_EMPTY \
    Py_INCREF(Py_None);     \
    return Py_None;
#define PYTHON_RETURN_STRING(__string)             \
    if (__string)                                  \
        return Py_BuildValue ("s", __string);      \
    return Py_BuildValue ("s", "")
#define PYTHON_RETURN_STRING_FREE(__string)        \
    if (__string)                                  \
    {                                              \
        object = Py_BuildValue ("s", __string);    \
        free (__string);                           \
        return object;                             \
    }                                              \
    return Py_BuildValue ("s", "")
#define PYTHON_RETURN_INT(__int)                \
    return Py_BuildValue("i", __int);


/*
 * weechat_python_api_register: startup function for all WeeChat Python scripts
 */

static PyObject *
weechat_python_api_register (PyObject *self, PyObject *args)
{
    char *name, *author, *version, *license, *shutdown_func, *description;
    char *charset;
    
    /* make C compiler happy */
    (void) self;
    
    python_current_script = NULL;
    
    name = NULL;
    author = NULL;
    version = NULL;
    license = NULL;
    description = NULL;
    shutdown_func = NULL;
    charset = NULL;
    
    if (!PyArg_ParseTuple (args, "sssssss", &name, &author, &version,
                           &license, &description, &shutdown_func, &charset))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(python_current_script_filename, "register");
        PYTHON_RETURN_ERROR;
    }
    
    if (script_search (weechat_python_plugin, python_scripts, name))
    {
        /* error: another scripts already exists with this name! */
        weechat_printf (NULL,
                        weechat_gettext ("%s%s: unable to register script "
                                         "\"%s\" (another script already "
                                         "exists with this name)"),
                        weechat_prefix ("error"), PYTHON_PLUGIN_NAME, name);
        PYTHON_RETURN_ERROR;
    }
    
    /* register script */
    python_current_script = script_add (weechat_python_plugin,
                                        &python_scripts, &last_python_script,
                                        (python_current_script_filename) ?
                                        python_current_script_filename : "",
                                        name, author, version, license,
                                        description, shutdown_func, charset);
    if (python_current_script)
    {
        if ((weechat_python_plugin->debug >= 1) || !python_quiet)
        {
            weechat_printf (NULL,
                            weechat_gettext ("%s: registered script \"%s\", "
                                             "version %s (%s)"),
                            PYTHON_PLUGIN_NAME, name, version, description);
        }
    }
    else
    {
        PYTHON_RETURN_ERROR;
    }
    
    PYTHON_RETURN_OK;
}

/*
 * weechat_python_api_plugin_get_name: get name of plugin (return "core" for
 *                                     WeeChat core)
 */

static PyObject *
weechat_python_api_plugin_get_name (PyObject *self, PyObject *args)
{
    char *plugin;
    const char *result;
    
    /* make C compiler happy */
    (void) self;
    
    if (!python_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PYTHON_CURRENT_SCRIPT_NAME, "plugin_get_name");
        PYTHON_RETURN_EMPTY;
    }
    
    plugin = NULL;
    
    if (!PyArg_ParseTuple (args, "s", &plugin))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PYTHON_CURRENT_SCRIPT_NAME, "plugin_get_name");
        PYTHON_RETURN_EMPTY;
    }
    
    result = weechat_plugin_get_name (script_str2ptr (plugin));
    
    PYTHON_RETURN_STRING(result);
}

/*
 * weechat_python_api_charset_set: set script charset
 */

static PyObject *
weechat_python_api_charset_set (PyObject *self, PyObject *args)
{
    char *charset;
    
    /* make C compiler happy */
    (void) self;
    
    if (!python_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PYTHON_CURRENT_SCRIPT_NAME, "charset_set");
        PYTHON_RETURN_ERROR;
    }
    
    charset = NULL;
    
    if (!PyArg_ParseTuple (args, "s", &charset))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PYTHON_CURRENT_SCRIPT_NAME, "charset_set");
        PYTHON_RETURN_ERROR;
    }
    
    script_api_charset_set (python_current_script,
                            charset);
    
    PYTHON_RETURN_OK;
}

/*
 * weechat_python_api_iconv_to_internal: convert string to internal WeeChat charset
 */

static PyObject *
weechat_python_api_iconv_to_internal (PyObject *self, PyObject *args)
{
    char *charset, *string, *result;
    PyObject *object;
    
    /* make C compiler happy */
    (void) self;
    
    if (!python_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PYTHON_CURRENT_SCRIPT_NAME, "iconv_to_internal");
        PYTHON_RETURN_EMPTY;
    }
    
    charset = NULL;
    string = NULL;
    
    if (!PyArg_ParseTuple (args, "ss", &charset, &string))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PYTHON_CURRENT_SCRIPT_NAME, "iconv_to_internal");
        PYTHON_RETURN_EMPTY;
    }
    
    result = weechat_iconv_to_internal (charset, string);
    
    PYTHON_RETURN_STRING_FREE(result);
}

/*
 * weechat_python_api_iconv_from_internal: convert string from WeeChat internal
 *                                         charset to another one
 */

static PyObject *
weechat_python_api_iconv_from_internal (PyObject *self, PyObject *args)
{
    char *charset, *string, *result;
    PyObject *object;
    
    /* make C compiler happy */
    (void) self;
    
    if (!python_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PYTHON_CURRENT_SCRIPT_NAME, "iconv_from_internal");
        PYTHON_RETURN_EMPTY;
    }
    
    charset = NULL;
    string = NULL;
    
    if (!PyArg_ParseTuple (args, "ss", &charset, &string))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PYTHON_CURRENT_SCRIPT_NAME, "iconv_from_internal");
        PYTHON_RETURN_EMPTY;
    }
    
    result = weechat_iconv_from_internal (charset, string);
    
    PYTHON_RETURN_STRING_FREE(result);
}

/*
 * weechat_python_api_gettext: get translated string
 */

static PyObject *
weechat_python_api_gettext (PyObject *self, PyObject *args)
{
    char *string;
    const char *result;
    
    /* make C compiler happy */
    (void) self;
    
    if (!python_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PYTHON_CURRENT_SCRIPT_NAME, "gettext");
        PYTHON_RETURN_EMPTY;
    }
    
    string = NULL;
    
    if (!PyArg_ParseTuple (args, "s", &string))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PYTHON_CURRENT_SCRIPT_NAME, "gettext");
        PYTHON_RETURN_EMPTY;
    }
    
    result = weechat_gettext (string);
    
    PYTHON_RETURN_STRING(result);
}

/*
 * weechat_python_api_ngettext: get translated string with plural form
 */

static PyObject *
weechat_python_api_ngettext (PyObject *self, PyObject *args)
{
    char *single, *plural;
    const char *result;
    int count;
    
    /* make C compiler happy */
    (void) self;
    
    if (!python_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PYTHON_CURRENT_SCRIPT_NAME, "ngettext");
        PYTHON_RETURN_EMPTY;
    }
    
    single = NULL;
    plural = NULL;
    count = 0;
    
    if (!PyArg_ParseTuple (args, "ssi", &single, &plural, &count))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PYTHON_CURRENT_SCRIPT_NAME, "ngettext");
        PYTHON_RETURN_EMPTY;
    }
    
    result = weechat_ngettext (single, plural, count);
    
    PYTHON_RETURN_STRING(result);
}

/*
 * weechat_python_api_string_remove_color: remove WeeChat color codes from string
 */

static PyObject *
weechat_python_api_string_remove_color (PyObject *self, PyObject *args)
{
    char *string, *replacement, *result;
    PyObject *object;
    
    /* make C compiler happy */
    (void) self;
    
    if (!python_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PYTHON_CURRENT_SCRIPT_NAME, "string_remove_color");
        PYTHON_RETURN_EMPTY;
    }
    
    string = NULL;
    replacement = NULL;
    
    if (!PyArg_ParseTuple (args, "ss", &string, &replacement))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PYTHON_CURRENT_SCRIPT_NAME, "string_remove_color");
        PYTHON_RETURN_EMPTY;
    }
    
    result = weechat_string_remove_color (string, replacement);
    
    PYTHON_RETURN_STRING_FREE(result);
}

/*
 * weechat_python_api_mkdir_home: create a directory in WeeChat home
 */

static PyObject *
weechat_python_api_mkdir_home (PyObject *self, PyObject *args)
{
    char *directory;
    int mode;
    
    /* make C compiler happy */
    (void) self;
    
    if (!python_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PYTHON_CURRENT_SCRIPT_NAME, "mkdir_home");
        PYTHON_RETURN_ERROR;
    }
    
    directory = NULL;
    mode = 0;
    
    if (!PyArg_ParseTuple (args, "si", &directory, &mode))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PYTHON_CURRENT_SCRIPT_NAME, "mkdir_home");
        PYTHON_RETURN_ERROR;
    }
    
    if (weechat_mkdir_home (directory, mode))
        PYTHON_RETURN_OK;

    PYTHON_RETURN_ERROR;
}

/*
 * weechat_python_api_mkdir: create a directory
 */

static PyObject *
weechat_python_api_mkdir (PyObject *self, PyObject *args)
{
    char *directory;
    int mode;
    
    /* make C compiler happy */
    (void) self;
    
    if (!python_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PYTHON_CURRENT_SCRIPT_NAME, "mkdir");
        PYTHON_RETURN_ERROR;
    }
    
    directory = NULL;
    mode = 0;
    
    if (!PyArg_ParseTuple (args, "si", &directory, &mode))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PYTHON_CURRENT_SCRIPT_NAME, "mkdir");
        PYTHON_RETURN_ERROR;
    }
    
    if (weechat_mkdir (directory, mode))
        PYTHON_RETURN_OK;
    
    PYTHON_RETURN_ERROR;
}

/*
 * weechat_python_api_mkdir_parents: create a directory and make parent
 *                                   directories as needed
 */

static PyObject *
weechat_python_api_mkdir_parents (PyObject *self, PyObject *args)
{
    char *directory;
    int mode;
    
    /* make C compiler happy */
    (void) self;
    
    if (!python_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PYTHON_CURRENT_SCRIPT_NAME, "mkdir_parents");
        PYTHON_RETURN_ERROR;
    }
    
    directory = NULL;
    mode = 0;
    
    if (!PyArg_ParseTuple (args, "si", &directory, &mode))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PYTHON_CURRENT_SCRIPT_NAME, "mkdir_parents");
        PYTHON_RETURN_ERROR;
    }
    
    if (weechat_mkdir_parents (directory, mode))
        PYTHON_RETURN_OK;
    
    PYTHON_RETURN_ERROR;
}

/*
 * weechat_python_api_list_new: create a new list
 */

static PyObject *
weechat_python_api_list_new (PyObject *self, PyObject *args)
{
    char *result;
    PyObject *object;
    
    /* make C compiler happy */
    (void) self;
    (void) args;
    
    if (!python_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PYTHON_CURRENT_SCRIPT_NAME, "list_new");
        PYTHON_RETURN_EMPTY;
    }
    
    result = script_ptr2str (weechat_list_new ());
    
    PYTHON_RETURN_STRING_FREE(result);
}

/*
 * weechat_python_api_list_add: add a string to list
 */

static PyObject *
weechat_python_api_list_add (PyObject *self, PyObject *args)
{
    char *weelist, *data, *where, *user_data, *result;
    PyObject *object;
    
    /* make C compiler happy */
    (void) self;
    
    if (!python_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PYTHON_CURRENT_SCRIPT_NAME, "list_add");
        PYTHON_RETURN_EMPTY;
    }
    
    weelist = NULL;
    data = NULL;
    where = NULL;
    user_data = NULL;
    
    if (!PyArg_ParseTuple (args, "ssss", &weelist, &data, &where, &user_data))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PYTHON_CURRENT_SCRIPT_NAME, "list_add");
        PYTHON_RETURN_EMPTY;
    }
    
    result = script_ptr2str (weechat_list_add (script_str2ptr (weelist),
                                               data,
                                               where,
                                               script_str2ptr (user_data)));
    
    PYTHON_RETURN_STRING_FREE(result);
}

/*
 * weechat_python_api_list_search: search a string in list
 */

static PyObject *
weechat_python_api_list_search (PyObject *self, PyObject *args)
{
    char *weelist, *data, *result;
    PyObject *object;
    
    /* make C compiler happy */
    (void) self;
    
    if (!python_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PYTHON_CURRENT_SCRIPT_NAME, "list_search");
        PYTHON_RETURN_EMPTY;
    }
    
    weelist = NULL;
    data = NULL;
    
    if (!PyArg_ParseTuple (args, "ss", &weelist, &data))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PYTHON_CURRENT_SCRIPT_NAME, "list_search");
        PYTHON_RETURN_EMPTY;
    }
    
    result = script_ptr2str (weechat_list_search (script_str2ptr (weelist),
                                                  data));
    
    PYTHON_RETURN_STRING_FREE(result);
}

/*
 * weechat_python_api_list_casesearch: search a string in list (ignore case)
 */

static PyObject *
weechat_python_api_list_casesearch (PyObject *self, PyObject *args)
{
    char *weelist, *data, *result;
    PyObject *object;
    
    /* make C compiler happy */
    (void) self;
    
    if (!python_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PYTHON_CURRENT_SCRIPT_NAME, "list_casesearch");
        PYTHON_RETURN_EMPTY;
    }
    
    weelist = NULL;
    data = NULL;
    
    if (!PyArg_ParseTuple (args, "ss", &weelist, &data))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PYTHON_CURRENT_SCRIPT_NAME, "list_casesearch");
        PYTHON_RETURN_EMPTY;
    }
    
    result = script_ptr2str (weechat_list_casesearch (script_str2ptr (weelist),
                                                      data));
    
    PYTHON_RETURN_STRING_FREE(result);
}

/*
 * weechat_python_api_list_get: get item by position
 */

static PyObject *
weechat_python_api_list_get (PyObject *self, PyObject *args)
{
    char *weelist, *result;
    int position;
    PyObject *object;
    
    /* make C compiler happy */
    (void) self;
    
    if (!python_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PYTHON_CURRENT_SCRIPT_NAME, "list_get");
        PYTHON_RETURN_EMPTY;
    }
    
    weelist = NULL;
    position = 0;
    
    if (!PyArg_ParseTuple (args, "si", &weelist, &position))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PYTHON_CURRENT_SCRIPT_NAME, "list_get");
        PYTHON_RETURN_EMPTY;
    }
    
    result = script_ptr2str (weechat_list_get (script_str2ptr (weelist),
                                               position));
    PYTHON_RETURN_STRING_FREE(result);
}

/*
 * weechat_python_api_list_set: set new value for item
 */

static PyObject *
weechat_python_api_list_set (PyObject *self, PyObject *args)
{
    char *item, *new_value;
    
    /* make C compiler happy */
    (void) self;
    
    if (!python_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PYTHON_CURRENT_SCRIPT_NAME, "list_set");
        PYTHON_RETURN_ERROR;
    }
    
    item = NULL;
    new_value = NULL;
    
    if (!PyArg_ParseTuple (args, "ss", &item, &new_value))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PYTHON_CURRENT_SCRIPT_NAME, "list_set");
        PYTHON_RETURN_ERROR;
    }
    
    weechat_list_set (script_str2ptr (item),
                      new_value);
    
    PYTHON_RETURN_OK;
}

/*
 * weechat_python_api_list_next: get next item
 */

static PyObject *
weechat_python_api_list_next (PyObject *self, PyObject *args)
{
    char *item, *result;
    PyObject *object;
    
    /* make C compiler happy */
    (void) self;
    
    if (!python_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PYTHON_CURRENT_SCRIPT_NAME, "list_next");
        PYTHON_RETURN_EMPTY;
    }
    
    item = NULL;
    
    if (!PyArg_ParseTuple (args, "s", &item))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PYTHON_CURRENT_SCRIPT_NAME, "list_next");
        PYTHON_RETURN_EMPTY;
    }
    
    result = script_ptr2str (weechat_list_next (script_str2ptr (item)));
    
    PYTHON_RETURN_STRING_FREE(result);
}

/*
 * weechat_python_api_list_prev: get previous item
 */

static PyObject *
weechat_python_api_list_prev (PyObject *self, PyObject *args)
{
    char *item, *result;
    PyObject *object;
    
    /* make C compiler happy */
    (void) self;
    
    if (!python_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PYTHON_CURRENT_SCRIPT_NAME, "list_prev");
        PYTHON_RETURN_EMPTY;
    }
    
    item = NULL;
    
    if (!PyArg_ParseTuple (args, "s", &item))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PYTHON_CURRENT_SCRIPT_NAME, "list_prev");
        PYTHON_RETURN_EMPTY;
    }
    
    result = script_ptr2str (weechat_list_prev (script_str2ptr (item)));
    
    PYTHON_RETURN_STRING_FREE(result);
}

/*
 * weechat_python_api_list_string: get string value of item
 */

static PyObject *
weechat_python_api_list_string (PyObject *self, PyObject *args)
{
    char *item;
    const char *result;
    
    /* make C compiler happy */
    (void) self;
    
    if (!python_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PYTHON_CURRENT_SCRIPT_NAME, "list_string");
        PYTHON_RETURN_EMPTY;
    }
    
    item = NULL;
    
    if (!PyArg_ParseTuple (args, "s", &item))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PYTHON_CURRENT_SCRIPT_NAME, "list_string");
        PYTHON_RETURN_EMPTY;
    }
    
    result = weechat_list_string (script_str2ptr (item));
    
    PYTHON_RETURN_STRING(result);
}

/*
 * weechat_python_api_list_size: get number of elements in list
 */

static PyObject *
weechat_python_api_list_size (PyObject *self, PyObject *args)
{
    char *weelist;
    int size;
    
    /* make C compiler happy */
    (void) self;
    
    if (!python_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PYTHON_CURRENT_SCRIPT_NAME, "list_size");
        PYTHON_RETURN_INT(0);
    }
    
    weelist = NULL;
    
    if (!PyArg_ParseTuple (args, "s", &weelist))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PYTHON_CURRENT_SCRIPT_NAME, "list_size");
        PYTHON_RETURN_INT(0);
    }
    
    size = weechat_list_size (script_str2ptr (weelist));
    
    PYTHON_RETURN_INT(size);
}

/*
 * weechat_python_api_list_remove: remove item from list
 */

static PyObject *
weechat_python_api_list_remove (PyObject *self, PyObject *args)
{
    char *weelist, *item;
    
    /* make C compiler happy */
    (void) self;
    
    if (!python_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PYTHON_CURRENT_SCRIPT_NAME, "list_remove");
        PYTHON_RETURN_ERROR;
    }
    
    weelist = NULL;
    item = NULL;
    
    if (!PyArg_ParseTuple (args, "ss", &weelist, &item))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PYTHON_CURRENT_SCRIPT_NAME, "list_remove");
        PYTHON_RETURN_ERROR;
    }
    
    weechat_list_remove (script_str2ptr (weelist),
                         script_str2ptr (item));
    
    PYTHON_RETURN_OK;
}

/*
 * weechat_python_api_list_remove_all: remove all items from list
 */

static PyObject *
weechat_python_api_list_remove_all (PyObject *self, PyObject *args)
{
    char *weelist;
    
    /* make C compiler happy */
    (void) self;
    
    if (!python_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PYTHON_CURRENT_SCRIPT_NAME, "list_remove_all");
        PYTHON_RETURN_ERROR;
    }
    
    weelist = NULL;
    
    if (!PyArg_ParseTuple (args, "s", &weelist))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PYTHON_CURRENT_SCRIPT_NAME, "list_remove_all");
        PYTHON_RETURN_ERROR;
    }
    
    weechat_list_remove_all (script_str2ptr (weelist));
    
    PYTHON_RETURN_OK;
}

/*
 * weechat_python_api_list_free: free list
 */

static PyObject *
weechat_python_api_list_free (PyObject *self, PyObject *args)
{
    char *weelist;
    
    /* make C compiler happy */
    (void) self;
    
    if (!python_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PYTHON_CURRENT_SCRIPT_NAME, "list_free");
        PYTHON_RETURN_ERROR;
    }
    
    weelist = NULL;
    
    if (!PyArg_ParseTuple (args, "s", &weelist))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PYTHON_CURRENT_SCRIPT_NAME, "list_free");
        PYTHON_RETURN_ERROR;
    }
    
    weechat_list_free (script_str2ptr (weelist));
    
    PYTHON_RETURN_OK;
}

/*
 * weechat_python_api_config_reload_cb: callback for config reload
 */

int
weechat_python_api_config_reload_cb (void *data,
                                     struct t_config_file *config_file)
{
    struct t_script_callback *script_callback;
    char *python_argv[3], empty_arg[1] = { '\0' };
    int *rc, ret;
    
    script_callback = (struct t_script_callback *)data;
    
    if (script_callback && script_callback->function && script_callback->function[0])
    {
        python_argv[0] = (script_callback->data) ? script_callback->data : empty_arg;
        python_argv[1] = script_ptr2str (config_file);
        python_argv[2] = NULL;
        
        rc = (int *) weechat_python_exec (script_callback->script,
                                          WEECHAT_SCRIPT_EXEC_INT,
                                          script_callback->function,
                                          python_argv);
        
        if (!rc)
            ret = WEECHAT_CONFIG_READ_FILE_NOT_FOUND;
        else
        {
            ret = *rc;
            free (rc);
        }
        if (python_argv[1])
            free (python_argv[1]);
        
        return ret;
    }
    
    return WEECHAT_CONFIG_READ_FILE_NOT_FOUND;
}

/*
 * weechat_python_api_config_new: create a new configuration file
 */

static PyObject *
weechat_python_api_config_new (PyObject *self, PyObject *args)
{
    char *name, *function, *data, *result;
    PyObject *object;
    
    /* make C compiler happy */
    (void) self;
    
    if (!python_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PYTHON_CURRENT_SCRIPT_NAME, "config_new");
        PYTHON_RETURN_EMPTY;
    }
    
    name = NULL;
    function = NULL;
    data = NULL;
    
    if (!PyArg_ParseTuple (args, "sss", &name, &function, &data))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PYTHON_CURRENT_SCRIPT_NAME, "config_new");
        PYTHON_RETURN_EMPTY;
    }
    
    result = script_ptr2str (script_api_config_new (weechat_python_plugin,
                                                    python_current_script,
                                                    name,
                                                    &weechat_python_api_config_reload_cb,
                                                    function,
                                                    data));
    
    PYTHON_RETURN_STRING_FREE(result);
}

/*
 * weechat_python_api_config_read_cb: callback for reading option in section
 */

int
weechat_python_api_config_read_cb (void *data,
                                   struct t_config_file *config_file,
                                   struct t_config_section *section,
                                   const char *option_name, const char *value)
{
    struct t_script_callback *script_callback;
    char *python_argv[6], empty_arg[1] = { '\0' };
    int *rc, ret;
    
    script_callback = (struct t_script_callback *)data;
    
    if (script_callback && script_callback->function && script_callback->function[0])
    {
        python_argv[0] = (script_callback->data) ? script_callback->data : empty_arg;
        python_argv[1] = script_ptr2str (config_file);
        python_argv[2] = script_ptr2str (section);
        python_argv[3] = (option_name) ? (char *)option_name : empty_arg;
        python_argv[4] = (value) ? (char *)value : empty_arg;
        python_argv[5] = NULL;
        
        rc = (int *) weechat_python_exec (script_callback->script,
                                          WEECHAT_SCRIPT_EXEC_INT,
                                          script_callback->function,
                                          python_argv);
        
        if (!rc)
            ret = WEECHAT_CONFIG_OPTION_SET_ERROR;
        else
        {
            ret = *rc;
            free (rc);
        }
        if (python_argv[1])
            free (python_argv[1]);
        if (python_argv[2])
            free (python_argv[2]);
        
        return ret;
    }
    
    return WEECHAT_CONFIG_OPTION_SET_ERROR;
}

/*
 * weechat_python_api_config_section_write_cb: callback for writing section
 */

void
weechat_python_api_config_section_write_cb (void *data,
                                            struct t_config_file *config_file,
                                            const char *section_name)
{
    struct t_script_callback *script_callback;
    char *python_argv[4], empty_arg[1] = { '\0' };
    int *rc;
    
    script_callback = (struct t_script_callback *)data;

    if (script_callback && script_callback->function && script_callback->function[0])
    {
        python_argv[0] = (script_callback->data) ? script_callback->data : empty_arg;
        python_argv[1] = script_ptr2str (config_file);
        python_argv[2] = (section_name) ? (char *)section_name : empty_arg;
        python_argv[3] = NULL;
        
        rc = (int *) weechat_python_exec (script_callback->script,
                                          WEECHAT_SCRIPT_EXEC_INT,
                                          script_callback->function,
                                          python_argv);
        
        if (rc)
            free (rc);
        if (python_argv[1])
            free (python_argv[1]);
    }
}

/*
 * weechat_python_api_config_section_write_default_cb: callback for writing
 *                                                     default values for section
 */

void
weechat_python_api_config_section_write_default_cb (void *data,
                                                    struct t_config_file *config_file,
                                                    const char *section_name)
{
    struct t_script_callback *script_callback;
    char *python_argv[4], empty_arg[1] = { '\0' };
    int *rc;
    
    script_callback = (struct t_script_callback *)data;

    if (script_callback && script_callback->function && script_callback->function[0])
    {
        python_argv[0] = (script_callback->data) ? script_callback->data : empty_arg;
        python_argv[1] = script_ptr2str (config_file);
        python_argv[2] = (section_name) ? (char *)section_name : empty_arg;
        python_argv[3] = NULL;
        
        rc = (int *) weechat_python_exec (script_callback->script,
                                          WEECHAT_SCRIPT_EXEC_INT,
                                          script_callback->function,
                                          python_argv);
        
        if (rc)
            free (rc);
        if (python_argv[1])
            free (python_argv[1]);
    }
}

/*
 * weechat_python_api_config_section_create_option_cb: callback to create an option
 */

int
weechat_python_api_config_section_create_option_cb (void *data,
                                                    struct t_config_file *config_file,
                                                    struct t_config_section *section,
                                                    const char *option_name,
                                                    const char *value)
{
    struct t_script_callback *script_callback;
    char *python_argv[6], empty_arg[1] = { '\0' };
    int *rc, ret;
    
    script_callback = (struct t_script_callback *)data;
    
    if (script_callback && script_callback->function && script_callback->function[0])
    {
        python_argv[0] = (script_callback->data) ? script_callback->data : empty_arg;
        python_argv[1] = script_ptr2str (config_file);
        python_argv[2] = script_ptr2str (section);
        python_argv[3] = (option_name) ? (char *)option_name : empty_arg;
        python_argv[4] = (value) ? (char *)value : empty_arg;
        python_argv[5] = NULL;

        rc = (int *) weechat_python_exec (script_callback->script,
                                          WEECHAT_SCRIPT_EXEC_INT,
                                          script_callback->function,
                                          python_argv);
        
        if (!rc)
            ret = WEECHAT_CONFIG_OPTION_SET_ERROR;
        else
        {
            ret = *rc;
            free (rc);
        }
        if (python_argv[1])
            free (python_argv[1]);
        if (python_argv[2])
            free (python_argv[2]);
        
        return ret;
    }
    
    return WEECHAT_CONFIG_OPTION_SET_ERROR;
}

/*
 * weechat_python_api_config_section_delete_option_cb: callback to delete an option
 */

int
weechat_python_api_config_section_delete_option_cb (void *data,
                                                    struct t_config_file *config_file,
                                                    struct t_config_section *section,
                                                    struct t_config_option *option)
{
    struct t_script_callback *script_callback;
    char *python_argv[5], empty_arg[1] = { '\0' };
    int *rc, ret;
    
    script_callback = (struct t_script_callback *)data;
    
    if (script_callback && script_callback->function && script_callback->function[0])
    {
        python_argv[0] = (script_callback->data) ? script_callback->data : empty_arg;
        python_argv[1] = script_ptr2str (config_file);
        python_argv[2] = script_ptr2str (section);
        python_argv[3] = script_ptr2str (option);
        python_argv[4] = NULL;

        rc = (int *) weechat_python_exec (script_callback->script,
                                          WEECHAT_SCRIPT_EXEC_INT,
                                          script_callback->function,
                                          python_argv);
        
        if (!rc)
            ret = WEECHAT_CONFIG_OPTION_UNSET_ERROR;
        else
        {
            ret = *rc;
            free (rc);
        }
        if (python_argv[1])
            free (python_argv[1]);
        if (python_argv[2])
            free (python_argv[2]);
        if (python_argv[3])
            free (python_argv[3]);
        
        return ret;
    }
    
    return WEECHAT_CONFIG_OPTION_UNSET_ERROR;
}

/*
 * weechat_python_api_config_new_section: create a new section in configuration file
 */

static PyObject *
weechat_python_api_config_new_section (PyObject *self, PyObject *args)
{
    char *config_file, *name, *function_read, *data_read, *function_write;
    char *data_write, *function_write_default, *data_write_default;
    char *function_create_option, *data_create_option, *function_delete_option;
    char *data_delete_option, *result;
    int user_can_add_options, user_can_delete_options;
    PyObject *object;
    
    /* make C compiler happy */
    (void) self;
    
    if (!python_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PYTHON_CURRENT_SCRIPT_NAME, "config_new_section");
        PYTHON_RETURN_EMPTY;
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
    
    if (!PyArg_ParseTuple (args, "ssiissssssssss", &config_file, &name,
                           &user_can_add_options, &user_can_delete_options,
                           &function_read, &data_read, &function_write,
                           &data_write, &function_write_default,
                           &data_write_default, &function_create_option,
                           &data_create_option, &function_delete_option,
                           &data_delete_option))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PYTHON_CURRENT_SCRIPT_NAME, "config_new_section");
        PYTHON_RETURN_EMPTY;
    }
    
    result = script_ptr2str (script_api_config_new_section (weechat_python_plugin,
                                                            python_current_script,
                                                            script_str2ptr (config_file),
                                                            name,
                                                            user_can_add_options,
                                                            user_can_delete_options,
                                                            &weechat_python_api_config_read_cb,
                                                            function_read,
                                                            data_read,
                                                            &weechat_python_api_config_section_write_cb,
                                                            function_write,
                                                            data_write,
                                                            &weechat_python_api_config_section_write_default_cb,
                                                            function_write_default,
                                                            data_write_default,
                                                            &weechat_python_api_config_section_create_option_cb,
                                                            function_create_option,
                                                            data_create_option,
                                                            &weechat_python_api_config_section_delete_option_cb,
                                                            function_delete_option,
                                                            data_delete_option));
    
    PYTHON_RETURN_STRING_FREE(result);
}

/*
 * weechat_python_api_config_search_section: search section in configuration file
 */

static PyObject *
weechat_python_api_config_search_section (PyObject *self, PyObject *args)
{
    char *config_file, *section_name, *result;
    PyObject *object;
    
    /* make C compiler happy */
    (void) self;
    
    if (!python_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PYTHON_CURRENT_SCRIPT_NAME, "config_search_section");
        PYTHON_RETURN_EMPTY;
    }
    
    config_file = NULL;
    section_name = NULL;
    
    if (!PyArg_ParseTuple (args, "ss", &config_file, &section_name))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PYTHON_CURRENT_SCRIPT_NAME, "config_search_section");
        PYTHON_RETURN_EMPTY;
    }
    
    result = script_ptr2str (weechat_config_search_section (script_str2ptr (config_file),
                                                            section_name));
    
    PYTHON_RETURN_STRING_FREE(result);
}

/*
 * weechat_python_api_config_option_check_cb: callback for checking new value
 *                                            for option
 */

int
weechat_python_api_config_option_check_value_cb (void *data,
                                                 struct t_config_option *option,
                                                 const char *value)
{
    struct t_script_callback *script_callback;
    char *python_argv[4], empty_arg[1] = { '\0' };
    int *rc, ret;
    
    script_callback = (struct t_script_callback *)data;
    
    if (script_callback && script_callback->function && script_callback->function[0])
    {
        python_argv[0] = (script_callback->data) ? script_callback->data : empty_arg;
        python_argv[1] = script_ptr2str (option);
        python_argv[2] = (value) ? (char *)value : empty_arg;
        python_argv[3] = NULL;
        
        rc = (int *) weechat_python_exec (script_callback->script,
                                          WEECHAT_SCRIPT_EXEC_INT,
                                          script_callback->function,
                                          python_argv);
        
        if (!rc)
            ret = 0;
        else
        {
            ret = *rc;
            free (rc);
        }
        if (python_argv[1])
            free (python_argv[1]);
        
        return ret;
    }
    
    return 0;
}

/*
 * weechat_python_api_config_option_change_cb: callback for option changed
 */

void
weechat_python_api_config_option_change_cb (void *data,
                                            struct t_config_option *option)
{
    struct t_script_callback *script_callback;
    char *python_argv[3], empty_arg[1] = { '\0' };
    int *rc;
    
    script_callback = (struct t_script_callback *)data;

    if (script_callback && script_callback->function && script_callback->function[0])
    {
        python_argv[0] = (script_callback->data) ? script_callback->data : empty_arg;
        python_argv[1] = script_ptr2str (option);
        python_argv[2] = NULL;
        
        rc = (int *) weechat_python_exec (script_callback->script,
                                          WEECHAT_SCRIPT_EXEC_INT,
                                          script_callback->function,
                                          python_argv);
        
        if (python_argv[1])
            free (python_argv[1]);
        
        if (rc)
            free (rc);
    }
}

/*
 * weechat_python_api_config_option_delete_cb: callback when option is deleted
 */

void
weechat_python_api_config_option_delete_cb (void *data,
                                            struct t_config_option *option)
{
    struct t_script_callback *script_callback;
    char *python_argv[3], empty_arg[1] = { '\0' };
    int *rc;
    
    script_callback = (struct t_script_callback *)data;

    if (script_callback && script_callback->function && script_callback->function[0])
    {
        python_argv[0] = (script_callback->data) ? script_callback->data : empty_arg;
        python_argv[1] = script_ptr2str (option);
        python_argv[2] = NULL;
        
        rc = (int *) weechat_python_exec (script_callback->script,
                                          WEECHAT_SCRIPT_EXEC_INT,
                                          script_callback->function,
                                          python_argv);
        
        if (python_argv[1])
            free (python_argv[1]);
        
        if (rc)
            free (rc);
    }
}

/*
 * weechat_python_api_config_new_option: create a new option in section
 */

static PyObject *
weechat_python_api_config_new_option (PyObject *self, PyObject *args)
{
    char *config_file, *section, *name, *type, *description, *string_values;
    char *default_value, *value, *result;
    char *function_check_value, *data_check_value, *function_change;
    char *data_change, *function_delete, *data_delete;
    int min, max, null_value_allowed;
    PyObject *object;
    
    /* make C compiler happy */
    (void) self;
    
    if (!python_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PYTHON_CURRENT_SCRIPT_NAME, "config_new_option");
        PYTHON_RETURN_EMPTY;
    }
    
    config_file = NULL;
    section = NULL;
    name = NULL;
    type = NULL;
    description = NULL;
    string_values = NULL;
    default_value = NULL;
    value = NULL;
    function_check_value = NULL;
    data_check_value = NULL;
    function_change = NULL;
    data_change = NULL;
    function_delete = NULL;
    data_delete = NULL;
    
    if (!PyArg_ParseTuple (args, "ssssssiississssss", &config_file, &section, &name,
                           &type, &description, &string_values, &min, &max,
                           &default_value, &value, &null_value_allowed,
                           &function_check_value, &data_check_value,
                           &function_change, &data_change, &function_delete,
                           &data_delete))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PYTHON_CURRENT_SCRIPT_NAME, "config_new_option");
        PYTHON_RETURN_EMPTY;
    }
    
    result = script_ptr2str (script_api_config_new_option (weechat_python_plugin,
                                                           python_current_script,
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
                                                           &weechat_python_api_config_option_check_value_cb,
                                                           function_check_value,
                                                           data_check_value,
                                                           &weechat_python_api_config_option_change_cb,
                                                           function_change,
                                                           data_change,
                                                           &weechat_python_api_config_option_delete_cb,
                                                           function_delete,
                                                           data_delete));
    
    PYTHON_RETURN_STRING_FREE(result);
}

/*
 * weechat_python_api_config_search_option: search option in configuration file or section
 */

static PyObject *
weechat_python_api_config_search_option (PyObject *self, PyObject *args)
{
    char *config_file, *section, *option_name, *result;
    PyObject *object;
    
    /* make C compiler happy */
    (void) self;
    
    if (!python_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PYTHON_CURRENT_SCRIPT_NAME, "config_search_option");
        PYTHON_RETURN_EMPTY;
    }
    
    config_file = NULL;
    section = NULL;
    option_name = NULL;
    
    if (!PyArg_ParseTuple (args, "sss", &config_file, &section, &option_name))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PYTHON_CURRENT_SCRIPT_NAME, "config_search_option");
        PYTHON_RETURN_EMPTY;
    }
    
    result = script_ptr2str (weechat_config_search_option (script_str2ptr (config_file),
                                                           script_str2ptr (section),
                                                           option_name));
    
    PYTHON_RETURN_STRING_FREE(result);
}

/*
 * weechat_python_api_config_string_to_boolean: return boolean value of a string
 */

static PyObject *
weechat_python_api_config_string_to_boolean (PyObject *self, PyObject *args)
{
    char *text;
    int value;
    
    /* make C compiler happy */
    (void) self;
    
    if (!python_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PYTHON_CURRENT_SCRIPT_NAME, "config_string_to_boolean");
        PYTHON_RETURN_INT(0);
    }
    
    text = NULL;
    
    if (!PyArg_ParseTuple (args, "s", &text))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PYTHON_CURRENT_SCRIPT_NAME, "config_string_to_boolean");
        PYTHON_RETURN_INT(0);
    }
    
    value = weechat_config_string_to_boolean (text);
    
    PYTHON_RETURN_INT(value);
}

/*
 * weechat_python_api_config_option_reset: reset an option with default value
 */

static PyObject *
weechat_python_api_config_option_reset (PyObject *self, PyObject *args)
{
    char *option;
    int run_callback, rc;
    
    /* make C compiler happy */
    (void) self;
    
    if (!python_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PYTHON_CURRENT_SCRIPT_NAME, "config_option_reset");
        PYTHON_RETURN_INT(0);
    }
    
    option = NULL;
    run_callback = 0;
    
    if (!PyArg_ParseTuple (args, "si", &option, &run_callback))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PYTHON_CURRENT_SCRIPT_NAME, "config_option_reset");
        PYTHON_RETURN_INT(0);
    }
    
    rc = weechat_config_option_reset (script_str2ptr (option),
                                      run_callback);
    
    PYTHON_RETURN_INT(rc);
}

/*
 * weechat_python_api_config_option_set: set new value for option
 */

static PyObject *
weechat_python_api_config_option_set (PyObject *self, PyObject *args)
{
    char *option, *new_value;
    int run_callback, rc;
    
    /* make C compiler happy */
    (void) self;
    
    if (!python_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PYTHON_CURRENT_SCRIPT_NAME, "config_option_set");
        PYTHON_RETURN_INT(WEECHAT_CONFIG_OPTION_SET_ERROR);
    }
    
    option = NULL;
    new_value = NULL;
    run_callback = 0;
    
    if (!PyArg_ParseTuple (args, "ssi", &option, &new_value, &run_callback))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PYTHON_CURRENT_SCRIPT_NAME, "config_option_set");
        PYTHON_RETURN_INT(WEECHAT_CONFIG_OPTION_SET_ERROR);
    }
    
    rc = weechat_config_option_set (script_str2ptr (option),
                                    new_value,
                                    run_callback);
    
    PYTHON_RETURN_INT(rc);
}

/*
 * weechat_python_api_config_option_set_null: set null (undefined) value for
 *                                            option
 */

static PyObject *
weechat_python_api_config_option_set_null (PyObject *self, PyObject *args)
{
    char *option;
    int run_callback, rc;
    
    /* make C compiler happy */
    (void) self;
    
    if (!python_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PYTHON_CURRENT_SCRIPT_NAME, "config_option_set_null");
        PYTHON_RETURN_INT(WEECHAT_CONFIG_OPTION_SET_ERROR);
    }
    
    option = NULL;
    run_callback = 0;
    
    if (!PyArg_ParseTuple (args, "si", &option, &run_callback))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PYTHON_CURRENT_SCRIPT_NAME, "config_option_set_null");
        PYTHON_RETURN_INT(WEECHAT_CONFIG_OPTION_SET_ERROR);
    }
    
    rc = weechat_config_option_set_null (script_str2ptr (option),
                                         run_callback);
    
    PYTHON_RETURN_INT(rc);
}

/*
 * weechat_python_api_config_option_unset: unset an option
 */

static PyObject *
weechat_python_api_config_option_unset (PyObject *self, PyObject *args)
{
    char *option;
    int rc;
    
    /* make C compiler happy */
    (void) self;
    
    if (!python_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PYTHON_CURRENT_SCRIPT_NAME, "config_option_unset");
        PYTHON_RETURN_INT(WEECHAT_CONFIG_OPTION_UNSET_ERROR);
    }
    
    option = NULL;
    
    if (!PyArg_ParseTuple (args, "s", &option))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PYTHON_CURRENT_SCRIPT_NAME, "config_option_unset");
        PYTHON_RETURN_INT(WEECHAT_CONFIG_OPTION_UNSET_ERROR);
    }
    
    rc = weechat_config_option_unset (script_str2ptr (option));
    
    PYTHON_RETURN_INT(rc);
}

/*
 * weechat_python_api_config_option_rename: rename an option
 */

static PyObject *
weechat_python_api_config_option_rename (PyObject *self, PyObject *args)
{
    char *option, *new_name;
    
    /* make C compiler happy */
    (void) self;
    
    if (!python_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PYTHON_CURRENT_SCRIPT_NAME, "config_option_rename");
        PYTHON_RETURN_ERROR;
    }
    
    option = NULL;
    new_name = NULL;
    
    if (!PyArg_ParseTuple (args, "ss", &option, &new_name))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PYTHON_CURRENT_SCRIPT_NAME, "config_option_rename");
        PYTHON_RETURN_ERROR;
    }
    
    weechat_config_option_rename (script_str2ptr (option),
                                  new_name);
    
    PYTHON_RETURN_OK;
}

/*
 * weechat_python_api_config_option_is_null: return 1 if value of option is null
 */

static PyObject *
weechat_python_api_config_option_is_null (PyObject *self, PyObject *args)
{
    char *option;
    int value;
    
    /* make C compiler happy */
    (void) self;
    
    if (!python_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PYTHON_CURRENT_SCRIPT_NAME, "config_option_is_null");
        PYTHON_RETURN_INT(1);
    }
    
    option = NULL;
    
    if (!PyArg_ParseTuple (args, "s", &option))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PYTHON_CURRENT_SCRIPT_NAME, "config_option_is_null");
        PYTHON_RETURN_INT(1);
    }
    
    value = weechat_config_option_is_null (script_str2ptr (option));
    
    PYTHON_RETURN_INT(value);
}

/*
 * weechat_python_api_config_option_default_is_null: return 1 if default value
 *                                                   of option is null
 */

static PyObject *
weechat_python_api_config_option_default_is_null (PyObject *self, PyObject *args)
{
    char *option;
    int value;
    
    /* make C compiler happy */
    (void) self;
    
    if (!python_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PYTHON_CURRENT_SCRIPT_NAME, "config_option_default_is_null");
        PYTHON_RETURN_INT(1);
    }
    
    option = NULL;
    
    if (!PyArg_ParseTuple (args, "s", &option))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PYTHON_CURRENT_SCRIPT_NAME, "config_option_default_is_null");
        PYTHON_RETURN_INT(1);
    }
    
    value = weechat_config_option_default_is_null (script_str2ptr (option));
    
    PYTHON_RETURN_INT(value);
}

/*
 * weechat_python_api_config_boolean: return boolean value of option
 */

static PyObject *
weechat_python_api_config_boolean (PyObject *self, PyObject *args)
{
    char *option;
    int value;
    
    /* make C compiler happy */
    (void) self;
    
    if (!python_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PYTHON_CURRENT_SCRIPT_NAME, "config_boolean");
        PYTHON_RETURN_INT(0);
    }
    
    option = NULL;
    
    if (!PyArg_ParseTuple (args, "s", &option))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PYTHON_CURRENT_SCRIPT_NAME, "config_boolean");
        PYTHON_RETURN_INT(0);
    }
    
    value = weechat_config_boolean (script_str2ptr (option));
    
    PYTHON_RETURN_INT(value);
}

/*
 * weechat_python_api_config_boolean_default: return default boolean value of option
 */

static PyObject *
weechat_python_api_config_boolean_default (PyObject *self, PyObject *args)
{
    char *option;
    int value;
    
    /* make C compiler happy */
    (void) self;
    
    if (!python_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PYTHON_CURRENT_SCRIPT_NAME, "config_boolean_default");
        PYTHON_RETURN_INT(0);
    }
    
    option = NULL;
    
    if (!PyArg_ParseTuple (args, "s", &option))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PYTHON_CURRENT_SCRIPT_NAME, "config_boolean_default");
        PYTHON_RETURN_INT(0);
    }
    
    value = weechat_config_boolean_default (script_str2ptr (option));
    
    PYTHON_RETURN_INT(value);
}

/*
 * weechat_python_api_config_integer: return integer value of option
 */

static PyObject *
weechat_python_api_config_integer (PyObject *self, PyObject *args)
{
    char *option;
    int value;
    
    /* make C compiler happy */
    (void) self;
    
    if (!python_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PYTHON_CURRENT_SCRIPT_NAME, "config_integer");
        PYTHON_RETURN_INT(0);
    }
    
    option = NULL;
    
    if (!PyArg_ParseTuple (args, "s", &option))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PYTHON_CURRENT_SCRIPT_NAME, "config_integer");
        PYTHON_RETURN_INT(0);
    }
    
    value = weechat_config_integer (script_str2ptr (option));
    
    PYTHON_RETURN_INT(value);
}

/*
 * weechat_python_api_config_integer_default: return default integer value of option
 */

static PyObject *
weechat_python_api_config_integer_default (PyObject *self, PyObject *args)
{
    char *option;
    int value;
    
    /* make C compiler happy */
    (void) self;
    
    if (!python_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PYTHON_CURRENT_SCRIPT_NAME, "config_integer_default");
        PYTHON_RETURN_INT(0);
    }
    
    option = NULL;
    
    if (!PyArg_ParseTuple (args, "s", &option))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PYTHON_CURRENT_SCRIPT_NAME, "config_integer_default");
        PYTHON_RETURN_INT(0);
    }
    
    value = weechat_config_integer_default (script_str2ptr (option));
    
    PYTHON_RETURN_INT(value);
}

/*
 * weechat_python_api_config_string: return string value of option
 */

static PyObject *
weechat_python_api_config_string (PyObject *self, PyObject *args)
{
    char *option;
    const char *result;
    
    /* make C compiler happy */
    (void) self;
    
    if (!python_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PYTHON_CURRENT_SCRIPT_NAME, "config_string");
        PYTHON_RETURN_EMPTY;
    }
    
    option = NULL;
    
    if (!PyArg_ParseTuple (args, "s", &option))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PYTHON_CURRENT_SCRIPT_NAME, "config_string");
        PYTHON_RETURN_EMPTY;
    }
    
    result = weechat_config_string (script_str2ptr (option));
    
    PYTHON_RETURN_STRING(result);
}

/*
 * weechat_python_api_config_string_default: return default string value of option
 */

static PyObject *
weechat_python_api_config_string_default (PyObject *self, PyObject *args)
{
    char *option;
    const char *result;
    
    /* make C compiler happy */
    (void) self;
    
    if (!python_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PYTHON_CURRENT_SCRIPT_NAME, "config_string_default");
        PYTHON_RETURN_EMPTY;
    }
    
    option = NULL;
    
    if (!PyArg_ParseTuple (args, "s", &option))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PYTHON_CURRENT_SCRIPT_NAME, "config_string_default");
        PYTHON_RETURN_EMPTY;
    }
    
    result = weechat_config_string_default (script_str2ptr (option));
    
    PYTHON_RETURN_STRING(result);
}

/*
 * weechat_python_api_config_color: return color value of option
 */

static PyObject *
weechat_python_api_config_color (PyObject *self, PyObject *args)
{
    char *option;
    const char *result;
    
    /* make C compiler happy */
    (void) self;
    
    if (!python_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PYTHON_CURRENT_SCRIPT_NAME, "config_color");
        PYTHON_RETURN_INT(0);
    }
    
    option = NULL;
    
    if (!PyArg_ParseTuple (args, "s", &option))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PYTHON_CURRENT_SCRIPT_NAME, "config_color");
        PYTHON_RETURN_INT(0);
    }
    
    result = weechat_config_color (script_str2ptr (option));
    
    PYTHON_RETURN_STRING(result);
}

/*
 * weechat_python_api_config_color_default: return default color value of option
 */

static PyObject *
weechat_python_api_config_color_default (PyObject *self, PyObject *args)
{
    char *option;
    const char *result;
    
    /* make C compiler happy */
    (void) self;
    
    if (!python_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PYTHON_CURRENT_SCRIPT_NAME, "config_color_default");
        PYTHON_RETURN_INT(0);
    }
    
    option = NULL;
    
    if (!PyArg_ParseTuple (args, "s", &option))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PYTHON_CURRENT_SCRIPT_NAME, "config_color_default");
        PYTHON_RETURN_INT(0);
    }
    
    result = weechat_config_color_default (script_str2ptr (option));
    
    PYTHON_RETURN_STRING(result);
}

/*
 * weechat_python_api_config_write_option: write an option in configuration file
 */

static PyObject *
weechat_python_api_config_write_option (PyObject *self, PyObject *args)
{
    char *config_file, *option;
    
    /* make C compiler happy */
    (void) self;
    
    if (!python_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PYTHON_CURRENT_SCRIPT_NAME, "config_write_option");
        PYTHON_RETURN_ERROR;
    }
    
    config_file = NULL;
    option = NULL;
    
    if (!PyArg_ParseTuple (args, "ss", &config_file, &option))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PYTHON_CURRENT_SCRIPT_NAME, "config_write_option");
        PYTHON_RETURN_ERROR;
    }
    
    weechat_config_write_option (script_str2ptr (config_file),
                                 script_str2ptr (option));
    
    PYTHON_RETURN_OK;
}

/*
 * weechat_python_api_config_write_line: write a line in configuration file
 */

static PyObject *
weechat_python_api_config_write_line (PyObject *self, PyObject *args)
{
    char *config_file, *option_name, *value;
    
    /* make C compiler happy */
    (void) self;
    
    if (!python_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PYTHON_CURRENT_SCRIPT_NAME, "config_write_line");
        PYTHON_RETURN_ERROR;
    }
    
    config_file = NULL;
    option_name = NULL;
    value = NULL;
    
    if (!PyArg_ParseTuple (args, "sss", &config_file, &option_name, &value))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PYTHON_CURRENT_SCRIPT_NAME, "config_write_line");
        PYTHON_RETURN_ERROR;
    }
    
    weechat_config_write_line (script_str2ptr (config_file),
                               option_name,
                               "%s",
                               value);
    
    PYTHON_RETURN_OK;
}

/*
 * weechat_python_api_config_write: write configuration file
 */

static PyObject *
weechat_python_api_config_write (PyObject *self, PyObject *args)
{
    char *config_file;
    int rc;
    
    /* make C compiler happy */
    (void) self;
    
    if (!python_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PYTHON_CURRENT_SCRIPT_NAME, "config_write");
        PYTHON_RETURN_INT(-1);
    }
    
    config_file = NULL;
    
    if (!PyArg_ParseTuple (args, "s", &config_file))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PYTHON_CURRENT_SCRIPT_NAME, "config_write");
        PYTHON_RETURN_INT(-1);
    }
    
    rc = weechat_config_write (script_str2ptr (config_file));
    
    PYTHON_RETURN_INT(rc);
}

/*
 * weechat_python_api_config_read: read configuration file
 */

static PyObject *
weechat_python_api_config_read (PyObject *self, PyObject *args)
{
    char *config_file;
    int rc;
    
    /* make C compiler happy */
    (void) self;
    
    if (!python_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PYTHON_CURRENT_SCRIPT_NAME, "config_read");
        PYTHON_RETURN_INT(-1);
    }
    
    config_file = NULL;
    
    if (!PyArg_ParseTuple (args, "s", &config_file))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PYTHON_CURRENT_SCRIPT_NAME, "config_read");
        PYTHON_RETURN_INT(-1);
    }
    
    rc = weechat_config_read (script_str2ptr (config_file));
    
    PYTHON_RETURN_INT(rc);
}

/*
 * weechat_python_api_config_reload: reload configuration file
 */

static PyObject *
weechat_python_api_config_reload (PyObject *self, PyObject *args)
{
    char *config_file;
    int rc;
    
    /* make C compiler happy */
    (void) self;
    
    if (!python_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PYTHON_CURRENT_SCRIPT_NAME, "config_reload");
        PYTHON_RETURN_INT(-1);
    }
    
    config_file = NULL;
    
    if (!PyArg_ParseTuple (args, "s", &config_file))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PYTHON_CURRENT_SCRIPT_NAME, "config_reload");
        PYTHON_RETURN_INT(-1);
    }
    
    rc = weechat_config_reload (script_str2ptr (config_file));
    
    PYTHON_RETURN_INT(rc);
}

/*
 * weechat_python_api_config_option_free: free an option in configuration file
 */

static PyObject *
weechat_python_api_config_option_free (PyObject *self, PyObject *args)
{
    char *option;
    
    /* make C compiler happy */
    (void) self;
    
    if (!python_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PYTHON_CURRENT_SCRIPT_NAME, "config_option_free");
        PYTHON_RETURN_ERROR;
    }
    
    option = NULL;
    
    if (!PyArg_ParseTuple (args, "s", &option))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PYTHON_CURRENT_SCRIPT_NAME, "config_option_free");
        PYTHON_RETURN_ERROR;
    }
    
    script_api_config_option_free (weechat_python_plugin,
                                   python_current_script,
                                   script_str2ptr (option));
    
    PYTHON_RETURN_OK;
}

/*
 * weechat_python_api_config_section_free_options: free all options of a section
 *                                                 in configuration file
 */

static PyObject *
weechat_python_api_config_section_free_options (PyObject *self, PyObject *args)
{
    char *section;
    
    /* make C compiler happy */
    (void) self;
    
    if (!python_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PYTHON_CURRENT_SCRIPT_NAME, "config_section_free_options");
        PYTHON_RETURN_ERROR;
    }
    
    section = NULL;
    
    if (!PyArg_ParseTuple (args, "s", &section))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PYTHON_CURRENT_SCRIPT_NAME, "config_section_free_options");
        PYTHON_RETURN_ERROR;
    }
    
    script_api_config_section_free_options (weechat_python_plugin,
                                            python_current_script,
                                            script_str2ptr (section));
    
    PYTHON_RETURN_OK;
}

/*
 * weechat_python_api_config_section_free: free section in configuration file
 */

static PyObject *
weechat_python_api_config_section_free (PyObject *self, PyObject *args)
{
    char *section;
    
    /* make C compiler happy */
    (void) self;
    
    if (!python_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PYTHON_CURRENT_SCRIPT_NAME, "config_section_free");
        PYTHON_RETURN_ERROR;
    }
    
    section = NULL;
    
    if (!PyArg_ParseTuple (args, "s", &section))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PYTHON_CURRENT_SCRIPT_NAME, "config_section_free");
        PYTHON_RETURN_ERROR;
    }
    
    script_api_config_section_free (weechat_python_plugin,
                                    python_current_script,
                                    script_str2ptr (section));
    
    PYTHON_RETURN_OK;
}

/*
 * weechat_python_api_config_free: free configuration file
 */

static PyObject *
weechat_python_api_config_free (PyObject *self, PyObject *args)
{
    char *config_file;
    
    /* make C compiler happy */
    (void) self;
    
    if (!python_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PYTHON_CURRENT_SCRIPT_NAME, "config_free");
        PYTHON_RETURN_ERROR;
    }
    
    config_file = NULL;
    
    if (!PyArg_ParseTuple (args, "s", &config_file))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PYTHON_CURRENT_SCRIPT_NAME, "config_free");
        PYTHON_RETURN_ERROR;
    }
    
    script_api_config_free (weechat_python_plugin,
                            python_current_script,
                            script_str2ptr (config_file));
    
    PYTHON_RETURN_OK;
}

/*
 * weechat_python_api_config_get: get config option
 */

static PyObject *
weechat_python_api_config_get (PyObject *self, PyObject *args)
{
    char *option, *result;
    PyObject *object;
    
    /* make C compiler happy */
    (void) self;
    
    if (!python_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PYTHON_CURRENT_SCRIPT_NAME, "config_get");
        PYTHON_RETURN_EMPTY;
    }
    
    option = NULL;
    
    if (!PyArg_ParseTuple (args, "s", &option))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PYTHON_CURRENT_SCRIPT_NAME, "config_get");
        PYTHON_RETURN_EMPTY;
    }
    
    result = script_ptr2str (weechat_config_get (option));
    
    PYTHON_RETURN_STRING_FREE(result);
}

/*
 * weechat_python_api_config_get_plugin: get value of a plugin option
 */

static PyObject *
weechat_python_api_config_get_plugin (PyObject *self, PyObject *args)
{
    char *option;
    const char *result;
    
    /* make C compiler happy */
    (void) self;
    
    if (!python_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PYTHON_CURRENT_SCRIPT_NAME, "config_get_plugin");
        PYTHON_RETURN_EMPTY;
    }
    
    option = NULL;
    
    if (!PyArg_ParseTuple (args, "s", &option))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PYTHON_CURRENT_SCRIPT_NAME, "config_get_plugin");
        PYTHON_RETURN_EMPTY;
    }
    
    result = script_api_config_get_plugin (weechat_python_plugin,
                                           python_current_script,
                                           option);
    
    PYTHON_RETURN_STRING(result);
}

/*
 * weechat_python_api_config_is_set_plugin: check if a plugin option is set
 */

static PyObject *
weechat_python_api_config_is_set_plugin (PyObject *self, PyObject *args)
{
    char *option;
    int rc;
    
    /* make C compiler happy */
    (void) self;
    
    if (!python_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PYTHON_CURRENT_SCRIPT_NAME, "config_is_set_plugin");
        PYTHON_RETURN_INT(0);
    }
    
    option = NULL;
    
    if (!PyArg_ParseTuple (args, "s", &option))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PYTHON_CURRENT_SCRIPT_NAME, "config_is_set_plugin");
        PYTHON_RETURN_INT(WEECHAT_CONFIG_OPTION_SET_ERROR);
    }
    
    rc = script_api_config_is_set_plugin (weechat_python_plugin,
                                          python_current_script,
                                          option);
    
    PYTHON_RETURN_INT(rc);
}

/*
 * weechat_python_api_config_set_plugin: set value of a plugin option
 */

static PyObject *
weechat_python_api_config_set_plugin (PyObject *self, PyObject *args)
{
    char *option, *value;
    int rc;
    
    /* make C compiler happy */
    (void) self;
    
    if (!python_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PYTHON_CURRENT_SCRIPT_NAME, "config_set_plugin");
        PYTHON_RETURN_INT(WEECHAT_CONFIG_OPTION_SET_ERROR);
    }
    
    option = NULL;
    value = NULL;
    
    if (!PyArg_ParseTuple (args, "ss", &option, &value))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PYTHON_CURRENT_SCRIPT_NAME, "config_set_plugin");
        PYTHON_RETURN_INT(WEECHAT_CONFIG_OPTION_SET_ERROR);
    }
    
    rc = script_api_config_set_plugin (weechat_python_plugin,
                                       python_current_script,
                                       option,
                                       value);
    
    PYTHON_RETURN_INT(rc);
}

/*
 * weechat_python_api_config_unset_plugin: unset plugin option
 */

static PyObject *
weechat_python_api_config_unset_plugin (PyObject *self, PyObject *args)
{
    char *option;
    int rc;
    
    /* make C compiler happy */
    (void) self;
    
    if (!python_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PYTHON_CURRENT_SCRIPT_NAME, "config_unset_plugin");
        PYTHON_RETURN_INT(WEECHAT_CONFIG_OPTION_UNSET_ERROR);
    }
    
    option = NULL;
    
    if (!PyArg_ParseTuple (args, "s", &option))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PYTHON_CURRENT_SCRIPT_NAME, "config_unset_plugin");
        PYTHON_RETURN_INT(WEECHAT_CONFIG_OPTION_UNSET_ERROR);
    }
    
    rc = script_api_config_unset_plugin (weechat_python_plugin,
                                         python_current_script,
                                         option);
    
    PYTHON_RETURN_INT(rc);
}

/*
 * weechat_python_api_prefix: get a prefix, used for display
 */

static PyObject *
weechat_python_api_prefix (PyObject *self, PyObject *args)
{
    char *prefix;
    const char *result;
    
    /* make C compiler happy */
    (void) self;
    
    if (!python_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PYTHON_CURRENT_SCRIPT_NAME, "prefix");
        PYTHON_RETURN_EMPTY;
    }
    
    prefix = NULL;
    
    if (!PyArg_ParseTuple (args, "s", &prefix))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PYTHON_CURRENT_SCRIPT_NAME, "prefix");
        PYTHON_RETURN_EMPTY;
    }
    
    result = weechat_prefix (prefix);
    
    PYTHON_RETURN_STRING(result);
}

/*
 * weechat_python_api_color: get a color code, used for display
 */

static PyObject *
weechat_python_api_color (PyObject *self, PyObject *args)
{
    char *color;
    const char *result;
    
    /* make C compiler happy */
    (void) self;
    
    if (!python_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PYTHON_CURRENT_SCRIPT_NAME, "color");
        PYTHON_RETURN_EMPTY;
    }
    
    color = NULL;
    
    if (!PyArg_ParseTuple (args, "s", &color))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PYTHON_CURRENT_SCRIPT_NAME, "color");
        PYTHON_RETURN_EMPTY;
    }
    
    result = weechat_color (color);
    
    PYTHON_RETURN_STRING(result);
}

/*
 * weechat_python_api_prnt: print message in a buffer
 */

static PyObject *
weechat_python_api_prnt (PyObject *self, PyObject *args)
{
    char *buffer, *message;
    
    /* make C compiler happy */
    (void) self;
    
    buffer = NULL;
    message = NULL;
    
    if (!PyArg_ParseTuple (args, "ss", &buffer, &message))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PYTHON_CURRENT_SCRIPT_NAME, "prnt");
        PYTHON_RETURN_ERROR;
    }
    
    script_api_printf (weechat_python_plugin,
                       python_current_script,
                       script_str2ptr (buffer),
                       "%s", message);
    
    PYTHON_RETURN_OK;
}

/*
 * weechat_python_api_prnt_date_tags: print message in a buffer with optional
 *                                    date and tags
 */

static PyObject *
weechat_python_api_prnt_date_tags (PyObject *self, PyObject *args)
{
    char *buffer, *tags, *message;
    int date;
    
    /* make C compiler happy */
    (void) self;
    
    if (!python_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PYTHON_CURRENT_SCRIPT_NAME, "prnt_date_tags");
        PYTHON_RETURN_ERROR;
    }
    
    buffer = NULL;
    date = 0;
    tags = NULL;
    message = NULL;
    
    if (!PyArg_ParseTuple (args, "siss", &buffer, &date, &tags, &message))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PYTHON_CURRENT_SCRIPT_NAME, "prnt_date_tags");
        PYTHON_RETURN_ERROR;
    }
    
    script_api_printf_date_tags (weechat_python_plugin,
                                 python_current_script,
                                 script_str2ptr (buffer),
                                 date,
                                 tags,
                                 "%s", message);
    
    PYTHON_RETURN_OK;
}

/*
 * weechat_python_api_prnt_y: print message in a buffer with free content
 */

static PyObject *
weechat_python_api_prnt_y (PyObject *self, PyObject *args)
{
    char *buffer, *message;
    int y;
    
    /* make C compiler happy */
    (void) self;
    
    if (!python_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PYTHON_CURRENT_SCRIPT_NAME, "prnt_y");
        PYTHON_RETURN_ERROR;
    }
    
    buffer = NULL;
    y = 0;
    message = NULL;
    
    if (!PyArg_ParseTuple (args, "sis", &buffer, &y, &message))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PYTHON_CURRENT_SCRIPT_NAME, "prnt_y");
        PYTHON_RETURN_ERROR;
    }
    
    script_api_printf_y (weechat_python_plugin,
                          python_current_script,
                          script_str2ptr (buffer),
                          y,
                          "%s", message);
    
    PYTHON_RETURN_OK;
}

/*
 * weechat_python_api_log_print: print message in WeeChat log file
 */

static PyObject *
weechat_python_api_log_print (PyObject *self, PyObject *args)
{
    char *message;
    
    /* make C compiler happy */
    (void) self;
    
    if (!python_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PYTHON_CURRENT_SCRIPT_NAME, "log_print");
        PYTHON_RETURN_ERROR;
    }
    
    message = NULL;
    
    if (!PyArg_ParseTuple (args, "s", &message))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PYTHON_CURRENT_SCRIPT_NAME, "log_print");
        PYTHON_RETURN_ERROR;
    }
    
    script_api_log_printf (weechat_python_plugin,
                           python_current_script,
                           "%s", message);
    
    PYTHON_RETURN_OK;
}

/*
 * weechat_python_api_hook_command_cb: callback for command hooked
 */

int
weechat_python_api_hook_command_cb (void *data, struct t_gui_buffer *buffer,
                                    int argc, char **argv, char **argv_eol)
{
    struct t_script_callback *script_callback;
    char *python_argv[4], empty_arg[1] = { '\0' };
    int *rc, ret;

    /* make C compiler happy */
    (void) argv;
    
    script_callback = (struct t_script_callback *)data;

    if (script_callback && script_callback->function && script_callback->function[0])
    {
        python_argv[0] = (script_callback->data) ? script_callback->data : empty_arg;
        python_argv[1] = script_ptr2str (buffer);
        python_argv[2] = (argc > 1) ? argv_eol[1] : empty_arg;
        python_argv[3] = NULL;
        
        rc = (int *) weechat_python_exec (script_callback->script,
                                          WEECHAT_SCRIPT_EXEC_INT,
                                          script_callback->function,
                                          python_argv);
        
        if (!rc)
            ret = WEECHAT_RC_ERROR;
        else
        {
            ret = *rc;
            free (rc);
        }
        if (python_argv[1])
            free (python_argv[1]);
        
        return ret;
    }
    
    return WEECHAT_RC_ERROR;
}

/*
 * weechat_python_api_hook_command: hook a command
 */

static PyObject *
weechat_python_api_hook_command (PyObject *self, PyObject *args)
{
    char *command, *description, *arguments, *args_description, *completion;
    char *function, *data, *result;
    PyObject *object;
    
    /* make C compiler happy */
    (void) self;
    
    if (!python_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PYTHON_CURRENT_SCRIPT_NAME, "hook_command");
        PYTHON_RETURN_EMPTY;
    }
    
    command = NULL;
    description = NULL;
    arguments = NULL;
    args_description = NULL;
    completion = NULL;
    function = NULL;
    data = NULL;
    
    if (!PyArg_ParseTuple (args, "sssssss", &command, &description, &arguments,
                           &args_description, &completion, &function, &data))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PYTHON_CURRENT_SCRIPT_NAME, "hook_command");
        PYTHON_RETURN_EMPTY;
    }
    
    result = script_ptr2str (script_api_hook_command (weechat_python_plugin,
                                                      python_current_script,
                                                      command,
                                                      description,
                                                      arguments,
                                                      args_description,
                                                      completion,
                                                      &weechat_python_api_hook_command_cb,
                                                      function,
                                                      data));
    
    PYTHON_RETURN_STRING_FREE(result);
}

/*
 * weechat_python_api_hook_command_run_cb: callback for command_run hooked
 */

int
weechat_python_api_hook_command_run_cb (void *data, struct t_gui_buffer *buffer,
                                        const char *command)
{
    struct t_script_callback *script_callback;
    char *python_argv[4], empty_arg[1] = { '\0' };
    int *rc, ret;
    
    script_callback = (struct t_script_callback *)data;
    
    if (script_callback && script_callback->function && script_callback->function[0])
    {
        python_argv[0] = (script_callback->data) ? script_callback->data : empty_arg;
        python_argv[1] = script_ptr2str (buffer);
        python_argv[2] = (command) ? (char *)command : empty_arg;
        python_argv[3] = NULL;
        
        rc = (int *) weechat_python_exec (script_callback->script,
                                          WEECHAT_SCRIPT_EXEC_INT,
                                          script_callback->function,
                                          python_argv);
        
        if (!rc)
            ret = WEECHAT_RC_ERROR;
        else
        {
            ret = *rc;
            free (rc);
        }
        if (python_argv[1])
            free (python_argv[1]);
        
        return ret;
    }
    
    return WEECHAT_RC_ERROR;
}

/*
 * weechat_python_api_hook_command_run: hook a command_run
 */

static PyObject *
weechat_python_api_hook_command_run (PyObject *self, PyObject *args)
{
    char *command, *function, *data, *result;
    PyObject *object;
    
    /* make C compiler happy */
    (void) self;
    
    if (!python_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PYTHON_CURRENT_SCRIPT_NAME, "hook_command_run");
        PYTHON_RETURN_EMPTY;
    }
    
    command = NULL;
    function = NULL;
    data = NULL;
    
    if (!PyArg_ParseTuple (args, "sss", &command, &function, &data))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PYTHON_CURRENT_SCRIPT_NAME, "hook_command_run");
        PYTHON_RETURN_EMPTY;
    }
    
    result = script_ptr2str (script_api_hook_command_run (weechat_python_plugin,
                                                          python_current_script,
                                                          command,
                                                          &weechat_python_api_hook_command_run_cb,
                                                          function,
                                                          data));
    
    PYTHON_RETURN_STRING_FREE(result);
}

/*
 * weechat_python_api_hook_timer_cb: callback for timer hooked
 */

int
weechat_python_api_hook_timer_cb (void *data, int remaining_calls)
{
    struct t_script_callback *script_callback;
    char *python_argv[3], str_remaining_calls[32], empty_arg[1] = { '\0' };
    int *rc, ret;
    
    script_callback = (struct t_script_callback *)data;

    if (script_callback && script_callback->function && script_callback->function[0])
    {
        snprintf (str_remaining_calls, sizeof (str_remaining_calls),
                  "%d", remaining_calls);

        python_argv[0] = (script_callback->data) ? script_callback->data : empty_arg;
        python_argv[1] = str_remaining_calls;
        python_argv[2] = NULL;
        
        rc = (int *) weechat_python_exec (script_callback->script,
                                          WEECHAT_SCRIPT_EXEC_INT,
                                          script_callback->function,
                                          python_argv);
        
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
 * weechat_python_api_hook_timer: hook a timer
 */

static PyObject *
weechat_python_api_hook_timer (PyObject *self, PyObject *args)
{
    int interval, align_second, max_calls;
    char *function, *data, *result;
    PyObject *object;
    
    /* make C compiler happy */
    (void) self;
    
    if (!python_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PYTHON_CURRENT_SCRIPT_NAME, "hook_timer");
        PYTHON_RETURN_EMPTY;
    }
    
    interval = 10;
    align_second = 0;
    max_calls = 0;
    function = NULL;
    data = NULL;
    
    if (!PyArg_ParseTuple (args, "iiiss", &interval, &align_second, &max_calls,
                           &function, &data))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PYTHON_CURRENT_SCRIPT_NAME, "hook_timer");
        PYTHON_RETURN_EMPTY;
    }
    
    result = script_ptr2str (script_api_hook_timer (weechat_python_plugin,
                                                    python_current_script,
                                                    interval,
                                                    align_second,
                                                    max_calls,
                                                    &weechat_python_api_hook_timer_cb,
                                                    function,
                                                    data));
    
    PYTHON_RETURN_STRING_FREE(result);
}

/*
 * weechat_python_api_hook_fd_cb: callback for fd hooked
 */

int
weechat_python_api_hook_fd_cb (void *data, int fd)
{
    struct t_script_callback *script_callback;
    char *python_argv[3], str_fd[32], empty_arg[1] = { '\0' };
    int *rc, ret;
    
    script_callback = (struct t_script_callback *)data;

    if (script_callback && script_callback->function && script_callback->function[0])
    {
        snprintf (str_fd, sizeof (str_fd), "%d", fd);
        
        python_argv[0] = (script_callback->data) ? script_callback->data : empty_arg;
        python_argv[1] = str_fd;
        python_argv[2] = NULL;
        
        rc = (int *) weechat_python_exec (script_callback->script,
                                          WEECHAT_SCRIPT_EXEC_INT,
                                          script_callback->function,
                                          python_argv);
        
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
 * weechat_python_api_hook_fd: hook a fd
 */

static PyObject *
weechat_python_api_hook_fd (PyObject *self, PyObject *args)
{
    int fd, read, write, exception;
    char *function, *data, *result;
    PyObject *object;
    
    /* make C compiler happy */
    (void) self;
    
    if (!python_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PYTHON_CURRENT_SCRIPT_NAME, "hook_fd");
        PYTHON_RETURN_EMPTY;
    }
    
    fd = 0;
    read = 0;
    write = 0;
    exception = 0;
    function = NULL;
    data = NULL;
    
    if (!PyArg_ParseTuple (args, "iiiiss", &fd, &read, &write, &exception,
                           &function, &data))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PYTHON_CURRENT_SCRIPT_NAME, "hook_fd");
        PYTHON_RETURN_EMPTY;
    }
    
    result = script_ptr2str (script_api_hook_fd (weechat_python_plugin,
                                                 python_current_script,
                                                 fd,
                                                 read,
                                                 write,
                                                 exception,
                                                 &weechat_python_api_hook_fd_cb,
                                                 function,
                                                 data));
    
    PYTHON_RETURN_STRING_FREE(result);
}

/*
 * weechat_python_api_hook_process_cb: callback for process hooked
 */

int
weechat_python_api_hook_process_cb (void *data,
                                    const char *command, int return_code,
                                    const char *out, const char *err)
{
    struct t_script_callback *script_callback;
    char *python_argv[6], str_rc[32], empty_arg[1] = { '\0' };
    int *rc, ret;
    
    script_callback = (struct t_script_callback *)data;

    if (script_callback && script_callback->function && script_callback->function[0])
    {
        snprintf (str_rc, sizeof (str_rc), "%d", return_code);
        
        python_argv[0] = (script_callback->data) ? script_callback->data : empty_arg;
        python_argv[1] = (command) ? (char *)command : empty_arg;
        python_argv[2] = str_rc;
        python_argv[3] = (out) ? (char *)out : empty_arg;
        python_argv[4] = (err) ? (char *)err : empty_arg;
        python_argv[5] = NULL;
        
        rc = (int *) weechat_python_exec (script_callback->script,
                                          WEECHAT_SCRIPT_EXEC_INT,
                                          script_callback->function,
                                          python_argv);
        
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
 * weechat_python_api_hook_process: hook a process
 */

static PyObject *
weechat_python_api_hook_process (PyObject *self, PyObject *args)
{
    char *command, *function, *data, *result;
    int timeout;
    PyObject *object;
    
    /* make C compiler happy */
    (void) self;
    
    if (!python_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PYTHON_CURRENT_SCRIPT_NAME, "hook_process");
        PYTHON_RETURN_EMPTY;
    }
    
    command = NULL;
    timeout = 0;
    function = NULL;
    data = NULL;
    
    if (!PyArg_ParseTuple (args, "siss", &command, &timeout, &function, &data))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PYTHON_CURRENT_SCRIPT_NAME, "hook_process");
        PYTHON_RETURN_EMPTY;
    }
    
    result = script_ptr2str (script_api_hook_process (weechat_python_plugin,
                                                      python_current_script,
                                                      command,
                                                      timeout,
                                                      &weechat_python_api_hook_process_cb,
                                                      function,
                                                      data));
    
    PYTHON_RETURN_STRING_FREE(result);
}

/*
 * weechat_python_api_hook_connect_cb: callback for connect hooked
 */

int
weechat_python_api_hook_connect_cb (void *data, int status,
                                    const char *error, const char *ip_address)
{
    struct t_script_callback *script_callback;
    char *python_argv[5], str_status[32], empty_arg[1] = { '\0' };
    int *rc, ret;
    
    script_callback = (struct t_script_callback *)data;

    if (script_callback && script_callback->function && script_callback->function[0])
    {
        snprintf (str_status, sizeof (str_status), "%d", status);
        
        python_argv[0] = (script_callback->data) ? script_callback->data : empty_arg;
        python_argv[1] = str_status;
        python_argv[2] = (ip_address) ? (char *)ip_address : empty_arg;
        python_argv[3] = (error) ? (char *)error : empty_arg;
        python_argv[4] = NULL;
        
        rc = (int *) weechat_python_exec (script_callback->script,
                                          WEECHAT_SCRIPT_EXEC_INT,
                                          script_callback->function,
                                          python_argv);
        
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
 * weechat_python_api_hook_connect: hook a connection
 */

static PyObject *
weechat_python_api_hook_connect (PyObject *self, PyObject *args)
{
    char *proxy, *address, *local_hostname, *function, *data, *result;
    int port, sock, ipv6;
    PyObject *object;
    
    /* make C compiler happy */
    (void) self;
    
    if (!python_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PYTHON_CURRENT_SCRIPT_NAME, "hook_connect");
        PYTHON_RETURN_EMPTY;
    }
    
    proxy = NULL;
    address = NULL;
    port = 0;
    sock = 0;
    ipv6 = 0;
    local_hostname = NULL;
    function = NULL;
    data = NULL;
    
    if (!PyArg_ParseTuple (args, "ssiiisss", &proxy, &address, &port, &sock,
                           &ipv6, &local_hostname, &function, &data))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PYTHON_CURRENT_SCRIPT_NAME, "hook_connect");
        PYTHON_RETURN_EMPTY;
    }
    
    result = script_ptr2str (script_api_hook_connect (weechat_python_plugin,
                                                      python_current_script,
                                                      proxy,
                                                      address,
                                                      port,
                                                      sock,
                                                      ipv6,
                                                      NULL, /* gnutls session */
                                                      local_hostname,
                                                      &weechat_python_api_hook_connect_cb,
                                                      function,
                                                      data));
    
    PYTHON_RETURN_STRING_FREE(result);
}

/*
 * weechat_python_api_hook_print_cb: callback for print hooked
 */

int
weechat_python_api_hook_print_cb (void *data, struct t_gui_buffer *buffer,
                                  time_t date,
                                  int tags_count, const char **tags,
                                  int displayed, int highlight,
                                  const char *prefix, const char *message)
{
    struct t_script_callback *script_callback;
    char *python_argv[9], empty_arg[1] = { '\0' };
    static char timebuffer[64];
    int *rc, ret;
    
    /* make C compiler happy */
    (void) tags_count;
    
    script_callback = (struct t_script_callback *)data;

    if (script_callback && script_callback->function && script_callback->function[0])
    {
        snprintf (timebuffer, sizeof (timebuffer) - 1, "%ld", (long int)date);
        
        python_argv[0] = (script_callback->data) ? script_callback->data : empty_arg;
        python_argv[1] = script_ptr2str (buffer);
        python_argv[2] = timebuffer;
        python_argv[3] = weechat_string_build_with_split_string (tags, ",");
        if (!python_argv[3])
            python_argv[3] = strdup ("");
        python_argv[4] = (displayed) ? strdup ("1") : strdup ("0");
        python_argv[5] = (highlight) ? strdup ("1") : strdup ("0");
        python_argv[6] = (prefix) ? (char *)prefix : empty_arg;
        python_argv[7] = (message) ? (char *)message : empty_arg;
        python_argv[8] = NULL;
        
        rc = (int *) weechat_python_exec (script_callback->script,
                                          WEECHAT_SCRIPT_EXEC_INT,
                                          script_callback->function,
                                          python_argv);
        
        if (!rc)
            ret = WEECHAT_RC_ERROR;
        else
        {
            ret = *rc;
            free (rc);
        }
        if (python_argv[1])
            free (python_argv[1]);
        if (python_argv[3])
            free (python_argv[3]);
        if (python_argv[4])
            free (python_argv[4]);
        if (python_argv[5])
            free (python_argv[5]);
        
        return ret;
    }
    
    return WEECHAT_RC_ERROR;
}

/*
 * weechat_python_api_hook_print: hook a print
 */

static PyObject *
weechat_python_api_hook_print (PyObject *self, PyObject *args)
{
    char *buffer, *tags, *message, *function, *data, *result;
    int strip_colors;
    PyObject *object;
    
    /* make C compiler happy */
    (void) self;
    
    if (!python_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PYTHON_CURRENT_SCRIPT_NAME, "hook_print");
        PYTHON_RETURN_EMPTY;
    }
    
    buffer = NULL;
    tags = NULL;
    message = NULL;
    strip_colors = 0;
    function = NULL;
    data = NULL;
    
    if (!PyArg_ParseTuple (args, "sssiss", &buffer, &tags, &message,
                           &strip_colors, &function, &data))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PYTHON_CURRENT_SCRIPT_NAME, "hook_print");
        PYTHON_RETURN_EMPTY;
    }
    
    result = script_ptr2str(script_api_hook_print (weechat_python_plugin,
                                                   python_current_script,
                                                   script_str2ptr (buffer),
                                                   tags,
                                                   message,
                                                   strip_colors,
                                                   &weechat_python_api_hook_print_cb,
                                                   function,
                                                   data));
    
    PYTHON_RETURN_STRING_FREE(result);
}

/*
 * weechat_python_api_hook_signal_cb: callback for signal hooked
 */

int
weechat_python_api_hook_signal_cb (void *data, const char *signal, const char *type_data,
                                   void *signal_data)
{
    struct t_script_callback *script_callback;
    char *python_argv[4], empty_arg[1] = { '\0' };
    static char value_str[64];
    int *rc, ret, free_needed;
    
    script_callback = (struct t_script_callback *)data;
    
    if (script_callback && script_callback->function && script_callback->function[0])
    {
        python_argv[0] = (script_callback->data) ? script_callback->data : empty_arg;
        python_argv[1] = (signal) ? (char *)signal : empty_arg;
        free_needed = 0;
        if (strcmp (type_data, WEECHAT_HOOK_SIGNAL_STRING) == 0)
        {
            python_argv[2] = (signal_data) ? (char *)signal_data : empty_arg;
        }
        else if (strcmp (type_data, WEECHAT_HOOK_SIGNAL_INT) == 0)
        {
            snprintf (value_str, sizeof (value_str) - 1,
                      "%d", *((int *)signal_data));
            python_argv[2] = value_str;
        }
        else if (strcmp (type_data, WEECHAT_HOOK_SIGNAL_POINTER) == 0)
        {
            python_argv[2] = script_ptr2str (signal_data);
            free_needed = 1;
        }
        else
            python_argv[2] = empty_arg;
        python_argv[3] = NULL;
        
        rc = (int *) weechat_python_exec (script_callback->script,
                                          WEECHAT_SCRIPT_EXEC_INT,
                                          script_callback->function,
                                          python_argv);
        
        if (!rc)
            ret = WEECHAT_RC_ERROR;
        else
        {
            ret = *rc;
            free (rc);
        }
        if (free_needed && python_argv[2])
            free (python_argv[2]);
        
        return ret;
    }
    
    return WEECHAT_RC_ERROR;
}

/*
 * weechat_python_api_hook_signal: hook a signal
 */

static PyObject *
weechat_python_api_hook_signal (PyObject *self, PyObject *args)
{
    char *signal, *function, *data, *result;
    PyObject *object;
    
    /* make C compiler happy */
    (void) self;
    
    if (!python_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PYTHON_CURRENT_SCRIPT_NAME, "hook_signal");
        PYTHON_RETURN_EMPTY;
    }
    
    signal = NULL;
    function = NULL;
    data = NULL;
    
    if (!PyArg_ParseTuple (args, "sss", &signal, &function, &data))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PYTHON_CURRENT_SCRIPT_NAME, "hook_signal");
        PYTHON_RETURN_EMPTY;
    }
    
    result = script_ptr2str (script_api_hook_signal (weechat_python_plugin,
                                                     python_current_script,
                                                     signal,
                                                     &weechat_python_api_hook_signal_cb,
                                                     function,
                                                     data));
    
    PYTHON_RETURN_STRING_FREE(result);
}

/*
 * weechat_python_api_hook_signal_send: send a signal
 */

static PyObject *
weechat_python_api_hook_signal_send (PyObject *self, PyObject *args)
{
    char *signal, *type_data, *signal_data, *error;
    int number;
    
    /* make C compiler happy */
    (void) self;
    
    if (!python_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PYTHON_CURRENT_SCRIPT_NAME, "hook_signal_send");
        PYTHON_RETURN_ERROR;
    }
    
    signal = NULL;
    type_data = NULL;
    signal_data = NULL;
    
    if (!PyArg_ParseTuple (args, "sss", &signal, &type_data, &signal_data))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PYTHON_CURRENT_SCRIPT_NAME, "hook_signal_send");
        PYTHON_RETURN_ERROR;
    }
    
    if (strcmp (type_data, WEECHAT_HOOK_SIGNAL_STRING) == 0)
    {
        weechat_hook_signal_send (signal, type_data, signal_data);
        PYTHON_RETURN_OK;
    }
    else if (strcmp (type_data, WEECHAT_HOOK_SIGNAL_INT) == 0)
    {
        error = NULL;
        number = (int)strtol (signal_data, &error, 10);
        if (error && !error[0])
        {
            weechat_hook_signal_send (signal, type_data, &number);
        }
        PYTHON_RETURN_OK;
    }
    else if (strcmp (type_data, WEECHAT_HOOK_SIGNAL_POINTER) == 0)
    {
        weechat_hook_signal_send (signal, type_data,
                                  script_str2ptr (signal_data));
        PYTHON_RETURN_OK;
    }
    
    PYTHON_RETURN_ERROR;
}

/*
 * weechat_python_api_hook_config_cb: callback for config option hooked
 */

int
weechat_python_api_hook_config_cb (void *data, const char *option, const char *value)
{
    struct t_script_callback *script_callback;
    char *python_argv[4], empty_arg[1] = { '\0' };
    int *rc, ret;
    
    script_callback = (struct t_script_callback *)data;

    if (script_callback && script_callback->function && script_callback->function[0])
    {
        python_argv[0] = (script_callback->data) ? script_callback->data : empty_arg;
        python_argv[1] = (option) ? (char *)option : empty_arg;
        python_argv[2] = (value) ? (char *)value : empty_arg;
        python_argv[3] = NULL;
        
        rc = (int *) weechat_python_exec (script_callback->script,
                                          WEECHAT_SCRIPT_EXEC_INT,
                                          script_callback->function,
                                          python_argv);
        
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
 * weechat_python_api_hook_config: hook a config option
 */

static PyObject *
weechat_python_api_hook_config (PyObject *self, PyObject *args)
{
    char *option, *function, *data, *result;
    PyObject *object;
    
    /* make C compiler happy */
    (void) self;
    
    if (!python_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PYTHON_CURRENT_SCRIPT_NAME, "hook_config");
        PYTHON_RETURN_EMPTY;
    }
    
    option = NULL;
    function = NULL;
    data = NULL;
    
    if (!PyArg_ParseTuple (args, "sss", &option, &function, &data))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PYTHON_CURRENT_SCRIPT_NAME, "hook_config");
        PYTHON_RETURN_EMPTY;
    }
    
    result = script_ptr2str(script_api_hook_config (weechat_python_plugin,
                                                    python_current_script,
                                                    option,
                                                    &weechat_python_api_hook_config_cb,
                                                    function,
                                                    data));
    
    PYTHON_RETURN_STRING_FREE(result);
}

/*
 * weechat_python_api_hook_completion_cb: callback for completion hooked
 */

int
weechat_python_api_hook_completion_cb (void *data, const char *completion_item,
                                       struct t_gui_buffer *buffer,
                                       struct t_gui_completion *completion)
{
    struct t_script_callback *script_callback;
    char *python_argv[5], empty_arg[1] = { '\0' };
    int *rc, ret;
    
    script_callback = (struct t_script_callback *)data;

    if (script_callback && script_callback->function && script_callback->function[0])
    {
        python_argv[0] = (script_callback->data) ? script_callback->data : empty_arg;
        python_argv[1] = (completion_item) ? (char *)completion_item : empty_arg;
        python_argv[2] = script_ptr2str (buffer);
        python_argv[3] = script_ptr2str (completion);
        python_argv[4] = NULL;
        
        rc = (int *) weechat_python_exec (script_callback->script,
                                          WEECHAT_SCRIPT_EXEC_INT,
                                          script_callback->function,
                                          python_argv);
        
        if (!rc)
            ret = WEECHAT_RC_ERROR;
        else
        {
            ret = *rc;
            free (rc);
        }
        if (python_argv[2])
            free (python_argv[2]);
        if (python_argv[3])
            free (python_argv[3]);
        
        return ret;
    }
    
    return WEECHAT_RC_ERROR;
}

/*
 * weechat_python_api_hook_completion: hook a completion
 */

static PyObject *
weechat_python_api_hook_completion (PyObject *self, PyObject *args)
{
    char *completion, *description, *function, *data, *result;
    PyObject *object;
    
    /* make C compiler happy */
    (void) self;
    
    if (!python_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PYTHON_CURRENT_SCRIPT_NAME, "hook_completion");
        PYTHON_RETURN_EMPTY;
    }
    
    completion = NULL;
    description = NULL;
    function = NULL;
    data = NULL;
    
    if (!PyArg_ParseTuple (args, "ssss", &completion, &description, &function,
                           &data))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PYTHON_CURRENT_SCRIPT_NAME, "hook_completion");
        PYTHON_RETURN_EMPTY;
    }
    
    result = script_ptr2str(script_api_hook_completion (weechat_python_plugin,
                                                        python_current_script,
                                                        completion,
                                                        description,
                                                        &weechat_python_api_hook_completion_cb,
                                                        function,
                                                        data));
    
    PYTHON_RETURN_STRING_FREE(result);
}

/*
 * weechat_python_api_hook_completion_list_add: add a word to list for a completion
 */

static PyObject *
weechat_python_api_hook_completion_list_add (PyObject *self, PyObject *args)
{
    char *completion, *word, *where;
    int nick_completion;
    
    /* make C compiler happy */
    (void) self;
    
    if (!python_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PYTHON_CURRENT_SCRIPT_NAME, "hook_completion_list_add");
        PYTHON_RETURN_ERROR;
    }
    
    completion = NULL;
    word = NULL;
    nick_completion = 0;
    where = NULL;
    
    if (!PyArg_ParseTuple (args, "ssis", &completion, &word, &nick_completion,
                           &where))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PYTHON_CURRENT_SCRIPT_NAME, "hook_completion_list_add");
        PYTHON_RETURN_ERROR;
    }
    
    weechat_hook_completion_list_add (script_str2ptr (completion),
                                      word,
                                      nick_completion,
                                      where);
    
    PYTHON_RETURN_OK;
}

/*
 * weechat_python_api_hook_modifier_cb: callback for modifier hooked
 */

char *
weechat_python_api_hook_modifier_cb (void *data, const char *modifier,
                                     const char *modifier_data, const char *string)
{
    struct t_script_callback *script_callback;
    char *python_argv[5], empty_arg[1] = { '\0' };
    
    script_callback = (struct t_script_callback *)data;
    
    if (script_callback && script_callback->function && script_callback->function[0])
    {
        python_argv[0] = (script_callback->data) ? script_callback->data : empty_arg;
        python_argv[1] = (modifier) ? (char *)modifier : empty_arg;
        python_argv[2] = (modifier_data) ? (char *)modifier_data : empty_arg;
        python_argv[3] = (string) ? (char *)string : empty_arg;
        python_argv[4] = NULL;
        
        return (char *)weechat_python_exec (script_callback->script,
                                            WEECHAT_SCRIPT_EXEC_STRING,
                                            script_callback->function,
                                            python_argv);
    }
    
    return NULL;
}

/*
 * weechat_python_api_hook_modifier: hook a modifier
 */

static PyObject *
weechat_python_api_hook_modifier (PyObject *self, PyObject *args)
{
    char *modifier, *function, *data, *result;
    PyObject *object;
    
    /* make C compiler happy */
    (void) self;
    
    if (!python_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PYTHON_CURRENT_SCRIPT_NAME, "hook_modifier");
        PYTHON_RETURN_EMPTY;
    }
    
    modifier = NULL;
    function = NULL;
    data = NULL;
    
    if (!PyArg_ParseTuple (args, "sss", &modifier, &function, &data))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PYTHON_CURRENT_SCRIPT_NAME, "hook_modifier");
        PYTHON_RETURN_EMPTY;
    }
    
    result = script_ptr2str(script_api_hook_modifier (weechat_python_plugin,
                                                      python_current_script,
                                                      modifier,
                                                      &weechat_python_api_hook_modifier_cb,
                                                      function,
                                                      data));
    
    PYTHON_RETURN_STRING_FREE(result);
}

/*
 * weechat_python_api_hook_modifier_exec: execute a modifier hook
 */

static PyObject *
weechat_python_api_hook_modifier_exec (PyObject *self, PyObject *args)
{
    char *modifier, *modifier_data, *string, *result;
    PyObject *object;
    
    /* make C compiler happy */
    (void) self;
    
    if (!python_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PYTHON_CURRENT_SCRIPT_NAME, "hook_modifier_exec");
        PYTHON_RETURN_EMPTY;
    }
    
    modifier = NULL;
    modifier_data = NULL;
    string = NULL;
    
    if (!PyArg_ParseTuple (args, "sss", &modifier, &modifier_data, &string))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PYTHON_CURRENT_SCRIPT_NAME, "hook_modifier_exec");
        PYTHON_RETURN_EMPTY;
    }
    
    result = weechat_hook_modifier_exec (modifier, modifier_data, string);
    
    PYTHON_RETURN_STRING_FREE(result);
}

/*
 * weechat_python_api_hook_info_cb: callback for info hooked
 */

const char *
weechat_python_api_hook_info_cb (void *data, const char *info_name,
                                 const char *arguments)
{
    struct t_script_callback *script_callback;
    char *python_argv[4], empty_arg[1] = { '\0' };
    
    script_callback = (struct t_script_callback *)data;
    
    if (script_callback && script_callback->function && script_callback->function[0])
    {
        python_argv[0] = (script_callback->data) ? script_callback->data : empty_arg;
        python_argv[1] = (info_name) ? (char *)info_name : empty_arg;
        python_argv[2] = (arguments) ? (char *)arguments : empty_arg;
        python_argv[3] = NULL;
        
        return (const char *)weechat_python_exec (script_callback->script,
                                                  WEECHAT_SCRIPT_EXEC_STRING,
                                                  script_callback->function,
                                                  python_argv);
    }
    
    return NULL;
}

/*
 * weechat_python_api_hook_info: hook an info
 */

static PyObject *
weechat_python_api_hook_info (PyObject *self, PyObject *args)
{
    char *info_name, *description, *function, *data, *result;
    PyObject *object;
    
    /* make C compiler happy */
    (void) self;
    
    if (!python_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PYTHON_CURRENT_SCRIPT_NAME, "hook_info");
        PYTHON_RETURN_EMPTY;
    }
    
    info_name = NULL;
    description = NULL;
    function = NULL;
    data = NULL;
    
    if (!PyArg_ParseTuple (args, "ssss", &info_name, &description, &function,
                           &data))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PYTHON_CURRENT_SCRIPT_NAME, "hook_info");
        PYTHON_RETURN_EMPTY;
    }
    
    result = script_ptr2str(script_api_hook_info (weechat_python_plugin,
                                                  python_current_script,
                                                  info_name,
                                                  description,
                                                  &weechat_python_api_hook_info_cb,
                                                  function,
                                                  data));
    
    PYTHON_RETURN_STRING_FREE(result);
}

/*
 * weechat_python_api_hook_infolist_cb: callback for infolist hooked
 */

struct t_infolist *
weechat_python_api_hook_infolist_cb (void *data, const char *infolist_name,
                                     void *pointer, const char *arguments)
{
    struct t_script_callback *script_callback;
    char *python_argv[5], empty_arg[1] = { '\0' };
    struct t_infolist *result;
    
    script_callback = (struct t_script_callback *)data;
    
    if (script_callback && script_callback->function && script_callback->function[0])
    {
        python_argv[0] = (script_callback->data) ? script_callback->data : empty_arg;
        python_argv[1] = (infolist_name) ? (char *)infolist_name : empty_arg;
        python_argv[2] = script_ptr2str (pointer);
        python_argv[3] = (arguments) ? (char *)arguments : empty_arg;
        python_argv[4] = NULL;
        
        result = (struct t_infolist *)weechat_python_exec (script_callback->script,
                                                           WEECHAT_SCRIPT_EXEC_STRING,
                                                           script_callback->function,
                                                           python_argv);
        
        if (python_argv[2])
            free (python_argv[2]);
        
        return result;
    }
    
    return NULL;
}

/*
 * weechat_python_api_hook_infolist: hook an infolist
 */

static PyObject *
weechat_python_api_hook_infolist (PyObject *self, PyObject *args)
{
    char *infolist_name, *description, *function, *data, *result;
    PyObject *object;
    
    /* make C compiler happy */
    (void) self;
    
    if (!python_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PYTHON_CURRENT_SCRIPT_NAME, "hook_infolist");
        PYTHON_RETURN_EMPTY;
    }
    
    infolist_name = NULL;
    description = NULL;
    function = NULL;
    data = NULL;
    
    if (!PyArg_ParseTuple (args, "ssss", &infolist_name, &description,
                           &function, &data))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PYTHON_CURRENT_SCRIPT_NAME, "hook_infolist");
        PYTHON_RETURN_EMPTY;
    }
    
    result = script_ptr2str(script_api_hook_infolist (weechat_python_plugin,
                                                      python_current_script,
                                                      infolist_name,
                                                      description,
                                                      &weechat_python_api_hook_infolist_cb,
                                                      function,
                                                      data));
    
    PYTHON_RETURN_STRING_FREE(result);
}

/*
 * weechat_python_api_unhook: unhook something
 */

static PyObject *
weechat_python_api_unhook (PyObject *self, PyObject *args)
{
    char *hook;
    
    /* make C compiler happy */
    (void) self;
    
    if (!python_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PYTHON_CURRENT_SCRIPT_NAME, "unhook");
        PYTHON_RETURN_ERROR;
    }
    
    hook = NULL;
    
    if (!PyArg_ParseTuple (args, "s", &hook))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PYTHON_CURRENT_SCRIPT_NAME, "unhook");
        PYTHON_RETURN_ERROR;
    }
    
    script_api_unhook (weechat_python_plugin,
                       python_current_script,
                       script_str2ptr (hook));
    
    PYTHON_RETURN_OK;
}

/*
 * weechat_python_api_unhook_all: unhook all for script
 */

static PyObject *
weechat_python_api_unhook_all (PyObject *self, PyObject *args)
{
    /* make C compiler happy */
    (void) self;
    (void) args;
    
    if (!python_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PYTHON_CURRENT_SCRIPT_NAME, "unhook_all");
        PYTHON_RETURN_ERROR;
    }
    
    script_api_unhook_all (python_current_script);
    
    PYTHON_RETURN_OK;
}

/*
 * weechat_python_api_buffer_input_data_cb: callback for input data in a buffer
 */

int
weechat_python_api_buffer_input_data_cb (void *data, struct t_gui_buffer *buffer,
                                         const char *input_data)
{
    struct t_script_callback *script_callback;
    char *python_argv[4], empty_arg[1] = { '\0' };
    int *rc, ret;
    
    script_callback = (struct t_script_callback *)data;
    
    if (script_callback && script_callback->function && script_callback->function[0])
    {
        python_argv[0] = (script_callback->data) ? script_callback->data : empty_arg;
        python_argv[1] = script_ptr2str (buffer);
        python_argv[2] = (input_data) ? (char *)input_data : empty_arg;
        python_argv[3] = NULL;
        
        rc = (int *) weechat_python_exec (script_callback->script,
                                          WEECHAT_SCRIPT_EXEC_INT,
                                          script_callback->function,
                                          python_argv);
        if (!rc)
            ret = WEECHAT_RC_ERROR;
        else
        {
            ret = *rc;
            free (rc);
        }
        if (python_argv[1])
            free (python_argv[1]);
        
        return ret;
    }
    
    return WEECHAT_RC_ERROR;
}

/*
 * weechat_python_api_buffer_close_cb: callback for buffer closed
 */

int
weechat_python_api_buffer_close_cb (void *data, struct t_gui_buffer *buffer)
{
    struct t_script_callback *script_callback;
    char *python_argv[3], empty_arg[1] = { '\0' };
    int *rc, ret;
    
    script_callback = (struct t_script_callback *)data;
    
    if (script_callback && script_callback->function && script_callback->function[0])
    {
        python_argv[0] = (script_callback->data) ? script_callback->data : empty_arg;
        python_argv[1] = script_ptr2str (buffer);
        python_argv[2] = NULL;
        
        rc = (int *) weechat_python_exec (script_callback->script,
                                          WEECHAT_SCRIPT_EXEC_INT,
                                          script_callback->function,
                                          python_argv);
        if (!rc)
            ret = WEECHAT_RC_ERROR;
        else
        {
            ret = *rc;
            free (rc);
        }
        if (python_argv[1])
            free (python_argv[1]);
        
        return ret;
    }
    
    return WEECHAT_RC_ERROR;
}

/*
 * weechat_python_api_buffer_new: create a new buffer
 */

static PyObject *
weechat_python_api_buffer_new (PyObject *self, PyObject *args)
{
    char *name, *function_input, *data_input, *function_close, *data_close;
    char *result;
    PyObject *object;
    
    /* make C compiler happy */
    (void) self;
    
    if (!python_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PYTHON_CURRENT_SCRIPT_NAME, "buffer_new");
        PYTHON_RETURN_EMPTY;
    }
    
    name = NULL;
    function_input = NULL;
    data_input = NULL;
    function_close = NULL;
    data_close = NULL;
    
    if (!PyArg_ParseTuple (args, "sssss", &name, &function_input, &data_input,
                           &function_close, &data_close))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PYTHON_CURRENT_SCRIPT_NAME, "buffer_new");
        PYTHON_RETURN_EMPTY;
    }
    
    result = script_ptr2str (script_api_buffer_new (weechat_python_plugin,
                                                    python_current_script,
                                                    name,
                                                    &weechat_python_api_buffer_input_data_cb,
                                                    function_input,
                                                    data_input,
                                                    &weechat_python_api_buffer_close_cb,
                                                    function_close,
                                                    data_close));
    
    PYTHON_RETURN_STRING_FREE(result);
}

/*
 * weechat_python_api_buffer_search: search a buffer
 */

static PyObject *
weechat_python_api_buffer_search (PyObject *self, PyObject *args)
{
    char *plugin, *name;
    char *result;
    PyObject *object;
    
    /* make C compiler happy */
    (void) self;
    
    if (!python_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PYTHON_CURRENT_SCRIPT_NAME, "buffer_search");
        PYTHON_RETURN_EMPTY;
    }
    
    plugin = NULL;
    name = NULL;
    
    if (!PyArg_ParseTuple (args, "ss", &plugin, &name))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PYTHON_CURRENT_SCRIPT_NAME, "buffer_search");
        PYTHON_RETURN_EMPTY;
    }
    
    result = script_ptr2str (weechat_buffer_search (plugin, name));
    
    PYTHON_RETURN_STRING_FREE(result);
}

/*
 * weechat_python_api_buffer_search_main: search main buffer (WeeChat core buffer)
 */

static PyObject *
weechat_python_api_buffer_search_main (PyObject *self, PyObject *args)
{
    char *result;
    PyObject *object;
    
    /* make C compiler happy */
    (void) self;
    (void) args;
    
    if (!python_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PYTHON_CURRENT_SCRIPT_NAME, "buffer_search_main");
        PYTHON_RETURN_EMPTY;
    }
    
    result = script_ptr2str (weechat_buffer_search_main ());
    
    PYTHON_RETURN_STRING_FREE(result);
}

/*
 * weechat_python_api_current_buffer: get current buffer
 */

static PyObject *
weechat_python_api_current_buffer (PyObject *self, PyObject *args)
{
    char *result;
    PyObject *object;
    
    /* make C compiler happy */
    (void) self;
    (void) args;
    
    if (!python_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PYTHON_CURRENT_SCRIPT_NAME, "current_buffer");
        PYTHON_RETURN_EMPTY;
    }
    
    result = script_ptr2str (weechat_current_buffer ());
    
    PYTHON_RETURN_STRING_FREE(result);
}

/*
 * weechat_python_api_buffer_clear: clear a buffer
 */

static PyObject *
weechat_python_api_buffer_clear (PyObject *self, PyObject *args)
{
    char *buffer;
    
    /* make C compiler happy */
    (void) self;
    
    if (!python_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PYTHON_CURRENT_SCRIPT_NAME, "buffer_clear");
        PYTHON_RETURN_ERROR;
    }
    
    buffer = NULL;
    
    if (!PyArg_ParseTuple (args, "s", &buffer))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PYTHON_CURRENT_SCRIPT_NAME, "buffer_clear");
        PYTHON_RETURN_ERROR;
    }
    
    weechat_buffer_clear (script_str2ptr (buffer));
    
    PYTHON_RETURN_OK;
}

/*
 * weechat_python_api_buffer_close: close a buffer
 */

static PyObject *
weechat_python_api_buffer_close (PyObject *self, PyObject *args)
{
    char *buffer;
    
    /* make C compiler happy */
    (void) self;
    
    if (!python_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PYTHON_CURRENT_SCRIPT_NAME, "buffer_close");
        PYTHON_RETURN_ERROR;
    }
    
    buffer = NULL;
    
    if (!PyArg_ParseTuple (args, "s", &buffer))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PYTHON_CURRENT_SCRIPT_NAME, "buffer_close");
        PYTHON_RETURN_ERROR;
    }
    
    script_api_buffer_close (weechat_python_plugin,
                             python_current_script,
                             script_str2ptr (buffer));
    
    PYTHON_RETURN_OK;
}

/*
 * weechat_python_api_buffer_merge: merge a buffer to another buffer
 */

static PyObject *
weechat_python_api_buffer_merge (PyObject *self, PyObject *args)
{
    char *buffer, *target_buffer;
    
    /* make C compiler happy */
    (void) self;
    
    if (!python_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PYTHON_CURRENT_SCRIPT_NAME, "buffer_merge");
        PYTHON_RETURN_ERROR;
    }
    
    buffer = NULL;
    target_buffer = NULL;
    
    if (!PyArg_ParseTuple (args, "ss", &buffer, &target_buffer))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PYTHON_CURRENT_SCRIPT_NAME, "buffer_merge");
        PYTHON_RETURN_ERROR;
    }
    
    weechat_buffer_merge (script_str2ptr (buffer),
                          script_str2ptr (target_buffer));
    
    PYTHON_RETURN_OK;
}

/*
 * weechat_python_api_buffer_unmerge: unmerge a buffer from group of merged
 *                                    buffers
 */

static PyObject *
weechat_python_api_buffer_unmerge (PyObject *self, PyObject *args)
{
    char *buffer;
    int number;
    
    /* make C compiler happy */
    (void) self;
    
    if (!python_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PYTHON_CURRENT_SCRIPT_NAME, "buffer_unmerge");
        PYTHON_RETURN_ERROR;
    }
    
    buffer = NULL;
    number = 0;
    
    if (!PyArg_ParseTuple (args, "si", &buffer, &number))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PYTHON_CURRENT_SCRIPT_NAME, "buffer_unmerge");
        PYTHON_RETURN_ERROR;
    }
    
    weechat_buffer_unmerge (script_str2ptr (buffer), number);
    
    PYTHON_RETURN_OK;
}

/*
 * weechat_python_api_buffer_get_integer get a buffer property as integer
 */

static PyObject *
weechat_python_api_buffer_get_integer (PyObject *self, PyObject *args)
{
    char *buffer, *property;
    int value;
    
    /* make C compiler happy */
    (void) self;
    
    if (!python_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PYTHON_CURRENT_SCRIPT_NAME, "buffer_get_integer");
        PYTHON_RETURN_INT(-1);
    }
    
    buffer = NULL;
    property = NULL;
    
    if (!PyArg_ParseTuple (args, "ss", &buffer, &property))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PYTHON_CURRENT_SCRIPT_NAME, "buffer_get_integer");
        PYTHON_RETURN_INT(-1);
    }
    
    value = weechat_buffer_get_integer (script_str2ptr (buffer), property);
    
    PYTHON_RETURN_INT(value);
}

/*
 * weechat_python_api_buffer_get_string: get a buffer property as string
 */

static PyObject *
weechat_python_api_buffer_get_string (PyObject *self, PyObject *args)
{
    char *buffer, *property;
    const char *result;
    
    /* make C compiler happy */
    (void) self;
    
    if (!python_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PYTHON_CURRENT_SCRIPT_NAME, "buffer_get_string");
        PYTHON_RETURN_ERROR;
    }
    
    buffer = NULL;
    property = NULL;
    
    if (!PyArg_ParseTuple (args, "ss", &buffer, &property))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PYTHON_CURRENT_SCRIPT_NAME, "buffer_get_string");
        PYTHON_RETURN_EMPTY;
    }
    
    result = weechat_buffer_get_string (script_str2ptr (buffer), property);
    
    PYTHON_RETURN_STRING(result);
}

/*
 * weechat_python_api_buffer_get_pointer: get a buffer property as pointer
 */

static PyObject *
weechat_python_api_buffer_get_pointer (PyObject *self, PyObject *args)
{
    char *buffer, *property, *result;
    PyObject *object;
    
    /* make C compiler happy */
    (void) self;
    
    if (!python_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PYTHON_CURRENT_SCRIPT_NAME, "buffer_get_pointer");
        PYTHON_RETURN_EMPTY;
    }
    
    buffer = NULL;
    property = NULL;
    
    if (!PyArg_ParseTuple (args, "ss", &buffer, &property))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PYTHON_CURRENT_SCRIPT_NAME, "buffer_get_pointer");
        PYTHON_RETURN_EMPTY;
    }
    
    result = script_ptr2str (weechat_buffer_get_pointer (script_str2ptr (buffer),
                                                         property));
    
    PYTHON_RETURN_STRING_FREE(result);
}

/*
 * weechat_python_api_buffer_set: set a buffer property
 */

static PyObject *
weechat_python_api_buffer_set (PyObject *self, PyObject *args)
{
    char *buffer, *property, *value;
    
    /* make C compiler happy */
    (void) self;
    
    if (!python_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PYTHON_CURRENT_SCRIPT_NAME, "buffer_set");
        PYTHON_RETURN_ERROR;
    }
    
    buffer = NULL;
    property = NULL;
    value = NULL;
    
    if (!PyArg_ParseTuple (args, "sss", &buffer, &property, &value))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PYTHON_CURRENT_SCRIPT_NAME, "buffer_set");
        PYTHON_RETURN_ERROR;
    }
    
    weechat_buffer_set (script_str2ptr (buffer),
                        property,
                        value);
    
    PYTHON_RETURN_OK;
}

/*
 * weechat_python_api_current_window: get current window
 */

static PyObject *
weechat_python_api_current_window (PyObject *self, PyObject *args)
{
    char *result;
    PyObject *object;
    
    /* make C compiler happy */
    (void) self;
    (void) args;
    
    if (!python_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PYTHON_CURRENT_SCRIPT_NAME, "current_window");
        PYTHON_RETURN_EMPTY;
    }
    
    result = script_ptr2str (weechat_current_window ());
    
    PYTHON_RETURN_STRING_FREE(result);
}

/*
 * weechat_python_api_window_get_integer get a window property as integer
 */

static PyObject *
weechat_python_api_window_get_integer (PyObject *self, PyObject *args)
{
    char *window, *property;
    int value;
    
    /* make C compiler happy */
    (void) self;
    
    if (!python_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PYTHON_CURRENT_SCRIPT_NAME, "window_get_integer");
        PYTHON_RETURN_INT(-1);
    }
    
    window = NULL;
    property = NULL;
    
    if (!PyArg_ParseTuple (args, "ss", &window, &property))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PYTHON_CURRENT_SCRIPT_NAME, "window_get_integer");
        PYTHON_RETURN_INT(-1);
    }
    
    value = weechat_window_get_integer (script_str2ptr (window), property);
    
    PYTHON_RETURN_INT(value);
}

/*
 * weechat_python_api_window_get_string: get a window property as string
 */

static PyObject *
weechat_python_api_window_get_string (PyObject *self, PyObject *args)
{
    char *window, *property;
    const char *result;
    
    /* make C compiler happy */
    (void) self;
    
    if (!python_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PYTHON_CURRENT_SCRIPT_NAME, "window_get_string");
        PYTHON_RETURN_EMPTY;
    }
    
    window = NULL;
    property = NULL;
    
    if (!PyArg_ParseTuple (args, "ss", &window, &property))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PYTHON_CURRENT_SCRIPT_NAME, "window_get_string");
        PYTHON_RETURN_EMPTY;
    }
    
    result = weechat_window_get_string (script_str2ptr (window), property);
    
    PYTHON_RETURN_STRING(result);
}

/*
 * weechat_python_api_window_get_pointer: get a window property as pointer
 */

static PyObject *
weechat_python_api_window_get_pointer (PyObject *self, PyObject *args)
{
    char *window, *property, *result;
    PyObject *object;
    
    /* make C compiler happy */
    (void) self;
    
    if (!python_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PYTHON_CURRENT_SCRIPT_NAME, "window_get_pointer");
        PYTHON_RETURN_EMPTY;
    }
    
    window = NULL;
    property = NULL;
    
    if (!PyArg_ParseTuple (args, "ss", &window, &property))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PYTHON_CURRENT_SCRIPT_NAME, "window_get_pointer");
        PYTHON_RETURN_EMPTY;
    }
    
    result = script_ptr2str (weechat_window_get_pointer (script_str2ptr (window),
                                                         property));
    
    PYTHON_RETURN_STRING_FREE(result);
}

/*
 * weechat_python_api_window_set_title: set window title
 */

static PyObject *
weechat_python_api_window_set_title (PyObject *self, PyObject *args)
{
    char *title;
    
    /* make C compiler happy */
    (void) self;
    
    if (!python_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PYTHON_CURRENT_SCRIPT_NAME, "window_set_title");
        PYTHON_RETURN_ERROR;
    }
    
    title = NULL;
    
    if (!PyArg_ParseTuple (args, "s", &title))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PYTHON_CURRENT_SCRIPT_NAME, "window_set_title");
        PYTHON_RETURN_ERROR;
    }
    
    weechat_window_set_title (title);
    
    PYTHON_RETURN_OK;
}

/*
 * weechat_python_api_nicklist_add_group: add a group in nicklist
 */

static PyObject *
weechat_python_api_nicklist_add_group (PyObject *self, PyObject *args)
{
    char *buffer, *parent_group, *name, *color, *result;
    int visible;
    PyObject *object;
    
    /* make C compiler happy */
    (void) self;
    
    if (!python_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PYTHON_CURRENT_SCRIPT_NAME, "nicklist_add_group");
        PYTHON_RETURN_EMPTY;
    }
    
    buffer = NULL;
    parent_group = NULL;
    name = NULL;
    color = NULL;
    visible = 0;
    
    if (!PyArg_ParseTuple (args, "ssssi", &buffer, &parent_group, &name,
                           &color, &visible))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PYTHON_CURRENT_SCRIPT_NAME, "nicklist_add_group");
        PYTHON_RETURN_EMPTY;
    }
    
    result = script_ptr2str (weechat_nicklist_add_group (script_str2ptr (buffer),
                                                         script_str2ptr (parent_group),
                                                         name,
                                                         color,
                                                         visible));
    
    PYTHON_RETURN_STRING_FREE(result);
}

/*
 * weechat_python_api_nicklist_search_group: search a group in nicklist
 */

static PyObject *
weechat_python_api_nicklist_search_group (PyObject *self, PyObject *args)
{
    char *buffer, *from_group, *name, *result;
    PyObject *object;
    
    /* make C compiler happy */
    (void) self;
    
    if (!python_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PYTHON_CURRENT_SCRIPT_NAME, "nicklist_search_group");
        PYTHON_RETURN_EMPTY;
    }
    
    buffer = NULL;
    from_group = NULL;
    name = NULL;
    
    if (!PyArg_ParseTuple (args, "sss", &buffer, &from_group, &name))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PYTHON_CURRENT_SCRIPT_NAME, "nicklist_search_group");
        PYTHON_RETURN_EMPTY;
    }
    
    result = script_ptr2str (weechat_nicklist_search_group (script_str2ptr (buffer),
                                                            script_str2ptr (from_group),
                                                            name));
    
    PYTHON_RETURN_STRING_FREE(result);
}

/*
 * weechat_python_api_nicklist_add_nick: add a nick in nicklist
 */

static PyObject *
weechat_python_api_nicklist_add_nick (PyObject *self, PyObject *args)
{
    char *buffer, *group, *name, *color, *prefix, *prefix_color, *result;
    int visible;
    PyObject *object;
    
    /* make C compiler happy */
    (void) self;
    
    if (!python_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PYTHON_CURRENT_SCRIPT_NAME, "nicklist_add_nick");
        PYTHON_RETURN_EMPTY;
    }
    
    buffer = NULL;
    group = NULL;
    name = NULL;
    color = NULL;
    prefix = NULL;
    prefix_color = NULL;
    visible = 0;
    
    if (!PyArg_ParseTuple (args, "ssssssi", &buffer, &group, &name, &color,
                           &prefix, &prefix_color, &visible))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PYTHON_CURRENT_SCRIPT_NAME, "nicklist_add_nick");
        PYTHON_RETURN_EMPTY;
    }
    
    result = script_ptr2str (weechat_nicklist_add_nick (script_str2ptr (buffer),
                                                        script_str2ptr (group),
                                                        name,
                                                        color,
                                                        prefix,
                                                        prefix_color,
                                                        visible));
    
    PYTHON_RETURN_STRING_FREE(result);
}

/*
 * weechat_python_api_nicklist_search_nick: search a nick in nicklist
 */

static PyObject *
weechat_python_api_nicklist_search_nick (PyObject *self, PyObject *args)
{
    char *buffer, *from_group, *name, *result;
    PyObject *object;
    
    /* make C compiler happy */
    (void) self;
    
    if (!python_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PYTHON_CURRENT_SCRIPT_NAME, "nicklist_search_nick");
        PYTHON_RETURN_EMPTY;
    }
    
    buffer = NULL;
    from_group = NULL;
    name = NULL;
    
    if (!PyArg_ParseTuple (args, "sss", &buffer, &from_group, &name))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PYTHON_CURRENT_SCRIPT_NAME, "nicklist_search_nick");
        PYTHON_RETURN_EMPTY;
    }
    
    result = script_ptr2str (weechat_nicklist_search_nick (script_str2ptr (buffer),
                                                           script_str2ptr (from_group),
                                                           name));
    
    PYTHON_RETURN_STRING_FREE(result);
}

/*
 * weechat_python_api_nicklist_remove_group: remove a group from nicklist
 */

static PyObject *
weechat_python_api_nicklist_remove_group (PyObject *self, PyObject *args)
{
    char *buffer, *group;
    
    /* make C compiler happy */
    (void) self;
    
    if (!python_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PYTHON_CURRENT_SCRIPT_NAME, "nicklist_remove_group");
        PYTHON_RETURN_ERROR;
    }
    
    buffer = NULL;
    group = NULL;
    
    if (!PyArg_ParseTuple (args, "ss", &buffer, &group))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PYTHON_CURRENT_SCRIPT_NAME, "nicklist_remove_group");
        PYTHON_RETURN_ERROR;
    }
    
    weechat_nicklist_remove_group (script_str2ptr (buffer),
                                   script_str2ptr (group));
    
    PYTHON_RETURN_OK;
}

/*
 * weechat_python_api_nicklist_remove_nick: remove a nick from nicklist
 */

static PyObject *
weechat_python_api_nicklist_remove_nick (PyObject *self, PyObject *args)
{
    char *buffer, *nick;
    
    /* make C compiler happy */
    (void) self;
    
    if (!python_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PYTHON_CURRENT_SCRIPT_NAME, "nicklist_remove_nick");
        PYTHON_RETURN_ERROR;
    }
    
    buffer = NULL;
    nick = NULL;
    
    if (!PyArg_ParseTuple (args, "ss", &buffer, &nick))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PYTHON_CURRENT_SCRIPT_NAME, "nicklist_remove_nick");
        PYTHON_RETURN_ERROR;
    }
    
    weechat_nicklist_remove_group (script_str2ptr (buffer),
                                   script_str2ptr (nick));
    
    PYTHON_RETURN_OK;
}

/*
 * weechat_python_api_nicklist_remove_all: remove all groups/nicks from nicklist
 */

static PyObject *
weechat_python_api_nicklist_remove_all (PyObject *self, PyObject *args)
{
    char *buffer;
    
    /* make C compiler happy */
    (void) self;
    
    if (!python_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PYTHON_CURRENT_SCRIPT_NAME, "nicklist_remove_all");
        PYTHON_RETURN_ERROR;
    }
    
    buffer = NULL;
    
    if (!PyArg_ParseTuple (args, "s", &buffer))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PYTHON_CURRENT_SCRIPT_NAME, "nicklist_remove_all");
        PYTHON_RETURN_ERROR;
    }
    
    weechat_nicklist_remove_all (script_str2ptr (buffer));
    
    PYTHON_RETURN_OK;
}

/*
 * weechat_python_api_bar_item_search: search a bar item
 */

static PyObject *
weechat_python_api_bar_item_search (PyObject *self, PyObject *args)
{
    char *name, *result;
    PyObject *object;
    
    /* make C compiler happy */
    (void) self;
    
    if (!python_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PYTHON_CURRENT_SCRIPT_NAME, "bar_item_search");
        PYTHON_RETURN_EMPTY;
    }
    
    name = NULL;
    
    if (!PyArg_ParseTuple (args, "s", &name))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PYTHON_CURRENT_SCRIPT_NAME, "bar_item_search");
        PYTHON_RETURN_EMPTY;
    }
    
    result = script_ptr2str (weechat_bar_item_search (name));
    
    PYTHON_RETURN_STRING_FREE(result);
}

/*
 * weechat_python_api_bar_item_build_cb: callback for building bar item
 */

char *
weechat_python_api_bar_item_build_cb (void *data, struct t_gui_bar_item *item,
                                      struct t_gui_window *window)
{
    struct t_script_callback *script_callback;
    char *python_argv[4], empty_arg[1] = { '\0' }, *ret;
    
    script_callback = (struct t_script_callback *)data;

    if (script_callback && script_callback->function && script_callback->function[0])
    {
        python_argv[0] = (script_callback->data) ? script_callback->data : empty_arg;
        python_argv[1] = script_ptr2str (item);
        python_argv[2] = script_ptr2str (window);
        python_argv[3] = NULL;
        
        ret = (char *)weechat_python_exec (script_callback->script,
                                           WEECHAT_SCRIPT_EXEC_STRING,
                                           script_callback->function,
                                           python_argv);
        
        if (python_argv[1])
            free (python_argv[1]);
        if (python_argv[2])
            free (python_argv[2]);
        
        return ret;
    }
    
    return NULL;
}

/*
 * weechat_python_api_bar_item_new: add a new bar item
 */

static PyObject *
weechat_python_api_bar_item_new (PyObject *self, PyObject *args)
{
    char *name, *function, *data, *result;
    PyObject *object;
    
    /* make C compiler happy */
    (void) self;
    
    if (!python_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PYTHON_CURRENT_SCRIPT_NAME, "bar_item_new");
        PYTHON_RETURN_EMPTY;
    }
    
    name = NULL;
    function = NULL;
    data = NULL;
    
    if (!PyArg_ParseTuple (args, "sss", &name, &function, &data))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PYTHON_CURRENT_SCRIPT_NAME, "bar_item_new");
        PYTHON_RETURN_EMPTY;
    }
    
    result = script_ptr2str (script_api_bar_item_new (weechat_python_plugin,
                                                      python_current_script,
                                                      name,
                                                      &weechat_python_api_bar_item_build_cb,
                                                      function,
                                                      data));
    
    PYTHON_RETURN_STRING_FREE(result);
}

/*
 * weechat_python_api_bar_item_update: update a bar item on screen
 */

static PyObject *
weechat_python_api_bar_item_update (PyObject *self, PyObject *args)
{
    char *name;
    
    /* make C compiler happy */
    (void) self;
    
    if (!python_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PYTHON_CURRENT_SCRIPT_NAME, "bar_item_update");
        PYTHON_RETURN_ERROR;
    }
    
    name = NULL;
    
    if (!PyArg_ParseTuple (args, "s", &name))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PYTHON_CURRENT_SCRIPT_NAME, "bar_item_update");
        PYTHON_RETURN_ERROR;
    }
    
    weechat_bar_item_update (name);
    
    PYTHON_RETURN_OK;
}

/*
 * weechat_python_api_bar_item_remove: remove a bar item
 */

static PyObject *
weechat_python_api_bar_item_remove (PyObject *self, PyObject *args)
{
    char *item;
    
    /* make C compiler happy */
    (void) self;
    
    if (!python_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PYTHON_CURRENT_SCRIPT_NAME, "bar_item_remove");
        PYTHON_RETURN_ERROR;
    }
    
    item = NULL;
    
    if (!PyArg_ParseTuple (args, "s", &item))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PYTHON_CURRENT_SCRIPT_NAME, "bar_item_remove");
        PYTHON_RETURN_ERROR;
    }
    
    script_api_bar_item_remove (weechat_python_plugin,
                                python_current_script,
                                script_str2ptr (item));
    
    PYTHON_RETURN_OK;
}

/*
 * weechat_python_api_bar_search: search a bar
 */

static PyObject *
weechat_python_api_bar_search (PyObject *self, PyObject *args)
{
    char *name, *result;
    PyObject *object;
    
    /* make C compiler happy */
    (void) self;
    
    if (!python_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PYTHON_CURRENT_SCRIPT_NAME, "bar_search");
        PYTHON_RETURN_EMPTY;
    }
    
    name = NULL;
    
    if (!PyArg_ParseTuple (args, "s", &name))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PYTHON_CURRENT_SCRIPT_NAME, "bar_search");
        PYTHON_RETURN_EMPTY;
    }
    
    result = script_ptr2str (weechat_bar_search (name));
    
    PYTHON_RETURN_STRING_FREE(result);
}

/*
 * weechat_python_api_bar_new: add a new bar
 */

static PyObject *
weechat_python_api_bar_new (PyObject *self, PyObject *args)
{
    char *name, *hidden, *priority, *type, *conditions, *position;
    char *filling_top_bottom, *filling_left_right, *size, *size_max;
    char *color_fg, *color_delim, *color_bg, *separator, *items, *result;
    PyObject *object;
    
    /* make C compiler happy */
    (void) self;
    
    if (!python_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PYTHON_CURRENT_SCRIPT_NAME, "bar_new");
        PYTHON_RETURN_EMPTY;
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
    
    if (!PyArg_ParseTuple (args, "sssssssssssssss", &name, &hidden, &priority,
                           &type, &conditions, &position, &filling_top_bottom,
                           &filling_left_right, &size, &size_max, &color_fg,
                           &color_delim, &color_bg, &separator, &items))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PYTHON_CURRENT_SCRIPT_NAME, "bar_new");
        PYTHON_RETURN_EMPTY;
    }
    
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
    
    PYTHON_RETURN_STRING_FREE(result);
}

/*
 * weechat_python_api_bar_set: set a bar property
 */

static PyObject *
weechat_python_api_bar_set (PyObject *self, PyObject *args)
{
    char *bar, *property, *value;
    
    /* make C compiler happy */
    (void) self;
    
    if (!python_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PYTHON_CURRENT_SCRIPT_NAME, "bar_set");
        PYTHON_RETURN_ERROR;
    }
    
    bar = NULL;
    property = NULL;
    value = NULL;
    
    if (!PyArg_ParseTuple (args, "sss", &bar, &property, &value))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PYTHON_CURRENT_SCRIPT_NAME, "bar_set");
        PYTHON_RETURN_ERROR;
    }
    
    weechat_bar_set (script_str2ptr (bar),
                     property,
                     value);
    
    PYTHON_RETURN_OK;
}

/*
 * weechat_python_api_bar_update: update a bar on screen
 */

static PyObject *
weechat_python_api_bar_update (PyObject *self, PyObject *args)
{
    char *name;
    
    /* make C compiler happy */
    (void) self;
    
    if (!python_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PYTHON_CURRENT_SCRIPT_NAME, "bar_item");
        PYTHON_RETURN_ERROR;
    }
    
    name = NULL;
    
    if (!PyArg_ParseTuple (args, "s", &name))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PYTHON_CURRENT_SCRIPT_NAME, "bar_item");
        PYTHON_RETURN_ERROR;
    }
    
    weechat_bar_update (name);
    
    PYTHON_RETURN_OK;
}

/*
 * weechat_python_api_bar_remove: remove a bar
 */

static PyObject *
weechat_python_api_bar_remove (PyObject *self, PyObject *args)
{
    char *bar;
    
    /* make C compiler happy */
    (void) self;
    
    if (!python_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PYTHON_CURRENT_SCRIPT_NAME, "bar_remove");
        PYTHON_RETURN_ERROR;
    }
    
    bar = NULL;
    
    if (!PyArg_ParseTuple (args, "s", &bar))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PYTHON_CURRENT_SCRIPT_NAME, "bar_remove");
        PYTHON_RETURN_ERROR;
    }
    
    weechat_bar_remove (script_str2ptr (bar));
    
    PYTHON_RETURN_OK;
}

/*
 * weechat_python_api_command: send command to server
 */

static PyObject *
weechat_python_api_command (PyObject *self, PyObject *args)
{
    char *buffer, *command;
    
    /* make C compiler happy */
    (void) self;
    
    if (!python_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PYTHON_CURRENT_SCRIPT_NAME, "command");
        PYTHON_RETURN_ERROR;
    }
    
    buffer = NULL;
    command = NULL;
    
    if (!PyArg_ParseTuple (args, "ss", &buffer, &command))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PYTHON_CURRENT_SCRIPT_NAME, "command");
        PYTHON_RETURN_ERROR;
    }
    
    script_api_command (weechat_python_plugin,
                        python_current_script,
                        script_str2ptr (buffer),
                        command);
    
    PYTHON_RETURN_OK;
}

/*
 * weechat_python_api_info_get: get info about WeeChat
 */

static PyObject *
weechat_python_api_info_get (PyObject *self, PyObject *args)
{
    char *info_name, *arguments;
    const char *result;
    
    /* make C compiler happy */
    (void) self;
    
    if (!python_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PYTHON_CURRENT_SCRIPT_NAME, "info_get");
        PYTHON_RETURN_EMPTY;
    }
    
    info_name = NULL;
    
    if (!PyArg_ParseTuple (args, "ss", &info_name, &arguments))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PYTHON_CURRENT_SCRIPT_NAME, "info_get");
        PYTHON_RETURN_EMPTY;
    }
    
    result = weechat_info_get (info_name, arguments);
    
    PYTHON_RETURN_STRING(result);
}

/*
 * weechat_python_api_infolist_new: create new infolist
 */

static PyObject *
weechat_python_api_infolist_new (PyObject *self, PyObject *args)
{
    char *result;
    PyObject *object;
    
    /* make C compiler happy */
    (void) self;
    (void) args;
    
    if (!python_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PYTHON_CURRENT_SCRIPT_NAME, "infolist_new");
        PYTHON_RETURN_EMPTY;
    }
    
    result = script_ptr2str (weechat_infolist_new ());
    
    PYTHON_RETURN_STRING_FREE(result);
}

/*
 * weechat_python_api_infolist_new_var_integer: create new integer variable in
 *                                              infolist
 */

static PyObject *
weechat_python_api_infolist_new_var_integer (PyObject *self, PyObject *args)
{
    char *infolist, *name, *result;
    int value;
    PyObject *object;
    
    /* make C compiler happy */
    (void) self;
    
    if (!python_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PYTHON_CURRENT_SCRIPT_NAME, "infolist_new_var_integer");
        PYTHON_RETURN_EMPTY;
    }
    
    infolist = NULL;
    name = NULL;
    value = 0;
    
    if (!PyArg_ParseTuple (args, "ssi", &infolist, &name, &value))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PYTHON_CURRENT_SCRIPT_NAME, "infolist_new_var_integer");
        PYTHON_RETURN_EMPTY;
    }
    
    result = script_ptr2str (weechat_infolist_new_var_integer (script_str2ptr (infolist),
                                                               name,
                                                               value));
    
    PYTHON_RETURN_STRING_FREE(result);
}

/*
 * weechat_python_api_infolist_new_var_string: create new string variable in
 *                                             infolist
 */

static PyObject *
weechat_python_api_infolist_new_var_string (PyObject *self, PyObject *args)
{
    char *infolist, *name, *value, *result;
    PyObject *object;
    
    /* make C compiler happy */
    (void) self;
    
    if (!python_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PYTHON_CURRENT_SCRIPT_NAME, "infolist_new_var_string");
        PYTHON_RETURN_EMPTY;
    }
    
    infolist = NULL;
    name = NULL;
    value = NULL;
    
    if (!PyArg_ParseTuple (args, "sss", &infolist, &name, &value))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PYTHON_CURRENT_SCRIPT_NAME, "infolist_new_var_string");
        PYTHON_RETURN_EMPTY;
    }
    
    result = script_ptr2str (weechat_infolist_new_var_string (script_str2ptr (infolist),
                                                              name,
                                                              value));
    
    PYTHON_RETURN_STRING_FREE(result);
}

/*
 * weechat_python_api_infolist_new_var_pointer: create new pointer variable in
 *                                              infolist
 */

static PyObject *
weechat_python_api_infolist_new_var_pointer (PyObject *self, PyObject *args)
{
    char *infolist, *name, *value, *result;
    PyObject *object;
    
    /* make C compiler happy */
    (void) self;
    
    if (!python_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PYTHON_CURRENT_SCRIPT_NAME, "infolist_new_var_pointer");
        PYTHON_RETURN_EMPTY;
    }
    
    infolist = NULL;
    name = NULL;
    value = NULL;
    
    if (!PyArg_ParseTuple (args, "sss", &infolist, &name, &value))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PYTHON_CURRENT_SCRIPT_NAME, "infolist_new_var_pointer");
        PYTHON_RETURN_EMPTY;
    }
    
    result = script_ptr2str (weechat_infolist_new_var_pointer (script_str2ptr (infolist),
                                                               name,
                                                               script_str2ptr (value)));
    
    PYTHON_RETURN_STRING_FREE(result);
}

/*
 * weechat_python_api_infolist_new_var_time: create new time variable in
 *                                           infolist
 */

static PyObject *
weechat_python_api_infolist_new_var_time (PyObject *self, PyObject *args)
{
    char *infolist, *name, *result;
    int value;
    PyObject *object;
    
    /* make C compiler happy */
    (void) self;
    
    if (!python_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PYTHON_CURRENT_SCRIPT_NAME, "infolist_new_var_time");
        PYTHON_RETURN_EMPTY;
    }
    
    infolist = NULL;
    name = NULL;
    value = 0;
    
    if (!PyArg_ParseTuple (args, "ssi", &infolist, &name, &value))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PYTHON_CURRENT_SCRIPT_NAME, "infolist_new_var_time");
        PYTHON_RETURN_EMPTY;
    }
    
    result = script_ptr2str (weechat_infolist_new_var_time (script_str2ptr (infolist),
                                                            name,
                                                            value));
    
    PYTHON_RETURN_STRING_FREE(result);
}

/*
 * weechat_python_api_infolist_get: get list with infos
 */

static PyObject *
weechat_python_api_infolist_get (PyObject *self, PyObject *args)
{
    char *name, *pointer, *arguments, *result;
    PyObject *object;
    
    /* make C compiler happy */
    (void) self;
    
    if (!python_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PYTHON_CURRENT_SCRIPT_NAME, "infolist_get");
        PYTHON_RETURN_EMPTY;
    }
    
    name = NULL;
    pointer = NULL;
    arguments = NULL;
    
    if (!PyArg_ParseTuple (args, "sss", &name, &pointer, &arguments))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PYTHON_CURRENT_SCRIPT_NAME, "infolist_get");
        PYTHON_RETURN_EMPTY;
    }
    
    result = script_ptr2str (weechat_infolist_get (name,
                                                   script_str2ptr (pointer),
                                                   arguments));
    
    PYTHON_RETURN_STRING_FREE(result);
}

/*
 * weechat_python_api_infolist_next: move item pointer to next item in infolist
 */

static PyObject *
weechat_python_api_infolist_next (PyObject *self, PyObject *args)
{
    char *infolist;
    int value;
    
    /* make C compiler happy */
    (void) self;
    
    if (!python_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PYTHON_CURRENT_SCRIPT_NAME, "infolist_next");
        PYTHON_RETURN_INT(0);
    }
    
    infolist = NULL;
    
    if (!PyArg_ParseTuple (args, "s", &infolist))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PYTHON_CURRENT_SCRIPT_NAME, "infolist_next");
        PYTHON_RETURN_INT(0);
    }
    
    value = weechat_infolist_next (script_str2ptr (infolist));
    
    PYTHON_RETURN_INT(value);
}

/*
 * weechat_python_api_infolist_prev: move item pointer to previous item in infolist
 */

static PyObject *
weechat_python_api_infolist_prev (PyObject *self, PyObject *args)
{
    char *infolist;
    int value;
    
    /* make C compiler happy */
    (void) self;
    
    if (!python_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PYTHON_CURRENT_SCRIPT_NAME, "infolist_prev");
        PYTHON_RETURN_INT(0);
    }
    
    infolist = NULL;
    
    if (!PyArg_ParseTuple (args, "s", &infolist))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PYTHON_CURRENT_SCRIPT_NAME, "infolist_prev");
        PYTHON_RETURN_INT(0);
    }
    
    value = weechat_infolist_prev (script_str2ptr (infolist));
    
    PYTHON_RETURN_INT(value);
}

/*
 * weechat_python_api_infolist_fields: get list of fields for current item of infolist
 */

static PyObject *
weechat_python_api_infolist_fields (PyObject *self, PyObject *args)
{
    char *infolist;
    const char *result;
    
    /* make C compiler happy */
    (void) self;
    
    if (!python_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PYTHON_CURRENT_SCRIPT_NAME, "infolist_fields");
        PYTHON_RETURN_EMPTY;
    }
    
    infolist = NULL;
    
    if (!PyArg_ParseTuple (args, "s", &infolist))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PYTHON_CURRENT_SCRIPT_NAME, "infolist_fields");
        PYTHON_RETURN_EMPTY;
    }
    
    result = weechat_infolist_fields (script_str2ptr (infolist));
    
    PYTHON_RETURN_STRING(result);
}

/*
 * weechat_python_api_infolist_integer: get integer value of a variable in infolist
 */

static PyObject *
weechat_python_api_infolist_integer (PyObject *self, PyObject *args)
{
    char *infolist, *variable;
    int value;
    
    /* make C compiler happy */
    (void) self;
    
    if (!python_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PYTHON_CURRENT_SCRIPT_NAME, "infolist_integer");
        PYTHON_RETURN_INT(0);
    }
    
    infolist = NULL;
    variable = NULL;
    
    if (!PyArg_ParseTuple (args, "ss", &infolist, &variable))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PYTHON_CURRENT_SCRIPT_NAME, "infolist_integer");
        PYTHON_RETURN_INT(0);
    }
    
    value = weechat_infolist_integer (script_str2ptr (infolist),
                                      variable);
    
    PYTHON_RETURN_INT(value);
}

/*
 * weechat_python_api_infolist_string: get string value of a variable in infolist
 */

static PyObject *
weechat_python_api_infolist_string (PyObject *self, PyObject *args)
{
    char *infolist, *variable;
    const char *result;
    
    /* make C compiler happy */
    (void) self;
    
    if (!python_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PYTHON_CURRENT_SCRIPT_NAME, "infolist_string");
        PYTHON_RETURN_EMPTY;
    }
    
    infolist = NULL;
    variable = NULL;
    
    if (!PyArg_ParseTuple (args, "ss", &infolist, &variable))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PYTHON_CURRENT_SCRIPT_NAME, "infolist_string");
        PYTHON_RETURN_EMPTY;
    }
    
    result = weechat_infolist_string (script_str2ptr (infolist),
                                      variable);
    
    PYTHON_RETURN_STRING(result);
}

/*
 * weechat_python_api_infolist_pointer: get pointer value of a variable in infolist
 */

static PyObject *
weechat_python_api_infolist_pointer (PyObject *self, PyObject *args)
{
    char *infolist, *variable, *result;
    PyObject *object;
    
    /* make C compiler happy */
    (void) self;
    
    if (!python_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PYTHON_CURRENT_SCRIPT_NAME, "infolist_pointer");
        PYTHON_RETURN_EMPTY;
    }
    
    infolist = NULL;
    variable = NULL;
    
    if (!PyArg_ParseTuple (args, "ss", &infolist, &variable))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PYTHON_CURRENT_SCRIPT_NAME, "infolist_pointer");
        PYTHON_RETURN_EMPTY;
    }
    
    result = script_ptr2str (weechat_infolist_pointer (script_str2ptr (infolist),
                                                       variable));
    
    PYTHON_RETURN_STRING_FREE(result);
}

/*
 * weechat_python_api_infolist_time: get time value of a variable in infolist
 */

static PyObject *
weechat_python_api_infolist_time (PyObject *self, PyObject *args)
{
    char *infolist, *variable, timebuffer[64], *result;
    time_t time;
    PyObject *object;
    
    /* make C compiler happy */
    (void) self;
    
    if (!python_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PYTHON_CURRENT_SCRIPT_NAME, "infolist_time");
        PYTHON_RETURN_EMPTY;
    }
    
    infolist = NULL;
    variable = NULL;
    
    if (!PyArg_ParseTuple (args, "ss", &infolist, &variable))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PYTHON_CURRENT_SCRIPT_NAME, "infolist_time");
        PYTHON_RETURN_EMPTY;
    }
    
    time = weechat_infolist_time (script_str2ptr (infolist),
                                  variable);
    strftime (timebuffer, sizeof (timebuffer), "%F %T", localtime (&time));
    result = strdup (timebuffer);
    
    PYTHON_RETURN_STRING_FREE(result);
}

/*
 * weechat_python_api_infolist_free: free infolist
 */

static PyObject *
weechat_python_api_infolist_free (PyObject *self, PyObject *args)
{
    char *infolist;
    
    /* make C compiler happy */
    (void) self;
    
    if (!python_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PYTHON_CURRENT_SCRIPT_NAME, "infolist_free");
        PYTHON_RETURN_ERROR;
    }
    
    infolist = NULL;
    
    if (!PyArg_ParseTuple (args, "s", &infolist))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PYTHON_CURRENT_SCRIPT_NAME, "infolist_free");
        PYTHON_RETURN_ERROR;
    }
    
    weechat_infolist_free (script_str2ptr (infolist));
    
    PYTHON_RETURN_OK;
}

/*
 * weechat_python_api_upgrade_new: create an upgrade file
 */

static PyObject *
weechat_python_api_upgrade_new (PyObject *self, PyObject *args)
{
    char *filename, *result;
    int write;
    PyObject *object;
    
    /* make C compiler happy */
    (void) self;
    
    if (!python_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PYTHON_CURRENT_SCRIPT_NAME, "upgrade_new");
        PYTHON_RETURN_EMPTY;
    }
    
    filename = NULL;
    write = 0;
    
    if (!PyArg_ParseTuple (args, "si", &filename, &write))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PYTHON_CURRENT_SCRIPT_NAME, "upgrade_new");
        PYTHON_RETURN_EMPTY;
    }
    
    result = script_ptr2str (weechat_upgrade_new (filename, write));
    
    PYTHON_RETURN_STRING_FREE(result);
}

/*
 * weechat_python_api_upgrade_write_object: write object in upgrade file
 */

static PyObject *
weechat_python_api_upgrade_write_object (PyObject *self, PyObject *args)
{
    char *upgrade_file, *infolist;
    int object_id, rc;
    
    /* make C compiler happy */
    (void) self;
    
    if (!python_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PYTHON_CURRENT_SCRIPT_NAME, "upgrade_write_object");
        PYTHON_RETURN_INT(0);
    }
    
    upgrade_file = NULL;
    object_id = 0;
    infolist = NULL;
    
    if (!PyArg_ParseTuple (args, "sis", &upgrade_file, &object_id, &infolist))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PYTHON_CURRENT_SCRIPT_NAME, "upgrade_write_object");
        PYTHON_RETURN_INT(0);
    }
    
    rc = weechat_upgrade_write_object (script_str2ptr (upgrade_file),
                                       object_id,
                                       script_str2ptr (infolist));
    
    PYTHON_RETURN_INT(rc);
}

/*
 * weechat_python_api_upgrade_read_cb: callback for reading object in upgrade file
 */

int
weechat_python_api_upgrade_read_cb (void *data,
                                    struct t_upgrade_file *upgrade_file,
                                    int object_id,
                                    struct t_infolist *infolist)
{
    struct t_script_callback *script_callback;
    char *python_argv[5], empty_arg[1] = { '\0' }, str_object_id[32];
    int *rc, ret;
    
    script_callback = (struct t_script_callback *)data;
    
    if (script_callback && script_callback->function && script_callback->function[0])
    {
        snprintf (str_object_id, sizeof (str_object_id), "%d", object_id);
        
        python_argv[0] = (script_callback->data) ? script_callback->data : empty_arg;
        python_argv[1] = script_ptr2str (upgrade_file);
        python_argv[2] = str_object_id;
        python_argv[3] = script_ptr2str (infolist);
        python_argv[4] = NULL;
        
        rc = (int *) weechat_python_exec (script_callback->script,
                                          WEECHAT_SCRIPT_EXEC_INT,
                                          script_callback->function,
                                          python_argv);
        
        if (!rc)
            ret = WEECHAT_RC_ERROR;
        else
        {
            ret = *rc;
            free (rc);
        }
        if (python_argv[1])
            free (python_argv[1]);
        if (python_argv[3])
            free (python_argv[3]);
        
        return ret;
    }
    
    return WEECHAT_RC_ERROR;
}

/*
 * weechat_python_api_upgrade_read: read upgrade file
 */

static PyObject *
weechat_python_api_upgrade_read (PyObject *self, PyObject *args)
{
    char *upgrade_file, *function, *data;
    int rc;
    
    /* make C compiler happy */
    (void) self;
    
    if (!python_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PYTHON_CURRENT_SCRIPT_NAME, "upgrade_read");
        PYTHON_RETURN_INT(0);
    }
    
    upgrade_file = NULL;
    function = NULL;
    data = NULL;
    
    if (!PyArg_ParseTuple (args, "sss", &upgrade_file, &function, &data))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PYTHON_CURRENT_SCRIPT_NAME, "upgrade_read");
        PYTHON_RETURN_INT(0);
    }
    
    rc = script_api_upgrade_read (weechat_python_plugin,
                                  python_current_script,
                                  script_str2ptr (upgrade_file),
                                  &weechat_python_api_upgrade_read_cb,
                                  function,
                                  data);
    
    PYTHON_RETURN_INT(rc);
}

/*
 * weechat_python_api_upgrade_close: close upgrade file
 */

static PyObject *
weechat_python_api_upgrade_close (PyObject *self, PyObject *args)
{
    char *upgrade_file;
    
    /* make C compiler happy */
    (void) self;
    
    if (!python_current_script)
    {
        WEECHAT_SCRIPT_MSG_NOT_INIT(PYTHON_CURRENT_SCRIPT_NAME, "upgrade_close");
        PYTHON_RETURN_ERROR;
    }
    
    upgrade_file = NULL;
    
    if (!PyArg_ParseTuple (args, "s", &upgrade_file))
    {
        WEECHAT_SCRIPT_MSG_WRONG_ARGS(PYTHON_CURRENT_SCRIPT_NAME, "upgrade_close");
        PYTHON_RETURN_ERROR;
    }
    
    weechat_upgrade_close (script_str2ptr (upgrade_file));
    
    PYTHON_RETURN_OK;
}

/*
 * Python subroutines
 */

PyMethodDef weechat_python_funcs[] =
{
    { "register", &weechat_python_api_register, METH_VARARGS, "" },
    { "plugin_get_name", &weechat_python_api_plugin_get_name, METH_VARARGS, "" },
    { "charset_set", &weechat_python_api_charset_set, METH_VARARGS, "" },
    { "iconv_to_internal", &weechat_python_api_iconv_to_internal, METH_VARARGS, "" },
    { "iconv_from_internal", &weechat_python_api_iconv_from_internal, METH_VARARGS, "" },
    { "gettext", &weechat_python_api_gettext, METH_VARARGS, "" },
    { "ngettext", &weechat_python_api_ngettext, METH_VARARGS, "" },
    { "string_remove_color", &weechat_python_api_string_remove_color, METH_VARARGS, "" },
    { "mkdir_home", &weechat_python_api_mkdir_home, METH_VARARGS, "" },
    { "mkdir", &weechat_python_api_mkdir, METH_VARARGS, "" },
    { "mkdir_parents", &weechat_python_api_mkdir_parents, METH_VARARGS, "" },
    { "list_new", &weechat_python_api_list_new, METH_VARARGS, "" },
    { "list_add", &weechat_python_api_list_add, METH_VARARGS, "" },
    { "list_search", &weechat_python_api_list_search, METH_VARARGS, "" },
    { "list_casesearch", &weechat_python_api_list_casesearch, METH_VARARGS, "" },
    { "list_get", &weechat_python_api_list_get, METH_VARARGS, "" },
    { "list_set", &weechat_python_api_list_set, METH_VARARGS, "" },
    { "list_next", &weechat_python_api_list_next, METH_VARARGS, "" },
    { "list_prev", &weechat_python_api_list_prev, METH_VARARGS, "" },
    { "list_string", &weechat_python_api_list_string, METH_VARARGS, "" },
    { "list_size", &weechat_python_api_list_size, METH_VARARGS, "" },
    { "list_remove", &weechat_python_api_list_remove, METH_VARARGS, "" },
    { "list_remove_all", &weechat_python_api_list_remove_all, METH_VARARGS, "" },
    { "list_free", &weechat_python_api_list_free, METH_VARARGS, "" },
    { "config_new", &weechat_python_api_config_new, METH_VARARGS, "" },
    { "config_new_section", &weechat_python_api_config_new_section, METH_VARARGS, "" },
    { "config_search_section", &weechat_python_api_config_search_section, METH_VARARGS, "" },
    { "config_new_option", &weechat_python_api_config_new_option, METH_VARARGS, "" },
    { "config_search_option", &weechat_python_api_config_search_option, METH_VARARGS, "" },
    { "config_string_to_boolean", &weechat_python_api_config_string_to_boolean, METH_VARARGS, "" },
    { "config_option_reset", &weechat_python_api_config_option_reset, METH_VARARGS, "" },
    { "config_option_set", &weechat_python_api_config_option_set, METH_VARARGS, "" },
    { "config_option_set_null", &weechat_python_api_config_option_set_null, METH_VARARGS, "" },
    { "config_option_unset", &weechat_python_api_config_option_unset, METH_VARARGS, "" },
    { "config_option_rename", &weechat_python_api_config_option_rename, METH_VARARGS, "" },
    { "config_option_is_null", &weechat_python_api_config_option_is_null, METH_VARARGS, "" },
    { "config_option_default_is_null", &weechat_python_api_config_option_default_is_null, METH_VARARGS, "" },
    { "config_boolean", &weechat_python_api_config_boolean, METH_VARARGS, "" },
    { "config_boolean_default", &weechat_python_api_config_boolean_default, METH_VARARGS, "" },
    { "config_integer", &weechat_python_api_config_integer, METH_VARARGS, "" },
    { "config_integer_default", &weechat_python_api_config_integer_default, METH_VARARGS, "" },
    { "config_string", &weechat_python_api_config_string, METH_VARARGS, "" },
    { "config_string_default", &weechat_python_api_config_string_default, METH_VARARGS, "" },
    { "config_color", &weechat_python_api_config_color, METH_VARARGS, "" },
    { "config_color_default", &weechat_python_api_config_color_default, METH_VARARGS, "" },
    { "config_write_option", &weechat_python_api_config_write_option, METH_VARARGS, "" },
    { "config_write_line", &weechat_python_api_config_write_line, METH_VARARGS, "" },
    { "config_write", &weechat_python_api_config_write, METH_VARARGS, "" },
    { "config_read", &weechat_python_api_config_read, METH_VARARGS, "" },
    { "config_reload", &weechat_python_api_config_reload, METH_VARARGS, "" },
    { "config_option_free", &weechat_python_api_config_option_free, METH_VARARGS, "" },
    { "config_section_free_options", &weechat_python_api_config_section_free_options, METH_VARARGS, "" },
    { "config_section_free", &weechat_python_api_config_section_free, METH_VARARGS, "" },
    { "config_free", &weechat_python_api_config_free, METH_VARARGS, "" },
    { "config_get", &weechat_python_api_config_get, METH_VARARGS, "" },
    { "config_get_plugin", &weechat_python_api_config_get_plugin, METH_VARARGS, "" },
    { "config_is_set_plugin", &weechat_python_api_config_is_set_plugin, METH_VARARGS, "" },
    { "config_set_plugin", &weechat_python_api_config_set_plugin, METH_VARARGS, "" },
    { "config_unset_plugin", &weechat_python_api_config_unset_plugin, METH_VARARGS, "" },
    { "prefix", &weechat_python_api_prefix, METH_VARARGS, "" },
    { "color", &weechat_python_api_color, METH_VARARGS, "" },
    { "prnt", &weechat_python_api_prnt, METH_VARARGS, "" },
    { "prnt_date_tags", &weechat_python_api_prnt_date_tags, METH_VARARGS, "" },
    { "prnt_y", &weechat_python_api_prnt_y, METH_VARARGS, "" },
    { "log_print", &weechat_python_api_log_print, METH_VARARGS, "" },
    { "hook_command", &weechat_python_api_hook_command, METH_VARARGS, "" },
    { "hook_command_run", &weechat_python_api_hook_command_run, METH_VARARGS, "" },
    { "hook_timer", &weechat_python_api_hook_timer, METH_VARARGS, "" },
    { "hook_fd", &weechat_python_api_hook_fd, METH_VARARGS, "" },
    { "hook_process", &weechat_python_api_hook_process, METH_VARARGS, "" },
    { "hook_connect", &weechat_python_api_hook_connect, METH_VARARGS, "" },
    { "hook_print", &weechat_python_api_hook_print, METH_VARARGS, "" },
    { "hook_signal", &weechat_python_api_hook_signal, METH_VARARGS, "" },
    { "hook_signal_send", &weechat_python_api_hook_signal_send, METH_VARARGS, "" },
    { "hook_config", &weechat_python_api_hook_config, METH_VARARGS, "" },
    { "hook_completion", &weechat_python_api_hook_completion, METH_VARARGS, "" },
    { "hook_completion_list_add", &weechat_python_api_hook_completion_list_add, METH_VARARGS, "" },
    { "hook_modifier", &weechat_python_api_hook_modifier, METH_VARARGS, "" },
    { "hook_modifier_exec", &weechat_python_api_hook_modifier_exec, METH_VARARGS, "" },
    { "hook_info", &weechat_python_api_hook_info, METH_VARARGS, "" },
    { "hook_infolist", &weechat_python_api_hook_infolist, METH_VARARGS, "" },
    { "unhook", &weechat_python_api_unhook, METH_VARARGS, "" },
    { "unhook_all", &weechat_python_api_unhook_all, METH_VARARGS, "" },
    { "buffer_new", &weechat_python_api_buffer_new, METH_VARARGS, "" },
    { "buffer_search", &weechat_python_api_buffer_search, METH_VARARGS, "" },
    { "buffer_search_main", &weechat_python_api_buffer_search_main, METH_VARARGS, "" },
    { "current_buffer", &weechat_python_api_current_buffer, METH_VARARGS, "" },
    { "buffer_clear", &weechat_python_api_buffer_clear, METH_VARARGS, "" },
    { "buffer_close", &weechat_python_api_buffer_close, METH_VARARGS, "" },
    { "buffer_merge", &weechat_python_api_buffer_merge, METH_VARARGS, "" },
    { "buffer_unmerge", &weechat_python_api_buffer_unmerge, METH_VARARGS, "" },
    { "buffer_get_integer", &weechat_python_api_buffer_get_integer, METH_VARARGS, "" },
    { "buffer_get_string", &weechat_python_api_buffer_get_string, METH_VARARGS, "" },
    { "buffer_get_pointer", &weechat_python_api_buffer_get_pointer, METH_VARARGS, "" },
    { "buffer_set", &weechat_python_api_buffer_set, METH_VARARGS, "" },
    { "current_window", &weechat_python_api_current_window, METH_VARARGS, "" },
    { "window_get_integer", &weechat_python_api_window_get_integer, METH_VARARGS, "" },
    { "window_get_string", &weechat_python_api_window_get_string, METH_VARARGS, "" },
    { "window_get_pointer", &weechat_python_api_window_get_pointer, METH_VARARGS, "" },
    { "window_set_title", &weechat_python_api_window_set_title, METH_VARARGS, "" },
    { "nicklist_add_group", &weechat_python_api_nicklist_add_group, METH_VARARGS, "" },
    { "nicklist_search_group", &weechat_python_api_nicklist_search_group, METH_VARARGS, "" },
    { "nicklist_add_nick", &weechat_python_api_nicklist_add_nick, METH_VARARGS, "" },
    { "nicklist_search_nick", &weechat_python_api_nicklist_search_nick, METH_VARARGS, "" },
    { "nicklist_remove_group", &weechat_python_api_nicklist_remove_group, METH_VARARGS, "" },
    { "nicklist_remove_nick", &weechat_python_api_nicklist_remove_nick, METH_VARARGS, "" },
    { "nicklist_remove_all", &weechat_python_api_nicklist_remove_all, METH_VARARGS, "" },
    { "bar_item_search", &weechat_python_api_bar_item_search, METH_VARARGS, "" },
    { "bar_item_new", &weechat_python_api_bar_item_new, METH_VARARGS, "" },
    { "bar_item_update", &weechat_python_api_bar_item_update, METH_VARARGS, "" },
    { "bar_item_remove", &weechat_python_api_bar_item_remove, METH_VARARGS, "" },
    { "bar_search", &weechat_python_api_bar_search, METH_VARARGS, "" },
    { "bar_new", &weechat_python_api_bar_new, METH_VARARGS, "" },
    { "bar_set", &weechat_python_api_bar_set, METH_VARARGS, "" },
    { "bar_update", &weechat_python_api_bar_update, METH_VARARGS, "" },
    { "bar_remove", &weechat_python_api_bar_remove, METH_VARARGS, "" },
    { "command", &weechat_python_api_command, METH_VARARGS, "" },
    { "info_get", &weechat_python_api_info_get, METH_VARARGS, "" },
    { "infolist_new", &weechat_python_api_infolist_new, METH_VARARGS, "" },
    { "infolist_new_var_integer", &weechat_python_api_infolist_new_var_integer, METH_VARARGS, "" },
    { "infolist_new_var_string", &weechat_python_api_infolist_new_var_string, METH_VARARGS, "" },
    { "infolist_new_var_pointer", &weechat_python_api_infolist_new_var_pointer, METH_VARARGS, "" },
    { "infolist_new_var_time", &weechat_python_api_infolist_new_var_time, METH_VARARGS, "" },
    { "infolist_get", &weechat_python_api_infolist_get, METH_VARARGS, "" },
    { "infolist_next", &weechat_python_api_infolist_next, METH_VARARGS, "" },
    { "infolist_prev", &weechat_python_api_infolist_prev, METH_VARARGS, "" },
    { "infolist_fields", &weechat_python_api_infolist_fields, METH_VARARGS, "" },
    { "infolist_integer", &weechat_python_api_infolist_integer, METH_VARARGS, "" },
    { "infolist_string", &weechat_python_api_infolist_string, METH_VARARGS, "" },
    { "infolist_pointer", &weechat_python_api_infolist_pointer, METH_VARARGS, "" },
    { "infolist_time", &weechat_python_api_infolist_time, METH_VARARGS, "" },
    { "infolist_free", &weechat_python_api_infolist_free, METH_VARARGS, "" },
    { "upgrade_new", &weechat_python_api_upgrade_new, METH_VARARGS, "" },
    { "upgrade_write_object", &weechat_python_api_upgrade_write_object, METH_VARARGS, "" },
    { "upgrade_read", &weechat_python_api_upgrade_read, METH_VARARGS, "" },
    { "upgrade_close", &weechat_python_api_upgrade_close, METH_VARARGS, "" },
    { NULL, NULL, 0, NULL }
};

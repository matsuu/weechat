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

/* gui-gtk-color.c: color functions for Gtk GUI */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "../../core/weechat.h"
#include "../../core/wee-config.h"
#include "../../core/wee-string.h"
#include "../gui-color.h"
#include "gui-gtk.h"


struct t_gui_color gui_weechat_colors[] =
{ { -1,                    0, 0,        "default"      },
  { WEECHAT_COLOR_BLACK,   0, 0,        "black"        },
  { WEECHAT_COLOR_RED,     0, 0,        "red"          },
  { WEECHAT_COLOR_RED,     0, A_BOLD,   "lightred"     },
  { WEECHAT_COLOR_GREEN,   0, 0,        "green"        },
  { WEECHAT_COLOR_GREEN,   0, A_BOLD,   "lightgreen"   },
  { WEECHAT_COLOR_YELLOW,  0, 0,        "brown"        },
  { WEECHAT_COLOR_YELLOW,  0, A_BOLD,   "yellow"       },
  { WEECHAT_COLOR_BLUE,    0, 0,        "blue"         },
  { WEECHAT_COLOR_BLUE,    0, A_BOLD,   "lightblue"    },
  { WEECHAT_COLOR_MAGENTA, 0, 0,        "magenta"      },
  { WEECHAT_COLOR_MAGENTA, 0, A_BOLD,   "lightmagenta" },
  { WEECHAT_COLOR_CYAN,    0, 0,        "cyan"         },
  { WEECHAT_COLOR_CYAN,    0, A_BOLD,   "lightcyan"    },
  { WEECHAT_COLOR_WHITE,   0, A_BOLD,   "white"        },
  { 0,                     0, 0,        NULL           }
};


/*
 * gui_color_search: search a color by name
 *                   Return: number of color in WeeChat colors table
 */

int
gui_color_search (const char *color_name)
{
    int i;
    
    for (i = 0; gui_weechat_colors[i].string; i++)
    {
        if (string_strcasecmp (gui_weechat_colors[i].string, color_name) == 0)
            return i;
    }
    
    /* color not found */
    return -1;
}

/*
 * gui_color_assign: assign a WeeChat color (read from config)
 */

int
gui_color_assign (int *color, const char *color_name)
{
    int i;
    
    /* look for curses colors in table */
    i = 0;
    while (gui_weechat_colors[i].string)
    {
        if (string_strcasecmp (gui_weechat_colors[i].string, color_name) == 0)
        {
            *color = i;
            return 1;
        }
        i++;
    }
    
    /* color not found */
    return 0;
}

/*
 * gui_color_get_number: get number of available colors
 */

int
gui_color_get_number ()
{
    return 0;
}

/*
 * gui_color_get_name: get color name
 */

const char *
gui_color_get_name (int num_color)
{
    return gui_weechat_colors[num_color].string;
}

/*
 * gui_color_get_pair: get color pair with a WeeChat color number
 */

int
gui_color_get_pair (int num_color)
{
    int fg, bg;
    
    if ((num_color < 0) || (num_color > GUI_COLOR_NUM_COLORS - 1))
        return WEECHAT_COLOR_WHITE;
    
    fg = gui_color[num_color]->foreground;
    bg = gui_color[num_color]->background;
    
    if (((fg == -1) || (fg == 99))
        && ((bg == -1) || (bg == 99)))
        return 63;
    if ((fg == -1) || (fg == 99))
        fg = WEECHAT_COLOR_WHITE;
    if ((bg == -1) || (bg == 99))
        bg = 0;
    
    return (bg * 8) + fg;
}

/*
 * gui_color_init_pairs: init color pairs
 */

void
gui_color_init_pairs ()
{
    /* This function does nothing in Gtk GUI */
}

/*
 * gui_color_init_weechat: init WeeChat colors
 */

void
gui_color_init_weechat ()
{
    /* TODO: write this function for Gtk */
}

/*
 * gui_color_rebuild_weechat: rebuild WeeChat colors
 */

void
gui_color_rebuild_weechat ()
{
    int i;
    
    for (i = 0; i < GUI_COLOR_NUM_COLORS; i++)
    {
        if (gui_color[i])
        {
            if (gui_color[i]->string)
                free (gui_color[i]->string);
            free (gui_color[i]);
            gui_color[i] = NULL;
        }
    }
    gui_color_init_weechat ();
}

/*
 * gui_color_pre_init: pre-init colors
 */

void
gui_color_pre_init ()
{
    int i;
    
    for (i = 0; i < GUI_COLOR_NUM_COLORS; i++)
    {
        gui_color[i] = NULL;
    }
}

/*
 * gui_color_init: init GUI colors
 */

void
gui_color_init ()
{
    gui_color_init_pairs ();
    gui_color_init_weechat ();
}

/*
 * gui_color_end: end GUI colors
 */

void
gui_color_end ()
{
    int i;
    
    for (i = 0; i < GUI_COLOR_NUM_COLORS; i++)
    {
        gui_color_free (gui_color[i]);
    }
}

# Copyright (c) 2003-2009 FlashCode <flashcode@flashtux.org>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#

INCLUDES = -DLOCALEDIR=\"$(datadir)/locale\"

noinst_LIBRARIES = lib_weechat_gui_common.a

lib_weechat_gui_common_a_SOURCES = gui-bar.c \
                                   gui-bar.h \
                                   gui-bar-item.c \
                                   gui-bar-item.h \
                                   gui-bar-window.c \
                                   gui-bar-window.h \
                                   gui-buffer.c \
                                   gui-buffer.h \
                                   gui-chat.c \
                                   gui-chat.h \
                                   gui-color.c \
                                   gui-color.h \
                                   gui-completion.c \
                                   gui-completion.h \
                                   gui-filter.c \
                                   gui-filter.h \
                                   gui-history.c \
                                   gui-history.h \
                                   gui-hotlist.c \
                                   gui-hotlist.h \
                                   gui-input.c \
                                   gui-input.h \
                                   gui-keyboard.c \
                                   gui-keyboard.h \
                                   gui-layout.c \
                                   gui-layout.h \
                                   gui-line.c \
                                   gui-line.h \
                                   gui-main.h \
                                   gui-nicklist.c \
                                   gui-nicklist.h \
                                   gui-window.c \
                                   gui-window.h

if GUI_NCURSES
curses_dir=curses
endif

if GUI_WXWIDGETS
wxwidgets_dir=wxwidgets
endif

if GUI_GTK
gtk_dir=gtk
endif

if GUI_QT
qt_dir=qt
endif

SUBDIRS = . $(curses_dir) $(wxwidgets_dir) $(gtk_dir) $(qt_dir)

EXTRA_DIST = CMakeLists.txt

# Copyright (c) 2003-2006 FlashCode <flashcode@flashtux.org>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
#

LANGCODE = de
BOOK = weechat.$(LANGCODE)
BOOK_INCLUDE = weechat_commands.xml irc_commands.xml key_functions.xml config.xml

EXTRA_DIST = $(BOOK).xml $(BOOK_INCLUDE)

docdir = $(datadir)/doc/$(PACKAGE)

all-local: html-stamp

# HTML output with chunks (many pages)

html: html-stamp

html-stamp: $(BOOK).xml $(BOOK_INCLUDE) ../weechat-html.xsl ../weechat-doc.css
	mkdir -p html/
	xsltproc -o html/ ../weechat-html.xsl $(BOOK).xml || true
	cp ../weechat-doc.css html/
	touch html-stamp

# HTML output, all in one page

html1: html1-stamp

html1-stamp: $(BOOK).xml $(BOOK_INCLUDE) ../weechat-html-one.xsl ../weechat-doc.css
	mkdir -p html1/
	xsltproc -o html1/$(BOOK).html ../weechat-html-one.xsl $(BOOK).xml || true
	cp ../weechat-doc.css html1/
	touch html1-stamp

# install docs

install-data-hook:
	$(mkinstalldirs) $(DESTDIR)$(docdir)/html/$(LANGCODE)/
	$(INSTALL_DATA) html/* $(DESTDIR)$(docdir)/html/$(LANGCODE)/

# clean

clean-local:
	-rm -f *.html *.pdf *.txt
	-rm -rf html/ html1/
	-rm -f html-stamp html1-stamp
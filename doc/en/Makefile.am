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

docdir = $(datadir)/doc/$(PACKAGE)

EXTRA_DIST = CMakeLists.txt \
		weechat_user.en.txt \
		weechat_plugin_api.en.txt \
		weechat_scripting.en.txt \
		weechat_faq.en.txt \
		weechat_quickstart.en.txt \
		weechat_tester.en.txt \
		$(wildcard autogen/user/*.txt) \
		$(wildcard autogen/plugin_api/*.txt)

all-local: weechat_user.en.html \
		weechat_plugin_api.en.html \
		weechat_scripting.en.html \
		weechat_faq.en.html \
		weechat_quickstart.en.html \
		weechat_tester.en.html

# user's guide
weechat_user.en.html: weechat_user.en.txt $(wildcard autogen/user/*.txt)
	$(ASCIIDOC) -a toc -a toclevels=3 -a date=`date "+%F"` -a revision="$(VERSION)" -n -o weechat_user.en.html weechat_user.en.txt

# plugin API reference
weechat_plugin_api.en.html: weechat_plugin_api.en.txt $(wildcard autogen/plugin_api/*.txt)
	$(ASCIIDOC) -a toc -a toclevels=3 -a date=`date "+%F"` -a revision="$(VERSION)" -n -o weechat_plugin_api.en.html weechat_plugin_api.en.txt

# scripting guide
weechat_scripting.en.html: weechat_scripting.en.txt
	$(ASCIIDOC) -a toc -a toclevels=3 -a date=`date "+%F"` -a revision="$(VERSION)" -n -o weechat_scripting.en.html weechat_scripting.en.txt

# FAQ
weechat_faq.en.html: weechat_faq.en.txt
	$(ASCIIDOC) -a toc -a date=`date "+%F"` -a revision="$(VERSION)" -n -o weechat_faq.en.html weechat_faq.en.txt

# quickstart
weechat_quickstart.en.html: weechat_quickstart.en.txt
	$(ASCIIDOC) -a toc -a date=`date "+%F"` -a revision="$(VERSION)" -n -o weechat_quickstart.en.html weechat_quickstart.en.txt

# tester's guide
weechat_tester.en.html: weechat_tester.en.txt
	$(ASCIIDOC) -a toc -a date=`date "+%F"` -a revision="$(VERSION)" -n -o weechat_tester.en.html weechat_tester.en.txt

# install docs

install-data-hook:
	$(mkinstalldirs) $(DESTDIR)$(docdir)/
	$(INSTALL_DATA) *.html $(DESTDIR)$(docdir)/

# clean

clean-local:
	-rm -f *.html

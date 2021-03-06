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

# FAQ
ADD_CUSTOM_COMMAND(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/weechat_faq.pl.html
  COMMAND ${ASCIIDOC_EXECUTABLE} ARGS -a toc -a toc_title='Spis treści' -a date=`date "+%F"` -a revision="${VERSION}" -n -o ${CMAKE_CURRENT_BINARY_DIR}/weechat_faq.pl.html ${CMAKE_CURRENT_SOURCE_DIR}/weechat_faq.pl.txt
  DEPENDS
  ${CMAKE_CURRENT_SOURCE_DIR}/weechat_faq.pl.txt
  COMMENT "Building weechat_faq.pl.html"
)
ADD_CUSTOM_TARGET(doc-faq-pl ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/weechat_faq.pl.html)
INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/weechat_faq.pl.html DESTINATION ${SHAREDIR}/doc/${PROJECT_NAME})

# quickstart
ADD_CUSTOM_COMMAND(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/weechat_quickstart.pl.html
  COMMAND ${ASCIIDOC_EXECUTABLE} ARGS -a toc -a toc_title='Spis treści' -a date=`date "+%F"` -a revision="${VERSION}" -n -o ${CMAKE_CURRENT_BINARY_DIR}/weechat_quickstart.pl.html ${CMAKE_CURRENT_SOURCE_DIR}/weechat_quickstart.pl.txt
  DEPENDS
  ${CMAKE_CURRENT_SOURCE_DIR}/weechat_quickstart.pl.txt
  COMMENT "Building weechat_quickstart.pl.html"
)
ADD_CUSTOM_TARGET(doc-quickstart-pl ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/weechat_quickstart.pl.html)
INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/weechat_quickstart.pl.html DESTINATION ${SHAREDIR}/doc/${PROJECT_NAME})

# tester's guide
ADD_CUSTOM_COMMAND(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/weechat_tester.pl.html
  COMMAND ${ASCIIDOC_EXECUTABLE} ARGS -a toc -a toc_title='Spis treści' -a date=`date "+%F"` -a revision="${VERSION}" -n -o ${CMAKE_CURRENT_BINARY_DIR}/weechat_tester.pl.html ${CMAKE_CURRENT_SOURCE_DIR}/weechat_tester.pl.txt
  DEPENDS
  ${CMAKE_CURRENT_SOURCE_DIR}/weechat_tester.pl.txt
  COMMENT "Building weechat_tester.pl.html"
)
ADD_CUSTOM_TARGET(doc-tester-pl ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/weechat_tester.pl.html)
INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/weechat_tester.pl.html DESTINATION ${SHAREDIR}/doc/${PROJECT_NAME})

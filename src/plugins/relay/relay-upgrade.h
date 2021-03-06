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


#ifndef __WEECHAT_RELAY_UPGRADE_H
#define __WEECHAT_RELAY_UPGRADE_H 1

#define RELAY_UPGRADE_FILENAME "relay"

/* For developers: please add new values ONLY AT THE END of enums */

enum t_relay_upgrade_type
{
    RELAY_UPGRADE_TYPE_RELAY = 0,
};

extern int relay_upgrade_save ();
extern int relay_upgrade_load ();

#endif /* relay-upgrade.h */

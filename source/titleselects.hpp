/*
*   This file is part of ModMoon
*   Copyright (C) 2018-2019 Swiftloke
*
*   This program is free software: you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 3 of the License, or
*   (at your option) any later version.
*
*   This program is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
*   Additional Terms 7.b and 7.c of GPLv3 apply to this file:
*       * Requiring preservation of specified reasonable legal notices or
*         author attributions in that material or in the Appropriate Legal
*         Notices displayed by works containing it.
*       * Prohibiting misrepresentation of the origin of that material,
*         or requiring that modified versions of such material be marked in
*         reasonable ways as different from the original version.
*/

#pragma once
#include <3ds/types.h>
//Both the regular title selection menu and the active title selection menu
void titleselect();
//The active title select menu can be used to select an arbitrary title,
//even one not in the list. This can be used to select a destination title
//for a modpack which has no information about it (i.e. what TID it goes with.)
u64 activetitleselect(bool picktitle = false);
//This is used during initial setup. This use case is based around when a title
//needs to be activated, but all the title icons need to finish loading
//before that can occur.
void queuetitleforactivationwithinmenu(u64 titleid, int mediatype);

//Allow this to be done manually. This use case is for modpack downloading.
void triggeractivationqueue();

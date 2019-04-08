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

#include <fstream>
#include "migrators.hpp"
#include "../main.hpp"
#include "../utils.hpp"
#include <sys/stat.h>
#include <algorithm>
#include "../error.hpp"
#include "../titleselects.hpp"

/*int foldercount = 0; //Immediately incremented to 1 in migration
int totalslots = 0; //Calculated later
bool isitdone = false;

std::tuple<int, int, bool> ss2xretriveinfo()
{
	return std::make_tuple(foldercount, totalslots, isitdone);
}*/

WorkerFunction ss2xworker(ss2xMigrate, \
	"Moving Smash Selector 2.x mods...\nMod [progress] / [total]");

int ss2xmaxSlotCheck(string ss2xmodsfolder)
{
	int currentFolderCount = 0;
	stringstream path2Check;
	stringstream NewPath2Check;
	do {
		currentFolderCount++;
		path2Check.str("");
		path2Check << ss2xmodsfolder << "Slot_" << currentFolderCount << "/";
	} while (pathExist(path2Check.str()));
	//the minus 1 is due to it returning the folder number that doesnt exist.
	return currentFolderCount - 1;
}

void ss2xmovemods(string srcmodsfolder, u64 title, int premigratemaxslot, WorkerFunction* notthis)
{
	string src, dest;
	while (true)
	{
		notthis->functionprogress++;
		src = srcmodsfolder + "Slot_" + to_string(notthis->functionprogress);
		dest = modsfolder + tid2str(title) + "/Slot_" + to_string(notthis->functionprogress + premigratemaxslot);
		if(!pathExist(src)) break;
		rename(src.c_str(), dest.c_str());
	}
}

void ss2xMigrate(WorkerFunction* notthis)
{
	Config ss2xconfig("/3ds/data/smash_selector/", "settings.txt");
	int gametype = ss2xconfig.read("GameType") == "Cia" ? 1 : 2;
	string region = ss2xconfig.read("GameRegion");
	u64 title = region == "Usa" ? 0x00040000000EDF00 : \
		region == "Eur" ? 0x00040000000EE000 : 0x00040000000B8B00;
	string ss2xmodsfolder = ss2xconfig.read("ModsFolder");
	int missingmod = ss2xconfig.read("SelectedModSlot", 0);
	queuetitleforactivationwithinmenu(title, gametype);
	if (!pathExist(modsfolder + tid2str(title)))
		_mkdir((modsfolder + tid2str(title)).c_str());
	//What's currently in /saltysd/smash? Keep the order.
	rename("/saltysd/smash", (ss2xmodsfolder + "Slot_" + to_string(missingmod)).c_str());
	int premigratemaxslot = maxslotcheck(title);
	notthis->functiontotal = ss2xmaxSlotCheck(ss2xmodsfolder);
	ss2xmovemods(ss2xmodsfolder, title, premigratemaxslot, notthis);
	remove("/3ds/data/smash_selector/settings.txt");
	notthis->functiondone = true;
}
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

#include "main.hpp"
#include "error.hpp"
#include "download.hpp"
#include "archive/archive.h"
#include "utils.hpp"
#include "titleselects.hpp"
#include <unistd.h>
#include <fstream>
#include <algorithm>
#include <sstream>

WorkerFunction* callbackAccess; //Must be at top level for the zip function callback :/

void modpackDownload()
{
	error("Welcome to the modpack\ndownloader! Press Start to\ncancel now.");
	if (errorwasstartpressed()) return;
	error("Enter a link to a modpack.");
	SwkbdState swkbd;
	//If it's larger than this the user has bigger problems.
	//Like carpal tunnel from inputting all of that on a tiny crappy touch keyboard.
	char keyboardinput[512];
	swkbdInit(&swkbd, SWKBD_TYPE_WESTERN, 2, -1);
	swkbdSetValidation(&swkbd, SWKBD_NOTEMPTY_NOTBLANK, 0, 0);
	swkbdSetHintText(&swkbd, "Enter a link to a modpack.");
	swkbdInputText(&swkbd, keyboardinput, sizeof(keyboardinput));

	error("Download from this link?\n" + string(keyboardinput) + "\nPress Start to cancel now.");
	if (errorwasstartpressed()) return;

	//The magic happens here :D
	//Start by removing the old stuff if it exists for some reason
	remove("/3ds/ModMoon/temp.zip");
	rmdir("/3ds/ModMoon/temp");

	int success;
	auto downloadfunc = [&keyboardinput, &success](WorkerFunction* notthis)
	{
		success = http_download(keyboardinput, "DOWNLOAD", notthis, "/3ds/ModMoon/temp.zip");
		notthis->functiondone = true;
	};
	DownloadWorker downloader(downloadfunc, 
		"Downloading modpack...\n[progress] bytes / [total] bytes...\nGet comfy, this is going to\ntake a while.", true);
	downloader.startworker();
	downloader.displayprogress();
	if (success != 0)
	{
		error("Error: Failed to download modpack!\nError code: " + hex2str(success));
		remove("/3ds/ModMoon/temp.zip");
		return;
	}

	//Now for the Lua Player Plus bit...
	auto zipextractfunc = [&success](WorkerFunction* notthis)
	{
		callbackAccess = notthis;
		Zip* zip = ZipOpen("/3ds/ModMoon/temp.zip");
		auto zipcallback = [](u32 progress, u32 total)
		{
			callbackAccess->functionprogress = progress;
			callbackAccess->functiontotal = total;
			return 0;
		};
		_mkdir("/3ds/ModMoon/temp");
		chdir("/3ds/ModMoon/temp");
		success = ZipExtract(zip, NULL, zipcallback);
		ZipClose(zip);
		notthis->functiondone = true;
	};
	WorkerFunction zipextractworker(zipextractfunc, 
		"Extracting modpack... Get comfy,\nthis is going to take a while.\nFile [progress] of [total]", true);
	zipextractworker.startworker();
	zipextractworker.displayprogress();
	//Lua Player Plus's ZIP extractor always returns an error, even on success. Whoops.
	/*if (success != 1)
	{
		error("Error: Failed to extract ZIP file!");
		chdir("/");
		remove("/3ds/ModMoon/temp.zip");
		rmdir("/3ds/ModMoon/temp");
		return;
	}*/
	ifstream namefile("/3ds/ModMoon/temp/desc.txt");


	//Check that the modpack is, well, a modpack
	//Eh, in reality, I trust the user enough to pick a real modpack.
	/*
	if(!namefile)
	{
		error("This modpack does not contain\na desc.txt file. As a result, it is\nunlikely that it is a real modpack.");
		error("The download has been\ndiscarded. Please try again with\na valid link to a modpack.");
		
		chdir("/");
		remove("/3ds/ModMoon/temp.zip");
		rmdir("/3ds/ModMoon/temp");
		return;
	}*/

	string name;
	getline(namefile, name);
	namefile.close();

	u64 tid = 0;
	//Before opening the config, check to see if a modpackinfo exists.
	//ModMoon's updater is recent relative to most modpacks. This file may
	//not exist. 
	ifstream check("/3ds/ModMoon/temp/modpackinfo.txt");
	if(!check)
	{
		error("This modpack does not contain\ninformation that ModMoon\nneeds to determine the game\nit goes with.");
		error("The active title select menu\nwill be opened. Please pick the\ntitle that this game is for.");
		
		tid = activetitleselect(true);
		while(!tid)
		{
			error("A title was not selected.\nIf your title is on a cartridge\nthat was not inserted when\nModMoon started,");
			error("Due to a bug, ModMoon cannot\ndetect it without restarting.\nPlease see the GitHub README\nfor more info on this bug.");
			tid = activetitleselect(true);
		}
	}
	else
	{
		//Read out info about the modpack
		Config info("/3ds/ModMoon/temp/", "modpackinfo.txt");
		tid = info.read("TitleID", 0, 0);
	}
	check.close();

	if (std::find(titleids.begin(), titleids.end(), tid) == titleids.end())
	{
		queuetitleforactivationwithinmenu(tid, MEDIATYPE_SD);
		triggeractivationqueue();
		if(std::find(titleids.begin(), titleids.end(), tid) == titleids.end())
			error("Warning: Could not find the\ntitle corresponding to this\nmodpack! Is it on a cartridge?");
		else
			error("This title has been activated\nautomatically for you.\nNow isn't that nice?");
	}

	int newslot = maxslotcheck(tid, 1000) + 1;
	string dest = "/3ds/ModMoon/" + hex2str(tid) + "/Slot_" + to_string(newslot);
	chdir("/");
	remove("/3ds/ModMoon/temp.zip");
	rename("/3ds/ModMoon/temp", dest.c_str());
	//If the game in question is the current title, we need it to recognize there's a new slot.
	maxslot = maxslotcheck();
	error("Finished installing modpack!\nModpack name:\n" + name);
}

/*
Modpack updates:
Provided in the "UpdateInfo" section of the modpack config.
Link to a Config text file that can be downloaded, opened,
and checked for an update.
Updates will be checked for by comparing the "Version" tag on disk
against the "Version" tag on the server.
When an update is available, an alert will pop up stating that an update
is available for "Modpack X" and it can be downloaded now. An "IgnoreUpdates" tag
should be written to the modpack on disk if the update is taken, downloading to a new slot,
or if the user responds "do not download" and says yes to "ignore from now on?".
When the update is accepted, just run the modpack downloader with an argument of
"UpdateLink" from the server config.
*/
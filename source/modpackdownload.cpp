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
		"Downloading modpack...\n[progress] bytes / [total] bytes", true);
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

	//Read out info about the modpack
	ifstream tidfile("/3ds/ModMoon/temp/titleid.txt");
	char titleid[16]={0};
	tidfile.read(titleid, 16);
	u64 tid = strtoull(titleid, NULL, 16);

	tidfile.close();
	ifstream namefile("/3ds/ModMoon/temp/desc.txt");
	string name;
	getline(namefile, name);
	namefile.close();

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
	string dest = "/3ds/ModMoon/" + string(titleid).substr(0,16) + "/Slot_" + to_string(newslot);
	chdir("/");
	remove("/3ds/ModMoon/temp.zip");
	rename("/3ds/ModMoon/temp", dest.c_str());
	//If the game in question is the current title, we need it to recognize there's a new slot.
	maxslot = maxslotcheck();
	error("Finished installing modpack!\nModpack name:\n" + name);
}

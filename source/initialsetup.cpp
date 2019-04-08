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
#include "utils.hpp"
#include "migrators/migrators.hpp"
#include "download.hpp"
#include "titleselects.hpp"
#include "toolsmenu.hpp"

string progresstext;
unsigned int progress = 0;
unsigned int total = 0;
float expandpos = 0;
C3D_Tex dummy; //We need a texture for the framebuffer, but we're against a black background.
bool progressrunning = true;
bool doprogressdraw = true;
bool dopopup = false;
bool dopopdown = false;
Handle progressthreaddone;
Handle popupdone;
Handle popdowndone;

void threadfunc_drawinitialsetupprogress(void* args)
{
	//insert the progress and total into the string?
	while (progressrunning)
	{
		if(!doprogressdraw)
			continue;
		if (dopopup)
		{
			while (expandpos < 1) //Let it stay at max once we're done
			{
				expandpos += .06f;
				drawprogresserror(progresstext, expandpos, (float)progress / total, dummy, dummy);
				if (expandpos >= 1) expandpos = 1; //Prevent it from going overboard
			}
			dopopup = false;
			svcSignalEvent(popupdone);
		}
		else if (dopopdown)
		{
			while (expandpos > 0)
			{
				expandpos -= .06f;
				if (expandpos <= 0) expandpos = 0;
				drawprogresserror(progresstext, expandpos, (float)progress / total, dummy, dummy);
			}
			dopopdown = false;
			svcSignalEvent(popdowndone);
		}
		else
		{
			drawprogresserror(progresstext, expandpos, (float)progress / total, dummy, dummy);
		}
	}
	svcSignalEvent(progressthreaddone);
}

void progresspopup()
{
	expandpos += .06f;
	if (expandpos >= 1) expandpos = 1; //Prevent it from going overboard
}

void progresspopdown()
{
	expandpos -= .06f;
	if (expandpos <= 0) expandpos = 0;
}

bool doallmigration()
{
	s32 mainthreadpriority;
	svcGetThreadPriority(&mainthreadpriority, CUR_THREAD_HANDLE);
	bool migrationwasdone = false;
	if (pathExist("/saltysd/card.txt"))
	{
		/*progresstext = "Migrating mods from\nSmash Selector 1.0...";
		//svcWaitSynchronization(popupdone, U64_MAX);
		threadCreate(ss1xMigrate, NULL, 20000, mainthreadpriority + 1, -2, true);
		int ss1xprogress;
		bool ss1xdone;
		do
		{
			std::tie(ss1xprogress, ss1xdone) = ss1xretrieveinfo();
			progresspopup();
			//I honestly have no idea how I would implement checking the max slot of SS 1.0, so "?" it is.
			drawprogresserror("Moving Smash Selector 1.0 mods...\nMod " + to_string(ss1xprogress) + " / ?", \
				expandpos, (float)1, dummy, dummy);
		} while (!ss1xdone);
		while (expandpos > 0)
		{
			progresspopdown();
			drawprogresserror("Moving Smash Selector 1.0 mods...\nMod " + to_string(ss1xprogress) + " / ?", \
				expandpos, (float)1, dummy, dummy);
		}
		//svcWaitSynchronization(popdowndone, U64_MAX);*/
		ss1xworker.startworker();
		ss1xworker.displayprogress();
		migrationwasdone = true;
	}
	if (pathExist("/3ds/data/smash_selector/settings.txt"))
	{
		/*threadCreate(ss2xMigrate, NULL, 20000, mainthreadpriority + 1, -2, true);
		int ss2xprogress, ss2xtotal;
		bool ss2xdone;
		do
		{
			std::tie(ss2xprogress, ss2xtotal, ss2xdone) = ss2xretriveinfo();
			progresspopup();
			drawprogresserror("Moving Smash Selector 2.x mods...\nMod " + to_string(ss2xprogress) + " / " + to_string(ss2xtotal), \
				expandpos, (float)ss2xprogress / ss2xtotal, dummy, dummy);
		} while (!ss2xdone);
		while (expandpos > 0)
		{
			progresspopdown();
			drawprogresserror("Moving Smash Selector 2.x mods...\nMod " + to_string(ss2xprogress) + " / " + to_string(ss2xtotal), \
				expandpos, (float)ss2xprogress / ss2xtotal, dummy, dummy);
		}*/
		ss2xworker.startworker();
		ss2xworker.displayprogress();
		migrationwasdone = true;
	}

	//Remove or update code.bin/code.ips files here.

	//Lasagna migrator? Maybe I should poll GBATemp to find out if it's worth my time to make it.
	//It seems like it's going to be quite annoying to pull off.
	//
	//
	return migrationwasdone;
}

void tutorial(bool migrationwasdone)
{
	//A quick tutorial?
	mainmenushiftin();
	//threadfunc_fade(colorvalues);
	//errorsetmode(MODE_FADE);
	error("Let's get you acquainted\nwith ModMoon.");
	error("Press Start now to skip\nthis tutorial.");
	if (errorwasstartpressed())
	{
		errorsetmode(MODE_POPUP);
		return;
	}
	error("First, let's select the\ngames you want to use.");
	error("Touch the tools button.");
	//Most of the same logic as the real main menu, but only the tools button is selectable
	touchPosition tpos, opos;
	u32 kDown, kHeld;
	int alphapos = 0;
	bool alphaplus = true;
	while (aptMainLoop())
	{
		hidScanInput();
		kDown = hidKeysDown();
		kHeld = hidKeysHeld();
		hidTouchRead(&tpos);
		if (buttonpressed(rightbutton, 169, 13, opos, kHeld) || kDown & KEY_A)
		{
			mainmenushiftout();
			break;
		}
		sdraw::framestart();
		drawtopscreen();
		sdraw::drawon(GFX_BOTTOM);
		highlighterhandle(alphapos, alphaplus);
		mainmenudraw(1, tpos, alphapos, false);
		sdraw::frameend();
		opos = tpos;
	}
	error("Tap on the Active Title Selection\nbutton.");
	alphapos = 0;
	alphaplus = true;
	tpos = touchPosition();
	opos = touchPosition();
	toolsmenushiftin();
	while (aptMainLoop())
	{
		highlighterhandle(alphapos, alphaplus);
		toolsmenudraw(1.0, 0, alphapos, false);
		hidScanInput();
		kDown = hidKeysDown();
		kHeld = hidKeysHeld();
		hidTouchRead(&tpos);
		if (buttonpressed(activetitlesbutton, 18, 6, opos, kHeld) || kDown & KEY_A)
		{
			toolsmenushiftout();
			break;
		}
		opos = tpos;
	}
	error("Select the titles you want to\nactivate for use by tapping them\nor using the Circle Pad and .");
	error("Activated titles will glow blue.\nThe cartridge is always active.");
	if (migrationwasdone)
		error("As part of migration, titles you\nused previously will already\nbe activated.");
	//errorsetmode(MODE_POPUP); //activetitleselect can call errors
	activetitleselect();
	//errorsetmode(MODE_FADE);
	error("You can always select new titles\nor deactivate existing ones by\nentering this menu.");
	toolsmenushiftin();
	error("Press  to return to the\nmain menu.");
	alphapos = 0;
	alphaplus = true;
	while (aptMainLoop())
	{
		highlighterhandle(alphapos, alphaplus);
		toolsmenudraw(1.0, 0, alphapos, false);
		hidScanInput();
		kDown = hidKeysDown();
		kHeld = hidKeysHeld();
		hidTouchRead(&tpos);
		if (kDown & KEY_B)
		{
			toolsmenushiftout();
			break;
		}
		opos = tpos;
	}
	mainmenushiftin();
	error("Now press  to enter the title\nselection menu.");
	//Blah, duplicated code that I can't do anything about.
	while (aptMainLoop())
	{
		hidScanInput();
		kDown = hidKeysDown();
		kHeld = hidKeysHeld();
		if (kDown & KEY_Y)
			break;
		sdraw::framestart();
		drawtopscreen();
		sdraw::drawon(GFX_BOTTOM);
		highlighterhandle(alphapos, alphaplus);
		mainmenudraw(1, tpos, alphapos, false);
		sdraw::frameend();
	}
	error("To select the title currently in\nuse, you can use this menu and\ntap it or use the Circle Pad and .");
	error("You can access this menu from\nany menu screen at any time,\nnot just in the main menu.");
	//errorsetmode(MODE_POPUP);
	titleselect();
	//errorsetmode(MODE_FADE);
	//Fix a small issue with the tools menu and the main menu clashing together in a copied framebuffer
	sdraw::framestart();
	drawtopscreen();
	sdraw::drawon(GFX_BOTTOM);
	highlighterhandle(alphapos, alphaplus);
	mainmenudraw(0, tpos, 0, false);
	sdraw::frameend();
	error("You're all set! Have fun,\nand happy modding!");
	//errorsetmode(MODE_POPUP);
}

void initialsetup()
{
	error("Welcome! Let's get you set up.");
	//Signal the progress drawer to simply draw a black rectangle for the background. See error.hpp
	dummy.height = 0;
	//svcCreateEvent(&progressthreaddone, RESET_ONESHOT);
	//svcCreateEvent(&popupdone, RESET_ONESHOT);
	//svcCreateEvent(&popdowndone, RESET_ONESHOT);
	//threadCreate(threadfunc_drawinitialsetupprogress, NULL, 20000, mainthreadpriority - 2, -2, true);
	bool migrationwasdone = doallmigration();

	//initupdatechecker();
	if (!shoulddisableupdater)
	{
		updatecheckworker.startworker();
		svcWaitSynchronization(event_downloadthreadfinished, U64_MAX);
	}
	if (isupdateavailable() && !shoulddisableupdater)
	{
		//doprogressdraw = false;
		error("An update is available.\nIt will be installed now.");
		updateinstallworker.startworker();
		updateinstallworker.displayprogress();
		/*doprogressdraw = true; //This may cause issues...
		initdownloadandinstallupdate();
		progress = 0;
		total = 100;
		progresstext = "Downloading update...\n[progress]% complete";
		progresspopup();
		do
		{
			progress = retrievedownloadprogress();
			if (progress == 101)
			{
				progresstext = "Installing update...";
				progress = 100;
			}
		} while(progress != 102);
		progresspopdown();
		progressrunning = false; //Another point of conflict?
		svcWaitSynchronization(progressthreaddone, U64_MAX);*/
		error("Update complete. The system\nwill now reboot.");
		nsRebootSystemClean();
	}
	progressrunning = false;
	tutorial(migrationwasdone);
	config.write("InitialSetupDone", true);
}
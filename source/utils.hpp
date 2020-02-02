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
#include <unistd.h>
#include <sys/stat.h>
#include <string>
#include "sdraw/sdraw.hpp"
using namespace std;

void drawtexturewithhighlight(sdraw_stex info, int x, int y, u32 color, 
	int alpha, int x1 = -1, int y1 = -1, float interpfactor = 0);

sdraw_stex constructSMDHtex(C3D_Tex* icon);

Result nsRebootSystemClean();

//When title is not provided this simply checks the current tid.
int maxslotcheck(u64 optionaltid = 1, int optionalslot = -1);
void launch();
void threadfunc_fade(void* main);
int countEntriesInDir(const char* dirname);

//Extremely common logic used in rendering code, but it doesn't feel right to be in sDraw
void highlighterhandle(int& alphapos, bool& alphaplus);

void writeSaltySD(u64 titleid, bool ishitboxdisplay = false);

//Fixes slots after a slot deletion.
void slotFixer(int missingSlot);

inline bool pathExist(const string filename) {
	struct stat buffer;
	return (stat(filename.c_str(), &buffer) == 0);
}

//Calculates the Murmur Hash 2 algorithm on a file.
unsigned int genHash(string filepath);

//Custom SaltySD files provided by modpacks
void movecustomsaltysdout();
void movecustomsaltysdin();

extern Handle event_fadefinished;
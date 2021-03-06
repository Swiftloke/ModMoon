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

#include "titleselects.hpp"
#include "main.hpp"
#include "utils.hpp"
#include "error.hpp"

//Actually initialized later, we can't use it before SMDH data is loaded
//but we can't declare a reference without initializing it.
vector<smdhdata>& icons = getSMDHdata(); 
int oldselectpos;

void titleselectdraw(C3D_Tex prevfb, float fbinterpfactor, int scrollsubtractrows, int selectpos, bool highlighterblink)
{
	sdraw::framestart();
	drawtopscreen();
	sdraw::drawon(GFX_BOTTOM);
	sdraw::MM::shader_basic->bind();
	sdraw::drawtexture(backgroundbot, 0, 0);
	int x = -39, y = 26; //Start at a smaller X coordinate as it'll be advanced in the first loop iteration
	int i = 0;
	static float highlighterinterpfactor = 0;
	static int highlighteroldx = 0;
	static int highlighteroldy = 0;
	static bool highlighterismoving = false;
	static int highlighteralpha = 0;
	static bool highlighteralphaplus = true;

	highlighterhandle(highlighteralpha, highlighteralphaplus);
	y -= 70 * scrollsubtractrows;

	if (scrollsubtractrows == 0)
	{
		sdraw::setfs("texture");
		sdraw::drawtexture(titleselectioncartridge, 22, 17);
	}

	for (vector<smdhdata>::iterator iter = icons.begin(); iter < icons.end(); iter++)
	{
		x += 70;
		if (x > 241)
		{
			x = 31;
			y += 70;
		}
		if (i == selectpos)
		{
			sdraw::drawtext(tid2str((*iter).titl).c_str(), 0, 0, .4, .4);
			if (selectpos != oldselectpos)
			{
				oldselectpos = selectpos;
				highlighterismoving = true;
			}
			if (!highlighterismoving) //We don't want to mess up the values while it's moving
			{
				highlighteroldx = x;
				highlighteroldy = y;
			}
			if (highlighterismoving)
			{
				highlighteralpha = 255;
				highlighterinterpfactor += 0.75;
				if (highlighterinterpfactor >= 1)
				{
					highlighteroldx = x;
					highlighteroldy = y;
					highlighterismoving = false; //Don't forget we need to update the selectpos when it starts moving (in input)
					highlighterinterpfactor = 0;
				}
			}
			if (highlighterblink)
			{
				sdraw::setfs("titleSelectBlink", 1, RGBA8(0, 0, 255, 255));
			}
			//If the highlighter blink is active, we need to give a full alpha, seeing as it's "blinking"
			sdraw::setfs("highlighter", 0, HIGHLIGHTERCOLORANDALPHA(titleselecthighlighter.highlightercolor, highlighterblink ? 255 : highlighteralpha));
			sdraw::drawtexture(titleselecthighlighter, highlighteroldx - 9, highlighteroldy - 9, \
				x - 9, y - 9, highlighterinterpfactor);
			//Now we need to reset stage 1
			if (highlighterblink)
			{
				sdraw::setfs("blank", 1);
			}
			sdraw::setfs("texture", 0);
		}
		i++;
		if(iter->titl != 0) //Not a null cartridge
			sdraw::drawtexture(constructSMDHtex(&(iter->icon)), x, y);
	}
	sdraw::setfs("texture", 0);
	sdraw::drawtexture(titleselectionboxes, 26, 21);
	sdraw::setfs("textColor", 0, RGBA8(165, 165, 165, 255));
	string versiontext = getversion();
	sdraw::drawtext(versiontext.c_str(), 320 - 18 - sdraw::gettextmaxwidth(versiontext.c_str(), .4), 1, .45, .45);
	sdraw::setfs("texture", 0);
	sdraw::MM::shader_twocoords->bind();
	sdraw::drawframebuffer(prevfb, 0, 0, false, 0, -240, fbinterpfactor);
	sdraw::frameend();
}


void titleselect()
{
	C3D_Tex prevbot;
	//Save the framebuffer from the previous menu
	C3D_TexInit(&prevbot, 256, 512, GPU_RGBA8);
	sdraw::retrieveframebuffers(NULL, &prevbot);
	icons = getSMDHdata();
	static int selectpos = currenttidpos;
	oldselectpos = selectpos;
	static int scrollsubtractrows = 0;

	//Calculate the positions of all the buttons the user could potentially press
	float quadsofbuttons[12][2];
	const int advancecount = 70;
	int quadx = 26 - advancecount; //Start smaller, it'll be incremented immediately
	int quady = 21;
	for (int i = 0; i < 12; i++)
	{
		quadx += advancecount;
		if (quadx > 241)
		{
			quadx = 26;
			quady += advancecount;
		}
		quadsofbuttons[i][0] = quadx;
		quadsofbuttons[i][1] = quady;
	}

	float fbinterpfactor = 0;
	while (fbinterpfactor < 1)
	{
		fbinterpfactor += 0.05;
		titleselectdraw(prevbot, fbinterpfactor, scrollsubtractrows, selectpos, false);
	}
	touchPosition tpos, opos;
	bool isbeingtouched = false;
	while (aptMainLoop())
	{
		if (cartridgeneedsupdating)
			updatecartridgedata();
		hidScanInput();
		u32 kDown = hidKeysDown();
		u32 kHeld = hidKeysHeld();
		//No scrolling support, too complex. Maybe I'll use a library for it in the future.
		hidTouchRead(&tpos);
		for (int i = 0; i < 12; i++)
		{
			if (touched(quadsofbuttons[i][0], quadsofbuttons[i][1], advancecount, advancecount, tpos))
			{
				int newselectpos = i + (4 * scrollsubtractrows);
				if (newselectpos <= (signed int)icons.size() - 1)
				{
					selectpos = newselectpos;
					isbeingtouched = true;
				}
			}
			else if (buttonpressed(quadsofbuttons[i][0], quadsofbuttons[i][1], advancecount, advancecount, opos, kHeld))
			{
				//Do the calculation again to ensure we don't accidentally hit something bad
				int newselectpos = i + (4 * scrollsubtractrows);
				if (newselectpos <= (signed int)icons.size() - 1)
				{
					opos = tpos;
					goto selecttitle;
				}
			}
		}
		opos = tpos;
		if (secretcodeadvance(kDown)) continue;
		if (kDown & KEY_LEFT)
		{
			int currow = (selectpos / 4) - scrollsubtractrows;
			selectpos--;
			if (selectpos < 0)
				selectpos++;
			else if (currow == 0 && (selectpos + 1) % 4 == 0 && scrollsubtractrows > 0)
				scrollsubtractrows--;
		}
		if (kDown & KEY_RIGHT)
		{
			int currow = (selectpos / 4) - scrollsubtractrows;
			selectpos++;
			//selectpos can't be unsigned due to logic preventing it from getting lower than 0
			if (selectpos >= (signed int)icons.size()) 
				selectpos--;
			else if (currow == 2 && selectpos % 4 == 0)
				scrollsubtractrows++;

		}
		if (kDown & KEY_UP)
		{
			//What row are we on?
			int currow = (selectpos / 4) - scrollsubtractrows;
			selectpos -= 4;
			if (selectpos < 0)
				selectpos = 0;
			else if(currow == 0 && scrollsubtractrows > 0)
				scrollsubtractrows--;
			//if (selectpos - scrollsubtractrows * 3 <= 12 && scrollsubtractrows > 0) //Will this work? 12?
		}
		if (kDown & KEY_DOWN)
		{
			int currow = (selectpos / 4) - scrollsubtractrows;
			selectpos += 4;
			//selectpos can't be unsigned due to logic preventing it from getting lower than 0
			if (selectpos >= (signed int)icons.size())
				selectpos = icons.size() - 1; //Whee off-by-one errors
			if(currow == 2)
				scrollsubtractrows++;
			//if (selectpos - scrollsubtractrows * 3 >= 12) //Will this work?
		}

		if (kDown & KEY_A)
		{
			selecttitle:
			if (icons[selectpos].titl != 0)
			{
				while (hidKeysHeld() & KEY_A)
				{
					hidScanInput();
					titleselectdraw(prevbot, fbinterpfactor, scrollsubtractrows, selectpos, true);
				}
				currenttidpos = selectpos;
				//Re-initialize stuff for the new TID. This is actually all we have to do, the design
				//Of accessing a vector based on currenttidpos does everything else.
				maxslot = maxslotcheck();
				if (maxslot == 0)
				{
					error("Warning: Failed to find mods for\nthis game!");
					error("Place them at " + modsfolder + '\n' + currenttitleidstr + "/Slot_X\nwhere X is a number starting at 1.");
				}
				mainmenuupdateslotname();
				config.write("SelectedTitleIDPos", currenttidpos);
				modsenabled = currentslot != 0;
				break;
			}
		}
		if (kDown & KEY_B)
			break;
		if (kDown & KEY_X)
		{
			error("Title Selection Menu:\nSelect a game to run mods for!\n");
			error("Games can be added/removed\nfrom the Active Title Selection\nmenu in the tools menu.");
		}
		titleselectdraw(prevbot, fbinterpfactor, scrollsubtractrows, selectpos, isbeingtouched);
	}
	while (fbinterpfactor > 0)
	{
		fbinterpfactor -= 0.05;
		titleselectdraw(prevbot, fbinterpfactor, scrollsubtractrows, selectpos, false);
	}
}
/*We need to do two things here:
Update the title ID pos to reflect our new title ID
Update info related to it (max slot, slot name). This should just need a call to those functions.
I designed this really well and completely accidentally.*/
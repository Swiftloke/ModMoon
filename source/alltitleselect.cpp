//This is a clone of titleselect.cpp, but instead of picking a title to load mods for, it provides a menu
//to set the "ActiveTitleIDs" config value.
//I didn't want to repeat nearly-identical-but-not-quite code in one file, so it's here instead.
#include "titleselects.hpp"
#include "main.hpp"
#include "utils.hpp"
#include "error.hpp"
#include <algorithm>
#include <queue>
#include <utility>

//Actually initialized later, we can't use it before SMDH data is loaded
//but we can't declare a reference without initializing it.
//We're sure to wait until it's finished loading...
vector<smdhdata>& allicons = getallSMDHdata();
int alloldselectpos;
std::queue<std::pair<u64, int>> queueforactivation;

void queuetitleforactivationwithinmenu(u64 titleid, int mediatype)
{
	queueforactivation.push(std::make_pair(titleid, mediatype));
}

void activetitleselectdraw(C3D_Tex prevbotfb, float fbinterpfactor, int scrollsubtractrows, int selectpos)
{
	draw.framestart();
	drawtopscreen();
	draw.drawon(GFX_BOTTOM);
	draw.drawtexture(backgroundbot, 0, 0);
	int x = -39, y = 26; //Start at a smaller X coordinate as it'll be advanced in the first loop iteration
	int i = 0;
	static float highlighterinterpfactor = 0;
	static int highlighteroldx = 0;
	static int highlighteroldy = 0;
	static bool highlighterismoving = false;
	static int highlighteralpha = 0;
	static bool highlighteralphaplus = true;
	//Configure TexEnv stage 1 to "blink" the texture by making it all blue
	C3D_TexEnv coloroverride;
	C3D_TexEnvSrc(&coloroverride, C3D_RGB, GPU_CONSTANT);
	C3D_TexEnvSrc(&coloroverride, C3D_Alpha, GPU_PREVIOUS);
	C3D_TexEnvOpRgb(&coloroverride, GPU_TEVOP_RGB_SRC_COLOR);
	C3D_TexEnvOpAlpha(&coloroverride, GPU_TEVOP_A_SRC_ALPHA, GPU_TEVOP_A_SRC_ALPHA);
	C3D_TexEnvFunc(&coloroverride, C3D_RGB, GPU_REPLACE);
	C3D_TexEnvFunc(&coloroverride, C3D_Alpha, GPU_REPLACE);
	C3D_TexEnvColor(&coloroverride, RGBA8(0, 0, 255, 255));

	highlighterhandle(highlighteralpha, highlighteralphaplus);
	y -= 70 * scrollsubtractrows;

	draw.drawtexture(titleselectioncartridge, 22, 17 - 70 * scrollsubtractrows);

	for (vector<smdhdata>::iterator iter = allicons.begin(); iter < allicons.end(); iter++)
	{
		x += 70;
		if (x > 241)
		{
			x = 31;
			y += 70;
		}
		if (i == 0) //Draw the cartridge icon
			draw.drawtexture(titleselectioncartridge, x - 9, y - 9);
		if (i == selectpos)
		{
			draw.drawtext(tid2str((*iter).titl).c_str(), 0, 0, .4, .4);
			if (selectpos != alloldselectpos)
			{
				alloldselectpos = selectpos;
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
			if(iter->isactive)
				C3D_SetTexEnv(1, &coloroverride);
			draw.drawhighlighter(titleselecthighlighter, highlighteroldx - 9, highlighteroldy - 9, highlighteralpha, x - 9, y - 9, highlighterinterpfactor);
			if(iter->isactive)
				C3D_TexEnvInit(C3D_GetTexEnv(1));
		}
		else if (iter->isactive && i != 0)
		{
			C3D_SetTexEnv(1, &coloroverride);
			draw.drawtexture(titleselecthighlighter, x - 9, y - 9);
			//Now we need to reset stage 1
			C3D_TexEnvInit(C3D_GetTexEnv(1));
		}
		i++;
		if (iter->titl != 0) //Not a null cartridge
			draw.drawSMDHicon((*iter).icon, x, y);
	}
	draw.drawtexture(titleselectionboxes, 26, 21);
	draw.drawframebuffer(prevbotfb, 0, 0, false, 0, -240, fbinterpfactor);
	draw.frameend();
}

void activetitleselect()
{
	C3D_Tex prevtop, prevbot;
	//Save the framebuffer from the previous menu
	C3D_TexInit(&prevtop, 256, 512, GPU_RGBA8);
	C3D_TexInit(&prevbot, 256, 512, GPU_RGBA8);
	draw.retrieveframebuffers(&prevtop, &prevbot);
	//A while after this was written, it was revamped...
	//Wait on the other thread to finish title loading
	SMDHworker.displayprogress();
	/*float popup = 0;
	if (!alltitlesareloaded())
	{
		//Unforunately we can't just have an "error" call because we still need to feed it the progress value
		while (aptMainLoop())
		{
			if (popup < 1) //Let it stay at max once we're done
			{
				popup += .06f;
				if (popup >= 1) popup = 1; //Prevent it from going overboard
			}
			int titlesloaded = getalltitlesloadedcount();
			int alltitlescount = getalltitlescount();
			drawprogresserror("Waiting for titles to be loaded...\n Title " + to_string(titlesloaded) + '/' + to_string(alltitlescount), \
				popup, (float)titlesloaded / alltitlescount, prevtop, prevbot);
			if (alltitlesareloaded())
			{
				while (popup > 0)
				{
					popup -= .06f;
					drawprogresserror("Waiting for titles to be loaded...\n Title " + to_string(titlesloaded) + '/' + to_string(alltitlescount), \
						popup, (float)titlesloaded / alltitlescount, prevtop, prevbot);
				}
				break;
			}
		}
	}*/

	allicons = getallSMDHdata();
	//Was having some seriously crazy bugs with this bit, not totally sure if it's fixed
	//so I'm leaving the debugging code here
	/*string isem = queueforactivation.empty() == true ? "true" : "false";
	error(isem);*/

	//We may need to add some titles to the list. Done here since loading is finished here
	while (!queueforactivation.empty())
	{
		u64 title = queueforactivation.front().first;
		int mediatype = queueforactivation.front().second;
		//We don't already have it, and it's not a cartridge.
		if (std::find(titleids.begin(), titleids.end(), title) == titleids.end() && mediatype == 1)
		{
			//It won't be active in the all titles vector so we've got to do that now
			vector<smdhdata>::iterator activepos = std::find_if(\
				getallSMDHdata().begin(), getallSMDHdata().end(), \
				[title](const smdhdata& data) {return data.titl == title; });
			if (!activepos->isactive && activepos != getallSMDHdata().end())
				activepos->isactive = true;
			getSMDHdata().push_back(*activepos);
			titleids.push_back(title);
			slots.push_back(1);
		}
		queueforactivation.pop();
	}

	static int selectpos = 0;
	alloldselectpos = selectpos;
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
		activetitleselectdraw(prevbot, fbinterpfactor, scrollsubtractrows, selectpos);
	}
	touchPosition tpos, opos;
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
				if (newselectpos <= (signed int)allicons.size() - 1)
				{
					selectpos = newselectpos;
				}
			}
			else if (buttonpressed(quadsofbuttons[i][0], quadsofbuttons[i][1], advancecount, advancecount, opos, kHeld))
			{
				//Do the calculation again to ensure we don't accidentally hit something bad
				int newselectpos = i + (4 * scrollsubtractrows);
				if (newselectpos <= (signed int)allicons.size() - 1)
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
			if (selectpos >= (signed int)allicons.size())
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
			else if (currow == 0 && scrollsubtractrows > 0)
				scrollsubtractrows--;
			//if (selectpos - scrollsubtractrows * 3 <= 12 && scrollsubtractrows > 0) //Will this work? 12?
		}
		if (kDown & KEY_DOWN)
		{
			int currow = (selectpos / 4) - scrollsubtractrows;
			selectpos += 4;
			//selectpos can't be unsigned due to logic preventing it from getting lower than 0
			if (selectpos >= (signed int)allicons.size())
				selectpos = allicons.size() - 1; //Whee off-by-one errors
			if (currow == 2)
				scrollsubtractrows++;
			//if (selectpos - scrollsubtractrows * 3 >= 12) //Will this work?
		}

		if (kDown & KEY_A)
		{
			selecttitle:
			if (selectpos != 0) //Not a cartridge, these rules don't apply to them
			{
				smdhdata& titleop = allicons.at(selectpos);
				if (titleop.isactive)
				{
					titleop.isactive = false;
					//Remove the title ID from the global entries
					vector<u64>::iterator i = std::find(titleids.begin(), titleids.end(), titleop.titl);
					vector<int>::iterator j = slots.begin();
					vector<smdhdata>::iterator k = getSMDHdata().begin();
					std::advance(j, i - titleids.begin()); //Get raw index, the two are in the same position
					std::advance(k, i - titleids.begin());
					titleids.erase(i);
					slots.erase(j);
					getSMDHdata().erase(k);
				}
				else
				{
					titleop.isactive = true;
					//Add it
					titleids.push_back(titleop.titl);
					slots.push_back(1); //Default
					getSMDHdata().push_back(allicons[selectpos]);
					//Add the folder if it doesn't exist
					if(!pathExist(modsfolder + tid2str(titleop.titl)))
						_mkdir((modsfolder + tid2str(titleop.titl)).c_str());
					if(issaltysdtitle(titleop.titl) && !pathExist("/luma/titles/" + tid2str(titleop.titl) + "/code.ips"))
						writeSaltySD(titleop.titl);
				}
			}
		}
		if (kDown & KEY_B)
			break;
		activetitleselectdraw(prevbot, fbinterpfactor, scrollsubtractrows, selectpos);
	}
	while (fbinterpfactor > 0)
	{
		fbinterpfactor -= 0.05;
		activetitleselectdraw(prevbot, fbinterpfactor, scrollsubtractrows, selectpos);
	}
}
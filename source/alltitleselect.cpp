//This is a clone of titleselect.cpp, but instead of picking a title to load mods for, it provides a menu
//to set the "ActiveTitleIDs" config value.
//I didn't want to repeat nearly-identical-but-not-quite code in one file, so it's here instead.
#include "titleselects.hpp"
#include "main.hpp"
#include "utils.hpp"
#include "error.hpp"
#include <algorithm>

//Actually initialized later, we can't use it before SMDH data is loaded
//but we can't declare a reference without initializing it.
//We're sure to wait until it's finished loading...
vector<smdhdata>& allicons = getallSMDHdata();
int alloldselectpos;

void activetitleselectdraw(C3D_Tex prevbotfb, float fbinterpfactor, int scrollsubtractrows, int selectpos, bool highlighterblink)
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
#define PLUSVALUE 5
	if (highlighteralphaplus)
	{
		highlighteralpha += PLUSVALUE;
		if (highlighteralpha > 255) { highlighteralpha -= PLUSVALUE; highlighteralphaplus = false; }
	}
	else
	{
		highlighteralpha -= PLUSVALUE;
		if (highlighteralpha < 0) { highlighteralpha += PLUSVALUE; highlighteralphaplus = true; }
	}
	y -= 70 * scrollsubtractrows;
	for (vector<smdhdata>::iterator iter = allicons.begin(); iter < allicons.end(); iter++)
	{
		x += 70;
		if (x > 241)
		{
			x = 31;
			y += 70;
		}
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
			//Apply the highlighter fade. There's an sdraw function for this but it shouldn't really exist,
			//configuring tev1 is the way it should be done.
			C3D_TexEnv* tev = C3D_GetTexEnv(1);
			C3D_TexEnvSrc(tev, C3D_RGB, GPU_CONSTANT);
			C3D_TexEnvSrc(tev, C3D_Alpha, GPU_PREVIOUS, GPU_CONSTANT);
			C3D_TexEnvOpRgb(tev, GPU_TEVOP_RGB_SRC_COLOR);
			C3D_TexEnvOpAlpha(tev, GPU_TEVOP_A_SRC_ALPHA);
			C3D_TexEnvFunc(tev, C3D_RGB, GPU_REPLACE);
			C3D_TexEnvFunc(tev, C3D_Alpha, GPU_MODULATE);
			C3D_TexEnvColor(tev, RGBA8(iter->isactive ? 0 : 255, 0, iter->isactive ? 255 : 0, highlighteralpha));
			/*C3D_TexEnv* tev = C3D_GetTexEnv(1);
			C3D_TexEnvSrc(tev, C3D_RGB, GPU_PREVIOUS, GPU_CONSTANT, GPU_CONSTANT);
			C3D_TexEnvOp(tev, C3D_RGB, GPU_TEVOP_RGB_SRC_COLOR, GPU_TEVOP_RGB_SRC_COLOR, GPU_TEVOP_RGB_ONE_MINUS_SRC_ALPHA);
			C3D_TexEnvSrc(tev, C3D_Alpha, GPU_PREVIOUS, 0, 0);
			C3D_TexEnvFunc(tev, C3D_RGB, GPU_INTERPOLATE);
			C3D_TexEnvFunc(tev, C3D_Alpha, GPU_REPLACE);
			C3D_TexEnvColor(tev, RGBA8(0, 0, 255, highlighteralpha));*/
			draw.drawtexture(titleselecthighlighter, highlighteroldx - 9, highlighteroldy - 9, x - 9, y - 9, highlighterinterpfactor);
			//Now we need to reset stage 1
			tev = C3D_GetTexEnv(1);
			C3D_TexEnvInit(tev);
		}
		else if (iter->isactive)
		{
			//Configure TexEnv stage 1 to "blink" the texture by making it all blue
			C3D_TexEnv* tev = C3D_GetTexEnv(1);
			C3D_TexEnvSrc(tev, C3D_RGB, GPU_CONSTANT);
			C3D_TexEnvSrc(tev, C3D_Alpha, GPU_PREVIOUS);
			C3D_TexEnvOpRgb(tev, GPU_TEVOP_RGB_SRC_COLOR);
			C3D_TexEnvOpAlpha(tev, GPU_TEVOP_A_SRC_ALPHA);
			C3D_TexEnvFunc(tev, C3D_RGB, GPU_REPLACE);
			C3D_TexEnvFunc(tev, C3D_Alpha, GPU_REPLACE);
			C3D_TexEnvColor(tev, RGBA8(0, 0, 255, 255));
			draw.drawtexture(titleselecthighlighter, x - 9, y - 9);
			//Now we need to reset stage 1
			tev = C3D_GetTexEnv(1);
			C3D_TexEnvInit(tev);
		}
		i++;
		if (iter == allicons.begin()) //It's the cartridge
		{
			//sdraw::drawicon(cartridgeicon, x - 12, y - 12);
			if (iter->titl == 0)
				continue; //There's no cartridge inserted
		}
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
	draw.framestart(); //Citro3D's rendering queue needs to be open for a TextureCopy
	GX_TextureCopy((u32*)draw.lastfbtop.data, GX_BUFFER_DIM((256 * 8 * 4) >> 4, 0), (u32*)prevtop.data, GX_BUFFER_DIM((256 * 8 * 4) >> 4, 0), 512 * 256 * 4, FRAMEBUFFER_TRANSFER_FLAGS);
	GX_TextureCopy((u32*)draw.lastfbbot.data, GX_BUFFER_DIM((256 * 8 * 4) >> 4, 0), (u32*)prevbot.data, GX_BUFFER_DIM((256 * 8 * 4) >> 4, 0), 512 * 256 * 4, FRAMEBUFFER_TRANSFER_FLAGS);
	gspWaitForPPF();
	//Well we do need to do something with this frame so let's draw the last one
	drawtopscreen();
	draw.drawon(GFX_BOTTOM);
	draw.drawframebuffer(prevbot, 0, 0, false);
	draw.frameend();
	//Wait on the other thread to finish title loading
	float popup = 0;
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
	}
	allicons = getallSMDHdata();
	static int selectpos = 0;
	alloldselectpos = selectpos;
	static int scrollsubtractrows = 0;

	float fbinterpfactor = 0;
	while (fbinterpfactor < 1)
	{
		fbinterpfactor += 0.05;
		activetitleselectdraw(prevbot, fbinterpfactor, scrollsubtractrows, selectpos, false);
	}
	while (aptMainLoop())
	{
		if (cartridgeneedsupdating)
			updatecartridgedata();
		hidScanInput();
		u32 kDown = hidKeysDown();
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
					slots.push_back(0); //Default
					getSMDHdata().push_back(allicons[selectpos]);
				}
			}
		}
		if (kDown & KEY_B)
			break;
		activetitleselectdraw(prevbot, fbinterpfactor, scrollsubtractrows, selectpos, false);
	}
	while (fbinterpfactor > 0)
	{
		fbinterpfactor -= 0.05;
		activetitleselectdraw(prevbot, fbinterpfactor, scrollsubtractrows, selectpos, false);
	}
}
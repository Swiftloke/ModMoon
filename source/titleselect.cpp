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

	highlighterhandle(highlighteralpha, highlighteralphaplus);
	y -= 70 * scrollsubtractrows;
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
			draw.drawtext(tid2str((*iter).titl).c_str(), 0, 0, .4, .4);
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
				//Configure TexEnv stage 1 to "blink" the texture by making it all blue
				C3D_TexEnv* tev = C3D_GetTexEnv(1);
				C3D_TexEnvSrc(tev, C3D_RGB, GPU_CONSTANT);
				C3D_TexEnvSrc(tev, C3D_Alpha, GPU_PREVIOUS);
				C3D_TexEnvOpRgb(tev, GPU_TEVOP_RGB_SRC_COLOR);
				C3D_TexEnvOpAlpha(tev, GPU_TEVOP_A_SRC_ALPHA);
				C3D_TexEnvFunc(tev, C3D_RGB, GPU_REPLACE);
				C3D_TexEnvFunc(tev, C3D_Alpha, GPU_REPLACE);
				C3D_TexEnvColor(tev, RGBA8(0, 0, 255, 0));
			}
			//Apply the highlighter fade. There's an sdraw function for this but it shouldn't really exist,
			//configuring tev1 is the way it should be done.
			/*else 
			{
				C3D_TexEnv* tev = C3D_GetTexEnv(1);
				C3D_TexEnvSrc(tev, C3D_RGB, GPU_PREVIOUS);
				C3D_TexEnvSrc(tev, C3D_Alpha, GPU_PREVIOUS, GPU_CONSTANT);
				C3D_TexEnvOpRgb(tev, GPU_TEVOP_RGB_SRC_COLOR);
				C3D_TexEnvOpAlpha(tev, GPU_TEVOP_A_SRC_ALPHA, GPU_TEVOP_A_SRC_ALPHA);
				C3D_TexEnvFunc(tev, C3D_RGB, GPU_REPLACE);
				C3D_TexEnvFunc(tev, C3D_Alpha, GPU_MODULATE);
				C3D_TexEnvColor(tev, RGBA8(0, 0, 0, highlighteralpha));
				//C3D_TexEnv* tev = C3D_GetTexEnv(1);
				//C3D_TexEnvSrc(tev, C3D_RGB, GPU_PREVIOUS, GPU_CONSTANT, GPU_CONSTANT);
				//C3D_TexEnvOp(tev, C3D_RGB, GPU_TEVOP_RGB_SRC_COLOR, GPU_TEVOP_RGB_SRC_COLOR, GPU_TEVOP_RGB_ONE_MINUS_SRC_ALPHA);
				//C3D_TexEnvSrc(tev, C3D_Alpha, GPU_PREVIOUS, 0, 0);
				//C3D_TexEnvFunc(tev, C3D_RGB, GPU_INTERPOLATE);
				//C3D_TexEnvFunc(tev, C3D_Alpha, GPU_REPLACE);
				/C3D_TexEnvColor(tev, RGBA8(0, 0, 255, highlighteralpha));
			}*/
			//If the highlighter blink is active, we need to give a full alpha, seeing as it's "blinking"
			draw.drawhighlighter(titleselecthighlighter, highlighteroldx - 9, highlighteroldy - 9, \
				highlighterblink ? 255 : highlighteralpha, x - 9, y - 9, highlighterinterpfactor);
			//Now we need to reset stage 1
			if (highlighterblink)
			{
				C3D_TexEnv* tev = C3D_GetTexEnv(1);
				C3D_TexEnvInit(tev);
			}
		}
		i++;
		if(iter->titl != 0) //Not a null cartridge
			draw.drawSMDHicon((*iter).icon, x, y);
	}
	draw.drawtexture(titleselectionboxes, 26, 21);
	draw.drawframebuffer(prevfb, 0, 0, false, 0, -240, fbinterpfactor);
	draw.frameend();
}

void titleselect()
{
	C3D_Tex prevbot;
	//Save the framebuffer from the previous menu
	C3D_TexInit(&prevbot, 256, 512, GPU_RGBA8);
	draw.retrieveframebuffers(NULL, &prevbot);
	icons = getSMDHdata();
	static int selectpos = currenttidpos;
	oldselectpos = selectpos;
	static int scrollsubtractrows = 0;

	float fbinterpfactor = 0;
	while (fbinterpfactor < 1)
	{
		fbinterpfactor += 0.05;
		titleselectdraw(prevbot, fbinterpfactor, scrollsubtractrows, selectpos, false);
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
				error("Place them at " + modsfolder + '\n' + currenttitleidstr + "/Slot_X\nWhere X is a number starting at 1.");
			}
			mainmenuupdateslotname();
			config.write("SelectedTitleIDPos", currenttidpos);
			break;
		}
		if (kDown & KEY_B)
			break;
		titleselectdraw(prevbot, fbinterpfactor, scrollsubtractrows, selectpos, false);
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
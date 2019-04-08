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

#include "error.hpp"
#include "main.hpp"
#include "utils.hpp"

#include <cstring>

#define TEXTSCALE 0.65

C3D_Tex prevtop, prevbot;

ERRORMODE errormode;

bool startwaspressed = false;

void errorsetmode(ERRORMODE mode)
{
	errormode = mode;
}

bool errorwasstartpressed()
{
	return startwaspressed;
}

void drawerrorbox(string text, int alphapos, float expandpos)
{
	sdraw::framestart();
	sdraw::MM::shader_basic->bind();
	sdraw::setfs("texture");
	sdraw::drawframebuffer(prevtop, 0, 0, true);
	sdraw::drawon(GFX_BOTTOM);
	sdraw::drawframebuffer(prevbot, 0, 0, false);
	sdraw::MM::shader_eventual->bind();
	sdraw::MM::shader_eventual->setUniformF("base", 320 / 2, 240 / 2);
	sdraw::MM::shader_eventual->setUniformF("expansion", expandpos);
	sdraw::drawtexture(textbox, 10, 20);
	//y = (240/2 - 20) - height of one line (sdraw function returns height of all lines combined, something I don't want here
	sdraw::drawcenteredtext(text.c_str(), TEXTSCALE, TEXTSCALE, 100 - (TEXTSCALE * fontGetInfo()->lineFeed));
	sdraw::drawtexture(textboxokbutton, 112, 163);
	//I did it this way before I wrote the stencil test highlighter, and besides which that wouldn't work because it uses
	//The eventual shader itself so the popup wouldn't show properly for this
	sdraw::setfs("highlighter", 0, HIGHLIGHTERCOLORANDALPHA(textboxokbuttonhighlight.highlightercolor, alphapos));
	sdraw::drawtexture(textboxokbuttonhighlight, 111, 162);
	sdraw::frameend();
}

void drawerrorfade(string text, int alphapos, float fadepos)
{
	sdraw::framestart();
	//sdraw::usebasicshader();
	sdraw::drawframebuffer(prevtop, 0, 0, true);
	int fade = fadepos * 127;
	sdraw::setfs("constColor", 0, RGBA8(0, 0, 0, fade));
	sdraw::drawrectangle(0, 0, 400, 240);
	sdraw::drawon(GFX_BOTTOM);
	sdraw::setfs("texture", 0);
	sdraw::drawframebuffer(prevbot, 0, 0, false);
	sdraw::setfs("constColor", 0, RGBA8(0, 0, 0, fade));
	sdraw::drawrectangle(0, 0, 320, 240, true);
	/*sdraw::useeventualshader();
	C3D_FVUnifSet(GPU_VERTEX_SHADER, sdraw::expand_baseloc, 320 / 2, 240 / 2, 0, 0);
	C3D_FVUnifSet(GPU_VERTEX_SHADER, sdraw::expand_expandloc, expandpos, 0, 0, 0);
	sdraw::drawtexture(textbox, 10, 20);*/
	//y = (240/2 - 20) - height of one line (sdraw function returns height of all lines combined, something I don't want here
	sdraw::setfs("textColor", 0, RGBA8(255, 255, 255, fade * 2));
	sdraw::drawcenteredtext(text.c_str(), TEXTSCALE, TEXTSCALE, 100 - (TEXTSCALE * fontGetInfo()->lineFeed));
	sdraw::drawtexture(textboxokbutton, 112, 163);
	//I did it this way before I wrote the stencil test highlighter, and besides which that wouldn't work because it uses
	//The eventual shader itself so the popup wouldn't show properly for this
	sdraw::setfs("highlighter", 0, HIGHLIGHTERCOLORANDALPHA(textboxokbuttonhighlight.highlightercolor, alphapos));
	sdraw::drawtexture(textboxokbuttonhighlight, 111, 162);
	sdraw::frameend();
}

bool handleerror(float expandpos, string text)
{
	static int alphapos = 0;
	static bool alphaplus = true;
	static touchPosition currentpos, lastpos;
	highlighterhandle(alphapos, alphaplus);
	if(errormode == MODE_POPUP)
		drawerrorbox(text, alphapos, expandpos);
	else if(errormode == MODE_FADE)
		drawerrorfade(text, alphapos, expandpos);
	hidScanInput();
	u32 kDown = hidKeysDown();
	hidTouchRead(&currentpos);
	//Button pressed and the text box has fully popped up (we do, after all, want the user to actually read this thing...)
	if ((buttonpressed(textboxokbutton, 112, 163, lastpos, hidKeysHeld()) || kDown & KEY_A) && expandpos >= 1)
		return true;
	if (kDown & KEY_START)
	{
		startwaspressed = true;
		return true;
	}
	lastpos = currentpos;
	return false;
}

void error(string text)
{
	startwaspressed = false;
	//Fade mode HAS to be used, no matter what, seeing as it's used in the tutorial
	if(shoulddisableerrors && errormode != MODE_FADE)
		return;
	//Save the framebuffers from the previous menu
	C3D_TexInit(&prevtop, 256, 512, GPU_RGBA8);
	C3D_TexInit(&prevbot, 256, 512, GPU_RGBA8);
	sdraw::retrieveframebuffers(&prevtop, &prevbot);
	//In the rare case that fade mode is used, this instead becomes the alpha of the fade
	float expandpos = 0;

	while (aptMainLoop())
	{
		if (expandpos < 1) //Let it stay at max once we're done
		{
			expandpos += .06f;
			if (expandpos >= 1) expandpos = 1; //Prevent it from going overboard
		}
		if (handleerror(expandpos, text)) break; //Break out if we're done
	}
	//Close the text box, clean up
	while (expandpos > 0)
	{
		expandpos -= .06f;
		handleerror(expandpos, text);
	}
	handleerror(0, text);
	sdraw::MM::shader_basic->bind();
	C3D_TexDelete(&prevtop);
	C3D_TexDelete(&prevbot);
}

/*
Stencil test the progress bar
Constantly increase the x texcoord to animate it
In handle, if it suddenly jumps smooth it... How
First two vertices have same coordinates for both positions, second two have them at the end of the progress bar
*/
void drawprogresserror(string text, float expandpos, float progress, C3D_Tex topfb, C3D_Tex botfb)
{
	static float texcoordplus = 0; //Constantly increase this for an animation
	texcoordplus += 0.005;
	sdraw::framestart();
	sdraw::MM::shader_basic->bind();
	if (topfb.height)
	{
		sdraw::setfs("texture");
		sdraw::drawframebuffer(topfb, 0, 0, true);
	}
	else
	{
		sdraw::setfs("constColor", 0, RGBA8(0, 0, 0, 255));
		sdraw::drawrectangle(0, 0, 400, 240);
	}
	sdraw::drawon(GFX_BOTTOM);
	if (botfb.height)
	{
		sdraw::setfs("texture");
		sdraw::drawframebuffer(botfb, 0, 0, false);
	}
	else
	{
		sdraw::setfs("constColor", 0, RGBA8(0, 0, 0, 255));
		sdraw::drawrectangle(0, 0, 320, 240);
	}
	sdraw::MM::shader_eventual->bind();
	sdraw::MM::shader_eventual->setUniformF("base", 320 / 2, 240 / 2);
	sdraw::MM::shader_eventual->setUniformF("expansion", expandpos);
	sdraw::setfs("texture", 0);
	sdraw::drawtexture(textbox, 10, 20);
	//y = (240/2 - 20) - height of one line (sdraw function returns height of all lines combined, something I don't want here
	sdraw::drawcenteredtext(text.c_str(), TEXTSCALE, TEXTSCALE, 100 - (TEXTSCALE * fontGetInfo()->lineFeed));
	//Progress bar

	int x = 30, y = 150;
	sdraw::drawtexture(progressbar, x, y);
	//Enable writing to the stencil buffer and draw the texture
	sdraw::stenciltest(true, GPU_ALWAYS, 1, 0xFF, 0xFF);
	sdraw::stencilop(GPU_STENCIL_KEEP, GPU_STENCIL_KEEP, GPU_STENCIL_REPLACE);
	sdraw::drawtexture(progressbarstenciltex, x, y);
	sdraw::stenciltest(true, GPU_EQUAL, 1, 0xFF, 0x00); //Turn off writes and allow a pass if it has been set
	//Calculate the right side's texcoord of how much we need to repeat for the texture to look right
	float rightsidex = ((1 - progress) * x) + (progress * (x + 260));
	float rightsidetexcoord = rightsidex / 32;
	sdraw::MM::shader_twocoords->bind();
	sdraw::MM::shader_twocoords->setUniformF("interpfactor", progress);
	sdraw::MM::shader_twocoords->setUniformF("base", 320 / 2, 240 / 2);
	sdraw::MM::shader_twocoords->setUniformF("baseinterpfactor", expandpos);
	sdraw::bindtex(0, progressfiller);

	sdraw::addVertex(x, y + 35, texcoordplus, 1, x, y + 35);
	sdraw::addVertex(x, y + 35, texcoordplus + rightsidetexcoord, 1, x + 260, y + 35);
	sdraw::addVertex(x, y, texcoordplus, 0, x, y);

	sdraw::addVertex(x, y, texcoordplus, 0, x, y);
	sdraw::addVertex(x, y + 35, texcoordplus + rightsidetexcoord, 1, x + 260, y + 35);
	sdraw::addVertex(x, y, texcoordplus + rightsidetexcoord, 0, x + 260, y);

	sdraw::drawCall();
	//TexEnv for basic texture is already set from the last drawtexture call so we don't need to bother
	sdraw::stenciltest(false, GPU_NEVER, 0, 0, 0); //Turn off the stencil test
	sdraw::MM::shader_basic->bind();
	sdraw::frameend();
}

void popupprogressbar(string text)
{

}
//Text box: 10, 20
//OK button (solo): 112, 163
//Highlighter: 111, 162
#include "main.hpp"
#include "sdraw.hpp"

#include <cstring>

#define ALPHAPLUSVALUE 5
C3D_Tex prevtop, prevbot;

void drawerrorbox(string text, int alphapos, float expandpos)
{
	draw.framestart();
	draw.usebasicshader();
	draw.drawframebuffer(prevtop, 0, 0, true);
	draw.drawon(GFX_BOTTOM);
	draw.drawframebuffer(prevbot, 0, 0, false);
	draw.useeventualshader();
	C3D_FVUnifSet(GPU_VERTEX_SHADER, draw.expand_baseloc, 320 / 2, 240 / 2, 0, 0);
	C3D_FVUnifSet(GPU_VERTEX_SHADER, draw.expand_expandloc, expandpos, 0, 0, 0);
	draw.drawtexture(textbox, 10, 20);
#define TEXTSCALE 0.7
	//y = (240/2 - 20) - height of one line (sdraw function returns height of all lines combined, something I don't want here
	draw.drawcenteredtext(text.c_str(), TEXTSCALE, TEXTSCALE, 100 - (TEXTSCALE * fontGetInfo()->lineFeed));
	draw.drawtexture(textboxokbutton, 112, 163);
	//I did it this way before I wrote the stencil test highlighter, and besides which that wouldn't work because it uses
	//The eventual shader itself so the popup wouldn't show properly for this
	draw.drawtexture_replacealpha(textboxokbuttonhighlight, 111, 162, alphapos);
	draw.frameend();
}

bool handleerror(float expandpos, string text)
{
	static int alphapos = 0;
	static bool alphaplus = true;
	static touchPosition currentpos, lastpos;
	if (alphaplus)
	{
		alphapos += ALPHAPLUSVALUE;
		if (alphapos > 255) { alphapos -= ALPHAPLUSVALUE; alphaplus = false; }
	}
	else
	{
		alphapos -= ALPHAPLUSVALUE;
		if (alphapos < 0) { alphapos += ALPHAPLUSVALUE; alphaplus = true; }
	}
	drawprogresserror(text, alphapos, expandpos, .5, prevtop, prevbot);
	hidScanInput();
	hidTouchRead(&currentpos);
	//Button pressed and the text box has fully popped up (we do, after all, want the user to actually read this thing...)
	if ((buttonpressed(textboxokbutton, 112, 163, lastpos, hidKeysHeld()) || hidKeysDown() & KEY_A) && expandpos >= 1)
		return true;
	lastpos = currentpos;
	return false;
}

void error(string text)
{
	//Save the framebuffers from the previous menu
	C3D_TexInit(&prevtop, 256, 512, GPU_RGBA8);
	C3D_TexInit(&prevbot, 256, 512, GPU_RGBA8);
	draw.framestart(); //Citro3D's rendering queue needs to be open for a TextureCopy
	GX_TextureCopy((u32*)draw.lastfbtop.data, GX_BUFFER_DIM((256 * 8 * 4) >> 4, 0), (u32*)prevtop.data, GX_BUFFER_DIM((256 * 8 * 4) >> 4, 0), 512 * 256 * 4, FRAMEBUFFER_TRANSFER_FLAGS);
	//gspWaitForPPF(); Hangs?
	GX_TextureCopy((u32*)draw.lastfbbot.data, GX_BUFFER_DIM((256 * 8 * 4) >> 4, 0), (u32*)prevbot.data, GX_BUFFER_DIM((256 * 8 * 4) >> 4, 0), 512 * 256 * 4, FRAMEBUFFER_TRANSFER_FLAGS);
	gspWaitForPPF();
	//We need to do something with this frame so let's draw the last one
	draw.drawframebuffer(prevtop, 0, 0, true);
	draw.drawon(GFX_BOTTOM);
	draw.drawframebuffer(prevbot, 0, 0, false);
	draw.frameend();
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
	draw.usebasicshader();
}

/*
Stencil test the progress bar
Constantly increase the x texcoord to animate it
In handle, if it suddenly jumps smooth it... How
First two vertices have same coordinates for both positions, second two have them at the end of the progress bar
*/
void drawprogresserror(string text, int alphapos, float expandpos, float progress, C3D_Tex prevtopfb, C3D_Tex prevbotfb)
{
	static float texcoordplus = 0; //Constantly increase this for an animation
	texcoordplus += 0.005;
	draw.framestart();
	draw.usebasicshader();
	draw.drawframebuffer(prevtopfb, 0, 0, true);
	draw.drawon(GFX_BOTTOM);
	draw.drawframebuffer(prevbotfb, 0, 0, false);
	draw.useeventualshader();
	C3D_FVUnifSet(GPU_VERTEX_SHADER, draw.expand_baseloc, 320 / 2, 240 / 2, 0, 0);
	C3D_FVUnifSet(GPU_VERTEX_SHADER, draw.expand_expandloc, expandpos, 0, 0, 0);
	draw.drawtexture(textbox, 10, 20);
#define TEXTSCALE 0.7
	//y = (240/2 - 20) - height of one line (sdraw function returns height of all lines combined, something I don't want here
	draw.drawcenteredtext(text.c_str(), TEXTSCALE, TEXTSCALE, 100 - (TEXTSCALE * fontGetInfo()->lineFeed));
	//Progress bar

	int x = 30, y = 150;
	draw.drawtexture(progressbar, x, y);
	//Enable writing to the stencil buffer and draw the texture
	C3D_StencilTest(true, GPU_ALWAYS, 1, 0xFF, 0xFF);
	C3D_StencilOp(GPU_STENCIL_KEEP, GPU_STENCIL_KEEP, GPU_STENCIL_REPLACE);
	draw.drawtexture(progressbarstenciltex, x, y);
	C3D_StencilTest(true, GPU_EQUAL, 1, 0xFF, 0x00); //Turn off writes and allow a pass if it has been set
	//Calculate the right side's texcoord of how much we need to repeat for the texture to look right
	float rightsidex = ((1 - progress) * x) + (progress * (x + 260));
	float rightsidetexcoord = rightsidex / 32;
	draw.usetwocoordsshader();
	C3D_FVUnifSet(GPU_VERTEX_SHADER, draw.twocds_interploc, progress, 0, 0, 0);
	C3D_FVUnifSet(GPU_VERTEX_SHADER, draw.twocds_baseloc, 320 / 2, 240 / 2, 0, 0);
	C3D_FVUnifSet(GPU_VERTEX_SHADER, draw.twocds_baseinterploc, expandpos, 0, 0, 0);
	draw.sDrawi_addTwoCoordsVertex(x, y, x, y, texcoordplus, 0);
	draw.sDrawi_addTwoCoordsVertex(x, y + 35, x, y + 35, texcoordplus, 1);
	draw.sDrawi_addTwoCoordsVertex(x, y, x + 260, y, texcoordplus + rightsidetexcoord, 0);
	draw.sDrawi_addTwoCoordsVertex(x, y + 35, x + 260, y + 35, texcoordplus + rightsidetexcoord, 1);
	C3D_TexBind(0, &progressfiller->image);
	//TexEnv for basic texture is already set from the last drawtexture call so we don't need to bother
	C3D_DrawArrays(GPU_TRIANGLE_STRIP, draw.sdrawTwoCdsVtxArrayPos-4, 4);
	C3D_StencilTest(false, GPU_NEVER, 0, 0, 0); //Turn off the stencil test
	draw.usebasicshader();
	draw.frameend();
}

void popupprogressbar(string text)
{

}
//Text box: 10, 20
//OK button (solo): 112, 163
//Highlighter: 111, 162
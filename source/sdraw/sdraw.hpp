/*
sDraw: A fully featured, reasonably designed rendering engine for the 3DS.
It was created specifically for ModMoon and uses tons of neat tricks, taking advantage
of the 3DS' power using the Citro3D library much more powerfully than most other GPU-using code out there.
Stuff that uses the GPU on the 3DS, especially using it well(ish) and with all the tricks this engine has,
is hard to come by, if not nonexistent, so this code is pretty awesome. It's a great example if you're trying
to learn to use Citro3D.
Well, mostly. It has some design flaws. If you're reading this code, TexEnv-vertex coupling in functions
is NOT a good idea; this holds true for fragment shading in most every other environment.
Furthermore, DrawArrays calls for every draw function is a terrible idea. 
Citro2D and new-hbmenu do this job properly.

ModMoon version 3.1's main goal is to fix every design flaw in sDraw. Please check the GitHub project page
for more information on this work.

...
This is probably the code I'm the most proud of in this entire project. I learned
graphics programming with this engine. Happy reading.
If you're considering using this code for yourself, I'd recommend against it. Citro2D is probably 
better for your own projects.
*/

#pragma once
#include <3ds.h>
#include <citro3d.h>
#include <tex3ds.h>

#include <vector>
#include <string>
#include <utility>

//#include "fragment.hpp" //See below
using namespace std;

#define CENTERED 1920 //A random number that no one will ever use realistically
#define RGBA8(r, g, b, a) ((((r)&0xFF)<<0) | (((g)&0xFF)<<8) | (((b)&0xFF)<<16) | (((a)&0xFF)<<24))
#define HIGHLIGHTERCOLORANDALPHA(color, alpha) ((color & 0xFFFFFF) | ((alpha & 0xFF) << 24))

#define FRAMEBUFFER_TRANSFER_FLAGS GX_TRANSFER_RAW_COPY(1)


struct sdraw_Vertex { float position[3]; float texcoord[2]; };
struct sdraw_TwoCdsVertex { float pos1[3]; float pos2[3]; float texcoord[2]; };
struct sdraw_ThreeTexVertex {float position[3]; float tc0[2]; float tc1[2]; float tc2[2]; };

struct sdraw_stex
{

	sdraw_stex(C3D_Tex* inputsheet, float posx, float posy, int inwidth, int inheight, bool optionalusesdarkmode = false) : \
		spritesheet(inputsheet), width(inwidth), height(inheight),
		//Spritesheet coords calculation
		botleft{ posx / spritesheet->width, posy / spritesheet->height },
		botright{ (posx + width) / spritesheet->width, posy / spritesheet->height },
		topleft{ posx / spritesheet->width, (posy + height) / spritesheet->height },
		topright{ (posx + width) / spritesheet->width, (posy + height) / spritesheet->height },
		//Dark mode
		usesdarkmode(optionalusesdarkmode) {};

	sdraw_stex(unsigned int subtexture, std::pair<C3D_Tex*, Tex3DS_Texture> input, bool inusesdarkmode = false) : \
		spritesheet(input.first), usesdarkmode(inusesdarkmode)
	{
		Tex3DS_Texture& t3x = input.second;
		if (subtexture > Tex3DS_GetNumSubTextures(t3x))
		{
			width = 0; height = 0;
			return;
		}
		const Tex3DS_SubTexture* coords = Tex3DS_GetSubTexture(t3x, subtexture);
		//Would like to have these in my own struct, especially for portability
		Tex3DS_SubTextureBottomLeft (coords, &botleft[0],  &botleft[1]);
		Tex3DS_SubTextureBottomRight(coords, &botright[0], &botright[1]);
		Tex3DS_SubTextureTopLeft	(coords, &topleft[0],  &topleft[1]);
		Tex3DS_SubTextureTopRight	(coords, &topright[0], &topright[1]);
		width = coords->width;
		height = coords->height;
	}

	C3D_Tex* spritesheet;
	int width, height;
	float botleft[2];
	float botright[2];
	float topleft[2];
	float topright[2];
	//Many UI elements need this disabled; most that don't are in the spritesheet, so this is a sane place to put it.
	bool usesdarkmode;
};

//Unfortunately, I can't put this at the top, due to this file requiring that sdraw_stex is defined :/
#include "fragment.hpp"

struct sdraw_highlighter : public sdraw_stex
{
	u32 highlightercolor;

	sdraw_highlighter(unsigned int subtexture, std::pair<C3D_Tex*, Tex3DS_Texture> input, u32 incolor, \
		bool optionalusesdarkmode = false) : sdraw_stex(subtexture, input, optionalusesdarkmode), \
		highlightercolor(incolor) {};
};


namespace sdraw
{
	int init();
	void cleanup();
	void drawon(gfxScreen_t window);
	void framestart();
	void drawtexture(C3D_Tex* tex, int x, float y);
	void drawtexture(sdraw_stex info, int x, int y, int x1 = -1, int y1 = -1, float interpfactor = 0); //Second coords are optional
	void drawframebuffer(C3D_Tex tex, int x, int y, bool istopfb, int x1 = -1, int y1 = -1, float interpfactor = 0);
	//Second coords functionality of this is broken
	void drawtexturewithhighlight(sdraw_stex info, int x, int y, u32 color, int alpha, int x1 = -1, int y1 = -1, float interpfactor = 0);
	void drawSMDHicon(C3D_Tex icon, int x, int y);
	void drawtext(const char* text, float x, float y, float sizeX, float sizeY);
	void drawtextinrec(const char* text, int x, int y, int width, float scalex, float scaley);
	void drawcenteredtext(const char* text, float scaleX, float scaleY, float y);
	void drawrectangle(int x, int y, int width, int height, u32 color, bool shouldusedarkmode = false);
	void drawmultipletextures(int x, int y, sdraw_stex info1, sdraw_stex info2, sdraw_stex info3);
	void frameend();
	float gettextheight(const char* text, float sizeY);
	vector<float> gettextwidths(const char* text, float sizeX, float sizeY);
	float gettextmaxwidth(const char* text, float sizeX, float sizeY);
	
	void drawquad(sdraw_stex info, int x, int y, int x1 = -1, int y1 = -1, float interpfactor = 0);
	void usebasicshader();
	void useeventualshader();
	void usetwocoordsshader();
	void usethreetexturesshader();
	void sDrawi_addTextVertex(float vx, float vy, float tx, float ty);
	void sDrawi_addTwoCoordsVertex(float vx1, float vy1, float vx2, float vy2, float tx, float ty);
	void sDrawi_addThreeTexturesVertex(float vx, float vy, float tc0x, float tc0y, float tc1x, float tc1y, float tc2x, float tc2y);

	//Copies last frame to provided textures of 256x512 dimensions.
	void retrieveframebuffers(C3D_Tex* topfb, C3D_Tex* botfb);

	//Sets up "dark mode". This inverts all colors of the UI, except pieces that need
	//to be displayed normally. The bool mentions whether dark mode is enabled at all,
	//the function enables it (if it needs to be, based on the bool).
	extern bool darkmodeshouldactivate;
	void enabledarkmode(bool enabled);
	
	extern int expand_baseloc, expand_expandloc;
	extern int twocds_interploc, twocds_baseloc, twocds_baseinterploc;
	extern int sdrawTwoCdsVtxArrayPos;
	extern int sdrawVtxArrayPos;
	extern int sdrawThreeTexturesVtxArrayPos;

	extern C3D_Tex lastfbtop;
	extern C3D_Tex lastfbbot;
	extern gfxScreen_t currentoutput;
	void sDrawi_renderText(float x, float y, float scaleX, float scaleY, bool baseline, const char* text);
} //namespace sdraw


C3D_Tex* loadpng(string filepath);
C3D_Tex* loadbin(string filepath, int width, int height);
std::pair<C3D_Tex*, Tex3DS_Texture> loadTextureFromFile(const char* filename);
unsigned int nextPow2(unsigned int v);
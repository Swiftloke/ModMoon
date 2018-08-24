/*
sDraw: A fully featured, reasonably designed rendering engine for the 3DS.
It was created specifically for ModMoon and uses tons of neat tricks, taking advantage of every bit
of the 3DS' power using the Citro3D library.
Stuff that uses the GPU on the 3DS, especially using it well and with all the tricks this engine has,
is hard to come by, if not nonexistent, so this code is pretty awesome. It's a great example if you're trying
to learn to use Citro3D.
Well, mostly. It has some design flaws. If you're reading this code, TexEnv-vertex coupling in functions
is NOT a good idea; this holds true for fragment shading in most every other environment.
Furthermore, DrawArrays calls for every draw function is a terrible idea, and it's kind of late to fix it
so it'll probably stay this way. Citro2D and new-hbmenu do this job properly.
...
This is probably the code I'm the most proud of in this entire project. I learned
graphics programming with this engine. Happy reading.
Oh, but if you're considering using this code for yourself, I'd recommend against it. Citro2D is probably 
better for your own projects.
*/

//Hey, maybe provide an optional argument for a C3D_TexEnv?

#pragma once
#include <3ds.h>
#include <citro3d.h>

#include <vector>
#include <string>
using namespace std;

#define CENTERED 1920 //A random number that no one will ever use realistically
#define RGBA8(r, g, b, a) ((((r)&0xFF)<<0) | (((g)&0xFF)<<8) | (((b)&0xFF)<<16) | (((a)&0xFF)<<24))

#define FRAMEBUFFER_TRANSFER_FLAGS GX_TRANSFER_RAW_COPY(1)


struct sdraw_Vertex { float position[3]; float texcoord[2]; };
struct sdraw_TwoCdsVertex { float pos1[3]; float pos2[3]; float texcoord[2]; };
struct sdraw_ThreeTexVertex {float position[3]; float tc0[2]; float tc1[2]; float tc2[2]; };

struct sdraw_stex
{

	sdraw_stex(C3D_Tex* inputsheet, float posx, float posy, int inwidth, int inheight, bool optionalusesdarkmode = false) : \
		spritesheet(inputsheet), x(posx), y(posy), width(inwidth), height(inheight), usesdarkmode(optionalusesdarkmode) {}
	C3D_Tex* spritesheet;
	float x, y, width, height;
	//Many UI elements need this disabled; most that don't are in the spritesheet, so this is a sane place to put it.
	bool usesdarkmode;
};

struct sdraw_highlighter : public sdraw_stex
{
	u32 highlightercolor;

	sdraw_highlighter(C3D_Tex* inputsheet, int posx, int posy, int width, int height, u32 incolor, \
		bool optionalusesdarkmode = false) : sdraw_stex(inputsheet, posx, posy, width, height, optionalusesdarkmode), \
		highlightercolor(incolor) {};
};


class sDraw_interface
{
	public:
	sDraw_interface();
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
	void settextcolor(u32 color);
	float gettextheight(const char* text, float sizeY);
	vector<float> gettextwidths(const char* text, float sizeX, float sizeY);
	float gettextmaxwidth(const char* text, float sizeX, float sizeY);
	
	void drawblendedtexture(C3D_Tex* texture1, C3D_Tex* texture2, int x, int y, int blendfactor);
	void drawhighlighter(sdraw_highlighter info, int x, int y, int alpha, int x1 = -1, int y1 = -1, float interpfactor = 0);
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
	bool darkmodeshouldactivate = false;
	void enabledarkmode(bool enabled);
	
	int expand_baseloc, expand_expandloc;
	int twocds_interploc, twocds_baseloc, twocds_baseinterploc;
	int sdrawTwoCdsVtxArrayPos;
	int sdrawVtxArrayPos;
	int sdrawThreeTexturesVtxArrayPos;
	
	private:
	C3D_Tex lastfbtop;
	C3D_Tex lastfbbot;
	gfxScreen_t currentoutput = GFX_TOP;
	void sDrawi_renderText(float x, float y, float scaleX, float scaleY, bool baseline, const char* text);
};


C3D_Tex* loadpng(string filepath);
C3D_Tex* loadbin(string filepath, int width, int height);
unsigned int nextPow2(unsigned int v);
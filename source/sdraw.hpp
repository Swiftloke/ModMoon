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

struct sdraw_texture
{
	int id;
	unsigned int width, height;
	C3D_Tex image;
};

struct sdraw_stex
{
	sdraw_stex(sdraw_texture* inputsheet, int posx, int posy, int width, int height);
	sdraw_texture* spritesheet;
	float x, y, width, height;
};


class sDraw_interface
{
	public:
	sDraw_interface();
	void cleanup();
	void drawon(gfxScreen_t window);
	void framestart();
	void drawtexture(sdraw_texture* tex, int x, float y);
	void drawtexture(sdraw_stex info, int x, int y, int x1 = -1, int y1 = -1, float interpfactor = 0); //Second coords are optional
	void drawframebuffer(C3D_Tex tex, int x, int y, bool istopfb, int x1 = -1, int y1 = -1, float interpfactor = 0);
	void drawtexturewithhighlight(sdraw_stex info, int x, int y, int alpha);
	void drawSMDHicon(C3D_Tex icon, int x, int y);
	void drawtext(const char* text, float x, float y, float sizeX, float sizeY);
	void drawtextinrec(const char* text, int x, int y, int width, float scalex, float scaley);
	void drawcenteredtext(const char* text, float scaleX, float scaleY, float y);
	void drawrectangle(int x, int y, int width, int height, u32 color);
	void frameend();
	void settextcolor(u32 color);
	float gettextheight(const char* text, float sizeY);
	vector<float> gettextwidths(const char* text, float sizeX, float sizeY);
	float gettextmaxwidth(const char* text, float sizeX, float sizeY);
	
	void drawblendedtexture(sdraw_texture* texture1, sdraw_texture* texture2, int x, int y, int blendfactor);
	void drawtexture_replacealpha(sdraw_stex info, int x, int y, int alpha, int x1 = -1, int y1 = -1, float interpfactor = 0);
	void drawquad(sdraw_stex info, int x, int y, int x1 = -1, int y1 = -1, float interpfactor = 0);
	void usebasicshader();
	void useeventualshader();
	void usetwocoordsshader();
	void sDrawi_addTextVertex(float vx, float vy, float tx, float ty);
	void sDrawi_addTwoCoordsVertex(float vx1, float vy1, float vx2, float vy2, float tx, float ty);

	//Copies last frame to provided textures of 256x512 dimensions.
	void retrieveframebuffers(C3D_Tex* topfb, C3D_Tex* botfb);
	
	int expand_baseloc, expand_expandloc;
	int twocds_interploc, twocds_baseloc, twocds_baseinterploc;
	int sdrawTwoCdsVtxArrayPos;
	int sdrawVtxArrayPos;
	
	private:
	C3D_Tex lastfbtop;
	C3D_Tex lastfbbot;
	gfxScreen_t currentoutput = GFX_TOP;
	void sDrawi_renderText(float x, float y, float scaleX, float scaleY, bool baseline, const char* text);
};


sdraw_texture* loadpng(string filepath);
sdraw_texture* loadbin(string filepath, int width, int height);
unsigned int nextPow2(unsigned int v);
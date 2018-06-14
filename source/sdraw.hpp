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
	
	C3D_Tex lastfbtop;
	C3D_Tex lastfbbot;
	int sdrawTwoCdsVtxArrayPos;
	int sdrawVtxArrayPos;
	gfxScreen_t currentoutput = GFX_TOP;
	int expand_baseloc, expand_expandloc;
	int twocds_interploc, twocds_baseloc, twocds_baseinterploc;
	
	private:
	void sDrawi_renderText(float x, float y, float scaleX, float scaleY, bool baseline, const char* text);
};


sdraw_texture* loadpng(string filepath);
sdraw_texture* loadbin(string filepath, int width, int height);
unsigned int nextPow2(unsigned int v);
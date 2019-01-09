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
///Standard macro for 32-bit RGBA colors.
#define RGBA8(r, g, b, a) ((((r)&0xFF)<<0) | (((g)&0xFF)<<8) | (((b)&0xFF)<<16) | (((a)&0xFF)<<24))
///Macro for combining an RGB color and an alpha value.
#define HIGHLIGHTERCOLORANDALPHA(color, alpha) ((color & 0xFFFFFF) | ((alpha & 0xFF) << 24))

#define FRAMEBUFFER_TRANSFER_FLAGS GX_TRANSFER_RAW_COPY(1)

///Primary drawing object in sDraw.
///This struct contains information about an object in a spritesheet. It can be constructed
///with Tex3DS (see Tex3DS documentation and examples for details about subtextures, TL;DR is that
///the subtextures are 0-indexed and based on the order of the textures you place in your atlas file).
///It also contains information about whether the object should have sDraw's "dark mode" (color inversion)
///applied to it.
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
#include "shader.hpp"

///Extension of sdraw_stex that also contains a color.
struct sdraw_highlighter : public sdraw_stex
{
	u32 highlightercolor;

	sdraw_highlighter(unsigned int subtexture, std::pair<C3D_Tex*, Tex3DS_Texture> input, u32 incolor, \
		bool optionalusesdarkmode = false) : sdraw_stex(subtexture, input, optionalusesdarkmode), \
		highlightercolor(incolor) {};
};


namespace sdraw
{
	//Initializes the engine.
	int init();
	//Finalizes the engine.
	void cleanup();
	//Changes the screen on which to draw objects.
	void drawon(gfxScreen_t window);
	//Starts a frame. All rendering actions must take place within a frame (obviously).
	void framestart();
	//Draws a plain old texture to the screen.
	void drawtexture(C3D_Tex* tex, int x, float y);
	///Draws an sdraw_stex to the screen. This is the recommended and most common drawing action.
	///info is the stex to be drawn. x and y are the coordinates to draw it at.
	///x1, y1, and interpfactor are optional arguments that send a second set of coordinates and
	///how far to interpolate between the two sets.
	void drawtexture(sdraw_stex info, int x, int y, int x1 = -1, int y1 = -1, float interpfactor = 0); //Second coords are optional
	///Draws a framebuffer to the screen. Because the 3DS' screens are tilted and C3D_Tex textures
	///must be powers of two, this function exists to handle the insane texcoord math to deal with it.
	///istopfb should be provided to let the engine know whether the framebuffer is for the top screen or
	///the bottom. This is because the two screens have different dimensions.
	void drawframebuffer(C3D_Tex& tex, int x, int y, bool istopfb, int x1 = -1, int y1 = -1, float interpfactor = 0);
	///Draws text to the screen. text is the text, x and y are the coordinates,
	///and sizeX / sizeY are the font sizes.
	void drawtext(const char* text, float x, float y, float sizeX, float sizeY);
	///Draws text squished into a rectangle of size "width". scalex will be modified within
	///the function if necessary.
	void drawtextinrec(const char* text, int x, int y, int width, float scalex, float scaley);
	///Draws text in the center of the screen. No x argument is allowed.
	void drawcenteredtext(const char* text, float scaleX, float scaleY, float y);
	///Draws a blank rectangle to the screen. A fragment shader is usually provided to colorize it.
	void drawrectangle(int x, int y, int width, int height, bool shouldusedarkmode = false);
	///Draws a texture and sends three texture coordinates from three sdraw_stex structs.
	void drawmultipletextures(int x, int y, sdraw_stex info1, sdraw_stex info2, sdraw_stex info3);
	///Ends a frame.
	void frameend();
	///Gets the height of text given the text and the font height.
	float gettextheight(const char* text, float sizeY);
	///Gets the width of the text input, accounting for newlines by using a vector of possible widths.
	vector<float> gettextwidths(const char* text, float sizeX);
	///Returns the largest width from gettextwidths (above) for utility.
	float gettextmaxwidth(const char* text, float sizeX);
	
	//Copies last frame to provided textures of 256x512 dimensions.
	void retrieveframebuffers(C3D_Tex* topfb, C3D_Tex* botfb);

	//Sets up "dark mode". This inverts all colors of the UI, except pieces that need
	//to be displayed normally. The bool mentions whether dark mode is enabled at all,
	//the function enables it (if it needs to be, based on the bool).
	extern bool darkmodeshouldactivate;
	void enabledarkmode(bool enabled);
	
	//Called by vertex shader classes. Updates internal structures for shaders
	//(what shader to apply actions to) + sets the projection matrix automatically.
	//Note: This function triggers a draw call in order to change state.
	void updateshaderstate(ShaderBase* shader);

	///Internal structs. Should not be used externally.
	extern C3D_Tex lastfbtop;
	extern C3D_Tex lastfbbot;
	extern gfxScreen_t currentoutput;
	extern int vtxstartindex;
	extern bool vtxbufflushed;
	extern ShaderBase* currentshader;


	///Internal function to actually draw quads. Should not be used externally.
	void drawquad(sdraw_stex info, int x, int y, int x1 = -1, int y1 = -1, float interpfactor = 0);

	///Internal function to add vertices to the currently bound shader. Should not be used externally.
	void addVertex(float vx1, float vy1, float tx1, float ty1, float vx2 = -1, float vy2 = -1, float tx2 = -1, float ty2 = -1, float tx3 = -1, float ty3 = -1);

	///Internal function to trigger a draw call with the currently bound state
	///in order to flush vertices. Should not be used externally.
	void drawCall();

} //namespace sdraw


//C3D_Tex* loadpng(string filepath);
///Loads a raw binary texture file.
C3D_Tex* loadbin(string filepath, int width, int height);
///Loads a Tex3DS texture file.
std::pair<C3D_Tex*, Tex3DS_Texture> loadTextureFromFile(const char* filename);
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <3ds.h>
#include <citro3d.h>
#include <tex3ds.h>
#include "vshader_shbin.h"
#include "eventualvertex_shbin.h"
#include "twocoordsinterp_shbin.h"
#include "threetextures_shbin.h"

#include <string>
#include <fstream>

#include <algorithm>

using namespace std;

#include "../lodepng.h"
#include "sdraw.hpp"

#define CLEAR_COLOR 0

#define DISPLAY_TRANSFER_FLAGS \
	(GX_TRANSFER_FLIP_VERT(0) | GX_TRANSFER_OUT_TILED(0) | GX_TRANSFER_RAW_COPY(0) | \
	GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGBA8) | GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGB8) | \
	GX_TRANSFER_SCALING(GX_TRANSFER_SCALE_NO))

// Used to convert textures to 3DS tiled format
// Note: vertical flip flag set so 0,0 is top left of texture
#define TEXTURE_TRANSFER_FLAGS \
	(GX_TRANSFER_FLIP_VERT(1) | GX_TRANSFER_OUT_TILED(1) | GX_TRANSFER_RAW_COPY(0) | \
	GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGBA8) | GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGBA8) | \
	GX_TRANSFER_SCALING(GX_TRANSFER_SCALE_NO))

	static C3D_Mtx projectionTop, projectionBot;
	static C3D_Tex* glyphSheets;
	C3D_RenderTarget* top;
	C3D_RenderTarget* bottom;

	bool sdraw::darkmodeshouldactivate = false;

	C3D_Tex sdraw::lastfbtop;
	C3D_Tex sdraw::lastfbbot;
	gfxScreen_t sdraw::currentoutput = GFX_TOP;

	sdraw::ShaderBase* currentshader;
	vector<sdraw::ShaderBase*> shaderlist;

	#define TEXT_VTX_ARRAY_COUNT (4*1024)

#define TEX_MIN_SIZE 64

//Unused. Here for historical purposes.
/*C3D_Tex* loadpng(string filepath)
{

	unsigned char* imagebuf;
	unsigned int width = 0, height = 0;
	C3D_Tex* tex = new C3D_Tex;

	lodepng_decode32_file(&imagebuf, &width, &height, filepath.c_str());

	u8 *gpusrc = (u8*)linearAlloc(width*height * 4);

	// GX_DisplayTransfer needs input buffer in linear RAM
	u8* src = imagebuf; u8 *dst = gpusrc;

	// lodepng outputs big endian rgba so we need to convert
	for (unsigned int i = 0; i < width*height; i++) {
	int r = *src++;
	int g = *src++;
	int b = *src++;
	int a = *src++;

	*dst++ = a;
	*dst++ = b;
	*dst++ = g;
	*dst++ = r;
	}


	// ensure data is in physical ram
	GSPGPU_FlushDataCache(gpusrc, width*height * 4);


	// Load the texture and bind it to the first texture unit
	C3D_TexInit(tex, width, height, GPU_RGBA8);
	//u32* dest = (u32*)linearAlloc(width*height*4);


	// Convert image to 3DS tiled texture format
	C3D_SyncDisplayTransfer((u32*)gpusrc, GX_BUFFER_DIM(width, height), (u32*)tex->data, GX_BUFFER_DIM(width, height), TEXTURE_TRANSFER_FLAGS);

	//if (filepath == "romfs:/rainbow.png") //Very old hack.
	//{
	//	ofstream out(("rainbow.bin"), ios::binary | ios::trunc);
	//	out.write((char*)tex->data, width*height * 4);
	//	out.close();
	//}

	C3D_TexSetFilter(tex, GPU_LINEAR, GPU_LINEAR);
	C3D_TexSetWrap(tex, GPU_REPEAT, GPU_REPEAT);

	free(imagebuf);
	linearFree(gpusrc);


	tex->width = width; tex->height = height;

	return tex;
}*/

C3D_Tex* loadbin(string filepath, int width, int height)
{
	C3D_Tex* tex = new C3D_Tex;
	C3D_TexInit(tex, width, height, GPU_RGBA8);
	//Read the file
	ifstream in((const char*)filepath.c_str(), ifstream::binary);
	if (!in)
	{
		in.close();
		return NULL;
	}
	in.seekg(0, in.end);
	int size = in.tellg();
	in.seekg(0);
	char* buf = new char[size];
	in.read(buf, size);
	in.close();
	C3D_TexUpload(tex, (void*)buf);
	C3D_TexSetFilter(tex, GPU_LINEAR, GPU_LINEAR);
	C3D_TexSetWrap(tex, GPU_REPEAT, GPU_REPEAT);
	//delete[] buf; //For some reason this kills everything on hardware. Works fine in Citra. Strange...
	return tex;
}

//Helper function for loading a texture from a file
std::pair<C3D_Tex*, Tex3DS_Texture> loadTextureFromFile(const char* filename)
{
	C3D_Tex* tex = new C3D_Tex;
	FILE* fp = fopen(filename, "r");
	Tex3DS_Texture t3x = Tex3DS_TextureImportStdio(fp, tex, nullptr, false);
	if (!t3x)
		return std::make_pair(nullptr, Tex3DS_Texture());
	fclose(fp);

	return std::make_pair(tex, t3x);
}


int sdraw::init()
{
	gfxInitDefault();
	C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
	fontEnsureMapped();

	// Initialize the render targets
	top = C3D_RenderTargetCreate(240, 400, GPU_RB_RGBA8, GPU_RB_DEPTH24_STENCIL8);
	C3D_RenderTargetSetOutput(top, GFX_TOP, GFX_LEFT, DISPLAY_TRANSFER_FLAGS);

	bottom = C3D_RenderTargetCreate(240, 320, GPU_RB_RGBA8, GPU_RB_DEPTH24_STENCIL8);
	C3D_RenderTargetSetOutput(bottom, GFX_BOTTOM, GFX_LEFT, DISPLAY_TRANSFER_FLAGS);

	C3D_RenderTargetClear(top, C3D_CLEAR_ALL, CLEAR_COLOR, 0);
	C3D_RenderTargetClear(bottom, C3D_CLEAR_ALL, CLEAR_COLOR, 0);

	// Compute the projection matrix
	Mtx_OrthoTilt(&projectionTop, 0.0, 400.0, 240.0, 0.0, 0.0, 1.0, true);
	Mtx_OrthoTilt(&projectionBot, 0.0, 320.0, 240.0, 0.0, 0.0, 1.0, true);


	// Configure depth test to overwrite pixels with the same depth (needed to draw overlapping textures)
	C3D_DepthTest(true, GPU_GEQUAL, GPU_WRITE_ALL);
	//Configure the alpha test to not waste the GPU's time rendering completely transparent fragments
	C3D_AlphaTest(true, GPU_GREATER, 0);

	//Initialize the textures that store the last framebuffer
	C3D_TexInit(&lastfbtop, 256, 512, GPU_RGBA8); //Reversed width/height because 3DS screen is tilted
	C3D_TexInit(&lastfbbot, 256, 512, GPU_RGBA8);

	// Load the glyph texture sheets
	int i;
	TGLP_s* glyphInfo = fontGetGlyphInfo();
	glyphSheets = (C3D_Tex*)malloc(sizeof(C3D_Tex)*glyphInfo->nSheets);
	for (i = 0; i < glyphInfo->nSheets; i++)
	{
		C3D_Tex* tex = &glyphSheets[i];
		tex->data = fontGetGlyphSheetTex(i);
		tex->fmt = (GPU_TEXCOLOR)glyphInfo->sheetFmt;
		tex->size = glyphInfo->sheetSize;
		tex->width = glyphInfo->sheetWidth;
		tex->height = glyphInfo->sheetHeight;
		tex->param = GPU_TEXTURE_MAG_FILTER(GPU_LINEAR) | GPU_TEXTURE_MIN_FILTER(GPU_LINEAR)
			| GPU_TEXTURE_WRAP_S(GPU_CLAMP_TO_EDGE) | GPU_TEXTURE_WRAP_T(GPU_CLAMP_TO_EDGE);
		tex->border = 0;
		tex->lodParam = 0;
	}
	/*
	C3D_TexEnv* tev = C3D_GetTexEnv(5);
	C3D_TexEnvSrc(tev, C3D_Both, GPU_PREVIOUS);
	C3D_TexEnvOpRgb(tev, GPU_TEVOP_RGB_ONE_MINUS_SRC_COLOR);
	C3D_TexEnvOpAlpha(tev, GPU_TEVOP_A_SRC_ALPHA);
	C3D_TexEnvFunc(tev, C3D_Both, GPU_REPLACE);*/

	return 0;
}

void sdraw::drawon(gfxScreen_t output)
{
	currentoutput = output;
	C3D_FrameDrawOn(currentoutput == GFX_TOP ? top : bottom);
	// Update the uniforms
	currentshader->setUniformMtx4x4("projection", output == GFX_TOP ? &projectionTop : &projectionBot);
	/*int loc = (shaderinuse == BASICSHADER) ?\
	uLoc_projection : (shaderinuse == EVENTUALSHADER) ? expand_projectionloc : \
	(shaderinuse == TWOCOORDSSHADER) ? twocds_projectionloc : threetextures_projectionloc;
	C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, loc, output == GFX_TOP ? &projectionTop : &projectionBot);*/
}

void sdraw::drawtext(const char* text, float x, float y, float sizeX, float sizeY)
{
	//Always enable dark mode for text.
	enabledarkmode(true);
	sDrawi_renderText(x, y, sizeX, sizeY, false, text);
	enabledarkmode(false);
}

void sdraw::drawrectangle(int x, int y, int width, int height, bool shouldusedarkmode)
{
	if (shouldusedarkmode)
		enabledarkmode(true);

	addVertex(x, y + height, 0, 0); //What texture
	addVertex(x + width, y + height, 0, 0);
	addVertex(x, y, 0, 0);
	addVertex(x + width, y, 0, 0);

	C3D_DrawArrays(GPU_TRIANGLE_STRIP, currentshader->getArrayPos() - 4, 4);
	if (shouldusedarkmode)
		enabledarkmode(false);
}

void sdraw::addVertex(float vx1, float vy1, float tx1, float ty1, float vx2, float vy2, float tx2, float ty2, float tx3, float ty3)
{
	sdraw::internal_vertex vert = { vx1, vy1, 0.5, vx2, vy2, 0.5, tx1, ty1, tx2, ty2, tx3, ty3 };
	currentshader->appendVertex(vert);
}

void sdraw::sDrawi_renderText(float x, float y, float scaleX, float scaleY, bool baseline, const char* text)
{
	ssize_t  units;
	uint32_t code;


	const uint8_t* p = (const uint8_t*)text;
	float firstX = x;
	u32 flags = GLYPH_POS_CALC_VTXCOORD | (baseline ? GLYPH_POS_AT_BASELINE : 0);
	int lastSheet = -1;
	do
	{
		if (!*p) break;
		units = decode_utf8(&code, p);
		if (units == -1)
			break;
		p += units;
		if (code == '\n')
		{
			x = firstX;
			y += scaleY * fontGetInfo()->lineFeed;
		}
		else if (code > 0)
		{
			int glyphIdx = fontGlyphIndexFromCodePoint(code);
			fontGlyphPos_s data;
			fontCalcGlyphPos(&data, glyphIdx, flags, scaleX, scaleY);

			// Bind the correct texture sheet
			if (data.sheetIndex != lastSheet)
			{
				lastSheet = data.sheetIndex;
				C3D_TexBind(0, &glyphSheets[lastSheet]);
			}

			int arrayIndex = currentshader->getArrayPos();
			if ((arrayIndex + 4) >= TEXT_VTX_ARRAY_COUNT)
				break; // We can't render more characters

					   // Add the vertices to the array
			addVertex(x + data.vtxcoord.left, y + data.vtxcoord.bottom, data.texcoord.left, data.texcoord.bottom);
			addVertex(x + data.vtxcoord.right, y + data.vtxcoord.bottom, data.texcoord.right, data.texcoord.bottom);
			addVertex(x + data.vtxcoord.left, y + data.vtxcoord.top, data.texcoord.left, data.texcoord.top);
			addVertex(x + data.vtxcoord.right, y + data.vtxcoord.top, data.texcoord.right, data.texcoord.top);

			// Draw the glyph
			C3D_DrawArrays(GPU_TRIANGLE_STRIP, arrayIndex, 4);

			x += data.xAdvance;

		}
	} while (code > 0);
}

float sdraw::gettextheight(const char* text, float sizeY)
{
	string tex(text);
	int lines = std::count(tex.begin(), tex.end(), '\n') + 1; //There's always one line that doesn't have a \n
	return (sizeY*fontGetInfo()->lineFeed)*lines;
}

//Get the width of the text input. Account for \n by using a vector of possible widths. Use gettextmaxwidth to return the longest one
vector<float> sdraw::gettextwidths(const char* text, float sizeX)
{
	ssize_t  units;
	uint32_t code;
	int x = 0;

	const uint8_t* p = (const uint8_t*)text;
	u32 flags = GLYPH_POS_CALC_VTXCOORD | 0;

	vector<float> xvalues;
	do
	{
		if (!*p) { xvalues.push_back(x); break; }
		units = decode_utf8(&code, p);
		if (units == -1)
		{
			xvalues.push_back(x);
			break;
		}
		p += units;
		if (code == '\n')
		{
			xvalues.push_back(x);
			x = 0; //We have a new possible max width so reset x to recieve it
		}
		else if (code > 0)
		{
			int glyphIdx = fontGlyphIndexFromCodePoint(code);
			fontGlyphPos_s data;
			fontCalcGlyphPos(&data, glyphIdx, flags, sizeX, 0);

			x += data.xAdvance;

		}
	} while (code > 0);

	return xvalues;
}

float sdraw::gettextmaxwidth(const char* text, float sizeX)
{
	vector<float> widths = gettextwidths(text, sizeX);
	//Compare the X values to see which one is the longest.
	return *std::max_element(widths.begin(), widths.end());
}

//Draw centered text by looping through each newline and its equivalent width in a vector.
// (screenwidth/2) - (stringwidth / 2)
void sdraw::drawcenteredtext(const char* text, float scaleX, float scaleY, float y)
{
	//Always enable dark mode for text.
	enabledarkmode(true);
	vector<float> widths = gettextwidths(text, scaleX);
	vector<float>::iterator widthiterator = widths.begin();
	float x = (((currentoutput == GFX_TOP) ? 400 : 320) / 2 - (*widthiterator / 2)) - 5; //-5 to make it look a bit better

	ssize_t  units;
	uint32_t code;

	const uint8_t* p = (const uint8_t*)text;
	u32 flags = GLYPH_POS_CALC_VTXCOORD;
	int lastSheet = -1;
	do
	{
		if (!*p) break;
		units = decode_utf8(&code, p);
		if (units == -1)
			break;
		p += units;
		if (code == '\n')
		{
			widthiterator++; //We won't end up past the vector's limits, as there are only as many values as there are newlines
			x = (((currentoutput == GFX_TOP) ? 400 : 320) / 2 - (*widthiterator / 2)) - 5;
			y += scaleY * fontGetInfo()->lineFeed;
		}
		else if (code > 0)
		{
			int glyphIdx = fontGlyphIndexFromCodePoint(code);
			fontGlyphPos_s data;
			fontCalcGlyphPos(&data, glyphIdx, flags, scaleX, scaleY);

			// Bind the correct texture sheet
			if (data.sheetIndex != lastSheet)
			{
				lastSheet = data.sheetIndex;
				C3D_TexBind(0, &glyphSheets[lastSheet]);
			}

			int arrayIndex = currentshader->getArrayPos();
			if ((arrayIndex + 4) >= TEXT_VTX_ARRAY_COUNT)
				break; // We can't render more characters

					   // Add the vertices to the array
			addVertex(x + data.vtxcoord.left, y + data.vtxcoord.bottom, data.texcoord.left, data.texcoord.bottom);
			addVertex(x + data.vtxcoord.right, y + data.vtxcoord.bottom, data.texcoord.right, data.texcoord.bottom);
			addVertex(x + data.vtxcoord.left, y + data.vtxcoord.top, data.texcoord.left, data.texcoord.top);
			addVertex(x + data.vtxcoord.right, y + data.vtxcoord.top, data.texcoord.right, data.texcoord.top);

			// Draw the glyph
			C3D_DrawArrays(GPU_TRIANGLE_STRIP, arrayIndex, 4);

			x += data.xAdvance;

		}
	} while (code > 0);
	enabledarkmode(false);
}

void sdraw::drawtextinrec(const char* text, int x, int y, int width, float scalex, float scaley)
{
	//Always enable dark mode for text.
	enabledarkmode(true);
	float textwidth = gettextmaxwidth(text, scalex);
	float finalsizex;
	if (textwidth > width)
	{
		scalex *= (width / textwidth);
		finalsizex = gettextmaxwidth(text, scalex);
	}
	else finalsizex = textwidth;
	//Center it
	float finalx = ((currentoutput == GFX_TOP ? 400 : 320) - finalsizex) / 2;
	finalx -= 3; //Make it look better
	drawtext(text, finalx, y, scalex, scaley);
	enabledarkmode(false);
}

void sdraw::drawtexture(C3D_Tex* tex, int x, float y)
{
	C3D_TexBind(0, tex);

	if (x == CENTERED && y == CENTERED) { x = ((currentoutput == GFX_TOP ? 400 : 320) / 2) - (tex->width / 2); y = (240 / 2) - (tex->height / 2); }

	addVertex(x, y + tex->height, 0.0f, 1); //left bottom
	addVertex(x + tex->width, y + tex->height, 1, 1); //right bottom
	addVertex(x, y, 0.0f, 0.0f); //left top
	addVertex(x + tex->width, y, 1, 0.0f); //right top

	C3D_DrawArrays(GPU_TRIANGLE_STRIP, currentshader->getArrayPos() - 4, 4);
}


void sdraw::updateshaderstate(ShaderBase* shader)
{
	currentshader = shader;
	if (std::find(shaderlist.begin(), shaderlist.end(), shader) == shaderlist.end())
		shaderlist.push_back(shader);
	drawon(currentoutput); //Set the projection matrix
}

void sdraw::drawtexture(sdraw_stex info, int x, int y, int x1, int y1, float interpfactor)
{
	if (info.usesdarkmode)
		enabledarkmode(true);
	bindtex(0, info);

	if (x == CENTERED && y == CENTERED) { x = ((currentoutput == GFX_TOP ? 400 : 320) / 2) - (info.spritesheet->width / 2); y = (240 / 2) - (info.spritesheet->height / 2); }

	drawquad(info, x, y, x1, y1, interpfactor);

	if (info.usesdarkmode)
		enabledarkmode(false);
}

//Draw a framebuffer, it's tilted sideways and stuffed into a larger texture and flipped so we need some extra maths for this
void sdraw::drawframebuffer(C3D_Tex tex, int x, int y, bool istopfb, int x1, int y1, float interpfactor)
{
	bindtex(0, &tex);
	const float scrwidth = 240, scrheight = istopfb ? 400 : 320;
	const float texwidth = 256, texheight = 512;

	//The existence of these uniforms is assumed. If a shader that does not have them
	//is currently bound, they are simply ignored.
	currentshader->setUniformF("baseinterpfactor", 1); //No base interpolation
	currentshader->setUniformF("interpfactor", interpfactor);
	addVertex(x, y + scrwidth, 0, (scrheight / texheight), x1, y1 + scrwidth); //left bottom
	addVertex(x + scrheight, y + scrwidth, 0, 0, x1 + scrheight, y1 + scrwidth); //right bottom
	addVertex(x, y, (scrwidth / texwidth), (scrheight / texheight), x1, y1); //left top
	addVertex(x + scrheight, y, (scrwidth / texwidth), 0, x1 + scrheight, y1); //right top
	C3D_DrawArrays(GPU_TRIANGLE_STRIP, currentshader->getArrayPos() - 4, 4);
	/*sDrawi_addTextVertex(x, y + tex.height - 450, 0, 0); //left bottom
	sDrawi_addTextVertex(x + tex.width, y + tex.height - 450, 1, 0); //right bottom
	sDrawi_addTextVertex(x, y - 450, 0, 1); //left top
	sDrawi_addTextVertex(x + tex.width, y - 450, 1, 1); //right top*/

}

//All textures should be the same width/height.
void sdraw::drawmultipletextures(int x, int y, sdraw_stex info1, sdraw_stex info2, sdraw_stex info3)
{
	if (info1.usesdarkmode || info2.usesdarkmode || info3.usesdarkmode)
		enabledarkmode(true);

	C3D_TexBind(0, info1.spritesheet);
	C3D_TexBind(1, info2.spritesheet);
	C3D_TexBind(2, info3.spritesheet);

	addVertex(x, y + info1.height, info1.botleft[0], info1.botleft[1], 0, 0, info2.botleft[0], info2.botleft[1], info3.botleft[0], info3.botleft[1]); //left bottom
	addVertex(x + info1.width, y + info1.height, info1.botright[0], info1.botright[1], 0, 0, info2.botright[0], info2.botright[1], info3.botright[0], info3.botright[1]); //right bottom
	addVertex(x, y, info1.topleft[0], info1.topleft[1], 0, 0, info2.topleft[0], info2.topleft[1], info3.topleft[0], info3.topleft[1]); //left top
	addVertex(x + info1.width, y, info1.topright[0], info1.topright[1], 0, 0, info2.topright[0], info2.topright[1], info3.topright[0], info3.topright[1]); //right top
	C3D_DrawArrays(GPU_TRIANGLE_STRIP, currentshader->getArrayPos() - 4, 4);


	if (info1.usesdarkmode || info2.usesdarkmode || info3.usesdarkmode)
		enabledarkmode(false);
}

//Draw with 0.75 texcoords as it's a 48x48 icon in a 64x64 texture (Power of two limits...)
//Also width/height is constant- 48x48
void sdraw::drawSMDHicon(C3D_Tex icon, int x, int y)
{
	bindtex(0, &icon);

	if (x == CENTERED && y == CENTERED) { x = ((currentoutput == GFX_TOP ? 400 : 320) / 2) - (48 / 2); y = (240 / 2) - (48 / 2); }

	addVertex(x, y + 48, 0, 0); //left bottom
	addVertex(x + 48, y + 48, 0.75, 0); //right bottom
	addVertex(x, y, 0, 0.75); //left top
	addVertex(x + 48, y, 0.75, 0.75); //right top

	C3D_DrawArrays(GPU_TRIANGLE_STRIP, currentshader->getArrayPos() - 4, 4);
}

void sdraw::drawquad(sdraw_stex info, int x, int y, int x1, int y1, float interpfactor)
{

	if (x == CENTERED && y == CENTERED) { x = ((currentoutput == GFX_TOP ? 400 : 320) / 2) - (info.spritesheet->width / 2); y = (240 / 2) - (info.spritesheet->height / 2); }

	//The existence of these uniforms is assumed. If a shader that does not have them
	//is currently bound, they are simply ignored.
	currentshader->setUniformF("baseinterpfactor", 1);
	currentshader->setUniformF("interpfactor", interpfactor);
	
	addVertex(x, y + info.height, info.botleft[0], info.botleft[1], x1, y1 + info.height);
	addVertex(x + info.width, y + info.height, info.botright[0], info.botright[1], x1 + info.width, y1 + info.height);
	addVertex(x, y, info.topleft[0], info.topleft[1], x1, y1);
	addVertex(x + info.width, y, info.topright[0], info.topright[1], x1 + info.width, y1);
	C3D_DrawArrays(GPU_TRIANGLE_STRIP, currentshader->getArrayPos() - 4, 4);
}

void sdraw::enabledarkmode(bool isenabled)
{
	if (!darkmodeshouldactivate)
		return;
	C3D_TexEnv* tev = C3D_GetTexEnv(5);
	//Invert colors...
	C3D_TexEnvOpRgb(tev, isenabled ? GPU_TEVOP_RGB_ONE_MINUS_SRC_COLOR : GPU_TEVOP_RGB_SRC_COLOR);
}

void sdraw::framestart()
{
	C3D_FrameBegin(C3D_FRAME_SYNCDRAW);

	for (auto iter = shaderlist.begin(); iter != shaderlist.end(); iter++)
		(*iter)->resetArrayPos();

	//Of all things, this is what breaks the framebuffer copy.
	//Well, OK, I don't really need or use this functionality anyway.
	//C3D_RenderTargetClear(top, C3D_CLEAR_ALL, CLEAR_COLOR, 0);
	//C3D_RenderTargetClear(bottom, C3D_CLEAR_ALL, CLEAR_COLOR, 0);

	//Except... We DO need to clear the stencil buffer.
	C3D_RenderTargetClear(top, C3D_CLEAR_DEPTH, 0, 0);
	C3D_RenderTargetClear(bottom, C3D_CLEAR_DEPTH, 0, 0);
	drawon(GFX_TOP); //Reset default output to the top screen
}

void sdraw::frameend()
{
	//Get the final framebuffer of this frame and save it
	//So the ctrulib docs have lies, GX_BUFFER_DIM is not width and height, but rather what 16-byte blocks to copy
	//And what 16-byte blocks to skip copying. Also >>4 because "that's how it works".
	//Other than that, self-explanatory if you stare at the numbers long enough.
	C3D_SyncTextureCopy(
		(u32*)top->frameBuf.colorBuf, GX_BUFFER_DIM((240 * 8 * 4) >> 4, 0),
		(u32*)lastfbtop.data + (512 - 400) * 256, GX_BUFFER_DIM((240 * 8 * 4) >> 4, ((256 - 240) * 8 * 4) >> 4),
		240 * 400 * 4, FRAMEBUFFER_TRANSFER_FLAGS);
	C3D_SyncTextureCopy(
		(u32*)bottom->frameBuf.colorBuf, GX_BUFFER_DIM((240 * 8 * 4) >> 4, 0),
		(u32*)lastfbbot.data + (512 - 320) * 256, GX_BUFFER_DIM((240 * 8 * 4) >> 4, ((256 - 240) * 8 * 4) >> 4),
		240 * 320 * 4, FRAMEBUFFER_TRANSFER_FLAGS);
	C3D_FrameEnd(0);
}

void sdraw::retrieveframebuffers(C3D_Tex* topfb, C3D_Tex* botfb)
{
	if (topfb)
		C3D_SyncTextureCopy((u32*)lastfbtop.data, GX_BUFFER_DIM((256 * 8 * 4) >> 4, 0), (u32*)topfb->data, \
			GX_BUFFER_DIM((256 * 8 * 4) >> 4, 0), 512 * 256 * 4, FRAMEBUFFER_TRANSFER_FLAGS);
	if (botfb)
		C3D_SyncTextureCopy((u32*)lastfbbot.data, GX_BUFFER_DIM((256 * 8 * 4) >> 4, 0), (u32*)botfb->data, \
			GX_BUFFER_DIM((256 * 8 * 4) >> 4, 0), 512 * 256 * 4, FRAMEBUFFER_TRANSFER_FLAGS);
}

void sdraw::cleanup()
{
	C3D_FrameSplit(0); //We need to wait for everything to finish before shutting it down
	// Free the textures
	free(glyphSheets);
	C3D_TexDelete(&lastfbtop);
	C3D_TexDelete(&lastfbbot);
	C3D_Fini();
}
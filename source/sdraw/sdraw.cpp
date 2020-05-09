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

	C3D_Mtx projectionTop, projectionBot;
	C3D_Tex* glyphSheets;
	C3D_RenderTarget* top;
	C3D_RenderTarget* bottom;

	bool sdraw::darkmodeshouldactivate = false;
	bool darkmodeenabled = false;

	C3D_Tex sdraw::lastfbtop;
	C3D_Tex sdraw::lastfbbot;
	gfxScreen_t sdraw::currentoutput = GFX_TOP;

	sdraw::ShaderBase* sdraw::currentshader;
	vector<sdraw::ShaderBase*> shaderlist;

	int sdraw::vtxstartindex = 0; //Keeps track of vertices for the next draw call
	bool sdraw::vtxbufflushed = true;

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
	TGLP_s* glyphInfo = fontGetGlyphInfo(nullptr);
	glyphSheets = (C3D_Tex*)malloc(sizeof(C3D_Tex)*glyphInfo->nSheets);
	for (i = 0; i < glyphInfo->nSheets; i++)
	{
		C3D_Tex* tex = &glyphSheets[i];
		tex->data = fontGetGlyphSheetTex(nullptr, i);
		tex->fmt = (GPU_TEXCOLOR)glyphInfo->sheetFmt;
		tex->size = glyphInfo->sheetSize;
		tex->width = glyphInfo->sheetWidth;
		tex->height = glyphInfo->sheetHeight;
		tex->param = GPU_TEXTURE_MAG_FILTER(GPU_LINEAR) | GPU_TEXTURE_MIN_FILTER(GPU_LINEAR)
			| GPU_TEXTURE_WRAP_S(GPU_CLAMP_TO_EDGE) | GPU_TEXTURE_WRAP_T(GPU_CLAMP_TO_EDGE);
		tex->border = 0;
		tex->lodParam = 0;
	}

	return 0;
}

void sdraw::drawCall()
{
	//Flush vertices
	if (!vtxbufflushed)
	{
		C3D_DrawArrays(GPU_TRIANGLES, vtxstartindex, currentshader->getArrayPos() - vtxstartindex);
		vtxstartindex = currentshader->getArrayPos();
		vtxbufflushed = true;
	}
}

void sdraw::drawon(gfxScreen_t output)
{
	drawCall(); //Flush state
	currentoutput = output;
	C3D_FrameDrawOn(currentoutput == GFX_TOP ? top : bottom);
	//Update the uniforms
	currentshader->setUniformMtx4x4("projection", output == GFX_TOP ? &projectionTop : &projectionBot);
}

void sdraw::drawrectangle(int x, int y, int width, int height, bool shouldusedarkmode)
{
	enabledarkmode(shouldusedarkmode);

	addVertex(x, y + height, 0, 0); //What texture
	addVertex(x + width, y + height, 0, 0);
	addVertex(x, y, 0, 0);

	addVertex(x, y, 0, 0);
	addVertex(x + width, y + height, 0, 0);
	addVertex(x + width, y, 0, 0);
}

void sdraw::addVertex(float vx1, float vy1, float tx1, float ty1, float vx2, float vy2, float tx2, float ty2, float tx3, float ty3)
{
	vtxbufflushed = false;
	sdraw::internal_vertex vert = { vx1, vy1, 0.5, vx2, vy2, 0.5, tx1, ty1, tx2, ty2, tx3, ty3 };
	currentshader->appendVertex(vert);
}

void sdraw::drawtext(const char* text, float x, float y, float sizeX, float sizeY)
{
	ssize_t  units;
	uint32_t code;


	const uint8_t* p = (const uint8_t*)text;
	float firstX = x;
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
			x = firstX;
			y += sizeY * fontGetInfo(nullptr)->lineFeed;
		}
		else if (code > 0)
		{
			int glyphIdx = fontGlyphIndexFromCodePoint(nullptr, code);
			fontGlyphPos_s data;
			fontCalcGlyphPos(&data, nullptr, glyphIdx, flags, sizeX, sizeY);

			// Bind the correct texture sheet
			if (data.sheetIndex != lastSheet)
			{
				lastSheet = data.sheetIndex;
				bindtex(0, &glyphSheets[lastSheet]);
			}

			// Add the vertices to the array
			addVertex(x + data.vtxcoord.left, y + data.vtxcoord.bottom, data.texcoord.left, data.texcoord.bottom);
			addVertex(x + data.vtxcoord.right, y + data.vtxcoord.bottom, data.texcoord.right, data.texcoord.bottom);
			addVertex(x + data.vtxcoord.left, y + data.vtxcoord.top, data.texcoord.left, data.texcoord.top);

			addVertex(x + data.vtxcoord.left, y + data.vtxcoord.top, data.texcoord.left, data.texcoord.top);
			addVertex(x + data.vtxcoord.right, y + data.vtxcoord.bottom, data.texcoord.right, data.texcoord.bottom);
			addVertex(x + data.vtxcoord.right, y + data.vtxcoord.top, data.texcoord.right, data.texcoord.top);

			x += data.xAdvance;

		}
	} while (code > 0);
}

float sdraw::gettextheight(const char* text, float sizeY)
{
	string tex(text);
	int lines = std::count(tex.begin(), tex.end(), '\n') + 1; //There's always one line that doesn't have a \n
	return (sizeY*fontGetInfo(nullptr)->lineFeed)*lines;
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
			int glyphIdx = fontGlyphIndexFromCodePoint(nullptr, code);
			fontGlyphPos_s data;
			fontCalcGlyphPos(&data, nullptr, glyphIdx, flags, sizeX, 0);

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
			y += scaleY * fontGetInfo(nullptr)->lineFeed;
		}
		else if (code > 0)
		{
			int glyphIdx = fontGlyphIndexFromCodePoint(nullptr, code);
			fontGlyphPos_s data;
			fontCalcGlyphPos(&data, nullptr, glyphIdx, flags, scaleX, scaleY);

			// Bind the correct texture sheet
			if (data.sheetIndex != lastSheet)
			{
				lastSheet = data.sheetIndex;
				bindtex(0, &glyphSheets[lastSheet]);
			}

			// Add the vertices to the array
			addVertex(x + data.vtxcoord.left, y + data.vtxcoord.bottom, data.texcoord.left, data.texcoord.bottom);
			addVertex(x + data.vtxcoord.right, y + data.vtxcoord.bottom, data.texcoord.right, data.texcoord.bottom);
			addVertex(x + data.vtxcoord.left, y + data.vtxcoord.top, data.texcoord.left, data.texcoord.top);

			addVertex(x + data.vtxcoord.left, y + data.vtxcoord.top, data.texcoord.left, data.texcoord.top);
			addVertex(x + data.vtxcoord.right, y + data.vtxcoord.bottom, data.texcoord.right, data.texcoord.bottom);
			addVertex(x + data.vtxcoord.right, y + data.vtxcoord.top, data.texcoord.right, data.texcoord.top);

			x += data.xAdvance;

		}
	} while (code > 0);
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
}

void sdraw::drawtexture(C3D_Tex* tex, int x, float y)
{
	sdraw::bindtex(0, tex);

	if (x == CENTERED && y == CENTERED) { x = ((currentoutput == GFX_TOP ? 400 : 320) / 2) - (tex->width / 2); y = (240 / 2) - (tex->height / 2); }

	addVertex(x, y + tex->height, 0.0f, 1); //left bottom
	addVertex(x + tex->width, y + tex->height, 1, 1); //right bottom
	addVertex(x, y, 0.0f, 0.0f); //left top

	addVertex(x, y, 0.0f, 0.0f); //left top
	addVertex(x + tex->width, y + tex->height, 1, 1); //right bottom
	addVertex(x + tex->width, y, 1, 0.0f); //right top
}


void sdraw::updateshaderstate(ShaderBase* shader)
{
	drawCall();
	currentshader = shader;
	vtxstartindex = currentshader->getArrayPos();
	if (std::find(shaderlist.begin(), shaderlist.end(), shader) == shaderlist.end())
		shaderlist.push_back(shader);
	drawon(currentoutput); //Set the projection matrix
}

void sdraw::drawtexture(sdraw_stex info, int x, int y, int x1, int y1, float interpfactor)
{
	enabledarkmode(info.usesdarkmode);
	bindtex(0, info);

	if (x == CENTERED && y == CENTERED) { x = ((currentoutput == GFX_TOP ? 400 : 320) / 2) - (info.spritesheet->width / 2); y = (240 / 2) - (info.spritesheet->height / 2); }

	drawquad(info, x, y, x1, y1, interpfactor);
}

//Draw a framebuffer, it's tilted sideways and stuffed into a larger texture and flipped so we need some extra maths for this
void sdraw::drawframebuffer(C3D_Tex& tex, int x, int y, bool istopfb, int x1, int y1, float interpfactor)
{
	//Framebuffers are never drawn with inverted colors!
	//This fixes a bug that caused frames drawn in dark mode to
	//be inverted, causing them to look as if they're in light mode.
	enabledarkmode(false);
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

	addVertex(x, y, (scrwidth / texwidth), (scrheight / texheight), x1, y1); //left top
	addVertex(x + scrheight, y + scrwidth, 0, 0, x1 + scrheight, y1 + scrwidth); //right bottom
	addVertex(x + scrheight, y, (scrwidth / texwidth), 0, x1 + scrheight, y1); //right top
}

//All textures should be the same width/height.
void sdraw::drawmultipletextures(int x, int y, sdraw_stex info1, sdraw_stex info2, sdraw_stex info3)
{
	enabledarkmode(info1.usesdarkmode || info2.usesdarkmode || info3.usesdarkmode);

	sdraw::bindtex(0, info1.spritesheet);
	sdraw::bindtex(1, info2.spritesheet);
	sdraw::bindtex(2, info3.spritesheet);

	addVertex(x, y + info1.height, info1.botleft[0], info1.botleft[1], 0, 0, info2.botleft[0], info2.botleft[1], info3.botleft[0], info3.botleft[1]); //left bottom
	addVertex(x + info1.width, y + info1.height, info1.botright[0], info1.botright[1], 0, 0, info2.botright[0], info2.botright[1], info3.botright[0], info3.botright[1]); //right bottom
	addVertex(x, y, info1.topleft[0], info1.topleft[1], 0, 0, info2.topleft[0], info2.topleft[1], info3.topleft[0], info3.topleft[1]); //left top

	addVertex(x, y, info1.topleft[0], info1.topleft[1], 0, 0, info2.topleft[0], info2.topleft[1], info3.topleft[0], info3.topleft[1]); //left top
	addVertex(x + info1.width, y + info1.height, info1.botright[0], info1.botright[1], 0, 0, info2.botright[0], info2.botright[1], info3.botright[0], info3.botright[1]); //right bottom
	addVertex(x + info1.width, y, info1.topright[0], info1.topright[1], 0, 0, info2.topright[0], info2.topright[1], info3.topright[0], info3.topright[1]); //right top
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

	
	addVertex(x, y, info.topleft[0], info.topleft[1], x1, y1);
	addVertex(x + info.width, y + info.height, info.botright[0], info.botright[1], x1 + info.width, y1 + info.height);
	addVertex(x + info.width, y, info.topright[0], info.topright[1], x1 + info.width, y1);
}

void sdraw::enabledarkmode(bool isenabled)
{
	if (!darkmodeshouldactivate || darkmodeenabled == isenabled)
		return;
	darkmodeenabled = isenabled;
	//Flush state. This could be removed in OpenGL with a conditional, and *maybe* in C3D with some trickery?
	drawCall();
	C3D_TexEnv* tev = C3D_GetTexEnv(5);
	//Invert colors...
	C3D_TexEnvOpRgb(tev, isenabled ? GPU_TEVOP_RGB_ONE_MINUS_SRC_COLOR : GPU_TEVOP_RGB_SRC_COLOR);
}

void sdraw::framestart()
{
	C3D_FrameBegin(C3D_FRAME_SYNCDRAW);

	for (auto iter = shaderlist.begin(); iter != shaderlist.end(); iter++)
		(*iter)->resetArrayPos();
	vtxstartindex = 0;
	vtxbufflushed = true;

	//Clear the stencil/depth buffer. Color buffer isn't cleared.
	C3D_RenderTargetClear(top, C3D_CLEAR_DEPTH, 0, 0);
	C3D_RenderTargetClear(bottom, C3D_CLEAR_DEPTH, 0, 0);
	drawon(GFX_TOP); //Reset default output to the top screen
}

void sdraw::frameend()
{
	drawCall(); //Flush remaining vertices

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
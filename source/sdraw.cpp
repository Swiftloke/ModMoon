//This needs a major revamp at this point.
//sDraw should be redesigned to not couple TexEnv states and vertex handling, but instead
//Have functions to set TexEnv states then functions to add + draw vertices.
//new-hbmenu does this right.
//Also it should be a namespace instead of a class. That just doesn't make sense.

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <3ds.h>
#include <citro3d.h>
#include "vshader_shbin.h"
#include "eventualvertex_shbin.h"
#include "twocoordsinterp_shbin.h"

#include <string>
#include <fstream>

#include <algorithm>

using namespace std;

#include "lodepng.h"
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

	int idcount = 0;
	static DVLB_s* basicvshader_dvlb;
	static DVLB_s* eventualvshader_dvlb;
	static DVLB_s* twocoordsinterp_dvlb;
	static int uLoc_projection;
	int expand_projectionloc;
	int twocds_projectionloc;
	static C3D_Mtx projectionTop, projectionBot;
	static C3D_Tex* glyphSheets;
	shaderProgram_s basicprogram;
	shaderProgram_s eventualprogram;
	shaderProgram_s twocoordsprogram;

	C3D_AttrInfo basicinfo;
	C3D_AttrInfo twocoordsinfo;
	C3D_BufInfo basicbuf;
	C3D_BufInfo twocoordsbuf;

	static sdraw_Vertex* sdrawVtxArray;
	static sdraw_TwoCdsVertex* twocdsVtxArray;
	C3D_RenderTarget* top;
	C3D_RenderTarget* bottom;

	enum SDRAW_SHADERINUSE
	{
		BASICSHADER, EVENTUALSHADER, TWOCOORDSSHADER
	};
	SDRAW_SHADERINUSE shaderinuse = BASICSHADER;

#define TEXT_VTX_ARRAY_COUNT (4*1024)





#define TEX_MIN_SIZE 64
//Grabbed from: http://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2
unsigned int nextPow2(unsigned int v)
{
    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    v++;
    return (v >= TEX_MIN_SIZE ? v : TEX_MIN_SIZE);
}

sdraw_stex::sdraw_stex(sdraw_texture* inputsheet, int posx, int posy, int inwidth, int inheight) : spritesheet(inputsheet), x(posx), y(posy), width(inwidth), height(inheight)
{}

//Take note that this can only load images with dimensions of powers of 2
sdraw_texture* loadpng(string filepath)
{

	unsigned char* imagebuf;
	unsigned int width = 0, height = 0;
	sdraw_texture* tex = (sdraw_texture*)malloc(sizeof(sdraw_texture));

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
	C3D_TexInit(&tex->image, width, height, GPU_RGBA8);
	//u32* dest = (u32*)linearAlloc(width*height*4);


	// Convert image to 3DS tiled texture format
	C3D_SafeDisplayTransfer((u32*)gpusrc, GX_BUFFER_DIM(width, height), (u32*)tex->image.data, GX_BUFFER_DIM(width, height), TEXTURE_TRANSFER_FLAGS);
	gspWaitForPPF();

	/*ofstream out(("spritesheet.bin"), ios::binary | ios::trunc);
	out.write((char*)dest, width*height*4);
	out.close();*/

	C3D_TexSetFilter(&tex->image, GPU_LINEAR, GPU_LINEAR);
	C3D_TexSetWrap(&tex->image, GPU_REPEAT, GPU_REPEAT);

	free(imagebuf);
	linearFree(gpusrc);
	

	tex->width = width; tex->height = height; 

	tex->id = idcount;
	idcount++;
	return tex;
}

sdraw_texture* loadbin(string filepath, int width, int height)
{
	sdraw_texture* tex = new sdraw_texture;
	C3D_TexInit(&tex->image, width, height, GPU_RGBA8);
	//Read the file
	ifstream in(filepath.c_str(), ifstream::binary);
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
	C3D_TexUpload(&tex->image, (void*)buf);
	C3D_TexSetFilter(&tex->image, GPU_LINEAR, GPU_LINEAR);
	C3D_TexSetWrap(&tex->image, GPU_REPEAT, GPU_REPEAT);
	delete[] buf;
	return tex;
}


sDraw_interface::sDraw_interface()
{
	gfxInitDefault();
	C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
	
	fontEnsureMapped();
	
	// Initialize the render targets
	top = C3D_RenderTargetCreate(240, 400, GPU_RB_RGBA8, GPU_RB_DEPTH24_STENCIL8);
	C3D_RenderTargetSetClear(top, C3D_CLEAR_ALL, CLEAR_COLOR, 0);
	C3D_RenderTargetSetOutput(top, GFX_TOP, GFX_LEFT, DISPLAY_TRANSFER_FLAGS);

	bottom = C3D_RenderTargetCreate(240, 320, GPU_RB_RGBA8, GPU_RB_DEPTH24_STENCIL8);
	C3D_RenderTargetSetClear(bottom, C3D_CLEAR_ALL, CLEAR_COLOR, 0);
	C3D_RenderTargetSetOutput(bottom, GFX_BOTTOM, GFX_LEFT, DISPLAY_TRANSFER_FLAGS);
	
	// Load the vertex shader, create a shader program and bind it
	basicvshader_dvlb = DVLB_ParseFile((u32*)vshader_shbin, vshader_shbin_size);
	shaderProgramInit(&basicprogram);
	shaderProgramSetVsh(&basicprogram, &basicvshader_dvlb->DVLE[0]);
	C3D_BindProgram(&basicprogram);
	
	//Load the eventual vertex shader
	eventualvshader_dvlb = DVLB_ParseFile((u32*)eventualvertex_shbin, eventualvertex_shbin_size);
	shaderProgramInit(&eventualprogram);
	shaderProgramSetVsh(&eventualprogram, &eventualvshader_dvlb->DVLE[0]);

	//Load the two coordinates interpolation shader
	twocoordsinterp_dvlb = DVLB_ParseFile((u32*)twocoordsinterp_shbin, twocoordsinterp_shbin_size);
	shaderProgramInit(&twocoordsprogram);
	shaderProgramSetVsh(&twocoordsprogram, &twocoordsinterp_dvlb->DVLE[0]);
	// Get the location of the uniforms
	uLoc_projection = shaderInstanceGetUniformLocation(basicprogram.vertexShader, "projection");
	
	expand_projectionloc = shaderInstanceGetUniformLocation(eventualprogram.vertexShader, "projection");
	expand_baseloc = shaderInstanceGetUniformLocation(eventualprogram.vertexShader, "base");
	expand_expandloc = shaderInstanceGetUniformLocation(eventualprogram.vertexShader, "expansion");

	twocds_projectionloc = shaderInstanceGetUniformLocation(twocoordsprogram.vertexShader, "projection");
	twocds_interploc = shaderInstanceGetUniformLocation(twocoordsprogram.vertexShader, "interpfactor");
	twocds_baseloc = shaderInstanceGetUniformLocation(twocoordsprogram.vertexShader, "base");
	twocds_baseinterploc = shaderInstanceGetUniformLocation(twocoordsprogram.vertexShader, "baseinterpfactor");

	//Configure AttrInfos for use with the vertex shader
	//Basic AttrInfos are the same in the eventual shader but position is actually the eventual position
	AttrInfo_Init(&basicinfo);
	AttrInfo_AddLoader(&basicinfo, 0, GPU_FLOAT, 3); // v0=position
	AttrInfo_AddLoader(&basicinfo, 1, GPU_FLOAT, 2); // v1=texcoord
	C3D_SetAttrInfo(&basicinfo); //This is the default
	//Two coordinates, different AttrInfo
	AttrInfo_Init(&twocoordsinfo);
	AttrInfo_AddLoader(&twocoordsinfo, 0, GPU_FLOAT, 3); // v0 = position 1
	AttrInfo_AddLoader(&twocoordsinfo, 1, GPU_FLOAT, 3); // v1 = position 2
	AttrInfo_AddLoader(&twocoordsinfo, 2, GPU_FLOAT, 2); // v2 = texcoords
	
	// Create the vertex arrays
	sdrawVtxArray = (sdraw_Vertex*)linearAlloc(sizeof(sdraw_Vertex)*TEXT_VTX_ARRAY_COUNT);
	twocdsVtxArray = (sdraw_TwoCdsVertex*)linearAlloc(sizeof(sdraw_TwoCdsVertex) * TEXT_VTX_ARRAY_COUNT);
	
	// Configure buffers
	BufInfo_Init(&basicbuf);
	BufInfo_Add(&basicbuf, sdrawVtxArray, sizeof(sdraw_Vertex), 2, 0x10);
	C3D_SetBufInfo(&basicbuf);
	BufInfo_Init(&twocoordsbuf);
	BufInfo_Add(&twocoordsbuf, twocdsVtxArray, sizeof(sdraw_TwoCdsVertex), 3, 0x210);

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
	for (i = 0; i < glyphInfo->nSheets; i ++)
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

}

void sDraw_interface::drawon(gfxScreen_t output)
{
	currentoutput = output;
	C3D_FrameDrawOn(currentoutput == GFX_TOP ? top : bottom);
	// Update the uniforms
	int loc = (shaderinuse == BASICSHADER) ?\
		uLoc_projection : (shaderinuse == EVENTUALSHADER) ? expand_projectionloc : twocds_projectionloc;
	C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, loc, output == GFX_TOP ? &projectionTop : &projectionBot);
}

void sDraw_interface::drawtext(const char* text, float x, float y, float sizeX, float sizeY)
{
	sDrawi_renderText(x, y, sizeX, sizeY, false, text);
}

void sDraw_interface::settextcolor(u32 color)
{
	C3D_TexEnv* env = C3D_GetTexEnv(0);
	C3D_TexEnvSrc(env, C3D_RGB, GPU_CONSTANT, 0, 0);
	C3D_TexEnvSrc(env, C3D_Alpha, GPU_TEXTURE0, GPU_CONSTANT, 0);
	C3D_TexEnvOp(env, C3D_Both, 0, 0, 0);
	C3D_TexEnvFunc(env, C3D_RGB, GPU_REPLACE);
	C3D_TexEnvFunc(env, C3D_Alpha, GPU_MODULATE);
	C3D_TexEnvColor(env, color);
}

void sDraw_interface::drawrectangle(int x, int y, int width, int height, u32 color)
{
	//Override the color entirely
	C3D_TexEnv *env = C3D_GetTexEnv(0);
	C3D_TexEnvSrc(env, C3D_Both, GPU_CONSTANT, GPU_CONSTANT, 0);
	C3D_TexEnvOp(env, C3D_Both, 0, 0, 0);
	C3D_TexEnvFunc(env, C3D_Both, GPU_REPLACE);
	C3D_TexEnvColor(env, color);

	sDrawi_addTextVertex(x, y + height, 0, 0); //What texture
	sDrawi_addTextVertex(x + width, y + height, 0, 0);
	sDrawi_addTextVertex(x, y , 0, 0);
	sDrawi_addTextVertex(x + width, y, 0, 0);
	
	
	C3D_DrawArrays(GPU_TRIANGLE_STRIP, sdrawVtxArrayPos-4, 4);
}

void sDraw_interface::sDrawi_addTextVertex(float vx, float vy, float tx, float ty)
{
	sdraw_Vertex* vtx = &sdrawVtxArray[sdrawVtxArrayPos++];
	vtx->position[0] = vx;
	vtx->position[1] = vy;
	vtx->position[2] = 0.5f;
	vtx->texcoord[0] = tx;
	vtx->texcoord[1] = ty;
}

void sDraw_interface::sDrawi_addTwoCoordsVertex(float vx1, float vy1, float vx2, float vy2, float tx, float ty)
{
	sdraw_TwoCdsVertex* vtx = &twocdsVtxArray[sdrawTwoCdsVtxArrayPos++];
	vtx->pos1[0] = vx1;
	vtx->pos1[1] = vy1;
	vtx->pos1[2] = 0.5f;
	vtx->pos2[0] = vx2;
	vtx->pos2[1] = vy2;
	vtx->pos2[2] = 0.5f;
	vtx->texcoord[0] = tx;
	vtx->texcoord[1] = ty;
}

void sDraw_interface::sDrawi_renderText(float x, float y, float scaleX, float scaleY, bool baseline, const char* text)
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
			y += scaleY*fontGetInfo()->lineFeed;
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

			int arrayIndex = sdrawVtxArrayPos;
			if ((arrayIndex+4) >= TEXT_VTX_ARRAY_COUNT)
				break; // We can't render more characters

			// Add the vertices to the array
			sDrawi_addTextVertex(x+data.vtxcoord.left,  y+data.vtxcoord.bottom, data.texcoord.left,  data.texcoord.bottom);
			sDrawi_addTextVertex(x+data.vtxcoord.right, y+data.vtxcoord.bottom, data.texcoord.right, data.texcoord.bottom);
			sDrawi_addTextVertex(x+data.vtxcoord.left,  y+data.vtxcoord.top,    data.texcoord.left,  data.texcoord.top);
			sDrawi_addTextVertex(x+data.vtxcoord.right, y+data.vtxcoord.top,    data.texcoord.right, data.texcoord.top);

			// Draw the glyph
			C3D_DrawArrays(GPU_TRIANGLE_STRIP, arrayIndex, 4);

			x += data.xAdvance;

		}
	} while (code > 0);
}

float sDraw_interface::gettextheight(const char* text, float sizeY)
{
	string tex(text);
	int lines = std::count(tex.begin(), tex.end(), '\n') + 1; //There's always one line that doesn't have a \n
	return (sizeY*fontGetInfo()->lineFeed)*lines;
}

//Get the width of the text input. Account for \n by using a vector of possible widths. Use gettextmaxwidth to return the longest one
vector<float> sDraw_interface::gettextwidths(const char* text, float sizeX, float sizeY)
{
	ssize_t  units;
	uint32_t code;
	int x = 0;

	const uint8_t* p = (const uint8_t*)text;
	u32 flags = GLYPH_POS_CALC_VTXCOORD | 0;
	
	vector<float> xvalues;
	do
	{
		if (!*p) {xvalues.push_back(x); break;}
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
			fontCalcGlyphPos(&data, glyphIdx, flags, sizeX, sizeY);

			x += data.xAdvance;

		}
	} while (code > 0);
	
	return xvalues;
}

float sDraw_interface::gettextmaxwidth(const char* text, float sizeX, float sizeY)
{
	vector<float> widths = gettextwidths(text, sizeX, sizeY);
	//Compare the X values to see which one is the longest.
	return *std::max_element(widths.begin(), widths.end());
}

//Draw centered text by looping through each newline and its equivalent width in a vector.
// (screenwidth/2) - (stringwidth / 2)
void sDraw_interface::drawcenteredtext(const char* text, float scaleX, float scaleY, float y)
{
	vector<float> widths = gettextwidths(text, scaleX, scaleY);
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
			y += scaleY*fontGetInfo()->lineFeed;
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

			int arrayIndex = sdrawVtxArrayPos;
			if ((arrayIndex+4) >= TEXT_VTX_ARRAY_COUNT)
				break; // We can't render more characters

			// Add the vertices to the array
			sDrawi_addTextVertex(x+data.vtxcoord.left,  y+data.vtxcoord.bottom, data.texcoord.left,  data.texcoord.bottom);
			sDrawi_addTextVertex(x+data.vtxcoord.right, y+data.vtxcoord.bottom, data.texcoord.right, data.texcoord.bottom);
			sDrawi_addTextVertex(x+data.vtxcoord.left,  y+data.vtxcoord.top,    data.texcoord.left,  data.texcoord.top);
			sDrawi_addTextVertex(x+data.vtxcoord.right, y+data.vtxcoord.top,    data.texcoord.right, data.texcoord.top);

			// Draw the glyph
			C3D_DrawArrays(GPU_TRIANGLE_STRIP, arrayIndex, 4);

			x += data.xAdvance;

		}
	} while (code > 0);
}

void sDraw_interface::drawtextinrec(const char* text, int x, int y, int width, float scalex, float scaley)
{
	float textwidth = gettextmaxwidth(text, scalex, scaley);
	float finalsizex;
	if(textwidth > width)
	{
		scalex *= (width / textwidth);
		finalsizex = gettextmaxwidth(text, scalex, scaley);
	}
	else finalsizex = textwidth;
	//Center it
	float finalx = ((currentoutput == GFX_TOP ? 400 : 320) - finalsizex) / 2;
	finalx -= 3; //Make it look better
	drawtext(text, finalx, y, scalex, scaley);
}

void sDraw_interface::drawtexture(sdraw_texture* tex, int x, float y)
{
	C3D_TexBind(0, &tex->image);
	
	C3D_TexEnv	*env = C3D_GetTexEnv(0);
	C3D_TexEnvSrc(env, C3D_Both, GPU_TEXTURE0, 0, 0);
	C3D_TexEnvOp(env, C3D_Both, 0, 0, 0);
	C3D_TexEnvFunc(env, C3D_Both, GPU_REPLACE);
	if(x == CENTERED && y == CENTERED) {x = ((currentoutput == GFX_TOP ? 400 : 320) / 2) - (tex->width / 2); y = (240 / 2) - (tex->height / 2);}
	
    sDrawi_addTextVertex(x, y + tex->height, 0.0f, 1); //left bottom
    sDrawi_addTextVertex(x + tex->width, y + tex->height, 1, 1); //right bottom
	sDrawi_addTextVertex(x, y, 0.0f, 0.0f); //left top
	sDrawi_addTextVertex(x + tex->width, y, 1, 0.0f); //right top
	
	C3D_DrawArrays(GPU_TRIANGLE_STRIP, sdrawVtxArrayPos-4, 4);
}

//I'm not certain whether I want this to be in utils or sdraw itself so I'm leaving it in both for now
void sDraw_interface::drawblendedtexture(sdraw_texture* texture1, sdraw_texture* texture2, int x, int y, int blendfactor)
{
	C3D_TexEnv* tev = C3D_GetTexEnv(0);
	C3D_Tex* texa = &texture1->image;
	C3D_Tex* texb = &texture2->image;
	C3D_TexBind(0, texa);
	C3D_TexBind(1, texb);
	//Configure the fragment shader to blend texture0 with texture1 based on the alpha of the constant
	C3D_TexEnvSrc(tev, C3D_RGB, GPU_TEXTURE0, GPU_TEXTURE1, GPU_CONSTANT);
	//One minus alpha to get it to be 0 -> all texture 0, 256 -> all texture1, whereas it would be the opposite otherwise
	C3D_TexEnvOp(tev, C3D_RGB, GPU_TEVOP_RGB_SRC_COLOR, GPU_TEVOP_RGB_SRC_COLOR, GPU_TEVOP_RGB_ONE_MINUS_SRC_ALPHA);
	C3D_TexEnvColor(tev, RGBA8(0,0,0,blendfactor));
	C3D_TexEnvFunc(tev, C3D_Both, GPU_INTERPOLATE);
	
    sDrawi_addTextVertex(x, y + texture1->height, 0.0f, 1); //left bottom
    sDrawi_addTextVertex(x + texture1->width, y + texture1->height, 1, 1); //right bottom
	sDrawi_addTextVertex(x, y, 0.0f, 0.0f); //left top
	sDrawi_addTextVertex(x + texture1->width, y, 1, 0.0f); //right top
	
	C3D_DrawArrays(GPU_TRIANGLE_STRIP, sdrawVtxArrayPos-4, 4);
}

/* finc's implementation
C3D_TexEnvSrc(env, C3D_RGB, GPU_TEXTURE0, GPU_TEXTURE1, GPU_CONSTANT);
C3D_TexEnvOp(env, C3D_RGB, GPU_TEVOP_RGB_SRC_COLOR, GPU_TEVOP_RGB_SRC_COLOR, GPU_TEVOP_RGB_ONE_MINUS_SRC_ALPHA);
C3D_TexEnvSrc(env, C3D_Alpha, GPU_TEXTURE0, GPU_TEXTURE1, 0);
C3D_TexEnvFunc(env, C3D_RGB, GPU_INTERPOLATE);
C3D_TexEnvFunc(env, C3D_Alpha, GPU_MODULATE);
C3D_TexEnvColor(tev, RGBA8(0,0,0,128));
*/


//Helper functions for switching shaders and providing uniforms automagically
void sDraw_interface::usebasicshader()
{
	C3D_SetAttrInfo(&basicinfo);
	C3D_SetBufInfo(&basicbuf);
	shaderinuse = BASICSHADER;
	C3D_BindProgram(&basicprogram);
	C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, uLoc_projection, currentoutput == GFX_TOP ? &projectionTop : &projectionBot);
}

void sDraw_interface::useeventualshader()
{
	C3D_SetAttrInfo(&basicinfo);
	C3D_SetBufInfo(&basicbuf);
	shaderinuse = EVENTUALSHADER;
	C3D_BindProgram(&eventualprogram);
	C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, expand_projectionloc, currentoutput == GFX_TOP ? &projectionTop : &projectionBot);
}

void sDraw_interface::usetwocoordsshader()
{
	C3D_SetAttrInfo(&twocoordsinfo);
	C3D_SetBufInfo(&twocoordsbuf);
	shaderinuse = TWOCOORDSSHADER;
	C3D_BindProgram(&twocoordsprogram);
	C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, twocds_projectionloc, currentoutput == GFX_TOP ? &projectionTop : &projectionBot);
}


void sDraw_interface::drawtexture(sdraw_stex info, int x, int y, int x1, int y1, float interpfactor)
{
	C3D_TexBind(0, &(info.spritesheet->image));
	
	C3D_TexEnv	*env = C3D_GetTexEnv(0);
	C3D_TexEnvSrc(env, C3D_Both, GPU_TEXTURE0, 0, 0);
	C3D_TexEnvOp(env, C3D_Both, 0, 0, 0);
	C3D_TexEnvFunc(env, C3D_Both, GPU_REPLACE);
	
	float rleft = info.x/info.spritesheet->width;
	float rright = (info.x + info.width) /info.spritesheet->width;
	float rtop = info.y/info.spritesheet->height;
	float rbot = (info.y + info.height) / info.spritesheet->height; //Get the real spritesheet coordinates between 0 and 1
	
	
	if(x == CENTERED && y == CENTERED) {x = ((currentoutput == GFX_TOP ? 400 : 320) / 2) - (info.spritesheet->width / 2); y = (240 / 2) - (info.spritesheet->height / 2);}
	
	//If we have a second coordinate we need to activate the interpolation shader and add coordinates to its buffer
	if (x1 != -1)
	{
		usetwocoordsshader();
		C3D_FVUnifSet(GPU_VERTEX_SHADER, twocds_baseinterploc, 1, 0, 0, 0); //No base interpolation
		C3D_FVUnifSet(GPU_VERTEX_SHADER, twocds_interploc, interpfactor, 0, 0, 0);
		sDrawi_addTwoCoordsVertex(x, y + info.height, x1, y1 + info.height, rleft, rbot);
		sDrawi_addTwoCoordsVertex(x + info.width, y + info.height, x1 + info.width, y1 + info.height, rright, rbot);
		sDrawi_addTwoCoordsVertex(x, y, x1, y1, rleft, rtop);
		sDrawi_addTwoCoordsVertex(x + info.width, y, x1 + info.width, y1, rright, rtop);
		C3D_DrawArrays(GPU_TRIANGLE_STRIP, sdrawTwoCdsVtxArrayPos - 4, 4);
		usebasicshader();
	}
	else
	{
		sDrawi_addTextVertex(x, y + info.height, rleft, rbot); //left bottom
		sDrawi_addTextVertex(x + info.width, y + info.height, rright, rbot); //right bottom
		sDrawi_addTextVertex(x, y, rleft, rtop); //left top
		sDrawi_addTextVertex(x + info.width, y, rright, rtop); //right top
		C3D_DrawArrays(GPU_TRIANGLE_STRIP, sdrawVtxArrayPos - 4, 4);
	}
}

//Draw a framebuffer, it's tilted sideways and stuffed into a larger texture and flipped so we need some extra maths for this
void sDraw_interface::drawframebuffer(C3D_Tex tex, int x, int y, bool istopfb, int x1, int y1, float interpfactor)
{
	C3D_TexBind(0, &tex);
	
	C3D_TexEnv	*env = C3D_GetTexEnv(0);
	C3D_TexEnvSrc(env, C3D_Both, GPU_TEXTURE0, 0, 0);
	C3D_TexEnvOp(env, C3D_Both, 0, 0, 0);
	C3D_TexEnvFunc(env, C3D_Both, GPU_REPLACE);
	const float scrwidth = 240, scrheight = istopfb ? 400 : 320;
	const float texwidth = 256, texheight = 512;
	
	if (x1 != -1) //See above function
	{
		usetwocoordsshader();
		C3D_FVUnifSet(GPU_VERTEX_SHADER, twocds_baseinterploc, 1, 0, 0, 0); //No base interpolation
		C3D_FVUnifSet(GPU_VERTEX_SHADER, twocds_interploc, interpfactor, 0, 0, 0);
		sDrawi_addTwoCoordsVertex(x, y + scrwidth, x1, y1 + scrwidth, 0, (scrheight / texheight)); //left bottom
		sDrawi_addTwoCoordsVertex(x + scrheight, y + scrwidth, x1 + scrheight, y1 + scrwidth, 0, 0); //right bottom
		sDrawi_addTwoCoordsVertex(x, y, x1, y1, (scrwidth / texwidth), (scrheight / texheight)); //left top
		sDrawi_addTwoCoordsVertex(x + scrheight, y, x1 + scrheight, y1, (scrwidth / texwidth), 0); //right top
		C3D_DrawArrays(GPU_TRIANGLE_STRIP, sdrawTwoCdsVtxArrayPos - 4, 4);
		usebasicshader();
	}
	else
	{
		sDrawi_addTextVertex(x, y + scrwidth, 0, (scrheight / texheight)); //left bottom
		sDrawi_addTextVertex(x + scrheight, y + scrwidth, 0, 0); //right bottom
		sDrawi_addTextVertex(x, y, (scrwidth / texwidth), (scrheight / texheight)); //left top
		sDrawi_addTextVertex(x + scrheight, y, (scrwidth / texwidth), 0); //right top
		C3D_DrawArrays(GPU_TRIANGLE_STRIP, sdrawVtxArrayPos - 4, 4);
	}
	/*sDrawi_addTextVertex(x, y + tex.height - 450, 0, 0); //left bottom
    sDrawi_addTextVertex(x + tex.width, y + tex.height - 450, 1, 0); //right bottom
	sDrawi_addTextVertex(x, y - 450, 0, 1); //left top
	sDrawi_addTextVertex(x + tex.width, y - 450, 1, 1); //right top*/
	
}

void sDraw_interface::drawtexturewithhighlight(sdraw_stex info, int x, int y, int alpha)
{
	//Enable writing to the stencil buffer and draw the texture
	C3D_StencilTest(true, GPU_ALWAYS, 1, 0xFF, 0xFF);
	C3D_StencilOp(GPU_STENCIL_KEEP, GPU_STENCIL_KEEP, GPU_STENCIL_REPLACE);
	drawtexture(info, x, y);
	
	float middlex = x + info.width/2;
	float middley = y + info.height/2;
	useeventualshader(); //We can also use this to extrapolate beyond the eventual value, allowing us to make a highlighter
	C3D_FVUnifSet(GPU_VERTEX_SHADER, expand_baseloc, middlex, middley, 0, 0);
	C3D_FVUnifSet(GPU_VERTEX_SHADER, expand_expandloc, 1.10, 0, 0, 0);
	C3D_StencilTest(true, GPU_NOTEQUAL, 1, 0xFF, 0x00); //Turn off writes and allow a pass if it hasn't been set
	
	C3D_TexEnv* tev = C3D_GetTexEnv(0);
	C3D_TexEnvSrc(tev, C3D_RGB, GPU_CONSTANT, 0, 0);
	C3D_TexEnvSrc(tev, C3D_Alpha, GPU_TEXTURE0, GPU_CONSTANT, 0);
	C3D_TexEnvOp(tev, C3D_Both, 0, 0, 0);
	C3D_TexEnvFunc(tev, C3D_RGB, GPU_REPLACE);
	C3D_TexEnvFunc(tev, C3D_Alpha, GPU_MODULATE);
	C3D_TexEnvColor(tev, RGBA8(255, 0, 0, alpha));
	
	C3D_DrawArrays(GPU_TRIANGLE_STRIP, sdrawVtxArrayPos-4, 4); //No need to add these vertices again
	
	//TODO: Ensure previous shader state is kept instead of switching back to the basic shader
	usebasicshader();
	C3D_StencilTest(false, GPU_NEVER, 0, 0, 0);
}

//Draw with 0.75 texcoords as it's a 48x48 icon in a 64x64 texture (Power of two limits...)
//Also width/height is constant- 48x48
void sDraw_interface::drawSMDHicon(C3D_Tex icon, int x, int y)
{
	C3D_TexBind(0, &icon);
	C3D_TexEnv	*env = C3D_GetTexEnv(0);
	C3D_TexEnvSrc(env, C3D_Both, GPU_TEXTURE0, 0, 0);
	C3D_TexEnvOp(env, C3D_Both, 0, 0, 0);
	C3D_TexEnvFunc(env, C3D_Both, GPU_REPLACE);
	
	if(x == CENTERED && y == CENTERED) {x = ((currentoutput == GFX_TOP ? 400 : 320) / 2) - (48 / 2); y = (240 / 2) - (48 / 2);}
	
    sDrawi_addTextVertex(x, y + 48, 0, 0); //left bottom
    sDrawi_addTextVertex(x + 48, y + 48, 0.75, 0); //right bottom
	sDrawi_addTextVertex(x, y, 0, 0.75); //left top
	sDrawi_addTextVertex(x + 48, y, 0.75, 0.75); //right top
	
	C3D_DrawArrays(GPU_TRIANGLE_STRIP, sdrawVtxArrayPos-4, 4);
}

//TODO: have all these different functions for individual texenv stuff configure their texenvs then call a drawquad() function.
//It's really getting out of hand and not making sense to repeat all this code.
void sDraw_interface::drawtexture_replacealpha(sdraw_stex info, int x, int y, int alpha, int x1, int y1, float interpfactor)
{
	C3D_TexBind(0, &(info.spritesheet->image));
	
	C3D_TexEnv* tev = C3D_GetTexEnv(0);
	C3D_TexEnvSrc(tev, C3D_RGB, GPU_TEXTURE0, 0, 0);
	C3D_TexEnvSrc(tev, C3D_Alpha, GPU_TEXTURE0, GPU_CONSTANT, 0);
	C3D_TexEnvFunc(tev, C3D_RGB, GPU_REPLACE);
	C3D_TexEnvFunc(tev, C3D_Alpha, GPU_MODULATE);
	C3D_TexEnvColor(tev, RGBA8(0, 0, 0, alpha));
	
	float rleft = info.x/info.spritesheet->width;
	float rright = (info.x + info.width) /info.spritesheet->width;
	float rtop = info.y/info.spritesheet->height;
	float rbot = (info.y + info.height) / info.spritesheet->height; //Get the real spritesheet coordinates between 0 and 1
	
	
	if(x == CENTERED && y == CENTERED) {x = ((currentoutput == GFX_TOP ? 400 : 320) / 2) - (info.spritesheet->width / 2); y = (240 / 2) - (info.spritesheet->height / 2);}
	
	//If we have a second coordinate we need to activate the interpolation shader and add coordinates to its buffer
	if (x1 != -1)
	{
		usetwocoordsshader();
		C3D_FVUnifSet(GPU_VERTEX_SHADER, twocds_interploc, interpfactor, 0, 0, 0);
		C3D_FVUnifSet(GPU_VERTEX_SHADER, twocds_baseinterploc, 1, 0, 0, 0); //No base interpolation
		sDrawi_addTwoCoordsVertex(x, y + info.height, x1, y1 + info.height, rleft, rbot);
		sDrawi_addTwoCoordsVertex(x + info.width, y + info.height, x1 + info.width, y1 + info.height, rright, rbot);
		sDrawi_addTwoCoordsVertex(x, y, x1, y1, rleft, rtop);
		sDrawi_addTwoCoordsVertex(x + info.width, y, x1 + info.width, y1, rright, rtop);
		C3D_DrawArrays(GPU_TRIANGLE_STRIP, sdrawTwoCdsVtxArrayPos - 4, 4);
		usebasicshader();
	}
	else
	{
		sDrawi_addTextVertex(x, y + info.height, rleft, rbot); //left bottom
		sDrawi_addTextVertex(x + info.width, y + info.height, rright, rbot); //right bottom
		sDrawi_addTextVertex(x, y, rleft, rtop); //left top
		sDrawi_addTextVertex(x + info.width, y, rright, rtop); //right top
		C3D_DrawArrays(GPU_TRIANGLE_STRIP, sdrawVtxArrayPos - 4, 4);
	}
	
	C3D_DrawArrays(GPU_TRIANGLE_STRIP, sdrawVtxArrayPos-4, 4);
}

void sDraw_interface::framestart()
{
	C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
	sdrawVtxArrayPos = 0; //Reset the vertex arrays
	sdrawTwoCdsVtxArrayPos = 0;
	drawon(GFX_TOP); //Reset default output to the top screen
}

void sDraw_interface::frameend()
{
	//Get the final framebuffer of this frame and save it
	C3D_FrameSplit(0); //Wait for all rendering operations to complete
	//So the ctrulib docs have lies, GX_BUFFER_DIM is not width and height, but rather what 16-byte blocks to copy
	//And what 16-byte blocks to skip copying. Also >>4 because "that's how it works".
	//Other than that, self-explanatory if you stare at the numbers long enough.
	GX_TextureCopy(
    (u32*)top->frameBuf.colorBuf, GX_BUFFER_DIM((240*8*4)>>4, 0),
    (u32*)lastfbtop.data + (512-400)*256, GX_BUFFER_DIM((240*8*4)>>4, ((256-240)*8*4)>>4),
    240*400*4, FRAMEBUFFER_TRANSFER_FLAGS);
	GX_TextureCopy(
    (u32*)bottom->frameBuf.colorBuf, GX_BUFFER_DIM((240*8*4)>>4, 0),
    (u32*)lastfbbot.data + (512-320)*256, GX_BUFFER_DIM((240*8*4)>>4, ((256-240)*8*4)>>4),
    240*320*4, FRAMEBUFFER_TRANSFER_FLAGS);
	C3D_FrameEnd(0);
}


void sDraw_interface::cleanup()
{
	// Free the textures
	free(glyphSheets);
	C3D_TexDelete(&lastfbtop);
	C3D_TexDelete(&lastfbbot);
	// Free the shader programs
	shaderProgramFree(&basicprogram);
	DVLB_Free(basicvshader_dvlb);
	shaderProgramFree(&eventualprogram);
	DVLB_Free(eventualvshader_dvlb);
	C3D_Fini();
}
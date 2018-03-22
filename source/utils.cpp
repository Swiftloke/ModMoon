#include <string.h>
#include <3ds.h>
#include <stdio.h>
#include <unistd.h>
#include <sstream>
#include <sys/stat.h>

#include "utils.hpp"
#include "main.hpp"

using namespace std;

Handle event_fadefinished;

inline bool pathExist(const string filename){ //The compiler doesn't like me defining this via a forward definition, so copy/paste. Thanks GCC
    struct stat buffer;
    return (stat (filename.c_str(),& buffer)==0);
}

int maxslotcheck()
{
	int currentFolderCount = 0;
	stringstream path2Check;
	stringstream NewPath2Check;
	do {
		currentFolderCount++;
		path2Check.str("");
		path2Check << modsfolder + currenttitleidstr << '/' << "Slot_" << currentFolderCount << '/';
	} while (pathExist(path2Check.str()) || currentFolderCount == currentslot);
	//the minus 1 is due to it returning the folder number that doesnt exist.
	return currentFolderCount - 1;
}


void threadfunc_fade(void* main)
{
	int alpha = 0;
	int* rgbvalues = (int*)main; //We expect an array of 3 ints for the RGB values
	//C3D_AlphaBlend(GPU_BLEND_ADD, GPU_BLEND_ADD, GPU_ONE_MINUS_CONSTANT_ALPHA, GPU_CONSTANT_COLOR, GPU_ONE_MINUS_CONSTANT_ALPHA, GPU_CONSTANT_ALPHA);
	//I tried to use blending operations but the equation I used didn't like an alpha value of < 1 for a texture, and overwrote it entirely with the color. It looked gross.
	while(alpha <= 255)
	{
		alpha += 3;
		draw.framestart();
		draw.drawtexture(backgroundtop, 0, 0);
		int bannerx = 400/2 - banner.width/2;
		float bannery = 240/2 - banner.height/2;
		bannery += 6.0f*sinf(C3D_Angle(minusy)); //Wr're not moving it, but we are keeping it where it's at.
		draw.drawtexture(banner, bannerx, bannery);
		draw.drawrectangle(0, 0, 400, 240, RGBA8(rgbvalues[0], rgbvalues[1], rgbvalues[2], alpha)); //Overlay an increasingly covering rectangle for a fade effect
		draw.drawon(GFX_BOTTOM);
		draw.drawtexture(backgroundbot, 0, 0);
		draw.drawtexture(leftbutton, 0, 13);
		draw.drawtexture(rightbutton, 169, 13);
		draw.drawtexture(selector, 0, 159);
		draw.drawtextinrec(slotname.c_str(), 35, 180, 251, 1.4, 1.4);
		draw.drawrectangle(0, 0, 320, 240, RGBA8(rgbvalues[0], rgbvalues[1], rgbvalues[2], alpha));
		draw.frameend();
	}
	svcSignalEvent(event_fadefinished);
}

/*static int fadepoint;
static sdraw_stex fadea, fadeb;

void texfadeinit(sdraw_stex texa, sdraw_stex texb)
{
	fadepoint = 0;
	fadea = texa;
	fadeb = texb;
}*/

//Citro3D port of this https://www.khronos.org/opengl/wiki/Texture_Combiners#Example_:_Blend_tex0_and_tex1_based_on_a_blending_factor_you_supply
// *Lack of fragment shader intensifies*

//I'm not certain whether I want this to be in utils or sdraw itself so I'm leaving it in both for now
void texfadeadvance(C3D_Tex* texture1, C3D_Tex* texture2, int blendfactor)
{
	C3D_TexEnv* tev = C3D_GetTexEnv(0);
	C3D_Tex* texa = texture1;
	C3D_Tex* texb = texture2;
	C3D_TexBind(0, texa);
	C3D_TexBind(1, texb);
	//Configure the fragment shader to blend texture0 with texture1 based on the alpha of the constant
	C3D_TexEnvSrc(tev, C3D_RGB, GPU_TEXTURE0, GPU_TEXTURE1, GPU_CONSTANT);
	//One minus alpha to get it to be 0 -> all texture 0, 256 -> all texture1, whereas it would be the opposite otherwise
	C3D_TexEnvOp(tev, C3D_RGB, GPU_TEVOP_RGB_SRC_COLOR, GPU_TEVOP_RGB_SRC_COLOR, GPU_TEVOP_RGB_ONE_MINUS_SRC_ALPHA);
	C3D_TexEnvColor(tev, RGBA8(0,0,0,blendfactor));
	C3D_TexEnvFunc(tev, C3D_Both, GPU_INTERPOLATE);
	
}

/* finc's implementation
C3D_TexEnvSrc(env, C3D_RGB, GPU_TEXTURE0, GPU_TEXTURE1, GPU_CONSTANT);
C3D_TexEnvOp(env, C3D_RGB, GPU_TEVOP_RGB_SRC_COLOR, GPU_TEVOP_RGB_SRC_COLOR, GPU_TEVOP_RGB_ONE_MINUS_SRC_ALPHA);
C3D_TexEnvSrc(env, C3D_Alpha, GPU_TEXTURE0, GPU_TEXTURE1, 0);
C3D_TexEnvFunc(env, C3D_RGB, GPU_INTERPOLATE);
C3D_TexEnvFunc(env, C3D_Alpha, GPU_MODULATE);
C3D_TexEnvColor(tev, RGBA8(0,0,0,128));
*/


void launch(){

	int rgb[3] = {0, 0, 0}; //Fade to black
	
	s32 mainthreadpriority;
	svcGetThreadPriority(&mainthreadpriority, CUR_THREAD_HANDLE);
	svcCreateEvent(&event_fadefinished, RESET_ONESHOT);
	threadCreate(threadfunc_fade, rgb, 8000, mainthreadpriority + 1, -2, true);
	config.flush();
	if(modsenabled)
	{
		string dest = issaltysdtitle() ? "/saltysd/smash" : "/luma/titles/" + currenttitleidstr + '/';
		if(rename((modsfolder + "Slot_" + to_string(currentslot)).c_str(), dest.c_str()))
		{
			//error("Failed to move slot file from " + modsfolder + "/Slot_" + to_string(currentslot) + "to /saltysd/smash!")
		}
	}
	svcWaitSynchronization(event_fadefinished, U64_MAX);
	draw.framestart(); //Prevent wonkiness with the app jump
	draw.drawrectangle(0, 0, 400, 240, 0); //Not actually drawn, just there to prevent Citro3D hang. Clear color does this for real
	draw.drawon(GFX_BOTTOM);
	draw.drawrectangle(0, 0, 320, 240, 0);
	draw.frameend();
	draw.cleanup();
	u8 param[0x300];
	u8 hmac[0x20];
	memset(param, 0, sizeof(param));
	memset(hmac, 0, sizeof(hmac));

	APT_PrepareToDoApplicationJump(0, currenttitleid, (*getSMDHdata())[currenttidpos].gametype);

	APT_DoApplicationJump(param, sizeof(param), hmac);
}

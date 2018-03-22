#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <sys/stat.h>
#include <cmath>

#include <3ds.h>

#include "sdraw.hpp"
#include "config.hpp"
#include "utils.hpp"
#include "main.hpp"

using namespace std;

Result res = romfsInit(); //Preinit romfs so we can load the spritesheet
sDraw_interface draw;

sdraw_texture* spritesheet = loadpng("romfs:/spritesheet.png"); //Texture conversion for this doesn't fucking work >:(
sdraw_texture* progressfiller = loadbin("romfs:/progress.bin", 32, 32); //This needs to be in its own texture due to usage of wrapping for animation
sdraw_stex leftbutton(spritesheet, 0, 324, 152, 134);
sdraw_stex leftbuttonenabled(spritesheet, 0, 458, 152, 134);
sdraw_stex rightbutton(spritesheet, 153, 324, 151, 134);
sdraw_stex selector(spritesheet, 0, 241, 320, 81);
sdraw_stex backgroundbot(spritesheet, 0, 0, 320, 240);
sdraw_stex backgroundtop(spritesheet, 320, 129, 400, 240);
sdraw_stex banner(spritesheet, 320, 0, 256, 128);
sdraw_stex textbox(spritesheet, 0, 592, 300, 200);
sdraw_stex textboxokbutton(spritesheet, 152, 458, 87, 33);
sdraw_stex textboxokbuttonhighlight(spritesheet, 152, 491, 89, 35);
sdraw_stex titleselectionboxes(spritesheet, 0, 792, 268, 198);
sdraw_stex titleselectionsinglebox(spritesheet, 0, 792, 58, 58);
sdraw_stex titleselecthighlighter(spritesheet, 268, 792, 65, 65);
sdraw_stex progressbar(spritesheet, 720, 240, 260, 35);
sdraw_stex progressbarstenciltex(spritesheet, 720, 275, 260, 35);

Config config("/3ds/data/ModMoon/", "settings.txt");

bool modsenabled = config.read("ModsEnabled", true);

string modsfolder = config.read("ModsFolder");

/*int gameregion = 0; //Initialized in setup function
FS_MediaType gametype = MEDIATYPE_SD;

string titleids[3] = {"00040000000EDF00", "00040000000EE000", "00040000000B8B00"};
u64 tids[3] = {0x00040000000EDF00, 0x00040000000EE000, 0x00040000000B8B00};*/

vector<u64> titleids = config.u64multiread("ActiveTitleIDs", true);
vector<int> slots = config.intmultiread("TitleIDSlots");
//Where is it in the vector? We need this for selection and for accessing its slot
int currenttidpos = config.read("SelectedTitleIDPos", 0);

int maxslot = maxslotcheck();

string slotname = "";

float minusy = 0;

inline bool pathExist(const string filename){ //The compiler doesn't like me defining this via a forward definition, so copy/paste. Thanks GCC
    struct stat buffer;
    return (stat (filename.c_str(),& buffer)==0);
}

string tid2str(u64 in)
{
	stringstream out;
	out << std::hex;
	out << std::setw(16) << std::setfill('0'); //Fix leading zeros
	out << std::uppercase; //We want an uppercase string for accessing folders
	out << in;
	return out.str();
}

bool issaltysdtitle()
{
	return currenttitleid == 0x00040000000EDF00 || currenttitleid ==  0x00040000000EE000 \
	|| currenttitleid == 0x00040000000B8B00;
}

void startup()
{
	if(modsenabled)
	{
		string source = issaltysdtitle() ? "/saltysd/smash" : "/luma/titles/" + currenttitleidstr + '/';
		if(rename(source.c_str(), (modsfolder + currenttitleidstr + '/' + "Slot_" + to_string(currentslot)).c_str()))
		{
			error("Failed to move slot file from /saltysd/smash to " + modsfolder + "/Slot_" + to_string(currentslot) + '!');
		}
	}
	mainmenuupdateslotname();
	cfguInit(); //For system language
	initializeallSMDHdata(titleids);
}

void enablemods(bool isenabled)
{
	modsenabled = isenabled;
}

void touchwaitforrelease()
{
	while(!(hidKeysUp() & KEY_TOUCH))
	{
		hidScanInput();
	}
}
bool touched(sdraw_stex button, int dx, int dy, touchPosition tpos)
{
	return tpos.px >= dx && tpos.px <= dx + button.width && tpos.py >= dy && tpos.py <= dy + button.height;
}
bool touched(int x, int y, int width, int height, touchPosition tpos)
{
	return tpos.px >= x && tpos.px <= x + width && tpos.py >= y && tpos.py <= y + height;
}

//This function differs from simply checking if it's touched: it checks if during the last frame the button was pressed
//And this frame the touch screen was released. Therefore, the user was pressing the button but then released it so we should do *something* now.
bool buttonpressed(sdraw_stex button, int bx, int by, touchPosition lastpos, u32 kHeld)
{
	return touched(button, bx, by, lastpos) && !(kHeld & KEY_TOUCH);
}

//This should be done with a shader instead- I wrote this when I wasn't good at OpenGL
void mainmenushiftin()
{
	//Shift the buttons in. if l equals 0 we're done all the buttons are moved in. Handle b separately as we're moving less
	//And along the y coordinate here.
	for(int l = -(leftbutton.width), r = 320, b = 240; l < 0; l += 10, r -= 10)
	{
		draw.framestart();
		drawtopscreen();
		draw.drawon(GFX_BOTTOM);
		draw.drawtexture(backgroundbot, 0, 0);
		draw.drawtexture(leftbutton, l, 13);
		draw.drawtexture(rightbutton, r, 13);
		draw.drawtexture(selector, 0, b);
		//21 is the distance from the top of the selection bar to the top of the text
		draw.drawtextinrec(slotname.c_str(), 35, 21 + b, 251, 1.4, 1.4);
		draw.frameend();
		if(b >= 161) b -= 6; //159 plus 2
		if(b < 161) b = 159; //Handle b not quite hitting what we want
	}


}

void mainmenushiftinb()
{
	for (float i = 0; i <= 1.0; i += 0.08)
	{
		draw.framestart();
		drawtopscreen();
		draw.drawon(GFX_BOTTOM);
		draw.drawtexture(backgroundbot, 0, 0);
		draw.drawtexture(leftbutton, -leftbutton.width, 13, 0, 13, i);
		draw.drawtexture(rightbutton, 320, 13, 169, 13, i);
		draw.drawtexture(selector, 0, 240, 0, 159, i);
		draw.frameend();
	}
}

void mainmenushiftout()
{
	//Opposite of shifting in (just some numbers changed)
	for(int l = 0, r = 169, b = 159; l > -(leftbutton.width); l -= 10, r += 10)
	{
		draw.framestart();
		drawtopscreen();
		draw.drawon(GFX_BOTTOM);
		draw.drawtexture(backgroundbot, 0, 0);
		draw.drawtexture(leftbutton, l, 13);
		draw.drawtexture(rightbutton, r, 13);
		draw.drawtexture(selector, 0, b);
		draw.drawtextinrec(slotname.c_str(), 35, 21 + b, 251, 1.4, 1.4);
		draw.frameend();
		if(b < 240) b += 6;
	}
}

void mainmenuupdateslotname()
{
	if(currentslot == 0) {slotname = "Disabled"; return;}
	ifstream in(modsfolder + currenttitleidstr + '/' + "Slot_" + to_string(currentslot) + "/desc.txt");
	if(!in) {slotname = "Slot " + to_string(currentslot); return;}
	getline(in, slotname);
	in.close();
}

void updateslots(bool plus)
{
	int oldslot = currentslot;
	currentslot += (plus ? 1 : -1);
	if(currentslot == -1) currentslot = maxslot; //
	if(currentslot > maxslot) currentslot = 0;
	if(currentslot == 0) enablemods(false);
	else if(oldslot == 0 && currentslot) enablemods(true); //It was moved from disabled
	
	mainmenuupdateslotname();
}

//All the things happen on the bottom screen, very rarely do we deviate from this pattern on the top screen
void drawtopscreen()
{
	draw.drawtexture(backgroundtop, 0, 0);
	int bannerx = 400 / 2 - banner.width / 2;
	float bannery = 240 / 2 - banner.height / 2;
	//Inspiration from new-hbmenu
	minusy -= 0.0052; // 1/192, the compiler didn't like assigning a division value for some reason.
	float addy = 6.0f*sinf(C3D_Angle(minusy));
	bannery += addy;
	draw.drawtexture(banner, bannerx, bannery);
	//Draw the title selection text
	draw.settextcolor(RGBA8(165, 165, 165, 255));
	draw.drawtext(": Enable/Disable mods", 5, 240 - 40, 0.55, 0.55);
	draw.drawtext(": Title selection", 5, 240 - 20, 0.55, 0.55);
	//Draw the current title
	draw.drawSMDHicon((*getSMDHdata())[currenttidpos].icon, 400 - 48 - 7, 240 - 48 - 7);
	draw.drawtexture(titleselectionsinglebox, 400 - 58 - 2, 240 - 58 - 2);
}

void mainmenudraw(int dpadpos, touchPosition tpos, int alphapos, bool highlighterblink)
{
	draw.drawtexture(backgroundbot, 0, 0);
	if (dpadpos == 0)
		draw.drawtexturewithhighlight(touched(leftbutton, 0, 13, tpos) ? leftbuttonenabled : leftbutton, 0, 13, alphapos);
	else draw.drawtexture(touched(leftbutton, 0, 13, tpos) ? leftbuttonenabled : leftbutton, 0, 13);
	if (dpadpos == 1)
		draw.drawtexturewithhighlight(rightbutton, 169, 13, alphapos);
	else draw.drawtexture(rightbutton, 169, 13);
	if (dpadpos == 2 || dpadpos == 3)
		draw.drawtexturewithhighlight(selector, 0, 159, alphapos);
	else draw.drawtexture(selector, 0, 159);
	draw.settextcolor(RGBA8(0, 0, 0, 255));
	draw.drawtextinrec(slotname.c_str(), 35, 180, 251, 1.4, 1.4);
}

int main(int argc, char **argv) {

	startup();
	touchPosition tpos;
	touchPosition opos;
	int alphapos = 0;
	bool alphaplus = true;
	int dpadpos = 0;
	
	mainmenushiftinb();
	while (aptMainLoop()) {
		hidScanInput();
		opos = tpos;
		hidTouchRead(&tpos);
		u32 kDown = hidKeysDown();
		u32 kHeld = hidKeysHeld();
		//So 0 is the launch button, 1 is the tools button,
		//2 is the selector bar that, when pressed up, goes back to the launch button, and 3 is the same for the tools button.
		if(kDown & KEY_A)
		{
			switch(dpadpos)
			{
				case 0: launch(); break;
				case 1: error("Progress Bar Test"); break;
			}
		}
		if(kDown & KEY_LEFT)
		{
			if(dpadpos == 1) //Is it not already on the left side?
			{
				dpadpos--;
				alphapos = 255;
			}
			else if(dpadpos == 2 || dpadpos == 3)
			{
				//Draw the left arrow with a highlight for one frame
				updateslots(false);
			}
			
		}
		if(kDown & KEY_RIGHT)
		{
			if(dpadpos == 0) 
			{
				dpadpos++; //Ditto
				alphapos = 255;
			}
			else if(dpadpos == 2 || dpadpos == 3)
			{
				//Draw the right arrow with a highlight for one frame
				updateslots(true);
			}
		}
		if(kDown & KEY_UP)
		{
			if(dpadpos == 2)
			{
				dpadpos = 0;
				alphapos = 255;
			}
			else if(dpadpos == 3)
			{
				dpadpos = 1;
				alphapos = 255;
			}
		}
		if(kDown & KEY_DOWN)
		{
			if(dpadpos == 0)
			{
				dpadpos = 2;
				alphapos = 255;
			}
			else if(dpadpos == 1)
			{
				dpadpos = 3;
				alphapos = 255;
			}
		}
		if (kDown & KEY_Y) titleselect();
		if(kDown & KEY_START) break;
		if(touched(leftbutton, 0, 13, opos) & !(kHeld & KEY_TOUCH)) //It was touched last frame and released this frame
			launch();
		if(touched(4, 180, 28, 42, opos) && !(kHeld & KEY_TOUCH)) //Left button coordinates
			updateslots(false);
		if(touched(288, 180, 28, 42, opos) && !(kHeld & KEY_TOUCH)) //Right button coordinates
			updateslots(true);
		if(touched(rightbutton, 169, 13, opos) && !(kHeld & KEY_TOUCH))
			error("The tools menu is not\nyet implemented. Sorry!");
		draw.framestart();
		drawtopscreen();
		draw.drawon(GFX_BOTTOM);
		#define PLUSVALUE 5
		if(alphaplus)
		{
			alphapos += PLUSVALUE;
			if(alphapos > 255) {alphapos -= PLUSVALUE; alphaplus = false;}
		}
		else
		{
			alphapos -= PLUSVALUE;
			if(alphapos < 0) {alphapos += PLUSVALUE; alphaplus = true;}
		}
		mainmenudraw(dpadpos, tpos, alphapos, false);
		draw.frameend();
	}

	/*int rgb[3] = {255, 255, 255}; //Fade to white 
	s32 mainthreadpriority;
	svcGetThreadPriority(&mainthreadpriority, CUR_THREAD_HANDLE);
	svcCreateEvent(&event_fadefinished, RESET_ONESHOT);
	threadCreate(threadfunc_fade, rgb, 8000, mainthreadpriority + 1, -2, true);*/
	mainmenushiftout();
	
	if (modsenabled)
	{
		string dest = issaltysdtitle() ? "/saltysd/smash" : "/luma/titles/" + currenttitleidstr + '/';
		if (rename((modsfolder + "Slot_" + to_string(currentslot)).c_str(), dest.c_str()))
		{
			//error("Failed to move slot file from " + modsfolder + "/Slot_" + to_string(currentslot) + "to /saltysd/smash!")
		}
	}
	config.flush();
	
	//svcWaitSynchronization(event_fadefinished, U64_MAX);
	C3D_TexDelete(&(spritesheet->image));
	freeSMDHdata();
	draw.cleanup();
	romfsExit();
	gfxExit();
	cfguExit();
	return 0;
}

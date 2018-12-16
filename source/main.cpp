#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <sys/stat.h>
#include <cmath>
#include <errno.h>
#include <algorithm>
#include <cctype>

#include <3ds.h>

#include "sdraw/sdraw.hpp"
#include "config.hpp"
#include "utils.hpp"
#include "main.hpp"
#include "error.hpp"
#include "titleselects.hpp"
#include "srv.hpp"
#include "download.hpp"
#include "toolsmenu.hpp"
#include "initialsetup.hpp"

using namespace std;

Result notused = romfsInit(); //Preinit romfs so we can load textures
int unused = sdraw::init(); //We need this preinited as well

Config config("/3ds/ModMoon/", "settings.txt");

//Due to their usage in globals construction, these need to be read immediately.
vector<int> mainmenuhighlightcolors = config.intmultiread("MainMenuHighlightColors");
vector<int> errorhighlightcolors = config.intmultiread("ErrorHighlightColors");
vector<int> titleselecthighlightcolors = config.intmultiread("TitleSelectHighlightColors");
vector<int> toolsmenuhighlightcolors = config.intmultiread("ToolsMenuHighlightColors");

//C3D_Tex* spritesheet = loadpng("romfs:/spritesheet.png"); //Texture conversion for this doesn't fucking work >:(
std::pair<C3D_Tex*, Tex3DS_Texture> spritesheet = loadTextureFromFile("romfs:/spritesheet.t3x");
C3D_Tex* progressfiller = loadbin("romfs:/progress.bin", 32, 32); //This needs to be in its own texture due to usage of wrapping for animation
C3D_Tex* rainbow = loadbin("romfs:/rainbow.bin", 256, 256); //Ditto; needs its own texture for animation
sdraw_stex leftbutton(0, spritesheet, true);//(spritesheet, 0, 324, 152, 134, true);
sdraw_stex leftbuttonmoon(1, spritesheet, true);//(spritesheet, 0, 458, 152, 134, true);
sdraw_stex rightbutton(2, spritesheet, true);//(spritesheet, 153, 324, 151, 134, true);
sdraw_stex selector(3, spritesheet, true);//(spritesheet, 0, 241, 320, 81, true);
sdraw_stex backgroundbot(4, spritesheet, true);//(spritesheet, 0, 0, 320, 240, true);
sdraw_stex backgroundtop(5, spritesheet, true);//(spritesheet, 320, 129, 400, 240, true);
sdraw_stex banner(6, spritesheet, false);//(spritesheet, 320, 0, 292, 128, false);
sdraw_stex bannermoonalpha(7, spritesheet, false);//(spritesheet, 612, 0, 90, 106, false);
sdraw_stex textbox(8, spritesheet, true);//(spritesheet, 0, 592, 300, 200, true);
sdraw_stex textboxokbutton(9, spritesheet, true);//(spritesheet, 152, 458, 87, 33, true);
sdraw_highlighter textboxokbuttonhighlight(10, spritesheet,
	RGBA8(errorhighlightcolors[0], errorhighlightcolors[1], errorhighlightcolors[2], 0), false);
sdraw_stex titleselectionboxes(11, spritesheet, true);//(spritesheet, 0, 792, 268, 198, true);
sdraw_stex titleselectionsinglebox(12, spritesheet, true);//(spritesheet, 0, 792, 58, 58, true);
sdraw_stex titleselectioncartridge(13, spritesheet, false);//(spritesheet, 268, 857, 70, 66, false);
sdraw_highlighter titleselecthighlighter(14, spritesheet,
	RGBA8(titleselecthighlightcolors[0], titleselecthighlightcolors[1], titleselecthighlightcolors[2], 0), false);
sdraw_stex progressbar(15, spritesheet, true);//(spritesheet, 720, 240, 260, 35, true);
sdraw_stex progressbarstenciltex(16, spritesheet, true);//(spritesheet, 720, 275, 260, 35, true);
sdraw_stex secret(17, spritesheet, false);//(spritesheet, 320, 369, 114, 113, false);
sdraw_highlighter toolsmenuhighlighter(18, spritesheet,
	RGBA8(toolsmenuhighlightcolors[0], toolsmenuhighlightcolors[1], toolsmenuhighlightcolors[2], 0), false);
sdraw_stex activetitlesbutton(19, spritesheet, false);//(spritesheet, 706, 428, 289, 45, false);
sdraw_stex smashcontrolsbutton(20, spritesheet, false);//(spritesheet, 706, 475, 289, 41, false);
sdraw_stex tutorialbutton(21, spritesheet, false);//(spritesheet, 706, 518, 289, 42, false);
sdraw_stex migrationbutton(22, spritesheet, false);//(spritesheet, 706, 560, 289, 51, false);
sdraw_stex darkmodebutton(23, spritesheet, false);//(spritesheet, 706, 611, 289, 46, false);
sdraw_stex lightmodebutton(24, spritesheet, false);//(spritesheet, 706, 657, 289, 46, false);

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

bool cartridgeneedsupdating = false;

bool shoulddisableerrors = config.read("DisableErrors", true);
bool shoulddisableupdater = config.read("DisableUpdater", true);

string slotname = "";

float minusy = 0;

string getversion()
{
	//After each digit, insert a period.
	string version = to_string(config.read("ModMoonVersion", 0));
	//Don't count the last digit.
	for (unsigned int i = 0; i < version.size() - 1; i++)
	{
		char c = version.at(i);
		if (isdigit(c))
		{
			version.insert(i + 1, ".");
		}
	}
	version.insert(0, "Version: ");
	return version;
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

string hex2str(u32 in)
{
	stringstream out;
	out << std::hex;
	out << std::uppercase;
	out << in;
	return out.str();
}

bool issaltysdtitle(u64 optionaltitleid)
{
	u64 titleop = optionaltitleid != 0 ? optionaltitleid : currenttitleid;
	return titleop == 0x00040000000EDF00 || titleop ==  0x00040000000EE000 \
	|| titleop == 0x00040000000B8B00;
}

string saltysdtidtoregion(u64 optionaltitleid)
{
	u64 titleop = optionaltitleid != 0 ? optionaltitleid : currenttitleid;
	switch (titleop)
	{
	case 0x00040000000EDF00: return "USA";
	case 0x00040000000EE000: return "EUR";
	case 0x00040000000B8B00: return "JPN";
	}
	return "???";
}

int movemodsin()
{
	string source = issaltysdtitle() ? "/saltysd/smash" : "/luma/titles/" + currenttitleidstr + '/';
	string dest = modsfolder + currenttitleidstr + "/Slot_" + to_string(currentslot);
	if (rename(source.c_str(), dest.c_str()))
	{
		return 1;
	}
	return 0;
}

int startup()
{
	initmodmoontevkeys();
	sdraw::MM::initmodmoonshaders();
	sdraw::MM::shader_basic->bind();

	//Draw a blank frame to allow error calls to retrieve a valid framebuffer
	sdraw::framestart();
	sdraw::setfs("constColor", 0, RGBA8(0, 0, 0, 255));
	sdraw::drawrectangle(0, 0, 400, 240);
	sdraw::drawon(GFX_BOTTOM);
	sdraw::drawrectangle(0, 0, 320, 240);
	sdraw::frameend();
	//Configure dark mode
	sdraw::darkmodeshouldactivate = config.read("DarkModeEnabled", false);
	//Rename mods
	int renamefailed = 0;
	if(modsenabled)
	{
		renamefailed = movemodsin();
	}
	if(issaltysdtitle())
		movecustomsaltysdout();
	mainmenuupdateslotname();
	cfguInit(); //For system language
	amInit(); //For getting all the installed titles + updating
	httpcInit(0); //Downloading
	initializeallSMDHdata(titleids);
	//Do this in the main thread, because it may throw error calls
	updatecartridgedata(); 
	if (config.read("EnableFlexibleCartridgeSystem", false))
	{
		srv::init();
		srv::hook(0x208, cartridgesrvhook); //Notif 0x208: Game cartridge inserted
		srv::hook(0x20A, cartridgesrvhook); //Notif 0x20A: Game cartridge removed
	}
	return renamefailed;
}

void enablemods(bool isenabled)
{
	//Disables SaltySD for Smash titles, changes state for both.
	modsenabled = isenabled;
	config.write("ModsEnabled", modsenabled);
	//This assumption is invalidated by LayeredFS because we move the folder
	//which makes this folder not exist.
	if (issaltysdtitle())
	{
		string src = "/luma/titles/" + currenttitleidstr;
		string dest = "/luma/titles/" + currenttitleidstr;
		//If we're enabling, the source is the "Disabled" folder
		if (isenabled) src.insert(13, "Disabled");
		else dest.insert(13, "Disabled");
		if (rename(src.c_str(), dest.c_str()))
		{
			string enable = isenabled ? "enable" : "disable";
			error("Failed to " + enable + " mods!\nError: " + hex2str(errno));
			error("This may resolve itself\nthrough normal usage.");
		}
	}
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

bool buttonpressed(int bx, int by, int bwidth, int bheight, touchPosition lastpos, u32 kHeld)
{
	return touched(bx, by, bwidth, bheight, lastpos) && !(kHeld & KEY_TOUCH);
}

void mainmenushiftin()
{
	for (float i = 0; i <= 1.0; i += 0.08)
	{
		sdraw::framestart();
		drawtopscreen();
		sdraw::drawon(GFX_BOTTOM);
		sdraw::MM::shader_twocoords->bind();
		sdraw::drawtexture(backgroundbot, 0, 0);
		sdraw::drawtexture(leftbuttonmoon, -leftbuttonmoon.width, 13, 0, 13, i);
		sdraw::drawtexture(leftbutton, -leftbutton.width, 13, 0, 13, i);
		sdraw::drawtexture(rightbutton, 320, 13, 169, 13, i);
		sdraw::drawtexture(selector, 0, 240, 0, 159, i);
		sdraw::frameend();
	}
}

const unsigned int codes[] = {
	KEY_UP, KEY_UP, KEY_DOWN, KEY_DOWN, KEY_LEFT, KEY_RIGHT, KEY_LEFT, KEY_RIGHT, KEY_B, KEY_A, KEY_START
};

void secretcodedraw()
{
	sdraw::MM::shader_basic->bind();
	sdraw::setfs("constColor", 0, RGBA8(0, 0, 0, 255));
	sdraw::drawrectangle(0, 0, 400, 240);
	//This secret is hidden from view in the spritesheet. It has only an alpha of 1 and no color.
	//This is to prevent it from easily being seen in the source code. ;)
	//Also because I'm a sucker for playing with graphics (as evidenced by my own rendering engine)
	//and when this idea popped up I had to try it.
	//If you're reading this, you won't understand what it is until you try it for yourself, seeing as you
	//don't know what has an alpha in this subtexture.
	//This code brings it back up to full alpha and gives it a color.
	C3D_TexEnv* tev = C3D_GetTexEnv(0);
	C3D_TexEnvSrc(tev, C3D_RGB, GPU_CONSTANT);
	C3D_TexEnvSrc(tev, C3D_Alpha, GPU_TEXTURE0, GPU_CONSTANT);
	C3D_TexEnvOpRgb(tev, GPU_TEVOP_RGB_SRC_COLOR);
	C3D_TexEnvOpAlpha(tev, GPU_TEVOP_A_SRC_ALPHA, GPU_TEVOP_A_SRC_ALPHA);
	C3D_TexEnvFunc(tev, C3D_RGB, GPU_REPLACE);
	C3D_TexEnvFunc(tev, C3D_Alpha, GPU_ADD);
	C3D_TexEnvColor(tev, RGBA8(255, 255, 0, 254));

	C3D_AlphaTest(true, GPU_EQUAL, 255); //1 + 254 = 255, ignore everything else
	sdraw::drawquad(secret, 0, 0);
	C3D_AlphaTest(true, GPU_GREATER, 0); //sdraw's default behavior
}

//Returns if the main loop should be skipped or not due to an undesirable button press
bool secretcodeadvance(u32 kDown)
{
	static int state = 0;
	static int timeout = 0;
	if (kDown & codes[state])
	{
		state++;
		timeout = 0;
		//A and B button's position, plus one, due to earlier increment
		if(state == 9 || state == 10) return true;
		if (state == 11)
		{
			state = 0;
			//An error call gets this thing to work properly? Something something massaging the GPU
			//right? IDK. Not worth investigating, especially when I can just put in another meme :)
			error("Congrats! You have gained\n30 extra lives!");
			sdraw::framestart();
			sdraw::drawon(GFX_TOP);
			secretcodedraw();
			sdraw::drawon(GFX_BOTTOM);
			secretcodedraw();
			sdraw::frameend();
			for (;;) {} //Freeze! EVERYBODY CLAP YOUR HANDS
		}
	}
	else
	{
		timeout++;
		if (timeout >= 45)
		{
			timeout = 0;
			state = 0;
		}
	}
	return false;
}

//This code was brought into existence at the very beginning of this project,
//before sDraw had any notion of interpolation.
//It should have been removed ages ago, but it actually proves very useful to have this kind
//of basic use case left around for debugging by comparing it to the modernized mainmenushiftin().
void mainmenushiftout()
{
	//Opposite of shifting in (just some numbers changed)
	for(int l = 0, r = 169, b = 159; l > -((signed int)leftbutton.width); l -= 10, r += 10)
	{
		sdraw::framestart();
		drawtopscreen();
		sdraw::MM::shader_basic->bind();
		sdraw::drawon(GFX_BOTTOM);
		sdraw::drawtexture(backgroundbot, 0, 0);
		sdraw::drawtexture(leftbuttonmoon, l, 13);
		sdraw::drawtexture(leftbutton, l, 13);
		sdraw::drawtexture(rightbutton, r, 13);
		sdraw::drawtexture(selector, 0, b);
		sdraw::drawtextinrec(slotname.c_str(), 35, 21 + b, 251, 1.4, 1.4);
		sdraw::frameend();
		if(b < 240) b += 6;
	}
}

void mainmenuupdateslotname()
{
	if(maxslot == 0) {slotname = "None"; return;}
	if(currentslot == 0) {slotname = "Disabled"; return;}
	ifstream in(modsfolder + currenttitleidstr + '/' + "Slot_" + to_string(currentslot) + "/desc.txt");
	if(!in) {slotname = "Slot " + to_string(currentslot); return;}
	getline(in, slotname);
	in.close();
}

void updateslots(bool plus)
{
	if(maxslot == 0) return; //No mods
	int oldslot = currentslot;
	currentslot += (plus ? 1 : -1);
	if(currentslot == -1) currentslot = maxslot; //
	if(currentslot > maxslot) currentslot = 0;
	if(currentslot == 0) enablemods(false);
	else if(oldslot == 0 && currentslot) enablemods(true); //It was moved from disabled
	
	//If there's a cartridge inserted, and it has the same title as something on the SD card, we need to synchronize slots.
	if (currenttidpos != 0)
	{
		if(titleids[0] == currenttitleid)
			slots[0] = currentslot;
	}
	//Redundant check, just for code readability
	else if (currenttidpos == 0)
	{
		vector<u64>::iterator tiditer = std::find(titleids.begin() + 1, titleids.end(), currenttitleid);
		if (tiditer != titleids.end())
		{
			vector<int>::iterator slotiter = slots.begin();
			//They're in the same raw index, get the iterator to match the other one
			std::advance(slotiter, tiditer - titleids.begin());
			*slotiter = currentslot;
		}
	}

	mainmenuupdateslotname();
}

//All the things happen on the bottom screen, very rarely do we deviate from this pattern on the top screen
void drawtopscreen()
{
	sdraw::MM::shader_basic->bind();
	sdraw::setfs("texture");
	sdraw::drawtexture(backgroundtop, 0, 0);
	int bannerx = 400 / 2 - banner.width / 2;
	float bannery = 240 / 2 - banner.height / 2;
	//From new-hbmenu
	minusy -= 0.0052; // 1/192, the compiler didn't like assigning a division value for some reason.
	float addy = 6.0f*sinf(C3D_Angle(minusy));
	bannery += addy;
	sdraw::drawtexture(banner, bannerx, bannery);

	//Animate the moon colors...
	const int moonx = 191, moony = 13; //Position of the moon in the banner
	//Same idea as the progress bar texture- increase texcoords constantly to animate the motion
	//This time, however, we're going to base it on an alpha texture of the moon, so only the
	//moon is actually animated with these colors.
	static float animationplus = 0;


	sdraw::setfs("topScreenMoon", 0);
	sdraw::MM::shader_threetextures->bind();
	
	sdraw_stex temp(rainbow, 0, 0 - animationplus, 256, 256, false);
	sdraw::drawmultipletextures(bannerx + moonx, bannery + moony, bannermoonalpha, temp, temp);

	//At some point, the PICA200 just doesn't like texcoords of an extremely high value and stops repeating.
	//A quick debugging session yielded that it got to 30000 without issue, and that's something like 10
	//minutes of running, so it's fine to reset it at that point. The value I use here is 256 * 117
	//so it also shouldn't(?) cause a jarring skip.
	if(animationplus >= 29952)
		animationplus = 0;
	animationplus += .75;

	//Draw the title selection text
	sdraw::MM::shader_basic->bind();
	sdraw::setfs("textColor", 0, RGBA8(165, 165, 165, 255));
	sdraw::drawtext(": Help", 5, 240 - 40, 0.55, 0.55);
	sdraw::drawtext(": Title selection", 5, 240 - 20, 0.55, 0.55);

	sdraw::setfs("texture");
	//Draw the current title
	if (getSMDHdata()[currenttidpos].titl != 0) //This may be a cartridge that's not inserted, if it is, don't draw it
	{
		sdraw::drawSMDHicon(getSMDHdata()[currenttidpos].icon, 400 - 48 - 7, 240 - 48 - 7);
	}
	sdraw::drawtexture(titleselectionsinglebox, 400 - 58 - 2, 240 - 58 - 2);
}

void mainmenudraw(unsigned int dpadpos, touchPosition tpos, unsigned int alphapos, bool highlighterblink)
{
	sdraw::setfs("texture");

	sdraw::MM::shader_basic->bind();

	sdraw::drawtexture(backgroundbot, 0, 0);

	//Draw the rainbow (or not) moon first.
	//A bit like the banner animation but not quite.
	//The texcoords for the rainbow are much smaller vertically,
	//meaning this focuses on one color at a time.
	//It also interpolates with the moon's colors, leading to a great
	//glow effect on the rainbow which was stolen from it, while ALSO
	//using that same moon as an alpha map. Pretty nice!
	static float animationplus = 0;
	//It's a bit jarring to enable mods and immediately
	//have the rainbow pop up. This code smoothes it out a bit.
	static int rainbowinterp = 0;
	if (modsenabled && rainbowinterp < 128)
	{
		rainbowinterp += 3;
		if(rainbowinterp > 128) rainbowinterp = 128;
	}
	if (!modsenabled && rainbowinterp > 0)
	{
		rainbowinterp -= 3;
		if(rainbowinterp < 0) rainbowinterp = 0;
	}
	sdraw::setfs("launchButtonMoon", 0, RGBA8(0, 0, 0, rainbowinterp));

	sdraw_stex temp(rainbow, 0, 0 - animationplus, 256, 40, false);
	sdraw::MM::shader_threetextures->bind();
	sdraw::drawmultipletextures(0, 13, leftbuttonmoon, temp, temp);
		
	//See above for why this is done
	if (animationplus >= 29952)
		animationplus = 0;
	animationplus += 0.5;

	sdraw::setfs("texture");
	sdraw::MM::shader_twocoords->bind();

	if (dpadpos == 0)
		drawtexturewithhighlight(leftbutton, 0, 13, \
			RGBA8(mainmenuhighlightcolors[0], mainmenuhighlightcolors[1], mainmenuhighlightcolors[2], 0), alphapos);
	else sdraw::drawtexture(leftbutton, 0, 13);

	if (dpadpos == 1)
		drawtexturewithhighlight(rightbutton, 169, 13, \
			RGBA8(mainmenuhighlightcolors[0], mainmenuhighlightcolors[1], mainmenuhighlightcolors[2], 0), alphapos);
	else sdraw::drawtexture(rightbutton, 169, 13);
	if (dpadpos == 2 || dpadpos == 3)
		drawtexturewithhighlight(selector, 0, 159, \
			RGBA8(mainmenuhighlightcolors[0], mainmenuhighlightcolors[1], mainmenuhighlightcolors[2], 0), alphapos);
	else sdraw::drawtexture(selector, 0, 159);

	sdraw::MM::shader_basic->bind();
	sdraw::setfs("textColor", 0, RGBA8(0, 0, 0, 255));
	sdraw::drawtextinrec(slotname.c_str(), 35, 180, 251, 1.4, 1.4);


	//sdraw::drawtext(to_string(sdraw::MM::shader_basic->getArrayPos()).c_str(), 0, 0, 1, 1);

	//sdraw::drawtext(to_string(sdraw::MM::shader_twocoords->getArrayPos()).c_str(), 0, 50, 1, 1);
}


int main(int argc, char **argv) {

	int renamefailed = startup();
	touchPosition tpos;
	touchPosition opos;
	int alphapos = 0;
	bool alphaplus = true;
	unsigned int dpadpos = 0;
	if (!config.read("InitialSetupDone", false))
	{
		initialsetup();
		config.write("InitialSetupDone", true);
	}

	if(!shoulddisableupdater)
		updatecheckworker.startworker();

	mainmenushiftin();
	//So, uh, sdraw doesn't like it when I trigger an error before shifting in the menu, and freezes the GPU...
	//I don't like this at all, it fragments the code and forces me to do bad things
	//UPDATE: Managed to fix that crash. But, I'm too lazy to put this code back where it belongs. It's fine.
	if(renamefailed)
	{
		renamefailederror:
		string source = issaltysdtitle() ? "/saltysd/smash" : "/luma/titles/" + currenttitleidstr + '/';
		string dest = modsfolder + currenttitleidstr + "/Slot_" + to_string(currentslot); //Just copy/pasted
		//Dest is almost always too large, so we'll split it up.
		string displaydest = dest;
		displaydest.insert(dest.size() / 2, "\n");
		error("Failed to move slot file from\n" + source + "\nto " + displaydest + '!');
		error("Error code:\n" + hex2str(errno));
		if ((unsigned int)errno == 0xC82044BE) //Destination already exists
		{
			if (countEntriesInDir(dest.c_str()) == 0)
			{
				rmdir(dest.c_str());
				error("ModMoon tackled this issue\nautomagically. Now isn't that\nnice? Retrying now...");
				if(movemodsin()) goto renamefailederror;
			}
		}
		else if(errno == 2) //Maybe they shut off the system, preventing us from moving to dest?
		{
			error("This error probably occurred\nbecause you shut off the system\nwhile using ModMoon.");
			error("It will likely resolve itself\nthrough normal usage.");
		}
	}
	if (maxslot == 0 && titleids[currenttidpos] != 0) //No mods and it's not an empty cartridge
	{
		error("Warning: Failed to find mods for\nthis game!");
		error("Place them at " + modsfolder + '\n' + currenttitleidstr + "/Slot_X\nWhere X is a number starting at 1.");
	}
	while (aptMainLoop()) {
		if(cartridgeneedsupdating)
			updatecartridgedata();
		if (isupdateavailable() && !shoulddisableupdater)
		{
			error("An update is available.\nIt will be installed now.\n(Press Start now to skip.)");
			if (!errorwasstartpressed())
			{
				updateinstallworker.startworker();
				updateinstallworker.displayprogress();
				error("Update complete. The system\nwill now reboot.");
				nsRebootSystemClean();
			}
			else
				shoulddisableupdater = true; //Disable it for the rest of this run
		}
		if (issaltysdupdateavailable() && !shoulddisableupdater)
		{
			error("A SaltySD update is available.\nIt will be downloaded now.\n(Press Start now to skip.)");
			if (!errorwasstartpressed())
			{
				saltysdupdaterworker.startworker();
				saltysdupdaterworker.displayprogress();
				error("SaltySD update complete.");
			}
			else
				shoulddisableupdater = true; //Disable it for the rest of this run
		}

		hidScanInput();
		opos = tpos;
		hidTouchRead(&tpos);
		u32 kDown = hidKeysDown();
		u32 kHeld = hidKeysHeld();
		if (secretcodeadvance(kDown)) continue;
		//So 0 is the launch button, 1 is the tools button,
		//2 is the selector bar that, when pressed up, goes back to the launch button, and 3 is the same for the tools button.
		if(kDown)
			alphapos = 255;
		if(kDown & KEY_A)
		{
			switch(dpadpos)
			{
				case 0: launch(); break;
				case 1: mainmenushiftout(); toolsmenu(); mainmenushiftin(); break;
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
		if (kDown & KEY_X)
		{
			string helptext;
			switch (dpadpos)
			{
			case 0:
				helptext = "Launch:\nApplies and launches mods\nfor the selected game."; break;
			case 1:
				helptext = "Tools Menu:\nAccess some of the goodies\nModMoon has to offer!"; break;
			case 2: //Fall through
			case 3:
				helptext = "Mod Selector:\nChange the active mod for the\ncurrent game. Tap the arrows or\nuse  left/right to change mods."; break;
			}
			error(helptext);
		}
		if(kDown & KEY_START) break;
		if (touched(leftbutton, 0, 13, tpos))
			dpadpos = 0;
		else if (buttonpressed(leftbutton, 0, 13, opos, kHeld))
			launch();

		if (touched(0, 174, 52, 66, tpos)) //Left button coordinates
			dpadpos = (dpadpos == 0) ? 2 : (dpadpos == 1) ? 3 : 3;
		else if(buttonpressed(0, 174, 52, 66, opos, kHeld))
			updateslots(false);

		if (touched(268, 174, 52, 66, tpos)) //Right button coordinates
			dpadpos = (dpadpos == 0) ? 2 : (dpadpos == 1) ? 3 : 3;
		else if(buttonpressed(268, 174, 52, 66, opos, kHeld))
			updateslots(true);

		if(touched(selector, 0, 159, tpos)) //No action, but do highlight it
			dpadpos = (dpadpos == 0) ? 2 : (dpadpos == 1) ? 3 : 3;

		if (touched(rightbutton, 169, 13, tpos))
			dpadpos = 1;
		else if (buttonpressed(rightbutton, 169, 13, opos, kHeld))
		{
			mainmenushiftout();
			toolsmenu();
			mainmenushiftin();
		}

		sdraw::framestart();
		drawtopscreen();
		sdraw::drawon(GFX_BOTTOM);
		highlighterhandle(alphapos, alphaplus);
		mainmenudraw(dpadpos, tpos, alphapos, false);
		sdraw::frameend();
	}

	/*int rgb[3] = {255, 255, 255}; //Fade to white 
	s32 mainthreadpriority;
	svcGetThreadPriority(&mainthreadpriority, CUR_THREAD_HANDLE);
	svcCreateEvent(&event_fadefinished, RESET_ONESHOT);
	threadCreate(threadfunc_fade, rgb, 8000, mainthreadpriority + 1, -2, true);*/
	mainmenushiftout();
	
	if (modsenabled)
	{
		if(issaltysdtitle())
			movecustomsaltysdin();
		string dest = issaltysdtitle() ? "/saltysd/smash" : "/luma/titles/" + currenttitleidstr;
		string src = modsfolder + currenttitleidstr + "/Slot_" + to_string(currentslot);
		attemptrename:
		if (rename(src.c_str(), dest.c_str()))
		{
			error("Failed to move slot file from\n" + modsfolder + '\n' + currenttitleidstr + "/Slot_" + to_string(currentslot) + "\nto " + dest + '!');
			error("Error code:\n" + hex2str(errno));
			if ((unsigned int)errno == 0xC82044BE) //Destination already exists
			{
				if (countEntriesInDir(dest.c_str()) == 0)
				{
					rmdir(dest.c_str());
					error("ModMoon tackled this issue\nautomagically. Now isn't that\nnice? Retrying now...");
					goto attemptrename;
				}
			}
		}
	}
	config.u64multiwrite("ActiveTitleIDs", titleids, true);
	config.intmultiwrite("TitleIDSlots", slots);
	config.write("SelectedTitleIDPos", currenttidpos);
	config.flush();

	updatecheckworker.shutdown();
	SMDHworker.shutdown();
	
	//svcWaitSynchronization(event_fadefinished, U64_MAX);
	//C3D_TexDelete(spritesheet);
	//C3D_TexDelete(progressfiller);
	//freeSMDHdata();
	sdraw::cleanup();
	sdraw::MM::destroymodmoonshaders();
	srv::exit();
	romfsExit();
	gfxExit();
	cfguExit();
	amExit();
	return 0;
}

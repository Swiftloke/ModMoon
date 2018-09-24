#include <string.h>
#include <3ds.h>
#include <stdio.h>
#include <unistd.h>
#include <sstream>
#include <sys/stat.h>
#include <errno.h>
#include <dirent.h>
#include <fstream>

#include "utils.hpp"
#include "main.hpp"
#include "error.hpp"
#include "srv.hpp"
#include "download.hpp"

using namespace std;

Handle event_fadefinished;

Result nsRebootSystemClean()
{
	static Handle nsHandle;
	srvGetServiceHandle(&nsHandle, "ns:s");
	u32 *cmdbuf = getThreadCommandBuffer();
	cmdbuf[0] = IPC_MakeHeader(0x16, 0, 0); // 0x00160000
	Result ret;
	if (R_FAILED(ret = svcSendSyncRequest(nsHandle)))return ret;
	return (Result)cmdbuf[1];
}

//The code to deal with a missing slot due to it being in the destination is outdated,
//but I've left it in in case it comes in handy later; there's no real penalty to keeping
//it anyway
int maxslotcheck(u64 optionaltid, int optionalslot)
{
	int currentFolderCount = 0;
	stringstream path2Check;
	stringstream NewPath2Check;
	string tid2check = optionaltid != 1 ? tid2str(optionaltid) : currenttitleidstr;
	int slottoskip = (optionalslot != -1) ? optionalslot : currentslot;
	do {
		currentFolderCount++;
		path2Check.str("");
		path2Check << modsfolder + tid2check << '/' << "Slot_" << currentFolderCount << '/';
	} while (pathExist(path2Check.str()) || currentFolderCount == slottoskip);
	//the minus 1 is due to it returning the folder number that doesnt exist.
	return currentFolderCount - 1;
}

void highlighterhandle(int& alphapos, bool& alphaplus)
{
#define PLUSVALUE 5
	if (alphaplus)
	{
		alphapos += PLUSVALUE;
		if (alphapos > 255) { alphapos -= PLUSVALUE; alphaplus = false; }
	}
	else
	{
		alphapos -= PLUSVALUE;
		if (alphapos < 0) { alphapos += PLUSVALUE; alphaplus = true; }
	}
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
		drawtopscreen();
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

//https://www.linuxquestions.org/questions/linux-newbie-8/how-to-check-if-a-folder-is-empty-661934/
int countEntriesInDir(const char* dirname)
{
	int n = 0;
	dirent* d;
	DIR* dir = opendir(dirname);
	if (dir == NULL) return 0;
	while ((d = readdir(dir)) != NULL) n++;
	closedir(dir);
	return n;
}


//Straight out of Smash Selector.

void fcopy(string input, string output)
{
	ifstream in(input, ios::binary);
	ofstream out(output, ios::binary);
	if(!in) { error("Failed to open input!\n" + input); return;}
	if(!out) { error("Failed to open output!\n" + output); return; }
	out << in.rdbuf();
	in.close();
	out.close();
}

void writeSaltySD(u64 titleid)
{
	string regionmodifier;
	switch (titleid)
	{
		case 0x00040000000EDF00: {regionmodifier = "USA"; break; }
		case 0x00040000000EE000: {regionmodifier = "EUR"; break; }
		case 0x00040000000B8B00: {regionmodifier = "JAP"; break; }
	}
	string outputpath = "/luma/titles/" + tid2str(titleid);
	if(!pathExist(outputpath.c_str()))
		_mkdir(outputpath.c_str());
	outputpath.append("/code.ips");
	//In this implementation, we expect that mods are enabled.
	//This may change if hitbox display is implemented

	//if (!saltySDEnabled) { outputpath.insert(13, ".Disabled"); }
	/*if (!pathExist(outputpath.c_str()))
	{
		string attempt2 = outputpath;
		attempt2.insert(13, "Disabled");
		if(pathExist(attempt2.c_str()))
			outputpath = attempt2;
		else
			
	}*/
	string inputpath = "romfs:/code.ips";
	inputpath.insert(7, regionmodifier);
	//if (hitboxdisplay) { inputpath.insert(7, "Hitbox"); }
	//else { inputpath.insert(7, "Normal"); }
	if (pathExist(outputpath)) { remove(outputpath.c_str()); }
	fcopy(inputpath, outputpath);
	error("New SaltySD file written.\nRegion Type: " + regionmodifier);
}

unsigned int MurmurHash2(const void * key, int len, unsigned int seed)
{
	const unsigned int m = 0x5bd1e995;
	const int r = 24;
	unsigned int h = seed ^ len;
	const unsigned char * data = (const unsigned char *)key;
	while (len >= 4)
	{
		unsigned int k = *(unsigned int *)data;
		k *= m;
		k ^= k >> r;
		k *= m;
		h *= m;
		h ^= k;
		data += 4;
		len -= 4;
	}
	switch (len)
	{
	case 3: h ^= data[2] << 16;
	case 2: h ^= data[1] << 8;
	case 1: h ^= data[0];
		h *= m;
	};
	h ^= h >> 13;
	h *= m;
	h ^= h >> 15;
	//delete[] data;
	//delete[] key;
	//delete[] len;
	return h;
}

//Thanks to Cydget for this code from Smash Selector!
unsigned int genHash(string filepath) {//give this function three arguments main mods folder, region, and settings folder
	unsigned int hashoffile = 0;;
	//cout << "\x1b[2;0H" << filepath;
	streampos size;
	char * memblock;
	ifstream codebinary(filepath, ios::in | std::ios::binary | ios::ate);
	if (codebinary.is_open()) {
		size = codebinary.tellg();
		memblock = new char[size];
		codebinary.seekg(0, ios::beg);
		codebinary.read(memblock, size);
		codebinary.close();
		hashoffile = MurmurHash2(memblock, (int)(size)-1, 0);
		//free(memblock);
		delete[] memblock;
		//cout << hashoffile <<endl;
		//cout << (int)hashoffile;
	}
	return hashoffile;
}

//As far as I can tell, there's no really good way to do this... The idea is to move around the original
//SaltySD file to make room for the custom one...

void movecustomsaltysdin()
{
	if (pathExist((modsfolder + currenttitleidstr + "/Slot_" + to_string(currentslot) + "/codes"))) //A custom code.ips needs to be moved.
	{
		//string regions[] = { "USA", "EUR", "JAP" };
		string originalmoveout = "/luma/titles/" + currenttitleidstr + "/code.ips";
		string originalmovein = "/3ds/ModMoon/" + saltysdtidtoregion() + "code.ips";
		if (rename(originalmoveout.c_str(), originalmovein.c_str()))
		{
			error("Custom SaltySD code.ips move\nfailed! (original move)");
			error("Error code: " + hex2str(errno));
		}
		string custommoveout = modsfolder + currenttitleidstr + "/Slot_" + to_string(currentslot) + "/codes/" + saltysdtidtoregion() + "code.ips";
		string custommovein = "/luma/titles/" + currenttitleidstr + "/code.ips";
		if (rename(custommoveout.c_str(), custommovein.c_str()))
		{
			error("Custom SaltySD code.ips move\nfailed! (custom move)");
			error("Error code: " + hex2str(errno));
		}
	}
}

void movecustomsaltysdout()
{
	if (pathExist((modsfolder + currenttitleidstr + "/Slot_" + to_string(currentslot) + "/codes"))) //A custom code.ips needs to be moved.
	{
		//string regions[] = { "USA", "EUR", "JAP" };
		string custommoveout = "/luma/titles/" + currenttitleidstr + "/code.ips";
		string custommovein = modsfolder + currenttitleidstr + "/Slot_" + to_string(currentslot) + "/codes/" + saltysdtidtoregion() + "code.ips";
		if (rename(custommoveout.c_str(), custommovein.c_str()))
		{
			error("Custom SaltySD code.ips move\nfailed! (custom move)");
			error("Error code: " + hex2str(errno));
		}

		string originalmoveout = "/3ds/ModMoon/" + saltysdtidtoregion() + "code.ips";
		string originalmovein = "/luma/titles/" + currenttitleidstr + "/code.ips";
		if (rename(originalmoveout.c_str(), originalmovein.c_str()))
		{
			error("Custom SaltySD code.ips move\nfailed! (original move)");
			error("Error code: " + hex2str(errno));
		}
	}
}

void launch(){

	int rgb[3] = {0, 0, 0}; //Fade to black
	
	s32 mainthreadpriority;
	svcGetThreadPriority(&mainthreadpriority, CUR_THREAD_HANDLE);
	svcCreateEvent(&event_fadefinished, RESET_ONESHOT);
	threadCreate(threadfunc_fade, rgb, 8000, mainthreadpriority + 1, -2, true);
	config.u64multiwrite("ActiveTitleIDs", titleids, true);
	config.intmultiwrite("TitleIDSlots", slots);
	config.write("SelectedTitleIDPos", currenttidpos);
	config.flush();
	if(modsenabled)
	{
		if(issaltysdtitle())
			movecustomsaltysdin();
		string dest = issaltysdtitle() ? "/saltysd/smash" : "/luma/titles/" + currenttitleidstr + '/';
		string src = modsfolder + currenttitleidstr + "/Slot_" + to_string(currentslot);
		attemptrename:
		if(rename(src.c_str(), dest.c_str()))
		{
			error("Failed to move slot file from\n" + modsfolder + '\n' + currenttitleidstr + "/Slot_" + to_string(currentslot) + " to\n" + dest + '!');
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
	svcWaitSynchronization(event_fadefinished, U64_MAX);
	draw.framestart(); //Prevent wonkiness with the app jump
	draw.drawrectangle(0, 0, 400, 240, 0); //Not actually drawn, just there to prevent Citro3D hang. Clear color does this for real
	draw.drawon(GFX_BOTTOM);
	draw.drawrectangle(0, 0, 320, 240, 0);
	draw.frameend();
	draw.cleanup();
	srv::exit();

	updatecheckworker.shutdown();
	SMDHworker.shutdown();

	u8 param[0x300];
	u8 hmac[0x20];
	memset(param, 0, sizeof(param));
	memset(hmac, 0, sizeof(hmac));

	APT_PrepareToDoApplicationJump(0, currenttitleid, getSMDHdata()[currenttidpos].gametype);
	APT_DoApplicationJump(param, sizeof(param), hmac);
}

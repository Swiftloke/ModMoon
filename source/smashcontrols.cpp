//This code is mostly from Smash Selector 2.4, with some slight modifications to use sDraw instead of stdout.
#include <3ds.h>
#include <sstream>
#include "main.hpp"
#include "error.hpp"

using namespace std;

extern int region;
extern int gameType;

string actionnames[] =
{
	"Attack", "Special", "Jump", "Shield", "Grab", "Smash Attack", "Up Taunt", "Side Taunt", "Down Taunt"
};
/*int buttonoffsets[] =
{
	0xE0E2, 0xE0E3, 0xE0E4, 0xE0E5, 0xE0E6, 0xE0E7, 0xE0E8, 0xE0E9, 0xE0EA, 0xE0EB, 0xE0EC, 0xE0ED
};*/
string buttonnames[] =
{
	//"L", "R", "ZL", "ZR", "D-Pad Up", "D-Pad Side", "D-Pad Down", "A", "B", "C-Stick", "X", "Y"
	//System font characters
	"", "", "ZL", "ZR", " Up", " Side", " Down", "", "", "C-Stick", "", "" 
};

int buttons[14];
int cursor = 0;
bool isn3ds;

void controlsdraw()
{
	//Smash Selector 2.4 used cout. I'd rather not change all of this code, so a stringstream works fine.
	stringstream strout;
	for (int i = 0; i <= 11; i++)
	{
		//Ensure we don't accidentally print buttons that aren't on an o3DS
		if (!isn3ds && (i == 2 || i == 3 || i == 9)) { continue; }
		string cursorprint = (cursor == i) ? "(x)" : "( )";
		strout << cursorprint << " " << buttonnames[i] << ": " << actionnames[buttons[i]] << '\n';
	}
	string cursorprint = (cursor == 12) ? "(x)" : "( )"; //Meh, no need for a loop
	string enabledordisabled = (buttons[12] == 1) ? "Enabled" : "Disabled";
	strout << cursorprint << " Tap Jump: <" << enabledordisabled << ">" << '\n';
	cursorprint = (cursor == 13) ? "(x)" : "( )";
	enabledordisabled = (buttons[13] == 1) ? "Enabled" : "Disabled";
	strout << cursorprint << " A+B Smash Attack: <" << enabledordisabled << ">" << '\n';
	sdraw::framestart();
	drawtopscreen();
	sdraw::drawon(GFX_BOTTOM);
	sdraw::drawtexture(backgroundbot, 0, 0);
	sdraw::setfs("textColor", 0, RGBA8(0, 0, 0, 255));
	sdraw::drawtext(strout.str().c_str(), 0, 0, 0.6, 0.55);
	sdraw::frameend();
}

void controlsmodifier() {
	smdhdata data = getSMDHdata()[currenttidpos];
	if (!issaltysdtitle(data.titl))
	{
		error("The currently selected title\nis not Smash. Please select\nSmash and try again.");
		return;
	}
	FS_Archive archive;
	u32 path[3] = { data.gametype, (u32)(data.titl & 0xFFFFFFFF), 0x00040000 };

	FS_Path archivepath = { PATH_BINARY, 12, path };
	FS_Path filepath = fsMakePath(PATH_ASCII, "/save_data/system_data.bin");
	Handle file;
	Result res;
	res = FSUSER_OpenArchive(&archive, ARCHIVE_USER_SAVEDATA, archivepath);
	if (R_FAILED(res))
	{
		error("FSUSER_OpenArchive failed!\nerror code:" + tid2str(res));
		FSUSER_CloseArchive(archive);
		return;
	}
	res = FSUSER_OpenFile(&file, archive, filepath, FS_OPEN_READ | FS_OPEN_WRITE, 0);
	if (R_FAILED(res))
	{
		error("FSUSER_ControlFile failed!\nerror code:" + tid2str(res));
		FSFILE_Close(file);
		FSUSER_CloseArchive(archive);
		return;
	}
	for (int i = 0x0; i <= 0xD; i++)
	{
		int buf = 4;
		u32 out;
		res = FSFILE_Read(file, &out, 0xE0E2 + i, &buf, 1);
		if (R_FAILED(res))
		{
			error("FSFILE_Read failed!\nerror code:" + tid2str(res));
			FSFILE_Close(file);
			FSUSER_CloseArchive(archive);
			return;
		}
		switch (buf) //Special cases for taunts
		{
		case 0xA: {buf = 6; break; }
		case 0xB: {buf = 7; break; }
		case 0xC: {buf = 8; break; }
		default: break;
		}
		buttons[i] = buf;
	}
	APT_CheckNew3DS(&isn3ds);
	while (aptMainLoop()) {
		hidScanInput();
		u32 kDown = hidKeysDown();
		if (kDown & KEY_B) break;
		if (kDown & KEY_UP)
		{
			cursor--;
			if (!isn3ds && cursor == 9) cursor--;
			if (!isn3ds && cursor == 3) cursor -= 2; //Skip 2 more to go past ZL and ZR to R
			if (cursor == -1) cursor = 13;
		}
		if (kDown & KEY_DOWN)
		{
			cursor++;
			if (!isn3ds && cursor == 9) cursor++;
			if (!isn3ds && cursor == 2) cursor += 2; //Skip 2 more to go past ZL and ZR to D-Up
			if (cursor == 14) cursor = 0;
		}
		if (kDown & KEY_LEFT)
		{
			if (cursor == 12 || cursor == 13) //Special cases for Tap Jump and A + B Smash Attack
			{
				if (buttons[cursor] == 1) buttons[cursor] = 0;
				else buttons[cursor] = 1;
			}
			else
			{
				buttons[cursor]--;
				if (buttons[cursor] == -1) { buttons[cursor] = 8; }
			}
		}
		if (kDown & KEY_RIGHT)
		{
			if (cursor == 12 || cursor == 13) //Special cases for Tap Jump and A + B Smash Attack
			{
				if (buttons[cursor] == 1) buttons[cursor] = 0;
				else buttons[cursor] = 1;
			}
			else
			{
				buttons[cursor]++;
				if (buttons[cursor] == 9) { buttons[cursor] = 0; }
			}
		}
		if (kDown & KEY_X)
		{
			error("Smash Controls Modifier:\nAllows you to modify the in-\ngame controls much further\nthan the game itself allows.");
			error("Select a button by moving the\nCircle Pad up and down. Select a\ncontrol setting with left and right.\nPress  to save and exit.");
		}
		controlsdraw();
	}
	for (int i = 0x0; i <= 0xD; i++)
	{
		switch (buttons[i]) //Special cases for taunts
		{
		case 6: {buttons[i] = 0xA; break; }
		case 7: {buttons[i] = 0xB; break; }
		case 8: {buttons[i] = 0xC; break; }
		default: break;
		}
		res = FSFILE_Write(file, NULL, 0xE0E2 + i, &buttons[i], 1, 0);
		if (R_FAILED(res))
			error("FSFILE_Write failed!\nerror code:" + tid2str(res));
	}
	FSFILE_Close(file);
	res = FSUSER_ControlArchive(archive, ARCHIVE_ACTION_COMMIT_SAVE_DATA, 0, 0, 0, 0);
	if(R_FAILED(res))
		error("FSUSER_ControlArchive failed!\nerror code:" + tid2str(res));
	FSUSER_CloseArchive(archive);
}
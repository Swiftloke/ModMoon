#include <fstream>
#include "migrators.hpp"
#include "../main.hpp"
#include "../utils.hpp"
#include <sys/stat.h>
#include <algorithm>
#include "../error.hpp"
#include "../titleselects.hpp"

int foldercount = 0; //Immediately incremented to 1 in migration
int totalslots = 0; //Calculated later
bool isitdone = false;

std::tuple<int, int, bool> ss2xretriveinfo()
{
	return std::make_tuple(foldercount, totalslots, isitdone);
}

int ss2xmaxSlotCheck(string ss2xmodsfolder)
{
	int currentFolderCount = 0;
	stringstream path2Check;
	stringstream NewPath2Check;
	do {
		currentFolderCount++;
		path2Check.str("");
		path2Check << ss2xmodsfolder << "Slot_" << currentFolderCount << "/";
	} while (pathExist(path2Check.str()));
	//the minus 1 is due to it returning the folder number that doesnt exist.
	return currentFolderCount - 1;
}

void ss2xmovemods(string srcmodsfolder, u64 title, int premigratemaxslot)
{
	string src, dest;
	while (true)
	{
		foldercount++;
		src = srcmodsfolder + "Slot_" + to_string(foldercount);
		dest = modsfolder + tid2str(title) + "/Slot_" + to_string(foldercount + premigratemaxslot);
		if(!pathExist(src)) break;
		rename(src.c_str(), dest.c_str());
	}
}

void ss2xMigrate(void* null)
{
	Config ss2xconfig("/3ds/data/smash_selector/", "settings.txt");
	int gametype = ss2xconfig.read("GameType") == "Cia" ? 1 : 2;
	string region = ss2xconfig.read("GameRegion");
	u64 title = region == "Usa" ? 0x00040000000EDF00 : \
		region == "Eur" ? 0x00040000000EE000 : 0x00040000000B8B00;
	string ss2xmodsfolder = ss2xconfig.read("ModsFolder");
	int missingmod = ss2xconfig.read("SelectedModSlot", 0);
	queuetitleforactivationwithinmenu(title, gametype);
	if (!pathExist(modsfolder + tid2str(title)))
		_mkdir((modsfolder + tid2str(title)).c_str());
	//What's currently in /saltysd/smash? Keep the order.
	rename("/saltysd/smash", (ss2xmodsfolder + "Slot_" + to_string(missingmod)).c_str());
	int premigratemaxslot = maxslotcheck(title);
	totalslots = ss2xmaxSlotCheck(ss2xmodsfolder);
	ss2xmovemods(ss2xmodsfolder, title, premigratemaxslot);
	remove("/3ds/data/smash_selector/settings.txt");
	isitdone = true;
}
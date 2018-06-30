// /saltysd/card.txt contains the game type
// /saltysd/select.txt contains the current slot

#include <fstream>
#include "../main.hpp"
#include "../utils.hpp"
#include <sys/stat.h>
#include <algorithm>
#include "../error.hpp"
#include "../titleselects.hpp"

int modsChecked = 1;
bool isdone = false;

std::pair<int, bool> ss1xretrieveinfo()
{
	return std::make_pair(modsChecked, isdone);
}

//Thanks to Cydget for this code from Smash Selector 2.x!
int checkOldMods(int startFolderNum, string destfolder, u64 title)
{
	//initially call this function at 0
	//this is better than before, because it makes sure there are no blanks if they deleted some mod folders, and it also will check up to 10 gaps between the folders.
	int currentFolderCount = startFolderNum;
	int freeFolderNum = maxslotcheck(title) + 1;
	stringstream freeFolder;
	stringstream path2Check;
	freeFolder << destfolder << "Slot_" << freeFolderNum << "/";
	path2Check << "/saltysd/smash" << currentFolderCount << "/";
	//the one condition(currentFolderCount < 10) is unneccesary as the other will always cover that exception under theses values
	while (pathExist(path2Check.str()) || currentFolderCount < 10 || currentFolderCount < (startFolderNum + 10)) {
		if (pathExist(path2Check.str())) {
			rename(path2Check.str().c_str(), freeFolder.str().c_str());
			//checkOldMods(currentFolderCount);]
			currentFolderCount++;
			return currentFolderCount;
			break;
		}
		path2Check.str("");
		currentFolderCount++;
		path2Check << "/saltysd/smash" << currentFolderCount << "/";
	}
	return -1;
}

void ss1xMigrate(void* null)
{
	ifstream card("/saltysd/card.txt");
	//For as garbage as C++'s file IO is, it works well in this one edge case
	int gametype;
	card >> gametype;
	card.close();
	u64 title = 0;
	switch (gametype)
	{
		case 0: title = 0x00040000000EDF00; gametype = 1; break;
		case 1: title = 0x00040000000EDF00; gametype = 2; break;
		case 2: title = 0x00040000000EE000; gametype = 1; break;
		case 3: title = 0x00040000000EE000; gametype = 2; break;
		case 4: title = 0x00040000000B8B00; gametype = 1; break;
		case 5: title = 0x00040000000B8B00; gametype = 2; break;
	}
	queuetitleforactivationwithinmenu(title, gametype);
	if (!pathExist(modsfolder + tid2str(title)))
		_mkdir((modsfolder + tid2str(title)).c_str());

	//What's currently in /saltysd/smash? Let's keep the order
	ifstream missingmodin("/saltysd/select.txt");
	int missingmod;
	missingmodin >> missingmod;
	missingmodin.close();
	rename("/saltysd/smash", ("saltysd/smash" + to_string(missingmod)).c_str());
	//Copy slots
	while (modsChecked != -1)
	{
		modsChecked = checkOldMods(modsChecked, modsfolder + tid2str(title) + '/', title);
	}
	remove("/saltysd/select.txt");
	remove("/saltysd/card.txt");
	isdone = true;
}
// /saltysd/smash/card.txt contains the game type
// /saltysd/smash/select.txt contains the current slot

#include <fstream>
#include "../main.hpp"
#include "../utils.hpp"
#include <sys/stat.h>
#include <algorithm>

//Thanks to Cydget for this code from SS 2.x!
int checkOldMods(int startFolderNum, int maxslot, string destfolder)
{
	//initially call this funciton at 0
	//this is better than before, because it makes sure there are no blanks if they deleted some mod folders, and it also will check up to 10 gaps between the folders.
	int currentFolderCount = startFolderNum;
	int freeFolderNum = maxslot + 1;
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

void ss1xMigrate()
{
	ifstream card("/saltysd/smash/card.txt");
	//For as garbage as C++'s file IO is, it works well in this one edge case
	int gametype;
	card >> gametype;
	card.close();
	u64 title;
	int mediatype;
	switch (gametype)
	{
		case 0: title = 0x00040000000EDF00; gametype = 1; break;
		case 1: title = 0x00040000000EDF00; gametype = 2; break;
		case 2: title = 0x00040000000EE000; gametype = 1; break;
		case 3: title = 0x00040000000EE000; gametype = 2; break;
		case 4: title = 0x00040000000B8B00; gametype = 1; break;
		case 5: title = 0x00040000000B8B00; gametype = 2; break;
	}
	int maxslot;
	if (std::find(titleids.begin(), titleids.end(), title) == titleids.end()) //We don't already have it
	{
		smdhdata data;
		data.load(title, mediatype);
		getSMDHdata().push_back(data);
		titleids.push_back(title);
		if(!pathExist("/3ds/data/ModMoon/" + tid2str(title)))
			_mkdir(("/3ds/data/ModMoon/" + tid2str(title)).c_str());
		//There aren't any slots yet as this title is new in the database
		maxslot = 0;
	}
	else
	{
		maxslot = maxslotcheck(title);
	}
	//Copy slots
	int modsChecked = 0;
	while (modsChecked != -1)
	{
		modsChecked = checkOldMods(modsChecked, maxslot, "/3ds/data/ModMoon/" + tid2str(title));
	}

}
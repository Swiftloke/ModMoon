#include "main.hpp" //Also includes smdh.hpp
#include "utils.hpp"
#include "error.hpp"
#include <algorithm>

vector<smdhdata> smdhvector;
vector<smdhdata> alltitlesvector;
//u32 alltitlesloadedcount = 0;
u32 alltitlescount = 0;
u8 language = 0;
vector<u64> tidstoload;
//bool alltitlesloaded = false;

WorkerFunction SMDHworker(worker_loadallsmdhdata, \
	"Waiting for titles to be loaded...\n Title [progress] / [total]");

void worker_loadallsmdhdata(WorkerFunction* notthis)
{
	//smdhvector[0].load(0, 2); //MEDIATYPE_GAME_CARD; Always load the cartridge in the first position
	//The cartridge was loaded from the main thread, so we don't need to worry about it. This is because updatecartridgedata() may use the rendering queue through error calls
	notthis->functiontotal = alltitlescount;

	for (unsigned int i = 1; i < tidstoload.size(); i++)
	{
		smdhvector[i].load(tidstoload[i]);
		if(notthis->cancel)
			return;
	}

	//Now, load everything that's not already loaded to support selection of new titles
	u64* alltids = new u64[alltitlescount];
	AM_GetTitleList(NULL, MEDIATYPE_SD, alltitlescount, alltids);
	for (unsigned int i = 0; i < alltitlescount; i++)
	{
		//Doesn't work, but that's OK, it's not *that* costly
		//If it's already loaded, why bother loading it again? Loading textures is expensive.
		/*bool alreadyloaded = false;
		for (unsigned int j = 0; j < smdhvector->size(); j++)
		{
			if ((*smdhvector)[j].titl == alltids[i])
			{
				alreadyloaded = true;
				C3D_TexDelete(&((*alltitlesvector)[i].icon));
				alltitlesvector->push_back((*smdhvector)[j]);
			}
		}
		//Well, it hasn't already been loaded, so let's load it!
		if(!alreadyloaded)*/
		if (i == 0) //The cartridge goes here- load this at the back instead
			alltitlesvector.back().load(alltids[i]);
		else
			alltitlesvector[i].load(alltids[i]);
		notthis->functionprogress++;
		if(notthis->cancel)
			return;
	}
	/*bool cardinserted;
	FSUSER_CardSlotIsInserted(&cardinserted);
	if (cardinserted)
	{
		alltitlescount += 1;
		//Push the existing first title to the back so it doesn't get entirely overwritten by the cartridge
		//alltitlesvector.push_back(alltitlesvector[0]);
		updatecartridgedata();
	}*/
	//this->functionprogress++;
	//alltitlesvector.insert(alltitlesvector.begin(), smdhvector[0]); //Done by updatecartridgedata()
	//Remove titles that aren't titles- extdata, updates, etc.
	//Also determine if the title is active or not.
	vector<smdhdata>::iterator remove = \
		std::remove_if(alltitlesvector.begin(), alltitlesvector.end(), \
		[](const smdhdata& data) {return tid2str(data.titl)[7] != '0';});
	alltitlesvector.erase(remove, alltitlesvector.end());
	notthis->functiondone = true;
}

void initializeallSMDHdata(vector<u64> intitleids)
{
	tidstoload = intitleids;
	smdhvector.resize(tidstoload.size());
	//Initialize the SMDH vector TODO: initialize it with a "!" texture by doing TexCopy instead
	//We need to do this ahead of time in the main thread, because what if the main thread attempts to load
	//the textures before they're allocated? Instant segfault!
	for(unsigned int i = 0; i < smdhvector.size(); i++)
		C3D_TexInit(&(smdhvector[i].icon), 64, 64, GPU_RGB565);

	//Initialize the all-titles vector in the same way
	AM_GetTitleCount(MEDIATYPE_SD, &alltitlescount);
	alltitlesvector.resize(alltitlescount + 1);
	for (unsigned int i = 0; i < alltitlesvector.size() + 1; i++)
		C3D_TexInit(&(alltitlesvector[i].icon), 64, 64, GPU_RGB565);

	CFGU_GetSystemLanguage(&language);
	//Create the thread
	SMDHworker.startworker();
}

void freeSMDHdata()
{
	/*for(unsigned int i = 0; i < smdhvector.size(); i++)
	{
		//Get a pointer to the icon by dereferencing the vector and going by array count. Quite a handful of operators.
		C3D_TexDelete(&(smdhvector[i].icon));
	}*/
	for(vector<smdhdata>::iterator iter = smdhvector.begin(); iter != smdhvector.end(); iter++)
		C3D_TexDelete(&(iter->icon));
}

vector<smdhdata>& getSMDHdata()
{
	return smdhvector;
}

vector<smdhdata>& getallSMDHdata()
{
	return alltitlesvector;
}

/*bool alltitlesareloaded()
{
	return alltitlesloaded;
}

int getalltitlescount()
{
	return alltitlescount;
}

int getalltitlesloadedcount()
{
	return alltitlesloadedcount;
}*/

bool smdhdata::load(u64 title, int ingametype) //gametype is optional
{
	this->titl = title;
	//Search the global active TIDs vector
	if (std::find(titleids.begin(), titleids.end(), this->titl) != titleids.end())
		this->isactive = true;
	else
		this->isactive = false;
	smdh_s smdhstruct;
	if (ingametype != -1) //Skip looping and checking
	{
		if (ingametype == 2) //Get the title ID from the cartridge
		{
			bool cardinserted;
			FS_CardType type;
			FSUSER_CardSlotIsInserted(&cardinserted);
			FSUSER_GetCardType(&type);
			if (cardinserted && type == CARD_CTR)
			{
				AM_GetTitleList(NULL, MEDIATYPE_GAME_CARD, 1, &title);
				this->titl = title; //Set the TID again; it isn't 0
				this->isactive = true; //Cartridges are always active
			}
			else
			{
				this->titl = 0; 
				this->isactive = false; //Make sure we don't try to draw a null image
				return false;
			}
		}
		if(titlesLoadSmdh(&smdhstruct, ingametype, title))
			this->gametype = ingametype;
		else
		{
			//Set the short description equal to the title id (to show what wasn't found)
			//Show an error message for the long description and return
			stringstream tohex;
			tohex << std::hex << title;
			this->shortdesc = tohex.str();
			this->longdesc = "Couldn't find anything! If it's on a cartridge, is it inserted?";
			this->titl = title;
			return false;
		}
	}
	else
	{
		for (u8 i = 0; i < 3; i++)
		{
			if (titlesLoadSmdh(&smdhstruct, i, title))
			{
				this->gametype = i;
				break;
			}
			if (i == 2)
			{
				//We couldn't find anything as we haven't broken out, 
				//set the short description equal to the title id (to show what wasn't found)
				//Show an error message for the long description and return
				stringstream tohex;
				tohex << std::hex << title;
				this->shortdesc = tohex.str();
				this->longdesc = "Couldn't find anything! If it's on a cartridge, is it inserted?";
				this->titl = title;
				return false;
			}

		}
	}
	//48x48 -> 64x64
	u16* dest = (u16*)this->icon.data + (64-48)*64;
	u16* src = (u16*)smdhstruct.bigIconData;
	for (int j = 0; j < 48; j += 8)
	{
		memcpy(dest, src, 48*8*sizeof(u16));
		src += 48*8;
		dest += 64*8;
	}
	char* buf = new char[0x40 * 3 + 1]; //Size from new-hbmenu, not sure why it's that big
	safe_utf8_convert(buf, smdhstruct.applicationTitles[language].shortDescription, 0x40 * 3);
	this->shortdesc = string(buf);
	safe_utf8_convert(buf, smdhstruct.applicationTitles[language].longDescription, 0x40 * 3); //Reusing the buffer isn't a problem.
	this->longdesc = string(buf);
	delete[] buf;
	return true;
}

//This was straight copy-pasted from new-hbmenu. Give them props for it
bool titlesLoadSmdh(smdh_s* smdh, u8 mediatype, u64 title)
{
	static const u32 filePath[] = {0, 0, 2, 0x6E6F6369, 0};
	u32 lowtid = title & 0xFFFFFFFF;
	u32 hightid = title >> 32;
	u32 archivePath[] = {lowtid, hightid, mediatype, 0};

	FS_Path apath = { PATH_BINARY, sizeof(archivePath), archivePath };
	FS_Path fpath = { PATH_BINARY, sizeof(filePath), filePath };
	Handle file = 0;
	Result res = FSUSER_OpenFileDirectly(&file, ARCHIVE_SAVEDATA_AND_CONTENT, apath, fpath, FS_OPEN_READ, 0);
	if (R_FAILED(res)) return false;

	if (!smdh)
	{
		FSFILE_Close(file);
		return false;
	}

	u32 bytesRead;
	res = FSFILE_Read(file, &bytesRead, 0, smdh, sizeof(*smdh));
	FSFILE_Close(file);

	return R_SUCCEEDED(res) && bytesRead==sizeof(*smdh);
}

void safe_utf8_convert(char* buf, const u16* input, size_t bufsize)
{
	ssize_t units = utf16_to_utf8((uint8_t*)buf, input, bufsize);
	if (units < 0) units = 0;
	buf[units] = 0;
}

/*bool cardthreadstop = false;

void threadfunc_checkforcartridgeinserted(void* args)
{
	while(!cardthreadstop)
	{
		FSUSER_CardSlotIsInserted(&cardinserted);
		FSUSER_GetCardType(&type);
		svcSleepThread()
	}
}

void cartridgethreadinit()
{
	s32 mainthreadpriority;
	svcGetThreadPriority(&mainthreadpriority, CUR_THREAD_HANDLE);
	APT_SetAppCpuTimeLimit(30);
	threadCreate(threadfunc_checkforcartridgeinserted, NULL, 8000, mainthreadpriority - 5, 1, true);
}

void cartridgethreadfinish()
{
	cardthreadstop = true;
}*/

void cartridgesrvhook(u32 NotificationID)
{
	cartridgeneedsupdating = true;
}

void updatecartridgedata()
{
	cartridgeneedsupdating = false;
	vector<smdhdata>& icons = getSMDHdata();
	//Title loading might be done when this is called, but what if the user has changed the cartridge? 
	//Then we need to update it
	//This will freeze, but only for a slight moment, and no animations are occurring- the user won't notice
	u64 cartridgetitle;
	bool cardinserted;
	FS_CardType type = CARD_TWL;
	FSUSER_GetCardType(&type);
	FSUSER_CardSlotIsInserted(&cardinserted);
	if (cardinserted && type == CARD_CTR)
	{
		AM_GetTitleList(NULL, MEDIATYPE_GAME_CARD, 1, &cartridgetitle);
		if (cartridgetitle != icons[0].titl)
		{
			//Load the cartridge, it may take a few tries because FS may not be ready for us
			while (!icons[0].load(0, 2)) {}
			//if (icons[0].titl == 0) return; //It read out garbage
			//Update related stuff as well
			titleids[0] = icons[0].titl;
			slots[0] = 0;
			getallSMDHdata()[0] = icons[0];
			if (currenttidpos == 0)
			{
				maxslot = maxslotcheck();
				if (maxslot == 0)
				{
					error("Warning: Failed to find mods for\nthis game!");
					error("Place them at " + modsfolder + '\n' + currenttitleidstr + "/Slot_X\nwhere X is a number starting at 1.");
				}
				mainmenuupdateslotname();
			}
			//return;
		}
	}
	else if(icons[0].titl != 0)
	{
		icons[0].titl = 0;
		icons[0].isactive = false;
		//Update related stuff as well
		titleids[0] = 0;
		slots[0] = 0;
		if (currenttidpos == 0)
		{
			maxslot = 0;
			mainmenuupdateslotname();
			config.write("ModsEnabled", false);
		}
	}
}
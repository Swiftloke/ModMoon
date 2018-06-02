#include "main.hpp" //Also includes smdh.hpp
#include <algorithm>

vector<smdhdata> smdhvector;
vector<smdhdata> alltitlesvector;
u32 alltitlesloadedcount = 0;
u32 alltitlescount = 0;
u8 language = 0;
vector<u64> tidstoload;
bool alltitlesloaded = false;

void threadfunc_loadallsmdhdata(void* main)
{
	for(unsigned int i = 0; i < tidstoload.size(); i++)
		smdhvector[i].load(tidstoload[i]);

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
		alltitlesvector[i].load(alltids[i]);
		alltitlesloadedcount++;
	}
	//Remove titles that aren't titles- extdata, updates, etc.
	//Also determine if the title is active or not.
	vector<smdhdata>::iterator remove = \
		std::remove_if(alltitlesvector.begin(), alltitlesvector.end(), \
		[](const smdhdata& data) {return tid2str(data.titl)[7] != '0';});
	alltitlesvector.erase(remove, alltitlesvector.end());
	alltitlesloaded = true;
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
	alltitlesvector.resize(alltitlescount);
	for (unsigned int i = 0; i < alltitlesvector.size(); i++)
		C3D_TexInit(&(alltitlesvector[i].icon), 64, 64, GPU_RGB565);

	CFGU_GetSystemLanguage(&language);
	//Create the thread
	s32 mainthreadpriority;
	svcGetThreadPriority(&mainthreadpriority, CUR_THREAD_HANDLE);
	threadCreate(threadfunc_loadallsmdhdata, 0, 24000, mainthreadpriority + 1, -2, true);
}

void freeSMDHdata()
{
	for(unsigned int i = 0; i < smdhvector.size(); i++)
	{
		//Get a pointer to the icon by dereferencing the vector and going by array count. Quite a handful of operators.
		C3D_TexDelete(&(smdhvector[i].icon));
	}
}

vector<smdhdata>& getSMDHdata()
{
	return smdhvector;
}

vector<smdhdata>& getallSMDHdata()
{
	return alltitlesvector;
}

bool alltitlesareloaded()
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
}

void smdhdata::load(u64 title, int ingametype) //gametype is optional
{
	this->titl = title;
	//Search the global active TIDs vector
	if (std::find(titleids.begin(), titleids.end(), this->titl) != titleids.end())
		this->isactive = true;
	smdh_s smdhstruct;
	if (ingametype != -1) //Skip looping and checking
	{
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
			return;
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
				return;
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

void smdhmenu()
{
	
}
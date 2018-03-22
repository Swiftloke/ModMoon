#include "smdh.hpp"

vector<smdhdata>* smdhvector;
u8 language = 0;
vector<u64> tidstoload;
void threadfunc_loadallsmdhdata(void* main)
{
	for(unsigned int i = 0; i < tidstoload.size(); i++)
		(*smdhvector)[i].load(tidstoload[i]);
}

void initializeallSMDHdata(vector<u64> intitleids)
{
	tidstoload = intitleids;
	smdhvector = new vector<smdhdata>(tidstoload.size()); //Love how pointers are necessary for this construction
	//Initialize the SMDH vector with blank white textures TODO: initialize it with a "!" texture by doing TexCopy instead
	for(unsigned int i = 0; i < smdhvector->size(); i++)
		C3D_TexInit(&((*smdhvector)[i].icon), 64, 64, GPU_RGB565);
	CFGU_GetSystemLanguage(&language);
	//Create the thread
	s32 mainthreadpriority;
	svcGetThreadPriority(&mainthreadpriority, CUR_THREAD_HANDLE);
	threadCreate(threadfunc_loadallsmdhdata, 0, 24000, mainthreadpriority + 1, -2, true);
}

void freeSMDHdata()
{
	for(unsigned int i = 0; i < smdhvector->size(); i++)
	{
		//Get a pointer to the icon by dereferencing the vector and going by array count. Quite a handful of operators.
		C3D_TexDelete(&((*smdhvector)[i].icon));
	}
	delete smdhvector;
}

vector<smdhdata>* getSMDHdata()
{
	return smdhvector;
}

void smdhdata::load(u64 title)
{
	titl = title;
	smdh_s smdhstruct;
	for(u8 i = 0; i < 3; i++)
	{
		if(titlesLoadSmdh(&smdhstruct, i, title))
		{
			gametype = i;
			break;
		}
		if(i == 2)
		{
			//We couldn't find anything as we haven't broken out, 
			//set the short description equal to the title id (to show what wasn't found)
			//Show an error message for the long description and return
			stringstream tohex;
			tohex << std::hex << title;
			shortdesc = tohex.str();
			longdesc = "Couldn't find anything! If it's on a cartridge, is it inserted?";
			titl = title;
			return;
		}
		
	}
	//48x48 -> 64x64
	u16* dest = (u16*)icon.data + (64-48)*64;
	u16* src = (u16*)smdhstruct.bigIconData;
	for (int j = 0; j < 48; j += 8)
	{
		memcpy(dest, src, 48*8*sizeof(u16));
		src += 48*8;
		dest += 64*8;
	}
	char* buf = new char[0x40 * 3 + 1]; //Size from new-hbmenu, not sure why it's that big
	safe_utf8_convert(buf, smdhstruct.applicationTitles[language].shortDescription, 0x40 * 3);
	shortdesc = string(buf);
	safe_utf8_convert(buf, smdhstruct.applicationTitles[language].longDescription, 0x40 * 3); //Reusing the buffer isn't a problem.
	longdesc = string(buf);
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
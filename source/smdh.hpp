//TODO: Once the revamp to support many titles occurs and that define in main.hpp is removed, change "title" back to "titleid"
//(It conflicted with the define, but "title" doesn't explain a whole lot)

#include "sdraw.hpp"
#include "workerfunction.hpp"
#include <string>
#include <vector>
#include <sstream>

class smdhdata
{
	public:
	bool load(u64 title, int ingametype = -1);
	u64 titl = -1ULL; //We can be certain it's loaded if this isn't this constant
	string shortdesc, longdesc;
	C3D_Tex icon;
	u8 gametype;
	bool isactive = false; //Calculated in the loading function
};

//Used below
void worker_loadallsmdhdata(WorkerFunction* notthis);

extern WorkerFunction SMDHworker;

void initializeallSMDHdata(vector<u64>& intitleids);
void freeSMDHdata();
vector<smdhdata>& getSMDHdata();
vector<smdhdata>& getallSMDHdata();

void updatecartridgedata();
void cartridgesrvhook(u32 NotificationID);

//Stuff to parse the SMDH data (from new-hbmenu)
typedef struct
{
	u32 magic;
	u16 version;
	u16 reserved;
} smdhHeader_s;

typedef struct
{
	u16 shortDescription[0x40];
	u16 longDescription[0x80];
	u16 publisher[0x40];
} smdhTitle_s;

typedef struct
{
	u8 gameRatings[0x10];
	u32 regionLock;
	u8 matchMakerId[0xC];
	u32 flags;
	u16 eulaVersion;
	u16 reserved;
	u32 defaultFrame;
	u32 cecId;
} smdhSettings_s;

typedef struct
{
	smdhHeader_s header;
	smdhTitle_s applicationTitles[16];
	smdhSettings_s settings;
	u8 reserved[0x8];
	u8 smallIconData[0x480];
	u16 bigIconData[0x900];
} smdh_s;

bool titlesLoadSmdh(smdh_s* smdh, u8 mediatype, u64 title);
void safe_utf8_convert(char* buf, const u16* input, size_t bufsize);
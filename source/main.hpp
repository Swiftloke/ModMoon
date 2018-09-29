#pragma once
#if __INTELLISENSE__
typedef unsigned int __SIZE_TYPE__;
typedef unsigned long __PTRDIFF_TYPE__;
#define __attribute__(q)
#define __builtin_strcmp(a,b) 0
#define __builtin_strlen(a) 0
#define __builtin_memcpy(a,b) 0
#define __builtin_va_list void*
#define __builtin_va_start(a,b)
#define __extension__
#endif

#if defined(_MSC_VER)
#include <BaseTsd.h>
typedef SSIZE_T ssize_t;
#endif

#pragma once
#include "sdraw.hpp"
#include "config.hpp"
#include "smdh.hpp"
#include <string>
#include <vector>

using namespace std;

//Globals
extern sDraw_interface draw;
extern std::pair<C3D_Tex*, Tex3DS_Texture> spritesheet;
extern C3D_Tex* progressfiller; //This needs to be in its own texture due to usage of wrapping for animation
extern sdraw_stex leftbutton;
extern sdraw_stex rightbutton;
extern sdraw_stex selector;
extern sdraw_stex backgroundtop;
extern sdraw_stex backgroundbot;
extern sdraw_stex banner;
extern sdraw_stex textbox;
extern sdraw_stex textboxokbutton;
extern sdraw_highlighter textboxokbuttonhighlight;
extern sdraw_stex titleselectionboxes;
extern sdraw_stex titleselectionsinglebox;
extern sdraw_stex titleselectioncartridge;
extern sdraw_highlighter titleselecthighlighter;
extern sdraw_stex progressbar;
extern sdraw_stex progressbarstenciltex;
extern sdraw_stex secret;
extern sdraw_highlighter toolsmenuhighlighter;
extern sdraw_stex activetitlesbutton;
extern sdraw_stex smashcontrolsbutton;
extern sdraw_stex tutorialbutton;
extern sdraw_stex migrationbutton;
extern sdraw_stex darkmodebutton;
extern sdraw_stex lightmodebutton;

extern Config config;
extern bool modsenabled;
extern string modsfolder;
extern string slotname;
extern int maxslot;
extern bool cartridgeneedsupdating;

extern bool shoulddisableerrors;
extern bool shoulddisableupdater;

extern float minusy; //Needed for the fading function

extern vector<u64> titleids;
extern vector<int> slots;
extern int currenttidpos;
#define currenttitleid titleids[currenttidpos]
#define currentslot slots[currenttidpos]
string tid2str(u64 in);
#define currenttitleidstr tid2str(currenttitleid)
string hex2str(u32 in);


int startup();
void enablemods(bool enabled);
void mainmenushiftout();
void mainmenushiftin();
void mainmenuupdateslotname();
void mainmenudraw(unsigned int dpadpos, touchPosition tpos, unsigned int alphapos, bool highlighterblink);
bool touched(sdraw_stex button, int dx, int dy, touchPosition tpos);
bool touched(int x, int y, int width, int height, touchPosition tpos);
bool buttonpressed(sdraw_stex button, int bx, int by, touchPosition lastpos, u32 kHeld);
bool buttonpressed(int bx, int by, int bwidth, int bheight, touchPosition lastpos, u32 kHeld);
bool issaltysdtitle(u64 optionaltitleid = 0);
//Returns "USA", "EUR" or "JPN"
string saltysdtidtoregion(u64 optionaltitleid = 0);
void drawtopscreen();
bool secretcodeadvance(u32 kDown);
string getversion();
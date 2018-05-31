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

using namespace std;

//Globals
extern sDraw_interface draw;
extern sdraw_texture* spritesheet;
extern sdraw_texture* progressfiller; //This needs to be in its own texture due to usage of wrapping for animation
extern sdraw_stex leftbutton;
extern sdraw_stex rightbutton;
extern sdraw_stex selector;
extern sdraw_stex backgroundtop;
extern sdraw_stex backgroundbot;
extern sdraw_stex banner;
extern sdraw_stex textbox;
extern sdraw_stex textboxokbutton;
extern sdraw_stex textboxokbuttonhighlight;
extern sdraw_stex titleselectionboxes;
extern sdraw_stex titleselectionsinglebox;
extern sdraw_stex titleselecthighlighter;
extern sdraw_stex progressbar;
extern sdraw_stex progressbarstenciltex;

extern Config config;
extern bool modsenabled;
extern string modsfolder;
extern string slotname;
extern int maxslot;

extern float minusy; //Needed for the fading function

extern vector<u64> titleids;
extern vector<int> slots;
extern int currenttidpos;
#define currenttitleid titleids[currenttidpos]
#define currentslot slots[currenttidpos]
string tid2str(u64 in);
#define currenttitleidstr tid2str(currenttitleid)


int startup();
void enablemods(bool enabled); //Vestigial from Smash-Selector 3.0
void mainmenushiftout();
void mainmenushiftin();
void mainmenuupdateslotname();
bool touched(sdraw_stex button, int dx, int dy, touchPosition tpos);
bool touched(int x, int y, int width, int height, touchPosition tpos);
bool buttonpressed(sdraw_stex button, int bx, int by, touchPosition lastpos, u32 kHeld);
bool issaltysdtitle();
void drawtopscreen();

void error(string text); //No need for a whole extra header for two functions...
void drawprogresserror(string text, int alphapos, float expandpos, float progress, C3D_Tex prevtopfb, C3D_Tex prevbotfb);
void titleselect(); //Ditto
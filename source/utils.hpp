#pragma once
#include <unistd.h>
#include <sys/stat.h>
#include <string>
#include "sdraw/sdraw.hpp"
using namespace std;

void drawtexturewithhighlight(sdraw_stex info, int x, int y, u32 color, 
	int alpha, int x1 = -1, int y1 = -1, float interpfactor = 0); 

Result nsRebootSystemClean();

//When title is not provided this simply checks the current tid.
int maxslotcheck(u64 optionaltid = 1, int optionalslot = -1);
void launch();
void threadfunc_fade(void* main);
int countEntriesInDir(const char* dirname);

//Extremely common logic used in rendering code, but it doesn't feel right to be in sDraw
void highlighterhandle(int& alphapos, bool& alphaplus);

void writeSaltySD(u64 titleid);

inline bool pathExist(const string filename) {
	struct stat buffer;
	return (stat(filename.c_str(), &buffer) == 0);
}

//Calculates the Murmur Hash 2 algorithm on a file.
unsigned int genHash(string filepath);

//Custom SaltySD files provided by modpacks
void movecustomsaltysdout();
void movecustomsaltysdin();

extern Handle event_fadefinished;
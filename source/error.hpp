#pragma once
#include "sdraw.hpp"
#include <string>
using namespace std;

enum ERRORMODE
{
	MODE_POPUP,
	MODE_FADE
};

void errorsetmode(ERRORMODE mode);
void error(string text);
void drawprogresserror(string text, float expandpos, float progress, C3D_Tex topfb, C3D_Tex botfb);
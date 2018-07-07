#pragma once
#include "sdraw.hpp"
#include <string>
using namespace std;

enum ERRORMODE
{
	MODE_POPUP,
	MODE_FADE
};

bool errorwasstartpressed();
void errorsetmode(ERRORMODE mode);
void error(string text);
//This can't have a complete function because the user still needs to feed it a progress value.
//When the C3D_Tex's height is 0, a black rectangle is drawn instead, to prevent issues
//with sDraw not using C3D_RenderTargetClear() due to it breaking framebuffer copy.
void drawprogresserror(string text, float expandpos, float progress, C3D_Tex topfb, C3D_Tex botfb);
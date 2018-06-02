#pragma once
#include "sdraw.hpp"
#include <string>
using namespace std;
void error(string text);
void drawprogresserror(string text, float expandpos, float progress, C3D_Tex topfb, C3D_Tex botfb);
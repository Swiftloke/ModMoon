/*
*   This file is part of ModMoon
*   Copyright (C) 2018-2019 Swiftloke
*
*   This program is free software: you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 3 of the License, or
*   (at your option) any later version.
*
*   This program is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
*   Additional Terms 7.b and 7.c of GPLv3 apply to this file:
*       * Requiring preservation of specified reasonable legal notices or
*         author attributions in that material or in the Appropriate Legal
*         Notices displayed by works containing it.
*       * Prohibiting misrepresentation of the origin of that material,
*         or requiring that modified versions of such material be marked in
*         reasonable ways as different from the original version.
*/

#pragma once
#include "sdraw/sdraw.hpp"
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
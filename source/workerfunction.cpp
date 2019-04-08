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

#include "workerfunction.hpp"
#include "error.hpp"
#include "main.hpp"

//Helper functions
void progresspopup(float& expandpos)
{
	expandpos += .06f;
	if (expandpos >= 1) expandpos = 1; //Prevent it from going overboard
}

void progresspopdown(float& expandpos)
{
	expandpos -= .06f;
	if (expandpos <= 0) expandpos = 0;
}

void WorkerFunction::startworker()
{
	s32 mainthreadpriority;
	svcGetThreadPriority(&mainthreadpriority, CUR_THREAD_HANDLE);
	//The lambda is to convert the std::function into a C function call and pass the argument of "this".
	this->workerthread = threadCreate([](void* passedThis)
	{ 
		auto thisPtr = (WorkerFunction*)passedThis; 
		thisPtr->worker(thisPtr);
	},
		this, 20000, (this->highpriority) ? 0x18 : mainthreadpriority + 1, -2, true);
}

void WorkerFunction::displayprogress()
{
	if(this->functiondone)
		return;
	C3D_TexInit(&this->progressfbtop, 256, 512, GPU_RGBA8);
	C3D_TexInit(&this->progressfbbot, 256, 512, GPU_RGBA8);
	sdraw::retrieveframebuffers(&progressfbtop, &progressfbbot);
	float expandpos = 0;
	string actualtext;
	do
	{
		actualtext = this->progressstring;
		actualtext.replace(actualtext.find("[progress]"), 10, to_string(this->functionprogress));
		actualtext.replace(actualtext.find("[total]"), 7, to_string(this->functiontotal));
		progresspopup(expandpos);
		drawprogresserror(actualtext, expandpos, (float)this->functionprogress / this->functiontotal, \
			this->progressfbtop, this->progressfbbot);
	} while (!this->functiondone);
	while (expandpos > 0)
	{
		progresspopdown(expandpos);
		drawprogresserror(actualtext, expandpos, (float)this->functionprogress / this->functiontotal, \
			this->progressfbtop, this->progressfbbot);
	}
	C3D_TexDelete(&this->progressfbtop);
	C3D_TexDelete(&this->progressfbbot);
}
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
#include <string>
#include <3ds.h>
#include <citro3d.h>
#include <functional>
using std::string;

/*
This is a class that defines a system of starting worker functions
as threads and provides a function that the main thread can use
to easily display progress errors for them.

So the way this works is that anything that calls a worker function
will call startworker() then optionally displayprogress().
Creating a worker function is done by passing a function
which accepts a WorkerFunction* (as an alternative to "this")
and progressstring with your progress text in the constructor.
*/
class WorkerFunction
{
public:
	WorkerFunction(std::function<void(WorkerFunction*)> infxn, string instr, bool ishighpriority = false) :
		worker(infxn), progressstring(instr), highpriority(ishighpriority) {};
	std::function<void(WorkerFunction*)> worker;
	void startworker();
	void displayprogress();
	string progressstring;
	//Forces the thread to stop. Because Horizon can not kill threads, 
	//it is the worker function's responsibility to handle this being set to true.
	//Unfortunate.
	void shutdown()
	{ 
		this->cancel = true; 
		threadJoin(workerthread, U64_MAX);
	}

//protected:
	
	//Extremely high priority threads, such as the ZIP extractor, may set this to true.
	bool highpriority;
	C3D_Tex progressfbtop, progressfbbot;
	bool functiondone = false;
	int functionprogress = 0;
	int functiontotal = 1;
	bool cancel = false;
	Thread workerthread;
};
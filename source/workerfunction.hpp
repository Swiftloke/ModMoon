#pragma once
#include <string>
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
	WorkerFunction(std::function<void(WorkerFunction*)> infxn, string instr) :
		worker(infxn), progressstring(instr) {};
	std::function<void(WorkerFunction*)> worker;
	void startworker();
	void displayprogress();
	string progressstring;

//protected:
	
	C3D_Tex progressfbtop, progressfbbot;
	bool functiondone = false;
	int functionprogress = 0;
	int functiontotal = 0;
};
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
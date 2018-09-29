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
		this, 20000, mainthreadpriority + 1, -2, true);
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
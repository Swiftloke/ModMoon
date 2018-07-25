#include "workerfunction.hpp"

//We have to override this to intialize the event.
class DownloadWorker : public WorkerFunction
{
public:
	DownloadWorker(std::function<void(WorkerFunction*)> infxn, string instr) : \
		WorkerFunction(infxn, instr) {};
	void startworker();
};

extern DownloadWorker updatecheckworker;
extern DownloadWorker updateinstallworker;

//void initupdatechecker();
bool isupdateavailable();
bool issaltysdupdateavailable();
//void initdownloadandinstallupdate();
void downloadsignalandwaitforcancel();
//unsigned int retrievedownloadprogress();
extern Handle event_downloadthreadfinished;

void threadfunc_updatechecker(WorkerFunction* notthis);
void threadfunc_downloadandinstallupdate(WorkerFunction* notthis);
void threadfunc_updatesaltysd(WorkerFunction* notthis);
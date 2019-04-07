#include "workerfunction.hpp"

//We have to override this to intialize the event.
class DownloadWorker : public WorkerFunction
{
public:
	DownloadWorker(std::function<void(WorkerFunction*)> infxn, string instr, bool ishighpriority = false) : \
		WorkerFunction(infxn, instr, ishighpriority) {};
	void startworker();
};

extern DownloadWorker updatecheckworker;
extern DownloadWorker updateinstallworker;
extern DownloadWorker saltysdupdaterworker;

//void initupdatechecker();
bool isupdateavailable();
bool issaltysdupdateavailable();
//void initdownloadandinstallupdate();
//unsigned int retrievedownloadprogress();
extern Handle event_downloadthreadfinished;

void threadfunc_updatechecker(WorkerFunction* notthis);
void threadfunc_downloadandinstallupdate(WorkerFunction* notthis);
void threadfunc_updatesaltysd(WorkerFunction* notthis);

Result http_download(const char *url, string savelocation, WorkerFunction* notthis, string fileout);
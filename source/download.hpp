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
//Modified http example for threaded usage and text returning / file saving
//NOTE TO SELF FOR LATER: If this doesn't work, check the URLs. Then check the files on the GitHub page.

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <inttypes.h>

#include <3ds.h>
#include <string>
#include <fstream>
#include "download.hpp"
#include "main.hpp"
using namespace std;

bool cancel = false;
bool updateisavailable = false;
string result;
unsigned int downloadprogress = 0;
Handle event_downloadthreadfinished;

Result http_download(const char *url, string savelocation)
{
	Result ret = 0;
	httpcContext context;
	char *newurl = NULL;
	u32 statuscode = 0;
	u32 contentsize = 0, readsize = 0, size = 0;
	u8 *buf, *lastbuf;

	do {
		ret = httpcOpenContext(&context, HTTPC_METHOD_GET, url, 1);

		// This disables SSL cert verification, so https:// will be usable
		ret = httpcSetSSLOpt(&context, SSLCOPT_DisableVerify);

		// Enable Keep-Alive connections
		ret = httpcSetKeepAlive(&context, HTTPC_KEEPALIVE_ENABLED);

		// Set a User-Agent header so websites can identify your application
		ret = httpcAddRequestHeaderField(&context, "User-Agent", "ModMoon");

		// Tell the server we can support Keep-Alive connections.
		// This will delay connection teardown momentarily (typically 5s)
		// in case there is another request made to the same server.
		ret = httpcAddRequestHeaderField(&context, "Connection", "Keep-Alive");

		ret = httpcBeginRequest(&context);
		if (ret != 0) {
			httpcCloseContext(&context);
			if (newurl != NULL) free(newurl);
			return ret;
		}

		ret = httpcGetResponseStatusCode(&context, &statuscode);
		if (ret != 0) {
			httpcCloseContext(&context);
			if (newurl != NULL) free(newurl);
			return ret;
		}

		if ((statuscode >= 301 && statuscode <= 303) || (statuscode >= 307 && statuscode <= 308)) {
			if (newurl == NULL) newurl = (char*)malloc(0x1000); // One 4K page for new URL
			if (newurl == NULL) {
				httpcCloseContext(&context);
				return -1;
			}
			ret = httpcGetResponseHeader(&context, "Location", newurl, 0x1000);
			url = newurl; // Change pointer to the url that we just learned
			httpcCloseContext(&context); // Close this context before we try the next

		if (cancel)
			return -5; //Let the thread die
		}
	} while ((statuscode >= 301 && statuscode <= 303) || (statuscode >= 307 && statuscode <= 308));

	if (statuscode != 200) {
		httpcCloseContext(&context);
		if (newurl != NULL) free(newurl);
		return -2;
	}

	// This relies on an optional Content-Length header and may be 0
	ret = httpcGetDownloadSizeState(&context, NULL, &contentsize);
	if (ret != 0) {
		httpcCloseContext(&context);
		if (newurl != NULL) free(newurl);
		return ret;
	}

	// Start with a single page buffer
	buf = (u8*)malloc(0x1000);
	if (buf == NULL) {
		httpcCloseContext(&context);
		if (newurl != NULL) free(newurl);
		return -1;
	}

	do {
		// This download loop resizes the buffer as data is read.
		downloadprogress = (size * 100) / contentsize;
		ret = httpcDownloadData(&context, buf + size, 0x1000, &readsize);
		size += readsize;
		if (ret == (s32)HTTPC_RESULTCODE_DOWNLOADPENDING) {
			lastbuf = buf; // Save the old pointer, in case realloc() fails.
			buf = (u8*)realloc(buf, size + 0x1000);
			if (buf == NULL) {
				httpcCloseContext(&context);
				free(lastbuf);
				if (newurl != NULL) free(newurl);
				return -1;
			}
		if(cancel)
			return -5; //Let the thread die
		}
	} while (ret == (s32)HTTPC_RESULTCODE_DOWNLOADPENDING);

	if (ret != 0) {
		httpcCloseContext(&context);
		if (newurl != NULL) free(newurl);
		free(buf);
		return -1;
	}

	// Resize the buffer back down to our actual final size
	lastbuf = buf;
	buf = (u8*)realloc(buf, size);
	if (buf == NULL) { // realloc() failed.
		httpcCloseContext(&context);
		free(lastbuf);
		if (newurl != NULL) free(newurl);
		return -1;
	}

	httpcCloseContext(&context);
	if (savelocation == "TEXT")
	{
		buf[contentsize] = '\0';
		result = string((char*)buf);
	}
	else if(savelocation == "FILE")
	{
		ofstream out(savelocation, ios::trunc);
		out << buf;
		out.close();
	}
	else if (savelocation == "INSTALL")
	{
		romfsExit(); //One cannot have an open handle to their romFS while they are updating it
		downloadprogress = 101; //Signal we're currently installing
		if (BUILDIS3DSX)
		{
			ofstream out(savelocation, ios::trunc);
			out << buf;
			out.close();
		}
		else //Install the CIA we downloaded
		{
			Handle cia;
			AM_QueryAvailableExternalTitleDatabase(NULL);
			AM_StartCiaInstall(MEDIATYPE_SD, &cia);
			FSFILE_Write(cia, NULL, 0, buf, contentsize, 0);
			AM_FinishCiaInstall(cia);
		}
		downloadprogress = 102; //Signal that we're all done
	}
	free(buf);
	if (newurl != NULL) free(newurl);
	return 0;
}

//Update checking will need to be changed to usage of ints like "30" and "31" for 3.0 and 3.1.
//Another bonus is that old versions will detect this as newer even with the stupid idea of using stof for update checking.
//
void threadfunc_updatechecker(void* unused)
{
	if(!osGetWifiStrength()) goto fail;
	string URL = "http://swiftloke.github.io/ModMoon/ModMoonVersion.txt";
	if (http_download(URL.c_str(), "TEXT")) goto fail;
	int newversion = stoi(result);
	if (newversion > config.read("ModMoonVersion", 0))
		updateisavailable = true;
	fail:
	svcSignalEvent(event_downloadthreadfinished);
}

void initupdatechecker()
{
	if (!osGetWifiStrength()) return; //We're not connected
	s32 mainthreadpriority;
	svcGetThreadPriority(&mainthreadpriority, CUR_THREAD_HANDLE);
	svcCreateEvent(&event_downloadthreadfinished, RESET_ONESHOT);
	threadCreate(threadfunc_updatechecker, NULL, 8000, mainthreadpriority + 1, -2, true);
}

bool isupdateavailable()
{
	return updateisavailable;
}

void threadfunc_downloadandinstallupdate(void* unused)
{
	string URL = "http://swiftloke.github.io/ModMoon/ModMoonBin";
	if (BUILDIS3DSX)
		URL.append(".3dsx");
	else
		URL.append(".cia");
	http_download(URL.c_str(), "INSTALL");
}

void initdownloadandinstallupdate()
{
	if (!osGetWifiStrength()) return; //We're not connected
	s32 mainthreadpriority;
	svcGetThreadPriority(&mainthreadpriority, CUR_THREAD_HANDLE);
	threadCreate(threadfunc_downloadandinstallupdate, NULL, 8000, mainthreadpriority + 1, -2, true);
}

unsigned int retrievedownloadprogress()
{
	return downloadprogress;
}

void downloadsignalandwaitforcancel()
{
	//Set the cancel flag to true from the main thread so the download thread will finish and close
	cancel = true;
	//Wait for it to close
	svcWaitSynchronization(event_downloadthreadfinished, U64_MAX);
}
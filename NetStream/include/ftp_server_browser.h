#ifndef _FTP_SERVER_BROWSER_H_
#define _FTP_SERVER_BROWSER_H_

#include <kernel.h>
#include <paf.h>
#include <psp2_compat/curl/curl.h>

#include "generic_server_browser.h"

using namespace paf;

class FtpServerBrowser : public GenericServerBrowser
{
public:

	FtpServerBrowser(const char *host, const char *port, const char *user, const char *password);

	~FtpServerBrowser();

	SceBool Probe();

	SceBool IsAtRoot(string *current);

	SceBool IsAtRoot();

	SceVoid SetPath(const char *ref);

	string GetPath();

	string GetBEAVUrl(string *in);

	vector<FtpServerBrowser::Entry *> *GoTo(const char *ref, SceInt32 *result);

private:

	static SceSize DownloadCore(char *buffer, SceSize size, SceSize nitems, ScePVoid userdata);

	CURLU *url;
	CURL *curl;
	string root;
	char *buffer;
	SceUInt32 posInBuf;
	SceInt32 useNlst;
};

#endif
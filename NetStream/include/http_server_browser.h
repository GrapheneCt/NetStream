#ifndef _HTTP_SERVER_BROWSER_H_
#define _HTTP_SERVER_BROWSER_H_

#include <kernel.h>
#include <paf.h>
#include <psp2_compat/curl/curl.h>

#include "generic_server_browser.h"

using namespace paf;

class HttpServerBrowser : public GenericServerBrowser
{
public:

	HttpServerBrowser(const char *host, const char *port, const char *user, const char *password);

	~HttpServerBrowser();

	SceBool Probe();

	SceBool IsAtRoot(string *current);

	SceBool IsAtRoot();

	SceVoid SetPath(const char *ref);

	string GetPath();

	string GetBEAVUrl(string *in);

	vector<HttpServerBrowser::Entry *> *GoTo(const char *ref, SceInt32 *result);

private:

	static SceSize DownloadCore(char *buffer, SceSize size, SceSize nitems, ScePVoid userdata);

	CURLU *url;
	CURL *curl;
	string root;
	char *buffer;
	SceUInt32 posInBuf;
};

#endif
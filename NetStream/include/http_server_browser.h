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

	bool Probe();

	bool IsAtRoot(string *current);

	bool IsAtRoot();

	void SetPath(const char *ref);

	string GetPath();

	string GetBEAVUrl(string const& in);

	vector<HttpServerBrowser::Entry *> *GoTo(const char *ref, int32_t *result);

private:

	static size_t DownloadCore(char *buffer, size_t size, size_t nitems, void *userdata);

	CURLU *url;
	CURL *curl;
	string root;
	char *buffer;
	uint32_t posInBuf;
};

#endif
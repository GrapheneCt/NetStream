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

	bool Probe();

	bool IsAtRoot(string *current);

	bool IsAtRoot();

	void SetPath(const char *ref);

	string GetPath();

	string GetBEAVUrl(string const& in);

	vector<FtpServerBrowser::Entry *> *GoTo(const char *ref, int32_t *result);

private:

	static size_t DownloadCore(char *buffer, size_t size, size_t nitems, void *userdata);

	CURLU *m_url;
	CURL *m_curl;
	string m_root;
	char *m_buffer;
	uint32_t m_posInBuf;
	int32_t m_useNlst;
};

#endif
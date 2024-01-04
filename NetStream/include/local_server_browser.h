#ifndef _LOCAL_SERVER_BROWSER_H_
#define _LOCAL_SERVER_BROWSER_H_

#include <kernel.h>
#include <paf.h>
#include <psp2_compat/curl/curl.h>

#include "generic_server_browser.h"

using namespace paf;

class LocalServerBrowser : public GenericServerBrowser
{
public:

	static bool DefaultFsSort(const LocalServerBrowser::Entry* a, const LocalServerBrowser::Entry* b);

	LocalServerBrowser();

	~LocalServerBrowser();

	bool Probe();

	bool IsAtRoot(string *current);

	bool IsAtRoot();

	void SetPath(const char *ref);

	string GetPath();

	string GetBEAVUrl(string const& in);

	vector<LocalServerBrowser::Entry *> *GoTo(const char *ref, int32_t *result);

private:

	string m_path;
};

#endif
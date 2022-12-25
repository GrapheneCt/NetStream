#ifndef _HTTP_SERVER_BROWSER_H_
#define _HTTP_SERVER_BROWSER_H_

#include <kernel.h>
#include <paf.h>
#include <psp2_compat/curl/curl.h>

class HttpServerBrowser
{
public:

	class Entry
	{
	public:

		enum Type
		{
			Type_UnsupportedFile,
			Type_SupportedFile,
			Type_Folder
		};

		string ref;
		Type type;
	};

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
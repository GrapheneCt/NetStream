#ifndef _LOCAL_SERVER_BROWSER_H_
#define _LOCAL_SERVER_BROWSER_H_

#include <kernel.h>
#include <paf.h>
#include <psp2_compat/curl/curl.h>

using namespace paf;

class LocalServerBrowser
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

	LocalServerBrowser();

	~LocalServerBrowser();

	SceBool Probe();

	SceBool IsAtRoot(string *current);

	SceBool IsAtRoot();

	SceVoid SetPath(const char *ref);

	string GetPath();

	string GetBEAVUrl(string *in);

	vector<LocalServerBrowser::Entry *> *GoTo(const char *ref, SceInt32 *result);

private:

	string path;
};

#endif
#ifndef _GENERIC_SERVER_BROWSER_H_
#define _GENERIC_SERVER_BROWSER_H_

#include <kernel.h>
#include <paf.h>

using namespace paf;

class GenericServerBrowser
{
public:

	class Entry
	{
	public:

		enum Type
		{
			Type_UnsupportedFile,
			Type_SupportedFile,
			Type_PlaylistFile,
			Type_Folder
		};

		string ref;
		SceUInt32 type;
	};

	GenericServerBrowser()
	{

	}

	virtual ~GenericServerBrowser()
	{

	}

	virtual SceBool Probe() = 0;

	virtual SceBool IsAtRoot(string *current) = 0;

	virtual SceBool IsAtRoot() = 0;

	virtual SceVoid SetPath(const char *ref) = 0;

	virtual string GetPath() = 0;

	virtual string GetBEAVUrl(string *in) = 0;

	virtual vector<GenericServerBrowser::Entry *> *GoTo(const char *ref, SceInt32 *result) = 0;
};

#endif
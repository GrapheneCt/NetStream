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

		paf::string ref;
		uint32_t type;
	};

	GenericServerBrowser()
	{

	}

	virtual ~GenericServerBrowser()
	{

	}

	virtual bool Probe() = 0;

	virtual bool IsAtRoot(string *current) = 0;

	virtual bool IsAtRoot() = 0;

	virtual void SetPath(const char *ref) = 0;

	virtual string GetPath() = 0;

	virtual string GetBEAVUrl(string *in) = 0;

	virtual vector<GenericServerBrowser::Entry *> *GoTo(const char *ref, int32_t *result) = 0;
};

#endif
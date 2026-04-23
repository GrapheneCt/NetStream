#ifndef _HVDB_TRACKS_BROWSER_H_
#define _HVDB_TRACKS_BROWSER_H_

#include <kernel.h>
#include <paf.h>
#include <psp2_compat/curl/curl.h>

#include "browsers/generic_server_browser.h"

using namespace paf;

class HvdbTracksBrowser : public GenericServerBrowser
{
public:

	HvdbTracksBrowser(const char *workId, const char *workBannerUrl);

	~HvdbTracksBrowser();

	bool Probe();

	bool IsAtRoot(string *current);

	bool IsAtRoot();

	void SetPath(const char *ref);

	string GetPath();

	void GetInfoForPlayer(string const& in, string& url, GenericPlayer::Option **opt);

	vector<HvdbTracksBrowser::Entry *> *GoTo(const char *ref, int32_t *result);

private:

	void ParseRefString(const char *str, HVDBItem **item, uint32_t *idx);

	HVDBItem *GetItemAt(HVDBItem *firstItem, uint32_t idx);

	string m_path;
	string m_currentDataName;
	string m_workId;
	string m_bannerUrl;
	void *m_hvdbCtx;
	GenericPlayer::Option *m_option;
};

#endif
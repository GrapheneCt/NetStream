#ifndef _MENU_HVDB_TRACKS_H_
#define _MENU_HVDB_TRACKS_H_

#include <kernel.h>
#include <paf.h>

#include "dialog.h"
#include "menu_server.h"
#include "browsers/hvdb_tracks_browser.h"

using namespace paf;

namespace menu {
	class HvdbTracks : public GenericServerMenu
	{
	public:

		HvdbTracks(const char *workId, const char *bannerUrl)
		{
			m_browser = new HvdbTracksBrowser(workId, bannerUrl);
		}

		MenuType GetMenuType() override
		{
			return MenuType_HvdbTracks;
		}

		const uint32_t *GetSupportedSettingsItems(int32_t *count) override
		{
			*count = sizeof(k_settingsIdList) / sizeof(char*);
			return k_settingsIdList;
		}

	private:

		const uint32_t k_settingsIdList[3] = {
			hvdb_account_setting,
			hvdb_search_setting,
			hvdb_player_setting
		};
	};
}

#endif
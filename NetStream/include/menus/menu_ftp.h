#ifndef _MENU_FTP_H_
#define _MENU_FTP_H_

#include <kernel.h>
#include <paf.h>

#include "dialog.h"
#include "menu_server.h"
#include "ftp_server_browser.h"
#include "menus/menu_player_simple.h"

using namespace paf;

namespace menu {
	class Ftp : public GenericServerMenu
	{
	public:

		Ftp()
		{
			sce::AppSettings *settings = menu::Settings::GetAppSetInstance();

			char host[256];
			char port[32];
			char user[256];
			char password[256];

			settings->GetString("ftp_host", host, sizeof(host), "");
			settings->GetString("ftp_port", port, sizeof(port), "");
			settings->GetString("ftp_user", user, sizeof(user), "");
			settings->GetString("ftp_password", password, sizeof(password), "");

			m_browser = new FtpServerBrowser(host, port, user, password);
		}

		MenuType GetMenuType()
		{
			return MenuType_Ftp;
		}

		const uint32_t *GetSupportedSettingsItems(int32_t *count)
		{
			*count = sizeof(k_settingsIdList) / sizeof(char*);
			return k_settingsIdList;
		}

	private:

		const uint32_t k_settingsIdList[1] = {
			ftp_setting
		};
	};
}

#endif
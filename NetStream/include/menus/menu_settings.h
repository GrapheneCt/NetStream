#ifndef _MENU_SETTINGS_H_
#define _MENU_SETTINGS_H_

#include <paf.h>
#include <app_settings.h>

#include "menus/menu_generic.h"

using namespace paf;

namespace menu {
	class Settings
	{
	public:

		enum
		{
			SettingsEvent = (ui::Handler::CB_STATE + 0x30000),
		};

		enum SettingsEvent
		{
			SettingsEvent_Close,
			SettingsEvent_ValueChange
		};

		enum Hash
		{
			Hash_HttpServerHost = 0x1,
			Hash_HttpServerPort = 0x1,
			Hash_HttpServerUser = 0x1,
			Hash_HttpServerPassword = 0x1
		};

		typedef void(*CloseCallback)(void *pUserArg);
		typedef int32_t(*ValueChangeCallback)(const char *id, const char *newValue, void *pUserArg);

		Settings();

		~Settings();

		static Settings *GetInstance();

		static sce::AppSettings *GetAppSetInstance();

		static void Init();

	private:

		static void CBOnStartPageTransition(const char *elementId, int32_t type);

		static void CBOnPageActivate(const char *elementId, int32_t type);

		static void CBOnPageDeactivate(const char *elementId, int32_t type);

		static int32_t CBOnCheckVisible(const char *elementId, bool *pIsVisible);

		static int32_t CBOnPreCreate(const char *elementId, sce::AppSettings::Element *element);

		static int32_t CBOnPostCreate(const char *elementId, paf::ui::Widget *widget);

		static int32_t CBOnPress(const char *elementId, const char *newValue);

		static int32_t CBOnPress2(const char *elementId, const char *newValue);

		static void CBOnTerm(int32_t result);

		static wchar_t *CBOnGetString(const char *elementId);

		static int32_t CBOnGetSurface(graph::Surface **surf, const char *elementId);
	};
}

#endif

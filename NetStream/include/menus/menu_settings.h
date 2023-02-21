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

		enum Hash
		{
			Hash_HttpServerHost = 0x1,
			Hash_HttpServerPort = 0x1,
			Hash_HttpServerUser = 0x1,
			Hash_HttpServerPassword = 0x1
		};

		typedef void(*CloseCallback)(ScePVoid pUserArg);
		typedef SceInt32(*ValueChangeCallback)(const char *id, const char *newValue, ScePVoid pUserArg);

		Settings();

		~Settings();

		SceVoid SetCloseCallback(CloseCallback cb, ScePVoid uarg);

		static SceVoid SetValueChangeCallback(ValueChangeCallback cb, ScePVoid uarg);

		static Settings *GetInstance();

		static sce::AppSettings *GetAppSetInstance();

		static SceVoid Init();

	private:

		static SceVoid CBOnStartPageTransition(const char *elementId, SceInt32 type);

		static SceVoid CBOnPageActivate(const char *elementId, SceInt32 type);

		static SceVoid CBOnPageDeactivate(const char *elementId, SceInt32 type);

		static SceInt32 CBOnCheckVisible(const char *elementId, SceBool *pIsVisible);

		static SceInt32 CBOnPreCreate(const char *elementId, sce::AppSettings::Element *element);

		static SceInt32 CBOnPostCreate(const char *elementId, paf::ui::Widget *widget);

		static SceInt32 CBOnPress(const char *elementId, const char *newValue);

		static SceInt32 CBOnPress2(const char *elementId, const char *newValue);

		static SceVoid CBOnTerm(SceInt32 result);

		static wchar_t *CBOnGetString(const char *elementId);

		static SceInt32 CBOnGetSurface(graph::Surface **surf, const char *elementId);

		CloseCallback closeCb;
		ScePVoid closeCbUserArg;

	};
}

#endif

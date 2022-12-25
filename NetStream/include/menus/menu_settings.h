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

		static SceVoid CBListChange(const char *elementId);

		static SceVoid CBListForwardChange(const char *elementId);

		static SceVoid CBListBackChange(const char *elementId);

		static SceInt32 CBIsVisible(const char *elementId, SceBool *pIsVisible);

		static SceInt32 CBElemInit(const char *elementId);

		static SceInt32 CBElemAdd(const char *elementId, paf::ui::Widget *widget);

		static SceInt32 CBValueChange(const char *elementId, const char *newValue);

		static SceInt32 CBValueChange2(const char *elementId, const char *newValue);

		static SceVoid CBTerm();

		static wchar_t *CBGetString(const char *elementId);

		static SceInt32 CBGetTex(graph::Surface **tex, const char *elementId);

		CloseCallback closeCb;
		ScePVoid closeCbUserArg;

	};
}

#endif

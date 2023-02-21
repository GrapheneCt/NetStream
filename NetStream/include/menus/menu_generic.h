#ifndef _MENU_GENERIC_H_
#define _MENU_GENERIC_H_

#include <kernel.h>
#include <paf.h>

using namespace paf;

namespace menu {

	enum MenuType
	{
		MenuType_First,
		MenuType_Http,
		MenuType_Ftp,
		MenuType_Local,
		MenuType_Youtube,
		MenuType_PlayerSimple,
		MenuType_PlayerYouTube,
	};

	SceVoid SettingsButtonCbFun(SceInt32 eventId, ui::Widget *self, SceInt32 a3, ScePVoid pUserData);

	class MenuOpenParam
	{
	public:

		MenuOpenParam(bool useFadeIn = false, SceFloat32 fadeinTimeMs = 200.0f, Plugin::PageEffectType effectType = Plugin::PageEffectType_None, SceUInt32 openFlags = 0, SceInt32 priority = -1)
		{
			this->ctxOpt = SCE_NULL;

			this->useFadein = useFadeIn;
			this->fadeinTimeMs = fadeinTimeMs;
			this->effectType = effectType;
			this->priority = priority;

			this->unk_00 = 0;
			this->unk_04 = 0;
			this->unk_0C = 0;
			this->unk_10 = 0;
			this->unk_20 = 0;
			if (openFlags != 0)
			{
				if ((openFlags & ui::Context::Option::Flag_ResolutionHalfHd) == ui::Context::Option::Flag_ResolutionHalfHd
					|| (openFlags & ui::Context::Option::Flag_ResolutionFullHd) == ui::Context::Option::Flag_ResolutionFullHd)
				{
					if (SCE_PAF_IS_DOLCE)
					{
						this->ctxOpt = new ui::Context::Option(openFlags);
					}
				}
				else
				{
					this->ctxOpt = new ui::Context::Option(openFlags);
				}
			}
			this->uiOpt = this->ctxOpt;
			this->unk_28_pageArg_a5 = 0x80;
		}

		~MenuOpenParam()
		{
			delete this->ctxOpt;
		}

		SceInt32 unk_00;
		SceInt32 unk_04;
		SceInt32 priority;
		SceInt32 unk_0C;
		SceInt32 unk_10;
		bool useFadein;
		SceFloat32 fadeinTimeMs;
		Plugin::PageEffectType effectType;
		SceInt32 unk_20;
		ui::Context::Option *uiOpt;
		SceInt32 unk_28_pageArg_a5;

	private:

		ui::Context::Option *ctxOpt;
	};

	class MenuCloseParam
	{
	public:

		MenuCloseParam(bool useFadeOut = false, SceFloat32 fadeoutTimeMs = 200.0f, Plugin::PageEffectType effectType = Plugin::PageEffectType_None)
		{
			this->useFadeout = useFadeOut;
			this->fadeoutTimeMs = fadeoutTimeMs;
			this->effectType = effectType;
			this->reserved = 0;
		}

		~MenuCloseParam()
		{

		}

		bool useFadeout;
		SceFloat32 fadeoutTimeMs;
		Plugin::PageEffectType effectType;
		SceInt32 reserved;
	};

	class GenericMenu
	{
	public:

		GenericMenu(const char *name, MenuOpenParam oparam, MenuCloseParam cparam);

		virtual ~GenericMenu();

		virtual SceVoid Activate();

		virtual SceVoid Deactivate();

		virtual SceVoid DisableInput();

		virtual SceVoid EnableInput();

		virtual MenuType GetMenuType() = 0;

		virtual const SceUInt32 *GetSupportedSettingsItems(SceInt32 *count) = 0;

		ui::Scene *root;

	private:

		static SceVoid DeactivatorFwCbFun(SceInt32 eventId, ui::Widget *self, SceInt32 a3, ScePVoid pUserData);

		Plugin::PageCloseParam closeParam;
	};

	SceVoid InitMenuSystem();

	SceVoid TermMenuSystem();

	SceUInt32 GetMenuCount();

	menu::GenericMenu *GetTopMenu();

	menu::GenericMenu *GetMenuAt(SceUInt32 idx);

	SceVoid DeactivateAll(SceUInt32 endMargin = 0);

	SceVoid ActivateAll();
}


#endif
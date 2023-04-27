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

	void SettingsButtonCbFun(int32_t type, ui::Handler *self, ui::Event *e, void *userdata);

	class MenuOpenParam : public Plugin::PageOpenParam
	{
	public:

		MenuOpenParam(bool useFadeIn = false, float fadeinTimeMs = 200.0f, Plugin::TransitionType effectType = Plugin::TransitionType_None, uint32_t openFlags = 0, int32_t priority = -1)
		{
			envOpt = NULL;

			fade = useFadeIn;
			fade_time_ms = fadeinTimeMs;
			transition_type = effectType;
			overwrite_draw_priority = priority;

			if (openFlags != 0)
			{
				if ((openFlags & ui::EnvironmentParam::RESOLUTION_HD_HALF) == ui::EnvironmentParam::RESOLUTION_HD_HALF
					|| (openFlags & ui::EnvironmentParam::RESOLUTION_HD_FULL) == ui::EnvironmentParam::RESOLUTION_HD_FULL)
				{
					if (SCE_PAF_IS_DOLCE)
					{
						envOpt = new ui::EnvironmentParam(openFlags);
					}
				}
				else
				{
					envOpt = new ui::EnvironmentParam(openFlags);
				}
			}

			env_param = envOpt;
			graphics_flag = 0x80;
		}

		~MenuOpenParam()
		{
			delete envOpt;
		}

	private:

		ui::EnvironmentParam *envOpt;
	};

	class MenuCloseParam : public Plugin::PageCloseParam
	{
	public:

		MenuCloseParam(bool useFadeOut = false, float fadeoutTimeMs = 200.0f, Plugin::TransitionType effectType = Plugin::TransitionType_None)
		{
			fade = useFadeOut;
			fade_time_ms = fadeoutTimeMs;
			transition_type = effectType;
		}

		~MenuCloseParam()
		{

		}
	};

	class GenericMenu
	{
	public:

		GenericMenu(const char *name, MenuOpenParam const& oparam, MenuCloseParam const& cparam);

		virtual ~GenericMenu();

		virtual void Activate();

		virtual void Deactivate(bool withDelay = false);

		virtual void DisableInput();

		virtual void EnableInput();

		virtual MenuType GetMenuType() = 0;

		virtual const uint32_t *GetSupportedSettingsItems(int32_t *count) = 0;

		ui::Scene *GetRoot();

	protected:

		ui::Scene *root;

	private:

		MenuCloseParam closeParam;
	};

	void InitMenuSystem();

	void TermMenuSystem();

	uint32_t GetMenuCount();

	menu::GenericMenu *GetTopMenu();

	menu::GenericMenu *GetMenuAt(uint32_t idx);

	void DeactivateAll(uint32_t endMargin = 0);

	void ActivateAll();
}


#endif
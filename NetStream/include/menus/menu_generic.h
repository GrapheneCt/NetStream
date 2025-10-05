#ifndef _MENU_GENERIC_H_
#define _MENU_GENERIC_H_

#include <kernel.h>
#include <paf.h>

using namespace paf;

namespace menu {

	enum MenuType
	{
		MenuType_First,			// Initial menu that is displayed after application boot
		MenuType_Http,			// HTTP server menu
		MenuType_Ftp,			// FTP server menu
		MenuType_Local,			// Local file browser menu
		MenuType_Youtube,		// YouTube menu
		MenuType_Hvdb,			// HVDB menu
		MenuType_PlayerSimple,	// Simple Player menu (simple fullscreen player)
		MenuType_PlayerYouTube,	// YouTube Player menu (description/comments etc.)
	};

	/**
	 * Opens settings menu
	 *
	 * @param[in] type - PAF event type
	 * @param[in] self - PAF widget event issuer
	 * @param[in] e - PAF event object
	 * @param[in] userdata - Event userdata
	 */
	void SettingsButtonCbFun(int32_t type, ui::Handler *self, ui::Event *e, void *userdata);

	/**
	 * Parameters that will be used for creating/opening the menu
	 */
	class MenuOpenParam : public Plugin::PageOpenParam
	{
	public:

		/**
		 * @param[in] useFadeIn - when true, menu fades in
		 * @param[in] fadeinTimeMs - fadein time in milliseconds
		 * @param[in] effectType - PAF transition effect, one of paf::Plugin::TransitionType, can be used in combination with fadein
		 * @param[in] openFlags - PAF environment option flags, see paf::ui::EnvironmentParam
		 * @param[in] priority - PAF menu priority (rendering order)
		 */
		MenuOpenParam(
			bool useFadeIn = false,
			float fadeinTimeMs = 200.0f,
			Plugin::TransitionType effectType = Plugin::TransitionType_None,
			uint32_t openFlags = 0,
			int32_t priority = -1) :
			m_envOpt(nullptr)
		{
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
						m_envOpt = new ui::EnvironmentParam(openFlags);
					}
				}
				else
				{
					m_envOpt = new ui::EnvironmentParam(openFlags);
				}
			}

			env_param = m_envOpt;
			graphics_flag = 0x80;
		}

		~MenuOpenParam()
		{
			delete m_envOpt;
		}

	private:

		ui::EnvironmentParam *m_envOpt;	// PAF environment parameters object
	};

	/**
	 * Parameters that will be used for closing the menu
	 */
	class MenuCloseParam : public Plugin::PageCloseParam
	{
	public:

		/**
		 * @param[in] useFadeOut - when true, menu fades out
		 * @param[in] fadeoutTimeMs - fadeout time in milliseconds
		 * @param[in] effectType - PAF transition effect, one of paf::Plugin::TransitionType, can be used in combination with fadeout
		 */
		MenuCloseParam(
			bool useFadeOut = false,
			float fadeoutTimeMs = 200.0f,
			Plugin::TransitionType effectType = Plugin::TransitionType_None)
		{
			fade = useFadeOut;
			fade_time_ms = fadeoutTimeMs;
			transition_type = effectType;
		}

		~MenuCloseParam()
		{

		}
	};

	/**
	 * Generic application menu. All other menus must inherit from this class
	 */
	class GenericMenu
	{
	public:

		/**
		 * @param[in] name - Menu template name. Must match name used for the template in the plugin XML
		 * @param[in] oparam - Menu open parameters
		 * @param[in] cparam - Menu close parameters
		 */
		GenericMenu(const char *, MenuOpenParam const& oparam, MenuCloseParam const& cparam);

		virtual ~GenericMenu();

		/**
		 * Activates the menu, enabling rendering and input for it
		 */
		virtual void Activate();

		/**
		 * Deactivates the menu, disabling rendering and input for it
		 *
		 * @param[in] withDelay - When set to true, deactivation will be delayed by 200 milliseconds
		 */
		virtual void Deactivate(bool withDelay = false);

		/**
		 * Disables user input for all menu elements
		 */
		virtual void DisableInput();

		/**
		 * Enables user input for all menu elements
		 */
		virtual void EnableInput();

		/**
		 * Get menu type
		 *
		 * @return menu type
		 */
		virtual MenuType GetMenuType() = 0;

		/**
		 * Get supported settings menu items.
		 *
		 * @param[out] count - Number of items in the hash array
		 *
		 * @return pointer to the array of hashes of the names of supported settings items
		 */
		virtual const uint32_t *GetSupportedSettingsItems(int32_t *count) = 0;

		/**
		 * Get menu root widget
		 *
		 * @return menu root scene widget
		 */
		ui::Scene *GetRoot() const;

	protected:

		ui::Scene *m_root;				// Menu root scene widget

	private:

		MenuCloseParam m_closeParam;	// Close parameters
	};

	/**
	 * Initializes menu system
	 */
	void InitMenuSystem();

	/**
	 * Terminates menu system
	 */
	void TermMenuSystem();

	/**
	 * Get total count of currently opened menus
	 *
	 * @return count of currently opened menus
	 */
	uint32_t GetMenuCount();

	/**
	 * Get top menu
	 *
	 * @return pointer to the Menu object at the top of the menu stack
	 */
	menu::GenericMenu *GetTopMenu();

	/**
	 * Get menu from the menu stack of currently opened menus
	 *
	 * @param[in] idx - Index of the menu in the menu stack
	 *
	 * @return pointer to the Menu object, or NULL if idx is invalid
	 */
	menu::GenericMenu *GetMenuAt(uint32_t idx);

	/**
	 * Deactivate all opened menus, up to the margin.
	 * Menus are deactivated starting from the top of the menu stack, with endMargin number
	 * of menus remaining active at the bottom of the stack
	 *
	 * @param[in] endMargin - indicates how many menus from the bottom of the menu stack should be left active
	 */
	void DeactivateAll(uint32_t endMargin = 0);

	/**
	 * Activate all opened menus
	 */
	void ActivateAll();
}


#endif
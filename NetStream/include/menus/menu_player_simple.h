#ifndef _MENU_PLAYER_SIMPLE_H_
#define _MENU_PLAYER_SIMPLE_H_

#include <kernel.h>
#include <paf.h>

#include "player_generic.h"
#include "menu_generic.h"
#include "menu_youtube.h"

using namespace paf;

namespace menu {
	class PlayerSimple : public GenericMenu
	{
	public:

		enum
		{
			PlayerSimpleEvent = (ui::Handler::CB_STATE + 0x10000),
		};

		enum PlayerEvent
		{
			PlayerEvent_InitFail,
			PlayerEvent_InitOk,
			PlayerEvent_Back
		};

		enum AccJumpState
		{
			AccJumpState_None,
			AccJumpState_Accumulate,
			AccJumpState_Perform
		};

		enum SettingsOverride
		{
			SettingsOverride_None,
			SettingsOverride_YouTube
		};

		class PadListener : public inputdevice::InputListener
		{
		public:

			PadListener(PlayerSimple *parent) : InputListener(inputdevice::DEVICE_TYPE_PAD), m_parent(parent)
			{

			}

			void OnUpdate(inputdevice::Data *data)
			{
				m_parent->OnPadUpdate(data);
			}

		private:

			PlayerSimple *m_parent;
		};

		typedef void(*PlayerSimpleCallback)(PlayerSimple *player, void *pUserArg);

		PlayerSimple(const char *url);

		~PlayerSimple();

		GenericPlayer *GetPlayer();

		void SetScale(float scale);

		float GetScale();

		void SetPosition(float x, float y);

		void SetSettingsOverride(SettingsOverride override);

		MenuType GetMenuType()
		{
			return MenuType_PlayerSimple;
		}

		const uint32_t *GetSupportedSettingsItems(int32_t *count)
		{
			switch (m_settingsOverride)
			{
			case SettingsOverride_None:
				*count = 0;
				return nullptr;
			case SettingsOverride_YouTube:
				*count = sizeof(k_settingsIdListYoutubeOverride) / sizeof(char*);
				return k_settingsIdListYoutubeOverride;
			}

			return nullptr;
		}

	private:

		static int32_t PowerCallback(SceUID notifyId, int32_t notifyCount, int32_t notifyArg, void *pCommon);
		static void UpdateTask(void *pArgBlock);
		void OnVideoPlaneTouch();
		void OnProgressBarStateChange(int32_t type);
		void OnBackButton();
		void OnPlayButton();
		void OnWholeRepeatButton();
		void OnPowerCallback(int32_t notifyArg);
		void OnGenericPlayerStateChange(int32_t state);
		void OnUpdate();
		void OnPadUpdate(inputdevice::Data *data);

		ui::Widget *m_videoPlane;
		ui::BusyIndicator *m_loadIndicator;
		ui::Widget *m_leftAccText;
		ui::Widget *m_rightAccText;
		ui::Widget *m_statPlane;
		ui::Widget *m_backButton;
		ui::Widget *m_wholeRepeatButton;
		ui::Text *elapsedTimeText;
		ui::ProgressBarTouch *m_progressBar;
		ui::Widget *m_progressPlane;
		ui::Widget *m_playButton;
		uint32_t m_oldCurrTime;
		bool m_progressPlaneShown;
		uint32_t m_progressPlaneShownTime;
		bool m_isSeeking;
		bool m_isLS;
		uint32_t m_oldButtons;
		GenericPlayer::PlayerState m_oldState;
		int32_t m_accJumpTime;
		uint32_t m_accStartTime;
		AccJumpState m_accJumpState;
		float m_currentScale;
		SettingsOverride m_settingsOverride;
		SceUID m_pwCbId;
		common::SharedPtr<inputdevice::InputListener> m_padListener;
		GenericPlayer *m_player;

		const uint32_t k_settingsIdListYoutubeOverride[4] = {
			youtube_search_setting,
			youtube_comment_setting,
			youtube_quality_setting,
			youtube_player_setting
		};
	};
}

#endif

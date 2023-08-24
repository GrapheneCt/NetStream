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

			PadListener(PlayerSimple *work) : InputListener(inputdevice::DEVICE_TYPE_PAD), workObj(work)
			{

			}

			void OnUpdate(inputdevice::Data *data)
			{
				workObj->PadUpdate(data);
			}

		private:

			PlayerSimple *workObj;
		};

		typedef void(*PlayerSimpleCallback)(PlayerSimple *player, void *pUserArg);

		static void VideoPlaneCbFun(int32_t type, ui::Handler *self, ui::Event *e, void *userdata);
		static void ProgressBarCbFun(int32_t type, ui::Handler *self, ui::Event *e, void *userdata);
		static void BackButtonCbFun(int32_t type, ui::Handler *self, ui::Event *e, void *userdata);
		static void PlayButtonCbFun(int32_t type, ui::Handler *self, ui::Event *e, void *userdata);
		static void WholeRepeatButtonCbFun(int32_t type, ui::Handler *self, ui::Event *e, void *userdata);
		static void GenericPlayerStateCbFun(int32_t type, ui::Handler *self, ui::Event *e, void *userdata);
		static int32_t PowerCallback(SceUID notifyId, int32_t notifyCount, int32_t notifyArg, void *pCommon);

		PlayerSimple(const char *url);

		~PlayerSimple();

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
			switch (settingsOverride)
			{
			case SettingsOverride_None:
				*count = 0;
				return NULL;
			case SettingsOverride_YouTube:
				*count = sizeof(k_settingsIdListYoutubeOverride) / sizeof(char*);
				return k_settingsIdListYoutubeOverride;
			}

			return NULL;
		}

		static void UpdateTask(void *pArgBlock);
		void PadUpdate(inputdevice::Data *data);

		ui::Widget *videoPlane;
		ui::BusyIndicator *loadIndicator;
		ui::Widget *leftAccText;
		ui::Widget *rightAccText;
		ui::Widget *statPlane;
		ui::Widget *backButton;
		ui::Widget *wholeRepeatButton;
		ui::Text *elapsedTimeText;
		ui::ProgressBarTouch *progressBar;
		ui::Widget *progressPlane;
		ui::Widget *playButton;
		uint32_t oldCurrTime;
		bool progressPlaneShown;
		uint32_t progressPlaneShownTime;
		bool isSeeking;
		bool isLS;
		uint32_t oldButtons;
		GenericPlayer::PlayerState oldState;
		int32_t accJumpTime;
		uint32_t accStartTime;
		AccJumpState accJumpState;
		float currentScale;
		SettingsOverride settingsOverride;
		SceUID pwCbId;
		common::SharedPtr<inputdevice::InputListener> padListener;

		GenericPlayer *player;

		const uint32_t k_settingsIdListYoutubeOverride[4] = {
			youtube_search_setting,
			youtube_comment_setting,
			youtube_quality_setting,
			youtube_player_setting
		};
	};
}

#endif

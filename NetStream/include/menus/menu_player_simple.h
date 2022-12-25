#ifndef _MENU_PLAYER_SIMPLE_H_
#define _MENU_PLAYER_SIMPLE_H_

#include <kernel.h>
#include <paf.h>

#include "beav_player.h"
#include "menu_generic.h"
#include "menu_youtube.h"

using namespace paf;

namespace menu {
	class PlayerSimple : public GenericMenu
	{
	public:

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

		typedef void(*PlayerSimpleCallback)(PlayerSimple *player, ScePVoid pUserArg);

		static SceVoid VideoPlaneCbFun(SceInt32 eventId, ui::Widget *self, SceInt32 a3, ScePVoid pUserData);
		static SceVoid ProgressBarCbFun(SceInt32 eventId, ui::Widget *self, SceInt32 a3, ScePVoid pUserData);
		static SceVoid BackButtonCbFun(SceInt32 eventId, ui::Widget *self, SceInt32 a3, ScePVoid pUserData);
		static SceVoid PlayButtonCbFun(SceInt32 eventId, ui::Widget *self, SceInt32 a3, ScePVoid pUserData);
		static SceVoid WholeRepeatButtonCbFun(SceInt32 eventId, ui::Widget *self, SceInt32 a3, ScePVoid pUserData);
		static SceInt32 PowerCallback(SceUID notifyId, SceInt32 notifyCount, SceInt32 notifyArg, void *pCommon);

		PlayerSimple(const char *url, PlayerSimpleCallback okCb = SCE_NULL, PlayerSimpleCallback failCb = SCE_NULL, PlayerSimpleCallback backCb = SCE_NULL, ScePVoid cbArg = SCE_NULL);

		~PlayerSimple();

		SceVoid SetScale(SceFloat32 scale);

		SceFloat32 GetScale();

		SceVoid SetPosition(SceFloat32 x, SceFloat32 y);

		SceVoid SetSettingsOverride(SettingsOverride override);

		SceVoid SetBackButtonCb(PlayerSimpleCallback backCb);

		MenuType GetMenuType()
		{
			return MenuType_PlayerSimple;
		}

		const SceUInt32 *GetSupportedSettingsItems(SceInt32 *count)
		{
			switch (settingsOverride)
			{
			case SettingsOverride_None:
				*count = 0;
				return SCE_NULL;
			case SettingsOverride_YouTube:
				*count = sizeof(k_settingsIdListYoutubeOverride) / sizeof(char*);
				return k_settingsIdListYoutubeOverride;
			}

			return SCE_NULL;
		}

		static SceVoid UpdateTask(ScePVoid pArgBlock);

		static SceVoid StateCheckTask(ScePVoid pArgBlock);

		static SceVoid DirectInputCallback(input::GamePad::GamePadData *pData);

		ui::Widget *videoPlane;
		ui::BusyIndicator *loadIndicator;
		ui::Widget *leftAccText;
		ui::Widget *rightAccText;
		ui::Widget *backButton;
		ui::Widget *wholeRepeatButton;
		ui::Text *elapsedTimeText;
		ui::ProgressBarTouch *progressBar;
		ui::Widget *progressPlane;
		ui::Widget *playButton;
		SceUInt32 oldCurrTime;
		SceBool progressPlaneShown;
		SceUInt32 progressPlaneShownTime;
		SceBool isSeeking;
		SceBool isLS;
		SceUInt32 oldButtons;
		SceInt32 accJumpTime;
		SceUInt32 accStartTime;
		AccJumpState accJumpState;
		SceFloat32 currentScale;
		PlayerSimpleCallback initOkCb;
		PlayerSimpleCallback initFailCb;
		PlayerSimpleCallback backButtonCb;
		ScePVoid cbUserArg;
		SettingsOverride settingsOverride;
		SceUID pwCbId;

		BEAVPlayer *player;

		const SceUInt32 k_settingsIdListYoutubeOverride[5] = {
			youtube_search_setting,
			youtube_comment_setting,
			youtube_quality_setting,
			youtube_player_setting,
			youtube_download_setting
		};
	};
}

#endif

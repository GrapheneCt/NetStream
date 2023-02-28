#include <kernel.h>
#include <power.h>
#include <paf.h>
#include <libdbg.h>

#include "common.h"
#include "utils.h"
#include "dialog.h"
#include "beav_player.h"
#include "menus/menu_player_simple.h"
#include "menus/menu_settings.h"

using namespace paf;

static menu::PlayerSimple *s_instance;

SceVoid menu::PlayerSimple::BackButtonCbFun(SceInt32 eventId, ui::Widget *self, SceInt32 a3, ScePVoid pUserData)
{
	PlayerSimple *workObj = (PlayerSimple *)pUserData;
	if (workObj->backButtonCb)
	{
		workObj->backButtonCb(workObj, workObj->cbUserArg);
	}
}

SceVoid  menu::PlayerSimple::VideoPlaneCbFun(SceInt32 eventId, ui::Widget *self, SceInt32 a3, ScePVoid pUserData)
{
	PlayerSimple *workObj = (PlayerSimple *)pUserData;

	if (workObj->progressPlaneShown)
	{
		if (!workObj->isLS)
		{
			workObj->progressPlane->PlayEffectReverse(0.0f, effect::EffectType_SlideFromBottom1);
		}
		workObj->backButton->PlayEffectReverse(0.0f, effect::EffectType_Reset);
		sceAppMgrSetInfobarState(SCE_APPMGR_INFOBAR_VISIBILITY_INVISIBLE, SCE_APPMGR_INFOBAR_COLOR_BLACK, SCE_APPMGR_INFOBAR_TRANSPARENCY_TRANSLUCENT);
		workObj->progressPlaneShown = SCE_FALSE;
	}
	else
	{
		if (!workObj->isLS)
		{
			workObj->progressPlane->PlayEffect(0.0f, effect::EffectType_SlideFromBottom1);
		}
		if (workObj->currentScale == 1.0f)
		{
			workObj->backButton->PlayEffect(0.0f, effect::EffectType_Reset);
		}
		workObj->progressPlaneShownTime = sceKernelGetProcessTimeLow();
		sceAppMgrSetInfobarState(SCE_APPMGR_INFOBAR_VISIBILITY_VISIBLE, SCE_APPMGR_INFOBAR_COLOR_BLACK, SCE_APPMGR_INFOBAR_TRANSPARENCY_TRANSLUCENT);
		workObj->progressPlaneShown = SCE_TRUE;
	}
}

SceVoid  menu::PlayerSimple::ProgressBarCbFun(SceInt32 eventId, ui::Widget *self, SceInt32 a3, ScePVoid pUserData)
{
	PlayerSimple *workObj = (PlayerSimple *)pUserData;
	ui::ProgressBarTouch *bar = (ui::ProgressBarTouch *)self;

	if (eventId == ui::EventMain_Tapped)
	{
		SceUInt32 val = (SceUInt32)(bar->currentValue / 100.0f * (SceFloat32)workObj->player->GetTotalTimeMs());
		workObj->player->JumpToTimeMs(val);
		workObj->isSeeking = SCE_FALSE;
	}
	else if (eventId == 0x1000D || eventId == 0x1000F)
	{
		workObj->isSeeking = SCE_TRUE;
	}
}

SceVoid menu::PlayerSimple::PlayButtonCbFun(SceInt32 eventId, ui::Widget *self, SceInt32 a3, ScePVoid pUserData)
{
	PlayerSimple *workObj = (PlayerSimple *)pUserData;
	rco::Element searchParam;
	graph::Surface *tex = SCE_NULL;
	
	workObj->player->SwitchPlaybackState();

	if (workObj->player->IsPaused())
	{
		tex = utils::GetTexture(tex_button_play);
	}
	else
	{
		tex = utils::GetTexture(tex_button_pause);
	}

	workObj->playButton->SetSurfaceBase(&tex);
}

SceVoid menu::PlayerSimple::WholeRepeatButtonCbFun(SceInt32 eventId, ui::Widget *self, SceInt32 a3, ScePVoid pUserData)
{
	PlayerSimple *workObj = (PlayerSimple *)pUserData;
	workObj->player->JumpToTimeMs(0);
	Rgba col(1.0f, 1.0f, 1.0f, 1.0f);
	workObj->videoPlane->SetColor(col);
	workObj->wholeRepeatButton->PlayEffectReverse(0.0f, effect::EffectType_Fadein1);
}

SceInt32 menu::PlayerSimple::PowerCallback(SceUID notifyId, SceInt32 notifyCount, SceInt32 notifyArg, void *pCommon)
{
	PlayerSimple *workObj = (PlayerSimple *)pCommon;

	if (notifyArg & SCE_POWER_CALLBACKARG_RESERVED_22)
	{
		workObj->player->SetPowerSaving(SCE_TRUE);
	}
	else if (notifyArg & SCE_POWER_CALLBACKARG_RESERVED_23)
	{
		workObj->player->SetPowerSaving(SCE_FALSE);
	}

	return SCE_OK;
}

SceVoid menu::PlayerSimple::DirectInputCallback(input::GamePad::GamePadData *pData)
{
	if (!s_instance)
		return;

	if ((pData->buttons & SCE_PAF_CTRL_CROSS) && !(s_instance->oldButtons & SCE_PAF_CTRL_CROSS))
	{
		if (s_instance->player->GetState() == SCE_BEAV_CORE_PLAYER_STATE_EOF && !s_instance->isLS)
		{
			WholeRepeatButtonCbFun(0, SCE_NULL, 0, s_instance);
		}
		else
		{
			PlayButtonCbFun(0, SCE_NULL, 0, s_instance);
		}
	}
	else if ((pData->buttons & SCE_PAF_CTRL_CIRCLE) && !(s_instance->oldButtons & SCE_PAF_CTRL_CIRCLE) && !s_instance->progressPlaneShown)
	{
		menu::PlayerSimple::BackButtonCbFun(0, SCE_NULL, 0, s_instance);
		return;
	}
	else if ((((pData->buttons & SCE_PAF_CTRL_RIGHT) && !(s_instance->oldButtons & SCE_PAF_CTRL_RIGHT)) ||
		((pData->buttons & SCE_PAF_CTRL_LEFT) && !(s_instance->oldButtons & SCE_PAF_CTRL_LEFT)) ||
		((pData->buttons & SCE_PAF_CTRL_R) && !(s_instance->oldButtons & SCE_PAF_CTRL_R)) ||
		((pData->buttons & SCE_PAF_CTRL_L) && !(s_instance->oldButtons & SCE_PAF_CTRL_L))) &&
		!s_instance->isLS)
	{
		Rgba col(0.4f, 0.4f, 0.4f, 1.0f);
		s_instance->videoPlane->SetColor(col);

		if ((pData->buttons & SCE_PAF_CTRL_RIGHT) && !(s_instance->oldButtons & SCE_PAF_CTRL_RIGHT))
		{
			s_instance->accJumpTime += 5000;
		}
		else if ((pData->buttons & SCE_PAF_CTRL_LEFT) && !(s_instance->oldButtons & SCE_PAF_CTRL_LEFT))
		{
			s_instance->accJumpTime -= 5000;
		}
		else if ((pData->buttons & SCE_PAF_CTRL_R) && !(s_instance->oldButtons & SCE_PAF_CTRL_R))
		{
			s_instance->accJumpTime += (SceInt32)((SceFloat32)s_instance->player->GetTotalTimeMs() * 0.05f);
		}
		else if ((pData->buttons & SCE_PAF_CTRL_L) && !(s_instance->oldButtons & SCE_PAF_CTRL_L))
		{
			s_instance->accJumpTime -= (SceInt32)((SceFloat32)s_instance->player->GetTotalTimeMs() * 0.05f);
		}

		string text8;
		wstring text16;
		utils::ConvertSecondsToString(text8, (SceUInt32)(sce_paf_abs(s_instance->accJumpTime) / 1000), SCE_FALSE);
		common::Utf8ToUtf16(text8, &text16);
		if (s_instance->accJumpTime < 0)
		{
			s_instance->leftAccText->SetLabel(&text16);
		}
		else if (s_instance->accJumpTime > 0)
		{
			s_instance->rightAccText->SetLabel(&text16);
		}

		s_instance->accStartTime = sceKernelGetProcessTimeLow();
		s_instance->accJumpState = AccJumpState_Accumulate;
	}
	else if ((pData->buttons & SCE_PAF_CTRL_START) && !(s_instance->oldButtons & SCE_PAF_CTRL_START))
	{
		scePowerRequestDisplayOff();
	}
	/*
	else if ((((pData->buttons & SCE_PAF_CTRL_UP) && !(s_instance->oldButtons & SCE_PAF_CTRL_UP)) ||
		((pData->buttons & SCE_PAF_CTRL_DOWN) && !(s_instance->oldButtons & SCE_PAF_CTRL_DOWN))) &&
		!s_instance->isLS)
	{
		Rgba col(0.4f, 0.4f, 0.4f, 1.0f);
		s_instance->videoPlane->SetFilterColor(&col, 0.0f);

		if ((pData->buttons & SCE_PAF_CTRL_UP) && !(s_instance->oldButtons & SCE_PAF_CTRL_UP))
		{
		}
		else if ((pData->buttons & SCE_PAF_CTRL_DOWN) && !(s_instance->oldButtons & SCE_PAF_CTRL_DOWN))
		{
		}
	}
	*/

	s_instance->oldButtons = pData->buttons;
}

SceVoid menu::PlayerSimple::UpdateTask(ScePVoid pArgBlock)
{
	PlayerSimple *workObj = (PlayerSimple *)pArgBlock;
	string text8;
	wstring text16;

	if (!workObj->isLS)
	{
		if (!workObj->isSeeking)
		{
			SceUInt32 currTime = workObj->player->GetCurrentTimeMs() / 1000;
			if (currTime != workObj->oldCurrTime)
			{
				utils::ConvertSecondsToString(text8, currTime, SCE_FALSE);
				common::Utf8ToUtf16(text8, &text16);
				workObj->elapsedTimeText->SetLabel(&text16);
				SceFloat32 progress = (SceFloat32)currTime * 100000.0f / (SceFloat32)workObj->player->GetTotalTimeMs();
				workObj->progressBar->SetProgress(progress, 0, 0);
				workObj->oldCurrTime = currTime;
			}
		}
		else
		{
			SceUInt32 val = (SceUInt32)(workObj->progressBar->currentValue / 100000.0f * (SceFloat32)workObj->player->GetTotalTimeMs());
			utils::ConvertSecondsToString(text8, val, SCE_FALSE);
			common::Utf8ToUtf16(text8, &text16);
			workObj->elapsedTimeText->SetLabel(&text16);
		}

		if (workObj->accJumpState == AccJumpState_Perform)
		{
			Rgba col(1.0f, 1.0f, 1.0f, 1.0f);
			s_instance->videoPlane->SetColor(col);
			text16 = L"";
			s_instance->leftAccText->SetLabel(&text16);
			s_instance->rightAccText->SetLabel(&text16);
			workObj->player->JumpToTimeMs(workObj->player->GetCurrentTimeMs() + workObj->accJumpTime);
			workObj->accJumpTime = 0;
			workObj->accJumpState = AccJumpState_None;
		}
		else if (workObj->accJumpState == AccJumpState_Accumulate)
		{
			if ((sceKernelGetProcessTimeLow() - workObj->accStartTime) > 500000)
			{
				workObj->accJumpState = AccJumpState_Perform;
			}
		}
	}

	if (workObj->progressPlaneShown)
	{
		if (workObj->isSeeking || workObj->player->IsPaused() || workObj->currentScale != 1.0f)
		{
			workObj->progressPlaneShownTime = sceKernelGetProcessTimeLow();
		}
		else if ((sceKernelGetProcessTimeLow() - workObj->progressPlaneShownTime) > 5000000)
		{
			if (!workObj->isLS)
			{
				workObj->progressPlane->PlayEffectReverse(0.0f, effect::EffectType_SlideFromBottom1);
			}
			workObj->backButton->PlayEffectReverse(0.0f, effect::EffectType_Reset);
			sceAppMgrSetInfobarState(SCE_APPMGR_INFOBAR_VISIBILITY_INVISIBLE, SCE_APPMGR_INFOBAR_COLOR_BLACK, SCE_APPMGR_INFOBAR_TRANSPARENCY_TRANSLUCENT);
			workObj->progressPlaneShown = SCE_FALSE;
		}
	}

	SceBeavCorePlayerState state = workObj->player->GetState();
	if (!workObj->isLS && state == SCE_BEAV_CORE_PLAYER_STATE_EOF)
	{
		Rgba col(0.4f, 0.4f, 0.4f, 1.0f);
		workObj->videoPlane->SetColor(col);
		workObj->wholeRepeatButton->PlayEffect(0.0f, effect::EffectType_Fadein1);
	}
}

SceVoid menu::PlayerSimple::StateCheckTask(ScePVoid pArgBlock)
{
	PlayerSimple *workObj = (PlayerSimple *)pArgBlock;
	string text8;
	wstring text16;

	BEAVPlayer::InitState state = workObj->player->GetInitState();

	if (state == BEAVPlayer::InitState_InitOk)
	{
		common::MainThreadCallList::Unregister(StateCheckTask, pArgBlock);
		workObj->loadIndicator->Stop();

		SceUInt32 totalTime = workObj->player->GetTotalTimeMs();
		ui::Widget *totalTimeText = utils::GetChild(workObj->root, text_video_control_panel_progressbar_label_total);
		if (totalTime > 0)
		{
			utils::ConvertSecondsToString(text8, totalTime / 1000, SCE_FALSE);
			common::Utf8ToUtf16(text8, &text16);
			totalTimeText->SetLabel(&text16);

			workObj->progressBar->RegisterEventCallback(ui::EventMain_Tapped, new utils::SimpleEventCallback(ProgressBarCbFun, pArgBlock));
			workObj->progressBar->RegisterEventCallback(0x1000D, new utils::SimpleEventCallback(ProgressBarCbFun, pArgBlock));
			workObj->progressBar->RegisterEventCallback(0x1000F, new utils::SimpleEventCallback(ProgressBarCbFun, pArgBlock));
		}
		else
		{
			workObj->isLS = SCE_TRUE;
		}

		input::GamePad::RegisterCallback(DirectInputCallback);

		ui::Widget *videoPlane = utils::GetChild(workObj->root, button_video_page_control_trigger);
		videoPlane->RegisterEventCallback(ui::EventMain_Decide, new utils::SimpleEventCallback(VideoPlaneCbFun, pArgBlock));
		videoPlane->SetDirectKey(SCE_PAF_CTRL_TRIANGLE);

		common::MainThreadCallList::Register(UpdateTask, pArgBlock);

		if (workObj->initOkCb)
		{
			workObj->initOkCb(workObj, workObj->cbUserArg);
		}
	}
	else if (state == BEAVPlayer::InitState_InitFail)
	{
		common::MainThreadCallList::Unregister(StateCheckTask, pArgBlock);

		if (workObj->initFailCb)
		{
			workObj->initFailCb(workObj, workObj->cbUserArg);
		}
	}
}

menu::PlayerSimple::PlayerSimple(const char *url, PlayerSimpleCallback okCb, PlayerSimpleCallback failCb, PlayerSimpleCallback backCb, ScePVoid cbArg) :
	GenericMenu("page_player_simple",
	MenuOpenParam(true, 200.0f, Plugin::PageEffectType_None, ui::Context::Option::Flag_ResolutionFullHd),
	MenuCloseParam(true, 200.0f, Plugin::PageEffectType_None))
{
	rco::Element searchParam;
	Plugin::TemplateOpenParam tmpParam;
	oldButtons = 0;
	accJumpTime = 0;
	accStartTime = 0;
	accJumpState = AccJumpState_None;
	progressPlaneShown = SCE_FALSE;
	isLS = SCE_FALSE;
	isSeeking = SCE_FALSE;
	currentScale = 1.0f;
	initOkCb = okCb;
	initFailCb = failCb;
	backButtonCb = backCb;
	cbUserArg = cbArg;
	settingsOverride = SettingsOverride_None;

	if (s_instance)
	{
		SCE_DBG_LOG_ERROR("[MENU] Attempt to create second singleton instance\n");
		return;
	}

	progressBar = (ui::ProgressBarTouch *)utils::GetChild(root, progressbar_touch_video_control_panel);
	elapsedTimeText = (ui::Text *)utils::GetChild(root, text_video_control_panel_progressbar_label_elapsed);
	leftAccText = utils::GetChild(root, text_video_page_player_simple_acc_left);
	rightAccText = utils::GetChild(root, text_video_page_player_simple_acc_right);
	progressPlane = utils::GetChild(root, plane_video_control_panel_bg);
	progressPlane->PlayEffectReverse(0.0f, effect::EffectType_Fadein1);

	loadIndicator = (ui::BusyIndicator *)utils::GetChild(root, busyindicator_video_page_player_simple);
	if (SCE_PAF_IS_DOLCE)
	{
		loadIndicator->SetBallSize(32.0f);
	}
	loadIndicator->Start();

	backButton = utils::GetChild(root, button_back_page_player_simple);
	backButton->PlayEffectReverse(0.0f, effect::EffectType_Reset);
	backButton->RegisterEventCallback(ui::EventMain_Decide, new utils::SimpleEventCallback(BackButtonCbFun, this));

	wholeRepeatButton = utils::GetChild(root, button_video_page_whole_repeat);
	wholeRepeatButton->PlayEffectReverse(0.0f, effect::EffectType_Fadein1);
	wholeRepeatButton->RegisterEventCallback(ui::EventMain_Decide, new utils::SimpleEventCallback(WholeRepeatButtonCbFun, this));

	playButton = utils::GetChild(root, button_video_control_panel_playpause);
	playButton->RegisterEventCallback(ui::EventMain_Decide, new utils::SimpleEventCallback(PlayButtonCbFun, this));

	videoPlane = utils::GetChild(root, plane_video_page_player_simple);
	player = new BEAVPlayer(videoPlane, url);
	player->InitAsync();
	common::MainThreadCallList::Register(StateCheckTask, this);

	pwCbId = sceKernelCreateCallback("PowerCallback", 0, PowerCallback, this);
	scePowerRegisterCallback(pwCbId);

	sceAppMgrSetInfobarState(SCE_APPMGR_INFOBAR_VISIBILITY_INVISIBLE, SCE_APPMGR_INFOBAR_COLOR_BLACK, SCE_APPMGR_INFOBAR_TRANSPARENCY_TRANSLUCENT);
	utils::SetPowerTickTask(utils::PowerTick_All);
	menu::GetMenuAt(menu::GetMenuCount() - 2)->Deactivate();

	s_instance = this;
}

menu::PlayerSimple::~PlayerSimple()
{
	common::MainThreadCallList::Unregister(UpdateTask, this);
	scePowerUnregisterCallback(pwCbId);
	sceKernelDeleteCallback(pwCbId);
	input::GamePad::RegisterCallback(SCE_NULL);
	delete player;
	sceAppMgrSetInfobarState(SCE_APPMGR_INFOBAR_VISIBILITY_VISIBLE, SCE_APPMGR_INFOBAR_COLOR_BLACK, SCE_APPMGR_INFOBAR_TRANSPARENCY_TRANSLUCENT);
	utils::SetPowerTickTask(utils::PowerTick_None);
	menu::GetTopMenu()->Activate();
	s_instance = SCE_NULL;
}

SceFloat32 menu::PlayerSimple::GetScale()
{
	return currentScale;
}

SceVoid menu::PlayerSimple::SetScale(SceFloat32 scale)
{
	if (currentScale == scale || scale > 1.0f || scale < 0.0f)
	{
		return;
	}

	Vector4 sz;
	Vector4 pos;
	ui::Text::CharacterSize tsz;
	ui::Widget *triggerPlane = utils::GetChild(root, button_video_page_control_trigger);
	ui::Text *totalTimeText = (ui::Text *)utils::GetChild(root, text_video_control_panel_progressbar_label_total);

	if (SCE_PAF_IS_DOLCE)
	{
		sz.x = 1920.0f * scale;
		sz.y = 1088.0f * scale;
		videoPlane->SetSize(sz);
		sz.x = 150.0f * scale;
		sz.y = 150.0f * scale;
		loadIndicator->SetSize(sz);
		loadIndicator->SetBallSize(32.0f * scale);
		sz.x = 180.0f * scale;
		sz.y = 180.0f * scale;
		wholeRepeatButton->SetSize(sz);
		if (scale == 1.0f)
		{
			sz.x = 1700.0f;
			sz.y = 112.0f;
			progressPlane->SetSize(sz);
			pos.x = 0.0f;
			pos.y = 56.0f;
			progressPlane->SetPosition(pos);
		}
		else
		{
			sz.x = 1920.0f * scale;
			sz.y = 112.0f * scale;
			progressPlane->SetSize(sz);
			pos.x = 0.0f;
			pos.y = 56.0f * scale;
			progressPlane->SetPosition(pos);
		}
		sz.x = 1400.0f * scale;
		sz.y = 20.0f * scale;
		progressBar->SetSize(sz);
		sz.x = 90.0f * scale;
		sz.y = 80.0f * scale;
		playButton->SetSize(sz);
		pos.x = 30.0f * scale;
		pos.y = 0.0f;
		playButton->SetPosition(pos);
		pos.x = -20.0f * scale;
		if (scale != 1.0f)
		{
			pos.x -= 20.0f * scale;
		}
		pos.y = 20.0f * scale;
		elapsedTimeText->SetPosition(pos);
		pos.y = -pos.y;
		totalTimeText->SetPosition(pos);
	}
	else
	{
		sz.x = 960.0f * scale;
		sz.y = 544.0f * scale;
		videoPlane->SetSize(sz);
		sz.x = 75.0f * scale;
		sz.y = 75.0f * scale;
		loadIndicator->SetSize(sz);
		loadIndicator->SetBallSize(16.0f * scale);
		sz.x = 90.0f * scale;
		sz.y = 90.0f * scale;
		wholeRepeatButton->SetSize(sz);
		if (scale == 1.0f)
		{
			sz.x = 790.0f;
			sz.y = 56.0f;
			progressPlane->SetSize(sz);
			pos.x = 0.0f;
			pos.y = 28.0f;
			progressPlane->SetPosition(pos);
		}
		else
		{
			sz.x = 960.0f * scale;
			sz.y = 112.0f * scale;
			progressPlane->SetSize(sz);
			pos.x = 0.0f;
			pos.y = 56.0f * scale;
			progressPlane->SetPosition(pos);
		}
		sz.x = 538.0f * scale;
		sz.y = 10.0f * scale;
		progressBar->SetSize(sz);
		/*
		pos.y = 0.0f;
		pos.x = 14.0f * scale;
		progressBar->SetPosition(&pos);
		*/
		sz.x = 48.0f * scale;
		sz.y = 80.0f * scale;
		playButton->SetSize(sz);
		pos.x = 32.0f * scale;
		pos.y = 0.0f;
		playButton->SetPosition(pos);
		pos.x = -20.0f * scale;
		if (scale != 1.0f)
		{
			pos.x -= 20.0f * scale;
		}
		if (scale != 1.0f)
		{
			pos.y = 20.0f * scale;
		}
		else
		{
			pos.y = 12.0f * scale;
		}
		elapsedTimeText->SetPosition(pos);
		pos.y = -pos.y;
		totalTimeText->SetPosition(pos);
	}

	if (scale != 1.0f)
	{
		tsz.width = 16.0f;
	}
	else
	{
		tsz.width = 20.0f;
	}

	tsz.height = tsz.width;
	elapsedTimeText->SetCharacterSize(ui::Text::CharacterType_Text, 0, 0, &tsz);
	totalTimeText->SetCharacterSize(ui::Text::CharacterType_Text, 0, 0, &tsz);

	if (scale == 1.0f)
	{
		input::GamePad::RegisterCallback(DirectInputCallback);
		sceAppMgrSetInfobarState(SCE_APPMGR_INFOBAR_VISIBILITY_INVISIBLE, SCE_APPMGR_INFOBAR_COLOR_BLACK, SCE_APPMGR_INFOBAR_TRANSPARENCY_TRANSLUCENT);
		backButton->PlayEffect(0.0f, effect::EffectType_Reset);
		triggerPlane->PlayEffect(0.0f, effect::EffectType_Fadein1);
		menu::GetMenuAt(menu::GetMenuCount() - 2)->Deactivate();
	}
	else
	{
		input::GamePad::RegisterCallback(SCE_NULL);
		sceAppMgrSetInfobarState(SCE_APPMGR_INFOBAR_VISIBILITY_VISIBLE, SCE_APPMGR_INFOBAR_COLOR_BLACK, SCE_APPMGR_INFOBAR_TRANSPARENCY_TRANSLUCENT);
		backButton->PlayEffectReverse(0.0f, effect::EffectType_Reset);
		triggerPlane->PlayEffectReverse(0.0f, effect::EffectType_Fadein1);
		if (!isLS)
		{
			progressPlane->PlayEffect(0.0f, effect::EffectType_Fadein1);
		}
		progressPlaneShown = SCE_TRUE;
		menu::GetMenuAt(menu::GetMenuCount() - 2)->Activate();
	}

	currentScale = scale;
}

SceVoid menu::PlayerSimple::SetPosition(SceFloat32 x, SceFloat32 y)
{
	Vector4 pos(x, y);
	videoPlane->SetPosition(pos);
}

SceVoid menu::PlayerSimple::SetSettingsOverride(SettingsOverride override)
{
	settingsOverride = override;
}

SceVoid menu::PlayerSimple::SetBackButtonCb(PlayerSimpleCallback backCb)
{
	backButtonCb = backCb;
}
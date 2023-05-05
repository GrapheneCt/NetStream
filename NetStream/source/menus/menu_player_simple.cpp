#include <kernel.h>
#include <power.h>
#include <paf.h>
#include <libdbg.h>

#include "common.h"
#include "utils.h"
#include "event.h"
#include "dialog.h"
#include "player_beav.h"
#include "player_fmod.h"
#include "menus/menu_player_simple.h"
#include "menus/menu_settings.h"

#include <vectormath.h>

using namespace paf;

static menu::PlayerSimple *s_instance;

void menu::PlayerSimple::BackButtonCbFun(int32_t type, ui::Handler *self, ui::Event *e, void *userdata)
{
	event::BroadcastGlobalEvent(g_appPlugin, PlayerSimpleEvent, PlayerEvent_Back);
}

void  menu::PlayerSimple::VideoPlaneCbFun(int32_t type, ui::Handler *self, ui::Event *e, void *userdata)
{
	PlayerSimple *workObj = (PlayerSimple *)userdata;

	if (workObj->progressPlaneShown)
	{
		if (!workObj->isLS)
		{
			workObj->progressPlane->Hide(common::transition::Type_SlideFromBottom1);
		}
		workObj->backButton->Hide(common::transition::Type_Reset);
		sceAppMgrSetInfobarState(SCE_APPMGR_INFOBAR_VISIBILITY_INVISIBLE, SCE_APPMGR_INFOBAR_COLOR_BLACK, SCE_APPMGR_INFOBAR_TRANSPARENCY_TRANSLUCENT);
		workObj->progressPlaneShown = false;
	}
	else
	{
		if (!workObj->isLS)
		{
			workObj->progressPlane->Show(common::transition::Type_SlideFromBottom1);
		}
		if (workObj->currentScale == 1.0f)
		{
			workObj->backButton->Show(common::transition::Type_Reset);
		}
		workObj->progressPlaneShownTime = sceKernelGetProcessTimeLow();
		sceAppMgrSetInfobarState(SCE_APPMGR_INFOBAR_VISIBILITY_VISIBLE, SCE_APPMGR_INFOBAR_COLOR_BLACK, SCE_APPMGR_INFOBAR_TRANSPARENCY_TRANSLUCENT);
		workObj->progressPlaneShown = true;
	}
}

void  menu::PlayerSimple::ProgressBarCbFun(int32_t type, ui::Handler *self, ui::Event *e, void *userdata)
{
	PlayerSimple *workObj = (PlayerSimple *)userdata;
	ui::ProgressBarTouch *bar = (ui::ProgressBarTouch *)self;

	if (type == ui::ProgressBarTouch::CB_PROGRESSBAR_DRAG_END)
	{
		uint32_t val = (uint32_t)(bar->GetValue() / 100.0f * (float)workObj->player->GetTotalTimeMs());
		workObj->player->JumpToTimeMs(val);
		workObj->isSeeking = false;
	}
	else if (type == ui::Handler::CB_TOUCH_MOVE || type == ui::Handler::CB_TOUCH_OUT)
	{
		workObj->isSeeking = true;
	}
}

void menu::PlayerSimple::PlayButtonCbFun(int32_t type, ui::Handler *self, ui::Event *e, void *userdata)
{
	PlayerSimple *workObj = (PlayerSimple *)userdata;
	uint32_t texid = 0;
	
	workObj->player->SwitchPlaybackState();

	if (workObj->player->IsPaused())
	{
		texid = tex_button_play;
	}
	else
	{
		texid = tex_button_pause;
	}

	intrusive_ptr<graph::Surface> tex = g_appPlugin->GetTexture(texid);
	if (tex.get())
	{
		workObj->playButton->SetTexture(tex);
	}
}

void menu::PlayerSimple::WholeRepeatButtonCbFun(int32_t type, ui::Handler *self, ui::Event *e, void *userdata)
{
	PlayerSimple *workObj = (PlayerSimple *)userdata;
	workObj->player->JumpToTimeMs(0);
	workObj->player->SwitchPlaybackState();
	math::v4 col(1.0f, 1.0f, 1.0f, 1.0f);
	workObj->videoPlane->SetColor(col);
	workObj->wholeRepeatButton->Hide(common::transition::Type_Fadein1);
}

int32_t menu::PlayerSimple::PowerCallback(SceUID notifyId, int32_t notifyCount, int32_t notifyArg, void *pCommon)
{
	PlayerSimple *workObj = (PlayerSimple *)pCommon;

	if (notifyArg & SCE_POWER_CALLBACKARG_RESERVED_22)
	{
		workObj->player->SetPowerSaving(true);
	}
	else if (notifyArg & SCE_POWER_CALLBACKARG_RESERVED_23)
	{
		workObj->player->SetPowerSaving(false);
	}

	return SCE_OK;
}

void menu::PlayerSimple::GenericPlayerStateCbFun(int32_t type, ui::Handler *self, ui::Event *e, void *userdata)
{
	PlayerSimple *workObj = (PlayerSimple *)userdata;
	string text8;
	wstring text16;

	int32_t state = e->GetValue(0);

	if (state == GenericPlayer::InitState_InitOk)
	{
		workObj->loadIndicator->Stop();

		uint32_t totalTime = workObj->player->GetTotalTimeMs();
		ui::Widget *totalTimeText = workObj->root->FindChild(text_video_control_panel_progressbar_label_total);
		if (totalTime > 0)
		{
			utils::ConvertSecondsToString(text8, totalTime / 1000, false);
			common::Utf8ToUtf16(text8, &text16);
			totalTimeText->SetString(text16);
			workObj->progressBar->AddEventCallback(ui::ProgressBarTouch::CB_PROGRESSBAR_DRAG_END, ProgressBarCbFun, userdata);
			workObj->progressBar->AddEventCallback(ui::Handler::CB_TOUCH_MOVE, ProgressBarCbFun, userdata);
			workObj->progressBar->AddEventCallback(ui::Handler::CB_TOUCH_OUT, ProgressBarCbFun, userdata);
		}
		else
		{
			workObj->isLS = true;
		}

		inputdevice::pad::SetDeviceHandler(DirectInputCallback);

		ui::Widget *videoPlane = workObj->root->FindChild(button_video_page_control_trigger);
		videoPlane->AddEventCallback(ui::Button::CB_BTN_DECIDE, VideoPlaneCbFun, userdata);

		videoPlane->SetKeycode(inputdevice::pad::Data::PAD_MENU);

		common::MainThreadCallList::Register(UpdateTask, userdata);

		event::BroadcastGlobalEvent(g_appPlugin, PlayerSimpleEvent, PlayerEvent_InitOk);
	}
	else if (state == GenericPlayer::InitState_InitFail)
	{
		event::BroadcastGlobalEvent(g_appPlugin, PlayerSimpleEvent, PlayerEvent_InitFail);
	}
}

void menu::PlayerSimple::DirectInputCallback(inputdevice::pad::Data *pData)
{
#define PRESSED(x) ((pData->paddata & x) && !(s_instance->oldButtons & x))

	if (!s_instance)
		return;

	if (PRESSED(inputdevice::pad::Data::PAD_ENTER))
	{
		if (s_instance->player->GetState() == GenericPlayer::PlayerState_Eof && !s_instance->isLS)
		{
			WholeRepeatButtonCbFun(0, NULL, 0, s_instance);
		}
		else
		{
			PlayButtonCbFun(0, NULL, 0, s_instance);
		}
	}
	else if (PRESSED(inputdevice::pad::Data::PAD_ESCAPE) && !s_instance->progressPlaneShown)
	{
		BackButtonCbFun(0, NULL, 0, s_instance);
		return;
	}
	else if (((PRESSED(inputdevice::pad::Data::PAD_RIGHT)) ||
		(PRESSED(inputdevice::pad::Data::PAD_LEFT)) ||
		(PRESSED(inputdevice::pad::Data::PAD_R)) ||
		(PRESSED(inputdevice::pad::Data::PAD_L))) &&
		!s_instance->isLS)
	{
		math::v4 col(0.4f, 0.4f, 0.4f, 1.0f);
		s_instance->videoPlane->SetColor(col);

		if (PRESSED(inputdevice::pad::Data::PAD_RIGHT))
		{
			s_instance->accJumpTime += 5000;
		}
		else if (PRESSED(inputdevice::pad::Data::PAD_LEFT))
		{
			s_instance->accJumpTime -= 5000;
		}
		else if (PRESSED(inputdevice::pad::Data::PAD_R))
		{
			s_instance->accJumpTime += (int32_t)((float)s_instance->player->GetTotalTimeMs() * 0.05f);
		}
		else if (PRESSED(inputdevice::pad::Data::PAD_L))
		{
			s_instance->accJumpTime -= (int32_t)((float)s_instance->player->GetTotalTimeMs() * 0.05f);
		}

		string text8;
		wstring text16;
		utils::ConvertSecondsToString(text8, (uint32_t)(sce_paf_abs(s_instance->accJumpTime) / 1000), false);
		common::Utf8ToUtf16(text8, &text16);
		if (s_instance->accJumpTime < 0)
		{
			s_instance->leftAccText->SetString(text16);
		}
		else if (s_instance->accJumpTime > 0)
		{
			s_instance->rightAccText->SetString(text16);
		}

		s_instance->accStartTime = sceKernelGetProcessTimeLow();
		s_instance->accJumpState = AccJumpState_Accumulate;
	}
	else if (PRESSED(inputdevice::pad::Data::PAD_START))
	{
		scePowerRequestDisplayOff();
	}

	s_instance->oldButtons = pData->paddata;

#undef PRESSED
}

void menu::PlayerSimple::UpdateTask(void *pArgBlock)
{
	PlayerSimple *workObj = (PlayerSimple *)pArgBlock;
	string text8;
	wstring text16;

	if (!workObj->isLS)
	{
		if (!workObj->isSeeking)
		{
			uint32_t currTime = workObj->player->GetCurrentTimeMs() / 1000;
			if (currTime != workObj->oldCurrTime)
			{
				utils::ConvertSecondsToString(text8, currTime, false);
				common::Utf8ToUtf16(text8, &text16);
				workObj->elapsedTimeText->SetString(text16);
				float progress = (float)currTime * 100000.0f / (float)workObj->player->GetTotalTimeMs();
				workObj->progressBar->SetValue(progress);
				workObj->oldCurrTime = currTime;
			}
		}
		else
		{
			uint32_t val = (uint32_t)(workObj->progressBar->GetValue() / 100000.0f * (float)workObj->player->GetTotalTimeMs());
			utils::ConvertSecondsToString(text8, val, false);
			common::Utf8ToUtf16(text8, &text16);
			workObj->elapsedTimeText->SetString(text16);
		}

		if (workObj->accJumpState == AccJumpState_Perform)
		{
			math::v4 col(1.0f, 1.0f, 1.0f, 1.0f);
			s_instance->videoPlane->SetColor(col);
			text16 = L"";
			s_instance->leftAccText->SetString(text16);
			s_instance->rightAccText->SetString(text16);
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
				workObj->progressPlane->Hide(common::transition::Type_SlideFromBottom1);
			}
			workObj->backButton->Hide(common::transition::Type_Reset);
			sceAppMgrSetInfobarState(SCE_APPMGR_INFOBAR_VISIBILITY_INVISIBLE, SCE_APPMGR_INFOBAR_COLOR_BLACK, SCE_APPMGR_INFOBAR_TRANSPARENCY_TRANSLUCENT);
			workObj->progressPlaneShown = false;
		}
	}

	GenericPlayer::PlayerState state = workObj->player->GetState();
	if (!workObj->isLS && state == GenericPlayer::PlayerState_Eof && workObj->oldState != GenericPlayer::PlayerState_Eof)
	{
		math::v4 col(0.4f, 0.4f, 0.4f, 1.0f);
		workObj->videoPlane->SetColor(col);
		workObj->wholeRepeatButton->Show(common::transition::Type_Fadein1);
	}
	workObj->oldState = state;
}

menu::PlayerSimple::PlayerSimple(const char *url) :
	GenericMenu("page_player_simple",
	MenuOpenParam(true, 200.0f, Plugin::TransitionType_None, ui::EnvironmentParam::RESOLUTION_HD_FULL),
	MenuCloseParam(true))
{
	Plugin::TemplateOpenParam tmpParam;
	oldButtons = 0;
	accJumpTime = 0;
	accStartTime = 0;
	accJumpState = AccJumpState_None;
	progressPlaneShown = false;
	isLS = false;
	isSeeking = false;
	currentScale = 1.0f;
	settingsOverride = SettingsOverride_None;

	if (s_instance)
	{
		SCE_DBG_LOG_ERROR("[MENU] Attempt to create second singleton instance\n");
		return;
	}

	progressBar = (ui::ProgressBarTouch *)root->FindChild(progressbar_touch_video_control_panel);
	elapsedTimeText = (ui::Text *)root->FindChild(text_video_control_panel_progressbar_label_elapsed);
	leftAccText = root->FindChild(text_video_page_player_simple_acc_left);
	rightAccText = root->FindChild(text_video_page_player_simple_acc_right);
	statPlane = root->FindChild(plane_statindicator);
	progressPlane = root->FindChild(plane_video_control_panel_bg);
	progressPlane->Hide(common::transition::Type_Fadein1);

	loadIndicator = (ui::BusyIndicator *)root->FindChild(busyindicator_video_page_player_simple);
	if (SCE_PAF_IS_DOLCE)
	{
		loadIndicator->SetBallSize(32.0f);
	}
	loadIndicator->Start();

	backButton = root->FindChild(button_back_page_player_simple);
	backButton->Hide(common::transition::Type_Reset);
	backButton->AddEventCallback(ui::Button::CB_BTN_DECIDE, BackButtonCbFun, this);

	wholeRepeatButton = root->FindChild(button_video_page_whole_repeat);
	wholeRepeatButton->Hide(common::transition::Type_Fadein1);
	wholeRepeatButton->AddEventCallback(ui::Button::CB_BTN_DECIDE, WholeRepeatButtonCbFun, this);

	playButton = root->FindChild(button_video_control_panel_playpause);
	playButton->AddEventCallback(ui::Button::CB_BTN_DECIDE, PlayButtonCbFun, this);

	videoPlane = root->FindChild(plane_video_page_player_simple);

	root->AddEventCallback(GenericPlayer::GenericPlayerChangeState, GenericPlayerStateCbFun, this);
	if (FMODPlayer::IsSupported(url) == GenericPlayer::SupportType_Supported)
	{
		player = new FMODPlayer(videoPlane, url);
	}
	else
	{
		player = new BEAVPlayer(videoPlane, url);
	}
	player->InitAsync();

	pwCbId = sceKernelCreateCallback("PowerCallback", 0, PowerCallback, this);
	scePowerRegisterCallback(pwCbId);

	sceAppMgrSetInfobarState(SCE_APPMGR_INFOBAR_VISIBILITY_INVISIBLE, SCE_APPMGR_INFOBAR_COLOR_BLACK, SCE_APPMGR_INFOBAR_TRANSPARENCY_TRANSLUCENT);
	utils::SetPowerTickTask(utils::PowerTick_All);
	menu::GetMenuAt(menu::GetMenuCount() - 2)->Deactivate();

	s_instance = this;
}

menu::PlayerSimple::~PlayerSimple()
{
	utils::SetDisplayResolution(ui::EnvironmentParam::RESOLUTION_PSP2);
	common::MainThreadCallList::Unregister(UpdateTask, this);
	scePowerUnregisterCallback(pwCbId);
	sceKernelDeleteCallback(pwCbId);
	inputdevice::pad::SetDeviceHandler(NULL);
	delete player;
	sceAppMgrSetInfobarState(SCE_APPMGR_INFOBAR_VISIBILITY_VISIBLE, SCE_APPMGR_INFOBAR_COLOR_BLACK, SCE_APPMGR_INFOBAR_TRANSPARENCY_TRANSLUCENT);
	utils::SetPowerTickTask(utils::PowerTick_None);
	menu::GetTopMenu()->Activate();
	s_instance = NULL;
}

float menu::PlayerSimple::GetScale()
{
	return currentScale;
}

void menu::PlayerSimple::SetScale(float scale)
{
	if (currentScale == scale || scale > 1.0f || scale < 0.0f)
	{
		return;
	}

	math::v4 sz(0.0f);
	math::v4 pos(0.0f);
	math::v2 tsz;
	ui::Widget *triggerPlane = root->FindChild(button_video_page_control_trigger);
	ui::Text *totalTimeText = (ui::Text *)root->FindChild(text_video_control_panel_progressbar_label_total);

	if (SCE_PAF_IS_DOLCE)
	{
		sz.set_x(1920.0f * scale);
		sz.set_y(1088.0f * scale);
		videoPlane->SetSize(sz);
		sz.set_x(150.0f * scale);
		sz.set_y(150.0f * scale);
		loadIndicator->SetSize(sz);
		loadIndicator->SetBallSize(32.0f * scale);
		sz.set_x(180.0f * scale);
		sz.set_y(180.0f * scale);
		wholeRepeatButton->SetSize(sz);
		if (scale == 1.0f)
		{
			sz.set_x(1700.0f);
			sz.set_y(112.0f);
			progressPlane->SetSize(sz);
			pos.set_x(0.0f);
			pos.set_y(56.0f);
			progressPlane->SetPos(pos);
		}
		else
		{
			sz.set_x(1920.0f * scale);
			sz.set_y(112.0f * scale);
			progressPlane->SetSize(sz);
			pos.set_x(0.0f);
			pos.set_y(56.0f * scale);
			progressPlane->SetPos(pos);
		}
		sz.set_x(1400.0f * scale);
		sz.set_y(20.0f * scale);
		progressBar->SetSize(sz);
		sz.set_x(90.0f * scale);
		sz.set_y(80.0f * scale);
		playButton->SetSize(sz);
		pos.set_x(30.0f * scale);
		pos.set_y(0.0f);
		playButton->SetPos(pos);
		pos.set_x(-20.0f * scale);
		if (scale != 1.0f)
		{
			pos.set_x(pos.extract_x() - 20.0f * scale);
		}
		pos.set_y(20.0f * scale);
		elapsedTimeText->SetPos(pos);
		pos.set_y(-pos.extract_y());
		totalTimeText->SetPos(pos);
	}
	else
	{
		sz.set_x(960.0f * scale);
		sz.set_y(544.0f * scale);
		videoPlane->SetSize(sz);
		sz.set_x(75.0f * scale);
		sz.set_y(75.0f * scale);
		loadIndicator->SetSize(sz);
		loadIndicator->SetBallSize(16.0f * scale);
		sz.set_x(90.0f * scale);
		sz.set_y(90.0f * scale);
		wholeRepeatButton->SetSize(sz);
		if (scale == 1.0f)
		{
			sz.set_x(790.0f);
			sz.set_y(56.0f);
			progressPlane->SetSize(sz);
			pos.set_x(0.0f);
			pos.set_y(28.0f);
			progressPlane->SetPos(pos);
		}
		else
		{
			sz.set_x(960.0f * scale);
			sz.set_y(112.0f * scale);
			progressPlane->SetSize(sz);
			pos.set_x(0.0f);
			pos.set_y(56.0f * scale);
			progressPlane->SetPos(pos);
		}
		sz.set_x(538.0f * scale);
		sz.set_y(10.0f * scale);
		progressBar->SetSize(sz);
		/*
		pos.y = 0.0f;
		pos.x = 14.0f * scale;
		progressBar->SetPosition(&pos);
		*/
		sz.set_x(48.0f * scale);
		sz.set_y(80.0f * scale);
		playButton->SetSize(sz);
		pos.set_x(32.0f * scale);
		pos.set_y(0.0f);
		playButton->SetPos(pos);
		pos.set_x(-20.0f * scale);
		if (scale != 1.0f)
		{
			pos.set_x(pos.extract_x() - 20.0f * scale);
		}
		if (scale != 1.0f)
		{
			pos.set_y(20.0f * scale);
		}
		else
		{
			pos.set_y(12.0f * scale);
		}
		elapsedTimeText->SetPos(pos);
		pos.set_y(-pos.extract_y());
		totalTimeText->SetPos(pos);
	}

	if (scale != 1.0f)
	{
		tsz.set_x(16.0f);
	}
	else
	{
		tsz.set_x(20.0f);
	}

	tsz.set_y(tsz.extract_x());
	elapsedTimeText->SetStyleAttribute(graph::TextStyleAttribute_Point, 0, 0, tsz);
	totalTimeText->SetStyleAttribute(graph::TextStyleAttribute_Point, 0, 0, tsz);

	if (scale == 1.0f)
	{
		inputdevice::pad::SetDeviceHandler(DirectInputCallback);
		sceAppMgrSetInfobarState(SCE_APPMGR_INFOBAR_VISIBILITY_INVISIBLE, SCE_APPMGR_INFOBAR_COLOR_BLACK, SCE_APPMGR_INFOBAR_TRANSPARENCY_TRANSLUCENT);
		backButton->Show(common::transition::Type_Reset);
		triggerPlane->Show(common::transition::Type_Fadein1);
		menu::GetMenuAt(menu::GetMenuCount() - 2)->Deactivate();
	}
	else
	{
		inputdevice::pad::SetDeviceHandler(NULL);
		sceAppMgrSetInfobarState(SCE_APPMGR_INFOBAR_VISIBILITY_VISIBLE, SCE_APPMGR_INFOBAR_COLOR_BLACK, SCE_APPMGR_INFOBAR_TRANSPARENCY_TRANSLUCENT);
		backButton->Hide(common::transition::Type_Reset);
		triggerPlane->Hide(common::transition::Type_Fadein1);
		if (!isLS)
		{
			progressPlane->Show(common::transition::Type_Fadein1);
		}
		progressPlaneShown = true;
		menu::GetMenuAt(menu::GetMenuCount() - 2)->Activate();
	}

	currentScale = scale;
}

void menu::PlayerSimple::SetPosition(float x, float y)
{
	math::v4 pos(x, y);
	videoPlane->SetPos(pos);
}

void menu::PlayerSimple::SetSettingsOverride(SettingsOverride override)
{
	settingsOverride = override;
}
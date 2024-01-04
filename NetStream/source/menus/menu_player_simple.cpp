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

int32_t menu::PlayerSimple::PowerCallback(SceUID notifyId, int32_t notifyCount, int32_t notifyArg, void *pCommon)
{
	reinterpret_cast<PlayerSimple *>(pCommon)->OnPowerCallback(notifyArg);
	return SCE_OK;
}

void menu::PlayerSimple::UpdateTask(void *pArgBlock)
{
	reinterpret_cast<PlayerSimple *>(pArgBlock)->OnUpdate();
}

void  menu::PlayerSimple::OnVideoPlaneTouch()
{
	if (m_progressPlaneShown)
	{
		if (!m_isLS)
		{
			m_progressPlane->Hide(common::transition::Type_SlideFromBottom1);
		}
		m_backButton->Hide(common::transition::Type_Reset);
		sceAppMgrSetInfobarState(SCE_APPMGR_INFOBAR_VISIBILITY_INVISIBLE, SCE_APPMGR_INFOBAR_COLOR_BLACK, SCE_APPMGR_INFOBAR_TRANSPARENCY_TRANSLUCENT);
		m_progressPlaneShown = false;
	}
	else
	{
		if (!m_isLS)
		{
			m_progressPlane->Show(common::transition::Type_SlideFromBottom1);
		}
		if (m_currentScale == 1.0f)
		{
			m_backButton->Show(common::transition::Type_Reset);
		}
		m_progressPlaneShownTime = sceKernelGetProcessTimeLow();
		sceAppMgrSetInfobarState(SCE_APPMGR_INFOBAR_VISIBILITY_VISIBLE, SCE_APPMGR_INFOBAR_COLOR_BLACK, SCE_APPMGR_INFOBAR_TRANSPARENCY_TRANSLUCENT);
		m_progressPlaneShown = true;
	}
}

void menu::PlayerSimple::OnProgressBarStateChange(int32_t type)
{
	if (type == ui::ProgressBarTouch::CB_PROGRESSBAR_DRAG_END)
	{
		uint32_t val = (uint32_t)(m_progressBar->GetValue() / 100.0f * static_cast<float>(m_player->GetTotalTimeMs()));
		m_player->JumpToTimeMs(val);
		m_isSeeking = false;
	}
	else if (type == ui::Handler::CB_TOUCH_MOVE || type == ui::Handler::CB_TOUCH_OUT)
	{
		m_isSeeking = true;
	}
}

void menu::PlayerSimple::OnBackButton()
{
	event::BroadcastGlobalEvent(g_appPlugin, PlayerSimpleEvent, PlayerEvent_Back);
}

void menu::PlayerSimple::OnPlayButton()
{
	uint32_t texid = 0;

	m_player->SwitchPlaybackState();

	if (m_player->IsPaused())
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
		m_playButton->SetTexture(tex);
	}
}

void menu::PlayerSimple::OnWholeRepeatButton()
{
	m_player->JumpToTimeMs(0);
	m_player->SwitchPlaybackState();
	m_videoPlane->SetColor({ 1.0f, 1.0f, 1.0f, 1.0f });
	m_wholeRepeatButton->Hide(common::transition::Type_Fadein1);
}

void menu::PlayerSimple::OnPowerCallback(int32_t notifyArg)
{
	if (notifyArg & SCE_POWER_CALLBACKARG_RESERVED_22)
	{
		m_player->SetPowerSaving(true);
	}
	else if (notifyArg & SCE_POWER_CALLBACKARG_RESERVED_23)
	{
		m_player->SetPowerSaving(false);
	}
}

void menu::PlayerSimple::OnGenericPlayerStateChange(int32_t state)
{
	string text8;
	wstring text16;

	if (state == GenericPlayer::InitState_InitOk)
	{
		m_loadIndicator->Stop();

		uint32_t totalTime = m_player->GetTotalTimeMs();
		ui::Widget *totalTimeText = m_root->FindChild(text_video_control_panel_progressbar_label_total);
		if (totalTime > 0)
		{
			utils::ConvertSecondsToString(text8, totalTime / 1000, false);
			common::Utf8ToUtf16(text8, &text16);
			totalTimeText->SetString(text16);

			auto progressBarCb = [](int32_t type, ui::Handler *self, ui::Event *e, void *userdata)
			{
				reinterpret_cast<menu::PlayerSimple *>(userdata)->OnProgressBarStateChange(type);
			};
			m_progressBar->AddEventCallback(ui::ProgressBarTouch::CB_PROGRESSBAR_DRAG_END, progressBarCb, this);
			m_progressBar->AddEventCallback(ui::Handler::CB_TOUCH_MOVE, progressBarCb, this);
			m_progressBar->AddEventCallback(ui::Handler::CB_TOUCH_OUT, progressBarCb, this);
		}
		else
		{
			m_isLS = true;
		}

		inputdevice::AddInputListener(m_padListener);

		ui::Widget *videoPlane = m_root->FindChild(button_video_page_control_trigger);
		videoPlane->AddEventCallback(ui::Button::CB_BTN_DECIDE,
		[](int32_t type, ui::Handler *self, ui::Event *e, void *userdata)
		{
			reinterpret_cast<menu::PlayerSimple *>(userdata)->OnVideoPlaneTouch();
		}, this);

		videoPlane->SetKeycode(inputdevice::pad::Data::PAD_MENU);

		common::MainThreadCallList::Register(UpdateTask, this);

		event::BroadcastGlobalEvent(g_appPlugin, PlayerSimpleEvent, PlayerEvent_InitOk);
	}
	else if (state == GenericPlayer::InitState_InitFail)
	{
		event::BroadcastGlobalEvent(g_appPlugin, PlayerSimpleEvent, PlayerEvent_InitFail);
	}
}

void menu::PlayerSimple::OnUpdate()
{
	string text8;
	wstring text16;

	if (!m_isLS)
	{
		if (!m_isSeeking)
		{
			uint32_t currTime = m_player->GetCurrentTimeMs() / 1000;
			if (currTime != m_oldCurrTime)
			{
				utils::ConvertSecondsToString(text8, currTime, false);
				common::Utf8ToUtf16(text8, &text16);
				elapsedTimeText->SetString(text16);
				float progress = static_cast<float>(currTime) * 100000.0f / static_cast<float>(m_player->GetTotalTimeMs());
				m_progressBar->SetValue(progress);
				m_oldCurrTime = currTime;
			}
		}
		else
		{
			uint32_t val = static_cast<uint32_t>(m_progressBar->GetValue() / 100000.0f * static_cast<float>(m_player->GetTotalTimeMs()));
			utils::ConvertSecondsToString(text8, val, false);
			common::Utf8ToUtf16(text8, &text16);
			elapsedTimeText->SetString(text16);
		}

		if (m_accJumpState == AccJumpState_Perform)
		{
			m_videoPlane->SetColor({ 1.0f, 1.0f, 1.0f, 1.0f });
			text16 = L"";
			m_leftAccText->SetString(text16);
			m_rightAccText->SetString(text16);
			m_player->JumpToTimeMs(m_player->GetCurrentTimeMs() + m_accJumpTime);
			m_accJumpTime = 0;
			m_accJumpState = AccJumpState_None;
		}
		else if (m_accJumpState == AccJumpState_Accumulate)
		{
			if ((sceKernelGetProcessTimeLow() - m_accStartTime) > 500000)
			{
				m_accJumpState = AccJumpState_Perform;
			}
		}
	}

	if (m_progressPlaneShown)
	{
		if (m_isSeeking || m_player->IsPaused() || m_currentScale != 1.0f)
		{
			m_progressPlaneShownTime = sceKernelGetProcessTimeLow();
		}
		else if ((sceKernelGetProcessTimeLow() - m_progressPlaneShownTime) > 5000000)
		{
			if (!m_isLS)
			{
				m_progressPlane->Hide(common::transition::Type_SlideFromBottom1);
			}
			m_backButton->Hide(common::transition::Type_Reset);
			sceAppMgrSetInfobarState(SCE_APPMGR_INFOBAR_VISIBILITY_INVISIBLE, SCE_APPMGR_INFOBAR_COLOR_BLACK, SCE_APPMGR_INFOBAR_TRANSPARENCY_TRANSLUCENT);
			m_progressPlaneShown = false;
		}
	}

	GenericPlayer::PlayerState state = m_player->GetState();
	if (!m_isLS && state == GenericPlayer::PlayerState_Eof && m_oldState != GenericPlayer::PlayerState_Eof)
	{
		m_videoPlane->SetColor({ 0.4f, 0.4f, 0.4f, 1.0f });
		m_wholeRepeatButton->Show(common::transition::Type_Fadein1);
	}
	m_oldState = state;
}

void menu::PlayerSimple::OnPadUpdate(inputdevice::Data *data)
{
#define PRESSED(x) ((data->m_pad_data->paddata & x) && !(data->m_pad_data_pre->paddata & x))

	if (PRESSED(inputdevice::pad::Data::PAD_ENTER))
	{
		if (m_player->GetState() == GenericPlayer::PlayerState_Eof && !m_isLS)
		{
			OnWholeRepeatButton();
		}
		else
		{
			OnPlayButton();
		}
	}
	else if (PRESSED(inputdevice::pad::Data::PAD_ESCAPE) && !m_progressPlaneShown)
	{
		OnBackButton();
		return;
	}
	else if (((PRESSED(inputdevice::pad::Data::PAD_RIGHT)) ||
		(PRESSED(inputdevice::pad::Data::PAD_LEFT)) ||
		(PRESSED(inputdevice::pad::Data::PAD_R)) ||
		(PRESSED(inputdevice::pad::Data::PAD_L))) &&
		!m_isLS)
	{
		m_videoPlane->SetColor({0.4f, 0.4f, 0.4f, 1.0f});

		if (PRESSED(inputdevice::pad::Data::PAD_RIGHT))
		{
			m_accJumpTime += 5000;
		}
		else if (PRESSED(inputdevice::pad::Data::PAD_LEFT))
		{
			m_accJumpTime -= 5000;
		}
		else if (PRESSED(inputdevice::pad::Data::PAD_R))
		{
			m_accJumpTime += static_cast<int32_t>(static_cast<float>(m_player->GetTotalTimeMs()) * 0.05f);
		}
		else if (PRESSED(inputdevice::pad::Data::PAD_L))
		{
			m_accJumpTime -= static_cast<int32_t>(static_cast<float>(m_player->GetTotalTimeMs()) * 0.05f);
		}

		string text8;
		wstring text16;
		utils::ConvertSecondsToString(text8, static_cast<uint32_t>(sce_paf_abs(m_accJumpTime) / 1000), false);
		common::Utf8ToUtf16(text8, &text16);
		if (m_accJumpTime < 0)
		{
			m_leftAccText->SetString(text16);
		}
		else if (m_accJumpTime > 0)
		{
			m_rightAccText->SetString(text16);
		}

		m_accStartTime = sceKernelGetProcessTimeLow();
		m_accJumpState = AccJumpState_Accumulate;
	}
	else if (PRESSED(inputdevice::pad::Data::PAD_START))
	{
		scePowerRequestDisplayOff();
	}

#undef PRESSED
}

menu::PlayerSimple::PlayerSimple(const char *url) :
	GenericMenu("page_player_simple",
	MenuOpenParam(true, 200.0f, Plugin::TransitionType_None, ui::EnvironmentParam::RESOLUTION_HD_FULL),
	MenuCloseParam(true)),
	m_oldButtons(0),
	m_accJumpTime(0),
	m_accStartTime(0),
	m_accJumpState(AccJumpState_None),
	m_progressPlaneShown(false),
	m_isLS(false),
	m_isSeeking(false),
	m_currentScale(1.0f),
	m_settingsOverride(SettingsOverride_None)
{
	Plugin::TemplateOpenParam tmpParam;

	common::SharedPtr<inputdevice::InputListener> listener(new PadListener(this));
	m_padListener = listener;

	m_progressBar = (ui::ProgressBarTouch *)m_root->FindChild(progressbar_touch_video_control_panel);
	elapsedTimeText = (ui::Text *)m_root->FindChild(text_video_control_panel_progressbar_label_elapsed);
	m_leftAccText = m_root->FindChild(text_video_page_player_simple_acc_left);
	m_rightAccText = m_root->FindChild(text_video_page_player_simple_acc_right);
	m_statPlane = m_root->FindChild(plane_statindicator);
	m_progressPlane = m_root->FindChild(plane_video_control_panel_bg);
	m_progressPlane->Hide(common::transition::Type_Fadein1);

	m_loadIndicator = (ui::BusyIndicator *)m_root->FindChild(busyindicator_video_page_player_simple);
	if (SCE_PAF_IS_DOLCE)
	{
		m_loadIndicator->SetBallSize(32.0f);
	}
	m_loadIndicator->Start();

	m_backButton = m_root->FindChild(button_back_page_player_simple);
	m_backButton->Hide(common::transition::Type_Reset);
	m_backButton->AddEventCallback(ui::Button::CB_BTN_DECIDE,
	[](int32_t type, ui::Handler *self, ui::Event *e, void *userdata)
	{
		reinterpret_cast<menu::PlayerSimple *>(userdata)->OnBackButton();
	}, this);

	m_wholeRepeatButton = m_root->FindChild(button_video_page_whole_repeat);
	m_wholeRepeatButton->Hide(common::transition::Type_Fadein1);
	m_wholeRepeatButton->AddEventCallback(ui::Button::CB_BTN_DECIDE,
	[](int32_t type, ui::Handler *self, ui::Event *e, void *userdata)
	{
		reinterpret_cast<menu::PlayerSimple *>(userdata)->OnWholeRepeatButton();
	}, this);

	m_playButton = m_root->FindChild(button_video_control_panel_playpause);
	m_playButton->AddEventCallback(ui::Button::CB_BTN_DECIDE,
	[](int32_t type, ui::Handler *self, ui::Event *e, void *userdata)
	{
		reinterpret_cast<menu::PlayerSimple *>(userdata)->OnPlayButton();
	}, this);

	m_videoPlane = m_root->FindChild(plane_video_page_player_simple);

	m_root->AddEventCallback(GenericPlayer::GenericPlayerChangeState,
	[](int32_t type, ui::Handler *self, ui::Event *e, void *userdata)
	{
		reinterpret_cast<menu::PlayerSimple *>(userdata)->OnGenericPlayerStateChange(e->GetValue(0));
	}, this);

	if (FMODPlayer::IsSupported(url) == GenericPlayer::SupportType_Supported)
	{
		m_player = new FMODPlayer(m_videoPlane, url);
	}
	else
	{
		m_player = new BEAVPlayer(m_videoPlane, url);
	}
	m_player->InitAsync();

	m_pwCbId = sceKernelCreateCallback("PowerCallback", 0, PowerCallback, this);
	scePowerRegisterCallback(m_pwCbId);

	sceAppMgrSetInfobarState(SCE_APPMGR_INFOBAR_VISIBILITY_INVISIBLE, SCE_APPMGR_INFOBAR_COLOR_BLACK, SCE_APPMGR_INFOBAR_TRANSPARENCY_TRANSLUCENT);
	utils::SetPowerTickTask(utils::PowerTick_All);
	menu::GetMenuAt(menu::GetMenuCount() - 2)->Deactivate();
}

menu::PlayerSimple::~PlayerSimple()
{
	utils::SetDisplayResolution(ui::EnvironmentParam::RESOLUTION_PSP2);
	common::MainThreadCallList::Unregister(UpdateTask, this);
	scePowerUnregisterCallback(m_pwCbId);
	sceKernelDeleteCallback(m_pwCbId);
	inputdevice::DelInputListener(m_padListener);
	delete m_player;
	sceAppMgrSetInfobarState(SCE_APPMGR_INFOBAR_VISIBILITY_VISIBLE, SCE_APPMGR_INFOBAR_COLOR_BLACK, SCE_APPMGR_INFOBAR_TRANSPARENCY_TRANSLUCENT);
	utils::SetPowerTickTask(utils::PowerTick_None);
	menu::GetTopMenu()->Activate();
}

float menu::PlayerSimple::GetScale()
{
	return m_currentScale;
}

void menu::PlayerSimple::SetScale(float scale)
{
	if (m_currentScale == scale || scale > 1.0f || scale < 0.0f)
	{
		return;
	}

	math::v4 sz(0.0f);
	math::v4 pos(0.0f);
	math::v2 tsz;
	ui::Widget *triggerPlane = m_root->FindChild(button_video_page_control_trigger);
	auto totalTimeText = static_cast<ui::Text *>(m_root->FindChild(text_video_control_panel_progressbar_label_total));

	if (SCE_PAF_IS_DOLCE)
	{
		sz.set_x(1920.0f * scale);
		sz.set_y(1088.0f * scale);
		m_videoPlane->SetSize(sz);
		sz.set_x(150.0f * scale);
		sz.set_y(150.0f * scale);
		m_loadIndicator->SetSize(sz);
		m_loadIndicator->SetBallSize(32.0f * scale);
		sz.set_x(180.0f * scale);
		sz.set_y(180.0f * scale);
		m_wholeRepeatButton->SetSize(sz);
		if (scale == 1.0f)
		{
			sz.set_x(1700.0f);
			sz.set_y(112.0f);
			m_progressPlane->SetSize(sz);
			pos.set_x(0.0f);
			pos.set_y(56.0f);
			m_progressPlane->SetPos(pos);
		}
		else
		{
			sz.set_x(1920.0f * scale);
			sz.set_y(112.0f * scale);
			m_progressPlane->SetSize(sz);
			pos.set_x(0.0f);
			pos.set_y(56.0f * scale);
			m_progressPlane->SetPos(pos);
		}
		sz.set_x(1400.0f * scale);
		sz.set_y(20.0f * scale);
		m_progressBar->SetSize(sz);
		sz.set_x(90.0f * scale);
		sz.set_y(80.0f * scale);
		m_playButton->SetSize(sz);
		pos.set_x(30.0f * scale);
		pos.set_y(0.0f);
		m_playButton->SetPos(pos);
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
		m_videoPlane->SetSize(sz);
		sz.set_x(75.0f * scale);
		sz.set_y(75.0f * scale);
		m_loadIndicator->SetSize(sz);
		m_loadIndicator->SetBallSize(16.0f * scale);
		sz.set_x(90.0f * scale);
		sz.set_y(90.0f * scale);
		m_wholeRepeatButton->SetSize(sz);
		if (scale == 1.0f)
		{
			sz.set_x(790.0f);
			sz.set_y(56.0f);
			m_progressPlane->SetSize(sz);
			pos.set_x(0.0f);
			pos.set_y(28.0f);
			m_progressPlane->SetPos(pos);
		}
		else
		{
			sz.set_x(960.0f * scale);
			sz.set_y(112.0f * scale);
			m_progressPlane->SetSize(sz);
			pos.set_x(0.0f);
			pos.set_y(56.0f * scale);
			m_progressPlane->SetPos(pos);
		}
		sz.set_x(538.0f * scale);
		sz.set_y(10.0f * scale);
		m_progressBar->SetSize(sz);
		/*
		pos.y = 0.0f;
		pos.x = 14.0f * scale;
		progressBar->SetPosition(&pos);
		*/
		sz.set_x(48.0f * scale);
		sz.set_y(80.0f * scale);
		m_playButton->SetSize(sz);
		pos.set_x(32.0f * scale);
		pos.set_y(0.0f);
		m_playButton->SetPos(pos);
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
		inputdevice::AddInputListener(m_padListener);
		sceAppMgrSetInfobarState(SCE_APPMGR_INFOBAR_VISIBILITY_INVISIBLE, SCE_APPMGR_INFOBAR_COLOR_BLACK, SCE_APPMGR_INFOBAR_TRANSPARENCY_TRANSLUCENT);
		m_backButton->Show(common::transition::Type_Reset);
		triggerPlane->Show(common::transition::Type_Fadein1);
		menu::GetMenuAt(menu::GetMenuCount() - 2)->Deactivate();
	}
	else
	{
		inputdevice::DelInputListener(m_padListener);
		sceAppMgrSetInfobarState(SCE_APPMGR_INFOBAR_VISIBILITY_VISIBLE, SCE_APPMGR_INFOBAR_COLOR_BLACK, SCE_APPMGR_INFOBAR_TRANSPARENCY_TRANSLUCENT);
		m_backButton->Hide(common::transition::Type_Reset);
		triggerPlane->Hide(common::transition::Type_Fadein1);
		if (!m_isLS)
		{
			m_progressPlane->Show(common::transition::Type_Fadein1);
		}
		m_progressPlaneShown = true;
		menu::GetMenuAt(menu::GetMenuCount() - 2)->Activate();
	}

	m_currentScale = scale;
}

void menu::PlayerSimple::SetPosition(float x, float y)
{
	m_videoPlane->SetPos({x, y});
}

void menu::PlayerSimple::SetSettingsOverride(SettingsOverride override)
{
	m_settingsOverride = override;
}

GenericPlayer *menu::PlayerSimple::GetPlayer()
{
	return m_player;
}
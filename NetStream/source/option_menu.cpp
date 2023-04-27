#include <kernel.h>
#include <libdbg.h>
#include <paf.h>

#include "event.h"
#include "option_menu.h"

#define template_option_menu_base (0xed36b1c7)
#define plane_option_menu_base (0x4446ff86)
#define box_option_menu (0xae3d8acd)
#define button_option_menu_close (0x655c6a32)
#define template_option_menu_button_single (0x9227a229)
#define button_option_menu_single (0x242a5612)
#define template_option_menu_button_top (0x9e7aa431)
#define button_option_menu_top (0x1f350609)
#define template_option_menu_button_bottom (0xeaf5dc69)
#define button_option_menu_bottom (0xe034ff9c)
#define template_option_menu_button_middle (0x429cab34)
#define button_option_menu_middle (0xbbc3bf22)
#define page_option_menu (0xd098335e)
#define text_option_menu_ruler (0xc9cdb2cc)

using namespace paf;

const float k_minBtLen = 202.0f;

void OptionMenu::ButtonCbFun(int32_t type, ui::Handler *self, ui::Event *e, void *userdata)
{
	ui::Widget *wdg = (ui::Widget*)self;
	uint32_t hash = wdg->GetName().GetIDHash();
	OptionMenu *workObj = (OptionMenu *)userdata;
	Plugin *plugin = workObj->plugin;
	delete workObj;

	if (hash == button_option_menu_close)
	{
		event::BroadcastGlobalEvent(plugin, OptionMenuEvent, OptionMenuEvent_Close);
	}
	else
	{
		event::BroadcastGlobalEvent(plugin, OptionMenuEvent, OptionMenuEvent_Button, hash - OPTION_MENU_BUTTON_MAGIC);
	}
}

void OptionMenu::SizeAdjustEventHandler(int32_t type, ui::Handler *self, ui::Event *e, void *userdata)
{
	ui::Widget *button = NULL;
	float len = 0.0f;
	ui::Text *wdg = (ui::Text*)self;
	OptionMenu *workObj = (OptionMenu *)userdata;

	len = wdg->GetDrawObj(ui::Text::OBJ_ROOT)->GetSize().extract_x() + 40.0f;

	if (len < k_minBtLen)
	{
		len = k_minBtLen;
	}

	math::v4 btSz(len, 60.0f);
	math::v4 plSz(len + 12.0f, 12.0f + 60.0f * workObj->buttonCount);
	math::v4 plPos(264.0f + ((202.0f - plSz.extract_x()) / 2.0f), 43.0f);

	ui::Widget *optRoot = workObj->optPlaneRoot->FindChild(box_option_menu);

	for (int i = 0; i < workObj->buttonCount; i++)
	{
		button = optRoot->FindChild(OPTION_MENU_BUTTON_MAGIC + i);
		button->SetAdjust(ui::Widget::ADJUST_NONE, ui::Widget::ADJUST_NONE, ui::Widget::ADJUST_NONE);
		button->SetSize(btSz);
	}

	workObj->optPlaneRoot->SetSize(plSz);
	workObj->optPlaneRoot->SetPos(plPos);
	workObj->optPlaneRoot->Show(common::transition::Type_Popup4, 0.0f);

	self->DeleteEventCallback(ui::Handler::CB_STATE_READY, SizeAdjustEventHandler, userdata);
}

OptionMenu::OptionMenu(Plugin *workPlugin, ui::Widget *root, vector<Button> *buttons)
{
	Plugin::TemplateOpenParam tmpParam;
	ui::Widget *closeButton = NULL;
	ui::Widget *button = NULL;

	parentRoot = root;
	plugin = workPlugin;
	buttonCount = buttons->size();

	Plugin::PageOpenParam openParam;
	openParam.option = Plugin::PageOption_Create;
	openParam.overwrite_draw_priority = 8;
	openParam.graphics_flag = 0x80;
	rootScene = plugin->PageOpen(page_option_menu, openParam);
	if (!rootScene)
	{
		SCE_DBG_LOG_ERROR("[OPTION_MENU] Attempt to create option menu twice!\n");
		return;
	}

	parentRoot->SetActivate(false);

	plugin->TemplateOpen(rootScene, template_option_menu_base, tmpParam);

	closeButton = rootScene->FindChild(button_option_menu_close);
	closeButton->AddEventCallback(ui::Button::CB_BTN_DECIDE, ButtonCbFun, this);
	closeButton->SetKeycode(inputdevice::pad::Data::PAD_ESCAPE | inputdevice::pad::Data::PAD_MENU);

	optPlaneRoot = closeButton->FindChild(plane_option_menu_base);
	ui::Widget *optRoot = optPlaneRoot->FindChild(box_option_menu);

	uint32_t maxLen = 0;
	uint32_t curLen = 0;
	uint32_t maxLenIdx = 0;
	uint32_t idhash = 0;

	for (int i = 0; i < buttonCount; i++)
	{
		Button btItem = buttons->at(i);

		if (i == 0)
		{
			if (buttonCount == 1)
			{
				idhash = template_option_menu_button_single;
			}
			else
			{
				idhash = template_option_menu_button_top;
			}
		}
		else if (i == buttonCount - 1)
		{
			idhash = template_option_menu_button_bottom;
		}
		else
		{
			idhash = template_option_menu_button_middle;
		}

		plugin->TemplateOpen(optRoot, idhash, tmpParam);
		button = optRoot->GetChild(optRoot->GetChildrenNum() - 1);
		button->SetName(OPTION_MENU_BUTTON_MAGIC + i);
		button->SetString(btItem.label);
		button->AddEventCallback(ui::Button::CB_BTN_DECIDE, ButtonCbFun, this);

		curLen = btItem.label.length();
		if (curLen > maxLen)
		{
			maxLen = curLen;
			maxLenIdx = i;
		}
	}

	ui::Widget *ruler = optPlaneRoot->FindChild(text_option_menu_ruler);
	ruler->SetString(buttons->at(maxLenIdx).label);
	ruler->AddEventCallback(ui::Handler::CB_STATE_READY, SizeAdjustEventHandler, this);
}

OptionMenu::~OptionMenu()
{
	Plugin::PageCloseParam closeParam;

	optPlaneRoot->Hide(common::transition::Type_Popup4, 0.0f);

	closeParam.fade = true;
	closeParam.fade_time_ms = 1000;
	plugin->PageClose(page_option_menu, closeParam);

	parentRoot->SetActivate(true);
}
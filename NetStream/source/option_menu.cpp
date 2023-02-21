#include <kernel.h>
#include <libdbg.h>
#include <paf.h>
#include <system_param.h>

#include "utils.h"
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

const SceFloat32 k_minBtLen = 202.0f;

SceInt32 OptionMenu::ButtonEventCallback::HandleEvent(SceInt32 eventId, paf::ui::Widget *self, SceInt32 a3)
{
	SceInt32 ret;

	if ((this->state & 1) == 0) {
		delete this->workObj;
		if (this->cb != 0) {
			if (self->elem.hash == button_option_menu_close)
			{
				this->cb(OPTION_MENU_BUTTON_MAGIC, this->pUserData);
			}
			else
			{
				this->cb(self->elem.hash - OPTION_MENU_BUTTON_MAGIC, this->pUserData);
			}
		}
		ret = SCE_OK;
	}
	else {
		ret = 0x80AF4101;
	}

	return ret;
};

SceVoid OptionMenu::SizeAdjustEventHandler(SceInt32 eventId, paf::ui::Widget *self, SceInt32, ScePVoid pUserData)
{
	rco::Element element;
	ui::Widget *button = SCE_NULL;
	SceFloat32 len = 0.0f;
	OptionMenu *workObj = (OptionMenu *)pUserData;

	len = self->GetDrawObj()->size.GetX() + 40.0f;

	if (len < k_minBtLen)
	{
		len = k_minBtLen;
	}

	Vector4 btSz(len, 60.0f);
	Vector4 plSz(len + 12.0f, 12.0f + 60.0f * workObj->buttonCount);
	Vector4 plPos(264.0f + ((202.0f - plSz.x) / 2.0f), 43.0f);

	element.hash = box_option_menu;
	ui::Widget *optRoot = workObj->optPlaneRoot->GetChild(&element, 0);

	for (int i = 0; i < workObj->buttonCount; i++)
	{
		element.hash = OPTION_MENU_BUTTON_MAGIC + i;
		button = optRoot->GetChild(&element, 0);
		button->SetAdjust(ui::Adjust_None, ui::Adjust_None, ui::Adjust_None);
		button->SetSize(btSz);
	}

	workObj->optPlaneRoot->SetSize(plSz);
	workObj->optPlaneRoot->SetPosition(plPos);
	workObj->optPlaneRoot->PlayEffect(0.0f, effect::EffectType_Popup4);

	self->UnregisterEventCallback(ui::EventRender_RenderStateReady, 0, 0);
}

OptionMenu::OptionMenu(Plugin *workPlugin, ui::Widget *root, vector<Button> *buttons, ButtonCallback eventCallback, ButtonCallback closeCallback, ScePVoid userArg)
{
	rco::Element element;
	Plugin::TemplateOpenParam tmpParam;
	ui::Widget *closeButton = SCE_NULL;
	ui::Widget *button = SCE_NULL;

	parentRoot = root;
	plugin = workPlugin;
	buttonCount = buttons->size();

	Plugin::PageOpenParam openParam;
	openParam.flags = 1;
	openParam.priority = 8;
	openParam.unk_28_pageArg_a5 = 0x80;
	element.hash = page_option_menu;
	rootScene = plugin->PageOpen(&element, &openParam);
	if (!rootScene)
	{
		SCE_DBG_LOG_ERROR("[OPTION_MENU] Attempt to create option menu twice!\n");
		return;
	}

	ui::Widget::SetControlFlags(parentRoot, 0);

	element.hash = template_option_menu_base;
	plugin->TemplateOpen(rootScene, &element, &tmpParam);

	element.hash = button_option_menu_close;
	closeButton = rootScene->GetChild(&element, 0);
	closeButton->RegisterEventCallback(ui::EventMain_Decide, new ButtonEventCallback(this, closeCallback, userArg));
	closeButton->SetDirectKey(SCE_PAF_CTRL_CIRCLE | SCE_PAF_CTRL_TRIANGLE);

	element.hash = plane_option_menu_base;
	optPlaneRoot = closeButton->GetChild(&element, 0);

	element.hash = box_option_menu;
	ui::Widget *optRoot = optPlaneRoot->GetChild(&element, 0);

	SceUInt32 maxLen = 0;
	SceUInt32 curLen = 0;
	SceUInt32 maxLenIdx = 0;

	for (int i = 0; i < buttonCount; i++)
	{
		Button btItem = buttons->at(i);

		if (i == 0)
		{
			if (buttonCount == 1)
			{
				element.hash = template_option_menu_button_single;
			}
			else
			{
				element.hash = template_option_menu_button_top;
			}
		}
		else if (i == buttonCount - 1)
		{
			element.hash = template_option_menu_button_bottom;
		}
		else
		{
			element.hash = template_option_menu_button_middle;
		}

		plugin->TemplateOpen(optRoot, &element, &tmpParam);
		button = optRoot->GetChild(optRoot->childNum - 1);
		button->elem.hash = OPTION_MENU_BUTTON_MAGIC + i;
		button->SetLabel(&btItem.label);
		button->RegisterEventCallback(ui::EventMain_Decide, new ButtonEventCallback(this, eventCallback, userArg));

		curLen = btItem.label.length();
		if (curLen > maxLen)
		{
			maxLen = curLen;
			maxLenIdx = i;
		}
	}

	element.hash = text_option_menu_ruler;
	ui::Widget *ruler = optPlaneRoot->GetChild(&element, 0);
	ruler->SetLabel(&buttons->at(maxLenIdx).label);
	ruler->RegisterEventCallback(ui::EventRender_RenderStateReady, new utils::SimpleEventCallback(SizeAdjustEventHandler, this));
}

OptionMenu::~OptionMenu()
{
	rco::Element element;
	Plugin::PageCloseParam closeParam;

	optPlaneRoot->PlayEffectReverse(0.0f, effect::EffectType_Popup4);

	element.hash = page_option_menu;
	closeParam.useFadeout = true;
	closeParam.fadeoutTimeMs = 1000;
	plugin->PageClose(&element, &closeParam);

	ui::Widget::SetControlFlags(parentRoot, 1);
}
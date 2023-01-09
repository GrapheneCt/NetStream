#include <kernel.h>
#include <libdbg.h>
#include <paf.h>

#include "utils.h"
#include "option_menu.h"

#define OPTION_MENU_BUTTON_MAGIC 0xa5448baf

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

using namespace paf;

const wchar_t *k_optionMenuFontVal[] = { L"A", L"B", L"C", L"D", L"E", L"F", L"G", L"H", L"I", L"J", L"K", L"L", L"M", L"N", L"O", L"P", L"Q", L"R", L"S", L"T", L"U", L"V", L"W", L"X", L"Y", L"Z", L"a", L"b", L"c", L"d", L"e", L"f", L"g", L"h", L"i", L"j", L"k", L"l", L"m", L"n", L"o", L"p", L"q", L"r", L"s", L"t", L"u", L"v", L"w", L"x", L"y", L"z", L"1", L"2", L"3", L"4", L"5", L"6", L"7", L"8", L"9", L" ", L"-" };
const SceFloat32 k_optionMenuFontSize[] = { 22.00f, 22.00f, 22.00f, 22.00f, 20.00f, 20.00f, 22.00f, 22.00f, 8.00f, 12.00f, 22.00f, 18.00f, 26.00f, 22.00f, 24.00f, 20.00f, 24.00f, 22.00f, 20.00f, 20.00f, 22.00f, 22.00f, 28.00f, 20.00f, 22.00f, 20.00f, 18.00f, 18.00f, 18.00f, 18.00f, 18.00f, 12.00f, 18.00f, 18.00f, 8.00f, 12.00f, 18.00f, 8.00f, 24.00f, 18.00f, 20.00f, 18.00f, 18.00f, 12.00f, 16.00f, 12.00f, 18.00f, 18.00f, 24.00f, 18.00f, 18.00f, 16.00f, 20.00f, 20.00f, 20.00f, 20.00f, 20.00f, 20.00f, 20.00f, 20.00f, 20.00f, 12.00f, 14.00f };

SceVoid OptionMenu::CloseButtonEventCallback::CloseButtonCbFun(SceInt32 eventId, ui::Widget *self, SceInt32 a3, ScePVoid pUserData)
{
	OptionMenu *workObj = (OptionMenu *)pUserData;

	delete workObj;
}

SceInt32 OptionMenu::ButtonEventCallback::HandleEvent(SceInt32 eventId, paf::ui::Widget *self, SceInt32 a3)
{
	SceInt32 ret;

	if ((this->state & 1) == 0) {
		delete this->workObj;
		if (this->cb != 0) {
			this->cb(self->elem.hash - OPTION_MENU_BUTTON_MAGIC, this->pUserData);
		}
		ret = SCE_OK;
	}
	else {
		ret = 0x80AF4101;
	}

	return ret;
};

OptionMenu::OptionMenu(Plugin *workPlugin, ui::Widget *root, vector<Button> *buttons, ButtonCallback eventCallback, ScePVoid userArg)
{
	rco::Element element;
	Plugin::TemplateOpenParam tmpParam;
	ui::Widget *closeButton = SCE_NULL;
	ui::Widget *button = SCE_NULL;
	SceFloat32 baseX = 162.0f;

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
	CloseButtonEventCallback *clcb = new CloseButtonEventCallback(this);
	closeButton->RegisterEventCallback(ui::EventMain_Decide, clcb);
	closeButton->SetDirectKey(SCE_PAF_CTRL_CIRCLE | SCE_PAF_CTRL_TRIANGLE);

	element.hash = plane_option_menu_base;
	optPlaneRoot = closeButton->GetChild(&element, 0);

	element.hash = box_option_menu;
	ui::Widget *optRoot = optPlaneRoot->GetChild(&element, 0);

	SceUInt32 maxLen = 0;
	SceUInt32 curLen = 0;
	SceFloat32 estLength = 0.0f;

	for (int i = 0; i < buttonCount; i++)
	{
		curLen = buttons->at(i).label.length();
		if (curLen > maxLen)
		{
			maxLen = curLen;
		}
	}

	for (int i = 0; i < buttonCount; i++)
	{
		if (buttons->at(i).label.length() == maxLen)
		{
			estLength = EstimateStringLength(&buttons->at(i).label);
			break;
		}
	}

	if (estLength > baseX)
	{
		baseX = estLength;
	}

	Vector4 btSz(baseX + 40.0f, 60.0f);
	Vector4 plSz(baseX + 52.0f, 12.0f + 60.0f * buttonCount);
	Vector4 plPos(264.0f + ((202.0f - plSz.x) / 2.0f), 43.0f);

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
		button->SetSize(&btSz);
		button->RegisterEventCallback(ui::EventMain_Decide, new ButtonEventCallback(this, eventCallback, userArg));
	}

	optPlaneRoot->SetSize(&plSz);
	optPlaneRoot->SetPosition(&plPos);
	optPlaneRoot->PlayEffect(0.0f, effect::EffectType_Popup4);
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

SceFloat32 OptionMenu::EstimateStringLength(wstring *s)
{
	const wchar_t *ws = s->c_str();
	wchar_t sval = 0;
	SceFloat32 res = 0.0f;
	SceBool found = SCE_FALSE;

	for (int i = 0; i < s->length(); i++)
	{
		sval = ws[i];
		found = SCE_FALSE;
		for (int j = 0; j < sizeof(k_optionMenuFontSize) / sizeof(SceFloat32); j++)
		{
			if (sval == *k_optionMenuFontVal[j])
			{
				res += k_optionMenuFontSize[j];
				found = SCE_TRUE;
				break;
			}
		}
		if (!found)
		{
			res += 19.0f;
		}
	}

	return res;
}
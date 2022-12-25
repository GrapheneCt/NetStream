#include <kernel.h>
#include <paf.h>

#include "common.h"
#include "utils.h"
#include "menus/menu_generic.h"
#include "menus/menu_settings.h"

using namespace paf;

static paf::vector<menu::GenericMenu*> s_menuStack;

SceVoid menu::SettingsButtonCbFun(SceInt32 eventId, ui::Widget *self, SceInt32 a3, ScePVoid pUserData)
{
	menu::Settings *set = new menu::Settings();
	if (pUserData)
	{
		SceUInt32 *uarg = (SceUInt32 *)pUserData;
		set->SetCloseCallback((menu::Settings::CloseCallback)uarg[0], (ScePVoid)uarg[1]);
	}
}

menu::GenericMenu::GenericMenu(const char *name, MenuOpenParam oparam, MenuCloseParam cparam)
{
	rco::Element searchParam;
	Plugin::PageOpenParam openParam;

	if (!name)
	{
		return;
	}

	closeParam = *(Plugin::PageCloseParam *)&cparam;

	searchParam.hash = utils::GetHash(name);
	root = g_appPlugin->PageOpen(&searchParam, (Plugin::PageOpenParam *)&oparam);

	if (!s_menuStack.empty())
	{
		menu::GenericMenu *oldMenu = s_menuStack.back();
		ui::Widget::SetControlFlags(oldMenu->root, 0);
	}

	s_menuStack.push_back(this);
}

menu::GenericMenu::~GenericMenu()
{
	s_menuStack.pop_back();

	g_appPlugin->PageClose(&root->elem, &closeParam);

	if (!s_menuStack.empty())
	{
		menu::GenericMenu *oldMenu = s_menuStack.back();
		ui::Widget::SetControlFlags(oldMenu->root, 1);
	}
}

SceVoid menu::HideAll(SceUInt32 endMargin)
{
	for (int i = 0; i < s_menuStack.size() - endMargin; i++)
	{
		s_menuStack.at(i)->root->SetGraphicsDisabled(true);
	}
}

SceVoid menu::ShowAll()
{
	for (int i = 0; i < s_menuStack.size(); i++)
	{
		s_menuStack.at(i)->root->SetGraphicsDisabled(false);
	}
}

SceVoid menu::InitMenuSystem()
{

}

SceVoid menu::TermMenuSystem()
{

}

menu::GenericMenu *menu::GetTopMenu()
{
	if (!s_menuStack.empty())
	{
		return s_menuStack.back();
	}

	return SCE_NULL;
}

SceUInt32 menu::GetMenuCount()
{
	return s_menuStack.size();
}

menu::GenericMenu *menu::GetMenuAt(SceUInt32 idx)
{
	return s_menuStack.at(idx);
}
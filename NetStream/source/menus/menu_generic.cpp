#include <kernel.h>
#include <paf.h>

#include "common.h"
#include "utils.h"
#include "menus/menu_generic.h"
#include "menus/menu_settings.h"

using namespace paf;

static paf::vector<menu::GenericMenu*> s_menuStack;

void menu::SettingsButtonCbFun(int32_t type, ui::Handler *self, ui::Event *e, void *userdata)
{
	menu::Settings *set = new menu::Settings();
}

menu::GenericMenu::GenericMenu(const char *name, MenuOpenParam const& oparam, MenuCloseParam const& cparam)
{
	if (!name)
	{
		return;
	}

	closeParam = cparam;

	root = g_appPlugin->PageOpen(name, oparam);
	root->SetName(name);

	if (!s_menuStack.empty())
	{
		s_menuStack.back()->Deactivate(true);
	}

	s_menuStack.push_back(this);
}

menu::GenericMenu::~GenericMenu()
{
	s_menuStack.pop_back();

	g_appPlugin->PageClose(root->GetName(), closeParam);

	if (!s_menuStack.empty())
	{
		s_menuStack.back()->Activate();
	}
}

void menu::GenericMenu::Deactivate(bool withDelay)
{
	Timer *t = NULL;
	root->SetActivate(false);
	if (withDelay)
	{
		t = new Timer(200.0f);
	}
	root->Hide(t);
}

void menu::GenericMenu::Activate()
{
	root->Show();
	root->SetActivate(true);
}

void menu::GenericMenu::DisableInput()
{
	root->SetActivate(false);
}

void menu::GenericMenu::EnableInput()
{
	root->SetActivate(true);
}

ui::Scene *menu::GenericMenu::GetRoot()
{
	return root;
}

void menu::DeactivateAll(uint32_t endMargin)
{
	for (int i = 0; i < s_menuStack.size() - endMargin; i++)
	{
		s_menuStack.at(i)->Deactivate();
	}
}

void menu::ActivateAll()
{
	for (int i = 0; i < s_menuStack.size(); i++)
	{
		s_menuStack.at(i)->Activate();
	}
}

void menu::InitMenuSystem()
{

}

void menu::TermMenuSystem()
{

}

menu::GenericMenu *menu::GetTopMenu()
{
	if (!s_menuStack.empty())
	{
		return s_menuStack.back();
	}

	return NULL;
}

uint32_t menu::GetMenuCount()
{
	return s_menuStack.size();
}

menu::GenericMenu *menu::GetMenuAt(uint32_t idx)
{
	return s_menuStack.at(idx);
}
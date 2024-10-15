#include <kernel.h>
#include <paf.h>

#include "common.h"
#include "utils.h"
#include "debug.h"
#include "menus/menu_generic.h"
#include "menus/menu_settings.h"

using namespace paf;

static paf::vector<menu::GenericMenu*> s_menuStack;

void menu::SettingsButtonCbFun(int32_t type, ui::Handler *self, ui::Event *e, void *userdata)
{
	new menu::Settings();
}

menu::GenericMenu::GenericMenu(const char *name, MenuOpenParam const& oparam, MenuCloseParam const& cparam)
{
	if (!name)
	{
		return;
	}

	m_closeParam = cparam;

	m_root = g_appPlugin->PageOpen(name, oparam);
	m_root->SetName(name);

	if (!s_menuStack.empty())
	{
		s_menuStack.back()->Deactivate(true);
	}

	s_menuStack.push_back(this);

#ifdef _DEBUG
	SetCurrentDebugParam(g_appPlugin, m_root);
#endif
}

menu::GenericMenu::~GenericMenu()
{
	s_menuStack.pop_back();

	g_appPlugin->PageClose(m_root->GetName(), m_closeParam);

	if (!s_menuStack.empty())
	{
		s_menuStack.back()->Activate();

#ifdef _DEBUG
		SetCurrentDebugParam(g_appPlugin, s_menuStack.back()->GetRoot());
#endif
	}
}

void menu::GenericMenu::Deactivate(bool withDelay)
{
	Timer *t = nullptr;
	m_root->SetActivate(false);
	if (withDelay)
	{
		t = new Timer(200.0f);
	}
	m_root->Hide(t);
}

void menu::GenericMenu::Activate()
{
	m_root->Show();
	m_root->SetActivate(true);
}

void menu::GenericMenu::DisableInput()
{
	m_root->SetActivate(false);
}

void menu::GenericMenu::EnableInput()
{
	m_root->SetActivate(true);
}

ui::Scene *menu::GenericMenu::GetRoot() const
{
	return m_root;
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
	for (auto menu : s_menuStack)
	{
		menu->Activate();
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

	return nullptr;
}

uint32_t menu::GetMenuCount()
{
	return s_menuStack.size();
}

menu::GenericMenu *menu::GetMenuAt(uint32_t idx)
{
	return s_menuStack.at(idx);
}
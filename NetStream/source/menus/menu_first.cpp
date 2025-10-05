#include <kernel.h>
#include <paf.h>

#include "common.h"
#include "utils.h"
#include "menus/menu_first.h"
#include "menus/menu_ftp.h"
#include "menus/menu_http.h"
#include "menus/menu_local.h"
#include "menus/menu_hvdb.h"
#include "menus/menu_youtube.h"
#include "menus/menu_settings.h"

#include <paf_file_ext.h>

using namespace paf;

static bool s_afterBoot = true;

menu::First::First() : GenericMenu("page_first", MenuOpenParam(true), MenuCloseParam()), m_showHvdb(false)
{
	Plugin::TemplateOpenParam tmpParam;

	if (LocalFile::Exists("app0:config.ini"))
	{
		m_showHvdb = true;
	}

	auto topText = static_cast<ui::Text *>(m_root->FindChild(text_top));
	wstring title = g_appPlugin->GetString(msg_title_menu_first);
	topText->SetString(title);

	auto listView = static_cast<ui::ListView *>(m_root->FindChild(list_view_generic));
	listView->SetItemFactory(new ListViewFactory(this));
	listView->InsertSegment(0, 1);
	listView->SetCellSizeDefault(0, { 960.0f, 80.0f });
	listView->SetSegmentLayoutType(0, ui::ListView::LAYOUT_TYPE_LIST);
	if (m_showHvdb)
	{
		listView->InsertCell(0, 0, 5);
	}
	else
	{
		listView->InsertCell(0, 0, 4);
	}
}

menu::First::~First()
{
	auto listView = static_cast<ui::ListView *>(m_root->FindChild(list_view_generic));
	listView->SetName(static_cast<uint32_t>(0));
}

ui::ListItem* menu::First::CreateListItem(ui::listview::ItemFactory::CreateParam& param)
{
	Plugin::TemplateOpenParam tmpParam;
	ui::ListItem *item = nullptr;
	int32_t idx = param.cell_index;
	uint32_t texid = 0;

	if (!param.list_view->GetName().GetIDHash())
	{
		return new ui::ListItem(param.parent, nullptr);
	}

	if (!m_showHvdb && idx > 0)
	{
		idx++;
	}

	g_appPlugin->TemplateOpen(param.parent, template_list_item_generic, tmpParam);
	item = static_cast<ui::ListItem *>(param.parent->GetChild(param.parent->GetChildrenNum() - 1));

	ui::Widget *button = item->FindChild(image_button_list_item);

	wstring name = utils::GetStringWithNum("msg_fpmenu_item_", idx);
	button->SetString(name);
	button->AddEventCallback(ui::Button::CB_BTN_DECIDE,
	[](int32_t type, ui::Handler *self, ui::Event *e, void *userdata)
	{
		reinterpret_cast<menu::First *>(userdata)->OnListButton(static_cast<ui::Button *>(self));
	}, this);

	switch (idx)
	{
	case 0:
		texid = tex_fpmenu_icon_youtube;
		break;
	case 1:
		texid = tex_fpmenu_icon_hvdb;
		break;
	case 2:
		texid = tex_fpmenu_icon_http;
		break;
	case 3:
		texid = tex_fpmenu_icon_ftp;
		break;
	case 4:
		texid = tex_fpmenu_icon_local;
		break;
	}

	button->SetName(texid);

	intrusive_ptr<graph::Surface> tex = g_appPlugin->GetTexture(texid);
	if (tex.get())
	{
		button->SetTexture(tex);
	}

	return item;
}

void menu::First::OnListButton(ui::Widget *self)
{
	switch (self->GetName().GetIDHash())
	{
	case tex_fpmenu_icon_youtube:
		menu::YouTube *ymenu = new menu::YouTube();
		break;
	case tex_fpmenu_icon_hvdb:
		menu::HVDB *hvdbmenu = new menu::HVDB();
		break;
	case tex_fpmenu_icon_http:
		menu::Http *hmenu = new menu::Http();
		hmenu->PushBrowserPage(NULL);
		break;
	case tex_fpmenu_icon_ftp:
		menu::Ftp *fmenu = new menu::Ftp();
		fmenu->PushBrowserPage(NULL);
		break;
	case tex_fpmenu_icon_local:
		menu::Local *lmenu = new menu::Local();
		if (s_afterBoot)
		{
			string ref = utils::SafememRead();
			lmenu->PushBrowserPage(&ref);
			s_afterBoot = false;
		}
		else
		{
			lmenu->PushBrowserPage(NULL);
		}
		break;
	}
}
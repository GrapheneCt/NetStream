#include <kernel.h>
#include <paf.h>

#include "common.h"
#include "utils.h"
#include "menus/menu_first.h"
#include "menus/menu_ftp.h"
#include "menus/menu_http.h"
#include "menus/menu_local.h"
#include "menus/menu_youtube.h"
#include "menus/menu_settings.h"

#include <paf_file_ext.h>

using namespace paf;

void menu::First::ListButtonCbFun(int32_t type, ui::Handler *self, ui::Event *e, void *userdata)
{
	ui::Widget *wdg = (ui::Widget*)self;
	switch (wdg->GetName().GetIDHash())
	{
	case 0:
		menu::YouTube *ymenu = new menu::YouTube();
		break;
	case 1:
		menu::Http *hmenu = new menu::Http();
		hmenu->PushBrowserPage(NULL);
		break;
	case 2:
		menu::Ftp *fmenu = new menu::Ftp();
		fmenu->PushBrowserPage(NULL);
		break;
	case 3:
		menu::Local *lmenu = new menu::Local();
		lmenu->PushBrowserPage(NULL);
		break;
	}
}

ui::ListItem *menu::First::ListViewFactory::Create(CreateParam& param)
{
	Plugin::TemplateOpenParam tmpParam;
	ui::Widget *item = NULL;
	uint32_t texid = 0;

	if (!param.list_view->GetName().GetIDHash())
	{
		return new ui::ListItem(param.parent, NULL);
	}

	g_appPlugin->TemplateOpen(param.parent, template_list_item_generic, tmpParam);
	item = param.parent->GetChild(param.parent->GetChildrenNum() - 1);

	ui::Widget *button = item->FindChild(image_button_list_item);
	button->SetName(param.cell_index);

	wstring name = utils::GetStringWithNum("msg_fpmenu_item_", param.cell_index);
	button->SetString(name);
	button->AddEventCallback(ui::Button::CB_BTN_DECIDE, ListButtonCbFun);

	switch (param.cell_index)
	{
	case 0:
		texid = tex_fpmenu_icon_youtube;
		break;
	case 1:
		texid = tex_fpmenu_icon_http;
		break;
	case 2:
		texid = tex_fpmenu_icon_ftp;
		break;
	case 3:
		texid = tex_fpmenu_icon_local;
		break;
	}

	intrusive_ptr<graph::Surface> tex = g_appPlugin->GetTexture(texid);
	if (tex.get())
	{
		button->SetTexture(tex);
	}

	return (ui::ListItem *)item;
}

menu::First::First() : GenericMenu("page_first", MenuOpenParam(true), MenuCloseParam())
{
	Plugin::TemplateOpenParam tmpParam;

	ui::Text *topText = (ui::Text *)root->FindChild(text_top);
	wstring title = g_appPlugin->GetString(msg_title_menu_first);
	topText->SetString(title);

	ui::ListView *listView = (ui::ListView *)root->FindChild(list_view_generic);
	listView->SetItemFactory(new ListViewFactory());
	listView->InsertSegment(0, 1);
	math::v4 sz(960.0f, 80.0f);
	listView->SetCellSizeDefault(0, sz);
	listView->SetSegmentLayoutType(0, ui::ListView::LAYOUT_TYPE_LIST);
	listView->InsertCell(0, 0, 4);
}

menu::First::~First()
{
	ui::ListView *listView = (ui::ListView *)root->FindChild(list_view_generic);
	listView->SetName((uint32_t)0);
}
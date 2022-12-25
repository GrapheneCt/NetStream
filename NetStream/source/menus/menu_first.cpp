#include <kernel.h>
#include <paf.h>

#include "common.h"
#include "utils.h"
#include "menus/menu_first.h"
#include "menus/menu_http.h"
#include "menus/menu_youtube.h"
#include "menus/menu_settings.h"

#include "curl_file.h"

using namespace paf;

SceVoid menu::First::ListButtonCbFun(SceInt32 eventId, ui::Widget *self, SceInt32 a3, ScePVoid pUserData)
{
	switch (self->elem.hash)
	{
	case 0:
		menu::YouTube *ymenu = new menu::YouTube();
		break;
	case 1:
		menu::Http *hmenu = new menu::Http();
		hmenu->PushBrowserPage(SCE_NULL);
		break;
	}
}

ui::ListItem *menu::First::ListViewCb::Create(Param *info)
{
	rco::Element searchParam;
	Plugin::TemplateOpenParam tmpParam;
	ui::Widget *item = SCE_NULL;
	graph::Surface *tex = SCE_NULL;

	ui::Widget *targetRoot = info->parent;

	searchParam.hash = template_list_item_generic;
	g_appPlugin->TemplateOpen(targetRoot, &searchParam, &tmpParam);
	item = targetRoot->GetChild(targetRoot->childNum - 1);

	ui::Widget *button = utils::GetChild(item, image_button_list_item);
	button->elem.hash = info->cellIndex;

	wstring name = utils::GetStringWithNum("msg_fpmenu_item_", info->cellIndex);
	button->SetLabel(&name);
	button->RegisterEventCallback(ui::EventMain_Decide, new utils::SimpleEventCallback(ListButtonCbFun));

	switch (info->cellIndex)
	{
	case 0:
		tex = utils::GetTexture(tex_fpmenu_icon_youtube);
		break;
	case 1:
		tex = utils::GetTexture(tex_fpmenu_icon_http);
		break;
	}

	button->SetSurfaceBase(&tex);
	tex->Release();

	return (ui::ListItem *)item;
}

menu::First::First() : GenericMenu("page_first", MenuOpenParam(true), MenuCloseParam())
{
	rco::Element searchParam;
	Plugin::TemplateOpenParam tmpParam;

	ui::Text *topText = (ui::Text *)utils::GetChild(root, text_top);
	wstring title = utils::GetString(msg_title_menu_first);
	topText->SetLabel(&title);

	ui::ListView *listView = (ui::ListView *)utils::GetChild(root, list_view_generic);
	listView->RegisterItemCallback(new ListViewCb());
	listView->SetSegmentEnable(0, 1);
	Vector4 sz(960.0f, 80.0f);
	listView->SetCellSize(0, &sz);
	listView->SetConfigurationType(0, ui::ListView::ConfigurationType_Simple);
	listView->AddItem(0, 0, 2);
}

menu::First::~First()
{

}
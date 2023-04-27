#ifndef _OPTION_MENU_H_
#define _OPTION_MENU_H_

#include <kernel.h>
#include <paf.h>

using namespace paf;

#define OPTION_MENU_BUTTON_MAGIC 0xa5448baf

class OptionMenu
{
public:

	enum
	{
		OptionMenuEvent = (ui::Handler::CB_STATE + 0x50000),
	};

	enum OptionMenuEvent
	{
		OptionMenuEvent_Button,
		OptionMenuEvent_Close
	};

	class Button
	{
	public:

		wstring label;
	};

	OptionMenu(Plugin *workPlugin, ui::Widget *root, vector<Button> *buttons);

	~OptionMenu();

private:

	static void ButtonCbFun(int32_t type, ui::Handler *self, ui::Event *e, void *userdata);

	static void SizeAdjustEventHandler(int32_t type, ui::Handler *self, ui::Event *e, void *userdata);

	Plugin *plugin;
	ui::Scene *rootScene;
	ui::Widget *parentRoot;
	ui::Widget *optPlaneRoot;
	uint32_t buttonCount;
};

#endif

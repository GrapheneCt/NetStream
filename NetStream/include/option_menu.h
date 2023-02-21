#ifndef _OPTION_MENU_H_
#define _OPTION_MENU_H_

#include <kernel.h>
#include <paf.h>

using namespace paf;

#define OPTION_MENU_BUTTON_MAGIC 0xa5448baf

class OptionMenu
{
public:

	typedef SceVoid(*ButtonCallback)(SceUInt32 index, ScePVoid userArg);

	class Button
	{
	public:

		wstring label;
	};

	OptionMenu(Plugin *workPlugin, ui::Widget *root, vector<Button> *buttons, ButtonCallback eventCallback, ButtonCallback closeCallback, ScePVoid userArg);

	~OptionMenu();

private:

	class ButtonEventCallback : public ui::EventCallback
	{
	public:

		ButtonEventCallback(OptionMenu *obj, ButtonCallback function, ScePVoid userArg = SCE_NULL)
		{
			workObj = obj;
			cb = function;
			pUserData = userArg;
		};

		~ButtonEventCallback()
		{

		};

		SceInt32 HandleEvent(SceInt32 eventId, paf::ui::Widget *self, SceInt32 a3);

		OptionMenu *workObj;
		ButtonCallback cb;
	};

	static SceVoid SizeAdjustEventHandler(SceInt32 eventId, paf::ui::Widget *self, SceInt32, ScePVoid pUserData);

	Plugin *plugin;
	ui::Scene *rootScene;
	ui::Widget *parentRoot;
	ui::Widget *optPlaneRoot;
	SceUInt32 buttonCount;
};

#endif

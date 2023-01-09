#ifndef _OPTION_MENU_H_
#define _OPTION_MENU_H_

#include <kernel.h>
#include <paf.h>

using namespace paf;

class OptionMenu
{
public:

	typedef SceVoid(*ButtonCallback)(SceUInt32 index, ScePVoid userArg);

	class Button
	{
	public:

		wstring label;
	};

	OptionMenu(Plugin *workPlugin, ui::Widget *root, vector<Button> *buttons, ButtonCallback eventCallback, ScePVoid userArg);

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

	class CloseButtonEventCallback : public ui::EventCallback
	{
	public:

		static SceVoid CloseButtonCbFun(SceInt32 eventId, ui::Widget *self, SceInt32 a3, ScePVoid pUserData);

		CloseButtonEventCallback(ScePVoid userArg)
		{
			eventHandler = CloseButtonCbFun;
			pUserData = userArg;
		};

		virtual ~CloseButtonEventCallback()
		{

		};
	};

	SceFloat32 EstimateStringLength(wstring *s);

	Plugin *plugin;
	ui::Scene *rootScene;
	ui::Widget *parentRoot;
	ui::Widget *optPlaneRoot;
	SceUInt32 buttonCount;
};

#endif

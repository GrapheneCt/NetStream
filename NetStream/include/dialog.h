#ifndef _DIALOG_H_
#define _DIALOG_H_

#include <kernel.h>
#include <paf.h>
#include <common_gui_dialog.h>

using namespace paf;

namespace dialog
{
	enum ButtonCode
	{
		ButtonCode_X = 1,
		ButtonCode_Ok,
		ButtonCode_Cancel,
		ButtonCode_Yes,
		ButtonCode_No,
		ButtonCode_Button1 = ButtonCode_Yes,
		ButtonCode_Button2 = ButtonCode_No,
		ButtonCode_Button3 = ButtonCode_Cancel
	};

	typedef void(*EventHandler)(ButtonCode buttonCode, ScePVoid pUserArg);

	SceVoid OpenPleaseWait(Plugin *workPlugin, const wchar_t *titleText, const wchar_t *messageText, SceBool withCancel = SCE_FALSE, EventHandler eventHandler= SCE_NULL, ScePVoid userArg = SCE_NULL);

	SceVoid OpenYesNo(Plugin *workPlugin, const wchar_t *titleText, const wchar_t *messageText, EventHandler eventHandler = SCE_NULL, ScePVoid userArg = SCE_NULL);

	SceVoid OpenOk(Plugin *workPlugin, const wchar_t *titleText, const wchar_t *messageText, EventHandler eventHandler = SCE_NULL, ScePVoid userArg = SCE_NULL);

	SceVoid OpenError(Plugin *workPlugin, SceInt32 errorCode, const wchar_t *messageText = SCE_NULL, EventHandler eventHandler = SCE_NULL, ScePVoid userArg = SCE_NULL);

	SceVoid OpenTwoButton(
		Plugin *workPlugin,
		const wchar_t *titleText,
		const wchar_t *messageText,
		SceUInt32 button1TextHashref,
		SceUInt32 button2TextHashref,
		EventHandler eventHandler = SCE_NULL,
		ScePVoid userArg = SCE_NULL);

	SceVoid OpenThreeButton(
		Plugin *workPlugin,
		const wchar_t *titleText,
		const wchar_t *messageText,
		SceUInt32 button1TextHashref,
		SceUInt32 button2TextHashref,
		SceUInt32 button3TextHashref,
		EventHandler eventHandler = SCE_NULL,
		ScePVoid userArg = SCE_NULL);

	ui::ListView *dialog::OpenListView(
		Plugin *workPlugin,
		const wchar_t *titleText,
		EventHandler eventHandler = SCE_NULL,
		ScePVoid userArg = SCE_NULL);

	ui::ScrollView *dialog::OpenScrollView(
		Plugin *workPlugin,
		const wchar_t *titleText,
		EventHandler eventHandler = SCE_NULL,
		ScePVoid userArg = SCE_NULL);

	SceVoid Close();

	SceVoid WaitEnd();
};

#endif

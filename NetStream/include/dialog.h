#ifndef _DIALOG_H_
#define _DIALOG_H_

#include <kernel.h>
#include <paf.h>
#include <common_gui_dialog.h>

using namespace paf;

namespace dialog
{
	enum
	{
		DialogEvent = (ui::Handler::CB_STATE + 0x40000),
	};

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

	void OpenPleaseWait(Plugin *workPlugin, const wchar_t *titleText, const wchar_t *messageText, bool withCancel = false);

	void OpenYesNo(Plugin *workPlugin, const wchar_t *titleText, const wchar_t *messageTextL);

	void OpenOk(Plugin *workPlugin, const wchar_t *titleText, const wchar_t *messageText);

	void OpenError(Plugin *workPlugin, int32_t errorCode, const wchar_t *messageText = NULL);

	void OpenTwoButton(
		Plugin *workPlugin,
		const wchar_t *titleText,
		const wchar_t *messageText,
		uint32_t button1TextHashref,
		uint32_t button2TextHashref);

	void OpenThreeButton(
		Plugin *workPlugin,
		const wchar_t *titleText,
		const wchar_t *messageText,
		uint32_t button1TextHashref,
		uint32_t button2TextHashref,
		uint32_t button3TextHashref);

	ui::ListView *OpenListView(
		Plugin *workPlugin,
		const wchar_t *titleText);

	ui::ScrollView *OpenScrollView(
		Plugin *workPlugin,
		const wchar_t *titleText);

	void Close();

	void WaitEnd();
};

#endif

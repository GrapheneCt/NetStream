#include <kernel.h>
#include <libdbg.h>
#include <paf.h>
#include <common_gui_dialog.h>

#include "dialog.h"

#define CURRENT_DIALOG_NONE -1

using namespace paf;
using namespace sce;

static SceInt32 s_currentDialog = CURRENT_DIALOG_NONE;
static dialog::EventHandler s_currentEventHandler = SCE_NULL;

static SceUInt32 s_twoButtonContTable[12];
static SceUInt32 s_threeButtonContTable[16];

namespace dialog {
	SceVoid CommonGuiEventHandler(SceInt32 instanceSlot, CommonGuiDialog::ButtonCode buttonCode, ScePVoid pUserArg)
	{
		CommonGuiDialog::Dialog::Close(instanceSlot);
		s_currentDialog = CURRENT_DIALOG_NONE;

		if (s_currentEventHandler) {
			s_currentEventHandler((dialog::ButtonCode)buttonCode, pUserArg);
			s_currentEventHandler = SCE_NULL;
		}
	}
}

SceVoid dialog::OpenPleaseWait(Plugin *workPlugin, const wchar_t *titleText, const wchar_t *messageText, SceBool withCancel, EventHandler eventHandler, ScePVoid userArg)
{
	if (s_currentDialog != CURRENT_DIALOG_NONE)
		return;

	SceBool isMainThread = thread::IsMainThread();

	wstring title = titleText;
	wstring message = messageText;

	s_currentEventHandler = eventHandler;

	if (!isMainThread)
		thread::s_mainThreadMutex.Lock();

	if (withCancel)
		s_currentDialog = CommonGuiDialog::Dialog::Show(workPlugin, &title, &message, &CommonGuiDialog::Param::s_dialogCancelBusy, CommonGuiEventHandler, userArg);
	else
		s_currentDialog = CommonGuiDialog::Dialog::Show(workPlugin, &title, &message, &CommonGuiDialog::Param::s_dialogTextSmallBusy, CommonGuiEventHandler, userArg);
	if (!isMainThread)
		thread::s_mainThreadMutex.Unlock();
}

SceVoid dialog::OpenYesNo(Plugin *workPlugin, const wchar_t *titleText, const wchar_t *messageText, EventHandler eventHandler, ScePVoid userArg)
{
	if (s_currentDialog != CURRENT_DIALOG_NONE)
		return;

	SceBool isMainThread = thread::IsMainThread();

	wstring title = titleText;
	wstring message = messageText;

	s_currentEventHandler = eventHandler;

	if (!isMainThread)
		thread::s_mainThreadMutex.Lock();
	s_currentDialog = CommonGuiDialog::Dialog::Show(workPlugin, &title, &message, &CommonGuiDialog::Param::s_dialogYesNo, CommonGuiEventHandler, userArg);
	if (!isMainThread)
		thread::s_mainThreadMutex.Unlock();
}

SceVoid dialog::OpenOk(Plugin *workPlugin, const wchar_t *titleText, const wchar_t *messageText, EventHandler eventHandler, ScePVoid userArg)
{
	if (s_currentDialog != CURRENT_DIALOG_NONE)
		return;

	SceBool isMainThread = thread::IsMainThread();

	wstring title = titleText;
	wstring message = messageText;

	s_currentEventHandler = eventHandler;

	if (!isMainThread)
		thread::s_mainThreadMutex.Lock();
	s_currentDialog = CommonGuiDialog::Dialog::Show(workPlugin, &title, &message, &CommonGuiDialog::Param::s_dialogOk, CommonGuiEventHandler, userArg);
	if (!isMainThread)
		thread::s_mainThreadMutex.Unlock();
}

SceVoid dialog::OpenError(Plugin *workPlugin, SceInt32 errorCode, const wchar_t *messageText, EventHandler eventHandler, ScePVoid userArg)
{
	if (s_currentDialog != CURRENT_DIALOG_NONE)
		return;

	SceBool isMainThread = thread::IsMainThread();

	CommonGuiDialog::ErrorDialog dialog;

	auto cb = new CommonGuiDialog::EventCallback();
	cb->eventHandler = CommonGuiEventHandler;
	cb->pUserData = userArg;

	dialog.workPlugin = workPlugin;
	dialog.errorCode = errorCode;
	dialog.eventHandler = cb;
	if (messageText)
		dialog.message = messageText;

	s_currentEventHandler = eventHandler;

	if (!isMainThread)
		thread::s_mainThreadMutex.Lock();
	s_currentDialog = dialog.Show();
	if (!isMainThread)
		thread::s_mainThreadMutex.Unlock();
}

SceVoid dialog::OpenThreeButton(
	Plugin *workPlugin,
	const wchar_t *titleText,
	const wchar_t *messageText,
	SceUInt32 button1TextHashref,
	SceUInt32 button2TextHashref,
	SceUInt32 button3TextHashref,
	EventHandler eventHandler,
	ScePVoid userArg)
{
	if (s_currentDialog != CURRENT_DIALOG_NONE)
		return;

	SceBool isMainThread = thread::IsMainThread();

	CommonGuiDialog::Param dparam;

	dparam = CommonGuiDialog::Param::s_dialogYesNoCancel;
	sce_paf_memcpy(s_threeButtonContTable, CommonGuiDialog::Param::s_dialogYesNoCancel.contentsList, sizeof(s_threeButtonContTable));
	s_threeButtonContTable[1] = button1TextHashref;
	s_threeButtonContTable[5] = button2TextHashref;
	s_threeButtonContTable[9] = button3TextHashref;
	s_threeButtonContTable[7] = 0x20413274;
	s_threeButtonContTable[11] = 0x20413274;
	dparam.contentsList = (CommonGuiDialog::ContentsHashTable *)s_threeButtonContTable;

	wstring title = titleText;
	wstring message = messageText;

	s_currentEventHandler = eventHandler;

	if (!isMainThread)
		thread::s_mainThreadMutex.Lock();
	s_currentDialog = CommonGuiDialog::Dialog::Show(workPlugin, &title, &message, &dparam, CommonGuiEventHandler, userArg);
	if (!isMainThread)
		thread::s_mainThreadMutex.Unlock();
}

SceVoid dialog::OpenTwoButton(
	Plugin *workPlugin,
	const wchar_t *titleText,
	const wchar_t *messageText,
	SceUInt32 button1TextHashref,
	SceUInt32 button2TextHashref,
	EventHandler eventHandler,
	ScePVoid userArg)
{
	if (s_currentDialog != CURRENT_DIALOG_NONE)
		return;

	SceBool isMainThread = thread::IsMainThread();

	CommonGuiDialog::Param dparam;

	dparam = CommonGuiDialog::Param::s_dialogYesNo;
	sce_paf_memcpy(s_twoButtonContTable, CommonGuiDialog::Param::s_dialogYesNo.contentsList, sizeof(s_twoButtonContTable));
	s_twoButtonContTable[1] = button2TextHashref;
	s_twoButtonContTable[5] = button1TextHashref;
	s_twoButtonContTable[3] = 0x20413274;
	dparam.contentsList = (CommonGuiDialog::ContentsHashTable *)s_twoButtonContTable;

	wstring title = titleText;
	wstring message = messageText;

	s_currentEventHandler = eventHandler;

	if (!isMainThread)
		thread::s_mainThreadMutex.Lock();
	s_currentDialog = CommonGuiDialog::Dialog::Show(workPlugin, &title, &message, &dparam, CommonGuiEventHandler, userArg);
	if (!isMainThread)
		thread::s_mainThreadMutex.Unlock();
}

ui::ListView *dialog::OpenListView(
	Plugin *workPlugin,
	const wchar_t *titleText,
	EventHandler eventHandler,
	ScePVoid userArg)
{
	if (s_currentDialog != CURRENT_DIALOG_NONE)
		return SCE_NULL;

	SceBool isMainThread = thread::IsMainThread();

	wstring title = titleText;

	s_currentEventHandler = eventHandler;

	if (!isMainThread)
		thread::s_mainThreadMutex.Lock();

	s_currentDialog = CommonGuiDialog::Dialog::Show(workPlugin, &title, SCE_NULL, &CommonGuiDialog::Param::s_dialogXLView, CommonGuiEventHandler, userArg);
	ui::Widget *ret = CommonGuiDialog::Dialog::GetWidget(s_currentDialog, CommonGuiDialog::WidgetType_ListView);

	if (!isMainThread)
		thread::s_mainThreadMutex.Unlock();

	return (ui::ListView *)ret;
}

ui::ScrollView *dialog::OpenScrollView(
	Plugin *workPlugin,
	const wchar_t *titleText,
	EventHandler eventHandler,
	ScePVoid userArg)
{
	if (s_currentDialog != CURRENT_DIALOG_NONE)
		return SCE_NULL;

	SceBool isMainThread = thread::IsMainThread();

	wstring title = titleText;

	s_currentEventHandler = eventHandler;

	if (!isMainThread)
		thread::s_mainThreadMutex.Lock();

	s_currentDialog = CommonGuiDialog::Dialog::Show(workPlugin, &title, SCE_NULL, &CommonGuiDialog::Param::s_dialogXView, CommonGuiEventHandler, userArg);
	ui::Widget *ret = CommonGuiDialog::Dialog::GetWidget(s_currentDialog, CommonGuiDialog::WidgetType_ScrollView);

	if (!isMainThread)
		thread::s_mainThreadMutex.Unlock();

	return (ui::ScrollView *)ret;
}

SceVoid dialog::Close()
{
	if (s_currentDialog == CURRENT_DIALOG_NONE)
		return;

	CommonGuiDialog::Dialog::Close(s_currentDialog);
	s_currentDialog = CURRENT_DIALOG_NONE;
	s_currentEventHandler = SCE_NULL;
}

SceVoid dialog::WaitEnd()
{
	if (s_currentDialog == CURRENT_DIALOG_NONE)
		return;

	while (s_currentDialog != CURRENT_DIALOG_NONE) {
		thread::Sleep(100);
	}
}
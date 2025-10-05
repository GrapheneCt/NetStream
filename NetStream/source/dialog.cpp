#include <kernel.h>
#include <libdbg.h>
#include <paf.h>
#include <common_gui_dialog.h>

#include "common.h"
#include "event.h"
#include "dialog.h"

#define CURRENT_DIALOG_NONE -1

using namespace paf;
using namespace sce;

static int32_t s_currentDialog = CURRENT_DIALOG_NONE;
static Plugin *s_currentPlugin = NULL;

static uint32_t s_twoButtonContTable[12];
static uint32_t s_threeButtonContTable[16];

namespace dialog {
	void CommonGuiEventHandler(int32_t instanceSlot, CommonGuiDialog::DIALOG_CB buttonCode, void *pUserArg)
	{
		CommonGuiDialog::Dialog::Close(instanceSlot);
		s_currentDialog = CURRENT_DIALOG_NONE;
		event::BroadcastGlobalEvent(s_currentPlugin, DialogEvent, buttonCode);
		s_currentPlugin = NULL;
	}
}

void dialog::OpenPleaseWait(Plugin *workPlugin, const wchar_t *titleText, const wchar_t *messageText, bool withCancel)
{
	if (s_currentDialog != CURRENT_DIALOG_NONE)
		return;

	bool isMainThread = thread::ThreadIDCache::Check();

	paf::wstring title = titleText;
	paf::wstring message = messageText;

	if (!isMainThread)
		thread::RMutex::MainThreadMutex()->Lock();

	if (withCancel)
		s_currentDialog = CommonGuiDialog::Dialog::Show(workPlugin, &title, &message, &CommonGuiDialog::Param::s_dialogCancelBusy, CommonGuiEventHandler, NULL);
	else
		s_currentDialog = CommonGuiDialog::Dialog::Show(workPlugin, &title, &message, &CommonGuiDialog::Param::s_dialogTextSmallBusy, CommonGuiEventHandler, NULL);

	s_currentPlugin = workPlugin;

	if (!isMainThread)
		thread::RMutex::MainThreadMutex()->Unlock();
}

void dialog::OpenYesNo(Plugin *workPlugin, const wchar_t *titleText, const wchar_t *messageText)
{
	if (s_currentDialog != CURRENT_DIALOG_NONE)
		return;

	bool isMainThread = thread::ThreadIDCache::Check();

	paf::wstring title = titleText;
	paf::wstring message = messageText;

	if (!isMainThread)
		thread::RMutex::MainThreadMutex()->Lock();

	s_currentDialog = CommonGuiDialog::Dialog::Show(workPlugin, &title, &message, &CommonGuiDialog::Param::s_dialogYesNo, CommonGuiEventHandler, NULL);
	s_currentPlugin = workPlugin;

	if (!isMainThread)
		thread::RMutex::MainThreadMutex()->Unlock();
}

void dialog::OpenOk(Plugin *workPlugin, const wchar_t *titleText, const wchar_t *messageText)
{
	if (s_currentDialog != CURRENT_DIALOG_NONE)
		return;

	bool isMainThread = thread::ThreadIDCache::Check();

	paf::wstring title = titleText;
	paf::wstring message = messageText;

	if (!isMainThread)
		thread::RMutex::MainThreadMutex()->Lock();

	s_currentDialog = CommonGuiDialog::Dialog::Show(workPlugin, &title, &message, &CommonGuiDialog::Param::s_dialogOk, CommonGuiEventHandler, NULL);
	s_currentPlugin = workPlugin;

	if (!isMainThread)
		thread::RMutex::MainThreadMutex()->Unlock();
}

void dialog::OpenError(Plugin *workPlugin, int32_t errorCode, const wchar_t *messageText)
{
	if (s_currentDialog != CURRENT_DIALOG_NONE)
		return;

	bool isMainThread = thread::ThreadIDCache::Check();

	CommonGuiDialog::ErrorDialog dialog;

	auto cb = new CommonGuiDialog::EventCBListener(CommonGuiEventHandler, NULL);

	dialog.work_plugin = workPlugin;
	dialog.error = errorCode;
	dialog.listener = cb;
	if (messageText)
		dialog.message = messageText;

	if (!isMainThread)
		thread::RMutex::MainThreadMutex()->Lock();

	s_currentDialog = dialog.Show();
	s_currentPlugin = workPlugin;

	if (!isMainThread)
		thread::RMutex::MainThreadMutex()->Unlock();
}

void dialog::OpenThreeButton(
	Plugin *workPlugin,
	const wchar_t *titleText,
	const wchar_t *messageText,
	uint32_t button1TextHashref,
	uint32_t button2TextHashref,
	uint32_t button3TextHashref)
{
	if (s_currentDialog != CURRENT_DIALOG_NONE)
		return;

	bool isMainThread = thread::ThreadIDCache::Check();

	CommonGuiDialog::Param dparam;

	dparam = CommonGuiDialog::Param::s_dialogYesNoCancel;
	sce_paf_memcpy(s_threeButtonContTable, CommonGuiDialog::Param::s_dialogYesNoCancel.contents_list, sizeof(s_threeButtonContTable));
	s_threeButtonContTable[1] = button1TextHashref;
	s_threeButtonContTable[5] = button2TextHashref;
	s_threeButtonContTable[9] = button3TextHashref;
	s_threeButtonContTable[7] = 0x20413274;
	s_threeButtonContTable[11] = 0x20413274;
	dparam.contents_list = (CommonGuiDialog::ContentsHashTable *)s_threeButtonContTable;

	paf::wstring title = titleText;
	paf::wstring message = messageText;

	if (!isMainThread)
		thread::RMutex::MainThreadMutex()->Lock();

	s_currentDialog = CommonGuiDialog::Dialog::Show(workPlugin, &title, &message, &dparam, CommonGuiEventHandler, NULL);
	s_currentPlugin = workPlugin;

	if (!isMainThread)
		thread::RMutex::MainThreadMutex()->Unlock();
}

void dialog::OpenTwoButton(
	Plugin *workPlugin,
	const wchar_t *titleText,
	const wchar_t *messageText,
	uint32_t button1TextHashref,
	uint32_t button2TextHashref)
{
	if (s_currentDialog != CURRENT_DIALOG_NONE)
		return;

	bool isMainThread = thread::ThreadIDCache::Check();

	CommonGuiDialog::Param dparam;

	dparam = CommonGuiDialog::Param::s_dialogYesNo;
	sce_paf_memcpy(s_twoButtonContTable, CommonGuiDialog::Param::s_dialogYesNo.contents_list, sizeof(s_twoButtonContTable));
	s_twoButtonContTable[1] = button2TextHashref;
	s_twoButtonContTable[5] = button1TextHashref;
	s_twoButtonContTable[3] = 0x20413274;
	dparam.contents_list = (CommonGuiDialog::ContentsHashTable *)s_twoButtonContTable;

	paf::wstring title = titleText;
	paf::wstring message = messageText;

	if (!isMainThread)
		thread::RMutex::MainThreadMutex()->Lock();

	s_currentDialog = CommonGuiDialog::Dialog::Show(workPlugin, &title, &message, &dparam, CommonGuiEventHandler, NULL);
	s_currentPlugin = workPlugin;

	if (!isMainThread)
		thread::RMutex::MainThreadMutex()->Unlock();
}

ui::ListView *dialog::OpenListView(
	Plugin *workPlugin,
	const wchar_t *titleText)
{
	if (s_currentDialog != CURRENT_DIALOG_NONE)
		return NULL;

	bool isMainThread = thread::ThreadIDCache::Check();

	paf::wstring title = titleText;

	if (!isMainThread)
		thread::RMutex::MainThreadMutex()->Lock();

	s_currentDialog = CommonGuiDialog::Dialog::Show(workPlugin, &title, NULL, &CommonGuiDialog::Param::s_dialogXLView, CommonGuiEventHandler, NULL);
	ui::Widget *ret = CommonGuiDialog::Dialog::GetWidget(s_currentDialog, CommonGuiDialog::REGISTER_ID_LIST_VIEW);
	s_currentPlugin = workPlugin;

	if (!isMainThread)
		thread::RMutex::MainThreadMutex()->Unlock();

	return (ui::ListView *)ret;
}

ui::ScrollView *dialog::OpenScrollView(
	Plugin *workPlugin,
	const wchar_t *titleText)
{
	if (s_currentDialog != CURRENT_DIALOG_NONE)
		return NULL;

	bool isMainThread = thread::ThreadIDCache::Check();

	paf::wstring title = titleText;

	if (!isMainThread)
		thread::RMutex::MainThreadMutex()->Lock();

	s_currentDialog = CommonGuiDialog::Dialog::Show(workPlugin, &title, NULL, &CommonGuiDialog::Param::s_dialogXView, CommonGuiEventHandler, NULL);
	ui::Widget *ret = CommonGuiDialog::Dialog::GetWidget(s_currentDialog, CommonGuiDialog::REGISTER_ID_SCROLL_VIEW);
	s_currentPlugin = workPlugin;

	if (!isMainThread)
		thread::RMutex::MainThreadMutex()->Unlock();

	return (ui::ScrollView *)ret;
}

void dialog::Close()
{
	if (s_currentDialog == CURRENT_DIALOG_NONE)
		return;

	CommonGuiDialog::Dialog::Close(s_currentDialog);
	s_currentDialog = CURRENT_DIALOG_NONE;
	s_currentPlugin = NULL;
}

void dialog::WaitEnd()
{
	if (s_currentDialog == CURRENT_DIALOG_NONE)
		return;

	while (s_currentDialog != CURRENT_DIALOG_NONE) {
		thread::Sleep(100);
	}
}
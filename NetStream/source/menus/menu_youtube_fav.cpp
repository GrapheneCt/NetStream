#include <kernel.h>
#include <appmgr.h>
#include <stdlib.h>
#include <string.h>
#include <paf.h>
#include <ini_file_processor.h>

#include "common.h"
#include "menus/menu_youtube.h"
#include "utils.h"
#include "yt_utils.h"
#include "invidious.h"

using namespace paf;

static menu::youtube::FavPage *s_currentFavPage = SCE_NULL;

SceVoid menu::youtube::FavParserThread::CreateVideoButton(FavPage *page, const char *data, SceUInt32 index, const char *keyWord)
{
	SceInt32 res;
	wstring title16;
	wstring subtext16;
	string text8;
	InvItemVideo *vidInfo;
	rco::Element searchParam;
	Plugin::TemplateInitParam tmpParam;
	ui::Widget *box;
	ui::Widget *button;
	ui::Widget *subtext;
	VideoButtonCB *buttonCB;
	SharedPtr<HttpFile> fres;
	graph::Surface *tmbTex;

	res = invParseVideo(data, &vidInfo);
	if (res != 1)
		return;

	if (keyWord) {
		if (!sce_paf_strstr(vidInfo->title, keyWord)) {
			invCleanupVideo(vidInfo);
			return;
		}
		else {
			dialog::Close();
		}
	}

	searchParam.hash = utils::GetHash("youtube_scroll_box");
	box = page->thisPage->GetChild(&searchParam, 0);

	searchParam.hash = utils::GetHash("menu_template_youtube_result_button");
	thread::s_mainThreadMutex.Lock();
	g_appPlugin->TemplateOpen(box, &searchParam, &tmpParam);
	thread::s_mainThreadMutex.Unlock();

	button = (ui::ImageButton *)box->GetChild(box->childNum - 1);

	searchParam.hash = utils::GetHash("yt_text_button_subtext");
	subtext = button->GetChild(&searchParam, 0);

	text8 = vidInfo->title;
	ccc::UTF8toUTF16(&text8, &title16);

	utils::ConvertSecondsToString(&text8, (SceUInt64)vidInfo->lengthSec, SCE_FALSE);
	text8 += "  |  ";
	text8 += vidInfo->author;
	text8 += "  |  ";
	text8 += vidInfo->published;
	ccc::UTF8toUTF16(&text8, &subtext16);

	thread::s_mainThreadMutex.Lock();
	buttonCB = new VideoButtonCB;
	buttonCB->pUserData = buttonCB;
	buttonCB->mode = menu::youtube::Base::Mode_Fav;
	buttonCB->id = vidInfo->id;

	button->SetLabel(&title16);
	subtext->SetLabel(&subtext16);
	button->RegisterEventCallback(ui::EventMain_Decide, buttonCB, 0);
	thread::s_mainThreadMutex.Unlock();

	fres = HttpFile::Open(vidInfo->thmbUrl, &res, 0);
	invCleanupVideo(vidInfo);
	if (res < 0) {
		return;
	}

	graph::Surface::Create(&tmbTex, g_appPlugin->memoryPool, (SharedPtr<File>*)&fres);

	fres.reset();

	if (tmbTex == SCE_NULL) {
		return;
	}

	thread::s_mainThreadMutex.Lock();
	tmbTex->UnsafeRelease();
	button->SetSurfaceBase(&tmbTex);
	thread::s_mainThreadMutex.Unlock();
}

SceVoid menu::youtube::FavParserThread::EntryFunction()
{
	rco::Element searchParam;
	Plugin::TemplateInitParam tmpParam;
	ui::Widget *commonWidget;
	SceUInt32 prevResNum = 0;
	SceInt32 parseRes = 0;
	char key[SCE_INI_FILE_PROCESSOR_KEY_BUFFER_SIZE];

	ytutils::GetFavLog()->Reset();

	if (prevPage) {
		prevResNum = prevPage->resNum;
		for (SceInt32 i = 0; i < prevResNum; i++) {
			ytutils::GetFavLog()->GetNext(key);
		}
	}

	searchParam.hash = utils::GetHash("yt_button_btmenu_right");
	commonWidget = g_rootPage->GetChild(&searchParam, 0);
	commonWidget->PlayEffectReverse(0.0f, effect::EffectType_Fadein1);

	searchParam.hash = utils::GetHash("yt_button_btmenu_left");
	commonWidget = g_rootPage->GetChild(&searchParam, 0);
	commonWidget->PlayEffectReverse(0.0f, effect::EffectType_Fadein1);

	searchParam.hash = utils::GetHash("menu_template_youtube");
	g_appPlugin->TemplateOpen(g_root, &searchParam, &tmpParam);

	searchParam.hash = utils::GetHash("plane_youtube_bg");
	workPage->thisPage = (ui::Plane *)g_root->GetChild(&searchParam, 0);
	workPage->thisPage->elem.hash = (SceUInt32)workPage->thisPage;

	if (workPage->prev != SCE_NULL) {
		if (workPage->prev->prev != SCE_NULL) {
			workPage->prev->prev->thisPage->PlayEffectReverse(0.0f, effect::EffectType_Reset);
			if (workPage->prev->prev->thisPage->animationStatus & 0x80)
				workPage->prev->prev->thisPage->animationStatus &= ~0x80;
		}
		workPage->prev->thisPage->PlayEffect(0.0f, effect::EffectType_3D_SlideToBack1);
		if (workPage->prev->thisPage->animationStatus & 0x80)
			workPage->prev->thisPage->animationStatus &= ~0x80;
	}
	workPage->thisPage->PlayEffect(-5000.0f, effect::EffectType_3D_SlideFromFront);
	if (workPage->thisPage->animationStatus & 0x80)
		workPage->thisPage->animationStatus &= ~0x80;

	if (prevPage) {
		searchParam.hash = utils::GetHash("yt_button_btmenu_left");
		commonWidget = g_rootPage->GetChild(&searchParam, 0);
		commonWidget->PlayEffect(0.0f, effect::EffectType_Fadein1);
	}

	if (keyWord.length()) {
		while (1) {
			ytutils::WaitMenuParsers();
			if (IsCanceled()) {
				Cancel();
				return;
			}
			parseRes = ytutils::GetFavLog()->GetNext(key);
			if (parseRes) {
				break;
			}
			CreateVideoButton(workPage, key, 0, keyWord.c_str());
			workPage->resNum++;
		}
	}
	else {
		for (SceInt32 i = prevResNum; i < menu::youtube::FavPage::k_resPerPage + prevResNum; i++) {
			ytutils::WaitMenuParsers();
			if (IsCanceled()) {
				Cancel();
				return;
			}
			parseRes = ytutils::GetFavLog()->GetNext(key);
			if (parseRes) {
				break;
			}
			CreateVideoButton(workPage, key, i, SCE_NULL);
			workPage->resNum++;
		}

		if (!parseRes) {
			searchParam.hash = utils::GetHash("yt_button_btmenu_right");
			commonWidget = g_rootPage->GetChild(&searchParam, 0);
			commonWidget->PlayEffect(0.0f, effect::EffectType_Fadein1);
		}
	}

	dialog::Close();

	Cancel();
}

SceVoid menu::youtube::FavPage::SearchActionButtonOp()
{
	rco::Element searchParam;
	ui::TextBox *searchBox;
	wstring text16;
	string text8;

	dialog::OpenPleaseWait(g_appPlugin, SCE_NULL, utils::GetString("msg_wait"), SCE_TRUE);

	// In destructor s_current(...)Page->prev is assigned to s_current(...)Page
	while (s_currentFavPage) {
		delete s_currentFavPage;
	}

	searchParam.hash = utils::GetHash("yt_text_box_top_search");
	searchBox = (ui::TextBox *)g_rootPage->GetChild(&searchParam, 0);

	searchBox->GetLabel(&text16);

	if (text16.length() != 0) {

		ccc::UTF16toUTF8(&text16, &text8);

		new FavPage(text8.c_str());
	}
}

// Constructor for first page
menu::youtube::FavPage::FavPage()
{
	prev = SCE_NULL;
	next = SCE_NULL;
	parserThread = SCE_NULL;
	resNum = 0;

	thread::Sleep(100);
	parserThread = new FavParserThread(SCE_KERNEL_DEFAULT_PRIORITY_USER, SCE_KERNEL_256KiB, "NS::YtFavParser");
	parserThread->prevPage = SCE_NULL;
	parserThread->workPage = this;
	parserThread->Start();

	s_currentFavPage = this;
}

// Constructor for next page
menu::youtube::FavPage::FavPage(FavPage *prevPage)
{
	prev = prevPage;
	next = SCE_NULL;
	parserThread = SCE_NULL;
	resNum = 0;

	thread::Sleep(100);
	if (prevPage)
		prev->parserThread->Join();

	parserThread = new FavParserThread(SCE_KERNEL_DEFAULT_PRIORITY_USER, SCE_KERNEL_256KiB, "NS::YtFavParser");
	parserThread->prevPage = prevPage;
	parserThread->workPage = this;
	parserThread->Start();

	s_currentFavPage = this;
}

// Constructor for search page
menu::youtube::FavPage::FavPage(const char *keyWord)
{
	prev = SCE_NULL;
	next = SCE_NULL;
	parserThread = SCE_NULL;
	resNum = 0;

	parserThread = new FavParserThread(SCE_KERNEL_DEFAULT_PRIORITY_USER, SCE_KERNEL_256KiB, "EMPVA::YtFavParser");
	parserThread->prevPage = SCE_NULL;
	parserThread->workPage = this;
	parserThread->keyWord = keyWord;
	parserThread->Start();

	s_currentFavPage = this;
}

menu::youtube::FavPage::~FavPage()
{
	rco::Element searchParam;
	ui::Widget *commonWidget;

	parserThread->Cancel();
	thread::s_mainThreadMutex.Unlock();
	parserThread->Join();
	thread::s_mainThreadMutex.Lock();
	delete parserThread;

	effect::Play(-100.0f, thisPage, effect::EffectType_3D_SlideFromFront, SCE_TRUE, SCE_FALSE);
	if (prev != SCE_NULL) {
		prev->thisPage->PlayEffectReverse(0.0f, effect::EffectType_3D_SlideToBack1);
		prev->thisPage->PlayEffect(0.0f, effect::EffectType_Reset);
		if (prev->thisPage->animationStatus & 0x80)
			prev->thisPage->animationStatus &= ~0x80;
		if (prev->prev != SCE_NULL) {
			prev->prev->thisPage->PlayEffect(0.0f, effect::EffectType_Reset);
			if (prev->prev->thisPage->animationStatus & 0x80)
				prev->prev->thisPage->animationStatus &= ~0x80;
		}
		else {
			searchParam.hash = utils::GetHash("yt_button_btmenu_left");
			commonWidget = g_rootPage->GetChild(&searchParam, 0);
			commonWidget->PlayEffectReverse(0.0f, effect::EffectType_Fadein1);
		}
	}

	searchParam.hash = utils::GetHash("yt_button_btmenu_right");
	commonWidget = g_rootPage->GetChild(&searchParam, 0);
	commonWidget->PlayEffect(0.0f, effect::EffectType_Fadein1);

	s_currentFavPage = prev;
}

SceVoid menu::youtube::FavPage::LeftButtonOp()
{
	delete s_currentFavPage;
}

SceVoid menu::youtube::FavPage::RightButtonOp()
{
	new FavPage(s_currentFavPage);
}

SceVoid menu::youtube::FavPage::TermOp()
{
	rco::Element searchParam;
	ui::Widget *commonWidget;

	// In destructor s_current(...)Page->prev is assigned to s_current(...)Page
	while (s_currentFavPage) {
		delete s_currentFavPage;
	}

	searchParam.hash = utils::GetHash("yt_button_btmenu_right");
	commonWidget = g_rootPage->GetChild(&searchParam, 0);
	commonWidget->PlayEffectReverse(0.0f, effect::EffectType_Fadein1);
}
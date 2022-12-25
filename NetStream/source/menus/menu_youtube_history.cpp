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

static menu::youtube::HistoryPage *s_currentHistoryPage = SCE_NULL;

SceVoid menu::youtube::HistoryParserThread::CreateVideoButton(HistoryPage *page, const char *data, SceUInt32 index)
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

	searchParam.hash = utils::GetHash("youtube_scroll_box");
	box = page->thisPage->GetChild(&searchParam, 0);

	searchParam.hash = utils::GetHash("menu_template_youtube_result_button");
	thread::s_mainThreadMutex.Lock();
	g_appPlugin->TemplateOpen(box, &searchParam, &tmpParam);
	thread::s_mainThreadMutex.Unlock();

	button = (ui::ImageButton *)box->GetChild(box->childNum - 1);

	searchParam.hash = utils::GetHash("yt_text_button_subtext");
	subtext = button->GetChild(&searchParam, 0);

	res = invParseVideo(data, &vidInfo);
	if (res != 1)
		return;

	//TODO: playlist
	/*
	if (vidInfo->playlist.videos.size()) {

		sce_paf_memset(vidCount, 0, sizeof(vidCount));

		text8 = vidInfo->playlist.title.c_str();
		ccc::UTF8toUTF16(&text8, &title16);

		sce_paf_snprintf(vidCount, sizeof(vidCount), "Playlist, %d videos  ", vidInfo->playlist.videos.size());
		text8 = vidCount;
		text8 += vidInfo->playlist.author_name.c_str();
		ccc::UTF8toUTF16(&text8, &subtext16);

		sce_paf_strncpy(tmb, vidInfo->playlist.videos[0].thumbnail_url.c_str(), sizeof(tmb));
	}
	else
	*/
	{
		text8 = vidInfo->title;
		ccc::UTF8toUTF16(&text8, &title16);

		utils::ConvertSecondsToString(&text8, (SceUInt64)vidInfo->lengthSec, SCE_FALSE);
		text8 += "  |  ";
		text8 += vidInfo->author;
		text8 += "  |  ";
		text8 += vidInfo->published;
		ccc::UTF8toUTF16(&text8, &subtext16);
	}

	thread::s_mainThreadMutex.Lock();
	buttonCB = new VideoButtonCB;
	buttonCB->pUserData = buttonCB;
	buttonCB->mode = menu::youtube::Base::Mode_History;
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

SceVoid menu::youtube::HistoryParserThread::EntryFunction()
{
	rco::Element searchParam;
	Plugin::TemplateInitParam tmpParam;
	char *entryData;

	ytutils::GetHistLog()->Reset();
	SceInt32 totalNum = ytutils::GetHistLog()->GetSize();

	entryData = (char *)sce_paf_calloc(totalNum, SCE_INI_FILE_PROCESSOR_KEY_BUFFER_SIZE);

	for (SceInt32 i = 0; i < totalNum; i++) {
		ytutils::GetHistLog()->GetNext(entryData + (i * SCE_INI_FILE_PROCESSOR_KEY_BUFFER_SIZE));
	}

	searchParam.hash = utils::GetHash("menu_template_youtube");
	g_appPlugin->TemplateOpen(g_root, &searchParam, &tmpParam);

	searchParam.hash = utils::GetHash("plane_youtube_bg");
	workPage->thisPage = (ui::Plane *)g_root->GetChild(&searchParam, 0);
	workPage->thisPage->elem.hash = (SceUInt32)workPage->thisPage;

	workPage->thisPage->PlayEffect(-5000.0f, effect::EffectType_3D_SlideFromFront);
	if (workPage->thisPage->animationStatus & 0x80)
		workPage->thisPage->animationStatus &= ~0x80;

	for (SceInt32 i = totalNum - 1; i != -1; i--) {
		ytutils::WaitMenuParsers();
		if (IsCanceled()) {
			break;
		}
		CreateVideoButton(workPage, entryData + (i * SCE_INI_FILE_PROCESSOR_KEY_BUFFER_SIZE), i);
	}

	sce_paf_free(entryData);

	Cancel();
}

menu::youtube::HistoryPage::HistoryPage()
{
	parserThread = SCE_NULL;

	parserThread = new HistoryParserThread(SCE_KERNEL_DEFAULT_PRIORITY_USER, SCE_KERNEL_256KiB, "EMPVA::YtHistoryParser");
	parserThread->workPage = this;
	parserThread->Start();

	s_currentHistoryPage = this;
}

menu::youtube::HistoryPage::~HistoryPage()
{
	parserThread->Cancel();
	thread::s_mainThreadMutex.Unlock();
	parserThread->Join();
	thread::s_mainThreadMutex.Lock();
	delete parserThread;

	effect::Play(-100.0f, thisPage, effect::EffectType_3D_SlideFromFront, SCE_TRUE, SCE_FALSE);

	s_currentHistoryPage = SCE_NULL;
}

SceVoid menu::youtube::HistoryPage::TermOp()
{
	delete s_currentHistoryPage;
}
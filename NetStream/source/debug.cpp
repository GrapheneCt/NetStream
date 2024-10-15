#ifdef _DEBUG

#include <kernel.h>
#include <libdbg.h>
#include <paf.h>
#include <libdbg.h>

#include "common.h"
#include "debug.h"

static int32_t s_oldMemSize = 0;
static int32_t s_oldGraphMemSize = 0;
static uint32_t s_iter = 0;

static ui::Scene *s_currentPage = NULL;
static Plugin *s_currentPlugin = NULL;
static ui::Text *s_debugText = NULL;

class PadListener : public inputdevice::InputListener
{
public:

	PadListener() : InputListener(inputdevice::DEVICE_TYPE_PAD)
	{

	}

	void OnUpdate(inputdevice::Data *data)
	{
#define PRESSED(x) ((data->m_pad_data->paddata & x) && !(data->m_pad_data_pre->paddata & x))
		if (PRESSED(inputdevice::pad::Data::PAD_SELECT) && s_currentPlugin && s_currentPage)
		{
			if (s_currentPage->GetDebugMode() != ui::Scene::DEBUG_OFF)
			{
				SetPageDebugMode(s_currentPlugin, s_currentPage->GetName(), Plugin::PageDebugMode_PageVisualEdit, false);
			}
			else
			{
				SetPageDebugMode(s_currentPlugin, s_currentPage->GetName(), Plugin::PageDebugMode_PageVisualEdit, true);
			}
		}
#undef PRESSED
	}
};

void DebugFocusCB(ui::Widget* pre, ui::Widget* curr, void *data)
{
	if (pre)
	{
		wchar_t buf[256];
		math::v4 pos;
		math::v4 *size;
		math::v4 off;
		pos = pre->GetPos(0, NULL, off);
		size = pre->GetSize(0);
		IDParam name = pre->GetName();
		if (!name.GetID().empty())
		{
			wstring nstr;
			common::Utf8ToUtf16(name.GetID(), &nstr);
			sce_paf_swprintf(buf, 256, L"%s\nx: %.2f\ny: %.2f\nwidth: %.2f\nheight: %.2f", nstr.c_str(), pos.x().f(), pos.y().f(), size->x().f(), size->y().f());
		}
		else
		{
			sce_paf_swprintf(buf, 256, L"0x%08X\nx: %.2f\ny: %.2f\nwidth: %.2f\nheight: %.2f", name.GetIDHash(), pos.x().f(), pos.y().f(), size->x().f(), size->y().f());
		}
		s_debugText->SetString(buf);
	}
}

void LeakTestTask(void *pUserData)
{
	int32_t sz = 0;
	int32_t delta = 0;
	string str;

#ifdef DEBUG_MEM_HEAP
	memory::HeapAllocator *glAlloc = memory::GetGlobalHeapAllocator();
	sz = glAlloc->GetFreeSize();

	str.clear();
	str = common::FormatBytesize(sz, 0);
	sceClibPrintf("[NS_DEBUG] Free heap memory: %s\n", str.c_str());

	delta = s_oldMemSize - sz;
	delta = -delta;
	if (delta) {
		sceClibPrintf("[NS_DEBUG] Memory delta: %d bytes\n", delta);
	}
	s_oldMemSize = sz;
#endif

#ifdef DEBUG_SURFACE_HEAP

	sz = gutil::GetDefaultSurfacePool()->GetFreeSize();

	str.clear();
	str = common::FormatBytesize(sz, 0);
	sceClibPrintf("[NS_DEBUG] Free graphics pool: %s\n", str.c_str());

	delta = s_oldGraphMemSize - sz;
	delta = -delta;
	if (delta) {
		sceClibPrintf("[NS_DEBUG] Graphics pool delta: %d bytes\n", delta);
	}
	s_oldGraphMemSize = sz;
#endif
}

void JobQueueTestTask(void *pUserData)
{

}

void InitDebug()
{
#ifdef DEBUG_EXTRA_TTY
	SCE_PAF_AUTO_TEST_SET_EXTRA_TTY(sceIoOpen("tty0:", SCE_O_WRONLY, 0));
	SCE_PAF_AUTO_TEST_DUMP("\n\n-----PAF EXTRA TTY DEBUG MODE ON-----\n\n");
#endif

	common::MainThreadCallList::Register(LeakTestTask, NULL);
#ifdef DEBUG_JOB_QUEUE
	common::MainThreadCallList::Register(JobQueueTestTask, NULL);
#endif

	common::SharedPtr<inputdevice::InputListener> listener(new PadListener());
	inputdevice::AddInputListener(listener);
}

void SetCurrentDebugParam(paf::Plugin *plugin, paf::ui::Scene *page)
{
	s_currentPlugin = plugin;
	s_currentPage = page;
}

void SetPageDebugMode(paf::Plugin *plugin, paf::IDParam const& id, paf::Plugin::PageDebugMode mode, bool on)
{
	plugin->SetPageDebugMode(id, mode, on);

	if (on && (mode == paf::Plugin::PageDebugMode::PageDebugMode_PageVisual || mode == paf::Plugin::PageDebugMode::PageDebugMode_PageVisualEdit))
	{
		ui::Scene *page = static_cast<ui::Scene *>(plugin->GetNodeEntity_Widget(id));
		page->SetDebugFocusCB(DebugFocusCB);

		s_debugText = (ui::Text *)plugin->CreateWidget(page->GetDebugCube(), "text", "debug_text", "_common_style_text_copy_dialog_desc");
		s_debugText->SetPos(0.0f, 0.0f, 0.0f);
		s_debugText->SetAlign(ui::Widget::ALIGN_MINUS, ui::Widget::ALIGN_PLUS, ui::Widget::ALIGN_NONE, NULL);
		s_debugText->SetAnchor(ui::Widget::ANCHOR_LEFT, ui::Widget::ANCHOR_TOP, ui::Widget::ANCHOR_CENTER, NULL);
		s_debugText->SetAdjust(ui::Widget::ADJUST_CONTENT, ui::Widget::ADJUST_CONTENT, ui::Widget::ADJUST_CONTENT);
		s_debugText->SetStyleAttribute(graph::TextStyleAttribute::TextStyleAttribute_BackColor, 0, 0, { 0.0f, 0.0f, 0.0f, 1.0f });
		s_debugText->EnableDebugFocus(false);
	}
}

/*
uint32_t oldButtons = 0;
ui::Widget *widg = NULL;

void DirectInputCallback(input::GamePad::GamePadData *pData)
{
	Vector4 opos = widg->GetPosition();

	if (pData->buttons & SCE_PAF_CTRL_UP)
	{
		opos.y += 1;
	}
	else if (pData->buttons & SCE_PAF_CTRL_DOWN)
	{
		opos.y -= 1;
	}
	else if (pData->buttons & SCE_PAF_CTRL_RIGHT)
	{
		opos.x += 1;
	}
	else if (pData->buttons & SCE_PAF_CTRL_LEFT)
	{
		opos.x -= 1;
	}
	else if ((pData->buttons & SCE_PAF_CTRL_CROSS) && !(oldButtons & SCE_PAF_CTRL_CROSS))
	{
		sceClibPrintf("\nx: %f\ny: %f\n", opos.x, opos.y);
	}

	widg->SetPosition(&opos);

	oldButtons = pData->buttons;
}
*/

#endif
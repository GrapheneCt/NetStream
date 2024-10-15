#include <kernel.h>
#include <paf.h>
#include <psp2_compat/curl/curl.h>
#include <ini_file_processor.h>

#include "common.h"
#include "yt_utils.h"
#include "hvdb_utils.h"
#include "hvdb.h"

using namespace paf;
using namespace sce;

static hvdbutils::EntryLog *s_entryLog = NULL;

hvdbutils::EntryLog::EntryLog()
{
	Ini::InitParameter param;
	Ini::MemAllocator alloc;
	alloc.allocate = sce_paf_malloc;
	alloc.deallocate = sce_paf_free;

	param.workmemSize = SCE_KERNEL_4KiB;
	param.unk_0x4 = SCE_KERNEL_4KiB;
	param.allocator = &alloc;

	m_ini = new Ini::IniFileProcessor();
	m_ini->initialize(&param);
	m_ini->open("savedata0:hvdb_entry_log.ini", "rw", 0);
}

void hvdbutils::EntryLog::Clean()
{
	if (s_entryLog)
	{
		delete s_entryLog;
		LocalFile::RemoveFile("savedata0:hvdb_entry_log.ini");
		s_entryLog = new hvdbutils::EntryLog();
	}
}

void hvdbutils::Init()
{
	hvdbInit(sce_paf_malloc, sce_paf_free, NULL);

	if (!s_entryLog)
		s_entryLog = new hvdbutils::EntryLog();
}

void hvdbutils::Flush()
{
	if (s_entryLog)
	{
		s_entryLog->Flush();
	}
}

void hvdbutils::Term()
{
	if (s_entryLog)
	{
		delete s_entryLog;
		s_entryLog = NULL;
	}

	hvdbTerm();
}

hvdbutils::EntryLog *hvdbutils::GetEntryLog()
{
	return s_entryLog;
}

int32_t hvdbutils::EnqueueDownload(const char *url, const char *name)
{
	return ytutils::EnqueueDownload(url, name);
}

int32_t hvdbutils::EnqueueDownloadAsync(const char *url, const char *name)
{
	return ytutils::EnqueueDownloadAsync(url, name);
}
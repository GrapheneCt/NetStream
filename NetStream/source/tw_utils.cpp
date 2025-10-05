#include <kernel.h>
#include <paf.h>
#include <psp2_compat/curl/curl.h>
#include <ini_file_processor.h>

#include "common.h"
#include "tw_utils.h"
#include "lootkit.h"

using namespace paf;
using namespace sce;

static twutils::HistLog *s_histLog = NULL;
static twutils::FavLog *s_favLog = NULL;

int32_t twutils::Log::GetNext(char *data)
{
	int32_t ret;
	char *sptr;
	char val[2];

	ret = m_ini->parse(data, val, sizeof(val));

	if (ret == SCE_OK)
	{
		// Restore '='
		sptr = sce_paf_strchr(data, '}');
		while (sptr) {
			*sptr = '=';
			sptr = sce_paf_strchr(sptr, '}');
		}
	}

	return ret;
}

int32_t twutils::Log::Get(const char *data)
{
	char *sptr;
	char dataCopy[SCE_INI_FILE_PROCESSOR_KEY_BUFFER_SIZE];
	char val[2];

	sce_paf_strncpy(dataCopy, data, SCE_INI_FILE_PROCESSOR_KEY_BUFFER_SIZE);

	// Restore '}'
	sptr = sce_paf_strchr(dataCopy, '=');
	while (sptr)
	{
		*sptr = '}';
		sptr = sce_paf_strchr(sptr, '=');
	}

	return  m_ini->get(dataCopy, val, sizeof(val), 0);
}

void twutils::Log::Flush()
{
	m_ini->flush();
}

int32_t twutils::Log::GetSize()
{
	return m_ini->size();
}

void twutils::Log::Reset()
{
	m_ini->reset();
}

void twutils::Log::Add(const char *data)
{
	char *sptr;
	char dataCopy[SCE_INI_FILE_PROCESSOR_KEY_BUFFER_SIZE];
	sce_paf_strncpy(dataCopy, data, SCE_INI_FILE_PROCESSOR_KEY_BUFFER_SIZE);

	// Replace '=' in playlists with '}' to not confuse INI processor
	sptr = sce_paf_strchr(dataCopy, '=');

	while (sptr)
	{
		*sptr = '}';
		sptr = sce_paf_strchr(sptr, '=');
	}

	m_ini->add(dataCopy, "");
	m_ini->flush();
}

int32_t twutils::Log::AddAsyncJob::Run()
{
	char *sptr;

	// Replace '=' in playlists with '}' to not confuse INI processor
	sptr = sce_paf_strchr(m_data.c_str(), '=');

	while (sptr)
	{
		*sptr = '}';
		sptr = sce_paf_strchr(sptr, '=');
	}

	m_parent->m_ini->add(m_data.c_str(), "");
	m_parent->m_ini->flush();

	return SCE_PAF_OK;
}

void twutils::Log::AddAsync(const char *data)
{
	AddAsyncJob *job = new AddAsyncJob(this, data);
	common::SharedPtr<job::JobItem> itemParam(job);
	job::JobQueue::DefaultQueue()->Enqueue(itemParam);
}

void twutils::Log::Remove(const char *data)
{
	char *sptr;
	char dataCopy[SCE_INI_FILE_PROCESSOR_KEY_BUFFER_SIZE];
	sce_paf_strncpy(dataCopy, data, SCE_INI_FILE_PROCESSOR_KEY_BUFFER_SIZE);

	// Replace '=' in playlists with '}' to not confuse INI processor
	sptr = sce_paf_strchr(dataCopy, '=');

	while (sptr)
	{
		*sptr = '}';
		sptr = sce_paf_strchr(sptr, '=');
	}

	m_ini->del(dataCopy);
	m_ini->flush();
}

twutils::HistLog::HistLog()
{
	uint32_t i = 0;
	char val[2];
	char data[SCE_INI_FILE_PROCESSOR_KEY_BUFFER_SIZE];
	Ini::InitParameter param;
	Ini::MemAllocator alloc;
	alloc.allocate = sce_paf_malloc;
	alloc.deallocate = sce_paf_free;

	param.workmemSize = SCE_KERNEL_4KiB;
	param.unk_0x4 = SCE_KERNEL_4KiB;
	param.allocator = &alloc;

	m_ini = new Ini::IniFileProcessor();
	m_ini->initialize(&param);
	m_ini->open("savedata0:tw_hist_log.ini", "rw", 0);

	i = m_ini->size();
	if (i <= k_maxHistItems)
		return;

	i = i - k_maxHistItems;

	while (i != 0)
	{
		m_ini->parse(data, val, sizeof(val));
		m_ini->del(data);
		i--;
	}

	m_ini->reset();
}

twutils::FavLog::FavLog()
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
	m_ini->open("savedata0:tw_fav_log.ini", "rw", 0);
}

void twutils::FavLog::Clean()
{
	if (s_favLog)
	{
		delete s_favLog;
		LocalFile::RemoveFile("savedata0:tw_fav_log.ini");
		s_favLog = new twutils::FavLog();
	}
}

void twutils::HistLog::Clean()
{
	if (s_histLog)
	{
		delete s_histLog;
		LocalFile::RemoveFile("savedata0:tw_hist_log.ini");
		s_histLog = new twutils::HistLog();
	}
}

void twutils::Init()
{
	ltkInit(sce_paf_malloc, sce_paf_free, "https://github.com/GrapheneCt/NetStream/raw/main/loot.bin");

	/*
	LtkItem *ch = NULL;
	LtkItemVideo *vid = NULL;
	int32_t ret = 0;
	char dummy[1024];

	ret = ltkParseSearch("", NULL, LTK_SEARCH_TYPE_CHANNEL, LTK_SEARCH_DIR_AFTER, &ch);
	sceClibPrintf("ltkParseSearch: 0x%08X\n", ret);
	ltkParseVideo(ch[0].channelItem, LTK_VIDEO_TYPE_VOD, NULL, LTK_SEARCH_DIR_AFTER, &vid);
	ltkGetVideoUrl(&vid[0], LTK_HLS_QUALITY_360P, dummy, sizeof(dummy));
	sceClibPrintf("video url: %s\n", dummy);
	*/

	if (!s_histLog)
		s_histLog = new twutils::HistLog();
	if (!s_favLog)
		s_favLog = new twutils::FavLog();
}

void twutils::Flush()
{
	if (s_histLog)
	{
		s_histLog->Flush();
	}
	if (s_favLog)
	{
		s_favLog->Flush();
	}
}

void twutils::Term()
{
	if (s_histLog)
	{
		delete s_histLog;
		s_histLog = NULL;
	}
	if (s_favLog)
	{
		delete s_favLog;
		s_favLog = NULL;
	}

	ltkTerm();
}

twutils::HistLog *twutils::GetHistLog()
{
	return s_histLog;
}

twutils::FavLog *twutils::GetFavLog()
{
	return s_favLog;
}
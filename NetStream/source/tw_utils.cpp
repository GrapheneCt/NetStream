#include <kernel.h>
#include <paf.h>
#include <psp2_compat/curl/curl.h>
#include <ini_file_processor.h>

#include "common.h"
#include "tw_utils.h"
#include "lootkit.h"

using namespace paf;
using namespace sce;

static twutils::HistLog *s_histLog = SCE_NULL;
static twutils::FavLog *s_favLog = SCE_NULL;

SceInt32 twutils::Log::GetNext(char *data)
{
	SceInt32 ret;
	char *sptr;
	char val[2];

	ret = ini->parse(data, val, sizeof(val));

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

SceInt32 twutils::Log::Get(const char *data)
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

	return  ini->get(dataCopy, val, sizeof(val), 0);
}

SceVoid twutils::Log::Flush()
{
	ini->flush();
}

SceInt32 twutils::Log::GetSize()
{
	return ini->size();
}

SceVoid twutils::Log::Reset()
{
	ini->reset();
}

SceVoid twutils::Log::Add(const char *data)
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

	ini->add(dataCopy, "");
	ini->flush();
}

SceVoid twutils::Log::AddAsyncJob::Run()
{
	char *sptr;

	// Replace '=' in playlists with '}' to not confuse INI processor
	sptr = sce_paf_strchr(data.c_str(), '=');

	while (sptr)
	{
		*sptr = '}';
		sptr = sce_paf_strchr(sptr, '=');
	}

	workObj->ini->add(data.c_str(), "");
	workObj->ini->flush();
}

SceVoid twutils::Log::AddAsync(const char *data)
{
	AddAsyncJob *job = new AddAsyncJob("utils::AddAsyncJob");
	job->workObj = this;
	job->data = data;
	SharedPtr<job::JobItem> itemParam(job);
	job::s_defaultJobQueue->Enqueue(&itemParam);
}

SceVoid twutils::Log::Remove(const char *data)
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

	ini->del(dataCopy);
	ini->flush();
}

twutils::HistLog::HistLog()
{
	SceUInt32 i = 0;
	char val[2];
	char data[SCE_INI_FILE_PROCESSOR_KEY_BUFFER_SIZE];
	Ini::InitParameter param;
	Ini::MemAllocator alloc;
	alloc.allocate = sce_paf_malloc;
	alloc.deallocate = sce_paf_free;

	param.workmemSize = SCE_KERNEL_4KiB;
	param.unk_0x4 = SCE_KERNEL_4KiB;
	param.allocator = &alloc;

	ini = new Ini::IniFileProcessor();
	ini->initialize(&param);
	ini->open("savedata0:tw_hist_log.ini", "rw", 0);

	i = ini->size();
	if (i <= k_maxHistItems)
		return;

	i = i - k_maxHistItems;

	while (i != 0)
	{
		ini->parse(data, val, sizeof(val));
		ini->del(data);
		i--;
	}

	ini->reset();
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

	ini = new Ini::IniFileProcessor();
	ini->initialize(&param);
	ini->open("savedata0:tw_fav_log.ini", "rw", 0);
}

SceVoid twutils::FavLog::Clean()
{
	if (s_favLog)
	{
		delete s_favLog;
		sceIoRemove("savedata0:tw_fav_log.ini");
		s_favLog = new twutils::FavLog();
	}
}

SceVoid twutils::HistLog::Clean()
{
	if (s_histLog)
	{
		delete s_histLog;
		sceIoRemove("savedata0:tw_hist_log.ini");
		s_histLog = new twutils::HistLog();
	}
}

SceVoid twutils::Init()
{
	ltkInit(sce_paf_malloc, sce_paf_free, "https://github.com/GrapheneCt/NetStream/raw/main/loot.bin");

	/*
	LtkItem *ch = SCE_NULL;
	LtkItemVideo *vid = SCE_NULL;
	SceInt32 ret = 0;
	char dummy[1024];

	ret = ltkParseSearch("", SCE_NULL, LTK_SEARCH_TYPE_CHANNEL, LTK_SEARCH_DIR_AFTER, &ch);
	sceClibPrintf("ltkParseSearch: 0x%08X\n", ret);
	ltkParseVideo(ch[0].channelItem, LTK_VIDEO_TYPE_VOD, SCE_NULL, LTK_SEARCH_DIR_AFTER, &vid);
	ltkGetVideoUrl(&vid[0], LTK_HLS_QUALITY_360P, dummy, sizeof(dummy));
	sceClibPrintf("video url: %s\n", dummy);
	*/

	if (!s_histLog)
		s_histLog = new twutils::HistLog();
	if (!s_favLog)
		s_favLog = new twutils::FavLog();
}

SceVoid twutils::Flush()
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

SceVoid twutils::Term()
{
	if (s_histLog)
	{
		delete s_histLog;
		s_histLog = SCE_NULL;
	}
	if (s_favLog)
	{
		delete s_favLog;
		s_favLog = SCE_NULL;
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
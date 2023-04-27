#include <kernel.h>
#include <paf.h>
#include <psp2_compat/curl/curl.h>
#include <ini_file_processor.h>

#include "common.h"
#include "downloader.h"
#include "yt_utils.h"
#include "invidious.h"

using namespace paf;
using namespace sce;

static ytutils::HistLog *s_histLog = NULL;
static ytutils::FavLog *s_favLog = NULL;
static Downloader *s_downloader = NULL;

int32_t ytutils::Log::GetNext(char *data)
{
	int32_t ret;
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

int32_t ytutils::Log::Get(const char *data)
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

void ytutils::Log::Flush()
{
	ini->flush();
}

int32_t ytutils::Log::GetSize()
{
	return ini->size();
}

void ytutils::Log::Reset()
{
	ini->reset();
}

void ytutils::Log::Add(const char *data)
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

void ytutils::Log::AddAsyncJob::Run()
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

void ytutils::Log::AddAsync(const char *data)
{
	AddAsyncJob *job = new AddAsyncJob("utils::AddAsyncJob");
	job->workObj = this;
	job->data = data;
	common::SharedPtr<job::JobItem> itemParam(job);
	job::JobQueue::default_queue->Enqueue(itemParam);
}

void ytutils::Log::Remove(const char *data)
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

ytutils::HistLog::HistLog()
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

	ini = new Ini::IniFileProcessor();
	ini->initialize(&param);
	ini->open("savedata0:yt_hist_log.ini", "rw", 0);

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

ytutils::FavLog::FavLog()
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
	ini->open("savedata0:yt_fav_log.ini", "rw", 0);
}

void ytutils::FavLog::Clean()
{
	if (s_favLog)
	{
		delete s_favLog;
		LocalFile::RemoveFile("savedata0:yt_fav_log.ini");
		s_favLog = new ytutils::FavLog();
	}
}

void ytutils::HistLog::Clean()
{
	if (s_histLog)
	{
		delete s_histLog;
		LocalFile::RemoveFile("savedata0:yt_hist_log.ini");
		s_histLog = new ytutils::HistLog();
	}
}

void ytutils::Init()
{
	invInit(sce_paf_malloc, sce_paf_free, NULL);

	if (!s_histLog)
		s_histLog = new ytutils::HistLog();
	if (!s_favLog)
		s_favLog = new ytutils::FavLog();

	s_downloader = new Downloader();
}

void ytutils::Flush()
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

void ytutils::Term()
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

	invTerm();
}

ytutils::HistLog *ytutils::GetHistLog()
{
	return s_histLog;
}

ytutils::FavLog *ytutils::GetFavLog()
{
	return s_favLog;
}

int32_t ytutils::EnqueueDownload(const char *url, const char *name)
{
	return s_downloader->Enqueue(url, name);
}

int32_t ytutils::EnqueueDownloadAsync(const char *url, const char *name, Downloader::OnStartCallback cb)
{
	return s_downloader->EnqueueAsync(url, name, cb);
}
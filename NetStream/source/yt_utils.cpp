#include <kernel.h>
#include <paf.h>
#include <psp2_compat/curl/curl.h>
#include <ini_file_processor.h>

#include "common.h"
#include "downloader.h"
#include "yt_utils.h"
#include "np_utils.h"
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

	return  m_ini->get(dataCopy, val, sizeof(val), 0);
}

void ytutils::Log::Flush()
{
	m_ini->flush();
}

int32_t ytutils::Log::GetSize()
{
	return m_ini->size();
}

void ytutils::Log::Reset()
{
	m_ini->reset();
}

void ytutils::Log::Add(const char *data)
{
	int32_t sync = 0;
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

	sce::AppSettings *settings = menu::Settings::GetAppSetInstance();
	settings->GetInt("cloud_sync_auto", static_cast<int32_t *>(&sync), 0);
	if (sync)
	{
		UploadToTUS();
	}
}

void ytutils::Log::AddAsyncJob::Run()
{
	int32_t sync = 0;
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

	sce::AppSettings *settings = menu::Settings::GetAppSetInstance();
	settings->GetInt("cloud_sync_auto", static_cast<int32_t *>(&sync), 0);
	if (sync)
	{
		m_parent->UploadToTUS();
	}
}

void ytutils::Log::AddAsync(const char *data)
{
	AddAsyncJob *job = new AddAsyncJob(this, data);
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

	m_ini->del(dataCopy);
	m_ini->flush();
}

int32_t ytutils::Log::UpdateFromTUS()
{
	return SCE_OK;
}

int32_t ytutils::Log::UploadToTUS()
{
	return SCE_OK;
}

ytutils::HistLog::HistLog(uint32_t tus) : Log(tus)
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
	m_ini->open("savedata0:yt_hist_log.ini", "rw", 0);

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

int32_t ytutils::HistLog::UpdateFromTUS()
{
	int32_t ret = SCE_ERROR_ERRNO_ECONNREFUSED;
	uint32_t tus = m_tus;

	if (!nputils::IsAllGreen())
	{
		return ret;
	}

	if (tus == UINT_MAX)
	{
		return ret;
	}

	ret = nputils::GetTUS()->DownloadFile(tus, "savedata0:yt_hist_log_tus.ini");
	if (ret < 0)
	{
		LocalFile::RemoveFile("savedata0:yt_hist_log_tus.ini");
		return UploadToTUS();
	}

	m_ini->close();
	m_ini->terminate();
	delete m_ini;

	LocalFile::RemoveFile("savedata0:yt_hist_log.ini");
	LocalFile::RenameFile("savedata0:yt_hist_log_tus.ini", "savedata0:yt_hist_log.ini");

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
	m_ini->open("savedata0:yt_hist_log.ini", "rw", 0);

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

int32_t ytutils::HistLog::UploadToTUS()
{
	int32_t ret = SCE_ERROR_ERRNO_ECONNREFUSED;
	uint32_t tus = m_tus;

	if (!nputils::IsAllGreen())
	{
		return ret;
	}

	if (tus == UINT_MAX)
	{
		return ret;
	}

	m_ini->flush();

	ret = nputils::GetTUS()->UploadFile(tus, "savedata0:yt_hist_log.ini");

	return ret;
}

ytutils::FavLog::FavLog(uint32_t tus) : Log(tus)
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
	m_ini->open("savedata0:yt_fav_log.ini", "rw", 0);
}

int32_t ytutils::FavLog::UpdateFromTUS()
{
	int32_t ret = SCE_ERROR_ERRNO_ECONNREFUSED;
	uint32_t tus = m_tus;

	if (!nputils::IsAllGreen())
	{
		return ret;
	}

	if (tus == UINT_MAX)
	{
		return ret;
	}

	ret = nputils::GetTUS()->DownloadFile(tus, "savedata0:yt_fav_log_tus.ini");
	if (ret < 0)
	{
		LocalFile::RemoveFile("savedata0:yt_fav_log_tus.ini");
		return UploadToTUS();
	}

	m_ini->close();
	m_ini->terminate();
	delete m_ini;

	LocalFile::RemoveFile("savedata0:yt_fav_log.ini");
	LocalFile::RenameFile("savedata0:yt_fav_log_tus.ini", "savedata0:yt_fav_log.ini");

	Ini::InitParameter param;
	Ini::MemAllocator alloc;
	alloc.allocate = sce_paf_malloc;
	alloc.deallocate = sce_paf_free;

	param.workmemSize = SCE_KERNEL_4KiB;
	param.unk_0x4 = SCE_KERNEL_4KiB;
	param.allocator = &alloc;

	m_ini = new Ini::IniFileProcessor();
	m_ini->initialize(&param);
	m_ini->open("savedata0:yt_fav_log.ini", "rw", 0);
}

int32_t ytutils::FavLog::UploadToTUS()
{
	int32_t ret = SCE_ERROR_ERRNO_ECONNREFUSED;
	uint32_t tus = m_tus;

	if (!nputils::IsAllGreen())
	{
		return ret;
	}

	if (tus == UINT_MAX)
	{
		return ret;
	}

	m_ini->flush();

	ret = nputils::GetTUS()->UploadFile(tus, "savedata0:yt_fav_log.ini");

	return ret;
}

void ytutils::FavLog::Clean()
{
	if (s_favLog)
	{
		uint32_t oldTus = s_favLog->m_tus;
		if (nputils::IsAllGreen())
		{
			nputils::GetTUS()->UploadString(s_favLog->m_tus, " ");
		}
		delete s_favLog;
		LocalFile::RemoveFile("savedata0:yt_fav_log.ini");
		s_favLog = new ytutils::FavLog(oldTus);
	}
}

void ytutils::HistLog::Clean()
{
	if (s_histLog)
	{
		uint32_t oldTus = s_histLog->m_tus;
		if (nputils::IsAllGreen())
		{
			nputils::GetTUS()->UploadString(s_histLog->m_tus, " ");
		}
		delete s_histLog;
		LocalFile::RemoveFile("savedata0:yt_hist_log.ini");
		s_histLog = new ytutils::HistLog(oldTus);
	}
}

void ytutils::Init(uint32_t histTUS, uint32_t favTUS)
{
	invInit(sce_paf_malloc, sce_paf_free, NULL);

	if (!s_histLog)
		s_histLog = new ytutils::HistLog(histTUS);
	if (!s_favLog)
		s_favLog = new ytutils::FavLog(favTUS);

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
	return s_downloader->Enqueue(g_appPlugin, url, name);
}

int32_t ytutils::EnqueueDownloadAsync(const char *url, const char *name)
{
	return s_downloader->EnqueueAsync(g_appPlugin, url, name);
}
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

static ytutils::HistLog *s_histLog = SCE_NULL;
static ytutils::FavLog *s_favLog = SCE_NULL;
static CURL *s_curl = SCE_NULL;
static Downloader *s_downloader = SCE_NULL;

SceSize ytutils::InvDownloadCore(char *buffer, SceSize size, SceSize nitems, ScePVoid userdata)
{
	InvDownloadData *dw = (InvDownloadData *)userdata;
	SceSize toCopy = size * nitems;

	if (toCopy != 0) {
		dw->buf = sce_paf_realloc(dw->buf, dw->pos + toCopy);
		sce_paf_memcpy(dw->buf + dw->pos, buffer, toCopy);
		dw->pos += toCopy;
	}

	return toCopy;
}

SceBool ytutils::InvDownload(char *url, ScePVoid *ppBuf, SceSize *pBufSize)
{
	InvDownloadData dw;
	dw.buf = SCE_NULL;
	dw.pos = 0;

	curl_easy_setopt(s_curl, CURLOPT_URL, url);
	curl_easy_setopt(s_curl, CURLOPT_WRITEDATA, &dw);

	if (curl_easy_perform(s_curl) != CURLE_OK)
	{
		sce_paf_free(dw.buf);
		return SCE_FALSE;
	}

	*ppBuf = dw.buf;
	*pBufSize = dw.pos;

	return SCE_TRUE;
}

SceInt32 ytutils::Log::GetNext(char *data)
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

SceInt32 ytutils::Log::Get(const char *data)
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

SceVoid ytutils::Log::Flush()
{
	ini->flush();
}

SceInt32 ytutils::Log::GetSize()
{
	return ini->size();
}

SceVoid ytutils::Log::Reset()
{
	ini->reset();
}

SceVoid ytutils::Log::Add(const char *data)
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

SceVoid ytutils::Log::AddAsyncJob::Run()
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

SceVoid ytutils::Log::AddAsync(const char *data)
{
	AddAsyncJob *job = new AddAsyncJob("utils::AddAsyncJob");
	job->workObj = this;
	job->data = data;
	SharedPtr<job::JobItem> itemParam(job);
	job::s_defaultJobQueue->Enqueue(&itemParam);
}

SceVoid ytutils::Log::Remove(const char *data)
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

SceVoid ytutils::FavLog::Clean()
{
	if (s_favLog)
	{
		delete s_favLog;
		sceIoRemove("savedata0:yt_fav_log.ini");
		s_favLog = new ytutils::FavLog();
	}
}

SceVoid ytutils::HistLog::Clean()
{
	if (s_histLog)
	{
		delete s_histLog;
		sceIoRemove("savedata0:yt_hist_log.ini");
		s_histLog = new ytutils::HistLog();
	}
}

SceVoid ytutils::Init()
{
	invInit(sce_paf_malloc, sce_paf_free, ytutils::InvDownload);

	if (!s_histLog)
		s_histLog = new ytutils::HistLog();
	if (!s_favLog)
		s_favLog = new ytutils::FavLog();
	if (!s_curl)
	{
		s_curl = curl_easy_init();
		curl_easy_setopt(s_curl, CURLOPT_WRITEFUNCTION, InvDownloadCore);
		curl_easy_setopt(s_curl, CURLOPT_USERAGENT, USER_AGENT);
		curl_easy_setopt(s_curl, CURLOPT_ACCEPT_ENCODING, "");
		curl_easy_setopt(s_curl, CURLOPT_SSL_VERIFYHOST, 0L);
		curl_easy_setopt(s_curl, CURLOPT_SSL_VERIFYPEER, 0L);
		curl_easy_setopt(s_curl, CURLOPT_FOLLOWLOCATION, 1L);
		curl_easy_setopt(s_curl, CURLOPT_TCP_KEEPALIVE, 1L);
		curl_easy_setopt(s_curl, CURLOPT_NOPROGRESS, 1L);
		curl_easy_setopt(s_curl, CURLOPT_CONNECTTIMEOUT, 5L);
		curl_easy_setopt(s_curl, CURLOPT_TIMEOUT, 7L);
	}

	s_downloader = new Downloader();
}

SceVoid ytutils::Flush()
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

SceVoid ytutils::Term()
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
	if (s_curl)
	{
		curl_easy_cleanup(s_curl);
		s_curl = SCE_NULL;
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

SceInt32 ytutils::EnqueueDownload(const char *url, const char *name)
{
	return s_downloader->Enqueue(url, name);
}
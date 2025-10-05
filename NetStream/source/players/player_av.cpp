#include <kernel.h>
#include <kernel/dmacmgr.h>
#include <paf.h>
#include <libdbg.h>
#include <appmgr.h>
#include <libsysmodule.h>
#include <sceavplayer_webmaf.h>
#include <avcdec.h>
#include <audioout.h>

#include "common.h"
#include "utils.h"
#include "event.h"
#include "players/player_av.h"

#undef SCE_DBG_LOG_COMPONENT
#define SCE_DBG_LOG_COMPONENT "[AV]"

using namespace paf;

static const char *k_supportedExtensions[] = {
		".ts",
		".mpg",
		".m2ts",
		".mp4",
		".m4v",
		".m4a",
		".mpd"
};

AVPlayer::SmoothStreaming *AVPlayer::SmoothStreaming::s_vodObjStorage = NULL;

AVPlayer::SmoothStreaming::SmoothStreaming()
{
	m_curl = NULL;
	m_tmpBuf = NULL;
	m_total = 0;
	m_lastError = 200;
	m_prVod = NULL;

	m_curl = curl_easy_init();
	//curl_easy_setopt(m_curl, CURLOPT_SHARE, s_fileCurlSh);
	curl_easy_setopt(m_curl, CURLOPT_BUFFERSIZE, SCE_KERNEL_64KiB);
	curl_easy_setopt(m_curl, CURLOPT_WRITEFUNCTION, CurlDownload);
	curl_easy_setopt(m_curl, CURLOPT_USERAGENT, USER_AGENT);
	curl_easy_setopt(m_curl, CURLOPT_ACCEPT_ENCODING, "");
	curl_easy_setopt(m_curl, CURLOPT_SSL_VERIFYHOST, 0L);
	curl_easy_setopt(m_curl, CURLOPT_SSL_VERIFYPEER, 0L);
	curl_easy_setopt(m_curl, CURLOPT_FOLLOWLOCATION, 1L);
	curl_easy_setopt(m_curl, CURLOPT_TCP_KEEPALIVE, 1L);
	curl_easy_setopt(m_curl, CURLOPT_NOPROGRESS, 1L);
	curl_easy_setopt(m_curl, CURLOPT_WRITEDATA, this);
	curl_easy_setopt(m_curl, CURLOPT_CONNECTTIMEOUT, 5L);
	curl_easy_setopt(m_curl, CURLOPT_TIMEOUT, 15L);
	if (utils::GetGlobalProxy())
	{
		curl_easy_setopt(m_curl, CURLOPT_PROXY, utils::GetGlobalProxy());
	}

	m_tmpBuf = static_cast<uint8_t *>(sce_paf_malloc(SCE_KERNEL_4MiB));
	sce_paf_memset(m_tmpBuf, 0, SCE_KERNEL_4MiB);
}

AVPlayer::SmoothStreaming::~SmoothStreaming()
{
	if (m_curl)
	{
		curl_easy_cleanup(m_curl);
	}
	if (m_tmpBuf)
	{
		sce_paf_free(m_tmpBuf);
	}
}

void AVPlayer::SmoothStreaming::GetLiveReplacement(SceAvPlayerHttpReplacement *repl)
{
	if (!repl)
	{
		return;
	}

	repl->init =
	[](SceAvPlayerHttpInstance *instancePP, void *objPtr, SceAvPlayerHTTPAllocate alloc,
		SceAvPlayerHTTPDeallocate dealloc, char *userAgent, SceAvPlayerHTTPCtx *httpCtxId)
	{
		SmoothStreaming *client = new SmoothStreaming();
		*instancePP = static_cast<void *>(client);
		return SCE_OK;
	};
	repl->deInit =
	[](SceAvPlayerHttpInstance *instancePP)
	{
		if (!instancePP || !*instancePP)
		{
			return -1;
		}
		SmoothStreaming *client = static_cast<SmoothStreaming *>(*instancePP);
		delete client;
		return SCE_OK;
	};
	repl->post =
	[](SceAvPlayerHttpInstance instanceP, char *url, bool continuation,
		SceAvPlayerHTTPHeader *headers, uint8_t numHeaders, uint8_t *body, uint32_t len,
		uint8_t *buf, uint32_t bufLen, bool *completed)
	{
		return (static_cast<SmoothStreaming *>(instanceP))->LivePostImpl(url, continuation, headers, numHeaders, body, len, buf, bufLen, completed);
	};
	repl->get =
	[](SceAvPlayerHttpInstance instanceP, char *url, bool continuation,
		uint64_t rangeStart, uint64_t rangeEnd, uint8_t *buf, uint32_t bufLen, bool *completed)
	{
		return (static_cast<SmoothStreaming *>(instanceP))->LiveGetImpl(url, continuation, rangeStart, rangeEnd, buf, bufLen, completed);
	};
	repl->setRedirectCb = [](SceAvPlayerHttpInstance instanceP, SceAvPlayerHttpRedirectCb *redirectCb, void *object) { return SCE_OK; };
	repl->setCert = [](SceAvPlayerHttpInstance instanceP, SceAvPlayerHTTPSData *cert, SceAvPlayerHTTPSData *privKey) { return SCE_OK; };
	repl->setRecvTimeout = [](SceAvPlayerHttpInstance instanceP, int64_t usec) { return SCE_OK; };
	repl->getLastStatus = [](SceAvPlayerHttpInstance instanceP) { return (static_cast<SmoothStreaming *>(instanceP))->GetLastStatusImpl(); };

}

void AVPlayer::SmoothStreaming::GetVODReplacement(SceAvPlayerFileReplacement *repl)
{
	if (!repl)
	{
		return;
	}

	repl->open =
	[](void *argP, const char *argFilename)
	{
		SmoothStreaming *client = new SmoothStreaming();
		SmoothStreaming **instancePP = static_cast<SmoothStreaming **>(argP);
		*instancePP = client;
		return client->VodOpenFileImpl(argFilename);
	};
	repl->close =
	[](void *argP)
	{
		SmoothStreaming *client = *(static_cast<SmoothStreaming **>(argP));
		int32_t ret = client->VodCloseFileImpl();
		delete client;
		return ret;
	};
	repl->readOffset = [](void *argP, uint8_t *argBuffer, uint64_t argPosition, uint32_t argLength) { return (*(static_cast<SmoothStreaming **>(argP)))->VodReadOffsetFileImpl(argBuffer, argPosition, argLength); };
	repl->size = [](void *argP) { return (*(static_cast<SmoothStreaming **>(argP)))->VodSizeFileImpl(); };
	repl->objectPointer = &s_vodObjStorage;
}

size_t AVPlayer::SmoothStreaming::CurlDownload(char *buffer, size_t size, size_t nitems, void *userdata)
{
	SmoothStreaming *obj = static_cast<SmoothStreaming *>(userdata);
	size_t toWrite = size * nitems;

	if (toWrite != 0)
	{
		if (obj->m_total + toWrite >= SCE_KERNEL_4MiB)
		{
			SCE_DBG_LOG_ERROR("[CurlDownload] tmp buffer is too small!");
			return 0;
		}
		sce_paf_memcpy(obj->m_tmpBuf + obj->m_total, buffer, toWrite);
		obj->m_total += toWrite;
	}

	return toWrite;
}

int64_t AVPlayer::SmoothStreaming::LivePostImpl(char *url, bool continuation,
	SceAvPlayerHTTPHeader *headers, uint8_t numHeaders, uint8_t *body, uint32_t len,
	uint8_t *buf, uint32_t bufLen, bool *completed)
{
	*completed = false;

	return -1;
}

/*
Notes on behaviour:

*completed = false
if(!continuation)
{
	if (rangeStart || rangeEnd)
	{
		setup ranged request
		return ranged request size in bytes
	}
	else
	{
		return content-length or -1 if it is unknown, but last status should be set to 200
	}
}
else
{
	if (ranged request)
	{
		readBytes = read(buf, bufLen);
		*completed = true
		return readBytes
	}
	else
	{
		readBytes = read(buf, bufLen);

		if (eof)
		{
			*completed = true
		}

		return readBytes
	}
}

Note that if size is unknown, next calls with continuation=true will come with arbitrary 4 KiB bufLen, expecting to read the file in chunks until EOF (as if it is normal local file)

*/
int64_t AVPlayer::SmoothStreaming::LiveGetImpl(
	char *url,
	bool continuation,
	uint64_t rangeStart,
	uint64_t rangeEnd,
	uint8_t *buf,
	uint32_t bufLen,
	bool *completed)
{
	*completed = false;

	SCE_DBG_LOG_TRACE("[LiveGetImpl] LiveGetImpl(): try read %d, %llu-%llu (%llu), %u, 0x%08X", continuation, rangeStart, rangeEnd, rangeEnd - rangeStart, bufLen, buf);

	if (!continuation)
	{
		// Reset state
		m_readSz = 0;
		m_hasRange = (rangeStart || rangeEnd);
		m_rangeStart = rangeStart;
		m_rangeEnd = rangeEnd;
		m_downloadDone = false;
		m_total = 0;
		m_vod.reset();

		curl_easy_setopt(m_curl, CURLOPT_URL, url);

		m_lastError = 200;

		if (!sce_paf_strncmp(url + 7, "localfile:", 10))
		{
			//SCE_DBG_LOG_INFO("[LiveGetImpl] Is a local file: %s", url);
			int32_t ret = SCE_OK;
			m_vod = LocalFile::Open(url + 17, File::RDONLY, 0, &ret);
			if (ret != SCE_PAF_OK)
			{
				m_lastError = 404;
				return -1;
			}
			//SCE_DBG_LOG_INFO("[LiveGetImpl] Open ok");
		}

		if (m_hasRange)
		{
			if (rangeEnd >= rangeStart)
			{
				//SCE_DBG_LOG_INFO("[LiveGetImpl] Req type: ranged, normal");
				return static_cast<int64_t>(rangeEnd - rangeStart + 1);
			}
			else
			{
				//SCE_DBG_LOG_ERROR("[LiveGetImpl] Req type: ranged, open-ended, aborting...");
				m_lastError = 404;
				return -1;
			}
		}
		else
		{
			//SCE_DBG_LOG_INFO("[LiveGetImpl] Req type: normal");

			if (m_vod.get())
			{
				return m_vod->GetFileSize();
			}

			return -1;
		}
	}
	else
	{
		// If already downloaded fully, just return EOF
		if (m_downloadDone && m_readSz >= m_total)
		{
			//SCE_DBG_LOG_ERROR("[LiveGetImpl] Trying to repeat completed request!");
			*completed = true;
			return 0;
		}

		// Configure curl on first continuation
		if (m_total == 0 && !m_downloadDone)
		{
			if (m_vod.get())
			{
				if (m_hasRange)
				{
					//SCE_DBG_LOG_INFO("[LiveGetImpl] Setup ranges...");
					m_vod->Seek(m_rangeStart, File::SET);
				}
				m_total = m_vod->GetFileSize();
				if (m_total > SCE_KERNEL_4MiB)
				{
					SCE_DBG_LOG_ERROR("[LiveGetImpl] tmp buffer is too small!");
					m_lastError = 404;
					return -1;
				}
				m_total = m_vod->Read(m_tmpBuf, m_total);
				//SCE_DBG_LOG_INFO("[LiveGetImpl] m_vod->Read(): %u", m_total);
			}
			else
			{
				if (m_hasRange)
				{
					char range[128];
					if (m_rangeEnd > 0)
					{
						sce_paf_snprintf(range, sizeof(range), "%llu-%llu", m_rangeStart, m_rangeEnd);
					}
					else
					{
						sce_paf_snprintf(range, sizeof(range), "%llu-", m_rangeStart);
					}
					curl_easy_setopt(m_curl, CURLOPT_RANGE, range);
					//SCE_DBG_LOG_INFO("[LiveGetImpl] Setup ranges: %s", range);
				}

				CURLcode res = curl_easy_perform(m_curl);
				if (res == CURLE_OPERATION_TIMEDOUT)
				{
					res = curl_easy_perform(m_curl);
				}
				curl_easy_getinfo(m_curl, CURLINFO_RESPONSE_CODE, &m_lastError);
				if (res != CURLE_OK)
				{
					SCE_DBG_LOG_ERROR("[LiveGetImpl] curl_easy_perform() returned %d, %d\n", res, m_lastError);
					return -1;
				}
			}

			m_readSz = 0;
			m_downloadDone = true;

			//SCE_DBG_LOG_INFO("[LiveGetImpl] Total: %u", m_total);
		}

		uint32_t remain = m_total - m_readSz;
		size_t toRead = paf::min<size_t>(remain, bufLen);
		//SCE_DBG_LOG_INFO("[LiveGetImpl] To read: %u", toRead);
		if (toRead > 0)
		{
			sce_paf_memcpy(buf, m_tmpBuf + m_readSz, toRead);
			m_readSz += toRead;
		}

		if (m_readSz >= m_total)
		{
			//SCE_DBG_LOG_INFO("[LiveGetImpl] EOF");
			*completed = true;
		}

		return static_cast<int64_t>(toRead);
	}

	return -1;
}

int32_t AVPlayer::SmoothStreaming::GetLastStatusImpl()
{
	return m_lastError;
}

int64_t AVPlayer::SmoothStreaming::GetFileSizeImpl()
{
	if (m_vod.get())
	{
		return m_vod->GetFileSize();
	}

	return 0;
}

int32_t AVPlayer::SmoothStreaming::VodOpenFileImpl(const char *argFilename)
{
	int32_t ret = SCE_OK;

	SCE_DBG_LOG_INFO("[VodOpenFileImpl] VodOpenFileImpl: %s", argFilename);

	if (!sce_paf_strncmp(argFilename, "localfile:", 10))
	{
		m_vod = LocalFile::Open(argFilename + 10, File::RDONLY, 0, &ret);
	}
	else
	{
		m_vod = CurlFile::Open(argFilename, File::RDONLY, 0, &ret, NULL, utils::GetGlobalProxy());
	}
	if (ret < 0)
	{
		return -1;
	}

	m_vodBuf = MallocBuffer::Allocate(SCE_KERNEL_16MiB);
	m_prVod = new PredictiveFile(m_vodBuf);

	ret = m_prVod->Open(m_vod);
	if (ret < 0)
	{
		return -1;
	}

	return 0;
}

int32_t AVPlayer::SmoothStreaming::VodCloseFileImpl()
{
	m_prVod->Close();
	delete m_prVod;
	m_vodBuf.reset();
	m_vod.reset();

	return 0;
}

int32_t AVPlayer::SmoothStreaming::VodReadOffsetFileImpl(uint8_t *argBuffer, uint64_t argPosition, uint32_t argLength)
{
	int32_t ret = m_prVod->ReadEx(argBuffer, argPosition, argLength);
	if (ret == PredictiveFile::PRF_ERR_EX_WAIT)
	{
		ret = 0;
	}
	return  ret;
}

uint64_t AVPlayer::SmoothStreaming::VodSizeFileImpl()
{
	return m_prVod->GetDwlSize();
}

void AVPlayer::AVAudioThread::EntryFunction()
{
	int32_t ret = false;
	SceAvPlayerFrameInfo data;
	int32_t outPortId = -1;
	bool usingFallback = false;

	SCE_DBG_LOG_INFO("[AVAudioThread] AVAudioThread start");

	uint8_t *silence = (uint8_t *)sce_paf_memalign(0x20, SCE_AV_PLAYER_PCM_BUFFER_SIZE * 2  * 4);
	sce_paf_memset(silence, 0, SCE_AV_PLAYER_PCM_BUFFER_SIZE * 2 * 4);

	outPortId = sceAudioOutOpenPort(
		SCE_AUDIO_OUT_PORT_TYPE_VOICE,
		(SCE_AV_PLAYER_PCM_BUFFER_SIZE / sizeof(int16_t)),
		44100,
		1);
	if (outPortId < 0)
	{
		SCE_DBG_LOG_ERROR("[AVAudioThread] Failed to open VOICE audio port");
	}

	while (!IsCanceled())
	{
		if (sceAvPlayerIsActive(m_playerCore))
		{
			ret = sceAvPlayerGetAudioData(m_playerCore, &data);
			if (ret)
			{
				if (m_parent->m_lastAudioDetails.channelCount != data.details.audio.channelCount ||
					m_parent->m_lastAudioDetails.sampleRate != data.details.audio.sampleRate)
				{
					SCE_DBG_LOG_INFO("[AVAudioThread] Audio params changed, reconfiguring port");
					SCE_DBG_LOG_INFO("[AVAudioThread] New params: %u %u", data.details.audio.channelCount, data.details.audio.sampleRate);

					usingFallback = false;

					ret = sceAudioOutSetConfig(
						outPortId,
						(SCE_AV_PLAYER_PCM_BUFFER_SIZE / sizeof(int16_t)),
						data.details.audio.sampleRate,
						data.details.audio.channelCount - 1);

					if (ret != SCE_OK)
					{
						SCE_DBG_LOG_ERROR("[AVAudioThread] Could not reconfigure audio port, using failsafe to keep timing");
						usingFallback = true;
					}

					int32_t maxVolumeArray[2] = { SCE_AUDIO_VOLUME_0DB, SCE_AUDIO_VOLUME_0DB };
					sceAudioOutSetVolume(outPortId, SCE_AUDIO_VOLUME_FLAG_L_CH | SCE_AUDIO_VOLUME_FLAG_R_CH, maxVolumeArray);

					m_parent->m_lastAudioDetails.channelCount = data.details.audio.channelCount;
					m_parent->m_lastAudioDetails.sampleRate = data.details.audio.sampleRate;
				}

				const void *outData = data.pData;
				if (usingFallback)
				{
					outData = silence;
				}
				sceAudioOutOutput(outPortId, outData);
			}
			else
			{
				sceAudioOutOutput(outPortId, silence);
			}
		}
	}

	sceAudioOutReleasePort(outPortId);

	sce_paf_free(silence);

	while (!IsCanceled())
	{
		thread::Sleep(1000);
	}

	Cancel();
}

void AVPlayer::SurfaceUpdateTask(void *pArgBlock)
{
	static_cast<AVPlayer *>(pArgBlock)->OnSurfaceUpdate();
}

void AVPlayer::OnSurfaceUpdate()
{
	SceAvPlayerFrameInfoEx data;
	bool ret = false;

	sce_paf_memset(&data, 0, sizeof(SceAvPlayerFrameInfo));

	if (sceAvPlayerIsActive(m_playerCore))
	{
		data.pData = NULL;
		ret = sceAvPlayerGetVideoDataEx(m_playerCore, &data);
		if (ret && data.pData && !m_powerSaving)
		{
			//SCE_DBG_LOG_INFO("TEXDBG got texture 0x%08X", data.pData);
			
			if (m_lastVideoDetails.width != data.details.video.width ||
				m_lastVideoDetails.height != data.details.video.height)
			{
				SCE_DBG_LOG_INFO("[SurfaceUpdateTask] Video dimensions changed, clearing draw surface map");
				SCE_DBG_LOG_INFO("[SurfaceUpdateTask] New dimensions: %u %u", data.details.video.width, data.details.video.height);
				SCE_DBG_LOG_INFO("[SurfaceUpdateTask] New pitch: %u", data.details.video.pitch);

				auto itr = m_drawSurf.begin();
				while (itr != m_drawSurf.end())
				{
					(*itr).second.clear();
					itr = m_drawSurf.erase(itr);
				}

				//float aspect = (float)data.uTexWidth / (float)data.uTexHeight;
				//if (aspect > 1.78f || aspect < 1.76f)
				{
					//SCE_DBG_LOG_INFO("[BEAV] Suspicious aspect ratio (%.03f), so respect it", aspect);
					graph::PlaneObj *po = static_cast<graph::PlaneObj *>(m_target->GetDrawObj(ui::Plane::OBJ_PLANE));
					po->SetScaleMode(graph::PlaneObj::SCALE_ASPECT_SIZE, graph::PlaneObj::SCALE_ASPECT_SIZE);
				}

				m_lastVideoDetails.width = data.details.video.width;
				m_lastVideoDetails.height = data.details.video.height;
			}

			uint64_t detailMagic = (uint32_t)data.pData + data.details.video.width + data.details.video.height;

			if (!m_drawSurf[detailMagic].get())
			{
				SCE_DBG_LOG_INFO("[SurfaceUpdateTask] Initting new texture...");

				m_drawSurf[detailMagic] = new graph::Surface(
					data.details.video.width,
					data.details.video.height,
					ImageMode_RGBA8888,
					ImageOrder_Linear,
					1,
					data.pData,
					data.details.video.pitch * 4,
					1,
					0);
			}

			m_target->SetTexture(m_drawSurf[detailMagic]);
		}
	}
}

void AVPlayer::OnBootJob()
{
	int32_t ret = false;
	m_playerCore = 0;
	m_audioThread = NULL;
	m_currentState = PlayerState_Stop;
	sce_paf_memset(&m_lastVideoDetails, 0, sizeof(SceAvPlayerVideo));
	sce_paf_memset(&m_lastAudioDetails, 0, sizeof(SceAvPlayerAudio));
	m_currentVideoStreamId = -1;
	m_currentAudioStreamId = -1;
	m_jumpTimeMs = -1;
	m_duration = 0;
	m_lastSeekState = SeekState_Ok;
	m_ssStream = false;

	SceAvPlayerInitData playerInit;
	sce_paf_memset(&playerInit, 0, sizeof(SceAvPlayerInitData));
	playerInit.memoryReplacement.allocate = PlayerAllocate;
	playerInit.memoryReplacement.deallocate = PlayerDeallocate;
	playerInit.memoryReplacement.allocateTexture = PlayerAllocateTexture;
	playerInit.memoryReplacement.deallocateTexture = PlayerDeallocateTexture;
	playerInit.eventReplacement.objectPointer = this;
	playerInit.eventReplacement.eventCallback = PlayerChangeStateCb;
	playerInit.autoStart = false;
	SmoothStreaming::GetVODReplacement(&playerInit.fileReplacement);
	playerInit.debugLevel = SCE_AVPLAYER_DBG_WARNINGS;
	m_playerCore = sceAvPlayerInit(&playerInit);
	if (!m_playerCore)
	{
		SetInitState(InitState_InitFail);
		return;
	}

	SceAvPlayerPostInitDataNew postData;
	sce_paf_memset(&postData, 0, sizeof(SceAvPlayerPostInitDataNew));
	SmoothStreaming::GetLiveReplacement(&postData.httpReplacement);
	ret = sceAvPlayerPostInit(m_playerCore, (SceAvPlayerPostInitData *)&postData);
	if (ret != SCE_OK)
	{
		SetInitState(InitState_InitFail);
		return;
	}

	/*
	SceAvPlayerPostInitDataEx postDataEx;
	sce_paf_memset(&postDataEx, 0, sizeof(SceAvPlayerPostInitDataEx));
	postDataEx.demuxSharedHeapSize = 64 * 1014 * 1024;
	postDataEx.demuxAudioBufferSize = 4 * 1024 * 1024;
	postDataEx.demuxVideoBufferSize = 4 * 1024 * 1024;
	ret = sceAvPlayerPostInitEx(m_playerCore, &postDataEx);
	if (ret != SCE_OK)
	{
		SetInitState(InitState_InitFail);
		return;
	}
	*/

	SCE_DBG_LOG_INFO("[BootJob] sceAvPlayerAddSource: %s", m_path.c_str());
	ret = sceAvPlayerAddSource(m_playerCore, m_path.c_str());
	if (ret != SCE_OK)
	{
		SetInitState(InitState_InitFail);
		return;
	}

	SCE_DBG_LOG_INFO("[BootJob] sceAvPlayerAddSource(): ok");

	while (m_currentState != PlayerState_Ready)
	{
		thread::Sleep(100);
	}

	SCE_DBG_LOG_INFO("[BootJob] Stream ready...");

	common::MainThreadCallList::Register(SurfaceUpdateTask, this);

	thread::Thread::Option opt;
	opt.affinity = thread::Thread::Affinity_All;
	m_audioThread = new AVAudioThread(this, m_playerCore, &opt);
	m_audioThread->Start();

	sceAvPlayerStart(m_playerCore);

	sceAvPlayerSetLogger(
	[](void *userData, const char *format, _Va_list ap)
	{
		static_cast<AVPlayer *>(userData)->OnPlayerWarning(format, ap);
		return SCE_OK;
	}, this);

	SetInitState(InitState_InitOk);
}

void AVPlayer::OnPlayerWarning(const char *format, _Va_list ap)
{
	const char *msg = va_arg(ap, const char *);
	if (m_lastSeekState == SeekState_Waiting)
	{
		if (!sce_paf_strncmp(msg, "E/ Seek [467]", 13))
		{
			m_lastSeekState = SeekState_Failed;
		}
		else if (!sce_paf_strncmp(msg, "W/ ProcessAckEvent [307]", 24))
		{
			m_lastSeekState = SeekState_Ok;
		}
	}
}

AVPlayer::AVPlayer(ui::Widget *targetPlane, const char *url, Option *opt)
{
	SCE_DBG_LOG_INFO("[AV] Open url: %s", url);
	m_target = targetPlane;
	m_basePath = url;
	if (utils::IsLocalPath(url))
	{
		m_path = "localfile:";
	}
	m_path += url;
	if (opt)
	{
		m_opt = *opt;
	}
}

AVPlayer::~AVPlayer()
{
	Term();
}

void AVPlayer::InitAsync()
{
	BootJob *job = new BootJob(this);
	common::SharedPtr<job::JobItem> itemParam(job);
	utils::GetJobQueue()->Enqueue(itemParam);
	SetInitState(InitState_InProgress);
}

void AVPlayer::Term()
{
	if (m_initState != InitState_NotInit)
	{
		if (m_playerCore)
		{
			sceAvPlayerStop(m_playerCore);
		}

		common::MainThreadCallList::Unregister(SurfaceUpdateTask, this);

		if (m_audioThread)
		{
			m_audioThread->Cancel();
			m_audioThread->Join();
			delete m_audioThread;
		}

		m_target->SetTexture(g_texTransparent);

		auto itr = m_drawSurf.begin();
		while (itr != m_drawSurf.end())
		{
			(*itr).second.clear();
			itr = m_drawSurf.erase(itr);
		}

		if (m_playerCore)
		{
			sceAvPlayerClose(m_playerCore);
			m_playerCore = NULL;
		}
	}

	SetInitState(InitState_NotInit);
}

GenericPlayer::InitState AVPlayer::GetInitState()
{
	return m_initState;
}

void AVPlayer::SetInitState(GenericPlayer::InitState state)
{
	m_initState = state;
	SCE_DBG_LOG_INFO("[SetInitState] State changed to: %d", state);
	event::BroadcastGlobalEvent(g_appPlugin, GenericPlayerChangeState, m_initState);
}

void AVPlayer::ParseStreamInfo()
{
	int32_t ret = SCE_OK;
	bool videoEnabled = false;
	bool audioEnabled = false;

	int32_t streamCount = sceAvPlayerStreamCount(m_playerCore);
	if (streamCount < 0)
	{
		streamCount = 0;
	}
	m_streams.clear();

	SCE_DBG_LOG_INFO("[ParseStreamInfo] scount: %d", streamCount);

	for (int i = 0; i < streamCount; i++)
	{
		StreamInfo streamInfo;
		SceAvPlayerStreamInfo avStreamInfo;
		ret = sceAvPlayerGetStreamInfo(m_playerCore, i, &avStreamInfo);
		if (ret == SCE_OK)
		{
			switch (avStreamInfo.type)
			{
			case SCE_AVPLAYER_VIDEO:
				if (!videoEnabled)
				{
					sceAvPlayerEnableStream(m_playerCore, i);
					m_currentVideoStreamId = i;
					videoEnabled = true;
				}
				else
				{
					sceAvPlayerDisableStream(m_playerCore, i);
					SCE_DBG_LOG_WARNING("[ParseStreamInfo] Multiple video streams detected, only one stream is supported at the time");
				}
				streamInfo.type = StreamType_Video;
				break;
			case SCE_AVPLAYER_AUDIO:
				if (!audioEnabled)
				{
					sceAvPlayerEnableStream(m_playerCore, i);
					m_currentAudioStreamId = i;
					audioEnabled = true;
				}
				else
				{
					sceAvPlayerDisableStream(m_playerCore, i);
					SCE_DBG_LOG_WARNING("[ParseStreamInfo] Multiple audio streams detected, only one stream is supported at the time");
				}
				streamInfo.type = StreamType_Audio;
				break;
			}

			if (m_duration == 0)
			{
				m_duration = avStreamInfo.duration;
				SCE_DBG_LOG_INFO("[AV] Total stream duration: %u", m_duration);
			}

			SCE_DBG_LOG_INFO("[ParseStreamInfo] Got stream %d of type %d", i, streamInfo.type);

			// For local streams this will return negative error
			// Also representationCount > 0 means we are using SmoothStreaming/yucca
			int32_t representationCount = sceAvPlayerRepresentationCount(m_playerCore, i);
			if (representationCount < 0)
			{
				representationCount = 0;
				SCE_DBG_LOG_INFO("[ParseStreamInfo] repcount: %d", 1);
			}
			else
			{
				m_ssStream = true;
				sceAvPlayerSetLooping(m_playerCore, true);
				SCE_DBG_LOG_INFO("[ParseStreamInfo] repcount: %d", representationCount);
			}

			if (representationCount == 0)
			{
				StreamInfo::RepresentationInfo repInfo;
				{
					SCE_DBG_LOG_INFO("[ParseStreamInfo] Got representation %d - %d", i, 0);
					repInfo.bitrate = 0;
					switch (avStreamInfo.type)
					{
					case SCE_AVPLAYER_VIDEO:
						repInfo.width = avStreamInfo.details.video.width;
						repInfo.height = avStreamInfo.details.video.height;
						SCE_DBG_LOG_INFO("[ParseStreamInfo] repinfo - bitrate: %u, width: %u, height: %u", repInfo.bitrate, repInfo.width, repInfo.height);
						break;
					case SCE_AVPLAYER_AUDIO:
						repInfo.ch = avStreamInfo.details.audio.channelCount;
						repInfo.srate = avStreamInfo.details.audio.sampleRate;
						SCE_DBG_LOG_INFO("[ParseStreamInfo] repinfo - bitrate: %u, ch: %u, srate: %u", repInfo.bitrate, repInfo.ch, repInfo.srate);
						break;
					}

					streamInfo.representation.push_back(repInfo);
				}
			}
			else
			{
				for (int j = 0; j < representationCount; j++)
				{
					StreamInfo::RepresentationInfo repInfo;
					SceAvPlayerRepresentationInfo avRepInfo;
					ret = sceAvPlayerGetRepresentationInfo(m_playerCore, i, j, &avRepInfo);
					if (ret == SCE_OK)
					{
						SCE_DBG_LOG_INFO("[ParseStreamInfo] Got representation %d - %d", i, j);
						repInfo.bitrate = avRepInfo.bitrate;
						switch (avRepInfo.type)
						{
						case SCE_AVPLAYER_REPRESENTATION_VIDEO:
							repInfo.width = avRepInfo.details.video.width;
							repInfo.height = avRepInfo.details.video.height;
							SCE_DBG_LOG_INFO("[ParseStreamInfo] repinfo - bitrate: %u, width: %u, height: %u", repInfo.bitrate, repInfo.width, repInfo.height);
							break;
						case SCE_AVPLAYER_REPRESENTATION_AUDIO:
							repInfo.ch = avRepInfo.details.audio.channelCount;
							repInfo.srate = avRepInfo.details.audio.sampleRate;
							SCE_DBG_LOG_INFO("[ParseStreamInfo] repinfo - bitrate: %u, ch: %u, srate: %u", repInfo.bitrate, repInfo.ch, repInfo.srate);
							break;
						}

						streamInfo.representation.push_back(repInfo);
					}
				}
			}

			m_streams.push_back(streamInfo);
		}
	}
}

uint32_t AVPlayer::GetTotalTimeMs()
{
	return m_duration;
}

uint32_t AVPlayer::GetCurrentTimeMs()
{
	if (m_jumpTimeMs != -1)
	{
		return m_jumpTimeMs;
	}
	return sceAvPlayerCurrentTime(m_playerCore);
}

bool AVPlayer::JumpToTimeMs(uint32_t time)
{
	SCE_DBG_LOG_INFO("[AV] JumpToTime: %u", time);
	if (time > (m_duration - 100))
	{
		time = m_duration - 100;
	}

	if (m_currentState == PlayerState_Eof && !m_ssStream)
	{
		sceAvPlayerStop(m_playerCore);
		sceAvPlayerStart(m_playerCore);
		m_currentState = PlayerState_Stop;
	}
	sceAvPlayerJumpToTime(m_playerCore, time);

	m_lastSeekState = SeekState_Waiting;
	while (m_lastSeekState == SeekState_Waiting)
	{
		thread::Sleep(10);
	}
	if (m_lastSeekState != SeekState_Ok)
	{
		return false;
	}

	m_jumpTimeMs = time;

	return true;
}

GenericPlayer::PlayerState AVPlayer::GetState()
{
	return m_currentState;
}

void AVPlayer::SetPowerSaving(bool enable)
{
	m_powerSaving = enable;
}

uint32_t AVPlayer::GetVideoRepresentationNum()
{
	return 1;
}

uint32_t AVPlayer::GetAudioRepresentationNum()
{
	return 1;
}

void AVPlayer::GetActiveRepresentationsInfo(StreamInfo::RepresentationInfo *info)
{
	if (!info)
	{
		return;
	}

	uint32_t vidBitrate = 0;
	sceAvPlayerGetStreamSetBitrate(m_playerCore, m_currentVideoStreamId, &vidBitrate);
	sceAvPlayerGetStreamSetBitrate(m_playerCore, m_currentAudioStreamId, &info->bitrate);
	info->bitrate += vidBitrate;
	info->width = m_lastVideoDetails.width;
	info->height = m_lastVideoDetails.height;
	info->srate = m_lastAudioDetails.sampleRate;
	info->ch = m_lastAudioDetails.channelCount;
}

void AVPlayer::SwitchPlaybackState()
{
	if (m_currentState == PlayerState_Stop || m_currentState == PlayerState_Ready)
	{
		sceAvPlayerStart(m_playerCore);
		sceAvPlayerJumpToTime(m_playerCore, 1000);
	}
	else if (m_currentState == PlayerState_Pause)
	{
		sceAvPlayerResume(m_playerCore);
	}
	else if (m_currentState == PlayerState_Play)
	{
		sceAvPlayerPause(m_playerCore);
	}
}

bool AVPlayer::IsPaused()
{
	return (m_currentState == PlayerState_Pause);
}

void AVPlayer::SetState(int32_t argEventId)
{
	switch (argEventId)
	{
	case SCE_AVPLAYER_STATE_STOP:
		m_currentState = PlayerState_Stop;
		break;
	case SCE_AVPLAYER_STATE_READY:
		ParseStreamInfo();
		m_currentState = PlayerState_Ready;
		break;
	case SCE_AVPLAYER_STATE_PLAY:
		m_currentState = PlayerState_Play;
		m_jumpTimeMs = -1;
		break;
	case SCE_AVPLAYER_STATE_PAUSE:
		m_currentState = PlayerState_Pause;
		break;
	case SCE_AVPLAYER_STATE_BUFFERING:
		m_currentState = PlayerState_Buffering;
		break;
	case SCE_AVPLAYER_STATE_EOS:
		m_currentState = PlayerState_Eof;
		break;
	case SCE_AVPLAYER_TIMED_TEXT_DELIVERY:
		break;
	case SCE_AVPLAYER_WARNING_ID:
		// If both are empty - player failed to init
		if (m_lastVideoDetails.width == 0 && m_lastAudioDetails.sampleRate == 0)
		{
			SetInitState(InitState_InitFail);
		}
		// Sometimes SceAvPlayer doesn't send PLAY event after seeking, but always sends this (warning about being out of sync for a frame)
		m_jumpTimeMs = -1;
		break;
	case SCE_AVPLAYER_ENCRYPTION:
		break;
	}
}

void *AVPlayer::PlayerAllocate(void *jumpback, uint32_t alignment, uint32_t size)
{
	return sce_paf_memalign(alignment, size);
};

void AVPlayer::PlayerDeallocate(void *jumpback, void *pMemory)
{
	sce_paf_free(pMemory);
};

void *AVPlayer::PlayerAllocateTexture(void *jumpback, uint32_t alignment, uint32_t size)
{
	SceKernelAllocMemBlockOpt blkOpt;
	if (alignment < SCE_KERNEL_256KiB)
	{
		alignment = SCE_KERNEL_256KiB;
	}
	sce_paf_memset(&blkOpt, 0, sizeof(SceKernelAllocMemBlockOpt));
	blkOpt.size = sizeof(SceKernelAllocMemBlockOpt);
	blkOpt.attr = SCE_KERNEL_ALLOC_MEMBLOCK_ATTR_HAS_ALIGNMENT;
	blkOpt.alignment = alignment;

	GraphMem::Option opt;
	opt.memblock_option = &blkOpt;
	opt.gpu_map_attrib = SCE_GXM_MEMORY_ATTRIB_READ | SCE_GXM_MEMORY_ATTRIB_WRITE;

	void *ret = GraphMem::AllocOSMemory(GraphMem::DeviceType_VideoMemory, ROUND_UP(size, alignment), "AVPlayer::DecodeMemoryPool", &opt);
	if (!ret)
	{
		SCE_DBG_LOG_ERROR("[PlayerDeallocateTexture] Out of memory");
	}
	else
	{
		SCE_DBG_LOG_TRACE("[PlayerAllocateTexture] Allocate texture 0x%08X : 0x%08X : %u", ret, alignment, size);
	}

	return ret;
};

void AVPlayer::PlayerDeallocateTexture(void *jumpback, void *pMemory)
{
	SCE_DBG_LOG_TRACE("[PlayerDeallocateTexture] Free texture 0x%08X", pMemory);
	GraphMem::FreeOSMemory(GraphMem::DeviceType_VideoMemory, pMemory);
};

void AVPlayer::PlayerChangeStateCb(void *instance, int32_t argEventId, int32_t argSourceId, void *argEventData)
{
	AVPlayer *obj = static_cast<AVPlayer *>(instance);
	SCE_DBG_LOG_INFO("[PlayerChangeStateCb] State type: 0x%08X", argEventId);
	obj->SetState(argEventId);
}

void AVPlayer::PreInit()
{
	new Module("app0:module/libSceAvPlayerPSVitaRGBA8888.suprx");
}

GenericPlayer::SupportType AVPlayer::IsSupported(const char *path)
{
	char *extStart = sce_paf_strrchr(path, '.');
	if (!extStart)
	{
		return SupportType_MaybeSupported;
	}

	for (int i = 0; i < sizeof(k_supportedExtensions) / sizeof(char*); i++)
	{
		if (!sce_paf_strcasecmp(extStart, k_supportedExtensions[i]))
			return SupportType_Supported;
	}

	return SupportType_NotSupported;
}
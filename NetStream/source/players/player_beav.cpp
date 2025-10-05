#include <kernel.h>
#include <kernel/dmacmgr.h>
#include <paf.h>
#include <libdbg.h>
#include <appmgr.h>
#include <libsysmodule.h>
#include <scebeavplayer.h>
#include <avcdec.h>
#include <audioout.h>
#include <taihen.h>
#include <psp2_compat/curl/curl.h>

#include "common.h"
#include "utils.h"
#include "event.h"
#include "players/player_beav.h"

#undef SCE_DBG_LOG_COMPONENT
#define SCE_DBG_LOG_COMPONENT "[BEAV]"

using namespace paf;

CURL *BEAVPlayer::LibLSInterface::s_plCurl = NULL;
CURL *BEAVPlayer::LibLSInterface::s_fileCurl = NULL;
thread::RMutex BEAVPlayer::LibLSInterface::s_plMtx = thread::RMutex("BEAVPlayer::plMtx");
thread::RMutex BEAVPlayer::LibLSInterface::s_fileMtx = thread::RMutex("BEAVPlayer::fileMtx");
SceBeavCorePlayerLibLSInterface BEAVPlayer::s_liblsInterface;
tai_hook_ref_t BEAVPlayer::s_liblsInitHookRef;

static const char *k_supportedExtensions[] = {
		".m3u8"
};

BEAVPlayer::LibLSInterface::LibLSInterface()
{
	m_curl = NULL;
	m_readPos = 0;
	m_buf = NULL;
	m_pos = 0;
	m_lastError = 0;
}

BEAVPlayer::LibLSInterface::~LibLSInterface()
{
	Abort();
	if (m_curl != s_plCurl && m_curl != s_fileCurl)
	{
		curl_easy_cleanup(m_curl);
	}
}

void BEAVPlayer::LibLSInterface::Init()
{
	s_plCurl = curl_easy_init();
	curl_easy_setopt(s_plCurl, CURLOPT_WRITEFUNCTION, DownloadCore);
	curl_easy_setopt(s_plCurl, CURLOPT_USERAGENT, USER_AGENT);
	curl_easy_setopt(s_plCurl, CURLOPT_ACCEPT_ENCODING, "");
	curl_easy_setopt(s_plCurl, CURLOPT_SSL_VERIFYHOST, 0L);
	curl_easy_setopt(s_plCurl, CURLOPT_SSL_VERIFYPEER, 0L);
	curl_easy_setopt(s_plCurl, CURLOPT_FOLLOWLOCATION, 1L);
	curl_easy_setopt(s_plCurl, CURLOPT_TCP_KEEPALIVE, 1L);
	curl_easy_setopt(s_plCurl, CURLOPT_NOPROGRESS, 1L);
	if (utils::GetGlobalProxy())
	{
		curl_easy_setopt(s_plCurl, CURLOPT_PROXY, utils::GetGlobalProxy());
	}

	s_fileCurl = curl_easy_init();
	curl_easy_setopt(s_fileCurl, CURLOPT_WRITEFUNCTION, DownloadCore);
	curl_easy_setopt(s_fileCurl, CURLOPT_USERAGENT, USER_AGENT);
	curl_easy_setopt(s_fileCurl, CURLOPT_ACCEPT_ENCODING, "");
	curl_easy_setopt(s_fileCurl, CURLOPT_SSL_VERIFYHOST, 0L);
	curl_easy_setopt(s_fileCurl, CURLOPT_SSL_VERIFYPEER, 0L);
	curl_easy_setopt(s_fileCurl, CURLOPT_FOLLOWLOCATION, 1L);
	curl_easy_setopt(s_fileCurl, CURLOPT_TCP_KEEPALIVE, 1L);
	curl_easy_setopt(s_fileCurl, CURLOPT_NOPROGRESS, 1L);
	curl_easy_setopt(s_fileCurl, CURLOPT_BUFFERSIZE, SCE_KERNEL_64KiB);
	if (utils::GetGlobalProxy())
	{
		curl_easy_setopt(s_fileCurl, CURLOPT_PROXY, utils::GetGlobalProxy());
	}
}

void BEAVPlayer::LibLSInterface::GetReplacement(SceBeavCorePlayerLsInputPluginInterface *repl)
{
	if (!repl)
	{
		return;
	}

	repl->Open =
	[](char *pcURI, uint64_t uOffset, uint32_t uTimeOutMSecs, LSInputHandle *pHandle)
	{
		LibLSInterface *instance = new LibLSInterface();
		*pHandle = instance;
		return instance->Open(pcURI, uOffset, uTimeOutMSecs);
	};
	repl->GetSize = [](LSInputHandle handle, uint64_t *pSize) { return static_cast<LibLSInterface *>(handle)->GetSize(pSize); };
	repl->Read = [](LSInputHandle handle, void *pBuffer, uint32_t uSize, uint32_t uTimeOutMSecs, uint32_t *pReadSize) { return static_cast<LibLSInterface *>(handle)->Read(pBuffer, uSize, uTimeOutMSecs, pReadSize); };
	repl->Abort = [](LSInputHandle handle) { return static_cast<LibLSInterface *>(handle)->Abort(); };
	repl->Close = [](LSInputHandle *pHandle) { delete static_cast<LibLSInterface *>(*pHandle); return LS_INPUT_OK; };
	repl->GetLastError = [](LSInputHandle handle, uint32_t *pNativeError) { return static_cast<LibLSInterface *>(handle)->GetLastError(pNativeError); };
}

void BEAVPlayer::LibLSInterface::Term()
{
	curl_easy_cleanup(s_plCurl);
	curl_easy_cleanup(s_fileCurl);
}

size_t BEAVPlayer::LibLSInterface::DownloadCore(char *buffer, size_t size, size_t nitems, void *userdata)
{
	LibLSInterface *obj = static_cast<LibLSInterface *>(userdata);
	size_t toCopy = size * nitems;

	if (toCopy != 0) {
		obj->m_buf = sce_paf_realloc(obj->m_buf, obj->m_pos + toCopy);
		sce_paf_memcpy(static_cast<char *>(obj->m_buf) + obj->m_pos, buffer, toCopy);
		obj->m_pos += toCopy;
	}

	return toCopy;
}

LSInputResult BEAVPlayer::LibLSInterface::ConvertError(int err)
{
	LSInputResult ret = LS_INPUT_OK;

	switch (err) {
	case CURLE_OK:
		break;
	case CURLE_COULDNT_RESOLVE_PROXY:
		ret = LS_INPUT_ERROR_NO_CONNECTION;
		break;
	case CURLE_FTP_ACCEPT_TIMEOUT:
	case CURLE_OPERATION_TIMEDOUT:
		ret = LS_INPUT_ERROR_TIME_OUT;
		break;
	case CURLE_AGAIN:
		ret = LS_INPUT_ERROR_NO_CONNECTION;
		break;
	default:
		ret = LS_INPUT_ERROR_NOT_SUPPORTED;
		break;
	}

	return ret;
}

LSInputResult BEAVPlayer::LibLSInterface::Open(char *pcURI, uint64_t uOffset, uint32_t uTimeOutMSecs)
{
	thread::RMutex *lockMtx = NULL;

	if (!pcURI)
	{
		return LS_INPUT_ERROR_INVALID_URI_PTR;
	}

	char *ext = NULL;
	ext = sce_paf_strrchr(pcURI, '.');
	if (ext)
	{
		if (!sce_paf_strncmp(ext, ".m3u8", 5))
		{
			m_curl = s_plCurl;
			lockMtx = &s_plMtx;
		}
		else if (!sce_paf_strncmp(ext, ".ts", 3))
		{
			m_curl = s_fileCurl;
			lockMtx = &s_fileMtx;
		}
	}

	if (!m_curl)
	{
		m_curl = curl_easy_init();
		curl_easy_setopt(m_curl, CURLOPT_WRITEFUNCTION, DownloadCore);
		curl_easy_setopt(m_curl, CURLOPT_USERAGENT, USER_AGENT);
		curl_easy_setopt(m_curl, CURLOPT_ACCEPT_ENCODING, "");
		curl_easy_setopt(m_curl, CURLOPT_SSL_VERIFYHOST, 0L);
		curl_easy_setopt(m_curl, CURLOPT_SSL_VERIFYPEER, 0L);
		curl_easy_setopt(m_curl, CURLOPT_FOLLOWLOCATION, 1L);
		curl_easy_setopt(m_curl, CURLOPT_TCP_KEEPALIVE, 1L);
		curl_easy_setopt(m_curl, CURLOPT_NOPROGRESS, 1L);
		if (utils::GetGlobalProxy())
		{
			curl_easy_setopt(m_curl, CURLOPT_PROXY, utils::GetGlobalProxy());
		}
	}
	else
	{
		lockMtx->Lock();
	}

	curl_easy_setopt(m_curl, CURLOPT_WRITEDATA, this);
	curl_easy_setopt(m_curl, CURLOPT_URL, pcURI);
	curl_easy_setopt(m_curl, CURLOPT_CONNECTTIMEOUT_MS, uTimeOutMSecs);
	curl_easy_setopt(m_curl, CURLOPT_TIMEOUT, 15L);

	if (uOffset)
	{
		curl_easy_setopt(m_curl, CURLOPT_RESUME_FROM_LARGE, uOffset);
	}

	CURLcode ret = curl_easy_perform(m_curl);
	if (lockMtx)
	{
		lockMtx->Unlock();
	}
	if (ret != CURLE_OK)
	{
		SCE_DBG_LOG_ERROR("[LS::Open] curl_easy_perform returned %d", ret);
		m_lastError = ret;
		return ConvertError(ret);
	}

	return LS_INPUT_OK;
}

LSInputResult BEAVPlayer::LibLSInterface::GetSize(uint64_t *pSize)
{
	if (!pSize)
	{
		return LS_INPUT_ERROR_INVALID_SIZE_PTR;
	}

	*pSize = static_cast<uint64_t>(m_pos);

	return LS_INPUT_OK;
}

LSInputResult BEAVPlayer::LibLSInterface::Read(void *pBuffer, uint32_t uSize, uint32_t uTimeOutMSecs, uint32_t *pReadSize)
{
	if (!pBuffer)
	{
		return LS_INPUT_ERROR_INVALID_BUFFER_PTR;
	}

	if (!pReadSize)
	{
		return LS_INPUT_ERROR_INVALID_SIZE_PTR;
	}

	if (m_readPos + uSize > m_pos)
	{
		uSize = m_pos - m_readPos;
	}
	if (uSize != 0)
	{
		sce_paf_memcpy(pBuffer, static_cast<char *>(m_buf) + m_readPos, uSize);
	}
	else
	{
		sce_paf_free(m_buf);
		m_buf = NULL;
		m_pos = 0;
		m_readPos = 0;
	}
	m_readPos += uSize;

	*pReadSize = uSize;

	SCE_DBG_LOG_TRACE("[LS::Read] Read actual %u", *pReadSize);

	return LS_INPUT_OK;
}

LSInputResult BEAVPlayer::LibLSInterface::Abort()
{
	if (m_buf)
	{
		sce_paf_free(m_buf);
		m_buf = NULL;
		m_pos = 0;
		m_readPos = 0;
	}

	return LS_INPUT_OK;
}

LSInputResult BEAVPlayer::LibLSInterface::GetLastError(uint32_t *pNativeError)
{
	if (!pNativeError)
	{
		return LS_INPUT_ERROR_INVALID_HANDLE;
	}

	*pNativeError = m_lastError;

	return LS_INPUT_OK;
}

void BEAVPlayer::BEAVAudioThread::EntryFunction()
{
	int32_t err = false;
	SceBeavCorePlayerAudioData data;
	int32_t outPortId = -1;
	bool usingFallback = false;

	SCE_DBG_LOG_INFO("[BEAVAudioThread] BEAVAudioThread start");

	void *silence = sce_paf_memalign(0x20, 4096 * 4);
	sce_paf_memset(silence, 0, 4096 * 4);

	while (!IsCanceled())
	{
		err = sceBeavCorePlayerGetAudioData(m_playerCore, &data);
		if (err)
		{
			if (m_parent->m_lastAudioChannelCount != data.uChannelCount ||
				m_parent->m_lastAudioSampleRate != data.uSampleRate)
			{
				SCE_DBG_LOG_INFO("[BEAVAudioThread] Audio params changed, opening port");
				SCE_DBG_LOG_INFO("[BEAVAudioThread] New params: %u %u", data.uChannelCount, data.uSampleRate);

				if (outPortId > 0)
				{
					sceAudioOutReleasePort(outPortId);
				}

				outPortId = sceAudioOutOpenPort(
					SCE_AUDIO_OUT_PORT_TYPE_VOICE,
					data.uPcmSize,
					data.uSampleRate,
					data.uChannelCount - 1);
				if (outPortId < 0)
				{
					SCE_DBG_LOG_ERROR("[BEAVAudioThread] Could not open audio port, using failsafe to keep timing");

					outPortId = sceAudioOutOpenPort(
						SCE_AUDIO_OUT_PORT_TYPE_VOICE,
						data.uPcmSize,
						44100,
						1);
					usingFallback = true;
				}

				int32_t maxVolumeArray[2] = { SCE_AUDIO_VOLUME_0DB, SCE_AUDIO_VOLUME_0DB };
				sceAudioOutSetVolume(outPortId, SCE_AUDIO_VOLUME_FLAG_L_CH | SCE_AUDIO_VOLUME_FLAG_R_CH, maxVolumeArray);

				if (m_parent->GetInitState() != InitState_InitOk)
				{
					m_parent->SetInitState(InitState_InitOk);
				}

				m_parent->m_lastAudioChannelCount = data.uChannelCount;
				m_parent->m_lastAudioSampleRate = data.uSampleRate;
			}

			if (outPortId > 0)
			{
				const void *outData = silence;
				if (!usingFallback)
				{
					outData = data.pBuffer;
				}
				sceAudioOutOutput(outPortId, outData);
			}
		}
		else
		{
			if (outPortId > 0)
			{
				sceAudioOutOutput(outPortId, silence);
			}
		}
	}

	if (outPortId > 0)
	{
		sceAudioOutReleasePort(outPortId);
	}

	sce_paf_free(silence);

	Cancel();
}

void BEAVPlayer::SurfaceUpdateTask(void *pArgBlock)
{
	static_cast<BEAVPlayer *>(pArgBlock)->OnSurfaceUpdate();
}

void BEAVPlayer::OnSurfaceUpdate()
{
	SceBeavCorePlayerVideoData data;
	int32_t err = false;

	data.pFrameBuffer = NULL;
	err = sceBeavCorePlayerGetVideoData(m_playerCore, &data);
	if (err && data.pFrameBuffer && !m_powerSaving)
	{
		if (m_lastVideoWidth != data.uTexWidth ||
			m_lastVideoHeight != data.uTexHeight)
		{
			SCE_DBG_LOG_INFO("[SurfaceUpdateTask] Video dimensions changed, clearing draw surface map");
			SCE_DBG_LOG_INFO("[SurfaceUpdateTask] New dimensions: %u %u", data.uTexWidth, data.uTexHeight);
			SCE_DBG_LOG_INFO("[SurfaceUpdateTask] New pitch: %u", data.uTexPitch);

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

			m_lastVideoWidth = data.uTexWidth;
			m_lastVideoHeight = data.uTexHeight;
		}

		uint64_t detailMagic = (uint32_t)data.pFrameBuffer + data.uTexWidth + data.uTexHeight;

		if (!m_drawSurf[detailMagic].get())
		{
			SCE_DBG_LOG_INFO("[SurfaceUpdateTask] Initting new texture...");

			m_drawSurf[detailMagic] = new graph::Surface(
				data.uTexWidth,
				data.uTexHeight,
				ImageMode_RGBA8888,
				ImageOrder_Linear,
				1,
				data.pFrameBuffer,
				data.uTexPitch * 4,
				1,
				0);
		}

		m_target->SetTexture(m_drawSurf[detailMagic]);
	}
}

void BEAVPlayer::OnBootJob()
{
	int32_t ret = false;
	m_playerCore = 0;
	m_audioThread = NULL;
	m_decodeBuffer = NULL;
	m_lastVideoWidth = 0;
	m_lastVideoHeight = 0;
	m_lastAudioChannelCount = 0;
	m_lastAudioSampleRate = 0;
	m_bootEarlyExit = false;
	m_currentLsStreamlist = NULL;

	LibLSInterface::Init();

	ret = sceBeavCorePlayerInitialize();
	if (!ret)
	{
		SetInitState(InitState_InitFail);
		return;
	}

	m_playerCore = sceBeavCorePlayerCreate();
	if (!m_playerCore)
	{
		SetInitState(InitState_InitFail);
		return;
	}

	s_liblsInterface.lsSetErrorHandler(0xFFFFFFFF, LsErrorHandler, this);
	s_liblsInterface.lsSetStatusHandler(0xFFFFFFFF, LsStatusHandler, this);
	sceBeavCorePlayerSetAgent(m_playerCore, USER_AGENT);

	m_decodeBuffer = GraphMem::AllocOSMemory(GraphMem::DeviceType_VideoMemory, BEAV_DECODE_BUFFER_SIZE, "BEAVPlayer::DecodeMemory");
	if (!m_decodeBuffer)
	{
		SetInitState(InitState_InitFail);
		return;
	}

	ret = sceBeavCorePlayerSetVideoBuffer(m_playerCore, BEAV_VIDEO_BUFFER_WIDTH, BEAV_VIDEO_BUFFER_HEIGHT, BEAV_VIDEO_BUFFER_COUNT, m_decodeBuffer);
	if (ret != 0)
	{
		SetInitState(InitState_InitFail);
		return;
	}

	sceBeavCorePlayerSetLsBandwidthOpt(m_playerCore, SCE_BEAV_CORE_PLAYER_BW_SELECT_LOWEST, 10000, 10, 11000);

	ret = sceBeavCorePlayerOpenTargetUrl(m_playerCore, m_path.c_str(), true);
	if (ret != 1)
	{
		SetInitState(InitState_InitFail);
		return;
	}

	thread::Thread::Option opt;
	opt.affinity = SCE_KERNEL_CPU_MASK_USER_2;
	m_audioThread = new BEAVAudioThread(this, m_playerCore, &opt);
	m_audioThread->Start();

	SceBeavCorePlayerVideoData data;
	bool err = false;
	while (err == false)
	{
		if (m_bootEarlyExit)
		{
			SCE_DBG_LOG_ERROR("[BootJob] Exit video waiting loop early");
			return;
		}
		sceBeavCorePlayerGetVideoData(m_playerCore, &data);
		err = sceBeavCorePlayerIsReady(m_playerCore);
		thread::Sleep(1000);
	}

	SCE_DBG_LOG_INFO("[BootJob] Video ready...");

	if (GetInitState() != InitState_InitOk)
	{
		SetInitState(InitState_InitOk);
	}

	common::MainThreadCallList::Register(SurfaceUpdateTask, this);

	while (m_lastVideoWidth == 0)
	{
		thread::Sleep(100);
	}

	SCE_DBG_LOG_INFO("[BootJob] Stream ready...");
}

BEAVPlayer::BEAVPlayer(ui::Widget *targetPlane, const char *url, Option *opt)
{
	SCE_DBG_LOG_INFO("[BEAVPlayer] Open url: %s", url);
	m_target = targetPlane;
	m_path = url;
	if (opt)
	{
		m_opt = *opt;
	}
}

BEAVPlayer::~BEAVPlayer()
{
	Term();
}

void BEAVPlayer::InitAsync()
{
	BootJob *job = new BootJob(this);
	common::SharedPtr<job::JobItem> itemParam(job);
	utils::GetJobQueue()->Enqueue(itemParam);
	SetInitState(InitState_InProgress);
}

void BEAVPlayer::Term()
{
	if (m_initState != InitState_NotInit)
	{
		m_bootEarlyExit = true;

		if (m_playerCore)
		{
			sceBeavCorePlayerStop(m_playerCore);
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
			sceBeavCorePlayerDestroy(m_playerCore);
			m_playerCore = 0;
		}

		sceBeavCorePlayerFinalize();

		if (m_decodeBuffer)
		{
			GraphMem::FreeOSMemory(GraphMem::DeviceType_VideoMemory, m_decodeBuffer);
		}

		LibLSInterface::Term();
	}
}

GenericPlayer::InitState BEAVPlayer::GetInitState()
{
	return m_initState;
}

void BEAVPlayer::SetInitState(GenericPlayer::InitState state)
{
	m_initState = state;
	SCE_DBG_LOG_INFO("[SetInitState] State changed to: %d", state);
	event::BroadcastGlobalEvent(g_appPlugin, GenericPlayerChangeState, m_initState);
}

void BEAVPlayer::ParseStreamInfo(LSStreamlist *streamlist, uint32_t representationCount)
{
	int32_t ret;

	SCE_DBG_LOG_INFO("[ParseStreamInfo] scount: 1");
	SCE_DBG_LOG_INFO("[ParseStreamInfo] Got stream 0 of type 0");
	SCE_DBG_LOG_INFO("[ParseStreamInfo] repcount: %d", representationCount);

	m_currentLsStreamlist = streamlist;

	m_streams.clear();

	StreamInfo streamInfo;
	streamInfo.type = StreamType_Video;

	uint32_t defaultHeight = 0;
	switch (m_opt.defaultRes)
	{
	case Option::Resolution_144p:
		defaultHeight = 144;
		break;
	case Option::Resolution_240p:
		defaultHeight = 240;
		break;
	case Option::Resolution_360p:
		defaultHeight = 360;
		break;
	case Option::Resolution_480p:
		defaultHeight = 480;
		break;
	case Option::Resolution_720p:
		defaultHeight = 720;
		break;
	}

	for (int i = 0; i < representationCount; i++)
	{
		StreamInfo::RepresentationInfo repInfo;
		s_liblsInterface.lsStreamlistGetStreamResolution(streamlist, i, &repInfo.width, &repInfo.height);
		ret = s_liblsInterface.lsStreamlistGetStreamBandwidth(streamlist, i, &repInfo.bitrate);
		if (ret == LS_OK)
		{
			SCE_DBG_LOG_INFO("[ParseStreamInfo] Got representation 0 - %d", i);
			SCE_DBG_LOG_INFO("[ParseStreamInfo] repinfo - bitrate: %u, width: %u, height: %u", repInfo.bitrate, repInfo.width, repInfo.height);

			if (!utils::IsVideoSupported(repInfo.width, repInfo.height))
			{
				SCE_DBG_LOG_INFO("[ParseStreamInfo] ^^^ This representation is unsupported and will be disabled");
				s_liblsInterface.lsStreamlistDisableStream(streamlist, i);
			}

			if (repInfo.width == defaultHeight || repInfo.height == defaultHeight)
			{
				s_liblsInterface.lsStreamlistSelectStream(streamlist, i);
				SCE_DBG_LOG_INFO("[ParseStreamInfo] Stream 0: selected rep %d as default", i);
			}

			streamInfo.representation.push_back(repInfo);
		}
	}

	m_streams.push_back(streamInfo);
}

uint32_t BEAVPlayer::GetTotalTimeMs()
{
	return sceBeavCorePlayerGetTotalTime(m_playerCore);
}

uint32_t BEAVPlayer::GetCurrentTimeMs()
{
	return sceBeavCorePlayerGetElapsedTime(m_playerCore);
}

bool BEAVPlayer::JumpToTimeMs(uint32_t time)
{
	SCE_DBG_LOG_INFO("[JumpToTimeMs] Jump to seconds: %u", time / 1000);
	return sceBeavCorePlayerJumpToTimeCode(m_playerCore, time / 1000);
}

GenericPlayer::PlayerState BEAVPlayer::GetState()
{
	return static_cast<GenericPlayer::PlayerState>(sceBeavCorePlayerGetPlayerState(m_playerCore));
}

void BEAVPlayer::SetPowerSaving(bool enable)
{
	m_powerSaving = enable;
}

uint32_t BEAVPlayer::GetVideoRepresentationNum()
{
	for (StreamInfo const& stream : m_streams)
	{
		if (stream.type == StreamType_Video)
		{
			return stream.representation.size();
		}
	}

	return 0;
}

uint32_t BEAVPlayer::GetAudioRepresentationNum()
{
	if (m_streams.empty())
	{
		return 0;
	}

	return 1;
}

void BEAVPlayer::GetVideoRepresentationInfo(GenericRepresentationInfo *info, uint32_t idx)
{
	int32_t currRepIdx = 0;

	s_liblsInterface.lsStreamlistGetStreamIndex(m_currentLsStreamlist, &currRepIdx);

	for (StreamInfo const& stream : m_streams)
	{
		if (stream.type == StreamType_Video && stream.representation.size() > idx)
		{
			StreamInfo::RepresentationInfo const& repinfo = stream.representation.at(idx);
			info->type = stream.type;
			if (currRepIdx == idx)
			{
				info->currentlySelected = true;
			}

			if (!utils::IsVideoSupported(repinfo.width, repinfo.height))
			{
				info->enabled = false;
			}

			utils::ResolutionToQualityString(info->string, repinfo.width, repinfo.height, info->currentlySelected);
		}
	}
}

void BEAVPlayer::SelectVideoRepresentation(uint32_t idx)
{
	s_liblsInterface.lsStreamlistSelectStream(m_currentLsStreamlist, idx);
}

void BEAVPlayer::GetActiveRepresentationsInfo(StreamInfo::RepresentationInfo *info)
{
	if (!info)
	{
		return;
	}

	int32_t currRepIdx = 0;
	s_liblsInterface.lsStreamlistGetStreamIndex(m_currentLsStreamlist, &currRepIdx);
	s_liblsInterface.lsStreamlistGetStreamBandwidth(m_currentLsStreamlist, currRepIdx, &info->bitrate);
	info->width = m_lastVideoWidth;
	info->height = m_lastVideoHeight;
	info->srate = m_lastAudioSampleRate;
	info->ch = m_lastAudioChannelCount;
}

void BEAVPlayer::SwitchPlaybackState()
{
	sceBeavCorePlayerSwitchPlayState(m_playerCore);
}

bool BEAVPlayer::IsPaused()
{
	return sceBeavCorePlayerIsPaused(m_playerCore);
}

uint32_t BEAVPlayer::GetFreeHeapSize()
{
	return static_cast<memory::HeapAllocator *>(GetGlobalHeapAllocator())->GetFreeSize() - SCE_KERNEL_512KiB;
}

LSResult BEAVPlayer::LsErrorHandler(LSResult error, LSSession *session, LSStreamlist *streamlist, LSStreamfile *streamfile, void *userdata)
{
	if (error != LS_STREAMFILE_ERROR_ABORTED)
	{
		SCE_DBG_LOG_ERROR("[LsErrorHandler] %d", error);
		s_liblsInterface.lsSetErrorHandler(0xFFFFFFFF, NULL, NULL);
		(static_cast<BEAVPlayer *>(userdata))->SetInitState(InitState_InitFail);
	}
	return LS_OK;
}

LSResult BEAVPlayer::LsStatusHandler(LSStatus status, LSSession *session, LSStreamlist *streamlist, LSStreamfile *streamfile, void *userdata)
{
	int32_t ret = LS_OK;
	uint32_t representationCount = 0;

	BEAVPlayer *player = static_cast<BEAVPlayer *>(userdata);

	ret = s_liblsInterface.lsStreamlistGetBandwidthTableCount(streamlist, &representationCount);
	if (ret == LS_OK)
	{
		s_liblsInterface.lsSessionSetBandwidthControl(session, LS_BW_CONTROL_DISABLED);
		player->ParseStreamInfo(streamlist, representationCount);
		s_liblsInterface.lsSetStatusHandler(0xFFFFFFFF, NULL, NULL);
	}

	return LS_OK;
}

// Most of the things in this hook can be ignored, but uMaxM3U8FileSize and uM3U8WorkingMemSize are important,
// sometimes YouTube likes to provide absolute units of a playlist, almost 1MiB in size
LSResult BEAVPlayer::LsInitGlobalsHook(LSLibraryInitParams *params)
{
	//params->uOptionFlags = ;
	//params->uOptionFlags |= ;
	params->uNoContentTimeOutSecs = 10;
	params->uNoConnectionTimeOutSecs = 10;
	params->uM3u8RetrySleepMSecs = 500;
	params->uM3u8RetryTimeOutMSecs = 10000;
	params->uSegmentMaxRetryCount = 5;
	params->uMinBufferPercentBeforeSwitchUp = 50;
	params->uMinSegmentsStreamedBeforeSwitchUp = 2;
	//params->uMaxNumFailOversPerVariant = 10;
	//params->uMaxNumVariantsToCache = 5;
	params->uMaxM3U8FileSize = SCE_KERNEL_1MiB;
	params->uM3U8WorkingMemSize = SCE_KERNEL_512KiB;
	//params->uMinUnPlayedCachedSegmentsBeforeSwitchUp = 2;

	return TAI_NEXT(LsInitGlobalsHook, s_liblsInitHookRef, params);
}

void BEAVPlayer::PreInit()
{
	sceSysmoduleLoadModule(SCE_SYSMODULE_BEISOBMF);
	sceSysmoduleLoadModule(SCE_SYSMODULE_BEMP2SYS);

	Module *beavCoreModule = new Module("vs0:vsh/common/libBEAVCorePlayer.suprx");

	sceBeavCorePlayerGetLibLSInterface(beavCoreModule->GetHandle(), &s_liblsInterface);

	if (utils::IsTaihenLoaded())
	{
		taiHookFunctionOffset(&s_liblsInitHookRef, beavCoreModule->GetHandle(), 0, 0x14908, 1, LsInitGlobalsHook);
	}
	else
	{
		SCE_DBG_LOG_ERROR("[PreInit] Taihen not loaded");
	}

	sceBeavCorePlayerMemmanagerSet(sce_paf_malloc, sce_paf_memalign, sce_paf_free);
	sceBeavCorePlayerMemmanagerSetGetFreeSize(GetFreeHeapSize);

	sceBeavCorePlayerLsHttpSetResolveConfig(1000, 3);
	sceBeavCorePlayerLsSetTimeout(5000, 5000);

	sceBeavCorePlayerSetProperty(SCE_BEAV_CORE_PLAYER_PROP_AVCDEC_MEMBLOCK_TYPE, SCE_KERNEL_MEMBLOCK_TYPE_USER_CDRAM_RW);
	sceBeavCorePlayerSetProperty(SCE_BEAV_CORE_PLAYER_PROP_AVCDEC_INIT_WITH_UNMAPMEM, true);
	sceBeavCorePlayerSetProperty(SCE_BEAV_CORE_PLAYER_PROP_AACDEC_INIT_WITH_UNMAPMEM, true);
	sceBeavCorePlayerSetProperty(SCE_BEAV_CORE_PLAYER_PROP_MP4DEC_START_TIMEOUT, 180000);
	sceBeavCorePlayerSetProperty(SCE_BEAV_CORE_PLAYER_PROP_VDEC_PIXTYPE, SCE_AVCDEC_PIXEL_RGBA8888);

#ifdef _DEBUG
	sceBeavCorePlayerSetLogLevel(SCE_BEAV_CORE_PLAYER_LOG_LEVEL_CRITICAL | SCE_BEAV_CORE_PLAYER_LOG_LEVEL_WARNING | SCE_BEAV_CORE_PLAYER_LOG_LEVEL_VERBOSE);
#endif

	SceBeavCorePlayerLsInputPluginInterface *interface = new SceBeavCorePlayerLsInputPluginInterface();
	LibLSInterface::GetReplacement(interface);
	sceBeavCorePlayerLsHttpSetInputPluginInterface(interface);
}

GenericPlayer::SupportType BEAVPlayer::IsSupported(const char *path)
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
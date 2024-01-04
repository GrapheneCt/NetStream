#include <kernel.h>
#include <kernel/dmacmgr.h>
#include <paf.h>
#include <libdbg.h>
#include <appmgr.h>
#include <libsysmodule.h>
#include <scebeavplayer.h>
#include <avcdec.h>
#include <audioout.h>
#include <psp2_compat/curl/curl.h>

#include "common.h"
#include "utils.h"
#include "event.h"
#include "player_beav.h"

using namespace paf;

static CURL *s_plCurl;
static CURL *s_fileCurl;

static graph::SurfacePool *s_beavSurfacePool = NULL;
static void *s_beavDecodeMem = NULL;

static const char *k_supportedExtensions[] = {
		".ts",
		".mpg",
		".m2ts",
		".mp4",
		".m4v",
		".m4a",
		".m3u8"
};

BEAVPlayer::LibLSInterface::LibLSInterface()
{
	lsOpen = Open;
	lsGetSize = GetSize;
	lsRead = Read;
	lsAbort = Abort;
	lsClose = Close;
	lsGetLastError = GetLastError;
}

BEAVPlayer::LibLSInterface::~LibLSInterface()
{

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
}

void BEAVPlayer::LibLSInterface::Term()
{
	curl_easy_cleanup(s_plCurl);
	curl_easy_cleanup(s_fileCurl);
}

size_t BEAVPlayer::LibLSInterface::DownloadCore(char *buffer, size_t size, size_t nitems, void *userdata)
{
	CurlLsHandle *obj = static_cast<CurlLsHandle *>(userdata);
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

LSInputResult BEAVPlayer::LibLSInterface::Open(char *pcURI, uint64_t uOffset, uint32_t uTimeOutMSecs, LSInputHandle *pHandle)
{
	if (!pcURI)
	{
		return LS_INPUT_ERROR_INVALID_URI_PTR;
	}

	if (!pHandle)
	{
		return LS_INPUT_ERROR_INVALID_HANDLE_PTR;
	}

	CurlLsHandle *obj = new CurlLsHandle();
	if (!obj)
	{
		return LS_INPUT_ERROR_OUT_OF_MEMORY;
	}

	char *ext = NULL;
	bool hasCurl = false;

	ext = sce_paf_strrchr(pcURI, '.');
	if (ext)
	{
		if (!sce_paf_strncmp(ext, ".m3u8", 5))
		{
			obj->m_curl = s_plCurl;
			hasCurl = true;
		}
		else if (!sce_paf_strncmp(ext, ".ts", 3))
		{
			obj->m_curl = s_fileCurl;
			hasCurl = true;
		}
	}

	if (!hasCurl)
	{
		obj->m_curl = curl_easy_init();
		curl_easy_setopt(obj->m_curl, CURLOPT_WRITEFUNCTION, DownloadCore);
		curl_easy_setopt(obj->m_curl, CURLOPT_USERAGENT, USER_AGENT);
		curl_easy_setopt(obj->m_curl, CURLOPT_ACCEPT_ENCODING, "");
		curl_easy_setopt(obj->m_curl, CURLOPT_SSL_VERIFYHOST, 0L);
		curl_easy_setopt(obj->m_curl, CURLOPT_SSL_VERIFYPEER, 0L);
		curl_easy_setopt(obj->m_curl, CURLOPT_FOLLOWLOCATION, 1L);
		curl_easy_setopt(obj->m_curl, CURLOPT_TCP_KEEPALIVE, 1L);
		curl_easy_setopt(obj->m_curl, CURLOPT_NOPROGRESS, 1L);
	}

	curl_easy_setopt(obj->m_curl, CURLOPT_WRITEDATA, obj);
	curl_easy_setopt(obj->m_curl, CURLOPT_URL, pcURI);
	curl_easy_setopt(obj->m_curl, CURLOPT_CONNECTTIMEOUT_MS, uTimeOutMSecs);

	if (uOffset)
	{
		curl_easy_setopt(obj->m_curl, CURLOPT_RESUME_FROM_LARGE, uOffset);
	}

	CURLcode ret = curl_easy_perform(obj->m_curl);
	if (ret != CURLE_OK)
	{
		SCE_DBG_LOG_ERROR("[BEAV] LibLSInterface: curl_easy_perform returned %d\n", ret);
		obj->m_lastError = ret;
		return ConvertError(ret);
	}

	*pHandle = static_cast<LSInputHandle>(obj);

	return LS_INPUT_OK;
}

LSInputResult BEAVPlayer::LibLSInterface::GetSize(LSInputHandle handle, uint64_t *pSize)
{
	if (!handle)
	{
		return LS_INPUT_ERROR_INVALID_HANDLE;
	}

	if (!pSize)
	{
		return LS_INPUT_ERROR_INVALID_SIZE_PTR;
	}

	CurlLsHandle *obj = static_cast<CurlLsHandle *>(handle);

	*pSize = static_cast<uint64_t>(obj->m_pos);

	return LS_INPUT_OK;
}

LSInputResult BEAVPlayer::LibLSInterface::Read(LSInputHandle handle, void *pBuffer, uint32_t uSize, uint32_t uTimeOutMSecs, uint32_t *pReadSize)
{
	if (!handle)
	{
		return LS_INPUT_ERROR_INVALID_HANDLE;
	}

	if (!pBuffer)
	{
		return LS_INPUT_ERROR_INVALID_BUFFER_PTR;
	}

	if (!pReadSize)
	{
		return LS_INPUT_ERROR_INVALID_SIZE_PTR;
	}

	CurlLsHandle *obj = static_cast<CurlLsHandle *>(handle);

	if (obj->m_readPos + uSize > obj->m_pos)
	{
		uSize = obj->m_pos - obj->m_readPos;
	}
	if (uSize != 0)
	{
		sce_paf_memcpy(pBuffer, static_cast<char *>(obj->m_buf) + obj->m_readPos, uSize);
	}
	else
	{
		sce_paf_free(obj->m_buf);
		obj->m_buf = NULL;
		obj->m_pos = 0;
		obj->m_readPos = 0;
	}
	obj->m_readPos += uSize;

	*pReadSize = uSize;

	return LS_INPUT_OK;
}

LSInputResult BEAVPlayer::LibLSInterface::Abort(LSInputHandle handle)
{
	if (!handle)
	{
		return LS_INPUT_ERROR_INVALID_HANDLE;
	}

	CurlLsHandle *obj = static_cast<CurlLsHandle *>(handle);

	if (obj->m_buf)
	{
		sce_paf_free(obj->m_buf);
		obj->m_buf = NULL;
		obj->m_pos = 0;
		obj->m_readPos = 0;
	}

	return LS_INPUT_OK;
}

LSInputResult BEAVPlayer::LibLSInterface::Close(LSInputHandle *pHandle)
{
	if (!pHandle)
	{
		return LS_INPUT_ERROR_INVALID_HANDLE_PTR;
	}

	if (!(*pHandle))
	{
		return LS_INPUT_ERROR_INVALID_HANDLE;
	}

	CurlLsHandle *obj = static_cast<CurlLsHandle *>(*pHandle);

	Abort(*pHandle);
	if (obj->m_curl != s_plCurl && obj->m_curl != s_fileCurl)
	{
		curl_easy_cleanup(obj->m_curl);
	}
	sce_paf_free(*pHandle);

	return LS_INPUT_OK;
}

LSInputResult BEAVPlayer::LibLSInterface::GetLastError(LSInputHandle handle, uint32_t *pNativeError)
{
	if (!handle)
	{
		return LS_INPUT_ERROR_INVALID_HANDLE;
	}

	if (!pNativeError)
	{
		return LS_INPUT_ERROR_INVALID_HANDLE;
	}

	CurlLsHandle *obj = static_cast<CurlLsHandle *>(handle);

	*pNativeError = obj->m_lastError;

	return LS_INPUT_OK;
}

void BEAVPlayer::BEAVAudioThread::EntryFunction()
{
	int32_t err = false;
	SceBeavCorePlayerAudioData data;

	SCE_DBG_LOG_INFO("[BEAV] BEAVAudioThread start\n");

	while (err != true)
	{
		err = sceBeavCorePlayerGetAudioData(m_playerCore, &data);
		if (IsCanceled())
		{
			Cancel();
			return;
		}
		thread::Sleep(10);
	}

	SCE_DBG_LOG_INFO("[BEAV] Audio: uSampleRate: %u, uChannelCount: %u\n", data.uSampleRate, data.uChannelCount);

	m_parent->SetInitState(InitState_InitOk);

	void *silence = sce_paf_malloc(data.uBufSize);
	sce_paf_memset(silence, 0, data.uBufSize);

	int32_t port = sceAudioOutOpenPort(SCE_AUDIO_OUT_PORT_TYPE_VOICE, data.uPcmSize, data.uSampleRate, data.uChannelCount - 1);
	int32_t maxVolumeArray[2] = { SCE_AUDIO_VOLUME_0DB, SCE_AUDIO_VOLUME_0DB };
	sceAudioOutSetVolume(port, SCE_AUDIO_VOLUME_FLAG_L_CH | SCE_AUDIO_VOLUME_FLAG_R_CH, maxVolumeArray);

	while (!IsCanceled())
	{
		err = sceBeavCorePlayerGetAudioData(m_playerCore, &data);
		if (err == true)
			sceAudioOutOutput(port, data.pBuffer);
		else
			sceAudioOutOutput(port, silence);
	}

	sceAudioOutReleasePort(port);

	sce_paf_free(silence);

	Cancel();
}

void BEAVPlayer::BEAVVideoThread::SurfaceUpdateTask(void *pArgBlock)
{
	SceBeavCorePlayerVideoData data;
	SceBeavCorePlayerAdvanceInfo adata;
	int32_t err = false;
	BEAVVideoThread *vthread = static_cast<BEAVVideoThread *>(pArgBlock);

	err = sceBeavCorePlayerGetVideoData(vthread->m_playerCore, &data);
	if (vthread->m_limitFps || vthread->m_parent->m_powerSaving)
	{
		sceDisplayWaitVblankStartMulti(2);
	}
	if (data.pFrameBuffer != NULL && !vthread->IsCanceled())
	{
		sceBeavCorePlayerAdvance(vthread->m_playerCore, data.pObj, &adata);
		if ((err == true || vthread->m_limitFps) && !vthread->m_parent->m_powerSaving)
		{
			int32_t currIdx = vthread->m_surfIdx;
			void *dst = NULL;
			if (data.uTexWidth == data.uTexPitch)
			{
				dst = vthread->m_drawSurf[currIdx]->Lock();
				sceDmacMemcpy(dst, data.pFrameBuffer, data.uTexWidth * data.uTexHeight * 4);
				vthread->m_drawSurf[currIdx]->Unlock();
			}
			else
			{
				if (Framework::Instance()->GetMode() == Framework::Mode_Normal)
				{
					dst = vthread->m_drawSurf[currIdx]->Lock();
					sceGxmTransferCopy(
						data.uTexWidth,
						data.uTexHeight,
						0,
						0,
						SCE_GXM_TRANSFER_COLORKEY_NONE,
						SCE_GXM_TRANSFER_FORMAT_U8U8U8U8_ABGR,
						SCE_GXM_TRANSFER_LINEAR,
						data.pFrameBuffer,
						0,
						0,
						data.uTexPitch * 4,
						SCE_GXM_TRANSFER_FORMAT_U8U8U8U8_ABGR,
						SCE_GXM_TRANSFER_LINEAR,
						dst,
						0,
						0,
						data.uTexWidth * 4,
						NULL,
						0,
						NULL
					);
					sceGxmTransferFinish();
					vthread->m_drawSurf[currIdx]->Unlock();
				}
				else
				{
					vthread->m_drawSurf[currIdx]->Copy(0, data.pFrameBuffer, ImageOrder_Linear, data.uTexPitch * 4);
				}
			}
			vthread->m_target->SetTexture(vthread->m_drawSurf[currIdx]);
			vthread->m_surfIdx ^= 1;
		}
	}
}

void BEAVPlayer::BEAVVideoThread::EntryFunction()
{
	int32_t err = false;
	SceBeavCorePlayerVideoData data;
	m_surfIdx = 0;

	SCE_DBG_LOG_INFO("[BEAV] BEAVVideoThread start\n");

	data.pFrameBuffer = NULL;

	while (err == false || data.pFrameBuffer == NULL)
	{
		sceBeavCorePlayerGetVideoData(m_playerCore, &data);
		err = sceBeavCorePlayerIsReady(m_playerCore);
		if (IsCanceled())
		{
			Cancel();
			return;
		}
		thread::Sleep(10);
	}

	SCE_DBG_LOG_INFO("[BEAV] Video: uTexWidth: %d, uTexPitch: %d, uTexHeight: %d\n", data.uTexWidth, data.uTexPitch, data.uTexHeight);

	//float aspect = (float)data.uTexWidth / (float)data.uTexHeight;
	//if (aspect > 1.78f || aspect < 1.76f)
	{
		//SCE_DBG_LOG_INFO("[BEAV] Suspicious aspect ratio (%.03f), so respect it\n", aspect);
		graph::PlaneObj *po = static_cast<graph::PlaneObj *>(m_target->GetDrawObj(ui::Plane::OBJ_PLANE));
		po->SetScaleMode(graph::PlaneObj::SCALE_ASPECT_SIZE, graph::PlaneObj::SCALE_ASPECT_SIZE);
	}

	for (int i = 0; i < BEAV_SURFACE_COUNT; i++)
	{
		m_drawSurf[i] = new graph::Surface(s_beavSurfacePool, data.uTexWidth, data.uTexHeight, ImageMode_RGBA8888, ImageOrder_Linear, 1, 1, 0);
	}

	common::MainThreadCallList::Register(SurfaceUpdateTask, this);

	while (!IsCanceled())
	{
		thread::Sleep(100);
	}

	common::MainThreadCallList::Unregister(SurfaceUpdateTask, this);

	m_target->SetTexture(g_texTransparent);

	for (int i = 0; i < BEAV_SURFACE_COUNT; i++)
	{
		m_drawSurf[i].clear();
	}

	Cancel();
}

void BEAVPlayer::BootJob::Run()
{
	int32_t ret = false;
	m_parent->m_playerCore = 0;
	m_parent->m_videoThread = NULL;
	m_parent->m_audioThread = NULL;
	m_parent->m_notifyCbMem = NULL;

	ret = sceBeavCorePlayerInitialize();
	if (!ret)
	{
		m_parent->SetInitState(InitState_InitFail);
		return;
	}

	m_parent->m_playerCore = sceBeavCorePlayerCreate();
	if (!m_parent->m_playerCore)
	{
		m_parent->SetInitState(InitState_InitFail);
		return;
	}

	m_parent->m_notifyCbMem = sce_paf_malloc(64);
	sceBeavCorePlayerSetNotifyCallback(m_parent->m_playerCore, m_parent->m_notifyCbMem, PlayerNotifyCb);

	sceBeavCorePlayerSetAgent(m_parent->m_playerCore, USER_AGENT);

	ret = sceBeavCorePlayerSetVideoBuffer(m_parent->m_playerCore, BEAV_VIDEO_BUFFER_WIDTH, BEAV_VIDEO_BUFFER_HEIGHT, BEAV_VIDEO_BUFFER_COUNT, s_beavDecodeMem);
	if (ret != 0)
	{
		m_parent->SetInitState(InitState_InitFail);
		return;
	}

	ret = sceBeavCorePlayerSetPgdCachePath(m_parent->m_playerCore, "", SCE_KERNEL_16MiB, 0x21, SCE_KERNEL_64KiB, -1);
	if (ret != 0)
	{
		m_parent->SetInitState(InitState_InitFail);
		return;
	}

	sceBeavCorePlayerSetHlsBandwidthOpt(m_parent->m_playerCore, SCE_BEAV_CORE_PLAYER_BW_SELECT_HIGHEST, 2920, 2000, 4000);

	ret = sceBeavCorePlayerOpenTargetUrl(m_parent->m_playerCore, m_parent->m_path.c_str(), true);
	if (ret != 1)
	{
		m_parent->SetInitState(InitState_InitFail);
		return;
	}

	thread::Thread::Option opt;

	opt.affinity = SCE_KERNEL_CPU_MASK_USER_1;
	m_parent->m_videoThread = new BEAVVideoThread(m_parent, m_parent->m_target, m_parent->m_playerCore, m_parent->m_limitFps, &opt);

	opt.affinity = SCE_KERNEL_CPU_MASK_USER_2;
	m_parent->m_audioThread = new BEAVAudioThread(m_parent, m_parent->m_playerCore, &opt);

	m_parent->m_videoThread->Start();
	m_parent->m_audioThread->Start();
}

BEAVPlayer::BEAVPlayer(ui::Widget *targetPlane, const char *url)
{
	SCE_DBG_LOG_INFO("[BEAV] Open url: %s\n", url);
	m_target = targetPlane;
	m_path = url;
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
		if (m_playerCore)
		{
			sceBeavCorePlayerStop(m_playerCore);
		}

		if (m_videoThread)
		{
			m_videoThread->Cancel();
			m_videoThread->Join();
			delete m_videoThread;
		}

		if (m_audioThread)
		{
			m_audioThread->Cancel();
			m_audioThread->Join();
			delete m_audioThread;
		}

		if (m_playerCore)
		{
			sceBeavCorePlayerDestroy(m_playerCore);
		}

		sceBeavCorePlayerFinalize();

		if (m_notifyCbMem)
		{
			sce_paf_free(m_notifyCbMem);
		}
	}

	SetInitState(InitState_NotInit);
}

GenericPlayer::InitState BEAVPlayer::GetInitState()
{
	return m_initState;
}

void BEAVPlayer::SetInitState(GenericPlayer::InitState state)
{
	m_initState = state;
	SCE_DBG_LOG_INFO("[BEAV] State changed to: %d\n", state);
	event::BroadcastGlobalEvent(g_appPlugin, GenericPlayerChangeState, m_initState);
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
	SCE_DBG_LOG_INFO("[BEAV] JumpToTime | seconds: %u\n", time / 1000);
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

void BEAVPlayer::LimitFPS(bool enable)
{
	m_limitFps = enable;
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
	return memory::GetGlobalHeapAllocator()->GetFreeSize() - SCE_KERNEL_512KiB;
}

void BEAVPlayer::PreInit()
{
	sceSysmoduleLoadModule(SCE_SYSMODULE_BEISOBMF);
	sceSysmoduleLoadModule(SCE_SYSMODULE_BEMP2SYS);

	new Module("app0:module/libSceHttpForBEAVCorePlayer.suprx");
	new Module("app0:module/libBEAVCorePlayer.suprx");

	sceBeavCorePlayerMemmanagerSet(sce_paf_malloc, sce_paf_memalign, sce_paf_free);
	sceBeavCorePlayerMemmanagerSetGetFreeSize(GetFreeHeapSize);

	sceBeavCorePlayerLsHttpSetResolveConfig(1000, 3);
	sceBeavCorePlayerLsSetTimeout(5000, 5000);

	sceBeavCorePlayerSetProperty(SCE_BEAV_CORE_PLAYER_PROP_AVCDEC_MEMBLOCK_TYPE, SCE_KERNEL_MEMBLOCK_TYPE_USER_CDRAM_RW);
	sceBeavCorePlayerSetProperty(SCE_BEAV_CORE_PLAYER_PROP_AVCDEC_INIT_WITH_UNMAPMEM, true);
	sceBeavCorePlayerSetProperty(SCE_BEAV_CORE_PLAYER_PROP_AACDEC_INIT_WITH_UNMAPMEM, true);
	sceBeavCorePlayerSetProperty(SCE_BEAV_CORE_PLAYER_PROP_MP4DEC_START_TIMEOUT, 180000);
	sceBeavCorePlayerSetProperty(SCE_BEAV_CORE_PLAYER_PROP_VDEC_PIXTYPE, SCE_AVCDEC_PIXEL_RGBA8888);
	sceBeavCorePlayerSetProperty(SCE_BEAV_CORE_PLAYER_PROP_RINGBUF_THREAD_STACK_SIZE, SCE_KERNEL_128KiB);

#ifdef _DEBUG
	sceBeavCorePlayerSetLogLevel(SCE_BEAV_CORE_PLAYER_LOG_LEVEL_CRITICAL | SCE_BEAV_CORE_PLAYER_LOG_LEVEL_WARNING | SCE_BEAV_CORE_PLAYER_LOG_LEVEL_VERBOSE);
#endif

	LibLSInterface::Init();
	sceBeavCorePlayerLsHttpSetInputPluginInterface(reinterpret_cast<SceBeavCorePlayerLsInputPluginInterface *>(new LibLSInterface()));

	uint32_t surfacePoolSize = BEAV_VIDEO_BUFFER_WIDTH * BEAV_VIDEO_BUFFER_HEIGHT * 4 * BEAV_VIDEO_BUFFER_COUNT * BEAV_SURFACE_COUNT;
	void *poolMem = GraphMem::AllocOSMemory(GraphMem::DeviceType_VideoMemory, surfacePoolSize + SCE_KERNEL_1KiB, "BEAVPlayer::DecodeMemoryPool");
	s_beavSurfacePool = new graph::SurfacePool(poolMem, surfacePoolSize, "BEAVPlayer::DecodeMemoryPool");
	s_beavDecodeMem = s_beavSurfacePool->Allocate(64, BEAV_VIDEO_BUFFER_WIDTH * BEAV_VIDEO_BUFFER_HEIGHT * 4 * BEAV_VIDEO_BUFFER_COUNT);
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

void BEAVPlayer::PlayerNotifyCb(int32_t reserved, SceBeavCorePlayerDecodeError *eventInfo)
{
	SCE_DBG_LOG_ERROR("[BEAV] PlayerNotifyCb, error type: %u, error code: 0x%X\n", eventInfo->eType, eventInfo->nErrCode);
}
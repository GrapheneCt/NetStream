#ifndef _BEAV_PLAYER_H_
#define _BEAV_PLAYER_H_

#include <kernel.h>
#include <paf.h>
#include <scebeavplayer.h>
#include <taihen.h>

#include "common.h"
#include "players/player_generic.h"
#include <paf_file_ext.h>

using namespace paf;

#define BEAV_SURFACE_COUNT			2
#define BEAV_VIDEO_BUFFER_COUNT		4
#define BEAV_VIDEO_BUFFER_WIDTH		1280
#define BEAV_VIDEO_BUFFER_HEIGHT	720
#define BEAV_DECODE_BUFFER_SIZE		(BEAV_VIDEO_BUFFER_WIDTH * BEAV_VIDEO_BUFFER_HEIGHT * 4 * BEAV_VIDEO_BUFFER_COUNT)
#define BEAV_USER_AGENT				USER_AGENT		

class BEAVPlayer : public GenericPlayer
{
public:

	class Option : public GenericPlayer::Option
	{
	public:

		enum Resolution : int32_t
		{
			Resolution_144p,
			Resolution_240p,
			Resolution_360p,
			Resolution_480p,
			Resolution_720p
		};

		Option()
		{
			playerType = PlayerType_BEAV;
			defaultRes = Resolution_480p;
		}

		Resolution defaultRes;
	};

	class LibLSInterface
	{
	public:

		static void Init();
		static void Term();

		static void GetReplacement(SceBeavCorePlayerLsInputPluginInterface *repl);

	private:

		LibLSInterface();
		~LibLSInterface();

		static size_t DownloadCore(char *buffer, size_t size, size_t nitems, void *userdata);
		static LSInputResult ConvertError(int err);

		LSInputResult Open(char *pcURI, uint64_t uOffset, uint32_t uTimeOutMSecs);
		LSInputResult GetSize(uint64_t *pSize);
		LSInputResult Read(void *pBuffer, uint32_t uSize, uint32_t uTimeOutMSecs, uint32_t *pReadSize);
		LSInputResult Abort();
		LSInputResult GetLastError(uint32_t *pNativeError);

		static CURL *s_fileCurl;
		static CURL *s_plCurl;
		static thread::RMutex s_plMtx;
		static thread::RMutex s_fileMtx;

		void *m_curl;
		int64_t m_readPos;
		void *m_buf;
		uint32_t m_pos;
		int32_t m_lastError;
	};

	class BootJob : public job::JobItem
	{
	public:

		BootJob(BEAVPlayer *parent) : job::JobItem("BEAVPlayer::BootJob", NULL), m_parent(parent)
		{

		}

		~BootJob() {}

		int32_t Run()
		{
			m_parent->OnBootJob();
			return SCE_PAF_OK;
		}

		void Finish(int32_t result) {}

	private:

		BEAVPlayer *m_parent;
	};

	BEAVPlayer(ui::Widget *targetPlane, const char *url, Option *opt);

	~BEAVPlayer();

	void InitAsync();

	void Term();

	InitState GetInitState();

	uint32_t GetTotalTimeMs();

	uint32_t GetCurrentTimeMs();

	bool JumpToTimeMs(uint32_t time);

	void SwitchPlaybackState();

	bool IsPaused();

	PlayerState GetState();

	void SetPowerSaving(bool enable);

	uint32_t GetVideoRepresentationNum();

	uint32_t GetAudioRepresentationNum();

	void GetVideoRepresentationInfo(GenericRepresentationInfo *info, uint32_t idx);

	void SelectVideoRepresentation(uint32_t idx);

	void GetActiveRepresentationsInfo(StreamInfo::RepresentationInfo *info);

	static void PreInit();

	static SupportType IsSupported(const char *path);

private:

	class BEAVAudioThread : public thread::Thread
	{
	public:

		BEAVAudioThread(BEAVPlayer *parent, SceBeavCorePlayerHandle core, Option *option) :
			thread::Thread(SCE_KERNEL_HIGHEST_PRIORITY_USER + 1, SCE_KERNEL_16KiB, "BEAVAudioThread", option)
		{
			m_parent = parent;
			m_playerCore = core;
		}

		void EntryFunction();

	private:

		BEAVPlayer *m_parent;
		SceBeavCorePlayerHandle m_playerCore;
	};

	static void SurfaceUpdateTask(void *pArgBlock);

	static uint32_t GetFreeHeapSize();

	static LSResult LsErrorHandler(LSResult error, LSSession *session, LSStreamlist *streamlist, LSStreamfile *streamfile, void *userdata);

	static LSResult LsStatusHandler(LSStatus status, LSSession *session, LSStreamlist *streamlist, LSStreamfile *streamfile, void *userdata);

	static LSResult LsInitGlobalsHook(LSLibraryInitParams *params);

	void OnSurfaceUpdate();
	void OnBootJob();

	void SetInitState(InitState state);

	void ParseStreamInfo(LSStreamlist *streamlist, uint32_t scount);

	static SceBeavCorePlayerLibLSInterface s_liblsInterface;
	static tai_hook_ref_t s_liblsInitHookRef;

	SceBeavCorePlayerHandle m_playerCore;
	BEAVAudioThread *m_audioThread;
	void *m_decodeBuffer;
	LSStreamlist *m_currentLsStreamlist;
	Option m_opt;
	uint32_t m_lastVideoWidth;
	uint32_t m_lastVideoHeight;
	uint32_t m_lastAudioChannelCount;
	uint32_t m_lastAudioSampleRate;
	bool m_bootEarlyExit;
	paf::map<uint64_t, intrusive_ptr<graph::Surface>> m_drawSurf;
};

#endif
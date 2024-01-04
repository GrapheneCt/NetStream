#ifndef _BEAV_PLAYER_H_
#define _BEAV_PLAYER_H_

#include <kernel.h>
#include <paf.h>
#include <scebeavplayer.h>

#include "common.h"
#include "player_generic.h"
#include <paf_file_ext.h>

using namespace paf;

#define BEAV_SURFACE_COUNT			2
#define BEAV_VIDEO_BUFFER_COUNT		4
#define BEAV_VIDEO_BUFFER_WIDTH		1280
#define BEAV_VIDEO_BUFFER_HEIGHT	720
#define BEAV_USER_AGENT USER_AGENT		

class BEAVPlayer : public GenericPlayer
{
public:

	class LibLSInterface
	{
	public:

		class CurlLsHandle
		{
		public:

			CurlLsHandle()
			{
				m_curl = NULL;
				m_readPos = 0;
				m_buf = NULL;
				m_pos = 0;
				m_lastError = 0;
			}

			~CurlLsHandle() {};

			void *m_curl;
			int64_t m_readPos;
			void *m_buf;
			uint32_t m_pos;
			int32_t m_lastError;
		};

		LibLSInterface();
		~LibLSInterface();

		static void Init();
		static void Term();
		static size_t DownloadCore(char *buffer, size_t size, size_t nitems, void *userdata);
		static LSInputResult ConvertError(int err);
		static LSInputResult Open(char *pcURI, uint64_t uOffset, uint32_t uTimeOutMSecs, LSInputHandle *pHandle);
		static LSInputResult GetSize(LSInputHandle handle, uint64_t *pSize);
		static LSInputResult Read(LSInputHandle handle, void *pBuffer, uint32_t uSize, uint32_t uTimeOutMSecs, uint32_t *pReadSize);
		static LSInputResult Abort(LSInputHandle handle);
		static LSInputResult Close(LSInputHandle *pHandle);
		static LSInputResult GetLastError(LSInputHandle handle, uint32_t *pNativeError);

		LSInputResult(*lsOpen)(char *pcURI, uint64_t uOffset, uint32_t uTimeOutMSecs, LSInputHandle *pHandle);
		LSInputResult(*lsGetSize)(LSInputHandle handle, uint64_t *pSize);
		LSInputResult(*lsRead)(LSInputHandle handle, void *pBuffer, uint32_t uSize, uint32_t uTimeOutMSecs, uint32_t *pReadSize);
		LSInputResult(*lsAbort)(LSInputHandle handle);
		LSInputResult(*lsClose)(LSInputHandle *pHandle);
		LSInputResult(*lsGetLastError)(LSInputHandle handle, uint32_t *pNativeError);
	};

	class BootJob : public job::JobItem
	{
	public:

		BootJob(BEAVPlayer *parent) : job::JobItem("BEAVPlayer::BootJob", NULL), m_parent(parent)
		{

		}

		~BootJob() {}

		void Run();

		void Finish() {}

	private:

		BEAVPlayer *m_parent;
	};

	BEAVPlayer(ui::Widget *targetPlane, const char *url);

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

	void LimitFPS(bool enable);

	static void PreInit();

	static SupportType IsSupported(const char *path);

private:

	class BEAVVideoThread : public thread::Thread
	{
	public:

		BEAVVideoThread(BEAVPlayer *parent, ui::Widget *target, SceBeavCorePlayerHandle core, bool limitFps, Option *option) :
			thread::Thread(SCE_KERNEL_HIGHEST_PRIORITY_USER + 10, SCE_KERNEL_16KiB, "BEAVVideoThread", option)
		{
			m_parent = parent;
			m_target = target;
			m_playerCore = core;
			m_limitFps = limitFps;
		}

		void EntryFunction();

	private:

		static void SurfaceUpdateTask(void *pArgBlock);

		BEAVPlayer *m_parent;
		ui::Widget *m_target;
		intrusive_ptr<graph::Surface> m_drawSurf[BEAV_SURFACE_COUNT];
		int32_t m_surfIdx;
		SceBeavCorePlayerHandle m_playerCore;
		bool m_limitFps;
	};

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

	static uint32_t GetFreeHeapSize();

	static void PlayerNotifyCb(int32_t reserved, SceBeavCorePlayerDecodeError *eventInfo);

	void SetInitState(InitState state);

	SceBeavCorePlayerHandle m_playerCore;
	BEAVVideoThread *m_videoThread;
	BEAVAudioThread *m_audioThread;
	void *m_notifyCbMem;
};

#endif
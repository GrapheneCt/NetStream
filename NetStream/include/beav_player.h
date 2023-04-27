#ifndef _BEAV_PLAYER_H_
#define _BEAV_PLAYER_H_

#include <kernel.h>
#include <paf.h>
#include <scebeavplayer.h>

#include "common.h"
#include <paf_file_ext.h>

using namespace paf;

#define BEAV_SURFACE_COUNT			2
#define BEAV_VIDEO_BUFFER_COUNT		4
#define BEAV_VIDEO_BUFFER_WIDTH		1280
#define BEAV_VIDEO_BUFFER_HEIGHT	720
#define BEAV_USER_AGENT				

class BEAVPlayer
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
				curl = NULL;
				readPos = 0;
				buf = NULL;
				pos = 0;
				lastError = 0;
			}

			~CurlLsHandle() {};

			void *curl;
			int64_t readPos;
			void *buf;
			uint32_t pos;
			int32_t lastError;
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

		using job::JobItem::JobItem;

		~BootJob() {}

		void Run();

		void Finish() {}

		BEAVPlayer *workObj;
	};

	enum
	{
		BEAVPlayerChangeState = (ui::Handler::CB_STATE + 0x20000),
	};

	enum SupportType
	{
		SupportType_NotSupported,
		SupportType_Supported,
		SupportType_MaybeSupported
	};

	enum InitState
	{
		InitState_NotInit,
		InitState_InProgress,
		InitState_InitOk,
		InitState_InitFail
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

	uint32_t GetPlaySpeed();

	void SetPlaySpeed(int32_t speed, int32_t milliSec);

	SceBeavCorePlayerState GetState();

	void SetPowerSaving(bool enable);

	void LimitFPS(bool enable);

	static void PreInit();

	static SupportType IsSupported(const char *path);

private:

	class BEAVVideoThread : public thread::Thread
	{
	public:

		using thread::Thread::Thread;

		void EntryFunction();

		static void SurfaceUpdateTask(void *pArgBlock);

		BEAVPlayer *workObj;
		ui::Widget *target;
		intrusive_ptr<graph::Surface> drawSurf[BEAV_SURFACE_COUNT];
		int32_t surfIdx;
		SceBeavCorePlayerHandle playerCore;
		bool limitFps;
	};

	class BEAVAudioThread : public thread::Thread
	{
	public:

		using thread::Thread::Thread;

		void EntryFunction();

		BEAVPlayer *workObj;
		SceBeavCorePlayerHandle playerCore;
	};

	static uint32_t GetFreeHeapSize();

	static void PlayerNotifyCb(int32_t reserved, SceBeavCorePlayerDecodeError *eventInfo);

	void SetInitState(InitState state);

	ui::Widget *target;
	string path;
	InitState initState;
	bool limitFps;
	bool powerSaving;

	SceBeavCorePlayerHandle playerCore;
	BEAVVideoThread *videoThread;
	BEAVAudioThread *audioThread;
	void *notifyCbMem;
};

#endif
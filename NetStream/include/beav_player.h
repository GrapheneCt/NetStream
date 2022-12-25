#ifndef _BEAV_PLAYER_H_
#define _BEAV_PLAYER_H_

#include <kernel.h>
#include <paf.h>
#include <scebeavplayer.h>

#include "common.h"
#include "curl_file.h"

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
				curl = SCE_NULL;
				readPos = 0;
				buf = SCE_NULL;
				pos = 0;
				lastError = 0;
			}

			~CurlLsHandle() {};

			ScePVoid curl;
			SceOff readPos;
			ScePVoid buf;
			SceUInt32 pos;
			SceInt32 lastError;
		};

		LibLSInterface();
		~LibLSInterface();

		static SceVoid Init();
		static SceVoid Term();
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

		SceVoid Run();

		SceVoid Finish() {}

		BEAVPlayer *workObj;
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

	SceVoid InitAsync();

	SceVoid Term();

	InitState GetInitState();

	SceUInt32 GetTotalTimeMs();

	SceUInt32 GetCurrentTimeMs();

	SceBool JumpToTimeMs(SceUInt32 time);

	SceVoid SwitchPlaybackState();

	SceBool IsPaused();

	SceUInt32 GetPlaySpeed();

	SceVoid SetPlaySpeed(SceInt32 speed, SceInt32 milliSec);

	SceBeavCorePlayerState GetState();

	SceVoid SetPowerSaving(SceBool enable);

	SceVoid LimitFPS(SceBool enable);

	static SceVoid PreInit();

	static SupportType IsSupported(const char *path);

	static SceVoid PlayerNotifyCb(SceInt32 reserved, SceBeavCorePlayerDecodeError *eventInfo);

	class BEAVVideoThread : public thread::Thread
	{
	public:

		using thread::Thread::Thread;

		SceVoid EntryFunction();

		static SceVoid SurfaceUpdateTask(void *pArgBlock);

		BEAVPlayer *workObj;
		ui::Widget *target;
		graph::Surface *drawSurf[BEAV_SURFACE_COUNT];
		SceInt32 surfIdx;
		SceBeavCorePlayerHandle playerCore;
		SceBool limitFps;
	};

	class BEAVAudioThread : public thread::Thread
	{
	public:

		using thread::Thread::Thread;

		SceVoid EntryFunction();

		SceBeavCorePlayerHandle playerCore;
	};

	static SceUInt32 GetFreeHeapSize();

	ui::Widget *target;
	string path;
	InitState initState;
	SceBool limitFps;
	SceBool powerSaving;

	SceBeavCorePlayerHandle playerCore;
	BEAVVideoThread *videoThread;
	BEAVAudioThread *audioThread;
	ScePVoid notifyCbMem;
};

#endif
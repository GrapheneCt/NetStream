#ifndef _AV_PLAYER_H_
#define _AV_PLAYER_H_

#include <kernel.h>
#include <paf.h>
#include <sceavplayer_webmaf.h>
#include <psp2_compat/curl/curl.h>

#include "common.h"
#include "players/player_generic.h"
#include <paf_file_ext.h>

using namespace paf;

class AVPlayer : public GenericPlayer
{
public:

	class Option : public GenericPlayer::Option
	{
	public:

	};

	class BootJob : public job::JobItem
	{
	public:

		BootJob(AVPlayer *parent) : job::JobItem("AVPlayer::BootJob", NULL), m_parent(parent)
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

		AVPlayer *m_parent;
	};

	AVPlayer(ui::Widget *targetPlane, const char *url, Option *opt);

	~AVPlayer();

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

	void GetActiveRepresentationsInfo(StreamInfo::RepresentationInfo *info);

	static void PreInit();

	static SupportType IsSupported(const char *path);

private:

	enum SeekState
	{
		SeekState_Ok,
		SeekState_Waiting,
		SeekState_Failed
	};

	class SmoothStreaming
	{
	public:

		SmoothStreaming();
		~SmoothStreaming();

		static void GetLiveReplacement(SceAvPlayerHttpReplacement *repl);
		static void GetVODReplacement(SceAvPlayerFileReplacement *repl);

	private:

		static size_t CurlDownload(char *buffer, size_t size, size_t nitems, void *userdata);

		int64_t LivePostImpl(char *url, bool continuation,
			SceAvPlayerHTTPHeader *headers, uint8_t numHeaders, uint8_t *body, uint32_t len,
			uint8_t *buf, uint32_t bufLen, bool *completed);
		int64_t LiveGetImpl(char *url, bool continuation, uint64_t rangeStart, uint64_t rangeEnd, uint8_t *buf, uint32_t bufLen, bool *completed);
		int32_t GetLastStatusImpl();
		int64_t GetFileSizeImpl();

		int32_t VodOpenFileImpl(const char *argFilename);
		int32_t VodCloseFileImpl();
		int32_t VodReadOffsetFileImpl(uint8_t *argBuffer, uint64_t argPosition, uint32_t argLength);
		uint64_t VodSizeFileImpl();

		static SmoothStreaming *s_vodObjStorage;

		CURL *m_curl;
		int32_t m_lastError;
		bool m_hasRange;
		bool m_downloadDone;
		uint32_t m_total;
		uint32_t m_readSz;
		uint64_t m_rangeStart;
		uint64_t m_rangeEnd;
		uint8_t *m_tmpBuf;
		common::SharedPtr<File> m_vod;
		common::SharedPtr<Buffer> m_vodBuf;
		PredictiveFile *m_prVod;
	};

	class AVAudioThread : public thread::Thread
	{
	public:

		AVAudioThread(AVPlayer *parent, SceAvPlayerHandle core, Option *option) :
			thread::Thread(SCE_KERNEL_DEFAULT_PRIORITY_USER - 10, SCE_KERNEL_16KiB, "AVAudioThread", option)
		{
			m_parent = parent;
			m_playerCore = core;
		}

		void EntryFunction();

	private:

		AVPlayer *m_parent;
		SceAvPlayerHandle m_playerCore;
	};

	static void SurfaceUpdateTask(void *pArgBlock);

	static void *PlayerAllocate(void *jumpback, uint32_t alignment, uint32_t size);

	static void PlayerDeallocate(void *jumpback, void *pMemory);

	static void *PlayerAllocateTexture(void *jumpback, uint32_t alignment, uint32_t size);

	static void PlayerDeallocateTexture(void *jumpback, void *pMemory);

	static void PlayerChangeStateCb(void *instance, int32_t argEventId, int32_t argSourceId, void *argEventData);

	void OnSurfaceUpdate();
	void OnBootJob();
	void OnPlayerWarning(const char *format, _Va_list ap);

	void SetState(int32_t argEventId);

	void SetInitState(InitState state);

	void ParseStreamInfo();

	SceAvPlayerHandle m_playerCore;
	Option m_opt;
	string m_basePath;
	vector<uint32_t> m_curSubIdx;
	vector<uint32_t> m_prevSubIdx;
	SceAvPlayerVideo m_lastVideoDetails;
	SceAvPlayerAudio m_lastAudioDetails;
	int32_t m_currentVideoStreamId;
	int32_t m_currentAudioStreamId;
	AVAudioThread *m_audioThread;
	PlayerState m_currentState;
	int64_t m_jumpTimeMs;
	paf::map<uint64_t, intrusive_ptr<graph::Surface>> m_drawSurf;
	uint32_t m_duration;
	SeekState m_lastSeekState;
	bool m_ssStream;
};

#endif
#ifndef _FMOD_PLAYER_H_
#define _FMOD_PLAYER_H_

#include <kernel.h>
#include <paf.h>
#include <fmod/fmod.hpp>

#include "common.h"
#include "players/player_generic.h"

using namespace paf;

class FMODPlayer : public GenericPlayer
{
public:

	class Option : public GenericPlayer::Option
	{
	public:

		Option()
		{
			playerType = PlayerType_FMOD;
		}

		string coverUrl;
	};

	class BootJob : public job::JobItem
	{
	public:

		BootJob(FMODPlayer *parent) : job::JobItem("FMODPlayer::BootJob", NULL), m_parent(parent)
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

		FMODPlayer *m_parent;
	};

	FMODPlayer(ui::Widget *targetPlane, const char *url, Option *opt);

	~FMODPlayer();

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

	uint32_t GetAudioRepresentationNum();

	static void PreInit();

	static SupportType IsSupported(const char *path);

private:

	class HybridInterface
	{
	public:

		static FMOD_RESULT Open(const char *name, unsigned int *filesize, void **handle, void *userdata);
		static FMOD_RESULT Close(void *handle, void *userdata);
		static FMOD_RESULT Read(void *handle, void *buffer, unsigned int sizebytes, unsigned int *bytesread, void *userdata);
		static FMOD_RESULT Seek(void *handle, unsigned int pos, void *userdata);
	};

	static void UpdateTask(void *pArgBlock);

	void OnBootJob();

	void SetInitState(InitState state);

	static FMOD::System *s_system;
	static FMOD::ChannelGroup *s_chg;

	FMOD::Sound *m_snd;
	FMOD::Channel *m_ch;
	Option m_opt;
};

#endif
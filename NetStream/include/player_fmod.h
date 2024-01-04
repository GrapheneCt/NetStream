#ifndef _FMOD_PLAYER_H_
#define _FMOD_PLAYER_H_

#include <kernel.h>
#include <paf.h>
#include <fmod/fmod.hpp>

#include "common.h"
#include "player_generic.h"

using namespace paf;

class FMODPlayer : public GenericPlayer
{
public:

	class HybridInterface
	{
	public:

		static FMOD_RESULT Open(const char *name, unsigned int *filesize, void **handle, void *userdata);

		static FMOD_RESULT Close(void *handle, void *userdata);

		static FMOD_RESULT Read(void *handle, void *buffer, unsigned int sizebytes, unsigned int *bytesread, void *userdata);

		static FMOD_RESULT Seek(void *handle, unsigned int pos, void *userdata);
	};

	class BootJob : public job::JobItem
	{
	public:

		BootJob(FMODPlayer *parent) : job::JobItem("FMODPlayer::BootJob", NULL), m_parent(parent)
		{

		}

		~BootJob() {}

		void Run();

		void Finish() {}

	private:

		FMODPlayer *m_parent;
	};

	FMODPlayer(ui::Widget *targetPlane, const char *url);

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

	void LimitFPS(bool enable);

	static void PreInit();

	static SupportType IsSupported(const char *path);

private:

	static void UpdateTask(void *pArgBlock);

	void SetInitState(InitState state);

	FMOD::Sound *m_snd;
	FMOD::Channel *m_ch;
};

#endif
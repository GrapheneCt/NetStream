#ifndef _GENERIC_PLAYER_H_
#define _GENERIC_PLAYER_H_

#include <kernel.h>
#include <paf.h>

using namespace paf;

class GenericPlayer
{
public:

	enum
	{
		GenericPlayerChangeState = (ui::Handler::CB_STATE + 0x20000),
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

	enum PlayerState
	{
		PlayerState_Stop = 0x00,
		PlayerState_Ready = 0x01,
		PlayerState_Play = 0x02,
		PlayerState_Pause = 0x03,
		PlayerState_Buffering = 0x04,
		PlayerState_Eof = 0x23
	};

	GenericPlayer() : m_target(NULL), m_initState(InitState_NotInit), m_limitFps(false), m_powerSaving(false)
	{

	}

	virtual ~GenericPlayer()
	{

	}

	virtual void InitAsync() = 0;

	virtual void Term() = 0;

	virtual InitState GetInitState() = 0;

	virtual uint32_t GetTotalTimeMs() = 0;

	virtual uint32_t GetCurrentTimeMs() = 0;

	virtual bool JumpToTimeMs(uint32_t time) = 0;

	virtual void SwitchPlaybackState() = 0;

	virtual bool IsPaused() = 0;

	virtual PlayerState GetState() = 0;

	virtual void SetPowerSaving(bool enable) = 0;

	virtual void LimitFPS(bool enable) = 0;

protected:

	ui::Widget *m_target;
	string m_path;
	string m_coverPath;
	InitState m_initState;
	bool m_limitFps;
	bool m_powerSaving;
};

#endif

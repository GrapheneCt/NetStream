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
		GenericPlayerChangeState = (ui::Handler::CB_STATE + 0x20000),	// Event base for all Players
	};

	enum SupportType
	{
		SupportType_NotSupported,		// File is definetely not supported
		SupportType_Supported,			// File is supported
		SupportType_MaybeSupported		// File support cannot be determined
	};

	enum InitState
	{
		InitState_NotInit,				// Initial Player state after constructing
		InitState_InProgress,			// Initialization is in progress
		InitState_InitOk,				// Initialization has been successfull
		InitState_InitFail				// Initialization failed
	};

	enum PlayerState
	{
		PlayerState_Stop = 0x00,		// Player is stopped
		PlayerState_Ready = 0x01,		// Player is ready to start playback
		PlayerState_Play = 0x02,		// Player is playing
		PlayerState_Pause = 0x03,		// Player is paused
		PlayerState_Buffering = 0x04,	// Player is buffering
		PlayerState_Eof = 0x23			// Player has reached end of the stream
	};

	enum PlayerType
	{
		PlayerType_Auto,				// Player type is not determined, use auto detection
		PlayerType_BEAV,				// SceBEAVCorePlayer
		PlayerType_AV,					// WebMAF SceAvPlayer
		PlayerType_FMOD					// FMOD
	};

	enum StreamType
	{
		StreamType_Video,				// Video stream
		StreamType_Audio				// Audio stream
	};

	/**
	 * Generic representation info, needed for UI purposes
	 */
	class GenericRepresentationInfo
	{
	public:

		GenericRepresentationInfo()
		{
			type = StreamType_Video;
			currentlySelected = false;
			enabled = true;
		}

		StreamType type;			// Base stream type for this representation
		wstring string;				// Representation name string, e.g. "1280x720"
		bool currentlySelected;		// Representation is currently selected for the baser stream
		bool enabled;				// Representation is enabled within the Player and can be selected
	};

	/**
	 * AV stream information
	 */
	class StreamInfo
	{
	public:

		/**
		 * AV stream representation information
		 */
		class RepresentationInfo
		{
		public:

			RepresentationInfo()
			{
				bitrate = 0;
				width = 0;
				height = 0;
				ch = 0;
				srate = 0;
			}

			uint32_t bitrate;	// Bitrate in kbps	
			uint32_t width;		// Width in pixels
			uint32_t height;	// Height in pixels
			uint16_t ch;		// Number of audio channels
			uint32_t srate;		// Audio sampling rate in Hz
		};

		StreamType type;							// Stream type
		vector<RepresentationInfo> representation;	// Representations available for the current stream
	};

	/**
	 * Optional structure that contains Player options passed during creation of the object
	 */
	class Option
	{
	public:

		Option()
		{
			playerType = PlayerType_Auto;
		}

		PlayerType playerType;	// This Player type. Must be set to the appropriate player type
	};

	/**
	 * Creates player instance. This method shouldn't do any long term blocking processing, move it to InitAsync().
	 */
	GenericPlayer() : m_target(NULL), m_initState(InitState_NotInit), m_powerSaving(false)
	{

	}

	/**
	 * Destructs player instance.
	 */
	virtual ~GenericPlayer()
	{

	}

	/**
	 * Initializes player asynchronously to the main framework thread.
	 */
	virtual void InitAsync() = 0;

	/**
	 * Terminates player.
	 */
	virtual void Term() = 0;

	/**
	 * Gets current state of the init process started with InitAsync().
	 *
	 * @return Current init process status.
	 */
	virtual InitState GetInitState() = 0;

	/**
	 * Gets total duration of the media in milliseconds if available, 0 otherwise (ex. if live).
	 *
	 * @return Total duration of the media in milliseconds if available, 0 otherwise (ex. if live).
	 */
	virtual uint32_t GetTotalTimeMs() = 0;

	/**
	 * Gets current timestamp of the media in milliseconds if available, 0 otherwise (ex. if live).
	 *
	 * @return Current timestamp of the media in milliseconds if available, 0 otherwise (ex. if live).
	 */
	virtual uint32_t GetCurrentTimeMs() = 0;

	/**
	 * Jumps to the timestamp, if possible.
	 *
	 * @param[in] time - Jump destination time in milliseconds.
	 *
	 * @return true if jump was executed, false otherwise (ex. if live).
	 */
	virtual bool JumpToTimeMs(uint32_t time) = 0;

	/**
	 * Switches playback state from play to pause in cycle.
	 */
	virtual void SwitchPlaybackState() = 0;

	/**
	 * Check if player is paused.
	 *
	 * @return true if paused, false otherwise.
	 */
	virtual bool IsPaused() = 0;

	/**
	 * Gets current state of the player.
	 *
	 * @return Current state of the player.
	 */
	virtual PlayerState GetState() = 0;

	/**
	 * Sets power saving mode. The call to this function indicates that player should enable all power saving features available to it.
	 * In power saving mode player must output audio, but may not output video.
	 *
	 * @param[in] enable - True if power saving mode should be enabled.
	 */
	virtual void SetPowerSaving(bool enable) = 0;

	/**
	 * Gets total amount of the video representations in the current media. If representation selection feature for video is not available,
	 * this method must return 1. If player doen't support video playback, this method must return 0
	 *
	 * @return Total amount of the video representations in the current media.
	 */
	virtual uint32_t GetVideoRepresentationNum()
	{
		return 0;
	}

	/**
	 * Gets total amount of the audio representations in the current media. If representation selection feature for audio is not available,
	 * this method must return 1. If player doen't support audio playback, this method must return 0
	 *
	 * @return Total amount of the audio representations in the current media.
	 */
	virtual uint32_t GetAudioRepresentationNum()
	{
		return 0;
	}

	/**
	 * Gets basic video representation info. May be left unimplemented if player doesn't support video representation selection or video playback.
	 *
	 * @param[in] info - Basic representation info.
	 * @param[in] idx - Index of the target representation, max is GetVideoRepresentationNum() - 1.
	 */
	virtual void GetVideoRepresentationInfo(GenericRepresentationInfo *info, uint32_t idx)
	{

	}

	/**
	 * Gets basic audio representation info. May be left unimplemented if player doesn't support audio representation selection or audio playback.
	 *
	 * @param[in] info - Basic representation info.
	 * @param[in] idx - Index of the target representation, max is GetAudioRepresentationNum() - 1.
	 */
	virtual void GetAudioRepresentationInfo(GenericRepresentationInfo *info, uint32_t idx)
	{

	}

	/**
	 * Select video representation. May be left unimplemented if player doesn't support video representation selection or video playback.
	 *
	 * @param[in] idx - Index of the target representation, max is GetVideoRepresentationNum() - 1.
	 */
	virtual void SelectVideoRepresentation(uint32_t idx)
	{

	}

	/**
	 * Select audio representation. May be left unimplemented if player doesn't support audio representation selection or audio playback.
	 *
	 * @param[in] idx - Index of the target representation, max is GetAudioRepresentationNum() - 1.
	 */
	virtual void SelectAudioRepresentation(uint32_t idx)
	{

	}

	/**
	 * Gets technical information about currently selected video and audio representations.
	 *
	 * @param[in] info - Representation info.
	 */
	virtual void GetActiveRepresentationsInfo(StreamInfo::RepresentationInfo *info)
	{

	}

protected:

	ui::Widget *m_target;			// Widget used for video rendering
	string m_path;					// Target stream path
	InitState m_initState;			// Current initialization state
	vector<StreamInfo> m_streams;	// AV stream information
	bool m_powerSaving;				// Indicates if power saving is enabled, see SetPowerSaving()
};

#endif

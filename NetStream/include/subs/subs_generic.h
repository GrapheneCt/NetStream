#ifndef _GENERIC_SUBS_H_
#define _GENERIC_SUBS_H_

#include <kernel.h>
#include <paf.h>

using namespace paf;

class GenericSubtitles
{
public:

	enum SubtitlesType
	{
		SubtitlesType_Auto,				// Subtitles type is not determined, use auto detection
		SubtitlesType_SRT,				// SubRip
	};

	enum SupportType
	{
		SupportType_NotSupported,		// File is definetely not supported
		SupportType_Supported			// File is supported
	};

	class SubtitleEntry
	{
	public:

		SubtitleEntry()
		{
			color = math::v4::_0000();
			position = math::v4::_0000();
			startTime = 0;
			endTime = 0;
			charSizeScale = 1.0f;
			anchorX = ui::Widget::ANCHOR_CENTER;
			anchorY = ui::Widget::ANCHOR_CENTER;
			alignX = ui::Widget::ALIGN_NONE;
			alignY = ui::Widget::ALIGN_NONE;
		}

		wstring string;			// Text string
		uint64_t startTime;		// Start time in ms
		uint64_t endTime;		// End time in ms
		float charSizeScale;	// Additional scaling for the base character size
		math::v4 color;
		uint32_t colorPacked;
		math::v4 position;
		uint32_t anchorX;
		uint32_t anchorY;
		uint32_t alignX;
		uint32_t alignY;
	};

	/**
	 * Optional structure that contains Subtitles options passed during creation of the object
	 */
	class Option
	{
	public:

		Option()
		{
			subtitlesType = SubtitlesType_Auto;
		}

		SubtitlesType subtitlesType;	// This Subtitles type. Must be set to the appropriate subtitles type
	};

	class SystemSettings
	{
	public:

		SystemSettings()
		{
			enable = false;
			size = 0;
			edge = 0;
			fontType = 0;
		}

		int32_t enable;
		int32_t size;
		int32_t edge;
		int32_t fontType;
	};

	static void GetSystemSettings(SystemSettings& settings);

	/**
	 * Creates subtitles instance
	 */
	GenericSubtitles();

	/**
	 * Destructs subtitles instance
	 */
	virtual ~GenericSubtitles();

	/**
	 * Initializes subtitles asynchronously to the main framework thread
	 */
	virtual void InitAsync() = 0;

	void GetActiveSubtitleIndices(vector<uint32_t>& result, uint64_t currentTimeMs);

	vector<SubtitleEntry> const& GetSubtitlieEntries();

protected:

	string m_path;						// Subtitles file path
	vector<SubtitleEntry> m_entries;	// Actual subtitle entries
};

#endif

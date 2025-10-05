#include <kernel.h>
#include <registrymgr.h>
#include <paf.h>
#include <libdbg.h>

#include "subs/subs_generic.h"
#include "subs/subs_srt.h"

using namespace paf;

void GenericSubtitles::GetSystemSettings(SystemSettings& settings)
{
	sceRegMgrGetKeyInt("/CONFIG/ACCESSIBILITY/CC", "enable_cc", &settings.enable);
	sceRegMgrGetKeyInt("/CONFIG/ACCESSIBILITY/CC", "char_size", &settings.size);
	sceRegMgrGetKeyInt("/CONFIG/ACCESSIBILITY/CC", "char_edge", &settings.edge);
	sceRegMgrGetKeyInt("/CONFIG/ACCESSIBILITY/CC", "font_type", &settings.fontType);
}

GenericSubtitles::GenericSubtitles()
{

}

GenericSubtitles::~GenericSubtitles()
{

}

void GenericSubtitles::GetActiveSubtitleIndices(vector<uint32_t>& result, uint64_t currentTimeMs)
{
	result.clear();

	// Use binary search to find first subtitle with startTime > currentTime
	auto iter = upper_bound(m_entries.begin(), m_entries.end(), currentTimeMs,
		[](uint64_t time, SubtitleEntry const& s)
	{
		return time < s.startTime;
	});

	// Scan backwards from there to collect all active subtitles
	for (auto rit = iter; rit != m_entries.begin(); )
	{
		--rit;
		if (rit->endTime > currentTimeMs)
		{
			result.push_back(static_cast<uint32_t>(rit - m_entries.begin()));
		}
		else
		{
			break; // no need to go further, since earlier ones end even earlier
		}
	}
}

vector<GenericSubtitles::SubtitleEntry> const& GenericSubtitles::GetSubtitlieEntries()
{
	return m_entries;
}
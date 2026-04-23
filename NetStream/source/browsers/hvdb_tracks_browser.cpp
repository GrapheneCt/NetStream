#include <kernel.h>
#include <paf.h>
#include <libdbg.h>
#include <psp2_compat/curl/curl.h>

#include "common.h"
#include "utils.h"
#include "hvdb.h"
#include "players/player_av.h"
#include "players/player_beav.h"
#include "players/player_fmod.h"
#include "browsers/hvdb_tracks_browser.h"

#undef SCE_DBG_LOG_COMPONENT
#define SCE_DBG_LOG_COMPONENT "[HvdbTracksBrowser]"

using namespace paf;

HvdbTracksBrowser::HvdbTracksBrowser(const char *workId, const char *workBannerUrl)
{
	m_workId = workId;
	m_bannerUrl = workBannerUrl;
	m_hvdbCtx = NULL;
	m_option = NULL;
}

HvdbTracksBrowser::~HvdbTracksBrowser()
{
	hvdbCleanup(m_hvdbCtx);

	if (m_option)
	{
		delete m_option;
	}
}

bool HvdbTracksBrowser::Probe()
{
	return true;
}

bool HvdbTracksBrowser::IsAtRoot(string *current)
{
	return current->empty();
}

bool HvdbTracksBrowser::IsAtRoot()
{
	return m_path.empty();
}

string HvdbTracksBrowser::GetPath()
{
	return m_path;
}

void HvdbTracksBrowser::GetInfoForPlayer(string const& in, string& url, GenericPlayer::Option **opt)
{
	HVDBItem *rootItem = NULL;
	uint32_t idx = 0;
	ParseRefString(in.c_str(), &rootItem, &idx);
	if (!rootItem)
	{
		return;
	}

	rootItem = GetItemAt(rootItem, idx);
	url = rootItem->dataItem->url;

	if (m_option)
	{
		delete m_option;
		m_option = NULL;
	}

	int32_t showBanner;
	sce::AppSettings *settings = menu::Settings::GetAppSetInstance();
	settings->GetInt("hvdb_show_banner", &showBanner, 0);
	if (showBanner)
	{
		GenericPlayer::Option *popt = new GenericPlayer::Option();
		popt->auxStreamUrl = m_bannerUrl;
		m_option = popt;
	}

	*opt = m_option;

	sceClibPrintf("get formatted url: %s\n", url.c_str());
}

void HvdbTracksBrowser::SetPath(const char *ref)
{
	if (ref)
	{
		if (!sce_paf_strcmp(ref, ".."))
		{
			char *tmp = static_cast<char *>(sce_paf_malloc(m_path.length() + 1));
			sce_paf_strcpy(tmp, m_path.c_str());
			char *sptr = sce_paf_strrchr(tmp, '/');
			if (!sptr)
			{
				m_path.clear();
				sce_paf_free(tmp);
				return;
			}
			*sptr = 0;
			sptr = sce_paf_strrchr(tmp, '/');
			if (!sptr)
			{
				m_path.clear();
				sce_paf_free(tmp);
				return;
			}
			sptr += 1;
			*sptr = 0;
			m_path = tmp;
			sce_paf_free(tmp);
		}
		else
		{
			m_path += m_currentDataName + "/";
		}
	}
	else
	{
		m_path.clear();
	}
}

vector<HvdbTracksBrowser::Entry *> *HvdbTracksBrowser::GoTo(const char *ref, int32_t *result)
{
	int32_t fret;
	HVDBItem *hvdbItem = NULL;
	HVDBItem *hvdbBaseItem = NULL;
	vector<HvdbTracksBrowser::Entry *> *ret = new vector<HvdbTracksBrowser::Entry *>;

	SCE_DBG_LOG_INFO("[GoTo] Attempt to open %s", m_path.c_str());

	if (ref)
	{
		HVDBItem *rootItem = NULL;
		uint32_t idx = 0;
		ParseRefString(ref, &rootItem, &idx);
		if (!rootItem)
		{
			*result = SCE_ERROR_ERRNO_ENOENT;
			goto error_return;
		}

		rootItem = GetItemAt(rootItem, idx);
		m_currentDataName = rootItem->dataItem->title;

		fret = hvdbParseDataFolder(m_hvdbCtx, rootItem->dataItem, &hvdbItem);
	}
	else
	{
		if (m_hvdbCtx)
		{
			hvdbCleanup(m_hvdbCtx);
			m_hvdbCtx = NULL;
		}

		m_currentDataName.clear();

		fret = hvdbParseData(&m_hvdbCtx, m_workId.c_str(), &hvdbItem);
	}

	SetPath(ref);

	if (fret <= 0)
	{
		if (fret == 0)
		{
			fret = SCE_ERROR_ERRNO_ENOENT;
		}
		*result = fret;
		goto error_return;
	}

	hvdbBaseItem = hvdbItem;

	for (int i = 0; i < fret; i++)
	{
		if (hvdbItem->dataItem->type == HVDB_DATA_TYPE_FOLDER)
		{
			HvdbTracksBrowser::Entry *entry = new HvdbTracksBrowser::Entry();
			entry->ref = common::ToString(reinterpret_cast<uint32_t>(hvdbBaseItem)) + "_" + common::ToString(i) + "/";
			entry->displayName = hvdbItem->dataItem->title;
			entry->type = HvdbTracksBrowser::Entry::Type_Folder;
			ret->push_back(entry);
		}
		else
		{
			GenericPlayer::SupportType supportType = utils::GetContentSupportType(hvdbItem->dataItem->url);
			if (supportType == GenericPlayer::SupportType_Supported)
			{
				HvdbTracksBrowser::Entry *entry = new HvdbTracksBrowser::Entry();
				entry->ref = common::ToString(reinterpret_cast<uint32_t>(hvdbBaseItem)) + "_" + common::ToString(i) + "/";
				entry->displayName = hvdbItem->dataItem->title;
				entry->type = HvdbTracksBrowser::Entry::Type_SupportedFile;
				ret->push_back(entry);
			}
		}

		hvdbItem = hvdbItem->next;
	}

	*result = SCE_OK;

error_return:

	return ret;
}

void HvdbTracksBrowser::ParseRefString(const char *str, HVDBItem **item, uint32_t *idx)
{
	if (!str)
	{
		return;
	}

	const char *last = NULL;
	const char *p = str;
	for (; *p; ++p)
	{
		if (((*p >= '0' && *p <= '9') || *p == '_') && (p == str || *(p - 1) == '/'))
		{
			last = p;
		}
	}

	if (last)
	{
		char *end = NULL;
		*item = reinterpret_cast<HVDBItem *>(sce_paf_strtoul(last, &end, 10));
		if (end)
		{
			*idx = sce_paf_strtoul(end + 1, NULL, 10);
		}
	}
}

HVDBItem *HvdbTracksBrowser::GetItemAt(HVDBItem *firstItem, uint32_t idx)
{
	uint32_t i = 0;
	HVDBItem *item = firstItem;
	do
	{
		if (i == idx)
		{
			return item;
		}
		item = item->next;
		i++;
	} while (item);

	return NULL;
}
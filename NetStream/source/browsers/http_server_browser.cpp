#include <kernel.h>
#include <paf.h>
#include <libdbg.h>
#include <psp2_compat/curl/curl.h>

#include "common.h"
#include "utils.h"
#include "players/player_av.h"
#include "players/player_beav.h"
#include "players/player_fmod.h"
#include "browsers/http_server_browser.h"

#undef SCE_DBG_LOG_COMPONENT
#define SCE_DBG_LOG_COMPONENT "[HTTPBrowser]"

using namespace paf;

HttpServerBrowser::HttpServerBrowser(const char *host, const char *port, const char *user, const char *password)
{
	m_url = curl_url();

	SCE_DBG_LOG_INFO("[HttpServerBrowser] %s : %s : %s : %s\n", host, port, user, password);

	if (host)
		curl_url_set(m_url, CURLUPART_URL, host, 0);
	if (port)
		curl_url_set(m_url, CURLUPART_PORT, port, 0);
	if (user)
		curl_url_set(m_url, CURLUPART_USER, user, 0);
	if (password)
		curl_url_set(m_url, CURLUPART_PASSWORD, password, 0);

	m_curl = curl_easy_init();

	char *addr;
	if (!curl_url_get(m_url, CURLUPART_URL, &addr, 0))
	{
		m_root = addr;
		curl_easy_setopt(m_curl, CURLOPT_URL, addr);
		curl_free(addr);
	}

	curl_easy_setopt(m_curl, CURLOPT_USERAGENT, USER_AGENT);
	curl_easy_setopt(m_curl, CURLOPT_ACCEPT_ENCODING, NULL);
	curl_easy_setopt(m_curl, CURLOPT_SSL_VERIFYHOST, 0L);
	curl_easy_setopt(m_curl, CURLOPT_SSL_VERIFYPEER, 0L);
	curl_easy_setopt(m_curl, CURLOPT_FOLLOWLOCATION, 1L);
	curl_easy_setopt(m_curl, CURLOPT_TCP_KEEPALIVE, 1L);
	curl_easy_setopt(m_curl, CURLOPT_CONNECTTIMEOUT, 5L);
	curl_easy_setopt(m_curl, CURLOPT_TIMEOUT, 15L);
	curl_easy_setopt(m_curl, CURLOPT_NOPROGRESS, 1L);
	curl_easy_setopt(m_curl, CURLOPT_WRITEFUNCTION, DownloadCore);
	curl_easy_setopt(m_curl, CURLOPT_WRITEDATA, this);
	curl_easy_setopt(m_curl, CURLOPT_HTTPGET, 1L);
	if (utils::GetGlobalProxy())
	{
		curl_easy_setopt(m_curl, CURLOPT_PROXY, utils::GetGlobalProxy());
	}
}

HttpServerBrowser::~HttpServerBrowser()
{
	curl_url_cleanup(m_url);
	curl_easy_cleanup(m_curl);
}

bool HttpServerBrowser::Probe()
{
	curl_easy_setopt(m_curl, CURLOPT_NOBODY, 1L);

	CURLcode ret = curl_easy_perform(m_curl);
	curl_easy_setopt(m_curl, CURLOPT_NOBODY, 0L);
	curl_easy_setopt(m_curl, CURLOPT_HTTPGET, 1L);

	if (ret != CURLE_OK)
	{
		return false;
	}

	return true;
}

bool HttpServerBrowser::IsAtRoot(string *current)
{
	return (m_root == *current);
}

bool HttpServerBrowser::IsAtRoot()
{
	return (m_root == GetPath());
}

string HttpServerBrowser::GetPath()
{
	string ret;
	char *path;
	curl_url_get(m_url, CURLUPART_URL, &path, 0);
	if (path)
	{
		char *decoded = curl_easy_unescape(m_curl, path, 0, NULL);
		ret = decoded;
		curl_free(decoded);
		curl_free(path);
	}

	return ret;
}

string HttpServerBrowser::GetBEAVUrl(string const& in)
{
	string ret;
	CURLU *turl = curl_url_dup(m_url);
	curl_url_set(turl, CURLUPART_URL, in.c_str(), 0);
	char *result;
	curl_url_get(turl, CURLUPART_URL, &result, 0);
	ret = result;
	curl_free(result);
	curl_url_cleanup(turl);

	return ret;
}

void HttpServerBrowser::SetPath(const char *ref)
{
	if (ref)
	{
		char *encoded = curl_easy_escape(m_curl, ref, 0);
		char *current;
		curl_url_get(m_url, CURLUPART_URL, &current, 0);
		string full = current;
		full += encoded;
		full += "/";
		curl_url_set(m_url, CURLUPART_URL, full.c_str(), 0);
		curl_free(encoded);
		curl_free(current);
	}
}

vector<HttpServerBrowser::Entry *> *HttpServerBrowser::GoTo(const char *ref, int32_t *result)
{
	vector<HttpServerBrowser::Entry *> *ret = new vector<HttpServerBrowser::Entry *>;
	CURLcode cret;

	SetPath(ref);

	char *addr;
	curl_url_get(m_url, CURLUPART_URL, &addr, 0);
	SCE_DBG_LOG_INFO("[GoTo] Attempt to open %s\n", addr);
	curl_easy_setopt(m_curl, CURLOPT_URL, addr);
	curl_free(addr);

	m_buffer = NULL;
	m_posInBuf = 0;

	cret = curl_easy_perform(m_curl);
	if (cret != CURLE_OK)
	{
		*result = cret;
		return ret;
	}

	if (m_buffer && m_posInBuf > 2)
	{
		m_buffer[m_posInBuf - 1] = 0;

		char *href = NULL;
		char *refEnd = m_buffer;
		while (1)
		{
			href = sce_paf_strstr(refEnd + 1, "<a href=\"");
			if (!href)
				break;
			href += 9;

			refEnd = sce_paf_strstr(href, "\">");
			*refEnd = 0;

			GenericPlayer::SupportType supportType = AVPlayer::IsSupported(href);
			if (supportType == GenericPlayer::SupportType_NotSupported)
			{
				supportType = BEAVPlayer::IsSupported(href);
				if (supportType == GenericPlayer::SupportType_NotSupported)
				{
					supportType = FMODPlayer::IsSupported(href);
				}
			}

			if (supportType != GenericPlayer::SupportType_NotSupported)
			{
				char *decoded = curl_easy_unescape(m_curl, href, 0, NULL);
				HttpServerBrowser::Entry *entry = new HttpServerBrowser::Entry();
				entry->ref = decoded;
				entry->type = HttpServerBrowser::Entry::Type_UnsupportedFile;
				curl_free(decoded);

				if (supportType == GenericPlayer::SupportType_MaybeSupported)
				{
					entry->type = HttpServerBrowser::Entry::Type_Folder;
				}
				else if (supportType == GenericPlayer::SupportType_Supported)
				{
					entry->type = HttpServerBrowser::Entry::Type_SupportedFile;
				}

				ret->push_back(entry);
			}
		}

		*result = SCE_OK;
	}
	else
	{
		*result = SCE_ERROR_ERRNO_ENOENT;
	}

	if (m_buffer)
		sce_paf_free(m_buffer);

	return ret;
}

size_t HttpServerBrowser::DownloadCore(char *buffer, size_t size, size_t nitems, void *userdata)
{
	HttpServerBrowser *obj = static_cast<HttpServerBrowser *>(userdata);
	size_t toCopy = size * nitems;

	if (toCopy != 0)
	{
		obj->m_buffer = static_cast<char *>(sce_paf_realloc(obj->m_buffer, obj->m_posInBuf + toCopy));
		sce_paf_memcpy(obj->m_buffer + obj->m_posInBuf, buffer, toCopy);
		obj->m_posInBuf += toCopy;
		return toCopy;
	}

	return 0;
}
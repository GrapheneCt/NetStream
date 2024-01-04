#include <kernel.h>
#include <paf.h>
#include <libdbg.h>
#include <psp2_compat/curl/curl.h>

#include "common.h"
#include "ftpparse.h"
#include "player_beav.h"
#include "ftp_server_browser.h"

using namespace paf;

FtpServerBrowser::FtpServerBrowser(const char *host, const char *port, const char *user, const char *password)
{
	m_useNlst = 0;
	m_url = curl_url();

	SCE_DBG_LOG_INFO("[FTP] %s : %s : %s : %s\n", host, port, user, password);

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

	curl_easy_setopt(m_curl, CURLOPT_SSL_VERIFYHOST, 0L);
	curl_easy_setopt(m_curl, CURLOPT_SSL_VERIFYPEER, 0L);
	curl_easy_setopt(m_curl, CURLOPT_CONNECTTIMEOUT, 5L);
	curl_easy_setopt(m_curl, CURLOPT_TIMEOUT, 5L);
	curl_easy_setopt(m_curl, CURLOPT_NOPROGRESS, 1L);
	curl_easy_setopt(m_curl, CURLOPT_WRITEFUNCTION, DownloadCore);
	curl_easy_setopt(m_curl, CURLOPT_WRITEDATA, this);
	menu::Settings::GetAppSetInstance()->GetInt("ftp_nlst", &m_useNlst, 0);
	if (m_useNlst)
	{
		curl_easy_setopt(m_curl, CURLOPT_DIRLISTONLY, 1L);
	}
}

FtpServerBrowser::~FtpServerBrowser()
{
	curl_url_cleanup(m_url);
	curl_easy_cleanup(m_curl);
}

bool FtpServerBrowser::Probe()
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

bool FtpServerBrowser::IsAtRoot(string *current)
{
	return (m_root == *current);
}

bool FtpServerBrowser::IsAtRoot()
{
	return (m_root == GetPath());
}

string FtpServerBrowser::GetPath()
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

string FtpServerBrowser::GetBEAVUrl(string const& in)
{
	string ret = "http://";
	CURLU *turl = curl_url_dup(m_url);
	curl_url_set(turl, CURLUPART_URL, in.c_str(), 0);
	char *result;
	curl_url_get(turl, CURLUPART_URL, &result, 0);
	ret += result;
	curl_free(result);
	curl_url_cleanup(turl);

	return ret;
}

void FtpServerBrowser::SetPath(const char *ref)
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

vector<FtpServerBrowser::Entry *> *FtpServerBrowser::GoTo(const char *ref, int32_t *result)
{
	vector<FtpServerBrowser::Entry *> *ret = new vector<FtpServerBrowser::Entry *>;
	CURLcode cret;

	SetPath(ref);

	char *addr;
	curl_url_get(m_url, CURLUPART_URL, &addr, 0);
	SCE_DBG_LOG_INFO("[FTP] Attempt to open %s\n", addr);
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

		char *tok = sce_paf_strtok(m_buffer, "\n");
		while (tok != NULL) {
			if (!m_useNlst)
			{
				struct ftpparse ftpe;
				int fret = ftpparse(&ftpe, tok, sce_paf_strlen(tok));
				if (fret == 1)
				{
					tok = ftpe.name;
				}
				else
				{
					tok = "";
				}
			}
			BEAVPlayer::SupportType beavType = BEAVPlayer::IsSupported(tok);
			if (beavType != BEAVPlayer::SupportType_NotSupported)
			{
				char *decoded = curl_easy_unescape(m_curl, tok, 0, NULL);
				FtpServerBrowser::Entry *entry = new FtpServerBrowser::Entry();
				entry->ref = decoded;
				entry->type = FtpServerBrowser::Entry::Type_UnsupportedFile;
				curl_free(decoded);

				if (beavType == BEAVPlayer::SupportType_MaybeSupported)
				{
					entry->type = FtpServerBrowser::Entry::Type_Folder;
				}
				else if (beavType == BEAVPlayer::SupportType_Supported)
				{
					entry->type = FtpServerBrowser::Entry::Type_SupportedFile;
				}

				ret->push_back(entry);
			}
			tok = sce_paf_strtok(NULL, "\n");
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

size_t FtpServerBrowser::DownloadCore(char *buffer, size_t size, size_t nitems, void *userdata)
{
	FtpServerBrowser *obj = static_cast<FtpServerBrowser *>(userdata);
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
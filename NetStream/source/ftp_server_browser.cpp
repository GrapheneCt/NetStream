#include <kernel.h>
#include <paf.h>
#include <libdbg.h>
#include <psp2_compat/curl/curl.h>

#include "common.h"
#include "ftpparse.h"
#include "beav_player.h"
#include "ftp_server_browser.h"

using namespace paf;

FtpServerBrowser::FtpServerBrowser(const char *host, const char *port, const char *user, const char *password)
{
	useNlst = 0;
	url = curl_url();

	SCE_DBG_LOG_INFO("[FTP] %s : %s : %s : %s\n", host, port, user, password);

	if (host)
		curl_url_set(url, CURLUPART_URL, host, 0);
	if (port)
		curl_url_set(url, CURLUPART_PORT, port, 0);
	if (user)
		curl_url_set(url, CURLUPART_USER, user, 0);
	if (password)
		curl_url_set(url, CURLUPART_PASSWORD, password, 0);

	curl = curl_easy_init();

	char *addr;
	if (!curl_url_get(url, CURLUPART_URL, &addr, 0))
	{
		root = addr;
		curl_easy_setopt(curl, CURLOPT_URL, addr);
		curl_free(addr);
	}

	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
	curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5L);
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);
	curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, DownloadCore);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, this);
	menu::Settings::GetAppSetInstance()->GetInt("ftp_nlst", &useNlst, 0);
	if (useNlst)
	{
		curl_easy_setopt(curl, CURLOPT_DIRLISTONLY, 1L);
	}
}

FtpServerBrowser::~FtpServerBrowser()
{
	curl_url_cleanup(url);
	curl_easy_cleanup(curl);
}

SceBool FtpServerBrowser::Probe()
{
	curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);

	CURLcode ret = curl_easy_perform(curl);
	curl_easy_setopt(curl, CURLOPT_NOBODY, 0L);
	curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);

	if (ret != CURLE_OK)
	{
		return SCE_FALSE;
	}

	return SCE_TRUE;
}

SceBool FtpServerBrowser::IsAtRoot(string *current)
{
	return (root == *current);
}

SceBool FtpServerBrowser::IsAtRoot()
{
	return (root == GetPath());
}

string FtpServerBrowser::GetPath()
{
	string ret;
	char *path;
	curl_url_get(url, CURLUPART_URL, &path, 0);
	if (path)
	{
		char *decoded = curl_easy_unescape(curl, path, 0, SCE_NULL);
		ret = decoded;
		curl_free(decoded);
		curl_free(path);
	}

	return ret;
}

string FtpServerBrowser::GetBEAVUrl(string *in)
{
	string ret = "http://";
	CURLU *turl = curl_url_dup(url);
	curl_url_set(turl, CURLUPART_URL, in->c_str(), 0);
	char *result;
	curl_url_get(turl, CURLUPART_URL, &result, 0);
	ret += result;
	curl_free(result);
	curl_url_cleanup(turl);

	return ret;
}

SceVoid FtpServerBrowser::SetPath(const char *ref)
{
	if (ref)
	{
		char *encoded = curl_easy_escape(curl, ref, 0);
		char *current;
		curl_url_get(url, CURLUPART_URL, &current, 0);
		string full = current;
		full += encoded;
		full += "/";
		curl_url_set(url, CURLUPART_URL, full.c_str(), 0);
		curl_free(encoded);
		curl_free(current);
	}
}

vector<FtpServerBrowser::Entry *> *FtpServerBrowser::GoTo(const char *ref, SceInt32 *result)
{
	vector<FtpServerBrowser::Entry *> *ret = new vector<FtpServerBrowser::Entry *>;
	CURLcode cret;

	SetPath(ref);

	char *addr;
	curl_url_get(url, CURLUPART_URL, &addr, 0);
	SCE_DBG_LOG_INFO("[FTP] Attempt to open %s\n", addr);
	curl_easy_setopt(curl, CURLOPT_URL, addr);
	curl_free(addr);

	buffer = SCE_NULL;
	posInBuf = 0;

	cret = curl_easy_perform(curl);
	if (cret != CURLE_OK)
	{
		*result = cret;
		return ret;
	}

	if (buffer && posInBuf > 2)
	{
		buffer[posInBuf - 1] = 0;

		char *tok = sce_paf_strtok(buffer, "\n");
		while (tok != SCE_NULL) {
			if (!useNlst)
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
				char *decoded = curl_easy_unescape(curl, tok, 0, SCE_NULL);
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
			tok = sce_paf_strtok(SCE_NULL, "\n");
		}

		*result = SCE_OK;
	}
	else
	{
		*result = SCE_ERROR_ERRNO_ENOENT;
	}

	if (buffer)
		sce_paf_free(buffer);

	return ret;
}

SceSize FtpServerBrowser::DownloadCore(char *buffer, SceSize size, SceSize nitems, ScePVoid userdata)
{
	FtpServerBrowser *obj = (FtpServerBrowser *)userdata;
	SceSize toCopy = size * nitems;

	if (toCopy != 0)
	{
		obj->buffer = (char *)sce_paf_realloc(obj->buffer, obj->posInBuf + toCopy);
		sce_paf_memcpy(obj->buffer + obj->posInBuf, buffer, toCopy);
		obj->posInBuf += toCopy;
		return toCopy;
	}

	return 0;
}
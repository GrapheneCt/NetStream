#include <kernel.h>
#include <paf.h>
#include <libdbg.h>
#include <psp2_compat/curl/curl.h>

#include "common.h"
#include "beav_player.h"
#include "http_server_browser.h"

using namespace paf;

HttpServerBrowser::HttpServerBrowser(const char *host, const char *port, const char *user, const char *password)
{
	url = curl_url();

	SCE_DBG_LOG_INFO("[HTTP] %s : %s : %s : %s\n", host, port, user, password);

	if (host)
		curl_url_set(url, CURLUPART_URL, host, 0);
	if (port)
		curl_url_set(url, CURLUPART_PORT, port, 0);
	if (user)
		curl_url_set(url, CURLUPART_PORT, user, 0);
	if (password)
		curl_url_set(url, CURLUPART_PORT, password, 0);

	curl = curl_easy_init();

	char *addr;
	if (!curl_url_get(url, CURLUPART_URL, &addr, 0))
	{
		root = addr;
		curl_easy_setopt(curl, CURLOPT_URL, addr);
		curl_free(addr);
	}

	curl_easy_setopt(curl, CURLOPT_USERAGENT, USER_AGENT);
	curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, SCE_NULL);
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
	curl_easy_setopt(curl, CURLOPT_TCP_KEEPALIVE, 1L);
	curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5L);
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);
	curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, DownloadCore);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, this);
	curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
}

HttpServerBrowser::~HttpServerBrowser()
{
	curl_url_cleanup(url);
	curl_easy_cleanup(curl);
}

SceBool HttpServerBrowser::Probe()
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

SceBool HttpServerBrowser::IsAtRoot(string *current)
{
	return (root == *current);
}

SceBool HttpServerBrowser::IsAtRoot()
{
	return (root == GetPath());
}

string HttpServerBrowser::GetPath()
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

string HttpServerBrowser::GetBEAVUrl(string *in)
{
	string ret;
	CURLU *turl = curl_url_dup(url);
	curl_url_set(turl, CURLUPART_URL, in->c_str(), 0);
	char *result;
	curl_url_get(turl, CURLUPART_URL, &result, 0);
	ret = result;
	curl_free(result);
	curl_url_cleanup(turl);

	return ret;
}

SceVoid HttpServerBrowser::SetPath(const char *ref)
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

vector<HttpServerBrowser::Entry *> *HttpServerBrowser::GoTo(const char *ref, SceInt32 *result)
{
	vector<HttpServerBrowser::Entry *> *ret = new vector<HttpServerBrowser::Entry *>;
	CURLcode cret;

	SetPath(ref);

	char *addr;
	curl_url_get(url, CURLUPART_URL, &addr, 0);
	SCE_DBG_LOG_INFO("[HTTP] Attempt to open %s\n", addr);
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

		char *ref = SCE_NULL;
		char *refEnd = buffer;
		while (1)
		{
			ref = sce_paf_strstr(refEnd + 1, "<a href=\"");
			if (!ref)
				break;
			ref += 9;

			refEnd = sce_paf_strstr(ref, "\">");
			*refEnd = 0;

			BEAVPlayer::SupportType beavType = BEAVPlayer::IsSupported(ref);

			if (beavType != BEAVPlayer::SupportType_NotSupported)
			{
				char *decoded = curl_easy_unescape(curl, ref, 0, SCE_NULL);
				HttpServerBrowser::Entry *entry = new HttpServerBrowser::Entry();
				entry->ref = decoded;
				entry->type = HttpServerBrowser::Entry::Type_UnsupportedFile;
				curl_free(decoded);

				if (beavType == BEAVPlayer::SupportType_MaybeSupported)
				{
					entry->type = HttpServerBrowser::Entry::Type_Folder;
				}
				else if (beavType == BEAVPlayer::SupportType_Supported)
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

	if (buffer)
		sce_paf_free(buffer);

	return ret;
}

SceSize HttpServerBrowser::DownloadCore(char *buffer, SceSize size, SceSize nitems, ScePVoid userdata)
{
	HttpServerBrowser *obj = (HttpServerBrowser *)userdata;
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
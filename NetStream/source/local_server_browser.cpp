#include <kernel.h>
#include <paf.h>
#include <libdbg.h>
#include <psp2_compat/curl/curl.h>

#include "common.h"
#include "beav_player.h"
#include "local_server_browser.h"

using namespace paf;

const char *k_supportedRoots[] = {
		"ux0:",
		"uma0:"
};

LocalServerBrowser::LocalServerBrowser()
{
	
}

LocalServerBrowser::~LocalServerBrowser()
{

}

SceBool LocalServerBrowser::Probe()
{
	return SCE_TRUE;
}

SceBool LocalServerBrowser::IsAtRoot(string *current)
{
	return current->empty();
}

SceBool LocalServerBrowser::IsAtRoot()
{
	return path.empty();
}

string LocalServerBrowser::GetPath()
{
	return path;
}

string LocalServerBrowser::GetBEAVUrl(string *in)
{
	string fullPath = path + *in;
	string ret;

	if (!sce_paf_strncmp(in->c_str(), "http", 4) || !sce_paf_strncmp(in->c_str(), "ftp", 3) || !sce_paf_strncmp(in->c_str(), "smb", 3))
	{
		ret = *in;
	}
	else
	{
		ret = "mp4://";
		ret += fullPath;
	}

	return ret;
}

SceVoid LocalServerBrowser::SetPath(const char *ref)
{
	if (ref)
	{
		if (!sce_paf_strcmp(ref, ".."))
		{
			char *tmp = (char *)sce_paf_malloc(path.length() + 1);
			sce_paf_strcpy(tmp, path.c_str());
			char *sptr = sce_paf_strrchr(tmp, '/');
			if (!sptr)
			{
				path.clear();
				sce_paf_free(tmp);
				return;
			}
			*sptr = 0;
			sptr = sce_paf_strrchr(tmp, '/');
			if (!sptr)
			{
				sptr = sce_paf_strchr(tmp, ':');
			}
			sptr += 1;
			*sptr = 0;
			path = tmp;
			sce_paf_free(tmp);
		}
		else
		{
			path += ref;
		}
	}
	else
	{
		path.clear();
	}
}

bool LocalServerBrowser::DefaultFsSort(const LocalServerBrowser::Entry *a, const LocalServerBrowser::Entry *b)
{
	sce::AppSettings *settings = menu::Settings::GetAppSetInstance();

	SceInt32 nameSort = 0;
	SceInt32 typeSort = 0;
	SceBool isFolderA = a->type == LocalServerBrowser::Entry::Type_Folder;
	SceBool isFolderB = b->type == LocalServerBrowser::Entry::Type_Folder;
	settings->GetInt("local_sort_name", (SceInt32 *)&nameSort, 0);
	settings->GetInt("local_sort_type", (SceInt32 *)&typeSort, 0);

	switch (typeSort)
	{
	case 0:
		if (isFolderA && !isFolderB)
		{
			return true;
		}
		else if (!isFolderA && isFolderB)
		{
			return false;
		}
		break;
	case 1:
		if (isFolderA && !isFolderB)
		{
			return false;
		}
		else if (!isFolderA && isFolderB)
		{
			return true;
		}
		break;
	}

	SceInt32 ret = sce_paf_strcasecmp(a->ref.c_str(), b->ref.c_str());

	switch (nameSort)
	{
	case 0:
		if (ret <= 0)
		{
			return true;
		}
		else
		{
			return false;
		}
	case 1:
		if (ret <= 0)
		{
			return false;
		}
		else
		{
			return true;
		}
	}

	return false;
}

vector<LocalServerBrowser::Entry *> *LocalServerBrowser::GoTo(const char *ref, SceInt32 *result)
{
	char prefix[256];
	vector<LocalServerBrowser::Entry *> *ret = new vector<LocalServerBrowser::Entry *>;
	menu::Settings::GetAppSetInstance()->GetString("local_playlist_prefix", prefix, sizeof(prefix), "");

	SetPath(ref);

	SCE_DBG_LOG_INFO("[LOCAL] Attempt to open %s\n", path.c_str());

	if (ref)
	{
		if (sce_paf_strstr(ref, ".m3u8"))
		{
			common::SharedPtr<LocalFile> fres;
			SceInt32 res = -1;
			SceSize fsz = 0;
			char *fbuf = SCE_NULL;
			char *begin = SCE_NULL;
			char *end = SCE_NULL;

			fres = LocalFile::Open(GetPath().c_str(), SCE_O_RDONLY, 0, &res);
			if (res < 0)
			{
				*result = res;
				goto error_return;
			}

			fsz = fres->GetFileSize();
			fbuf = (char *)sce_paf_malloc(fsz + 1);
			if (!fbuf)
			{
				*result = SCE_ERROR_ERRNO_ENOMEM;
				fres.reset();
				goto error_return;
			}

			fbuf[fsz] = 0;
			fres->Read(fbuf, fsz);
			fres->Close();
			fres.reset();

			begin = sce_paf_strtok(fbuf, "#");
			while (begin)
			{
				begin = sce_paf_strchr(begin, '\n');
				if (begin && sce_paf_strlen(begin) > 3)
				{
					begin++;
					if (*begin == '#')
					{
						begin = sce_paf_strtok(SCE_NULL, "#");
						continue;
					}

					end = sce_paf_strchr(begin, '\r');
					if (end)
					{
						*end = 0;
					}
					end = sce_paf_strchr(begin, '\n');
					if (end)
					{
						*end = 0;
					}

					if (!sce_paf_strncmp(begin, "http", 4))
					{
						LocalServerBrowser::Entry *entry = new LocalServerBrowser::Entry();
						entry->ref = begin;
						entry->type = LocalServerBrowser::Entry::Type_SupportedFile;
						ret->push_back(entry);
					}
					else
					{
						LocalServerBrowser::Entry *entry = new LocalServerBrowser::Entry();
						entry->ref = prefix;
						entry->ref += begin;
						entry->type = LocalServerBrowser::Entry::Type_SupportedFile;
						ret->push_back(entry);
					}
				}

				begin = sce_paf_strtok(SCE_NULL, "#");
			}

			sce_paf_free(fbuf);
		}
		else
		{
			Dir dir;
			DirEnt dentry;

			if (dir.Open(path.c_str()) >= 0)
			{
				while (dir.Read(&dentry) >= 0)
				{
					BEAVPlayer::SupportType beavType = BEAVPlayer::IsSupported(dentry.name.c_str());

					if (beavType != BEAVPlayer::SupportType_NotSupported)
					{
						LocalServerBrowser::Entry *entry = new LocalServerBrowser::Entry();
						entry->ref = dentry.name.c_str();
						entry->type = LocalServerBrowser::Entry::Type_UnsupportedFile;

						if (beavType == BEAVPlayer::SupportType_MaybeSupported)
						{
							entry->ref += "/";
							entry->type = LocalServerBrowser::Entry::Type_Folder;
						}
						else if (beavType == BEAVPlayer::SupportType_Supported)
						{
							if (sce_paf_strstr(entry->ref.c_str(), ".m3u8"))
							{
								entry->ref += "/";
								entry->type = LocalServerBrowser::Entry::Type_PlaylistFile;
							}
							else
							{
								entry->type = LocalServerBrowser::Entry::Type_SupportedFile;
							}
						}

						ret->push_back(entry);
					}
				}

				dir.Close();

				*result = SCE_OK;
			}
			else
			{
				*result = SCE_ERROR_ERRNO_ENOENT;
			}
		}
	}
	else
	{
		for (int i = 0; i < sizeof(k_supportedRoots) / sizeof(char *); i++)
		{
			if (LocalFile::Exists(k_supportedRoots[i]))
			{
				LocalServerBrowser::Entry *entry = new LocalServerBrowser::Entry();
				entry->ref = k_supportedRoots[i];
				entry->type = LocalServerBrowser::Entry::Type_Folder;
				ret->push_back(entry);
			}
		}

		*result = SCE_OK;
	}

	paf::sort(ret->begin(), ret->end(), DefaultFsSort);

error_return:

	return ret;
}
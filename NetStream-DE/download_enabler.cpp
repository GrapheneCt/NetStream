/*
  VitaTweaks: Download Enabler
  Copyright (C) 2018, TheFloW

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <kernel.h>
#include <taihen.h>
#include <paf.h>

static tai_hook_ref_t GetFileTypeRef;
static tai_hook_ref_t ExportFileRef;
static SceUID hooks[2];

static int ExportFilePatched(uint32_t *data)
{
	int res = TAI_NEXT(ExportFilePatched, ExportFileRef, data);

	if (res == 0x80101A09)
	{
		paf::string download_path;
		paf::string bgdl_path;
		char file_name[256];
		char short_name[256];
		uint16_t url_length = 0;
		uint32_t count = 0;

		uint32_t num = *(uint32_t *)data[0];

		bgdl_path = paf::common::FormatString("ux0:bgdl/t/%08x/d0.pdb", num);

		int ret = 0;
		paf::common::SharedPtr<paf::LocalFile> fd = paf::LocalFile::Open(bgdl_path.c_str(), SCE_O_RDONLY, 0, &ret);
		if (ret != SCE_PAF_OK)
		{
			return ret;
		}

		fd->Seek(0xD6, SCE_SEEK_SET);
		fd->Read(&url_length, sizeof(uint16_t));
		fd->Seek(0xF7 + url_length, SCE_SEEK_SET);
		fd->Read(file_name, sizeof(file_name));
		fd->Close();

		bgdl_path.clear();
		bgdl_path = paf::common::FormatString("ux0:bgdl/t/%08x/%s", num, file_name);

		char *ext = sce_paf_strrchr(file_name, '.');
		if (ext)
		{
			int len = ext - file_name;
			if (len > sizeof(short_name) - 1)
				len = sizeof(short_name) - 1;
			sce_paf_strncpy(short_name, file_name, len);
			short_name[len] = '\0';
		}
		else
		{
			sce_paf_strncpy(short_name, file_name, sizeof(short_name));
			ext = "";
		}

		while (1)
		{
			if (count == 0)
			{
				download_path = paf::common::FormatString("ux0:download/%s", file_name);
			}
			else
			{
				download_path = paf::common::FormatString("ux0:download/%s (%d)%s", short_name, count, ext);
			}

			if (!paf::LocalFile::Exists(download_path.c_str()))
			{
				break;
			}

			count++;
		}

		res = sceIoMkdir("ux0:download", 0006);
		if (res < 0 && res != 0x80010011)
		{
			return res;
		}

		return paf::LocalFile::RenameFile(bgdl_path.c_str(), download_path.c_str());
	}

	return res;
}

static int32_t GetFileTypePatched(int unk, int *type, char **filename, char **mime_type)
{
	int res = TAI_NEXT(GetFileTypePatched, GetFileTypeRef, unk, type, filename, mime_type);

	if (!sceClibStrcmp(*mime_type, "video/mp4"))
	{
		*type = 3;
		return 0;
	}
	if (res == 0x80103A21)
	{
		*type = 1;
		return 0;
	}

	return res;
}

extern "C" {

	#include <moduleinfo.h>

	SCE_MODULE_INFO(NetStream_DE, 2, 1, 1)

	int module_start(size_t args, const void * argp)
	{
		tai_module_info_t info;
		info.size = sizeof(info);
		if (taiGetModuleInfo("SceShell", &info) >= 0)
		{
			switch (info.module_nid)
			{
			case 0x0552F692: // 3.60 retail
			{
				hooks[0] = taiHookFunctionOffset(&ExportFileRef, info.modid, 0, 0x1163F6, 1, ExportFilePatched);
				hooks[1] = taiHookFunctionOffset(&GetFileTypeRef, info.modid, 0, 0x11B5E4, 1, GetFileTypePatched);
				break;
			}

			case 0x6CB01295: // 3.60 PDEL
			{
				hooks[0] = taiHookFunctionOffset(&ExportFileRef, info.modid, 0, 0x111D5A, 1, ExportFilePatched);
				hooks[1] = taiHookFunctionOffset(&GetFileTypeRef, info.modid, 0, 0x116F48, 1, GetFileTypePatched);
				break;
			}

			case 0xEAB89D5C: // 3.60 PTEL
			{
				hooks[0] = taiHookFunctionOffset(&ExportFileRef, info.modid, 0, 0x112756, 1, ExportFilePatched);
				hooks[1] = taiHookFunctionOffset(&GetFileTypeRef, info.modid, 0, 0x117944, 1, GetFileTypePatched);
				break;
			}

			case 0x5549BF1F: // 3.65 retail
			case 0x34B4D82E: // 3.67 retail
			case 0x12DAC0F3: // 3.68 retail
			{
				hooks[0] = taiHookFunctionOffset(&ExportFileRef, info.modid, 0, 0x11644E, 1, ExportFilePatched);
				hooks[1] = taiHookFunctionOffset(&GetFileTypeRef, info.modid, 0, 0x11B63C, 1, GetFileTypePatched);
				break;
			}

			case 0x0703C828: // 3.69 retail
			case 0x2053B5A5: // 3.70 retail
			case 0xF476E785: // 3.71 retail
			case 0x939FFBE9: // 3.72 retail
			case 0x734D476A: // 3.73 retail
			case 0x51CB6207: // 3.74 retail
			{
				hooks[0] = taiHookFunctionOffset(&ExportFileRef, info.modid, 0, 0x11644E, 1, ExportFilePatched);
				hooks[1] = taiHookFunctionOffset(&GetFileTypeRef, info.modid, 0, 0x11B63C, 1, GetFileTypePatched);
				break;
			}

			case 0xE6A02F2B: // 3.65 PDEL
			{
				hooks[0] = taiHookFunctionOffset(&ExportFileRef, info.modid, 0, 0x111DB2, 1, ExportFilePatched);
				hooks[1] = taiHookFunctionOffset(&GetFileTypeRef, info.modid, 0, 0x116FA0, 1, GetFileTypePatched);
				break;
			}

			case 0x587F9CED: // 3.65 PTEL
			{
				hooks[0] = taiHookFunctionOffset(&ExportFileRef, info.modid, 0, 0x1127AE, 1, ExportFilePatched);
				hooks[1] = taiHookFunctionOffset(&GetFileTypeRef, info.modid, 0, 0x11799C, 1, GetFileTypePatched);
				break;
			}
			}
		}

		return SCE_KERNEL_START_SUCCESS;
	}

	int module_stop(size_t args, const void * argp)
	{
		return SCE_KERNEL_STOP_SUCCESS;
	}
}
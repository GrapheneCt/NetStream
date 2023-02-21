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
#include <moduleinfo.h>

SCE_MODULE_INFO(NetStream_DE, 2, 1, 1)

static tai_hook_ref_t GetFileTypeRef;
static SceUID hooks[1];

static SceInt32 GetFileTypePatched(int unk, int *type, char **filename, char **mime_type)
{
	int res = TAI_NEXT(GetFileTypePatched, GetFileTypeRef, unk, type, filename, mime_type);

	if (!sceClibStrcmp(*mime_type, "video/mp4"))
	{
		*type = 3;
		return 0;
	}

	return res;
}

int module_start(SceSize args, const void * argp)
{
	tai_module_info_t info;
	info.size = sizeof(info);
	if (taiGetModuleInfo("SceShell", &info) >= 0) {
		switch (info.module_nid) {
		case 0x0552F692: // 3.60 retail
		{
			hooks[0] = taiHookFunctionOffset(&GetFileTypeRef, info.modid, 0, 0x11B5E4, 1, GetFileTypePatched);
			break;
		}

		case 0x6CB01295: // 3.60 PDEL
		{
			hooks[0] = taiHookFunctionOffset(&GetFileTypeRef, info.modid, 0, 0x116F48, 1, GetFileTypePatched);
			break;
		}

		case 0xEAB89D5C: // 3.60 PTEL
		{
			hooks[0] = taiHookFunctionOffset(&GetFileTypeRef, info.modid, 0, 0x117944, 1, GetFileTypePatched);
			break;
		}

		case 0x5549BF1F: // 3.65 retail
		case 0x34B4D82E: // 3.67 retail
		case 0x12DAC0F3: // 3.68 retail
		{
			hooks[0] = taiHookFunctionOffset(&GetFileTypeRef, info.modid, 0, 0x11B63C, 1, GetFileTypePatched);
			break;
		}

		case 0x0703C828: // 3.69 retail
		case 0x2053B5A5: // 3.70 retail
		case 0xF476E785: // 3.71 retail
		case 0x939FFBE9: // 3.72 retail
		case 0x734D476A: // 3.73 retail
		case 0x51CB6207: // 3.74 retail
		{
			hooks[0] = taiHookFunctionOffset(&GetFileTypeRef, info.modid, 0, 0x11B63C, 1, GetFileTypePatched);
			break;
		}

		case 0xE6A02F2B: // 3.65 PDEL
		{
			hooks[0] = taiHookFunctionOffset(&GetFileTypeRef, info.modid, 0, 0x116FA0, 1, GetFileTypePatched);
			break;
		}

		case 0x587F9CED: // 3.65 PTEL
		{
			hooks[0] = taiHookFunctionOffset(&GetFileTypeRef, info.modid, 0, 0x11799C, 1, GetFileTypePatched);
			break;
		}
		}
	}

	return SCE_KERNEL_START_SUCCESS;
}

int module_stop(SceSize args, const void * argp)
{
	if (hooks[0] >= 0)
		taiHookRelease(hooks[0], GetFileTypeRef);
	return SCE_KERNEL_STOP_SUCCESS;
}
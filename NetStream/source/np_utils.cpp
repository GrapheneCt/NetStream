#include <kernel.h>
#include <paf.h>
#include <stdlib.h>
#include <np.h>

#include "np_utils.h"

static const SceNpCommunicationId s_CommId = {
	{'N', 'P', 'W', 'R', '0', '0', '6', '4', '9'},
	'\0',
	0,
	0
};

static const SceNpCommunicationPassphrase s_CommPassphrase =
{
	{
	0x4F, 0x09, 0x1C, 0xF6, 0x6F, 0x7A, 0xEB, 0xE2, 0xC5, 0x1D, 0x68, 0x9B,
	0x61, 0x7E, 0x3B, 0x49, 0xFC, 0x10, 0xCB, 0xBF, 0xCA, 0xD4, 0x7D, 0x6C,
	0x5C, 0x2F, 0x79, 0x70, 0x06, 0x71, 0x03, 0x5F, 0x36, 0xCE, 0x99, 0x28,
	0xC5, 0xAE, 0x42, 0xAB, 0xBA, 0x39, 0xCE, 0xF0, 0x6E, 0x8E, 0x63, 0xAF,
	0x25, 0x6D, 0x76, 0xA1, 0x9B, 0x44, 0x07, 0xB0, 0x9B, 0xDB, 0xEF, 0xB5,
	0x10, 0x80, 0x86, 0x43, 0x21, 0xED, 0xF5, 0x0A, 0x05, 0x62, 0x4C, 0x61,
	0xF7, 0xFE, 0x18, 0xDB, 0x96, 0x3E, 0xA6, 0x36, 0x9C, 0xB7, 0xB9, 0xFE,
	0x9A, 0x7A, 0x33, 0x33, 0x72, 0xAD, 0x90, 0x0F, 0x60, 0x7D, 0x59, 0x46,
	0x88, 0xB1, 0xC3, 0xE7, 0x71, 0xCE, 0x83, 0xF8, 0x3F, 0x3E, 0x50, 0x0F,
	0x3E, 0xE4, 0x8C, 0x3C, 0x58, 0xF1, 0xF5, 0x3C, 0x25, 0xA5, 0xE2, 0x14,
	0x5B, 0x83, 0xCA, 0x34, 0xE9, 0x15, 0x38, 0x3D
	}
};

static const SceNpCommunicationSignature s_CommSignature =
{
	{
	0xB9, 0xDD, 0xE1, 0x3B, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x9C, 0x62, 0x47, 0xCC, 0xD5, 0x2A, 0xEF, 0x7D, 0xE6, 0xC9, 0x36, 0xC3,
	0xB4, 0x19, 0xD4, 0x8E, 0xD4, 0x95, 0x00, 0x71, 0xC4, 0xCD, 0xC7, 0xF8,
	0x3C, 0xD3, 0xFC, 0x3D, 0xF3, 0xA5, 0x96, 0x30, 0xC0, 0xF0, 0x1D, 0x69,
	0xAF, 0xEE, 0x37, 0xB9, 0x4B, 0xB2, 0x3D, 0xE4, 0x02, 0xE1, 0xCE, 0xF6,
	0x90, 0xEB, 0x08, 0x4B, 0xCC, 0xB2, 0x50, 0x57, 0xCE, 0x33, 0xB3, 0xA9,
	0xAA, 0xC6, 0x77, 0x1D, 0x6A, 0x19, 0xF9, 0x2E, 0x55, 0x4E, 0x03, 0x83,
	0x60, 0x4F, 0x0A, 0xB5, 0x08, 0x49, 0xBE, 0x11, 0xC1, 0xEF, 0x2F, 0x69,
	0x32, 0xD2, 0xCF, 0x6F, 0x7C, 0x4C, 0xD4, 0x0D, 0x27, 0xB2, 0x34, 0x4F,
	0x5A, 0x2B, 0x12, 0x2F, 0xD0, 0xEF, 0xE4, 0xD7, 0x22, 0xE4, 0x9B, 0x80,
	0x5A, 0x8E, 0xBB, 0x3A, 0xE1, 0x9A, 0xE3, 0x8C, 0x29, 0x3F, 0x9C, 0xEC,
	0x58, 0x74, 0x6C, 0xB0, 0x55, 0x37, 0x9C, 0x93, 0xAE, 0x0A, 0x92, 0x1C,
	0x13, 0x41, 0x06, 0x67, 0x04, 0x53, 0x7A, 0xD4, 0x6E, 0xCC, 0x0F, 0xED,
	0xC6, 0xB3, 0x8F, 0x57
	}
};


static const SceNpCommunicationConfig s_commConfig = {
	/* .commId =		*/ &s_CommId,
	/* .commPassphrase =*/ &s_CommPassphrase,
	/* .commSignature = */ &s_CommSignature
};

static nputils::TUS *s_tus = NULL;
static char s_opbuf[SCE_KERNEL_1KiB];
static string s_tag;

nputils::TUS::TUS()
{
	m_ctx = 0;
}

nputils::TUS::~TUS()
{
	Term();
}

int32_t nputils::TUS::Init()
{
	int32_t ret = sceNpTusInit(SCE_KERNEL_DEFAULT_PRIORITY_USER, SCE_KERNEL_THREAD_CPU_AFFINITY_MASK_DEFAULT, NULL);
	if (ret < 0)
	{
		return ret;
	}

	ret = sceNpManagerGetNpId(&m_npid);
	if (ret < 0)
	{
		sceNpTusTerm();
		return ret;
	}

	ret = sceNpTusCreateTitleCtx(NULL, NULL, NULL);
	if (ret < 0)
	{
		sceNpTusTerm();
		return ret;
	}

	m_ctx = ret;

	return SCE_OK;
}

int32_t nputils::TUS::Term()
{
	int32_t ret = sceNpTusDeleteTitleCtx(m_ctx);
	if (ret < 0)
	{
		return ret;
	}

	return sceNpTusTerm();
}

int32_t nputils::TUS::UploadFile(uint32_t slot, const char *path)
{
	int32_t ret = SCE_OK;
	int32_t request = 0;
	SceNpTusDataInfo labelInfo;

	ret = sceNpTusCreateRequest(m_ctx);
	if (ret < 0)
	{
		return ret;
	}
	request = ret;

	common::SharedPtr<LocalFile> file = LocalFile::Open(path, SCE_O_RDONLY, 0, &ret);
	if (ret < 0)
	{
		sceNpTusDeleteRequest(request);
		return ret;
	}

	uint32_t fsz = file->GetFileSize();
	char *fbuf = static_cast<char*>(sce_paf_malloc(fsz));
	if (!fbuf)
	{
		file->Close();
		sceNpTusDeleteRequest(request);
		return SCE_ERROR_ERRNO_ENOMEM;
	}
	file->Read(fbuf, fsz);
	file->Close();

	sce_paf_memset(&labelInfo, 0, sizeof(SceNpTusDataInfo));
	sce_paf_strncpy(reinterpret_cast<char*>(labelInfo.data), s_tag.c_str(), sizeof(labelInfo.data) - 1);
	labelInfo.infoSize = sce_paf_strlen(reinterpret_cast<char*>(labelInfo.data)) + 1;

	ret = sceNpTusSetData(request, &m_npid, slot, fsz, fsz, fbuf, &labelInfo, sizeof(SceNpTusDataInfo), NULL, NULL, NULL);

	sce_paf_free(fbuf);

	sceNpTusDeleteRequest(request);

	return ret;
}

int32_t nputils::TUS::DownloadFile(uint32_t slot, const char *path)
{
	int32_t ret = SCE_OK;
	int32_t request = 0;
	int32_t recvedSize = 0;
	SceNpTusDataStatus dataStatus;

	ret = sceNpTusCreateRequest(m_ctx);
	if (ret < 0)
	{
		return ret;
	}
	request = ret;

	common::SharedPtr<LocalFile> file = LocalFile::Open(path, SCE_O_WRONLY | SCE_O_CREAT, 0666, &ret);
	if (ret < 0)
	{
		sceNpTusDeleteRequest(request);
		return ret;
	}

	do
	{
		ret = sceNpTusGetData(request, &m_npid, slot, &dataStatus, sizeof(SceNpTusDataStatus), s_opbuf, sizeof(s_opbuf), NULL);
		if (ret < 0)
		{
			recvedSize = ret;
			break;
		}
		if (dataStatus.hasData == 0)
		{
			recvedSize = -1;
			break;
		}

		file->Write(s_opbuf, ret);
		recvedSize += ret;

	} while (recvedSize < dataStatus.dataSize);

	file->Close();

	sceNpTusDeleteRequest(request);

	return recvedSize;
}

int32_t nputils::TUS::UploadString(uint32_t slot, const string& data)
{
	int32_t ret = SCE_OK;
	int32_t request = 0;
	SceNpTusDataInfo labelInfo;

	ret = sceNpTusCreateRequest(m_ctx);
	if (ret < 0)
	{
		return ret;
	}
	request = ret;

	sce_paf_memset(&labelInfo, 0, sizeof(SceNpTusDataInfo));
	sce_paf_strncpy(reinterpret_cast<char*>(labelInfo.data), s_tag.c_str(), sizeof(labelInfo.data) - 1);
	labelInfo.infoSize = sce_paf_strlen(reinterpret_cast<char*>(labelInfo.data)) + 1;

	ret = sceNpTusSetData(request, &m_npid, slot, data.length(), data.length(), data.c_str(), &labelInfo, sizeof(SceNpTusDataInfo), NULL, NULL, NULL);

	sceNpTusDeleteRequest(request);

	return ret;
}

int32_t nputils::TUS::DownloadString(uint32_t slot, string& data)
{
	int32_t ret = SCE_OK;
	int32_t request = 0;
	int32_t recvedSize = 0;
	SceNpTusDataStatus dataStatus;

	ret = sceNpTusCreateRequest(m_ctx);
	if (ret < 0)
	{
		return ret;
	}
	request = ret;

	do
	{
		ret = sceNpTusGetData(request, &m_npid, slot, &dataStatus, sizeof(SceNpTusDataStatus), s_opbuf, sizeof(s_opbuf), NULL);
		if (ret < 0)
		{
			recvedSize = ret;
			break;
		}
		if (dataStatus.hasData == 0)
		{
			recvedSize = -1;
			break;
		}

		data.append(s_opbuf, ret);
		recvedSize += ret;

	} while (recvedSize < dataStatus.dataSize);

	sceNpTusDeleteRequest(request);

	return recvedSize;
}

int32_t nputils::TUS::GetDataStatus(uint32_t slot, SceNpTusDataStatus *status)
{
	int32_t ret = SCE_OK;
	int32_t request = 0;

	ret = sceNpTusCreateRequest(m_ctx);
	if (ret < 0)
	{
		return ret;
	}
	request = ret;

	ret = sceNpTusGetMultiSlotDataStatus(request, &m_npid, reinterpret_cast<const SceNpTusSlotId*>(&slot), status, sizeof(SceNpTusDataStatus), 1, NULL);

	sceNpTusDeleteRequest(request);

	return ret;
}

int32_t nputils::TUS::DeleteData(uint32_t slot)
{
	vector<uint32_t> lslots;
	lslots.push_back(slot);

	return DeleteData(&lslots);
}

int32_t nputils::TUS::DeleteData(const vector<uint32_t> *slots)
{
	int32_t ret = SCE_OK;
	int32_t request = 0;
	SceNpTusSlotId lslots[16];

	if (!slots)
	{
		return ret;
	}

	ret = sceNpTusCreateRequest(m_ctx);
	if (ret < 0)
	{
		return ret;
	}
	request = ret;

	for (int i = 0; i < slots->size(); i++)
	{
		lslots[i] = slots->at(i);
	}

	ret = sceNpTusDeleteMultiSlotData(request, &m_npid, lslots, slots->size(), NULL);

	sceNpTusDeleteRequest(request);

	return ret;
}

int32_t nputils::Init(const char *tag, bool needTUS, const vector<uint32_t> *usedTUSSlots)
{
	int32_t ret = sceNpInit(&s_commConfig, NULL);
	if (ret < 0)
	{
		return ret;
	}

	ret = sceNpAuthInit();
	if (ret < 0)
	{
		Term();
		return ret;
	}

	if (tag)
	{
		s_tag = tag;
	}

	if (needTUS)
	{
		s_tus = new TUS();
		ret = s_tus->Init();
		if (ret < 0)
		{
			Term();
			return ret;
		}

		if (usedTUSSlots)
		{
			SceNpTusDataStatus status;
			ret = s_tus->GetDataStatus(usedTUSSlots->at(0), &status);
			if (ret < 0)
			{
				Term();
				return ret;
			}

			if (status.hasData)
			{
				if ((status.info.infoSize != s_tag.length() + 1) || sce_paf_strcmp(reinterpret_cast<char*>(status.info.data), s_tag.c_str()))
				{
					s_tus->DeleteData(usedTUSSlots);
				}
			}
		}
	}

	return ret;
}

int32_t nputils::Term()
{
	if (s_tus)
	{
		delete s_tus;
		s_tus = NULL;
	}

	sceNpAuthTerm();

	return sceNpTerm();
}

nputils::TUS *nputils::GetTUS()
{
	return s_tus;
}
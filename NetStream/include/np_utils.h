#ifndef _NP_UTILS_H_
#define _NP_UTILS_H_

#include <kernel.h>
#include <paf.h>
#include <np.h>

using namespace paf;

namespace nputils
{
	class TUS
	{
	public:

		TUS();

		~TUS();

		int32_t Init();

		int32_t Term();

		int32_t UploadFile(uint32_t slot, const char *path);

		int32_t DownloadFile(uint32_t slot, const char *path);

		int32_t UploadString(uint32_t slot, const string& data);

		int32_t DownloadString(uint32_t slot, string& data);

		int32_t GetDataStatus(uint32_t slot, SceNpTusDataStatus *status);

		int32_t DeleteData(uint32_t slot);

		int32_t DeleteData(const vector<uint32_t> *slots);

	private:

		SceNpId m_npid;
		int32_t m_ctx;
	};

	int32_t PreInit(thread::SyncCall::Function onComplete);

	int32_t Init(const char *tag = NULL, bool needTUS = false, const vector<uint32_t> *usedTUSSlots = NULL);

	int32_t Term();

	bool IsAllGreen();

	TUS *GetTUS();

	void NetDialogInit(void *data);

	void NetDialogCheck(void *data);
};

#endif

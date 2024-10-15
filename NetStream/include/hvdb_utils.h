#ifndef _HVDBUTILS_H_
#define _HVDBUTILS_H_

#include <kernel.h>
#include <paf.h>
#include <ini_file_processor.h>

#include "yt_utils.h"
#include "downloader.h"

using namespace paf;

namespace hvdbutils
{
	class EntryLog : public ytutils::Log
	{
	public:

		EntryLog();

		static void Clean();
	};

	void Init();

	void Term();

	void Flush();

	EntryLog *GetEntryLog();

	int32_t EnqueueDownload(const char *url, const char *name);

	int32_t EnqueueDownloadAsync(const char *url, const char *name);
};

#endif

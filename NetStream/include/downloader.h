#ifndef _ELEVENMPV_DOWNLOADER_H_
#define _ELEVENMPV_DOWNLOADER_H_

#include <kernel.h>
#include <paf.h>
#include <ipmi.h>
#include <download_service.h>

using namespace paf;

class Downloader
{
public:

	Downloader();

	~Downloader();

	SceInt32 Enqueue(const char *url, const char *name);

	SceInt32 EnqueueAsync(const char *url, const char *name);

private:

	class AsyncEnqueue : public job::JobItem
	{
	public:

		using job::JobItem::JobItem;

		~AsyncEnqueue() {}

		SceVoid Run()
		{
			Downloader *pdownloader = (Downloader *)downloader;
			pdownloader->Enqueue(url8.c_str(), name8.c_str());
		}

		SceVoid Finish() {}

		string url8;
		string name8;
		ScePVoid downloader;
	};

	sce::Download dw;
};

#endif

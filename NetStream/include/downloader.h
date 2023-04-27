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

	typedef void(*OnStartCallback)(int32_t result);

	Downloader();

	~Downloader();

	int32_t Enqueue(const char *url, const char *name, OnStartCallback cb = NULL);

	int32_t EnqueueAsync(const char *url, const char *name, OnStartCallback cb = NULL);

private:

	class AsyncEnqueue : public job::JobItem
	{
	public:

		using job::JobItem::JobItem;

		~AsyncEnqueue() {}

		void Run()
		{
			Downloader *pdownloader = (Downloader *)downloader;
			pdownloader->Enqueue(url8.c_str(), name8.c_str(), onStart);
		}

		void Finish() {}

		string url8;
		string name8;
		void *downloader;
		OnStartCallback onStart;
	};

	sce::Download dw;
};

#endif

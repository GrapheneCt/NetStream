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

	enum
	{
		DownloaderEvent = (ui::Handler::CB_STATE + 0x60000),
	};

	Downloader();

	~Downloader();

	int32_t Enqueue(Plugin *workPlugin, const char *url, const char *name);

	int32_t EnqueueAsync(Plugin *workPlugin, const char *url, const char *name);

private:

	class AsyncEnqueue : public job::JobItem
	{
	public:

		using job::JobItem::JobItem;

		~AsyncEnqueue() {}

		void Run()
		{
			Downloader *pdownloader = (Downloader *)downloader;
			pdownloader->Enqueue(plugin, url8.c_str(), name8.c_str());
		}

		void Finish() {}

		string url8;
		string name8;
		void *downloader;
		Plugin *plugin;
	};

	sce::Download dw;
};

#endif

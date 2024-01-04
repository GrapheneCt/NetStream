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

		AsyncEnqueue(Plugin *workPlugin, Downloader *downloader, const char *url, const char *name) :
		job::JobItem("Downloader::AsyncEnqueue", NULL), m_plugin(workPlugin), m_downloader(downloader), m_url8(url), m_name8(name)
		{

		}

		~AsyncEnqueue() {}

		void Run()
		{
			Downloader *pdownloader = (Downloader *)m_downloader;
			pdownloader->Enqueue(m_plugin, m_url8.c_str(), m_name8.c_str());
		}

		void Finish() {}

	private:

		string m_url8;
		string m_name8;
		void *m_downloader;
		Plugin *m_plugin;
	};

	sce::Download dw;
};

#endif

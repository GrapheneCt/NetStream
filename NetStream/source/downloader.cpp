#include <kernel.h>
#include <libsysmodule.h>
#include <paf.h>
#include <ipmi.h>
#include <download_service.h>

#include "downloader.h"
#include "event.h"

using namespace paf;

Downloader::Downloader()
{
	IPMI::Client::Config conf;
	IPMI::Client *client;
	uint32_t clMemSize;
	int32_t ret;

	sce_paf_memset(&dw, 0, sizeof(sce::Download));
	sce_paf_memset(&conf, 0, sizeof(IPMI::Client::Config));

	sce_paf_strncpy(reinterpret_cast<char *>(conf.serviceName), "SceDownload", sizeof(conf.serviceName));
	conf.pipeBudgetType = IPMI::BudgetType_Default;
	conf.numResponses = 1;
	conf.requestQueueSize = 0x1E00;
	conf.receptionQueueSize = 0x1E00;
	conf.numAsyncResponses = 1;
	conf.requestQueueAddsize1 = 0xF00;
	conf.requestQueueAddsize2 = 0xF00;
	conf.numEventFlags = 1;
	conf.msgQueueSize = 0;
	conf.serverPID = SCE_UID_INVALID_UID;

	clMemSize = conf.estimateClientMemorySize();
	dw.clientMem = sce_paf_malloc(clMemSize);
	dw.bufMem = sce_paf_malloc(SCE_KERNEL_4KiB);

	IPMI::Client::create(&client, &conf, &dw, dw.clientMem);

	dw.client = client;

	sce::Download::ConnectionOpt connOpt;
	sce_paf_memset(&connOpt, 0, sizeof(sce::Download::ConnectionOpt));
	connOpt.budgetType = conf.pipeBudgetType;

	int ret2 = client->connect(&connOpt, sizeof(sce::Download::ConnectionOpt), &ret);

	dw.unk_00 = SCE_UID_INVALID_UID;
	dw.unk_04 = SCE_UID_INVALID_UID;
	dw.unk_08 = SCE_UID_INVALID_UID;
}

Downloader::~Downloader()
{
	dw.client->disconnect();
	dw.client->destroy();
	sce_paf_free(dw.clientMem);
	sce_paf_free(dw.bufMem);
}

int32_t Downloader::Enqueue(Plugin *workPlugin, const char *url, const char *name)
{
	IPMI::DataInfo dtInfo[3];
	IPMI::BufferInfo bfInfo[1];
	int32_t ret = SCE_OK;
	int32_t ret2 = SCE_OK;
	int32_t dwRes = 0;

	sce::Download::GetHeaderInfoParam hparam;
	sce::Download::AuthParam aparam;
	sce::Download::DownloadParam dparam;
	sce::Download::HeaderInfo minfo;
	sce_paf_memset(&hparam, 0, sizeof(sce::Download::GetHeaderInfoParam));
	sce_paf_memset(&aparam, 0, sizeof(sce::Download::AuthParam));
	sce_paf_memset(&dparam, 0, sizeof(sce::Download::DownloadParam));
	sce_paf_memset(&minfo, 0, sizeof(sce::Download::HeaderInfo));

	hparam.contentType = sce::Download::ContentType_Multimedia;
	hparam.resolveTimeout = 5000;
	hparam.resolveRetry = 2;
	hparam.connectTimeout = 3000;
	hparam.sendTimeout = 0;
	hparam.recvTimeout = 3000;
	sce_paf_strcpy((char *)hparam.url, url);

	dtInfo[0].data = &hparam;
	dtInfo[0].dataSize = sizeof(sce::Download::GetHeaderInfoParam);
	dtInfo[1].data = &aparam;
	dtInfo[1].dataSize = sizeof(sce::Download::AuthParam);

	bfInfo[0].data = &minfo;
	bfInfo[0].dataSize = sizeof(sce::Download::HeaderInfo);

	ret = dw.client->invokeSyncMethod(sce::Download::Method_GetHeaderInfo, dtInfo, 2, &ret2, bfInfo, 1);
	if (ret != SCE_OK)
		goto end;
	else if (ret2 != SCE_OK)
	{
		ret = ret2;
		goto end;
	}

	sce_paf_memset(&minfo.name, 0, sizeof(minfo.name));
	sce_paf_strcpy(reinterpret_cast<char *>(minfo.name), name);

	dparam.bgdlLocation = 1;
	sce_paf_strcpy(reinterpret_cast<char *>(dparam.url), url);

	dtInfo[0].data = &dparam;
	dtInfo[0].dataSize = sizeof(sce::Download::DownloadParam);
	dtInfo[1].data = &aparam;
	dtInfo[1].dataSize = sizeof(sce::Download::AuthParam);
	dtInfo[2].data = &minfo;
	dtInfo[2].dataSize = sizeof(sce::Download::HeaderInfo);

	bfInfo[0].data = &dwRes;
	bfInfo[0].dataSize = sizeof(int32_t);

	ret2 = SCE_OK;
	ret = dw.client->invokeSyncMethod(sce::Download::Method_Download, dtInfo, 3, &ret2, bfInfo, 1);
	if (ret2 != SCE_OK)
	{
		//invalid filename?
		sce_paf_memset(&minfo.name, 0, sizeof(minfo.name));
		char *ext = sce_paf_strrchr(name, '.');
		sce_paf_snprintf(reinterpret_cast<char *>(minfo.name), sizeof(minfo.name), "DefaultFilename_%u%s", sceKernelGetProcessTimeLow(), ext);

		ret2 = SCE_OK;
		ret = dw.client->invokeSyncMethod(sce::Download::Method_Download, dtInfo, 3, &ret2, bfInfo, 1);
		if (ret2 != SCE_OK)
		{
			ret = ret2;
			goto end;
		}
	}

end:

	event::BroadcastGlobalEvent(workPlugin, DownloaderEvent, ret);

	return ret;
}

int32_t Downloader::EnqueueAsync(Plugin *workPlugin, const char *url, const char *name)
{
	AsyncEnqueue *dwJob = new AsyncEnqueue(workPlugin, this, url, name);
	common::SharedPtr<job::JobItem> itemParam(dwJob);

	return job::JobQueue::DefaultQueue()->Enqueue(itemParam);
}
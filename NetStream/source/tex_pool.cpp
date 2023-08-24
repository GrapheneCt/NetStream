#include <kernel.h>
#include <libdbg.h>
#include <paf.h>
#include <paf_file_ext.h>

#include "tex_pool.h"

TexPool::TexPool(Plugin *_cbPlugin, bool useDefaultQueue)
{
	storMtx = new thread::RMutex("TexPool::StorMtx");
	if (useDefaultQueue)
	{
		addAsyncQueue = job::JobQueue::default_queue;
	}
	else
	{
		addAsyncQueue = new job::JobQueue("TexPool::AddAsyncJobQueue");
	}
	share = NULL;
	alive = true;
	cbPlugin = _cbPlugin;
}

TexPool::~TexPool()
{
	SetAlive(false);
	AddAsyncWaitComplete();
	RemoveAll();
	if (addAsyncQueue != job::JobQueue::default_queue)
	{
		delete addAsyncQueue;
	}
	delete storMtx;
}

void TexPool::DestroyAsync()
{
	SetAlive(false);
	DestroyJob *job = new DestroyJob("TexPool::DestroyJob");
	job->pool = this;
	common::SharedPtr<job::JobItem> itemParam(job);
	job::JobQueue::default_queue->Enqueue(itemParam);
}

bool TexPool::Add(Plugin *plugin, IDParam const& id, bool allowReplace)
{
	if (!alive)
	{
		return false;
	}

	if (!allowReplace)
	{
		if (Exist(id))
		{
			return false;
		}
	}

	intrusive_ptr<graph::Surface> tex = plugin->GetTexture(id);
	if (tex.get())
	{
		storMtx->Lock();
		RemoveForReplace(id);
		stor[id.GetIDHash()] = tex;
		storMtx->Unlock();
		return true;
	}

	Remove(id);

	return false;
}

bool TexPool::Add(IDParam const& id, uint8_t *buffer, size_t bufferSize, bool allowReplace)
{
	if (!alive)
	{
		return false;
	}

	if (!allowReplace)
	{
		if (Exist(id))
		{
			return false;
		}
	}

	int32_t res = -1;
	common::SharedPtr<MemFile> mfile = MemFile::Open(buffer, bufferSize, &res);
	if (res != SCE_PAF_OK)
	{
		Remove(id);
		return false;
	}

	intrusive_ptr<graph::Surface> tex = graph::Surface::Load(gutil::GetDefaultSurfacePool(), (common::SharedPtr<File>&)mfile);
	if (tex.get())
	{
		storMtx->Lock();
		RemoveForReplace(id);
		stor[id.GetIDHash()] = tex;
		storMtx->Unlock();
		return true;
	}

	Remove(id);

	return false;
}

bool TexPool::Add(IDParam const& id, const char *path, bool allowReplace)
{
	if (!alive)
	{
		return false;
	}

	if (!allowReplace)
	{
		if (Exist(id))
		{
			return false;
		}
	}

	if (!sce_paf_strncmp(path, "http", 4))
	{
		return AddHttp(id, path);
	}
	
	return AddLocal(id, path);
}

bool TexPool::Add(IDParam const& id, intrusive_ptr<graph::Surface>& surf, bool allowReplace)
{
	if (!alive)
	{
		return false;
	}

	if (!allowReplace)
	{
		if (Exist(id))
		{
			return false;
		}
	}

	if (surf.get())
	{
		storMtx->Lock();
		RemoveForReplace(id);
		stor[id.GetIDHash()] = surf;
		storMtx->Unlock();
		return true;
	}

	Remove(id);

	return false;
}

bool TexPool::AddAsync(IDParam const& id, uint8_t *buffer, size_t bufferSize, bool allowReplace)
{
	if (!alive)
	{
		return false;
	}

	if (!allowReplace)
	{
		if (Exist(id))
		{
			return false;
		}
	}
	AddAsyncJob *job = new AddAsyncJob("TexPool::AddAsyncJob");
	job->workObj = this;
	job->hash = id.GetIDHash();
	job->buf = buffer;
	job->bufSize = bufferSize;
	common::SharedPtr<job::JobItem> itemParam(job);
	addAsyncQueue->Enqueue(itemParam);
	return true;
}

bool TexPool::AddAsync(IDParam const& id, const char *path, bool allowReplace)
{
	if (!alive)
	{
		return false;
	}

	if (!allowReplace)
	{
		if (Exist(id))
		{
			return false;
		}
	}

	AddAsyncJob *job = new AddAsyncJob("TexPool::AddAsyncJob");
	job->workObj = this;
	job->hash = id.GetIDHash();
	job->path = path;
	job->buf = NULL;
	job->bufSize = 0;
	common::SharedPtr<job::JobItem> itemParam(job);
	addAsyncQueue->Enqueue(itemParam);
	return true;
}

void TexPool::Remove(IDParam const& id)
{
	storMtx->Lock();
	stor[id.GetIDHash()].clear();
	stor.erase(id.GetIDHash());
	storMtx->Unlock();
}

void TexPool::RemoveAll()
{
	storMtx->Lock();
	map<uint32_t, intrusive_ptr<graph::Surface>>::iterator it = stor.begin();
	while (it != stor.end())
	{
		it->second.clear();
		it++;
	}
	stor.clear();
	storMtx->Unlock();
}

bool TexPool::Exist(IDParam const& id)
{
	storMtx->Lock();
	bool ret = (stor[id.GetIDHash()].get() != NULL);
	storMtx->Unlock();
	return ret;
}

intrusive_ptr<graph::Surface> TexPool::Get(IDParam const& id)
{
	storMtx->Lock();
	intrusive_ptr<graph::Surface> tex = stor[id.GetIDHash()];
	storMtx->Unlock();
	return tex;
}

void TexPool::AddAsyncWaitComplete()
{
	addAsyncQueue->WaitEmpty();
}

int32_t TexPool::AddAsyncActiveNum()
{
	return addAsyncQueue->NumItems();
}

uint32_t TexPool::GetSize()
{
	storMtx->Lock();
	uint32_t sz = stor.size();
	storMtx->Unlock();
	return sz;
}

void TexPool::RemoveForReplace(IDParam const& id)
{
	stor[id.GetIDHash()].clear();
}

bool TexPool::AddHttp(IDParam const& id, const char *path)
{
	int32_t res = -1;
	CurlFile::OpenArg oarg;
	oarg.ParseUrl(path);
	oarg.SetOption(3000, CurlFile::OpenArg::OptionType_ConnectTimeOut);
	oarg.SetOption(5000, CurlFile::OpenArg::OptionType_RecvTimeOut);
	if (share)
	{
		oarg.SetShare(share);
	}

	if (!alive)
	{
		return false;
	}
	CurlFile *file = new CurlFile();
	res = file->Open(&oarg);
	if (res != SCE_PAF_OK)
	{
		delete file;
		Remove(id);
		return false;
	}

	if (!alive)
	{
		delete file;
		return false;
	}
	common::SharedPtr<CurlFile> hfile(file);
	intrusive_ptr<graph::Surface> tex = graph::Surface::Load(gutil::GetDefaultSurfacePool(), (common::SharedPtr<File>&)hfile);
	if (tex.get())
	{
		storMtx->Lock();
		RemoveForReplace(id);
		stor[id.GetIDHash()] = tex;
		storMtx->Unlock();
		return true;
	}

	Remove(id);

	return false;
}

bool TexPool::AddLocal(IDParam const& id, const char *path)
{
	if (!alive)
	{
		return false;
	}
	int32_t res = -1;
	common::SharedPtr<LocalFile> lfile = LocalFile::Open(path, SCE_O_RDONLY, 0, &res);
	if (res != SCE_PAF_OK)
	{
		Remove(id);
		return false;
	}

	if (!alive)
	{
		lfile.reset();
		return false;
	}
	intrusive_ptr<graph::Surface> tex = graph::Surface::Load(gutil::GetDefaultSurfacePool(), (common::SharedPtr<File>&)lfile);
	if (tex.get())
	{
		storMtx->Lock();
		RemoveForReplace(id);
		stor[id.GetIDHash()] = tex;
		storMtx->Unlock();
		return true;
	}

	Remove(id);

	return false;
}
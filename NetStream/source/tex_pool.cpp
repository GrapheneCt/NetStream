#include <kernel.h>
#include <libdbg.h>
#include <paf.h>
#include <paf_file_ext.h>

#include "utils.h"
#include "tex_pool.h"

TexPool::TexPool(Plugin *_cbPlugin, bool useDefaultQueue)
{
	m_storMtx = new thread::RMutex("TexPool::StorMtx");
	if (useDefaultQueue)
	{
		m_addAsyncQueue = job::JobQueue::DefaultQueue();
	}
	else
	{
		m_addAsyncQueue = new job::JobQueue("TexPool::AddAsyncJobQueue");
	}
	m_share = NULL;
	m_alive = true;
	m_cbPlugin = _cbPlugin;
}

TexPool::~TexPool()
{
	SetAlive(false);
	AddAsyncCancelPending();
	RemoveAll();
	if (m_addAsyncQueue != job::JobQueue::DefaultQueue())
	{
		delete m_addAsyncQueue;
	}
	delete m_storMtx;
}

void TexPool::DestroyAsync()
{
	SetAlive(false);
	DestroyJob *job = new DestroyJob(this);
	common::SharedPtr<job::JobItem> itemParam(job);
	job::JobQueue::DefaultQueue()->Enqueue(itemParam);
}

bool TexPool::Add(Plugin *plugin, IDParam const& id, bool allowReplace)
{
	if (!m_alive)
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
		m_storMtx->Lock();
		RemoveForReplace(id);
		m_stor[id.GetIDHash()] = tex;
		m_storMtx->Unlock();
		return true;
	}

	Remove(id);

	return false;
}

bool TexPool::Add(IDParam const& id, uint8_t *buffer, size_t bufferSize, bool allowReplace)
{
	if (!m_alive)
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
		m_storMtx->Lock();
		RemoveForReplace(id);
		m_stor[id.GetIDHash()] = tex;
		m_storMtx->Unlock();
		return true;
	}

	Remove(id);

	return false;
}

bool TexPool::Add(IDParam const& id, const char *path, bool allowReplace)
{
	if (!m_alive)
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
	if (!m_alive)
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
		m_storMtx->Lock();
		RemoveForReplace(id);
		m_stor[id.GetIDHash()] = surf;
		m_storMtx->Unlock();
		return true;
	}

	Remove(id);

	return false;
}

bool TexPool::AddAsync(IDParam const& id, uint8_t *buffer, size_t bufferSize, bool allowReplace)
{
	if (!m_alive)
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
	AddAsyncJob *job = new AddAsyncJob(this, id.GetIDHash(), buffer, bufferSize);
	common::SharedPtr<job::JobItem> itemParam(job);
	m_addAsyncQueue->Enqueue(itemParam);
	return true;
}

bool TexPool::AddAsync(IDParam const& id, const char *path, bool allowReplace)
{
	if (!m_alive)
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

	AddAsyncJob *job = new AddAsyncJob(this, id.GetIDHash(), path);
	common::SharedPtr<job::JobItem> itemParam(job);
	m_addAsyncQueue->Enqueue(itemParam);
	return true;
}

void TexPool::Remove(IDParam const& id)
{
	m_storMtx->Lock();
	m_stor[id.GetIDHash()].clear();
	m_stor.erase(id.GetIDHash());
	m_storMtx->Unlock();
}

void TexPool::RemoveAll()
{
	m_storMtx->Lock();
	map<uint32_t, intrusive_ptr<graph::Surface>>::iterator it = m_stor.begin();
	while (it != m_stor.end())
	{
		it->second.clear();
		it++;
	}
	m_stor.clear();
	m_storMtx->Unlock();
}

bool TexPool::Exist(IDParam const& id)
{
	m_storMtx->Lock();
	bool ret = (m_stor[id.GetIDHash()].get() != NULL);
	m_storMtx->Unlock();
	return ret;
}

intrusive_ptr<graph::Surface> TexPool::Get(IDParam const& id)
{
	m_storMtx->Lock();
	intrusive_ptr<graph::Surface> tex = m_stor[id.GetIDHash()];
	m_storMtx->Unlock();
	return tex;
}

void TexPool::AddAsyncWaitComplete()
{
	m_addAsyncQueue->WaitEmpty();
}

int32_t TexPool::AddAsyncActiveNum()
{
	return m_addAsyncQueue->NumItems();
}

void TexPool::AddAsyncCancelPending()
{
	m_addAsyncQueue->CancelAllItems();
}

uint32_t TexPool::GetSize()
{
	m_storMtx->Lock();
	uint32_t sz = m_stor.size();
	m_storMtx->Unlock();
	return sz;
}

void TexPool::RemoveForReplace(IDParam const& id)
{
	m_stor[id.GetIDHash()].clear();
}

bool TexPool::AddHttp(IDParam const& id, const char *path)
{
	int32_t res = -1;
	CurlFile::OpenArg oarg;
	oarg.ParseUrl(path);
	oarg.SetOption(3000, CurlFile::OpenArg::OptionType_ConnectTimeOut);
	oarg.SetOption(5000, CurlFile::OpenArg::OptionType_RecvTimeOut);
	oarg.SetProxy(utils::GetGlobalProxy());
	if (m_share)
	{
		oarg.SetShare(m_share);
	}

	if (!m_alive)
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

	if (!m_alive)
	{
		delete file;
		return false;
	}
	common::SharedPtr<CurlFile> hfile(file);
	intrusive_ptr<graph::Surface> tex = graph::Surface::Load(gutil::GetDefaultSurfacePool(), reinterpret_cast<common::SharedPtr<File>&>(hfile));
	if (tex.get())
	{
		m_storMtx->Lock();
		RemoveForReplace(id);
		m_stor[id.GetIDHash()] = tex;
		m_storMtx->Unlock();
		return true;
	}

	Remove(id);

	return false;
}

bool TexPool::AddLocal(IDParam const& id, const char *path)
{
	if (!m_alive)
	{
		return false;
	}
	int32_t res = -1;
	common::SharedPtr<LocalFile> lfile = LocalFile::Open(path, File::RDONLY, 0, &res);
	if (res != SCE_PAF_OK)
	{
		Remove(id);
		return false;
	}

	if (!m_alive)
	{
		lfile.reset();
		return false;
	}
	intrusive_ptr<graph::Surface> tex = graph::Surface::Load(gutil::GetDefaultSurfacePool(), reinterpret_cast<common::SharedPtr<File>&>(lfile));
	if (tex.get())
	{
		m_storMtx->Lock();
		RemoveForReplace(id);
		m_stor[id.GetIDHash()] = tex;
		m_storMtx->Unlock();
		return true;
	}

	Remove(id);

	return false;
}
#ifndef _TEX_POOL_H_
#define _TEX_POOL_H_

#include <kernel.h>
#include <paf.h>
#include <paf_file_ext.h>

#include "event.h"

using namespace paf;

class TexPool
{
public:

	typedef void(*OnAddAsyncComplete)(uint32_t hash, bool result, void *pArgBlock);

	TexPool(Plugin *_cbPlugin = NULL, bool useDefaultQueue = false);

	~TexPool();

	void DestroyAsync();

	intrusive_ptr<graph::Surface> Get(IDParam const& id);

	bool Add(Plugin *plugin, IDParam const& id, bool allowReplace = false);
	bool Add(IDParam const& id, uint8_t *buffer, size_t bufferSize, bool allowReplace = false);
	bool Add(IDParam const& id, const char *path, bool allowReplace = false);
	bool Add(IDParam const& id, intrusive_ptr<graph::Surface>& surf, bool allowReplace = false);

	bool AddAsync(IDParam const& id, uint8_t *buffer, size_t bufferSize, bool allowReplace = false);
	bool AddAsync(IDParam const& id, const char *path, bool allowReplace = false);

	void Remove(IDParam const& id);
	void RemoveAll();

	bool Exist(IDParam const& id);

	void AddAsyncWaitComplete();
	int32_t AddAsyncActiveNum();
	void AddAsyncCancelPending();

	uint32_t GetSize();

	inline void SetShare(CurlFile::Share *sh)
	{
		m_share = sh;
	}

	inline bool IsAlive()
	{
		return m_alive;
	}

	inline void SetAlive(bool value)
	{
		m_alive = value;
	}

private:

	class AddAsyncJob : public job::JobItem
	{
	public:

		AddAsyncJob(TexPool *parent, uint32_t hash, uint8_t *buf, size_t bufSize) :
			job::JobItem("TexPool::AddAsyncJob", NULL), m_parent(parent), m_hash(hash), m_buf(buf), m_bufSize(bufSize)
		{

		}

		AddAsyncJob(TexPool *parent, uint32_t hash, const string& path) :
			job::JobItem("TexPool::AddAsyncJob", NULL), m_parent(parent), m_hash(hash), m_buf(NULL), m_path(path)
		{

		}

		~AddAsyncJob()
		{

		}

		int32_t Run()
		{
			bool result = false;

			if (m_buf)
			{
				result = m_parent->Add(m_hash, m_buf, m_bufSize, true);
			}
			else
			{
				result = m_parent->Add(m_hash, m_path.c_str(), true);
			}

			if (result && m_parent->IsAlive() && m_parent->m_cbPlugin)
			{
				event::BroadcastGlobalEvent(m_parent->m_cbPlugin, ui::Handler::CB_STATE_READY_CACHEIMAGE, m_hash);
			}

			return SCE_PAF_OK;
		}

		void Finish(int32_t result)
		{

		}

	private:

		TexPool *m_parent;
		uint8_t *m_buf;
		size_t m_bufSize;
		uint32_t m_hash;
		string m_path;
	};

	class DestroyJob : public job::JobItem
	{
	public:

		DestroyJob(TexPool *parent) : job::JobItem("TexPool::DestroyJob", NULL), m_parent(parent)
		{

		}

		~DestroyJob()
		{

		}

		int32_t Run()
		{
			delete m_parent;

			return SCE_PAF_OK;
		}

		void Finish(int32_t result)
		{

		}

	private:

		TexPool *m_parent;
	};

	void RemoveForReplace(IDParam const& id);

	bool AddHttp(IDParam const& id, const char *path);

	bool AddLocal(IDParam const& id, const char *path);

	map<uint32_t, intrusive_ptr<graph::Surface>> m_stor;
	CurlFile::Share *m_share;
	thread::RMutex *m_storMtx;
	job::JobQueue *m_addAsyncQueue;
	bool m_alive;
	Plugin *m_cbPlugin;
};

#endif

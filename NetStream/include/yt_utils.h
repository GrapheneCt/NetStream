#ifndef _YTUTILS_H_
#define _YTUTILS_H_

#include <kernel.h>
#include <paf.h>
#include <ini_file_processor.h>

#include "downloader.h"

using namespace paf;

namespace ytutils
{
	class Log
	{
	public:

		class AddAsyncJob : public job::JobItem
		{
		public:

			AddAsyncJob(Log *parent, const char *data) : job::JobItem("ytutils::AddAsyncJob", NULL), m_parent(parent), m_data(data)
			{

			}

			~AddAsyncJob() {}

			void Run();

			void Finish() {}

		private:

			Log *m_parent;
			string m_data;
		};

		Log(uint32_t tus = UINT_MAX) : m_tus(tus)
		{

		}

		virtual ~Log()
		{
			m_ini->flush();
			m_ini->close();
			delete m_ini;
		}

		virtual int32_t GetNext(char *data);

		virtual int32_t Get(const char *data);

		virtual void Reset();

		virtual void Add(const char *data);

		virtual void AddAsync(const char *data);

		virtual void Remove(const char *data);

		virtual void Flush();

		virtual int32_t GetSize();

		virtual int32_t UpdateFromTUS();

		virtual int32_t UploadToTUS();

	protected:

		sce::Ini::IniFileProcessor *m_ini;
		uint32_t m_tus;
	};

	class HistLog : public Log
	{
	public:

		HistLog(uint32_t tus = UINT_MAX);

		virtual int32_t UpdateFromTUS();

		virtual int32_t UploadToTUS();

		static void Clean();

		static const uint32_t k_maxHistItems = 20;
	};

	class FavLog : public Log
	{
	public:

		FavLog(uint32_t tus = UINT_MAX);

		virtual int32_t UpdateFromTUS();

		virtual int32_t UploadToTUS();

		static void Clean();
	};

	class InvDownloadData
	{
	public:

		void *buf;
		uint32_t pos;
	};

	void Init(uint32_t histTUS = UINT_MAX, uint32_t favTUS = UINT_MAX);

	void Term();

	void Flush();

	HistLog *GetHistLog();

	FavLog *GetFavLog();

	int32_t EnqueueDownload(const char *url, const char *name);

	int32_t EnqueueDownloadAsync(const char *url, const char *name);
};

#endif

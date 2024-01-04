#ifndef _TWUTILS_H_
#define _TWUTILS_H_

#include <kernel.h>
#include <paf.h>
#include <ini_file_processor.h>

using namespace paf;

namespace twutils
{
	class Log
	{
	public:

		class AddAsyncJob : public job::JobItem
		{
		public:

			AddAsyncJob(Log *parent, const char *data) : job::JobItem("twutils::AddAsyncJob", NULL), m_parent(parent), m_data(data)
			{

			}

			~AddAsyncJob() {}

			void Run();

			void Finish() {}

		private:

			Log *m_parent;
			string m_data;
		};

		Log()
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

	protected:

		sce::Ini::IniFileProcessor *m_ini;
	};

	class HistLog : public Log
	{
	public:

		HistLog();

		static void Clean();

		static const uint32_t k_maxHistItems = 20;
	};

	class FavLog : public Log
	{
	public:

		FavLog();

		static void Clean();
	};

	class InvDownloadData
	{
	public:

		void *buf;
		uint32_t pos;
	};

	void Init();

	void Term();

	void Flush();

	HistLog *GetHistLog();

	FavLog *GetFavLog();
};

#endif

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

			using job::JobItem::JobItem;

			~AddAsyncJob() {}

			void Run();

			void Finish() {}

			Log *workObj;
			string data;
		};

		Log()
		{

		}

		virtual ~Log()
		{
			ini->flush();
			ini->close();
			delete ini;
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

		sce::Ini::IniFileProcessor *ini;
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

	int32_t EnqueueDownload(const char *url, const char *name);

	int32_t EnqueueDownloadAsync(const char *url, const char *name, Downloader::OnStartCallback cb);
};

#endif

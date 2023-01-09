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

			using job::JobItem::JobItem;

			~AddAsyncJob() {}

			SceVoid Run();

			SceVoid Finish() {}

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

		virtual SceInt32 GetNext(char *data);

		virtual SceInt32 Get(const char *data);

		virtual SceVoid Reset();

		virtual SceVoid Add(const char *data);

		virtual SceVoid AddAsync(const char *data);

		virtual SceVoid Remove(const char *data);

		virtual SceVoid Flush();

		virtual SceInt32 GetSize();

	protected:

		sce::Ini::IniFileProcessor *ini;
	};

	class HistLog : public Log
	{
	public:

		HistLog();

		static SceVoid Clean();

		static const SceUInt32 k_maxHistItems = 20;
	};

	class FavLog : public Log
	{
	public:

		FavLog();

		static SceVoid Clean();
	};

	class InvDownloadData
	{
	public:

		ScePVoid buf;
		SceUInt32 pos;
	};

	SceVoid Init();

	SceVoid Term();

	SceVoid Flush();

	HistLog *GetHistLog();

	FavLog *GetFavLog();
};

#endif

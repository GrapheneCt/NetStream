#ifndef _UTILS_H_
#define _UTILS_H_

#include <kernel.h>
#include <paf.h>

using namespace paf;

#define ROUND_UP(x, a)	(((x) + ((a) - 1)) & ~((a) - 1))
#define ROUND_DOWN(x, a) ((x) & ~((a) - 1))
#define IS_ALIGNED(x, a) (((x) & ((a) - 1)) == 0)

namespace utils
{
	enum PowerTick
	{
		PowerTick_None,
		PowerTick_All,
		PowerTick_Suspend
	};

	class SimpleEventCallback : public ui::EventCallback
	{
	public:

		SimpleEventCallback(EventCallback::EventHandler function, ScePVoid userArg = SCE_NULL)
		{
			eventHandler = function;
			pUserData = userArg;
		};

		virtual ~SimpleEventCallback()
		{

		};
	};

	class AsyncNetworkSurfaceLoader
	{
	public:

		AsyncNetworkSurfaceLoader(const char *url, ui::Widget *target, graph::Surface **surf = SCE_NULL, SceBool autoLoad = true);

		~AsyncNetworkSurfaceLoader();

		SceVoid Load();

		SceVoid Abort();

	private:

		class TargetDeleteEventCallback : public ui::EventCallback
		{
		public:

			TargetDeleteEventCallback(AsyncNetworkSurfaceLoader *parent)
			{
				workObj = parent;
			}

			virtual ~TargetDeleteEventCallback();

			AsyncNetworkSurfaceLoader *workObj;
		};

		class Job : public job::JobItem
		{
		public:

			using job::JobItem::JobItem;

			~Job() { }

			SceVoid Run();

			SceVoid Finish();

			AsyncNetworkSurfaceLoader *workObj;
			ui::Widget *target;
			string url;
			graph::Surface **loadedSurface;
		};

		Job *item;
	};

	SceVoid Init();

	SceUInt32 GetHash(const char *name);

	rco::Element CreateElement(const char *name);

	wchar_t *GetStringWithNum(const char *name, SceUInt32 num);
	wchar_t *GetString(const char *name);
	wchar_t *GetString(SceUInt32 hash);

	graph::Surface *GetTexture(const char *name);
	graph::Surface *GetTexture(SceUInt32 hash);

	ui::Widget *GetChild(ui::Widget *parent, const char *id);
	ui::Widget *GetChild(ui::Widget *parent, SceUInt32 hash);

	SceVoid SetPowerTickTask(PowerTick mode);

	SceVoid ConvertSecondsToString(string& string, SceUInt64 seconds, SceBool needSeparator);

	SceVoid Lock(SceUInt32 flag);

	SceVoid Unlock(SceUInt32 flag);

	SceVoid Wait(SceUInt32 flag);

	job::JobQueue *GetJobQueue();

	SceBool LoadNetworkSurface(const char *url, graph::Surface **ret);
};


#endif

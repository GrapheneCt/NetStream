#ifndef _UTILS_H_
#define _UTILS_H_

#include <kernel.h>
#include <paf.h>
#include <paf_file_ext.h>

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

	void Init();

	uint32_t GetHash(const char *name);

	wchar_t *GetStringWithNum(const char *name, uint32_t num);

	void SetPowerTickTask(PowerTick mode);

	void ConvertSecondsToString(string& string, uint64_t seconds, bool needSeparator);

	void Lock(uint32_t flag);

	void Unlock(uint32_t flag);

	void Wait(uint32_t flag);

	void SetDisplayResolution(uint32_t resolution);

	job::JobQueue *GetJobQueue();

	CurlFile::Share *GetShare();

	uint32_t SafememGetSettingsSize();

	void SafememWrite(string const& str, uint32_t offset = 0);

	string SafememRead(uint32_t offset = 0);

	typedef void(*TimeoutFunc)(void *userdata1, void *userdata2);

	class TimeoutListener : public TimerListener
	{
	public:

		TimeoutListener(Timer *t, TimeoutFunc _func) : TimerListener(t, NULL), m_func(_func)
		{

		}

		void OnFinish(void *data1, void *data2)
		{
			if (m_func)
			{
				m_func(data1, data2);
			}
			TimerListenerList::s_default_list->Unregister(this);
			delete this;
		}

	private:

		TimeoutFunc m_func;
	};

	typedef TimeoutListener* TimeoutID;

	TimeoutID SetTimeout(TimeoutFunc func, float timeoutMs, void *userdata1 = NULL, void *userdata2 = NULL);

	void ClearTimeout(TimeoutID id);
};


#endif

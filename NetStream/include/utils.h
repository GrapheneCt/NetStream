#ifndef _UTILS_H_
#define _UTILS_H_

#include <kernel.h>
#include <paf.h>
#include <paf_file_ext.h>

#include "ftube.h"

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

	bool IsTaihenLoaded();

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
			TimerListenerList::DefaultList()->Unregister(this);
			delete this;
		}

	private:

		TimeoutFunc m_func;
	};

	typedef TimeoutListener* TimeoutID;

	TimeoutID SetTimeout(TimeoutFunc func, float timeoutMs, void *userdata1 = NULL, void *userdata2 = NULL);

	void ClearTimeout(TimeoutID id);

	class CurlDownloadContext
	{
	public:

		CurlDownloadContext()
		{
			m_buf = NULL;
			m_pos = 0;
		}

		~CurlDownloadContext() {};

		void *m_buf;
		uint32_t m_pos;
	};

	void SetGlobalProxy(const char *proxy);

	const char *GetGlobalProxy();

	bool DoGETRequest(const char *curl, void **ppRespBuf, size_t *pRespBufSize, const char **ppHeaders, uint32_t headerNum,
		int32_t *pRespCode, void*(*liballoc)(size_t), void(*libdealloc)(void *), void*(*librealloc)(void *, size_t));

	bool DoPOSTRequest(const char *curl, void *pBuf, size_t bufSize, const char **ppHeaders, uint32_t headerNum, void **ppRespBuf,
		size_t *pRespBufSize, int32_t *pRespCode, void*(*liballoc)(size_t), void(*libdealloc)(void *), void*(*librealloc)(void *, size_t));

	void SetRequestShare(CURLSH *share);

	void ResolutionToQualityString(wstring& res, uint32_t width, uint32_t height, bool selected = false);

	void AudioParamsToQualityString(wstring& res, uint32_t ch, uint32_t srate, bool selected = false);

	bool IsVideoSupported(uint32_t width, uint32_t height);

	bool IsAudioSupported(uint32_t ch, uint32_t srate);

	bool IsLocalPath(const char *path);
};


#endif

#ifndef _SRT_SUBS_H_
#define _SRT_SUBS_H_

#include <kernel.h>
#include <paf.h>

#include "subs/subs_generic.h"

using namespace paf;

class SRTSubtitles : public GenericSubtitles
{
public:

	class Option : public GenericSubtitles::Option
	{
	public:

		Option()
		{
			subtitlesType = SubtitlesType_SRT;
		}

		SubtitlesType subtitlesType;
	};

	SRTSubtitles(const char *videoPath, Option *opt);

	~SRTSubtitles() override;

	void InitAsync() override;

	static SupportType IsSupported(const char *videoPath);

private:

	enum SrtPosition
	{
		SRT_POSITION_BOTTOM_DEFAULT,
		SRT_POSITION_BOTTOM_LEFT,
		SRT_POSITION_BOTTOM_CENTER,
		SRT_POSITION_BOTTOM_RIGHT,
		SRT_POSITION_MIDDLE_LEFT,
		SRT_POSITION_MIDDLE_CENTER,
		SRT_POSITION_MIDDLE_RIGHT,
		SRT_POSITION_TOP_LEFT,
		SRT_POSITION_TOP_CENTER,
		SRT_POSITION_TOP_RIGHT
	};

	class BootJob : public job::JobItem
	{
	public:

		BootJob(SRTSubtitles *parent) : job::JobItem("SRTSubtitles::BootJob", NULL), m_parent(parent)
		{

		}

		~BootJob() {}

		int32_t Run()
		{
			m_parent->OnBootJob();
			return SCE_PAF_OK;
		}

		void Finish(int32_t result) {}

	private:

		SRTSubtitles *m_parent;
	};

	class SrtMemFile
	{
	public:

		SrtMemFile(common::SharedPtr<File> file)
		{
			m_pos = 0;
			m_size = file->GetFileSize();
			m_mem = new char[m_size];
			if (!m_mem)
			{
				m_size = 0;
				return;
			}
			file->Read(m_mem, m_size);
		}

		~SrtMemFile()
		{
			delete m_mem;
		}

		int32_t Fgetc()
		{
			if (m_pos >= m_size)
			{
				return EOF;
			}

			int32_t ret = static_cast<int32_t>(m_mem[m_pos]);
			m_pos++;

			return ret;
		}

		char *Fgets(char *s, int32_t n)
		{
			int32_t i, c;
			if (n <= 0) return s;
			for (i = 0; i < n - 1; i++)
			{
				c = Fgetc();
				if (c == EOF)
				{
					break;
				}
				s[i] = c;
				if (c == '\n')
				{
					++i;
					break;
				}
			}
			if (c == EOF)
			{
				if (i != 0)  s[i] = '\0';
				return NULL;
			}
			s[i] = '\0';
			return s;
		}

	private:

		char *m_mem;
		uint32_t m_size;
		uint32_t m_pos;
		char m_dummy;
	};

	void OnBootJob();

	void ParseSrt(common::SharedPtr<File> file);

	static uint64_t GetMilliseconds(const char *time);

	static int32_t GetSize(const char *dialog);

	static uint32_t GetColor(const char *dialog);

	static SrtPosition GetPosition(const char *dialog);

	static char *CleanSrtDialog(char *dialog);

	static math::v4 ConvertColor(uint32_t color);

	static const char *s_srtDelim;
};

#endif

#include <kernel.h>
#include <paf.h>
#include <libdbg.h>

#include "common.h"
#include "utils.h"
#include "event.h"
#include "subs/subs_srt.h"

#undef SCE_DBG_LOG_COMPONENT
#define SCE_DBG_LOG_COMPONENT "[SRTSubtitles]"

#define SUB_SCALE_BASE	(26.0f)
#define SUB_MARGIN_H	(0.0f)
#define SUB_MARGIN_V	(12.0f)
#define RGBA8(r,g,b,a) ((((a)&0xFF)<<24) | (((b)&0xFF)<<16) | (((g)&0xFF)<<8) | (((r)&0xFF)<<0))

using namespace paf;

const char *SRTSubtitles::s_srtDelim = " --> "; // Most if not all SRT should have this for time

// http://www.w3.org/TR/css3-color/
typedef struct NamedColor { const char *const name; const unsigned int color; } NamedColor;
const NamedColor s_namedColors[] = {
	{ "transparent", RGBA8(0, 0, 0, 0) }, { "aliceblue", RGBA8(240, 248, 255, 255) },
	{ "antiquewhite", RGBA8(250, 235, 215, 255) }, { "aqua", RGBA8(0, 255, 255, 255) },
	{ "aquamarine", RGBA8(127, 255, 212, 255) }, { "azure", RGBA8(240, 255, 255, 255) },
	{ "beige", RGBA8(245, 245, 220, 255) }, { "bisque", RGBA8(255, 228, 196, 255) },
	{ "black", RGBA8(0, 0, 0, 1) }, { "blanchedalmond", RGBA8(255, 235, 205, 255) },
	{ "blue", RGBA8(0, 0, 255, 255) }, { "blueviolet", RGBA8(138, 43, 226, 255) },
	{ "brown", RGBA8(165, 42, 42, 255) }, { "burlywood", RGBA8(222, 184, 135, 255) },
	{ "cadetblue", RGBA8(95, 158, 160, 255) }, { "chartreuse", RGBA8(127, 255, 0, 255) },
	{ "chocolate", RGBA8(210, 105, 30, 255) }, { "coral", RGBA8(255, 127, 80, 255) },
	{ "cornflowerblue", RGBA8(100, 149, 237, 255) }, { "cornsilk", RGBA8(255, 248, 220, 255) },
	{ "crimson", RGBA8(220, 20, 60, 255) }, { "cyan", RGBA8(0, 255, 255, 255) },
	{ "darkblue", RGBA8(0, 0, 139, 255) }, { "darkcyan", RGBA8(0, 139, 139, 255) },
	{ "darkgoldenrod", RGBA8(184, 134, 11, 255) }, { "darkgray", RGBA8(169, 169, 169, 255) },
	{ "darkgreen", RGBA8(0, 100, 0, 255) }, { "darkgrey", RGBA8(169, 169, 169, 255) },
	{ "darkkhaki", RGBA8(189, 183, 107, 255) }, { "darkmagenta", RGBA8(139, 0, 139, 255) },
	{ "darkolivegreen", RGBA8(85, 107, 47, 255) }, { "darkorange", RGBA8(255, 140, 0, 255) },
	{ "darkorchid", RGBA8(153, 50, 204, 255) }, { "darkred", RGBA8(139, 0, 0, 255) },
	{ "darksalmon", RGBA8(233, 150, 122, 255) }, { "darkseagreen", RGBA8(143, 188, 143, 255) },
	{ "darkslateblue", RGBA8(72, 61, 139, 255) }, { "darkslategray", RGBA8(47, 79, 79, 255) },
	{ "darkslategrey", RGBA8(47, 79, 79, 255) }, { "darkturquoise", RGBA8(0, 206, 209, 255) },
	{ "darkviolet", RGBA8(148, 0, 211, 255) }, { "deeppink", RGBA8(255, 20, 147, 255) },
	{ "deepskyblue", RGBA8(0, 191, 255, 255) }, { "dimgray", RGBA8(105, 105, 105, 255) },
	{ "dimgrey", RGBA8(105, 105, 105, 255) }, { "dodgerblue", RGBA8(30, 144, 255, 255) },
	{ "firebrick", RGBA8(178, 34, 34, 255) }, { "floralwhite", RGBA8(255, 250, 240, 255) },
	{ "forestgreen", RGBA8(34, 139, 34, 255) }, { "fuchsia", RGBA8(255, 0, 255, 255) },
	{ "gainsboro", RGBA8(220, 220, 220, 255) }, { "ghostwhite", RGBA8(248, 248, 255, 255) },
	{ "gold", RGBA8(255, 215, 0, 255) }, { "goldenrod", RGBA8(218, 165, 32, 255) },
	{ "gray", RGBA8(128, 128, 128, 255) }, { "green", RGBA8(0, 128, 0, 255) },
	{ "greenyellow", RGBA8(173, 255, 47, 255) }, { "grey", RGBA8(128, 128, 128, 255) },
	{ "honeydew", RGBA8(240, 255, 240, 255) }, { "hotpink", RGBA8(255, 105, 180, 255) },
	{ "indianred", RGBA8(205, 92, 92, 255) }, { "indigo", RGBA8(75, 0, 130, 255) },
	{ "ivory", RGBA8(255, 255, 240, 255) }, { "khaki", RGBA8(240, 230, 140, 255) },
	{ "lavender", RGBA8(230, 230, 250, 255) }, { "lavenderblush", RGBA8(255, 240, 245, 255) },
	{ "lawngreen", RGBA8(124, 252, 0, 255) }, { "lemonchiffon", RGBA8(255, 250, 205, 255) },
	{ "lightblue", RGBA8(173, 216, 230, 255) }, { "lightcoral", RGBA8(240, 128, 128, 255) },
	{ "lightcyan", RGBA8(224, 255, 255, 255) }, { "lightgoldenrodyellow", RGBA8(250, 250, 210, 255) },
	{ "lightgray", RGBA8(211, 211, 211, 255) }, { "lightgreen", RGBA8(144, 238, 144, 255) },
	{ "lightgrey", RGBA8(211, 211, 211, 255) }, { "lightpink", RGBA8(255, 182, 193, 255) },
	{ "lightsalmon", RGBA8(255, 160, 122, 255) }, { "lightseagreen",RGBA8(32, 178, 170, 255) },
	{ "lightskyblue", RGBA8(135, 206, 250, 255) }, { "lightslategray",RGBA8(119, 136, 153, 255) },
	{ "lightslategrey", RGBA8(119, 136, 153, 255) }, { "lightsteelblue", RGBA8(176, 196, 222, 255) },
	{ "lightyellow", RGBA8(255, 255, 224, 255) }, { "lime", RGBA8(0, 255, 0, 255) },
	{ "limegreen", RGBA8(50, 205, 50, 255) }, { "linen", RGBA8(250, 240, 230, 255) },
	{ "magenta", RGBA8(255, 0, 255, 255) }, { "maroon", RGBA8(128, 0, 0, 255) },
	{ "mediumaquamarine", RGBA8(102, 205, 170, 255) }, { "mediumblue", RGBA8(0, 0, 205, 255) },
	{ "mediumorchid", RGBA8(186, 85, 211, 255) }, { "mediumpurple", RGBA8(147, 112, 219, 255) },
	{ "mediumseagreen", RGBA8(60, 179, 113, 255) }, { "mediumslateblue", RGBA8(123, 104, 238, 255) },
	{ "mediumspringgreen", RGBA8(0, 250, 154, 255) }, { "mediumturquoise", RGBA8(72, 209, 204, 255) },
	{ "mediumvioletred", RGBA8(199, 21, 133, 255) }, { "midnightblue", RGBA8(25, 25, 112, 255) },
	{ "mintcream", RGBA8(245, 255, 250, 255) }, { "mistyrose", RGBA8(255, 228, 225, 255) },
	{ "moccasin", RGBA8(255, 228, 181, 255) }, { "navajowhite", RGBA8(255, 222, 173, 255) },
	{ "navy", RGBA8(0, 0, 128, 255) }, { "oldlace", RGBA8(253, 245, 230, 255) },
	{ "olive", RGBA8(128, 128, 0, 255) }, { "olivedrab", RGBA8(107, 142, 35, 255) },
	{ "orange", RGBA8(255, 165, 0, 255) }, { "orangered",RGBA8(255, 69, 0, 255) },
	{ "orchid", RGBA8(218, 112, 214, 255) }, { "palegoldenrod", RGBA8(238, 232, 170, 255) },
	{ "palegreen", RGBA8(152, 251, 152, 255) }, { "paleturquoise", RGBA8(175, 238, 238, 255) },
	{ "palevioletred", RGBA8(219, 112, 147, 255) }, { "papayawhip", RGBA8(255, 239, 213, 255) },
	{ "peachpuff", RGBA8(255, 218, 185, 255) }, { "peru", RGBA8(205, 133, 63, 255) },
	{ "pink", RGBA8(255, 192, 203, 255) }, { "plum", RGBA8(221, 160, 221, 255) },
	{ "powderblue", RGBA8(176, 224, 230, 255) }, { "purple", RGBA8(128, 0, 128, 255) },
	{ "red", RGBA8(255, 0, 0, 1) }, { "rosybrown", RGBA8(188, 143, 143, 255) },
	{ "royalblue", RGBA8(65, 105, 225, 255) }, { "saddlebrown", RGBA8(139, 69, 19, 255) },
	{ "salmon", RGBA8(250, 128, 114, 255) }, { "sandybrown", RGBA8(244, 164, 96, 255) },
	{ "seagreen", RGBA8(46, 139, 87, 255) }, { "seashell", RGBA8(255, 245, 238, 255) },
	{ "sienna", RGBA8(160, 82, 45, 255) }, { "silver", RGBA8(192, 192, 192, 255) },
	{ "skyblue", RGBA8(135, 206, 235, 255) }, { "slateblue", RGBA8(106, 90, 205, 255) },
	{ "slategray", RGBA8(112, 128, 144, 255) }, { "slategrey", RGBA8(112, 128, 144, 255) },
	{ "snow", RGBA8(255, 250, 250, 255) }, { "springgreen",RGBA8(0, 255, 127, 255) },
	{ "steelblue", RGBA8(70, 130, 180, 255) }, { "tan",RGBA8(210, 180, 140, 255) },
	{ "teal", RGBA8(0, 128, 128, 255) }, { "thistle",RGBA8(216, 191, 216, 255) },
	{ "tomato", RGBA8(255, 99, 71, 255) }, { "turquoise", RGBA8(64, 224, 208, 255) },
	{ "violet", RGBA8(238, 130, 238, 255) }, { "wheat", RGBA8(245, 222, 179, 255) },
	{ "white", RGBA8(255, 255, 255, 255) }, { "whitesmoke", RGBA8(245, 245, 245, 255) },
	{ "yellow", RGBA8(255, 255, 0, 255) }, { "yellowgreen", RGBA8(154, 205, 50, 255) }
};

SRTSubtitles::SRTSubtitles(const char *videoPath, Option *opt)
{
	int32_t ret;
	m_path = common::StripFilename(videoPath, "PF");
	m_path += ".srt";
}

SRTSubtitles::~SRTSubtitles()
{

}

void SRTSubtitles::OnBootJob()
{
	int32_t ret;
	common::SharedPtr<File> file;

	SCE_DBG_LOG_INFO("[OnBootJob] Opening %s", m_path.c_str());

	if (utils::IsLocalPath(m_path.c_str()))
	{
		file = LocalFile::Open(m_path.c_str(), File::RDONLY, File::RU, &ret);
	}
	else
	{
		file = CurlFile::Open(m_path.c_str(), File::RDONLY, File::RU, &ret, NULL, utils::GetGlobalProxy());
	}
	if (ret != SCE_PAF_OK)
	{
		return;
	}

	ParseSrt(file);

	SCE_DBG_LOG_INFO("[OnBootJob] ParseSrt() ok");
}

void SRTSubtitles::ParseSrt(common::SharedPtr<File> file)
{
	SrtMemFile memfile(file);

	uint32_t idx = 0;
	char buf[SCE_KERNEL_1KiB]; // line buffer
	char *sbuf = new char[SCE_KERNEL_2KiB]; // string buffer
	if (!sbuf)
	{
		return;
	}
	while (memfile.Fgets(buf, sizeof(buf))) // New line
	{
		char *timeLine = sce_paf_strstr(buf, s_srtDelim); // Check to see if current line contains timestamp data
		if (timeLine)
		{
			char *start = sce_paf_strtok(buf, s_srtDelim);
			//sceClibPrintf("Start Text: %s\n", start);
			char *end = sce_paf_strtok(NULL, s_srtDelim);
			//sceClibPrintf("End Text: %s\n", end);

			SubtitleEntry entry;

			entry.startTime = GetMilliseconds(start);
			entry.endTime = GetMilliseconds(end);

			//sceClibPrintf("Start Time %llu\nEnd Time %llu\nStart Text: %s\nEnd Text: %s\n", entry.startTime, entry.endTime, start, end);

			uint32_t fullLen = 0;
			sbuf[0] = 0;
			while (memfile.Fgets(buf, sizeof(buf)))
			{
				uint32_t len = sce_paf_strlen(buf);
				if (len == 1)
				{
					break;
				}
				fullLen += len;
				if (fullLen >= SCE_KERNEL_2KiB)
				{
					break;
				}
				sce_paf_strcat(sbuf, buf);
			}

			int32_t size = GetSize(sbuf);
			if (size != -1)
			{
				entry.charSizeScale = (static_cast<float>(size) / SUB_SCALE_BASE);
			}

			uint32_t color = GetColor(sbuf);
			entry.colorPacked = color;
			entry.color = ConvertColor(color);

			switch (GetPosition(sbuf))
			{
			case SRT_POSITION_BOTTOM_LEFT:
				entry.anchorX = ui::Widget::ANCHOR_LEFT;
				entry.anchorY = ui::Widget::ANCHOR_BOTTOM;
				entry.alignX = ui::Widget::ALIGN_LEFT;
				entry.alignY = ui::Widget::ALIGN_BOTTOM;
				entry.position.set_x(SUB_MARGIN_H);
				entry.position.set_y(SUB_MARGIN_V);
				break;
			case SRT_POSITION_BOTTOM_CENTER:
				entry.anchorY = ui::Widget::ANCHOR_BOTTOM;
				entry.alignY = ui::Widget::ALIGN_BOTTOM;
				entry.position.set_y(SUB_MARGIN_V);
				break;
			case SRT_POSITION_BOTTOM_RIGHT:
				entry.anchorX = ui::Widget::ANCHOR_RIGHT;
				entry.anchorY = ui::Widget::ANCHOR_BOTTOM;
				entry.alignX = ui::Widget::ALIGN_RIGHT;
				entry.alignY = ui::Widget::ALIGN_BOTTOM;
				entry.position.set_x(SUB_MARGIN_H);
				entry.position.set_y(SUB_MARGIN_V);
				break;
			case SRT_POSITION_MIDDLE_LEFT:
				entry.anchorX = ui::Widget::ANCHOR_LEFT;
				entry.alignX = ui::Widget::ALIGN_LEFT;
				entry.position.set_x(SUB_MARGIN_H);
				break;
			case SRT_POSITION_MIDDLE_CENTER:
				break;
			case SRT_POSITION_MIDDLE_RIGHT:
				entry.anchorX = ui::Widget::ANCHOR_RIGHT;
				entry.alignX = ui::Widget::ALIGN_RIGHT;
				entry.position.set_x(SUB_MARGIN_H);
				break;
			case SRT_POSITION_TOP_LEFT:
				entry.anchorX = ui::Widget::ANCHOR_LEFT;
				entry.anchorY = ui::Widget::ANCHOR_TOP;
				entry.alignX = ui::Widget::ALIGN_LEFT;
				entry.alignY = ui::Widget::ALIGN_TOP;
				entry.position.set_x(SUB_MARGIN_H);
				entry.position.set_y(-SUB_MARGIN_V);
				break;
			case SRT_POSITION_TOP_CENTER:
				entry.anchorY = ui::Widget::ANCHOR_TOP;
				entry.alignY = ui::Widget::ALIGN_TOP;
				entry.position.set_y(-SUB_MARGIN_V);
				break;
			case SRT_POSITION_TOP_RIGHT:
				entry.anchorX = ui::Widget::ANCHOR_RIGHT;
				entry.anchorY = ui::Widget::ANCHOR_TOP;
				entry.alignX = ui::Widget::ALIGN_RIGHT;
				entry.alignY = ui::Widget::ALIGN_TOP;
				entry.position.set_x(SUB_MARGIN_H);
				entry.position.set_y(-SUB_MARGIN_V);
				break;
			}

			common::Utf8ToUtf16(CleanSrtDialog(sbuf), &entry.string);

			m_entries.push_back(entry);
		}
	}

	delete sbuf;
}

uint64_t SRTSubtitles::GetMilliseconds(const char *time)
{
	int32_t hours, mins, seconds, milliseconds;

	char chunk[4] = { 0 };
	chunk[0] = time[0];
	chunk[1] = time[1];
	hours = sce_paf_atoi(chunk);
	chunk[0] = time[3];
	chunk[1] = time[4];
	mins = sce_paf_atoi(chunk);
	chunk[0] = time[6];
	chunk[1] = time[7];
	seconds = sce_paf_atoi(chunk);
	chunk[0] = time[9];
	chunk[1] = time[10];
	chunk[2] = time[11];
	milliseconds = sce_paf_atoi(chunk);

	//sceClibPrintf("Hours %d\nMins %d\nSecs %d\nMilliseconds %d\n", hours, mins, seconds, milliseconds);

	return hours * 3600000 + mins * 60000 + seconds * 1000 + milliseconds;
}

int32_t SRTSubtitles::GetSize(const char *dialog)
{
	char *sz = sce_paf_strstr(dialog, "size=\"");
	if (sz)
	{
		sz += 6;
		return (int32_t)sce_paf_strtol(sz, NULL, 10);
	}

	return -1;
}

uint32_t SRTSubtitles::GetColor(const char *dialog)
{
	char *col = sce_paf_strstr(dialog, "color=\"#");
	if (col)
	{
		col += 8;
		unsigned int ret = (unsigned int)sce_paf_strtoul(col, NULL, 16);
		ret |= (0xFF << 24);
		return ret;
	}
	else
	{
		col = sce_paf_strstr(dialog, "color=\"");
		if (col)
		{
			col += 7;
			char collen = sce_paf_strchr(col, '"') - col;
			char tmp[100];
			sce_paf_memset(tmp, 0, sizeof(tmp));
			sce_paf_strncpy(tmp, col, collen);
			for (int i = 0; i < sizeof(s_namedColors) / sizeof(NamedColor); i++)
			{
				if (!sce_paf_strcmp(tmp, s_namedColors[i].name))
				{
					return s_namedColors[i].color;
				}
			}
		}
	}

	return RGBA8(255, 255, 255, 255);
}

SRTSubtitles::SrtPosition SRTSubtitles::GetPosition(const char *dialog)
{
	char *pos = sce_paf_strstr(dialog, "{\\an");
	if (pos)
	{
		pos += 4;
		return (SrtPosition)sce_paf_strtol(pos, NULL, 10);
	}

	return SRT_POSITION_BOTTOM_CENTER;
}

char *SRTSubtitles::CleanSrtDialog(char *dialog)
{
	char *tempChar = sce_paf_strstr(dialog, "</");
	char *out = NULL;
	if (tempChar) // Found XML data
	{
		*tempChar = 0;
		char *strStart = sce_paf_strrchr(dialog, '>') + 1;
		char *pos = sce_paf_strstr(strStart, "{\\an");
		if (pos)
		{
			out = pos + 6;
		}
		else
		{
			out = strStart;
		}
	}

	if (!out)
	{
		out = dialog;
	}

	for (int i = 0; i <= sce_paf_strlen(out); i++)
	{
		if (out[i] == 0x0D)
		{
			out[i] = ' ';
		}
	}

	return out;
}

math::v4 SRTSubtitles::ConvertColor(uint32_t color)
{
	math::v4 ret;

	ret.set_x(((color >> 0) & 0xFF) / 255.0f);
	ret.set_y(((color >> 8) & 0xFF) / 255.0f);
	ret.set_z(((color >> 16) & 0xFF) / 255.0f);
	ret.set_w(((color >> 24) & 0xFF) / 255.0f);

	return ret;
}

void SRTSubtitles::InitAsync()
{
	BootJob *job = new BootJob(this);
	common::SharedPtr<job::JobItem> itemParam(job);
	utils::GetJobQueue()->Enqueue(itemParam);
}

GenericSubtitles::SupportType SRTSubtitles::IsSupported(const char *videoPath)
{
	string subPath = common::StripFilename(videoPath, "PF");
	subPath += ".srt";

	SCE_DBG_LOG_INFO("[IsSupported] Probe subs at: %s", subPath.c_str());

	if (utils::IsLocalPath(videoPath))
	{
		if (LocalFile::Exists(subPath.c_str()))
		{
			SCE_DBG_LOG_INFO("[IsSupported] Detected local subs");
			return GenericSubtitles::SupportType_Supported;
		}
	}
	else
	{
		CurlFile::CurlFileStat stat;
		if (CurlFile::Getstat(subPath.c_str(), &stat, NULL, utils::GetGlobalProxy()) == SCE_PAF_OK)
		{
			SCE_DBG_LOG_INFO("[IsSupported] Detected remote subs");
			return GenericSubtitles::SupportType_Supported;
		}
	}

	return GenericSubtitles::SupportType_NotSupported;
}
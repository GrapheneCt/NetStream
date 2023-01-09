#pragma once

#include <kernel.h>

SCE_CDECL_BEGIN

#ifdef LOOTKIT_BUILD
#define LTK_EXPORT __declspec(dllexport)
#else
#define LTK_EXPORT
#endif

typedef enum LtkItemType
{
	LTK_ITEM_TYPE_CATEGORY,
	LTK_ITEM_TYPE_CHANNEL,

	INV_ITEM_TYPE_MAX
} LtkItemType;

typedef enum LtkSearchType
{
	LTK_SEARCH_TYPE_CATEGORY,
	LTK_SEARCH_TYPE_CHANNEL,

	LTK_SEARCH_TYPE_MAX
} LtkSearchType;

typedef enum LtkHlsQuality
{
	LTK_HLS_QUALITY_160P,
	LTK_HLS_QUALITY_360P,
	LTK_HLS_QUALITY_480P,
	LTK_HLS_QUALITY_720P,

	LTK_HLS_QUALITY_MAX
} InvHlsQuality;

typedef enum LtkSearchDirection
{
	LTK_SEARCH_DIR_AFTER,
	LTK_SEARCH_DIR_BEFORE,

	LTK_SEARCH_DIR_MAX
} LtkSearchDirection;

typedef enum LtkVideoType
{
	LTK_VIDEO_TYPE_VOD,
	LTK_VIDEO_TYPE_HIGHLIGHT,
	LTK_VIDEO_TYPE_UPLOAD,
	LTK_VIDEO_TYPE_ALL,

	LTK_VIDEO_TYPE_MAX
} LtkVideoType;

typedef struct LtkItemCategory
{
	const char *id;
	const char *name;
	const char *thmbUrl;
	const char *pagination;
} LtkItemCategory;

typedef struct LtkItemChannel
{
	const char *id;
	const char *login;
	const char *name;
	const char *title;
	const char *categoryName;
	const char *thmbUrl;
	const char *startedAt;
	SceBool isLive;
	const char *pagination;
} LtkItemChannel;

typedef struct LtkItemVideo
{
	ScePVoid reserved;
	LtkVideoType type;
	const char *id;
	const char *title;
	const char *description;
	const char *duration;
	const char *date;
	const char *thmbUrl;
	const char *pagination;
} LtkItemVideo;

typedef struct LtkItem
{
	ScePVoid reserved;
	LtkItemType type;
	union
	{
		LtkItemCategory *categoryItem;
		LtkItemChannel *channelItem;
	};
} LtkItem;

typedef ScePVoid(*LtkAllocator)(size_t size);

typedef SceVoid(*LtkDeallocator)(ScePVoid ptr);

typedef SceBool(*LtkDownloader)(const char *url, ScePVoid *ppBuf, SceSize *pBufSize, ScePVoid postData, SceSize postDataSize);

LTK_EXPORT SceInt32 ltkInit(LtkAllocator allocator, LtkDeallocator deallocator, const char *lootUrl);

LTK_EXPORT SceInt32 ltkTerm();

LTK_EXPORT SceInt32 ltkParseSearch(const char *request, const char *pagination, LtkSearchType type, LtkSearchDirection dir, LtkItem **firstItem);

LTK_EXPORT SceInt32 ltkParseVideo(LtkItemChannel *item, LtkVideoType type, const char *pagination, LtkSearchDirection dir, LtkItemVideo **videos);

LTK_EXPORT SceInt32 ltkGetHlsUrl(LtkItemChannel *item, LtkHlsQuality quality, char *hlsUrl, SceInt32 sizeOfUrl);

LTK_EXPORT SceInt32 ltkGetVideoUrl(LtkItemVideo *item, LtkHlsQuality quality, char *videoUrl, SceInt32 sizeOfUrl);

LTK_EXPORT SceInt32 ltkCleanupSearch(LtkItem *items);

LTK_EXPORT SceInt32 ltkCleanupVideo(LtkItemVideo *items);

SCE_CDECL_END

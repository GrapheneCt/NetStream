#pragma once

#include <stdint.h>
#include <stddef.h>

#if defined(_LANGUAGE_C_PLUS_PLUS)||defined(__cplusplus)||defined(c_plusplus)
extern "C" {
#endif

#ifdef FT_BUILD
#define FT_EXPORT __declspec(dllexport)
#else
#define FT_EXPORT
#endif

#define FT_OK			(0)
#define FT_EBADF		(0x80010009)
#define FT_ENOMEM		(0x8001000C)
#define FT_EFAULT		(0x8001000E)
#define FT_EINVAL		(0x80010016)
#define FT_EUNSUP		(0x80010030)
#define FT_EALREADY		(0x80010078)
#define FT_EFORMAT		(0x8001002F)

typedef void*(*FTAllocator)(size_t size);

typedef void(*FTDeallocator)(void *ptr);

typedef void*(*FTReallocator)(void *ptr, size_t size);

typedef bool(*FTGetter)(const char *url, void **ppBuf, size_t *pBufSize, const char **ppHeaders, uint32_t headerNum,
	int32_t *pRespCode, FTAllocator ftalloc, FTDeallocator ftdealloc, FTReallocator ftrealloc);

typedef bool(*FTPoster)(const char *url, void *pBuf, size_t bufSize, const char **ppHeaders, uint32_t headerNum, void **ppRespBuf,
	size_t *pRespBufSize, int32_t *pRespCode, FTAllocator ftalloc, FTDeallocator ftdealloc, FTReallocator ftrealloc);

typedef enum FTItemType
{
	FT_ITEM_TYPE_VIDEO,
	FT_ITEM_TYPE_PLAYLIST,
	FT_ITEM_TYPE_CHANNEL,
	FT_ITEM_TYPE_COMMENT,
	FT_ITEM_TYPE_ALL,

	FT_ITEM_TYPE_MAX
} FTItemType;

typedef enum FTSort
{
	FT_SORT_RELEVANCE,
	FT_SORT_RATING,
	FT_SORT_DATE,
	FT_SORT_VIEW_COUNT,

	FT_SORT_MAX
} FTSort;

typedef enum FTDate
{
	FT_DATE_HOUR,
	FT_DATE_TODAY,
	FT_DATE_WEEK,
	FT_DATE_MONTH,
	FT_DATE_YEAR,
	FT_DATE_ALL,

	FT_DATE_MAX
} FTDate;

typedef enum FTFeature
{
	FT_FEATURE_LIVE,
	FT_FEATURE_NONE,

	FT_FEATURE_MAX
} FTFeature;

typedef enum FTDuration
{
	FT_DURATION_SHORT,
	FT_DURATION_MEDIUM,
	FT_DURATION_LONG,
	FT_DURATION_ALL,

	FT_DURATION_MAX
} FTDuration;

typedef enum FTVODVideoQuality
{
	FT_VIDEO_VOD_QUALITY_NONE = 0,
	FT_VIDEO_VOD_QUALITY_144P = 1,
	FT_VIDEO_VOD_QUALITY_240P = 2,
	FT_VIDEO_VOD_QUALITY_360P = 4,
	FT_VIDEO_VOD_QUALITY_480P = 8,
	FT_VIDEO_VOD_QUALITY_720P = 16,
	FT_VIDEO_VOD_QUALITY_ALL = 0xFFFFFFFF
} FTVODQuality;

typedef enum FTVODAudioQuality
{
	FT_AUDIO_VOD_QUALITY_NONE = 0,
	FT_AUDIO_VOD_QUALITY_LOW = 1,
	FT_AUDIO_VOD_QUALITY_MEDIUM = 2,
	FT_AUDIO_VOD_QUALITY_ALL = 0xFFFFFFFF
} FTVODAudioQuality;

typedef enum FTRegion
{
	FT_REGION_EN_US,

	FT_REGION_MAX
} FTRegion;

typedef enum FTCommentSort
{
	FT_COMMENT_SORT_TOP,
	FT_COMMENT_SORT_NEW,

	FT_COMMENT_SORT_MAX
} FTCommentSort;

typedef struct FTItemVideo
{
	const char *title;
	const char *id;
	const char *author;
	const char *authorId;
	const char *published; // Only with ftParseSearch
	const char *thmbUrl;
	const char *thmbUrlHq;
	uint32_t lengthSec;
	bool isLive;
	bool isShort;

	// Only with ftParseVideo
	const char *dashManifest;
	const char *audioOnlyUrl;
	const char *compositeVideoUrl;
	uint32_t videoRepCount;
	uint32_t audioRepCount;
	const char *description;
	uint32_t viewCount;
	const char *dashManifestUrl;
	const char *hlsManifestUrl;
} FTItemVideo;

typedef struct FTItemPlaylist
{
	const char *title;
	const char *id;
	const char *author;
	const char *authorId;
	const char *published;
	const char *thmbUrl;
	const char *thmbUrlHq;
	uint32_t videoCount;
} FTItemPlaylist;

typedef struct FTItemChannel
{
	const char *author;
	const char *authorId;
	const char *thmbUrl;
	const char *subCount;
} FTItemChannel;

typedef struct FTItemComment
{
	char reserved[sizeof(char *) + sizeof(uint32_t)];
	const char *author;
	const char *authorId;
	const char *published;
	const char *thmbUrl;
	const char *content;
	const char *voteCount;
	uint32_t replyCount;
	bool isOwner;
	bool likedByOwner;
	const char *replyContinuation;
} FTItemComment;

typedef struct FTItem
{
	FTItemType type;
	union
	{
		FTItemVideo *videoItem;
		FTItemPlaylist *playlistItem;
		FTItemChannel *channelItem;
		FTItemComment *commentItem;
	};
	FTItem *next;
} FTItem;

FT_EXPORT int32_t ftSetRegion(FTRegion region);

FT_EXPORT int32_t ftInit(FTAllocator allocator, FTDeallocator deallocator, FTReallocator reallocator, FTGetter getter, FTPoster poster);

FT_EXPORT int32_t ftTerm();

FT_EXPORT int32_t ftParseSearch(void **ppContext, const char *request, const char *continuation, FTItemType type, FTSort sort, FTDate date, FTFeature feature, FTDuration dur, FTItem **firstItem, const char **continuationToken);

FT_EXPORT int32_t ftParseVideo(void **ppContext, const char *videoId, uint32_t videoQuality, uint32_t audioQuality, FTItem **item);

FT_EXPORT int32_t ftParseComments(void **ppContext, const char *videoId, const char *continuation, FTCommentSort sort, FTItem **item, const char **continuationToken);

FT_EXPORT int32_t ftParseLiveComments(void **ppContext, const char *videoId, uint32_t maxNum, FTItem **firstItem);

FT_EXPORT int32_t ftCleanup(void *pContext);

#if defined(_LANGUAGE_C_PLUS_PLUS)||defined(__cplusplus)||defined(c_plusplus)
}
#endif
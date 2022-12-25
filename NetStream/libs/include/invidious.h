#pragma once

#include <kernel.h>

SCE_CDECL_BEGIN

#ifdef INVIDIOUS_BUILD
#define INV_EXPORT __declspec(dllexport)
#else
#define INV_EXPORT
#endif

typedef ScePVoid(*InvAllocator)(size_t size);

typedef SceVoid(*InvDeallocator)(ScePVoid ptr);

typedef SceBool(*InvDownloader)(char *url, ScePVoid *ppBuf, SceSize *pBufSize);

typedef enum InvItemType
{
	INV_ITEM_TYPE_VIDEO,
	INV_ITEM_TYPE_PLAYLIST,
	INV_ITEM_TYPE_CHANNEL,
	INV_ITEM_TYPE_ALL,

	INV_ITEM_TYPE_MAX
} InvItemType;

typedef enum InvSort
{
	INV_SORT_RELEVANCE,
	INV_SORT_RATING,
	INV_SORT_DATE,
	INV_SORT_VIEW_COUNT,

	INV_SORT_MAX
} InvSort;

typedef enum InvCommentSort
{
	INV_COMMENT_SORT_TOP,
	INV_COMMENT_SORT_NEW,

	INV_COMMENT_SORT_MAX
} InvCommentSort;

typedef enum InvDate
{
	INV_DATE_HOUR,
	INV_DATE_TODAY,
	INV_DATE_WEEK,
	INV_DATE_MONTH,
	INV_DATE_YEAR,
	INV_DATE_ALL,

	INV_DATE_MAX
} InvDate;

typedef enum InvHlsQuality
{
	INV_HLS_QUALITY_144P,
	INV_HLS_QUALITY_240P,
	INV_HLS_QUALITY_360P,
	INV_HLS_QUALITY_480P,
	INV_HLS_QUALITY_720P,

	INV_HLS_QUALITY_MAX
} InvHlsQuality;

typedef struct InvItemVideo
{
	ScePVoid reserved;
	const char *title;
	const char *id;
	const char *author;
	const char *authorId;
	const char *published;
	char thmbUrl[128];
	char thmbUrlHq[128];
	// Only with invParseVideo
	const char *avcLqUrl;
	const char *avcHqUrl;
	const char *audioLqUrl;
	const char *audioMqUrl;
	const char *audioHqUrl;
	SceInt32 lengthSec;
	SceBool isLive;
	const char *description;
	const char *subCount;
	SceInt32 likeCount;
} InvItemVideo;

typedef struct InvItemPlaylist
{
	const char *title;
	const char *id;
	const char *author;
	const char *authorId;
	SceInt32 videoCount;
	InvItemVideo *videoItems;
} InvItemPlaylist;

typedef struct InvItemChannel
{
	const char *author;
	const char *authorId;
	const char *thmbUrl;
	SceInt32 videoCount;
	SceInt32 subCount;
} InvItemChannel;

typedef struct InvItemComment
{
	ScePVoid reserved;
	const char *author;
	const char *published;
	const char *thmbUrl;
	const char *content;
	const char *continuation;
	const char *replyContinuation;
	SceInt32 likeCount;
	SceBool isOwner;
	SceBool likedByOwner;
} InvItemComment;

typedef struct InvItem
{
	ScePVoid reserved;
	InvItemType type;
	union
	{
		InvItemVideo *videoItem;
		InvItemPlaylist *playlistItem;
		InvItemChannel *channelItem;
	};
} InvItem;

INV_EXPORT SceInt32 invInit(InvAllocator allocator, InvDeallocator deallocator, InvDownloader downloader);

INV_EXPORT SceInt32 invTerm();

INV_EXPORT SceInt32 invSetInstanceUrl(const char *url);

INV_EXPORT SceInt32 invParseSearch(const char *request, SceInt32 page, InvItemType searchTypes, InvSort sort, InvDate date, InvItem **firstItem);

INV_EXPORT SceInt32 invParseVideo(const char *videoId, InvItemVideo **item);

INV_EXPORT SceInt32 invParseComments(const char *videoId, const char *continuation, InvCommentSort sort, InvItemComment **item);

INV_EXPORT SceInt32 invParseHlsComments(const char *videoId, InvItemComment **item);

INV_EXPORT SceInt32 invCleanupVideo(InvItemVideo *item);

INV_EXPORT SceInt32 invCleanupSearch(InvItem *items);

INV_EXPORT SceInt32 invCleanupComments(InvItemComment *item);

INV_EXPORT SceInt32 invCleanupHlsComments(InvItemComment *item);

INV_EXPORT SceInt32 invGetHlsUrl(const char *videoId, InvHlsQuality quality, char *hlsUrl, SceInt32 sizeOfUrl);

SCE_CDECL_END

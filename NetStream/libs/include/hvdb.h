#pragma once

#include <stdint.h>
#include <stddef.h>

#if defined(_LANGUAGE_C_PLUS_PLUS)||defined(__cplusplus)||defined(c_plusplus)
extern "C" {
#endif

#ifdef HVDB_BUILD
#define HVDB_EXPORT __declspec(dllexport)
#else
#define HVDB_EXPORT
#endif

#define HVDB_OK			(0)
#define HVDB_EPERM		(0x80010001)
#define HVDB_EBADF		(0x80010009)
#define HVDB_ENOMEM		(0x8001000C)
#define HVDB_EFAULT		(0x8001000E)
#define HVDB_EINVAL		(0x80010016)
#define HVDB_EUNSUP		(0x80010030)
#define HVDB_EALREADY	(0x80010078)
#define HVDB_EFORMAT	(0x8001002F)

typedef void*(*HVDBAllocator)(size_t size);
typedef void(*HVDBDeallocator)(void *ptr);
typedef void*(*HVDBReallocator)(void *ptr, size_t size);

typedef bool(*HVDBGetter)(const char *url, void **ppBuf, size_t *pBufSize, const char **ppHeaders, uint32_t headerNum,
	int32_t *pRespCode, HVDBAllocator ftalloc, HVDBDeallocator ftdealloc, HVDBReallocator ftrealloc);

typedef bool(*HVDBPoster)(const char *url, void *pBuf, size_t bufSize, const char **ppHeaders, uint32_t headerNum, void **ppRespBuf,
	size_t *pRespBufSize, int32_t *pRespCode, HVDBAllocator ftalloc, HVDBDeallocator ftdealloc, HVDBReallocator ftrealloc);

typedef enum HVDBItemType
{
	HVDB_ITEM_TYPE_WORK,
	HVDB_ITEM_TYPE_DATA,

	HVDB_ITEM_TYPE_MAX
} HVDBItemType;

typedef enum HVDBDataType
{
	HVDB_DATA_TYPE_FOLDER,
	HVDB_DATA_TYPE_MEDIA,

	HVDB_DATA_TYPE_MAX
} HVDBDataType;

typedef enum HVDBSort
{
	HVDB_SORT_ASCENDING,
	HVDB_SORT_DESCENDING,

	HVDB_SORT_MAX
} HVDBSort;

typedef enum HVDBOrder
{
	HVDB_ORDER_CREATE_DATE,
	HVDB_ORDER_RELEASE,
	HVDB_ORDER_DL_COUNT,
	HVDB_ORDER_PRICE,
	HVDB_ORDER_RATE_AVERAGE,
	HVDB_ORDER_REVIEW_COUNT,
	HVDB_ORDER_ID,
	HVDB_ORDER_NSFW,
	HVDB_ORDER_UPDATED_AT,

	HVDB_ORDER_MAX
} HVDBOrder;

typedef enum HVDBFilter
{
	HVDB_FILTER_MARKED,
	HVDB_FILTER_LISTENING,
	HVDB_FILTER_LISTENED,
	HVDB_FILTER_REPLAY,
	HVDB_FILTER_POSTPONED,
	HVDB_FILTER_ALL,

	HVDB_FILTER_MAX
} HVDBFilter;

typedef struct HVDBItemWork
{
	const char *title;
	char id[16];
	char circleId[16];
	const char *circle;
	const char *published;
	float rating;
	uint32_t rateCount;
	uint32_t dlCount;
	const char *thmbUrl;
	const char *bannerUrl;
	bool isNsfw;
} HVDBItemWork;

typedef struct HVDBItemData
{
	uint64_t reserved; // sizeof(ArduinoJson::JsonVariantConst)
	HVDBDataType type;
	const char *title;
	const char *url;
} HVDBItemData;

typedef struct HVDBItem
{
	HVDBItemType type;
	union
	{
		HVDBItemWork *workItem;
		HVDBItemData *dataItem;
	};
	HVDBItem *next;
} HVDBItem;

typedef struct HVDBPaginationInfo
{
	uint32_t currentPage;
	uint32_t worksPerPage;
	uint32_t worksTotal;
} HVDBPaginationInfo;

HVDB_EXPORT int32_t hvdbInit(HVDBAllocator allocator, HVDBDeallocator deallocator, HVDBReallocator reallocator, HVDBGetter getter, HVDBPoster poster);

HVDB_EXPORT int32_t hvdbSetInstance(const char *instance);

HVDB_EXPORT int32_t hvdbTerm();

HVDB_EXPORT int32_t hvdbParseWork(void **ppContext, const char *audioId, HVDBItem **item);

HVDB_EXPORT int32_t hvdbParseData(void **ppContext, const char *audioId, HVDBItem **item);

HVDB_EXPORT int32_t hvdbParseDataFolder(void *pContext, HVDBItemData *folder, HVDBItem **item);

HVDB_EXPORT int32_t hvdbParseFavourites(void **ppContext, uint32_t page, HVDBSort sort, HVDBOrder order, HVDBFilter filter, HVDBPaginationInfo *pagination, HVDBItem **item);

HVDB_EXPORT int32_t hvdbLogin(const char *name, const char *password);

HVDB_EXPORT int32_t hvdbIsLoggedIn();

HVDB_EXPORT int32_t hvdbCleanup(void *pContext);

#if defined(_LANGUAGE_C_PLUS_PLUS)||defined(__cplusplus)||defined(c_plusplus)
}
#endif
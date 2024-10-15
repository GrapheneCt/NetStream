#pragma once

#include <kernel.h>

SCE_CDECL_BEGIN

#ifdef HVDB_BUILD
#define HVDB_EXPORT __declspec(dllexport)
#else
#define HVDB_EXPORT
#endif

typedef ScePVoid(*HvdbAllocator)(size_t size);

typedef SceVoid(*HvdbDeallocator)(ScePVoid ptr);

typedef SceBool(*HvdbDownloader)(char *url, ScePVoid *ppBuf, SceSize *pBufSize);

typedef struct HvdbItemAudio
{
	ScePVoid reserved;
	const char *title;
	SceInt32 id;
	SceInt32 circleId;
	const char *circle;
	const char *published;
	float rating;
	SceInt32 rateCount;
	SceInt32 dlCount;
	const char *thmbUrl;
	const char *thmbUrlHq;
	bool isNsfw;
} HvdbItemAudio;

typedef struct HvdbItemTrack
{
	ScePVoid reserved;
	const char *title;
	const char *url;
} HvdbItemTrack;

HVDB_EXPORT SceInt32 hvdbInit(HvdbAllocator allocator, HvdbDeallocator deallocator, HvdbDownloader downloader);

HVDB_EXPORT SceInt32 hvdbTerm();

HVDB_EXPORT SceInt32 hvdbParseAudio(const char *audioId, HvdbItemAudio **item);

HVDB_EXPORT SceInt32 hvdbCleanupAudio(HvdbItemAudio *item);

HVDB_EXPORT SceInt32 hvdbParseTrack(const char *audioId, HvdbItemTrack **tracks);

HVDB_EXPORT SceInt32 hvdbCleanupTrack(HvdbItemTrack *item);

SCE_CDECL_END

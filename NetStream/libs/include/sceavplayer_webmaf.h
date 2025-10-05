#ifndef _SCE_AVPLAYER_SCEAVPLAYER_WEBMAF_H
#define _SCE_AVPLAYER_SCEAVPLAYER_WEBMAF_H

#include <stdarg.h>
#include <stdint.h>
#include <sceavplayer.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SCE_AV_PLAYER_PCM_BUFFER_SIZE 2048

#define SCE_AVPLAYER_STATE_EOS 0x6

typedef enum SceAvPlayerRepresentationType {
	SCE_AVPLAYER_REPRESENTATION_UNKNOWN,		///< An unknown stream type.
	SCE_AVPLAYER_REPRESENTATION_VIDEO,			///< The video stream type.
	SCE_AVPLAYER_REPRESENTATION_AUDIO,			///< The audio stream type.
	SCE_AVPLAYER_REPRESENTATION_TIMEDTEXT		///< The timed text stream type.
} SceAvPlayerRepresentationType;

typedef enum SceAvPlayerEventsEx {
	SCE_AVPLAYER_STATE_NO_DATA = 0x06,
	SCE_AVPLAYER_DRM_ERROR = 0x40,
	SCE_AVPLAYER_OUTPUT_PROTECTION = 0x50,
	SCE_AVPLAYER_TIMED_TEXT_DELIVERY_EX = 0x60,
	SCE_AVPLAYER_ACTIVATION_REQUIRED = 0x70
} SceAvPlayerEventsEx;

typedef enum NMLogLevel {
	NM_LOG_LEVEL_NONE = 0,
	NM_LOG_LEVEL_INFO = 1,
	NM_LOG_LEVEL_WARNINGS = 2,
	NM_LOG_LEVEL_ALL = 3
} NMLogLevel;

typedef enum SceAvPlayerUrlType {
	SceAvPlayer_LAUrl = 0,
	SceAvPlayer_CustomData = 1,
	SceAvPlayer_KeyUrl = 2,
	SceAvPlayer_ManifestUrl = 99
} SceAvPlayerUrlType;

typedef enum SceAvPlayerSourceType {
	SceAvPlayerSource_Unknown = 0,
	SceAvPlayerSource_File = 1,
	SceAvPlayerSource_SmoothStreaming = 2,
	SceAvPlayerSource_DashMp4 = 4,
	SceAvPlayerSource_DashTS = 6,
	SceAvPlayerSource_HLS = 8,
	SceAvPlayerSource_FileTS = 9
} SceAvPlayerSourceType;

typedef void *SceAvPlayerHttpInstance;

typedef void*(*SceAvPlayerHTTPAllocate)(void *, uint32_t, uint32_t);

typedef void(*SceAvPlayerHTTPDeallocate)(void *, void *);

typedef int32_t(*SceAvPlayerHttpRedirectCb)(char *, void *);

typedef struct SceAvPlayerHTTPCtx {
	uint32_t httpCtxId;
	uint32_t sslCtxId;
} SceAvPlayerHTTPCtx;

typedef struct SceAvPlayerHTTPHeader {
	char * name;
	char * value;
} SceAvPlayerHTTPHeader;

typedef struct SceAvPlayerHTTPSData {
	char * ptr;
	size_t size;
} SceAvPlayerHTTPSData;

typedef struct SceAvPlayerHttpReplacement {
	int32_t(*init)(SceAvPlayerHttpInstance *instancePP, void *objPtr, SceAvPlayerHTTPAllocate alloc,
		SceAvPlayerHTTPDeallocate dealloc, char *userAgent, SceAvPlayerHTTPCtx *httpCtxId);
	int32_t(*deInit)(SceAvPlayerHttpInstance *instancePP);
	int32_t(*setRedirectCb)(SceAvPlayerHttpInstance instanceP, SceAvPlayerHttpRedirectCb *redirectCb, void *object);
	int64_t(*post)(SceAvPlayerHttpInstance instanceP, char *url, bool continuation,
		SceAvPlayerHTTPHeader *headers, uint8_t numHeaders, uint8_t *body, uint32_t len,
		uint8_t *buf, uint32_t bufLen, bool *completed);
	int64_t(*get)(SceAvPlayerHttpInstance instanceP, char *url, bool continuation,
		uint64_t rangeStart, uint64_t rangeEnd, uint8_t *buf, uint32_t bufLen, bool *completed);
	int32_t(*setCert)(SceAvPlayerHttpInstance instanceP, SceAvPlayerHTTPSData *cert, SceAvPlayerHTTPSData *privKey);
	int32_t(*setRecvTimeout)(SceAvPlayerHttpInstance instanceP, int64_t usec);
	int32_t(*getLastStatus)(SceAvPlayerHttpInstance instanceP);
} SceAvPlayerHttpReplacement;

typedef struct SceAvPlayerPostInitDataNew {
	uint32_t demuxVideoBufferSize;
	uint8_t _reserved[0x40];
	SceAvPlayerHttpReplacement httpReplacement;
	SceAvPlayerHTTPCtx httpCtx;
	uint8_t reserved[0x14];
} SceAvPlayerPostInitDataNew;

typedef struct SceAvPlayerURICommon {
	char *name;
	uint32_t length;
} SceAvPlayerURICommon;

typedef struct SceAvPlayerURISimpleQuery {
	SceAvPlayerURICommon userName;
	SceAvPlayerURICommon password;
	SceAvPlayerURICommon query;
	SceAvPlayerURICommon fragment;
} SceAvPlayerURISimpleQuery;

typedef struct SceAvPlayerURISimpleDetails {
	SceAvPlayerURICommon uri;
	SceAvPlayerURISimpleQuery details;
	SceAvPlayerSourceType sourceType;
} SceAvPlayerURISimpleDetails;

typedef struct SceAvPlayerStreamOptions {
	uint8_t reserved[4];
	bool allowWMAPro;
} SceAvPlayerStreamOptions;

typedef union SceAvPlayerSourceDetails {
	struct SceAvPlayerURISimpleDetails UriDetails;
	uint8_t reserved[128];
} SceAvPlayerSourceDetails;

typedef struct SceAvPlayerPostInitDataEx {
	uint32_t unk_00;
	uint32_t unk_04;
	uint32_t demuxSharedHeapSize;
	uint32_t demuxVideoBufferSize;
	uint32_t demuxAudioBufferSize;
	uint32_t streamingContainerBufferSize;
	uint32_t unk_18;
} SceAvPlayerPostInitDataEx;

typedef union SceAvPlayerRepresentationDetails {
	SceAvPlayerAudio audio;
	SceAvPlayerVideo video;
	SceAvPlayerTimedText subs;
} SceAvPlayerRepresentationDetails;

typedef struct SceAvPlayerRepresentationInfo {
	uint32_t bitrate;
	SceAvPlayerRepresentationType type;
	uint8_t reserved[16];
	SceAvPlayerRepresentationDetails details;
} SceAvPlayerRepresentationInfo;

typedef struct SceAvPlayerAudioEx {
	uint16_t channelCount;
	uint8_t reserved[2];
	uint32_t sampleRate;
	uint32_t size;
	uint8_t languageCode[4];
	uint8_t reserved1[64];
} SceAvPlayerAudioEx;

typedef struct SceAvPlayerVideoEx {
	uint32_t width;
	uint32_t height;
	float aspectRatio;
	uint8_t languageCode[4];
	uint32_t framerate;
	uint32_t cropLeftOffset;
	uint32_t cropRightOffset;
	uint32_t cropTopOffset;
	uint32_t cropBottomOffset;
	uint16_t cropLeftPixels;
	uint16_t cropRightPixels;
	uint16_t cropTopPixels;
	uint16_t cropBottomPixels;
	uint32_t pitch;
	uint8_t reserved[32];
} SceAvPlayerVideoEx;

typedef struct SceAvPlayerTimedTextEx {
	uint8_t languageCode[4];
	uint8_t reserved[12];
	uint8_t reserved1[64];
} SceAvPlayerTimedTextEx;

typedef union SceAvPlayerStreamDetailsEx {
	SceAvPlayerAudioEx audio;
	SceAvPlayerVideoEx video;
	SceAvPlayerTimedTextEx subs;
	uint8_t reserved[80];
} SceAvPlayerStreamDetailsEx;

typedef struct SceAvPlayerFrameInfoEx {
	void * pData;
	uint8_t reserved[4];
	uint64_t timeStamp;
	SceAvPlayerStreamDetailsEx details;
} SceAvPlayerFrameInfoEx;

typedef int32_t SceAvPlayerLogger(void *userData, const char *format, _Va_list ap);

int32_t sceAvPlayerGetVersionInfo(char *versionInfo, uint32_t bufferLen, uint32_t *versionInfoLen);

int32_t sceAvPlayerSetLogger(SceAvPlayerLogger f, void *userData);

int32_t sceAvPlayerOsalPrintf(char *format);

int32_t sceAvPlayerOsalPrintf2(char *format);

int32_t sceAvPlayerAddSourceEx(SceAvPlayerHandle h, SceAvPlayerUrlType UrlType, SceAvPlayerSourceDetails *pUrlExtras);

SceAvPlayerFileReplacement NETMediaInit(uint8_t *pNetBuffer, size_t buffsz, int32_t sslCtxId, int32_t httpCtxId, SceAvPlayerMemAllocator *pMemAllocator, NMLogLevel logLevel);

void NETMediaDeInit(void *arg, SceAvPlayerMemAllocator *pMemAllocator);

int32_t sceAvPlayerSetAvailableBW(SceAvPlayerHandle h, uint32_t argBW);

int32_t sceAvPlayerGetAvailableBW(SceAvPlayerHandle h, uint32_t *argpBW);

int32_t sceAvPlayerGetStreamBitrate(SceAvPlayerHandle h, uint32_t argStreamID, uint32_t *argpBitrate);

int32_t sceAvPlayerEnableRepresentation(SceAvPlayerHandle h, uint32_t argStreamID, uint32_t representationID);

int32_t sceAvPlayerGetRepresentationInfo(SceAvPlayerHandle h, uint32_t argStreamID, uint32_t representationID, SceAvPlayerRepresentationInfo *argInfo);

int32_t sceAvPlayerDisableRepresentation(SceAvPlayerHandle h, uint32_t argStreamID, uint32_t representationID);

int32_t sceAvPlayerRepresentationCount(SceAvPlayerHandle h, uint32_t argStreamID);

int32_t sceAvPlayerGetStreamSetBitrate(SceAvPlayerHandle h, uint32_t argStreamID, uint32_t *argpBitrate);

int32_t sceAvPlayerRepresentationIsEnabled(SceAvPlayerHandle h, uint32_t argStreamID, uint32_t representationID, bool *enabled);

int32_t sceAvPlayerChangeStream(SceAvPlayerHandle h, uint32_t argStreamID, uint32_t argTargetStreamID);

int32_t sceAvPlayerEnableStreamEx(SceAvPlayerHandle h, uint32_t argStreamID, SceAvPlayerStreamOptions *argOpt);

int32_t sceAvPlayerPostInitEx(SceAvPlayerHandle h, SceAvPlayerPostInitDataEx *pPostInit);

int32_t sceAvPlayerGetVideoDataEx(SceAvPlayerHandle h, SceAvPlayerFrameInfoEx *argFrameInfo);

//int32_t sceAvPlayerGetStreamInfoEx(SceAvPlayerHandle h);

//int32_t sceAvPlayerGetAudioDataEx(SceAvPlayerHandle h);

//int32_t SceAvPlayer_07CBDB80(SceAvPlayerHandle h);
//int32_t SceAvPlayer_3534430F(SceAvPlayerHandle h);
//int32_t SceAvPlayer_3F125760(SceAvPlayerHandle h);
//int32_t SceAvPlayer_62EE177D(SceAvPlayerHandle h);

#ifdef __cplusplus
}
#endif

#endif	/* _SCE_AVPLAYER_SCEAVPLAYER_WEBMAF_H */


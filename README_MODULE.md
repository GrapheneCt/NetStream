### About modules used in this project (module folder)

## download_enabler_netstream.suprx

Modification of download_enabler plugin from VitaTweaks collection that enables NetStream-specific downloads while keeping the original download_enabler functionality.
If original download_enabler is present, it will override it.
This module is optional: if taihen is not running or fails for some reason, NetStream will still work, but without option to download streams.

## libcurl.suprx

Version of libcurl for PS Vita without any external dependencies. Uses mbedtls. You only need to provide memory allocators via curl_global_memmanager_set_np().

## libfmodstudio.suprx

FMOD Studio for PS Vita.

## libfmodngpext.suprx

Extension for FMOD that adds some Vita-specific codec support, such as ATRAC3+, ATRAC9, OPUS etc.

## libFourthTube.suprx

InnerTube API parser.

## libLootkit.suprx

Twitch API parser (currently unfinished).

## libBEAVCorePlayer.suprx

BEAVCorePlayer is the system module that implements playback of HLS and DASH streams, remote and locally stored video files.
It uses Sony's libLivestream (libLS) library which can be explored in detail in PS4 Trilithium unstripped video.prx.
The reason this module is provided with NetStream instead of just loading it from vs0: is beacuse we need to adjust some internal values without using taihen.
This adjustments are hardcoded into this module and are the following:
1. Module is linked with libSceHttpForBEAVCorePlayer instead of SceHttp (see libSceHttpForBEAVCorePlayer.suprx desc.).
2. Maximum playlist size in libLS options (lsSetLibraryParamDefaults()) is ncreased from 0x80000 to 0xF0000 since YouTube likes to use some absolute units of playlists, sometimes reaching 1 MiB in size.

## libSceHttpForBEAVCorePlayer.suprx

Partial reimplementation of SceHttp that uses libcurl. Needed to support modern cypher methods that are used pretty much everywhere nowadays.

## libSceAvPlayerPSVitaRGBA8888.suprx

SceAvPlayer for WebMAF, modified to output RGBA8888 decoded frames.
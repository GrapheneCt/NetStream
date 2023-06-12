#ifndef _FMODNGPEXT_H_
#define _FMODNGPEXT_H_

#include <fmod/fmod.h>

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct FMOD_NGPEXT_PARAM
{
	unsigned int at9prio;
	unsigned int at3prio;
	unsigned int opusprio;
	unsigned int aacprio;
	unsigned int webmprio;
} FMOD_NGPEXT_PARAM;

FMOD_RESULT F_API FMOD_NGP_System_Init(FMOD_SYSTEM *system, FMOD_NGPEXT_PARAM *param);

#ifdef __cplusplus
}
#endif

#endif

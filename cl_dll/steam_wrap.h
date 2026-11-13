// steam_wrap.h: Counter-Strike 1.4: file functions that go through Steam.dll when it is there.
//
// Everything here is OURS but the 28 function names of Steam.dll (the image's strings) and the two
// messages: retail's client.dll shows six functions of this code behind its last object, compiled without
// the project's /Ob2 (the three small ones are called, not expanded), which reads like a static library
// of Valve's that the client linked.  We have no such library: the file is compiled as the project's last
// source with its own options.  Names, types and the header are ours.

#ifndef STEAM_WRAP_H
#define STEAM_WRAP_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
	int		eSteamError;			// 0: none, 4: no usable Steam.dll, 8: bad argument
	int		eDetailErrorType;
	int		nDetailErrorCode;
	char	szDesc[ 255 ];
} TSteamError;

// called on every file request of the client while g_bSteamCallNotify is set (nothing in the client sets
// either)
extern int g_bSteamCallNotify;
extern void (*g_pfnSteamCallNotify)( void *pArg );

int STEAM_GetLocalFileCopy( const char *cszName, TSteamError *pError );
int STEAM_IsFileImmediatelyAvailable( const char *cszName, TSteamError *pError );

#ifdef __cplusplus
}
#endif

#endif // STEAM_WRAP_H

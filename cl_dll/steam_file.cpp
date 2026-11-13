// steam_file.cpp: Counter-Strike 1.4: the two file requests of the client, with the path's slashes turned.
// Retail: an object between statusbar.obj and StudioModelRenderer.obj (0x1940120, 0x19401a0).  The file's
// name, the functions' names and the statics' names are ours.

#include <string.h>
#include "steam_wrap.h"

static TSteamError s_SteamError;
static int s_nCalls;
static void *s_pNotifyArg;

extern "C" int Steam_GetLocalCopy( const char *pszName )
{
	char szName[ 260 ];
	char *p;

	if ( g_bSteamCallNotify )
	{
		s_nCalls++;
		if ( g_pfnSteamCallNotify )
			g_pfnSteamCallNotify( s_pNotifyArg );
	}

	strcpy( szName, pszName );
	for ( p = szName; *p; p++ )
	{
		if ( *p == '/' )
			*p = '\\';
	}

	return STEAM_GetLocalFileCopy( szName, &s_SteamError );
}

extern "C" int Steam_FileExists( const char *pszName )
{
	char szName[ 260 ];
	char *p;

	if ( g_bSteamCallNotify )
	{
		s_nCalls++;
		if ( g_pfnSteamCallNotify )
			g_pfnSteamCallNotify( s_pNotifyArg );
	}

	strcpy( szName, pszName );
	for ( p = szName; *p; p++ )
	{
		if ( *p == '/' )
			*p = '\\';
	}

	return STEAM_IsFileImmediatelyAvailable( szName, &s_SteamError );
}

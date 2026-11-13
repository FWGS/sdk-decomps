// steam_wrap.c: Counter-Strike 1.4: file functions that go through Steam.dll when it is there
// (see steam_wrap.h: everything but Steam.dll's names is ours).
//
// Retail (client.dll, behind voice_banmgr.obj): STEAM_GetLocalFileCopy 0x1961240,
// STEAM_IsFileImmediatelyAvailable 0x19612c0, LoadSteam 0x1961380, ClearError 0x1961670,
// SetConfigError 0x1961690, SetBadArgument 0x19616c0; the 28 pointers, the module handle and the mode
// are its 120 bytes of .bss (0x1a08dfc); the two notify variables are common symbols (0x1a0923c).

#include <windows.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <string.h>
#include "steam_wrap.h"

int g_bSteamCallNotify;
void (*g_pfnSteamCallNotify)( void *pArg );

enum
{
	STEAM_MODE_UNKNOWN = 0,		// not looked for yet
	STEAM_MODE_NONE,			// no Steam.dll: the C runtime's files
	STEAM_MODE_STEAM,			// Steam.dll with every function
};

// sic: the statics are initialised so that VC6 lays them out in .bss in the order of definition, and they are
// defined in the order retail's .bss has them (0x1a08dfc on).  Valve's names, which would have put them in
// that order by their hash, are not known (no prefix/suffix scheme on Steam.dll's names gives it).
static FARPROC s_pSteamIsFileImmediatelyAvailable = NULL;
static FARPROC s_pSteamStat = NULL;
static FARPROC s_pSteamFindClose = NULL;
static FARPROC s_pSteamOpenTmpFile = NULL;
static FARPROC s_pSteamCreateLogContext = NULL;
static FARPROC s_pSteamGetEncryptedUserIDTicket = NULL;
static FARPROC s_pSteamCloseFile = NULL;
static FARPROC s_pSteamLogResourceLoadStarted = NULL;
static FARPROC s_pSteamOpenFile = NULL;
static FARPROC s_pSteamStartup = NULL;
static FARPROC s_pSteamSeekFile = NULL;
static FARPROC s_pSteamGetVersion = NULL;
static FARPROC s_pSteamPauseCachePreloading = NULL;
static FARPROC s_pSteamLog = NULL;
static FARPROC s_pSteamLogResourceLoadFinished = NULL;
static FARPROC s_pSteamResumeCachePreloading = NULL;
static FARPROC s_pSteamTellFile = NULL;
static FARPROC s_pSteamPrintFile = NULL;
static FARPROC s_pSteamWaitForResources = NULL;
static FARPROC s_pSteamFindNext = NULL;
static FARPROC s_pSteamCleanup = NULL;
static FARPROC s_pSteamSizeFile = NULL;
static FARPROC s_pSteamFindFirst = NULL;
static FARPROC s_pSteamReadFile = NULL;
static FARPROC s_pSteamWriteFile = NULL;
static FARPROC s_pSteamGetLocalFileCopy = NULL;
static FARPROC s_pSteamHintResourceNeed = NULL;
static FARPROC s_pSteamForgetAllHints = NULL;
static HMODULE s_hSteam = NULL;
static int s_eMode = 0;

static int LoadSteam( void );
static void ClearError( TSteamError *pError );
static void SetConfigError( TSteamError *pError );
static void SetBadArgument( TSteamError *pError );

// sic: a stand-in, name and text ours.  Retail links the C runtime's tmpfile.obj and rmtmp.obj (its .CRT$XPX
// has _rmtmp, 0x196c6e6; their 40 bytes of .bss) although no live code calls tmpfile: a function nothing calls
// referred to it.  Steam.dll's SteamOpenTmpFile, which LoadSteam looks up, is the call this wrapper falls back from.
FILE *STEAM_OpenTmpFile( TSteamError *pError )
{
	if ( !pError )
		return NULL;

	ClearError( pError );

	if ( s_eMode == STEAM_MODE_UNKNOWN )
		s_eMode = LoadSteam();

	if ( s_eMode == STEAM_MODE_STEAM )
		return ( (FILE *(*)( TSteamError * ))s_pSteamOpenTmpFile )( pError );
	else if ( s_eMode == STEAM_MODE_NONE )
		return tmpfile();

	SetConfigError( pError );
	return NULL;
}

int STEAM_GetLocalFileCopy( const char *cszName, TSteamError *pError )
{
	int result;

	if ( !pError )
		return 0;

	if ( !cszName || !*cszName )
	{
		SetBadArgument( pError );
		return 0;
	}

	ClearError( pError );

	result = 0;

	if ( s_eMode == STEAM_MODE_UNKNOWN )
		s_eMode = LoadSteam();

	if ( s_eMode == STEAM_MODE_STEAM )
	{
		result = ( (int (*)( const char *, TSteamError * ))s_pSteamGetLocalFileCopy )( cszName, pError );
	}
	else if ( s_eMode == STEAM_MODE_NONE )
	{
		result = 1;
	}
	else
	{
		SetConfigError( pError );
	}

	return result;
}

int STEAM_IsFileImmediatelyAvailable( const char *cszName, TSteamError *pError )
{
	int result = 0;
	struct _stat buf;

	if ( !pError )
		return 0;

	if ( !cszName || !*cszName )
	{
		SetBadArgument( pError );
		return 0;
	}

	ClearError( pError );

	if ( s_eMode == STEAM_MODE_UNKNOWN )
		s_eMode = LoadSteam();

	if ( s_eMode == STEAM_MODE_STEAM )
	{
		result = ( (int (*)( const char *, TSteamError * ))s_pSteamIsFileImmediatelyAvailable )( cszName, pError );
	}
	else if ( s_eMode == STEAM_MODE_NONE )
	{
		if ( _stat( cszName, &buf ) != -1 && ( buf.st_mode & _S_IFREG ) )
			result = 1;
		else
			result = 0;
	}
	else
	{
		SetConfigError( pError );
	}

	return result;
}

static int LoadSteam( void )
{
	int eMode;
	char bMissing = 0;

	s_hSteam = LoadLibrary( "Steam.dll" );
	if ( s_hSteam )
	{
		if ( ( s_pSteamGetVersion = GetProcAddress( s_hSteam, "SteamGetVersion" ) ) == NULL )
			bMissing = 1;
		if ( ( s_pSteamStartup = GetProcAddress( s_hSteam, "SteamStartup" ) ) == NULL )
			bMissing = 1;
		if ( ( s_pSteamCleanup = GetProcAddress( s_hSteam, "SteamCleanup" ) ) == NULL )
			bMissing = 1;
		if ( ( s_pSteamStat = GetProcAddress( s_hSteam, "SteamStat" ) ) == NULL )
			bMissing = 1;
		if ( ( s_pSteamOpenFile = GetProcAddress( s_hSteam, "SteamOpenFile" ) ) == NULL )
			bMissing = 1;
		if ( ( s_pSteamOpenTmpFile = GetProcAddress( s_hSteam, "SteamOpenTmpFile" ) ) == NULL )
			bMissing = 1;
		if ( ( s_pSteamCloseFile = GetProcAddress( s_hSteam, "SteamCloseFile" ) ) == NULL )
			bMissing = 1;
		if ( ( s_pSteamReadFile = GetProcAddress( s_hSteam, "SteamReadFile" ) ) == NULL )
			bMissing = 1;
		if ( ( s_pSteamWriteFile = GetProcAddress( s_hSteam, "SteamWriteFile" ) ) == NULL )
			bMissing = 1;
		if ( ( s_pSteamPrintFile = GetProcAddress( s_hSteam, "SteamPrintFile" ) ) == NULL )
			bMissing = 1;
		if ( ( s_pSteamSeekFile = GetProcAddress( s_hSteam, "SteamSeekFile" ) ) == NULL )
			bMissing = 1;
		if ( ( s_pSteamTellFile = GetProcAddress( s_hSteam, "SteamTellFile" ) ) == NULL )
			bMissing = 1;
		if ( ( s_pSteamSizeFile = GetProcAddress( s_hSteam, "SteamSizeFile" ) ) == NULL )
			bMissing = 1;
		if ( ( s_pSteamFindFirst = GetProcAddress( s_hSteam, "SteamFindFirst" ) ) == NULL )
			bMissing = 1;
		if ( ( s_pSteamFindNext = GetProcAddress( s_hSteam, "SteamFindNext" ) ) == NULL )
			bMissing = 1;
		if ( ( s_pSteamFindClose = GetProcAddress( s_hSteam, "SteamFindClose" ) ) == NULL )
			bMissing = 1;
		if ( ( s_pSteamGetLocalFileCopy = GetProcAddress( s_hSteam, "SteamGetLocalFileCopy" ) ) == NULL )
			bMissing = 1;
		if ( ( s_pSteamHintResourceNeed = GetProcAddress( s_hSteam, "SteamHintResourceNeed" ) ) == NULL )
			bMissing = 1;
		if ( ( s_pSteamForgetAllHints = GetProcAddress( s_hSteam, "SteamForgetAllHints" ) ) == NULL )
			bMissing = 1;
		if ( ( s_pSteamPauseCachePreloading = GetProcAddress( s_hSteam, "SteamPauseCachePreloading" ) ) == NULL )
			bMissing = 1;
		if ( ( s_pSteamResumeCachePreloading = GetProcAddress( s_hSteam, "SteamResumeCachePreloading" ) ) == NULL )
			bMissing = 1;
		if ( ( s_pSteamWaitForResources = GetProcAddress( s_hSteam, "SteamWaitForResources" ) ) == NULL )
			bMissing = 1;
		if ( ( s_pSteamIsFileImmediatelyAvailable = GetProcAddress( s_hSteam, "SteamIsFileImmediatelyAvailable" ) ) == NULL )
			bMissing = 1;
		if ( ( s_pSteamCreateLogContext = GetProcAddress( s_hSteam, "SteamCreateLogContext" ) ) == NULL )
			bMissing = 1;
		if ( ( s_pSteamLog = GetProcAddress( s_hSteam, "SteamLog" ) ) == NULL )
			bMissing = 1;
		if ( ( s_pSteamLogResourceLoadStarted = GetProcAddress( s_hSteam, "SteamLogResourceLoadStarted" ) ) == NULL )
			bMissing = 1;
		if ( ( s_pSteamLogResourceLoadFinished = GetProcAddress( s_hSteam, "SteamLogResourceLoadFinished" ) ) == NULL )
			bMissing = 1;
		if ( ( s_pSteamGetEncryptedUserIDTicket = GetProcAddress( s_hSteam, "SteamGetEncryptedUserIDTicket" ) ) == NULL )
			bMissing = 1;

		eMode = STEAM_MODE_STEAM;
		if ( bMissing )
		{
			FreeLibrary( s_hSteam );
			eMode = STEAM_MODE_UNKNOWN;
		}

		return eMode;
	}

	return STEAM_MODE_NONE;
}

static void ClearError( TSteamError *pError )
{
	pError->eSteamError = 0;
	pError->eDetailErrorType = 0;
	pError->nDetailErrorCode = 0;
	pError->szDesc[ 0 ] = 0;
}

static void SetConfigError( TSteamError *pError )
{
	pError->eSteamError = 4;
	pError->eDetailErrorType = 0;
	pError->nDetailErrorCode = 0;
	strncpy( pError->szDesc, "Configuration error", sizeof( pError->szDesc ) );
}

static void SetBadArgument( TSteamError *pError )
{
	pError->eSteamError = 8;
	pError->eDetailErrorType = 0;
	pError->nDetailErrorCode = 0;
	strncpy( pError->szDesc, "Bad argument", sizeof( pError->szDesc ) );
}

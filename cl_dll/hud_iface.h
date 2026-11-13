//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#if !defined( HUD_IFACEH )
#define HUD_IFACEH
#pragma once

#define EXPORT		_declspec( dllexport )
#define _DLLEXPORT		// Counter-Strike 1.4: nothing is exported but F (cdll_int.cpp)

typedef int (*pfnUserMsgHook)(const char *pszName, int iSize, void *pbuf);
#include "wrect.h"
#include "../engine/cdll_int.h"
extern cl_enginefunc_t gEngfuncs;

#endif
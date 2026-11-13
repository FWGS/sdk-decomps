/***
*
*	Copyright (c) 1999, 2000 Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/
#ifndef EXTDLL_H
#define EXTDLL_H


//
// Global header file for extension DLLs
//

// Allow "DEBUG" in addition to default "_DEBUG"
#ifdef _DEBUG
#define DEBUG 1
#endif

// Silence certain warnings
#pragma warning(disable : 4244)		// int or float down-conversion
#pragma warning(disable : 4305)		// int or float data truncation
#pragma warning(disable : 4201)		// nameless struct/union
#pragma warning(disable : 4514)		// unreferenced inline function removed
#pragma warning(disable : 4100)		// unreferenced formal parameter

#ifdef _WIN32
// Prevent tons of unused windows definitions
#define WIN32_LEAN_AND_MEAN
#define NOWINRES
#define NOSERVICE
#define NOMCX
#define NOIME
#include "WINDOWS.H"

// sic: a stand-in, names ours.  The compiler's intermediate code refers to every declared name by
// its index, and an index from 0x8000 on takes two bytes more.  In Counter-Strike 1.0 the files that
// include this header reached their own code with about 8,000 more names declared than this tree's
// text and Visual C++ 6.0's own windows.h give (the build machine's windows.h, a later Platform SDK's,
// is the likely source; WIN32_LEAN_AND_MEAN and the NO* defines above do not account for it).  So
// part of hl_weapons.cpp's own code refers to its names by long indices and its intermediate code
// is larger, by about 2,300 bytes before HUD_InitClientWeapons and 3,400 before Game_GetSequence;
// hl_weapons.obj writes its own precompiled header and places the headers' inline functions by
// that size.  With the client's own headers client.dll is retail's file with 8,020 to 8,044 more names
// here (full builds).  The window moves by one for every name the headers before a file's
// own code gain or lose: with 1.1's server headers it is 7,888 to 7,902 (1.0b: 7,889 to 7,903); 7,896 are declared.
#define R10_NAMES10( p )	p##0, p##1, p##2, p##3, p##4, p##5, p##6, p##7, p##8, p##9
#define R10_NAMES100( p )	R10_NAMES10( p##0 ), R10_NAMES10( p##1 ), R10_NAMES10( p##2 ), R10_NAMES10( p##3 ), R10_NAMES10( p##4 ), R10_NAMES10( p##5 ), R10_NAMES10( p##6 ), R10_NAMES10( p##7 ), R10_NAMES10( p##8 ), R10_NAMES10( p##9 )
#define R10_NAMES1000( p )	R10_NAMES100( p##0 ), R10_NAMES100( p##1 ), R10_NAMES100( p##2 ), R10_NAMES100( p##3 ), R10_NAMES100( p##4 ), R10_NAMES100( p##5 ), R10_NAMES100( p##6 ), R10_NAMES100( p##7 ), R10_NAMES100( p##8 ), R10_NAMES100( p##9 )
enum r10_unknown_e
{
	R10_NAMES1000( R10_UNKNOWN0 ), R10_NAMES1000( R10_UNKNOWN1 ), R10_NAMES1000( R10_UNKNOWN2 ), R10_NAMES1000( R10_UNKNOWN3 ),
	R10_NAMES1000( R10_UNKNOWN4 ), R10_NAMES1000( R10_UNKNOWN5 ), R10_NAMES1000( R10_UNKNOWN6 ), R10_NAMES100( R10_UNKNOWN70 ),
	R10_NAMES100( R10_UNKNOWN71 ), R10_NAMES100( R10_UNKNOWN72 ), R10_NAMES100( R10_UNKNOWN73 ), R10_NAMES100( R10_UNKNOWN74 ),
	R10_NAMES100( R10_UNKNOWN75 ), R10_NAMES100( R10_UNKNOWN76 ), R10_NAMES100( R10_UNKNOWN77 ), R10_NAMES10( R10_UNKNOWN780 ),
	R10_NAMES10( R10_UNKNOWN781 ), R10_NAMES10( R10_UNKNOWN782 ), R10_NAMES10( R10_UNKNOWN783 ), R10_NAMES10( R10_UNKNOWN784 ),
	R10_NAMES10( R10_UNKNOWN785 ), R10_NAMES10( R10_UNKNOWN786 ), R10_NAMES10( R10_UNKNOWN787 ), R10_NAMES10( R10_UNKNOWN788 ),
	R10_UNKNOWN7890, R10_UNKNOWN7891, R10_UNKNOWN7892, R10_UNKNOWN7893,
	R10_UNKNOWN7894, R10_UNKNOWN7895,
};
 
// Misc C-runtime library headers
#include "STDIO.H"
#include "STDLIB.H"
#include "MATH.H"
 
#else
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>
 
#define MAX_PATH PATH_MAX
#define ULONG ulong
#define FALSE 0
#define TRUE  1
 
#ifndef max
#define max(a,b)    (((a) > (b)) ? (a) : (b))
#endif
 
#ifndef min
#define min(a,b)    (((a) < (b)) ? (a) : (b))
#endif
 
#define itoa(a,b,c) sprintf(b, "%d", a)
 
typedef unsigned char BYTE;
#endif

// Header file containing definition of globalvars_t and entvars_t
typedef int	func_t;					//
typedef int	string_t;				// from engine's pr_comp.h;
typedef float vec_t;				// needed before including progdefs.h

// Vector class
#include "vector.h"

// Defining it as a (bogus) struct helps enforce type-checking
#define vec3_t Vector

// Shared engine/DLL constants
#include "const.h"
#include "progdefs.h"
#include "edict.h"

// Shared header describing protocol between engine and DLLs
#include "eiface.h"

// Shared header between the client DLL and the game DLLs
#include "cdll_dll.h"

#endif //EXTDLL_H

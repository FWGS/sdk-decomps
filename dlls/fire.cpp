/***
*
*	Copyright (c) 1996-2001, Valve LLC. All rights reserved.
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
// flame (CFire), fire_trail.
// A stand-in until the fire code is written: only the function other files call
// (the incendiary grenade leaves flames with it), doing nothing, so the server links.

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "weapons.h"

void FireSpread( const Vector &vecOrigin, float flDamage, entvars_t *pevOwner, edict_t *pentIgnore )
{
	// stand-in: no flames
}

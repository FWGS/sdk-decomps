/***
*
*	Copyright (c) 1999, Valve LLC. All rights reserved.
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

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"
#ifdef VALVE_DLL
#include "items.h"
#endif

#ifdef VALVE_DLL

// POV: the glock and its clip are props that can't be taken; the clip
// precaches what the SDK's gun did. The client builds nothing from this
// file (retail's client has no glock code and no weapon prediction).
class CGlock : public CItem
{
	void Spawn( void )
	{ 
		Precache( );
		SET_MODEL(ENT(pev), "models/w_9mmhandgun.mdl");
		CItem::Spawn( );
	}
	void Precache( void )
	{
		PRECACHE_MODEL("models/w_9mmhandgun.mdl");
	}
	BOOL MyTouch( CBasePlayer *pPlayer )
	{
		return FALSE;
	}
};
LINK_ENTITY_TO_CLASS( weapon_glock, CGlock );
LINK_ENTITY_TO_CLASS( weapon_9mmhandgun, CGlock );



class CGlockAmmo : public CItem
{
	void Spawn( void )
	{ 
		Precache( );
		SET_MODEL(ENT(pev), "models/w_9mmclip.mdl");
		CItem::Spawn( );
	}
	void Precache( void )
	{
		PRECACHE_MODEL ("models/w_9mmclip.mdl");

		PRECACHE_MODEL ("models/shell.mdl");// brass shell

		PRECACHE_SOUND("items/9mmclip1.wav");
		PRECACHE_SOUND("items/9mmclip2.wav");

		PRECACHE_SOUND ("weapons/pl_gun1.wav");//silenced handgun
		PRECACHE_SOUND ("weapons/pl_gun2.wav");//silenced handgun
		PRECACHE_SOUND ("weapons/pl_gun3.wav");//handgun
	}
	BOOL MyTouch( CBasePlayer *pPlayer )
	{
		return FALSE;
	}
};
LINK_ENTITY_TO_CLASS( ammo_glockclip, CGlockAmmo );
LINK_ENTITY_TO_CLASS( ammo_9mmclip, CGlockAmmo );

#endif	// VALVE_DLL

















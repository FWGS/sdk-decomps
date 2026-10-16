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
#if !defined( OEM_BUILD ) && !defined( HLDEMO_BUILD )

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"
#include "gamerules.h"
#include "items.h"

// POV: the crossbow is a prop that can't be taken; it precaches what the
// SDK's bolt did
class CCrossbow : public CItem
{
	void Spawn( void )
	{ 
		Precache( );
		SET_MODEL(ENT(pev), "models/w_crossbow.mdl");
		CItem::Spawn( );
		UTIL_MakeProp( this );
	}
	void Precache( void )
	{
		PRECACHE_MODEL("models/w_crossbow.mdl");
		PRECACHE_SOUND("weapons/xbow_hitbod1.wav");
		PRECACHE_SOUND("weapons/xbow_hitbod2.wav");
		PRECACHE_SOUND("weapons/xbow_fly1.wav");
		PRECACHE_SOUND("weapons/xbow_hit1.wav");
		PRECACHE_SOUND("fvox/beep.wav");
		PRECACHE_MODEL("sprites/streak.spr");
	}
	BOOL MyTouch( CBasePlayer *pPlayer )
	{
		return FALSE;
	}
};
LINK_ENTITY_TO_CLASS( weapon_crossbow, CCrossbow );



class CCrossbowAmmo : public CItem
{
	void Spawn( void )
	{ 
		Precache( );
		SET_MODEL(ENT(pev), "models/w_crossbow_clip.mdl");
		CItem::Spawn( );
	}
	void Precache( void )
	{
		PRECACHE_MODEL ("models/w_crossbow_clip.mdl");
	}
	BOOL MyTouch( CBasePlayer *pPlayer )
	{
		return FALSE;
	}
};
LINK_ENTITY_TO_CLASS( ammo_crossbow, CCrossbowAmmo );



#endif
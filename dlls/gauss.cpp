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
#include "soundent.h"
#include "shake.h"
#include "gamerules.h"
#include "items.h"


// POV: the gauss gun and its ammo are props that can't be taken; the ammo
// precaches what the SDK's gun did
class CGauss : public CItem
{
public:
	void Spawn( void )
	{
		Precache( );
		SET_MODEL(ENT(pev), "models/w_gauss.mdl");
		CItem::Spawn( );
		UTIL_MakeProp( this );
	}
	void Precache( void )
	{
		PRECACHE_MODEL("models/w_gauss.mdl");
	}
	BOOL MyTouch( CBasePlayer *pPlayer )
	{
		return FALSE;
	}
};
LINK_ENTITY_TO_CLASS( weapon_gauss, CGauss );


class CGaussAmmo : public CItem
{
	void Spawn( void )
	{ 
		Precache( );
		SET_MODEL(ENT(pev), "models/w_gaussammo.mdl");
		CItem::Spawn( );
		UTIL_MakeProp( this );
	}
	void Precache( void )
	{
		PRECACHE_MODEL ("models/w_gaussammo.mdl");
		PRECACHE_SOUND("items/9mmclip1.wav");

		PRECACHE_SOUND("weapons/gauss2.wav");
		PRECACHE_SOUND("weapons/electro4.wav");
		PRECACHE_SOUND("weapons/electro5.wav");
		PRECACHE_SOUND("weapons/electro6.wav");
		PRECACHE_SOUND("ambience/pulsemachine.wav");
	
		PRECACHE_MODEL( "sprites/hotglow.spr" );
		PRECACHE_MODEL( "sprites/hotglow.spr" );
		PRECACHE_MODEL( "sprites/smoke.spr" );
	}
	BOOL MyTouch( CBasePlayer *pPlayer )
	{
		return FALSE;
	}
};
LINK_ENTITY_TO_CLASS( ammo_gaussclip, CGaussAmmo );


#endif

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

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"
#include "soundent.h"
#include "gamerules.h"
#include "items.h"

#define SF_MP5_LYING	2	// POV, name ours: no hull, laid on its side

// POV: the MP5 and its ammo are props that can't be taken; the clip
// precaches what the SDK's gun did
class CMP5 : public CItem
{
public:
	void Spawn( void )
	{
		Precache( );
		SET_MODEL(ENT(pev), "models/w_9mmAR.mdl");
		CItem::Spawn( );

		if ( pev->spawnflags & SF_MP5_LYING )
		{
			UTIL_SetSize( pev, Vector( 0, 0, 0 ), Vector( 0, 0, 0 ) );
			pev->angles = Vector( pev->angles.x, pev->angles.y, 270 );
		}
		else
			UTIL_MakeProp( this );
	}
	void Precache( void )
	{
		PRECACHE_MODEL("models/w_9mmAR.mdl");
	}
	BOOL MyTouch( CBasePlayer *pPlayer )
	{
		return FALSE;
	}
};
LINK_ENTITY_TO_CLASS( weapon_mp5, CMP5 );
LINK_ENTITY_TO_CLASS( weapon_9mmAR, CMP5 );



class CMP5AmmoClip : public CItem
{
	void Spawn( void )
	{ 
		Precache( );
		SET_MODEL(ENT(pev), "models/w_9mmARclip.mdl");
		CItem::Spawn( );
	}
	void Precache( void )
	{
		PRECACHE_MODEL ("models/w_9mmARclip.mdl");

		PRECACHE_MODEL ("models/shell.mdl");// brass shellTE_MODEL

		PRECACHE_MODEL("models/grenade.mdl");	// grenade

		PRECACHE_MODEL("models/w_9mmARclip.mdl");
		PRECACHE_SOUND("items/9mmclip1.wav");              

		PRECACHE_SOUND("items/clipinsert1.wav");
		PRECACHE_SOUND("items/cliprelease1.wav");

		PRECACHE_SOUND ("weapons/hks1.wav");// H to the K
		PRECACHE_SOUND ("weapons/hks2.wav");// H to the K
		PRECACHE_SOUND ("weapons/hks3.wav");// H to the K

		PRECACHE_SOUND( "weapons/glauncher.wav" );
		PRECACHE_SOUND( "weapons/glauncher2.wav" );

		PRECACHE_SOUND ("weapons/357_cock1.wav");
	}
	BOOL MyTouch( CBasePlayer *pPlayer )
	{
		return FALSE;
	}
};
LINK_ENTITY_TO_CLASS( ammo_mp5clip, CMP5AmmoClip );
LINK_ENTITY_TO_CLASS( ammo_9mmAR, CMP5AmmoClip );



class CMP5AmmoGrenade : public CItem
{
	void Spawn( void )
	{ 
		Precache( );
		SET_MODEL(ENT(pev), "models/w_ARgrenade.mdl");
		CItem::Spawn( );
	}
	void Precache( void )
	{
		PRECACHE_MODEL ("models/w_ARgrenade.mdl");
	}
	BOOL MyTouch( CBasePlayer *pPlayer )
	{
		return FALSE;
	}
};
LINK_ENTITY_TO_CLASS( ammo_mp5grenades, CMP5AmmoGrenade );
LINK_ENTITY_TO_CLASS( ammo_ARgrenades, CMP5AmmoGrenade );


class CMP5Chainammo : public CItem
{
	void Spawn( void )
	{ 
		Precache( );
		SET_MODEL(ENT(pev), "models/w_chainammo.mdl");
		CItem::Spawn( );
	}
	void Precache( void )
	{
		PRECACHE_MODEL ("models/w_chainammo.mdl");
		PRECACHE_SOUND("items/9mmclip1.wav");
	}
	BOOL MyTouch( CBasePlayer *pPlayer )
	{
		return FALSE;
	}
};
LINK_ENTITY_TO_CLASS( ammo_9mmbox, CMP5Chainammo );

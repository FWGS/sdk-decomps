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
#include "gamerules.h"
#include "items.h"

#define SF_SHOTGUN_LYING	2	// POV, name ours: no hull, laid on its side

// POV: the shotgun and its ammo are props that can't be taken
class CShotgun : public CItem
{
public:
	void Spawn( void )
	{
		Precache( );
		SET_MODEL(ENT(pev), "models/w_shotgun.mdl");
		CItem::Spawn( );

		if ( pev->spawnflags & SF_SHOTGUN_LYING )
		{
			UTIL_SetSize( pev, Vector( 0, 0, 0 ), Vector( 0, 0, 0 ) );
			pev->angles.y -= 90;
			pev->angles = Vector( 270, pev->angles.y, pev->angles.z );
		}
		else
			UTIL_MakeProp( this );
	}
	void Precache( void )
	{
		PRECACHE_MODEL("models/w_shotgun.mdl");

		PRECACHE_MODEL ("models/shotgunshell.mdl");// shotgun shell

		PRECACHE_SOUND("items/9mmclip1.wav");              

		PRECACHE_SOUND ("weapons/dbarrel1.wav");//shotgun
		PRECACHE_SOUND ("weapons/sbarrel1.wav");//shotgun

		PRECACHE_SOUND ("weapons/reload1.wav");	// shotgun reload
		PRECACHE_SOUND ("weapons/reload3.wav");	// shotgun reload

		PRECACHE_SOUND ("weapons/357_cock1.wav"); // gun empty sound
		PRECACHE_SOUND ("weapons/scock1.wav");	// cock gun
	}
	BOOL MyTouch( CBasePlayer *pPlayer )
	{
		return FALSE;
	}
};
LINK_ENTITY_TO_CLASS( weapon_shotgun, CShotgun );



class CShotgunAmmo : public CItem
{
	void Spawn( void )
	{ 
		Precache( );
		SET_MODEL(ENT(pev), "models/w_shotbox.mdl");
		CItem::Spawn( );
	}
	void Precache( void )
	{
		PRECACHE_MODEL ("models/w_shotbox.mdl");
	}
	BOOL MyTouch( CBasePlayer *pPlayer )
	{
		return FALSE;
	}
};
LINK_ENTITY_TO_CLASS( ammo_buckshot, CShotgunAmmo );



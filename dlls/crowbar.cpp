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


// POV: the crowbar is a prop that can't be taken
class CCrowbar : public CItem
{
public:
	void Spawn( void );
	void Precache( void );
	BOOL MyTouch( CBasePlayer *pPlayer );
};
LINK_ENTITY_TO_CLASS( weapon_crowbar, CCrowbar );


void CCrowbar::Spawn( )
{
	Precache( );
	SET_MODEL(ENT(pev), "models/w_crowbar.mdl");
	CItem::Spawn( );
	UTIL_MakeProp( this );
}


void CCrowbar::Precache( void )
{
	PRECACHE_MODEL("models/w_crowbar.mdl");
	PRECACHE_SOUND("weapons/cbar_hit1.wav");
	PRECACHE_SOUND("weapons/cbar_hit2.wav");
	PRECACHE_SOUND("weapons/cbar_hitbod1.wav");
	PRECACHE_SOUND("weapons/cbar_hitbod2.wav");
	PRECACHE_SOUND("weapons/cbar_hitbod3.wav");
	PRECACHE_SOUND("weapons/cbar_miss1.wav");
}

BOOL CCrowbar::MyTouch( CBasePlayer *pPlayer )
{
	return FALSE;
}

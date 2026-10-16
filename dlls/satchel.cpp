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

// POV: the satchel is a prop that can't be taken; the charge is gone
class CSatchel : public CItem
{
public:
	void Spawn( void )
	{
		Precache( );
		SET_MODEL(ENT(pev), "models/w_satchel.mdl");
		CItem::Spawn( );
		UTIL_MakeProp( this );
	}
	void Precache( void )
	{
		PRECACHE_MODEL("models/w_satchel.mdl");
		PRECACHE_SOUND("weapons/g_bounce1.wav");
		PRECACHE_SOUND("weapons/g_bounce2.wav");
		PRECACHE_SOUND("weapons/g_bounce3.wav");
	}
	BOOL MyTouch( CBasePlayer *pPlayer )
	{
		return FALSE;
	}
};
LINK_ENTITY_TO_CLASS( weapon_satchel, CSatchel );

#endif

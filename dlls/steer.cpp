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
/*

===== steer.cpp ========================================================

  The steer bones: item_steerskull (models/steerskull.mdl, 40 x 40 x 20
  units) and item_steerribs (models/steerribs.mdl, 80 x 80 x 20 units).
  Like Wanted's other props they are items that give nothing (CItem's
  MyTouch): CItem::Spawn drops them to the floor, then they are made solid.

  The file and class names are ours.

*/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"
#include "items.h"
#include "gamerules.h"

class CItemSteerSkull : public CItem
{
	void Spawn( void )
	{
		CItem::Spawn();
		Precache();
		SET_MODEL( ENT( pev ), "models/steerskull.mdl" );
		UTIL_SetSize( pev, Vector( -20, -20, 0 ), Vector( 20, 20, 20 ) );
		pev->solid = SOLID_BBOX;
		UTIL_SetOrigin( pev, pev->origin );
	}
	void Precache( void )
	{
		PRECACHE_MODEL( "models/steerskull.mdl" );
	}
};

LINK_ENTITY_TO_CLASS( item_steerskull, CItemSteerSkull );


class CItemSteerRibs : public CItem
{
	void Spawn( void )
	{
		CItem::Spawn();
		Precache();
		SET_MODEL( ENT( pev ), "models/steerribs.mdl" );
		UTIL_SetSize( pev, Vector( -40, -40, 0 ), Vector( 40, 40, 20 ) );
		pev->solid = SOLID_BBOX;
		UTIL_SetOrigin( pev, pev->origin );
	}
	void Precache( void )
	{
		PRECACHE_MODEL( "models/steerribs.mdl" );
	}
};

LINK_ENTITY_TO_CLASS( item_steerribs, CItemSteerRibs );

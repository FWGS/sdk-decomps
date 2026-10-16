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

===== bottle.cpp ========================================================

  item_bottle: a bottle (models/bottle.mdl), 16 x 16 x 30 units, solid
  and, unlike the other props, not falling (MOVETYPE_NONE) once
  CItem::Spawn has dropped it to the floor. It gives nothing (CItem's
  MyTouch).

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

class CItemBottle : public CItem
{
	void Spawn( void )
	{
		CItem::Spawn();
		Precache();
		SET_MODEL( ENT( pev ), "models/bottle.mdl" );
		UTIL_SetSize( pev, Vector( -8, -8, 0 ), Vector( 8, 8, 30 ) );
		pev->solid = SOLID_BBOX;
		pev->movetype = MOVETYPE_NONE;
		UTIL_SetOrigin( pev, pev->origin );
	}
	void Precache( void )
	{
		PRECACHE_MODEL( "models/bottle.mdl" );
	}
};

LINK_ENTITY_TO_CLASS( item_bottle, CItemBottle );

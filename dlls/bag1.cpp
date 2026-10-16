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

===== bag1.cpp ========================================================

  item_bag1: a bag (models/w_bag1.mdl), 30 x 30 x 45 units. Like
  Wanted's other props it is an item that gives nothing (CItem's
  MyTouch): CItem::Spawn drops it to the floor, then it is made solid.

  Spawn and Precache are written in the class body: retail has them
  right after the factory, and after them the inline virtuals that
  no earlier object needed (CBaseEntity's KeyValue, GetToggleState
  and GetDelay, CItem's MyTouch), which the props share from here.

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

class CItemBag1 : public CItem
{
	void Spawn( void )
	{
		CItem::Spawn();
		Precache();
		SET_MODEL( ENT( pev ), "models/w_bag1.mdl" );
		UTIL_SetSize( pev, Vector( -15, -15, 0 ), Vector( 15, 15, 45 ) );
		pev->solid = SOLID_BBOX;
		UTIL_SetOrigin( pev, pev->origin );
	}
	void Precache( void )
	{
		PRECACHE_MODEL( "models/w_bag1.mdl" );
	}
};

LINK_ENTITY_TO_CLASS( item_bag1, CItemBag1 );

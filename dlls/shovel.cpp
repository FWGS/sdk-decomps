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

===== shovel.cpp ========================================================

  item_shovel: a shovel (models/shovel.mdl). Its box runs from -16 to 1
  and -16 to 2 across (retail's numbers) and is 32 units high.
  Like Wanted's other props it is an item that gives nothing (CItem's
  MyTouch): CItem::Spawn drops it to the floor, then it is made solid.

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

class CItemShovel : public CItem
{
	void Spawn( void )
	{
		CItem::Spawn();
		Precache();
		SET_MODEL( ENT( pev ), "models/shovel.mdl" );
		UTIL_SetSize( pev, Vector( -16, -16, 0 ), Vector( 1, 2, 32 ) );	// sic: not centred
		pev->solid = SOLID_BBOX;
		UTIL_SetOrigin( pev, pev->origin );
	}
	void Precache( void )
	{
		PRECACHE_MODEL( "models/shovel.mdl" );
	}
};

LINK_ENTITY_TO_CLASS( item_shovel, CItemShovel );

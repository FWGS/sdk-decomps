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

// Wanted: item_pickaxe, a pickaxe lying about as decoration. The file and
// class names are ours; like Wanted's other decoration items (bag, bottle,
// cactus, dish, glass, ...) it has a file of its own, sorted by the item's
// name. It is a solid CItem that can't be picked up (MyTouch stays FALSE).

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "items.h"

class CItemPickaxe : public CItem
{
	void Spawn( void );
	void Precache( void );
};
LINK_ENTITY_TO_CLASS( item_pickaxe, CItemPickaxe );

void CItemPickaxe::Spawn( void )
{
	CItem::Spawn( );
	Precache( );
	SET_MODEL( ENT(pev), "models/pickaxe.mdl" );
	UTIL_SetSize( pev, Vector( -16, -16, 0 ), Vector( 1, 2, 32 ) );	// sic: lopsided box
	pev->solid = SOLID_BBOX;
	UTIL_SetOrigin( pev, pev->origin );
}

void CItemPickaxe::Precache( void )
{
	PRECACHE_MODEL( "models/pickaxe.mdl" );
}

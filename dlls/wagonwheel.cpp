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

===== wagonwheel.cpp ========================================================

  item_wagonwheel: a standing wagon wheel (models/wagonwheel.mdl, 64 x 64
  x 74 units). The wheel itself isn't solid; Spawn puts five invisible
  blockers in a row along its right vector, 13 units apart and centred
  on it: plain CBaseEntity objects without a classname, with
  dynamite.mdl's model index (drawn with EF_NODRAW), 20 x 20 x 72 units,
  SOLID_SLIDEBOX and MOVETYPE_FLY.

  The file, class and helper names are ours.

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

class CItemWagonWheel : public CBaseEntity
{
	void Spawn( void );
	void Precache( void );
	void SpawnBlocker( Vector vecOrigin );
};

LINK_ENTITY_TO_CLASS( item_wagonwheel, CItemWagonWheel );

void CItemWagonWheel::Spawn( void )
{
	Precache();
	SET_MODEL( ENT( pev ), "models/wagonwheel.mdl" );
	pev->mins = Vector( -32, -32, 0 );
	pev->maxs = Vector( 32, 32, 74 );
	UTIL_SetSize( pev, pev->mins, pev->maxs );

	MAKE_VECTORS( pev->angles );
	for ( int i = -2; i < 3; i++ )
		SpawnBlocker( pev->origin + gpGlobals->v_right * i * 13 );
}

void CItemWagonWheel::Precache( void )
{
	PRECACHE_MODEL( "models/wagonwheel.mdl" );
	PRECACHE_MODEL( "models/dynamite.mdl" );
}

void CItemWagonWheel::SpawnBlocker( Vector vecOrigin )
{
	CBaseEntity *pBlocker = GetClassPtr( (CBaseEntity *)NULL );

	pBlocker->pev->modelindex = MODEL_INDEX( "models/dynamite.mdl" );
	pBlocker->pev->effects = EF_NODRAW;
	pBlocker->pev->origin = vecOrigin;
	pBlocker->pev->mins = Vector( -10, -10, 0 );
	pBlocker->pev->maxs = Vector( 10, 10, 72 );
	UTIL_SetSize( pBlocker->pev, pBlocker->pev->mins, pBlocker->pev->maxs );
	pBlocker->pev->solid = SOLID_SLIDEBOX;
	pBlocker->pev->movetype = MOVETYPE_FLY;
}

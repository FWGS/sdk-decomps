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

===== herbs.cpp ========================================================

  item_herbs: healing herbs (models/herbs.mdl), healthkit.cpp's
  CHealthKit again. Touching them heals sk_herbs points
  (gSkillData.herbsCapacity) if the player is hurt, plays
  items/herbs.wav, and starts the player's "radiation" timer with a
  -1 point DMG_RADIATION hit: Wanted's player.cpp turns radiation into
  healing (RADIATION_DAMAGE -1 every 2 seconds, RADIATION_DURATION 10
  times), so the herbs go on healing for about 20 seconds.

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

extern int gmsgItemPickup;

class CItemHerbs : public CItem
{
	void Spawn( void );
	void Precache( void );
	BOOL MyTouch( CBasePlayer *pPlayer );
};


LINK_ENTITY_TO_CLASS( item_herbs, CItemHerbs );

void CItemHerbs :: Spawn( void )
{
	Precache( );
	SET_MODEL(ENT(pev), "models/herbs.mdl");

	CItem::Spawn();
}

void CItemHerbs::Precache( void )
{
	PRECACHE_MODEL("models/herbs.mdl");
	PRECACHE_SOUND("items/herbs.wav");
}

BOOL CItemHerbs::MyTouch( CBasePlayer *pPlayer )
{
	if ( pPlayer->TakeHealth( gSkillData.herbsCapacity, DMG_GENERIC ) )
	{
		// heal over time: the "radiation" timer heals in Wanted
		pPlayer->TakeDamage( pev, pev, -1, DMG_RADIATION );

		MESSAGE_BEGIN( MSG_ONE, gmsgItemPickup, NULL, pPlayer->pev );
			WRITE_STRING( STRING(pev->classname) );
		MESSAGE_END();

		EMIT_SOUND(ENT(pPlayer->pev), CHAN_ITEM, "items/herbs.wav", 1, ATTN_NORM);

		if ( g_pGameRules->ItemShouldRespawn( this ) )
		{
			Respawn();
		}
		else
		{
			UTIL_Remove(this);	
		}

		return TRUE;
	}

	return FALSE;
}

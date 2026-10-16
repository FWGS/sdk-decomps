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

===== elixer.cpp ========================================================

  item_elixer: the medicine bottle (models/medicine.mdl). A copy of
  healthkit.cpp's CHealthKit: touching it heals sk_elixer points
  (gSkillData.elixerCapacity) if the player is hurt, and plays
  items/medicine.wav.

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

class CItemElixer : public CItem
{
	void Spawn( void );
	void Precache( void );
	BOOL MyTouch( CBasePlayer *pPlayer );
};


LINK_ENTITY_TO_CLASS( item_elixer, CItemElixer );

void CItemElixer :: Spawn( void )
{
	Precache( );
	SET_MODEL(ENT(pev), "models/medicine.mdl");

	CItem::Spawn();
}

void CItemElixer::Precache( void )
{
	PRECACHE_MODEL("models/medicine.mdl");
	PRECACHE_SOUND("items/medicine.wav");
}

BOOL CItemElixer::MyTouch( CBasePlayer *pPlayer )
{
	if ( pPlayer->TakeHealth( gSkillData.elixerCapacity, DMG_GENERIC ) )
	{
		MESSAGE_BEGIN( MSG_ONE, gmsgItemPickup, NULL, pPlayer->pev );
			WRITE_STRING( STRING(pev->classname) );
		MESSAGE_END();

		EMIT_SOUND(ENT(pPlayer->pev), CHAN_ITEM, "items/medicine.wav", 1, ATTN_NORM);

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

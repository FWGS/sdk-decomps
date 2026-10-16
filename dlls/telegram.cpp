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

===== telegram.cpp ========================================================

  item_telegram: the letters. The map gives the model; "useimage" names
  the picture the client shows, "readdelay" how long it shows it (seconds,
  default 30) and "delay" how long after that the targets fire.

  A player who touches a letter is frozen (EnableControl( FALSE ), with
  a maxspeed of 0.001 and no velocity), hears items/gunpickup2.wav and
  gets VGUIMenu 3 with the image name: Wanted's client shows the letter
  in place of the class menu. The letter hides itself (EF_NODRAW,
  SOLID_NOT), remembers the reader as its owner, and after readdelay
  (CloseThink) sends VGUIMenu 0 and gives the player back control; the
  client's "closetelegram" command does the same at once through the
  DamageDecal override (client.cpp). "delay" seconds later the letter
  fires its targets with the reader as activator and removes itself.

  The file, class and think names are ours.

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

extern int gmsgVGUIMenu;

class CItemTelegram : public CBaseEntity
{
public:
	void Spawn( void );
	void Precache( void );
	void KeyValue( KeyValueData *pkvd );
	void Touch( CBaseEntity *pOther );
	int DamageDecal( int bitsDamageType );	// closes the letter (client.cpp's "closetelegram")

	void EXPORT CloseThink( void );
	void EXPORT UseThink( void );
};

LINK_ENTITY_TO_CLASS( item_telegram, CItemTelegram );

void CItemTelegram::Spawn( void )
{
	Precache();
	SET_MODEL( ENT( pev ), STRING( pev->model ) );
	UTIL_SetSize( pev, Vector( -16, -16, 0 ), Vector( 16, 16, 16 ) );
	pev->solid = SOLID_TRIGGER;

	if ( pev->speed == 0 )
		pev->speed = 30;

	DROP_TO_FLOOR( ENT( pev ) );
}

void CItemTelegram::Touch( CBaseEntity *pOther )
{
	if ( !pOther || !pOther->IsPlayer() )
		return;

	CBasePlayer *pPlayer = (CBasePlayer *)pOther;

	pPlayer->pev->maxspeed = 0.001;
	pPlayer->EnableControl( FALSE );
	pPlayer->pev->velocity = g_vecZero;
	pPlayer->pev->flags |= FL_ONGROUND;

	EMIT_SOUND( ENT( pPlayer->pev ), CHAN_ITEM, "items/gunpickup2.wav", 1, ATTN_NORM );

	MESSAGE_BEGIN( MSG_ONE, gmsgVGUIMenu, NULL, pPlayer->pev );
		WRITE_BYTE( 3 );
		WRITE_STRING( STRING( pev->noise ) );
	MESSAGE_END();

	pev->owner = pPlayer->edict();
	pev->solid = SOLID_NOT;
	pev->maxspeed = gpGlobals->time + 0.5;	// never read
	pev->startpos = pPlayer->pev->v_angle;	// never read
	pev->effects |= EF_NODRAW;
	pev->nextthink = gpGlobals->time + pev->speed;
	SetThink( CloseThink );
}

void CItemTelegram::Precache( void )
{
	PRECACHE_MODEL( (char *)STRING( pev->model ) );
	PRECACHE_SOUND( "items/gunpickup2.wav" );
}

void CItemTelegram::KeyValue( KeyValueData *pkvd )
{
	if ( FStrEq( pkvd->szKeyName, "readdelay" ) )
	{
		pev->speed = atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "useimage" ) )
	{
		pev->noise = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "delay" ) )
	{
		pev->dmg = atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		CBaseEntity::KeyValue( pkvd );
}

int CItemTelegram::DamageDecal( int bitsDamageType )
{
	CBasePlayer *pPlayer = (CBasePlayer *)GET_PRIVATE( pev->owner );

	if ( pPlayer )
	{
		pPlayer->EnableControl( TRUE );
		pPlayer->pev->maxspeed = 0;

		MESSAGE_BEGIN( MSG_ONE, gmsgVGUIMenu, NULL, pPlayer->pev );
			WRITE_BYTE( 0 );
		MESSAGE_END();

		pev->nextthink = gpGlobals->time + pev->dmg;
		SetThink( UseThink );
	}

	return 0;
}

void CItemTelegram::CloseThink( void )
{
	CBasePlayer *pPlayer = (CBasePlayer *)GET_PRIVATE( pev->owner );

	if ( pPlayer )
	{
		pPlayer->EnableControl( TRUE );
		pPlayer->pev->maxspeed = 0;

		MESSAGE_BEGIN( MSG_ONE, gmsgVGUIMenu, NULL, pPlayer->pev );
			WRITE_BYTE( 0 );
		MESSAGE_END();

		pev->nextthink = gpGlobals->time + pev->dmg;
		SetThink( UseThink );
	}
}

void CItemTelegram::UseThink( void )
{
	CBaseEntity *pPlayer = (CBaseEntity *)GET_PRIVATE( pev->owner );

	if ( pPlayer )
		SUB_UseTargets( pPlayer, USE_TOGGLE, 0 );

	SUB_Remove();
}

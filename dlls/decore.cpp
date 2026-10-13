/***
*
*	Copyright (c) 1999, Valve LLC. All rights reserved.
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
//=========================================================
// decorations: props that sit in the level, animate, gib
// and react to the player; the embryos in their tubes, the
// lab clutter, the asteroids and space debris, and the
// training bot
//=========================================================

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "schedule.h"
#include "effects.h"
#include "soundent.h"
#include "decals.h"
#include "weapons.h"

//=========================================================
// CDecore - the base of the decorations: a monster that
// stands in place, animates and notices players touching it
//=========================================================
class CDecore : public CBaseMonster
{
public:
	void KeyValue( KeyValueData *pkvd );
	int TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );

	// a player touched the decoration (slot 139)
	virtual int PlayerTouch( CBaseEntity *pOther );

	void DecoreInit( void );
	void EXPORT DecoreTouch( CBaseEntity *pOther );
	void EXPORT Animate( void );

	int m_iAnimate;		// +0x270
	int m_maxFrame;		// +0x274
	int m_iUnused[3];	// +0x278
	int m_fAnimEnd;		// +0x284: CBigFootCrush waits for it, nothing sets it
	int m_iUnused2;		// +0x288
};

// ==== decore part A: CDecore .. CBigFootCrush (fork A) ====

void CDecore::DecoreInit( void )
{
	pev->takedamage = DAMAGE_YES;
	pev->movetype = MOVETYPE_NONE;
	pev->solid = SOLID_BBOX;
	UTIL_SetOrigin( pev, pev->origin );
	UTIL_SetSize( pev, Vector( -16, -16, 0 ), Vector( 16, 16, 16 ) );
	SetTouch( DecoreTouch );
	pev->nextthink = gpGlobals->time + 0.1;
	SetThink( Animate );
	pev->health = 20;
	ResetSequenceInfo( );
}

void CDecore::DecoreTouch( CBaseEntity *pOther )
{
	if ( pOther && ( pOther->pev->flags & FL_CLIENT ) )
		PlayerTouch( pOther );
}

void CDecore::Animate( void )
{
	StudioFrameAdvance( );
	pev->nextthink = gpGlobals->time + 0.1;
}

void CDecore::KeyValue( KeyValueData *pkvd )
{
	if ( FStrEq( pkvd->szKeyName, "frame" ) )
	{
		pev->frame = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		pev->frame = 0;	// sic: every other key is dropped
}

// ==== end of part A ====

// ==== decore part B: CXenomeEmbryo .. CSpaceDebris (fork B) ====

// CDecore's own defaults sit here in retail, between the tube embryo and
// the lab stuff.
int CDecore::PlayerTouch( CBaseEntity *pOther )
{
	return 0;
}

int CDecore::TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	return CBaseMonster::TakeDamage( pevInflictor, pevAttacker, flDamage, bitsDamageType );
}

// ==== end of part B ====

// ==== decore part C: CDummyBot (fork C) ====

// ==== end of part C ====

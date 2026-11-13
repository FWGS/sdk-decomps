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

===== generic grenade.cpp ========================================================

*/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "soundent.h"
#include "decals.h"
#include "player.h"
#include "gamerules.h"

extern void Broadcast( const char *sentence );	// multiplay_gamerules.cpp (Beta 3.0)
extern int gmsgItemPickup;	// Beta 4.1 (player.cpp)
extern int gmsgBarTime;	// Beta 5.0 (leak) (player.cpp)


//===================grenade


LINK_ENTITY_TO_CLASS( grenade, CGrenade );

// Grenades flagged with this will be triggered when the owner calls detonateSatchelCharges
#define SF_DETONATE		0x0001

//
// Grenade Explode
//
void CGrenade::Explode( Vector vecSrc, Vector vecAim )
{
	TraceResult tr;
	UTIL_TraceLine ( pev->origin, pev->origin + Vector ( 0, 0, -32 ),  ignore_monsters, ENT(pev), & tr);

	Explode( &tr, DMG_BLAST );
}

// UNDONE: temporary scorching for PreAlpha - find a less sleazy permenant solution.
void CGrenade::Explode( TraceResult *pTrace, int bitsDamageType )
{
	float		flRndSound;// sound randomizer

	pev->model = iStringNull;//invisible
	pev->solid = SOLID_NOT;// intangible

	pev->takedamage = DAMAGE_NO;

	// Pull out of the wall a bit
	if ( pTrace->flFraction != 1.0 )
	{
		pev->origin = pTrace->vecEndPos + (pTrace->vecPlaneNormal * (pev->dmg - 24) * 0.6);
	}

	int iContents = UTIL_PointContents ( pev->origin );

	// Counter-Strike Beta 6.0: no more shrapnel (Beta 3.0 to 5.2: five or
	// seven CDebris thrown around the horizon)

/*
	MESSAGE_BEGIN( MSG_PAS, SVC_TEMPENTITY, pev->origin );
		WRITE_BYTE( TE_EXPLOSION );		// This makes a dynamic light and the explosion sprites/sound
		WRITE_COORD( pev->origin.x );	// Send to PAS because of the sound
		WRITE_COORD( pev->origin.y );
		WRITE_COORD( pev->origin.z );
		if (iContents != CONTENTS_WATER)
		{
			WRITE_SHORT( g_sModelIndexFireball );
		}
		else
		{
			WRITE_SHORT( g_sModelIndexWExplosion );
		}
		WRITE_BYTE( (pev->dmg - 50) * .60  ); // scale * 10
		WRITE_BYTE( 15  ); // framerate
		WRITE_BYTE( TE_EXPLFLAG_NONE );
	MESSAGE_END();
*/

	CSoundEnt::InsertSound ( bits_SOUND_COMBAT, pev->origin, NORMAL_EXPLOSION_VOLUME, 3.0 );
	entvars_t *pevOwner;
	if ( pev->owner )
		pevOwner = VARS( pev->owner );
	else
		pevOwner = NULL;

	pev->owner = NULL; // can't traceline attack owner if this is set

	RadiusFlash ( pev->origin, pev, pevOwner, 4, CLASS_NONE, bitsDamageType );

	if ( RANDOM_FLOAT( 0 , 1 ) < 0.5 )
	{
		UTIL_DecalTrace( pTrace, DECAL_SCORCH1 );
	}
	else
	{
		UTIL_DecalTrace( pTrace, DECAL_SCORCH2 );
	}

	flRndSound = RANDOM_FLOAT( 0 , 1 );

	// (an if, not Beta 3.0's switch with a default: with inline expansion on, retail calls
	// the Vector copy constructor three times in the origin's sum above, which Visual C++
	// 6.0 does only while this function stays within its smallest inlining budget; the
	// switch makes it a few units too long. A ?: for pevOwner or the spark count as the
	// loop's own variable would do the same; the sounds are the part that is not SDK text.)
	if ( RANDOM_LONG( 0, 1 ) )
		EMIT_SOUND(ENT(pev), CHAN_VOICE, "weapons/flashbang-1.wav", 0.55, ATTN_NORM);
	else
		EMIT_SOUND(ENT(pev), CHAN_VOICE, "weapons/flashbang-2.wav", 0.55, ATTN_NORM);

	pev->effects |= EF_NODRAW;
	SetThink( Smoke );
	pev->velocity = g_vecZero;
	pev->nextthink = gpGlobals->time + 0.3;

	if (iContents != CONTENTS_WATER)
	{
		int sparkCount = RANDOM_LONG(0,3);
		for ( int i = 0; i < sparkCount; i++ )
			Create( "spark_shower", pev->origin, pTrace->vecPlaneNormal, NULL );
	}
}


//
// Counter-Strike Beta 4.0: the C4 goes off (Detonate2). Near an info_bomb_target
// (256 units) the target counts as bombed; the round is checked at once.
// Four sprites (one at the spot, three up to 512 units around it), the shake,
// 5 times pev->dmg as radius damage, then Smoke2 0.85 seconds later.
//
void CGrenade::Explode2( TraceResult *pTrace, int bitsDamageType )
{
	float		flRndSound;// sound randomizer
	CHalfLifeMultiplay *mp = (CHalfLifeMultiplay *)g_pGameRules;	// Beta 6.0 (name ours)

	pev->model = iStringNull;//invisible
	pev->solid = SOLID_NOT;// intangible

	pev->takedamage = DAMAGE_NO;

	UTIL_ScreenShake( pTrace->vecEndPos, 25.0, 150.0, 1.0, 3000 );

	// Beta 5.0 (leak): a C4 is planted only at a bomb target, so it always counts
	// (Beta 4.0 and 4.1: within 256 units of an info_bomb_target)
	mp->m_fTargetBombed = true;

	m_fC4Done = TRUE;
	mp->CheckWinConditions();

	// Pull out of the wall a bit
	if ( pTrace->flFraction != 1.0 )
	{
		pev->origin = pTrace->vecEndPos + (pTrace->vecPlaneNormal * (pev->dmg - 24) * 0.6);
	}

	int iContents = UTIL_PointContents ( pev->origin );

	MESSAGE_BEGIN( MSG_PAS, SVC_TEMPENTITY, pev->origin );
		WRITE_BYTE( TE_SPRITE );
		WRITE_COORD( pev->origin.x );
		WRITE_COORD( pev->origin.y );
		WRITE_COORD( pev->origin.z - 10 );
		WRITE_SHORT( g_sModelIndexFireball3 );
		WRITE_BYTE( (pev->dmg - 275) * 0.6 );	// scale
		WRITE_BYTE( 150 );	// brightness
	MESSAGE_END();

	MESSAGE_BEGIN( MSG_PAS, SVC_TEMPENTITY, pev->origin );
		WRITE_BYTE( TE_SPRITE );
		WRITE_COORD( pev->origin.x + RANDOM_FLOAT( -512, 512 ) );
		WRITE_COORD( pev->origin.y + RANDOM_FLOAT( -512, 512 ) );
		WRITE_COORD( pev->origin.z + RANDOM_FLOAT( -10, 10 ) );
		WRITE_SHORT( g_sModelIndexFireball2 );
		WRITE_BYTE( (pev->dmg - 275) * 0.6 );
		WRITE_BYTE( 150 );
	MESSAGE_END();

	MESSAGE_BEGIN( MSG_PAS, SVC_TEMPENTITY, pev->origin );
		WRITE_BYTE( TE_SPRITE );
		WRITE_COORD( pev->origin.x + RANDOM_FLOAT( -512, 512 ) );
		WRITE_COORD( pev->origin.y + RANDOM_FLOAT( -512, 512 ) );
		WRITE_COORD( pev->origin.z + RANDOM_FLOAT( -10, 10 ) );
		WRITE_SHORT( g_sModelIndexFireball3 );
		WRITE_BYTE( (pev->dmg - 275) * 0.6 );
		WRITE_BYTE( 150 );
	MESSAGE_END();

	MESSAGE_BEGIN( MSG_PAS, SVC_TEMPENTITY, pev->origin );
		WRITE_BYTE( TE_SPRITE );
		WRITE_COORD( pev->origin.x + RANDOM_FLOAT( -512, 512 ) );
		WRITE_COORD( pev->origin.y + RANDOM_FLOAT( -512, 512 ) );
		WRITE_COORD( pev->origin.z + RANDOM_FLOAT( -10, 10 ) );
		WRITE_SHORT( g_sModelIndexFireball );
		WRITE_BYTE( (pev->dmg - 275) * 0.6 );
		WRITE_BYTE( 17 );
	MESSAGE_END();

	EMIT_SOUND(ENT(pev), CHAN_WEAPON, "weapons/c4_explode1.wav", 1.0, 0.25);

	CSoundEnt::InsertSound ( bits_SOUND_COMBAT, pev->origin, NORMAL_EXPLOSION_VOLUME, 3.0 );
	entvars_t *pevOwner;
	if ( pev->owner )
		pevOwner = VARS( pev->owner );
	else
		pevOwner = NULL;

	pev->owner = NULL; // can't traceline attack owner if this is set

	RadiusDamage ( pev, pevOwner, mp->m_flBombRadius, CLASS_NONE, bitsDamageType );	// Beta 6.0: the map's bomb radius (info_map_parameters; 500 by default) (was pev->dmg x 5)

	if ( RANDOM_FLOAT( 0 , 1 ) < 0.5 )
	{
		UTIL_DecalTrace( pTrace, DECAL_SCORCH1 );
	}
	else
	{
		UTIL_DecalTrace( pTrace, DECAL_SCORCH2 );
	}

	flRndSound = RANDOM_FLOAT( 0 , 1 );

	switch ( RANDOM_LONG( 0, 2 ) )
	{
		case 0:	EMIT_SOUND(ENT(pev), CHAN_VOICE, "weapons/debris1.wav", 0.55, ATTN_NORM);	break;
		case 1:	EMIT_SOUND(ENT(pev), CHAN_VOICE, "weapons/debris2.wav", 0.55, ATTN_NORM);	break;
		case 2:	EMIT_SOUND(ENT(pev), CHAN_VOICE, "weapons/debris3.wav", 0.55, ATTN_NORM);	break;
	}

	pev->effects |= EF_NODRAW;
	SetThink( Smoke2 );
	pev->velocity = g_vecZero;
	pev->nextthink = gpGlobals->time + 0.85;

	if (iContents != CONTENTS_WATER)
	{
		int sparkCount = RANDOM_LONG(0,3);
		for ( int i = 0; i < sparkCount; i++ )
			Create( "spark_shower", pev->origin, pTrace->vecPlaneNormal, NULL );
	}
}


//
// Counter-Strike Beta 4.0: the HE grenade goes off (Detonate3): two
// explosions, the radius damage of pev->dmg, then Smoke3_C 0.55 seconds later
//
void CGrenade::Explode3( TraceResult *pTrace, int bitsDamageType )
{
	float		flRndSound;// sound randomizer

	pev->model = iStringNull;//invisible
	pev->solid = SOLID_NOT;// intangible

	pev->takedamage = DAMAGE_NO;

	// Pull out of the wall a bit
	if ( pTrace->flFraction != 1.0 )
	{
		pev->origin = pTrace->vecEndPos + (pTrace->vecPlaneNormal * (pev->dmg - 24) * 0.6);
	}

	MESSAGE_BEGIN( MSG_PAS, SVC_TEMPENTITY, pev->origin );
		WRITE_BYTE( TE_EXPLOSION );		// This makes a dynamic light and the explosion sprites/sound
		WRITE_COORD( pev->origin.x );	// Send to PAS because of the sound
		WRITE_COORD( pev->origin.y );
		WRITE_COORD( pev->origin.z + 20 );
		WRITE_SHORT( g_sModelIndexFireball3 );
		WRITE_BYTE( 25 ); // scale * 10
		WRITE_BYTE( 30  ); // framerate
		WRITE_BYTE( TE_EXPLFLAG_NONE );
	MESSAGE_END();

	MESSAGE_BEGIN( MSG_PAS, SVC_TEMPENTITY, pev->origin );
		WRITE_BYTE( TE_EXPLOSION );
		WRITE_COORD( pev->origin.x + RANDOM_FLOAT( -64, 64 ) );
		WRITE_COORD( pev->origin.y + RANDOM_FLOAT( -64, 64 ) );
		WRITE_COORD( pev->origin.z + RANDOM_FLOAT( 30, 35 ) );
		WRITE_SHORT( g_sModelIndexFireball2 );
		WRITE_BYTE( 30 ); // scale * 10
		WRITE_BYTE( 30  ); // framerate
		WRITE_BYTE( TE_EXPLFLAG_NONE );
	MESSAGE_END();

	CSoundEnt::InsertSound ( bits_SOUND_COMBAT, pev->origin, NORMAL_EXPLOSION_VOLUME, 3.0 );
	entvars_t *pevOwner;
	if ( pev->owner )
		pevOwner = VARS( pev->owner );
	else
		pevOwner = NULL;

	pev->owner = NULL; // can't traceline attack owner if this is set

	RadiusDamage ( pev, pevOwner, pev->dmg, CLASS_NONE, bitsDamageType );

	if ( RANDOM_FLOAT( 0 , 1 ) < 0.5 )
	{
		UTIL_DecalTrace( pTrace, DECAL_SCORCH1 );
	}
	else
	{
		UTIL_DecalTrace( pTrace, DECAL_SCORCH2 );
	}

	flRndSound = RANDOM_FLOAT( 0 , 1 );

	switch ( RANDOM_LONG( 0, 2 ) )
	{
		case 0:	EMIT_SOUND(ENT(pev), CHAN_VOICE, "weapons/debris1.wav", 0.55, ATTN_NORM);	break;
		case 1:	EMIT_SOUND(ENT(pev), CHAN_VOICE, "weapons/debris2.wav", 0.55, ATTN_NORM);	break;
		case 2:	EMIT_SOUND(ENT(pev), CHAN_VOICE, "weapons/debris3.wav", 0.55, ATTN_NORM);	break;
	}

	pev->effects |= EF_NODRAW;
	SetThink( Smoke3_C );
	pev->velocity = g_vecZero;
	pev->nextthink = gpGlobals->time + 0.55;

	int sparkCount = RANDOM_LONG(0,3);
	for ( int i = 0; i < sparkCount; i++ )
		Create( "spark_shower", pev->origin, pTrace->vecPlaneNormal, NULL );
}


//
// Counter-Strike Beta 6.5: the flashbang's Explode without the flash, thinking
// SG_Smoke next (nothing calls it: SG_Detonate starts the smoke itself)
//
void CGrenade::SG_Explode( TraceResult *pTrace, int bitsDamageType )
{
	float		flRndSound;// sound randomizer

	pev->model = iStringNull;//invisible
	pev->solid = SOLID_NOT;// intangible

	pev->takedamage = DAMAGE_NO;

	// Pull out of the wall a bit
	if ( pTrace->flFraction != 1.0 )
	{
		pev->origin = pTrace->vecEndPos + (pTrace->vecPlaneNormal * (pev->dmg - 24) * 0.6);
	}

	int iContents = UTIL_PointContents ( pev->origin );

	CSoundEnt::InsertSound ( bits_SOUND_COMBAT, pev->origin, NORMAL_EXPLOSION_VOLUME, 3.0 );
	entvars_t *pevOwner;
	if ( pev->owner )
		pevOwner = VARS( pev->owner );
	else
		pevOwner = NULL;

	pev->owner = NULL; // can't traceline attack owner if this is set

	if ( RANDOM_FLOAT( 0 , 1 ) < 0.5 )
	{
		UTIL_DecalTrace( pTrace, DECAL_SCORCH1 );
	}
	else
	{
		UTIL_DecalTrace( pTrace, DECAL_SCORCH2 );
	}

	flRndSound = RANDOM_FLOAT( 0 , 1 );

	if ( RANDOM_LONG( 0, 1 ) )
		EMIT_SOUND(ENT(pev), CHAN_VOICE, "weapons/flashbang-1.wav", 0.55, ATTN_NORM);
	else
		EMIT_SOUND(ENT(pev), CHAN_VOICE, "weapons/flashbang-2.wav", 0.55, ATTN_NORM);

	SetThink( SG_Smoke );
	pev->nextthink = gpGlobals->time + 0.1;

	if (iContents != CONTENTS_WATER)
	{
		int sparkCount = RANDOM_LONG(0,3);
		for ( int i = 0; i < sparkCount; i++ )
			Create( "spark_shower", pev->origin, pTrace->vecPlaneNormal, NULL );
	}
}


// Counter-Strike Beta 4.0: the HE grenade's smoke (Explode3 thinks it)
void CGrenade::Smoke3_C( void )
{
	if (UTIL_PointContents ( pev->origin ) == CONTENTS_WATER)
	{
		UTIL_Bubbles( pev->origin - Vector( 64, 64, 64 ), pev->origin + Vector( 64, 64, 64 ), 100 );
	}
	else
	{
		MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, pev->origin );
			WRITE_BYTE( TE_SMOKE );
			WRITE_COORD( pev->origin.x );
			WRITE_COORD( pev->origin.y );
			WRITE_COORD( pev->origin.z - 5 );
			WRITE_SHORT( g_sModelIndexSmoke );
			WRITE_BYTE( RANDOM_FLOAT( 0, 10 ) + 35 ); // scale * 10
			WRITE_BYTE( 5  ); // framerate
		MESSAGE_END();
	}
	UTIL_Remove( this );
}


// Counter-Strike Beta 4.0: a puff up to 128 units off, then Smoke3_A (nothing thinks it)
void CGrenade::Smoke3_B( void )
{
	if (UTIL_PointContents ( pev->origin ) == CONTENTS_WATER)
	{
		UTIL_Bubbles( pev->origin - Vector( 64, 64, 64 ), pev->origin + Vector( 64, 64, 64 ), 100 );
	}
	else
	{
		MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, pev->origin );
			WRITE_BYTE( TE_SMOKE );
			WRITE_COORD( pev->origin.x + RANDOM_FLOAT( -128, 128 ) );
			WRITE_COORD( pev->origin.y + RANDOM_FLOAT( -128, 128 ) );
			WRITE_COORD( pev->origin.z + RANDOM_FLOAT( -10, 10 ) );
			WRITE_SHORT( g_sModelIndexSmoke );
			WRITE_BYTE( RANDOM_FLOAT( 0, 10 ) + 15 ); // scale * 10
			WRITE_BYTE( 10  ); // framerate
		MESSAGE_END();
	}

	pev->nextthink = gpGlobals->time + 0.15;
	SetThink( Smoke3_A );
}


// Counter-Strike Beta 4.0: a puff up to 128 units off (Smoke3_B thinks it)
void CGrenade::Smoke3_A( void )
{
	if (UTIL_PointContents ( pev->origin ) == CONTENTS_WATER)
	{
		UTIL_Bubbles( pev->origin - Vector( 64, 64, 64 ), pev->origin + Vector( 64, 64, 64 ), 100 );
	}
	else
	{
		MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, pev->origin );
			WRITE_BYTE( TE_SMOKE );
			WRITE_COORD( pev->origin.x + RANDOM_FLOAT( -128, 128 ) );
			WRITE_COORD( pev->origin.y + RANDOM_FLOAT( -128, 128 ) );
			WRITE_COORD( pev->origin.z + RANDOM_FLOAT( -10, 10 ) );
			WRITE_SHORT( g_sModelIndexSmoke );
			WRITE_BYTE( RANDOM_FLOAT( 0, 10 ) + 15 ); // scale * 10
			WRITE_BYTE( 12  ); // framerate
		MESSAGE_END();
	}
	UTIL_Remove( this );
}


// Counter-Strike Beta 4.0: the C4's smoke (Explode2 thinks it)
void CGrenade::Smoke2( void )
{
	if (UTIL_PointContents ( pev->origin ) == CONTENTS_WATER)
	{
		UTIL_Bubbles( pev->origin - Vector( 64, 64, 64 ), pev->origin + Vector( 64, 64, 64 ), 100 );
	}
	else
	{
		MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, pev->origin );
			WRITE_BYTE( TE_SMOKE );
			WRITE_COORD( pev->origin.x );
			WRITE_COORD( pev->origin.y );
			WRITE_COORD( pev->origin.z );
			WRITE_SHORT( g_sModelIndexSmoke );
			WRITE_BYTE( 150 ); // scale * 10
			WRITE_BYTE( 8  ); // framerate
		MESSAGE_END();
	}
	UTIL_Remove( this );
}


void CGrenade::Smoke( void )
{
	if (UTIL_PointContents ( pev->origin ) == CONTENTS_WATER)
	{
		UTIL_Bubbles( pev->origin - Vector( 64, 64, 64 ), pev->origin + Vector( 64, 64, 64 ), 100 );
	}
	else
	{
		MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, pev->origin );
			WRITE_BYTE( TE_SMOKE );
			WRITE_COORD( pev->origin.x );
			WRITE_COORD( pev->origin.y );
			WRITE_COORD( pev->origin.z );
			WRITE_SHORT( g_sModelIndexSmoke );
			WRITE_BYTE( 25 ); // scale * 10 (Beta 3.0; was (pev->dmg - 50) * 0.80)
			WRITE_BYTE( 6  ); // framerate
		MESSAGE_END();
	}
	UTIL_Remove( this );
}

//
// Counter-Strike Beta 6.5: the smoke grenade smokes: once a second it plays
// its event (the client draws the puffs, EV_CreateSmoke) with a direction that
// turns 30 degrees each time, 22 times (14 for a light one), then it goes.
// The locals' names are Counter-Strike 1.6's (debug information).
//
void CGrenade::SG_Smoke( void )
{
	if (UTIL_PointContents ( pev->origin ) == CONTENTS_WATER)
	{
		UTIL_Bubbles( pev->origin - Vector( 64, 64, 64 ), pev->origin + Vector( 64, 64, 64 ), 100 );
	}
	else
	{
		Vector origin, angle;	// (angle is not used; without it the code differs by three instructions)
		float x_old, y_old, R_angle;

		UTIL_MakeVectors( pev->angles );

		origin = gpGlobals->v_forward * RANDOM_FLOAT( 5, 10 );

		R_angle = m_angle * ( 3.141517 / 180 );
		m_angle = ( m_angle + 30 ) % 360;

		x_old = origin.x;
		y_old = origin.y;
		origin.x = x_old * cos( R_angle ) - y_old * sin( R_angle );
		origin.y = x_old * sin( R_angle ) + y_old * cos( R_angle );

		origin.z = RANDOM_FLOAT( 0, 500 );

		PLAYBACK_EVENT_FULL( 0, NULL, m_usEvent, 0.0, pev->origin, (float *)&g_vecZero, origin.x, origin.y, origin.z * 10000, 4, m_bLightSmoke, 6 );
	}

	int iMaxSmokePuffs;
	float flSmokeInterval = 1.0;

	if ( m_bLightSmoke == true )
		iMaxSmokePuffs = 12;
	else
		iMaxSmokePuffs = 20;

	if ( m_SGSmoke > iMaxSmokePuffs )
	{
		pev->effects |= EF_NODRAW;
		UTIL_Remove( this );
		return;
	}

	pev->nextthink = gpGlobals->time + flSmokeInterval;
	SetThink( SG_Smoke );
	m_SGSmoke++;
}

void CGrenade::Killed( entvars_t *pevAttacker, int iGib )
{
	Detonate( );
}


// Timed grenade, this think is called when time runs out.
void CGrenade::DetonateUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	SetThink( Detonate );
	pev->nextthink = gpGlobals->time;
}

void CGrenade::PreDetonate( void )
{
	CSoundEnt::InsertSound ( bits_SOUND_DANGER, pev->origin, 400, 0.3 );

	SetThink( Detonate );
	pev->nextthink = gpGlobals->time + 1;
}


void CGrenade::Detonate( void )
{
	TraceResult tr;
	Vector		vecSpot;// trace starts here!

	vecSpot = pev->origin + Vector ( 0 , 0 , 8 );
	UTIL_TraceLine ( vecSpot, vecSpot + Vector ( 0, 0, -40 ),  ignore_monsters, ENT(pev), & tr);

	Explode( &tr, DMG_BLAST );
}


//
// Counter-Strike Beta 6.5: the smoke grenade goes off (SG_TumbleThink): its
// sound, and SG_Smoke from the next tenth of a second on. Another "grenade"
// within 250 units whose time has passed (a smoke grenade still smoking; a
// planted C4, whose dmgtime is never set) makes this one's smoke light.
// (the trace is Detonate's and not used; pentFind, pEnt, fDistance: CS 1.6's names)
//
void CGrenade::SG_Detonate( void )
{
	TraceResult tr;
	Vector		vecSpot;// trace starts here!

	vecSpot = pev->origin + Vector ( 0 , 0 , 8 );
	UTIL_TraceLine ( vecSpot, vecSpot + Vector ( 0, 0, -40 ),  ignore_monsters, ENT(pev), & tr);

	EMIT_SOUND(ENT(pev), CHAN_WEAPON, "weapons/sg_explode.wav", 1.0, ATTN_NORM);

	edict_t *pentFind = FIND_ENTITY_BY_CLASSNAME( NULL, "grenade" );
	while ( !FNullEnt( pentFind ) )
	{
		CBaseEntity *pEnt = CBaseEntity::Instance( pentFind );
		if ( pEnt )
		{
			float fDistance = ( pEnt->pev->origin - pev->origin ).Length();
			if ( fDistance != 0 && fDistance <= 250 )
			{
				if ( pEnt->pev->dmgtime < gpGlobals->time )
					m_bLightSmoke = true;
			}
		}
		pentFind = FIND_ENTITY_BY_CLASSNAME( pentFind, "grenade" );
	}

	m_bDetonated = true;
	pev->nextthink = gpGlobals->time + 0.1;
	SetThink( SG_Smoke );
}


// Counter-Strike Beta 4.0: the C4 (C4Think)
void CGrenade::Detonate2( void )
{
	TraceResult tr;
	Vector		vecSpot;// trace starts here!

	vecSpot = pev->origin + Vector ( 0 , 0 , 8 );
	UTIL_TraceLine ( vecSpot, vecSpot + Vector ( 0, 0, -40 ),  ignore_monsters, ENT(pev), & tr);

	Explode2( &tr, DMG_BLAST );
}


// Counter-Strike Beta 4.0: the HE grenade (TumbleThink)
void CGrenade::Detonate3( void )
{
	TraceResult tr;
	Vector		vecSpot;// trace starts here!

	vecSpot = pev->origin + Vector ( 0 , 0 , 8 );
	UTIL_TraceLine ( vecSpot, vecSpot + Vector ( 0, 0, -40 ),  ignore_monsters, ENT(pev), & tr);

	Explode3( &tr, DMG_HEGRENADE );
}


//
// Contact grenade, explode when it touches something
// 
void CGrenade::ExplodeTouch( CBaseEntity *pOther )
{
	TraceResult tr;
	Vector		vecSpot;// trace starts here!

	pev->enemy = pOther->edict();

	vecSpot = pev->origin - pev->velocity.Normalize() * 32;
	UTIL_TraceLine( vecSpot, vecSpot + pev->velocity.Normalize() * 64, ignore_monsters, ENT(pev), &tr );

	Explode( &tr, DMG_BLAST );
}


void CGrenade::DangerSoundThink( void )
{
	if (!IsInWorld())
	{
		UTIL_Remove( this );
		return;
	}

	CSoundEnt::InsertSound ( bits_SOUND_DANGER, pev->origin + pev->velocity * 0.5, pev->velocity.Length( ), 0.2 );
	pev->nextthink = gpGlobals->time + 0.2;

	if (pev->waterlevel != 0)
	{
		pev->velocity = pev->velocity * 0.5;
	}
}


void CGrenade::BounceTouch( CBaseEntity *pOther )
{
	// don't hit the guy that launched this grenade
	if ( pOther->edict() == pev->owner )
		return;

	// Counter-Strike Beta 6.0: off see-through breakables (glass, which the
	// grenade's touch breaks: CBreakable::BreakTouch) it flies back twice as fast
	if ( !strcmp( STRING( pOther->pev->classname ), "func_breakable" ) && pOther->pev->rendermode != kRenderNormal )
	{
		pev->velocity = pev->velocity * -2;
		return;
	}

/*
	// only do damage if we're moving fairly fast
	if (m_flNextAttack < gpGlobals->time && pev->velocity.Length() > 100)
	{
		entvars_t *pevOwner = VARS( pev->owner );
		if (pevOwner)
		{
			TraceResult tr = UTIL_GetGlobalTrace( );
			ClearMultiDamage( );
			pOther->TraceAttack(pevOwner, 1, gpGlobals->v_forward, &tr, DMG_CLUB ); 
			ApplyMultiDamage( pev, pevOwner);
		}
		m_flNextAttack = gpGlobals->time + 1.0; // debounce
	}
*/

	Vector vecTestVelocity;
	// pev->avelocity = Vector (300, 300, 300);

	// this is my heuristic for modulating the grenade velocity because grenades dropped purely vertical
	// or thrown very far tend to slow down too quickly for me to always catch just by testing velocity. 
	// trimming the Z velocity a bit seems to help quite a bit.
	vecTestVelocity = pev->velocity; 
	vecTestVelocity.z *= 0.7;	// Beta 4.0 (was 0.45)

	if ( !m_fRegisteredSound && vecTestVelocity.Length() <= 60 )
	{
		//ALERT( at_console, "Grenade Registered!: %f\n", vecTestVelocity.Length() );

		// grenade is moving really slow. It's probably very close to where it will ultimately stop moving. 
		// go ahead and emit the danger sound.
		
		// register a radius louder than the explosion, so we make sure everyone gets out of the way
		CSoundEnt::InsertSound ( bits_SOUND_DANGER, pev->origin, pev->dmg / 0.4, 0.3 );
		m_fRegisteredSound = TRUE;
	}

	if (pev->flags & FL_ONGROUND)
	{
		// add a bit of static friction
		pev->velocity = pev->velocity * 0.8;

		pev->sequence = RANDOM_LONG( 1, 1 );
	}
	else
	{
		// play bounce sound
		BounceSound();
	}
	pev->framerate = pev->velocity.Length() / 200.0;
	if (pev->framerate > 1.0)
		pev->framerate = 1;
	else if (pev->framerate < 0.5)
		pev->framerate = 0;

}



void CGrenade::SlideTouch( CBaseEntity *pOther )
{
	// don't hit the guy that launched this grenade
	if ( pOther->edict() == pev->owner )
		return;

	// pev->avelocity = Vector (300, 300, 300);

	if (pev->flags & FL_ONGROUND)
	{
		// add a bit of static friction
		pev->velocity = pev->velocity * 0.95;

		if (pev->velocity.x != 0 || pev->velocity.y != 0)
		{
			// maintain sliding sound
		}
	}
	else
	{
		BounceSound();
	}
}

void CGrenade :: BounceSound( void )
{
	// Beta 4.0: a grenade of more than 50 damage (the HE grenade) has its own sound
	if ( pev->dmg > 50 )
	{
		EMIT_SOUND(ENT(pev), CHAN_VOICE, "weapons/he_bounce-1.wav", 0.25, ATTN_NORM);
		return;
	}

	switch ( RANDOM_LONG( 0, 2 ) )
	{
	case 0:	EMIT_SOUND(ENT(pev), CHAN_VOICE, "weapons/grenade_hit1.wav", 0.25, ATTN_NORM);	break;
	case 1:	EMIT_SOUND(ENT(pev), CHAN_VOICE, "weapons/grenade_hit2.wav", 0.25, ATTN_NORM);	break;
	case 2:	EMIT_SOUND(ENT(pev), CHAN_VOICE, "weapons/grenade_hit3.wav", 0.25, ATTN_NORM);	break;
	}
}

void CGrenade :: TumbleThink( void )
{
	if (!IsInWorld())
	{
		UTIL_Remove( this );
		return;
	}

	StudioFrameAdvance( );
	pev->nextthink = gpGlobals->time + 0.1;

	if (pev->dmgtime - 1 < gpGlobals->time)
	{
		CSoundEnt::InsertSound ( bits_SOUND_DANGER, pev->origin + pev->velocity * (pev->dmgtime - gpGlobals->time), 400, 0.1 );
	}

	if (pev->dmgtime <= gpGlobals->time)
	{
		// Beta 4.0: more than 40 damage (the HE grenade) explodes as one
		if ( pev->dmg > 40 )
			SetThink( Detonate3 );
		else
			SetThink( Detonate );
	}
	if (pev->waterlevel != 0)
	{
		pev->velocity = pev->velocity * 0.5;
		pev->framerate = 0.2;
	}
}


// Counter-Strike Beta 6.5: the smoke grenade in flight (ShootSmokeGrenade); TumbleThink's text
void CGrenade :: SG_TumbleThink( void )
{
	if (!IsInWorld())
	{
		UTIL_Remove( this );
		return;
	}

	StudioFrameAdvance( );
	pev->nextthink = gpGlobals->time + 0.1;

	if (pev->dmgtime - 1 < gpGlobals->time)
	{
		CSoundEnt::InsertSound ( bits_SOUND_DANGER, pev->origin + pev->velocity * (pev->dmgtime - gpGlobals->time), 400, 0.1 );
	}

	if (pev->dmgtime <= gpGlobals->time)
	{
		SetThink( SG_Detonate );
	}
	if (pev->waterlevel != 0)
	{
		pev->velocity = pev->velocity * 0.5;
		pev->framerate = 0.2;
	}
}


void CGrenade:: Spawn( void )
{
	pev->movetype = MOVETYPE_BOUNCE;
	pev->classname = MAKE_STRING( "grenade" );
	m_bIsC4 = false;	// Beta 5.0 (leak)
	
	pev->solid = SOLID_BBOX;

	SET_MODEL(ENT(pev), "models/grenade.mdl");
	UTIL_SetSize(pev, Vector( 0, 0, 0), Vector(0, 0, 0));

	pev->dmg = 30;	// Beta 3.0 (was 100)
	m_fRegisteredSound = FALSE;
}


CGrenade *CGrenade::ShootContact( entvars_t *pevOwner, Vector vecStart, Vector vecVelocity )
{
	CGrenade *pGrenade = GetClassPtr( (CGrenade *)NULL );
	pGrenade->Spawn();
	// contact grenades arc lower
	pGrenade->pev->gravity = 0.5;// lower gravity since grenade is aerodynamic and engine doesn't know it.
	UTIL_SetOrigin( pGrenade->pev, vecStart );
	pGrenade->pev->velocity = vecVelocity;
	pGrenade->pev->angles = UTIL_VecToAngles (pGrenade->pev->velocity);
	pGrenade->pev->owner = ENT(pevOwner);
	
	// make monsters afaid of it while in the air
	pGrenade->SetThink( DangerSoundThink );
	pGrenade->pev->nextthink = gpGlobals->time;
	
	// Tumble in air
	pGrenade->pev->avelocity.x = RANDOM_FLOAT ( -100, -500 );
	
	// Explode on contact
	pGrenade->SetTouch( ExplodeTouch );

	pGrenade->pev->dmg = gSkillData.plrDmgM203Grenade;
	pGrenade->m_fC4Done = TRUE;	// Beta 4.0: no bomb

	return pGrenade;
}


//
// Counter-Strike Beta 4.0: the HE grenade, ShootTimed's text: 75 damage, the
// thrower's team (an HE grenade does not hurt his team mates)
//
CGrenade * CGrenade:: ShootTimed2( entvars_t *pevOwner, Vector vecStart, Vector vecVelocity, float time, int iTeam )
{
	CGrenade *pGrenade = GetClassPtr( (CGrenade *)NULL );
	pGrenade->Spawn();
	UTIL_SetOrigin( pGrenade->pev, vecStart );
	pGrenade->pev->velocity = vecVelocity * 1;
	pGrenade->pev->angles = pevOwner->angles;
	pGrenade->pev->owner = ENT(pevOwner);
	
	pGrenade->SetTouch( BounceTouch );	// Bounce if touched
	
	pGrenade->pev->dmgtime = gpGlobals->time + time;
	pGrenade->SetThink( TumbleThink );
	pGrenade->pev->nextthink = gpGlobals->time + 0.1;
		
	pGrenade->pev->sequence = RANDOM_LONG( 3, 6 );
	pGrenade->pev->framerate = 1.0;
	pGrenade->m_fC4Done = TRUE;

	pGrenade->pev->gravity = 0.55;
	pGrenade->pev->friction = 0.7;
	pGrenade->m_iTeam = iTeam;

	SET_MODEL(ENT(pGrenade->pev), "models/w_hegrenade.mdl");
	pGrenade->pev->dmg = 100;	// Beta 5.0 (leak) (Beta 4.0 and 4.1: 75)

	return pGrenade;
}


CGrenade * CGrenade:: ShootTimed( entvars_t *pevOwner, Vector vecStart, Vector vecVelocity, float time )
{
	CGrenade *pGrenade = GetClassPtr( (CGrenade *)NULL );
	pGrenade->Spawn();
	UTIL_SetOrigin( pGrenade->pev, vecStart );
	pGrenade->pev->velocity = vecVelocity;
	pGrenade->pev->angles = pevOwner->angles;	// Beta 3.0: the thrower's angles (was the flight direction)
	pGrenade->pev->owner = ENT(pevOwner);
	
	pGrenade->SetTouch( BounceTouch );	// Bounce if touched
	
	// Take one second off of the desired detonation time and set the think to PreDetonate. PreDetonate
	// will insert a DANGER sound into the world sound list and delay detonation for one second so that 
	// the grenade explodes after the exact amount of time specified in the call to ShootTimed(). 

	pGrenade->pev->dmgtime = gpGlobals->time + time;
	pGrenade->SetThink( TumbleThink );
	pGrenade->pev->nextthink = gpGlobals->time + 0.1;
	if (time < 0.1)
	{
		pGrenade->pev->nextthink = gpGlobals->time;
		pGrenade->pev->velocity = Vector( 0, 0, 0 );
	}
		
	pGrenade->pev->sequence = RANDOM_LONG( 3, 6 );
	pGrenade->pev->framerate = 1.0;
	pGrenade->m_fC4Done = TRUE;	// Beta 4.0: no bomb

	// Tumble through the air
	// pGrenade->pev->avelocity.x = -400;

	pGrenade->pev->gravity = 0.5;
	pGrenade->pev->friction = 0.8;

	SET_MODEL(ENT(pGrenade->pev), "models/w_flashbang.mdl");
	pGrenade->pev->dmg = 35;	// Beta 3.0 (was 100)

	return pGrenade;
}

//
// Beta 5.0 (leak): a counter-terrorist defuses the planted C4 by holding USE on it
// (FCAP_CONTINUOUS_USE): 5 seconds with a defuse kit, 10 without; the progress
// bar shows it (BarTime). C4Think checks that he goes on using it.
//
void CGrenade::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if ( m_bIsC4 )
	{
		CBasePlayer *player = GetClassPtr( (CBasePlayer *)pActivator->pev );	// name ours

		if ( player->m_iTeam == TEAM_CT )
		{
			if ( !m_bStartDefuse )
			{
				if ( player->m_fHasDefuser == true )
				{
					ClientPrint( player->pev, HUD_PRINTCENTER, "Defusing bomb WITH Defuse kit..." );
					EMIT_SOUND( ENT(player->pev), CHAN_ITEM, "weapons/c4_disarm.wav", 1.0, ATTN_NORM );
					m_pDefuser = pActivator;
					m_bStartDefuse = true;
					m_flDefuseDone = gpGlobals->time + 5.0;
					m_flNextDefuseCheck = gpGlobals->time + 0.5;

					MESSAGE_BEGIN( MSG_ONE, gmsgBarTime, NULL, player->pev );
						WRITE_SHORT( 5 );
					MESSAGE_END();
				}
				else
				{
					ClientPrint( player->pev, HUD_PRINTCENTER, "Defusing bomb WITHOUT Defuse kit..." );
					EMIT_SOUND( ENT(player->pev), CHAN_ITEM, "weapons/c4_disarm.wav", 1.0, ATTN_NORM );
					m_pDefuser = pActivator;
					m_bStartDefuse = true;
					m_flDefuseDone = gpGlobals->time + 10.0;
					m_flNextDefuseCheck = gpGlobals->time + 0.5;

					MESSAGE_BEGIN( MSG_ONE, gmsgBarTime, NULL, player->pev );
						WRITE_SHORT( 10 );
					MESSAGE_END();
				}
			}
			else
			{
				m_flNextDefuseCheck = gpGlobals->time + 0.5;
			}
		}
	}
}


//
// Counter-Strike Beta 4.0: the planted C4 (CBasePlayer::PlantC4). It goes off
// mp_c4timer seconds later and beeps from the start every eighth of that
// time, faster by 1 % every tenth of a second (never more often than ten
// times a second); 100 damage, Explode2 makes it 500.
//
CGrenade * CGrenade :: ShootSatchelCharge( entvars_t *pevOwner, Vector vecStart, Vector vecVelocity )
{
	CGrenade *pGrenade = GetClassPtr( (CGrenade *)NULL );
	pGrenade->pev->movetype = MOVETYPE_TOSS;	// Beta 5.0 (leak) (was MOVETYPE_BOUNCE)
	pGrenade->pev->classname = MAKE_STRING( "grenade" );
	
	pGrenade->pev->solid = SOLID_BBOX;

	SET_MODEL(ENT(pGrenade->pev), "models/w_c4.mdl");

	UTIL_SetSize(pGrenade->pev, Vector( 0, 0, 0), Vector(8, 8, 8));

	pGrenade->pev->dmg = 100;
	UTIL_SetOrigin( pGrenade->pev, vecStart );
	pGrenade->pev->velocity = g_vecZero;		// Beta 5.0 (leak): it lies where it is planted,
	pGrenade->pev->angles = vecVelocity;		// turned the way the planter looked (CC4 passes his angles)
	pGrenade->pev->owner = ENT(pevOwner);
	
	// Detonate in "time" seconds
	pGrenade->SetThink( C4Think );
	pGrenade->SetTouch( C4Touch );
	pGrenade->pev->spawnflags = SF_DETONATE;

	pGrenade->pev->nextthink = gpGlobals->time + 0.1;
	pGrenade->m_flC4Blow = gpGlobals->time + ((CHalfLifeMultiplay *)g_pGameRules)->m_iC4Timer;
	// Beta 5.0: five beeps in turn, each lasting 10 % less than the last,
	// the first a quarter of the timer (the leak: one beep, faster and faster)
	pGrenade->m_flBeepInterval = ((CHalfLifeMultiplay *)g_pGameRules)->m_iC4Timer / 4;
	pGrenade->m_flNextBeepChange = gpGlobals->time;
	pGrenade->m_flNextBeep = gpGlobals->time + 0.5;
	pGrenade->m_iBeepStep = 0;
	pGrenade->m_flBeepAttenuation = 0;
	pGrenade->m_pszBeepSound = NULL;
	pGrenade->m_flNextBlink = gpGlobals->time + 2.0;
	pGrenade->m_bIsC4 = true;
	pGrenade->m_flNextDefuseCheck = 0;
	pGrenade->m_bStartDefuse = false;

	pGrenade->pev->friction = 0.9;
	pGrenade->m_fC4Done = false;

	// the bomb target the planter stands in is used when the C4 goes off
	CBaseEntity *pOwner = CBaseEntity::Instance( ENT( pevOwner ) );	// name ours
	if ( pOwner && pOwner->IsPlayer() )
	{
		pGrenade->m_pentBombTarget = ((CBasePlayer *)pOwner)->m_pentBombTarget;
		return pGrenade;
	}

	pGrenade->m_pentBombTarget = NULL;
	return pGrenade;
}


//
// Counter-Strike Beta 6.5: the smoke grenade (CSmokeGrenade), ShootTimed's text:
// 35 damage (BounceSound's and TumbleThink's limits read it), 3 units large, the
// event SG_Smoke plays
//
CGrenade * CGrenade:: ShootSmokeGrenade( entvars_t *pevOwner, Vector vecStart, Vector vecVelocity, float time, unsigned short usEvent )
{
	CGrenade *pGrenade = GetClassPtr( (CGrenade *)NULL );
	pGrenade->Spawn();
	UTIL_SetOrigin( pGrenade->pev, vecStart );
	pGrenade->pev->velocity = vecVelocity;
	pGrenade->pev->angles = pevOwner->angles;
	pGrenade->pev->owner = ENT(pevOwner);
	pGrenade->m_usEvent = usEvent;
	pGrenade->m_bLightSmoke = false;
	pGrenade->m_bDetonated = false;
	
	pGrenade->SetTouch( BounceTouch );	// Bounce if touched
	
	pGrenade->pev->dmgtime = gpGlobals->time + time;
	pGrenade->SetThink( SG_TumbleThink );
	pGrenade->pev->nextthink = gpGlobals->time + 0.1;
	if (time < 0.1)
	{
		pGrenade->pev->nextthink = gpGlobals->time;
		pGrenade->pev->velocity = Vector( 0, 0, 0 );
	}
		
	pGrenade->pev->sequence = RANDOM_LONG( 3, 6 );
	pGrenade->pev->framerate = 1.0;
	pGrenade->m_fC4Done = TRUE;

	pGrenade->pev->gravity = 0.5;
	pGrenade->pev->friction = 0.8;
	pGrenade->m_SGSmoke = 0;

	SET_MODEL(ENT(pGrenade->pev), "models/w_smokegrenade.mdl");
	pGrenade->pev->dmg = 35;
	UTIL_SetSize(pGrenade->pev, Vector( 0, 0, 0), Vector(3, 3, 3));

	return pGrenade;
}


//
// Counter-Strike Beta 4.0: the planted C4 beeps, louder (smaller attenuation)
// as the beeps come faster; at its time it explodes (Detonate2), in water it
// is just removed. A defuse that has run 5 seconds succeeds if the defuser
// is still within 64 units, alive and ducking: BOMBDEF to everyone, the
// round is checked.
//
void CGrenade::C4Think( void )
{
	if (!IsInWorld())
	{
		UTIL_Remove( this );
		return;
	}

	pev->nextthink = gpGlobals->time + 0.12;	// Beta 5.0 (leak) (was 0.1)

	// Beta 5.0: five beeps, each carrying farther (a smaller attenuation)
	// than the last, each lasting 10 % less; a beep every 1.4 seconds (the
	// leak: one beep, faster and louder at every think)
	if ( gpGlobals->time >= m_flNextBeepChange )
	{
		m_flNextBeepChange = gpGlobals->time + m_flBeepInterval;
		m_flBeepInterval *= 0.9;

		switch ( m_iBeepStep )
		{
		case 0:
			m_pszBeepSound = "weapons/c4_beep1.wav";
			m_flBeepAttenuation = 1.5;
			break;
		case 1:
			m_pszBeepSound = "weapons/c4_beep2.wav";
			m_flBeepAttenuation = 1.0;
			break;
		case 2:
			m_pszBeepSound = "weapons/c4_beep3.wav";
			m_flBeepAttenuation = 0.8;
			break;
		case 3:
			m_pszBeepSound = "weapons/c4_beep4.wav";
			m_flBeepAttenuation = 0.5;
			break;
		case 4:
			m_pszBeepSound = "weapons/c4_beep5.wav";
			m_flBeepAttenuation = 0.2;
			break;
		}

		m_iBeepStep++;
	}

	if ( gpGlobals->time >= m_flNextBeep )
	{
		m_flNextBeep = gpGlobals->time + 1.4;
		EMIT_SOUND(ENT(pev), CHAN_VOICE, m_pszBeepSound, 1.0, m_flBeepAttenuation);
	}

	// Beta 5.0 (leak): the C4 glows every two seconds
	if ( gpGlobals->time >= m_flNextBlink )
	{
		m_flNextBlink = gpGlobals->time + 2.0;

		MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, pev->origin );	// Beta 6.0: to those who can see it (was every client)
			WRITE_BYTE( TE_GLOWSPRITE );
			WRITE_COORD( pev->origin.x );
			WRITE_COORD( pev->origin.y );
			WRITE_COORD( pev->origin.z + 5 );
			WRITE_SHORT( g_sModelIndexC4Glow );
			WRITE_BYTE( 1 );	// life
			WRITE_BYTE( 3 );	// scale
			WRITE_BYTE( 255 );	// brightness
		MESSAGE_END();
	}

	if ( m_flC4Blow <= gpGlobals->time )
	{
		// Beta 5.0 (leak): the bomb target the C4 was planted in is used
		if ( m_pentBombTarget )
		{
			CBaseEntity *pBombTarget = CBaseEntity::Instance( m_pentBombTarget );	// name ours
			if ( pBombTarget )
				pBombTarget->Use( CBaseEntity::Instance( pev->owner ), this, USE_TOGGLE, 0 );
		}

		if ( pev->waterlevel == 0 )
			SetThink( Detonate2 );
		else
			UTIL_Remove( this );
	}

	// Beta 5.0 (leak): the defuser must keep using the C4 (CGrenade::Use); he may
	// move, crouch or not, and needs no kit
	if ( m_bStartDefuse == true && m_pDefuser )
	{
		if ( m_flDefuseDone > gpGlobals->time )
		{
			if ( m_flNextDefuseCheck < gpGlobals->time )
			{
				// he let go: the progress bar goes and the defusing stops
				MESSAGE_BEGIN( MSG_ONE, gmsgBarTime, NULL, m_pDefuser->pev );
					WRITE_BYTE( 0 );
				MESSAGE_END();

				m_pDefuser = NULL;
				m_bStartDefuse = false;
				m_flDefuseDone = 0;
			}
		}
		else
		{
			if ( m_pDefuser->pev->deadflag == DEAD_NO )
			{
				Broadcast( "BOMBDEF" );
				UTIL_LogPrintf( "%s has defused the bomb\n", STRING( m_pDefuser->pev->netname ) );	// Beta 6.6
				// Beta 5.0: the last beep, still sounding, is stopped
				UTIL_EmitAmbientSound( ENT(pev), pev->origin, "weapons/c4_beep5.wav", 0, ATTN_NONE, SND_STOP, 0 );
				EMIT_SOUND(ENT(m_pDefuser->pev), CHAN_WEAPON, "weapons/c4_disarmed.wav", 1.0, ATTN_NORM);
				UTIL_Remove( this );
				m_fC4Done = TRUE;
				((CHalfLifeMultiplay *)g_pGameRules)->m_fBombDefused = true;
				((CHalfLifeMultiplay *)g_pGameRules)->CheckWinConditions();
				m_pDefuser = NULL;
				m_bStartDefuse = false;
				return;
			}

			// Beta 5.0: a defuser who died at the end lets the next one start
			// afresh (the leak left the defuse running)
			m_bStartDefuse = false;
			m_pDefuser = NULL;
		}
	}
}


//
// Counter-Strike Beta 4.0: a ducking counter-terrorist with a defuser who
// touches the planted C4 starts defusing it (5 seconds).
// Beta 5.0 (leak): touching the planted C4 does nothing (Beta 4.0 and 4.1: a
// crouching CT with a defuser defused it, Beta 4.1: a crouching terrorist took
// it back away from the bomb targets); it is used to defuse it
void CGrenade::C4Touch( CBaseEntity *pOther )
{
}



void CGrenade :: UseSatchelCharges( entvars_t *pevOwner, SATCHELCODE code )
{
	edict_t *pentFind;
	edict_t *pentOwner;

	if ( !pevOwner )
		return;

	CBaseEntity	*pOwner = CBaseEntity::Instance( pevOwner );

	pentOwner = pOwner->edict();

	pentFind = FIND_ENTITY_BY_CLASSNAME( NULL, "grenade" );
	while ( !FNullEnt( pentFind ) )
	{
		CBaseEntity *pEnt = Instance( pentFind );
		if ( pEnt )
		{
			if ( FBitSet( pEnt->pev->spawnflags, SF_DETONATE ) && pEnt->pev->owner == pentOwner )
			{
				if ( code == SATCHEL_DETONATE )
					pEnt->Use( pOwner, pOwner, USE_ON, 0 );
				else	// SATCHEL_RELEASE
					pEnt->pev->owner = NULL;
			}
		}
		pentFind = FIND_ENTITY_BY_CLASSNAME( pentFind, "grenade" );
	}
}

//======================end grenade


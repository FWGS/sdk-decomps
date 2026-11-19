/***
*
*	Copyright (c) 1996-2001, Valve LLC. All rights reserved.
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


//===================grenade


LINK_ENTITY_TO_CLASS( grenade, CGrenade );

// Scientist Slaughterhouse's grenade types, set in spawnflags by the weapons that throw
// or fire them (names ours)
#define SF_GRENADE_INCENDIARY	0x0001	// FireExplode: fire sprite, burn damage
#define SF_GRENADE_FLAMES		0x0002	// FireExplode leaves flames
#define SF_GRENADE_BLASTSPHERE	0x0008	// the blastsphere event with the explosion
#define SF_GRENADE_ENERGY		0x0010	// SphereBlast instead of an explosion
#define SF_GRENADE_MORTAR		0x0020	// an explosion, then MortarBlast
#define SF_GRENADE_KEEPOWNER	0x0100	// pev->owner is kept through the damage
#define SF_GRENADE_NOEFFECTS	0x0200	// damage only, removed right away

//
// Grenade Explode
//
void CGrenade::Explode( Vector vecSrc, Vector vecAim )
{
	TraceResult tr;
	UTIL_TraceLine ( pev->origin, pev->origin + Vector ( 0, 0, -32 ),  ignore_monsters, ENT(pev), & tr);

	if ( pev->spawnflags & SF_GRENADE_INCENDIARY )
		FireExplode( &tr );
	else if ( HasSpawnFlag( SF_GRENADE_ENERGY ) )
		SphereBlast();
	else if ( HasSpawnFlag( SF_GRENADE_MORTAR ) )
	{
		Explode( &tr, DMG_BLAST );
		MortarBlast();
	}
	else
		Explode( &tr, DMG_BLAST );
}

// UNDONE: temporary scorching for PreAlpha - find a less sleazy permenant solution.
void CGrenade::Explode( TraceResult *pTrace, int bitsDamageType )
{
	float		flRndSound;// sound randomizer

	pev->model = iStringNull;//invisible
	pev->solid = SOLID_NOT;// intangible

	pev->takedamage = DAMAGE_NO;

	int iContents = UTIL_PointContents ( pev->origin );

	// kill any beam following us
	MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
		WRITE_BYTE( TE_KILLBEAM );
		WRITE_SHORT( entindex() );
	MESSAGE_END();

	if ( HasSpawnFlag( SF_GRENADE_BLASTSPHERE ) && iContents != CONTENTS_WATER )
		PLAYBACK_EVENT_FULL( 0, ENT( pev ), g_iSphereEvent, 0.0, (float *)&pev->origin, (float *)&pev->angles, 0.0, 0.5, pev->dmg * 2, pev->dmg * 2, 0, 0 );

	// Pull out of the wall a bit
	if ( pTrace->flFraction != 1.0 )
	{
		pev->origin = pTrace->vecEndPos + (pTrace->vecPlaneNormal * (pev->dmg - 24) * 0.6);
	}

	int iScale = ( pev->dmg - 50 ) * .60;
	if ( iScale < 10 )
		iScale = 10;

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
		WRITE_BYTE( min( iScale, 255 ) ); // scale * 10
		WRITE_BYTE( 15  ); // framerate
		WRITE_BYTE( TE_EXPLFLAG_NONE );
	MESSAGE_END();

	entvars_t *pevOwner;
	if ( pev->owner )
		pevOwner = VARS( pev->owner );
	else
		pevOwner = NULL;

	if ( !HasSpawnFlag( SF_GRENADE_KEEPOWNER ) )
		pev->owner = NULL; // can't traceline attack owner if this is set

	RadiusDamage ( pev, pevOwner, pev->dmg, CLASS_NONE, bitsDamageType );

	if ( HasSpawnFlag( SF_GRENADE_NOEFFECTS ) )
	{
		SetThink( SUB_Remove );
		pev->nextthink = gpGlobals->time + 0.1;
		return;
	}

	CSoundEnt::InsertSound ( bits_SOUND_COMBAT, pev->origin, NORMAL_EXPLOSION_VOLUME, 3.0 );

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

// Incendiary explosion: fire sprite and burn damage around the origin; with
// SF_GRENADE_FLAMES it leaves flames. Under water it's a plain explosion at half damage.
void CGrenade::FireExplode( TraceResult *pTrace )
{
	pev->model = iStringNull;//invisible
	pev->solid = SOLID_NOT;// intangible

	pev->takedamage = DAMAGE_NO;

	if ( UTIL_PointContents ( pev->origin ) == CONTENTS_WATER )
	{
		pev->dmg *= 0.5;
		Explode( pTrace, DMG_BLAST );
		return;
	}

	MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
		WRITE_BYTE( TE_KILLBEAM );
		WRITE_SHORT( entindex() );
	MESSAGE_END();

	if ( HasSpawnFlag( SF_GRENADE_BLASTSPHERE ) )
		PLAYBACK_EVENT_FULL( 0, ENT( pev ), g_iSphereEvent, 0.0, (float *)&pev->origin, (float *)&pev->angles, 0.0, 0.5, pev->dmg * 2, pev->dmg * 2, 0, 0 );

	// Pull out of the wall a bit
	if ( pTrace->flFraction != 1.0 )
	{
		pev->origin = pTrace->vecEndPos + (pTrace->vecPlaneNormal * (pev->dmg - 24) * 0.6);
	}

	int iScale = ( pev->dmg - 50 ) * .60;
	if ( iScale < 10 )
		iScale = 10;
	else if ( iScale > 255 )
		iScale = 255;

	MESSAGE_BEGIN( MSG_PAS, SVC_TEMPENTITY, pev->origin );
		WRITE_BYTE( TE_EXPLOSION );
		WRITE_COORD( pev->origin.x );
		WRITE_COORD( pev->origin.y );
		WRITE_COORD( pev->origin.z );
		WRITE_SHORT( g_sModelIndexFireExplo );
		WRITE_BYTE( iScale ); // scale * 10
		WRITE_BYTE( 15  ); // framerate
		WRITE_BYTE( TE_EXPLFLAG_NOPARTICLES );
	MESSAGE_END();

	CSoundEnt::InsertSound ( bits_SOUND_COMBAT, pev->origin, NORMAL_EXPLOSION_VOLUME, 3.0 );

	entvars_t *pevOwner = pev;
	if ( pev->owner )
		pevOwner = VARS( pev->owner );

	if ( !HasSpawnFlag( SF_GRENADE_KEEPOWNER ) )
		pev->owner = NULL; // can't traceline attack owner if this is set

	if ( !m_bitsDamageInflict )
		m_bitsDamageInflict = DMG_BURN | DMG_NEVERGIB;

	RadiusDamage ( pev->origin, pev, pevOwner, pev->dmg, CLASS_NONE, m_bitsDamageInflict );

	if ( RANDOM_FLOAT( 0 , 1 ) < 0.5 )
	{
		UTIL_DecalTrace( pTrace, DECAL_SCORCH1 );
	}
	else
	{
		UTIL_DecalTrace( pTrace, DECAL_SCORCH2 );
	}

	switch ( RANDOM_LONG( 0, 2 ) )
	{
		case 0:	EMIT_SOUND(ENT(pev), CHAN_VOICE, "weapons/debris1.wav", 0.55, ATTN_NORM);	break;
		case 1:	EMIT_SOUND(ENT(pev), CHAN_VOICE, "weapons/debris2.wav", 0.55, ATTN_NORM);	break;
		case 2:	EMIT_SOUND(ENT(pev), CHAN_VOICE, "weapons/debris3.wav", 0.55, ATTN_NORM);	break;
	}

	pev->effects |= EF_NODRAW;
	SetThink( Smoke );
	pev->velocity = g_vecZero;
	pev->nextthink = gpGlobals->time + 0.3;

	if ( pev->spawnflags & SF_GRENADE_FLAMES )
	{
		edict_t *pentHit = NULL;
		// sic: FL_MONSTER|FL_CLIENT is always nonzero, so any entity hit is passed
		if ( pTrace->pHit && pTrace->pHit->v.flags & FL_MONSTER|FL_CLIENT )
			pentHit = pTrace->pHit;

		FireSpread( pTrace->vecEndPos + pTrace->vecPlaneNormal * 64, 5, pevOwner, pentHit );
	}
}

// Energy sphere: the blastsphere event and a damage sphere that grows by pev->dmg every
// 0.1 seconds up to twice the damage, for m_flDelay seconds, then fades (HurtThink).
void CGrenade::SphereBlast( void )
{
	pev->solid = SOLID_NOT;// intangible
	pev->takedamage = DAMAGE_NO;

	PLAYBACK_EVENT_FULL( 0, ENT( pev ), g_iSphereEvent, 0.0, (float *)&pev->origin, (float *)&pev->angles, m_flDelay, 0.3, pev->dmg * 2, pev->dmg * 2.5, 0, 0 );
	CSoundEnt::InsertSound ( bits_SOUND_DANGER, pev->origin, pev->dmg * 2, 5.0 );
	EMIT_SOUND( ENT(pev), CHAN_VOICE, "doors/aliendoor1.wav", 1, ATTN_NORM );

	pev->rendermode = kRenderTransColor;
	pev->renderamt = 0;
	SetThink( HurtThink );
	pev->velocity = g_vecZero;
	pev->movetype = MOVETYPE_NONE;
	pev->nextthink = gpGlobals->time + 0.1;

	if ( !m_bitsDamageInflict )
		m_bitsDamageInflict = DMG_ENERGYBEAM;

	pev->dmgtime = gpGlobals->time + m_flDelay;
	pev->frags = pev->dmg * 2;	// largest radius
	pev->speed = 1;				// radius
	pev->dmg *= 0.25;
}

void CGrenade::HurtThink( void )
{
	pev->nextthink = gpGlobals->time + 0.1;

	if ( pev->frags * 2 > pev->speed )
	{
		pev->speed += pev->dmg;
		if ( pev->speed > pev->frags )
			pev->speed = pev->frags;
	}

	float flDamage = pev->dmg;
	if ( pev->dmgtime < gpGlobals->time )
	{
		flDamage -= ( gpGlobals->time - pev->dmgtime ) * pev->dmg * 0.3;
		if ( flDamage <= 0 )
		{
			UTIL_Remove( this );
			return;
		}
	}

	entvars_t *pevOwner = pev;
	if ( pev->owner )
		pevOwner = VARS( pev->owner );

	if ( !HasSpawnFlag( SF_GRENADE_KEEPOWNER ) )
		pev->owner = NULL; // can't traceline attack owner if this is set

	RadiusDamageNoTrace( pev->origin, pev, pevOwner, flDamage * 0.75, pev->speed, CLASS_NONE, m_bitsDamageInflict );

	pev->owner = ENT( pevOwner );
}

// The mortar's energy sphere: a shake, a light and the blastsphere event, then SphereBlast's
// growing damage sphere.
void CGrenade::MortarBlast( void )
{
	pev->solid = SOLID_NOT;// intangible
	pev->takedamage = DAMAGE_NO;

	UTIL_ScreenShakeAll( pev->origin, min( 255, pev->dmg ), 0.4, 4.0 );
	EMIT_SOUND( ENT(pev), CHAN_WEAPON, "weapons/mortarhit.wav", 1.0, ATTN_NONE );

	MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, pev->origin );
		WRITE_BYTE( TE_DLIGHT );
		WRITE_COORD( pev->origin.x );	// X
		WRITE_COORD( pev->origin.y );	// Y
		WRITE_COORD( pev->origin.z );	// Z
		WRITE_BYTE( pev->dmg * 0.4 );	// radius * 0.1
		WRITE_BYTE( 255 );		// r
		WRITE_BYTE( 255 );		// g
		WRITE_BYTE( 128 );		// b
		WRITE_BYTE( 40 );		// time * 10
		WRITE_BYTE( 2 );		// decay * 0.1
	MESSAGE_END( );

	PLAYBACK_EVENT_FULL( 0, ENT( pev ), g_iSphereEvent, 0.0, (float *)&pev->origin, (float *)&pev->angles, m_flDelay, 0.3, pev->dmg * 2, pev->dmg * 2.5, 1, 0 );
	CSoundEnt::InsertSound ( bits_SOUND_DANGER, pev->origin, pev->dmg * 2, 5.0 );

	pev->rendermode = kRenderTransColor;
	pev->renderamt = 0;
	SetThink( HurtThink );
	pev->velocity = g_vecZero;
	pev->movetype = MOVETYPE_NONE;
	pev->nextthink = gpGlobals->time + 0.1;

	if ( !m_bitsDamageInflict )
		m_bitsDamageInflict = DMG_ENERGYBEAM;

	pev->dmgtime = m_flDelay + gpGlobals->time;
	pev->frags = pev->dmg * 2;	// largest radius
	pev->speed = 1;				// radius
	pev->dmg *= 0.25;
}


void CGrenade::Smoke( void )
{
	if (UTIL_PointContents ( pev->origin ) == CONTENTS_WATER)
	{
		UTIL_Bubbles( pev->origin - Vector( 64, 64, 64 ), pev->origin + Vector( 64, 64, 64 ), 100 );
	}
	else
	{
		int iScale = ( pev->dmg - 50 ) * 0.80;
		if ( iScale < 10 )
			iScale = 10;
		else if ( iScale > 255 )
			iScale = 255;

		MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, pev->origin );
			WRITE_BYTE( TE_SMOKE );
			WRITE_COORD( pev->origin.x );
			WRITE_COORD( pev->origin.y );
			WRITE_COORD( pev->origin.z );
			WRITE_SHORT( g_sModelIndexSmoke );
			WRITE_BYTE( iScale ); // scale * 10
			WRITE_BYTE( 12  ); // framerate
		MESSAGE_END();
	}
	UTIL_Remove( this );
}

void CGrenade::MakeSolid( void )
{
	pev->solid = SOLID_BBOX;
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

	if ( pev->spawnflags & SF_GRENADE_INCENDIARY )
		FireExplode( &tr );
	else if ( HasSpawnFlag( SF_GRENADE_ENERGY ) )
		SphereBlast();
	else if ( HasSpawnFlag( SF_GRENADE_MORTAR ) )
	{
		Explode( &tr, DMG_BLAST );
		MortarBlast();
	}
	else
		Explode( &tr, DMG_BLAST );
}


// Explodes where it is, against pev->movedir (ShootExplosion)
void CGrenade::ExDetonate( void )
{
	TraceResult tr;
	Vector		vecSpot;// trace starts here!

	vecSpot = pev->origin - pev->movedir * 8;
	UTIL_TraceLine ( vecSpot, vecSpot + pev->movedir * 40,  ignore_monsters, ENT(pev), & tr);

	if ( pev->spawnflags & SF_GRENADE_INCENDIARY )
		FireExplode( &tr );
	else if ( HasSpawnFlag( SF_GRENADE_ENERGY ) )
		SphereBlast();
	else if ( HasSpawnFlag( SF_GRENADE_MORTAR ) )
	{
		Explode( &tr, DMG_BLAST );
		MortarBlast();
	}
	else
		Explode( &tr, DMG_BLAST );
}


//
// Contact grenade, explode when it touches something
// 
void CGrenade::ExplodeTouch( CBaseEntity *pOther )
{
	TraceResult tr;
	Vector		vecSpot;// trace starts here!

	vecSpot = pev->origin - pev->velocity.Normalize() * 32;
	UTIL_TraceLine( vecSpot, vecSpot + pev->velocity.Normalize() * 64, ignore_monsters, ENT(pev), &tr );

	if ( pev->spawnflags & SF_GRENADE_INCENDIARY )
		FireExplode( &tr );
	else if ( HasSpawnFlag( SF_GRENADE_ENERGY ) )
		SphereBlast();
	else if ( HasSpawnFlag( SF_GRENADE_MORTAR ) )
	{
		Explode( &tr, DMG_BLAST );
		MortarBlast();
	}
	else
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

	Vector vecTestVelocity;
	// pev->avelocity = Vector (300, 300, 300);

	// this is my heuristic for modulating the grenade velocity because grenades dropped purely vertical
	// or thrown very far tend to slow down too quickly for me to always catch just by testing velocity. 
	// trimming the Z velocity a bit seems to help quite a bit.
	vecTestVelocity = pev->velocity; 
	vecTestVelocity.z *= 0.45;

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
		SetThink( Detonate );
	}
	if (pev->waterlevel != 0)
	{
		pev->velocity = pev->velocity * 0.5;
		pev->framerate = 0.2;
	}
}


// Contact grenade with a fuse: explodes on what can take damage, bounces off the rest
void CGrenade::HybridThink( void )
{
	DangerSoundThink();

	if ( pev->dmgtime <= gpGlobals->time )
		Detonate();
}

void CGrenade::HybridTouch( CBaseEntity *pOther )
{
	if ( pOther->pev->takedamage != DAMAGE_NO )
		ExplodeTouch( pOther );
	else
		BounceTouch( pOther );
}

// Mine: detonates when anything with a class comes within pev->dmg (at most 256) units
void CGrenade::MineThink( void )
{
	pev->nextthink = gpGlobals->time + 0.1;

	float flRadius = min( 256, pev->dmg );
	CBaseEntity *pEntity = NULL;
	while ( ( pEntity = UTIL_FindEntityInSphere( pEntity, pev->origin, flRadius ) ) != NULL )
	{
		if ( pEntity->Classify() > CLASS_NONE )
			SetThink( Detonate );
	}
}


void CGrenade:: Spawn( void )
{
	pev->movetype = MOVETYPE_BOUNCE;
	pev->classname = MAKE_STRING( "grenade" );
	
	pev->solid = SOLID_BBOX;

	SET_MODEL(ENT(pev), "models/grenade.mdl");
	UTIL_SetSize(pev, Vector( 0, 0, 0), Vector(0, 0, 0));

	pev->dmg = 100;
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

	return pGrenade;
}


CGrenade *CGrenade::ShootHybrid( entvars_t *pevOwner, Vector vecStart, Vector vecVelocity, float time )
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
	pGrenade->SetThink( HybridThink );
	pGrenade->pev->nextthink = gpGlobals->time;
	
	// Tumble in air
	pGrenade->pev->avelocity.x = RANDOM_FLOAT ( -100, -500 );
	
	// Explode on contact with what can take damage
	pGrenade->SetTouch( HybridTouch );

	pGrenade->pev->friction = 0.8;
	pGrenade->pev->dmg = gSkillData.plrDmgM203Grenade;
	pGrenade->pev->dmgtime = gpGlobals->time + time;

	return pGrenade;
}


CGrenade *CGrenade::ShootMine( entvars_t *pevOwner, Vector vecStart, Vector vecVelocity )
{
	CGrenade *pGrenade = GetClassPtr( (CGrenade *)NULL );
	pGrenade->Spawn();
	pGrenade->pev->gravity = 0.5;
	UTIL_SetOrigin( pGrenade->pev, vecStart );
	pGrenade->pev->velocity = vecVelocity;
	pGrenade->pev->angles = UTIL_VecToAngles (pGrenade->pev->velocity);
	pGrenade->pev->owner = ENT(pevOwner);
	
	// arms after three seconds
	pGrenade->SetThink( MineThink );
	pGrenade->pev->nextthink = gpGlobals->time + 3;
	
	pGrenade->pev->avelocity.z = RANDOM_FLOAT ( -100, -500 );
	
	pGrenade->SetTouch( HybridTouch );

	pGrenade->pev->friction = 0.8;
	pGrenade->pev->dmg = gSkillData.plrDmgTripmine;

	return pGrenade;
}


CGrenade * CGrenade:: ShootTimed( entvars_t *pevOwner, Vector vecStart, Vector vecVelocity, float time )
{
	CGrenade *pGrenade = GetClassPtr( (CGrenade *)NULL );
	pGrenade->Spawn();
	UTIL_SetOrigin( pGrenade->pev, vecStart );
	pGrenade->pev->velocity = vecVelocity;
	pGrenade->pev->angles = UTIL_VecToAngles(pGrenade->pev->velocity);
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

	// Tumble through the air
	// pGrenade->pev->avelocity.x = -400;

	pGrenade->pev->gravity = 0.5;
	pGrenade->pev->friction = 0.8;

	SET_MODEL(ENT(pGrenade->pev), "models/w_grenade.mdl");
	pGrenade->pev->dmg = 100;

	return pGrenade;
}


// An invisible grenade that explodes after flDelay seconds at vecStart, against vecDir (ExDetonate)
CGrenade *CGrenade::ShootExplosion( entvars_t *pevOwner, const Vector &vecStart, float flDelay, const Vector &vecDir )
{
	CGrenade *pGrenade = GetClassPtr( (CGrenade *)NULL );
	pGrenade->Spawn();
	pGrenade->pev->takedamage = DAMAGE_NO;
	pGrenade->pev->effects |= EF_NODRAW;
	pGrenade->pev->movetype = MOVETYPE_NONE;
	pGrenade->pev->solid = SOLID_NOT;
	UTIL_SetOrigin( pGrenade->pev, vecStart );
	pGrenade->pev->owner = ENT(pevOwner);
	pGrenade->pev->movedir = vecDir;

	pGrenade->SetThink( ExDetonate );
	pGrenade->pev->nextthink = gpGlobals->time + flDelay;

	return pGrenade;
}

//======================end grenade


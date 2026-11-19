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
//=========================================================
// monster_crane - an overhead crane that runs along a
// path_corner path. At a "teleport" corner it lowers its
// cable and picks up what stands under it; at a "wait for
// retrigger" corner it drops its load, or, empty, waits
// until something stands under the next "teleport" corner.
//=========================================================

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"trains.h"

const float CABLE_STEP = 5;	// name ours: how far the cable moves each think

class CCraneMonster : public CBaseMonster
{
public:
	const char *GetKillDesc( void ) { return "a crane"; }
	const char *GetKillVerb( BOOL fGibbed, int &iHitGroup ) { return DamageVerb( DMG_CRUSH, fGibbed ); }

	void Spawn( void );
	void Precache( void );

	void EXPORT Initialize( void );
	void CarryLoad( void );		// name ours
	void EXPORT Move( void );
	void EXPORT Pause( void );
	void SetCableLength( float flLength );	// name ours
	void EXPORT Pickup( void );
	void EXPORT Drop( void );
	void EXPORT Wait( void );
	void MoveTo( const Vector &vecDest, float flSpeed );	// name ours
	void EXPORT StopSound( void );

	// pev->weapons: carrying; pev->frags: the cable's length; pev->movedir.z: the hook's
	// offset under the crane; pev->view_ofs: where the load hangs; pev->dmgtime: when the
	// current move or pause ends; m_hEnemy: the load (or, while waiting, the pickup corner)

	int		m_iUnused1;		// names ours: never read or written
	int		m_iUnused2;
	float	m_flFloor;		// how far the cable goes down
	Vector	m_vecPickupMins;	// names ours: the box under the pickup corner
	Vector	m_vecPickupMaxs;
};
LINK_ENTITY_TO_CLASS( monster_crane, CCraneMonster );

void CCraneMonster::Spawn( void )
{
	Precache();

	if ( pev->speed <= 0 )
		pev->speed = 100;

	if ( !pev->target )
		ALERT( at_console, "CraneMonster with no target" );

	pev->movetype = MOVETYPE_NOCLIP;
	pev->solid = SOLID_NOT;

	SET_MODEL( ENT(pev), "models/crane.mdl" );
	UTIL_SetSize( pev, Vector( -20, -20, -70 ), Vector( 20, 20, 19 ) );
	UTIL_SetOrigin( pev, pev->origin );

	SetMoveDone( StopSound );
	pev->view_ofs = pev->movedir = g_vecZero;

	SetThink( Initialize );
	pev->nextthink = gpGlobals->time + 1;
}

void CCraneMonster::Precache( void )
{
	PRECACHE_MODEL( "models/crane.mdl" );
	PRECACHE_SOUND( "ambience/turretrot1.wav" );
	PRECACHE_SOUND( "ambience/loader_hydra1.wav" );
}

//=========================================================
// Initialize - start at the first path_corner
//=========================================================
void CCraneMonster::Initialize( void )
{
	m_hTargetEnt = GetNextTarget();
	if ( m_hTargetEnt == NULL )
	{
		ALERT( at_console, "CraneMonster with no target" );
		return;
	}

	UTIL_SetOrigin( pev, m_hTargetEnt->pev->origin );
	pev->target = m_hTargetEnt->pev->target;
	m_hTargetEnt = GetNextTarget();

	if ( m_hTargetEnt->HasSpawnFlag( SF_CORNER_WAITFORTRIG ) )
		SetThink( Wait );
	else
		SetThink( Pause );

	pev->dmgtime = gpGlobals->time;
	pev->nextthink = gpGlobals->time + 1;
}

//=========================================================
// CarryLoad - the load follows the hook every frame
//=========================================================
void CCraneMonster::CarryLoad( void )
{
	if ( pev->weapons && m_hEnemy != NULL )
	{
		pev->nextthink = gpGlobals->time + 0.01;
		UTIL_SetOrigin( m_hEnemy->pev, pev->origin + pev->view_ofs );
		m_hEnemy->pev->velocity = pev->velocity;
		return;
	}

	if ( pev->dmgtime > gpGlobals->time )
		pev->nextthink = pev->dmgtime;
}

//=========================================================
// Move - at a path_corner: speed, targets, then pick up,
// drop, wait, pause or move on
//=========================================================
void CCraneMonster::Move( void )
{
	CarryLoad();

	if ( !m_fSequenceFinished )
	{
		pev->framerate = -1;
		StudioFrameAdvance();
	}

	if ( pev->dmgtime > gpGlobals->time )
		return;

	LinearMoveDone();

	CBaseEntity *pTarg = m_hTargetEnt;
	pev->target = m_hTargetEnt->pev->target;
	m_hTargetEnt = GetNextTarget();

	if ( m_hTargetEnt == NULL || !FClassnameIs( m_hTargetEnt->pev, "path_corner" ) )
	{
		ALERT( at_console, "CraneMonster: dead end or target not a path_corner" );
		SetThink( NULL );
		return;
	}

	if ( pTarg->pev->speed > 0 )
		pev->speed = pTarg->pev->speed;

	if ( pTarg->pev->message )
	{
		CBaseEntity *pActivator = this;
		if ( pev->weapons && m_hEnemy != NULL )
			pActivator = m_hEnemy;
		FireTargets( STRING( pTarg->pev->message ), pActivator, this, USE_TOGGLE, 0 );
		if ( pTarg->HasSpawnFlag( SF_CORNER_FIREONCE ) )
			pTarg->pev->message = 0;
	}

	if ( pTarg->HasSpawnFlag( SF_CORNER_TELEPORT ) )
	{
		// the pickup corner: lower the cable to the floor
		TraceResult tr;
		UTIL_TraceLine( pev->origin, pev->origin - Vector( 0, 0, 1024 ), ignore_monsters, ENT(pev), &tr );

		if ( tr.flFraction < 1 )
		{
			m_flFloor = tr.flFraction * 1024;
			if ( m_flFloor <= 72 )
				m_flFloor = 0;
			else
				m_flFloor -= 72;
			ALERT( at_console, "m_flFloor = %f\n", m_flFloor );

			SetThink( Pickup );
			m_vecPickupMins = tr.vecEndPos - Vector( 32, 32, 0 );
			m_vecPickupMaxs = tr.vecEndPos + Vector( 32, 32, 64 );
			pev->nextthink = gpGlobals->time + 0.1;
			EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, "ambience/loader_hydra1.wav", 1, ATTN_NORM, 0, 100 );

			// open the claw
			ResetSequenceInfo();
			pev->frame = 0;
			pev->framerate = 1;
			return;
		}
	}
	else if ( pTarg->HasSpawnFlag( SF_CORNER_WAITFORTRIG ) )
	{
		if ( pev->weapons && m_hEnemy != NULL )
		{
			// drop the load
			TraceResult tr;
			UTIL_TraceLine( pev->origin, pev->origin - Vector( 0, 0, 1024 ), ignore_monsters, ENT(pev), &tr );

			m_flFloor = tr.flFraction * 1024;
			if ( m_flFloor <= 200 )
				m_flFloor = 0;
			else
				m_flFloor -= 200;

			SetThink( Drop );
			pev->nextthink = gpGlobals->time + 0.01;
			EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, "ambience/loader_hydra1.wav", 1, ATTN_NORM, 0, 100 );
			return;
		}

		SetThink( Wait );
		pev->nextthink = gpGlobals->time + 0.1;
		return;
	}

	if ( pTarg->GetDelay() > 0 )
	{
		SetThink( Pause );
		pev->dmgtime = gpGlobals->time + pTarg->GetDelay();
		if ( pev->weapons && m_hEnemy != NULL )
			pev->nextthink = gpGlobals->time + 0.01;
		else
			pev->nextthink = pev->dmgtime;
		return;
	}

	MoveTo( m_hTargetEnt->pev->origin, pev->speed );
	SetThink( Move );
	pev->dmgtime = pev->nextthink;
	if ( pev->weapons && m_hEnemy != NULL )
		pev->nextthink = gpGlobals->time + 0.01;
}

//=========================================================
// Pause - wait out a path_corner's delay
//=========================================================
void CCraneMonster::Pause( void )
{
	CarryLoad();

	if ( pev->dmgtime <= gpGlobals->time )
	{
		MoveTo( m_hTargetEnt->pev->origin, pev->speed );
		SetThink( Move );
		pev->dmgtime = pev->nextthink;
		if ( pev->weapons && m_hEnemy != NULL )
			pev->nextthink = gpGlobals->time + 0.01;
	}
}

void CCraneMonster::SetCableLength( float flLength )
{
	pev->frags = flLength;
	SetBoneController( 0, min( 1024, flLength ) );
	pev->view_ofs.z = pev->movedir.z - flLength;
}

//=========================================================
// Pickup - lower the cable, close the claw on what is in
// the box under it and raise the cable again
//=========================================================
void CCraneMonster::Pickup( void )
{
	StudioFrameAdvance();

	if ( pev->weapons )
	{
		// raising
		if ( pev->frags > CABLE_STEP )
		{
			SetCableLength( pev->frags - CABLE_STEP );
			CarryLoad();
		}
		else
		{
			if ( m_hEnemy == NULL )
				pev->weapons = 0;
			SetCableLength( 0 );
			pev->dmgtime = gpGlobals->time;
			Pause();
			return;
		}
	}
	else if ( m_flFloor - CABLE_STEP > pev->frags )
	{
		// lowering
		SetCableLength( pev->frags + CABLE_STEP );
	}
	else if ( pev->framerate != -1 )
	{
		// at the floor: close the claw
		SetCableLength( m_flFloor );
		m_hEnemy = NULL;
		ResetSequenceInfo();
		pev->frame = 255;
		pev->framerate = -1;
	}
	else if ( m_fSequenceFinished )
	{
		EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, "ambience/loader_hydra1.wav", 1, ATTN_NORM, 0, 100 );
		pev->weapons = 1;

		CBaseEntity *pList[32];
		int count = UTIL_EntitiesInBox( pList, 32, m_vecPickupMins, m_vecPickupMaxs, FL_ONGROUND | FL_PARTIALGROUND );
		for ( int i = 0; i < count; i++ )
		{
			if ( pList[i]->pev->movetype != MOVETYPE_NONE && pList[i]->pev->movetype != MOVETYPE_PUSH )
			{
				m_hEnemy = pList[i];

				// hang it by its top
				if ( m_hEnemy->pev->size.z >= 72 )
					pev->movedir.z = -min( 72, m_hEnemy->pev->maxs.z );
				else
					pev->movedir.z = -( m_hEnemy->pev->mins.z + 72 );
				break;
			}
		}

		if ( pev->dmg > 0 && m_hEnemy != NULL && FBitSet( m_hEnemy->pev->flags, FL_MONSTER ) )
		{
			m_hEnemy->TakeDamage( pev, pev, pev->dmg, DMG_CRUSH | DMG_ALWAYSGIB );
			UTIL_BloodDrips( m_hEnemy->Center(), g_vecZero, m_hEnemy->BloodColor(), 100 );
		}
	}

	pev->nextthink = gpGlobals->time + 0.01;
}

//=========================================================
// Drop - lower the load to the floor, let go of it and
// raise the empty cable
//=========================================================
void CCraneMonster::Drop( void )
{
	StudioFrameAdvance();

	if ( pev->weapons )
	{
		if ( m_flFloor - CABLE_STEP > pev->frags )
		{
			// lowering
			SetCableLength( pev->frags + CABLE_STEP );
			CarryLoad();
		}
		else
		{
			SetCableLength( m_flFloor );
			pev->weapons = 0;

			if ( m_hEnemy != NULL )
			{
				m_hEnemy->pev->velocity.z -= pev->dmg + 5;
				if ( pev->dmg > 0 && FBitSet( m_hEnemy->pev->flags, FL_MONSTER ) )
				{
					m_hEnemy->TakeDamage( pev, pev, pev->dmg, DMG_CRUSH | DMG_ALWAYSGIB );
					UTIL_BloodDrips( m_hEnemy->Center(), g_vecZero, m_hEnemy->BloodColor(), 100 );
				}
			}

			m_hEnemy = NULL;

			// open the claw
			ResetSequenceInfo();
			pev->frame = 0;
			pev->framerate = 1;
			EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, "ambience/loader_hydra1.wav", 1, ATTN_NORM, 0, 100 );
			CarryLoad();
		}
	}
	else
	{
		// raising the empty cable, closing the claw
		if ( m_fSequenceFinished && pev->framerate != -1 )
		{
			ResetSequenceInfo();
			pev->frame = 255;
			pev->framerate = -1;
		}

		if ( pev->frags > CABLE_STEP )
		{
			SetCableLength( pev->frags - CABLE_STEP );
		}
		else
		{
			SetCableLength( 0 );
			SetThink( Wait );
		}
	}

	pev->nextthink = gpGlobals->time + 0.01;
}

//=========================================================
// Wait - empty, at a "wait for retrigger" corner: find the
// next "teleport" corner ahead and wait until something
// stands under it
//=========================================================
void CCraneMonster::Wait( void )
{
	if ( !m_fSequenceFinished )
	{
		pev->framerate = -1;
		StudioFrameAdvance();
	}

	pev->nextthink = gpGlobals->time + RANDOM_FLOAT( 0.2, 0.5 );

	if ( m_hEnemy == NULL )
	{
		int i = 10;
		for ( CBaseEntity *pTarg = GetNextTarget(); pTarg; pTarg = pTarg->GetNextTarget() )
		{
			if ( pTarg->HasSpawnFlag( SF_CORNER_TELEPORT ) )
			{
				m_hEnemy = pTarg;
				break;
			}

			if ( --i <= 0 || pTarg->HasSpawnFlag( SF_CORNER_WAITFORTRIG ) )
			{
				// no pickup corner: go on
				pev->dmgtime = gpGlobals->time;
				Pause();
				return;
			}
		}

		TraceResult tr;
		UTIL_TraceLine( m_hEnemy->pev->origin, m_hEnemy->pev->origin - Vector( 0, 0, 1024 ), ignore_monsters, ENT(pev), &tr );
		m_flFloor = tr.flFraction * 1024;
		m_vecPickupMins = tr.vecEndPos - Vector( 32, 32, 0 );
		m_vecPickupMaxs = tr.vecEndPos + Vector( 32, 32, 64 );
	}

	if ( m_hEnemy != NULL )
	{
		CBaseEntity *pList[32];
		int count = UTIL_EntitiesInBox( pList, 32, m_vecPickupMins, m_vecPickupMaxs, FL_ONGROUND | FL_PARTIALGROUND );
		for ( int i = 0; i < count; i++ )
		{
			if ( pList[i]->pev->movetype != MOVETYPE_NONE && pList[i]->pev->movetype != MOVETYPE_PUSH )
			{
				// something to pick up: go
				m_hEnemy = NULL;
				pev->dmgtime = gpGlobals->time;
				Pause();
				return;
			}
		}
	}
}

//=========================================================
// MoveTo - CBaseToggle::LinearMove with the motor's sound,
// timed by gpGlobals->time (the crane is no pusher)
//=========================================================
void CCraneMonster::MoveTo( const Vector &vecDest, float flSpeed )
{
	m_vecFinalDest = vecDest;

	// Already there?
	if ( vecDest == pev->origin )
	{
		LinearMoveDone();
		pev->nextthink = gpGlobals->time;
		return;
	}

	EMIT_SOUND_DYN( ENT(pev), CHAN_BODY, "ambience/turretrot1.wav", 1, ATTN_NORM, 0, 100 );

	// set destdelta to the vector needed to move
	Vector vecDestDelta = vecDest - pev->origin;
	
	// divide vector length by speed to get time to reach dest
	float flTravelTime = vecDestDelta.Length() / flSpeed;

	// set nextthink to trigger a call to LinearMoveDone when dest is reached
	pev->nextthink = gpGlobals->time + flTravelTime;
	SetThink( LinearMoveDone );

	// scale the destdelta vector by the time spent traveling to get velocity
	pev->velocity = vecDestDelta / flTravelTime;
}

void CCraneMonster::StopSound( void )
{
	STOP_SOUND( ENT(pev), CHAN_BODY, "ambience/turretrot1.wav" );
}

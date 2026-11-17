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
// CBaseFlyingObject. A stand-in until the flight code is written: the members
// the derived classes' vtables and calls need, mostly CBaseMonster's behaviour.

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"schedule.h"
#include	"flyingobject.h"

void CBaseFlyingObject :: Spawn( void )
{
	pev->movetype = MOVETYPE_FLY;
	pev->solid = SOLID_BBOX;
	UTIL_SetSize( pev, Vector( -16, -16, 0 ), Vector( 16, 16, 32 ) );
	UTIL_SetOrigin( pev, pev->origin );

	pev->flags |= FL_MONSTER | FL_FLY;
	pev->takedamage = DAMAGE_AIM;
	m_bloodColor = DONT_BLEED;
	m_flFieldOfView = -1;
	m_flHeight = 256;

	pev->sequence = 0;
	ResetSequenceInfo( );
	pev->frame = RANDOM_LONG( 0, 255 );

	SetYawSpeed( );
	MonsterInit( );
}

void CBaseFlyingObject :: Killed( entvars_t *pevAttacker, int iGib )
{
	CBaseMonster::Killed( pevAttacker, iGib );
}

void CBaseFlyingObject :: DyingThink( void )
{
	SetThink( NULL );
}

void CBaseFlyingObject :: CrashTouch( CBaseEntity *pOther )
{
	SetTouch( NULL );
	pev->dmgtime = gpGlobals->time;
	pev->nextthink = gpGlobals->time;
}

void CBaseFlyingObject :: RunAI( void )
{
	CBaseMonster::RunAI( );
}

void CBaseFlyingObject :: StartTask( Task_t *pTask )
{
	CBaseMonster::StartTask( pTask );
}

void CBaseFlyingObject :: RunTask( Task_t *pTask )
{
	CBaseMonster::RunTask( pTask );
}

void CBaseFlyingObject :: SetYawSpeed( void )
{
	pev->yaw_speed = 80;
	pev->pitch_speed = 60;
}

void CBaseFlyingObject :: ChangePitch( float speed )
{
	float diff = UTIL_AngleDiff( pev->idealpitch, pev->angles.x );	// sic: unused
	pev->angles.x = UTIL_Approach( pev->idealpitch, pev->angles.x, speed );
}

Schedule_t *CBaseFlyingObject :: GetSchedule( void )
{
	return CBaseMonster::GetSchedule( );
}

BOOL CBaseFlyingObject :: ShouldAdvanceRoute( float flWaypointDist )
{
	return CBaseMonster::ShouldAdvanceRoute( flWaypointDist );
}

void CBaseFlyingObject :: Move( float flInterval )
{
	CBaseMonster::Move( flInterval );
}

BOOL CBaseFlyingObject :: FRefreshRoute( void )
{
	return CBaseMonster::FRefreshRoute( );
}

float CBaseFlyingObject :: CeilingZ( const Vector &position )
{
	return position.z + m_flHeight;
}

float CBaseFlyingObject :: FloorZ( const Vector &position )
{
	return position.z - m_flHeight;
}

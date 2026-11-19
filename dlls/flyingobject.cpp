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
// CBaseFlyingObject: the base of the flying spheres and cubes (dd_sphere.cpp).
// It flies by setting its velocity towards the next waypoint (no MoveExecute),
// keeps to the middle between floor and ceiling when it heads for its enemy,
// and when killed tumbles down and explodes like the apache.

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"schedule.h"
#include	"weapons.h"
#include	"soundent.h"
#include	"flyingobject.h"

const float DIST_TO_CHECK = 200;	// sic: a variable, unlike monsters.cpp's define

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
	if ( HasMemory( bits_MEMORY_KILLED ) )
		return;
	Remember( bits_MEMORY_KILLED );

	pev->deadflag = DEAD_DEAD;
	pev->movetype = MOVETYPE_TOSS;
	pev->gravity = 0.3;
	pev->avelocity = pev->avelocity + Vector( RANDOM_FLOAT( -50, 50 ), RANDOM_FLOAT( -50, 50 ), RANDOM_FLOAT( -50, 50 ) );

	SetThink( DyingThink );
	SetTouch( CrashTouch );
	pev->nextthink = gpGlobals->time + 0.1;

	// explode on impact, after 15 seconds at the latest
	pev->dmgtime = gpGlobals->time + 15;
	pev->takedamage = DAMAGE_YES;
}

void CBaseFlyingObject :: DyingThink( void )
{
	StudioFrameAdvance( );
	pev->nextthink = gpGlobals->time + 0.1;

	if ( pev->dmgtime <= gpGlobals->time || pev->health <= -100 )
	{
		pev->takedamage = DAMAGE_NO;
		pev->solid = SOLID_NOT;
		pev->velocity = g_vecZero;
		pev->movetype = MOVETYPE_NONE;
		pev->effects = EF_NODRAW;

		Vector vecSpot( Center( ) );
		int iScale = ( pev->dmg - 50 ) * 0.6;
		if ( iScale < 10 )
			iScale = 10;

		// smoke
		MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, vecSpot );
			WRITE_BYTE( TE_SMOKE );
			WRITE_COORD( vecSpot.x );
			WRITE_COORD( vecSpot.y );
			WRITE_COORD( vecSpot.z + pev->size.z );
			WRITE_SHORT( g_sModelIndexSmoke );
			WRITE_BYTE( iScale * 2 ); // scale * 10
			WRITE_BYTE( 5  ); // framerate
		MESSAGE_END();

		if ( m_iSpriteTexture )
		{
			// blast circle
			float flRadius = pev->dmg * 6;
			MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, pev->origin );
				WRITE_BYTE( TE_BEAMCYLINDER );
				WRITE_COORD( pev->origin.x);
				WRITE_COORD( pev->origin.y);
				WRITE_COORD( pev->origin.z);
				WRITE_COORD( pev->origin.x);
				WRITE_COORD( pev->origin.y);
				WRITE_COORD( pev->origin.z + flRadius );
				WRITE_SHORT( m_iSpriteTexture );
				WRITE_BYTE( 0 ); // startframe
				WRITE_BYTE( 0 ); // framerate
				WRITE_BYTE( 4 ); // life
				WRITE_BYTE( 32 );  // width
				WRITE_BYTE( 0 );   // noise
				WRITE_BYTE( 255 );   // r, g, b
				WRITE_BYTE( 255 );   // r, g, b
				WRITE_BYTE( 192 );   // r, g, b
				WRITE_BYTE( 128 ); // brightness
				WRITE_BYTE( 0 );		// speed
			MESSAGE_END();

			EMIT_SOUND(ENT(pev), CHAN_STATIC, "weapons/mortarhit.wav", 1.0, 0.3);

			// fireball
			MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, vecSpot );
				WRITE_BYTE( TE_SPRITE );
				WRITE_COORD( vecSpot.x );
				WRITE_COORD( vecSpot.y );
				WRITE_COORD( vecSpot.z + pev->size.z * 0.5 );
				WRITE_SHORT( g_sModelIndexFireExplo );
				WRITE_BYTE( iScale ); // scale * 10
				WRITE_BYTE( 255 ); // brightness
			MESSAGE_END();
		}
		else
		{
			MESSAGE_BEGIN( MSG_PAS, SVC_TEMPENTITY, vecSpot );
				WRITE_BYTE( TE_EXPLOSION );
				WRITE_COORD( vecSpot.x );
				WRITE_COORD( vecSpot.y );
				WRITE_COORD( vecSpot.z );
				WRITE_SHORT( g_sModelIndexFireExplo );
				WRITE_BYTE( iScale ); // scale * 10
				WRITE_BYTE( 10 ); // framerate
				WRITE_BYTE( TE_EXPLFLAG_NOPARTICLES );
			MESSAGE_END();
		}

		if ( pev->dmg != 0 )
			RadiusDamage( pev->origin, pev, pev, pev->dmg, CLASS_NONE, DMG_BLAST );

		if ( m_iBodyGibs )
		{
			// gibs, as many as the box holds
			float flVolume = pev->size.x * pev->size.y * pev->size.z;
			MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, vecSpot );
				WRITE_BYTE( TE_BREAKMODEL);

				// position
				WRITE_COORD( vecSpot.x );
				WRITE_COORD( vecSpot.y );
				WRITE_COORD( vecSpot.z );

				// size
				WRITE_COORD( pev->size.x );
				WRITE_COORD( pev->size.y );
				WRITE_COORD( pev->size.z );

				// velocity
				WRITE_COORD( 0 );
				WRITE_COORD( 0 );
				WRITE_COORD( 200 );

				// randomization
				WRITE_BYTE( 30 );

				// Model
				WRITE_SHORT( m_iBodyGibs );	//model id#

				// # of shards
				WRITE_BYTE( flVolume * 0.001 );

				// duration
				WRITE_BYTE( 200 );// 20.0 seconds

				// flags

				WRITE_BYTE( BREAK_METAL );
			MESSAGE_END();
		}

		SetThink( SUB_Remove );
		pev->nextthink = gpGlobals->time + 1;

		CBaseEntity *pOwner = CBaseEntity::Instance( pev->owner );
		if ( pOwner )
		{
			pOwner->DeathNotice( pev );
		}
	}
}

void CBaseFlyingObject :: CrashTouch( CBaseEntity *pOther )
{
	SetTouch( NULL );
	pev->dmgtime = gpGlobals->time;
	pev->nextthink = gpGlobals->time;
}

//=========================================================
// RunAI: CBaseMonster's, and the derived class's attack
// once the enemy is known
//=========================================================
void CBaseFlyingObject :: RunAI( void )
{
	// to test model's eye height
	//UTIL_ParticleEffect ( pev->origin + pev->view_ofs, g_vecZero, 255, 10 );

	// IDLE sound permitted in ALERT state is because monsters were silent in ALERT state. Only play IDLE sound in IDLE state
	// once we have sounds for that state.
	if ( ( m_MonsterState == MONSTERSTATE_IDLE || m_MonsterState == MONSTERSTATE_ALERT ) && RANDOM_LONG(0,99) == 0 && !(pev->flags & SF_MONSTER_GAG) )
	{
		IdleSound();
	}

	if ( m_MonsterState != MONSTERSTATE_NONE	&&
		 m_MonsterState != MONSTERSTATE_PRONE   &&
		 m_MonsterState != MONSTERSTATE_DEAD )// don't bother with this crap if monster is prone.
	{
		// collect some sensory Condition information.
		// don't let monsters outside of the player's PVS act up, or most of the interesting
		// things will happen before the player gets there!
		// UPDATE: We now let COMBAT state monsters think and act fully outside of player PVS. This allows the player to leave
		// an area where monsters are fighting, and the fight will continue.
		if ( !FNullEnt( FIND_CLIENT_IN_PVS( edict() ) ) || ( m_MonsterState == MONSTERSTATE_COMBAT ) )
		{
			Look( m_flDistLook );
			Listen();// check for audible sounds.

			// now filter conditions.
			ClearConditions( IgnoreConditions() );

			GetEnemy();
			RunAttack();
		}

		// do these calculations if monster has an enemy.
		if ( m_hEnemy != NULL )
		{
			CheckEnemy( m_hEnemy );
		}

		CheckAmmo();
	}

	FCheckAITrigger();

	PrescheduleThink();

	MaintainSchedule();

	// if the monster didn't use these conditions during the above call to MaintainSchedule() or CheckAITrigger()
	// we throw them out cause we don't want them sitting around through the lifespan of a schedule
	// that doesn't use them.
	ClearConditions( bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE );
}

void CBaseFlyingObject :: StartTask( Task_t *pTask )
{
	switch ( pTask->iTask )
	{
	case TASK_GET_PATH_TO_ENEMY:
		{
			CBaseEntity *pEnemy = m_hEnemy;

			if ( pEnemy == NULL )
			{
				TaskFail();
				return;
			}

			if ( m_flHeight > 0 )
			{
				// head for the enemy's last known position, halfway between floor and ceiling
				Vector vecDest = m_vecEnemyLKP;
				float flFloor = FloorZ( vecDest );
				vecDest.z = ( CeilingZ( vecDest ) - flFloor ) * 0.5 + flFloor;

				if ( BuildRoute ( vecDest, bits_MF_TO_ENEMY, pEnemy ) )
				{
					TaskComplete();
				}
				else if (BuildNearestRoute( pEnemy->pev->origin, pEnemy->pev->view_ofs, 0, (pEnemy->pev->origin - pev->origin).Length() ))
				{
					TaskComplete();
				}
				else
				{
					// no way to get there =(
					ALERT ( at_aiconsole, "GetPathToEnemy failed!!\n" );
					TaskFail();
				}
				break;
			}
		}
		// fall through
	default:
		CBaseMonster::StartTask( pTask );
		break;
	}
}

void CBaseFlyingObject :: RunTask( Task_t *pTask )
{
	switch ( pTask->iTask )
	{
	case TASK_WAIT:
	case TASK_WAIT_FACE_ENEMY:
	case TASK_WAIT_PVS:
	case TASK_WAIT_FOR_MOVEMENT:
		if ( pev->yaw_speed > 0 )
		{
			ChangeYaw( pev->yaw_speed );
			if ( pev->pitch_speed > 0 )
				ChangePitch( pev->pitch_speed );
		}
		break;
	}

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
	switch ( m_MonsterState )
	{
	case MONSTERSTATE_IDLE:
		if ( HasConditions( bits_COND_HEAR_SOUND ) )
		{
			CSound *pSound = PBestSound();
			if ( pSound->m_iType & bits_SOUND_COMBAT )
				return GetScheduleOfType( SCHED_INVESTIGATE_SOUND );
		}
		break;

	case MONSTERSTATE_COMBAT:
		if ( HasConditions( bits_COND_SEE_ENEMY ) )
			return GetScheduleOfType( SCHED_CHASE_ENEMY );
		if ( !HasConditions( bits_COND_ENEMY_OCCLUDED ) )
			return GetScheduleOfType( SCHED_COMBAT_FACE );
		return GetScheduleOfType( SCHED_CHASE_ENEMY );
	}

	return CBaseMonster::GetSchedule();
}

BOOL CBaseFlyingObject :: ShouldAdvanceRoute( float flWaypointDist )
{
	// Get true 3D distance to the goal so we actually reach the correct height
	if ( m_Route[ m_iRouteIndex ].iType & bits_MF_IS_GOAL )
		flWaypointDist = ( m_Route[ m_iRouteIndex ].vecLocation - pev->origin ).Length();

	if ( flWaypointDist <= 64 + (m_flGroundSpeed * gpGlobals->frametime) )
		return TRUE;

	return FALSE;
}

//=========================================================
// Move: CBaseMonster's, flying straight at the waypoint
// at m_flGroundSpeed (100 at least) instead of MoveExecute
//=========================================================
void CBaseFlyingObject :: Move( float flInterval )
{
	float		flWaypointDist;
	float		flCheckDist;
	float		flDist;// how far the lookahead check got before hitting an object.
	Vector		vecDir;
	Vector		vecApex;
	CBaseEntity	*pTargetEnt;

	// Don't move if no valid route
	if ( FRouteClear() )
	{
		// If we still have a movement goal, then this is probably a route truncated by SimplifyRoute()
		// so refresh it.
		if ( m_movementGoal == MOVEGOAL_NONE || !FRefreshRoute() )
		{
			ALERT( at_aiconsole, "Tried to move with no route!\n" );
			TaskFail();
			return;
		}
	}

	if ( m_flMoveWaitFinished > gpGlobals->time )
		return;

	// if the monster is moving directly towards an entity (enemy for instance), we'll set this pointer
	// to that entity for the CheckLocalMove and Triangulate functions.
	pTargetEnt = NULL;

	m_flGroundSpeed = max( 100, m_flGroundSpeed );

	// local move to waypoint.
	vecDir = ( m_Route[ m_iRouteIndex ].vecLocation - pev->origin ).Normalize();
	flWaypointDist = ( m_Route[ m_iRouteIndex ].vecLocation - pev->origin ).Length2D();

	if ( pev->yaw_speed > 0 )
	{
		MakeIdealYaw ( m_Route[ m_iRouteIndex ].vecLocation );
		ChangeYaw ( pev->yaw_speed );
	}
	pev->angles.y = UTIL_AngleMod( pev->angles.y );

	// if the waypoint is closer than CheckDist, CheckDist is the dist to waypoint
	if ( flWaypointDist < DIST_TO_CHECK )
	{
		flCheckDist = flWaypointDist;
	}
	else
	{
		flCheckDist = DIST_TO_CHECK;
	}

	if ( (m_Route[ m_iRouteIndex ].iType & (~bits_MF_NOT_TO_MASK)) == bits_MF_TO_ENEMY )
	{
		// only on a PURE move to enemy ( i.e., ONLY MF_TO_ENEMY set, not MF_TO_ENEMY and DETOUR )
		pTargetEnt = m_hEnemy;
	}
	else if ( (m_Route[ m_iRouteIndex ].iType & ~bits_MF_NOT_TO_MASK) == bits_MF_TO_TARGETENT )
	{
		pTargetEnt = m_hTargetEnt;
	}

	// !!!BUGBUG - CheckDist should be derived from ground speed.
	// If this fails, it should be because of some dynamic entity blocking this guy.
	// We've already checked this path, so we should wait and time out if the entity doesn't move
	flDist = 0;
	if ( CheckLocalMove ( pev->origin, pev->origin + vecDir * flCheckDist, pTargetEnt, &flDist ) != LOCALMOVE_VALID )
	{
		CBaseEntity *pBlocker;

		// Can't move, stop
		pev->velocity = g_vecZero;
		Stop();
		// Blocking entity is in global trace_ent
		pBlocker = CBaseEntity::Instance( gpGlobals->trace_ent );
		if (pBlocker)
		{
			DispatchBlocked( edict(), pBlocker->edict() );
		}

		if ( pBlocker && m_moveWaitTime > 0 && pBlocker->IsMoving() && !pBlocker->IsPlayer() && (gpGlobals->time-m_flMoveWaitFinished) > 3.0 )
		{
			// Can we still move toward our target?
			if ( flDist < m_flGroundSpeed )
			{
				// No, Wait for a second
				m_flMoveWaitFinished = gpGlobals->time + m_moveWaitTime;
				return;
			}
			// Ok, still enough room to take a step
		}
		else
		{
			// try to triangulate around whatever is in the way.
			if ( FTriangulate( pev->origin, m_Route[ m_iRouteIndex ].vecLocation, flDist, pTargetEnt, &vecApex ) )
			{
				InsertWaypoint( vecApex, bits_MF_TO_DETOUR );
				RouteSimplify( pTargetEnt );
			}
			else
			{
//				ALERT ( at_aiconsole, "Couldn't Triangulate\n" );
				pev->velocity = g_vecZero;
				Stop();
				// Only do this once until your route is cleared
				if ( m_moveWaitTime > 0 && !(m_afMemory & bits_MEMORY_MOVE_FAILED) )
				{
					FRefreshRoute();
					if ( FRouteClear() )
					{
						TaskFail();
					}
					else
					{
						// Don't get stuck
						if ( (gpGlobals->time - m_flMoveWaitFinished) < 0.2 )
							Remember( bits_MEMORY_MOVE_FAILED );

						m_flMoveWaitFinished = gpGlobals->time + 0.1;
					}
				}
				else
				{
					TaskFail();
					ALERT( at_aiconsole, "%s Failed to move (%d)!\n", STRING(pev->classname), HasMemory( bits_MEMORY_MOVE_FAILED ) );
					//ALERT( at_aiconsole, "%f, %f, %f\n", pev->origin.z, (pev->origin + (vecDir * flCheckDist)).z, m_Route[m_iRouteIndex].vecLocation.z );
				}
				return;
			}
		}
	}

	// close enough to the target, now advance to the next target. This is done before actually reaching
	// the target so that we get a nice natural turn while moving.
	if ( ShouldAdvanceRoute( flWaypointDist ) )///!!!BUGBUG- magic number
	{
		AdvanceRoute( flWaypointDist );
	}

	// Might be waiting for a door
	if ( m_flMoveWaitFinished > gpGlobals->time )
	{
		pev->velocity = g_vecZero;
		Stop();
		return;
	}

	pev->velocity = ( m_Route[ m_iRouteIndex ].vecLocation - pev->origin ).Normalize() * m_flGroundSpeed;

	if ( MovementIsComplete() )
	{
		pev->velocity = g_vecZero;
		Stop();
		RouteClear();
	}
}

BOOL CBaseFlyingObject :: FRefreshRoute( void )
{
	CBaseEntity	*pPathCorner;
	int			i;
	BOOL		returnCode;

	RouteNew();

	returnCode = FALSE;

	switch( m_movementGoal )
	{
		case MOVEGOAL_PATHCORNER:
			{
				// monster is on a path_corner loop
				pPathCorner = m_pGoalEnt;
				i = 0;

				while ( pPathCorner && i < ROUTE_SIZE )
				{
					m_Route[ i ].iType = bits_MF_TO_PATHCORNER;
					m_Route[ i ].vecLocation = pPathCorner->pev->origin;

					pPathCorner = pPathCorner->GetNextTarget();

					// Last path_corner in list?
					if ( !pPathCorner )
						m_Route[i].iType |= bits_MF_IS_GOAL;

					i++;
				}
			}
			returnCode = TRUE;
			break;

		case MOVEGOAL_ENEMY:
			{
				// halfway between floor and ceiling
				Vector vecDest = m_vecEnemyLKP;
				float flFloor = FloorZ( vecDest );
				vecDest.z = ( CeilingZ( vecDest ) - flFloor ) * 0.5 + flFloor;
				returnCode = BuildRoute( vecDest, bits_MF_TO_ENEMY, m_hEnemy );
			}
			break;

		case MOVEGOAL_LOCATION:
			returnCode = BuildRoute( m_vecMoveGoal, bits_MF_TO_LOCATION, NULL );
			break;

		case MOVEGOAL_TARGETENT:
			if (m_hTargetEnt != NULL)
			{
				returnCode = BuildRoute( m_hTargetEnt->pev->origin, bits_MF_TO_TARGETENT, m_hTargetEnt );
			}
			break;

		case MOVEGOAL_NODE:
			returnCode = FGetNodeRoute( m_vecMoveGoal );
//			if ( returnCode )
//				RouteSimplify( NULL );
			break;
	}

	return returnCode;
}

// the ceiling above position, at most m_flHeight up
float CBaseFlyingObject :: CeilingZ( const Vector &position )
{
	TraceResult tr;

	Vector up = position;
	up.z += m_flHeight;

	UTIL_TraceLine( position, up, ignore_monsters, NULL, &tr );

	if ( tr.flFraction != 1.0 )
		return tr.vecEndPos.z;

	return up.z;
}

// the floor below position, at most m_flHeight down
float CBaseFlyingObject :: FloorZ( const Vector &position )
{
	TraceResult tr;

	Vector down = position;
	down.z -= m_flHeight;

	UTIL_TraceLine( position, down, ignore_monsters, NULL, &tr );

	if ( tr.flFraction != 1.0 )
		return tr.vecEndPos.z;

	return down.z;
}

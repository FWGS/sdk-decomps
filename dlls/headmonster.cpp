/***
*
*	Scientist Slaughterhouse: the floating scientist head.
*	The file name and the names marked "ours" are ours; class, field and
*	entity names come from the binary (save tables, exports).
*
****/
/*

===== headmonster.cpp ========================================================

  monster_head (CFlyingHead): a scientist's head that floats like an alien
  controller and burns its enemy with two eye lasers. Its movement code and
  schedules are the controller's (controller.cpp).

*/

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"effects.h"
#include	"schedule.h"
#include	"weapons.h"

// the controller's schedules (controller.cpp)
extern Schedule_t slControllerChaseEnemy[];
extern Schedule_t slControllerStrafe[];
extern Schedule_t slControllerTakeCover[];
extern Schedule_t slControllerFail[];

class CFlyingHead : public CBaseMonster
{
public:
	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	const char *GetKillDesc( void ) { return "a floating head"; }

	void Spawn( void );
	void Precache( void );
	void SetYawSpeed( void ) { pev->yaw_speed = 100; }
	int  Classify ( void );
	int  IRelationship( CBaseEntity *pTarget );
	int  GetBattlePower( void );

	BOOL CheckRangeAttack1 ( float flDot, float flDist );	// eye lasers
	BOOL CheckRangeAttack2 ( float flDot, float flDist );
	Schedule_t* GetScheduleOfType ( int Type );
	void StartTask ( Task_t *pTask );
	void RunTask ( Task_t *pTask );
	CUSTOM_SCHEDULES;

	void Stop( void );
	void Move ( float flInterval );
	int  CheckLocalMove ( const Vector &vecStart, const Vector &vecEnd, CBaseEntity *pTarget, float *pflDist );
	void MoveExecute( CBaseEntity *pTargetEnt, const Vector &vecDir, float flInterval );
	BOOL ShouldAdvanceRoute( float flWaypointDist );
	void EyeLasers( void );	// name ours

	void PainSound( void );
	void AlertSound( void );
	void IdleSound( void );
	void AttackSound( void );
	void DeathSound( void );

	static const char *pAttackSounds[];
	static const char *pIdleSounds[];
	static const char *pPainSounds[];

	int TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );
	void Killed( entvars_t *pevAttacker, int iGib );

	CBeam *m_pBeam[2];		// eye lasers
	float m_flBeamPitch;	// their pitch
	Vector m_vecEstVelocity;

	Vector m_velocity;
	float m_flShootEnd;		// name ours: the lasers burn until then
	float m_flNextBlood;	// name ours: when the neck sprays blood
};

LINK_ENTITY_TO_CLASS( monster_head, CFlyingHead );

TYPEDESCRIPTION	CFlyingHead::m_SaveData[] =
{
	DEFINE_ARRAY( CFlyingHead, m_pBeam, FIELD_CLASSPTR, 2 ),
	DEFINE_FIELD( CFlyingHead, m_vecEstVelocity, FIELD_VECTOR ),
	DEFINE_FIELD( CFlyingHead, m_flBeamPitch, FIELD_FLOAT ),
};
IMPLEMENT_SAVERESTORE( CFlyingHead, CBaseMonster );


const char *CFlyingHead::pAttackSounds[] =
{
	"scientist/holdstill.wav",
	"!SC_QUESTION24",
	"!SC_HEAL7",
};

const char *CFlyingHead::pIdleSounds[] =
{
	"!SC_QUESTION7",
	"!SC_QUESTION8",
	"!SC_IDLE3",
	"!SC_PQUEST15",
};

const char *CFlyingHead::pPainSounds[] =
{
	"scientist/sci_pain1.wav",
	"scientist/sci_pain2.wav",
	"scientist/sci_pain3.wav",
	"scientist/sci_pain4.wav",
	"scientist/sci_pain5.wav",
};


DEFINE_CUSTOM_SCHEDULES( CFlyingHead )
{
	slControllerChaseEnemy,
	slControllerStrafe,
	slControllerTakeCover,
	slControllerFail,
};

IMPLEMENT_CUSTOM_SCHEDULES( CFlyingHead, CBaseMonster );


//=========================================================
// Classify - indicates this monster's place in the
// relationship table.
//=========================================================
int	CFlyingHead :: Classify ( void )
{
	return	18;	// the heads' own class (no CLASS_ define)
}

//=========================================================
// IRelationship - heads leave other heads alone
//=========================================================
int CFlyingHead :: IRelationship( CBaseEntity *pTarget )
{
	if ( pTarget->pev->modelindex == pev->modelindex )
		return R_NO;

	return CBaseMonster::IRelationship( pTarget );
}

//=========================================================
// GetBattlePower - the target ID's rating: the green laser
// (Glasses) hurts three times as much as the red ones
//=========================================================
int CFlyingHead :: GetBattlePower( void )
{
	return ( pev->body ? 10 : 30 ) * ( pev->max_health * 0.1 );
}

int CFlyingHead :: TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	// HACK HACK -- until we fix this.
	if ( IsAlive() )
		PainSound();
	return CBaseMonster::TakeDamage( pevInflictor, pevAttacker, flDamage, bitsDamageType );
}


void CFlyingHead::Killed( entvars_t *pevAttacker, int iGib )
{
	// delete the lasers
	if (m_pBeam[0])
		UTIL_Remove( m_pBeam[0] );
	if (m_pBeam[1])
		UTIL_Remove( m_pBeam[1] );
	m_pBeam[1] = NULL;
	m_pBeam[0] = NULL;

	// the head pops
	SpawnBlood( Center(), BloodColor(), 100 );
	CGib::SpawnPoppedHead( pev, 0 );
	DeathSound();
	PLAYBACK_EVENT_FULL( 0, edict(), g_iBloodSprayEvent, 0.0, (float *)&pev->origin, (float *)&pev->angles, 0.0, 0.0, 0, 0, 1, 0 );

	// tell the monster maker
	CBaseEntity *pOwner = CBaseEntity::Instance( pev->owner );
	if ( pOwner )
		pOwner->DeathNotice( pev );

	UTIL_Remove( this );
}


void CFlyingHead :: PainSound( void )
{
	if (RANDOM_LONG(0,5) < 2)
		EMIT_SOUND_ARRAY_DYN( CHAN_VOICE, pPainSounds );
}

void CFlyingHead :: AlertSound( void )
{
	SENTENCEG_PlayRndSz( ENT(pev), "SC_FEAR", 1.0, ATTN_NORM, 0, 80 );
}

void CFlyingHead :: IdleSound( void )
{
	EMIT_SOUND_ARRAY_DYN( CHAN_VOICE, pIdleSounds );
}

void CFlyingHead :: AttackSound( void )
{
	EMIT_SOUND_ARRAY_DYN( CHAN_VOICE, pAttackSounds );
}

void CFlyingHead :: DeathSound( void )
{
	EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "scientist/scream1.wav", 1.0, ATTN_NORM, 0, 100 );
}

//=========================================================
// Spawn
//=========================================================
void CFlyingHead :: Spawn()
{
	Precache( );

	SET_MODEL(ENT(pev), "models/heads.mdl");
	UTIL_SetSize( pev, Vector( -12, -12, 0 ), Vector( 12, 12, 24 ));

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_FLY;
	pev->flags			|= FL_FLY;
	m_bloodColor		= BLOOD_COLOR_RED;
	pev->health			= gSkillData.scientistHealth;
	pev->view_ofs		= Vector( 0, 0, 12 );// position of the eyes relative to monster's origin.
	m_flFieldOfView		= VIEW_FIELD_FULL;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState		= MONSTERSTATE_NONE;

	// a monster maker's weapon type picks the head
	if ( pev->weapons )
		pev->body = pev->weapons;

	if ( pev->body < 0 )
		pev->body = RANDOM_LONG( 0, 3 );

	MonsterInit();
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CFlyingHead :: Precache()
{
	PRECACHE_MODEL("models/heads.mdl");

	PRECACHE_SOUND_ARRAY( pPainSounds );
	PRECACHE_SOUND( "scientist/holdstill.wav" );
	PRECACHE_SOUND( "scientist/scream1.wav" );

	m_flNextBlood = gpGlobals->time + 1;
}


//=========================================================
// CheckRangeAttack1  - the eye lasers
//=========================================================
BOOL CFlyingHead :: CheckRangeAttack1 ( float flDot, float flDist )
{
	if ( flDot > 0.5 && flDist <= 1024 && gpGlobals->time >= m_flNextAttack )
	{
		return TRUE;
	}
	return FALSE;
}


BOOL CFlyingHead :: CheckRangeAttack2 ( float flDot, float flDist )
{
	return FALSE;
}


//=========================================================
// StartTask
//=========================================================
void CFlyingHead :: StartTask ( Task_t *pTask )
{
	// the neck sprays blood once, a second after the head came
	if ( m_flNextBlood > 0 && m_flNextBlood < gpGlobals->time )
	{
		PLAYBACK_EVENT_FULL( 0, edict(), g_iBloodSprayEvent, 0.0, (float *)&pev->origin, (float *)&pev->angles, 0.0, 0.0, 2, 0, 0, 0 );
		m_flNextBlood = 0;
	}

	switch ( pTask->iTask )
	{
	case TASK_RANGE_ATTACK1:
		m_flShootEnd = gpGlobals->time + 2;
		m_flNextAttack = gpGlobals->time + RANDOM_FLOAT( 3, 5 );
		pev->sequence = LookupActivity( ACT_RANGE_ATTACK1 );
		pev->frame = 0;
		ResetSequenceInfo( );
		break;
	case TASK_GET_PATH_TO_ENEMY_LKP:
		{
			if (BuildNearestRoute( m_vecEnemyLKP, pev->view_ofs, pTask->flData, (m_vecEnemyLKP - pev->origin).Length() + 1024 ))
			{
				TaskComplete();
			}
			else
			{
				// no way to get there =(
				ALERT ( at_aiconsole, "GetPathToEnemyLKP failed!!\n" );
				TaskFail();
			}
			break;
		}
	case TASK_GET_PATH_TO_ENEMY:
		{
			CBaseEntity *pEnemy = m_hEnemy;

			if ( pEnemy == NULL )
			{
				TaskFail();
				return;
			}

			if (BuildNearestRoute( pEnemy->pev->origin, pEnemy->pev->view_ofs, pTask->flData, (pEnemy->pev->origin - pev->origin).Length() + 1024 ))
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
	default:
		CBaseMonster :: StartTask ( pTask );
		break;
	}
}


//=========================================================
// RunTask
//=========================================================
void CFlyingHead :: RunTask ( Task_t *pTask )
{
	if (m_flShootEnd > gpGlobals->time)
	{
		EyeLasers( );
	}
	else if (m_flShootEnd != 0)
	{
		// the lasers are done
		if (m_pBeam[0])
			UTIL_Remove( m_pBeam[0] );
		if (m_pBeam[1])
			UTIL_Remove( m_pBeam[1] );
		m_flShootEnd = 0;
		m_pBeam[1] = NULL;
		m_pBeam[0] = NULL;

		if ( pTask->iTask == TASK_RANGE_ATTACK1 )
			TaskComplete();
	}

	switch ( pTask->iTask )
	{
	case TASK_WAIT_FOR_MOVEMENT:
	case TASK_WAIT:
	case TASK_WAIT_FACE_ENEMY:
	case TASK_WAIT_PVS:
		MakeIdealYaw( m_vecEnemyLKP );
		ChangeYaw( pev->yaw_speed );

		CBaseMonster :: RunTask ( pTask );

		if (m_flShootEnd == 0)
		{
			if (HasConditions ( bits_COND_CAN_RANGE_ATTACK1 ))
			{
				pev->sequence = LookupActivity( ACT_RANGE_ATTACK1 );
				pev->frame = 0;
				ResetSequenceInfo( );
				m_flShootEnd = gpGlobals->time + 2;
				m_flNextAttack = gpGlobals->time + RANDOM_FLOAT( 3, 5 );
			}
		}
		else if (m_fSequenceFinished)
		{
			pev->sequence = LookupActivity( ACT_WALK );
			pev->frame = 0;
			ResetSequenceInfo( );
		}
		break;
	case TASK_RANGE_ATTACK1:
		if (m_flShootEnd > gpGlobals->time)
		{
			MakeIdealYaw( m_vecEnemyLKP );
			ChangeYaw( pev->yaw_speed );
		}
		break;
	default:
		CBaseMonster :: RunTask ( pTask );
		break;
	}
}


//=========================================================
//=========================================================
Schedule_t* CFlyingHead :: GetScheduleOfType ( int Type )
{
	// ALERT( at_console, "%d\n", m_iFrustration );
	switch	( Type )
	{
	case SCHED_CHASE_ENEMY:
		return slControllerChaseEnemy;
	case SCHED_RANGE_ATTACK1:
		return slControllerStrafe;
	case SCHED_RANGE_ATTACK2:
	case SCHED_MELEE_ATTACK1:
	case SCHED_MELEE_ATTACK2:
	case SCHED_TAKE_COVER_FROM_ENEMY:
		return slControllerTakeCover;
	case SCHED_FAIL:
		return slControllerFail;
	}

	return CBaseMonster :: GetScheduleOfType( Type );
}


void CFlyingHead::Stop( void )
{
	m_IdealActivity = GetStoppedActivity();
}


#define DIST_TO_CHECK	200
void CFlyingHead :: Move ( float flInterval )
{
	float		flWaypointDist;
	float		flCheckDist;
	float		flDist;// how far the lookahead check got before hitting an object.
	float		flMoveDist;
	Vector		vecDir;
	Vector		vecApex;
	CBaseEntity	*pTargetEnt;

	// Don't move if no valid route
	if ( FRouteClear() )
	{
		ALERT( at_aiconsole, "Tried to move with no route!\n" );
		TaskFail();
		return;
	}

	if ( m_flMoveWaitFinished > gpGlobals->time )
		return;

	// if the monster is moving directly towards an entity (enemy for instance), we'll set this pointer
	// to that entity for the CheckLocalMove and Triangulate functions.
	pTargetEnt = NULL;

	if (m_flGroundSpeed == 0)
	{
		m_flGroundSpeed = 100;
		// TaskFail( );
		// return;
	}

	flMoveDist = m_flGroundSpeed * flInterval;

	do
	{
		// local move to waypoint.
		vecDir = ( m_Route[ m_iRouteIndex ].vecLocation - pev->origin ).Normalize();
		flWaypointDist = ( m_Route[ m_iRouteIndex ].vecLocation - pev->origin ).Length();

		// MakeIdealYaw ( m_Route[ m_iRouteIndex ].vecLocation );
		// ChangeYaw ( pev->yaw_speed );

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
					// Wait for a second
					m_flMoveWaitFinished = gpGlobals->time + m_moveWaitTime;
	//				ALERT( at_aiconsole, "Move %s!!!\n", STRING( pBlocker->pev->classname ) );
					return;
				}
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
	 			    ALERT ( at_aiconsole, "Couldn't Triangulate\n" );
					Stop();
					if ( m_moveWaitTime > 0 )
					{
						FRefreshRoute();
						m_flMoveWaitFinished = gpGlobals->time + m_moveWaitTime * 0.5;
					}
					else
					{
						TaskFail();
						ALERT( at_aiconsole, "Failed to move!\n" );
						//ALERT( at_aiconsole, "%f, %f, %f\n", pev->origin.z, (pev->origin + (vecDir * flCheckDist)).z, m_Route[m_iRouteIndex].vecLocation.z );
					}
					return;
				}
			}
		}

		// UNDONE: this is a hack to quit moving farther than it has looked ahead.
		if (flCheckDist < flMoveDist)
		{
			MoveExecute( pTargetEnt, vecDir, flCheckDist / m_flGroundSpeed );

			// ALERT( at_console, "%.02f\n", flInterval );
			AdvanceRoute( flWaypointDist );
			flMoveDist -= flCheckDist;
		}
		else
		{
			MoveExecute( pTargetEnt, vecDir, flMoveDist / m_flGroundSpeed );

			if ( ShouldAdvanceRoute( flWaypointDist - flMoveDist ) )
			{
				AdvanceRoute( flWaypointDist );
			}
			flMoveDist = 0;
		}

		if ( MovementIsComplete() )
		{
			Stop();
			RouteClear();
		}
	} while (flMoveDist > 0 && flCheckDist > 0);

	// cut corner?
	if (flWaypointDist < 128)
	{
		if ( m_movementGoal == MOVEGOAL_ENEMY )
			RouteSimplify( m_hEnemy );
		else
			RouteSimplify( m_hTargetEnt );
		FRefreshRoute();

		if (m_flGroundSpeed > 100)
			m_flGroundSpeed -= 40;
	}
	else
	{
		if (m_flGroundSpeed < 400)
			m_flGroundSpeed += 10;
	}
}



BOOL CFlyingHead:: ShouldAdvanceRoute( float flWaypointDist )
{
	if ( flWaypointDist <= 32  )
	{
		return TRUE;
	}

	return FALSE;
}


int CFlyingHead :: CheckLocalMove ( const Vector &vecStart, const Vector &vecEnd, CBaseEntity *pTarget, float *pflDist )
{
	TraceResult tr;

	UTIL_TraceHull( vecStart + Vector( 0, 0, 32), vecEnd + Vector( 0, 0, 32), dont_ignore_monsters, head_hull, edict(), &tr );

	// ALERT( at_console, "%.0f %.0f %.0f : ", vecStart.x, vecStart.y, vecStart.z );
	// ALERT( at_console, "%.0f %.0f %.0f\n", vecEnd.x, vecEnd.y, vecEnd.z );

	if (pflDist)
	{
		*pflDist = ( (tr.vecEndPos - Vector( 0, 0, 32 )) - vecStart ).Length();// get the distance.
	}

	// ALERT( at_console, "check %d %d %f\n", tr.fStartSolid, tr.fAllSolid, tr.flFraction );
	if (tr.fStartSolid || tr.flFraction < 1.0)
	{
		if ( pTarget && pTarget->edict() == gpGlobals->trace_ent )
			return LOCALMOVE_VALID;
		return LOCALMOVE_INVALID;
	}

	return LOCALMOVE_VALID;
}


void CFlyingHead::MoveExecute( CBaseEntity *pTargetEnt, const Vector &vecDir, float flInterval )
{
	if ( m_IdealActivity != m_movementActivity )
		m_IdealActivity = m_movementActivity;

	// ALERT( at_console, "move %.4f %.4f %.4f : %f\n", vecDir.x, vecDir.y, vecDir.z, flInterval );

	// float flTotal = m_flGroundSpeed * pev->framerate * flInterval;
	// UTIL_MoveToOrigin ( ENT(pev), m_Route[ m_iRouteIndex ].vecLocation, flTotal, MOVE_STRAFE );

	m_velocity = m_velocity * 0.8 + m_flGroundSpeed * vecDir * 0.2;

	UTIL_MoveToOrigin ( ENT(pev), pev->origin + m_velocity, m_velocity.Length() * flInterval, MOVE_NORMAL );

}


//=========================================================
// EyeLasers - a laser from each eye (attachments 0 and 1)
// along the head's aim: the Glasses head's are green, wide
// and do 3 damage, the others' red, thin and do 1
//=========================================================
void CFlyingHead :: EyeLasers( void )
{
	Vector vecSrc, vecAngles;
	TraceResult tr;

	vecAngles = pev->angles;

	if ( m_hEnemy )
	{
		Vector vecEyes = pev->origin + pev->view_ofs;
		vecAngles.x = m_flBeamPitch = UTIL_VecToAngles( m_hEnemy->BodyTarget( pev->origin ) - vecEyes ).x;
	}
	else
		vecAngles.x = m_flBeamPitch;

	UTIL_MakeAimVectors( vecAngles );

	for ( int i = 0; i < 2; i++ )
	{
		GetAttachment( i, vecSrc, vecAngles );
		UTIL_TraceLine( vecSrc, vecSrc + gpGlobals->v_forward * 1024, dont_ignore_monsters, ignore_glass, edict(), &tr );

		if ( m_pBeam[i] == NULL )
		{
			if ( pev->body )
			{
				m_pBeam[i] = CBeam::BeamCreate( g_pModelNameLaser, 5 );
				m_pBeam[i]->SetColor( 255, 16, 16 );
			}
			else
			{
				m_pBeam[i] = CBeam::BeamCreate( g_pModelNameLaser, 10 );
				m_pBeam[i]->SetColor( 16, 255, 16 );
			}
			m_pBeam[i]->PointEntInit( pev->origin, entindex() );
			m_pBeam[i]->SetEndAttachment( i + 1 );
			m_pBeam[i]->SetBrightness( 192 );
			m_pBeam[i]->SetScrollRate( 35 );
			m_pBeam[i]->m_iszKillDesc = MAKE_STRING( "laser eye" );
			m_pBeam[i]->pev->owner = edict();
			m_pBeam[i]->pev->spawnflags |= SF_BEAM_SPARKSTART | SF_BEAM_DECALS;
		}

		m_pBeam[i]->pev->dmg = pev->body ? 1 : 3;
		m_pBeam[i]->pev->dmgtime = gpGlobals->time - 1;
		m_pBeam[i]->BeamDamage( &tr );
		m_pBeam[i]->SetStartPos( tr.vecEndPos );
		m_pBeam[i]->DoSparks( tr.vecEndPos, g_vecZero );
	}
}

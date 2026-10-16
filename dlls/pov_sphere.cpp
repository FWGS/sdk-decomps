/***
*
*	Copyright (c) 1999, Valve LLC. All rights reserved.
*
*	This product contains software technology licensed from Id
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc.
*	All Rights Reserved.
*
*   This source code contains proprietary and confidential information of
*   Valve LLC and its suppliers.  Access to this code is restricted to
*   persons who have executed a written SDK license with Valve.  Any access,
*   use or distribution of this code by or to any unlicensed person is illegal.
*
****/
//=========================================================
// POV: the XTBS sphere, a small flying alien that charges
// its eight arm beams and zaps. Movement and schedules from
// the controller, beams from the alien slave, the crash
// from the apache. File name and the names retail doesn't
// give are ours.
//=========================================================

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"effects.h"
#include	"schedule.h"
#include	"weapons.h"
#include	"soundent.h"
#include	"decals.h"

#define SPHERE_MAX_BEAMS	8
#define SPHERE_RECHARGES	5	// arm beam glows before the zap

// m_iBeamState (names ours)
#define SPHERE_BEAMS_OFF		0
#define SPHERE_BEAMS_CHARGE		1
#define SPHERE_BEAMS_ZAP		2
#define SPHERE_BEAMS_DONE		3
#define SPHERE_BEAMS_SETTLE		4

class CSphere : public CBaseMonster
{
public:
	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	void Spawn( void );
	void Precache( void );
	void SetYawSpeed( void );
	int  Classify ( void );
	int	 BloodColor( void ) { return DONT_BLEED; }
	void SetObjectCollisionBox( void )
	{
		pev->absmin = pev->origin + Vector( -12, -12, 0 );
		pev->absmax = pev->origin + Vector( 12, 12, 12 );
	}

	BOOL CheckRangeAttack1 ( float flDot, float flDist );
	Schedule_t* GetSchedule ( void );
	Schedule_t* GetScheduleOfType ( int Type );
	void StartTask ( Task_t *pTask );
	void RunTask ( Task_t *pTask );
	CUSTOM_SCHEDULES;

	void MonsterThink( void );
	void Stop( void );
	void Move ( float flInterval );
	int  CheckLocalMove ( const Vector &vecStart, const Vector &vecEnd, CBaseEntity *pTarget, float *pflDist );
	void MoveExecute( CBaseEntity *pTargetEnt, const Vector &vecDir, float flInterval );
	void SetActivity ( Activity NewActivity );
	BOOL ShouldAdvanceRoute( float flWaypointDist );

	void PainSound( void );
	void AlertSound( void );
	void IdleSound( void );

	static const char *pIdleSounds[];
	static const char *pAlertSounds[];
	static const char *pPainSounds[];

	int TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );
	void Killed( entvars_t *pevAttacker, int iGib );

	void EXPORT SphereDyingThink( void );
	void EXPORT SphereCrashTouch( CBaseEntity *pOther );

	void StartMotorSound( void );
	void StopMotorSound( void );
	void FaceEnemy( void );
	void ClearBeams( void );
	void ArmBeams( void );
	void BeamGlow( void );
	void ZapBeam( void );
	void StartAttack( void );
	void AttackThink( void );
	void Fire( void );
	void StopAttack( void );
	BOOL CanAttack( void );

	CBeam *m_pBeam[SPHERE_MAX_BEAMS];
	int m_iBeams;
	int m_iRechargeTimes;
	int m_iBeamState;

	float m_flMotorSoundTime;
	int m_iExplode;
	int m_iBigExplode;
	int m_iBodyGibs;

	Vector m_vecEstVelocity;

	Vector m_velocity;
	BOOL m_fMotorSoundOn;
	int m_flHitSomething;	// 1: hit the world, 2: hit something else
};

LINK_ENTITY_TO_CLASS( monster_alien_sphere, CSphere );

TYPEDESCRIPTION	CSphere::m_SaveData[] =
{
	DEFINE_FIELD( CSphere, m_flHitSomething, FIELD_INTEGER ),
	DEFINE_ARRAY( CSphere, m_pBeam, FIELD_CLASSPTR, SPHERE_MAX_BEAMS ),
	DEFINE_FIELD( CSphere, m_iBeams, FIELD_INTEGER ),
	DEFINE_FIELD( CSphere, m_iRechargeTimes, FIELD_INTEGER ),
	DEFINE_FIELD( CSphere, m_iBeamState, FIELD_INTEGER ),
	DEFINE_FIELD( CSphere, m_vecEstVelocity, FIELD_VECTOR ),
};
IMPLEMENT_SAVERESTORE( CSphere, CBaseMonster );


const char *CSphere::pIdleSounds[] =
{
	"sphere/sph_idle1.wav",
	"sphere/sph_idle2.wav",
};

const char *CSphere::pAlertSounds[] =
{
	"sphere/sph_alert1.wav",
};

const char *CSphere::pPainSounds[] =
{
	"sphere/sph_pain1.wav",
};


//=========================================================
// IdleSound - and a spark from an arm to nearby geometry
//=========================================================
void CSphere :: IdleSound( void )
{
	EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, pIdleSounds[ RANDOM_LONG(0,ARRAYSIZE(pIdleSounds)-1) ], 1.0, ATTN_STATIC, 0, PITCH_NORM );

	TraceResult tr;
	float flDist = 1.0;

	UTIL_MakeAimVectors( pev->angles );
	Vector vecSrc = pev->origin + gpGlobals->v_up * 36 + gpGlobals->v_right * 16 + gpGlobals->v_forward * 32;

	Vector vecAim = gpGlobals->v_right * RANDOM_FLOAT( 0, 1 ) + gpGlobals->v_up * RANDOM_FLOAT( -1, 1 );
	TraceResult tr1;
	UTIL_TraceLine ( vecSrc, vecSrc + vecAim * 512, dont_ignore_monsters, ENT( pev ), &tr1);
	if (flDist > tr1.flFraction)
	{
		tr = tr1;
		flDist = tr.flFraction;
	}

	// Couldn't find anything close enough
	if ( flDist == 1.0 )
		return;

	DecalGunshot( &tr, BULLET_PLAYER_CROWBAR );

	CBeam *pBeam = CBeam::BeamCreate( "sprites/lgtning.spr", 30 );
	if (!pBeam)
		return;

	pBeam->PointEntInit( tr.vecEndPos, entindex( ) );
	pBeam->SetColor( 96, 96, 96 );
	pBeam->SetBrightness( 64 );
	pBeam->SetNoise( 40 );
	pBeam->LiveForTime( 1.2 );
	pBeam->pev->spawnflags |= SF_BEAM_TEMPORARY;

	EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, "debris/zap1.wav", 1.0, ATTN_STATIC, 0, PITCH_NORM );
}

void CSphere :: AlertSound( void )
{
	EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, pAlertSounds[ RANDOM_LONG(0,ARRAYSIZE(pAlertSounds)-1) ], 1.0, ATTN_STATIC, 0, PITCH_NORM );
}

void CSphere :: PainSound( void )
{
	EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, pPainSounds[ RANDOM_LONG(0,ARRAYSIZE(pPainSounds)-1) ], 1.0, ATTN_STATIC, 0, PITCH_NORM );
}

//=========================================================
// StartMotorSound - the hum starts half a second after the
// first think
//=========================================================
void CSphere :: StartMotorSound( void )
{
	if ( !m_fMotorSoundOn && m_flMotorSoundTime <= gpGlobals->time )
	{
		if ( m_flMotorSoundTime == 0 )
		{
			m_flMotorSoundTime = gpGlobals->time + 0.5;
			return;
		}

		EMIT_SOUND_DYN( ENT(pev), CHAN_STATIC, "sphere/sph_motor1.wav", 0.4, ATTN_IDLE, 0, 90 );
		m_fMotorSoundOn = TRUE;
	}
}

void CSphere :: StopMotorSound( void )
{
	EMIT_SOUND_DYN( ENT(pev), CHAN_STATIC, "sphere/sph_motor1.wav", 0.4, ATTN_IDLE, SND_STOP, 90 );
}

//=========================================================
// Classify - indicates this monster's place in the
// relationship table.
//=========================================================
int	CSphere :: Classify ( void )
{
	return	CLASS_ALIEN_MONSTER;
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CSphere :: SetYawSpeed ( void )
{
	pev->yaw_speed = 100;
}

//=========================================================
// TakeDamage - answer the player's shots
//=========================================================
int CSphere :: TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	if ( m_MonsterState != MONSTERSTATE_PRONE && !(m_afMemory & bits_MEMORY_PROVOKED) )
	{
		switch ( UTIL_PlayerProvoke( this, pevAttacker ) )
		{
		case 1:
			PainSound( );
		case 2:
			AlertSound( );
			Remember( bits_MEMORY_SUSPICIOUS );
			break;
		case 3:
			AlertSound( );
			Remember( bits_MEMORY_PROVOKED );
			break;
		}
	}

	return CBaseMonster::TakeDamage( pevInflictor, pevAttacker, flDamage, bitsDamageType );
}

//=========================================================
// Killed - fall down, sparking
//=========================================================
void CSphere :: Killed( entvars_t *pevAttacker, int iGib )
{
	StopAttack( );

	pev->movetype = MOVETYPE_TOSS;
	pev->gravity = 0.3;

	StopMotorSound( );
	EMIT_SOUND_DYN( ENT(pev), CHAN_STATIC, "ambience/sparks.wav", 0.6, ATTN_IDLE, 0, 95 );

	SetThink( SphereDyingThink );
	SetTouch( SphereCrashTouch );
	pev->nextthink = gpGlobals->time + 0.1;
	pev->health = 0;
	pev->takedamage = DAMAGE_NO;
	m_flHitSomething = 0;
}

void CSphere :: SphereDyingThink( void )
{
	pev->nextthink = gpGlobals->time + 0.1;

	pev->avelocity = pev->avelocity * 1.02;

	// still falling?
	if ( !m_flHitSomething )
	{
		UTIL_Sparks( pev->origin );
		pev->nextthink = gpGlobals->time + 0.1;
	}
	else
	{
		EMIT_SOUND_DYN( ENT(pev), CHAN_STATIC, "ambience/sparks.wav", 0.6, ATTN_IDLE, SND_STOP, 95 );

		MESSAGE_BEGIN( MSG_PAS, SVC_TEMPENTITY, pev->origin );
			WRITE_BYTE( TE_EXPLOSION );
			WRITE_COORD( pev->origin.x );
			WRITE_COORD( pev->origin.y );
			WRITE_COORD( pev->origin.z );
			if ( m_flHitSomething == 2 )
			{
				WRITE_SHORT( m_iBigExplode );
				WRITE_BYTE( 10 ); // scale * 10
			}
			else
			{
				WRITE_SHORT( m_iExplode );
				WRITE_BYTE( 8 ); // scale * 10
			}
			WRITE_BYTE( 15 ); // framerate
			WRITE_BYTE( TE_EXPLFLAG_NONE );
		MESSAGE_END();

		// gibs
		MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, pev->origin );
			WRITE_BYTE( TE_BREAKMODEL);

			// position
			WRITE_COORD( pev->origin.x );
			WRITE_COORD( pev->origin.y );
			WRITE_COORD( pev->origin.z );

			// size
			WRITE_COORD( 10 );
			WRITE_COORD( 10 );
			WRITE_COORD( 10 );

			// velocity
			WRITE_COORD( 0 );
			WRITE_COORD( 0 );
			WRITE_COORD( RANDOM_LONG( 80, 100 ) );

			// randomization
			WRITE_BYTE( 10 );

			// Model
			WRITE_SHORT( m_iBodyGibs );	//model id#

			// # of shards
			WRITE_BYTE( RANDOM_LONG( 10, 15 ) );

			// duration
			WRITE_BYTE( 50 );// 5.0 seconds

			// flags
			WRITE_BYTE( BREAK_METAL );
		MESSAGE_END();

		RadiusDamage( pev->origin, pev, pev, 40, CLASS_NONE, DMG_BLAST );

		FCheckAITrigger( );

		SetThink( SUB_Remove );
		pev->nextthink = gpGlobals->time + 0.1;
	}
}

void CSphere :: SphereCrashTouch( CBaseEntity *pOther )
{
	SetTouch( NULL );
	pev->nextthink = gpGlobals->time;

	m_flHitSomething = 2;
	if ( pOther && pOther->pev->solid == SOLID_BSP )
		m_flHitSomething = 1;
}

//=========================================================
// Spawn
//=========================================================
void CSphere :: Spawn()
{
	Precache( );

	SET_MODEL(ENT(pev), "models/sphere.mdl");
	UTIL_SetSize( pev, Vector( -8, -8, -8 ), Vector( 8, 8, 8 ) );

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_FLY;
	pev->flags			|= FL_FLY;
	m_bloodColor		= DONT_BLEED;
	pev->health			= gSkillData.sphereHealth;
	pev->view_ofs		= Vector( 0, 0, 0 );
	m_flFieldOfView		= VIEW_FIELD_FULL;
	m_MonsterState		= MONSTERSTATE_NONE;
	m_afCapability		= bits_CAP_STRAFE | bits_CAP_HEAR | bits_CAP_RANGE_ATTACK1 | bits_CAP_FLY;
	pev->yaw_speed		= 200;
	m_flGroundSpeed		= 200;

	m_flMotorSoundTime	= gpGlobals->time + 1.5;
	m_fMotorSoundOn		= FALSE;

	MonsterInit();
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CSphere :: Precache()
{
	PRECACHE_MODEL("models/sphere.mdl");
	PRECACHE_MODEL("sprites/lgtning.spr");

	PRECACHE_SOUND_ARRAY( pIdleSounds );
	PRECACHE_SOUND_ARRAY( pAlertSounds );
	PRECACHE_SOUND_ARRAY( pPainSounds );

	PRECACHE_SOUND("sphere/sph_motor1.wav");
	PRECACHE_SOUND("ambience/sparks.wav");
	PRECACHE_SOUND("debris/zap4.wav");
	PRECACHE_SOUND("debris/zap1.wav");
	PRECACHE_SOUND("weapons/electro4.wav");

	m_iExplode = PRECACHE_MODEL( "sprites/fexplo.spr" );
	m_iBigExplode = PRECACHE_MODEL( "sprites/bexplo.spr" );
	m_iBodyGibs = PRECACHE_MODEL( "models/spheregibs.mdl" );
}

//=========================================================
// AI Schedules Specific to this monster
//=========================================================


// Chase enemy schedule
Task_t tlSphereChaseEnemy[] =
{
	{ TASK_GET_PATH_TO_ENEMY,	(float)128		},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_WAIT_FOR_MOVEMENT,	(float)0		},

};

Schedule_t slSphereChaseEnemy[] =
{
	{
		tlSphereChaseEnemy,
		ARRAYSIZE ( tlSphereChaseEnemy ),
		bits_COND_NEW_ENEMY			|
		bits_COND_TASK_FAILED,
		0,
		"SphereChaseEnemy"
	},
};



Task_t	tlSphereStrafe[] =
{
	{ TASK_WAIT,					(float)0.2					},
	{ TASK_GET_PATH_TO_ENEMY,		(float)128					},
	{ TASK_FACE_ENEMY,				(float)0					},
	{ TASK_WAIT_FOR_MOVEMENT,		(float)0					},
	{ TASK_WAIT,					(float)1					},
};

Schedule_t	slSphereStrafe[] =
{
	{
		tlSphereStrafe,
		ARRAYSIZE ( tlSphereStrafe ),
		bits_COND_NEW_ENEMY,
		0,
		"SphereStrafe"
	},
};


Task_t	tlSphereTakeCover[] =
{
	{ TASK_WAIT,					(float)0.2					},
	{ TASK_FIND_COVER_FROM_ENEMY,	(float)0					},
	{ TASK_FACE_ENEMY,				(float)0					},
	{ TASK_WAIT_FOR_MOVEMENT,		(float)0					},
	{ TASK_WAIT,					(float)1					},
};

Schedule_t	slSphereTakeCover[] =
{
	{
		tlSphereTakeCover,
		ARRAYSIZE ( tlSphereTakeCover ),
		bits_COND_NEW_ENEMY,
		0,
		"SphereTakeCover"
	},
};


Task_t	tlSphereFail[] =
{
	{ TASK_STOP_MOVING,			0				},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE },
	{ TASK_WAIT,				(float)2		},
	{ TASK_WAIT_PVS,			(float)0		},
};

Schedule_t	slSphereFail[] =
{
	{
		tlSphereFail,
		ARRAYSIZE ( tlSphereFail ),
		0,
		0,
		"SphereFail"
	},
};



DEFINE_CUSTOM_SCHEDULES( CSphere )
{
	slSphereChaseEnemy,
	slSphereStrafe,
	slSphereTakeCover,
	slSphereFail,
};

IMPLEMENT_CUSTOM_SCHEDULES( CSphere, CBaseMonster );



//=========================================================
// StartTask
//=========================================================
void CSphere :: StartTask ( Task_t *pTask )
{
	switch ( pTask->iTask )
	{
	case TASK_RANGE_ATTACK1:
		CBaseMonster :: StartTask ( pTask );
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
void CSphere :: RunTask ( Task_t *pTask )
{
	if ( CanAttack() && m_iBeamState == SPHERE_BEAMS_OFF )
	{
		m_flGroundSpeed = 200;
		StartAttack( );
	}

	AttackThink( );

	switch ( pTask->iTask )
	{
	case TASK_WAIT_FOR_MOVEMENT:
	case TASK_WAIT:
	case TASK_WAIT_FACE_ENEMY:
	case TASK_WAIT_PVS:
		FaceEnemy( );
		break;
	}

	CBaseMonster :: RunTask ( pTask );
}


void CSphere :: MonsterThink( void )
{
	StartMotorSound( );
	CBaseMonster :: MonsterThink( );
}


//=========================================================
// GetSchedule - Decides which type of schedule best suits
// the monster's current state and conditions. Then calls
// monster's member function to get a pointer to a schedule
// of the proper type.
//=========================================================
Schedule_t *CSphere :: GetSchedule ( void )
{
	if ( m_iBeamState != SPHERE_BEAMS_OFF )
		StopAttack( );

	switch	( m_MonsterState )
	{
	case MONSTERSTATE_IDLE:
		m_flGroundSpeed = 100;
		break;

	case MONSTERSTATE_ALERT:
		AlertSound( );
		m_flGroundSpeed = 200;
		break;

	case MONSTERSTATE_COMBAT:
		{
			// dead enemy
			if ( HasConditions( bits_COND_ENEMY_DEAD ) )
			{
				m_flGroundSpeed = 100;
				StopAttack( );
				return CBaseMonster :: GetSchedule();
			}

			if ( HasConditions( bits_COND_NEW_ENEMY ) )
			{
				m_flGroundSpeed = 200;
				AlertSound( );
			}

			if ( HasConditions( bits_COND_CAN_RANGE_ATTACK1 ) )
			{
				m_flGroundSpeed = 200;
				StartAttack( );
				return GetScheduleOfType( SCHED_RANGE_ATTACK1 );
			}

			StopAttack( );
			return GetScheduleOfType( SCHED_CHASE_ENEMY );
		}
		break;
	}

	return CBaseMonster :: GetSchedule();
}



//=========================================================
//=========================================================
Schedule_t* CSphere :: GetScheduleOfType ( int Type )
{
	switch	( Type )
	{
	case SCHED_CHASE_ENEMY:
		return slSphereChaseEnemy;
	case SCHED_RANGE_ATTACK1:
		return slSphereStrafe;
	case SCHED_RANGE_ATTACK2:
	case SCHED_MELEE_ATTACK1:
	case SCHED_MELEE_ATTACK2:
	case SCHED_TAKE_COVER_FROM_ENEMY:
		return slSphereTakeCover;
	case SCHED_FAIL:
		return slSphereFail;
	}

	return CBaseMonster :: GetScheduleOfType( Type );
}


//=========================================================
// CheckRangeAttack1 - zap
//=========================================================
BOOL CSphere :: CheckRangeAttack1 ( float flDot, float flDist )
{
	if ( flDot > 0.5 && flDist < 1024 )
	{
		return TRUE;
	}
	return FALSE;
}


void CSphere :: SetActivity ( Activity NewActivity )
{
	CBaseMonster::SetActivity( NewActivity );

	switch ( m_Activity)
	{
	case ACT_WALK:
		m_flGroundSpeed = 100;
		break;
	default:
		m_flGroundSpeed = 100;
		break;
	}
}


void CSphere::Stop( void )
{
	m_IdealActivity = GetStoppedActivity();
}


#define DIST_TO_CHECK	200
void CSphere :: Move ( float flInterval )
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

		FaceEnemy( );

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
			m_flGroundSpeed -= 10;
	}
	else
	{
		if (m_flGroundSpeed < 200)
			m_flGroundSpeed += 10;
	}
}



BOOL CSphere:: ShouldAdvanceRoute( float flWaypointDist )
{
	if ( flWaypointDist <= 32  )
	{
		return TRUE;
	}

	return FALSE;
}


int CSphere :: CheckLocalMove ( const Vector &vecStart, const Vector &vecEnd, CBaseEntity *pTarget, float *pflDist )
{
	TraceResult tr;

	UTIL_TraceHull( vecStart + Vector( 0, 0, 32), vecEnd + Vector( 0, 0, 32), dont_ignore_monsters, large_hull, edict(), &tr );

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


void CSphere::MoveExecute( CBaseEntity *pTargetEnt, const Vector &vecDir, float flInterval )
{
	if ( m_IdealActivity != m_movementActivity )
		m_IdealActivity = m_movementActivity;

	// ALERT( at_console, "move %.4f %.4f %.4f : %f\n", vecDir.x, vecDir.y, vecDir.z, flInterval );

	// float flTotal = m_flGroundSpeed * pev->framerate * flInterval;
	// UTIL_MoveToOrigin ( ENT(pev), m_Route[ m_iRouteIndex ].vecLocation, flTotal, MOVE_STRAFE );

	m_velocity = m_velocity * 0.8 + m_flGroundSpeed * vecDir * 0.2;

	UTIL_MoveToOrigin ( ENT(pev), pev->origin + m_velocity, m_velocity.Length() * flInterval, MOVE_STRAFE );

}


//=========================================================
// FaceEnemy - turn towards the enemy, or along the route
//=========================================================
void CSphere :: FaceEnemy( void )
{
	if ( m_hEnemy == NULL )
		MakeIdealYaw( m_Route[ m_iRouteIndex ].vecLocation );
	else
		MakeIdealYaw( m_vecEnemyLKP );

	ChangeYaw( pev->yaw_speed );
}


//=========================================================
// ClearBeams - remove all beams
//=========================================================
void CSphere :: ClearBeams( void )
{
	for (int i = 0; i < SPHERE_MAX_BEAMS; i++)
	{
		if (m_pBeam[i])
		{
			UTIL_Remove( m_pBeam[i] );
			m_pBeam[i] = NULL;
		}
	}
	m_iBeams = 0;

	EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, "debris/zap4.wav", 1.0, ATTN_STATIC, SND_STOP, PITCH_NORM );
}


//=========================================================
// ArmBeams - eight small beams from the sphere to nearby
// geometry
//=========================================================
void CSphere :: ArmBeams( void )
{
	for ( m_iBeams = 0; m_iBeams < SPHERE_MAX_BEAMS; m_iBeams++ )
	{
		TraceResult tr;
		float flDist = 1.0;

		UTIL_MakeAimVectors( pev->angles );
		Vector vecSrc = pev->origin + gpGlobals->v_up * 36 + gpGlobals->v_right * 16 + gpGlobals->v_forward * 32;

		for (int i = 0; i < 3; i++)
		{
			Vector vecAim = gpGlobals->v_right * RANDOM_FLOAT( 0, 1 ) + gpGlobals->v_up * RANDOM_FLOAT( -1, 1 );
			TraceResult tr1;
			UTIL_TraceLine ( vecSrc, vecSrc + vecAim * 512, dont_ignore_monsters, ENT( pev ), &tr1);
			if (flDist > tr1.flFraction)
			{
				tr = tr1;
				flDist = tr.flFraction;
			}
		}

		// Couldn't find anything close enough
		if ( flDist == 1.0 )
			continue;

		DecalGunshot( &tr, BULLET_PLAYER_CROWBAR );

		m_pBeam[m_iBeams] = CBeam::BeamCreate( "sprites/lgtning.spr", 30 );
		if (!m_pBeam[m_iBeams])
			continue;

		m_pBeam[m_iBeams]->PointEntInit( tr.vecEndPos, entindex( ) );
		m_pBeam[m_iBeams]->SetColor( 96, 96, 96 );
		m_pBeam[m_iBeams]->SetBrightness( 64 );
		m_pBeam[m_iBeams]->SetNoise( 80 );
	}

	EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, "debris/zap4.wav", 1.0, ATTN_STATIC, 0, PITCH_NORM );
}


//=========================================================
// BeamGlow - brighten all beams as the charge grows
//=========================================================
void CSphere :: BeamGlow( void )
{
	int b = (m_iRechargeTimes * 100 / SPHERE_RECHARGES) * (255 - 64) / 100 + 64;
	if (b > 255)
		b = 255;

	for (int i = 0; i < m_iBeams; i++)
	{
		if (m_pBeam[i])
		{
			if (m_pBeam[i]->GetBrightness() != 255)
			{
				m_pBeam[i]->SetBrightness( b );
			}
		}
	}
}


//=========================================================
// ZapBeam - heavy damage directly forward
//=========================================================
void CSphere :: ZapBeam( void )
{
	Vector vecSrc, vecAim;
	TraceResult tr;
	CBaseEntity *pEntity;

	vecSrc = pev->origin + gpGlobals->v_up * 36;
	vecAim = ShootAtEnemy( vecSrc );
	float deflection = 0.01;
	vecAim = vecAim + gpGlobals->v_right * RANDOM_FLOAT( 0, deflection ) + gpGlobals->v_up * RANDOM_FLOAT( -deflection, deflection );
	UTIL_TraceLine ( vecSrc, vecSrc + vecAim * 1024, dont_ignore_monsters, ENT( pev ), &tr);

	CBeam *pBeam = CBeam::BeamCreate( "sprites/lgtning.spr", 50 );
	if (!pBeam)
		return;

	pBeam->PointsInit( tr.vecEndPos, pev->origin );
	pBeam->SetColor( 128, 128, 128 );
	pBeam->SetBrightness( 255 );
	pBeam->SetNoise( 20 );
	pBeam->LiveForTime( 0.2 );
	pBeam->pev->spawnflags |= SF_BEAM_TEMPORARY;

	pEntity = CBaseEntity::Instance(tr.pHit);
	if (pEntity != NULL)
	{
		if (pEntity->pev->takedamage)
		{
			if (pEntity->IsPlayer())
			{
				ClearMultiDamage( );
				gMultiDamage.type = DMG_ENERGYBEAM | DMG_NEVERGIB;
				pEntity->TraceAttack( pev, gSkillData.sphereDmgZap, vecAim, &tr, DMG_ENERGYBEAM | DMG_NEVERGIB );
				ApplyMultiDamage( pEntity->pev, pev );
			}
			else
			{
				pEntity->TraceAttack( pev, gSkillData.sphereDmgZap, vecAim, &tr, DMG_SHOCK | DMG_NEVERGIB );
			}
		}
		else if (pEntity->Classify() == CLASS_NONE)
		{
			UTIL_DecalTrace( &tr, RANDOM_LONG( DECAL_SMALLSCORCH1, DECAL_SMALLSCORCH3 ) );
		}
	}

	EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, "weapons/electro4.wav", 0.5, ATTN_STATIC, 0, RANDOM_LONG( 140, 160 ) );
	CSoundEnt::InsertSound( bits_SOUND_COMBAT, pev->origin, 600, 0.2 );
}


//=========================================================
// StartAttack - put out the arm beams and charge them
//=========================================================
void CSphere :: StartAttack( void )
{
	if ( m_iBeamState == SPHERE_BEAMS_OFF )
	{
		StopAttack( );
		m_iBeamState = SPHERE_BEAMS_CHARGE;
		m_iRechargeTimes = 0;
		ArmBeams( );

		pev->sequence = 2;
		pev->frame = 0;
		pev->framerate = 1.0;
		ResetSequenceInfo( );
		pev->nextthink = gpGlobals->time + 0.2;
	}
}


//=========================================================
// AttackThink - charge, zap, settle (from RunTask)
//=========================================================
void CSphere :: AttackThink( void )
{
	if ( pev->sequence )
		StudioFrameAdvance( );

	if ( m_iBeamState == SPHERE_BEAMS_OFF )
		return;

	if ( m_iBeamState == SPHERE_BEAMS_DONE )
	{
		pev->sequence = 2;
		pev->framerate = -18;
		ResetSequenceInfo( );
		StopAttack( );
		pev->nextthink = gpGlobals->time + 0.1;
		return;
	}

	if ( m_iBeamState == SPHERE_BEAMS_SETTLE )
	{
		if ( m_fSequenceFinished )
		{
			pev->sequence = 0;
			pev->frame = 0;
			pev->framerate = 0;
			ResetSequenceInfo( );
			m_iBeamState = SPHERE_BEAMS_OFF;
		}
		pev->nextthink = gpGlobals->time + 0.1;
		return;
	}

	if ( m_iBeamState == SPHERE_BEAMS_CHARGE )
	{
		if ( m_iRechargeTimes == SPHERE_RECHARGES )
		{
			m_iBeamState = SPHERE_BEAMS_ZAP;
			Fire( );
			m_iBeamState = SPHERE_BEAMS_DONE;
			pev->nextthink = gpGlobals->time + 0.1;
			return;
		}
		m_iRechargeTimes++;
		BeamGlow( );
	}

	pev->nextthink = gpGlobals->time + 0.2;
}


//=========================================================
// Fire - the arm beams go, the zap goes out
//=========================================================
void CSphere :: Fire( void )
{
	if ( m_iBeamState == SPHERE_BEAMS_ZAP )
	{
		ClearBeams( );
		if ( !CanAttack( ) )
		{
			StopAttack( );
			return;
		}
		ZapBeam( );
	}
}


void CSphere :: StopAttack( void )
{
	ClearBeams( );
	if ( m_fSequenceFinished )
		m_iBeamState = SPHERE_BEAMS_OFF;
	else
		m_iBeamState = SPHERE_BEAMS_SETTLE;
}


BOOL CSphere :: CanAttack( void )
{
	if ( m_hEnemy == NULL || HasConditions( bits_COND_ENEMY_DEAD ) || HasConditions( bits_COND_ENEMY_OCCLUDED ) )
		return FALSE;

	if ( HasConditions( bits_COND_SEE_ENEMY ) && !HasConditions( bits_COND_CAN_RANGE_ATTACK1 ) )
		return FALSE;

	if ( HasConditions( bits_COND_CAN_RANGE_ATTACK1 ) )
		return TRUE;

	if ( HasConditions( bits_COND_NEW_ENEMY ) )
		return TRUE;

	if ( m_hEnemy != NULL )
		UTIL_FInViewCone( this, m_hEnemy, m_flFieldOfView );

	return FALSE;
}

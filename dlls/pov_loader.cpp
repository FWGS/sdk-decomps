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
// POV: monster_loader_armed, the armed loader: a walking
// cargo loader with the human assault's minigun and a
// grenade launcher. Made from barney.cpp, agrunt.cpp,
// hgrunt.cpp and apache.cpp. File name and the names
// retail doesn't give are ours.
//=========================================================

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"schedule.h"
#include	"animation.h"
#include	"weapons.h"
#include	"soundent.h"
#include	"explode.h"

//=========================================================
// monster-specific schedule types
//=========================================================
enum
{
	SCHED_ALOADER_ESTABLISH_LINE_OF_FIRE = LAST_COMMON_SCHEDULE + 1,// move to a location to set up an attack against the enemy. (usually when a friendly is in the way).
	SCHED_ALOADER_COVER_AND_RELOAD,
	SCHED_ALOADER_SWEEP,
	SCHED_ALOADER_ELOF_FAIL,
};

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define		ALOADER_AE_SHOOT		( 3 )
#define		ALOADER_AE_STEP			( 4 )
#define		ALOADER_AE_RELOAD		( 5 )

// gun states (m_iWeaponState), as the human assault's
#define		ALOADER_GUN_IDLE		0
#define		ALOADER_GUN_SPINUP		1
#define		ALOADER_GUN_FIRE		2
#define		ALOADER_GUN_SPINDOWN	3	// never set

class CALoader : public CBaseMonster
{
public:
	void Spawn( void );
	void Precache( void );
	void SetYawSpeed( void );
	int  ISoundMask( void );
	int  Classify ( void );
	int  IRelationship( CBaseEntity *pTarget );
	void HandleAnimEvent( MonsterEvent_t *pEvent );
	void GibMonster( void ) {}
	void SetObjectCollisionBox( void )
	{
		pev->absmin = pev->origin - Vector( 112, 112, 0 );
		pev->absmax = pev->origin + Vector( 112, 112, 160 );
	}

	void RunTask( Task_t *pTask );
	void StartTask( Task_t *pTask );
	void MonsterThink( void );
	BOOL FCanCheckAttacks ( void );
	BOOL CheckRangeAttack1 ( float flDot, float flDist );
	BOOL CheckRangeAttack2 ( float flDot, float flDist );
	void CheckAmmo ( void );
	void SetActivity ( Activity NewActivity );

	void AlertSound( void );
	void IdleSound( void );
	void PainSound( void );
	void DeathSound( void );
	void AttackSound( void );	// names ours
	void StepSound( void );
	void SparkSound( Vector vecLocation );
	void StartMotorSound( void );
	void StopMotorSound( void );

	void Shoot( void );
	void SpinUp( void );
	void FireGun( void );
	void SpinDown( void );
	void LaunchGrenade( void );

	void TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType);
	int  TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );
	void Killed( entvars_t *pevAttacker, int iGib );
	void EXPORT BlastKilled( void );
	void EXPORT DeadSmoke( void );
	void EXPORT LoaderTouch( CBaseEntity *pOther );

	// Override these to set behavior
	Schedule_t *GetScheduleOfType ( int Type );
	Schedule_t *GetSchedule ( void );

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	static const char *pIdleSounds[];	// names ours
	static const char *pAlertSounds[];
	static const char *pAttackSounds[];
	static const char *pPainSounds[];
	static const char *pDieSounds[];
	static const char *pStepSounds[];
	static const char *pSparkSounds[];

	float	m_painTime;
	float	m_checkAttackTime;
	BOOL	m_lastAttackCheck;
	int		m_cClipSize;
	int		m_voicePitch;
	int		m_iWeaponState;
	int		m_iExplode;		// names ours (not saved)
	int		m_iShell;
	int		m_iBodyGibs;
	int		m_iLegGibs;
	float	m_flNextFireTime;
	float	m_flCheckTargetTime;
	BOOL	m_bLastAttackCheck;
	BOOL	m_fMotorSound;		// names ours (not saved)
	float	m_flMotorTime;
	BOOL	m_flPlayDeathSequenceOne;
	float	m_flNextHurtTouch;
	Vector	m_vecTossVelocity;
	float	m_flNextGrenadeCheck;
	BOOL	m_fThrowGrenade;

	CUSTOM_SCHEDULES;
};

LINK_ENTITY_TO_CLASS( monster_loader_armed, CALoader );

TYPEDESCRIPTION	CALoader::m_SaveData[] =
{
	DEFINE_FIELD( CALoader, m_painTime, FIELD_TIME ),
	DEFINE_FIELD( CALoader, m_checkAttackTime, FIELD_TIME ),
	DEFINE_FIELD( CALoader, m_cClipSize, FIELD_INTEGER ),
	DEFINE_FIELD( CALoader, m_lastAttackCheck, FIELD_BOOLEAN ),
	DEFINE_FIELD( CALoader, m_flPlayDeathSequenceOne, FIELD_BOOLEAN ),
	DEFINE_FIELD( CALoader, m_voicePitch, FIELD_INTEGER ),
	DEFINE_FIELD( CALoader, m_iWeaponState, FIELD_INTEGER ),
	DEFINE_FIELD( CALoader, m_bLastAttackCheck, FIELD_BOOLEAN ),
	DEFINE_FIELD( CALoader, m_flNextGrenadeCheck, FIELD_TIME ),
	DEFINE_FIELD( CALoader, m_fThrowGrenade, FIELD_BOOLEAN ),
	DEFINE_FIELD( CALoader, m_vecTossVelocity, FIELD_VECTOR ),
	DEFINE_FIELD( CALoader, m_flNextHurtTouch, FIELD_TIME ),
};

IMPLEMENT_SAVERESTORE( CALoader, CBaseMonster );

const char *CALoader::pIdleSounds[] =
{
	"aloader/al_idle1.wav",
};

const char *CALoader::pAlertSounds[] =
{
	"aloader/al_alert1.wav",
};

const char *CALoader::pAttackSounds[] =
{
	"hassault/hw_fire2.wav",
};

const char *CALoader::pPainSounds[] =
{
	"weapons/xbow_hit1.wav",
};

const char *CALoader::pDieSounds[] =
{
	"debris/zap2.wav",
	"debris/zap6.wav",
};

const char *CALoader::pStepSounds[] =
{
	"ambience/loader_step1.wav",
};

const char *CALoader::pSparkSounds[] =
{
	"buttons/spark1.wav",
	"buttons/spark2.wav",
	"buttons/spark3.wav",
	"buttons/spark4.wav",
	"buttons/spark5.wav",
	"buttons/spark6.wav",
};

//=========================================================
// AI Schedules Specific to this monster
//=========================================================

//=========================================================
// reload
//=========================================================
Task_t	tlALoaderReload[] =
{
	{ TASK_STOP_MOVING,				(float)0					},
	{ TASK_SET_FAIL_SCHEDULE,		(float)SCHED_RELOAD			},
	{ TASK_PLAY_SEQUENCE,			(float)ACT_RELOAD			},
};

Schedule_t slALoaderReload[] =
{
	{
		tlALoaderReload,
		ARRAYSIZE ( tlALoaderReload ),
		bits_COND_HEAVY_DAMAGE	|
		bits_COND_HEAR_SOUND,

		bits_SOUND_DANGER,
		"ALoaderReload"
	}
};

//=========================================================
// Fail
//=========================================================
Task_t	tlALoaderFail[] =
{
	{ TASK_STOP_MOVING,			0				},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE },
	{ TASK_WAIT,				(float)2		},
	{ TASK_WAIT_PVS,			(float)0		},
};

Schedule_t	slALoaderFail[] =
{
	{
		tlALoaderFail,
		ARRAYSIZE ( tlALoaderFail ),
		bits_COND_CAN_RANGE_ATTACK1 |
		bits_COND_CAN_RANGE_ATTACK2,
		0,
		"ALoader Fail"
	},
};

//=========================================================
// Combat Fail
//=========================================================
Task_t	tlALoaderCombatFail[] =
{
	{ TASK_STOP_MOVING,			0				},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE },
	{ TASK_WAIT_FACE_ENEMY,		(float)2		},
	{ TASK_WAIT_PVS,			(float)0		},
};

Schedule_t	slALoaderCombatFail[] =
{
	{
		tlALoaderCombatFail,
		ARRAYSIZE ( tlALoaderCombatFail ),
		bits_COND_CAN_RANGE_ATTACK1	|
		bits_COND_CAN_RANGE_ATTACK2,
		0,
		"ALoader Combat Fail"
	},
};

//=========================================================
// primary range attack
//=========================================================
Task_t	tlALoaderRangeAttack1[] =
{
	{ TASK_STOP_MOVING,			(float)0		},
	{ TASK_FACE_IDEAL,			(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
};

Schedule_t	slALoaderRangeAttack1[] =
{
	{
		tlALoaderRangeAttack1,
		ARRAYSIZE ( tlALoaderRangeAttack1 ),
		bits_COND_NEW_ENEMY			|
		bits_COND_ENEMY_DEAD		|
		bits_COND_HEAVY_DAMAGE		|
		bits_COND_HEAR_SOUND,

		bits_SOUND_DANGER,
		"ALoaderEstablishLineOfFire"	// sic: the line of fire schedule's name, as the human assault's
	},
};

//=========================================================
// secondary range attack: the grenade launcher
//=========================================================
Task_t	tlALoaderRangeAttack2[] =
{
	{ TASK_STOP_MOVING,			(float)0		},
	{ TASK_FACE_IDEAL,			(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_RANGE_ATTACK2,		(float)0		},
};

Schedule_t	slALoaderRangeAttack2[] =
{
	{
		tlALoaderRangeAttack2,
		ARRAYSIZE ( tlALoaderRangeAttack2 ),
		bits_COND_NEW_ENEMY			|
		bits_COND_ENEMY_DEAD		|
		bits_COND_HEAVY_DAMAGE		|
		bits_COND_HEAR_SOUND,

		bits_SOUND_DANGER,
		"ALoaderRangeAttack2"
	},
};

//=========================================================
// Establish line of fire - move to a position that allows
// the loader to attack.
//=========================================================
Task_t tlALoaderEstablishLineOfFire[] =
{
	{ TASK_SET_FAIL_SCHEDULE,	(float)SCHED_ALOADER_ELOF_FAIL	},
	{ TASK_GET_PATH_TO_ENEMY,	(float)0						},
	{ TASK_RUN_PATH,			(float)0						},
	{ TASK_WAIT_FOR_MOVEMENT,	(float)0						},
};

Schedule_t slALoaderEstablishLineOfFire[] =
{
	{
		tlALoaderEstablishLineOfFire,
		ARRAYSIZE ( tlALoaderEstablishLineOfFire ),
		bits_COND_NEW_ENEMY			|
		bits_COND_ENEMY_DEAD		|
		bits_COND_CAN_RANGE_ATTACK1	|
		bits_COND_CAN_RANGE_ATTACK2	|
		bits_COND_HEAR_SOUND,

		bits_SOUND_DANGER,
		"ALoaderEstablishLineOfFire"
	},
};

//=========================================================
// Do a turning sweep of the area
//=========================================================
Task_t	tlALoaderSweep[] =
{
	{ TASK_TURN_LEFT,			(float)179	},
	{ TASK_WAIT,				(float)1	},
	{ TASK_TURN_LEFT,			(float)179	},
	{ TASK_WAIT,				(float)1	},
};

Schedule_t	slALoaderSweep[] =
{
	{
		tlALoaderSweep,
		ARRAYSIZE ( tlALoaderSweep ),

		bits_COND_NEW_ENEMY		|
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE	|
		bits_COND_CAN_RANGE_ATTACK1	|
		bits_COND_CAN_RANGE_ATTACK2	|
		bits_COND_HEAR_SOUND,

		bits_SOUND_WORLD		|// sound flags
		bits_SOUND_DANGER		|
		bits_SOUND_PLAYER,

		"ALoader Sweep"
	},
};

//=========================================================
// Combat Face
//=========================================================
Task_t	tlALoaderCombatFace1[] =
{
	{ TASK_STOP_MOVING,				0							},
	{ TASK_SET_ACTIVITY,			(float)ACT_IDLE				},
	{ TASK_FACE_ENEMY,				(float)0					},
	{ TASK_WAIT,					(float)1.5					},
	{ TASK_SET_SCHEDULE,			(float)SCHED_ALOADER_SWEEP	},
};

Schedule_t	slALoaderCombatFace[] =
{
	{
		tlALoaderCombatFace1,
		ARRAYSIZE ( tlALoaderCombatFace1 ),
		bits_COND_NEW_ENEMY				|
		bits_COND_ENEMY_DEAD			|
		bits_COND_CAN_RANGE_ATTACK1		|
		bits_COND_CAN_RANGE_ATTACK2,
		0,
		"ALoader Combat Face"
	},
};

DEFINE_CUSTOM_SCHEDULES( CALoader )
{
	slALoaderReload,
	slALoaderFail,
	slALoaderCombatFail,
	slALoaderCombatFace,
	slALoaderSweep,
	slALoaderRangeAttack1,
	slALoaderRangeAttack2,
	slALoaderEstablishLineOfFire,
};

IMPLEMENT_CUSTOM_SCHEDULES( CALoader, CBaseMonster );

//=========================================================
// StartTask
//=========================================================
void CALoader :: StartTask ( Task_t *pTask )
{
	m_iTaskStatus = TASKSTATUS_RUNNING;

	switch ( pTask->iTask )
	{
	case TASK_RELOAD:
		m_IdealActivity = ACT_RELOAD;
		break;

	default:
		CBaseMonster :: StartTask( pTask );
		break;
	}
}

void CALoader :: RunTask( Task_t *pTask )
{
	switch ( pTask->iTask )
	{
	case TASK_RANGE_ATTACK1:
		if (m_hEnemy != NULL && (m_hEnemy->IsPlayer()))
		{
			pev->framerate = 1.5;
		}
		CBaseMonster::RunTask( pTask );
		break;
	default:
		CBaseMonster::RunTask( pTask );
		break;
	}
}

//=========================================================
// ISoundMask - returns a bit mask indicating which types
// of sounds this monster regards.
//=========================================================
int CALoader :: ISoundMask ( void)
{
	return	bits_SOUND_WORLD	|
			bits_SOUND_COMBAT	|
			bits_SOUND_CARCASS	|
			bits_SOUND_MEAT		|
			bits_SOUND_GARBAGE	|
			bits_SOUND_DANGER	|
			bits_SOUND_PLAYER;
}

//=========================================================
// Classify - indicates this monster's place in the
// relationship table.
//=========================================================
int	CALoader :: Classify ( void )
{
	return	CLASS_HUMAN_MILITARY;
}

//=========================================================
// IRelationship - hornets are none of its business
//=========================================================
int CALoader::IRelationship ( CBaseEntity *pTarget )
{
	if ( FClassnameIs( pTarget->pev, "hornet" ) )
	{
		return R_NO;
	}

	return CBaseMonster :: IRelationship( pTarget );
}

//=========================================================
// AlertSound
//=========================================================
void CALoader :: AlertSound( void )
{
	if ( m_hEnemy != NULL )
	{
		EMIT_SOUND_DYN ( ENT(pev), CHAN_VOICE, pAlertSounds[ RANDOM_LONG(0,ARRAYSIZE(pAlertSounds)-1) ], 1.0, ATTN_NORM, 0, m_voicePitch );
	}
}

//=========================================================
// IdleSound
//=========================================================
void CALoader :: IdleSound( void )
{
	if ( pev->spawnflags & SF_MONSTER_GAG )
		return;

	if (RANDOM_LONG( 0, 2 ) == 0)
	{
		EMIT_SOUND_DYN ( ENT(pev), CHAN_VOICE, pIdleSounds[ RANDOM_LONG(0,ARRAYSIZE(pIdleSounds)-1) ], 1.0, ATTN_NORM, 0, m_voicePitch );
	}
}

//=========================================================
// AttackSound - the gun's shot
//=========================================================
void CALoader :: AttackSound( void )
{
	if (RANDOM_LONG( 0, 2 ) == 0)
	{
		EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, pAttackSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackSounds)-1) ], 1.0, ATTN_NORM, 0, RANDOM_LONG( 95, 105 ) );
	}
}

//=========================================================
// PainSound
//=========================================================
void CALoader :: PainSound( void )
{
	if (RANDOM_LONG( 0, 2 ) == 0)
	{
		EMIT_SOUND_DYN ( ENT(pev), CHAN_VOICE, pPainSounds[ RANDOM_LONG(0,ARRAYSIZE(pPainSounds)-1) ], 1.0, ATTN_NORM, 0, m_voicePitch );
	}
}

//=========================================================
// DeathSound
//=========================================================
void CALoader :: DeathSound( void )
{
	EMIT_SOUND_DYN ( ENT(pev), CHAN_VOICE, pDieSounds[ RANDOM_LONG(0,ARRAYSIZE(pDieSounds)-1) ], 1.0, ATTN_NORM, 0, m_voicePitch );
}

//=========================================================
// StepSound
//=========================================================
void CALoader :: StepSound( void )
{
	EMIT_SOUND_DYN ( ENT(pev), CHAN_VOICE, pStepSounds[ RANDOM_LONG(0,ARRAYSIZE(pStepSounds)-1) ], 1.0, ATTN_NORM, 0, m_voicePitch );
}

//=========================================================
// SparkSound - a spark's crackle, played from a temporary
// entity at the spot
//=========================================================
void CALoader :: SparkSound( Vector vecLocation )
{
	edict_t *pent = CREATE_NAMED_ENTITY( MAKE_STRING( "info_target" ) );

	if ( FNullEnt( pent ) )
	{
		ALERT ( at_console, "CALoader - ERROR - couldn't create info_target!\n" );
		return;
	}

	pent->v.origin = vecLocation;
	DispatchSpawn( pent->v.pContainingEntity );

	EMIT_SOUND_DYN( ENT( &pent->v ), CHAN_ITEM, pSparkSounds[ RANDOM_LONG(0,ARRAYSIZE(pSparkSounds)-1) ], RANDOM_LONG( 0, 1 ), ATTN_NORM, 0, m_voicePitch );
	REMOVE_ENTITY( ENT( &pent->v ) );
}

//=========================================================
// StartMotorSound - the motor's loop, once a second and a
// half after spawning
//=========================================================
void CALoader :: StartMotorSound( void )
{
	if ( !m_fMotorSound && m_flMotorTime <= gpGlobals->time )
	{
		EMIT_SOUND_DYN( ENT(pev), CHAN_STATIC, "aloader/al_motor1.wav", 0.6, 1.25, 0, 100 );
		m_fMotorSound = TRUE;
	}
}

//=========================================================
// StopMotorSound
//=========================================================
void CALoader :: StopMotorSound( void )
{
	if ( m_fMotorSound )
		EMIT_SOUND_DYN( ENT(pev), CHAN_STATIC, "aloader/al_motor1.wav", 0.6, 1.25, SND_STOP, 100 );
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CALoader :: SetYawSpeed ( void )
{
	int ys;

	ys = 0;

	switch ( m_Activity )
	{
	case ACT_IDLE:
		ys = 70;
		break;
	case ACT_WALK:
		ys = 70;
		break;
	case ACT_RUN:
		ys = 90;
		break;
	case ACT_RANGE_ATTACK1:
	case ACT_RANGE_ATTACK2:
		ys = 100;
		break;
	default:
		ys = 70;
		break;
	}

	pev->yaw_speed = ys;
}

//=========================================================
// FCanCheckAttacks
//=========================================================
BOOL CALoader :: FCanCheckAttacks ( void )
{
	if ( !HasConditions( bits_COND_ENEMY_TOOFAR ) )
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

//=========================================================
// CheckRangeAttack1
//=========================================================
BOOL CALoader :: CheckRangeAttack1 ( float flDot, float flDist )
{
	if ( flDist >= 256 && flDist <= m_flDistTooFar && flDot >= 0.5 )
	{
		if ( gpGlobals->time > m_checkAttackTime )
		{
			TraceResult tr;

			Vector shootOrigin, shootAngles;
			GetAttachment( 0, shootOrigin, shootAngles );
			CBaseEntity *pEnemy = m_hEnemy;
			Vector shootTarget = ( (pEnemy->BodyTarget( shootOrigin ) - pEnemy->pev->origin) + m_vecEnemyLKP );
			UTIL_TraceLine( shootOrigin, shootTarget, dont_ignore_monsters, dont_ignore_glass, ENT(pev), &tr );
			m_checkAttackTime = gpGlobals->time + 1;
			if ( tr.flFraction == 1.0 || (tr.pHit != NULL && CBaseEntity::Instance(tr.pHit) == pEnemy) )
				m_lastAttackCheck = TRUE;
			else
				m_lastAttackCheck = FALSE;
			m_checkAttackTime = gpGlobals->time + 1.5;
		}
		return m_lastAttackCheck;
	}
	return FALSE;
}

//=========================================================
// CheckRangeAttack2 - the grenade launcher
//=========================================================
BOOL CALoader :: CheckRangeAttack2 ( float flDot, float flDist )
{
	// assume things haven't changed too much since last time
	if (gpGlobals->time < m_flNextGrenadeCheck )
	{
		return m_fThrowGrenade;
	}

	Vector vecTarget;

	// find feet
	if (RANDOM_LONG(0,1))
	{
		// magically know where they are
		vecTarget = Vector( m_hEnemy->pev->origin.x, m_hEnemy->pev->origin.y, m_hEnemy->pev->absmin.z );
	}
	else
	{
		// toss it to where you last saw them
		vecTarget = m_vecEnemyLKP;
	}

	if ( ( vecTarget - pev->origin ).Length2D() <= 256 )
	{
		// crap, I don't want to blow myself up
		m_flNextGrenadeCheck = gpGlobals->time + 1; // one full second.
		m_fThrowGrenade = FALSE;
		return m_fThrowGrenade;
	}

	Vector vecToss = VecCheckToss( pev, GetGunPosition(), vecTarget, 0.5 );

	if ( vecToss != g_vecZero )
	{
		m_vecTossVelocity = vecToss;

		// throw a hand grenade
		m_fThrowGrenade = TRUE;
		// don't check again for a while.
		m_flNextGrenadeCheck = gpGlobals->time + RANDOM_LONG( 1, 2 );
	}
	else
	{
		// don't throw
		m_fThrowGrenade = FALSE;
		// don't check again for a while.
		m_flNextGrenadeCheck = gpGlobals->time + 1; // one full second.
	}

	return m_fThrowGrenade;
}

//=========================================================
// Shoot - one round from the minigun
//=========================================================
void CALoader :: Shoot ( void )
{
	Vector vecShootOrigin;
	Vector vecShootDir;

	GetAttachment( 0, vecShootOrigin, vecShootDir );
	vecShootDir = ShootAtEnemy( vecShootOrigin );

	Vector angDir = UTIL_VecToAngles( vecShootDir );
	SetBlending( 0, angDir.x );
	pev->effects |= EF_MUZZLEFLASH;
	UTIL_MuzzleLight( this, vecShootOrigin );

	Vector	vecShellVelocity = gpGlobals->v_right * RANDOM_FLOAT(40,90) + gpGlobals->v_up * RANDOM_FLOAT(75,200);
	EjectBrass ( vecShootOrigin - vecShootDir * 24, vecShellVelocity, pev->angles.y, m_iShell, TE_BOUNCE_NULL );
	FireBullets( 1, vecShootOrigin, vecShootDir, VECTOR_CONE_10DEGREES, m_flDistTooFar, BULLET_MONSTER_9MM, 3 );

	CSoundEnt::InsertSound ( bits_SOUND_COMBAT, pev->origin, 600, 0.1 );

	m_cAmmoLoaded--;// take away a bullet!
}

//=========================================================
// SpinUp - start the barrels turning
//=========================================================
void CALoader :: SpinUp ( void )
{
	if ( m_iWeaponState != ALOADER_GUN_FIRE && m_iWeaponState != ALOADER_GUN_SPINUP )
	{
		m_iWeaponState = ALOADER_GUN_SPINUP;
		EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, "hassault/hw_spinup2.wav", 1, ATTN_NORM, 0, 100 );
		pev->effects = 0;
		m_flNextFireTime = gpGlobals->time + 0.2;
		m_flCheckTargetTime = gpGlobals->time + 0.1;
	}
}

//=========================================================
// FireGun - the fire anim event: once spun up, a shot
//=========================================================
void CALoader :: FireGun ( void )
{
	if ( m_iWeaponState != ALOADER_GUN_IDLE && m_iWeaponState != ALOADER_GUN_SPINDOWN && m_flNextFireTime <= gpGlobals->time )
	{
		if ( m_iWeaponState == ALOADER_GUN_SPINUP )
		{
			AttackSound();
			m_iWeaponState = ALOADER_GUN_FIRE;
			m_flNextFireTime = gpGlobals->time - 0.1;
			return;
		}

		AttackSound();
		Shoot();
		pev->nextthink = gpGlobals->time + 0.1;
	}
}

//=========================================================
// SpinDown - stop the barrels, throw out a shell
//=========================================================
void CALoader :: SpinDown ( void )
{
	if ( m_iWeaponState != ALOADER_GUN_IDLE && m_iWeaponState != ALOADER_GUN_SPINDOWN )
	{
		Vector vecShootOrigin;
		Vector vecShootDir;

		GetAttachment( 0, vecShootOrigin, vecShootDir );
		vecShootDir = ShootAtEnemy( vecShootOrigin );

		Vector	vecShellVelocity = gpGlobals->v_right * RANDOM_FLOAT(40,90) + gpGlobals->v_up * RANDOM_FLOAT(75,200) + gpGlobals->v_forward * RANDOM_FLOAT(-40, 40);
		EjectBrass ( vecShootOrigin - vecShootDir * 24, vecShellVelocity, pev->angles.y, m_iShell, TE_BOUNCE_SHELL );

		EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, "hassault/hw_spindown.wav", 1, ATTN_NORM, 0, 100 );
		m_iWeaponState = ALOADER_GUN_IDLE;
	}
}

//=========================================================
// LaunchGrenade
//=========================================================
void CALoader :: LaunchGrenade ( void )
{
	Vector vecAngles = pev->angles;	// sic: never used (name ours)
	float flSide = 0;	// the grenade leaves from the middle (name ours)

	UTIL_MakeVectors( pev->angles );
	Vector vecSrc = pev->origin + gpGlobals->v_forward * 64 + gpGlobals->v_right * flSide + gpGlobals->v_up * 150;
	CGrenade *pGrenade = CGrenade::ShootTimed( pev, vecSrc, m_vecTossVelocity, 3.5 );
	SET_MODEL( ENT(pGrenade->pev), "models/algrenade.mdl" );

	EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, "aloader/al_glauncher.wav", 0.8, ATTN_NONE, 0, 100 );

	m_fThrowGrenade = FALSE;
	m_flNextGrenadeCheck = gpGlobals->time + 6;// wait six seconds before even looking again to see if a grenade can be thrown.
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CALoader :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	switch( pEvent->event )
	{
	case ALOADER_AE_SHOOT:
		FireGun();
		break;

	case ALOADER_AE_STEP:
		StepSound();
		break;

	case ALOADER_AE_RELOAD:
		EMIT_SOUND( ENT(pev), CHAN_WEAPON, "weapons/reload2.wav", 1, ATTN_NORM );
		m_cAmmoLoaded = m_cClipSize;
		ClearConditions(bits_COND_NO_AMMO_LOADED);
		break;

	default:
		CBaseMonster::HandleAnimEvent( pEvent );
		break;
	}
}

//=========================================================
// Spawn
//=========================================================
void CALoader :: Spawn()
{
	Precache( );

	SET_MODEL(ENT(pev), "models/povloader.mdl");
	UTIL_SetSize(pev, Vector( -112, -112, 0 ), Vector( 112, 112, 160 ));

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= DONT_BLEED;
	pev->health			= gSkillData.gargantuaHealth * 1.2;
	pev->view_ofs		= Vector ( 0, 0, 120 );// position of the eyes relative to monster's origin.
	m_flFieldOfView		= -0.7;
	m_MonsterState		= MONSTERSTATE_NONE;

	pev->body			= 0;

	m_afCapability		= bits_CAP_HEAR | bits_CAP_RANGE_ATTACK1 | bits_CAP_RANGE_ATTACK2;

	m_cClipSize			= 400;
	m_cAmmoLoaded		= m_cClipSize;
	m_voicePitch		= RANDOM_LONG( 95, 105 );
	m_iWeaponState		= ALOADER_GUN_IDLE;
	m_bLastAttackCheck	= FALSE;
	m_flMotorTime		= gpGlobals->time + 1.5;
	m_fMotorSound		= FALSE;

	MonsterInit();

	SetTouch( LoaderTouch );
	m_flNextHurtTouch	= gpGlobals->time;
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CALoader :: Precache()
{
	int i;

	PRECACHE_MODEL("models/povloader.mdl");
	PRECACHE_MODEL("models/algrenade.mdl");

	for ( i = 0; i < ARRAYSIZE( pIdleSounds ); i++ )
		PRECACHE_SOUND((char *)pIdleSounds[i]);

	for ( i = 0; i < ARRAYSIZE( pAlertSounds ); i++ )
		PRECACHE_SOUND((char *)pAlertSounds[i]);

	for ( i = 0; i < ARRAYSIZE( pAttackSounds ); i++ )
		PRECACHE_SOUND((char *)pAttackSounds[i]);

	for ( i = 0; i < ARRAYSIZE( pPainSounds ); i++ )
		PRECACHE_SOUND((char *)pPainSounds[i]);

	for ( i = 0; i < ARRAYSIZE( pDieSounds ); i++ )
		PRECACHE_SOUND((char *)pDieSounds[i]);

	for ( i = 0; i < ARRAYSIZE( pStepSounds ); i++ )
		PRECACHE_SOUND((char *)pStepSounds[i]);

	for ( i = 0; i < ARRAYSIZE( pSparkSounds ); i++ )
		PRECACHE_SOUND((char *)pSparkSounds[i]);

	PRECACHE_SOUND("weapons/reload2.wav");
	PRECACHE_SOUND("hassault/hw_spinup2.wav");
	PRECACHE_SOUND("hassault/hw_spindown.wav");
	PRECACHE_SOUND("aloader/al_motor1.wav");
	PRECACHE_SOUND("aloader/al_glauncher.wav");
	PRECACHE_SOUND("ambience/loader_hydra1.wav");

	m_iBodyGibs = PRECACHE_MODEL( "models/loadergib.mdl" );
	m_iLegGibs = PRECACHE_MODEL( "models/loadergib_leg.mdl" );
	m_iExplode	= PRECACHE_MODEL( "sprites/fexplo.spr" );
	m_iShell = PRECACHE_MODEL ("models/shell.mdl");// brass shell
}

//=========================================================
// CheckAmmo - overridden for the loader because he actually
// uses ammo! (base class doesn't)
//=========================================================
void CALoader :: CheckAmmo ( void )
{
	if ( m_cAmmoLoaded <= 0 )
	{
		SetConditions(bits_COND_NO_AMMO_LOADED);
	}
}

//=========================================================
// TraceAttack - no hit groups
//=========================================================
void CALoader :: TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType)
{
	ptr->iHitgroup = HITGROUP_GENERIC;

	CBaseMonster::TraceAttack( pevAttacker, flDamage, vecDir, ptr, bitsDamageType );
}

//=========================================================
// TakeDamage - damage conditions from the share of its
// health a hit takes
//=========================================================
int CALoader :: TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	int ret = CBaseMonster::TakeDamage( pevInflictor, pevAttacker, flDamage, bitsDamageType );

	if ( flDamage > 0 )
	{
		ClearConditions( bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE );

		if ( flDamage > pev->health / 6 )
			SetConditions( bits_COND_LIGHT_DAMAGE );

		if ( flDamage > pev->health / 3 )
			SetConditions( bits_COND_HEAVY_DAMAGE );
	}

	return ret;
}

//=========================================================
// Killed - falls over, then explodes (BlastKilled)
//=========================================================
void CALoader :: Killed( entvars_t *pevAttacker, int iGib )
{
	SetTouch( NULL );
	pev->movetype = MOVETYPE_TOSS;
	pev->gravity = 0.3;

	StopMotorSound();

	pev->health = 0;
	pev->takedamage = DAMAGE_NO;

	FCheckAITrigger();
	DeathSound();

	if ( RANDOM_LONG( 0, 2 ) == 0 )
		m_flPlayDeathSequenceOne = FALSE;
	else
		m_flPlayDeathSequenceOne = TRUE;

	pev->sequence = LookupSequence( "smlflinch" );
	ResetSequenceInfo( );

	if ( m_flPlayDeathSequenceOne == FALSE )
		pev->framerate = 1.0;
	else
		pev->framerate = 0.8;

	SetThink( BlastKilled );
	pev->nextthink = gpGlobals->time + 0.1;
}

//=========================================================
// BlastKilled - either blows up after the explosions, or
// sparks and falls into a smoking wreck
//=========================================================
void CALoader :: BlastKilled( void )
{
	StudioFrameAdvance( );

	if ( m_flPlayDeathSequenceOne )
	{
		if ( m_fSequenceFinished )
		{
			ExplosionCreate( pev->origin, pev->angles, ENT(pev), 128, TRUE );

			// gibs
			MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, pev->origin );
				WRITE_BYTE( TE_BREAKMODEL);

				// position
				WRITE_COORD( pev->origin.x );
				WRITE_COORD( pev->origin.y );
				WRITE_COORD( pev->origin.z + 64);

				// size
				WRITE_COORD( 224 );
				WRITE_COORD( 224 );
				WRITE_COORD( 128 );

				// velocity
				WRITE_COORD( 0 );
				WRITE_COORD( 0 );
				WRITE_COORD( RANDOM_LONG( 100, 150 ) );

				// randomization
				WRITE_BYTE( 30 );

				// Model
				WRITE_SHORT( m_iBodyGibs );	//model id#

				// # of shards
				WRITE_BYTE( RANDOM_LONG( 50, 60 ) );

				// duration
				WRITE_BYTE( 200 );// 10.0 seconds

				// flags

				WRITE_BYTE( BREAK_METAL );
			MESSAGE_END();

			// legs
			MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, pev->origin );
				WRITE_BYTE( TE_BREAKMODEL);

				// position
				WRITE_COORD( pev->origin.x );
				WRITE_COORD( pev->origin.y );
				WRITE_COORD( pev->origin.z + 64);

				// size
				WRITE_COORD( 224 );
				WRITE_COORD( 224 );
				WRITE_COORD( 128 );

				// velocity
				WRITE_COORD( 0 );
				WRITE_COORD( 0 );
				WRITE_COORD( RANDOM_LONG( 100, 150 ) );

				// randomization
				WRITE_BYTE( 10 );

				// Model
				WRITE_SHORT( m_iLegGibs );	//model id#

				// # of shards
				WRITE_BYTE( RANDOM_LONG( 1, 4 ) );

				// duration
				WRITE_BYTE( 200 );// 10.0 seconds

				// flags

				WRITE_BYTE( BREAK_METAL );
			MESSAGE_END();

			UTIL_DropGun( this, "weapon_loader", TRUE );

			RadiusDamage( pev->origin, pev, pev, 128, CLASS_NONE, DMG_BLAST );

			SetThink( SUB_Remove );
		}
		else
		{
			ExplosionCreate( Vector( pev->origin.x + RANDOM_LONG( -108, 108 ), pev->origin.y + RANDOM_LONG( -108, 108 ), pev->origin.z + RANDOM_LONG( 8, 128 ) ), pev->angles, ENT(pev), RANDOM_LONG( 48, 72 ), TRUE );
		}
	}
	else
	{
		if ( m_fSequenceFinished )
		{
			if ( pev->sequence == LookupSequence( "diesimple" ) )
			{
				SetThink( DeadSmoke );
				UTIL_SetSize( pev, Vector( -80, -80, 0 ), Vector( 80, 80, 72 ) );
			}
			else
			{
				pev->sequence = LookupSequence( "diesimple" );
				pev->framerate = 0.8;
				ResetSequenceInfo( );
			}
		}
		else
		{
			CBaseEntity::Create( "spark_shower", Vector( pev->origin.x + RANDOM_LONG( -108, 108 ), pev->origin.y + RANDOM_LONG( -108, 108 ), pev->origin.z + RANDOM_LONG( 8, 128 ) ), Vector( 0, 0, 0 ), NULL );
		}
	}

	pev->nextthink = gpGlobals->time + 0.1;
}

//=========================================================
// DeadSmoke - the wreck smokes and sparks
//=========================================================
void CALoader :: DeadSmoke( void )
{
	if ( RANDOM_LONG( 0, 1 ) == 0 )
	{
		MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, pev->origin );
			WRITE_BYTE( TE_SMOKE );
			WRITE_COORD( pev->origin.x + RANDOM_LONG( -72, 72 ) );
			WRITE_COORD( pev->origin.y + RANDOM_LONG( -72, 72 ) );
			WRITE_COORD( pev->origin.z + RANDOM_LONG( 32, 80 ) );
			WRITE_SHORT( g_sModelIndexSmoke );
			WRITE_BYTE( RANDOM_LONG( 16, 24 ) ); // scale * 10
			WRITE_BYTE( RANDOM_LONG( 12, 14 ) ); // framerate
		MESSAGE_END();
	}

	if ( RANDOM_LONG( 0, 1 ) == 0 )
	{
		Vector vecSpot;
		vecSpot = Vector( pev->origin.x + RANDOM_LONG( -108, 108 ), pev->origin.y + RANDOM_LONG( -108, 108 ), pev->origin.z + RANDOM_LONG( 8, 128 ) );
		SparkSound( vecSpot );
		UTIL_Sparks( vecSpot );
	}

	pev->nextthink = gpGlobals->time + RANDOM_LONG( 1, 2 );
}

//=========================================================
// MonsterThink - starts the motor's sound
//=========================================================
void CALoader :: MonsterThink ( void )
{
	StartMotorSound();
	CBaseMonster::MonsterThink();
}

//=========================================================
// SetActivity
//=========================================================
void CALoader :: SetActivity ( Activity NewActivity )
{
	int	iSequence = ACTIVITY_NOT_AVAILABLE;
	void *pmodel = GET_MODEL_PTR( ENT(pev) );

	switch ( NewActivity)
	{
	case ACT_RANGE_ATTACK1:
		iSequence = LookupSequence( "shootgun" );
		break;
	case ACT_RELOAD:
		iSequence = LookupSequence( "reload" );
		break;
	default:
		iSequence = LookupActivity ( NewActivity );
		break;
	}

	m_Activity = NewActivity; // Go ahead and set this so it doesn't keep trying when the anim is not present

	// Set to the desired anim, or default anim if the desired is not present
	if ( iSequence > ACTIVITY_NOT_AVAILABLE )
	{
		if ( pev->sequence != iSequence || !m_fSequenceLoops )
		{
			pev->frame = 0;
		}

		pev->sequence		= iSequence;	// Set to the reset anim (if it's there)
		ResetSequenceInfo( );
		SetYawSpeed();
	}
	else
	{
		// Not available try to get default anim
		pev->sequence		= 0;	// Set to the reset anim (if it's there)
	}
}

//=========================================================
// GetScheduleOfType
//=========================================================
Schedule_t* CALoader :: GetScheduleOfType ( int Type )
{
	switch( Type )
	{
	case SCHED_FAIL:
		if ( m_hEnemy != NULL )
			return slALoaderCombatFail;
		return slALoaderFail;

	case SCHED_RANGE_ATTACK1:
		return slALoaderRangeAttack1;

	case SCHED_RANGE_ATTACK2:
		return slALoaderRangeAttack2;

	case SCHED_ALOADER_ESTABLISH_LINE_OF_FIRE:
		return slALoaderEstablishLineOfFire;

	case SCHED_ALOADER_ELOF_FAIL:
		return GetScheduleOfType ( SCHED_FAIL );

	case SCHED_ALOADER_COVER_AND_RELOAD:
		return slALoaderReload;

	default:
		return CBaseMonster::GetScheduleOfType( Type );
	}
}

//=========================================================
// GetSchedule
//=========================================================
Schedule_t *CALoader :: GetSchedule ( void )
{
	switch( m_MonsterState )
	{
	case MONSTERSTATE_COMBAT:
		{
// dead enemy
			if ( HasConditions( bits_COND_ENEMY_DEAD ) )
			{
				SpinDown();

				if ( pev->health < (gSkillData.apacheHealth / 3) )
				{
					SetConditions( bits_COND_HEAVY_DAMAGE );
					return CBaseMonster :: GetSchedule();
				}

				// call base class, all code to handle dead enemies is centralized there.
				return CBaseMonster :: GetSchedule();
			}

			// new enemy
			if ( HasConditions( bits_COND_NEW_ENEMY ) )
			{
				AlertSound();
				SpinUp();
			}

			if ( HasConditions( bits_COND_LIGHT_DAMAGE ) && m_hEnemy != NULL )
				ClearConditions( bits_COND_LIGHT_DAMAGE );

			if ( HasConditions( bits_COND_HEAVY_DAMAGE ) && m_hEnemy != NULL )
			{
				SpinDown();
				return GetScheduleOfType( SCHED_SMALL_FLINCH );
			}
			// no ammo
			else if ( HasConditions ( bits_COND_NO_AMMO_LOADED ) )
			{
				SpinDown();
				return GetScheduleOfType ( SCHED_ALOADER_COVER_AND_RELOAD );
			}
			else if ( HasConditions( bits_COND_CAN_RANGE_ATTACK2 ) )
			{
				SpinDown();
				LaunchGrenade();
				return GetScheduleOfType( SCHED_RANGE_ATTACK2 );
			}
			else if ( HasConditions( bits_COND_ENEMY_OCCLUDED ) )
			{
				SpinDown();
				return GetScheduleOfType( SCHED_ALOADER_ESTABLISH_LINE_OF_FIRE );
			}
			else if ( HasConditions( bits_COND_SEE_ENEMY ) && !HasConditions( bits_COND_CAN_RANGE_ATTACK1 ) )
			{
				SpinDown();
				return GetScheduleOfType( SCHED_ALOADER_ESTABLISH_LINE_OF_FIRE );
			}
			else if ( HasConditions( bits_COND_CAN_RANGE_ATTACK1 ) )
			{
				SpinUp();
				return GetScheduleOfType( SCHED_RANGE_ATTACK1 );
			}
		}
		// sic: no break, combat falls into the idle code

	case MONSTERSTATE_IDLE:
	case MONSTERSTATE_ALERT:
		if ( HasConditions( bits_COND_HEAVY_DAMAGE ) && RANDOM_LONG( 0, 2 ) == 0 )
			return GetScheduleOfType( SCHED_SMALL_FLINCH );

		IdleSound();
		break;
	}

	return CBaseMonster :: GetSchedule();
}

//=========================================================
// LoaderTouch - crushes the player, toads and snarks that
// get in its way
//=========================================================
void CALoader :: LoaderTouch( CBaseEntity *pOther )
{
	if ( !pOther->IsPlayer() && !FClassnameIs( pOther->pev, "monster_toad" ) && !FClassnameIs( pOther->pev, "monster_snark" ) )
		return;

	if ( m_flNextHurtTouch < gpGlobals->time )
	{
		pOther->TakeDamage( pev, pev, gSkillData.monDmg9MM, DMG_BURN );
		m_flNextHurtTouch = gpGlobals->time + 1.0;
	}
}

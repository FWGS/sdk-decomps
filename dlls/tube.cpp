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
// monster_tube: the rust tube, a squad monster that fires
// tube rockets at its enemy and bursts into a spray of them
// when it dies. Damage wears down its mental health, and a
// tube that loses its mind blows itself up. Its schedules,
// range attack test and speech timing are the alien grunt's.
//=========================================================

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "schedule.h"
#include "squadmonster.h"
#include "soundent.h"
#include "tube.h"

//=========================================================
// monster-specific schedule types
//=========================================================
enum
{
	SCHED_TUBE_SUPPRESS = LAST_COMMON_SCHEDULE + 1,
};

//=========================================================
// monster-specific tasks
//=========================================================
enum
{
	TASK_TUBE_GET_PATH_TO_ENEMY_CORPSE = LAST_COMMON_TASK + 1,	// no schedule uses it
};

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define TUBE_AE_ROCKET1		( 1 )
#define TUBE_AE_ROCKET2		( 2 )
#define TUBE_AE_ROCKET3		( 3 )
#define TUBE_AE_MOURN		( 4 )

// gib and gore spray sprites of the tube species
int g_iGoreTube;
int g_iGibTube;

//=========================================================
// AI Schedules Specific to this monster
//=========================================================
Task_t	tlTubeFail[] =
{
	{ TASK_STOP_MOVING,			(float)0		},
	{ TASK_SET_ACTIVITY_IDLE,	(float)0		},
	{ TASK_WAIT,				(float)2		},
	{ TASK_WAIT_PVS,			(float)0		},
};

Schedule_t	slTubeFail[] =
{
	{
		tlTubeFail,
		ARRAYSIZE ( tlTubeFail ),
		bits_COND_CAN_RANGE_ATTACK1 |
		bits_COND_CAN_MELEE_ATTACK1,
		0,
		"Tube Fail"
	},
};

Task_t	tlTubeCombatFail[] =
{
	{ TASK_STOP_MOVING,			(float)0		},
	{ TASK_SET_ACTIVITY_IDLE,	(float)0		},
	{ TASK_WAIT_FACE_ENEMY,		(float)2		},
	{ TASK_WAIT_PVS,			(float)0		},
};

Schedule_t	slTubeCombatFail[] =
{
	{
		tlTubeCombatFail,
		ARRAYSIZE ( tlTubeCombatFail ),
		bits_COND_CAN_RANGE_ATTACK1 |
		bits_COND_CAN_MELEE_ATTACK1,
		0,
		"Tube Combat Fail"
	},
};

Task_t	tlTubeStandoff[] =
{
	{ TASK_STOP_MOVING,			(float)0		},
	{ TASK_SET_ACTIVITY_IDLE,	(float)0		},
	{ TASK_WAIT_FACE_ENEMY,		(float)2		},
};

Schedule_t	slTubeStandoff[] =
{
	{
		tlTubeStandoff,
		ARRAYSIZE ( tlTubeStandoff ),
		bits_COND_SEE_ENEMY			|
		bits_COND_CAN_RANGE_ATTACK1	|
		bits_COND_CAN_MELEE_ATTACK1	|
		bits_COND_NEW_ENEMY			|
		bits_COND_HEAR_SOUND,
		bits_SOUND_DANGER,
		"Tube Standoff"
	},
};

// four rockets in a row at a new enemy
Task_t	tlTubeSuppressRocket[] =
{
	{ TASK_STOP_MOVING,				(float)0		},
	{ TASK_RANGE_ATTACK1_NOTURN,	(float)0		},
	{ TASK_RANGE_ATTACK1_NOTURN,	(float)0		},
	{ TASK_RANGE_ATTACK1_NOTURN,	(float)0		},
	{ TASK_RANGE_ATTACK1_NOTURN,	(float)0		},
};

Schedule_t	slTubeSuppressRocket[] =
{
	{
		tlTubeSuppressRocket,
		ARRAYSIZE ( tlTubeSuppressRocket ),
		0,
		0,
		"Tube Suppress Rocket"
	},
};

Task_t	tlTubeRangeAttack1[] =
{
	{ TASK_STOP_MOVING,				(float)0		},
	{ TASK_FACE_ENEMY,				(float)0		},
	{ TASK_PLAY_SEQUENCE,			(float)ACT_ARM	},
	{ TASK_RANGE_ATTACK1_NOTURN,	(float)0		},
};

Schedule_t	slTubeRangeAttack1[] =
{
	{
		tlTubeRangeAttack1,
		ARRAYSIZE ( tlTubeRangeAttack1 ),
		0,
		0,
		"Tube Range Attack1"
	},
};

Task_t	tlTubeRangeAttack2[] =
{
	{ TASK_STOP_MOVING,			(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_RANGE_ATTACK2,		(float)0		},
};

Schedule_t	slTubeRangeAttack2[] =
{
	{
		tlTubeRangeAttack2,
		ARRAYSIZE ( tlTubeRangeAttack2 ),
		bits_COND_HEAVY_DAMAGE		|
		bits_COND_NEW_ENEMY			|
		bits_COND_ENEMY_DEAD,
		0,
		"Tube Range Attack2"
	},
};

Task_t	tlTubeTakeCoverFromEnemy[] =
{
	{ TASK_STOP_MOVING,				(float)0					},
	{ TASK_WAIT,					(float)0.2					},
	{ TASK_FIND_COVER_FROM_ENEMY,	(float)0					},
	{ TASK_RUN_PATH,				(float)0					},
	{ TASK_WAIT_FOR_MOVEMENT,		(float)0					},
	{ TASK_REMEMBER,				(float)bits_MEMORY_INCOVER	},
	{ TASK_FACE_ENEMY,				(float)0					},
};

Schedule_t	slTubeTakeCoverFromEnemy[] =
{
	{
		tlTubeTakeCoverFromEnemy,
		ARRAYSIZE ( tlTubeTakeCoverFromEnemy ),
		bits_COND_NEW_ENEMY,
		0,
		"Tube TakeCoverFromEnemy"
	},
};

LINK_ENTITY_TO_CLASS( monster_tube, CTube );

TYPEDESCRIPTION	CTube::m_SaveData[] =
{
	DEFINE_FIELD( CTube, m_iResidual, FIELD_INTEGER ),
	DEFINE_FIELD( CTube, m_fInstability, FIELD_FLOAT ),
	DEFINE_FIELD( CTube, m_iMentalHealth, FIELD_INTEGER ),
	DEFINE_FIELD( CTube, m_iModelIndexTubeSuicide, FIELD_INTEGER ),
	DEFINE_FIELD( CTube, m_fCanRocketAttack, FIELD_BOOLEAN ),
	DEFINE_FIELD( CTube, m_tNextRocketAttackCheck, FIELD_TIME ),
	DEFINE_FIELD( CTube, m_tNextAttack2Time, FIELD_TIME ),
	DEFINE_FIELD( CTube, m_tNextSpeakTime, FIELD_TIME ),
	DEFINE_FIELD( CTube, m_tNextWordTime, FIELD_TIME ),
	DEFINE_FIELD( CTube, m_iVoicePitch, FIELD_INTEGER ),
};

IMPLEMENT_SAVERESTORE( CTube, CSquadMonster );

DEFINE_CUSTOM_SCHEDULES( CTube )
{
	slTubeFail,
	slTubeCombatFail,
	slTubeStandoff,
	slTubeSuppressRocket,
	slTubeRangeAttack1,
	slTubeRangeAttack2,
	slTubeTakeCoverFromEnemy,
};

IMPLEMENT_CUSTOM_SCHEDULES( CTube, CSquadMonster );

const char *CTube::pTubeSounds[] =
{
	"tube/Tube_Footstep_Left1.wav",
	"tube/Tube_Footstep_Right1.wav",
	"tube/Tube_Flinch.wav",
	"tube/Tube_Arming.wav",
	"tube/Tube_Shoot.wav",
	"tube/Tube_Qkfire.wav",
	"tube/Tube_DieSimple.wav",
	"tube/Tube_DieBack.wav",
	"tube/tube_explodingdeath.wav",
	"tube/tube_idle.wav",
	"tube/tube_idle2.wav",
	"tube/tube_idle3.wav",
	"tube/Tube_SeePlayer.wav",
	"tube/tube_mourn1.wav",
	"tube/tube_mourn2.wav",
	"tube/tube_mourn3.wav",
};

void CTube::PrecacheSounds( void )
{
	for ( int i = 0; i < ARRAYSIZE( pTubeSounds ); i++ )
		PRECACHE_SOUND( (char *)pTubeSounds[i] );
}

void CTube::IdleSound( void )
{
	switch ( RANDOM_LONG( 0, 2 ) )
	{
	case 0:
		EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "tube/tube_idle.wav", 1, 0.8, 0, m_iVoicePitch );
		break;
	case 1:
		EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "tube/tube_idle2.wav", 1, 0.8, 0, m_iVoicePitch );
		break;
	case 2:
		EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "tube/tube_idle3.wav", 1, 0.8, 0, m_iVoicePitch );
		break;
	}
}

void CTube::AlertSound( void )
{
	EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "tube/tube_seeplayer.wav", 1, 0.8, 0, m_iVoicePitch );
}

//=========================================================
// StopTalking - won't speak again for 10-20 seconds.
//=========================================================
void CTube::StopTalking( void )
{
	m_tNextWordTime = m_tNextSpeakTime = gpGlobals->time + 10 + RANDOM_LONG( 0, 10 );
}

//=========================================================
// ShouldSpeak - Should this tube be talking?
//=========================================================
BOOL CTube::ShouldSpeak( void )
{
	if ( m_tNextSpeakTime > gpGlobals->time )
	{
		// my time to talk is still in the future.
		return FALSE;
	}

	if ( pev->spawnflags & SF_MONSTER_GAG )
	{
		if ( m_MonsterState != MONSTERSTATE_COMBAT )
		{
			// if gagged, don't talk outside of combat.
			// if not going to talk because of this, put the talk time
			// into the future a bit, so we don't talk immediately after
			// going into combat
			m_tNextSpeakTime = gpGlobals->time + 3;
			return FALSE;
		}
	}

	return TRUE;
}

//=========================================================
// ISoundMask - returns a bit mask indicating which types
// of sounds this monster regards.
//=========================================================
int CTube::ISoundMask( void )
{
	return	bits_SOUND_WORLD	|
			bits_SOUND_COMBAT	|
			bits_SOUND_PLAYER	|
			bits_SOUND_DANGER;
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CTube::SetYawSpeed( void )
{
	int ys;

	ys = 75;

	if ( m_Activity >= ACT_TURN_LEFT && m_Activity <= ACT_TURN_RIGHT )
		ys = 110;

	pev->yaw_speed = ys;
}

void CTube::SetObjectCollisionBox( void )
{
	pev->absmin = pev->origin - Vector( 32, 32, 0 );
	pev->absmax = pev->origin + Vector( 32, 32, 64 );
}

//=========================================================
// GibMonster - the tube bursts into a spray of tube
// rockets
//=========================================================
void CTube::GibMonster( void )
{
	Vector vecDir;

	EMIT_SOUND_DYN( ENT(pev), CHAN_BODY, "tube/tube_explodingdeath.wav", 1, 0.8, 0, 100 );

	int iRockets = RANDOM_LONG( 6, 12 );
	for ( int i = 0; i < iRockets; i++ )
	{
		CBaseMonster *pRocket = ShootRocket( );

		// sic: ShootRocket can return NULL
		vecDir.x = RANDOM_FLOAT( -0.5, 0.5 );
		vecDir.y = RANDOM_FLOAT( -0.5, 0.5 );
		vecDir.z = RANDOM_FLOAT( 0.1, 0.5 );
		vecDir = vecDir.Normalize( );
		pRocket->pev->angles = UTIL_VecToAngles( vecDir );
		UTIL_MakeAimVectors( pRocket->pev->angles );
		pRocket->pev->velocity = gpGlobals->v_forward * 300;
		pRocket->pev->origin = pRocket->pev->origin + Vector( RANDOM_FLOAT( -20, 20 ), RANDOM_FLOAT( -20, 20 ), RANDOM_FLOAT( 32, 64 ) );

		if ( g_iSkillLevel != SKILL_EASY )
		{
			pRocket->pev->movetype = MOVETYPE_TOSS;
			pRocket->pev->nextthink = gpGlobals->time + 0.3;
		}

		if ( m_MonsterState == MONSTERSTATE_COMBAT )
		{
			pRocket->m_vecEnemyLKP = m_vecEnemyLKP;
			pRocket->m_hEnemy = m_hEnemy;
		}
	}

	MESSAGE_BEGIN( MSG_PAS, SVC_TEMPENTITY, pev->origin );
		WRITE_BYTE( TE_EXPLOSION );
		WRITE_COORD( pev->origin.x );
		WRITE_COORD( pev->origin.y );
		WRITE_COORD( pev->origin.z );
		WRITE_SHORT( m_iModelIndexTubeSuicide );
		WRITE_BYTE( 10 );	// scale * 10
		WRITE_BYTE( 15 );	// framerate
		WRITE_BYTE( TE_EXPLFLAG_NODLIGHTS | TE_EXPLFLAG_NOSOUND );
	MESSAGE_END( );

	CBaseMonster::GibMonster( );
}

//=========================================================
// ShootRocket - a tube rocket 48 units up, toward the
// enemy's last known position
//=========================================================
CBaseMonster *CTube::ShootRocket( void )
{
	Vector vecDir = ( m_vecEnemyLKP - pev->origin ).Normalize( );
	Vector vecAngles;

	vecAngles = UTIL_VecToAngles( vecDir );

	CBaseEntity *pRocket = CBaseEntity::Create( "tuberocket", pev->origin + Vector( 0, 0, 48 ), vecAngles, edict( ) );

	if ( m_hEnemy )
		pRocket->pev->velocity = vecDir * 500;
	else
	{
		UTIL_MakeVectors( vecAngles );
		pRocket->pev->velocity = gpGlobals->v_forward * 500;
		Vector vecForward = gpGlobals->v_forward;	// sic: never used
	}

	CBaseMonster *pMonster = pRocket->MyMonsterPointer( );
	if ( pMonster )
		pMonster->m_hEnemy = m_hEnemy;

	return pMonster;
}

//=========================================================
// PrescheduleThink
//=========================================================
void CTube::PrescheduleThink( void )
{
	if ( ShouldSpeak( ) )
	{
		if ( m_tNextWordTime < gpGlobals->time )
		{
			IdleSound( );

			// is this word our last?
			if ( UTIL_PercentChance( 10 ) )
			{
				// stop talking.
				StopTalking( );
			}
			else
			{
				m_tNextWordTime = gpGlobals->time + RANDOM_FLOAT( 0.5, 1 );
			}
		}
	}
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CTube::HandleAnimEvent( MonsterEvent_t *pEvent )
{
	switch ( pEvent->event )
	{
	case TUBE_AE_ROCKET1:
	case TUBE_AE_ROCKET2:
		{
			Vector vecGunPos, vecGunAngles;

			GetAttachment( 0, vecGunPos, vecGunAngles );
			CBaseMonster *pRocket = ShootRocket( );
			UTIL_SetOrigin( pRocket->pev, vecGunPos );
		}
		break;

	case TUBE_AE_ROCKET3:
		{
			Vector vecGunPos, vecGunAngles;

			GetAttachment( 0, vecGunPos, vecGunAngles );
			CBaseMonster *pRocket = ShootRocket( );
			UTIL_SetOrigin( pRocket->pev, vecGunPos );
		}
		break;

	case TUBE_AE_MOURN:
		switch ( RANDOM_LONG( 0, 2 ) )
		{
		case 0:
			EMIT_SOUND_DYN( ENT(pev), CHAN_BODY, "tube/tube_mourn1.wav", 1, 0.8, 0, m_iVoicePitch );
			break;
		case 1:
			EMIT_SOUND_DYN( ENT(pev), CHAN_BODY, "tube/tube_mourn2.wav", 1, 0.8, 0, m_iVoicePitch );
			break;
		case 2:
			EMIT_SOUND_DYN( ENT(pev), CHAN_BODY, "tube/tube_mourn3.wav", 1, 0.8, 0, m_iVoicePitch );
			break;
		}
		break;

	default:
		CBaseMonster::HandleAnimEvent( pEvent );
		break;
	}
}

//=========================================================
// RunAI - a tube that has lost its mind blows itself up
//=========================================================
void CTube::RunAI( void )
{
	if ( IsAlive( ) )
	{
		if ( CheckMentalHealth( ) == 1 )
			TakeDamage( NULL, NULL, 2000, DMG_ALWAYSGIB );
	}

	CBaseMonster::RunAI( );
}

//=========================================================
// CheckMentalHealth - works off a point of the residual
// change; returns 1 when the mental health is gone
//=========================================================
int CTube::CheckMentalHealth( void )
{
	if ( m_iMentalHealth < 0 )
		return 1;

	if ( m_iResidual )
	{
		if ( m_iResidual > 0 )
		{
			ChangeMentalHealth( -1 );
			m_iResidual--;
		}

		if ( m_iResidual < 0 )
		{
			ChangeMentalHealth( 1 );
			m_iResidual++;
			return 0;
		}

		ChangeMentalHealth( m_iResidual );
		m_iResidual = 0;
	}

	return 0;
}

//=========================================================
// ChangeMentalHealth - a loss is scaled by the instability,
// which grows with every loss, and swings back in the
// residual
//=========================================================
void CTube::ChangeMentalHealth( int iChange )
{
	if ( iChange > 0 )
	{
		m_iMentalHealth += iChange;
		return;
	}

	m_iMentalHealth += iChange * m_fInstability;

	int iLoss = -iChange;
	if ( iLoss > m_iMentalHealth / 2 )
	{
		m_fInstability += 0.3;

		if ( m_iResidual > 0 )
			m_iResidual = 0;
	}

	m_fInstability += 0.2;
	m_iResidual = iLoss / 2;

	if ( UTIL_PercentChance( 75 ) )
		m_iResidual = -m_iResidual;
}

int CTube::TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	Forget( bits_MEMORY_INCOVER );
	ChangeMentalHealth( flDamage * -2 );

	return CBaseMonster::TakeDamage( pevInflictor, pevAttacker, flDamage, bitsDamageType );
}

//=========================================================
// Spawn
//=========================================================
void CTube::Spawn( void )
{
	Precache( );

	SET_MODEL( ENT(pev), "models/tube.mdl" );
	UTIL_SetSize( pev, Vector( -32, -32, 0 ), Vector( 32, 32, 64 ) );

	pev->solid			= SOLID_SLIDEBOX;
	pev->health			= gSkillData.tubeHealth;
	m_afCapability		|= bits_CAP_MELEE_ATTACK1;	// sic: not bits_CAP_SQUAD
	m_tNextAttack2Time	= gpGlobals->time;

	m_HackedGunPos		= Vector( 24, 64, 48 );

	m_tNextSpeakTime	= m_tNextWordTime = gpGlobals->time + 10 + RANDOM_LONG( 0, 10 );

	m_iMentalHealth		= RANDOM_LONG( 0, 100 ) + 100;
	m_iResidual			= 11;
	m_iVoicePitch		= RANDOM_LONG( 95, 105 );

	// a tube the queen pulled out stays where she holds it
	if ( pev->owner && FClassnameIs( pev->owner, "monster_tubequeen" ) )
		pev->spawnflags |= SF_MONSTER_FALL_TO_GROUND;

	MonsterInit( );

	pev->view_ofs = Vector( 0, 0, 40 );
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CTube::Precache( void )
{
	UTIL_PrecacheOther( "tuberocket" );

	g_iGibTube = PRECACHE_MODEL( "sprites/gibtube.spr" );
	g_iGoreTube = PRECACHE_MODEL( "sprites/goretube.spr" );
	m_iModelIndexTubeSuicide = PRECACHE_MODEL( "sprites/tubeguts.spr" );
	PRECACHE_MODEL( "models/tube.mdl" );

	CBaseMonster::Precache( );
}

//=========================================================
// FCanCheckAttacks - only when the enemy is too far away
// to see
//=========================================================
BOOL CTube::FCanCheckAttacks( void )
{
	if ( HasConditions( bits_COND_ENEMY_TOOFAR ) )
		return FALSE;

	return TRUE;
}

//=========================================================
// CheckRangeAttack1 - a rocket from 25 units out toward the
// enemy must reach it before the world
//=========================================================
BOOL CTube::CheckRangeAttack1( float flDot, float flDist )
{
	if ( gpGlobals->time < m_tNextRocketAttackCheck )
	{
		return m_fCanRocketAttack;
	}

	if ( HasConditions( bits_COND_SEE_ENEMY ) && flDist <= 1024 && flDot >= 0.5 && NoFriendlyFire( ) )
	{
		TraceResult	tr;
		Vector	vecDir, vecSrc;

		UTIL_MakeVectors( pev->angles );
		vecDir = ( m_vecEnemyLKP - pev->origin ).Normalize( );
		vecSrc = pev->origin + vecDir * 25;
		vecSrc.z += 16;

		UTIL_TraceLine( vecSrc, m_hEnemy->BodyTarget( vecSrc ), dont_ignore_monsters, ENT(pev), &tr );

		if ( tr.flFraction == 1.0 || tr.pHit == m_hEnemy->edict( ) )
		{
			m_tNextRocketAttackCheck = gpGlobals->time + RANDOM_FLOAT( 2, 5 );
			m_fCanRocketAttack = TRUE;
			return m_fCanRocketAttack;
		}
	}

	m_tNextRocketAttackCheck = gpGlobals->time + 0.2;
	m_fCanRocketAttack = FALSE;
	return m_fCanRocketAttack;
}

//=========================================================
// CheckRangeAttack2 - the same shot, standing or walking
//=========================================================
BOOL CTube::CheckRangeAttack2( float flDot, float flDist )
{
	// sic: m_tNextAttack2Time starts at the spawn time and only
	// a successful check moves it, so the time test fails for good
	// once the spawn time has passed
	if ( ( m_Activity != ACT_IDLE && m_Activity != ACT_WALK ) || gpGlobals->time > m_tNextAttack2Time || flDist > 1024 || !NoFriendlyFire( ) )
		return FALSE;

	TraceResult	tr;
	Vector	vecDir, vecSrc;

	UTIL_MakeVectors( pev->angles );
	vecDir = ( m_vecEnemyLKP - pev->origin ).Normalize( );
	vecSrc = pev->origin + vecDir * 25;
	vecSrc.z += 16;

	UTIL_TraceLine( vecSrc, m_hEnemy->BodyTarget( vecSrc ), dont_ignore_monsters, ENT(pev), &tr );

	if ( tr.flFraction != 1.0 && tr.pHit != m_hEnemy->edict( ) )
		return FALSE;

	m_tNextAttack2Time = gpGlobals->time + RANDOM_FLOAT( 2, 5 );
	return TRUE;
}

//=========================================================
// StartTask
//=========================================================
void CTube::StartTask( Task_t *pTask )
{
	switch ( pTask->iTask )
	{
	case TASK_TUBE_GET_PATH_TO_ENEMY_CORPSE:
		{
			UTIL_MakeVectors( pev->angles );
			if ( BuildRoute( m_vecEnemyLKP - gpGlobals->v_forward * 50, bits_MF_TO_LOCATION, NULL ) )
			{
				TaskComplete( );
			}
			else
			{
				TaskFail( );
			}
		}
		break;

	default:
		CBaseMonster::StartTask( pTask );
		break;
	}
}

//=========================================================
// GetSchedule - Decides which type of schedule best suits
// the monster's current state and conditions. Then calls
// monster's member function to get a pointer to a schedule
// of the proper type.
//=========================================================
Schedule_t *CTube::GetSchedule( void )
{
	if ( HasConditions( bits_COND_HEAR_SOUND ) )
	{
		CSound *pSound;
		pSound = PBestSound( );

		if ( pSound && ( pSound->m_iType & bits_SOUND_DANGER ) )
		{
			// dangerous sound nearby!
			return GetScheduleOfType( SCHED_TAKE_COVER_FROM_BEST_SOUND );
		}
	}

	switch ( m_MonsterState )
	{
	case MONSTERSTATE_COMBAT:
		{
			// dead enemy
			if ( HasConditions( bits_COND_ENEMY_DEAD ) )
			{
				// call base class, all code to handle dead enemies is centralized there.
				return CBaseMonster::GetSchedule( );
			}

			if ( HasConditions( bits_COND_NEW_ENEMY ) )
			{
				if ( UTIL_PercentChance( 15 ) )
					return GetScheduleOfType( SCHED_TUBE_SUPPRESS );

				return GetScheduleOfType( SCHED_WAKE_ANGRY );
			}

			if ( HasConditions( bits_COND_HEAVY_DAMAGE ) )
			{
				return GetScheduleOfType( SCHED_SMALL_FLINCH );
			}

			// can attack
			if ( HasConditions( bits_COND_CAN_RANGE_ATTACK1 ) && OccupySlot( bits_SLOTS_AGRUNT_HORNET ) )
			{
				return GetScheduleOfType( SCHED_RANGE_ATTACK1 );
			}

			if ( HasConditions( bits_COND_CAN_RANGE_ATTACK2 ) )
			{
				return GetScheduleOfType( SCHED_RANGE_ATTACK2 );
			}

			if ( OccupySlot( bits_SLOT_AGRUNT_CHASE ) )
			{
				return GetScheduleOfType( SCHED_CHASE_ENEMY );
			}

			if ( OccupySlot( bits_SLOTS_AGRUNT_HORNET ) )
			{
				return GetScheduleOfType( SCHED_RANGE_ATTACK1 );
			}
		}
	}

	return CBaseMonster::GetSchedule( );
}

//=========================================================
// GetScheduleOfType
//=========================================================
Schedule_t *CTube::GetScheduleOfType( int Type )
{
	switch ( Type )
	{
	case SCHED_TAKE_COVER_FROM_ENEMY:
		return &slTubeTakeCoverFromEnemy[ 0 ];
		break;

	case SCHED_RANGE_ATTACK1:
		return &slTubeRangeAttack1[ 0 ];
		break;

	case SCHED_RANGE_ATTACK2:
		return &slTubeRangeAttack2[ 0 ];
		break;

	case SCHED_TUBE_SUPPRESS:
		return &slTubeSuppressRocket[ 0 ];
		break;

	case SCHED_STANDOFF:
		return &slTubeStandoff[ 0 ];
		break;

	case SCHED_FAIL:
		// no fail schedule specified, so pick a good generic one.
		{
			if ( m_hEnemy != NULL )
			{
				// I have an enemy
				return &slTubeCombatFail[ 0 ];
			}

			return &slTubeFail[ 0 ];
		}
		break;
	}

	return CSquadMonster::GetScheduleOfType( Type );
}

int CTube::MonsterType( void )
{
	return 18;
}

//=========================================================
// Classify - indicates this monster's place in the
// relationship table.
//=========================================================
int CTube::Classify( void )
{
	return CLASS_ALIEN_TUBE;
}

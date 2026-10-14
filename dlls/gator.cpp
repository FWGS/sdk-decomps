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
// monster_gator: the mayan alligator. It runs at its enemy
// and bites, swerving when it is hurt on the way. Written
// from the bullsquid, without the spit and the hops.
//=========================================================

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "schedule.h"
#include "soundent.h"

#define		GATOR_SPRINT_DIST	256 // how close the gator has to get before starting to sprint and refusing to swerve

//=========================================================
// monster-specific events
//=========================================================
#define		GATOR_AE_FOOTSTEP_LEFT		( 3 )
#define		GATOR_AE_FOOTSTEP_RIGHT		( 4 )
#define		GATOR_AE_BITE_SOUND			( 7 )
#define		GATOR_AE_BITE				( 8 )

//=========================================================
// AI Schedules Specific to this monster
//=========================================================

// chase enemy schedule
Task_t tlGatorChaseEnemy1[] = 
{
	{ TASK_SET_FAIL_SCHEDULE,	(float)SCHED_IDLE_STAND		},
	{ TASK_GET_PATH_TO_ENEMY,	(float)0					},
	{ TASK_RUN_PATH,			(float)0					},
	{ TASK_WAIT_FOR_MOVEMENT,	(float)0					},
};

Schedule_t slGatorChaseEnemy[] =
{
	{ 
		tlGatorChaseEnemy1,
		ARRAYSIZE ( tlGatorChaseEnemy1 ),
		bits_COND_NEW_ENEMY			|
		bits_COND_ENEMY_DEAD		|
		bits_COND_SMELL_FOOD		|
		bits_COND_CAN_MELEE_ATTACK1	|
		bits_COND_TASK_FAILED		|
		bits_COND_HEAR_SOUND,
		
		bits_SOUND_DANGER			|
		bits_SOUND_MEAT,
		"Gator Chase Enemy"
	},
};

class CGator : public CBaseMonster
{
public:
	void Spawn( void );
	void Precache( void );
	void PrecacheSounds( void );
	int  MonsterType( void ) { return 6; }
	void SetObjectCollisionBox( void )
	{
		pev->absmin = pev->origin + Vector( -48, -48, 0 );
		pev->absmax = pev->origin + Vector( 48, 48, 32 );
	}
	void SetYawSpeed( void );
	int  ISoundMask( void );
	int  VisionFlags( void );
	int  Classify( void );
	void HandleAnimEvent( MonsterEvent_t *pEvent );
	void MeleeAttackSound( void );
	void AlertSound( void );
	void PainSound( void );
	void AttackHitSound( CBaseMonster *pVictim );
	void RunAI( void );
	BOOL CheckMeleeAttack1( float flDot, float flDist );
	int  TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );
	void StartTask( Task_t *pTask );
	Schedule_t *GetSchedule( void );
	Schedule_t *GetScheduleOfType( int Type );

	int	Save( CSave &save ); 
	int Restore( CRestore &restore );

	CUSTOM_SCHEDULES;
	static TYPEDESCRIPTION m_SaveData[];

	static const char *pGatorSounds[];

	float m_flLastAttackTime;	// saved, never used
};

LINK_ENTITY_TO_CLASS( monster_gator, CGator );

TYPEDESCRIPTION	CGator::m_SaveData[] = 
{
	DEFINE_FIELD( CGator, m_flLastAttackTime, FIELD_TIME ),
};

IMPLEMENT_SAVERESTORE( CGator, CBaseMonster );

DEFINE_CUSTOM_SCHEDULES( CGator )
{
	slGatorChaseEnemy,
};

IMPLEMENT_CUSTOM_SCHEDULES( CGator, CBaseMonster );

const char *CGator::pGatorSounds[] =
{
	"gator/gator_footstep1.wav",
	"gator/gator_footstep2.wav",
	"gator/gator_idleroarlow_f0.wav",
	"gator/gator_idlenormal_f0.wav",
	"gator/Gator_Sflinch_F0.wav",
	"gator/Gator_SeePlayer_F0.wav",
	"gator/Gator_BiteAttack.wav",
	"gator/Gator_BiteAttack_1.wav",
	"gator/Gator_BiteAttack_2_F0.wav",
	"gator/gator_diesimple.wav",
};

void CGator::PrecacheSounds( void )
{
	for ( int i = 0; i < ARRAYSIZE( pGatorSounds ); i++ )
		PRECACHE_SOUND( (char *)pGatorSounds[i] );
}

void CGator::PainSound( void )
{
	EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "gator/Gator_Sflinch_F0.wav", 1, ATTN_NORM, 0, PITCH_NORM );
}

void CGator::AlertSound( void )
{
	EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "gator/Gator_SeePlayer_F0.wav", 1, ATTN_NORM, 0, PITCH_NORM );
}

//=========================================================
// MeleeAttackSound - the houndeye's warm-up sound code
//=========================================================
void CGator::MeleeAttackSound( void )
{
	switch ( RANDOM_LONG( 0, 1 ) )
	{
	case 0:
		EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, "gator/Gator_BiteAttack_1.wav", 0.7, ATTN_NORM, 0, PITCH_NORM );
		break;
	case 1:
		EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, "gator/Gator_BiteAttack.wav", 0.7, ATTN_NORM, 0, PITCH_NORM );
		break;
	}
}

void CGator::AttackHitSound( CBaseMonster *pVictim )
{
	EMIT_SOUND_DYN( ENT(pVictim->pev), CHAN_VOICE, "gator/Gator_BiteAttack_2_F0.wav", 1, ATTN_NORM, 0, PITCH_NORM );
}

//=========================================================
// TakeDamage - overridden for gator so we can keep track
// of how much time has passed since it was last injured
//=========================================================
int CGator::TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	float flDist;
	Vector vecApex;

	// if the gator is running, has an enemy, was hurt by the enemy, hasn't been hurt in the last 3 seconds, and isn't too close to the enemy,
	// it will swerve. (whew).
	if ( m_hEnemy != NULL && IsMoving() && pevAttacker == m_hEnemy->pev && gpGlobals->time - m_tLastTimeHurt > 3 )
	{
		flDist = ( pev->origin - m_hEnemy->pev->origin ).Length2D();
		
		if ( flDist > GATOR_SPRINT_DIST )
		{
			flDist = ( pev->origin - m_Route[ m_iRouteIndex ].vecLocation ).Length2D();// reusing flDist. 

			if ( FTriangulate( pev->origin, m_Route[ m_iRouteIndex ].vecLocation, flDist * 0.5, m_hEnemy, &vecApex ) )
			{
				InsertWaypoint( vecApex, bits_MF_TO_DETOUR | bits_MF_DONT_SIMPLIFY );
			}
		}
	}

	return CBaseMonster::TakeDamage( pevInflictor, pevAttacker, flDamage, bitsDamageType );
}

BOOL CGator::CheckMeleeAttack1( float flDot, float flDist )
{
	if ( flDist <= 64 && flDot >= 0.7 )
	{
		return TRUE;
	}
	return FALSE;
}

int CGator::ISoundMask( void )
{
	return	bits_SOUND_WORLD	|
			bits_SOUND_COMBAT	|
		    bits_SOUND_CARCASS	|
			bits_SOUND_MEAT		|
			bits_SOUND_GARBAGE	|
			bits_SOUND_PLAYER;
}

int CGator::VisionFlags( void )
{
	return 7;
}

//=========================================================
// Classify - sic: the gator counts as a human soldier
//=========================================================
int CGator::Classify( void )
{
	return CLASS_HUMAN_MILITARY;
}

void CGator::SetYawSpeed( void )
{
	int ys;

	switch ( m_Activity )
	{
	case ACT_WALK:
	case ACT_RUN:
		ys = 60;
		break;
	case ACT_TURN_LEFT:
	case ACT_TURN_RIGHT:
		ys = 75;
		break;
	default:
		ys = 60;
		break;
	}

	pev->yaw_speed = ys;
}

void CGator::HandleAnimEvent( MonsterEvent_t *pEvent )
{
	switch ( pEvent->event )
	{
	case GATOR_AE_BITE_SOUND:
		MeleeAttackSound();
		// fall through
	case GATOR_AE_BITE:
		{
			int iDamage = gSkillData.gatorDmgBite;
			CBaseEntity *pHurt = CheckEnemyMeleeHit( 72, iDamage, DMG_SLASH, 15, 30 );
			if ( pHurt )
			{
				CBaseMonster *pMonster = pHurt->MyMonsterPointer();
				if ( pMonster )
					AttackHitSound( pMonster );
			}
		}
		break;

	case GATOR_AE_FOOTSTEP_LEFT:
		EMIT_SOUND_DYN( ENT(pev), CHAN_BODY, "gator/gator_footstep1.wav", 1, ATTN_NORM, 0, PITCH_NORM );
		break;

	case GATOR_AE_FOOTSTEP_RIGHT:
		EMIT_SOUND_DYN( ENT(pev), CHAN_BODY, "gator/gator_footstep2.wav", 1, ATTN_NORM, 0, PITCH_NORM );
		break;

	default:
		CBaseMonster::HandleAnimEvent( pEvent );
		break;
	}
}

void CGator::Precache( void )
{
	PRECACHE_MODEL( "models/gator.mdl" );
	CBaseMonster::Precache();
}

void CGator::Spawn( void )
{
	Precache();

	SET_MODEL( ENT(pev), "models/gator.mdl" );
	UTIL_SetSize( pev, Vector( -16, -16, 0 ), Vector( 16, 16, 25 ) );

	pev->solid			= SOLID_SLIDEBOX;
	pev->health			= UTIL_RandomizeHealth( gSkillData.gatorHealth );
	m_flFieldOfView		= 0.7;

	MonsterInit();

	pev->view_ofs		= Vector( 0, 0, 20 );
}

//=========================================================
// RunAI - sprint the last bit of a chase
//=========================================================
void CGator::RunAI( void )
{
	CBaseMonster::RunAI();

	if ( m_hEnemy != NULL && m_Activity == ACT_RUN )
	{
		// chasing enemy. Sprint for last bit
		if ( ( pev->origin - m_hEnemy->pev->origin ).Length2D() < GATOR_SPRINT_DIST )
		{
			pev->framerate = 1.25;
		}
	}
}

Schedule_t *CGator::GetSchedule( void )
{
	switch ( m_MonsterState )
	{
	case MONSTERSTATE_COMBAT:
		{
			// dead enemy
			if ( HasConditions( bits_COND_ENEMY_DEAD ) )
			{
				// call base class, all code to handle dead enemies is centralized there.
				return CBaseMonster::GetSchedule();
			}

			// hurt far from where it last saw the enemy: flinch, now and then
			if ( HasConditions( bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE ) && ( pev->origin - m_vecEnemyLKP ).Length() >= 300 && RANDOM_LONG( 0, 99 ) <= 20 )
			{
				return GetScheduleOfType( SCHED_SMALL_FLINCH );
			}

			if ( HasConditions( bits_COND_NEW_ENEMY ) )
			{
				return GetScheduleOfType( SCHED_WAKE_ANGRY );
			}

			if ( HasConditions( bits_COND_CAN_MELEE_ATTACK1 ) )
			{
				return GetScheduleOfType( SCHED_MELEE_ATTACK1 );
			}

			break;
		}
	case MONSTERSTATE_ALERT:
		{
			if ( HasConditions( bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE ) )
			{
				return GetScheduleOfType( SCHED_SMALL_FLINCH );
			}
			break;
		}
	}

	return CBaseMonster::GetSchedule();
}

Schedule_t *CGator::GetScheduleOfType( int Type )
{
	switch ( Type )
	{
	case SCHED_CHASE_ENEMY:
		return slGatorChaseEnemy;
	}

	return CBaseMonster::GetScheduleOfType( Type );
}

void CGator::StartTask( Task_t *pTask )
{
	m_iTaskStatus = TASKSTATUS_RUNNING;

	switch ( pTask->iTask )
	{
	case TASK_GET_PATH_TO_ENEMY:
		{
			if ( BuildRoute( m_hEnemy->pev->origin, bits_MF_TO_ENEMY, m_hEnemy ) )
			{
				m_iTaskStatus = TASKSTATUS_COMPLETE;
			}
			else
			{
				ALERT( at_aiconsole, "GetPathToEnemy failed!!\n" );
				TaskFail();
			}
			break;
		}
	default:
		CBaseMonster::StartTask( pTask );
		break;
	}
}

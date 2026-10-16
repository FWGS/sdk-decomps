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
//=========================================================
// Wanted: the puma (monster_puma). Built on the SDK houndeye
// (houndeye.cpp, which Wanted doesn't link): the same file
// layout, sounds, hint types and sleeping, with a claw, a
// pounce that hurts what it touches, sitting and lying down,
// and an enemy it forgets when it can't get anywhere. The
// file name, the schedule and task names and the helpers'
// names are ours.
//=========================================================

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"schedule.h"
#include	"nodes.h"
#include	"soundent.h"

extern CGraph WorldGraph;

extern void SaveHorseHull( edict_t *pent );	// name ours, horse.cpp: keeps the size for ExpandHorseHulls

//=========================================================
// monster-specific tasks
//=========================================================
enum
{
	TASK_PUMA_LAND = LAST_COMMON_TASK + 1,
	TASK_PUMA_WANDER,
};

//=========================================================
// monster-specific schedule types
//=========================================================
enum
{
	SCHED_PUMA_SPARE = LAST_COMMON_SCHEDULE + 1,	// unused
	SCHED_PUMA_MELEE_ATTACK1,
	SCHED_PUMA_RANGE_ATTACK1,
};

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define		PUMA_AE_CLAW		1
#define		PUMA_AE_POUNCE		2

class CPuma : public CBaseMonster
{
public:
	void Spawn( void );
	void Precache( void );
	int  Classify ( void );
	void HandleAnimEvent( MonsterEvent_t *pEvent );
	void SetYawSpeed ( void );
	void AttackSound( void );
	void WarnSound( void );
	void AlertSound( void );
	void DeathSound( void );
	void PainSound( void );
	void IdleSound( void );
	void RunAI( void );
	void StartTask( Task_t *pTask );
	void Touch( CBaseEntity *pOther );
	void RunTask ( Task_t *pTask );
	Activity GetDeathActivity( void );
	void PrescheduleThink( void );
	void SetActivity ( Activity NewActivity );
	BOOL CheckMeleeAttack1 ( float flDot, float flDist );
	BOOL CheckRangeAttack1 ( float flDot, float flDist );
	BOOL CheckPounce( float flDist );
	BOOL FValidateHintType ( short sHint );
	float HearingSensitivity( void ) { if ( m_fAsleep ) return 2; return 15; };
	Schedule_t *GetScheduleOfType ( int Type );
	Schedule_t *GetSchedule( void );
	BOOL ShouldAdvanceRoute( float flWaypointDist );
	void MoveExecute( CBaseEntity *pTargetEnt, const Vector &vecDir, float flInterval );
	void StepUp( void );

	int	Save( CSave &save );
	int Restore( CRestore &restore );

	CUSTOM_SCHEDULES;
	static TYPEDESCRIPTION m_SaveData[];

	static const char *pAttackHitSounds[];
	static const char *pAttackMissSounds[];

	BOOL	m_fAsleep;// lying down
	BOOL	m_fSitting;// sitting
	int		m_iUnused1;// never used; name ours
	BOOL	fPounceOffGrnd;// the pounce has left the ground
	float	m_flNextPounce;
	float	flForgetEnemyTime;// give up on an enemy it can't reach by this time
	int		m_iUnused2;// never used; name ours
	float	m_StepSize;
	Vector	m_vecLastOrigin;// where it was when it last saw its enemy or moved; name ours
};
LINK_ENTITY_TO_CLASS( monster_puma, CPuma );

TYPEDESCRIPTION	CPuma::m_SaveData[] =
{
	DEFINE_FIELD( CPuma, m_fAsleep, FIELD_BOOLEAN ),
	DEFINE_FIELD( CPuma, m_fSitting, FIELD_BOOLEAN ),
	DEFINE_FIELD( CPuma, m_StepSize, FIELD_FLOAT ),
	DEFINE_FIELD( CPuma, m_flNextPounce, FIELD_TIME ),
	DEFINE_FIELD( CPuma, flForgetEnemyTime, FIELD_TIME ),
	DEFINE_FIELD( CPuma, fPounceOffGrnd, FIELD_BOOLEAN ),
};

IMPLEMENT_SAVERESTORE( CPuma, CBaseMonster );

const char *CPuma::pAttackHitSounds[] =
{
	"player/pl_pain4.wav",	// sic: the player's pain sounds, not precached here
	"player/pl_pain5.wav",
	"player/pl_pain6.wav",
};

const char *CPuma::pAttackMissSounds[] =
{
	"puma/miss1.wav",
	"puma/miss2.wav",
};

//=========================================================
// Classify - indicates this monster's place in the
// relationship table.
//=========================================================
int	CPuma :: Classify ( void )
{
	return	CLASS_PUMA;
}

//=========================================================
//  FValidateHintType
//=========================================================
BOOL CPuma :: FValidateHintType ( short sHint )
{
	int i;

	static short sPumaHints[] =
	{
		HINT_WORLD_MACHINERY,
		HINT_WORLD_BLINKING_LIGHT,
		HINT_WORLD_HUMAN_BLOOD,
		HINT_WORLD_ALIEN_BLOOD,
	};

	for ( i = 0 ; i < ARRAYSIZE ( sPumaHints ) ; i++ )
	{
		if ( sPumaHints[ i ] == sHint )
		{
			return TRUE;
		}
	}

	ALERT ( at_aiconsole, "Couldn't validate hint type" );
	return FALSE;
}

//=========================================================
// CheckMeleeAttack1 - the claw
//=========================================================
BOOL CPuma :: CheckMeleeAttack1 ( float flDot, float flDist )
{
	if ( m_hEnemy != NULL && flDist <= 85 && flDot >= 0.7 && m_flNextAttack < gpGlobals->time )
	{
		return TRUE;
	}
	return FALSE;
}

//=========================================================
// CheckRangeAttack1 - the pounce: from the ground, at an
// enemy 128-148 units away, a little higher than the puma
// (by less than 40 units), out of claw reach, with room to
// jump 200 units
//=========================================================
BOOL CPuma :: CheckRangeAttack1 ( float flDot, float flDist )
{
	if ( FBitSet( pev->flags, FL_ONGROUND ) && flDist > 128 && flDist <= 148 && flDot >= 0.85 &&
		 m_flNextAttack < gpGlobals->time && m_flNextPounce < gpGlobals->time && m_hEnemy != NULL &&
		 pev->origin.z - m_hEnemy->pev->origin.z < 0 && m_hEnemy->pev->origin.z - pev->origin.z < 40 &&
		 !CheckMeleeAttack1( flDot, flDist ) && pev->movetype != MOVETYPE_NONE && CheckPounce( 200 ) )
	{
		return TRUE;
	}
	return FALSE;
}

//=========================================================
// CheckPounce - is there room to jump flDist units forward?
// The puma is tried as a 104 unit box, its enemy made
// non-solid for the test. Name ours.
//=========================================================
BOOL CPuma :: CheckPounce( float flDist )
{
	BOOL fClear = TRUE;
	float flMoveDist;

	SetBits( pev->flags, FL_FLY );
	UTIL_MakeVectors( pev->angles );

	Vector vecMins = pev->mins;
	Vector vecMaxs = pev->maxs;

	int iEnemySolid;
	if ( m_hEnemy != NULL )
		iEnemySolid = m_hEnemy->pev->solid;
	else
		iEnemySolid = SOLID_NOT;

	if ( m_hEnemy != NULL )
		m_hEnemy->pev->solid = SOLID_NOT;

	UTIL_SetSize( pev, Vector( -52, -52, 0 ), Vector( 52, 52, 44 ) );

	if ( CheckLocalMove( pev->origin, pev->origin + gpGlobals->v_forward * flDist, m_hEnemy, &flMoveDist ) != LOCALMOVE_VALID )
		fClear = FALSE;

	UTIL_SetSize( pev, vecMins, vecMaxs );

	if ( m_hEnemy != NULL )
		m_hEnemy->pev->solid = iEnemySolid;

	ClearBits( pev->flags, FL_FLY );
	return fClear;
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CPuma :: SetYawSpeed ( void )
{
	int ys;

	ys = 30;

	switch ( m_Activity )
	{
	case ACT_CROUCH:
	case ACT_CROUCHIDLE:
	case ACT_RANGE_ATTACK1:
		ys = 0;
		break;
	case ACT_IDLE:
		ys = 30;
		break;
	case ACT_WALK:
		ys = 10;
		break;
	case ACT_RUN:
		ys = 30;
		if ( m_Route[ m_iRouteIndex ].iType )
		{
			// turn faster near the next waypoint: 150 at it, 30 from 400 units
			float flDist = ( m_Route[ m_iRouteIndex ].vecLocation - pev->origin ).Make2D().Length();

			if ( flDist >= 0 && flDist < 400 )
			{
				float flScale = ( 1 - flDist / 400 ) * ys;
				ys = flScale * 4 + ys;
			}
		}
		break;
	case ACT_TURN_LEFT:
	case ACT_TURN_RIGHT:
		ys = 180;
		break;
	}

	pev->yaw_speed = ys;
}

//=========================================================
// SetActivity
//=========================================================
void CPuma :: SetActivity ( Activity NewActivity )
{
	CBaseMonster::SetActivity( NewActivity );

	switch ( NewActivity )
	{
	case ACT_CROUCH:
		m_fSitting = TRUE;
		m_fAsleep = FALSE;
		break;
	case ACT_CROUCHIDLE:
		m_fAsleep = TRUE;
		m_fSitting = FALSE;
		break;
	case ACT_EXCITED:
		m_fSitting = FALSE;
		break;
	case ACT_STAND:
		m_fSitting = FALSE;
		m_fAsleep = FALSE;
		pev->framerate = 0.5;
		break;
	}
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CPuma :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	switch ( pEvent->event )
	{
		case PUMA_AE_CLAW:
		{
			pev->origin.z += 16;
			CBaseEntity *pHurt = CheckTraceHullAttack( 80, gSkillData.pumaDmgClaw, DMG_SLASH );
			pev->origin.z -= 16;

			if ( pHurt )
				EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, pAttackHitSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackHitSounds)-1) ], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
			else
				EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, pAttackMissSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackMissSounds)-1) ], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
			break;
		}

		case PUMA_AE_POUNCE:
		{
			ClearBits( pev->flags, FL_ONGROUND );

			UTIL_MakeVectors ( pev->angles );

			Vector vecJumpDir;	// sic: left unset without an enemy
			if ( m_hEnemy != NULL )
			{
				float gravity = CVAR_GET_FLOAT( "sv_gravity" );
				if ( gravity <= 1 )
					gravity = 1;

				MAKE_VECTORS( pev->angles );
				vecJumpDir = gpGlobals->v_forward;
				vecJumpDir.z = 0;
				vecJumpDir = vecJumpDir * ( 400 / vecJumpDir.Length() );
				float flTime = gravity / 800;
				vecJumpDir.z = flTime * 150;
			}

			pev->velocity = vecJumpDir;
			m_flNextPounce = gpGlobals->time + 1;
			break;
		}

		default:
			CBaseMonster::HandleAnimEvent( pEvent );
			break;
	}
}

//=========================================================
// Spawn
//=========================================================
void CPuma :: Spawn()
{
	Precache( );

	SET_MODEL(ENT(pev), "models/puma.mdl");
	UTIL_SetSize(pev, Vector ( -25, -25, 0 ), Vector ( 25, 25, 44 ) );
	SaveHorseHull( ENT(pev) );

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= BLOOD_COLOR_RED;
	pev->effects		= 0;
	pev->health			= gSkillData.pumaHealth;
	pev->yaw_speed		= 5;//!!! should we put this in the monster's changeanim function since turn rates may vary with state/anim?
	m_flFieldOfView		= 0;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState		= MONSTERSTATE_NONE;
	m_fAsleep			= FALSE;
	m_fSitting			= FALSE;
	m_StepSize			= 36;

	MonsterInit();
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CPuma :: Precache()
{
	PRECACHE_MODEL("models/puma.mdl");

	PRECACHE_SOUND("puma/alert1.wav");
	PRECACHE_SOUND("puma/alert2.wav");
	PRECACHE_SOUND("puma/alert3.wav");

	PRECACHE_SOUND("puma/die1.wav");
	PRECACHE_SOUND("puma/die2.wav");
	PRECACHE_SOUND("puma/die3.wav");

	PRECACHE_SOUND("puma/idle1.wav");
	PRECACHE_SOUND("puma/idle2.wav");
	PRECACHE_SOUND("puma/idle3.wav");

	PRECACHE_SOUND("puma/hunt1.wav");
	PRECACHE_SOUND("puma/hunt2.wav");
	PRECACHE_SOUND("puma/hunt3.wav");

	PRECACHE_SOUND("puma/pain1.wav");
	PRECACHE_SOUND("puma/pain2.wav");
	PRECACHE_SOUND("puma/pain3.wav");

	PRECACHE_SOUND("puma/attack1.wav");
	PRECACHE_SOUND("puma/attack2.wav");

	PRECACHE_SOUND("puma/miss1.wav");
	PRECACHE_SOUND("puma/miss2.wav");
}

//=========================================================
// IdleSound
//=========================================================
void CPuma :: IdleSound ( void )
{
	switch ( RANDOM_LONG(0,2) )
	{
	case 0:
		EMIT_SOUND( ENT(pev), CHAN_VOICE, "puma/idle1.wav", 1, ATTN_NORM );
		break;
	case 1:
		EMIT_SOUND( ENT(pev), CHAN_VOICE, "puma/idle2.wav", 1, ATTN_NORM );
		break;
	case 2:
		EMIT_SOUND( ENT(pev), CHAN_VOICE, "puma/idle3.wav", 1, ATTN_NORM );
		break;
	}
}

//=========================================================
// AttackSound
//=========================================================
void CPuma :: AttackSound ( void )
{
	switch ( RANDOM_LONG(0,1) )
	{
	case 0:
		EMIT_SOUND( ENT(pev), CHAN_WEAPON, "puma/attack1.wav", 1, ATTN_NORM );
		break;
	case 1:
		EMIT_SOUND( ENT(pev), CHAN_WEAPON, "puma/attack2.wav", 1, ATTN_NORM );
		break;
	}
}

//=========================================================
// WarnSound
//=========================================================
void CPuma :: WarnSound ( void )
{
	switch ( RANDOM_LONG(0,2) )
	{
	case 0:
		EMIT_SOUND( ENT(pev), CHAN_VOICE, "puma/hunt1.wav", 1, ATTN_NORM );
		break;
	case 1:
		EMIT_SOUND( ENT(pev), CHAN_VOICE, "puma/hunt2.wav", 1, ATTN_NORM );
		break;
	case 2:
		EMIT_SOUND( ENT(pev), CHAN_VOICE, "puma/hunt3.wav", 1, ATTN_NORM );
		break;
	}
}

//=========================================================
// AlertSound
//=========================================================
void CPuma :: AlertSound ( void )
{
	switch ( RANDOM_LONG(0,2) )
	{
	case 0:
		EMIT_SOUND( ENT(pev), CHAN_VOICE, "puma/alert1.wav", 1, ATTN_NORM );
		break;
	case 1:
		EMIT_SOUND( ENT(pev), CHAN_VOICE, "puma/alert2.wav", 1, ATTN_NORM );
		break;
	case 2:
		EMIT_SOUND( ENT(pev), CHAN_VOICE, "puma/alert3.wav", 1, ATTN_NORM );
		break;
	}
}

//=========================================================
// DeathSound
//=========================================================
void CPuma :: DeathSound ( void )
{
	switch ( RANDOM_LONG(0,2) )
	{
	case 0:
		EMIT_SOUND( ENT(pev), CHAN_VOICE, "puma/die1.wav", 1, ATTN_NORM );
		break;
	case 1:
		EMIT_SOUND( ENT(pev), CHAN_VOICE, "puma/die2.wav", 1, ATTN_NORM );
		break;
	case 2:
		EMIT_SOUND( ENT(pev), CHAN_VOICE, "puma/die3.wav", 1, ATTN_NORM );
		break;
	}
}

//=========================================================
// PainSound
//=========================================================
void CPuma :: PainSound ( void )
{
	switch ( RANDOM_LONG(0,2) )
	{
	case 0:
		EMIT_SOUND( ENT(pev), CHAN_VOICE, "puma/pain1.wav", 1, ATTN_NORM );
		break;
	case 1:
		EMIT_SOUND( ENT(pev), CHAN_VOICE, "puma/pain2.wav", 1, ATTN_NORM );
		break;
	case 2:
		EMIT_SOUND( ENT(pev), CHAN_VOICE, "puma/pain3.wav", 1, ATTN_NORM );
		break;
	}
}

//=========================================================
// RunAI - drop an enemy that is notarget, or one the puma
// hasn't seen for 7 seconds while it stayed within 200
// units of the same spot
//=========================================================
void CPuma :: RunAI( void )
{
	if ( m_hEnemy != NULL && FBitSet( m_hEnemy->pev->flags, FL_NOTARGET ) )
	{
		ClearConditions( bits_COND_SEE_ENEMY | bits_COND_ENEMY_OCCLUDED | bits_COND_NEW_ENEMY );
		SetState( MONSTERSTATE_IDLE );
		flForgetEnemyTime = 0;
	}

	if ( m_MonsterState == MONSTERSTATE_COMBAT )
	{
		if ( !HasConditions( bits_COND_SEE_ENEMY ) )
		{
			if ( ( pev->origin - m_vecLastOrigin ).Length() < 200 )
			{
				if ( flForgetEnemyTime != 0 && flForgetEnemyTime <= gpGlobals->time )
				{
					ClearConditions( bits_COND_SEE_ENEMY | bits_COND_ENEMY_OCCLUDED | bits_COND_NEW_ENEMY );
					SetState( MONSTERSTATE_IDLE );
					flForgetEnemyTime = 0;
					CBaseMonster::RunAI();
					return;
				}
			}
			else
			{
				m_vecLastOrigin = pev->origin;
				flForgetEnemyTime = gpGlobals->time + 7.0;
			}
		}
		else
		{
			m_vecLastOrigin = pev->origin;
			flForgetEnemyTime = gpGlobals->time + 7.0;
		}
	}

	CBaseMonster::RunAI();
}

//=========================================================
// start task
//=========================================================
void CPuma :: StartTask ( Task_t *pTask )
{
	m_iTaskStatus = TASKSTATUS_RUNNING;

	switch ( pTask->iTask )
	{
	case TASK_MELEE_ATTACK1:
		{
			if ( RANDOM_LONG( 0, 1 ) )
				AttackSound();
			m_flNextAttack = gpGlobals->time + 0.5;
			m_flNextPounce = gpGlobals->time + 1;
			CBaseMonster::StartTask( pTask );
			break;
		}
	case TASK_RANGE_ATTACK1_NOTURN:
		{
			pev->movetype = MOVETYPE_TOSS;
			AttackSound();
			m_flNextPounce = 0;
			fPounceOffGrnd = FALSE;
			CBaseMonster::StartTask( pTask );
			m_Activity = ACT_RESET;
			break;
		}
	case TASK_PUMA_LAND:
		{
			if ( FBitSet( pev->flags, FL_ONGROUND ) && pev->velocity.z > 0 )
				pev->velocity.z = -10;
			TaskComplete();
			break;
		}
	case TASK_PUMA_WANDER:
		{
			// walk to a node linked to the nearest one
			int iNode = WorldGraph.FindNearestNode( pev->origin, this );
			if ( iNode == -1 )
			{
				TaskComplete();
				break;
			}

			CNode &node = WorldGraph.m_pNodes[ iNode ];
			Vector vecDist( node.m_vecOrigin - pev->origin );
			if ( vecDist.Make2D().Length() < 64 && node.m_cNumLinks )
			{
				int iLink = RANDOM_LONG( 0, node.m_cNumLinks - 1 );
				CNode &dest = WorldGraph.m_pNodes[ WorldGraph.m_pLinkPool[ node.m_iFirstLink + iLink ].m_iDestNode ];
				if ( ( dest.m_vecOrigin - pev->origin ).Make2D().Length() < 64 )
					dest = WorldGraph.m_pNodes[ WorldGraph.m_pLinkPool[ node.m_iFirstLink + iLink ].m_iSrcNode ];	// sic: copies the node over the graph's
				node = dest;	// sic: likewise
			}

			float flDist = ( node.m_vecOrigin - pev->origin ).Length();	// sic: unused (it only moves VC6's inlining)

			TaskComplete();
			MoveToNode( ACT_WALK, 0, node.m_vecOrigin );
			MakeIdealYaw( node.m_vecOrigin );
			break;
		}
	default:
		{
			CBaseMonster::StartTask( pTask );
			break;
		}
	}
}

//=========================================================
// Touch - the pounce hurts what it lands on
//=========================================================
void CPuma :: Touch( CBaseEntity *pOther )
{
	if ( m_Activity == ACT_RANGE_ATTACK1 && pOther->pev->takedamage != DAMAGE_NO && pOther->Classify() != Classify() )
	{
		if ( !FBitSet( pev->flags, FL_ONGROUND ) )
		{
			pOther->TakeDamage( pev, pev, gSkillData.pumaDmgPounce, DMG_SLASH );
			pev->velocity = g_vecZero;
		}
		SetTouch( NULL );
	}
}

//=========================================================
// RunTask
//=========================================================
void CPuma :: RunTask ( Task_t *pTask )
{
	if ( m_Activity == ACT_RUN )
		SetYawSpeed();
	if ( m_Activity == ACT_TURN_LEFT )
		SetYawSpeed();
	if ( m_Activity == ACT_TURN_RIGHT )
		SetYawSpeed();

	switch ( pTask->iTask )
	{
	case TASK_RANGE_ATTACK1_NOTURN:
		{
			// the pounce ends when it lands, after a second, or
			// when something is in the way
			BOOL fStop = !CheckPounce( 40 );
			TraceResult tr;

			UTIL_TraceLine( pev->origin, pev->origin - Vector( 0, 0, 3 ), dont_ignore_monsters, ENT(pev), &tr );

			if ( tr.flFraction < 1 && fStop )
				fStop = FALSE;

			if ( ( tr.flFraction < 1 && fPounceOffGrnd ) || ( m_flNextPounce != 0 && gpGlobals->time > m_flNextPounce ) )
				fStop = TRUE;
			else if ( tr.flFraction >= 1 && !fPounceOffGrnd )
				fPounceOffGrnd = TRUE;

			if ( fStop )
			{
				pev->movetype = MOVETYPE_STEP;
				SetTouch( NULL );
				m_Activity = ACT_RESET;
				m_IdealActivity = ACT_IDLE;
				TaskComplete();
			}
			break;
		}
	default:
		{
			CBaseMonster::RunTask( pTask );
			break;
		}
	}
}

//=========================================================
// GetDeathActivity
//=========================================================
Activity CPuma :: GetDeathActivity( void )
{
	if ( m_Activity != ACT_RUN )
		return CBaseMonster::GetDeathActivity();

	return ACT_DIEVIOLENT;
}

//=========================================================
// PrescheduleThink
//=========================================================
void CPuma::PrescheduleThink ( void )
{
	// if the puma is mad and is running, make hunt noises.
	if ( m_MonsterState == MONSTERSTATE_COMBAT && m_Activity == ACT_RUN && RANDOM_FLOAT( 0, 1 ) < 0.2 )
	{
		WarnSound();
	}
}

//=========================================================
// AI Schedules Specific to this monster
//=========================================================
Task_t	tlPumaIdleWander[] =
{
	{ TASK_STOP_MOVING,			(float)0		},
	{ TASK_PUMA_WANDER,			(float)0		},
	{ TASK_FACE_IDEAL,			(float)0		},
	{ TASK_WAIT_RANDOM,			(float)0.8		},
	{ TASK_WAIT_FOR_MOVEMENT,	(float)0		},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE	},
	{ TASK_WAIT_RANDOM,			(float)2		},
};

Schedule_t	slPumaIdleWander[] =
{
	{
		tlPumaIdleWander,
		ARRAYSIZE ( tlPumaIdleWander ),
		bits_COND_SEE_FEAR		|
		bits_COND_SMELL_FOOD	|
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE	|
		bits_COND_PROVOKED		|
		bits_COND_NEW_ENEMY		|
		bits_COND_HEAR_SOUND	|
		bits_COND_SMELL,

		bits_SOUND_COMBAT		|// sound flags
		bits_SOUND_WORLD		|
		bits_SOUND_PLAYER		|
		bits_SOUND_DANGER,
		"PumaIdleWander"
	},
};

// the claw
Task_t	tlPumaMeleeAttack1[] =
{
	{ TASK_SET_FAIL_SCHEDULE,	(float)SCHED_SLEEP	},	// sic
	{ TASK_STOP_MOVING,			(float)0			},
	{ TASK_MELEE_ATTACK1,		(float)0			},
};

Schedule_t	slPumaMeleeAttack1[] =
{
	{
		tlPumaMeleeAttack1,
		ARRAYSIZE ( tlPumaMeleeAttack1 ),
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE,
		0,
		"PumaMeleeAttack1"
	},
};

// the pounce
Task_t	tlPumaRangeAttack1[] =
{
	{ TASK_STOP_MOVING,				(float)0		},
	{ TASK_RANGE_ATTACK1_NOTURN,	(float)0		},
	{ TASK_PUMA_LAND,				(float)0		},
};

Schedule_t	slPumaRangeAttack1[] =
{
	{
		tlPumaRangeAttack1,
		ARRAYSIZE ( tlPumaRangeAttack1 ),
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE,
		0,
		"PumaRangeAttack1"
	},
};

// sit down
Task_t	tlPumaSit[] =
{
	{ TASK_STOP_MOVING,			(float)0		},
	{ TASK_SET_ACTIVITY,		(float)ACT_CROUCH	},
	{ TASK_WAIT_RANDOM,			(float)5		},
	{ TASK_WAIT_RANDOM,			(float)5		},
};

Schedule_t	slPumaSit[] =
{
	{
		tlPumaSit,
		ARRAYSIZE ( tlPumaSit ),
		bits_COND_HEAR_SOUND	|
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE	|
		bits_COND_NEW_ENEMY,

		bits_SOUND_COMBAT		|
		bits_SOUND_PLAYER		|
		bits_SOUND_WORLD,
		"Puma Sit"
	},
};

// stay sitting
Task_t	tlPumaSitIdle[] =
{
	{ TASK_PLAY_SEQUENCE,		(float)ACT_CROUCH	},
};

Schedule_t	slPumaSitIdle[] =
{
	{
		tlPumaSitIdle,
		ARRAYSIZE ( tlPumaSitIdle ),
		bits_COND_HEAR_SOUND	|
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE	|
		bits_COND_NEW_ENEMY,

		bits_SOUND_COMBAT		|
		bits_SOUND_PLAYER		|
		bits_SOUND_WORLD,
		"Puma Sit"
	},
};

// get up and look about
Task_t	tlPumaIdleSitup[] =
{
	{ TASK_STOP_MOVING,			(float)0			},
	{ TASK_PLAY_SEQUENCE,		(float)ACT_EXCITED	},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE		},
	{ TASK_WAIT,				(float)3			},
	{ TASK_WAIT_RANDOM,			(float)4			},
};

Schedule_t	slPumaIdleSitup[] =
{
	{
		tlPumaIdleSitup,
		ARRAYSIZE ( tlPumaIdleSitup ),
		0,
		0,
		"Puma IdleSitup"
	},
};

// get up
Task_t	tlPumaSitup[] =
{
	{ TASK_STOP_MOVING,			(float)0			},
	{ TASK_PLAY_SEQUENCE,		(float)ACT_EXCITED	},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE		},
};

Schedule_t	slPumaSitup[] =
{
	{
		tlPumaSitup,
		ARRAYSIZE ( tlPumaSitup ),
		0,
		0,
		"Puma Situp"
	},
};

// lie down and fall asleep
Task_t	tlPumaSleep[] =
{
	{ TASK_STOP_MOVING,			(float)0				},
	{ TASK_SET_ACTIVITY,		(float)ACT_CROUCHIDLE	},
	{ TASK_WAIT,				(float)8				},
	{ TASK_WAIT_RANDOM,			(float)8				},
};

Schedule_t	slPumaSleep[] =
{
	{
		tlPumaSleep,
		ARRAYSIZE ( tlPumaSleep ),
		bits_COND_HEAR_SOUND	|
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE	|
		bits_COND_NEW_ENEMY,

		bits_SOUND_COMBAT		|
		bits_SOUND_PLAYER		|
		bits_SOUND_WORLD,
		"Puma Sleep"
	},
};

// keep sleeping
Task_t	tlPumaSleepIdle[] =
{
	{ TASK_SET_ACTIVITY,		(float)ACT_CROUCHIDLE	},
	{ TASK_WAIT,				(float)3				},
	{ TASK_WAIT_RANDOM,			(float)5				},
};

Schedule_t	slPumaSleepIdle[] =
{
	{
		tlPumaSleepIdle,
		ARRAYSIZE ( tlPumaSleepIdle ),
		bits_COND_HEAR_SOUND	|
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE	|
		bits_COND_NEW_ENEMY,

		bits_SOUND_COMBAT		|
		bits_SOUND_PLAYER		|
		bits_SOUND_WORLD,
		"Puma SleepIdle"
	},
};

// wake and stand up lazily
Task_t	tlPumaWakeLazy[] =
{
	{ TASK_STOP_MOVING,			(float)0			},
	{ TASK_PLAY_SEQUENCE,		(float)ACT_STAND	},
};

Schedule_t	slPumaWakeLazy[] =
{
	{
		tlPumaWakeLazy,
		ARRAYSIZE ( tlPumaWakeLazy ),
		0,
		0,
		"WakeLazy"
	},
};

// puma fails in combat
Task_t	tlPumaCombatFailPVS[] =
{
	{ TASK_SET_FAIL_SCHEDULE,	(float)SCHED_CHASE_ENEMY	},
	{ TASK_STOP_MOVING,			(float)0					},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE				},
};

Schedule_t	slPumaCombatFailPVS[] =
{
	{
		tlPumaCombatFailPVS,
		ARRAYSIZE ( tlPumaCombatFailPVS ),
		bits_COND_NEW_ENEMY			|
		bits_COND_LIGHT_DAMAGE		|
		bits_COND_HEAVY_DAMAGE,
		0,
		"PumaCombatFailPVS"
	},
};

// walk to a sound
Task_t	tlPumaInvestigateSound[] =
{
	{ TASK_STOP_MOVING,				(float)0				},
	{ TASK_GET_PATH_TO_BESTSOUND,	(float)0				},
	{ TASK_WALK_PATH,				(float)0				},
	{ TASK_WAIT_FOR_MOVEMENT,		(float)0				},
	{ TASK_SET_ACTIVITY,			(float)ACT_IDLE			},
	{ TASK_WAIT_RANDOM,				(float)5				},
};

Schedule_t	slPumaInvestigateSound[] =
{
	{
		tlPumaInvestigateSound,
		ARRAYSIZE ( tlPumaInvestigateSound ),
		bits_COND_SEE_FEAR			|
		bits_COND_LIGHT_DAMAGE		|
		bits_COND_HEAVY_DAMAGE		|
		bits_COND_NEW_ENEMY			|
		bits_COND_HEAR_SOUND,

		bits_SOUND_COMBAT		|
		bits_SOUND_WORLD		|
		bits_SOUND_PLAYER		|
		bits_SOUND_DANGER,
		"InvestigateSound"
	},
};

DEFINE_CUSTOM_SCHEDULES( CPuma )
{
	slPumaIdleWander,
	slPumaMeleeAttack1,
	slPumaRangeAttack1,
	slPumaSit,
	slPumaSitIdle,
	slPumaSitup,
	slPumaIdleSitup,
	slPumaSleep,
	slPumaSleepIdle,
	slPumaWakeLazy,
	slPumaCombatFailPVS,
	slPumaInvestigateSound,
};

IMPLEMENT_CUSTOM_SCHEDULES( CPuma, CBaseMonster );

//=========================================================
// GetScheduleOfType
//=========================================================
Schedule_t* CPuma :: GetScheduleOfType ( int Type )
{
	if ( !IsAlive() )
		return CBaseMonster :: GetScheduleOfType ( Type );

	if ( m_fAsleep && Type != SCHED_IDLE_STAND )
	{
		// a sleeping puma gets up first
		return &slPumaWakeLazy[ 0 ];
	}

	if ( m_fSitting && Type != SCHED_IDLE_STAND )
	{
		// so does a sitting one
		return &slPumaSitup[ 0 ];
	}

	switch	( Type )
	{
	case SCHED_IDLE_STAND:
		{
			if ( m_MonsterState == MONSTERSTATE_IDLE && m_IdealMonsterState == MONSTERSTATE_IDLE )
			{
				if ( !m_fSitting && !m_fAsleep && RANDOM_FLOAT( 0, 1 ) < 0.0 )	// sic: never sits down
					return &slPumaSit[ 0 ];

				if ( m_fSitting )
				{
					if ( !m_fAsleep && RANDOM_FLOAT( 0, 1 ) < 0.4 )
						return &slPumaSleep[ 0 ];

					if ( RANDOM_FLOAT( 0, 1 ) < 0.6 )
						return &slPumaIdleSitup[ 0 ];

					return &slPumaSitIdle[ 0 ];
				}

				if ( m_fAsleep )
				{
					if ( RANDOM_FLOAT( 0, 1 ) < 0.5 )
						return &slPumaWakeLazy[ 0 ];

					return &slPumaSleepIdle[ 0 ];
				}

				return &slPumaIdleWander[ 0 ];
			}
			break;
		}
	case SCHED_PUMA_MELEE_ATTACK1:
		{
			return &slPumaMeleeAttack1[ 0 ];
		}
	case SCHED_PUMA_RANGE_ATTACK1:
		{
			return &slPumaRangeAttack1[ 0 ];
		}
	case SCHED_WAKE_ANGRY:
	case SCHED_COMBAT_FACE:
	case SCHED_CHASE_ENEMY:
		{
			if ( ( m_hEnemy->pev->origin - pev->origin ).Length() <= 85 && fabs( pev->angles.y - pev->ideal_yaw ) < 10 )
			{
				if ( HasConditions( bits_COND_CAN_MELEE_ATTACK1 ) )
					CBaseMonster::GetScheduleOfType( SCHED_IDLE_STAND );	// sic: the result is dropped

				return GetScheduleOfType( SCHED_PUMA_MELEE_ATTACK1 );
			}

			return CBaseMonster::GetScheduleOfType( SCHED_CHASE_ENEMY );
		}
	case SCHED_ALERT_FACE:
		{
			m_movementActivity = ACT_WALK;
			return &slPumaInvestigateSound[ 0 ];
		}
	case SCHED_CHASE_ENEMY_FAILED:
	case SCHED_FAIL:
		{
			if ( m_MonsterState == MONSTERSTATE_COMBAT )
				return &slPumaCombatFailPVS[ 0 ];

			return GetScheduleOfType( SCHED_IDLE_STAND );
		}
	}

	return CBaseMonster :: GetScheduleOfType ( Type );
}

//=========================================================
// GetSchedule
//=========================================================
Schedule_t *CPuma :: GetSchedule( void )
{
	switch	( m_MonsterState )
	{
	case MONSTERSTATE_COMBAT:
		{
			// dead enemy
			if ( HasConditions( bits_COND_ENEMY_DEAD ) )
			{
				// call base class, all code to handle dead enemies is centralized there.
				return CBaseMonster :: GetSchedule();
			}

			if ( HasConditions( bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE ) && RANDOM_FLOAT( 0, 1 ) <= 0.6 )
			{
				return GetScheduleOfType( SCHED_TAKE_COVER_FROM_ENEMY );
			}

			if ( HasConditions( bits_COND_CAN_MELEE_ATTACK1 ) )
			{
				return GetScheduleOfType( SCHED_PUMA_MELEE_ATTACK1 );
			}

			if ( HasConditions( bits_COND_CAN_RANGE_ATTACK1 ) )
			{
				return GetScheduleOfType( SCHED_PUMA_RANGE_ATTACK1 );
			}
			break;
		}
	}

	return CBaseMonster :: GetSchedule();
}

//=========================================================
// ShouldAdvanceRoute - cut corners by 64 units
//=========================================================
BOOL CPuma :: ShouldAdvanceRoute( float flWaypointDist )
{
	if ( flWaypointDist <= 64 )
	{
		return TRUE;
	}

	return FALSE;
}

//=========================================================
// MoveExecute - walk the way the puma faces, slower while
// it is still turning
//=========================================================
void CPuma :: MoveExecute( CBaseEntity *pTargetEnt, const Vector &vecDir, float flInterval )
{
	if ( m_IdealActivity != m_movementActivity )
		m_IdealActivity = m_movementActivity;

	// slow down while turning, to no less than 60%
	float flSpeed = m_flGroundSpeed;
	flSpeed *= max( 1.0 - fabs( FlYawDiff() ) / 180, 0.6 );

	float flTotal = flSpeed * pev->framerate * flInterval;
	float flStep;

	MAKE_VECTORS( pev->angles );
	while ( flTotal > 0.001 )
	{
		// don't walk more than 16 units or stairs stop working
		flStep = min( 16.0, flTotal );
		if ( !WALK_MOVE( ENT(pev), pev->angles.y, flStep, WALKMOVE_NORMAL ) )
			ResetSequenceInfo();
		flTotal -= flStep;
	}
}

//=========================================================
// StepUp - climb onto a step up to m_StepSize high in front
// of the puma. Nothing calls it. Name ours.
//=========================================================
void CPuma :: StepUp( void )
{
	TraceResult tr;

	UTIL_MakeVectors( pev->angles );

	Vector vecSrc;
	Vector vecStart = pev->origin;	// sic: unused

	// half the puma's width ahead
	vecSrc = pev->origin + gpGlobals->v_forward * ( pev->maxs - pev->mins ).Make2D().Length() * 0.5;

	SetBits( gpGlobals->trace_flags, FTRACE_SIMPLEBOX );
	UTIL_TraceLine( pev->origin + Vector( 0, 0, 10 ), vecSrc, dont_ignore_monsters, ENT(pev), &tr );
	if ( tr.flFraction >= 1.0 )
		return;

	CBaseEntity *pHit = CBaseEntity::Instance( tr.pHit );
	if ( pHit && m_hEnemy != NULL && pHit->edict() == m_hEnemy->edict() )
		return;

	vecSrc = vecSrc + Vector( 0, 0, m_StepSize );
	SetBits( gpGlobals->trace_flags, FTRACE_SIMPLEBOX );
	UTIL_TraceLine( pev->origin + Vector( 0, 0, m_StepSize ), vecSrc, dont_ignore_monsters, ENT(pev), &tr );
	if ( tr.flFraction < 1.0 )
		return;

	vecSrc = tr.vecEndPos;
	SetBits( gpGlobals->trace_flags, FTRACE_SIMPLEBOX );
	UTIL_TraceLine( vecSrc, vecSrc - Vector( 0, 0, 4000 ), dont_ignore_monsters, ENT(pev), &tr );
	if ( tr.flFraction < 1.0 )
	{
		vecSrc = pev->origin;
		vecSrc.z = tr.vecEndPos.z;
		SetBits( gpGlobals->trace_flags, FTRACE_SIMPLEBOX );
		UTIL_TraceHull( tr.vecEndPos + ( Center() - pev->origin ) * 2, tr.vecEndPos + ( Center() - pev->origin ) * 2, dont_ignore_monsters, head_hull, ENT(pev), &tr );
		if ( !tr.fStartSolid )
			pev->origin = vecSrc;
		else
			pev->origin = pev->origin + Vector( 0, 0, m_StepSize );
	}
	else
	{
		pev->origin = pev->origin + Vector( 0, 0, m_StepSize );
	}

	UTIL_SetOrigin( pev, pev->origin );
}

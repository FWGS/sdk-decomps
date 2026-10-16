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
// Wanted: the grizzly bear (monster_bear). Walks on all fours, rears
// up on its hind legs to swipe with both claws, pounces at enemies a
// little way off and sleeps until something wakes it. The claw events
// and sound arrays follow the SDK zombie, the sounds and save/restore
// the bullsquid; the rest is Wanted's own. The file name, the event,
// task and schedule names, m_fMeleeAttack and the helpers are ours.
//=========================================================

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"schedule.h"
#include	"nodes.h"
#include	"animation.h"
#include	"soundent.h"
#include	"weapons.h"

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define	BEAR_AE_CLAW_RIGHT		( 1 )
#define	BEAR_AE_CLAW_LEFT		( 2 )
#define	BEAR_AE_JUMPATTACK		( 3 )

//=========================================================
// monster-specific schedule types (retail's numbering skips
// LAST_COMMON_SCHEDULE + 1)
//=========================================================
enum
{
	SCHED_BEAR_MELEE_ATTACK = LAST_COMMON_SCHEDULE + 2,
	SCHED_BEAR_RANGE_ATTACK,
};

//=========================================================
// monster-specific tasks (retail's numbering skips
// LAST_COMMON_TASK + 1)
//=========================================================
enum
{
	TASK_BEAR_STAND = LAST_COMMON_TASK + 2,	// rear up on the hind legs
	TASK_BEAR_DROP,							// back down on all fours
	TASK_BEAR_WANDER,						// walk to a node next to the nearest one
	TASK_BEAR_LAND,							// brake after a pounce
	TASK_BEAR_SET_ACTIVITY,					// TASK_SET_ACTIVITY that lets a turn finish
};

class CBear : public CBaseMonster
{
public:
	void Spawn( void );
	void Precache( void );
	int  Classify ( void ) { return CLASS_BEAR; }
	float HearingSensitivity( void ) { return 6.0; }
	BOOL FCanActiveIdle( void );
	BOOL CheckMeleeAttack1 ( float flDot, float flDist );
	BOOL CheckMeleeAttack2 ( float flDot, float flDist );
	BOOL CheckRangeAttack1 ( float flDot, float flDist );
	void SetYawSpeed( void );
	void HandleAnimEvent( MonsterEvent_t *pEvent );
	void IdleSound( void );
	void AttackSound( void );
	void HuntSound( void );
	void AlertSound( void );
	void DeathSound( void );
	void PainSound( void );
	void RunAI( void );
	void StartTask ( Task_t *pTask );
	void EXPORT LeapTouch ( CBaseEntity *pOther );
	void RunTask ( Task_t *pTask );
	Activity GetDeathActivity( void );
	void SetTurnActivity( void );
	void SetActivity ( Activity NewActivity );
	void PrescheduleThink( void );
	Schedule_t *GetScheduleOfType ( int Type );
	Schedule_t *GetSchedule( void );
	void TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType );

	int	Save( CSave &save );
	int Restore( CRestore &restore );

	CUSTOM_SCHEDULES;
	static TYPEDESCRIPTION m_SaveData[];

	static const char *pAttackHitSounds[];
	static const char *pAttackMissSounds[];

	BOOL	m_fStanding;		// up on the hind legs
	BOOL	m_fSleep;
	float	m_flNextPounce;		// a leap lands by this time; no new one before it
	float	m_flSitTime;		// saved, never used
	Vector	m_vecUnused;		// sic: never used (a Vector: the factory constructs three)
	int		m_iUnused;			// sic: never used
	Vector	m_mins;				// the size on all fours
	Vector	m_maxs;
	BOOL	m_fMeleeAttack;		// a melee schedule is running (not saved)
};

LINK_ENTITY_TO_CLASS( monster_bear, CBear );

TYPEDESCRIPTION	CBear::m_SaveData[] =
{
	DEFINE_FIELD( CBear, m_mins, FIELD_VECTOR ),
	DEFINE_FIELD( CBear, m_maxs, FIELD_VECTOR ),
	DEFINE_FIELD( CBear, m_fStanding, FIELD_BOOLEAN ),
	DEFINE_FIELD( CBear, m_fSleep, FIELD_BOOLEAN ),
	DEFINE_FIELD( CBear, m_flSitTime, FIELD_FLOAT ),
	DEFINE_FIELD( CBear, m_flNextPounce, FIELD_FLOAT ),
};

IMPLEMENT_SAVERESTORE( CBear, CBaseMonster );

// sic: never played
const char *CBear::pAttackHitSounds[] =
{
	"player/pl_pain4.wav",
	"player/pl_pain5.wav",
	"player/pl_pain6.wav",
};

const char *CBear::pAttackMissSounds[] =
{
	"grizzly/miss1.wav",
	"grizzly/miss2.wav",
};

//========================================================
// AI Schedules Specific to this monster
//=========================================================

// wander to a node next to the nearest one
Task_t	tlBearIdleWander[] =
{
	{ TASK_STOP_MOVING,			(float)0		},
	{ TASK_BEAR_WANDER,			(float)0		},
	{ TASK_FACE_IDEAL,			(float)0		},
	{ TASK_WAIT_RANDOM,			(float)0.8		},
	{ TASK_WAIT_FOR_MOVEMENT,	(float)0		},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE	},
	{ TASK_WAIT_RANDOM,			(float)2		},
};

Schedule_t	slBearIdleWander[] =
{
	{
		tlBearIdleWander,
		ARRAYSIZE ( tlBearIdleWander ),
		bits_COND_NEW_ENEMY		|
		bits_COND_SEE_FEAR		|
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE	|
		bits_COND_HEAR_SOUND	|
		bits_COND_SMELL			|
		bits_COND_PROVOKED		|
		bits_COND_SMELL_FOOD,

		bits_SOUND_COMBAT		|
		bits_SOUND_WORLD		|
		bits_SOUND_PLAYER		|
		bits_SOUND_DANGER,
		"BearIdleWander"
	},
};

// claws on all fours
Task_t	tlBearMeleeAttack1a[] =
{
	{ TASK_STOP_MOVING,			(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_MELEE_ATTACK1,		(float)0		},
	{ TASK_BEAR_DROP,			(float)0		},
};

Schedule_t	slBearMeleeAttack1a[] =
{
	{
		tlBearMeleeAttack1a,
		ARRAYSIZE ( tlBearMeleeAttack1a ),
		0,
		0,
		"BearMeleeAttack1a"
	},
};

// claws standing up
Task_t	tlBearMeleeAttack1b[] =
{
	{ TASK_STOP_MOVING,			(float)0				},
	{ TASK_FACE_ENEMY,			(float)0				},
	{ TASK_BEAR_STAND,			(float)0				},
	{ TASK_MELEE_ATTACK1,		(float)0				},
	{ TASK_SET_ACTIVITY,		(float)ACT_CROUCHIDLE	},
	{ TASK_WAIT_RANDOM,			(float)0.1				},
	{ TASK_BEAR_DROP,			(float)0				},
};

Schedule_t	slBearMeleeAttack1b[] =
{
	{
		tlBearMeleeAttack1b,
		ARRAYSIZE ( tlBearMeleeAttack1b ),
		0,
		0,
		"BearMeleeAttack1b"
	},
};

// the pounce
Task_t	tlBearRangeAttack1[] =
{
	{ TASK_STOP_MOVING,				(float)0		},
	{ TASK_FACE_ENEMY,				(float)0		},
	{ TASK_RANGE_ATTACK1_NOTURN,	(float)0		},
	{ TASK_BEAR_LAND,				(float)0		},
	{ TASK_STOP_MOVING,				(float)0		},
	{ TASK_SET_ACTIVITY,			(float)ACT_IDLE	},
	{ TASK_FACE_IDEAL,				(float)0		},
};

Schedule_t	slBearRangeAttack1[] =
{
	{
		tlBearRangeAttack1,
		ARRAYSIZE ( tlBearRangeAttack1 ),
		0,
		0,
		"BearRangeAttack1"
	},
};

// stand up for a while (no code picks it)
Task_t	tlBearStand[] =
{
	{ TASK_STOP_MOVING,			(float)0				},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE			},
	{ TASK_WAIT_RANDOM,			(float)3				},
	{ TASK_BEAR_STAND,			(float)0				},
	{ TASK_SET_ACTIVITY,		(float)ACT_CROUCHIDLE	},
	{ TASK_WAIT,				(float)3				},
	{ TASK_WAIT_RANDOM,			(float)6				},
	{ TASK_BEAR_DROP,			(float)0				},
};

Schedule_t	slBearStand[] =
{
	{
		tlBearStand,
		ARRAYSIZE ( tlBearStand ),
		bits_COND_NEW_ENEMY		|
		bits_COND_HEAVY_DAMAGE	|
		bits_COND_HEAR_SOUND,

		bits_SOUND_COMBAT		|
		bits_SOUND_WORLD		|
		bits_SOUND_PLAYER,
		"Bear stand"
	},
};

// back on all fours (no code picks it)
Task_t	tlBearDrop[] =
{
	{ TASK_STOP_MOVING,			(float)0			},
	{ TASK_SET_ACTIVITY,		(float)ACT_CROUCH	},
	{ TASK_WAIT_RANDOM,			(float)3			},
};

Schedule_t	slBearDrop[] =
{
	{
		tlBearDrop,
		ARRAYSIZE ( tlBearDrop ),
		0,
		0,
		"Bear drop"
	},
};

Task_t	tlBearWake[] =
{
	{ TASK_STOP_MOVING,			(float)0			},
	{ TASK_PLAY_SEQUENCE,		(float)ACT_EXCITED	},
};

Schedule_t	slBearWake[] =
{
	{
		tlBearWake,
		ARRAYSIZE ( tlBearWake ),
		0,
		0,
		"Bear wake"
	},
};

Task_t	tlBearSleep[] =
{
	{ TASK_STOP_MOVING,			(float)0				},
	{ TASK_SET_ACTIVITY,		(float)ACT_COMBAT_IDLE	},
	{ TASK_WAIT_INDEFINITE,		(float)0				},
};

Schedule_t	slBearSleep[] =
{
	{
		tlBearSleep,
		ARRAYSIZE ( tlBearSleep ),
		bits_COND_NEW_ENEMY		|
		bits_COND_SEE_FEAR		|
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE	|
		bits_COND_HEAR_SOUND	|
		bits_COND_SMELL			|
		bits_COND_PROVOKED		|
		bits_COND_SMELL_FOOD,

		bits_SOUND_COMBAT		|
		bits_SOUND_WORLD		|
		bits_SOUND_PLAYER		|
		bits_SOUND_DANGER,
		"Bear sleep"
	},
};

Task_t	tlBearCombatFailPVS[] =
{
	{ TASK_STOP_MOVING,			(float)0			},
	{ TASK_BEAR_SET_ACTIVITY,	(float)ACT_IDLE		},
	{ TASK_FACE_ENEMY,			(float)0			},
};

Schedule_t	slBearCombatFailPVS[] =
{
	{
		tlBearCombatFailPVS,
		ARRAYSIZE ( tlBearCombatFailPVS ),
		bits_COND_NEW_ENEMY		|
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE,
		0,
		"BearCombatFailPVS"
	},
};

// defaultai.cpp's with a random wait added
Task_t	tlBearInvestigateSound[] =
{
	{ TASK_STOP_MOVING,				(float)0				},
	{ TASK_STORE_LASTPOSITION,		(float)0				},
	{ TASK_GET_PATH_TO_BESTSOUND,	(float)0				},
	{ TASK_FACE_IDEAL,				(float)0				},
	{ TASK_WALK_PATH,				(float)0				},
	{ TASK_WAIT_FOR_MOVEMENT,		(float)0				},
	{ TASK_PLAY_SEQUENCE,			(float)ACT_IDLE			},
	{ TASK_WAIT,					(float)5				},
	{ TASK_WAIT_RANDOM,				(float)5				},
	{ TASK_GET_PATH_TO_LASTPOSITION,(float)0				},
	{ TASK_WALK_PATH,				(float)0				},
	{ TASK_WAIT_FOR_MOVEMENT,		(float)0				},
	{ TASK_CLEAR_LASTPOSITION,		(float)0				},
};

Schedule_t	slBearInvestigateSound[] =
{
	{
		tlBearInvestigateSound,
		ARRAYSIZE ( tlBearInvestigateSound ),
		bits_COND_NEW_ENEMY			|
		bits_COND_SEE_FEAR			|
		bits_COND_LIGHT_DAMAGE		|
		bits_COND_HEAVY_DAMAGE		|
		bits_COND_HEAR_SOUND,

		bits_SOUND_COMBAT		|
		bits_SOUND_WORLD		|
		bits_SOUND_PLAYER		|
		bits_SOUND_DANGER,
		"InvestigateSound"
	},
};

//=========================================================
// FCanActiveIdle
//=========================================================
BOOL CBear :: FCanActiveIdle ( void )
{
	if ( m_MonsterState == MONSTERSTATE_IDLE && m_IdealMonsterState == MONSTERSTATE_IDLE )
	{
		return TRUE;
	}

	return FALSE;
}

//=========================================================
// CheckMeleeAttack1 - the claws on all fours, once the bear
// is down to 60% of its health
//=========================================================
BOOL CBear :: CheckMeleeAttack1 ( float flDot, float flDist )
{
	if ( m_hEnemy != NULL )
	{
		if ( ( ( flDist <= 100 && RANDOM_FLOAT( 0, 1 ) < 0.3 ) || flDist <= 80 ) && flDot >= 0.65 && pev->health < pev->max_health * 0.6 )
		{
			return TRUE;
		}
	}
	return FALSE;
}

//=========================================================
// CheckMeleeAttack2 - while healthier
//=========================================================
BOOL CBear :: CheckMeleeAttack2 ( float flDot, float flDist )
{
	if ( m_hEnemy != NULL )
	{
		if ( flDist <= 100 && flDot >= 0.5 && pev->health >= pev->max_health * 0.6 )
		{
			return TRUE;
		}
	}
	return FALSE;
}

//=========================================================
// CheckRangeAttack1 - the pounce
//=========================================================
BOOL CBear :: CheckRangeAttack1 ( float flDot, float flDist )
{
	if ( FBitSet( pev->flags, FL_ONGROUND ) && flDist > 100 && flDist <= 140 && flDot >= 0.65 && m_flNextAttack < gpGlobals->time && m_flNextPounce < gpGlobals->time && RANDOM_FLOAT( 0, 1 ) < 0.3 && pev->movetype )
	{
		return TRUE;
	}
	return FALSE;
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CBear :: SetYawSpeed ( void )
{
	int ys;

	ys = 60;

	if ( pev->movetype == MOVETYPE_NONE )
	{
		pev->yaw_speed = 45;
		return;
	}

	switch ( m_IdealActivity )
	{
	case ACT_STAND:
	case ACT_BIG_FLINCH:
	case ACT_RANGE_ATTACK1:
	case ACT_COMBAT_IDLE:
		ys = 0;
		break;
	case ACT_TURN_LEFT:
	case ACT_TURN_RIGHT:
		ys = 180;
		break;
	case ACT_WALK:
	case ACT_RUN:
		ys = 30;
		if ( m_Route[ m_iRouteIndex ].iType )
		{
			// turn faster the closer the next waypoint is
			float flDist = ( m_Route[ m_iRouteIndex ].vecLocation - pev->origin ).Length2D();

			if ( flDist >= 0 && flDist < 600 )
			{
				// 30 at 600 units, up to 150 next to it (in two steps,
				// which keeps retail's two multiplies)
				float flTurn = ( 1 - flDist / 600 ) * 30;
				ys = flTurn * 4 + 30;
			}
		}
		break;
	}

	pev->yaw_speed = ys;
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CBear :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	switch( pEvent->event )
	{
		case BEAR_AE_CLAW_RIGHT:
		{
			// the hull trace is moved up or down for a ducking enemy
			int iHeight = m_fMeleeAttack ? -16 : 32;

			if ( m_hEnemy != NULL && FBitSet( m_hEnemy->pev->flags, FL_DUCKING ) )
				iHeight = m_fMeleeAttack ? -48 : 16;

			pev->origin.z += iHeight;
			CBaseEntity *pHurt = CheckTraceHullAttack( 130, gSkillData.bearDmgClaw, DMG_SLASH );
			pev->origin.z -= iHeight;

			if ( pHurt && pHurt != this )
			{
				if ( pHurt->pev->flags & (FL_MONSTER|FL_CLIENT) )
				{
					pHurt->pev->punchangle.z = RANDOM_FLOAT( 0, 3 );
					pHurt->pev->punchangle.y = RANDOM_FLOAT( -2, 2 );
					pHurt->pev->punchangle.x = -5;
				}
				// sic: a hit plays the miss sound too
				EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, pAttackMissSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackMissSounds)-1) ], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
			}
			else
				EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, pAttackMissSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackMissSounds)-1) ], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
		}
		break;

		case BEAR_AE_CLAW_LEFT:
		{
			int iHeight = -16;

			if ( m_hEnemy != NULL && FBitSet( m_hEnemy->pev->flags, FL_DUCKING ) )
				iHeight = -48;

			pev->origin.z += iHeight;
			CBaseEntity *pHurt = CheckTraceHullAttack( 130, gSkillData.bearDmgClaw, DMG_SLASH );
			pev->origin.z -= iHeight;

			if ( pHurt && pHurt != this )
			{
				if ( pHurt->pev->flags & (FL_MONSTER|FL_CLIENT) )
				{
					pHurt->pev->punchangle.z = RANDOM_FLOAT( -3, 0 );
					pHurt->pev->punchangle.y = RANDOM_FLOAT( -2, 2 );
					pHurt->pev->punchangle.x = -5;
				}
			}
			else
				EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, pAttackMissSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackMissSounds)-1) ], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
		}
		break;

		case BEAR_AE_JUMPATTACK:
		{
			ClearBits( pev->flags, FL_ONGROUND );

			UTIL_SetOrigin (pev, pev->origin + Vector ( 0 , 0 , 1) );// take him off ground so engine doesn't instantly reset onground
			UTIL_MakeVectors ( pev->angles );

			Vector vecJumpDir;
			if ( m_hEnemy != NULL )
			{
				float gravity = CVAR_GET_FLOAT( "sv_gravity" );
				if ( gravity <= 1 )
					gravity = 1;

				// 600 units/s flat towards the enemy, up at a speed that
				// grows with gravity
				vecJumpDir = ( m_hEnemy->pev->origin - pev->origin ).Normalize();
				vecJumpDir.z = 0;
				float flSpeed = 600 / vecJumpDir.Length2D();
				vecJumpDir = vecJumpDir * flSpeed;
				float flScale = gravity / 800;	// 120 at sv_gravity 800
				vecJumpDir.z = flScale * 120;
			}
			// sic: without an enemy the velocity is uninitialised

			pev->velocity = vecJumpDir;
			m_flNextPounce = gpGlobals->time + 1;
		}
		break;

		default:
			CBaseMonster::HandleAnimEvent( pEvent );
			break;
	}
}

//=========================================================
// Spawn
//=========================================================
void CBear :: Spawn()
{
	Precache( );

	SET_MODEL(ENT(pev), "models/bear.mdl");
	pev->mins = Vector( -38, -38, 0 );
	pev->maxs = Vector( 38, 38, 64 );
	m_mins = pev->mins;
	m_maxs = pev->maxs;
	UTIL_SetSize( pev, pev->mins, pev->maxs );

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= BLOOD_COLOR_RED;
	pev->effects		= 0;
	pev->health			= gSkillData.bearHealth;
	pev->yaw_speed		= 5;
	m_flFieldOfView		= 0.4;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState		= MONSTERSTATE_NONE;
	m_fStanding			= FALSE;

	MonsterInit();
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CBear :: Precache()
{
	PRECACHE_MODEL("models/bear.mdl");

	PRECACHE_SOUND("grizzly/alert1.wav");
	PRECACHE_SOUND("grizzly/alert2.wav");
	PRECACHE_SOUND("grizzly/alert3.wav");

	PRECACHE_SOUND("grizzly/die1.wav");
	PRECACHE_SOUND("grizzly/die2.wav");
	PRECACHE_SOUND("grizzly/die3.wav");

	PRECACHE_SOUND("grizzly/idle1.wav");
	PRECACHE_SOUND("grizzly/idle2.wav");
	PRECACHE_SOUND("grizzly/idle3.wav");

	PRECACHE_SOUND("grizzly/hunt1.wav");
	PRECACHE_SOUND("grizzly/hunt2.wav");
	PRECACHE_SOUND("grizzly/hunt3.wav");

	PRECACHE_SOUND("grizzly/pain1.wav");
	PRECACHE_SOUND("grizzly/pain2.wav");
	PRECACHE_SOUND("grizzly/pain3.wav");

	PRECACHE_SOUND("grizzly/attack1.wav");
	PRECACHE_SOUND("grizzly/attack2.wav");

	PRECACHE_SOUND("grizzly/miss1.wav");
	PRECACHE_SOUND("grizzly/miss2.wav");
}

//=========================================================
// IdleSound
//=========================================================
void CBear :: IdleSound ( void )
{
	switch ( RANDOM_LONG(0,2) )
	{
	case 0:
		EMIT_SOUND( ENT(pev), CHAN_VOICE, "grizzly/idle1.wav", 1, ATTN_NORM );
		break;
	case 1:
		EMIT_SOUND( ENT(pev), CHAN_VOICE, "grizzly/idle2.wav", 1, ATTN_NORM );
		break;
	case 2:
		EMIT_SOUND( ENT(pev), CHAN_VOICE, "grizzly/idle3.wav", 1, ATTN_NORM );
		break;
	}
}

//=========================================================
// AttackSound
//=========================================================
void CBear :: AttackSound ( void )
{
	switch ( RANDOM_LONG(0,1) )
	{
	case 0:
		EMIT_SOUND( ENT(pev), CHAN_WEAPON, "grizzly/attack1.wav", 1, ATTN_NORM );
		break;
	case 1:
		EMIT_SOUND( ENT(pev), CHAN_WEAPON, "grizzly/attack2.wav", 1, ATTN_NORM );
		break;
	}
}

//=========================================================
// HuntSound - growls while running at an enemy
//=========================================================
void CBear :: HuntSound ( void )
{
	switch ( RANDOM_LONG(0,2) )
	{
	case 0:
		EMIT_SOUND( ENT(pev), CHAN_VOICE, "grizzly/hunt1.wav", 1, ATTN_NORM );
		break;
	case 1:
		EMIT_SOUND( ENT(pev), CHAN_VOICE, "grizzly/hunt2.wav", 1, ATTN_NORM );
		break;
	case 2:
		EMIT_SOUND( ENT(pev), CHAN_VOICE, "grizzly/hunt3.wav", 1, ATTN_NORM );
		break;
	}
}

//=========================================================
// AlertSound
//=========================================================
void CBear :: AlertSound ( void )
{
	switch ( RANDOM_LONG(0,2) )
	{
	case 0:
		EMIT_SOUND( ENT(pev), CHAN_VOICE, "grizzly/alert1.wav", 1, ATTN_NORM );
		break;
	case 1:
		EMIT_SOUND( ENT(pev), CHAN_VOICE, "grizzly/alert2.wav", 1, ATTN_NORM );
		break;
	case 2:
		EMIT_SOUND( ENT(pev), CHAN_VOICE, "grizzly/alert3.wav", 1, ATTN_NORM );
		break;
	}
}

//=========================================================
// DeathSound
//=========================================================
void CBear :: DeathSound ( void )
{
	switch ( RANDOM_LONG(0,2) )
	{
	case 0:
		EMIT_SOUND( ENT(pev), CHAN_VOICE, "grizzly/die1.wav", 1, ATTN_NORM );
		break;
	case 1:
		EMIT_SOUND( ENT(pev), CHAN_VOICE, "grizzly/die2.wav", 1, ATTN_NORM );
		break;
	case 2:
		EMIT_SOUND( ENT(pev), CHAN_VOICE, "grizzly/die3.wav", 1, ATTN_NORM );
		break;
	}
}

//=========================================================
// PainSound
//=========================================================
void CBear :: PainSound ( void )
{
	switch ( RANDOM_LONG(0,2) )
	{
	case 0:
		EMIT_SOUND( ENT(pev), CHAN_VOICE, "grizzly/pain1.wav", 1, ATTN_NORM );
		break;
	case 1:
		EMIT_SOUND( ENT(pev), CHAN_VOICE, "grizzly/pain2.wav", 1, ATTN_NORM );
		break;
	case 2:
		EMIT_SOUND( ENT(pev), CHAN_VOICE, "grizzly/pain3.wav", 1, ATTN_NORM );
		break;
	}
}

//=========================================================
// RunAI
//=========================================================
void CBear :: RunAI ( void )
{
	CBaseMonster :: RunAI();
}

//=========================================================
// StartTask
//=========================================================
void CBear :: StartTask ( Task_t *pTask )
{
	m_iTaskStatus = TASKSTATUS_RUNNING;

	if ( m_Activity != ACT_BIG_FLINCH )
		pev->effects &= ~EF_NOINTERP;

	switch ( pTask->iTask )
	{
	case TASK_PLAY_SEQUENCE:
		{
			if ( pTask->flData == ACT_EXCITED )
				m_fSleep = FALSE;	// the wake-up sequence

			CBaseMonster :: StartTask ( pTask );
			break;
		}
	case TASK_MELEE_ATTACK1:
		{
			UTIL_SetSize( pev, m_mins, Vector( m_maxs.x, m_maxs.y, 200 ) );

			if ( RANDOM_LONG( 0, 1 ) )
				AttackSound();

			m_flNextAttack = gpGlobals->time + 0.5;
			m_Activity = ACT_RESET;
			CBaseMonster :: StartTask ( pTask );
			break;
		}
	case TASK_RANGE_ATTACK1_NOTURN:
		{
			pev->movetype = MOVETYPE_TOSS;
			AttackSound();
			SetTouch ( LeapTouch );
			m_flNextPounce = 0;
			CBaseMonster :: StartTask ( pTask );
			m_Activity = ACT_RESET;
			break;
		}
	case TASK_BEAR_SET_ACTIVITY:
		{
			if ( FacingIdeal() || ( m_Activity != ACT_TURN_LEFT && m_Activity != ACT_TURN_RIGHT ) )
				m_IdealActivity = (Activity)(int)pTask->flData;

			TaskComplete();
			CBaseMonster :: StartTask ( pTask );
			break;
		}
	case TASK_BEAR_LAND:
		{
			if ( FBitSet( pev->flags, FL_ONGROUND ) )
			{
				pev->velocity.x *= 0.01;
				pev->velocity.y *= 0.01;
				if ( pev->velocity.z > 0 )
					pev->velocity.z = 0;
			}
			TaskComplete();
			break;
		}
	case TASK_BEAR_STAND:
		{
			m_fStanding = TRUE;
			m_Activity = ACT_RESET;
			m_IdealActivity = ACT_STAND;
			UTIL_SetSize( pev, m_mins, Vector( m_maxs.x, m_maxs.y, 200 ) );
			break;
		}
	case TASK_BEAR_DROP:
		{
			m_fStanding = FALSE;
			m_Activity = ACT_RESET;
			m_IdealActivity = ACT_CROUCH;
			UTIL_SetSize( pev, m_mins, m_maxs );
			break;
		}
	case TASK_BEAR_WANDER:
		{
			int iNode = WorldGraph.FindNearestNode( pev->origin, this );

			if ( iNode != NO_NODE )
			{
				CNode &node = WorldGraph.Node( iNode );

				if ( ( node.m_vecOrigin - pev->origin ).Length2D() < 64 && node.m_cNumLinks )
				{
					// sic: copies the linked node over the nearest one in the graph
					node = WorldGraph.Node( WorldGraph.NodeLink( node, RANDOM_LONG( 0, node.m_cNumLinks - 1 ) ).m_iDestNode );
				}

				MoveToNode( ACT_WALK, 0, node.m_vecOrigin );
				MakeIdealYaw( node.m_vecOrigin );
			}
			TaskComplete();
			break;
		}
	default:
		{
			CBaseMonster :: StartTask ( pTask );
			break;
		}
	}
}

//=========================================================
// LeapTouch - the pounce hurts whatever it lands on
//=========================================================
void CBear :: LeapTouch ( CBaseEntity *pOther )
{
	if ( !pOther->pev->takedamage )
	{
		return;
	}

	if ( pOther->Classify() == Classify() )
	{
		return;
	}

	// Don't hit if back on ground
	if ( !FBitSet( pev->flags, FL_ONGROUND ) )
	{
		pOther->TakeDamage( pev, pev, gSkillData.bearDmgPounce, DMG_SLASH );
		pev->velocity = g_vecZero;
	}

	SetTouch( NULL );
}

//=========================================================
// RunTask
//=========================================================
void CBear :: RunTask ( Task_t *pTask )
{
	if ( pev->sequence == LookupSequence( "bear_sleep" ) )
		m_fSleep = TRUE;

	if ( m_Activity == ACT_RUN )
		SetYawSpeed();

	if ( m_Activity == ACT_BIG_FLINCH && m_fSequenceFinished )
	{
		// the flinch ends where the model's root bone went
		MAKE_VECTORS( pev->angles );

		float flZ = pev->origin.z;
		GetBonePosition( 0, pev->origin, Vector( 0, 0, 0 ) );
		pev->origin.z = flZ + 1;
		pev->effects |= EF_NOINTERP;
		UTIL_SetOrigin( pev, pev->origin );
	}

	switch ( pTask->iTask )
	{
	case TASK_FACE_IDEAL:
		{
			ChangeYaw( pev->yaw_speed );

			if ( fabs( FlYawDiff() ) > 10.0 )
				return;

			TaskComplete();
			return;
		}
	case TASK_FACE_ENEMY:
		{
			SetTurnActivity();
			break;
		}
	case TASK_MELEE_ATTACK1:
		{
			if ( m_fSequenceFinished )
			{
				if ( HasConditions( bits_COND_CAN_MELEE_ATTACK1 ) )
				{
					// claw again
					m_fSequenceFinished = FALSE;
					m_Activity = ACT_RESET;
					StartTask( pTask );
				}
				else if ( !m_fStanding )
				{
					UTIL_SetSize( pev, m_mins, m_maxs );
				}
			}
			break;
		}
	case TASK_RANGE_ATTACK1_NOTURN:
		{
			BOOL fBlocked = FALSE;
			float flDist;

			pev->flags |= FL_FLY;
			UTIL_MakeVectors( pev->angles );
			UTIL_SetSize( pev, Vector( -70, -70, 0 ), Vector( 70, 70, 50 ) );

			Vector vecEnd = pev->origin + gpGlobals->v_forward * 40;
			if ( CheckLocalMove( pev->origin, vecEnd, m_hEnemy, &flDist ) != LOCALMOVE_VALID )
				fBlocked = TRUE;

			pev->mins = m_mins;
			pev->maxs = m_maxs;
			UTIL_SetSize( pev, pev->mins, pev->maxs );
			pev->flags &= ~FL_FLY;

			TraceResult tr;
			UTIL_TraceLine( pev->origin, pev->origin - Vector( 0, 0, 3 ), dont_ignore_monsters, ENT(pev), &tr );

			if ( tr.flFraction < 1 )
			{
				// on the ground again: only the end of the leap counts
				if ( fBlocked )
					fBlocked = FALSE;
			}

			if ( ( tr.flFraction < 1 && m_fSequenceFinished ) || ( m_flNextPounce != 0 && gpGlobals->time > m_flNextPounce ) || fBlocked )
			{
				pev->movetype = MOVETYPE_STEP;
				SetTouch( NULL );
				m_Activity = ACT_RESET;
				m_IdealActivity = ACT_IDLE;
				TaskComplete();
			}
			return;
		}
	case TASK_BEAR_STAND:
	case TASK_BEAR_DROP:
		{
			if ( m_fSequenceFinished )
				TaskComplete();
			return;
		}
	}

	// sic: only GetSchedule's side effects remain
	if ( !IsAlive() && ( m_pSchedule == slBearMeleeAttack1a || m_pSchedule == slBearMeleeAttack1b || m_fStanding ) )
		GetSchedule();

	CBaseMonster :: RunTask( pTask );
}

//=========================================================
// GetDeathActivity - standing up or clawing, the bear falls
// over violently
//=========================================================
Activity CBear :: GetDeathActivity ( void )
{
	switch ( m_Activity )
	{
	case ACT_CROUCHIDLE:
	case ACT_STAND:
	case ACT_MELEE_ATTACK1:
		return ACT_DIEVIOLENT;
	}

	return CBaseMonster :: GetDeathActivity();
}

//=========================================================
// SetTurnActivity - CBaseMonster's with a 1 degree threshold
//=========================================================
void CBear :: SetTurnActivity ( void )
{
	float flYD;
	flYD = FlYawDiff();

	if ( flYD <= -1 && LookupActivity ( ACT_TURN_RIGHT ) != ACTIVITY_NOT_AVAILABLE )
	{// big right turn
		m_IdealActivity = ACT_TURN_RIGHT;
	}
	else if ( flYD > 1 && LookupActivity ( ACT_TURN_LEFT ) != ACTIVITY_NOT_AVAILABLE )
	{// big left turn
		m_IdealActivity = ACT_TURN_LEFT;
	}
}

//=========================================================
// SetActivity - CBaseMonster's, but a bear that can't move
// doesn't walk or run, and a small flinch plays out
//=========================================================
void CBear :: SetActivity ( Activity NewActivity )
{
	int	iSequence;

	if ( pev->movetype == MOVETYPE_NONE )
	{
		if ( NewActivity == ACT_WALK || NewActivity == ACT_RUN )
			return;

		if ( NewActivity == ACT_IDLE && m_Activity == ACT_SMALL_FLINCH && !m_fSequenceFinished )
			return;
	}

	iSequence = LookupActivity ( NewActivity );

	// Set to the desired anim, or default anim if the desired is not present
	if ( iSequence > ACTIVITY_NOT_AVAILABLE )
	{
		if ( pev->sequence != iSequence || !m_fSequenceLoops )
		{
			// don't reset frame between walk and run
			if ( !(m_Activity == ACT_WALK || m_Activity == ACT_RUN) || !(NewActivity == ACT_WALK || NewActivity == ACT_RUN))
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

	m_Activity = NewActivity; // Go ahead and set this so it doesn't keep trying when the anim is not present

	// In case someone calls this with something other than the ideal activity
	m_IdealActivity = m_Activity;
}

//=========================================================
// PrescheduleThink
//=========================================================
void CBear :: PrescheduleThink ( void )
{
	if ( m_MonsterState == MONSTERSTATE_COMBAT && m_Activity == ACT_RUN && RANDOM_FLOAT( 0, 1 ) < 0.2 )
	{
		HuntSound();
	}
}

DEFINE_CUSTOM_SCHEDULES( CBear )
{
	slBearIdleWander,
	slBearMeleeAttack1a,
	slBearMeleeAttack1b,
	slBearRangeAttack1,
	slBearStand,
	slBearDrop,
	slBearWake,
	slBearSleep,
	slBearCombatFailPVS,
	slBearInvestigateSound,
};

IMPLEMENT_CUSTOM_SCHEDULES( CBear, CBaseMonster );

//=========================================================
// GetScheduleOfType
//=========================================================
Schedule_t* CBear :: GetScheduleOfType ( int Type )
{
	m_fMeleeAttack = FALSE;

	if ( !IsAlive() )
		return CBaseMonster :: GetScheduleOfType ( Type );

	if ( m_fSleep )
	{
		if ( m_MonsterState == MONSTERSTATE_IDLE )
			return &slBearSleep[ 0 ];
		else
			return &slBearWake[ 0 ];
	}

	switch	( Type )
	{
	case SCHED_COMBAT_FACE:
		{
			if ( !FVisible( m_hEnemy ) )
				return GetScheduleOfType( SCHED_CHASE_ENEMY );
			break;
		}
	case SCHED_BEAR_MELEE_ATTACK:
		{
			m_fMeleeAttack = TRUE;
			if ( RANDOM_FLOAT( 0, 1 ) < 0.5 )
				return &slBearMeleeAttack1a[ 0 ];
			else
				return &slBearMeleeAttack1b[ 0 ];
		}
	case SCHED_BEAR_RANGE_ATTACK:
		{
			return &slBearRangeAttack1[ 0 ];
		}
	case SCHED_CHASE_ENEMY:
		{
			if ( pev->movetype == MOVETYPE_NONE )
				return GetScheduleOfType( SCHED_COMBAT_FACE );

			if ( ( m_hEnemy->pev->origin - pev->origin ).Length() <= 100 )
			{
				if ( !HasConditions( bits_COND_CAN_MELEE_ATTACK1 ) )
					return CBaseMonster :: GetScheduleOfType ( SCHED_IDLE_STAND );
				return GetScheduleOfType( SCHED_BEAR_MELEE_ATTACK );
			}
			break;
		}
	case SCHED_ALERT_FACE:
		{
			m_movementActivity = ACT_WALK;
			return &slBearInvestigateSound[ 0 ];
		}
	case SCHED_CHASE_ENEMY_FAILED:
	case SCHED_FAIL:
		{
			if ( m_MonsterState == MONSTERSTATE_COMBAT )
				return &slBearCombatFailPVS[ 0 ];
			return GetScheduleOfType( SCHED_IDLE_STAND );
		}
	}

	return CBaseMonster :: GetScheduleOfType ( Type );
}

//=========================================================
// GetSchedule
//=========================================================
Schedule_t *CBear :: GetSchedule( void )
{
	switch	( m_MonsterState )
	{
	case MONSTERSTATE_COMBAT:
		{
			if ( HasConditions( bits_COND_ENEMY_DEAD ) )
			{
				return CBaseMonster :: GetSchedule();
			}

			if ( HasConditions( bits_COND_CAN_RANGE_ATTACK1 ) && pev->movetype != MOVETYPE_NONE )
			{
				return GetScheduleOfType ( SCHED_BEAR_RANGE_ATTACK );
			}

			if ( HasConditions( bits_COND_CAN_MELEE_ATTACK1 ) )
			{
				return GetScheduleOfType ( SCHED_BEAR_MELEE_ATTACK );
			}

			break;
		}
	}

	return CBaseMonster :: GetSchedule();
}

//=========================================================
// TraceAttack - every hitgroup but the generic one does half
// as much again; a hard hit knocks a standing or clawing bear
// back, a bear trap makes it flinch
//=========================================================
void CBear :: TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType)
{
	if ( pev->takedamage )
	{
		m_LastHitGroup = ptr->iHitgroup;

		if ( ptr->iHitgroup != HITGROUP_GENERIC && IsAlive() )
		{
			flDamage *= 1.5;

			MAKE_VECTORS( pev->angles );
			if ( CheckLocalMove( pev->origin, pev->origin - gpGlobals->v_forward * 50, NULL, NULL ) && m_Activity != ACT_BIG_FLINCH && ( m_fMeleeAttack || m_fStanding ) && flDamage < pev->health )
			{
				m_IdealActivity = ACT_BIG_FLINCH;
				SetActivity( ACT_BIG_FLINCH );
			}
		}

		if ( pevAttacker && FClassnameIs( pevAttacker, "monster_beartrap" ) && IsAlive() )
		{
			m_IdealActivity = ACT_SMALL_FLINCH;
			SetActivity( ACT_SMALL_FLINCH );
		}

		SpawnBlood(ptr->vecEndPos, BloodColor(), flDamage);// a little surface blood.
		TraceBleed( flDamage, vecDir, ptr, bitsDamageType );
		AddMultiDamage( pevAttacker, this, flDamage, bitsDamageType );
	}
}

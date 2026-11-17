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
#if !defined( OEM_BUILD ) && !defined( HLDEMO_BUILD )

//=========================================================
// hassassin - Human assassin, fast and stealthy
//=========================================================

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"schedule.h"
#include	"squadmonster.h"
#include	"weapons.h"
#include	"soundent.h"
#include	"game.h"
#include	"effects.h"
#include	"decals.h"
#include	"nodes.h"

extern DLL_GLOBAL int  g_iSkillLevel;

//=========================================================
// monster-specific schedule types
//=========================================================
enum
{
	SCHED_ASSASSIN_EXPOSED = LAST_COMMON_SCHEDULE + 1,// cover was blown.
	SCHED_ASSASSIN_JUMP,	// fly through the air
	SCHED_ASSASSIN_JUMP_ATTACK,	// fly through the air and shoot
	SCHED_ASSASSIN_JUMP_LAND, // hit and run away
};

//=========================================================
// monster-specific tasks
//=========================================================

enum
{
	TASK_ASSASSIN_FALL_TO_GROUND = LAST_COMMON_TASK + 1, // falling and waiting to hit ground
};


//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define		ASSASSIN_AE_SHOOT1	1
#define		ASSASSIN_AE_TOSS1	2
#define		ASSASSIN_AE_JUMP	3


#define bits_MEMORY_BADJUMP		(bits_MEMORY_CUSTOM1)

class CHAssassin : public CBaseMonster
{
public:
	void Spawn( void );
	void Precache( void );
	void SetYawSpeed ( void );
	int  Classify ( void );
	int  ISoundMask ( void);
	void Shoot( void );
	void HandleAnimEvent( MonsterEvent_t *pEvent );
	Schedule_t* GetSchedule ( void );
	Schedule_t* GetScheduleOfType ( int Type );
	BOOL CheckMeleeAttack1 ( float flDot, float flDist );	// jump
	// BOOL CheckMeleeAttack2 ( float flDot, float flDist );
	BOOL CheckRangeAttack1 ( float flDot, float flDist );	// shoot
	BOOL CheckRangeAttack2 ( float flDot, float flDist );	// throw grenade
	void StartTask ( Task_t *pTask );
	void RunAI( void );
	void RunTask ( Task_t *pTask );
	void DeathSound ( void );
	void IdleSound ( void );
	CUSTOM_SCHEDULES;

	int HeatSignature( void ) { return (int)pev->renderamt / 100; }	// the cloak hides her from heat seekers
	const char *GetKillDesc( void ) { return "an assassin"; }
	const char *GetKillVerb( BOOL fGibbed, int &iHitGroup ) { return "shot"; }
	const char *GetHitGroupDesc( int iHitGroup ) { return HitGroupIn( iHitGroup ); }
	BOOL CanBeGrenaded( int fLauncher ) { return TRUE; }
	float GetHoldDistance( void ) { return CBaseMonster::GetHoldDistance() * 0.5; }
	int TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
	{
		// a hit uncloaks her
		if ( IsAlive() && flDamage > 0 )
			pev->renderamt = pev->renderamt < 200 ? 200 : pev->renderamt;
		return CBaseMonster::TakeDamage( pevInflictor, pevAttacker, flDamage, bitsDamageType );
	}
	int GetBattlePower( void );

	int	Save( CSave &save ); 
	int Restore( CRestore &restore );
	static TYPEDESCRIPTION m_SaveData[];

	float m_flLastShot;
	float m_flDiviation;

	float m_flNextJump;
	Vector m_vecJumpVelocity;

	float m_flNextGrenadeCheck;
	Vector	m_vecTossVelocity;
	BOOL	m_fThrowGrenade;

	int		m_iTargetRanderamt;

	int		m_iFrustration;

	int		m_iShell;
};
LINK_ENTITY_TO_CLASS( monster_human_assassin, CHAssassin );


TYPEDESCRIPTION	CHAssassin::m_SaveData[] = 
{
	DEFINE_FIELD( CHAssassin, m_flLastShot, FIELD_TIME ),
	DEFINE_FIELD( CHAssassin, m_flDiviation, FIELD_FLOAT ),

	DEFINE_FIELD( CHAssassin, m_flNextJump, FIELD_TIME ),
	DEFINE_FIELD( CHAssassin, m_vecJumpVelocity, FIELD_VECTOR ),

	DEFINE_FIELD( CHAssassin, m_flNextGrenadeCheck, FIELD_TIME ),
	DEFINE_FIELD( CHAssassin, m_vecTossVelocity, FIELD_VECTOR ),
	DEFINE_FIELD( CHAssassin, m_fThrowGrenade, FIELD_BOOLEAN ),

	DEFINE_FIELD( CHAssassin, m_iTargetRanderamt, FIELD_INTEGER ),
	DEFINE_FIELD( CHAssassin, m_iFrustration, FIELD_INTEGER ),
};

IMPLEMENT_SAVERESTORE( CHAssassin, CBaseMonster );


//=========================================================
// DieSound
//=========================================================
void CHAssassin :: DeathSound ( void )
{
}

//=========================================================
// IdleSound
//=========================================================
void CHAssassin :: IdleSound ( void )
{
}

//=========================================================
// ISoundMask - returns a bit mask indicating which types
// of sounds this monster regards. 
//=========================================================
int CHAssassin :: ISoundMask ( void) 
{
	return	bits_SOUND_WORLD	|
			bits_SOUND_COMBAT	|
			bits_SOUND_DANGER	|
			bits_SOUND_PLAYER;
}


//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CHAssassin :: Classify ( void )
{
	// the "MyClass" keyvalue, if any
	if ( m_iMyClass )
		return m_iMyClass;

	return	CLASS_HUMAN_MILITARY;
}

//=========================================================
// GetBattlePower - the target ID's rating of this monster
//=========================================================
int CHAssassin :: GetBattlePower( void )
{
	float flHealth = pev->max_health * 0.1;

	if ( g_iSkillLevel == SKILL_HARD )
		flHealth *= 1.25;

	return flHealth * ( gSkillData.plrDmgHandGrenade * 0.1667 + gSkillData.monDmg9MM * 10 );
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CHAssassin :: SetYawSpeed ( void )
{
	int ys;

	switch ( m_Activity )
	{
	case ACT_TURN_LEFT:
	case ACT_TURN_RIGHT:
		ys = 360;
		break;
	default:			
		ys = 360;
		break;
	}

	pev->yaw_speed = ys;
}


//=========================================================
// Shoot
//=========================================================
void CHAssassin :: Shoot ( void )
{
	if (m_hEnemy == NULL)
	{
		return;
	}

	Vector vecShootOrigin = GetGunPosition();
	Vector vecShootDir = ShootAtEnemy( vecShootOrigin );

	if (m_flLastShot + 2 < gpGlobals->time)
	{
		m_flDiviation = 0.10;
	}
	else
	{
		m_flDiviation -= 0.01;
		if (m_flDiviation < 0.02)
			m_flDiviation = 0.02;
	}
	m_flLastShot = gpGlobals->time;

	UTIL_MakeVectors ( pev->angles );

	Vector	vecShellVelocity = gpGlobals->v_right * RANDOM_FLOAT(40,90) + gpGlobals->v_up * RANDOM_FLOAT(75,200) + gpGlobals->v_forward * RANDOM_FLOAT(-40, 40);
	EjectBrass ( pev->origin + gpGlobals->v_up * 32 + gpGlobals->v_forward * 12, vecShellVelocity, pev->angles.y, m_iShell, TE_BOUNCE_SHELL); 
	FireBullets(1, vecShootOrigin, vecShootDir, Vector( m_flDiviation, m_flDiviation, m_flDiviation ), 2048, BULLET_MONSTER_9MM ); // shoot +-8 degrees

	switch(RANDOM_LONG(0,1))
	{
	case 0:
		EMIT_SOUND(ENT(pev), CHAN_WEAPON, "weapons/pl_gun1.wav", RANDOM_FLOAT(0.6, 0.8), ATTN_NORM);
		break;
	case 1:
		EMIT_SOUND(ENT(pev), CHAN_WEAPON, "weapons/pl_gun2.wav", RANDOM_FLOAT(0.6, 0.8), ATTN_NORM);
		break;
	}

	pev->effects |= EF_MUZZLEFLASH;

	Vector angDir = UTIL_VecToAngles( vecShootDir );
	SetBlending( 0, angDir.x );

	m_cAmmoLoaded--;
}


//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//
// Returns number of events handled, 0 if none.
//=========================================================
void CHAssassin :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	switch( pEvent->event )
	{
	case ASSASSIN_AE_SHOOT1:
		Shoot( );
		break;
	case ASSASSIN_AE_TOSS1:
		{
			UTIL_MakeVectors( pev->angles );
			CGrenade::ShootTimed( pev, pev->origin + gpGlobals->v_forward * 34 + Vector (0, 0, 32), m_vecTossVelocity, 2.0 );

			m_flNextGrenadeCheck = gpGlobals->time + 6;// wait six seconds before even looking again to see if a grenade can be thrown.
			m_fThrowGrenade = FALSE;
			// !!!LATER - when in a group, only try to throw grenade if ordered.
		}
		break;
	case ASSASSIN_AE_JUMP:
		{
			// ALERT( at_console, "jumping");
			UTIL_MakeAimVectors( pev->angles );
			pev->movetype = MOVETYPE_TOSS;
			pev->flags &= ~FL_ONGROUND;
			pev->velocity = m_vecJumpVelocity;
			m_flNextJump = gpGlobals->time + 3.0;
		}
		return;
	default:
		CBaseMonster::HandleAnimEvent( pEvent );
		break;
	}
}

//=========================================================
// Spawn
//=========================================================
void CHAssassin :: Spawn()
{
	Precache( );

	SET_MODEL(ENT(pev), "models/hassassin.mdl");
	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= BLOOD_COLOR_RED;
	pev->effects		= 0;
	pev->health			= gSkillData.hassassinHealth;
	m_flFieldOfView		= VIEW_FIELD_FULL; // indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState		= MONSTERSTATE_NONE;
	m_afCapability		= bits_CAP_MELEE_ATTACK1 | bits_CAP_DOORS_GROUP;
	pev->friction		= 1;

	m_HackedGunPos		= Vector( 0, 24, 48 );

	m_iTargetRanderamt	= 20;
	pev->renderamt		= 20;
	pev->rendermode		= kRenderTransTexture;

	MonsterInit();
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CHAssassin :: Precache()
{
	PRECACHE_MODEL("models/hassassin.mdl");

	PRECACHE_SOUND("weapons/pl_gun1.wav");
	PRECACHE_SOUND("weapons/pl_gun2.wav");

	PRECACHE_SOUND("debris/beamstart1.wav");

	m_iShell = PRECACHE_MODEL ("models/shell.mdl");// brass shell
}	
	


//=========================================================
// AI Schedules Specific to this monster
//=========================================================

//=========================================================
// Fail Schedule
//=========================================================
Task_t	tlAssassinFail[] =
{
	{ TASK_STOP_MOVING,			0				},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE },
	{ TASK_WAIT_FACE_ENEMY,		(float)2		},
	// { TASK_WAIT_PVS,			(float)0		},
	{ TASK_SET_SCHEDULE,		(float)SCHED_CHASE_ENEMY },
};

Schedule_t	slAssassinFail[] =
{
	{
		tlAssassinFail,
		ARRAYSIZE ( tlAssassinFail ),
		bits_COND_LIGHT_DAMAGE		|
		bits_COND_HEAVY_DAMAGE		|
		bits_COND_PROVOKED			|
		bits_COND_CAN_RANGE_ATTACK1 |
		bits_COND_CAN_RANGE_ATTACK2 |
		bits_COND_CAN_MELEE_ATTACK1 |
		bits_COND_HEAR_SOUND,
	
		bits_SOUND_DANGER |
		bits_SOUND_PLAYER,
		"AssassinFail"
	},
};


//=========================================================
// Enemy exposed Agrunt's cover
//=========================================================
Task_t	tlAssassinExposed[] =
{
	{ TASK_STOP_MOVING,			(float)0							},
	{ TASK_RANGE_ATTACK1,		(float)0							},
	{ TASK_SET_FAIL_SCHEDULE,	(float)SCHED_ASSASSIN_JUMP			},
	{ TASK_SET_SCHEDULE,		(float)SCHED_TAKE_COVER_FROM_ENEMY	},
};

Schedule_t slAssassinExposed[] =
{
	{
		tlAssassinExposed,
		ARRAYSIZE ( tlAssassinExposed ),
		bits_COND_CAN_MELEE_ATTACK1,
		0,
		"AssassinExposed",
	},
};


//=========================================================
// Take cover from enemy! Tries lateral cover before node 
// cover! 
//=========================================================
Task_t	tlAssassinTakeCoverFromEnemy[] =
{
	{ TASK_STOP_MOVING,				(float)0					},
	{ TASK_WAIT,					(float)0.2					},
	{ TASK_SET_FAIL_SCHEDULE,		(float)SCHED_RANGE_ATTACK1	},
	{ TASK_FIND_COVER_FROM_ENEMY,	(float)0					},
	{ TASK_RUN_PATH,				(float)0					},
	{ TASK_WAIT_FOR_MOVEMENT,		(float)0					},
	{ TASK_REMEMBER,				(float)bits_MEMORY_INCOVER	},
	{ TASK_FACE_ENEMY,				(float)0					},
};

Schedule_t	slAssassinTakeCoverFromEnemy[] =
{
	{ 
		tlAssassinTakeCoverFromEnemy,
		ARRAYSIZE ( tlAssassinTakeCoverFromEnemy ), 
		bits_COND_NEW_ENEMY |
		bits_COND_CAN_MELEE_ATTACK1		|
		bits_COND_HEAR_SOUND,
		
		bits_SOUND_DANGER,
		"AssassinTakeCoverFromEnemy"
	},
};


//=========================================================
// Take cover from enemy! Tries lateral cover before node 
// cover! 
//=========================================================
Task_t	tlAssassinTakeCoverFromEnemy2[] =
{
	{ TASK_STOP_MOVING,				(float)0					},
	{ TASK_WAIT,					(float)0.2					},
	{ TASK_FACE_ENEMY,				(float)0					},
	{ TASK_RANGE_ATTACK1,			(float)0					},
	{ TASK_SET_FAIL_SCHEDULE,		(float)SCHED_RANGE_ATTACK2	},
	{ TASK_FIND_FAR_NODE_COVER_FROM_ENEMY,	(float)384			},
	{ TASK_RUN_PATH,				(float)0					},
	{ TASK_WAIT_FOR_MOVEMENT,		(float)0					},
	{ TASK_REMEMBER,				(float)bits_MEMORY_INCOVER	},
	{ TASK_FACE_ENEMY,				(float)0					},
};

Schedule_t	slAssassinTakeCoverFromEnemy2[] =
{
	{ 
		tlAssassinTakeCoverFromEnemy2,
		ARRAYSIZE ( tlAssassinTakeCoverFromEnemy2 ), 
		bits_COND_NEW_ENEMY |
		bits_COND_CAN_MELEE_ATTACK2		|
		bits_COND_HEAR_SOUND,
		
		bits_SOUND_DANGER,
		"AssassinTakeCoverFromEnemy2"
	},
};


//=========================================================
// hide from the loudest sound source
//=========================================================
Task_t	tlAssassinTakeCoverFromBestSound[] =
{
	{ TASK_SET_FAIL_SCHEDULE,			(float)SCHED_MELEE_ATTACK1	},
	{ TASK_STOP_MOVING,					(float)0					},
	{ TASK_FIND_COVER_FROM_BEST_SOUND,	(float)0					},
	{ TASK_RUN_PATH,					(float)0					},
	{ TASK_WAIT_FOR_MOVEMENT,			(float)0					},
	{ TASK_REMEMBER,					(float)bits_MEMORY_INCOVER	},
	{ TASK_TURN_LEFT,					(float)179					},
};

Schedule_t	slAssassinTakeCoverFromBestSound[] =
{
	{ 
		tlAssassinTakeCoverFromBestSound,
		ARRAYSIZE ( tlAssassinTakeCoverFromBestSound ), 
		bits_COND_NEW_ENEMY,
		0,
		"AssassinTakeCoverFromBestSound"
	},
};





//=========================================================
// AlertIdle Schedules
//=========================================================
Task_t	tlAssassinHide[] =
{
	{ TASK_STOP_MOVING,			0						 },
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE			 },
	{ TASK_WAIT,				(float)2				 },
	{ TASK_SET_SCHEDULE,		(float)SCHED_CHASE_ENEMY },
};

Schedule_t	slAssassinHide[] =
{
	{ 
		tlAssassinHide,
		ARRAYSIZE ( tlAssassinHide ), 
		bits_COND_NEW_ENEMY				|
		bits_COND_SEE_ENEMY				|
		bits_COND_SEE_FEAR				|
		bits_COND_LIGHT_DAMAGE			|
		bits_COND_HEAVY_DAMAGE			|
		bits_COND_PROVOKED		|
		bits_COND_HEAR_SOUND,
		
		bits_SOUND_DANGER,
		"AssassinHide"
	},
};



//=========================================================
// HUNT Schedules
//=========================================================
Task_t tlAssassinHunt[] = 
{
	{ TASK_GET_PATH_TO_ENEMY,	(float)0		},
	{ TASK_RUN_PATH,			(float)0		},
	{ TASK_WAIT_FOR_MOVEMENT,	(float)0		},
};

Schedule_t slAssassinHunt[] =
{
	{ 
		tlAssassinHunt,
		ARRAYSIZE ( tlAssassinHunt ),
		bits_COND_NEW_ENEMY			|
		// bits_COND_SEE_ENEMY			|
		bits_COND_CAN_RANGE_ATTACK1	|
		bits_COND_HEAR_SOUND,
		
		bits_SOUND_DANGER,
		"AssassinHunt"
	},
};


//=========================================================
// Jumping Schedules
//=========================================================
Task_t	tlAssassinJump[] =
{
	{ TASK_STOP_MOVING,			(float)0		},
	{ TASK_PLAY_SEQUENCE,		(float)ACT_HOP	},
	{ TASK_SET_SCHEDULE,		(float)SCHED_ASSASSIN_JUMP_ATTACK },
};

Schedule_t	slAssassinJump[] =
{
	{ 
		tlAssassinJump,
		ARRAYSIZE ( tlAssassinJump ), 
		0, 
		0, 
		"AssassinJump"
	},
};


//=========================================================
// repel 
//=========================================================
Task_t	tlAssassinJumpAttack[] =
{
	{ TASK_SET_FAIL_SCHEDULE,	(float)SCHED_ASSASSIN_JUMP_LAND	},
	// { TASK_SET_ACTIVITY,		(float)ACT_FLY	},
	{ TASK_ASSASSIN_FALL_TO_GROUND, (float)0		},
};


Schedule_t	slAssassinJumpAttack[] =
{
	{ 
		tlAssassinJumpAttack,
		ARRAYSIZE ( tlAssassinJumpAttack ), 
		0, 
		0,
		"AssassinJumpAttack"
	},
};


//=========================================================
// repel 
//=========================================================
Task_t	tlAssassinJumpLand[] =
{
	{ TASK_SET_FAIL_SCHEDULE,		(float)SCHED_ASSASSIN_EXPOSED	},
	// { TASK_SET_FAIL_SCHEDULE,		(float)SCHED_MELEE_ATTACK1	},
	{ TASK_SET_ACTIVITY,			(float)ACT_IDLE				},
	{ TASK_REMEMBER,				(float)bits_MEMORY_BADJUMP	},
	{ TASK_FIND_NODE_COVER_FROM_ENEMY,	(float)0					},
	{ TASK_RUN_PATH,				(float)0					},
	{ TASK_FORGET,					(float)bits_MEMORY_BADJUMP	},
	{ TASK_WAIT_FOR_MOVEMENT,		(float)0					},
	{ TASK_REMEMBER,				(float)bits_MEMORY_INCOVER	},
	{ TASK_FACE_ENEMY,				(float)0					},
	{ TASK_SET_FAIL_SCHEDULE,		(float)SCHED_RANGE_ATTACK1	},
};

Schedule_t	slAssassinJumpLand[] =
{
	{ 
		tlAssassinJumpLand,
		ARRAYSIZE ( tlAssassinJumpLand ), 
		0, 
		0,
		"AssassinJumpLand"
	},
};

DEFINE_CUSTOM_SCHEDULES( CHAssassin )
{
	slAssassinFail,
	slAssassinExposed,
	slAssassinTakeCoverFromEnemy,
	slAssassinTakeCoverFromEnemy2,
	slAssassinTakeCoverFromBestSound,
	slAssassinHide,
	slAssassinHunt,
	slAssassinJump,
	slAssassinJumpAttack,
	slAssassinJumpLand,
};

IMPLEMENT_CUSTOM_SCHEDULES( CHAssassin, CBaseMonster );


//=========================================================
// CheckMeleeAttack1 - jump like crazy if the enemy gets too close. 
//=========================================================
BOOL CHAssassin :: CheckMeleeAttack1 ( float flDot, float flDist )
{
	if ( m_flNextJump < gpGlobals->time && (flDist <= 128 || HasMemory( bits_MEMORY_BADJUMP )) && m_hEnemy != NULL )
	{
		TraceResult	tr;

		Vector vecDest = pev->origin + Vector( RANDOM_FLOAT( -64, 64), RANDOM_FLOAT( -64, 64 ), 160 );

		UTIL_TraceHull( pev->origin + Vector( 0, 0, 36 ), vecDest + Vector( 0, 0, 36 ), dont_ignore_monsters, human_hull, ENT(pev), &tr);

		if ( tr.fStartSolid || tr.flFraction < 1.0)
		{
			return FALSE;
		}

		float flGravity = g_psv_gravity->value;

		float time = sqrt( 160 / (0.5 * flGravity));
		float speed = flGravity * time / 160;
		m_vecJumpVelocity = (vecDest - pev->origin) * speed;

		return TRUE;
	}
	return FALSE;
}

//=========================================================
// CheckRangeAttack1  - drop a cap in their ass
//
//=========================================================
BOOL CHAssassin :: CheckRangeAttack1 ( float flDot, float flDist )
{
	if ( !HasConditions( bits_COND_ENEMY_OCCLUDED ) && flDist > 64 && flDist <= 2048 /* && flDot >= 0.5 */ /* && NoFriendlyFire() */ )
	{
		TraceResult	tr;

		Vector vecSrc = GetGunPosition();

		// verify that a bullet fired from the gun will hit the enemy before the world.
		UTIL_TraceLine( vecSrc, m_hEnemy->BodyTarget(vecSrc), dont_ignore_monsters, ENT(pev), &tr);

		if ( tr.flFraction == 1 || tr.pHit == m_hEnemy->edict() )
		{
			return TRUE;
		}
	}
	return FALSE;
}

//=========================================================
// CheckRangeAttack2 - toss grenade is enemy gets in the way and is too close. 
//=========================================================
BOOL CHAssassin :: CheckRangeAttack2 ( float flDot, float flDist )
{
	m_fThrowGrenade = FALSE;
	if ( !FBitSet ( m_hEnemy->pev->flags, FL_ONGROUND ) )
	{
		// don't throw grenades at anything that isn't on the ground!
		return FALSE;
	}

	// don't get grenade happy unless the player starts to piss you off
	if ( m_iFrustration <= 2)
		return FALSE;

	if ( m_flNextGrenadeCheck < gpGlobals->time && !HasConditions( bits_COND_ENEMY_OCCLUDED ) && flDist <= 512 /* && flDot >= 0.5 */ /* && NoFriendlyFire() */ )
	{
		Vector vecToss = VecCheckThrow( pev, GetGunPosition( ), m_hEnemy->Center(), flDist, 0.5 ); // use dist as speed to get there in 1 second

		if ( vecToss != g_vecZero )
		{
			m_vecTossVelocity = vecToss;

			// throw a hand grenade
			m_fThrowGrenade = TRUE;

			return TRUE;
		}
	}

	return FALSE;
}


//=========================================================
// RunAI
//=========================================================
void CHAssassin :: RunAI( void )
{
	CBaseMonster :: RunAI();

	// always visible if moving
	// always visible is not on hard
	if (g_iSkillLevel != SKILL_HARD || m_hEnemy == NULL || pev->deadflag != DEAD_NO || m_Activity == ACT_RUN || m_Activity == ACT_WALK || !(pev->flags & FL_ONGROUND))
		m_iTargetRanderamt = 255;
	else
		m_iTargetRanderamt = 20;

	if (pev->renderamt > m_iTargetRanderamt)
	{
		if (pev->renderamt == 255)
		{
			EMIT_SOUND (ENT(pev), CHAN_BODY, "debris/beamstart1.wav", 0.2, ATTN_NORM );
		}

		pev->renderamt = max( pev->renderamt - 50, m_iTargetRanderamt );
		pev->rendermode = kRenderTransTexture;
	}
	else if (pev->renderamt < m_iTargetRanderamt)
	{
		pev->renderamt = min( pev->renderamt + 50, m_iTargetRanderamt );
		if (pev->renderamt == 255)
			pev->rendermode = kRenderNormal;
	}

	if (m_Activity == ACT_RUN || m_Activity == ACT_WALK)
	{
		static int iStep = 0;
		iStep = ! iStep;
		if (iStep)
		{
			switch( RANDOM_LONG( 0, 3 ) )
			{
			case 0:	EMIT_SOUND( ENT(pev), CHAN_BODY, "player/pl_step1.wav", 0.5, ATTN_NORM);	break;
			case 1:	EMIT_SOUND( ENT(pev), CHAN_BODY, "player/pl_step3.wav", 0.5, ATTN_NORM);	break;
			case 2:	EMIT_SOUND( ENT(pev), CHAN_BODY, "player/pl_step2.wav", 0.5, ATTN_NORM);	break;
			case 3:	EMIT_SOUND( ENT(pev), CHAN_BODY, "player/pl_step4.wav", 0.5, ATTN_NORM);	break;
			}
		}
	}
}


//=========================================================
// StartTask
//=========================================================
void CHAssassin :: StartTask ( Task_t *pTask )
{
	switch ( pTask->iTask )
	{
	case TASK_RANGE_ATTACK2:
		if (!m_fThrowGrenade)
		{
			TaskComplete( );
		}
		else
		{
			CBaseMonster :: StartTask ( pTask );
		}
		break;
	case TASK_ASSASSIN_FALL_TO_GROUND:
		break;
	default:
		CBaseMonster :: StartTask ( pTask );
		break;
	}
}


//=========================================================
// RunTask 
//=========================================================
void CHAssassin :: RunTask ( Task_t *pTask )
{
	switch ( pTask->iTask )
	{
	case TASK_ASSASSIN_FALL_TO_GROUND:
		MakeIdealYaw( m_vecEnemyLKP );
		ChangeYaw( pev->yaw_speed );

		if (m_fSequenceFinished)
		{
			if (pev->velocity.z > 0)
			{
				pev->sequence = LookupSequence( "fly_up" );
			}
			else if (HasConditions ( bits_COND_SEE_ENEMY ))
			{
				pev->sequence = LookupSequence( "fly_attack" );
				pev->frame = 0;
			}
			else
			{
				pev->sequence = LookupSequence( "fly_down" );
				pev->frame = 0;
			}
			
			ResetSequenceInfo( );
			SetYawSpeed();
		}
		if (pev->flags & FL_ONGROUND)
		{
			// ALERT( at_console, "on ground\n");
			TaskComplete( );
		}
		break;
	default: 
		CBaseMonster :: RunTask ( pTask );
		break;
	}
}

//=========================================================
// GetSchedule - Decides which type of schedule best suits
// the monster's current state and conditions. Then calls
// monster's member function to get a pointer to a schedule
// of the proper type.
//=========================================================
Schedule_t *CHAssassin :: GetSchedule ( void )
{
	switch	( m_MonsterState )
	{
	case MONSTERSTATE_IDLE:
	case MONSTERSTATE_ALERT:
		{
			if ( HasConditions ( bits_COND_HEAR_SOUND ))
			{
				CSound *pSound;
				pSound = PBestSound();

				ASSERT( pSound != NULL );
				if ( pSound && (pSound->m_iType & bits_SOUND_DANGER) )
				{
					return GetScheduleOfType( SCHED_TAKE_COVER_FROM_BEST_SOUND );
				}
				if ( pSound && (pSound->m_iType & bits_SOUND_COMBAT) )
				{
					return GetScheduleOfType( SCHED_INVESTIGATE_SOUND );
				}
			}
		}
		break;

	case MONSTERSTATE_COMBAT:
		{
// dead enemy
			if ( HasConditions( bits_COND_ENEMY_DEAD ) )
			{
				// call base class, all code to handle dead enemies is centralized there.
				return CBaseMonster :: GetSchedule();
			}

			// flying?
			if ( pev->movetype == MOVETYPE_TOSS)
			{
				if (pev->flags & FL_ONGROUND)
				{
					// ALERT( at_console, "landed\n");
					// just landed
					pev->movetype = MOVETYPE_STEP;
					return GetScheduleOfType ( SCHED_ASSASSIN_JUMP_LAND );
				}
				else
				{
					// ALERT( at_console, "jump\n");
					// jump or jump/shoot
					if ( m_MonsterState == MONSTERSTATE_COMBAT )
						return GetScheduleOfType ( SCHED_ASSASSIN_JUMP );
					else
						return GetScheduleOfType ( SCHED_ASSASSIN_JUMP_ATTACK );
				}
			}

			if ( HasConditions ( bits_COND_HEAR_SOUND ))
			{
				CSound *pSound;
				pSound = PBestSound();

				ASSERT( pSound != NULL );
				if ( pSound && (pSound->m_iType & bits_SOUND_DANGER) )
				{
					return GetScheduleOfType( SCHED_TAKE_COVER_FROM_BEST_SOUND );
				}
			}

			if ( HasConditions ( bits_COND_LIGHT_DAMAGE ) )
			{
				m_iFrustration++;
			}
			if ( HasConditions ( bits_COND_HEAVY_DAMAGE ) )
			{
				m_iFrustration++;
			}

		// jump player!
			if ( HasConditions ( bits_COND_CAN_MELEE_ATTACK1 ) )
			{
				// ALERT( at_console, "melee attack 1\n");
				return GetScheduleOfType ( SCHED_MELEE_ATTACK1 );
			}

		// throw grenade
			if ( HasConditions ( bits_COND_CAN_RANGE_ATTACK2 ) )
			{
				// ALERT( at_console, "range attack 2\n");
				return GetScheduleOfType ( SCHED_RANGE_ATTACK2 );
			}

		// spotted
			if ( HasConditions ( bits_COND_SEE_ENEMY ) && HasConditions ( bits_COND_ENEMY_FACING_ME ) )
			{
				// ALERT( at_console, "exposed\n");
				m_iFrustration++;
				return GetScheduleOfType ( SCHED_ASSASSIN_EXPOSED );
			}

		// can attack
			if ( HasConditions ( bits_COND_CAN_RANGE_ATTACK1 ) )
			{
				// ALERT( at_console, "range attack 1\n");
				m_iFrustration = 0;
				return GetScheduleOfType ( SCHED_RANGE_ATTACK1 );
			}

			if ( HasConditions ( bits_COND_SEE_ENEMY ) )
			{
				// ALERT( at_console, "face\n");
				return GetScheduleOfType ( SCHED_COMBAT_FACE );
			}

		// new enemy
			if ( HasConditions ( bits_COND_NEW_ENEMY ) )
			{
				// ALERT( at_console, "take cover\n");
				return GetScheduleOfType ( SCHED_TAKE_COVER_FROM_ENEMY );
			}

			// ALERT( at_console, "stand\n");
			return GetScheduleOfType ( SCHED_ALERT_STAND );
		}
		break;
	}

	return CBaseMonster :: GetSchedule();
}

//=========================================================
//=========================================================
Schedule_t* CHAssassin :: GetScheduleOfType ( int Type ) 
{
	// ALERT( at_console, "%d\n", m_iFrustration );
	switch	( Type )
	{
	case SCHED_TAKE_COVER_FROM_ENEMY:
		if (pev->health > 30)
			return slAssassinTakeCoverFromEnemy;
		else
			return slAssassinTakeCoverFromEnemy2;
	case SCHED_TAKE_COVER_FROM_BEST_SOUND:
		return slAssassinTakeCoverFromBestSound;
	case SCHED_ASSASSIN_EXPOSED:
		return slAssassinExposed;
	case SCHED_FAIL:
		if (m_MonsterState == MONSTERSTATE_COMBAT)
			return slAssassinFail;
		break;
	case SCHED_ALERT_STAND:
		if (m_MonsterState == MONSTERSTATE_COMBAT)
			return slAssassinHide;
		break;
	case SCHED_CHASE_ENEMY:
		return slAssassinHunt;
	case SCHED_MELEE_ATTACK1:
		if (pev->flags & FL_ONGROUND)
		{
			if (m_flNextJump > gpGlobals->time)
			{
				// can't jump yet, go ahead and fail
				return slAssassinFail;
			}
			else
			{
				return slAssassinJump;
			}
		}
		else
		{
			return slAssassinJumpAttack;
		}
	case SCHED_ASSASSIN_JUMP:
	case SCHED_ASSASSIN_JUMP_ATTACK:
		return slAssassinJumpAttack;
	case SCHED_ASSASSIN_JUMP_LAND:
		return slAssassinJumpLand;
	}

	return CBaseMonster :: GetScheduleOfType( Type );
}


//=========================================================
// powerwave - the Nephilim agent's power wave, a slow rocket.
// A living thing it touches that can be pushed is carried
// along in front of it; anything else (or a second victim)
// makes it blow up: 1000 damage to what it hit, its burden
// gibbed, then an energy sphere like the mortar's.
//=========================================================
class CKamehameha : public CRocket
{
public:
	void Spawn( void );
	void Precache( void );
	const char *GetKillDesc( void ) { return "Nepha Power Wave"; }
	const char *GetKillVerb( BOOL fGibbed, int &iHitGroup ) { if ( pev->weapons ) return "annihilated"; return DamageVerb( DMG_BLAST, fGibbed ); }

	void EXPORT KameTouch( CBaseEntity *pOther );
	void EXPORT KameThink( void );

	int m_iTrail;	// name ours
};
LINK_ENTITY_TO_CLASS( powerwave, CKamehameha );

void CKamehameha :: Spawn( void )
{
	Precache( );

	SET_MODEL( ENT( pev ), "models/powerwave.mdl" );
	UTIL_SetSize( pev, Vector( -16, 0, -16 ), Vector( 16, 16, 16 ) );
	UTIL_SetOrigin( pev, pev->origin );

	EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, "ambience/rocketflame1.wav", 1.0, 0.1, 0, 80 );
	EMIT_SOUND_DYN( ENT( pev ), CHAN_WEAPON, "ambience/rocketrumble1.wav", 1.0, ATTN_NONE, 0, PITCH_NORM );

	if ( !pev->owner || pev->owner->v.movetype != MOVETYPE_TOSS )
		BeamFollow( m_iTrail, 100, 10, Vector( 255, 255, 255 ), 255 );

	SetThink( KameThink );
	SetTouch( KameTouch );
	pev->nextthink = gpGlobals->time + 0.1;

	UTIL_MakeVectors( pev->angles );

	pev->speed = 2000;
	pev->movetype = MOVETYPE_FLYMISSILE;
	pev->solid = SOLID_BBOX;
	pev->weapons = 0;	// not carrying anything
	pev->rendermode = kRenderTransAdd;
	pev->renderamt = 255;
	pev->skin = 0;
	pev->rendercolor = Vector( 128, 100, 0 );
}

void CKamehameha :: Precache( void )
{
	PRECACHE_MODEL( "models/powerwave.mdl" );
	m_iTrail = PRECACHE_MODEL( "sprites/LRed.spr" );
	PRECACHE_SOUND( "ambience/rocketflame1.wav" );
	PRECACHE_SOUND( "ambience/rocketrumble1.wav" );
	PRECACHE_SOUND( "weapons/mortarhit.wav" );
}

void CKamehameha :: KameTouch( CBaseEntity *pOther )
{
	int iClass = pOther->Classify();

	if ( iClass == CLASS_PLAYER_BIOWEAPON || iClass == CLASS_ALIEN_BIOWEAPON )
	{
		ALERT( at_console, "contact with %s, kill it\n", STRING( pOther->pev->classname ) );
		pOther->Killed( pev, GIB_NEVER );
		return;
	}

	if ( pOther->pev->takedamage && iClass > 0 && pOther->pev->health <= 0 )
	{
		ALERT( at_console, "contact with %s, gib it\n", STRING( pOther->pev->classname ) );
		pOther->Killed( pev, GIB_ALWAYS );
		return;
	}

	if ( pOther->pev->takedamage && pOther->CanBeRepulsed( false ) && iClass > 0 && !( pOther->pev->flags & FL_GODMODE )
		&& !( pev->weapons == 1 && m_hEnemy != NULL ) )
	{
		// carry it along
		pev->weapons = 1;
		m_hEnemy = pOther;
		m_hEnemy->pev->solid = SOLID_NOT;
		ALERT( at_console, "contact with %s, push it\n", STRING( pOther->pev->classname ) );
		m_hEnemy->pev->velocity = pev->velocity;
		m_hEnemy->pev->movetype = MOVETYPE_NOCLIP;
		if ( m_hEnemy->IsPlayer() )
			m_hEnemy->pev->flags |= FL_FLY;
		pev->movedir = m_hEnemy->Center() - m_hEnemy->pev->origin;
		UTIL_SetOrigin( m_hEnemy->pev, pev->origin - pev->movedir + pev->velocity.Normalize() * 16 );
		m_hEnemy->pev->nextthink = 0;
		return;
	}

	ALERT( at_console, "contact with %s, blow up\n", STRING( pOther->pev->classname ) );
	EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, "ambience/rocketflame1.wav", 0, 0, SND_STOP, PITCH_NORM );
	EMIT_SOUND_DYN( ENT( pev ), CHAN_WEAPON, "ambience/rocketrumble1.wav", 0, 0, SND_STOP, PITCH_NORM );

	UTIL_ScreenShakeAll( pev->origin, 255, 0.4, 4.0 );
	EMIT_SOUND_DYN( ENT( pev ), CHAN_WEAPON, "weapons/mortarhit.wav", 1.0, ATTN_NONE, 0, PITCH_NORM );

	MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, pev->origin );
		WRITE_BYTE( TE_DLIGHT );
		WRITE_COORD( pev->origin.x );	// X
		WRITE_COORD( pev->origin.y );	// Y
		WRITE_COORD( pev->origin.z );	// Z
		WRITE_BYTE( 100 );		// radius * 0.1
		WRITE_BYTE( 255 );		// r
		WRITE_BYTE( 255 );		// g
		WRITE_BYTE( 128 );		// b
		WRITE_BYTE( 40 );		// time * 10
		WRITE_BYTE( 2 );		// decay * 0.1
	MESSAGE_END( );

	SetThink( NULL );
	SetTouch( NULL );

	entvars_t *pevOwner = pev;
	if ( pev->owner )
		pevOwner = VARS( pev->owner );

	if ( pev->weapons && m_hEnemy != NULL )
	{
		// what it carries is blown apart
		UTIL_SetOrigin( m_hEnemy->pev, pev->origin - ( m_hEnemy->Center() - m_hEnemy->pev->origin ) );
		m_hEnemy->TakeDamage( pev, pevOwner, m_hEnemy->pev->health + 1000, DMG_ENERGYBEAM | DMG_ALWAYSGIB );
	}

	pev->weapons = 1;
	pOther->TakeDamage( pev, pevOwner, 1000, DMG_ENERGYBEAM | DMG_ALWAYSGIB );
	pev->weapons = 0;

	TraceResult tr = UTIL_GetGlobalTrace( );
	pev->owner = ENT( pevOwner );
	pev->dmg = 250;
	Explode( &tr, DMG_BLAST );

	m_flDelay = 2;
	pev->owner = ENT( pevOwner );

	PLAYBACK_EVENT_FULL( 0, ENT( pev ), g_iBlastSphereEvent, 0.0, (float *)&pev->origin, (float *)&pev->angles, m_flDelay, 0.3, pev->dmg * 2, pev->dmg * 2.5, 1, 0 );
	CSoundEnt::InsertSound ( bits_SOUND_DANGER, pev->origin, pev->dmg * 2, 5.0 );

	pev->rendermode = kRenderTransColor;
	pev->renderamt = 0;
	SetThink( HurtThink );
	pev->velocity = g_vecZero;
	pev->movetype = MOVETYPE_NONE;
	pev->nextthink = gpGlobals->time + 0.1;

	m_bitsDamageInflict = DMG_ENERGYBEAM;

	pev->dmgtime = m_flDelay + gpGlobals->time;
	pev->frags = pev->dmg * 2;	// largest radius
	pev->speed = 1;				// radius
	pev->dmg *= 0.25;
}

void CKamehameha :: KameThink( void )
{
	if ( pev->weapons == 1 && m_hEnemy != NULL )
	{
		// keep the burden in front
		FlyThink( );
		m_hEnemy->pev->velocity = pev->velocity;
		UTIL_SetOrigin( m_hEnemy->pev, pev->origin - pev->movedir + pev->velocity.Normalize() * 16 );
	}
	else if ( m_hEnemy != NULL )
		TargetThink( );
	else
		FlyThink( );
}


//=========================================================
// wallop - the "pattern bomb": a pink energy sprite. What it
// hits takes pev->dmg (140) radiation damage and, if it can be
// pushed, flies off at 1400 units/s; it then sits there for two
// seconds zapping lightning around (10 damage in pev->dmg units
// every think). A large one (scale 0.5 and up) also throws a
// blast_shower off the surface it hit.
//=========================================================
class CWallop : public CMagicMissile
{
public:
	void Spawn( void );
	void Precache( void );
	const char *GetKillDesc( void ) { return "a pattern bomb"; }
	const char *GetKillVerb( BOOL fGibbed, int &iHitGroup ) { return DamageVerb( DMG_ENERGYBEAM, TRUE ); }

	void EXPORT WallopThink( void );
	void EXPORT WallopTouch( CBaseEntity *pOther );
	void EXPORT ZapThink( void );
};
LINK_ENTITY_TO_CLASS( wallop, CWallop );

void CWallop :: Spawn( void )
{
	Precache( );

	pev->solid = SOLID_BBOX;
	SET_MODEL( ENT( pev ), "sprites/wallop.spr" );
	pev->rendermode = kRenderTransAdd;
	pev->renderamt = 255;
	pev->movetype = MOVETYPE_FLY;

	m_maxFrame = (float) MODEL_FRAMES( pev->modelindex ) - 1;
	pev->framerate = 10;
	TurnOn( );

	SetThink( WallopThink );
	SetTouch( WallopTouch );
	pev->nextthink = gpGlobals->time;

	pev->scale = 0.25;
	pev->speed = 140;
	pev->dmg = 140;
	pev->dmgtime = 0;

	UTIL_SetSize( pev, g_vecZero, g_vecZero );
}

void CWallop :: Precache( void )
{
	PRECACHE_MODEL( "sprites/wallop.spr" );
	PRECACHE_SOUND( "doors/aliendoor1.wav" );
}

void CWallop :: WallopThink( void )
{
	// a large one zaps while it flies
	if ( pev->scale < 0.5 )
		AnimateThink( );
	else
		ZapThink( );
}

void CWallop :: WallopTouch( CBaseEntity *pOther )
{
	SetTouch( NULL );
	SetThink( NULL );

	TraceResult tr = UTIL_GetGlobalTrace( );

	if ( pOther->pev->takedamage )
	{
		entvars_t *pevOwner = VARS( pev->owner );

		ClearMultiDamage( );
		pOther->TraceAttack( pevOwner, pev->dmg, pev->velocity.Normalize(), &tr, DMG_NOGIB | DMG_RADIATION );
		if ( pOther->CanBeRepulsed( pev->scale >= 0.5 ) )
		{
			pOther->pev->velocity = pev->velocity.Normalize() * 1400 + Vector( 0, 0, 200 );
		}
		ApplyMultiDamage( pev, pevOwner );

		if ( pOther->Classify() == CLASS_HUMAN_PASSIVE )
		{
			pOther->pev->rendercolor = Vector( 255, 0, 128 );
		}
		else if ( FBitSet( pOther->pev->flags, FL_MONSTER ) && !pOther->IsBSPModel() && pOther->pev->health <= 0 )
		{
			pOther->pev->renderfx = kRenderFxGlowShell;
			pOther->pev->rendercolor = Vector( 255, 0, 128 );
			if ( pOther->pev->rendermode == kRenderNormal )
				pOther->pev->renderamt = 0;
		}
	}
	else if ( !ENTINDEX( pOther->edict() ) )
	{
		// the world: vanish into the sky
		Vector vecEnd = pev->origin + pev->velocity;
		const char *pTextureName = TRACE_TEXTURE( ENT( pOther->pev ), pev->origin, vecEnd );
		if ( pTextureName && !strcmp( pTextureName, "sky" ) )
		{
			SetThink( &CBaseEntity::SUB_Remove );
			return;
		}
	}

	if ( pOther->IsBSPModel() )
		UTIL_DecalTrace( &tr, DECAL_SCORCH1 + RANDOM_LONG( 0, 1 ) );

	EMIT_SOUND_DYN( ENT( pev ), CHAN_WEAPON, "doors/aliendoor1.wav", 1.0, 0.8, 0, PITCH_NORM );
	CSoundEnt::InsertSound( bits_SOUND_COMBAT, pev->origin, 1024, 2.0 );

	if ( pOther->Shielded() )
	{
		SetThink( &CBaseEntity::SUB_Remove );
		return;
	}

	// zap around for two seconds
	pev->dmgtime = gpGlobals->time + 2;
	SetThink( ZapThink );

	if ( pev->scale >= 0.5 )
	{
		Vector vecDir = pev->velocity.Normalize();

		if ( pOther->IsBSPModel() )
		{
			// off the wall
			float n = -DotProduct( tr.vecPlaneNormal, vecDir );
			vecDir = 2.0 * tr.vecPlaneNormal * n + vecDir;
		}
		else
			vecDir = pev->velocity.Normalize();

		CBaseEntity *pShower = CBaseEntity::Create( "blast_shower", pev->origin + vecDir * 8, g_vecZero, pev->owner );
		pShower->pev->velocity = vecDir * 450;
		pShower->pev->dmgtime = gpGlobals->time + 1;
		pShower->pev->speed = 0.2;
		pShower->pev->dmg = 100;
		pShower->pev->enemy = pev->owner;
		pShower->m_iszKillDesc = MAKE_STRING( "an explosion wave" );
	}

	pev->velocity = g_vecZero;
	pev->speed = pev->dmg;	// ZapThink's radius
}

void CWallop :: ZapThink( void )
{
	AnimateThink( );

	entvars_t *pevOwner = VARS( pev->owner );
	::RadiusDamage( pev->origin, pev, pevOwner, 10, pev->speed, CLASS_NONE, DMG_BLAST | DMG_NEVERGIB );

	// up to three bolts a think
	int c = ( 1 / gpGlobals->frametime ) / 3;
	if ( c > 3 )
		c = 3;

	for ( int i = 0; i < c; i++ )
	{
		Vector vecAim = Vector( RANDOM_FLOAT( -1, 1 ), RANDOM_FLOAT( -1, 1 ), RANDOM_FLOAT( -1, 1 ) ).Normalize();

		TraceResult tr;
		UTIL_TraceLine( pev->origin, pev->origin + vecAim * pev->speed * 2, dont_ignore_monsters, ENT( pev ), &tr );

		if ( tr.flFraction > 0.2 )
		{
			MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, pev->origin );
				WRITE_BYTE( TE_BEAMENTPOINT );
				WRITE_SHORT( entindex() );
				WRITE_COORD( tr.vecEndPos.x );
				WRITE_COORD( tr.vecEndPos.y );
				WRITE_COORD( tr.vecEndPos.z );
				WRITE_SHORT( g_sModelIndexLightning );
				WRITE_BYTE( 0 ); // frame start
				WRITE_BYTE( 10 ); // framerate
				WRITE_BYTE( RANDOM_LONG( 2, 4 ) ); // life
				WRITE_BYTE( 20 );  // width
				WRITE_BYTE( 36 );   // noise
				WRITE_BYTE( 255 );   // r, g, b
				WRITE_BYTE( 0 );   // r, g, b
				WRITE_BYTE( 128 );   // r, g, b
				WRITE_BYTE( 255 );	// brightness
				WRITE_BYTE( 35 );		// speed
			MESSAGE_END();
		}
	}

	if ( pev->dmgtime != 0 && pev->dmgtime < gpGlobals->time )
		SetThink( &CBaseEntity::SUB_Remove );
}


//=========================================================
// vortex_mine - a glowing grenade that bounces about for
// three seconds, then opens a vortex_blast of its damage
// (300) where it lies.
//=========================================================
class CVortexMine : public CGrenade
{
public:
	void Spawn( void );
	void Precache( void );

	void EXPORT VortexThink( void );
};
LINK_ENTITY_TO_CLASS( vortex_mine, CVortexMine );

void CVortexMine :: Spawn( void )
{
	pev->movetype = MOVETYPE_TOSS;
	pev->solid = SOLID_BBOX;

	SET_MODEL( ENT( pev ), "models/grenade.mdl" );
	UTIL_SetSize( pev, Vector( 0, 0, 0 ), Vector( 0, 0, 0 ) );

	pev->dmg = 300;
	m_fRegisteredSound = FALSE;

	pev->rendercolor = Vector( 255, 0, 128 );
	pev->renderfx = kRenderFxGlowShell;
	pev->gravity = 0.5;

	SetThink( VortexThink );
	pev->nextthink = gpGlobals->time + 3;
	SetTouch( BounceTouch );
}

void CVortexMine :: Precache( void )
{
	PRECACHE_MODEL( "models/grenade.mdl" );
	UTIL_PrecacheOther( "vortex_blast" );
}

void CVortexMine :: VortexThink( void )
{
	CBaseEntity *pBlast = CBaseEntity::Create( "vortex_blast", pev->origin, pev->angles, pev->owner );
	pBlast->pev->dmg = pev->dmg;
	pBlast->pev->movedir = Vector( 0, 0, -32 );
	pBlast->pev->scale = pev->dmg / 250;

	UTIL_Remove( this );
}


//=========================================================
// slip_stream - a small red homing sprite rocket: 75 damage
// on contact, then it is gone.
//=========================================================
class CSlipStream : public CRocket	// name ours
{
public:
	void Spawn( void );
	void Precache( void );

	void SlipTouch( CBaseEntity *pOther );	// name ours

	int m_iTrail;	// name ours
};
LINK_ENTITY_TO_CLASS( slip_stream, CSlipStream );

void CSlipStream :: Spawn( void )
{
	Precache( );

	pev->movetype = MOVETYPE_FLY;
	pev->solid = SOLID_BBOX;
	pev->rendermode = kRenderTransAdd;
	pev->renderamt = 0;

	SET_MODEL( ENT( pev ), "sprites/lred.spr" );
	UTIL_SetSize( pev, Vector( 0, 0, 0 ), Vector( 0, 0, 0 ) );
	UTIL_SetOrigin( pev, pev->origin );

	SetThink( TargetThink );
	SetTouch( SlipTouch );

	pev->speed = 1000;
	m_flFieldOfView = VIEW_FIELD_FULL;
	pev->nextthink = gpGlobals->time + 0.1;
	m_flSeekTime = RANDOM_FLOAT( 0.5, 1 ) + gpGlobals->time;
	pev->dmg = 75;

	BeamFollow( m_iTrail, 20, 3, Vector( 255, 255, 255 ), 255 );
}

void CSlipStream :: Precache( void )
{
	m_iTrail = PRECACHE_MODEL( "sprites/lred.spr" );
}

void CSlipStream :: SlipTouch( CBaseEntity *pOther )
{
	SetThink( NULL );
	SetTouch( NULL );

	TraceResult tr = UTIL_GetGlobalTrace( );

	if ( pOther->pev->takedamage )
	{
		entvars_t *pevOwner = VARS( pev->owner );

		ClearMultiDamage( );
		pOther->TraceAttack( pevOwner, pev->dmg, pev->velocity.Normalize(), &tr, DMG_NOGIB | DMG_ENERGYBEAM );
		ApplyMultiDamage( pev, pevOwner );
	}

	if ( pOther->IsBSPModel() )
		UTIL_GunshotDecalTrace( &tr, DECAL_GUNSHOT1 + RANDOM_LONG( 0, 4 ) );

	UTIL_Remove( this );
}


//=========================================================
// powerbeam - the beam an owner (pev->owner, m_hLinked)
// fires from its attachment 0 through this entity's origin
// until pev->dmgtime: 30 energy damage a tenth of a second,
// sparks, and it burns through up to four walls. The Nephilim
// agent moves it onto its enemy every shot.
//=========================================================
class CPowerBeam : public CBaseEntity
{
public:
	void Spawn( void );
	void Precache( void );
	const char *GetKillDesc( void ) { return "a powerbeam attack"; }

	int		Save( CSave &save );
	int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	void EXPORT WeaponThink( void );

	EHANDLE m_hLinked;
	CBeam *m_pBeam;				// name ours
	CBaseEntity *m_pSprite;		// name ours: the agent's charge sprite, removed with the beam
};
LINK_ENTITY_TO_CLASS( powerbeam, CPowerBeam );

TYPEDESCRIPTION	CPowerBeam::m_SaveData[] = 
{
	DEFINE_FIELD( CPowerBeam, m_hLinked, FIELD_EHANDLE ),
};

IMPLEMENT_SAVERESTORE( CPowerBeam, CBaseEntity );

void CPowerBeam :: Spawn( void )
{
	Precache( );

	if ( !pev->owner )
	{
		UTIL_Remove( this );
		return;
	}

	m_hLinked = CBaseEntity::Instance( pev->owner );

	pev->movetype = MOVETYPE_NOCLIP;
	pev->solid = SOLID_NOT;
	pev->effects = EF_NODRAW;

	SetThink( WeaponThink );
	pev->nextthink = gpGlobals->time + 0.1;
}

void CPowerBeam :: Precache( void )
{
	PRECACHE_MODEL( "sprites/nbeam.spr" );
}

void CPowerBeam :: WeaponThink( void )
{
	if ( m_hLinked == NULL || pev->dmgtime < gpGlobals->time )
	{
		UTIL_Remove( m_pSprite );
		UTIL_Remove( m_pBeam );
		UTIL_Remove( this );
		return;
	}

	pev->nextthink = gpGlobals->time + 0.1;

	if ( !m_pBeam )
	{
		m_pBeam = CBeam::BeamCreate( "sprites/nbeam.spr", 80 );
		m_pBeam->PointEntInit( pev->origin, ENTINDEX( m_hLinked->edict() ) );
		m_pBeam->SetEndAttachment( 1 );
		m_pBeam->SetBrightness( 255 );
		m_pBeam->SetColor( 255, 255, 255 );
		m_pBeam->SetScrollRate( 35 );
		m_pBeam->pev->spawnflags |= SF_BEAM_TEMPORARY;
	}

	Vector vecSrc, vecDir;
	edict_t *pentIgnore = m_hLinked->edict();

	GET_ATTACHMENT( pentIgnore, 0, vecSrc, vecDir );
	vecDir = ( pev->origin - vecSrc ).Normalize();

	for ( int i = 0; i < 5; i++ )
	{
		Vector vecDest = vecSrc + vecDir * 4096;
		TraceResult tr, beam_tr;

		UTIL_TraceLine( vecSrc, vecDest, dont_ignore_monsters, pentIgnore, &tr );
		if ( tr.fAllSolid )
			return;

		UTIL_Sparks( tr.vecEndPos );
		m_pBeam->SetStartPos( tr.vecEndPos );

		CBaseEntity *pEntity = CBaseEntity::Instance( tr.pHit );
		if ( pEntity == NULL )
			return;

		if ( pEntity->IsBSPModel() )
			UTIL_DecalTrace( &tr, DECAL_BIGSHOT1 + RANDOM_LONG( 0, 4 ) );

		if ( pEntity->pev->takedamage )
		{
			ClearMultiDamage( );
			pEntity->TraceAttack( m_hLinked->pev, 30, vecDir, &tr, DMG_ENERGYBEAM );
			ApplyMultiDamage( pev, m_hLinked->pev );
		}

		if ( pEntity->Shielded() )
			return;

		if ( pEntity->IsBSPModel() )
		{
			// burn through the wall
			UTIL_TraceLine( tr.vecEndPos + vecDir * 8, vecDest, ignore_monsters, pentIgnore, &beam_tr );
			if ( beam_tr.fAllSolid )
				return;

			// trace backwards to find exit point
			UTIL_TraceLine( beam_tr.vecEndPos, tr.vecEndPos, ignore_monsters, pentIgnore, &beam_tr );
			UTIL_DecalTrace( &tr, DECAL_BIGSHOT1 + RANDOM_LONG( 0, 4 ) );	// sic: the entry point again
			UTIL_Sparks( tr.vecEndPos );

			vecSrc = beam_tr.vecEndPos;
			pentIgnore = NULL;
		}
		else
		{
			vecSrc = tr.vecEndPos;
			pentIgnore = tr.pHit;
		}
	}
}


//=========================================================
// monster_na_agent - the Nephilim Alliance agent: an
// assassin with an energy shield (armorvalue 2000, regrows
// a quarter point a think, flashes pink when hit) who blinks
// from node to node. Primary attacks (pev->frags): 0 the
// wallop (charged: a slow big one), 1 a laser pulse (charged:
// the power wave), 2 a slip stream (charged: the power
// beam); secondary (pev->weapons bits): 1 a vortex mine,
// 2 an energy disk, 4 a flux rocket.
//=========================================================

//=========================================================
// monster-specific schedule types
//=========================================================
enum
{
	SCHED_AGENT_BLINK_COVER_FROM_ENEMY = SCHED_ASSASSIN_JUMP_LAND + 1,	// names ours
	SCHED_AGENT_BLINK_COVER_FROM_SOUND,
	SCHED_AGENT_BLINK_ATTACK,
	SCHED_AGENT_BLINK_NPW_COMBO,
};

//=========================================================
// monster-specific tasks
//=========================================================
enum
{
	TASK_AGENT_BLINK = TASK_ASSASSIN_FALL_TO_GROUND + 1,	// names ours: teleport to m_vecMoveGoal
	TASK_AGENT_FIND_COVER_FROM_ENEMY,
	TASK_AGENT_FIND_COVER_FROM_SOUND,
	TASK_AGENT_FIND_NODE_NEAR_ENEMY,
	TASK_AGENT_FIND_NODE_AWAY_FROM_ENEMY,
};

class CNAgent : public CHAssassin
{
public:
	void Spawn( void );
	void Precache( void );
	int Shielded( void ) { if ( pev->armorvalue > 0 ) return 2; return 0; }
	BOOL CanBeTractored( void ) { return !Shielded(); }
	BOOL CanBeRepulsed( bool fForce ) { if ( Shielded() && !fForce ) return FALSE; return TRUE; }
	const char *GetHudName( void ) { return "Classified, NA-8"; }
	int HeatSignature( void ) { return 0; }
	const char *GetKillDesc( void ) { return "a Nephilim Alliance agent"; }
	int Classify( void ) { if ( m_iMyClass ) return m_iMyClass; return 14; }	// the Nephilim Alliance (no CLASS_ define)
	void EXPORT StopTouch( CBaseEntity *pOther );
	int GetBattlePower( void );

	void PowerWave( void );		// names ours
	void PowerBeam( void );
	void Shoot( void );
	void HandleAnimEvent( MonsterEvent_t *pEvent );
	void RunAI( void );
	void StartTask( Task_t *pTask );
	void RunTask( Task_t *pTask );
	Schedule_t *GetSchedule( void );
	Schedule_t *GetScheduleOfType( int Type );
	BOOL CheckRangeAttack1( float flDot, float flDist );
	BOOL CheckRangeAttack2( float flDot, float flDist );
	BOOL FCanCheckAttacks( void );
	int TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );
	void Killed( entvars_t *pevAttacker, int iGib );
	BOOL CheckJump( void );		// name ours
	CUSTOM_SCHEDULES;

	CSprite	*m_pChargeSprite;	// names ours; not saved
	EHANDLE	m_hPowerBeam;
	float	m_flChargeTime;		// charging since; 0 when charged
	float	m_flChargeScale;	// the full charge's sprite scale
	float	m_flWeaponTime;		// when pev->frags last changed
};
LINK_ENTITY_TO_CLASS( monster_na_agent, CNAgent );

//=========================================================
// StopTouch - recoil from a power wave fired in the air
// ends at the first thing it touches
//=========================================================
void CNAgent :: StopTouch( CBaseEntity *pOther )
{
	pev->velocity = g_vecZero;
	SetTouch( NULL );
}

//=========================================================
// GetBattlePower - the target ID's rating of this monster
//=========================================================
int CNAgent :: GetBattlePower( void )
{
	float flPower = 2747083.25f;	// sic: a constant
	if ( Shielded() )
		flPower += 1000;
	return flPower;
}

//=========================================================
// PowerWave - fire the charged power wave; in the air the
// recoil throws the agent back along a beam
//=========================================================
void CNAgent :: PowerWave( void )
{
	Vector vecSrc, vecDir;

	GetAttachment( 0, vecSrc, vecDir );
	UTIL_MakeVectors( pev->angles );
	vecDir = ShootAtEnemy( vecSrc );

	CBaseMonster *pWave = (CBaseMonster *)CBaseEntity::Create( "powerwave", vecSrc, UTIL_VecToAngles( vecDir ), edict() );
	pWave->m_hEnemy = m_hEnemy;
	pWave->pev->speed = 1000;

	pev->nextthink = gpGlobals->time + 6;
	pev->animtime = pev->nextthink;
	m_pChargeSprite->pev->scale = 0.7;
	m_cAmmoLoaded = 0;

	if ( pev->movetype == MOVETYPE_TOSS )
	{
		SetTouch( StopTouch );
		pev->movetype = MOVETYPE_FLY;
		pev->velocity = vecDir * -256;

		CBeam *pBeam = CBeam::BeamCreate( "sprites/LRed.spr", 100 );
		pBeam->EntsInit( entindex(), pWave->entindex() );
		pBeam->SetStartAttachment( 1 );
		pBeam->SetThink( &CBaseEntity::SUB_Remove );
		pBeam->pev->nextthink = gpGlobals->time + 6;
	}
}

//=========================================================
// PowerBeam - start or aim the power beam at the enemy
//=========================================================
void CNAgent :: PowerBeam( void )
{
	if ( m_hEnemy == NULL )
		return;

	if ( m_hPowerBeam == NULL )
	{
		CPowerBeam *pBeam = (CPowerBeam *)CBaseEntity::Create( "powerbeam", m_hEnemy->BodyTarget( pev->origin ), g_vecZero, edict() );
		m_hPowerBeam = pBeam;
		m_hPowerBeam->pev->dmgtime = gpGlobals->time + 1;
		pBeam->m_pSprite = m_pChargeSprite;	// the beam removes the charge sprite
		m_pChargeSprite = NULL;
	}
	else
	{
		m_hPowerBeam->pev->origin = m_hEnemy->BodyTarget( pev->origin );
		m_hPowerBeam->pev->dmgtime = gpGlobals->time + 1;
	}
}

//=========================================================
// Shoot - the primary attack
//=========================================================
void CNAgent :: Shoot( void )
{
	if ( m_hPowerBeam != NULL || ( m_pChargeSprite && pev->frags == 2 ) )
	{
		PowerBeam( );
		return;
	}

	if ( m_pChargeSprite && pev->frags == 1 )
	{
		PowerWave( );
		return;
	}

	Vector vecShootOrigin, vecShootDir;

	GetAttachment( 0, vecShootOrigin, vecShootDir );
	UTIL_MakeVectors( pev->angles );
	vecShootDir = ShootAtEnemy( vecShootOrigin );

	if ( pev->frags == 2 || m_flLastShot + 2 < gpGlobals->time )
	{
		m_flDiviation = 0.10;
	}
	else
	{
		m_flDiviation -= 0.01;
		if (m_flDiviation < 0.02)
			m_flDiviation = 0.02;
	}
	m_flLastShot = gpGlobals->time;

	EMIT_SOUND( ENT( pev ), CHAN_WEAPON, "x/x_shoot1.wav", RANDOM_FLOAT( 0.6, 0.8 ), ATTN_NORM );

	float x, y, z;
	do {
		x = RANDOM_FLOAT( -0.5, 0.5 ) + RANDOM_FLOAT( -0.5, 0.5 );
		y = RANDOM_FLOAT( -0.5, 0.5 ) + RANDOM_FLOAT( -0.5, 0.5 );
		z = x * x + y * y;
	} while ( z > 1 );

	Vector vecDir = vecShootDir + x * m_flDiviation * gpGlobals->v_right + y * m_flDiviation * gpGlobals->v_up;

	CBaseEntity *pShot;
	if ( pev->frags == 0 )
	{
		pShot = CBaseEntity::Create( "wallop", vecShootOrigin, g_vecZero, edict() );
		pShot->pev->velocity = vecDir * 900;
	}
	else if ( pev->frags == 1 )
	{
		pShot = CLaserPulse::Shoot( vecShootOrigin, edict(), Vector( 255, 255, 255 ), 4, 0.25, 4 );
		pShot->pev->velocity = vecDir * 1400;
		pShot->pev->spawnflags |= 0x10c;
		pShot->pev->movedir = Vector( 255, 16, 128 );
		pShot->pev->scale = 0.5;
		pShot->pev->dmg = 100;
		pShot->m_iszKillDesc = MAKE_STRING( "an energy wave" );
	}
	else
	{
		pShot = CBaseEntity::Create( "slip_stream", vecShootOrigin, UTIL_VecToAngles( vecDir ), edict() );
		pShot->pev->velocity = vecDir * 1000;
		pShot->m_iszKillDesc = MAKE_STRING( "a slip stream" );
		CBaseMonster *pStream = pShot->MyMonsterPointer();
		pStream->m_hEnemy = m_hEnemy;
		pStream->m_vecEnemyLKP = m_vecEnemyLKP;
	}

	CSoundEnt::InsertSound( bits_SOUND_COMBAT, vecShootOrigin, 600, 0.3 );

	pev->effects |= EF_MUZZLEFLASH;

	Vector angDir = UTIL_VecToAngles( vecShootDir );
	SetBlending( 0, angDir.x );

	if ( m_pChargeSprite && pev->frags == 0 )
	{
		// a charged wallop: slower, bigger and stronger
		pShot->pev->velocity = vecShootDir * 700;
		pShot->pev->dmg = m_pChargeSprite->pev->scale * pShot->pev->dmg * 4;
		pShot->pev->scale = m_pChargeSprite->pev->scale;
		UTIL_Remove( m_pChargeSprite );
		m_pChargeSprite = NULL;
		m_cAmmoLoaded = 0;
	}
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CNAgent :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	switch( pEvent->event )
	{
	case ASSASSIN_AE_SHOOT1:
		if ( m_cAmmoLoaded )
		{
			if ( m_flChargeTime != 0 )
			{
				// still charging: keep aiming
				Vector vecSrc, vecDir;
				GetAttachment( 0, vecSrc, vecDir );
				UTIL_MakeVectors( pev->angles );
				vecDir = ShootAtEnemy( vecSrc );

				Vector angDir = UTIL_VecToAngles( vecDir );
				SetBlending( 0, angDir.x );

				m_flChargeTime -= 0.1;
				pev->framerate = 0;
			}
			else
				Shoot( );
		}
		break;
	case ASSASSIN_AE_TOSS1:
		{
			if ( m_pChargeSprite )
			{
				UTIL_Remove( m_pChargeSprite );
				m_pChargeSprite = NULL;
			}

			if ( !pev->weapons )
			{
				ALERT( at_console, "secondary attack with no possible attack types!\n" );
				return;
			}

			UTIL_MakeVectors( pev->angles );

			if ( m_hEnemy != NULL )
			{
				if ( ( pev->weapons & 4 ) && m_hEnemy->pev->movetype == MOVETYPE_FLY )
					pev->weapons = 4;
				if ( ( pev->weapons & 1 ) && m_hEnemy->pev->size.z < 32 && ( m_hEnemy->pev->flags & FL_ONGROUND ) )
					pev->weapons = 1;
			}

			// pick one of the possible attacks
			if ( pev->weapons == 7 )
			{
				pev->weapons = RANDOM_LONG( 0, 2 );
				pev->weapons += max( 1, pev->weapons );
			}
			else if ( pev->weapons == 3 )
				pev->weapons = RANDOM_LONG( 1, 2 );
			else if ( pev->weapons == 6 )
				pev->weapons = RANDOM_LONG( 1, 2 ) * 2;

			if ( pev->weapons == 1 )
			{
				CBaseEntity *pMine = CBaseEntity::Create( "vortex_mine", pev->origin + gpGlobals->v_forward * 34 + Vector( 0, 0, 32 ), g_vecZero, edict() );
				pMine->pev->velocity = m_vecTossVelocity;
				m_flNextGrenadeCheck = gpGlobals->time + 6;
				m_fThrowGrenade = FALSE;
			}
			else if ( pev->weapons == 4 )
			{
				Vector vecSrc, vecDir;
				GetAttachment( 1, vecSrc, vecDir );
				vecDir = ShootAtEnemy( vecSrc );

				CRocket *pRocket = CRocket::CreateRocket( vecSrc, UTIL_VecToAngles( vecDir ), edict(), Vector( 255, 0, 128 ), MAKE_STRING( "sprites/flux_t.spr" ) );
				if ( pRocket )
				{
					pRocket->pev->scale = 0.7;
					pRocket->SetThink( &CRocket::TargetThink );
					pRocket->pev->speed = 1000;
					pRocket->pev->renderamt = 255;
					pRocket->pev->rendermode = kRenderTransAdd;
					pRocket->pev->velocity = vecDir * 256 + Vector( 0, 0, 128 );
					pRocket->m_hEnemy = m_hEnemy;
					pRocket->pev->dmg = 200;
					pRocket->pev->spawnflags |= 8;
					pRocket->m_iszKillDesc = MAKE_STRING( "a powerball attack" );
				}
				m_flNextGrenadeCheck = gpGlobals->time + 6;
				m_fThrowGrenade = FALSE;
			}
			else
			{
				Vector vecSrc, vecAngles;
				GetAttachment( 1, vecSrc, vecAngles );

				CBaseMonster *pDisk = (CBaseMonster *)CBaseEntity::Create( "energy_disk", vecSrc, pev->angles, edict() );
				pDisk->pev->speed = 512;
				pDisk->pev->body = 1;
				pDisk->m_hEnemy = m_hEnemy;
				pDisk->m_hActivator = this;
				pDisk->pev->nextthink += 0.4;
				pDisk->m_iszKillDesc = MAKE_STRING( "a ringwave attack" );
				m_flNextGrenadeCheck = gpGlobals->time + 6;
				m_fThrowGrenade = FALSE;
			}
		}
		break;
	case ASSASSIN_AE_JUMP:
		{
			UTIL_MakeAimVectors( pev->angles );
			pev->movetype = MOVETYPE_TOSS;
			pev->flags &= ~FL_ONGROUND;
			pev->velocity = m_vecJumpVelocity;
			m_flNextJump = gpGlobals->time + 4.0;
		}
		return;
	default:
		CHAssassin::HandleAnimEvent( pEvent );
		break;
	}
}

//=========================================================
// Spawn
//=========================================================
void CNAgent :: Spawn()
{
	Precache( );

	SET_MODEL(ENT(pev), "models/nassassin.mdl");
	UTIL_SetSize(pev, Vector( -16, -16, 0 ), Vector( 16, 16, 72 ) );

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= BLOOD_COLOR_RED;
	pev->effects		= 0;
	pev->health			= 500;
	pev->armorvalue		= 2000;	// the shield
	m_MonsterState		= MONSTERSTATE_NONE;
	m_afCapability		= bits_CAP_MELEE_ATTACK1 | bits_CAP_DOORS_GROUP;
	pev->friction		= 1;

	m_HackedGunPos		= Vector( 0, 24, 48 );

	pev->frags			= RANDOM_LONG( 0, 2 );	// the primary attack
	m_cAmmoLoaded		= 1;

	MonsterInit();

	m_flDistLook		= 4096;
	m_flFieldOfView		= VIEW_FIELD_FULL;
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CNAgent :: Precache()
{
	PRECACHE_MODEL("models/nassassin.mdl");

	PRECACHE_SOUND("x/x_shoot1.wav");
	PRECACHE_SOUND("houndeye/he_blast1.wav");

	m_iShell = PRECACHE_MODEL("sprites/redflare2.spr");	// the blink's spray
	PRECACHE_MODEL("sprites/n_charge.spr");
	PRECACHE_MODEL("sprites/flux_t.spr");

	UTIL_PrecacheOther( "wallop" );
	UTIL_PrecacheOther( "laser_pulse" );
	CLaserPulse::PrecacheColor( 4 );
	UTIL_PrecacheOther( "powerwave" );
	UTIL_PrecacheOther( "powerbeam" );
	UTIL_PrecacheOther( "energy_disk" );
	UTIL_PrecacheOther( "vortex_mine" );
	UTIL_PrecacheOther( "rocket" );
}

//=========================================================
// AI Schedules Specific to this monster
//=========================================================

//=========================================================
// blink into cover
//=========================================================
Task_t	tlAgentBlinkCoverFromEnemy[] =
{
	{ TASK_STOP_MOVING,					(float)0					},
	{ TASK_WAIT,						(float)0.2					},
	{ TASK_SET_FAIL_SCHEDULE,			(float)SCHED_TAKE_COVER_FROM_ENEMY	},
	{ TASK_AGENT_FIND_COVER_FROM_ENEMY,	(float)0					},
	{ TASK_AGENT_BLINK,					(float)0					},
	{ TASK_REMEMBER,					(float)bits_MEMORY_INCOVER	},
	{ TASK_FACE_ENEMY,					(float)0					},
};

Schedule_t	slAgentBlinkCoverFromEnemy[] =
{
	{ 
		tlAgentBlinkCoverFromEnemy,
		ARRAYSIZE ( tlAgentBlinkCoverFromEnemy ), 
		bits_COND_NEW_ENEMY |
		bits_COND_CAN_MELEE_ATTACK1		|
		bits_COND_HEAR_SOUND,
		
		bits_SOUND_DANGER,
		"AgentBlinkCoverFromEnemy"
	},
};

//=========================================================
// blink away from a dangerous sound
//=========================================================
Task_t	tlAgentBlinkCoverFromSound[] =
{
	{ TASK_SET_FAIL_SCHEDULE,			(float)SCHED_TAKE_COVER_FROM_BEST_SOUND	},
	{ TASK_STOP_MOVING,					(float)0					},
	{ TASK_AGENT_FIND_COVER_FROM_SOUND,	(float)0					},
	{ TASK_AGENT_BLINK,					(float)0					},
	{ TASK_REMEMBER,					(float)bits_MEMORY_INCOVER	},
	{ TASK_TURN_LEFT,					(float)179					},
};

Schedule_t	slAgentBlinkCoverFromSound[] =
{
	{ 
		tlAgentBlinkCoverFromSound,
		ARRAYSIZE ( tlAgentBlinkCoverFromSound ), 
		bits_COND_NEW_ENEMY,
		0,
		"AgentBlinkCoverFromSound"
	},
};

//=========================================================
// blink near the enemy and shoot
//=========================================================
Task_t	tlAgentBlinkAttack[] =
{
	{ TASK_SET_FAIL_SCHEDULE,			(float)SCHED_CHASE_ENEMY	},
	{ TASK_AGENT_FIND_NODE_NEAR_ENEMY,	(float)0					},
	{ TASK_AGENT_BLINK,					(float)0					},
	{ TASK_SET_SCHEDULE,				(float)SCHED_RANGE_ATTACK1	},
};

Schedule_t	slAgentBlinkAttack[] =
{
	{ 
		tlAgentBlinkAttack,
		ARRAYSIZE ( tlAgentBlinkAttack ), 
		bits_COND_NEW_ENEMY |
		bits_COND_CAN_RANGE_ATTACK1		|
		bits_COND_HEAR_SOUND,
		
		bits_SOUND_DANGER,
		"AgentBlinkAttack"
	},
};

//=========================================================
// blink around the enemy, dropping vortex mines
//=========================================================
Task_t	tlMineField[] =
{
	{ TASK_SET_FAIL_SCHEDULE,			(float)SCHED_CHASE_ENEMY	},
	{ TASK_AGENT_FIND_NODE_NEAR_ENEMY,	(float)0					},
	{ TASK_AGENT_BLINK,					(float)0					},
	{ TASK_SPECIAL_ATTACK1,				(float)0					},
	{ TASK_AGENT_FIND_NODE_NEAR_ENEMY,	(float)256					},
	{ TASK_AGENT_BLINK,					(float)0					},
	{ TASK_SPECIAL_ATTACK1,				(float)0					},
	{ TASK_AGENT_FIND_NODE_NEAR_ENEMY,	(float)256					},
	{ TASK_AGENT_BLINK,					(float)0					},
	{ TASK_SPECIAL_ATTACK1,				(float)0					},
	{ TASK_AGENT_FIND_NODE_NEAR_ENEMY,	(float)256					},
	{ TASK_AGENT_BLINK,					(float)0					},
	{ TASK_SPECIAL_ATTACK1,				(float)0					},
	{ TASK_AGENT_FIND_NODE_NEAR_ENEMY,	(float)256					},
	{ TASK_AGENT_BLINK,					(float)0					},
	{ TASK_SPECIAL_ATTACK1,				(float)0					},
	{ TASK_SET_SCHEDULE,				(float)SCHED_AGENT_BLINK_COVER_FROM_SOUND	},
};

Schedule_t	slMineField[] =
{
	{ 
		tlMineField,
		ARRAYSIZE ( tlMineField ), 
		bits_COND_ENEMY_DEAD,
		0,
		"Mine Field"
	},
};

//=========================================================
// blink away from the enemy and fire the power wave
//=========================================================
Task_t	tlBlinkNPWCombo[] =
{
	{ TASK_SET_FAIL_SCHEDULE,			(float)SCHED_MELEE_ATTACK1	},
	{ TASK_AGENT_FIND_NODE_AWAY_FROM_ENEMY,	(float)600				},
	{ TASK_AGENT_BLINK,					(float)0					},
	{ TASK_RANGE_ATTACK1,				(float)0					},
};

Schedule_t	slBlinkNPWCombo[] =
{
	{ 
		tlBlinkNPWCombo,
		ARRAYSIZE ( tlBlinkNPWCombo ), 
		bits_COND_ENEMY_DEAD,
		0,
		"BlinkNPWCombo"
	},
};

DEFINE_CUSTOM_SCHEDULES( CNAgent )
{
	slAgentBlinkCoverFromEnemy,
	slAgentBlinkCoverFromSound,
	slAgentBlinkAttack,
	slMineField,
	slBlinkNPWCombo,
};

IMPLEMENT_CUSTOM_SCHEDULES( CNAgent, CHAssassin );

//=========================================================
// RunAI - the charge, the shield and the footsteps
//=========================================================
void CNAgent :: RunAI( void )
{
	if ( pev->movetype == MOVETYPE_FLY )
	{
		// the power wave's recoil is over
		pev->movetype = MOVETYPE_TOSS;
		SetTouch( NULL );
	}

	CBaseMonster :: RunAI();

	if ( m_flChargeTime != 0 && m_pChargeSprite )
	{
		if ( pev->weapons == 0 )
			m_pChargeSprite->pev->scale = ( gpGlobals->time - m_flChargeTime ) * m_flChargeScale;
		else
			m_pChargeSprite->pev->scale = ( gpGlobals->time - m_flChargeTime ) * m_flChargeScale * 0.5;

		if ( m_pChargeSprite->pev->scale < 0.1 )
			m_pChargeSprite->pev->scale = 0.1;

		if ( m_pChargeSprite->pev->scale >= m_flChargeScale )
		{
			// charged
			pev->framerate = 1;
			m_flChargeTime = 0;
			m_pChargeSprite->pev->scale = m_flChargeScale;
		}
	}
	else if ( !m_cAmmoLoaded && m_pChargeSprite )
	{
		UTIL_Remove( m_pChargeSprite );
		m_pChargeSprite = NULL;
	}

	if ( pev->armortype > 0 )
	{
		// the shield is down for a while
		pev->renderfx = kRenderFxNone;
		pev->armortype -= 0.1;
		if ( pev->armortype <= 0 )
		{
			pev->armorvalue = 0;
			pev->renderfx = kRenderFxGlowShell;
			pev->renderamt = 255;
			pev->rendercolor = Vector( 255, 0, 128 );
		}
	}
	else if ( pev->armorvalue < 2000 )
		pev->armorvalue += 0.25;

	if ( pev->health < 500 )
		pev->health += 0.1;

	if ( pev->renderfx == kRenderFxGlowShell )
	{
		// the shield's flash fades
		pev->renderamt -= 10;
		if ( pev->renderamt < 0 )
		{
			pev->renderfx = kRenderFxNone;
			pev->renderamt = 0;
			pev->rendercolor = g_vecZero;
		}
		else
			pev->rendercolor = Vector( pev->renderamt, 0, pev->renderamt * 0.5 );
	}

	if (m_Activity == ACT_RUN || m_Activity == ACT_WALK)
	{
		static int iStep = 0;
		iStep = ! iStep;
		if (iStep)
		{
			switch( RANDOM_LONG( 0, 3 ) )
			{
			case 0:	EMIT_SOUND( ENT(pev), CHAN_BODY, "player/pl_step1.wav", 0.5, ATTN_NORM);	break;
			case 1:	EMIT_SOUND( ENT(pev), CHAN_BODY, "player/pl_step3.wav", 0.5, ATTN_NORM);	break;
			case 2:	EMIT_SOUND( ENT(pev), CHAN_BODY, "player/pl_step2.wav", 0.5, ATTN_NORM);	break;
			case 3:	EMIT_SOUND( ENT(pev), CHAN_BODY, "player/pl_step4.wav", 0.5, ATTN_NORM);	break;
			}
		}
	}
}

//=========================================================
// StartTask
//=========================================================
void CNAgent :: StartTask ( Task_t *pTask )
{
	Vector vecResult;

	switch ( pTask->iTask )
	{
	case TASK_AGENT_BLINK:
		{
			Vector vecDist = m_vecMoveGoal - pev->origin;
			if ( vecDist.Length() >= 128 )
			{
				Vector vecOld = pev->origin;
				UTIL_SetOrigin( pev, m_vecMoveGoal );
				if ( !WALK_MOVE( ENT(pev), 0, 0, WALKMOVE_NORMAL ) )
				{
					// no room there
					UTIL_SetOrigin( pev, vecOld );
				}
				else
				{
					// a spray of sparks where it was
					MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, vecOld );
						WRITE_BYTE( TE_SPRAY );
						WRITE_COORD( vecOld.x );
						WRITE_COORD( vecOld.y );
						WRITE_COORD( vecOld.z );
						WRITE_COORD( 0 );
						WRITE_COORD( 0 );
						WRITE_COORD( 1 );
						WRITE_SHORT( m_iShell );
						WRITE_BYTE( 14 );	// count
						WRITE_BYTE( 130 );	// speed
						WRITE_BYTE( 128 );	// noise
						WRITE_BYTE( kRenderTransAdd );
					MESSAGE_END();
					EMIT_SOUND_DYN( ENT(pev), CHAN_STATIC, "houndeye/he_blast1.wav", 1.0, 0.3, 0, PITCH_NORM );
					TaskComplete();
					break;
				}
			}
			TaskFail();
		}
		break;
	case TASK_AGENT_FIND_COVER_FROM_ENEMY:
		if ( m_hEnemy != NULL )
		{
			if ( FindNodeInCover( m_hEnemy->pev->origin, m_hEnemy->pev->view_ofs, 0, CoverRadius(), vecResult ) )
			{
				m_vecMoveGoal = vecResult;
				TaskComplete();
				break;
			}
		}
		TaskFail();
		break;
	case TASK_AGENT_FIND_COVER_FROM_SOUND:
		{
			CSound *pBestSound = PBestSound();
			float flMaxDist = pBestSound->m_iVolume * 2;	// sic: before the test for no sound
			if ( pBestSound )
			{
				if ( FindNodeInCover( pBestSound->m_vecOrigin, g_vecZero, pBestSound->m_iVolume, max( flMaxDist, CoverRadius() ), vecResult )
					|| FindNodeAway( pBestSound->m_vecOrigin, pBestSound->m_iVolume, max( flMaxDist, CoverRadius() ), vecResult ) )
				{
					m_vecMoveGoal = vecResult;
					TaskComplete();
					break;
				}
			}
			TaskFail();
		}
		break;
	case TASK_AGENT_FIND_NODE_NEAR_ENEMY:
		if ( m_hEnemy == NULL )
		{
			TaskFail();
			break;
		}
		{
			if ( FindEnemyNode( m_hEnemy->pev->origin, vecResult, pTask->flData, 512, 0 ) )
			{
				m_vecMoveGoal = vecResult;
				TaskComplete();
			}
			else
				TaskFail();
		}
		// sic: falls through
	case TASK_AGENT_FIND_NODE_AWAY_FROM_ENEMY:
		if ( m_hEnemy == NULL )
		{
			TaskFail();
			break;
		}
		{
			if ( FindEnemyNode( m_hEnemy->pev->origin, vecResult, 0, 0, pTask->flData ) )
			{
				m_vecMoveGoal = vecResult;
				TaskComplete();
			}
			else
				TaskFail();
		}
		// sic: falls through
	case TASK_SPECIAL_ATTACK1:
		{
			// drop a vortex mine if it can be thrown
			Vector vecToss;
			if ( m_hEnemy != NULL )
				vecToss = VecCheckToss( pev, GetGunPosition(), m_hEnemy->pev->origin + Vector( RANDOM_FLOAT( -64, 64 ), RANDOM_FLOAT( -64, 64 ), 0 ), 0.5 );
			else
				vecToss = VecCheckToss( pev, GetGunPosition(), m_vecEnemyLKP + Vector( RANDOM_FLOAT( -64, 64 ), RANDOM_FLOAT( -64, 64 ), 0 ), 0.5 );

			if ( vecToss == g_vecZero )
			{
				TaskFail();
				break;
			}
			pev->weapons = 1;
			m_IdealActivity = ACT_RANGE_ATTACK2;
		}
		break;
	default:
		CHAssassin :: StartTask ( pTask );
		break;
	}
}

//=========================================================
// RunTask
//=========================================================
void CNAgent :: RunTask ( Task_t *pTask )
{
	switch ( pTask->iTask )
	{
	case TASK_SPECIAL_ATTACK1:
		MakeIdealYaw ( m_vecEnemyLKP );
		ChangeYaw ( pev->yaw_speed );

		if ( m_fSequenceFinished )
		{
			m_Activity = ACT_RESET;
			TaskComplete();
		}
		break;
	default:
		CHAssassin :: RunTask ( pTask );
		break;
	}
}

//=========================================================
// GetSchedule - Decides which type of schedule best suits
// the monster's current state and conditions. Then calls
// monster's member function to get a pointer to a schedule
// of the proper type.
//=========================================================
Schedule_t *CNAgent :: GetSchedule ( void )
{
	if ( !m_pChargeSprite || m_flChargeTime != 0 )
		m_cAmmoLoaded = 1;

	switch	( m_MonsterState )
	{
	case MONSTERSTATE_IDLE:
	case MONSTERSTATE_ALERT:
		{
			if ( HasConditions ( bits_COND_HEAR_SOUND ))
			{
				CSound *pSound;
				pSound = PBestSound();

				if ( pSound && (pSound->m_iType & bits_SOUND_DANGER) )
				{
					return GetScheduleOfType( SCHED_AGENT_BLINK_COVER_FROM_SOUND );
				}
				if ( pSound && (pSound->m_iType & bits_SOUND_COMBAT) )
				{
					return GetScheduleOfType( SCHED_INVESTIGATE_SOUND );
				}
			}
		}
		break;

	case MONSTERSTATE_COMBAT:
		{
// dead enemy
			if ( HasConditions( bits_COND_ENEMY_DEAD ) )
			{
				// call base class, all code to handle dead enemies is centralized there.
				return CBaseMonster :: GetSchedule();
			}

			// flying?
			if ( pev->movetype == MOVETYPE_TOSS)
			{
				if (pev->flags & FL_ONGROUND)
				{
					// just landed
					pev->movetype = MOVETYPE_STEP;
					return GetScheduleOfType ( SCHED_ASSASSIN_JUMP_LAND );
				}
				else
				{
					// jump or jump/shoot
					if ( m_MonsterState == MONSTERSTATE_COMBAT )
						return GetScheduleOfType ( SCHED_ASSASSIN_JUMP );
					else
						return GetScheduleOfType ( SCHED_ASSASSIN_JUMP_ATTACK );
				}
			}

			if ( HasConditions ( bits_COND_HEAR_SOUND ))
			{
				CSound *pSound;
				pSound = PBestSound();

				if ( pSound && (pSound->m_iType & bits_SOUND_DANGER) )
				{
					return GetScheduleOfType( SCHED_AGENT_BLINK_COVER_FROM_SOUND );
				}
			}

			if ( HasConditions ( bits_COND_LIGHT_DAMAGE ) )
			{
				m_iFrustration++;
			}
			if ( HasConditions ( bits_COND_HEAVY_DAMAGE ) )
			{
				m_iFrustration += 2;
			}

		// a charged power wave
			if ( m_pChargeSprite && m_flChargeTime == 0 && m_hEnemy != NULL && pev->frags == 1 )
			{
				if ( HasConditions( bits_COND_CAN_RANGE_ATTACK1 ) && !HasConditions( bits_COND_SPECIAL1 ) )
					return GetScheduleOfType ( SCHED_RANGE_ATTACK1 );

				TraceResult tr;

				if ( m_hEnemy->CanBeRepulsed( false ) )
				{
					// room behind the enemy to blow it away?
					Vector vecSrc = Center();
					Vector vecDir = ( m_hEnemy->Center() - vecSrc ).Normalize();
					UTIL_TraceLine( vecSrc + vecDir * 64, vecSrc + vecDir * 600, dont_ignore_monsters, m_hEnemy->edict(), &tr );
					if ( tr.flFraction >= 0.9 )
						goto attack;
				}

				// no: get some room first
				BOOL fJump = FALSE;
				UTIL_MakeVectors( pev->angles );
				Vector vecSrc = m_hEnemy->Center();
				Vector vecDir = ( gpGlobals->v_up - gpGlobals->v_forward ).Normalize();
				UTIL_TraceLine( vecSrc, Center() + vecDir * 600, ignore_monsters, m_hEnemy->edict(), &tr );
				if ( ( tr.vecEndPos - vecSrc ).Length() >= 480 )
					fJump = CheckJump();

				if ( !WorldGraph.m_fGraphPresent || !WorldGraph.m_fGraphPointersSet )
				{
					if ( fJump && RANDOM_LONG( 0, 255 ) > 127 )
						return GetScheduleOfType ( SCHED_MELEE_ATTACK1 );
					return GetScheduleOfType ( SCHED_AGENT_BLINK_NPW_COMBO );
				}

				if ( fJump )
					return GetScheduleOfType ( SCHED_MELEE_ATTACK1 );
				return GetScheduleOfType ( SCHED_RANGE_ATTACK1 );
			}
attack:
		// jump player!
			if ( HasConditions ( bits_COND_CAN_MELEE_ATTACK1 ) )
			{
				return GetScheduleOfType ( SCHED_MELEE_ATTACK1 );
			}

		// secondary attack
			if ( HasConditions ( bits_COND_CAN_RANGE_ATTACK2 ) )
			{
				if ( m_iFrustration > 1 && m_hEnemy != NULL && ( pev->weapons & 1 ) )
					return slMineField;
				return GetScheduleOfType ( SCHED_RANGE_ATTACK2 );
			}

		// spotted
			if ( HasConditions ( bits_COND_SEE_ENEMY ) && HasConditions ( bits_COND_ENEMY_FACING_ME ) )
			{
				m_iFrustration++;
				if ( RANDOM_FLOAT( 0, 1 ) > 0.25 )
					return GetScheduleOfType ( SCHED_ASSASSIN_EXPOSED );
				return GetScheduleOfType ( SCHED_AGENT_BLINK_ATTACK );
			}

		// can attack
			if ( HasConditions ( bits_COND_CAN_RANGE_ATTACK1 ) )
			{
				if ( pev->armorvalue > 0 )
					m_iFrustration = 0;

				// the stronger the enemy, the likelier a charged attack
				int iHealth = min( m_hEnemy->pev->health + m_hEnemy->pev->armorvalue, 100 );
				if ( !m_pChargeSprite && RANDOM_LONG( 0, 140 ) < iHealth )
				{
					m_flChargeTime = gpGlobals->time;
					m_flChargeScale = 0.5;
					if ( pev->frags == 1 )
					{
						m_pChargeSprite = CSprite::SpriteCreate( "sprites/n_charge.spr", pev->origin, TRUE );
						m_pChargeSprite->SetAttachment( edict(), 1 );
						m_pChargeSprite->SetScale( 0.1 );
						m_pChargeSprite->SetTransparency( kRenderTransAdd, 255, 255, 255, 255, kRenderFxNone );
						m_pChargeSprite->pev->framerate = 10;
					}
					else
					{
						m_pChargeSprite = CSprite::SpriteCreate( "sprites/flux_t.spr", pev->origin, TRUE );
						m_pChargeSprite->SetAttachment( edict(), 1 );
						m_pChargeSprite->SetScale( 0.1 );
						m_pChargeSprite->SetTransparency( kRenderTransAdd, 255, 255, 255, 255, kRenderFxDistort );
					}
					m_pChargeSprite->pev->spawnflags |= SF_SPRITE_TEMPORARY;
				}
				return GetScheduleOfType ( SCHED_RANGE_ATTACK1 );
			}

			if ( m_iFrustration > 2 && m_hEnemy != NULL )
			{
				if ( !m_pChargeSprite )
				{
					pev->frags = RANDOM_LONG( 0, 2 );
					m_flWeaponTime = gpGlobals->time;
				}
				return GetScheduleOfType ( SCHED_AGENT_BLINK_ATTACK );
			}

			if ( HasConditions ( bits_COND_SEE_ENEMY ) )
			{
				return GetScheduleOfType ( SCHED_COMBAT_FACE );
			}

			if ( HasConditions ( bits_COND_ENEMY_TOOFAR ) )
			{
				return GetScheduleOfType ( SCHED_AGENT_BLINK_ATTACK );
			}

		// new enemy
			if ( HasConditions ( bits_COND_NEW_ENEMY ) )
			{
				if ( RANDOM_FLOAT( 0, 1 ) > 0.5 )
					return GetScheduleOfType ( SCHED_AGENT_BLINK_COVER_FROM_ENEMY );
				return GetScheduleOfType ( SCHED_AGENT_BLINK_ATTACK );
			}

			return GetScheduleOfType ( SCHED_ALERT_STAND );
		}
		break;
	}

	return CHAssassin :: GetSchedule();
}

//=========================================================
//=========================================================
Schedule_t* CNAgent :: GetScheduleOfType ( int Type ) 
{
	switch	( Type )
	{
	case SCHED_AGENT_BLINK_COVER_FROM_ENEMY:
		return slAgentBlinkCoverFromEnemy;
	case SCHED_AGENT_BLINK_COVER_FROM_SOUND:
		return slAgentBlinkCoverFromSound;
	case SCHED_AGENT_BLINK_ATTACK:
		return slAgentBlinkAttack;
	case SCHED_AGENT_BLINK_NPW_COMBO:
		return slBlinkNPWCombo;
	}

	return CHAssassin :: GetScheduleOfType( Type );
}

//=========================================================
// CheckRangeAttack1 - picks the primary attack now and then
// by the enemy's speed, strength and distance
//=========================================================
BOOL CNAgent :: CheckRangeAttack1 ( float flDot, float flDist )
{
	float flLow;
	if ( !m_pChargeSprite && m_flWeaponTime + 14 <= gpGlobals->time
		&& RANDOM_FLOAT( flLow = gpGlobals->time - ( m_flWeaponTime + 14 ), 14 ) >= 10 )
	{
		int iWeapon = pev->frags;
		float flSpeed = m_hEnemy->MovementSpeed();
		int iHealth = m_hEnemy->pev->armorvalue + m_hEnemy->pev->health;

		switch ( (int)pev->frags )
		{
		case 0:
			if ( flSpeed > 200 )
				iWeapon = 2;
			else if ( flSpeed > 100 && flDist > 1024 )
				iWeapon = iHealth < 200 ? 2 : 1;
			else if ( flDist > 2048 )
			{
				if ( flSpeed < 100 )
					iWeapon = 1;
				else
					iWeapon = 2;
			}
			else if ( !( flSpeed < 50 && flDist < 512 && iHealth > 500 ) )
				iWeapon = RANDOM_LONG( 0, 2 );
			break;
		case 1:
			if ( flSpeed > 200 || flDist > 2048 )
				iWeapon = 2;
			else if ( ( flSpeed < 50 && flDist < 512 ) || iHealth > 500 )
				iWeapon = 0;
			else if ( !( flSpeed < 100 && flDist < 2048 ) )
				iWeapon = RANDOM_LONG( 0, 2 );
			break;
		case 2:
			if ( iHealth > 500 )
			{
				if ( flDist > 1024 )
					iWeapon = 1;
				else
					iWeapon = 0;
			}
			else if ( flDist < 512 )
			{
				if ( flSpeed > 50 )
					iWeapon = 1;
				else
					iWeapon = 0;
			}
			else if ( flSpeed <= 200 && flDist <= 2048 )
				iWeapon = RANDOM_LONG( 0, 2 );
			break;
		default:
			iWeapon = RANDOM_LONG( 0, 2 );
			m_flWeaponTime = gpGlobals->time;
			break;
		}

		if ( iWeapon != pev->frags )
		{
			pev->frags = iWeapon;
			m_flWeaponTime = gpGlobals->time;
		}
	}

	// the laser pulse is no good up close
	if ( pev->frags == 1 && flDist < 500 )
		SetConditions( bits_COND_SPECIAL1 );
	else
		ClearConditions( bits_COND_SPECIAL1 );

	float flRange = 2048;
	if ( pev->frags != 0 )
		flRange = 4096;

	if ( m_hPowerBeam != NULL || ( m_pChargeSprite && pev->frags == 2 ) )
	{
		if ( flDist <= flRange && flDot >= 0.5 )
			return TRUE;
	}
	else if ( !HasConditions( bits_COND_ENEMY_OCCLUDED ) && flDist <= flRange && flDot >= 0.5 )
	{
		TraceResult	tr;

		Vector vecSrc = GetGunPosition();

		// verify that a bullet fired from the gun will hit the enemy before the world.
		UTIL_TraceLine( vecSrc, m_hEnemy->BodyTarget(vecSrc), dont_ignore_monsters, ENT(pev), &tr);

		if ( tr.flFraction == 1 || tr.pHit == m_hEnemy->edict() )
		{
			return TRUE;
		}
	}
	return FALSE;
}

//=========================================================
// CheckRangeAttack2 - which secondary attacks will do
//=========================================================
BOOL CNAgent :: CheckRangeAttack2 ( float flDot, float flDist )
{
	if ( m_pChargeSprite )
	{
		m_fThrowGrenade = FALSE;
		return FALSE;
	}

	if ( gpGlobals->time < m_flNextGrenadeCheck )
		return m_fThrowGrenade;

	pev->weapons = 7;

	// the vortex mine: grenadable enemies in range
	if ( flDist > 1024 || flDist < 256 || !m_hEnemy->CanBeGrenaded( FALSE ) )
		pev->weapons &= ~1;

	// the flux rocket: not up close
	if ( flDist < 256 )
		pev->weapons &= ~4;

	Vector vecToss = g_vecZero;

	if ( HasConditions( bits_COND_ENEMY_OCCLUDED ) )
	{
		pev->weapons &= ~6;
		if ( pev->weapons & 1 )
			vecToss = VecCheckToss( pev, GetGunPosition(), m_hEnemy->pev->origin, 0.5 );
		else
		{
			m_fThrowGrenade = FALSE;
			return FALSE;
		}
	}
	else if ( pev->weapons & 1 )
		vecToss = VecCheckThrow( pev, GetGunPosition(), m_hEnemy->Center(), flDist, 0.5 );

	if ( vecToss != g_vecZero )
		m_vecTossVelocity = vecToss;

	// the energy disk: not at machines or small things
	if ( m_hEnemy->BloodColor() == DONT_BLEED || m_hEnemy->pev->size.z < 32 )
		pev->weapons &= ~2;

	m_fThrowGrenade = pev->weapons != 0;
	return m_fThrowGrenade;
}

BOOL CNAgent :: FCanCheckAttacks( void )
{
	return TRUE;
}

//=========================================================
// TakeDamage - the shield takes the damage while it lasts
// and stays down five seconds when it breaks
//=========================================================
int CNAgent :: TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	int iResult = TRUE;

	if ( pev->armorvalue > 0 )
	{
		pev->armorvalue -= flDamage;
		pev->renderfx = kRenderFxGlowShell;
		pev->renderamt = 255;
		pev->rendercolor = Vector( 255, 0, 128 );

		if ( flDamage > 20 )
			SetConditions( bits_COND_LIGHT_DAMAGE );
		if ( flDamage >= 100 )
			SetConditions( bits_COND_HEAVY_DAMAGE );

		CBaseMonster::TakeDamage( pevInflictor, pevAttacker, 0, 0 );
	}
	else
		iResult = CBaseMonster::TakeDamage( pevInflictor, pevAttacker, flDamage, bitsDamageType );

	if ( pev->armorvalue <= 0 )
		pev->armortype = 5;	// seconds until the shield regrows

	Forget( bits_MEMORY_INCOVER );
	return iResult;
}

//=========================================================
// Killed - the agent leaves a vortex behind
//=========================================================
void CNAgent :: Killed( entvars_t *pevAttacker, int iGib )
{
	if ( m_pChargeSprite )
		UTIL_Remove( m_pChargeSprite );
	m_pChargeSprite = NULL;

	if ( !HasMemory( bits_MEMORY_KILLED ) )
	{
		CBaseEntity *pBlast = CBaseEntity::Create( "vortex_blast", pev->origin, pev->angles, pev->owner );
		pBlast->pev->dmg = 250;
		pBlast->pev->movedir = Vector( 0, 0, -32 );
		pBlast->pev->scale = 1;
	}

	CBaseMonster::Killed( pevAttacker, iGib );
}

//=========================================================
// CheckJump - room for the jump? sets m_vecJumpVelocity
//=========================================================
BOOL CNAgent :: CheckJump( void )
{
	TraceResult	tr;

	Vector vecDest = pev->origin + Vector( RANDOM_FLOAT( -64, 64), RANDOM_FLOAT( -64, 64 ), 160 );

	UTIL_TraceHull( pev->origin + Vector( 0, 0, 36 ), vecDest + Vector( 0, 0, 36 ), dont_ignore_monsters, human_hull, ENT(pev), &tr);

	if ( tr.fStartSolid || tr.flFraction < 1.0)
	{
		return FALSE;
	}

	float flGravity = g_psv_gravity->value;

	float time = sqrt( 160 / (0.5 * flGravity));
	float speed = flGravity * time / 160;
	m_vecJumpVelocity = (vecDest - pev->origin) * speed;

	return TRUE;
}

#endif
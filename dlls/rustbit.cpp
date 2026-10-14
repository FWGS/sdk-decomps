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
// the rustbit: a small robot of the rust chapter that fires
// glowing bolts from both hands and claws at close range;
// monster_rustbit fights the player, monster_rustbit_friendly
// is on his side
//=========================================================

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"schedule.h"
#include	"weapons.h"
#include	"effects.h"
#include	"soundent.h"
#include	"rust.h"

extern DLL_GLOBAL int g_iSkillLevel;

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define		RUSTBIT_AE_SHOOT_RIGHT		( 1 )	// fire from the right hand
#define		RUSTBIT_AE_SHOOT_LEFT		( 2 )	// fire from the left hand
#define		RUSTBIT_AE_SHOOT_RIGHT2		( 3 )
#define		RUSTBIT_AE_SHOOT_LEFT2		( 4 )
#define		RUSTBIT_AE_CLAW				( 5 )
#define		RUSTBIT_AE_CLAW2			( 6 )

//=========================================================
// squad slots
//=========================================================
#define		bits_SLOTS_RUSTBIT_COVER	( bits_SLOT_HGRUNT_GRENADE2 | bits_SLOT_AGRUNT_HORNET1 )	// 0x18
#define		bits_SLOTS_RUSTBIT_ATTACK	( bits_SLOT_HGRUNT_ENGAGE2 | bits_SLOT_HGRUNT_GRENADE1 )	// 0x06

//=========================================================
// monster-specific schedule types
//=========================================================
enum
{
	SCHED_RUSTBIT_EXPOSED = LAST_COMMON_SCHEDULE + 1,	// the enemy looks at me: get out of his sight
	SCHED_RUSTBIT_TAKECOVER_FAILED,
};

class CRustbit : public CBaseRust
{
public:
	void Spawn( void );
	void Precache( void );
	void SetYawSpeed( void );
	int  Classify( void );
	int  MonsterType( void );
	void PrecacheSounds( void );
	void HandleAnimEvent( MonsterEvent_t *pEvent );
	BOOL CheckRangeAttack1( float flDot, float flDist );
	BOOL CheckRangeAttack2( float flDot, float flDist );
	void RunAI( void );
	void PrescheduleThink( void );
	void Killed( entvars_t *pevAttacker, int iGib );
	void PreRemove( void );

	void Shoot( Vector vecSrc );

	void RangeAttackSound( void );
	void DeathSound( void );
	void AlertSound( void );
	void IdleSound( void );
	void AttackHitSound( CBaseMonster *pVictim );

	Schedule_t *GetSchedule( void );
	Schedule_t *GetScheduleOfType( int Type );

	int	Save( CSave &save );
	int Restore( CRestore &restore );

	CUSTOM_SCHEDULES;
	static TYPEDESCRIPTION m_SaveData[];

	static const char *pSounds[];

	float	m_flLastShot;
	float	m_flDiviation;		// how far the shots stray
	int		m_iVoicePitch;		// the base's is not used
	int		m_iCover;			// schedules picked since the last attack
	int		m_iSpin;			// the turning parts' angle
	CSprite	*m_pLeftHand;		// the glows on the hands
	CSprite	*m_pRightHand;
};

//=========================================================
// AI Schedules Specific to this monster
//=========================================================

//=========================================================
// Fail - wait and look at the enemy, then go after him
//=========================================================
Task_t	tlRustbitFail[] =
{
	{ TASK_STOP_MOVING,			(float)0		},
	{ TASK_SET_ACTIVITY_IDLE,	(float)0		},
	{ TASK_WAIT_FACE_ENEMY,		(float)2		},
	{ TASK_SET_SCHEDULE,		(float)SCHED_CHASE_ENEMY },
};

Schedule_t	slRustbitFail[] =
{
	{
		tlRustbitFail,
		ARRAYSIZE ( tlRustbitFail ),
		bits_COND_LIGHT_DAMAGE		|
		bits_COND_HEAVY_DAMAGE		|
		bits_COND_CAN_RANGE_ATTACK1	|
		bits_COND_CAN_MELEE_ATTACK1	|
		bits_COND_PROVOKED,

		bits_SOUND_PLAYER			|
		bits_SOUND_DANGER,
		"RustbitFail"
	},
};

//=========================================================
// Exposed - the enemy is looking at me
//=========================================================
Task_t	tlRustbitExposed[] =
{
	{ TASK_STOP_MOVING,			(float)0		},
	{ TASK_SET_SCHEDULE,		(float)SCHED_TAKE_COVER_FROM_ENEMY },
};

Schedule_t	slRustbitExposed[] =
{
	{
		tlRustbitExposed,
		ARRAYSIZE ( tlRustbitExposed ),
		bits_COND_CAN_MELEE_ATTACK1	|
		bits_COND_CAN_RANGE_ATTACK2,
		0,
		"RustbitExposed"
	},
};

//=========================================================
// TakeCoverFromEnemy - run to cover
//=========================================================
Task_t	tlRustbitTakeCoverFromEnemy[] =
{
	{ TASK_STOP_MOVING,				(float)0					},
	{ TASK_WAIT,					(float)0.2					},
	{ TASK_SET_FAIL_SCHEDULE,		(float)SCHED_RUSTBIT_TAKECOVER_FAILED },
	{ TASK_FIND_COVER_FROM_ENEMY,	(float)0					},
	{ TASK_RUN_PATH,				(float)0					},
	{ TASK_WAIT_FOR_MOVEMENT,		(float)0					},
	{ TASK_REMEMBER,				(float)bits_MEMORY_INCOVER	},
	{ TASK_FACE_ENEMY,				(float)0					},
};

Schedule_t	slRustbitTakeCoverFromEnemy[] =
{
	{
		tlRustbitTakeCoverFromEnemy,
		ARRAYSIZE ( tlRustbitTakeCoverFromEnemy ),
		bits_COND_CAN_MELEE_ATTACK1	|
		bits_COND_NEW_ENEMY,
		0,
		"RustbitTakeCoverFromEnemy"
	},
};

//=========================================================
// TakeCoverFromEnemy2 - hurt: fire once, then run to cover
// far away
//=========================================================
Task_t	tlRustbitTakeCoverFromEnemy2[] =
{
	{ TASK_STOP_MOVING,						(float)0					},
	{ TASK_WAIT,							(float)0.2					},
	{ TASK_FACE_ENEMY,						(float)0					},
	{ TASK_RANGE_ATTACK2,					(float)0					},
	{ TASK_SET_FAIL_SCHEDULE,				(float)SCHED_RUSTBIT_TAKECOVER_FAILED },
	{ TASK_FIND_FAR_NODE_COVER_FROM_ENEMY,	(float)384					},
	{ TASK_RUN_PATH,						(float)0					},
	{ TASK_WAIT_FOR_MOVEMENT,				(float)0					},
	{ TASK_REMEMBER,						(float)bits_MEMORY_INCOVER	},
	{ TASK_FACE_ENEMY,						(float)0					},
};

Schedule_t	slRustbitTakeCoverFromEnemy2[] =
{
	{
		tlRustbitTakeCoverFromEnemy2,
		ARRAYSIZE ( tlRustbitTakeCoverFromEnemy2 ),
		bits_COND_CAN_MELEE_ATTACK2	|
		bits_COND_NEW_ENEMY,
		0,
		"RustbitTakeCoverFromEnemy2"
	},
};

//=========================================================
// TakeCoverFromBestSound - run away from a danger sound
//=========================================================
Task_t	tlRustbitTakeCoverFromBestSound[] =
{
	{ TASK_SET_FAIL_SCHEDULE,			(float)SCHED_RUSTBIT_TAKECOVER_FAILED },
	{ TASK_STOP_MOVING,					(float)0					},
	{ TASK_FIND_COVER_FROM_BEST_SOUND,	(float)0					},
	{ TASK_RUN_PATH,					(float)0					},
	{ TASK_WAIT_FOR_MOVEMENT,			(float)0					},
	{ TASK_REMEMBER,					(float)bits_MEMORY_INCOVER	},
	{ TASK_TURN_LEFT,					(float)179					},
};

Schedule_t	slRustbitTakeCoverFromBestSound[] =
{
	{
		tlRustbitTakeCoverFromBestSound,
		ARRAYSIZE ( tlRustbitTakeCoverFromBestSound ),
		bits_COND_NEW_ENEMY,
		0,
		"RustbitTakeCoverFromBestSound"
	},
};

//=========================================================
// Hide - wait out of sight, then go after the enemy
//=========================================================
Task_t	tlRustbitHide[] =
{
	{ TASK_STOP_MOVING,			(float)0		},
	{ TASK_SET_ACTIVITY_IDLE,	(float)0		},
	{ TASK_WAIT,				(float)2		},
	{ TASK_SET_SCHEDULE,		(float)SCHED_CHASE_ENEMY },
};

Schedule_t	slRustbitHide[] =
{
	{
		tlRustbitHide,
		ARRAYSIZE ( tlRustbitHide ),
		bits_COND_SEE_FEAR			|
		bits_COND_SEE_ENEMY			|
		bits_COND_LIGHT_DAMAGE		|
		bits_COND_HEAVY_DAMAGE		|
		bits_COND_PROVOKED			|
		bits_COND_NEW_ENEMY			|
		bits_COND_HEAR_SOUND,

		bits_SOUND_DANGER,
		"RustbitHide"
	},
};

//=========================================================
// Hunt - run to the enemy
//=========================================================
Task_t	tlRustbitHunt[] =
{
	{ TASK_GET_PATH_TO_ENEMY,	(float)0		},
	{ TASK_RUN_PATH,			(float)0		},
	{ TASK_WAIT_FOR_MOVEMENT,	(float)0		},
};

Schedule_t	slRustbitHunt[] =
{
	{
		tlRustbitHunt,
		ARRAYSIZE ( tlRustbitHunt ),
		bits_COND_CAN_RANGE_ATTACK1	|
		bits_COND_CAN_MELEE_ATTACK1	|
		bits_COND_CAN_RANGE_ATTACK2	|
		bits_COND_NEW_ENEMY			|
		bits_COND_HEAR_SOUND,

		bits_SOUND_DANGER,
		"RustbitHunt"
	},
};

//=========================================================
// Range1 - two volleys
//=========================================================
Task_t	tlRustbitRange1[] =
{
	{ TASK_STOP_MOVING,			(float)0		},
	{ TASK_FACE_IDEAL,			(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_RANGE_ATTACK2,		(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_RANGE_ATTACK2,		(float)0		},
};

Schedule_t	slRustbitRange1[] =
{
	{
		tlRustbitRange1,
		ARRAYSIZE ( tlRustbitRange1 ),
		bits_COND_CAN_MELEE_ATTACK1	|
		bits_COND_NEW_ENEMY,
		0,
		"Rustbit Range1"
	},
};

//=========================================================
// Range2 - five volleys
//=========================================================
Task_t	tlRustbitRange2[] =
{
	{ TASK_STOP_MOVING,			(float)0		},
	{ TASK_FACE_IDEAL,			(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_RANGE_ATTACK2,		(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_RANGE_ATTACK2,		(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_RANGE_ATTACK2,		(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_RANGE_ATTACK2,		(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_RANGE_ATTACK2,		(float)0		},
};

Schedule_t	slRustbitRange2[] =
{
	{
		tlRustbitRange2,
		ARRAYSIZE ( tlRustbitRange2 ),
		bits_COND_CAN_MELEE_ATTACK1	|
		bits_COND_NEW_ENEMY,
		0,
		"Rustbit Range2"
	},
};

LINK_ENTITY_TO_CLASS( monster_rustbit, CRustbit );

//=========================================================
// Classify - indicates this monster's place in the
// relationship table.
//=========================================================
int CRustbit::Classify( void )
{
	return CLASS_ROBOT_GUNNER;
}

int CRustbit::MonsterType( void )
{
	return 12;
}

TYPEDESCRIPTION	CRustbit::m_SaveData[] =
{
	DEFINE_FIELD( CRustbit, m_flLastShot, FIELD_TIME ),
	DEFINE_FIELD( CRustbit, m_flDiviation, FIELD_FLOAT ),
	DEFINE_FIELD( CRustbit, m_iVoicePitch, FIELD_INTEGER ),
	DEFINE_FIELD( CRustbit, m_pLeftHand, FIELD_CLASSPTR ),
	DEFINE_FIELD( CRustbit, m_pRightHand, FIELD_CLASSPTR ),
};

IMPLEMENT_SAVERESTORE( CRustbit, CBaseRust );

DEFINE_CUSTOM_SCHEDULES( CRustbit )
{
	slRustbitFail,
	slRustbitExposed,
	slRustbitTakeCoverFromEnemy,
	slRustbitTakeCoverFromEnemy2,
	slRustbitTakeCoverFromBestSound,
	slRustbitHide,
	slRustbitHunt,
	slRustbitRange1,
	slRustbitRange2,
};

IMPLEMENT_CUSTOM_SCHEDULES( CRustbit, CBaseRust );

const char *CRustbit::pSounds[] =
{
	"rustbit/Rustbit_Clawing_F0.wav",
	"rustbit/Rustbit_Death_F0.wav",
	"rustbit/Rustbit_Flinch_F0.wav",
	"rustbit/Rustbit_Idle_F0.wav",
	"rustbit/RustBit_Walk_LeftFoot_F11.wav",
	"rustbit/RustBit_Walk_RightFoot_F35.wav",
	"rustbit/RustBit_Run_LeftFoot_F0.wav",
	"rustbit/RustBit_Run_RightFoot_F17.wav",
	"rustbit/Rustbit_SeePlayer.wav",
	"rustbit/Rustbit_RapidFire_FirstShot_F0.wav",
	"rustbit/Rustbit_RapidFire_SecondShot_F1.wav",
	"debris/bustflesh2.wav",
};

void CRustbit::PrecacheSounds( void )
{
	for ( int i = 0; i < ARRAYSIZE( pSounds ); i++ )
		PRECACHE_SOUND( (char *)pSounds[i] );
}

//=========================================================
// AlertSound
//=========================================================
void CRustbit::AlertSound( void )
{
	if ( !FBitSet( pev->spawnflags, SF_MONSTER_GAG ) )
		EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "rustbit/Rustbit_SeePlayer.wav", 1.0, ATTN_NORM, 0, m_iVoicePitch );
}

//=========================================================
// RangeAttackSound
//=========================================================
void CRustbit::RangeAttackSound( void )
{
	switch ( RANDOM_LONG( 0, 1 ) )
	{
	case 0:
		EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, "rustbit/Rustbit_RapidFire_SecondShot_F1.wav", 0.7, ATTN_NORM, 0, m_iVoicePitch );
		break;
	case 1:
		EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, "rustbit/Rustbit_RapidFire_FirstShot_F0.wav", 0.7, ATTN_NORM, 0, m_iVoicePitch );
		break;
	}
}

//=========================================================
// DeathSound
//=========================================================
void CRustbit::DeathSound( void )
{
	EMIT_SOUND_DYN( ENT(pev), CHAN_BODY, "rustbit/Rustbit_Death_F0.wav", 1.0, ATTN_NORM, 0, m_iVoicePitch );
}

//=========================================================
// IdleSound
//=========================================================
void CRustbit::IdleSound( void )
{
	if ( RANDOM_LONG( 0, 3 ) == 0 && !FBitSet( pev->spawnflags, SF_MONSTER_GAG ) )
		EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "rustbit/Rustbit_Idle_F0.wav", 0.7, ATTN_NORM, 0, m_iVoicePitch );
}

//=========================================================
// AttackHitSound - the claw hit something
//=========================================================
void CRustbit::AttackHitSound( CBaseMonster *pVictim )
{
	EMIT_SOUND_DYN( ENT(pVictim->pev), CHAN_WEAPON, "debris/bustflesh2.wav", 1.0, ATTN_NORM, 0, 50 );
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CRustbit::SetYawSpeed( void )
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
// CheckRangeAttack1 - a clear shot from either hand
//=========================================================
BOOL CRustbit::CheckRangeAttack1( float flDot, float flDist )
{
	if ( flDist > 64 && flDist <= 2048 )
	{
		TraceResult	tr;
		Vector vecSrc;

		vecSrc = pev->origin + gpGlobals->v_right * 10 + gpGlobals->v_up * 20;
		UTIL_TraceLine( vecSrc, m_hEnemy->BodyTarget( vecSrc ), dont_ignore_monsters, ENT(pev), &tr );

		if ( tr.flFraction == 1 || tr.pHit == m_hEnemy->edict() )
			return TRUE;

		vecSrc = pev->origin + gpGlobals->v_right * -10 + gpGlobals->v_up * 20;
		UTIL_TraceLine( vecSrc, m_hEnemy->BodyTarget( vecSrc ), dont_ignore_monsters, ENT(pev), &tr );

		if ( tr.flFraction == 1 || tr.pHit == m_hEnemy->edict() )
			return TRUE;
	}

	return FALSE;
}

BOOL CRustbit::CheckRangeAttack2( float flDot, float flDist )
{
	return FALSE;
}

//=========================================================
// Shoot - one bolt at the enemy. Back to back shots stray
// less and less; after a pause they stray more.
//=========================================================
void CRustbit::Shoot( Vector vecSrc )
{
	if ( m_hEnemy != NULL )
	{
		Vector vecDir = ShootAtEnemy( vecSrc );

		if ( m_flLastShot + 0.5 < gpGlobals->time )
		{
			switch ( g_iSkillLevel )
			{
			case SKILL_MEDIUM:
				m_flDiviation += 0.05;
				break;
			case SKILL_HARD:
				m_flDiviation += 0.03;
				break;
			default:
				m_flDiviation += 0.1;
				break;
			}
		}
		else
		{
			switch ( g_iSkillLevel )
			{
			case SKILL_MEDIUM:
				m_flDiviation -= 0.02;
				break;
			case SKILL_HARD:
				m_flDiviation -= 0.03;
				break;
			default:
				m_flDiviation -= 0.01;
				break;
			}
		}

		if ( m_flDiviation < 0.01 )
			m_flDiviation = 0.01;
		if ( m_flDiviation > 0.15 )
			m_flDiviation = 0.15;

		m_flLastShot = gpGlobals->time;

		UTIL_MakeVectors( pev->angles );

		Vector vecSpread = Vector( m_flDiviation, m_flDiviation, m_flDiviation / 1.2 );
		CRustbitShot::Shoot( pev, vecSrc, vecDir, vecSpread, 1000 );

		RangeAttackSound();

		pev->effects |= EF_MUZZLEFLASH;

		Vector angDir = UTIL_VecToAngles( vecDir );
		SetBlending( 0, angDir.x );
	}
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CRustbit::HandleAnimEvent( MonsterEvent_t *pEvent )
{
	switch ( pEvent->event )
	{
	case RUSTBIT_AE_SHOOT_RIGHT:
	case RUSTBIT_AE_SHOOT_RIGHT2:
		{
			Vector vecSrc, vecAngles;
			GetAttachment( 0, vecSrc, vecAngles );
			Shoot( vecSrc );

			if ( m_pRightHand == NULL )
			{
				m_pRightHand = CSprite::SpriteCreate( "sprites/rustbitglow.spr", vecSrc, TRUE );
				m_pRightHand->SetAttachment( ENT(pev), 2 );
				m_pRightHand->SetTransparency( kRenderGlow, 255, 255, 255, 200, kRenderFxNoDissipation );
				m_pRightHand->SetScale( 0.5 );
			}

			if ( m_pRightHand )
				m_pRightHand->SetBrightness( 200 );
		}
		break;

	case RUSTBIT_AE_SHOOT_LEFT:
	case RUSTBIT_AE_SHOOT_LEFT2:
		{
			Vector vecSrc, vecAngles;
			GetAttachment( 1, vecSrc, vecAngles );
			Shoot( vecSrc );

			if ( m_pLeftHand == NULL )
			{
				m_pLeftHand = CSprite::SpriteCreate( "sprites/rustbitglow.spr", vecSrc, TRUE );
				m_pLeftHand->SetAttachment( ENT(pev), 1 );
				m_pLeftHand->SetTransparency( kRenderGlow, 255, 255, 255, 200, kRenderFxNoDissipation );
				m_pLeftHand->SetScale( 0.5 );
			}

			if ( m_pLeftHand )
				m_pLeftHand->SetBrightness( 200 );
		}
		break;

	case RUSTBIT_AE_CLAW:
	case RUSTBIT_AE_CLAW2:
		{
			CBaseEntity *pHurt = CheckTraceHullAttack( 80, gSkillData.rustbitDmgClaw, DMG_SLASH );

			if ( pHurt )
			{
				pHurt->pev->punchangle.z = rand() % 20 - 10;
				pHurt->pev->punchangle.x = rand() % 20 - 10;
				pHurt->pev->velocity = pHurt->pev->velocity - gpGlobals->v_forward * 20;
				pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_up * 20;

				CBaseMonster *pMonster = pHurt->MyMonsterPointer();
				if ( pMonster )
				{
					TraceResult tr;
					UTIL_TraceLine( BodyTarget( g_vecZero ), pMonster->BodyTarget( g_vecZero ), dont_ignore_monsters, ENT(pev), &tr );
					pMonster->TraceBleed( gSkillData.rustbitDmgClaw, ( pMonster->BodyTarget( g_vecZero ) - BodyTarget( g_vecZero ) ).Normalize(), &tr, DMG_SLASH, tr.vecEndPos );

					if ( pMonster->IsPlayer() )
						AttackHitSound( pMonster );
				}
			}
		}
		break;

	default:
		CBaseRust::HandleAnimEvent( pEvent );
		break;
	}
}

//=========================================================
// Killed - the glows go with the rustbit
//=========================================================
void CRustbit::Killed( entvars_t *pevAttacker, int iGib )
{
	if ( m_pLeftHand )
	{
		UTIL_Remove( m_pLeftHand );
		m_pLeftHand = NULL;
	}
	if ( m_pRightHand )
	{
		UTIL_Remove( m_pRightHand );
		m_pRightHand = NULL;
	}

	CBaseRust::Killed( pevAttacker, iGib );
}

void CRustbit::PreRemove( void )
{
	if ( m_pLeftHand )
	{
		UTIL_Remove( m_pLeftHand );
		m_pLeftHand = NULL;
	}
	if ( m_pRightHand )
	{
		UTIL_Remove( m_pRightHand );
		m_pRightHand = NULL;
	}

	CBaseRust::PreRemove();
}

//=========================================================
// Spawn
//=========================================================
void CRustbit::Spawn( void )
{
	Precache( );

	SET_MODEL( ENT(pev), "models/rustbit.mdl" );
	UTIL_SetSize( pev, Vector( -16, -16, 0 ), Vector( 16, 16, 42 ) );

	pev->solid = SOLID_SLIDEBOX;
	pev->health = UTIL_RandomizeHealth( gSkillData.rustbitHealth );
	m_iVoicePitch = RANDOM_LONG( -10, 10 ) + 80;
	m_iSpin = 0;

	RustInit( );
	MonsterInit( );

	pev->view_ofs = Vector( 0, 0, 40 );

	m_pRightHand = NULL;
	m_pLeftHand = NULL;
}

//=========================================================
// RunAI - walks faster than its animations, and keeps its
// turning parts going
//=========================================================
void CRustbit::RunAI( void )
{
	switch ( m_Activity )
	{
	case ACT_WALK:
	case ACT_RUN:
		pev->framerate = 1.7;
		break;
	case ACT_ARM:
		pev->framerate = 1.4;
		break;
	default:
		pev->framerate = 1.0;
		break;
	}

	if ( ( m_iSpin += 15 ) > 359 )
	{
		pev->effects |= EF_NOINTERP;
		m_iSpin -= 359;
	}

	SetBoneController( 0, m_iSpin );
	SetBoneController( 1, m_iSpin );

	CBaseRust::RunAI();
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CRustbit::Precache( void )
{
	PRECACHE_MODEL( "models/rustbit.mdl" );
	PRECACHE_MODEL( "sprites/rustbitglow.spr" );

	UTIL_PrecacheOther( "rustbitshot" );

	CBaseRust::Precache( );
}

//=========================================================
// PrescheduleThink - keeps the squad's enemy sighting time,
// and fades the hand glows
//=========================================================
void CRustbit::PrescheduleThink( void )
{
	if ( InSquad() && m_hEnemy != NULL )
	{
		if ( HasConditions( bits_COND_SEE_ENEMY ) )
		{
			// update the squad's last enemy sighting time.
			MySquadLeader()->m_flLastEnemySightTime = gpGlobals->time;
			m_flLastEnemySightTime = gpGlobals->time;
		}
		else
		{
			if ( gpGlobals->time - MySquadLeader()->m_flLastEnemySightTime > 5 )
			{
				// been a while since we've seen the enemy
				MySquadLeader()->m_fEnemyEluded = TRUE;
			}
		}
	}
	else if ( m_hEnemy != NULL && HasConditions( bits_COND_SEE_ENEMY ) )
	{
		m_flLastEnemySightTime = gpGlobals->time;
	}

	if ( m_pRightHand )
	{
		m_pRightHand->pev->renderamt = UTIL_Approach( 0, m_pRightHand->pev->renderamt, 4 );
		if ( m_pRightHand->pev->renderamt == 0 )
			m_pRightHand->pev->effects |= EF_NODRAW;
		else
			m_pRightHand->pev->effects &= ~EF_NODRAW;
	}

	if ( m_pLeftHand )
	{
		m_pLeftHand->pev->renderamt = UTIL_Approach( 0, m_pLeftHand->pev->renderamt, 4 );
		if ( m_pLeftHand->pev->renderamt == 0 )
			m_pLeftHand->pev->effects |= EF_NODRAW;
		else
			m_pLeftHand->pev->effects &= ~EF_NODRAW;
	}
}

//=========================================================
// GetSchedule - Decides which type of schedule best suits
// the monster's current state and conditions. Then calls
// monster's member function to get a pointer to a schedule
// of the proper type.
//=========================================================
Schedule_t *CRustbit::GetSchedule( void )
{
	switch ( m_MonsterState )
	{
	case MONSTERSTATE_IDLE:
	case MONSTERSTATE_ALERT:
		{
			if ( HasConditions( bits_COND_HEAR_SOUND ) )
			{
				CSound *pSound;
				pSound = PBestSound();

				if ( pSound )
				{
					if ( pSound->m_iType & bits_SOUND_DANGER )
						return GetScheduleOfType( SCHED_TAKE_COVER_FROM_BEST_SOUND );
					if ( pSound->m_iType & bits_SOUND_COMBAT )
						return GetScheduleOfType( SCHED_INVESTIGATE_SOUND );
				}
			}
		}
		break;

	case MONSTERSTATE_COMBAT:
		{
			// dead enemy
			if ( HasConditions( bits_COND_ENEMY_DEAD ) )
				break;

			// new enemy
			if ( HasConditions( bits_COND_NEW_ENEMY ) )
				return GetScheduleOfType( SCHED_WAKE_ANGRY );

			if ( HasConditions( bits_COND_HEAR_SOUND ) )
			{
				CSound *pSound;
				pSound = PBestSound();

				if ( pSound && ( pSound->m_iType & bits_SOUND_DANGER ) )
				{
					m_iCover++;
					return GetScheduleOfType( SCHED_TAKE_COVER_FROM_BEST_SOUND );
				}
			}

			if ( HasConditions( bits_COND_CAN_MELEE_ATTACK1 ) )
				return GetScheduleOfType( SCHED_MELEE_ATTACK1 );

			if ( HasConditions( bits_COND_LIGHT_DAMAGE ) && !HasMemory( bits_MEMORY_FLINCHED ) )
			{
				m_iCover++;
				Remember( bits_MEMORY_FLINCHED );
				return GetScheduleOfType( SCHED_SMALL_FLINCH );
			}

			if ( HasConditions( bits_COND_SEE_ENEMY ) && HasConditions( bits_COND_ENEMY_FACING_ME ) && OccupySlot( bits_SLOTS_RUSTBIT_COVER ) && m_iCover < 3 )
			{
				m_iCover++;
				return GetScheduleOfType( SCHED_RUSTBIT_EXPOSED );
			}

			if ( HasConditions( bits_COND_CAN_RANGE_ATTACK1 ) && ( m_iCover > 3 || OccupySlot( bits_SLOTS_RUSTBIT_ATTACK ) ) )
			{
				m_iCover = 0;
				return GetScheduleOfType( SCHED_RANGE_ATTACK1 );
			}

			if ( HasConditions( bits_COND_SEE_ENEMY ) )
				return GetScheduleOfType( SCHED_COMBAT_FACE );

			if ( OccupySlot( bits_SLOTS_RUSTBIT_COVER ) )
				return GetScheduleOfType( SCHED_TAKE_COVER_FROM_ENEMY );
		}
		break;
	}

	return CBaseRust::GetSchedule();
}

//=========================================================
// GetScheduleOfType
//=========================================================
Schedule_t *CRustbit::GetScheduleOfType( int Type )
{
	switch ( Type )
	{
	case SCHED_RANGE_ATTACK2:
		return &slRustbitRange2[ 0 ];

	case SCHED_RUSTBIT_TAKECOVER_FAILED:
		if ( gpGlobals->time - m_flLastEnemySightTime > 10 )
			return &slRustbitRange1[ 0 ];
		return &slRustbitFail[ 0 ];

	case SCHED_RANGE_ATTACK1:
		return &slRustbitRange1[ 0 ];

	case SCHED_TAKE_COVER_FROM_ENEMY:
		if ( pev->health > 30 )
			return &slRustbitTakeCoverFromEnemy[ 0 ];
		return &slRustbitTakeCoverFromEnemy2[ 0 ];

	case SCHED_TAKE_COVER_FROM_BEST_SOUND:
		return &slRustbitTakeCoverFromBestSound[ 0 ];

	case SCHED_RUSTBIT_EXPOSED:
		return &slRustbitExposed[ 0 ];

	case SCHED_FAIL:
		if ( m_MonsterState == MONSTERSTATE_COMBAT )
			return &slRustbitFail[ 0 ];
		break;

	case SCHED_ALERT_STAND:
		if ( m_MonsterState == MONSTERSTATE_COMBAT )
			return &slRustbitHide[ 0 ];
		break;

	case SCHED_CHASE_ENEMY:
		return &slRustbitHunt[ 0 ];
	}

	return CBaseRust::GetScheduleOfType( Type );
}

//=========================================================
// monster_rustbit_friendly - on the player's side
//=========================================================
class CRustbitFriendly : public CRustbit
{
public:
	int  Classify( void );
};

LINK_ENTITY_TO_CLASS( monster_rustbit_friendly, CRustbitFriendly );

int CRustbitFriendly::Classify( void )
{
	return CLASS_PLAYER_ALLY;
}

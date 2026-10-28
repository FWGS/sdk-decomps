/***
*
*	Copyright (c) 1999, 2000 Valve LLC. All rights reserved.
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
// chumtoad.cpp - T2DEF's chumtoad, a headcrab that leaves
// a poison cloud when it dies (file name ours)
//=========================================================

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"schedule.h"
#include	"game.h"


//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define		HC_AE_JUMPATTACK	( 2 )
#define		CT_AE_STEP			( 3 ) // name ours

Task_t	tlCTRangeAttack1[] =
{
	{ TASK_STOP_MOVING,			(float)0		},
	{ TASK_FACE_IDEAL,			(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE	},
	{ TASK_FACE_IDEAL,			(float)0		},
	{ TASK_WAIT_RANDOM,			(float)0.5		},
};

Schedule_t	slCTRangeAttack1[] =
{
	{
		tlCTRangeAttack1,
		ARRAYSIZE ( tlCTRangeAttack1 ),
		bits_COND_ENEMY_OCCLUDED	|
		bits_COND_NO_AMMO_LOADED,
		0,
		"CTRangeAttack1"
	},
};

Task_t	tlCTRangeAttack1Fast[] =
{
	{ TASK_STOP_MOVING,			(float)0		},
	{ TASK_FACE_IDEAL,			(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE	},
};

Schedule_t	slCTRangeAttack1Fast[] =
{
	{
		tlCTRangeAttack1Fast,
		ARRAYSIZE ( tlCTRangeAttack1Fast ),
		bits_COND_ENEMY_OCCLUDED	|
		bits_COND_NO_AMMO_LOADED,
		0,
		"CTRAFast"
	},
};

int iChumPoisonSprite; // name ours

//=========================================================
// CChumPoisonCloud - the cloud a dead chumtoad leaves
//=========================================================
class CChumPoisonCloud : public CBaseEntity
{
public:
	void Spawn( void );
	static CChumPoisonCloud *Create( Vector vecOrigin );
	void EXPORT CloudThink( void );

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	float m_flTimeOfSpawn;
	int m_maxFrame;
};
LINK_ENTITY_TO_CLASS( chum_cloud, CChumPoisonCloud );

TYPEDESCRIPTION	CChumPoisonCloud::m_SaveData[] =
{
	DEFINE_FIELD( CChumPoisonCloud, m_flTimeOfSpawn, FIELD_TIME ),
	DEFINE_FIELD( CChumPoisonCloud, m_maxFrame, FIELD_INTEGER ),
};

IMPLEMENT_SAVERESTORE( CChumPoisonCloud, CBaseEntity );

CChumPoisonCloud *CChumPoisonCloud::Create( Vector vecOrigin )
{
	CChumPoisonCloud *pCloud = GetClassPtr( (CChumPoisonCloud *)NULL );
	pCloud->pev->classname = MAKE_STRING("chum_poison_cloud");
	pCloud->pev->origin = vecOrigin;
	pCloud->Spawn();
	pCloud->m_flTimeOfSpawn = gpGlobals->time;

	return pCloud;
}

void CChumPoisonCloud :: Spawn( void )
{
	pev->movetype = MOVETYPE_NONE;
	pev->solid = SOLID_NOT;
	pev->rendermode = kRenderGlow;
	pev->renderamt = 255;
	pev->rendercolor.x = 255;
	pev->rendercolor.y = 255;
	pev->rendercolor.z = 255;
	pev->renderfx = kRenderFxNoDissipation;

	SET_MODEL(ENT(pev), "sprites/poison.spr");
	pev->frame = 0;
	pev->scale = 0.5;

	UTIL_SetSize(pev, Vector( 0, 0, 0), Vector(0, 0, 0));
	UTIL_SetOrigin( this, pev->origin );

	m_maxFrame = (float) MODEL_FRAMES( pev->modelindex ) - 1;

	SetThink( CloudThink );
	SetNextThink( 0.1 );
}

void CChumPoisonCloud :: CloudThink( void )
{
	float flRenderAmt; // name ours

	if ( pev->frame++ && pev->frame > m_maxFrame )
		pev->frame = 0;

	if ( gpGlobals->time > m_flTimeOfSpawn + 12 - 2.0 && pev->renderamt > 0 )
	{
		flRenderAmt = pev->renderamt - 10;
		pev->renderamt = flRenderAmt;
	}

	if ( m_flTimeOfSpawn + 12 > gpGlobals->time )
	{
		CBaseEntity *pPlayer = CBaseEntity::Instance( g_engfuncs.pfnPEntityOfEntIndex( 1 ) );
		Vector vecDistance = pev->origin - pPlayer->pev->origin; // sic: pPlayer used before the NULL test

		if ( pPlayer )
		{
			if ( vecDistance.Length() < 48 && ( pev->dmgtime <= gpGlobals->time || gpGlobals->time == pev->pain_finished ) )
			{
				pPlayer->TakeDamage( pev, pev, 2, DMG_POISON );
				pev->pain_finished = gpGlobals->time;
				pev->dmgtime = gpGlobals->time + 0.5;
			}
		}

		SetNextThink( 0.1 );
	}
	else
	{
		SetThink( SUB_Remove );
		SetNextThink( 0 );
	}
}


class CChumToad : public CBaseMonster
{
public:
	void Spawn( void );
	void Precache( void );
	void RunTask ( Task_t *pTask );
	void StartTask ( Task_t *pTask );
	void SetYawSpeed ( void );
	void EXPORT LeapTouch ( CBaseEntity *pOther );
	Vector Center( void );
	Vector BodyTarget( const Vector &posSrc );
	void PainSound( void );
	void DeathSound( void );
	void IdleSound( void );
	void AlertSound( void );
	void StepSound( void ); // name ours
	void PrescheduleThink( void );
	int  Classify ( void );
	void HandleAnimEvent( MonsterEvent_t *pEvent );
	BOOL CheckRangeAttack1 ( float flDot, float flDist );
	void RunAI( void );

	virtual float GetDamageAmount( void ) { return gSkillData.headcrabDmgBite * 0.5; }
	virtual int GetVoicePitch( void ) { return 100; }
	virtual float GetSoundVolue( void ) { return 1.0; }
	Schedule_t* GetScheduleOfType ( int Type );

	CUSTOM_SCHEDULES;

	static const char *pIdleSounds[];
	static const char *pAlertSounds[];
	static const char *pPainSounds[];
	static const char *pAttackSounds[];
	static const char *pDeathSounds[];
	static const char *pBiteSounds[];
	static const char *pStepSounds[]; // name ours
};
LINK_ENTITY_TO_CLASS( monster_chumtoad, CChumToad );

DEFINE_CUSTOM_SCHEDULES( CChumToad )
{
	slCTRangeAttack1,
	slCTRangeAttack1Fast,
};

IMPLEMENT_CUSTOM_SCHEDULES( CChumToad, CBaseMonster );

const char *CChumToad::pIdleSounds[] =
{
	"chumtoad/chum_idle1.wav",
	"chumtoad/chum_idle2.wav",
	"chumtoad/chum_idle3.wav",
	"chumtoad/chum_idle4.wav",
};
const char *CChumToad::pAlertSounds[] =
{
	"chumtoad/chum_alert1.wav",
	"chumtoad/chum_alert2.wav",
};
const char *CChumToad::pPainSounds[] =
{
	"headcrab/hc_pain1.wav",
	"headcrab/hc_pain2.wav",
	"headcrab/hc_pain3.wav",
};
const char *CChumToad::pAttackSounds[] =
{
	"headcrab/hc_attack1.wav",
};

const char *CChumToad::pDeathSounds[] =
{
	"chumtoad/chum_die1.wav",
	"chumtoad/chum_die2.wav",
};

const char *CChumToad::pBiteSounds[] =
{
	"headcrab/hc_headbite.wav",
};

const char *CChumToad::pStepSounds[] =
{
	"chumtoad/chum_step1.wav",
	"chumtoad/chum_step2.wav",
	"chumtoad/chum_step3.wav",
	"chumtoad/chum_step4.wav",
};

//=========================================================
// Classify - indicates this monster's place in the
// relationship table.
//=========================================================
int	CChumToad :: Classify ( void )
{
	return m_iClass?m_iClass:CLASS_ALIEN_PREY;
}

//=========================================================
// Center - returns the real center of the headcrab.  The
// bounding box is much larger than the actual creature so
// this is needed for targeting
//=========================================================
Vector CChumToad :: Center ( void )
{
	return Vector( pev->origin.x, pev->origin.y, pev->origin.z + 6 );
}


Vector CChumToad :: BodyTarget( const Vector &posSrc )
{
	return Center( );
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CChumToad :: SetYawSpeed ( void )
{
	int ys;

	ys = 60;

	pev->yaw_speed = ys;
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CChumToad :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	switch( pEvent->event )
	{
		case HC_AE_JUMPATTACK:
		{
			ClearBits( pev->flags, FL_ONGROUND );

			UTIL_SetOrigin (this, pev->origin + Vector ( 0 , 0 , 1) );// take him off ground so engine doesn't instantly reset onground
			UTIL_MakeVectors ( pev->angles );

			Vector vecJumpDir;
			if (m_hEnemy != NULL)
			{
				float gravity = g_psv_gravity->value;
				if (gravity <= 1)
					gravity = 1;

				// How fast does the headcrab need to travel to reach that height given gravity?
				float height = (m_hEnemy->pev->origin.z + m_hEnemy->pev->view_ofs.z - pev->origin.z);
				if (height < 16)
					height = 16;
				float speed = sqrt( 2 * gravity * height );
				float time = speed / gravity;

				// Scale the sideways velocity to get there at the right time
				vecJumpDir = (m_hEnemy->pev->origin + m_hEnemy->pev->view_ofs - pev->origin);
				vecJumpDir = vecJumpDir * ( 1.0 / time );

				// Speed to offset gravity at the desired height
				vecJumpDir.z = speed;

				// Don't jump too far/fast
				float distance = vecJumpDir.Length();

				if (distance > 650)
				{
					vecJumpDir = vecJumpDir * ( 650.0 / distance );
				}
			}
			else
			{
				// jump hop, don't care where
				vecJumpDir = Vector( gpGlobals->v_forward.x, gpGlobals->v_forward.y, gpGlobals->v_up.z ) * 350;
			}

			int iSound = RANDOM_LONG(0,2);
			if ( iSound != 0 )
				EMIT_SOUND_DYN( edict(), CHAN_VOICE, pAttackSounds[iSound], GetSoundVolue(), ATTN_IDLE, 0, GetVoicePitch() ); // sic: pAttackSounds has one entry

			pev->velocity = vecJumpDir;
			m_flNextAttack = gpGlobals->time + 2;
		}
		break;

		case CT_AE_STEP:
			StepSound();
		break;

		default:
			CBaseMonster::HandleAnimEvent( pEvent );
			break;
	}
}

//=========================================================
// Spawn
//=========================================================
void CChumToad :: Spawn()
{
	Precache( );

	SET_MODEL(ENT(pev), "models/chumtoad.mdl");
	UTIL_SetSize(pev, Vector(-12, -12, 0), Vector(12, 12, 24));

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= BLOOD_COLOR_GREEN;
	pev->effects		= 0;
	pev->health			= gSkillData.headcrabHealth * 0.8;
	pev->view_ofs		= Vector ( 0, 0, 20 );// position of the eyes relative to monster's origin.
	pev->yaw_speed		= 5;//!!! should we put this in the monster's changeanim function since turn rates may vary with state/anim?
	m_flFieldOfView		= 0.5;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState		= MONSTERSTATE_NONE;
	pev->iuser1			= 0;

	MonsterInit();
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CChumToad :: Precache()
{
	PRECACHE_SOUND_ARRAY(pIdleSounds);
	PRECACHE_SOUND_ARRAY(pAlertSounds);
	PRECACHE_SOUND_ARRAY(pPainSounds);
	PRECACHE_SOUND_ARRAY(pAttackSounds);
	PRECACHE_SOUND_ARRAY(pDeathSounds);
	PRECACHE_SOUND_ARRAY(pBiteSounds);
	PRECACHE_SOUND_ARRAY(pStepSounds);

	PRECACHE_MODEL("models/chumtoad.mdl");
	iChumPoisonSprite = PRECACHE_MODEL("sprites/poison.spr");
}


//=========================================================
// RunTask
//=========================================================
void CChumToad :: RunTask ( Task_t *pTask )
{
	switch ( pTask->iTask )
	{
	case TASK_RANGE_ATTACK1:
	case TASK_RANGE_ATTACK2:
		{
			if ( m_fSequenceFinished )
			{
				TaskComplete();
				SetTouch( NULL );
				m_IdealActivity = ACT_IDLE;
			}
			break;
		}
	case TASK_DIE:
		{
			if ( m_fSequenceFinished && pev->frame >= 255 )
			{
				CChumPoisonCloud *pCloud = CChumPoisonCloud::Create( pev->origin + Vector( 0, 0, 12 ) );
				pCloud->pev->owner = edict();
			}
		}
	default:
		{
			CBaseMonster :: RunTask(pTask);
		}
	}
}

//=========================================================
// LeapTouch - this is the headcrab's touch function when it
// is in the air
//=========================================================
void CChumToad :: LeapTouch ( CBaseEntity *pOther )
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
		EMIT_SOUND_DYN( edict(), CHAN_WEAPON, RANDOM_SOUND_ARRAY(pBiteSounds), GetSoundVolue(), ATTN_IDLE, 0, GetVoicePitch() );

		pOther->TakeDamage( pev, pev, GetDamageAmount(), DMG_POISON );
	}

	SetTouch( NULL );
}

//=========================================================
// PrescheduleThink
//=========================================================
void CChumToad :: PrescheduleThink ( void )
{
	// make the crab coo a little bit in combat state
	if ( m_MonsterState == MONSTERSTATE_COMBAT && RANDOM_FLOAT( 0, 5 ) < 0.1 )
	{
		IdleSound();
	}
}

void CChumToad :: StartTask ( Task_t *pTask )
{
	m_iTaskStatus = TASKSTATUS_RUNNING;

	switch ( pTask->iTask )
	{
	case TASK_RANGE_ATTACK1:
		{
			EMIT_SOUND_DYN( edict(), CHAN_WEAPON, pAttackSounds[0], GetSoundVolue(), ATTN_IDLE, 0, GetVoicePitch() );
			m_IdealActivity = ACT_RANGE_ATTACK1;
			SetTouch ( LeapTouch );
			break;
		}
	default:
		{
			CBaseMonster :: StartTask( pTask );
		}
	}
}


//=========================================================
// CheckRangeAttack1
//=========================================================
BOOL CChumToad :: CheckRangeAttack1 ( float flDot, float flDist )
{
	if ( FBitSet( pev->flags, FL_ONGROUND ) && flDist <= 192 && flDot >= 0.65 )
	{
		return TRUE;
	}
	return FALSE;
}

//=========================================================
// IdleSound
//=========================================================
#define CRAB_ATTN_IDLE (float)1.5
void CChumToad :: IdleSound ( void )
{
	EMIT_SOUND_DYN( edict(), CHAN_VOICE, RANDOM_SOUND_ARRAY(pIdleSounds), GetSoundVolue(), ATTN_IDLE, 0, GetVoicePitch() );
}

//=========================================================
// AlertSound
//=========================================================
void CChumToad :: AlertSound ( void )
{
	EMIT_SOUND_DYN( edict(), CHAN_VOICE, RANDOM_SOUND_ARRAY(pAlertSounds), GetSoundVolue(), ATTN_IDLE, 0, GetVoicePitch() );
}

//=========================================================
// AlertSound
//=========================================================
void CChumToad :: PainSound ( void )
{
	EMIT_SOUND_DYN( edict(), CHAN_VOICE, RANDOM_SOUND_ARRAY(pPainSounds), GetSoundVolue(), ATTN_IDLE, 0, GetVoicePitch() );
}

//=========================================================
// DeathSound
//=========================================================
void CChumToad :: DeathSound ( void )
{
	EMIT_SOUND_DYN( edict(), CHAN_VOICE, RANDOM_SOUND_ARRAY(pDeathSounds), GetSoundVolue(), ATTN_IDLE, 0, GetVoicePitch() );
}

//=========================================================
// StepSound
//=========================================================
void CChumToad :: StepSound ( void )
{
	EMIT_SOUND_DYN( edict(), CHAN_VOICE, RANDOM_SOUND_ARRAY(pStepSounds), GetSoundVolue(), ATTN_IDLE, 0, GetVoicePitch() );
}

Schedule_t* CChumToad :: GetScheduleOfType ( int Type )
{
	switch	( Type )
	{
		case SCHED_RANGE_ATTACK1:
		{
			return &slCTRangeAttack1[ 0 ];
		}
		break;
	}

	return CBaseMonster::GetScheduleOfType( Type );
}

//=========================================================
// RunAI - blinks with pev->skin, pev->iuser1 is the state
//=========================================================
void CChumToad :: RunAI( void )
{
	CBaseMonster::RunAI();

	if ( pev->skin )
	{
		if ( pev->skin == 1 )
		{
			if ( pev->iuser1 == 1 )
				pev->skin = 2;
			else if ( pev->iuser1 == 2 )
				pev->skin = 0;
		}
		else if ( pev->skin == 2 )
		{
			pev->skin = 1;
			pev->iuser1 = 2;
		}
	}
	else if ( RANDOM_LONG( 0, 19 ) == 0 )
	{
		pev->skin = 1;
		pev->iuser1 = 1;
	}
}

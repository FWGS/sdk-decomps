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
// headcrab.cpp - tiny, jumpy alien parasite
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

Task_t	tlHCRangeAttack1[] =
{
	{ TASK_STOP_MOVING,			(float)0		},
	{ TASK_FACE_IDEAL,			(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE	},
	{ TASK_FACE_IDEAL,			(float)0		},
	{ TASK_WAIT_RANDOM,			(float)0.5		},
};

Schedule_t	slHCRangeAttack1[] =
{
	{ 
		tlHCRangeAttack1,
		ARRAYSIZE ( tlHCRangeAttack1 ), 
		bits_COND_ENEMY_OCCLUDED	|
		bits_COND_NO_AMMO_LOADED,
		0,
		"HCRangeAttack1"
	},
};

Task_t	tlHCRangeAttack1Fast[] =
{
	{ TASK_STOP_MOVING,			(float)0		},
	{ TASK_FACE_IDEAL,			(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE	},
};

Schedule_t	slHCRangeAttack1Fast[] =
{
	{ 
		tlHCRangeAttack1Fast,
		ARRAYSIZE ( tlHCRangeAttack1Fast ), 
		bits_COND_ENEMY_OCCLUDED	|
		bits_COND_NO_AMMO_LOADED,
		0,
		"HCRAFast"
	},
};

class CHeadCrab : public CBaseMonster
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
	// They Hunger: the three attack sounds, for the hand and the chicken
	virtual void einar_AttackSoundZero( void );
	virtual void einar_AttackSoundNotZero( void );
	virtual void einar_BiteSound( void );
	void PrescheduleThink( void );
	int  Classify ( void );
	void HandleAnimEvent( MonsterEvent_t *pEvent );
	BOOL CheckRangeAttack1 ( float flDot, float flDist );
	BOOL CheckRangeAttack2 ( float flDot, float flDist );
	int TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );

	virtual float GetDamageAmount( void ) { return gSkillData.headcrabDmgBite; }
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
};
LINK_ENTITY_TO_CLASS( monster_headcrab, CHeadCrab );

DEFINE_CUSTOM_SCHEDULES( CHeadCrab )
{
	slHCRangeAttack1,
	slHCRangeAttack1Fast,
};

IMPLEMENT_CUSTOM_SCHEDULES( CHeadCrab, CBaseMonster );

const char *CHeadCrab::pIdleSounds[] = 
{
	"headcrab/hc_idle1.wav",
	"headcrab/hc_idle2.wav",
	"headcrab/hc_idle3.wav",
};
const char *CHeadCrab::pAlertSounds[] = 
{
	"headcrab/hc_alert1.wav",
};
const char *CHeadCrab::pPainSounds[] = 
{
	"headcrab/hc_pain1.wav",
	"headcrab/hc_pain2.wav",
	"headcrab/hc_pain3.wav",
};
const char *CHeadCrab::pAttackSounds[] = 
{
	"headcrab/hc_attack1.wav",
	"headcrab/hc_attack2.wav",
	"headcrab/hc_attack3.wav",
};

const char *CHeadCrab::pDeathSounds[] = 
{
	"headcrab/hc_die1.wav",
	"headcrab/hc_die2.wav",
};

const char *CHeadCrab::pBiteSounds[] = 
{
	"headcrab/hc_headbite.wav",
};

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CHeadCrab :: Classify ( void )
{
	return	CLASS_ALIEN_PREY;
}

//=========================================================
// Center - returns the real center of the headcrab.  The 
// bounding box is much larger than the actual creature so 
// this is needed for targeting
//=========================================================
Vector CHeadCrab :: Center ( void )
{
	return Vector( pev->origin.x, pev->origin.y, pev->origin.z + 6 );
}


Vector CHeadCrab :: BodyTarget( const Vector &posSrc ) 
{ 
	return Center( );
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CHeadCrab :: SetYawSpeed ( void )
{
	int ys;

	switch ( m_Activity )
	{
	case ACT_IDLE:			
		ys = 30;
		break;
	case ACT_RUN:			
	case ACT_WALK:			
		ys = 20;
		break;
	case ACT_TURN_LEFT:
	case ACT_TURN_RIGHT:
		ys = 60;
		break;
	case ACT_RANGE_ATTACK1:	
		ys = 30;
		break;
	default:
		ys = 30;
		break;
	}

	pev->yaw_speed = ys;
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CHeadCrab :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	switch( pEvent->event )
	{
		case HC_AE_JUMPATTACK:
		{
			ClearBits( pev->flags, FL_ONGROUND );

			UTIL_SetOrigin (pev, pev->origin + Vector ( 0 , 0 , 1) );// take him off ground so engine doesn't instantly reset onground 
			UTIL_MakeVectors ( pev->angles );

			Vector vecJumpDir;
			if (m_hEnemy != NULL)
			{
				float gravity = CVAR_GET_FLOAT( "sv_gravity" );
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

			einar_AttackSoundNotZero();

			pev->velocity = vecJumpDir;
			m_flNextAttack = gpGlobals->time + 2;
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
void CHeadCrab :: Spawn()
{
	Precache( );

	SET_MODEL(ENT(pev), "models/headcrab.mdl");
	UTIL_SetSize(pev, Vector(-12, -12, 0), Vector(12, 12, 24));

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= BLOOD_COLOR_RED;
	pev->effects		= 0;
	pev->health			= gSkillData.headcrabHealth;
	pev->view_ofs		= Vector ( 0, 0, 20 );// position of the eyes relative to monster's origin.
	pev->yaw_speed		= 5;//!!! should we put this in the monster's changeanim function since turn rates may vary with state/anim?
	m_flFieldOfView		= 0.5;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState		= MONSTERSTATE_NONE;

	MonsterInit();
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CHeadCrab :: Precache()
{
	PRECACHE_SOUND_ARRAY(pIdleSounds);
	PRECACHE_SOUND_ARRAY(pAlertSounds);
	PRECACHE_SOUND_ARRAY(pPainSounds);
	PRECACHE_SOUND_ARRAY(pAttackSounds);
	PRECACHE_SOUND_ARRAY(pDeathSounds);
	PRECACHE_SOUND_ARRAY(pBiteSounds);

	PRECACHE_MODEL("models/headcrab.mdl");
}	


//=========================================================
// RunTask 
//=========================================================
void CHeadCrab :: RunTask ( Task_t *pTask )
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
void CHeadCrab :: LeapTouch ( CBaseEntity *pOther )
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
		einar_BiteSound();
		
		pOther->TakeDamage( pev, pev, GetDamageAmount(), DMG_SLASH );
	}

	SetTouch( NULL );
}

//=========================================================
// PrescheduleThink
//=========================================================
void CHeadCrab :: PrescheduleThink ( void )
{
	// make the crab coo a little bit in combat state
	if ( m_MonsterState == MONSTERSTATE_COMBAT && RANDOM_FLOAT( 0, 5 ) < 0.1 )
	{
		IdleSound();
	}
}

void CHeadCrab :: StartTask ( Task_t *pTask )
{
	m_iTaskStatus = TASKSTATUS_RUNNING;

	switch ( pTask->iTask )
	{
	case TASK_RANGE_ATTACK1:
		{
			einar_AttackSoundZero();
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
BOOL CHeadCrab :: CheckRangeAttack1 ( float flDot, float flDist )
{
	if ( FBitSet( pev->flags, FL_ONGROUND ) && flDist <= 256 && flDot >= 0.65 )
	{
		return TRUE;
	}
	return FALSE;
}

//=========================================================
// CheckRangeAttack2
//=========================================================
BOOL CHeadCrab :: CheckRangeAttack2 ( float flDot, float flDist )
{
	return FALSE;
	// BUGBUG: Why is this code here?  There is no ACT_RANGE_ATTACK2 animation.  I've disabled it for now.
#if 0
	if ( FBitSet( pev->flags, FL_ONGROUND ) && flDist > 64 && flDist <= 256 && flDot >= 0.5 )
	{
		return TRUE;
	}
	return FALSE;
#endif
}

int CHeadCrab :: TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	// Don't take any acid damage -- BigMomma's mortar is acid
	if ( bitsDamageType & DMG_ACID )
		flDamage = 0;

	return CBaseMonster::TakeDamage( pevInflictor, pevAttacker, flDamage, bitsDamageType );
}

//=========================================================
// IdleSound
//=========================================================
#define CRAB_ATTN_IDLE (float)1.5
void CHeadCrab :: IdleSound ( void )
{
	EMIT_SOUND_DYN( edict(), CHAN_VOICE, RANDOM_SOUND_ARRAY(pIdleSounds), GetSoundVolue(), ATTN_IDLE, 0, GetVoicePitch() );
}

//=========================================================
// AlertSound 
//=========================================================
void CHeadCrab :: AlertSound ( void )
{
	EMIT_SOUND_DYN( edict(), CHAN_VOICE, RANDOM_SOUND_ARRAY(pAlertSounds), GetSoundVolue(), ATTN_IDLE, 0, GetVoicePitch() );
}

//=========================================================
// AlertSound 
//=========================================================
void CHeadCrab :: PainSound ( void )
{
	EMIT_SOUND_DYN( edict(), CHAN_VOICE, RANDOM_SOUND_ARRAY(pPainSounds), GetSoundVolue(), ATTN_IDLE, 0, GetVoicePitch() );
}

//=========================================================
// DeathSound 
//=========================================================
void CHeadCrab :: DeathSound ( void )
{
	EMIT_SOUND_DYN( edict(), CHAN_VOICE, RANDOM_SOUND_ARRAY(pDeathSounds), GetSoundVolue(), ATTN_IDLE, 0, GetVoicePitch() );
}

//=========================================================
// einar_AttackSoundZero - when the leap task starts
//=========================================================
void CHeadCrab :: einar_AttackSoundZero ( void )
{
	EMIT_SOUND_DYN( edict(), CHAN_WEAPON, pAttackSounds[0], GetSoundVolue(), ATTN_IDLE, 0, GetVoicePitch() );
}

//=========================================================
// einar_AttackSoundNotZero - at the jump event
//=========================================================
void CHeadCrab :: einar_AttackSoundNotZero ( void )
{
	int iSound = RANDOM_LONG(0,2);
	if ( iSound != 0 )
		EMIT_SOUND_DYN( edict(), CHAN_VOICE, pAttackSounds[iSound], GetSoundVolue(), ATTN_IDLE, 0, GetVoicePitch() );
}

//=========================================================
// einar_BiteSound - when the leap hits
//=========================================================
void CHeadCrab :: einar_BiteSound ( void )
{
	EMIT_SOUND_DYN( edict(), CHAN_WEAPON, RANDOM_SOUND_ARRAY(pBiteSounds), GetSoundVolue(), ATTN_IDLE, 0, GetVoicePitch() );
}

Schedule_t* CHeadCrab :: GetScheduleOfType ( int Type )
{
	switch	( Type )
	{
		case SCHED_RANGE_ATTACK1:
		{
			return &slHCRangeAttack1[ 0 ];
		}
		break;
	}

	return CBaseMonster::GetScheduleOfType( Type );
}


class CBabyCrab : public CHeadCrab
{
public:
	void Spawn( void );
	void Precache( void );
	void SetYawSpeed ( void );
	float GetDamageAmount( void ) { return gSkillData.headcrabDmgBite * 0.3; }
	BOOL CheckRangeAttack1 ( float flDot, float flDist );
	Schedule_t* GetScheduleOfType ( int Type );
	virtual int GetVoicePitch( void ) { return PITCH_NORM + RANDOM_LONG(40,50); }
	virtual float GetSoundVolue( void ) { return 0.8; }
};
LINK_ENTITY_TO_CLASS( monster_babycrab, CBabyCrab );

void CBabyCrab :: Spawn( void )
{
	CHeadCrab::Spawn();
	SET_MODEL(ENT(pev), "models/baby_headcrab.mdl");
	pev->rendermode = kRenderTransTexture;
	pev->renderamt = 192;
	UTIL_SetSize(pev, Vector(-12, -12, 0), Vector(12, 12, 24));
	
	pev->health	= gSkillData.headcrabHealth * 0.25;	// less health than full grown
}

void CBabyCrab :: Precache( void )
{
	PRECACHE_MODEL( "models/baby_headcrab.mdl" );
	CHeadCrab::Precache();
}


void CBabyCrab :: SetYawSpeed ( void )
{
	pev->yaw_speed = 120;
}


BOOL CBabyCrab :: CheckRangeAttack1( float flDot, float flDist )
{
	if ( pev->flags & FL_ONGROUND )
	{
		if ( pev->groundentity && (pev->groundentity->v.flags & (FL_CLIENT|FL_MONSTER)) )
			return TRUE;

		// A little less accurate, but jump from closer
		if ( flDist <= 180 && flDot >= 0.55 )
			return TRUE;
	}

	return FALSE;
}


Schedule_t* CBabyCrab :: GetScheduleOfType ( int Type )
{
	switch( Type )
	{
		case SCHED_FAIL:	// If you fail, try to jump!
			if ( m_hEnemy != NULL )
				return slHCRangeAttack1Fast;
		break;

		case SCHED_RANGE_ATTACK1:
		{
			return slHCRangeAttack1Fast;
		}
		break;
	}

	return CHeadCrab::GetScheduleOfType( Type );
}


//=========================================================
// They Hunger: the hand, a headcrab with twice the health
// and bite that makes no voice sounds
//=========================================================
class CEinarHand : public CHeadCrab
{
public:
	void Spawn( void );
	void Precache( void );
	void SetYawSpeed ( void );
	int  Classify ( void );
	float GetDamageAmount( void ) { return gSkillData.headcrabDmgBite * 2; }
	void DeathSound( void ) { }
	void AlertSound( void ) { }
	void IdleSound( void ) { }
	void PainSound( void ) { }
	void einar_AttackSoundNotZero( void ) { }
	void einar_AttackSoundZero( void );
	void einar_BiteSound( void );
};
LINK_ENTITY_TO_CLASS( einar_hand, CEinarHand );

//=========================================================
// Spawn
//=========================================================
void CEinarHand :: Spawn()
{
	Precache( );

	SET_MODEL(ENT(pev), "models/thehand.mdl");
	UTIL_SetSize(pev, Vector(-12, -12, 0), Vector(12, 12, 24));

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= BLOOD_COLOR_RED;
	pev->effects		= 0;
	pev->health			= gSkillData.headcrabHealth * 2;
	pev->view_ofs		= Vector ( 0, 0, 20 );// position of the eyes relative to monster's origin.
	pev->yaw_speed		= 5;//!!! should we put this in the monster's changeanim function since turn rates may vary with state/anim?
	m_flFieldOfView		= 0.5;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState		= MONSTERSTATE_NONE;

	MonsterInit();
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CEinarHand :: Precache()
{
	PRECACHE_SOUND("thehand/hnd_attack1.wav");
	PRECACHE_SOUND("thehand/hnd_headbite.wav");

	PRECACHE_MODEL("models/thehand.mdl");
}	

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CEinarHand :: SetYawSpeed ( void )
{
	int ys;

	switch ( m_Activity )
	{
	case ACT_IDLE:			
		ys = 60;
		break;
	case ACT_RUN:			
	case ACT_WALK:			
		ys = 30;
		break;
	case ACT_TURN_LEFT:
	case ACT_TURN_RIGHT:
		ys = 120;
		break;
	case ACT_RANGE_ATTACK1:	
		ys = 60;
		break;
	default:
		ys = 60;
		break;
	}

	pev->yaw_speed = ys;
}

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CEinarHand :: Classify ( void )
{
	return	CLASS_ALIEN_MONSTER;
}

//=========================================================
// einar_AttackSoundZero 
//=========================================================
void CEinarHand :: einar_AttackSoundZero ( void )
{
	EMIT_SOUND_DYN( edict(), CHAN_WEAPON, "thehand/hnd_attack1.wav", GetSoundVolue(), ATTN_IDLE, 0, GetVoicePitch() );
}

//=========================================================
// einar_BiteSound 
//=========================================================
void CEinarHand :: einar_BiteSound ( void )
{
	EMIT_SOUND_DYN( edict(), CHAN_WEAPON, "thehand/hnd_headbite.wav", GetSoundVolue(), ATTN_IDLE, 0, GetVoicePitch() );
}

//=========================================================
// They Hunger: the chicken, a headcrab that can leap again
// at once
//=========================================================
class CEinarChicken : public CHeadCrab
{
public:
	void Spawn( void );
	void Precache( void );
	void SetYawSpeed ( void );
	int  Classify ( void );
	void HandleAnimEvent( MonsterEvent_t *pEvent );
	void DeathSound( void ) { }
	void IdleSound( void );
	void AlertSound( void );
	void PainSound( void );
	void einar_AttackSoundZero( void );
	void einar_AttackSoundNotZero( void );
	void einar_BiteSound( void );

	static const char *pEinarChickenIdleSounds[];
	static const char *pEinarChickenAlertSounds[];
	static const char *pEinarChickenPainSounds[];
	static const char *pEinarChickenAttackSounds[];
};
LINK_ENTITY_TO_CLASS( monster_th_chicken, CEinarChicken );

const char *CEinarChicken::pEinarChickenIdleSounds[] = 
{
	"chicken/ch_idle1.wav",
	"chicken/ch_idle2.wav",
};
const char *CEinarChicken::pEinarChickenAlertSounds[] = 
{
	"chicken/ch_alert1.wav",
	"chicken/ch_alert2.wav",
};
const char *CEinarChicken::pEinarChickenPainSounds[] = 
{
	"chicken/ch_attack2.wav",// sic
	"chicken/ch_pain2.wav",
	"chicken/ch_die1.wav",
	"chicken/ch_die2.wav",
};
const char *CEinarChicken::pEinarChickenAttackSounds[] = 
{
	"chicken/ch_attack1.wav",
	"chicken/ch_pain1.wav",// sic
};

//=========================================================
// HandleAnimEvent - the headcrab's, without the two seconds
// to the next attack
//=========================================================
void CEinarChicken :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	switch( pEvent->event )
	{
		case HC_AE_JUMPATTACK:
		{
			ClearBits( pev->flags, FL_ONGROUND );

			UTIL_SetOrigin (pev, pev->origin + Vector ( 0 , 0 , 1) );// take him off ground so engine doesn't instantly reset onground 
			UTIL_MakeVectors ( pev->angles );

			Vector vecJumpDir;
			if (m_hEnemy != NULL)
			{
				float gravity = CVAR_GET_FLOAT( "sv_gravity" );
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

			einar_AttackSoundNotZero();

			pev->velocity = vecJumpDir;
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
void CEinarChicken :: Spawn()
{
	Precache( );

	SET_MODEL(ENT(pev), "models/chicken.mdl");
	UTIL_SetSize(pev, Vector(-12, -12, 0), Vector(12, 12, 24));

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= BLOOD_COLOR_RED;
	pev->effects		= 0;
	pev->health			= gSkillData.headcrabHealth;
	pev->view_ofs		= Vector ( 0, 0, 20 );// position of the eyes relative to monster's origin.
	pev->yaw_speed		= 5;//!!! should we put this in the monster's changeanim function since turn rates may vary with state/anim?
	m_flFieldOfView		= 0.5;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState		= MONSTERSTATE_NONE;

	MonsterInit();
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CEinarChicken :: Precache()
{
	PRECACHE_SOUND("thehand/hnd_attack1.wav");
	PRECACHE_SOUND("thehand/hnd_headbite.wav");

	PRECACHE_SOUND_ARRAY(pEinarChickenIdleSounds);
	PRECACHE_SOUND_ARRAY(pEinarChickenAlertSounds);
	PRECACHE_SOUND_ARRAY(pEinarChickenPainSounds);
	PRECACHE_SOUND_ARRAY(pEinarChickenAttackSounds);

	PRECACHE_MODEL("models/chicken.mdl");
}	

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CEinarChicken :: SetYawSpeed ( void )
{
	int ys;

	switch ( m_Activity )
	{
	case ACT_IDLE:			
		ys = 60;
		break;
	case ACT_RUN:			
	case ACT_WALK:			
		ys = 30;
		break;
	case ACT_TURN_LEFT:
	case ACT_TURN_RIGHT:
		ys = 120;
		break;
	case ACT_RANGE_ATTACK1:	
		ys = 60;
		break;
	default:
		ys = 60;
		break;
	}

	pev->yaw_speed = ys;
}

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CEinarChicken :: Classify ( void )
{
	return	CLASS_ALIEN_MONSTER;
}

//=========================================================
// IdleSound
//=========================================================
void CEinarChicken :: IdleSound ( void )
{
	EMIT_SOUND_DYN( edict(), CHAN_VOICE, RANDOM_SOUND_ARRAY(pEinarChickenIdleSounds), GetSoundVolue(), ATTN_IDLE, 0, GetVoicePitch() );
}

//=========================================================
// AlertSound 
//=========================================================
void CEinarChicken :: AlertSound ( void )
{
	EMIT_SOUND_DYN( edict(), CHAN_VOICE, RANDOM_SOUND_ARRAY(pEinarChickenAlertSounds), GetSoundVolue(), ATTN_IDLE, 0, GetVoicePitch() );
}

//=========================================================
// PainSound 
//=========================================================
void CEinarChicken :: PainSound ( void )
{
	EMIT_SOUND_DYN( edict(), CHAN_VOICE, RANDOM_SOUND_ARRAY(pEinarChickenPainSounds), GetSoundVolue(), ATTN_IDLE, 0, GetVoicePitch() );
}

//=========================================================
// einar_AttackSoundZero - one leap in ten starts with an idle sound
//=========================================================
void CEinarChicken :: einar_AttackSoundZero ( void )
{
	if ( RANDOM_LONG(0,9) == 0 )
		IdleSound();
}

//=========================================================
// einar_AttackSoundNotZero 
//=========================================================
void CEinarChicken :: einar_AttackSoundNotZero ( void )
{
	if ( RANDOM_LONG(0,1) == 0 )
		EMIT_SOUND_DYN( edict(), CHAN_VOICE, RANDOM_SOUND_ARRAY(pEinarChickenAttackSounds), GetSoundVolue(), ATTN_IDLE, 0, GetVoicePitch() );

	EMIT_SOUND_DYN( edict(), CHAN_WEAPON, "thehand/hnd_attack1.wav", GetSoundVolue(), ATTN_IDLE, 0, GetVoicePitch() );
}

//=========================================================
// einar_BiteSound 
//=========================================================
void CEinarChicken :: einar_BiteSound ( void )
{
	if ( RANDOM_LONG(0,9) == 0 )
		EMIT_SOUND_DYN( edict(), CHAN_VOICE, "chicken/ch_alert1.wav", GetSoundVolue(), ATTN_IDLE, 0, GetVoicePitch() );

	EMIT_SOUND_DYN( edict(), CHAN_WEAPON, "thehand/hnd_headbite.wav", GetSoundVolue(), ATTN_IDLE, 0, GetVoicePitch() );
}

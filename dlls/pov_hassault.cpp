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
// POV: monster_human_assault, a soldier with a minigun.
// The readme credits Sweet Half-Life's human assault: the
// schedules, SetYawSpeed, GetScheduleOfType and SetActivity
// are Sweet Half-Life's; the gun (spin up, fire, spin down
// with the shells), the sounds and the rest are POV's.
// File name and the names retail doesn't give are ours.
//=========================================================

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"schedule.h"
#include	"animation.h"
#include	"weapons.h"
#include	"soundent.h"

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define		HASSAULT_AE_DROP_GUN	( 10 )	// events 1-9 fire the gun

#define		HASSAULT_BODY_GUN		1	// bodygroup
#define		HASSAULT_BODY_GUNGONE	1

// gun states (m_iWeaponState; names ours)
#define		HASSAULT_GUN_IDLE		0
#define		HASSAULT_GUN_SPINUP		1
#define		HASSAULT_GUN_FIRE		2
#define		HASSAULT_GUN_SPINDOWN	3	// never set

//=========================================================
// monster-specific schedule types
//=========================================================
enum
{
	SCHED_HASSAULT_ESTABLISH_LINE_OF_FIRE = LAST_COMMON_SCHEDULE + 1,
};

class CHAssault : public CBaseMonster
{
public:
	void Spawn( void );
	void Precache( void );
	void SetYawSpeed( void );
	int  ISoundMask( void );
	void AlertSound( void );
	int  Classify ( void );
	void HandleAnimEvent( MonsterEvent_t *pEvent );
	void SetActivity ( Activity NewActivity );
	void GibMonster( void );

	BOOL CheckRangeAttack1 ( float flDot, float flDist );

	void SpinUp( void );
	void FireGun( void );
	void SpinDown( void );
	void Shoot( void );

	// Override these to set behavior
	Schedule_t *GetScheduleOfType ( int Type );
	Schedule_t *GetSchedule ( void );

	void DeathSound( void );
	void PainSound( void );

	void Killed( entvars_t *pevAttacker, int iGib );

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	float	m_flPainTime;
	float	m_flCheckTargetTime;
	BOOL	m_bLastAttackCheck;
	float	m_flNextFireTime;
	int		m_iWeaponState;
	int		m_iShell;	// name ours

	CUSTOM_SCHEDULES;
};

LINK_ENTITY_TO_CLASS( monster_human_assault, CHAssault );

TYPEDESCRIPTION	CHAssault::m_SaveData[] =
{
	DEFINE_FIELD( CHAssault, m_flPainTime, FIELD_TIME ),
	DEFINE_FIELD( CHAssault, m_flCheckTargetTime, FIELD_TIME ),
	DEFINE_FIELD( CHAssault, m_iWeaponState, FIELD_INTEGER ),
	DEFINE_FIELD( CHAssault, m_bLastAttackCheck, FIELD_BOOLEAN ),
	DEFINE_FIELD( CHAssault, m_flNextFireTime, FIELD_TIME ),
};

IMPLEMENT_SAVERESTORE( CHAssault, CBaseMonster );

//=========================================================
// AI Schedules Specific to this monster
//=========================================================

//=========================================================
// Fail
//=========================================================
Task_t	tlHAssaultFail[] =
{
	{ TASK_STOP_MOVING,			0				},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE },
	{ TASK_WAIT,				(float)2		},
	{ TASK_WAIT_PVS,			(float)0		},
};

Schedule_t	slHAssaultFail[] =
{
	{
		tlHAssaultFail,
		ARRAYSIZE ( tlHAssaultFail ),
		bits_COND_CAN_RANGE_ATTACK1,
		0,
		"HAssault Fail"
	},
};

//=========================================================
// Combat Fail
//=========================================================
Task_t	tlHAssaultCombatFail[] =
{
	{ TASK_STOP_MOVING,			0				},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE },
	{ TASK_WAIT_FACE_ENEMY,		(float)2		},
	{ TASK_WAIT_PVS,			(float)0		},
};

Schedule_t	slHAssaultCombatFail[] =
{
	{
		tlHAssaultCombatFail,
		ARRAYSIZE ( tlHAssaultCombatFail ),
		bits_COND_CAN_RANGE_ATTACK1,
		0,
		"HAssault Combat Fail"
	},
};

//=========================================================
// primary range attack
//=========================================================
Task_t	tlHAssaultRangeAttack1[] =
{
	{ TASK_STOP_MOVING,			(float)0		},
	{ TASK_FACE_IDEAL,			(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
};

Schedule_t	slHAssaultRangeAttack1[] =
{
	{
		tlHAssaultRangeAttack1,
		ARRAYSIZE ( tlHAssaultRangeAttack1 ),
		bits_COND_NEW_ENEMY			|
		bits_COND_ENEMY_DEAD		|
		bits_COND_HEAVY_DAMAGE		|
		bits_COND_HEAR_SOUND,

		bits_SOUND_DANGER,
		"HAssaultEstablishLineOfFire"	// sic: the next schedule's name
	},
};

//=========================================================
// Establish line of fire - move to a position that allows
// the grunt to attack.
//=========================================================
Task_t tlHAssaultEstablishLineOfFire[] =
{
	{ TASK_GET_PATH_TO_ENEMY,	(float)0		},
	{ TASK_RUN_PATH,			(float)0		},
	{ TASK_WAIT_FOR_MOVEMENT,	(float)0		},
};

Schedule_t slHAssaultEstablishLineOfFire[] =
{
	{
		tlHAssaultEstablishLineOfFire,
		ARRAYSIZE ( tlHAssaultEstablishLineOfFire ),
		bits_COND_NEW_ENEMY			|
		bits_COND_ENEMY_DEAD		|
		bits_COND_CAN_RANGE_ATTACK1	|
		bits_COND_HEAR_SOUND,

		bits_SOUND_DANGER,
		"HAssaultEstablishLineOfFire"
	},
};

DEFINE_CUSTOM_SCHEDULES( CHAssault )
{
	slHAssaultFail,
	slHAssaultCombatFail,
	slHAssaultRangeAttack1,
	slHAssaultEstablishLineOfFire,
};

IMPLEMENT_CUSTOM_SCHEDULES( CHAssault, CBaseMonster );

//=========================================================
// GibMonster - throw the gun if it's still in his hands
//=========================================================
void CHAssault :: GibMonster ( void )
{
	if ( GetBodygroup( HASSAULT_BODY_GUN ) == 0 )
	{
		UTIL_DropGun( this, "weapon_vulcan", TRUE );
	}

	CBaseMonster :: GibMonster();
}

//=========================================================
// ISoundMask - returns a bit mask indicating which types
// of sounds this monster regards.
//=========================================================
int CHAssault :: ISoundMask ( void)
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
int	CHAssault :: Classify ( void )
{
	return	CLASS_HUMAN_MILITARY;
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CHAssault :: SetYawSpeed ( void )
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
	case ACT_TURN_LEFT:
	case ACT_TURN_RIGHT:
		ys = 110;
		break;
	case ACT_RANGE_ATTACK1:
		ys = 40;
	default:
		ys = 70;
		break;
	}

	pev->yaw_speed = ys;
}

//=========================================================
// SpinUp - start the barrels turning
//=========================================================
void CHAssault :: SpinUp ( void )
{
	if ( m_iWeaponState != HASSAULT_GUN_FIRE && m_iWeaponState != HASSAULT_GUN_SPINUP )
	{
		m_iWeaponState = HASSAULT_GUN_SPINUP;
		EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, "hassault/hw_spinup2.wav", 1, ATTN_NORM, 0, 100 );
		pev->effects = 0;
		m_flNextFireTime = gpGlobals->time + 0.2;
		m_flCheckTargetTime = gpGlobals->time + 0.1;
	}
}

//=========================================================
// FireGun - the fire anim events: once spun up, a shot
//=========================================================
void CHAssault :: FireGun ( void )
{
	if ( m_iWeaponState != HASSAULT_GUN_IDLE && m_iWeaponState != HASSAULT_GUN_SPINDOWN && m_flNextFireTime <= gpGlobals->time )
	{
		if ( m_iWeaponState == HASSAULT_GUN_SPINUP )
		{
			EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, "hassault/hw_fire2.wav", 1, ATTN_NORM, 0, 100 );
			m_iWeaponState = HASSAULT_GUN_FIRE;
			m_flNextFireTime = gpGlobals->time - 0.1;
			return;
		}

		EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, "hassault/hw_fire2.wav", 1, ATTN_NORM, 0, 100 );
		Shoot();
		pev->nextthink = gpGlobals->time + 0.1;
	}
}

//=========================================================
// SpinDown - stop the barrels, throw out a shell
//=========================================================
void CHAssault :: SpinDown ( void )
{
	if ( m_iWeaponState != HASSAULT_GUN_IDLE && m_iWeaponState != HASSAULT_GUN_SPINDOWN )
	{
		Vector vecShootOrigin;
		Vector vecShootDir;

		GetAttachment( 0, vecShootOrigin, vecShootDir );
		vecShootDir = ShootAtEnemy( vecShootOrigin );

		Vector	vecShellVelocity = gpGlobals->v_right * RANDOM_FLOAT(40,90) + gpGlobals->v_up * RANDOM_FLOAT(75,200) + gpGlobals->v_forward * RANDOM_FLOAT(-40, 40);
		EjectBrass ( vecShootOrigin - vecShootDir * 24, vecShellVelocity, pev->angles.y, m_iShell, TE_BOUNCE_SHELL );

		EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, "hassault/hw_spindown.wav", 1, ATTN_NORM, 0, 100 );
		m_iWeaponState = HASSAULT_GUN_IDLE;
	}
}

//=========================================================
// CheckRangeAttack1
//=========================================================
BOOL CHAssault :: CheckRangeAttack1 ( float flDot, float flDist )
{
	// while firing, keep the last answer for a while
	if ( m_iWeaponState == HASSAULT_GUN_FIRE && m_flCheckTargetTime > gpGlobals->time )
		return m_bLastAttackCheck;

	m_bLastAttackCheck = FALSE;

	if ( flDist <= m_flDistTooFar && flDot >= 0.5 )
	{
		TraceResult tr;

		Vector shootOrigin = pev->origin + Vector( 0, 0, 55 );
		CBaseEntity *pEnemy = m_hEnemy;
		Vector shootTarget = ( (pEnemy->BodyTarget( shootOrigin ) - pEnemy->pev->origin) + m_vecEnemyLKP );
		UTIL_TraceLine( shootOrigin, shootTarget, dont_ignore_monsters, dont_ignore_glass, ENT(pev), &tr );
		if ( tr.flFraction == 1.0 || (tr.pHit != NULL && CBaseEntity::Instance(tr.pHit) == pEnemy) )
			m_bLastAttackCheck = TRUE;
		m_flCheckTargetTime = gpGlobals->time + 0.5;
	}

	return m_bLastAttackCheck;
}

//=========================================================
// Shoot - one round from the minigun
//=========================================================
void CHAssault :: Shoot ( void )
{
	Vector vecShootOrigin;
	Vector vecShootDir;

	GetAttachment( 0, vecShootOrigin, vecShootDir );
	vecShootDir = ShootAtEnemy( vecShootOrigin );

	Vector angDir = UTIL_VecToAngles( vecShootDir );
	SetBlending( 0, angDir.x );
	pev->effects |= EF_MUZZLEFLASH;
	UTIL_MuzzleLight( this, vecShootOrigin );

	Vector	vecShellVelocity = gpGlobals->v_right * RANDOM_FLOAT(40,90) + gpGlobals->v_up * RANDOM_FLOAT(75,200) + gpGlobals->v_forward * RANDOM_FLOAT(-40, 40);
	EjectBrass ( vecShootOrigin - vecShootDir * 24, vecShellVelocity, pev->angles.y, m_iShell, TE_BOUNCE_NULL );
	FireBullets( 1, vecShootOrigin, vecShootDir, VECTOR_CONE_15DEGREES, m_flDistTooFar, BULLET_MONSTER_9MM, 3 );

	CSoundEnt::InsertSound ( bits_SOUND_COMBAT, pev->origin, 600, 0.1 );
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//
// Returns number of events handled, 0 if none.
//=========================================================
void CHAssault :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	switch( pEvent->event )
	{
	case 1:	// the fire events (anim events 1-9)
	case 2:
	case 3:
	case 4:
	case 5:
	case 6:
	case 7:
	case 8:
	case 9:
		FireGun();
		break;

	case HASSAULT_AE_DROP_GUN:
		UTIL_DropGun( this, "weapon_vulcan", TRUE );
		SetBodygroup( HASSAULT_BODY_GUN, HASSAULT_BODY_GUNGONE );
		break;

	default:
		CBaseMonster::HandleAnimEvent( pEvent );
		break;
	}
}

//=========================================================
// Spawn
//=========================================================
void CHAssault :: Spawn()
{
	Precache( );

	SET_MODEL(ENT(pev), "models/hassault.mdl");
	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= BLOOD_COLOR_RED;
	pev->health			= gSkillData.hgruntHealth * 1.1;
	pev->view_ofs		= Vector ( 0, 0, 50 );// position of the eyes relative to monster's origin.
	m_flFieldOfView		= 0.2;
	m_MonsterState		= MONSTERSTATE_NONE;

	pev->body			= 0;
	m_iWeaponState		= HASSAULT_GUN_IDLE;
	m_bLastAttackCheck	= FALSE;

	m_afCapability		= bits_CAP_HEAR | bits_CAP_TURN_HEAD | bits_CAP_DOORS_GROUP;

	SetBodygroup( HASSAULT_BODY_GUN, 0 );

	MonsterInit();
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CHAssault :: Precache()
{
	PRECACHE_MODEL("models/hassault.mdl");

	PRECACHE_SOUND("hassault/hw_fire2.wav");
	PRECACHE_SOUND("hassault/hw_spinup2.wav");
	PRECACHE_SOUND("hassault/hw_spindown.wav");

	PRECACHE_SOUND("hassault/assault_die1.wav");
	PRECACHE_SOUND("hassault/assault_die2.wav");
	PRECACHE_SOUND("hassault/assault_die3.wav");

	PRECACHE_SOUND("hassault/assault_pain1.wav");
	PRECACHE_SOUND("hassault/assault_pain2.wav");
	PRECACHE_SOUND("hassault/assault_pain3.wav");

	PRECACHE_SOUND("hassault/assault_alert1.wav");
	PRECACHE_SOUND("hassault/assault_alert2.wav");

	m_iShell = PRECACHE_MODEL ("models/shell.mdl");// brass shell
}

//=========================================================
// AlertSound
//=========================================================
void CHAssault :: AlertSound( void )
{
	switch (RANDOM_LONG(0,1))
	{
	case 0: EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "hassault/assault_alert1.wav", 1, ATTN_NORM, 0, 100); break;
	case 1: EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "hassault/assault_alert2.wav", 1, ATTN_NORM, 0, 100); break;
	}
}

//=========================================================
// PainSound
//=========================================================
void CHAssault :: PainSound ( void )
{
	if (m_flPainTime > gpGlobals->time)
		return;

	switch (RANDOM_LONG(0,2))
	{
	case 0: EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "hassault/assault_pain1.wav", 1, ATTN_NORM, 0, 100); break;
	case 1: EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "hassault/assault_pain2.wav", 1, ATTN_NORM, 0, 100); break;
	case 2: EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "hassault/assault_pain3.wav", 1, ATTN_NORM, 0, 100); break;
	}

	m_flPainTime = gpGlobals->time + RANDOM_FLOAT(0.6, 0.8);
}

//=========================================================
// DeathSound
//=========================================================
void CHAssault :: DeathSound ( void )
{
	switch (RANDOM_LONG(0,2))
	{
	case 0: EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "hassault/assault_die1.wav", 1, ATTN_NORM, 0, 100); break;
	case 1: EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "hassault/assault_die2.wav", 1, ATTN_NORM, 0, 100); break;
	case 2: EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "hassault/assault_die3.wav", 1, ATTN_NORM, 0, 100); break;
	}
}

void CHAssault::Killed( entvars_t *pevAttacker, int iGib )
{
	if ( m_iWeaponState == HASSAULT_GUN_FIRE || m_iWeaponState == HASSAULT_GUN_SPINUP )
		SpinDown();

	CBaseMonster::Killed( pevAttacker, iGib );
}

//=========================================================
// SetActivity
//=========================================================
void CHAssault :: SetActivity ( Activity NewActivity )
{
	int	iSequence = ACTIVITY_NOT_AVAILABLE;
	void *pmodel = GET_MODEL_PTR( ENT(pev) );

	switch ( NewActivity)
	{
	case ACT_RANGE_ATTACK1:
		iSequence = LookupSequence( "attack" );
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
// AI Schedules Specific to this monster
//=========================================================

Schedule_t* CHAssault :: GetScheduleOfType ( int Type )
{
	switch( Type )
	{
	case SCHED_FAIL:
		if ( m_hEnemy != NULL )
			return slHAssaultCombatFail;
		return slHAssaultFail;

	case SCHED_RANGE_ATTACK1:
		return slHAssaultRangeAttack1;

	case SCHED_HASSAULT_ESTABLISH_LINE_OF_FIRE:
		return slHAssaultEstablishLineOfFire;

	default:
		return CBaseMonster::GetScheduleOfType( Type );
	}
}

//=========================================================
// GetSchedule - Decides which type of schedule best suits
// the monster's current state and conditions. Then calls
// monster's member function to get a pointer to a schedule
// of the proper type.
//=========================================================
Schedule_t *CHAssault :: GetSchedule ( void )
{
	switch( m_MonsterState )
	{
	case MONSTERSTATE_COMBAT:
		{
// dead enemy
			if ( HasConditions( bits_COND_ENEMY_DEAD ) )
			{
				SpinDown();
				// call base class, all code to handle dead enemies is centralized there.
				return CBaseMonster :: GetSchedule();
			}

			if ( HasConditions( bits_COND_NEW_ENEMY ) )
			{
				AlertSound();
				SpinUp();
			}

			if ( HasConditions( bits_COND_CAN_RANGE_ATTACK1 ) )
			{
				SpinUp();
				return GetScheduleOfType( SCHED_RANGE_ATTACK1 );
			}

			if ( HasConditions( bits_COND_ENEMY_OCCLUDED ) )
			{
				SpinDown();
				return GetScheduleOfType( SCHED_HASSAULT_ESTABLISH_LINE_OF_FIRE );
			}

			if ( HasConditions( bits_COND_SEE_ENEMY ) )
			{
				SpinDown();
				return GetScheduleOfType( SCHED_HASSAULT_ESTABLISH_LINE_OF_FIRE );
			}
		}
		break;
	}

	return CBaseMonster :: GetSchedule();
}

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
// POV: CBaseBarn, the armed human that barney, barniel,
// kate, gordon and the HEV family are made of, and the
// dead barns. The SDK's barney with the human grunt's
// weapons, reload and kick. File name and the names retail
// doesn't give are ours.
//=========================================================

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"talkmonster.h"
#include	"schedule.h"
#include	"defaultai.h"
#include	"scripted.h"
#include	"weapons.h"
#include	"soundent.h"
#include	"animation.h"
#include	"basebarn.h"

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define		BARN_AE_DRAW		( 2 )
#define		BARN_AE_SHOOT		( 3 )
#define		BARN_AE_HOLSTER		( 4 )
#define		BARN_AE_RELOAD		( 5 )
#define		BARN_AE_KICK		( 6 )
#define		BARN_AE_SHOOT2		( 7 )
#define		BARN_AE_BURST1		( 8 )	// fire without the shot sound
#define		BARN_AE_BURST2		( 9 )

TYPEDESCRIPTION	CBaseBarn::m_SaveData[] =
{
	DEFINE_FIELD( CBaseBarn, m_fGunDrawn, FIELD_BOOLEAN ),
	DEFINE_FIELD( CBaseBarn, m_painTime, FIELD_TIME ),
	DEFINE_FIELD( CBaseBarn, m_checkAttackTime, FIELD_TIME ),
	DEFINE_FIELD( CBaseBarn, m_lastAttackCheck, FIELD_BOOLEAN ),
	DEFINE_FIELD( CBaseBarn, m_fStanding, FIELD_BOOLEAN ),
	DEFINE_FIELD( CBaseBarn, m_fUsesMeleeAttack, FIELD_BOOLEAN ),
	DEFINE_FIELD( CBaseBarn, m_KarateMoveNum, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseBarn, m_iWeaponUsed, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseBarn, m_cClipSize, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseBarn, m_flPlayerDamage, FIELD_FLOAT ),
};

IMPLEMENT_SAVERESTORE( CBaseBarn, CTalkMonster );

//=========================================================
// AI Schedules Specific to this monster
//=========================================================
Task_t	tlBaFollow[] =
{
	{ TASK_MOVE_TO_TARGET_RANGE,(float)128		},	// Move within 128 of target ent (client)
	{ TASK_SET_SCHEDULE,		(float)SCHED_TARGET_FACE },
};

Schedule_t	slBaFollow[] =
{
	{
		tlBaFollow,
		ARRAYSIZE ( tlBaFollow ),
		bits_COND_NEW_ENEMY		|
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE	|
		bits_COND_HEAR_SOUND |
		bits_COND_PROVOKED,
		bits_SOUND_DANGER,
		"Follow"
	},
};

//=========================================================
// hide from the loudest sound source (to run from grenade)
//=========================================================
Task_t	tlBaseBarnHideReload[] =
{
	{ TASK_STOP_MOVING,				(float)0					},
	{ TASK_SET_FAIL_SCHEDULE,		(float)SCHED_RELOAD			},
	{ TASK_FIND_COVER_FROM_ENEMY,	(float)0					},
	{ TASK_RUN_PATH,				(float)0					},
	{ TASK_WAIT_FOR_MOVEMENT,		(float)0					},
	{ TASK_REMEMBER,				(float)bits_MEMORY_INCOVER	},
	{ TASK_FACE_ENEMY,				(float)0					},
	{ TASK_PLAY_SEQUENCE,			(float)ACT_RELOAD			},
};

Schedule_t slBaseBarnHideReload[] =
{
	{
		tlBaseBarnHideReload,
		ARRAYSIZE ( tlBaseBarnHideReload ),
		bits_COND_HEAVY_DAMAGE	|
		bits_COND_HEAR_SOUND,

		bits_SOUND_DANGER,
		"BaseBarnHideReload"
	}
};

//=========================================================
// BarneyDraw- much better looking draw schedule for when
// barney knows who he's gonna attack.
//=========================================================
Task_t	tlBaseBarnEnemyDraw[] =
{
	{ TASK_STOP_MOVING,					0				},
	{ TASK_FACE_ENEMY,					0				},
	{ TASK_PLAY_SEQUENCE_FACE_ENEMY,	(float) ACT_ARM },
};

Schedule_t slBaseBarnEnemyDraw[] =
{
	{
		tlBaseBarnEnemyDraw,
		ARRAYSIZE ( tlBaseBarnEnemyDraw ),
		0,
		0,
		"BaseBarn Enemy Draw"
	}
};

Task_t	tlBaFaceTarget[] =
{
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE },
	{ TASK_FACE_TARGET,			(float)0		},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE },
	{ TASK_SET_SCHEDULE,		(float)SCHED_TARGET_CHASE },
};

Schedule_t	slBaFaceTarget[] =
{
	{
		tlBaFaceTarget,
		ARRAYSIZE ( tlBaFaceTarget ),
		bits_COND_NO_AMMO_LOADED |
		bits_COND_CLIENT_PUSH	|
		bits_COND_NEW_ENEMY		|
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE	|
		bits_COND_HEAR_SOUND |
		bits_COND_PROVOKED,
		bits_SOUND_DANGER,
		"FaceTarget"
	},
};


Task_t	tlIdleBaStand[] =
{
	{ TASK_STOP_MOVING,			0				},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE },
	{ TASK_WAIT,				(float)2		}, // repick IDLESTAND every two seconds.
	{ TASK_TLK_HEADRESET,		(float)0		}, // reset head position
};

Schedule_t	slIdleBaStand[] =
{
	{
		tlIdleBaStand,
		ARRAYSIZE ( tlIdleBaStand ),
		bits_COND_NEW_ENEMY		|
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE	|
		bits_COND_HEAR_SOUND	|
		bits_COND_SMELL			|
		bits_COND_PROVOKED,

		bits_SOUND_COMBAT		|// sound flags - change these, and you'll break the talking code.
		//bits_SOUND_PLAYER		|
		//bits_SOUND_WORLD		|

		bits_SOUND_DANGER		|
		bits_SOUND_MEAT			|// scents
		bits_SOUND_CARCASS		|
		bits_SOUND_GARBAGE,
		"IdleStand"
	},
};

DEFINE_CUSTOM_SCHEDULES( CBaseBarn )
{
	slBaFollow,
	slBaseBarnEnemyDraw,
	slBaFaceTarget,
	slBaseBarnHideReload,
	slIdleBaStand,
};


IMPLEMENT_CUSTOM_SCHEDULES( CBaseBarn, CTalkMonster );

//=========================================================
// SetGun - the gun bodygroup (the HEV models override this)
//=========================================================
void CBaseBarn :: SetGun( int iGun )
{
	pev->body = iGun;
}

//=========================================================
// HasGun - gun in the hand, to drop when killed
//=========================================================
BOOL CBaseBarn :: HasGun( void )
{
	if ( pev->body == BARN_BODY_GUNGONE || pev->body == BARN_BODY_GUNHOLSTERED )
		return FALSE;

	return TRUE;
}

//=========================================================
// Spawn
//=========================================================
void CBaseBarn :: Spawn()
{
	Precache( );

	SET_MODEL(ENT(pev), "models/barney.mdl");
	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= BLOOD_COLOR_RED;
	pev->health			= gSkillData.normbarnsHealth;
	pev->view_ofs		= Vector ( 0, 0, 50 );// position of the eyes relative to monster's origin.
	m_flFieldOfView		= VIEW_FIELD_WIDE; // NOTE: we need a wide field of view so npc will notice player and say hello
	m_MonsterState		= MONSTERSTATE_NONE;

	SetupWeapon();

	m_fUsesMeleeAttack	= FALSE;
	SetBits(m_bitsSaid, bit_saidHelloPlayer);

	m_afCapability		= bits_CAP_HEAR | bits_CAP_TURN_HEAD | bits_CAP_DOORS_GROUP;

	MonsterInit();
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CBaseBarn :: Precache()
{
	PRECACHE_MODEL("models/barney.mdl");

	switch ( m_iWeaponUsed )
	{
	case BARN_WEAPON_GLOCK:
		PRECACHE_SOUND("barney/ba_attack2.wav" );
		PRECACHE_SOUND("weapons/reload3.wav" );
		break;
	case BARN_WEAPON_PYTHON:
		PRECACHE_SOUND("weapons/357_shot1.wav" );
		PRECACHE_SOUND("weapons/357_shot2.wav" );
		PRECACHE_SOUND("weapons/357_reload1.wav" );
		break;
	case BARN_WEAPON_MP5:
		PRECACHE_SOUND("hgrunt/gr_mgun1.wav" );
		PRECACHE_SOUND("hgrunt/gr_mgun2.wav" );
		PRECACHE_SOUND("hgrunt/gr_reload1.wav" );
		break;
	case BARN_WEAPON_SHOTGUN:
		PRECACHE_SOUND("barney/ba_attack2.wav" );	// sic
		PRECACHE_SOUND("hgrunt/gr_reload1.wav" );
		break;
	}

	PRECACHE_SOUND("zombie/claw_miss2.wav");
	PRECACHE_SOUND("common/kick.wav");
	PRECACHE_SOUND("common/punch.wav");

	m_iBrassShell = PRECACHE_MODEL ("models/shell.mdl");// brass shell
	m_iShotgunShell = PRECACHE_MODEL ("models/shotgunshell.mdl");

	// every new barney must call this, otherwise
	// when a level is loaded, nobody will talk (time is reset to 0)
	TalkInit();
	CTalkMonster::Precache();
}

// Init talk data
void CBaseBarn :: TalkInit()
{

	CTalkMonster::TalkInit();

	// scientists speach group names (group names are in sentences.txt)

	m_szGrp[TLK_ANSWER]  =	"BA_ANSWER";
	m_szGrp[TLK_QUESTION] =	"BA_QUESTION";
	m_szGrp[TLK_IDLE] =		"BA_IDLE";
	m_szGrp[TLK_PIDLE] =	"BA_PIDLE";
	m_szGrp[TLK_PQUESTION] = "BA_PQUEST";

	m_szGrp[TLK_SMELL] =	"BA_SMELL";

	m_szGrp[TLK_WOUND] =	"BA_WOUND";
	m_szGrp[TLK_MORTAL] =	"BA_MORTAL";

	m_szGrp[TLK_PAIN] =		"BA_PAIN";
	m_szGrp[TLK_DIE] =		"BA_DIE";
	m_szGrp[TLK_ATTACK] =	"BA_ATTACK";
	m_szGrp[TLK_KILL] =		"BA_KILL";

	m_voicePitch = RANDOM_LONG( 98, 102 );
}

//=========================================================
// SetupWeapon - clip and gun for the weapon a mapper chose
//=========================================================
void CBaseBarn :: SetupWeapon( void )
{
	pev->body = BARN_BODY_GUNHOLSTERED;

	switch ( m_iWeaponUsed )
	{
	case BARN_WEAPON_PYTHON:
		m_cAmmoLoaded = m_cClipSize = 6;
		SetGun( BARN_BODY_PYTHON );
		m_fGunDrawn = TRUE;
		m_fStanding = TRUE;
		break;
	case BARN_WEAPON_MP5:
		m_cAmmoLoaded = m_cClipSize = 48;
		SetGun( BARN_BODY_MP5 );
		m_fStanding = FALSE;
		m_fGunDrawn = TRUE;
		break;
	case BARN_WEAPON_SHOTGUN:
		m_cAmmoLoaded = m_cClipSize = 8;
		SetGun( BARN_BODY_SHOTGUN );
		m_fStanding = FALSE;
		m_fGunDrawn = TRUE;
		break;
	default:
		m_iWeaponUsed = BARN_WEAPON_GLOCK;
	case BARN_WEAPON_GLOCK:
		m_cAmmoLoaded = m_cClipSize = 17;
		SetGun( BARN_BODY_GUNHOLSTERED );
		m_fGunDrawn = FALSE;
		m_fStanding = TRUE;
		break;
	}
}

void CBaseBarn :: KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "weaponused"))
	{
		if ( atoi(pkvd->szValue) >= 0 && atoi(pkvd->szValue) <= 3 )
			m_iWeaponUsed = atoi(pkvd->szValue);
		else
			m_iWeaponUsed = BARN_WEAPON_GLOCK;
		pkvd->fHandled = TRUE;
	}
	else
		CBaseMonster::KeyValue( pkvd );
}

void CBaseBarn :: StartTask( Task_t *pTask )
{
	m_iTaskStatus = TASKSTATUS_RUNNING;

	switch ( pTask->iTask )
	{
	case TASK_RELOAD:
		m_IdealActivity = ACT_RELOAD;
		break;

	default:
		CTalkMonster::StartTask( pTask );
		break;
	}
}

void CBaseBarn :: RunTask( Task_t *pTask )
{
	switch ( pTask->iTask )
	{
	case TASK_RANGE_ATTACK1:
		CTalkMonster::RunTask( pTask );
		break;
	default:
		CTalkMonster::RunTask( pTask );
		break;
	}
}




//=========================================================
// ISoundMask - returns a bit mask indicating which types
// of sounds this monster regards.
//=========================================================
int CBaseBarn :: ISoundMask ( void)
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
int	CBaseBarn :: Classify ( void )
{
	return	CLASS_PLAYER_ALLY;
}

//=========================================================
// Kick and return if you hit anything (the human grunt's)
//=========================================================
CBaseEntity *CBaseBarn :: Kick( void )
{
	TraceResult tr;

	UTIL_MakeVectors( pev->angles );
	Vector vecStart = pev->origin;
	vecStart.z += pev->size.z * 0.5;
	Vector vecEnd = vecStart + (gpGlobals->v_forward * 70);

	UTIL_TraceHull( vecStart, vecEnd, dont_ignore_monsters, head_hull, ENT(pev), &tr );

	if ( tr.pHit )
	{
		CBaseEntity *pEntity = CBaseEntity::Instance( tr.pHit );
		return pEntity;
	}

	return NULL;
}

//=========================================================
// ALertSound - barney says "Freeze!"
//=========================================================
void CBaseBarn :: AlertSound( void )
{
	if ( m_hEnemy != NULL )
	{
		if ( FOkToSpeak() )
		{
			PlaySentence( m_szGrp[TLK_ATTACK], RANDOM_FLOAT(2.8, 3.2), VOL_NORM, ATTN_NORM );
		}
	}

}
//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CBaseBarn :: SetYawSpeed ( void )
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
	default:
		ys = 70;
		break;
	}

	pev->yaw_speed = ys;
}

//=========================================================
// CheckMeleeAttack1 (the human grunt's)
//=========================================================
BOOL CBaseBarn :: CheckMeleeAttack1 ( float flDot, float flDist )
{
	CBaseMonster *pEnemy;

	if ( m_hEnemy != NULL )
	{
		pEnemy = m_hEnemy->MyMonsterPointer();

		if ( !pEnemy )
		{
			return FALSE;
		}
	}

	if ( flDist <= 64 && flDot >= 0.7	&&
		 pEnemy->Classify() != CLASS_ALIEN_BIOWEAPON &&
		 pEnemy->Classify() != CLASS_PLAYER_BIOWEAPON )
	{
		return TRUE;
	}
	return FALSE;
}

//=========================================================
// CheckRangeAttack1
//=========================================================
BOOL CBaseBarn :: CheckRangeAttack1 ( float flDot, float flDist )
{
	if ( flDist <= m_flDistTooFar && flDot >= 0.5 )
	{
		if ( gpGlobals->time > m_checkAttackTime )
		{
			TraceResult tr;
			Vector shootOrigin;

			if ( m_iWeaponUsed == BARN_WEAPON_GLOCK || m_iWeaponUsed == BARN_WEAPON_PYTHON )
				shootOrigin = pev->origin + Vector( 0, 0, 55 );
			else
				shootOrigin = pev->origin + Vector( 0, 0, 16 );
			CBaseEntity *pEnemy = m_hEnemy;
			Vector shootTarget = ( (pEnemy->BodyTarget( shootOrigin ) - pEnemy->pev->origin) + m_vecEnemyLKP );
			UTIL_TraceLine( shootOrigin, shootTarget, dont_ignore_monsters, dont_ignore_glass, ENT(pev), &tr );
			if ( tr.flFraction == 1.0 || (tr.pHit != NULL && CBaseEntity::Instance(tr.pHit) == pEnemy) )
				m_lastAttackCheck = TRUE;
			else
				m_lastAttackCheck = FALSE;
			if ( m_iWeaponUsed == BARN_WEAPON_PYTHON )
				m_checkAttackTime = gpGlobals->time + 3;
			else
				m_checkAttackTime = gpGlobals->time + 1.5;
		}
		return m_lastAttackCheck;
	}
	return FALSE;
}

//=========================================================
// FireWeapon - shoots the gun this barn carries
//=========================================================
void CBaseBarn :: FireWeapon( BOOL fSound )
{
	switch ( m_iWeaponUsed )
	{
	case BARN_WEAPON_GLOCK:
		FireGlock( fSound );
		break;
	case BARN_WEAPON_PYTHON:
		FirePython( fSound );
		break;
	case BARN_WEAPON_MP5:
		FireMP5( fSound );
		break;
	case BARN_WEAPON_SHOTGUN:
		FireShotgun( fSound );
		break;
	}
}

//=========================================================
// FireGlock - shoots one round from the pistol at
// the enemy barney is facing.
//=========================================================
void CBaseBarn :: FireGlock ( BOOL fSound )
{
	if ( m_hEnemy == NULL )
		return;

	Vector vecShootOrigin;

	UTIL_MakeVectors(pev->angles);
	vecShootOrigin = pev->origin + Vector( 0, 0, 55 );
	Vector vecShootDir = ShootAtEnemy( vecShootOrigin );

	Vector angDir = UTIL_VecToAngles( vecShootDir );
	SetBlending( 0, angDir.x );
	pev->effects = EF_MUZZLEFLASH;
	UTIL_MuzzleLight( this, vecShootOrigin );

	if ( g_iSkillLevel == SKILL_HARD )
		FireBullets(1, vecShootOrigin, vecShootDir, VECTOR_CONE_2DEGREES, m_flDistTooFar, BULLET_MONSTER_9MM );
	else
		FireBullets(1, vecShootOrigin, vecShootDir, VECTOR_CONE_6DEGREES, m_flDistTooFar, BULLET_MONSTER_9MM );

	if ( fSound )
		WeaponSound();

	m_cAmmoLoaded--;// take away a bullet!
}

//=========================================================
// FirePython - two 357 rounds
//=========================================================
void CBaseBarn :: FirePython ( BOOL fSound )
{
	if ( m_hEnemy == NULL )
		return;

	Vector vecShootOrigin;

	UTIL_MakeVectors(pev->angles);
	vecShootOrigin = pev->origin + Vector( 0, 0, 55 );
	Vector vecShootDir = ShootAtEnemy( vecShootOrigin );

	Vector angDir = UTIL_VecToAngles( vecShootDir );
	SetBlending( 0, angDir.x );
	pev->effects = EF_MUZZLEFLASH;
	UTIL_MuzzleLight( this, vecShootOrigin );

	if ( g_iSkillLevel == SKILL_HARD )
		FireBullets(2, vecShootOrigin, vecShootDir, VECTOR_CONE_2DEGREES, m_flDistTooFar, BULLET_MONSTER_12MM );
	else
		FireBullets(2, vecShootOrigin, vecShootDir, VECTOR_CONE_6DEGREES, m_flDistTooFar, BULLET_MONSTER_12MM );

	if ( fSound )
		WeaponSound();

	m_cAmmoLoaded--;// take away a bullet!
}

//=========================================================
// FireMP5 (the human grunt's Shoot)
//=========================================================
void CBaseBarn :: FireMP5 ( BOOL fSound )
{
	if (m_hEnemy == NULL)
	{
		return;
	}

	Vector vecShootOrigin = GetGunPosition();
	Vector vecShootDir = ShootAtEnemy( vecShootOrigin );

	UTIL_MakeVectors ( pev->angles );

	Vector	vecShellVelocity = gpGlobals->v_right * RANDOM_FLOAT(40,90) + gpGlobals->v_up * RANDOM_FLOAT(75,200) + gpGlobals->v_forward * RANDOM_FLOAT(-40, 40);
	EjectBrass ( vecShootOrigin - vecShootDir * 24, vecShellVelocity, pev->angles.y, m_iBrassShell, TE_BOUNCE_SHELL);
	FireBullets(1, vecShootOrigin, vecShootDir, VECTOR_CONE_10DEGREES, m_flDistTooFar, BULLET_MONSTER_MP5 ); // shoot +-5 degrees

	pev->effects |= EF_MUZZLEFLASH;
	UTIL_MuzzleLight( this, vecShootOrigin );

	if ( fSound )
		WeaponSound();

	m_cAmmoLoaded--;// take away a bullet!

	Vector angDir = UTIL_VecToAngles( vecShootDir );
	SetBlending( 0, angDir.x );
}

//=========================================================
// FireShotgun (the human grunt's Shotgun)
//=========================================================
void CBaseBarn :: FireShotgun ( BOOL fSound )
{
	if (m_hEnemy == NULL)
	{
		return;
	}

	Vector vecShootOrigin = GetGunPosition();
	Vector vecShootDir = ShootAtEnemy( vecShootOrigin );

	UTIL_MakeVectors ( pev->angles );

	Vector	vecShellVelocity = gpGlobals->v_right * RANDOM_FLOAT(40,90) + gpGlobals->v_up * RANDOM_FLOAT(75,200) + gpGlobals->v_forward * RANDOM_FLOAT(-40, 40);
	EjectBrass ( vecShootOrigin - vecShootDir * 24, vecShellVelocity, pev->angles.y, m_iShotgunShell, TE_BOUNCE_SHOTSHELL);
	FireBullets(gSkillData.hgruntShotgunPellets, vecShootOrigin, vecShootDir, VECTOR_CONE_15DEGREES, m_flDistTooFar, BULLET_PLAYER_BUCKSHOT, 0 ); // shoot +-7.5 degrees

	pev->effects |= EF_MUZZLEFLASH;
	UTIL_MuzzleLight( this, vecShootOrigin );

	if ( fSound )
		WeaponSound();

	m_cAmmoLoaded--;// take away a bullet!

	Vector angDir = UTIL_VecToAngles( vecShootDir );
	SetBlending( 0, angDir.x );
}

//=========================================================
// GetGunPosition	return the end of the barrel
//=========================================================
Vector CBaseBarn :: GetGunPosition( )
{
	if (m_fStanding )
	{
		return pev->origin + Vector( 0, 0, 60 );
	}
	else
	{
		return pev->origin + Vector( 0, 0, 48 );
	}
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//
// Returns number of events handled, 0 if none.
//=========================================================
void CBaseBarn :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	switch( pEvent->event )
	{
	case BARN_AE_KICK:
		if ( m_fUsesMeleeAttack )
		{
			CBaseEntity *pHurt = Kick();

			if ( pHurt )
			{
				if ( m_KarateMoveNum == -1 )
				{
					EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, "common/kick.wav", 1, ATTN_NORM, 0, PITCH_NORM );
				}
				else
				{
					m_KarateMoveNum++;
					if ( m_KarateMoveNum == 3 )
						EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, "common/kick.wav", 1, ATTN_NORM, 0, PITCH_NORM );
					else
						EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, "common/punch.wav", 1, ATTN_NORM, 0, PITCH_NORM );
				}

				UTIL_MakeVectors( pev->angles );
				pHurt->pev->punchangle.x = 15;
				pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_forward * 100 + gpGlobals->v_up * 50;
				pHurt->TakeDamage( pev, pev, gSkillData.hgruntDmgKick, DMG_CLUB );
			}
		}
		break;

	case BARN_AE_RELOAD:
		ReloadSound();
		m_cAmmoLoaded = m_cClipSize;
		ClearConditions(bits_COND_NO_AMMO_LOADED);
		break;

	case BARN_AE_SHOOT:
	case BARN_AE_SHOOT2:
		FireWeapon( TRUE );
		break;

	case BARN_AE_BURST1:
	case BARN_AE_BURST2:
		FireWeapon( FALSE );
		break;

	case BARN_AE_DRAW:
		// barney's bodygroup switches here so he can pull gun from holster
		if ( m_iWeaponUsed == BARN_WEAPON_GLOCK )
			SetGun( BARN_BODY_GLOCK );
		m_fGunDrawn = TRUE;
		break;

	case BARN_AE_HOLSTER:
		// change bodygroup to replace gun in holster
		if ( m_iWeaponUsed == BARN_WEAPON_GLOCK )
			SetGun( BARN_BODY_GUNHOLSTERED );
		m_fGunDrawn = FALSE;
		break;

	default:
		CTalkMonster::HandleAnimEvent( pEvent );
	}
}

//=========================================================
// CheckAmmo - overridden for the grunt because he actually
// uses ammo! (base class doesn't)
//=========================================================
void CBaseBarn :: CheckAmmo ( void )
{
	if ( m_cAmmoLoaded <= 0 )
	{
		SetConditions(bits_COND_NO_AMMO_LOADED);
	}
}

int CBaseBarn :: TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType)
{
	// make sure friends talk about it if player hurts talkmonsters...
	int ret = CTalkMonster::TakeDamage(pevInflictor, pevAttacker, flDamage, bitsDamageType);
	if ( !IsAlive() || pev->deadflag == DEAD_DYING )
		return ret;

	return ret;
}


//=========================================================
// PainSound
//=========================================================
void CBaseBarn :: PainSound ( void )
{
	if (gpGlobals->time < m_painTime)
		return;

	m_painTime = gpGlobals->time + RANDOM_FLOAT(0.5, 0.75);

	PlaySentence( m_szGrp[TLK_PAIN], m_painTime, VOL_NORM, ATTN_NORM );	// sic: the time, not a duration
}

//=========================================================
// DeathSound
//=========================================================
void CBaseBarn :: DeathSound ( void )
{
	PlaySentence( m_szGrp[TLK_DIE], 4, VOL_NORM, ATTN_NORM );
}

//=========================================================
// WeaponSound - the shot
//=========================================================
void CBaseBarn :: WeaponSound( void )
{
	int pitchShift = RANDOM_LONG( 0, 20 );

	// Only shift about half the time
	if ( pitchShift > 10 )
		pitchShift = 0;
	else
		pitchShift -= 5;

	switch ( m_iWeaponUsed )
	{
	case BARN_WEAPON_GLOCK:
		EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, "barney/ba_attack2.wav", 1, ATTN_NORM, 0, 100 + pitchShift );
		break;
	case BARN_WEAPON_PYTHON:
		if ( RANDOM_LONG( 0, 1 ) == 0 )
			EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, "weapons/357_shot1.wav", 1, ATTN_NORM, 0, 100 + pitchShift );
		else
			EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, "weapons/357_shot2.wav", 1, ATTN_NORM, 0, 100 + pitchShift );
		break;
	case BARN_WEAPON_MP5:
		if ( RANDOM_LONG( 0, 1 ) == 0 )
			EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, "hgrunt/gr_mgun1.wav", 1, ATTN_NORM, 0, 100 + pitchShift );
		else
			EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, "hgrunt/gr_mgun2.wav", 1, ATTN_NORM, 0, 100 + pitchShift );
		break;
	case BARN_WEAPON_SHOTGUN:
		EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, "weapons/sbarrel1.wav", 1, ATTN_NORM, 0, 100 + pitchShift );
		break;
	}

	CSoundEnt::InsertSound ( bits_SOUND_COMBAT, pev->origin, 384, 0.3 );
}

//=========================================================
// ReloadSound
//=========================================================
void CBaseBarn :: ReloadSound( void )
{
	switch ( m_iWeaponUsed )
	{
	case BARN_WEAPON_GLOCK:
		EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, "weapons/reload3.wav", 1, ATTN_NORM, 0, PITCH_NORM );
		break;
	case BARN_WEAPON_PYTHON:
		EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, "weapons/357_reload1.wav", 1, ATTN_NORM, 0, PITCH_NORM );
		break;
	case BARN_WEAPON_MP5:
		EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, "hgrunt/gr_reload1.wav", 1, ATTN_NORM, 0, PITCH_NORM );
		break;
	case BARN_WEAPON_SHOTGUN:
		EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, "hgrunt/gr_reload1.wav", 1, ATTN_NORM, 0, PITCH_NORM );
		break;
	}
}


void CBaseBarn::TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType)
{
	switch( ptr->iHitgroup)
	{
	case HITGROUP_CHEST:
	case HITGROUP_STOMACH:
		if (bitsDamageType & (DMG_BULLET | DMG_SLASH | DMG_BLAST))
		{
			flDamage = flDamage / 2;
		}
		break;
	case 10:
		if (bitsDamageType & (DMG_BULLET | DMG_SLASH | DMG_CLUB))
		{
			flDamage -= 20;
			if (flDamage <= 0)
			{
				UTIL_Ricochet( ptr->vecEndPos, 1.0 );
				flDamage = 0.01;
			}
		}
		// always a head shot
		ptr->iHitgroup = HITGROUP_HEAD;
		break;
	}

	CTalkMonster::TraceAttack( pevAttacker, flDamage, vecDir, ptr, bitsDamageType );
}


void CBaseBarn::Killed( entvars_t *pevAttacker, int iGib )
{
	if ( HasGun() )
	{// drop the gun!
		switch ( m_iWeaponUsed )
		{
		case BARN_WEAPON_GLOCK:
			SetGun( BARN_BODY_GUNGONE );
			UTIL_DropGun( this, "weapon_9mmhandgun", TRUE );
			break;
		case BARN_WEAPON_PYTHON:
			SetGun( BARN_BODY_GUNHOLSTERED );
			UTIL_DropGun( this, "weapon_python", TRUE );
			break;
		case BARN_WEAPON_MP5:
			SetGun( BARN_BODY_GUNHOLSTERED );
			UTIL_DropGun( this, "weapon_9mmAR", TRUE );
			break;
		case BARN_WEAPON_SHOTGUN:
			SetGun( BARN_BODY_GUNHOLSTERED );
			UTIL_DropGun( this, "weapon_shotgun", TRUE );
			break;
		}
	}

	CTalkMonster::Killed( pevAttacker, iGib );
}

//=========================================================
// AI Schedules Specific to this monster
//=========================================================

Schedule_t* CBaseBarn :: GetScheduleOfType ( int Type )
{
	Schedule_t *psched;

	switch( Type )
	{
	case SCHED_ARM_WEAPON:
		if ( m_hEnemy != NULL )
		{
			// face enemy, then draw.
			return slBaseBarnEnemyDraw;
		}
		break;

	// Hook these to make a looping schedule
	case SCHED_TARGET_FACE:
		// call base class default so that barney will talk
		// when 'used'
		psched = CTalkMonster::GetScheduleOfType(Type);

		if (psched == slIdleStand)
			return slBaFaceTarget;	// override this for different target face behavior
		else
			return psched;

	case SCHED_TARGET_CHASE:
		return slBaFollow;

	case SCHED_BASEBARN_HIDE_RELOAD:
		return slBaseBarnHideReload;

	case SCHED_IDLE_STAND:
		// call base class default so that scientist will talk
		// when standing during idle
		psched = CTalkMonster::GetScheduleOfType(Type);

		if (psched == slIdleStand)
		{
			// just look straight ahead.
			return slIdleBaStand;
		}
		else
			return psched;
	}

	return CTalkMonster::GetScheduleOfType( Type );
}

//=========================================================
// SetActivity
//=========================================================
void CBaseBarn :: SetActivity ( Activity NewActivity )
{
	int	iSequence = ACTIVITY_NOT_AVAILABLE;
	void *pmodel = GET_MODEL_PTR( ENT(pev) );

	switch ( NewActivity )
	{
	case ACT_IDLE:
		pev->sequence = LookupSequence( "cower_stand" );
		if ( pev->sequence )
			iSequence = LookupSequence( "idle1" );
		break;
	case ACT_RANGE_ATTACK1:
		switch ( m_iWeaponUsed )
		{
		case BARN_WEAPON_GLOCK:
			iSequence = LookupSequence( "shootgun2" );
			break;
		case BARN_WEAPON_PYTHON:
			iSequence = LookupSequence( "shootgun" );
			break;
		case BARN_WEAPON_MP5:
			iSequence = LookupSequence( "mp5" );
			break;
		case BARN_WEAPON_SHOTGUN:
			iSequence = LookupSequence( "shotgun" );
			break;
		}
		break;
	case ACT_RELOAD:
		switch ( m_iWeaponUsed )
		{
		case BARN_WEAPON_GLOCK:
			iSequence = LookupSequence( "reload" );
			break;
		case BARN_WEAPON_PYTHON:
			iSequence = LookupSequence( "reload2" );
			break;
		case BARN_WEAPON_MP5:
			iSequence = LookupSequence( "reload3" );
			break;
		case BARN_WEAPON_SHOTGUN:
			iSequence = LookupSequence( "reload4" );
			break;
		}
		break;
	case ACT_MELEE_ATTACK1:
		if ( !m_fUsesMeleeAttack )
		{
			CTalkMonster::SetActivity( NewActivity );
			return;
		}
		if ( RANDOM_LONG( 0, 2 ) == 0 )
		{
			m_KarateMoveNum = 0;
			iSequence = LookupSequence( "karate_bighit" );
		}
		else
		{
			m_KarateMoveNum = -1;
			iSequence = LookupSequence( "karate_hit" );
		}
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
// GetSchedule - Decides which type of schedule best suits
// the monster's current state and conditions. Then calls
// monster's member function to get a pointer to a schedule
// of the proper type.
//=========================================================
Schedule_t *CBaseBarn :: GetSchedule ( void )
{
	if ( HasConditions( bits_COND_HEAR_SOUND ) )
	{
		CSound *pSound;
		pSound = PBestSound();

		ASSERT( pSound != NULL );
		if ( pSound && (pSound->m_iType & bits_SOUND_DANGER) )
			return GetScheduleOfType( SCHED_TAKE_COVER_FROM_BEST_SOUND );
	}
	if ( HasConditions( bits_COND_ENEMY_DEAD ) && FOkToSpeak() )
	{
		PlaySentence( m_szGrp[TLK_KILL], 4, VOL_NORM, ATTN_NORM );
	}

	switch( m_MonsterState )
	{
	case MONSTERSTATE_COMBAT:
		{
// dead enemy
			if ( HasConditions( bits_COND_ENEMY_DEAD ) )
			{
				// call base class, all code to handle dead enemies is centralized there.
				return CBaseMonster :: GetSchedule();
			}

			if ( HasConditions( bits_COND_NEW_ENEMY ) )
				AlertSound();

			// always act surprized with a new enemy
			if ( HasConditions( bits_COND_NEW_ENEMY ) && HasConditions( bits_COND_LIGHT_DAMAGE) )
				return GetScheduleOfType( SCHED_SMALL_FLINCH );

			// wait for one schedule to draw gun
			if (!m_fGunDrawn )
				return GetScheduleOfType( SCHED_ARM_WEAPON );

			if ( HasConditions( bits_COND_HEAVY_DAMAGE ) )
				return GetScheduleOfType( SCHED_TAKE_COVER_FROM_ENEMY );

			if ( m_fUsesMeleeAttack && HasConditions( bits_COND_CAN_MELEE_ATTACK1 ) )
				return GetScheduleOfType( SCHED_MELEE_ATTACK1 );

			// no ammo
			if ( HasConditions ( bits_COND_NO_AMMO_LOADED ) )
				return GetScheduleOfType ( SCHED_BASEBARN_HIDE_RELOAD );
		}
		break;

	case MONSTERSTATE_ALERT:
	case MONSTERSTATE_IDLE:
		if ( HasConditions(bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE))
		{
			// flinch if hurt
			return GetScheduleOfType( SCHED_SMALL_FLINCH );
		}

		// try to say something about smells
		TrySmellTalk();
		break;
	}

	return CTalkMonster::GetSchedule();
}

MONSTERSTATE CBaseBarn :: GetIdealState ( void )
{
	return CTalkMonster::GetIdealState();
}





//=========================================================
// DEAD BARN PROP
//
// Designer selects a pose in worldcraft, 0 through num_poses-1
// this value is added to what is selected as the 'first dead pose'
// among the monster's normal animations. All dead poses must
// appear sequentially in the model file. Be sure and set
// the m_iFirstPose properly!
//
// POV: "deadmodel" picks barney, barniel or one of the HEV
// models (class name ours)
//=========================================================
class CDeadBarns : public CBaseMonster
{
public:
	void Spawn( void );
	int	Classify ( void ) { return	CLASS_PLAYER_ALLY; }

	void KeyValue( KeyValueData *pkvd );

	int	m_iPose;// which sequence to display	-- temporary, don't need to save
	int	m_iDeadModel;
	static char *m_szPoses[3];
};

char *CDeadBarns::m_szPoses[] = { "lying_on_back", "lying_on_side", "lying_on_stomach" };

void CDeadBarns::KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "pose"))
	{
		m_iPose = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	if (FStrEq(pkvd->szKeyName, "deadmodel"))
	{
		m_iDeadModel = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CBaseMonster::KeyValue( pkvd );
}

LINK_ENTITY_TO_CLASS( monster_barns_dead, CDeadBarns );

//=========================================================
// ********** DeadBarns SPAWN **********
//=========================================================
void CDeadBarns :: Spawn( )
{
	switch ( m_iDeadModel )
	{
	case 1:
		PRECACHE_MODEL("models/barniel.mdl");
		SET_MODEL(ENT(pev), "models/barniel.mdl");
		pev->body = BARN_BODY_GUNGONE;
		break;
	case 2:
	case 3:
	case 4:
		PRECACHE_MODEL("models/hev_barney.mdl");
		SET_MODEL(ENT(pev), "models/hev_barney.mdl");
		pev->body = 0;
		if ( m_iDeadModel == 2 )
			SetBodygroup( 1, 0 );
		else if ( m_iDeadModel == 3 )
			SetBodygroup( 1, 1 );
		else
			SetBodygroup( 1, 2 );
		SetBodygroup( 2, 5 );
		break;
	case 5:
	case 6:
	case 7:
		PRECACHE_MODEL("models/hev_scientist.mdl");
		SET_MODEL(ENT(pev), "models/hev_scientist.mdl");
		pev->body = 0;
		if ( m_iDeadModel == 5 )
			SetBodygroup( 1, 2 );
		else if ( m_iDeadModel == 6 )
			SetBodygroup( 1, 1 );
		else
			SetBodygroup( 1, 1 );
		SetBodygroup( 2, 5 );
		break;
	default:
		m_iDeadModel = 0;
	case 0:
		PRECACHE_MODEL("models/barney.mdl");
		SET_MODEL(ENT(pev), "models/barney.mdl");
		pev->body = BARN_BODY_GUNGONE;
		break;
	}

	pev->effects		= 0;
	pev->yaw_speed		= 8;
	pev->sequence		= 0;
	m_bloodColor		= BLOOD_COLOR_RED;

	pev->sequence = LookupSequence( m_szPoses[m_iPose] );
	if (pev->sequence == -1)
	{
		ALERT ( at_console, "Dead barn type model with bad pose\n" );
	}
	// Corpses have less health
	pev->health			= 8;//gSkillData.barneyHealth;

	MonsterInitDead();
}

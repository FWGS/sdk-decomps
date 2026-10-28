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
// evilscientist.cpp - T2DEF's evil scientist: shoots a
// pistol, hits with his fists (file name ours)
//=========================================================

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"schedule.h"
#include	"weapons.h"
#include	"soundent.h"


//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define	EVILSCI_AE_SHOOT			1 // name ours
#define	EVILSCI_AE_HIT				2 // name ours

class CEvilScientist : public CBaseMonster
{
public:
	void Spawn( void );
	void Precache( void );
	void SetYawSpeed( void );
	int  Classify ( void );
	int  ISoundMask( void );
	int  IRelationship( CBaseEntity *pTarget );
	void HandleAnimEvent( MonsterEvent_t *pEvent );
	void RunAI( void );
	BOOL CheckMeleeAttack1 ( float flDot, float flDist );
	BOOL CheckRangeAttack1 ( float flDot, float flDist );

	void PainSound( void );
	void AlertSound( void );
	void AttackSound( void );
	void Shoot( void );

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	float m_flLastShot;
	float m_flDiviation;

	int m_iShell;

	static const char *pAttackSounds[];
	static const char *pAlertSounds[];
	static const char *pPainSounds[];
};

LINK_ENTITY_TO_CLASS( monster_evil_scientist, CEvilScientist );

const char *CEvilScientist::pAttackSounds[] =
{
	"evilsci/evilsci_beat1.wav",
	"evilsci/evilsci_beat2.wav",
	"evilsci/evilsci_beat3.wav",
	"evilsci/evilsci_beat4.wav",
};

const char *CEvilScientist::pAlertSounds[] =
{
	"evilsci/evilsci_alert1.wav",
	"evilsci/evilsci_alert2.wav",
	"evilsci/evilsci_alert3.wav",
};

const char *CEvilScientist::pPainSounds[] =
{
	"evilsci/evilsci_pain1.wav",
	"evilsci/evilsci_pain2.wav",
};

TYPEDESCRIPTION	CEvilScientist::m_SaveData[] =
{
	DEFINE_FIELD( CEvilScientist, m_flLastShot, FIELD_TIME ),
	DEFINE_FIELD( CEvilScientist, m_flDiviation, FIELD_FLOAT ),
};

IMPLEMENT_SAVERESTORE( CEvilScientist, CBaseMonster );

void CEvilScientist :: PainSound( void )
{
	int pitch = 95 + RANDOM_LONG(0,9);

	if (RANDOM_LONG(0,5) < 2)
		EMIT_SOUND_DYN ( ENT(pev), CHAN_VOICE, pPainSounds[ RANDOM_LONG(0,ARRAYSIZE(pPainSounds)-1) ], 1.0, ATTN_NORM, 0, pitch );
}

void CEvilScientist :: AlertSound( void )
{
	int pitch = 95 + RANDOM_LONG(0,9);

	EMIT_SOUND_DYN ( ENT(pev), CHAN_VOICE, pAlertSounds[ RANDOM_LONG(0,ARRAYSIZE(pAlertSounds)-1) ], 1.0, ATTN_NORM, 0, pitch );
}

void CEvilScientist :: AttackSound( void )
{
	int pitch = 95 + RANDOM_LONG(0,9);

	EMIT_SOUND_DYN ( ENT(pev), CHAN_VOICE, pAttackSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackSounds)-1) ], 1.0, ATTN_NORM, 0, pitch );
}

//=========================================================
// ISoundMask - returns a bit mask indicating which types
// of sounds this monster regards.
//=========================================================
int CEvilScientist :: ISoundMask ( void )
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
int	CEvilScientist :: Classify ( void )
{
	return m_iClass?m_iClass:CLASS_FACTION_A;
}

int CEvilScientist::IRelationship( CBaseEntity *pTarget )
{
	if ( FClassnameIs( pTarget->pev, "monster_alien_slave" ) )
		return R_NM;

	return CBaseMonster::IRelationship( pTarget );
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CEvilScientist :: SetYawSpeed ( void )
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
void CEvilScientist :: Shoot ( void )
{
	pev->framerate = 1.0;

	if (m_hEnemy == NULL && !m_pCine) //LRC
	{
		return;
	}

	pev->framerate = RANDOM_FLOAT( 0.8, 1.5 );

	Vector vecShootOrigin = GetGunPosition();
	Vector vecShootDir = ShootAtEnemy( vecShootOrigin );

	if (m_flLastShot + 2 < gpGlobals->time)
	{
		m_flDiviation = 0.50;
	}
	else
	{
		m_flDiviation -= 0.1;
		if (m_flDiviation < 0.02)
			m_flDiviation = 0.02;
	}
	m_flLastShot = gpGlobals->time;

	UTIL_MakeVectors ( pev->angles );

	Vector	vecShellVelocity = gpGlobals->v_right * RANDOM_FLOAT(40,90) + gpGlobals->v_up * RANDOM_FLOAT(75,200) + gpGlobals->v_forward * RANDOM_FLOAT(-40, 40);
	EjectBrass ( pev->origin + gpGlobals->v_up * 32 + gpGlobals->v_forward * 12, vecShellVelocity, pev->angles.y, m_iShell, TE_BOUNCE_SHELL);
	FireBullets(1, vecShootOrigin, vecShootDir, VECTOR_CONE_20DEGREES, 2048, BULLET_MONSTER_9MM );

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
//=========================================================
void CEvilScientist :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	switch( pEvent->event )
	{
		case EVILSCI_AE_SHOOT:
			Shoot();
			break;

		case EVILSCI_AE_HIT:
		{
			AttackSound();

			CBaseEntity *pHurt = CheckTraceHullAttack( 64, 5, DMG_CLUB );
			if ( pHurt )
			{
				if ( pHurt->pev->flags & (FL_MONSTER|FL_CLIENT) )
				{
					pHurt->pev->punchangle.z = -25;
					pHurt->pev->punchangle.x = 5;
				}

				switch( RANDOM_LONG(0,2) )
				{
				case 0:
					EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, "weapons/cbar_hitbod1.wav", 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
					break;
				case 1:
					EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, "weapons/cbar_hitbod2.wav", 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
					break;
				case 2:
					EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, "weapons/cbar_hitbod3.wav", 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
					break;
				}
			}
			else
				EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, "zombie/claw_miss2.wav", 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
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
void CEvilScientist :: Spawn()
{
	Precache( );

	SET_MODEL(ENT(pev), "models/evilsci.mdl");
	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= BLOOD_COLOR_RED;
	pev->effects		= 0;
	pev->health			= 30;
	m_flFieldOfView		= VIEW_FIELD_WIDE;
	pev->view_ofs		= Vector ( 0, 0, 50 );// position of the eyes relative to monster's origin.
	m_MonsterState		= MONSTERSTATE_NONE;
	m_afCapability		= bits_CAP_HEAR | bits_CAP_DOORS_GROUP;
	m_cAmmoLoaded		= 15;
	m_HackedGunPos		= Vector( 0, 24, 48 );

	MonsterInit();
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CEvilScientist :: Precache()
{
	int i;

	PRECACHE_MODEL("models/evilsci.mdl");

	PRECACHE_SOUND("weapons/pl_gun1.wav");
	PRECACHE_SOUND("weapons/pl_gun2.wav");
	PRECACHE_SOUND("weapons/cbar_hitbod1.wav");
	PRECACHE_SOUND("weapons/cbar_hitbod2.wav");
	PRECACHE_SOUND("weapons/cbar_hitbod3.wav");
	PRECACHE_SOUND("zombie/claw_miss2.wav");

	for ( i = 0; i < ARRAYSIZE( pAlertSounds ); i++ )
		PRECACHE_SOUND((char *)pAlertSounds[i]);

	for ( i = 0; i < ARRAYSIZE( pPainSounds ); i++ )
		PRECACHE_SOUND((char *)pPainSounds[i]);

	for ( i = 0; i < ARRAYSIZE( pAttackSounds ); i++ )
		PRECACHE_SOUND((char *)pAttackSounds[i]);

	m_iShell = PRECACHE_MODEL("models/shell.mdl");// brass shell
}

//=========================================================
// CheckMeleeAttack1
//=========================================================
BOOL CEvilScientist :: CheckMeleeAttack1 ( float flDot, float flDist )
{
	if ( flDist <= 72 && flDot >= 0.7 && !HasConditions( bits_COND_CAN_MELEE_ATTACK1 ) )
	{
		return TRUE;
	}
	return FALSE;
}

//=========================================================
// CheckRangeAttack1
//=========================================================
BOOL CEvilScientist :: CheckRangeAttack1 ( float flDot, float flDist )
{
	if ( !HasConditions( bits_COND_ENEMY_OCCLUDED ) && flDist > 64 && flDist <= 2048 && flDot >= 0.5 && m_cAmmoLoaded > 0 )
	{
		TraceResult	tr;

		Vector vecSrc = GetGunPosition();

		// verify that a bullet fired from the gun will hit the enemy before the world.
		UTIL_TraceLine( vecSrc, m_hEnemy->BodyTarget(vecSrc), ignore_monsters, ignore_glass, ENT(pev), &tr);

		if ( tr.flFraction == 1 || tr.pHit == m_hEnemy->edict() )
		{
			return TRUE;
		}
	}
	return FALSE;
}

//=========================================================
// RunAI
//=========================================================
void CEvilScientist :: RunAI( void )
{
	CBaseMonster::RunAI();

	if ( m_Activity != ACT_RANGE_ATTACK1 && IsAlive() )
		pev->framerate = 1.0;
}

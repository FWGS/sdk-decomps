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
// the hostile scientist: a bandit in a gas mask who throws
// chemical blobs from his chemical gun
//=========================================================

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"schedule.h"
#include	"animation.h"
#include	"weapons.h"
#include	"soundent.h"
#include	"effects.h"
#include	"chemgrenade.h"
#include	"bandit.h"

extern DLL_GLOBAL int g_iSkillLevel;
extern int g_iGibHumanSpr;
extern int g_iGoreHuman;

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define		SCIENTIST_AE_THROW		( 1 )
#define		SCIENTIST_AE_PUNCH1		( 4 )
#define		SCIENTIST_AE_PUNCH2		( 5 )

class CHostileScientist : public CBaseBandit
{
public:
	void Spawn( void );
	void Precache( void );
	void SetYawSpeed( void );
	int  Classify( void );
	int  MonsterType( void )
	{
		if ( g_Language == LANGUAGE_GERMAN || CVAR_GET_FLOAT( "violence_germany" ) != 0 )
			return 26;
		return 25;
	}
	void GetHeadGib( BOOL *pfHeadGib, int *piGroup, int *piHeadless )
	{
		*pfHeadGib = TRUE;
		*piGroup = 1;
		*piHeadless = 1;
	}
	int GetGunsFiring( void ) { return 0; }
	BOOL HasUnlimitedAmmo( void ) { return TRUE; }
	void HandleAnimEvent( MonsterEvent_t *pEvent );
	BOOL CheckMeleeAttack1( float flDot, float flDist );
	BOOL CheckRangeAttack1( float flDot, float flDist );
	Vector GetGunPosition( void );
	CChemGrenade *ThrowChemical( Vector vecSrc );
	void PainSound( void );
	void AlertSound( void );
	void AttackHitSound( CBaseMonster *pVictim );
	void Killed( entvars_t *pevAttacker, int iGib );
	int  TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );

	int	Save( CSave &save );
	int Restore( CRestore &restore );
	static TYPEDESCRIPTION m_SaveData[];

	BOOL	m_bHasAmmo;
	float	m_painTime;
	float	m_fearTime;
	float	m_flNextGrenadeCheck;	// the base's aren't used
	BOOL	m_bThrowGrenade;
	Vector	m_vecTossVelocity;
	int		m_iChemType;			// which mixture the next throw is
};

LINK_ENTITY_TO_CLASS( monster_human_scientist, CHostileScientist );

TYPEDESCRIPTION	CHostileScientist::m_SaveData[] =
{
	DEFINE_FIELD( CHostileScientist, m_painTime, FIELD_TIME ),
	DEFINE_FIELD( CHostileScientist, m_fearTime, FIELD_TIME ),
	DEFINE_FIELD( CHostileScientist, m_flNextGrenadeCheck, FIELD_TIME ),
	DEFINE_FIELD( CHostileScientist, m_bThrowGrenade, FIELD_BOOLEAN ),
	DEFINE_FIELD( CHostileScientist, m_vecTossVelocity, FIELD_VECTOR ),
	DEFINE_FIELD( CHostileScientist, m_bHasAmmo, FIELD_BOOLEAN ),
};

IMPLEMENT_SAVERESTORE( CHostileScientist, CBaseBandit );

void CHostileScientist::Spawn( void )
{
	Precache( );

	SET_MODEL( ENT(pev), "models/evil_scientist.mdl" );

	pev->solid = SOLID_SLIDEBOX;
	pev->health = gSkillData.scientistHealth;
	m_MonsterState = MONSTERSTATE_NONE;
	UTIL_SetSize( pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX );

	m_bHasAmmo = TRUE;
	pev->health = gSkillData.scientistHealth * 2;
	pev->weapons = BANDIT_ROCKETLAUNCHER2;

	m_painTime = m_flNextGrenadeCheck = gpGlobals->time;
	m_bThrowGrenade = FALSE;
	m_iChemType = RANDOM_LONG( 0, 3 );
	m_bHasAmmo = TRUE;

	MonsterInit( );
	CBaseBandit::Spawn( );

	pev->view_ofs = Vector( 0, 0, 50 );
}

void CHostileScientist::Precache( void )
{
	g_iGoreHuman = PRECACHE_MODEL( "sprites/gorehuman.spr" );
	g_iGibHumanSpr = PRECACHE_MODEL( "sprites/gibhuman.spr" );

	PRECACHE_MODEL( "models/evil_scientist.mdl" );

	PRECACHE_SOUND( "gscientist/scientist_diebackwards.wav" );
	PRECACHE_SOUND( "gscientist/scientist_dieforwards.wav" );
	PRECACHE_SOUND( "gscientist/scientist_dieforwards2.wav" );
	PRECACHE_SOUND( "gscientist/scientist_diesimple.wav" );
	PRECACHE_SOUND( "gscientist/scientist_dieviolent.wav" );
	PRECACHE_SOUND( "gscientist/scientist_dieheadshot.wav" );
	PRECACHE_SOUND( "gscientist/hurtmask1.wav" );
	PRECACHE_SOUND( "gscientist/hurtmask2.wav" );
	PRECACHE_SOUND( "gscientist/hurtmask3.wav" );
	PRECACHE_SOUND( "gscientist/hurtmask4.wav" );

	UTIL_PrecacheOther( "weapon_SPchemicalgun" );

	CBaseBandit::Precache( );
}

BOOL CHostileScientist::CheckMeleeAttack1( float flDot, float flDist )
{
	if ( m_hEnemy != NULL && flDist < 48 )
		return TRUE;
	return FALSE;
}

//=========================================================
// CheckRangeAttack1 - can a blob reach the enemy?
//=========================================================
BOOL CHostileScientist::CheckRangeAttack1( float flDot, float flDist )
{
	// assume things haven't changed too much since last time
	if ( gpGlobals->time < m_flNextGrenadeCheck )
		return m_bThrowGrenade;

	if ( !HasUnlimitedAmmo() && HasConditions( bits_COND_NO_AMMO_LOADED ) )
	{
		m_bThrowGrenade = FALSE;
		return m_bThrowGrenade;
	}

	if ( !FBitSet( m_hEnemy->pev->flags, FL_ONGROUND ) && m_hEnemy->pev->waterlevel == 0 )
	{
		// don't throw at anything that isn't on the ground!
		m_bThrowGrenade = FALSE;
		return m_bThrowGrenade;
	}

	// find target
	Vector vecTarget = m_vecEnemyLKP + (m_hEnemy->BodyTarget( pev->origin ) - m_hEnemy->pev->origin);

	// estimate position
	if ( HasConditions( bits_COND_SEE_ENEMY ) )
		vecTarget = vecTarget + ((vecTarget - pev->origin).Length() / 900) * m_hEnemy->pev->velocity;

	Vector vecToss = VecCheckThrow( pev, GetGunPosition(), vecTarget, 600, 1.5 );

	if ( vecToss != g_vecZero )
	{
		m_bThrowGrenade = TRUE;
		m_vecTossVelocity = vecToss;
		m_flNextGrenadeCheck = gpGlobals->time + 0.3;
	}
	else
	{
		m_bThrowGrenade = FALSE;
		m_flNextGrenadeCheck = gpGlobals->time + 0.2;
	}

	return m_bThrowGrenade;
}

//=========================================================
// GetGunPosition - the chemical gun's muzzle
//=========================================================
Vector CHostileScientist::GetGunPosition( void )
{
	Vector vecGunPos;
	Vector vecGunAngles;

	GetAttachment( 0, vecGunPos, vecGunAngles );
	return vecGunPos;
}

//=========================================================
// ThrowChemical - one of four mixtures, stronger with the
// skill level; the source position isn't used. Returns the
// grenade (retail keeps it in eax), which the caller ignores
//=========================================================
CChemGrenade *CHostileScientist::ThrowChemical( Vector vecSrc )
{
	chemicalmixture_t mixture;	// sic: only some of it is set
	float flScale;
	int iBase;

	UTIL_MakeAimVectors( pev->angles );

	switch ( g_iSkillLevel )
	{
	case SKILL_MEDIUM:
		flScale = 1;
		iBase = 30;
		break;
	case SKILL_HARD:
		flScale = 1.3;
		iBase = 40;
		break;
	default:
		flScale = 0.7;
		iBase = 20;
		break;
	}

	switch ( m_iChemType )
	{
	case 0:
		mixture.chemical_type = 7;
		mixture.explosion = 3;
		mixture.stability = 80;
		mixture.viscosity = 4;
		mixture.volatility = flScale * 30 + iBase;
		break;
	case 1:
		mixture.chemical_type = 7;
		mixture.explosion = 3;
		mixture.stability = 30;
		mixture.viscosity = 43;
		mixture.volatility = flScale * 40 + iBase;
		break;
	case 2:
		mixture.chemical_type = 7;
		mixture.explosion = 1;
		mixture.stability = 0;
		mixture.viscosity = 100;
		mixture.volatility = flScale * 30 + iBase;
		break;
	case 3:
		mixture.chemical_type = 7;
		mixture.explosion = 3;
		mixture.stability = 20;
		mixture.viscosity = 69;
		mixture.volatility = flScale * 50 + iBase;
		break;
	}

	// now and then pick another mixture
	if ( RANDOM_LONG( 0, 5 ) == 0 )
		m_iChemType = RANDOM_LONG( 0, 3 );

	CChemGrenade *pGrenade;
	if ( mixture.explosion == 1 )
		pGrenade = CChemGrenade::ShootSplatter( pev, GetGunPosition(), m_vecTossVelocity, mixture );
	else
		pGrenade = CChemGrenade::ShootBouncy( pev, GetGunPosition(), m_vecTossVelocity, mixture );

	pGrenade->pev->gravity = 1.5;
	return pGrenade;
}

int CHostileScientist::Classify( void )
{
	return CLASS_HUMAN_MILITARY;
}

void CHostileScientist::SetYawSpeed( void )
{
	int ys;

	ys = 90;

	switch ( m_Activity )
	{
	case ACT_IDLE:
		ys = 120;
		break;
	case ACT_WALK:
		ys = 180;
		break;
	case ACT_RUN:
		ys = 150;
		break;
	case ACT_TURN_LEFT:
	case ACT_TURN_RIGHT:
		ys = 140;
		break;
	}

	pev->yaw_speed = ys;
}

void CHostileScientist::HandleAnimEvent( MonsterEvent_t *pEvent )
{
	switch ( pEvent->event )
	{
	case SCIENTIST_AE_THROW:
		ThrowChemical( pev->origin );
		m_flNextGrenadeCheck = gpGlobals->time + RANDOM_FLOAT( 2, 5 );
		m_bThrowGrenade = FALSE;
		break;

	case SCIENTIST_AE_PUNCH1:
	case SCIENTIST_AE_PUNCH2:
		{
			CBaseEntity *pHurt = CheckTraceHullAttack( 64, 10, DMG_SLASH );

			if ( pHurt )
			{
				pHurt->pev->punchangle.z = rand() % 8 - 4;
				pHurt->pev->punchangle.x = rand() % 8 - 4;

				CBaseMonster *pMonster = pHurt->MyMonsterPointer();
				if ( pMonster )
				{
					TraceResult tr;
					UTIL_TraceLine( BodyTarget( g_vecZero ), pMonster->BodyTarget( g_vecZero ), dont_ignore_monsters, ENT(pev), &tr );
					pMonster->TraceBleed( gSkillData.xenomeDmgMelee, ( pMonster->BodyTarget( g_vecZero ) - BodyTarget( g_vecZero ) ).Normalize(), &tr, DMG_SLASH, tr.vecEndPos );	// sic: the xenome's melee damage

					if ( pMonster->IsPlayer() )
						AttackHitSound( pMonster );
				}
			}
		}
		break;

	default:
		CBaseBandit::HandleAnimEvent( pEvent );
		break;
	}
}

void CHostileScientist::PainSound( void )
{
	if ( gpGlobals->time < m_painTime )
		return;

	if ( !IsAlive() || m_MonsterState == MONSTERSTATE_SCRIPT || m_MonsterState == MONSTERSTATE_PRONE || m_MonsterState == MONSTERSTATE_PLAYDEAD || m_MonsterState == MONSTERSTATE_NONE )
		return;

	m_painTime = gpGlobals->time + RANDOM_FLOAT( 0.5, 0.75 );

	switch ( RANDOM_LONG( 0, 3 ) )
	{
	case 0:
		EMIT_SOUND( ENT(pev), CHAN_VOICE, "gscientist/hurtmask1.wav", 1, ATTN_NORM );
		break;
	case 1:
		EMIT_SOUND( ENT(pev), CHAN_VOICE, "gscientist/hurtmask2.wav", 1, ATTN_NORM );
		break;
	case 2:
		EMIT_SOUND( ENT(pev), CHAN_VOICE, "gscientist/hurtmask3.wav", 1, ATTN_NORM );
		break;
	case 3:
		EMIT_SOUND( ENT(pev), CHAN_VOICE, "gscientist/hurtmask4.wav", 1, ATTN_NORM );
		break;
	}
}

void CHostileScientist::AlertSound( void )
{
}

void CHostileScientist::AttackHitSound( CBaseMonster *pVictim )
{
	EMIT_SOUND_DYN( pVictim->edict(), CHAN_VOICE, "debris/bustflesh1.wav", 0.3, ATTN_IDLE, 0, 110 );
}

void CHostileScientist::Killed( entvars_t *pevAttacker, int iGib )
{
	if ( m_bHasAmmo )
	{
		Vector vecGunPos;
		Vector vecGunAngles;

		m_bHasAmmo = FALSE;
		SetBodygroup( 3, 1 );

		GetAttachment( 0, vecGunPos, vecGunAngles );

		CBaseEntity *pGun = DropItem( "weapon_SPchemicalgun", vecGunPos, vecGunAngles );
		if ( pGun )
		{
			pGun->pev->velocity = Vector( RANDOM_FLOAT( -100, 100 ), RANDOM_FLOAT( -100, 100 ), RANDOM_FLOAT( 200, 300 ) );
			pGun->pev->avelocity = Vector( 0, RANDOM_FLOAT( 200, 400 ), 0 );
		}
	}

	CBaseBandit::Killed( pevAttacker, iGib );
}

int CHostileScientist::TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	PainSound( );
	return CBaseBandit::TakeDamage( pevInflictor, pevAttacker, flDamage, bitsDamageType );
}

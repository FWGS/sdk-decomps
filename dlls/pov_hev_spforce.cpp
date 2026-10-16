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
// POV: monster_hev_spforce, a special forces soldier in an
// HEV suit. "weaponused" 1-3 is the M41A (3 also precaches
// the grenade launcher's sound), 4-5 the beretta. Talks with
// the human grunt's sentences, is in the special forces'
// class and hates what the grunts hate. One of CBaseBarn's
// monsters (basebarn.h, pov_barn.cpp). File and class names
// and the names retail doesn't give are ours.
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

// m_iWeaponUsed ("weaponused" key)
#define SPF_WEAPON_M41A			1
#define SPF_WEAPON_M41A2		2
#define SPF_WEAPON_M41A_GL		3
#define SPF_WEAPON_BERETTA		4
#define SPF_WEAPON_BERETTA2		5

// gun bodygroup
#define SPF_BODY_GUNHOLSTERED	0
#define SPF_BODY_BERETTA		1
#define SPF_BODY_M41A			2
#define SPF_BODY_GUNGONE		4

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define		SPF_AE_SHOOT		( 3 )
#define		SPF_AE_RELOAD		( 5 )
#define		SPF_AE_SHOOT2		( 7 )
#define		SPF_AE_BURST1		( 8 )	// fire without the shot sound
#define		SPF_AE_BURST2		( 9 )

class CHEVSPForce : public CBaseBarn	// name ours
{
public:
	void Spawn( void );
	void Precache( void );
	void KeyValue( KeyValueData *pkvd );
	int  Classify ( void );
	int IRelationship ( CBaseEntity *pTarget );

	void SetupWeapon( void );
	void SetGun( int iGun );
	BOOL HasGun( void );
	int GetHead( void );

	BOOL CheckRangeAttack1 ( float flDot, float flDist );
	BOOL CheckRangeAttack2 ( float flDot, float flDist );

	void FireWeapon( BOOL fSound );
	void FireM41A( BOOL fSound );
	void FireBeretta( BOOL fSound );
	void WeaponSound( void );
	void ReloadSound( void );

	void HandleAnimEvent( MonsterEvent_t *pEvent );
	void SetActivity ( Activity NewActivity );
	void Killed( entvars_t *pevAttacker, int iGib );

	void TalkInit( void );
};

LINK_ENTITY_TO_CLASS( monster_hev_spforce, CHEVSPForce );

//=========================================================
// Spawn
//=========================================================
void CHEVSPForce :: Spawn()
{
	CBaseBarn::Spawn();

	SET_MODEL(ENT(pev), "models/hev_spforce.mdl");
	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	SetupWeapon();

	if ( pev->spawnflags & 8 )
		pev->health		= 80000;
	else
		pev->health		= gSkillData.hevbarnsHealth;

	m_fUsesMeleeAttack	= TRUE;
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CHEVSPForce :: Precache()
{
	PRECACHE_MODEL("models/hev_spforce.mdl");

	CBaseBarn::Precache();

	switch ( m_iWeaponUsed )
	{
	case SPF_WEAPON_M41A_GL:
		PRECACHE_SOUND("weapons/glauncher.wav" );
	case SPF_WEAPON_M41A:
	case SPF_WEAPON_M41A2:
		PRECACHE_SOUND("weapons/m41ahks1.wav" );
		PRECACHE_SOUND("weapons/m41ahks2.wav" );
		PRECACHE_SOUND("spforce/spf_reload1.wav" );
		break;
	case SPF_WEAPON_BERETTA:
	case SPF_WEAPON_BERETTA2:
		PRECACHE_SOUND("weapons/beretta_fire1.wav" );
		PRECACHE_SOUND("spforce/spf_reload2.wav" );
		break;
	}

	TalkInit();
}

//=========================================================
// SetupWeapon - clip and gun for the weapon a mapper chose
//=========================================================
void CHEVSPForce :: SetupWeapon( void )
{
	pev->body = SPF_BODY_GUNHOLSTERED;

	switch ( m_iWeaponUsed )
	{
	case SPF_WEAPON_BERETTA:
	case SPF_WEAPON_BERETTA2:
		m_cAmmoLoaded = m_cClipSize = 15;
		SetGun( SPF_BODY_BERETTA );
		m_fGunDrawn = TRUE;
		m_fStanding = TRUE;
		break;
	default:
		m_iWeaponUsed = SPF_WEAPON_M41A;
	case SPF_WEAPON_M41A:
	case SPF_WEAPON_M41A2:
	case SPF_WEAPON_M41A_GL:
		m_cAmmoLoaded = m_cClipSize = 45;
		SetGun( SPF_BODY_M41A );
		m_fGunDrawn = TRUE;
		m_fStanding = FALSE;
		break;
	}
}

//=========================================================
// SetGun - the HEV models have the head in bodygroup 1
// and the gun in bodygroup 2
//=========================================================
void CHEVSPForce :: SetGun( int iGun )
{
	pev->body = 0;
	SetBodygroup( 1, GetHead() );
	SetBodygroup( 2, iGun );
}

//=========================================================
// HasGun
//=========================================================
BOOL CHEVSPForce :: HasGun( void )
{
	if ( GetBodygroup( 2 ) == SPF_BODY_GUNGONE || GetBodygroup( 2 ) == SPF_BODY_GUNHOLSTERED )
		return FALSE;

	return TRUE;
}

//=========================================================
// CheckRangeAttack1 (the human grunt's)
//=========================================================
BOOL CHEVSPForce :: CheckRangeAttack1 ( float flDot, float flDist )
{
	if ( flDist <= m_flDistTooFar && flDot >= 0.5 )
	{
		if ( m_hEnemy->IsPlayer() || flDist > 64 )
		{
			TraceResult	tr;

			Vector vecSrc = GetGunPosition();

			// verify that a bullet fired from the gun will hit the enemy before the world.
			UTIL_TraceLine( vecSrc, m_hEnemy->BodyTarget(vecSrc), dont_ignore_monsters, dont_ignore_glass, ENT(pev), &tr);

			if ( tr.flFraction == 1.0 || (tr.pHit != NULL && CBaseEntity::Instance(tr.pHit) == m_hEnemy) )
			{
				return TRUE;
			}
		}
	}

	return FALSE;
}

//=========================================================
// CheckRangeAttack2 - never. Retail keeps only the load of
// m_iWeaponUsed: two tests whose arms all return FALSE (the
// weapons tested are a guess; VC6 drops the compares).
//=========================================================
BOOL CHEVSPForce :: CheckRangeAttack2 ( float flDot, float flDist )
{
	if ( m_iWeaponUsed == SPF_WEAPON_M41A_GL )
		return FALSE;
	else if ( m_iWeaponUsed == SPF_WEAPON_M41A )
		return FALSE;

	return FALSE;
}

//=========================================================
// FireWeapon - shoots the gun this soldier carries
//=========================================================
void CHEVSPForce :: FireWeapon( BOOL fSound )
{
	switch ( m_iWeaponUsed )
	{
	case SPF_WEAPON_M41A:
	case SPF_WEAPON_M41A2:
	case SPF_WEAPON_M41A_GL:
		FireM41A( fSound );
		break;
	case SPF_WEAPON_BERETTA:
	case SPF_WEAPON_BERETTA2:
		FireBeretta( fSound );
		break;
	}
}

//=========================================================
// FireM41A (CBaseBarn's FireMP5)
//=========================================================
void CHEVSPForce :: FireM41A ( BOOL fSound )
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
// FireBeretta (CBaseBarn's FireMP5)
//=========================================================
void CHEVSPForce :: FireBeretta ( BOOL fSound )
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
	FireBullets(1, vecShootOrigin, vecShootDir, VECTOR_CONE_4DEGREES, m_flDistTooFar, BULLET_MONSTER_9MM, 0 );

	pev->effects |= EF_MUZZLEFLASH;
	UTIL_MuzzleLight( this, vecShootOrigin );

	if ( fSound )
		WeaponSound();

	m_cAmmoLoaded--;// take away a bullet!

	Vector angDir = UTIL_VecToAngles( vecShootDir );
	SetBlending( 0, angDir.x );
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CHEVSPForce :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	switch( pEvent->event )
	{
	case SPF_AE_RELOAD:
		ReloadSound();
		m_cAmmoLoaded = m_cClipSize;
		ClearConditions(bits_COND_NO_AMMO_LOADED);
		break;

	case SPF_AE_SHOOT:
	case SPF_AE_SHOOT2:
		FireWeapon( TRUE );
		break;

	case SPF_AE_BURST1:
	case SPF_AE_BURST2:
		FireWeapon( FALSE );
		break;

	default:
		CTalkMonster::HandleAnimEvent( pEvent );
	}
}

//=========================================================
// SetActivity
//=========================================================
void CHEVSPForce :: SetActivity ( Activity NewActivity )
{
	int	iSequence = ACTIVITY_NOT_AVAILABLE;
	void *pmodel = GET_MODEL_PTR( ENT(pev) );

	switch ( NewActivity )
	{
	case ACT_RANGE_ATTACK1:
		switch ( m_iWeaponUsed )
		{
		case SPF_WEAPON_M41A:
		case SPF_WEAPON_M41A2:
		case SPF_WEAPON_M41A_GL:
			iSequence = LookupSequence( "shootgun2" );
			break;
		case SPF_WEAPON_BERETTA:
		case SPF_WEAPON_BERETTA2:
			iSequence = LookupSequence( "shootgun" );
			break;
		}
		break;
	case ACT_MELEE_ATTACK1:
		if ( !m_fUsesMeleeAttack )
		{
			CTalkMonster::SetActivity( NewActivity );
			return;
		}
		m_KarateMoveNum = -1;
		iSequence = LookupSequence( "karate_hit" );
		break;
	case ACT_RELOAD:
		switch ( m_iWeaponUsed )
		{
		case SPF_WEAPON_M41A:
		case SPF_WEAPON_M41A2:
		case SPF_WEAPON_M41A_GL:
			iSequence = LookupSequence( "reload3" );
			break;
		case SPF_WEAPON_BERETTA:
		case SPF_WEAPON_BERETTA2:
			iSequence = LookupSequence( "reload" );
			break;
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
// WeaponSound - the shot
//=========================================================
void CHEVSPForce :: WeaponSound( void )
{
	int pitchShift = RANDOM_LONG( 0, 20 );

	// Only shift about half the time
	if ( pitchShift > 10 )
		pitchShift = 0;
	else
		pitchShift -= 5;

	switch ( m_iWeaponUsed )
	{
	case SPF_WEAPON_M41A:
	case SPF_WEAPON_M41A2:
	case SPF_WEAPON_M41A_GL:
		if ( RANDOM_LONG( 0, 1 ) == 0 )
			EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, "weapons/m41ahks1.wav", 1, ATTN_NORM, 0, 100 + pitchShift );
		else
			EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, "weapons/m41ahks2.wav", 1, ATTN_NORM, 0, 100 + pitchShift );
		break;
	case SPF_WEAPON_BERETTA:
	case SPF_WEAPON_BERETTA2:
		EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, "weapons/beretta_fire1.wav", 1, ATTN_NORM, 0, 100 + pitchShift );
		break;
	}

	CSoundEnt::InsertSound ( bits_SOUND_COMBAT, pev->origin, 384, 0.3 );
}

//=========================================================
// ReloadSound
//=========================================================
void CHEVSPForce :: ReloadSound( void )
{
	switch ( m_iWeaponUsed )
	{
	case SPF_WEAPON_M41A:
	case SPF_WEAPON_M41A2:
	case SPF_WEAPON_M41A_GL:
		EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, "spforce/spf_reload1.wav", 1, ATTN_NORM, 0, PITCH_NORM );
		break;
	case SPF_WEAPON_BERETTA:
	case SPF_WEAPON_BERETTA2:
		EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, "spforce/spf_reload2.wav", 1, ATTN_NORM, 0, PITCH_NORM );
		break;
	}
}

void CHEVSPForce::Killed( entvars_t *pevAttacker, int iGib )
{
	if ( HasGun() )
	{// drop the gun!
		switch ( m_iWeaponUsed )
		{
		case SPF_WEAPON_M41A:
		case SPF_WEAPON_M41A2:
		case SPF_WEAPON_M41A_GL:
			SetGun( SPF_BODY_GUNHOLSTERED );
			UTIL_DropGun( this, "weapon_9mmm41a", TRUE );
			break;
		case SPF_WEAPON_BERETTA:
		case SPF_WEAPON_BERETTA2:
			SetGun( SPF_BODY_GUNHOLSTERED );
			UTIL_DropGun( this, "weapon_beretta", TRUE );
			break;
		}
	}

	CTalkMonster::Killed( pevAttacker, iGib );
}

// Init talk data
void CHEVSPForce :: TalkInit()
{

	CTalkMonster::TalkInit();

	// the human grunt's sentence groups (group names are in sentences.txt)

	m_szGrp[TLK_ANSWER]  =	"HG_ANSWER";
	m_szGrp[TLK_QUESTION] =	"HG_QUESTION";
	m_szGrp[TLK_IDLE] =		"HG_IDLE";
	m_szGrp[TLK_SMELL] =	"HG_CHECK";

	m_szGrp[TLK_WOUND] =	"HG_TAUNT";
	m_szGrp[TLK_MORTAL] =	"HG_TAUNT";

	m_szGrp[TLK_PAIN] =		"HG_PAIN";
	m_szGrp[TLK_DIE] =		"HG_DIE";
	m_szGrp[TLK_ATTACK] =	"HG_ALERT";
	m_szGrp[TLK_KILL] =		"HG_CLEAR";

	m_voicePitch = 92 + RANDOM_LONG( 0, 6 );
}

int CHEVSPForce :: GetHead( void )
{
	return 0;
}

void CHEVSPForce :: KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "weaponused"))
	{
		if ( atoi(pkvd->szValue) > 0 && atoi(pkvd->szValue) <= 6 )
			m_iWeaponUsed = atoi(pkvd->szValue);
		else
			m_iWeaponUsed = SPF_WEAPON_M41A;
		pkvd->fHandled = TRUE;
	}
	else
		CBaseMonster::KeyValue( pkvd );
}

//=========================================================
// Classify - indicates this monster's place in the
// relationship table.
//=========================================================
int	CHEVSPForce :: Classify ( void )
{
	return	CLASS_HUMAN_SPFORCE;
}

//=========================================================
// IRelationship - the human grunt's: alien grunts and
// gargantuas are the nemesis.
//=========================================================
int CHEVSPForce::IRelationship ( CBaseEntity *pTarget )
{
	if ( FClassnameIs( pTarget->pev, "monster_alien_grunt" ) || ( FClassnameIs( pTarget->pev,  "monster_gargantua" ) ) )
	{
		return R_NM;
	}

	return CBaseBarn::IRelationship( pTarget );
}

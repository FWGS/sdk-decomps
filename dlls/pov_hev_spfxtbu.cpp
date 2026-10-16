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
// POV: monster_hev_spforce_xtbu, the HEV special forces
// soldier who carries an XTBU (gun bodygroup 3) and doesn't
// shoot: the orange helmet XTBU carrier's code on the HEV
// special forces' model, sentences, class and relationships.
// With spawnflag 8 he starts with the XTBU; the animation
// event 20 takes it (removes the prop entity named by
// "xtbupropent"). Killed with it, he drops an
// item_xtbs_silver_onfloor. One of CBaseBarn's monsters
// (basebarn.h, pov_barn.cpp). File name ours: it has to sort
// after pov_hev_spfscihelmet.cpp, as retail links it.
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

// m_iWeaponUsed (name ours)
#define HEV_WEAPON_XTBU		6

// gun bodygroup (names ours)
#define SPF_BODY_XTBU		3
#define SPF_BODY_GUNGONE	4

// spawnflags (name ours)
#define SF_SPFXTBU_HASXTBU		8		// starts with the XTBU

// animation event (name ours)
#define HEVXTBU_AE_TAKEXTBU		20

class CHEVSPForceXTBU : public CBaseBarn
{
public:
	void Spawn( void );
	void Precache( void );
	void KeyValue( KeyValueData *pkvd );
	int  Classify ( void );
	int IRelationship ( CBaseEntity *pTarget );

	BOOL CheckRangeAttack1 ( float flDot, float flDist ) { return FALSE; }
	BOOL CheckRangeAttack2 ( float flDot, float flDist ) { return FALSE; }

	void SetupWeapon( void );
	void SetGun( int iGun );
	BOOL HasGun( void );
	int GetHead( void );

	void HandleAnimEvent( MonsterEvent_t *pEvent );
	void SetActivity ( Activity NewActivity );
	void Killed( entvars_t *pevAttacker, int iGib );

	void TalkInit( void );

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	string_t	m_iszXTBUPropEnt;
};

LINK_ENTITY_TO_CLASS( monster_hev_spforce_xtbu, CHEVSPForceXTBU );

TYPEDESCRIPTION	CHEVSPForceXTBU::m_SaveData[] =
{
	DEFINE_FIELD( CHEVSPForceXTBU, m_iszXTBUPropEnt, FIELD_STRING ),
};

IMPLEMENT_SAVERESTORE( CHEVSPForceXTBU, CBaseBarn );

//=========================================================
// Spawn
//=========================================================
void CHEVSPForceXTBU :: Spawn()
{
	CBaseBarn::Spawn();

	SET_MODEL(ENT(pev), "models/hev_spforce.mdl");
	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	if ( pev->spawnflags & SF_SPFXTBU_HASXTBU )
		m_iWeaponUsed = HEV_WEAPON_XTBU;
	else
		m_iWeaponUsed = BARN_WEAPON_GLOCK;

	SetupWeapon();

	pev->health			= gSkillData.hevbarnsHealth;

	m_fUsesMeleeAttack	= TRUE;
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CHEVSPForceXTBU :: Precache()
{
	PRECACHE_MODEL("models/hev_spforce.mdl");

	CBaseBarn::Precache();

	TalkInit();
}

//=========================================================
// SetupWeapon - no gun, or the XTBU
//=========================================================
void CHEVSPForceXTBU :: SetupWeapon( void )
{
	pev->body = BARN_BODY_GUNHOLSTERED;

	switch ( m_iWeaponUsed )
	{
	case BARN_WEAPON_GLOCK:
		m_cAmmoLoaded = m_cClipSize = 1;
		SetGun( BARN_BODY_GUNHOLSTERED );
		m_fGunDrawn = FALSE;
		break;
	case HEV_WEAPON_XTBU:
		m_cAmmoLoaded = m_cClipSize = 1;
		SetGun( SPF_BODY_XTBU );
		m_fGunDrawn = FALSE;
		break;
	}
}

//=========================================================
// SetGun - the HEV models have the head in bodygroup 1
// and the gun in bodygroup 2
//=========================================================
void CHEVSPForceXTBU :: SetGun( int iGun )
{
	pev->body = 0;
	SetBodygroup( 1, GetHead() );
	SetBodygroup( 2, iGun );
}

//=========================================================
// HasGun
//=========================================================
BOOL CHEVSPForceXTBU :: HasGun( void )
{
	if ( GetBodygroup( 2 ) == SPF_BODY_GUNGONE || GetBodygroup( 2 ) == BARN_BODY_GUNHOLSTERED )
		return FALSE;

	return TRUE;
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CHEVSPForceXTBU :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	switch( pEvent->event )
	{
	case HEVXTBU_AE_TAKEXTBU:
		if ( m_iszXTBUPropEnt )
		{
			char	*pszTarget = "";	// sic: POV copies the name into a string literal

			strcpy( pszTarget, STRING( m_iszXTBUPropEnt ) );

			edict_t *pentTarget = FIND_ENTITY_BY_TARGETNAME( NULL, pszTarget );
			while ( !FNullEnt( pentTarget ) )
			{
				UTIL_Remove( CBaseEntity::Instance( pentTarget ) );
				pentTarget = FIND_ENTITY_BY_TARGETNAME( pentTarget, pszTarget );
			}
		}
		else
		{
			ALERT( at_console, "HEVSPFXTBU: Warning - no XTBU prop ent name given\n" );
		}

		m_iWeaponUsed = HEV_WEAPON_XTBU;
		SetupWeapon();
		break;

	default:
		CTalkMonster::HandleAnimEvent( pEvent );
	}
}

//=========================================================
// SetActivity
//=========================================================
void CHEVSPForceXTBU :: SetActivity ( Activity NewActivity )
{
	int	iSequence = ACTIVITY_NOT_AVAILABLE;
	void *pmodel = GET_MODEL_PTR( ENT(pev) );

	switch ( NewActivity )
	{
	case ACT_MELEE_ATTACK1:
		if ( !m_fUsesMeleeAttack )
		{
			CTalkMonster::SetActivity( NewActivity );
			return;
		}
		m_KarateMoveNum = -1;
		iSequence = LookupSequence( "karate_hit" );
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

void CHEVSPForceXTBU::Killed( entvars_t *pevAttacker, int iGib )
{
	if ( HasGun() && m_iWeaponUsed == HEV_WEAPON_XTBU )
	{// drop the XTBU!
		SetGun( BARN_BODY_GUNHOLSTERED );
		UTIL_DropGun( this, "item_xtbs_silver_onfloor", TRUE );
	}

	CTalkMonster::Killed( pevAttacker, iGib );
}

// Init talk data
void CHEVSPForceXTBU :: TalkInit()
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

int CHEVSPForceXTBU :: GetHead( void )
{
	return 0;
}

void CHEVSPForceXTBU :: KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "xtbupropent"))
	{
		m_iszXTBUPropEnt = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		CBaseMonster::KeyValue( pkvd );
}

//=========================================================
// Classify - indicates this monster's place in the
// relationship table.
//=========================================================
int	CHEVSPForceXTBU :: Classify ( void )
{
	return	CLASS_HUMAN_SPFORCE;
}

//=========================================================
// IRelationship - the human grunt's: alien grunts and
// gargantuas are the nemesis.
//=========================================================
int CHEVSPForceXTBU::IRelationship ( CBaseEntity *pTarget )
{
	if ( FClassnameIs( pTarget->pev, "monster_alien_grunt" ) || ( FClassnameIs( pTarget->pev,  "monster_gargantua" ) ) )
	{
		return R_NM;
	}

	return CBaseBarn::IRelationship( pTarget );
}

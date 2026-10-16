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
// POV: monster_hev_orangehelmet_xtbu, the orange helmeted
// HEV scientist who carries an XTBU (gun bodygroup 6) and
// doesn't shoot. With spawnflag 32 he starts with it; the
// animation event 20 takes it: it removes the prop entity
// named by "xtbupropent" and puts the XTBU in his hands.
// Killed with it, he drops an item_xtbs_silver_onfloor. One
// of CBaseBarn's monsters (basebarn.h, pov_barn.cpp). File
// name ours.
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

// m_iWeaponUsed, gun bodygroup (name ours)
#define HEV_WEAPON_XTBU		6

// spawnflags (names ours)
#define SF_HEVXTBU_STRONG		8		// 80000 health
#define SF_HEVXTBU_HASXTBU		32		// starts with the XTBU

// animation event (name ours)
#define HEVXTBU_AE_TAKEXTBU		20

class CHEVOHelmetXTBU : public CBaseBarn
{
public:
	void Spawn( void );
	void Precache( void );
	void KeyValue( KeyValueData *pkvd );

	int GetHead( void ) { return 2; }
	BOOL CheckRangeAttack1 ( float flDot, float flDist ) { return FALSE; }
	BOOL CheckRangeAttack2 ( float flDot, float flDist ) { return FALSE; }

	void SetupWeapon( void );
	void SetGun( int iGun );
	BOOL HasGun( void );

	void HandleAnimEvent( MonsterEvent_t *pEvent );
	void SetActivity ( Activity NewActivity );
	void Killed( entvars_t *pevAttacker, int iGib );

	void TalkInit( void );

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	string_t	m_iszXTBUPropEnt;
};

LINK_ENTITY_TO_CLASS( monster_hev_orangehelmet_xtbu, CHEVOHelmetXTBU );

TYPEDESCRIPTION	CHEVOHelmetXTBU::m_SaveData[] =
{
	DEFINE_FIELD( CHEVOHelmetXTBU, m_iszXTBUPropEnt, FIELD_STRING ),
};

IMPLEMENT_SAVERESTORE( CHEVOHelmetXTBU, CBaseBarn );

//=========================================================
// Spawn
//=========================================================
void CHEVOHelmetXTBU :: Spawn()
{
	CBaseBarn::Spawn();

	SET_MODEL(ENT(pev), "models/hev_scientist.mdl");
	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	if ( pev->spawnflags & SF_HEVXTBU_HASXTBU )
		m_iWeaponUsed = HEV_WEAPON_XTBU;
	else
		m_iWeaponUsed = BARN_WEAPON_GLOCK;

	SetupWeapon();

	if ( pev->spawnflags & SF_HEVXTBU_STRONG )
		pev->health		= 80000;
	else
		pev->health		= gSkillData.hevbarnsHealth;

	m_fUsesMeleeAttack	= TRUE;
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CHEVOHelmetXTBU :: Precache()
{
	PRECACHE_MODEL("models/hev_scientist.mdl");

	CBaseBarn::Precache();

	TalkInit();
}

//=========================================================
// SetupWeapon - no gun, or the XTBU
//=========================================================
void CHEVOHelmetXTBU :: SetupWeapon( void )
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
		SetGun( HEV_WEAPON_XTBU );
		m_fGunDrawn = FALSE;
		break;
	}
}

//=========================================================
// SetGun - the HEV models have the head in bodygroup 1
// and the gun in bodygroup 2
//=========================================================
void CHEVOHelmetXTBU :: SetGun( int iGun )
{
	pev->body = 0;
	SetBodygroup( 1, GetHead() );
	SetBodygroup( 2, iGun );
}

//=========================================================
// HasGun
//=========================================================
BOOL CHEVOHelmetXTBU :: HasGun( void )
{
	if ( GetBodygroup( 2 ) == BARN_BODY_GUNGONE || GetBodygroup( 2 ) == BARN_BODY_GUNHOLSTERED )
		return FALSE;

	return TRUE;
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CHEVOHelmetXTBU :: HandleAnimEvent( MonsterEvent_t *pEvent )
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
			ALERT( at_console, "HEVORHXTBU: Warning - no XTBU prop ent name given\n" );
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
void CHEVOHelmetXTBU :: SetActivity ( Activity NewActivity )
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

void CHEVOHelmetXTBU::Killed( entvars_t *pevAttacker, int iGib )
{
	if ( HasGun() && m_iWeaponUsed == HEV_WEAPON_XTBU )
	{// drop the XTBU!
		SetGun( BARN_BODY_GUNHOLSTERED );
		UTIL_DropGun( this, "item_xtbs_silver_onfloor", TRUE );
	}

	CTalkMonster::Killed( pevAttacker, iGib );
}

// Init talk data
void CHEVOHelmetXTBU :: TalkInit()
{

	CTalkMonster::TalkInit();

	// scientists speach group names (group names are in sentences.txt)

	m_szGrp[TLK_ANSWER]  =	"SC_ANSWER";
	m_szGrp[TLK_QUESTION] =	"SC_QUESTION";
	m_szGrp[TLK_IDLE] =		"SC_IDLE";
	m_szGrp[TLK_SMELL] =	"SC_SMELL";

	m_szGrp[TLK_WOUND] =	"SC_WOUND";
	m_szGrp[TLK_MORTAL] =	"SC_MORTAL";

	m_szGrp[TLK_PAIN] =		"SC_PAIN";
	m_szGrp[TLK_DIE] =		"SC_PAIN";
	m_szGrp[TLK_ATTACK] =	"SC_ATTACK";
	m_szGrp[TLK_KILL] =		"SC_KILL";

	// get voice for head
	switch ( RANDOM_LONG( 0, 3 ) )
	{
	default:
	case 0:	m_voicePitch = 105; break;	//glasses
	case 1: m_voicePitch = 100; break;	//einstein
	case 2:	m_voicePitch = 95;  break;	//luther
	case 3:	m_voicePitch = 100;  break;	//slick
	}
}

void CHEVOHelmetXTBU :: KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "xtbupropent"))
	{
		m_iszXTBUPropEnt = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		CBaseMonster::KeyValue( pkvd );
}

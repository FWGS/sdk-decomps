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
// POV: monster_hev_bluehelmet, a guard in an HEV suit with
// the blue helmet (head 2 of the HEV barney's model);
// "monssex" picks barney's (1) or barniel's (2) voice, 0 a
// random one. One of CBaseBarn's monsters (basebarn.h,
// pov_barn.cpp). File name ours.
//=========================================================

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"basebarn.h"

class CHEVBlueHelmet : public CBaseBarn
{
public:
	void Spawn( void );
	void Precache( void );
	void KeyValue( KeyValueData *pkvd );

	int GetHead( void ) { return 2; }
	void SetGun( int iGun );
	BOOL HasGun( void );

	void TalkInit( void );

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	int		m_iBlueHelmetSex;
};

LINK_ENTITY_TO_CLASS( monster_hev_bluehelmet, CHEVBlueHelmet );

TYPEDESCRIPTION	CHEVBlueHelmet::m_SaveData[] =
{
	DEFINE_FIELD( CHEVBlueHelmet, m_iBlueHelmetSex, FIELD_INTEGER ),
};

IMPLEMENT_SAVERESTORE( CHEVBlueHelmet, CBaseBarn );

//=========================================================
// Spawn
//=========================================================
void CHEVBlueHelmet :: Spawn()
{
	CBaseBarn::Spawn();

	SET_MODEL(ENT(pev), "models/hev_barney.mdl");
	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	SetupWeapon();

	pev->health			= gSkillData.hevbarnsHealth;
	m_fUsesMeleeAttack	= TRUE;
}

void CHEVBlueHelmet :: KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "monssex"))
	{
		if ( atoi(pkvd->szValue) >= 0 && atoi(pkvd->szValue) <= 2 )
			m_iBlueHelmetSex = atoi(pkvd->szValue);
		else
			m_iBlueHelmetSex = 0;
		pkvd->fHandled = TRUE;
	}
	else
		CBaseMonster::KeyValue( pkvd );	// sic: "weaponused" is never read
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CHEVBlueHelmet :: Precache()
{
	PRECACHE_MODEL("models/hev_barney.mdl");

	CBaseBarn::Precache();

	TalkInit();
}

// Init talk data
void CHEVBlueHelmet :: TalkInit()
{

	CTalkMonster::TalkInit();

	if ( m_iBlueHelmetSex == 0 )
		m_iBlueHelmetSex = RANDOM_LONG( 1, 2 );

	// speach group names (group names are in sentences.txt)

	if ( m_iBlueHelmetSex == 2 )
	{
		m_szGrp[TLK_ANSWER]  =	"BN_ANSWER";
		m_szGrp[TLK_QUESTION] =	"BN_QUESTION";
		m_szGrp[TLK_IDLE] =		"BN_IDLE";
		m_szGrp[TLK_PIDLE] =	"BN_PIDLE";
		m_szGrp[TLK_PQUESTION] = "BN_PQUEST";

		m_szGrp[TLK_SMELL] =	"BN_SMELL";

		m_szGrp[TLK_WOUND] =	"BN_WOUND";
		m_szGrp[TLK_MORTAL] =	"BN_MORTAL";

		m_szGrp[TLK_PAIN] =		"BN_PAIN";
		m_szGrp[TLK_DIE] =		"BN_DIE";
		m_szGrp[TLK_ATTACK] =	"BN_ATTACK";
		m_szGrp[TLK_KILL] =		"BN_KILL";
	}
	else
	{
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
	}

	m_voicePitch = RANDOM_LONG( 98, 102 );
}

//=========================================================
// SetGun - the HEV models have the head in bodygroup 1
// and the gun in bodygroup 2
//=========================================================
void CHEVBlueHelmet :: SetGun( int iGun )
{
	pev->body = 0;
	SetBodygroup( 1, GetHead() );
	SetBodygroup( 2, iGun );
}

//=========================================================
// HasGun
//=========================================================
BOOL CHEVBlueHelmet :: HasGun( void )
{
	if ( GetBodygroup( 2 ) == BARN_BODY_GUNGONE || GetBodygroup( 2 ) == BARN_BODY_GUNHOLSTERED )
		return FALSE;

	return TRUE;
}

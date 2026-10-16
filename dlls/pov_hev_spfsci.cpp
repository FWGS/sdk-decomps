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
// POV: monster_hev_spfsci, a special forces scientist in an HEV suit
// (models/hev_spfsci.mdl, head 0), in the special forces'
// class.
// One of CBaseBarn's monsters (basebarn.h, pov_barn.cpp).
// File name ours.
//=========================================================

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"basebarn.h"

class CHEVSPFSci : public CBaseBarn
{
public:
	void Spawn( void );
	void Precache( void );
	int  Classify ( void );

	int GetHead( void );
	void SetGun( int iGun );
	BOOL HasGun( void );

	void TalkInit( void );

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	int		m_iHeadUsed;
};

LINK_ENTITY_TO_CLASS( monster_hev_spfsci, CHEVSPFSci );

TYPEDESCRIPTION	CHEVSPFSci::m_SaveData[] =
{
	DEFINE_FIELD( CHEVSPFSci, m_iHeadUsed, FIELD_INTEGER ),
};

IMPLEMENT_SAVERESTORE( CHEVSPFSci, CBaseBarn );

//=========================================================
// Spawn
//=========================================================
void CHEVSPFSci :: Spawn()
{
	pev->body = 0;
	m_iHeadUsed = 0;

	CBaseBarn::Spawn();

	SET_MODEL(ENT(pev), "models/hev_spfsci.mdl");
	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	SetupWeapon();

	pev->health			= gSkillData.hevbarnsHealth;
	m_fUsesMeleeAttack	= TRUE;
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CHEVSPFSci :: Precache()
{
	PRECACHE_MODEL("models/hev_spfsci.mdl");

	CBaseBarn::Precache();

	TalkInit();
}

// Init talk data
void CHEVSPFSci :: TalkInit()
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

	m_voicePitch = RANDOM_LONG( 95, 105 );
}

int CHEVSPFSci :: GetHead( void )
{
	return m_iHeadUsed;
}

//=========================================================
// SetGun - the HEV models have the head in bodygroup 1
// and the gun in bodygroup 2
//=========================================================
void CHEVSPFSci :: SetGun( int iGun )
{
	pev->body = 0;
	SetBodygroup( 1, GetHead() );
	SetBodygroup( 2, iGun );
}

//=========================================================
// HasGun
//=========================================================
BOOL CHEVSPFSci :: HasGun( void )
{
	if ( GetBodygroup( 2 ) == BARN_BODY_GUNGONE || GetBodygroup( 2 ) == BARN_BODY_GUNHOLSTERED )
		return FALSE;

	return TRUE;
}

//=========================================================
// Classify - indicates this monster's place in the
// relationship table.
//=========================================================
int	CHEVSPFSci :: Classify ( void )
{
	return	CLASS_HUMAN_SPFORCE;
}

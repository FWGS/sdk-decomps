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
// POV: monster_hev_scientist, a scientist in an HEV suit.
// The mapper's "body" picks the head (m_iHeadUsed; only head
// 1 is used). One of CBaseBarn's monsters (basebarn.h,
// pov_barn.cpp). File name ours.
//=========================================================

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"basebarn.h"

class CHEVScientist : public CBaseBarn
{
public:
	void Spawn( void );
	void Precache( void );

	int GetHead( void );
	void SetGun( int iGun );
	BOOL HasGun( void );

	void TalkInit( void );

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	int		m_iHeadUsed;
};

LINK_ENTITY_TO_CLASS( monster_hev_scientist, CHEVScientist );

TYPEDESCRIPTION	CHEVScientist::m_SaveData[] =
{
	DEFINE_FIELD( CHEVScientist, m_iHeadUsed, FIELD_INTEGER ),
};

IMPLEMENT_SAVERESTORE( CHEVScientist, CBaseBarn );

//=========================================================
// Spawn
//=========================================================
void CHEVScientist :: Spawn()
{
	m_iHeadUsed = pev->body;
	pev->body = 0;

	switch ( m_iHeadUsed )
	{
	case 0:
		m_iHeadUsed = 1;
		break;
	case 1:
		m_iHeadUsed = 1;
		break;
	default:
		m_iHeadUsed = RANDOM_LONG( 1, 1 );
		break;
	}

	CBaseBarn::Spawn();

	SET_MODEL(ENT(pev), "models/hev_scientist.mdl");
	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	SetupWeapon();

	pev->health			= gSkillData.hevbarnsHealth;
	m_fUsesMeleeAttack	= TRUE;
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CHEVScientist :: Precache()
{
	PRECACHE_MODEL("models/hev_scientist.mdl");

	CBaseBarn::Precache();

	TalkInit();
}

// Init talk data
void CHEVScientist :: TalkInit()
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
	if ( GetHead() == 1 )
		m_voicePitch = 105;
	else
		m_voicePitch = 100;
}

int CHEVScientist :: GetHead( void )
{
	return m_iHeadUsed;
}

//=========================================================
// SetGun - the HEV models have the head in bodygroup 1
// and the gun in bodygroup 2
//=========================================================
void CHEVScientist :: SetGun( int iGun )
{
	pev->body = 0;
	SetBodygroup( 1, GetHead() );
	SetBodygroup( 2, iGun );
}

//=========================================================
// HasGun
//=========================================================
BOOL CHEVScientist :: HasGun( void )
{
	if ( GetBodygroup( 2 ) == BARN_BODY_GUNGONE || GetBodygroup( 2 ) == BARN_BODY_GUNHOLSTERED )
		return FALSE;

	return TRUE;
}

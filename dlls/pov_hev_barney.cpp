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
// POV: monster_hev_barney, a security guard in an HEV suit.
// One of CBaseBarn's monsters (basebarn.h, pov_barn.cpp).
// File and class names are ours.
//=========================================================

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"basebarn.h"

class CHEVBarney : public CBaseBarn	// name ours
{
public:
	void Spawn( void );
	void Precache( void );

	int GetHead( void ) { return 0; }
	void SetGun( int iGun );
	BOOL HasGun( void );

	void TalkInit( void );
};

LINK_ENTITY_TO_CLASS( monster_hev_barney, CHEVBarney );

//=========================================================
// Spawn
//=========================================================
void CHEVBarney :: Spawn()
{
	CBaseBarn::Spawn();

	SET_MODEL(ENT(pev), "models/hev_barney.mdl");
	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	SetupWeapon();

	pev->health			= gSkillData.hevbarnsHealth;
	m_fUsesMeleeAttack	= TRUE;
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CHEVBarney :: Precache()
{
	PRECACHE_MODEL("models/hev_barney.mdl");

	CBaseBarn::Precache();

	TalkInit();
}

// Init talk data
void CHEVBarney :: TalkInit()
{

	CTalkMonster::TalkInit();

	// speach group names (group names are in sentences.txt)

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
// SetGun - the HEV models have the head in bodygroup 1
// and the gun in bodygroup 2
//=========================================================
void CHEVBarney :: SetGun( int iGun )
{
	pev->body = 0;
	SetBodygroup( 1, GetHead() );
	SetBodygroup( 2, iGun );
}

//=========================================================
// HasGun
//=========================================================
BOOL CHEVBarney :: HasGun( void )
{
	if ( GetBodygroup( 2 ) == BARN_BODY_GUNGONE || GetBodygroup( 2 ) == BARN_BODY_GUNHOLSTERED )
		return FALSE;

	return TRUE;
}

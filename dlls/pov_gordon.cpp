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
// POV: monster_gordon, Gordon Freeman in his HEV suit. One
// of CBaseBarn's monsters (basebarn.h, pov_barn.cpp), on the
// HEV scientist's model. File and class names are ours.
//=========================================================

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"basebarn.h"

class CGordon : public CBaseBarn	// name ours
{
public:
	void Spawn( void );
	void Precache( void );

	int GetHead( void ) { return 0; }
	void SetGun( int iGun );
	BOOL HasGun( void );

	void TalkInit( void );
};

LINK_ENTITY_TO_CLASS( monster_gordon, CGordon );

//=========================================================
// Spawn
//=========================================================
void CGordon :: Spawn()
{
	CBaseBarn::Spawn();

	SET_MODEL(ENT(pev), "models/hev_scientist.mdl");
	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	SetupWeapon();

	pev->health			= gSkillData.specbarnsHealth;
	m_fUsesMeleeAttack	= TRUE;
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CGordon :: Precache()
{
	PRECACHE_MODEL("models/hev_scientist.mdl");

	CBaseBarn::Precache();

	TalkInit();
}

// Init talk data
void CGordon :: TalkInit()
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

	m_voicePitch = 100;
}

//=========================================================
// SetGun - the HEV models have the head in bodygroup 1
// and the gun in bodygroup 2
//=========================================================
void CGordon :: SetGun( int iGun )
{
	pev->body = 0;
	SetBodygroup( 1, GetHead() );
	SetBodygroup( 2, iGun );
}

//=========================================================
// HasGun
//=========================================================
BOOL CGordon :: HasGun( void )
{
	if ( GetBodygroup( 2 ) == BARN_BODY_GUNGONE || GetBodygroup( 2 ) == BARN_BODY_GUNHOLSTERED )
		return FALSE;

	return TRUE;
}

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
// POV: monster_barniel, barney's female colleague. One of
// CBaseBarn's monsters (basebarn.h, pov_barn.cpp). File and
// class names are ours.
//=========================================================

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"basebarn.h"

class CBarniel : public CBaseBarn	// name ours
{
public:
	void Spawn( void );
	void Precache( void );

	void TalkInit( void );
};

LINK_ENTITY_TO_CLASS( monster_barniel, CBarniel );

//=========================================================
// Spawn
//=========================================================
void CBarniel :: Spawn()
{
	CBaseBarn::Spawn();

	SET_MODEL(ENT(pev), "models/barniel.mdl");
	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CBarniel :: Precache()
{
	PRECACHE_MODEL("models/barniel.mdl");

	CBaseBarn::Precache();

	TalkInit();
}

// Init talk data
void CBarniel :: TalkInit()
{

	CTalkMonster::TalkInit();

	// barniel's speach group names (group names are in sentences.txt)

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

	m_voicePitch = RANDOM_LONG( 98, 102 );
}

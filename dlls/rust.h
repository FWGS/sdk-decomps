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
// the rust robots' base and their projectiles: the rustbit's
// shot (also the rust flier's), the gunner's rocket and its
// mortar
//=========================================================
#ifndef RUST_H
#define RUST_H

#ifndef SQUADMONSTER_H
#include "squadmonster.h"
#endif

class CSprite;

#define RUST_SENT_NONE	-1

//=========================================================
// CBaseRust - the rustbit, the rust bot and the gunner
//=========================================================
class CBaseRust : public CSquadMonster
{
public:
	void Precache( void );
	void GibMonster( void );
	void CallForHelp( char *szClassname, float flDist, EHANDLE hEnemy, Vector &vecLocation );

	void SpeakSentence( void );
	void RustInit( void );
	void JustSpoke( void );
	BOOL FOkToSpeak( void );

	int		Save( CSave &save );
	int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	int		m_iVoicePitch;
	int		m_iSentence;			// the SentenceGroup to say next
	int		m_iSentenceVolume;		// not saved
	int		m_iModelIndexBotDeath;	// sprites/botdeath.spr
	float	m_flSpawnTime;			// not saved
};

//=========================================================
// rustbitshot: a small bolt of rust
//=========================================================
class CRustbitShot : public CBaseEntity
{
public:
	void Spawn( void );
	void Precache( void );

	static void Shoot( entvars_t *pevOwner, Vector vecStart, Vector vecDir, Vector vecSpread, int iSpeed );

	void EXPORT ShotTouch( CBaseEntity *pOther );

	int		Save( CSave &save );
	int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	int		m_maxFrame;
};

//=========================================================
// gunner_rocket: the apache's rocket, homing in on players
//=========================================================
class CGunnerRocket : public CGrenade
{
public:
	void Spawn( void );
	void Precache( void );
	void EXPORT RocketExplodeTouch( CBaseEntity *pOther );
	void EXPORT IgniteThink( void );
	void EXPORT AccelerateThink( void );

	int		Save( CSave &save );
	int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	int		m_iTrail;		// not saved
	Vector	m_vecForward;
	CSprite	*m_pFlare;
};

//=========================================================
// gunner_mortar: bursts into four DML cluster bombs
//=========================================================
class CGunnerMortar : public CBaseEntity
{
public:
	void Spawn( void );
	void Precache( void );

	static CGunnerMortar *MortarCreate( void );

	void Fragment( void );
	void EXPORT MortarThink( void );
	void EXPORT FragmentTouch( CBaseEntity *pOther );

	int		m_iTrail;
};

#endif // RUST_H

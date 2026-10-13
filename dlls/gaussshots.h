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
// the gauss pistol's shots, which monsters fire too
//=========================================================
#ifndef GAUSSSHOTS_H
#define GAUSSSHOTS_H

extern unsigned short g_usGaussFire;

// the rapid fire's shot
class CFastShot : public CGrenade
{
public:
	void Spawn( void );
	void Precache( void );
#ifndef CLIENT_DLL
	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];
#endif

	static CFastShot *Shoot( Vector vecSrc, Vector vecAngles, CBaseEntity *pOwner, float flDamage );

	void EXPORT Animate( void );
	void EXPORT FastShotTouch( CBaseEntity *pOther );

	int		m_iUnused274;
	int		m_iTrail;
	int		m_iUnused27c;
	int		m_iMaxFrame;
	int		m_iGlow;
	unsigned short m_usRapidImpact;
};

// the charged bolt
class CBoltShot : public CGrenade
{
public:
	void Spawn( void );
	void Precache( void );
#ifndef CLIENT_DLL
	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];
#endif

	static CBoltShot *Shoot( Vector vecSrc, Vector vecAngles, CBaseEntity *pOwner );

	void EXPORT ThrowSparks( void );
	void EXPORT BoltTouch( CBaseEntity *pOther );
	void CreateBeam( void );
	void KillBeam( void );

	int		m_iUnused274;
	int		m_iTrail;
	CBeam	*m_pBeam;
	int		m_iUnused280;
	int		m_iMaxFrame;
	unsigned short m_usChargedImpact;
};

// the glow a shot leaves
class CGaussGlow : public CGrenade
{
public:
	void Spawn( void );
	void Precache( void );

	static CGaussGlow *CreateGlow( Vector vecOrigin, int iType );

	void EXPORT AnimateGlow( void );
	void SetGlow( int iType );

	int		m_iMaxFrame;
	int		m_iUnused278;
};

// a bouncing spark
class CGaussBall : public CGrenade
{
public:
	void Spawn( void );
	void Precache( void );

	static CGaussBall *CreateBall( void );

	void EXPORT RemoveBall( void );
	void EXPORT TumbleClusterThink( void );
	void EXPORT BallBounceTouch( CBaseEntity *pOther );

	short	m_sKaboom;
	int		m_iTrail;
};

#endif // GAUSSSHOTS_H

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
// the chemical gun's mixtures and the blobs it fires
//=========================================================
#ifndef CHEMGRENADE_H
#define CHEMGRENADE_H

// what the chemical gun's sliders mix
typedef struct
{
	int		acid;
	int		base;
	int		nuet;
	int		viscosity;
	int		volatility;
	float	stability;		// how long the blob lasts, and how much it bounces
	int		burn_rate;
	int		explosion;		// 1 acid, 2 base, 3 explosive, 4 both, 5 neutral
	int		chemical_type;
	int		color;
} chemicalmixture_t;

// a blob of chemicals
class CChemGrenade : public CGrenade
{
public:
	void Spawn( void );
	void Precache( void );
	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	static CChemGrenade *ShootSticky( entvars_t *pevOwner, Vector vecStart, Vector vecVelocity, chemicalmixture_t mixture );
	static CChemGrenade *ShootBouncy( entvars_t *pevOwner, Vector vecStart, Vector vecVelocity, chemicalmixture_t mixture );
	static CChemGrenade *ShootSplatter( entvars_t *pevOwner, Vector vecStart, Vector vecVelocity, chemicalmixture_t mixture );

private:
	void EXPORT SplatterTouch( CBaseEntity *pOther );
	void EXPORT Splatter( TraceResult *pTrace );
	void EXPORT ChemicalSmoke( void );
	void EXPORT ChemicalSmokeThink( void );
	void EXPORT StickyChemicalTouch( CBaseEntity *pOther );
	void EXPORT BouncyChemicalTouch( CBaseEntity *pOther );
	void EXPORT TimedChemicalThink( void );
	void EXPORT ChemicalExplode( void );
	void EXPORT ChemicalTrail( void );
	void Explode( TraceResult *pTrace, int bitsDamageType );

	float	m_flTimeExploded;
	int		m_iGreenSplash;
	int		m_iRedSplash;
	int		m_iYellowSplash;
	int		m_iFlame;
	unsigned short m_usChemicalSpray;
	int		m_bDontSpray;
	CSprite	*m_pSmokeSprite;
	chemicalmixture_t ChemicalMixture;
	int		m_iUnused2bc;
};

#endif // CHEMGRENADE_H

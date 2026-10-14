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
// sphere_explosion: the client draws a growing sphere of
// tracers; monsters' gibbings and other explosions make one
// with CreateSphereExplosion
//=========================================================
#ifndef SPHEREEXPLOSION_H
#define SPHEREEXPLOSION_H

class CSphereExplosion : public CBaseEntity
{
public:
	CSphereExplosion( void );
	void Spawn( void );
	void Precache( void );
	void KeyValue( KeyValueData *pkvd );
	void EXPORT UseToggle( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void EXPORT ExplodeEvent( void );

	static CSphereExplosion *CreateSphereExplosion( Vector vecOrigin, CBaseEntity *pOwner,
		float flMaxRadius, float flRadiusStep, int iTrans, int iTransStep, int iTracers,
		BOOL bShockwave, BOOL bImplosion, float flShakeDuration, float flFadeTime, float flHoldTime );

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	float	m_flMaxRadius;
	float	m_flRadiusStep;		// growth per frame on the client
	int		m_iTrans;
	int		m_iTransStep;
	int		m_iTracers;
	BOOL	m_bShockwave;		// also a beam cylinder
	BOOL	m_bImplosion;
	float	m_iShakeDuration;	// a float despite the name; 0 for no shake
	Vector	m_vFadeColor;		// no key sets it
	float	m_flFadeTime;		// 0 for no screen fade
	float	m_flHoldTime;

	unsigned short	m_usSphereExplode;
	int		m_iShockwaveSprite;
};

#endif // SPHEREEXPLOSION_H

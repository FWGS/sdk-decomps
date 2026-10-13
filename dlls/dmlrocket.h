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
// the DML's rocket and the cluster bombs it can carry
//=========================================================
#ifndef DMLROCKET_H
#define DMLROCKET_H

// what the DML's and the DML grenade's customization sliders set
typedef struct
{
	int		launch;		// 1 at once, 2 when locked on
	int		flightpath;	// 1 guided (laser spot), 2 homing, 3 spiral
	int		detonate;
	int		payload;
} dmlconfig_t;

// the cluster bomb payload
class CClusterGrenade : public CGrenade
{
public:
	void Spawn( void );
	void Precache( void );

	static CClusterGrenade *CreateCluster( void );

	void EXPORT ClusterDetonate( void );
	void EXPORT TumbleClusterThink( void );
	void ClusterExplode( TraceResult *pTrace, int bitsDamageType );
	void EXPORT ClusterSmokeThink( void );

	short	m_sKaboom;
	int		m_iSmoke;
	int		m_iClusterShock;
	CSprite	*m_pSmokeRing;
	BOOL	m_fTrailOn;
	int		m_iExplosion;		// the explosion event's bparam1
	unsigned short m_usClusterExplosion;
};


// the rocket: a hornet that the DML's customization makes fly, home in,
// detonate and deliver its payload in different ways
class CDmlRocket : public CBaseMonster
{
public:
	void Spawn( void );
	void Precache( void );
	int  Classify ( void ) { return CLASS_PLAYER_BIOWEAPON; }
	int  BloodColor( void ) { return DONT_BLEED; }
	int  IRelationship ( CBaseEntity *pTarget );
	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	static CDmlRocket *CreateSpiralRocket( const Vector &vecOrigin, const Vector &vecAngles, edict_t *pentOwner, int iFlightpath, int iPayload, int iDetonate );
	static CDmlRocket *CreateDmlRocket( const Vector &vecOrigin, const Vector &vecAngles, edict_t *pentOwner, int iFlightpath, int iPayload, int iDetonate );

	int TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );
	void Killed( entvars_t *pevAttacker, int iGib );

	float			m_flStopAttack;
	int				m_iHornetType;
	float			m_flFlySpeed;

	int				m_iFlightpath;
	int				m_iPayload;
	int				m_iDetonate;
	float			m_flStopHoming;
	int				m_iUnknown28c;
	int				m_iUnknown290;
	int				m_iIsBrother;
	float			m_iSpiralScalar_In;
	float			m_iSpiralScalar_Up;
	float			m_flIgniteTime;
	CDmlRocket		*m_BrotherRocket;
	CSprite			*m_pSprite;
	float			m_flNextBounceSound;
	int				m_iUnknown2b0;

	// tripmine payload
	float			m_flPowerUp;
	Vector			m_vecDir;
	Vector			m_vecEnd;
	float			m_flBeamLength;
	EHANDLE			m_hOwner;
	CBeam			*m_pBeam;
	Vector			m_posOwner;
	Vector			m_angleOwner;
	bool			m_bSpawnedTrip;
	edict_t			*m_pRealOwner;
	int				m_iUnknown300;
	int				m_iUnknown304;
};

#endif // DMLROCKET_H

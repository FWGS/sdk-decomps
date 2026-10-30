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
// the beam gun (polaris blade), shared with the client's weapon prediction
//=========================================================
#ifndef BEAMGUN_H
#define BEAMGUN_H

// what the three customization sliders make of the beam
typedef struct
{
	int		frequency;		// cust_1BeamFrequency: the fire type
	float	current;		// cust_2BeamCurrent
	float	voltage;		// cust_3BeamVoltage
	int		width;
	int		noise;
	Vector	spread;
	float	damage;			// energy beam damage
	float	shockdamage;	// shock damage, and the chain lightning's
	float	flUnused;
	float	ballchance;		// chance of throwing ball lightning
	int		iUnused;
	int		red;
	int		green;
	int		blue;
	int		range;
} beamconfig_t;

class CBallLightning;

class CBeamGun : public CBasePlayerWeapon
{
public:
#ifndef CLIENT_DLL
	int		Save( CSave &save );
	int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];
#endif

	void Spawn( void );
	void Precache( void );
	int iItemSlot( void ) { return 5; }
	int GetItemInfo(ItemInfo *p);
	int AddToPlayer( CBasePlayer *pPlayer );
	void UpdateOwner( void );
	void InventoryFrame( void );
	void Customize( char cOption, int iValue );

	BOOL Deploy( void );
	void Holster( int skiplocal = 0 );

	void CreateEffect( void );
	void UpdateEffect( Vector &startPoint, const Vector &endPoint, float timeBlend );
	void DestroyEffect( void );

	void EndAttack( void );
	void Discharge( const Vector &vecOrigSrc, const Vector &vecDir );
	void PrimaryAttack( void );
	void SecondaryAttack( void );
	void WeaponIdle( void );

	float m_flAmmoUseTime;// since we use < 1 point of ammo per update, we subtract ammo on a timer.
	beamconfig_t Configuration;

	float GetPulseInterval( void );
	float GetDischargeInterval( void );

	void Fire( const Vector &vecOrigSrc, const Vector &vecDir );
	void ShotgunLightning( Vector vecSrc, Vector vecDir, TraceResult tr );
	void ChainLightning( const Vector &vecSrc );
	void ShootTracer( void );

	BOOL CanArc( void );
	BOOL HasAmmo( void );
	void UseAmmo( int count );

	enum EGON_FIRESTATE { FIRE_OFF, FIRE_CHARGE };

	int					m_iTemp;
	float				m_flNextTempChangeTime;
	int					m_fOverheating;
	float				m_flTimeStartOverheating;
	float				m_flNextTempDecreaseTime;
	float				m_flNextIdleAnimTime;
	float				m_flTimeAttackAnim;
	float				m_flStartAttack;
	float				m_shootTime;
	CBeam				*m_pBeam;
	CBeam				*m_pLightningBeam;
	CBeam				*m_pNoise;
	CSprite				*m_pSprite;
	EGON_FIRESTATE		m_fireState;
	float				m_shakeTime;
	int					m_iBeamLight;
	float				m_flNextBeamRing;
	float				m_flShootLightningBugTime;
	int					m_iTracerSpread;
	float				m_flLastCustomTime;
	float				m_flChainLightningTime;
	float				m_flDrawShotgunLightning;
	int					m_iFireType;
	unsigned short		m_usMinigunHeat;
	unsigned short		m_usLightning;
};

#endif // BEAMGUN_H

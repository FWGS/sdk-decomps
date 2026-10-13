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
// the chemical gun, shared with the client's weapon prediction
//=========================================================
#ifndef SPCHEMICALGUN_H
#define SPCHEMICALGUN_H

class CSPChemicalGun : public CBasePlayerWeapon
{
public:
	void Spawn( void );
	void Precache( void );
	int iItemSlot( void ) { return 8; }
	int GetItemInfo(ItemInfo *p);
	int AddToPlayer( CBasePlayer *pPlayer );
	void UpdateOwner( void );
	void Customize( char cOption, int iValue );
	int PrimaryAmmoIndex( void );

	void PrimaryAttack( void );
	BOOL Deploy( void );
	BOOL IsUseable( void );
	void WeaponIdle( void );

	void LaunchChemical( entvars_t *pevOwner, Vector vecSrc, Vector vecVelocity );
	int CalcViscosity( void );
	int CalcVolatility( void );
	int CalcStability( void );
	int CalcExplosion( void );

#ifndef CLIENT_DLL
	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];
#endif

	int		m_iPressure;
	int		m_iUnusedA4;
	float	m_flLastMixture;	// when the mixture was last changed
	float	m_iVelocity;
	int		m_iSplash;
	Vector	m_vSpread;
	chemicalmixture_t ChemicalMixture;
	float	m_flTimeNextIdle;
};

#endif // SPCHEMICALGUN_H

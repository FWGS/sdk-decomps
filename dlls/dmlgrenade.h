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
// the DML's grenades and tripmines, shared with the client's weapon prediction
//=========================================================
#ifndef DMLGRENADE_H
#define DMLGRENADE_H

class CDmlGrenade : public CBasePlayerWeapon
{
public:
	void Spawn( void );
	void Precache( void );
	int iItemSlot( void ) { return 7; }
	int GetItemInfo(ItemInfo *p);
	int AddToPlayer( CBasePlayer *pPlayer );
	void UpdateOwner( void );
	void Customize( char cOption, int iValue );

	void PrimaryAttack( void );
	BOOL Deploy( void );
	BOOL CanHolster( void );
	void Holster( int skiplocal = 0 );
	void WeaponIdle( void );
	void PlaceTripmine( void );

#ifndef CLIENT_DLL
	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];
#endif

	float		m_flStartThrow;
	float		m_flReleaseThrow;
	float		m_flTakeoutAnim;
	int			m_iShell1;
	int			m_iShell2;
	float		m_flEjectShell1;
	float		m_flEjectShell2;
	dmlconfig_t	m_Config;
	int			m_iModelIndexRocketExpl;
	int			m_iModelIndexSmoke;
	float		m_flTimePlacedTripmine;
	float		m_flTimeNextCustomAnim;
	float		m_flTimeWeaponIdleThrow;
	int			m_bRetired;
	int			m_fDeployTakeout;
};

#endif // DMLGRENADE_H

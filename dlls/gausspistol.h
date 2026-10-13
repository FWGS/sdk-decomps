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
// the gauss pistol, shared with the client's weapon prediction
//=========================================================
#ifndef GAUSSPISTOL_H
#define GAUSSPISTOL_H

class CGaussPistol : public CBasePlayerWeapon
{
public:
#ifndef CLIENT_DLL
	int		Save( CSave &save );
	int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];
#endif

	void Spawn( void );
	void Precache( void );
	int iItemSlot( void ) { return 2; }
	int GetItemInfo(ItemInfo *p);
	int AddToPlayer( CBasePlayer *pPlayer );
	void Customize( char cOption, int iValue );
#ifndef CLIENT_DLL
	void UpdateOwner( void );
#endif

	BOOL Deploy( void );
	void Holster( int skiplocal = 0  );
	void InventoryFrame( void );

	void PrimaryAttack( void );
	void SecondaryAttack( void );
	void WeaponIdle( void );

	float GetFullChargeTime( void );
	void FireRapid( void );
	void FireBolt( void );
	void ChargeSniper( void );
	void FireSniper( void );
	void FireSingle( void );

	int m_fInAttack;
	float m_flStartCharge;
	float m_flPlayAftershock;
	int m_iBeam;
	float m_flNextAmmoBurn;
	int m_iFireType;
	int m_iUnused;
	int m_iNormalShotCount;	// three quick shots, then a pause
	int m_iSlidePos;
	float m_flTimeGaussHolster;
	float m_flSniperTime;	// when the sniper shot started charging
	int m_fSniperAttack;
	BOOL m_bCharged;
	int m_fFirstIdle;
	int m_iHaveSniperRifle;
	int m_iHolstered;
	unsigned short m_usSniperShot;
};

#endif // GAUSSPISTOL_H

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
// the minigun, shared with the client's weapon prediction
//=========================================================
#ifndef MINIGUN_H
#define MINIGUN_H

class CMinigun : public CBasePlayerWeapon
{
public:
	void Spawn( void );
	void Precache( void );
	int iItemSlot( void ) { return 4; }
	int GetItemInfo(ItemInfo *p);
	int AddToPlayer( CBasePlayer *pPlayer );
	void UpdateOwner( void );

	void PrimaryAttack( void );
	BOOL Deploy( void );
	void Reload( void );
	void InventoryFrame( void );
	void WeaponIdle( void );
	void Customize( char cOption, int iValue );
	void Holster( int skiplocal = 0 );

#ifndef CLIENT_DLL
	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];
#endif

	int m_iSpinState;
	int m_iOverheating;
	int m_iTemp;
	float m_flTimeStartSpinning;
	float m_flTimeStartOverheating;
	float m_flNextTempChangeTime;
	float m_flNextSpinAnimation;
	float m_flNextIdleAnimTime;
	float m_flNextTempDrop;
	float m_flNextSpinCheck;
	int m_iShell;
	float m_flNextFireAnimTime;
	int m_iMuzzleSprite;
	int m_iSlidePos;
	bool m_bWaterCooled;
	float m_flTimeDrawBodyChange;
	int m_fIdleStarted;
	unsigned short m_usMinigun;
	unsigned short m_usMinigunHeat;
	int m_iGlowshell;
	int m_iClientTemp;
	float m_flNextConfigTime;
};

#endif // MINIGUN_H

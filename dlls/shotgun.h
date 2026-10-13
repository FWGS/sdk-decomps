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
// the shotgun, shared with the client's weapon prediction
//=========================================================
#ifndef SHOTGUN_H
#define SHOTGUN_H

class CShotgun : public CBasePlayerWeapon
{
public:
#ifndef CLIENT_DLL
	int		Save( CSave &save );
	int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];
#endif

	void Spawn( void );
	void Precache( void );
	int iItemSlot( ) { return 3; }
	int GetItemInfo(ItemInfo *p);
	int AddToPlayer( CBasePlayer *pPlayer );
#ifndef CLIENT_DLL
	void UpdateOwner( void );
#endif

	void PrimaryAttack( void );
	BOOL Deploy( );
	void WeaponIdle( void );
	void Customize( char cOption, int iValue );
	int m_fInReload;
	float m_flNextReload;
	int m_iShell;
	float m_flPumpTime;
	float m_flNextCustomizeAnim;
	Vector m_vecSpread;
	int m_iSlidePos;		// spread setting
	int m_iNumPellets;
	int m_iNumShells;		// shells per shot
private:
	unsigned short m_usShotgun;
};

#endif // SHOTGUN_H

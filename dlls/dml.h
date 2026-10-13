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
// the dual mode launcher, shared with the client's weapon prediction
//=========================================================
#ifndef DML_H
#define DML_H

class CDml : public CBasePlayerWeapon
{
public:
#ifndef CLIENT_DLL
	int		Save( CSave &save );
	int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];
#endif

	void Spawn( void );
	void Precache( void );
	int iItemSlot( void ) { return 6; }
	int GetItemInfo(ItemInfo *p);
	int AddToPlayer( CBasePlayer *pPlayer );
#ifndef CLIENT_DLL
	void UpdateOwner( void );
#endif
	void InventoryFrame( void );
	void Customize( char cOption, int iValue );

	BOOL Deploy( void );
	void Holster( int skiplocal = 0 );

	void PrimaryAttack( void );
	void FireSpiral( void );
	void FireSingle( void );
	void OffsetRocket( CBaseEntity *pRocket, int iSide );
	CDmlRocket *CreateRocket( void );
	void Reload( void );
	void WeaponIdle( void );

	void UpdateSpot( void );
	CBaseEntity *FindTarget( bool fAutoaim );
	Vector GetAutoaimVector( float flDelta );
	Vector AutoaimDeflection( Vector &vecSrc, float flDist, float flDelta );
	void SetBody( int iBody );

	int				m_iSide;
	int				m_iFirstSide;
	int				m_iStyle;
	float			m_iSpiralScalar_In;
	float			m_iSpiralScalar_Up;
	int				m_fSpotActive;
	float			m_flNextConfigTime;
	float			m_flLastAttack;
	float			m_flNextSkinChange;
	dmlconfig_t		m_Config;
	CLaserSpot		*m_pSpot;
	CBaseEntity		*m_pLockTarget;
	int				m_fCrosshairOn;
	int				m_fLocked;
	int				m_iHomingState;
};

#endif // DML_H

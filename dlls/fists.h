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
// the fists and the knife, shared with the client's
// weapon prediction
//=========================================================
#ifndef FISTS_H
#define FISTS_H

class CFists : public CBasePlayerWeapon
{
public:
	void Spawn( void );
	void Precache( void );
	int iItemSlot( void ) { return 1; }
	void EXPORT SwingAgain( void );
	void EXPORT Smack( void );
	int GetItemInfo(ItemInfo *p);
	int AddToPlayer( CBasePlayer *pPlayer );

	void PrimaryAttack( void );
	int Swing( int fFirst );
	BOOL Deploy( void );
	void WeaponIdle( void );
	void Customize( char cOption, int iValue );

#ifndef CLIENT_DLL
	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];
#endif

	int m_iSwing;
	TraceResult m_trHit;

	float m_flNextIdle;
	int m_iWeapon;			// 0 fists, 1 knife
	float m_flDrawAnimTime;	// when to play the draw of the other weapon
	float m_flLastRipple;
	Vector m_vecLastRipplePos;
	unsigned short m_usWaterRipple;
};

#endif // FISTS_H

/***
*
*	Copyright (c) 1999, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   This source code contains proprietary and confidential information of
*   Valve LLC and its suppliers.  Access to this code is restricted to
*   persons who have executed a written SDK license with Valve.  Any access,
*   use or distribution of this code by or to any unlicensed person is illegal.
*
****/
//=========================================================
// monster template
//=========================================================
// UNDONE: Holster weapon?

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"basebarn.h"

//=========================================================
// POV: barney is one of CBaseBarn's monsters (basebarn.h,
// pov_barn.cpp); he is the one that kicks and punches.
//=========================================================
class CBarney : public CBaseBarn
{
public:
	void Spawn( void );
};

LINK_ENTITY_TO_CLASS( monster_barney, CBarney );

//=========================================================
// Spawn
//=========================================================
void CBarney :: Spawn()
{
	CBaseBarn::Spawn();

	m_fUsesMeleeAttack = TRUE;
}

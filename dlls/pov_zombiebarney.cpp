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
// POV: zombie barney, the zombie with the zbarney model and
// a quarter more health. File name and class name ours.
//=========================================================

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"schedule.h"
#include	"zombie.h"

class CZBarney : public CZombie
{
public:
	void Spawn( void );
	void Precache( void );
};

LINK_ENTITY_TO_CLASS( monster_zbarney, CZBarney );

//=========================================================
// Spawn
//=========================================================
void CZBarney :: Spawn()
{
	Precache( );

	SET_MODEL(ENT(pev), "models/zbarney.mdl");
	UTIL_SetSize( pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX );

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= BLOOD_COLOR_GREEN;
	pev->health			= (int)( gSkillData.zombieHealth * 1.25 );
	pev->view_ofs		= VEC_VIEW;// position of the eyes relative to monster's origin.
	m_flFieldOfView		= 0.5;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState		= MONSTERSTATE_NONE;
	m_afCapability		= bits_CAP_DOORS_GROUP;

	MonsterInit();
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CZBarney :: Precache()
{
	CZombie::Precache();

	PRECACHE_MODEL("models/zbarney.mdl");
}

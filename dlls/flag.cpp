/***
*
*	Copyright (c) 1999, 2000 Valve LLC. All rights reserved.
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
// Wanted: the flags (monster_flag1 to monster_flag4). Each is a
// monster that only stands and animates (models/flag1.mdl to
// flag4.mdl, 30 x 30 x 58 units): it can be shot, never dies (every
// hit sets its health back to half of 100) and flinches like any
// monster. The four classes differ only in the model. The file and
// class names are ours.
//=========================================================

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"schedule.h"

//=========================================================
// monster_flag1
//=========================================================
class CFlag1 : public CBaseMonster
{
public:
	int  Classify( void );
	void Spawn( void );
	void Precache( void );
	int  TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );
};

LINK_ENTITY_TO_CLASS( monster_flag1, CFlag1 );

int CFlag1 :: Classify( void )
{
	return CLASS_NONE;
}

void CFlag1 :: Spawn( void )
{
	Precache( );

	SET_MODEL( ENT( pev ), "models/flag1.mdl" );
	UTIL_SetSize( pev, Vector( -15, -15, 0 ), Vector( 15, 15, 58 ) );

	pev->solid			= SOLID_SLIDEBOX;
	m_bloodColor		= DONT_BLEED;
	pev->health			= 100;
	pev->effects		= 0;
	m_MonsterState		= MONSTERSTATE_NONE;

	MonsterInit();
	SetActivity( ACT_IDLE );
}

void CFlag1 :: Precache( void )
{
	PRECACHE_MODEL( "models/flag1.mdl" );
}

// a flag can't be killed: each hit sets its health back to half of
// its full health, and only the damage conditions remain
int CFlag1 :: TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	pev->health = pev->max_health / 2;

	if ( flDamage > 0 )
	{
		SetConditions( bits_COND_LIGHT_DAMAGE );
	}

	if ( flDamage >= 20 )
	{
		SetConditions( bits_COND_HEAVY_DAMAGE );
	}

	return 1;
}

//=========================================================
// monster_flag2
//=========================================================
class CFlag2 : public CBaseMonster
{
public:
	int  Classify( void );
	void Spawn( void );
	void Precache( void );
	int  TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );
};

LINK_ENTITY_TO_CLASS( monster_flag2, CFlag2 );

int CFlag2 :: Classify( void )
{
	return CLASS_NONE;
}

void CFlag2 :: Spawn( void )
{
	Precache( );

	SET_MODEL( ENT( pev ), "models/flag2.mdl" );
	UTIL_SetSize( pev, Vector( -15, -15, 0 ), Vector( 15, 15, 58 ) );

	pev->solid			= SOLID_SLIDEBOX;
	m_bloodColor		= DONT_BLEED;
	pev->effects		= 0;
	pev->health			= 100;
	m_MonsterState		= MONSTERSTATE_NONE;

	MonsterInit();
	SetActivity( ACT_IDLE );
}

void CFlag2 :: Precache( void )
{
	PRECACHE_MODEL( "models/flag2.mdl" );
}

int CFlag2 :: TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	pev->health = pev->max_health / 2;

	if ( flDamage > 0 )
	{
		SetConditions( bits_COND_LIGHT_DAMAGE );
	}

	if ( flDamage >= 20 )
	{
		SetConditions( bits_COND_HEAVY_DAMAGE );
	}

	return 1;
}

//=========================================================
// monster_flag3
//=========================================================
class CFlag3 : public CBaseMonster
{
public:
	int  Classify( void );
	void Spawn( void );
	void Precache( void );
	int  TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );
};

LINK_ENTITY_TO_CLASS( monster_flag3, CFlag3 );

int CFlag3 :: Classify( void )
{
	return CLASS_NONE;
}

void CFlag3 :: Spawn( void )
{
	Precache( );

	SET_MODEL( ENT( pev ), "models/flag3.mdl" );
	UTIL_SetSize( pev, Vector( -15, -15, 0 ), Vector( 15, 15, 58 ) );

	pev->solid			= SOLID_SLIDEBOX;
	m_bloodColor		= DONT_BLEED;
	pev->effects		= 0;
	pev->health			= 100;
	m_MonsterState		= MONSTERSTATE_NONE;

	MonsterInit();
	SetActivity( ACT_IDLE );
}

void CFlag3 :: Precache( void )
{
	PRECACHE_MODEL( "models/flag3.mdl" );
}

int CFlag3 :: TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	pev->health = pev->max_health / 2;

	if ( flDamage > 0 )
	{
		SetConditions( bits_COND_LIGHT_DAMAGE );
	}

	if ( flDamage >= 20 )
	{
		SetConditions( bits_COND_HEAVY_DAMAGE );
	}

	return 1;
}

//=========================================================
// monster_flag4
//=========================================================
class CFlag4 : public CBaseMonster
{
public:
	int  Classify( void );
	void Spawn( void );
	void Precache( void );
	int  TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );
};

LINK_ENTITY_TO_CLASS( monster_flag4, CFlag4 );

int CFlag4 :: Classify( void )
{
	return CLASS_NONE;
}

void CFlag4 :: Spawn( void )
{
	Precache( );

	SET_MODEL( ENT( pev ), "models/flag4.mdl" );
	UTIL_SetSize( pev, Vector( -15, -15, 0 ), Vector( 15, 15, 58 ) );

	pev->solid			= SOLID_SLIDEBOX;
	m_bloodColor		= DONT_BLEED;
	pev->health			= 100;
	pev->effects		= 0;
	m_MonsterState		= MONSTERSTATE_NONE;

	MonsterInit();
	SetActivity( ACT_IDLE );
}

void CFlag4 :: Precache( void )
{
	PRECACHE_MODEL( "models/flag4.mdl" );
}

int CFlag4 :: TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	pev->health = pev->max_health / 2;

	if ( flDamage > 0 )
	{
		SetConditions( bits_COND_LIGHT_DAMAGE );
	}

	if ( flDamage >= 20 )
	{
		SetConditions( bits_COND_HEAVY_DAMAGE );
	}

	return 1;
}

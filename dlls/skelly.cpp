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
// Wanted: skeletons (models/skeleton.mdl). monster_skelly is a
// corpse in one of four poses (keyvalue "pose"), a copy of the SDK's
// dead scientist (scientist.cpp); monster_skellydance is a skeleton
// that plays sequence 4 as a live monster. The file and class names
// are ours.
//=========================================================

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"

//=========================================================
// DEAD SKELETON PROP
//=========================================================
class CSkelly : public CBaseMonster
{
public:
	void Spawn( void );
	int	Classify ( void ) { return	CLASS_NONE; }

	void KeyValue( KeyValueData *pkvd );
	int	m_iPose;// which sequence to display
	static char *m_szPoses[4];
};
char *CSkelly::m_szPoses[] = { "s_on_back", "s_sitting", "dead_against_wall", "dead_stomach" };

void CSkelly::KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "pose"))
	{
		m_iPose = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CBaseMonster::KeyValue( pkvd );
}
LINK_ENTITY_TO_CLASS( monster_skelly, CSkelly );

//
// ********** Skelly SPAWN **********
//
void CSkelly :: Spawn( )
{
	PRECACHE_MODEL("models/skeleton.mdl");
	SET_MODEL(ENT(pev), "models/skeleton.mdl");

	m_MonsterState		= MONSTERSTATE_NONE;
	pev->effects		= 0;
	pev->sequence		= 0;
	// Corpses have less health
	pev->health			= 8;

	m_bloodColor = DONT_BLEED;

	pev->sequence = LookupSequence( m_szPoses[m_iPose] );
	if (pev->sequence == -1)
	{
		ALERT ( at_console, "Skelly with bad pose\n" );
	}

	MonsterInitDead();
}

//=========================================================
// DANCING SKELETON
//=========================================================
class CSkellyDance : public CBaseMonster
{
public:
	void Spawn( void );
	int	Classify ( void ) { return	CLASS_NONE; }

	int	m_iPose;// sic: copied from CSkelly, never used
};

LINK_ENTITY_TO_CLASS( monster_skellydance, CSkellyDance );

void CSkellyDance :: Spawn( )
{
	PRECACHE_MODEL("models/skeleton.mdl");
	SET_MODEL(ENT(pev), "models/skeleton.mdl");

	m_MonsterState		= MONSTERSTATE_NONE;
	pev->effects		= 0;
	pev->sequence		= 0;
	pev->health			= 8;

	m_bloodColor = DONT_BLEED;

	pev->sequence = 4;	// the dance

	MonsterInit();
}

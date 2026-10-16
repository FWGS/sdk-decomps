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
// POV: monster_spfsci and monster_spfsci_dead, the special
// forces' scientist and his corpse. File and class names
// ours.
//=========================================================

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"talkmonster.h"
#include	"schedule.h"
#include	"defaultai.h"
#include	"scripted.h"
#include	"animation.h"
#include	"soundent.h"
#include	"scientist.h"


//=======================================================
// SPF scientist
//=======================================================

class CSPFScientist : public CScientist
{
public:
	void Spawn( void );
	void Precache( void );
	int  Classify ( void );
};

LINK_ENTITY_TO_CLASS( monster_spfsci, CSPFScientist );

//=========================================================
// Spawn
//=========================================================
void CSPFScientist :: Spawn( void )
{
	CScientist::Spawn();

	SET_MODEL(ENT(pev), "models/spfsci.mdl");
	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	// White hands
	pev->skin = 0;

	if ( pev->body == -1 )
	{// -1 chooses a random head
		pev->body = RANDOM_LONG(0, NUM_SCIENTIST_HEADS-1);// pick a head, any head
	}

	// Luther is black, make his hands black
	if ( pev->body == HEAD_LUTHER )
		pev->skin = 1;

	MonsterInit();		// sic: CScientist::Spawn did it already
	Remember( bits_MEMORY_PROVOKED );
}

//=========================================================
// Classify - indicates this monster's place in the
// relationship table.
//=========================================================
int	CSPFScientist :: Classify ( void )
{
	return	CLASS_HUMAN_SPFORCE;
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CSPFScientist :: Precache( void )
{
	PRECACHE_MODEL("models/spfsci.mdl");

	CScientist::Precache();

	TalkInit();
}


//=========================================================
// Dead SPF Scientist PROP
//=========================================================
class CDeadSPFScientist : public CBaseMonster
{
public:
	void Spawn( void );
	int	Classify ( void ) { return	CLASS_HUMAN_PASSIVE; }

	void KeyValue( KeyValueData *pkvd );
	int	m_iPose;// which sequence to display
	static char *m_szPoses[7];
};
char *CDeadSPFScientist::m_szPoses[] = { "lying_on_back", "lying_on_stomach", "dead_sitting", "dead_hang", "dead_table1", "dead_table2", "dead_table3" };

void CDeadSPFScientist::KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "pose"))
	{
		m_iPose = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CBaseMonster::KeyValue( pkvd );
}
LINK_ENTITY_TO_CLASS( monster_spfsci_dead, CDeadSPFScientist );

//
// ********** DeadSPFScientist SPAWN **********
//
void CDeadSPFScientist :: Spawn( )
{
	PRECACHE_MODEL("models/spfsci.mdl");
	SET_MODEL(ENT(pev), "models/spfsci.mdl");

	pev->effects		= 0;
	pev->sequence		= 0;
	// Corpses have less health
	pev->health			= 8;//gSkillData.scientistHealth;

	m_bloodColor = BLOOD_COLOR_RED;

	if ( pev->body == -1 )
	{// -1 chooses a random head
		pev->body = RANDOM_LONG(0, NUM_SCIENTIST_HEADS-1);// pick a head, any head
	}
	// Luther is black, make his hands black
	if ( pev->body == HEAD_LUTHER )
		pev->skin = 1;
	else
		pev->skin = 0;

	pev->sequence = LookupSequence( m_szPoses[m_iPose] );
	if (pev->sequence == -1)
	{
		ALERT ( at_console, "Dead scientist with bad pose\n" );
	}

	// spawnflag 4 picks the second set of heads
	if ( pev->spawnflags & 4 )
		pev->body = (pev->body % NUM_SCIENTIST_HEADS) + NUM_SCIENTIST_HEADS * 1;

	MonsterInitDead();
}

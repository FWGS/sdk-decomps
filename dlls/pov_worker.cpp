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
// POV: monster_worker and monster_worker_dead, the
// construction worker (a scientist with gus.mdl) and his
// corpse (barney's). File and class names ours.
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
// Worker
//=======================================================

class CWorker : public CScientist
{
public:
	void Spawn( void );
	void Precache( void );
	void TalkInit( void );
};

LINK_ENTITY_TO_CLASS( monster_worker, CWorker );

//=========================================================
// Spawn
//=========================================================
void CWorker :: Spawn( void )
{
	Precache( );

	SET_MODEL(ENT(pev), "models/gus.mdl");
	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= BLOOD_COLOR_RED;
	pev->health			= gSkillData.workerHealth;
	pev->view_ofs		= Vector ( 0, 0, 50 );// position of the eyes relative to monster's origin.
	m_flFieldOfView		= VIEW_FIELD_WIDE; // NOTE: we need a wide field of view so scientists will notice player and say hello
	m_MonsterState		= MONSTERSTATE_NONE;

	m_afCapability		= bits_CAP_HEAR | bits_CAP_TURN_HEAD | bits_CAP_OPEN_DOORS | bits_CAP_AUTO_DOORS;

	pev->skin = 0;

	MonsterInit();
	Remember( bits_MEMORY_PROVOKED );
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CWorker :: Precache( void )
{
	PRECACHE_MODEL("models/gus.mdl");
	PRECACHE_SOUND("scientist/sci_pain1.wav");
	PRECACHE_SOUND("scientist/sci_pain2.wav");
	PRECACHE_SOUND("scientist/sci_pain3.wav");
	PRECACHE_SOUND("scientist/sci_pain4.wav");
	PRECACHE_SOUND("scientist/sci_pain5.wav");

	CTalkMonster::Precache();

	// every new scientist must call this, otherwise
	// when a level is loaded, nobody will talk (time is reset to 0)
	TalkInit();
}

// Init talk data
void CWorker :: TalkInit()
{
	CScientist::TalkInit();

	m_voicePitch = RANDOM_LONG( 90, 94 );
}


//=========================================================
// Dead Worker PROP (barney's)
//=========================================================
class CDeadWorker : public CBaseMonster
{
public:
	void Spawn( void );
	int	Classify ( void ) { return	CLASS_PLAYER_ALLY; }

	void KeyValue( KeyValueData *pkvd );

	int	m_iWeapon;	// body to show ("weapon")
	int	m_iPose;// which sequence to display	-- temporary, don't need to save
	static char *m_szPoses[3];
};

char *CDeadWorker::m_szPoses[] = { "lying_on_back", "lying_on_side", "lying_on_stomach" };

void CDeadWorker::KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "pose"))
	{
		m_iPose = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "weapon"))
	{
		m_iWeapon = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CBaseMonster::KeyValue( pkvd );
}

LINK_ENTITY_TO_CLASS( monster_worker_dead, CDeadWorker );

//=========================================================
// ********** DeadWorker SPAWN **********
//=========================================================
void CDeadWorker :: Spawn( )
{
	PRECACHE_MODEL("models/construction.mdl");
	SET_MODEL(ENT(pev), "models/construction.mdl");

	pev->effects		= 0;
	pev->yaw_speed		= 8;
	pev->sequence		= 0;
	m_bloodColor		= BLOOD_COLOR_RED;

	pev->sequence = LookupSequence( m_szPoses[m_iPose] );
	if (pev->sequence == -1)
	{
		ALERT ( at_console, "Dead Worker with bad pose\n" );
	}
	// Corpses have less health
	pev->health			= 8;

	if ( m_iWeapon )
		pev->body = m_iWeapon;

	MonsterInitDead();
}

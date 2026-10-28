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
// Monster Maker - this is an entity that creates monsters
// in the game.
//=========================================================

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "saverestore.h"

// Monstermaker spawnflags
#define	SF_MONSTERMAKER_START_ON	1 // start active ( if has targetname )
#define	SF_MONSTERMAKER_CYCLIC		4 // drop one monster every time fired.
#define SF_MONSTERMAKER_MONSTERCLIP	8 // Children are blocked by monsterclip
#define SF_MONSTERMAKER_LEAVECORPSE 16 // Don't fade corpses.

// T2DEF: trigger_arcade (arcade.cpp) scores for monsters killed by a turret
char *g_szArcadeMonsters[] = // name ours
{
	"monster_bullchicken",
	"monster_houndeye",
	"monster_bigmomma",
	"monster_zombie",
	"monster_alien_controller",
	"monster_alien_grunt",
};

const int g_iArcadeScores[] = // name ours
{
	100,
	200,
	800,
	150,
	250,
	300,
};

int g_iMakerToggle;		// name ours; T2DEF: set, never read
float g_flMakerTime;		// name ours; T2DEF: set, never read

class CArcade : public CBaseDelay // declared in arcade.cpp
{
public:
	void AddScore( int iScore ); // name ours
};

//=========================================================
// MonsterMaker - this ent creates monsters during the game.
//=========================================================
class CMonsterMaker : public CBaseMonster
{
public:
	void Spawn( void );
	void Precache( void );
	void KeyValue( KeyValueData* pkvd);
	void EXPORT ToggleUse ( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void EXPORT CyclicUse ( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void EXPORT MakerThink ( void );
	void DeathNotice ( entvars_t *pevChild );// monster maker children use this to tell the monster maker that they have died.
	void TryMakeMonster( void ); //LRC- to allow for a spawndelay
	void EXPORT MakeMonster( void );

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );

	static	TYPEDESCRIPTION m_SaveData[];
	
	string_t m_iszMonsterClassname;// classname of the monster(s) that will be created.
	string_t m_iszRunToTarget;
	
	int	 m_cNumMonsters;// max number of monsters this ent can create

	
	int  m_cLiveChildren;// how many monsters made by this monster maker that are currently alive
	int	 m_iMaxLiveChildren;// max number of monsters that this maker may have out at one time.

	float m_flGround; // z coord of the ground under me, used to make sure no monsters are under the maker when it drops a new child

	BOOL m_fActive;
	BOOL m_fFadeChildren;// should we make the children fadeout?
	float m_fSpawnDelay;// LRC- delay between triggering targets and making a child (for env_warpball, mainly)
};

LINK_ENTITY_TO_CLASS( monstermaker, CMonsterMaker );

TYPEDESCRIPTION	CMonsterMaker::m_SaveData[] = 
{
	DEFINE_FIELD( CMonsterMaker, m_iszMonsterClassname, FIELD_STRING ),
	DEFINE_FIELD( CMonsterMaker, m_cNumMonsters, FIELD_INTEGER ),
	DEFINE_FIELD( CMonsterMaker, m_cLiveChildren, FIELD_INTEGER ),
	DEFINE_FIELD( CMonsterMaker, m_flGround, FIELD_FLOAT ),
	DEFINE_FIELD( CMonsterMaker, m_iMaxLiveChildren, FIELD_INTEGER ),
	DEFINE_FIELD( CMonsterMaker, m_fActive, FIELD_BOOLEAN ),
	DEFINE_FIELD( CMonsterMaker, m_fFadeChildren, FIELD_BOOLEAN ),
	DEFINE_FIELD( CMonsterMaker, m_fSpawnDelay, FIELD_FLOAT ),
	DEFINE_FIELD( CMonsterMaker, m_iszRunToTarget, FIELD_STRING ),
};


IMPLEMENT_SAVERESTORE( CMonsterMaker, CBaseMonster );

void CMonsterMaker :: KeyValue( KeyValueData *pkvd )
{
	
	if ( FStrEq(pkvd->szKeyName, "monstercount") )
	{
		m_cNumMonsters = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq(pkvd->szKeyName, "m_imaxlivechildren") )
	{
		m_iMaxLiveChildren = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq(pkvd->szKeyName, "monstertype") )
	{
		m_iszMonsterClassname = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq(pkvd->szKeyName, "spawndelay") )
	{
		m_fSpawnDelay = atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq(pkvd->szKeyName, "runtotarget") )
	{
		m_iszRunToTarget = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		CBaseMonster::KeyValue( pkvd );
}


void CMonsterMaker :: Spawn( )
{
	pev->solid = SOLID_NOT;

	m_cLiveChildren = 0;
	Precache();
	if ( !FStringNull ( pev->targetname ) )
	{
		if ( pev->spawnflags & SF_MONSTERMAKER_CYCLIC )
		{
			SetUse ( CyclicUse );// drop one monster each time we fire
			m_fActive = FALSE;
		}
		else
		{
			SetUse ( ToggleUse );// can be turned on/off

			if ( FBitSet ( pev->spawnflags, SF_MONSTERMAKER_START_ON ) )
			{// start making monsters as soon as monstermaker spawns
				m_fActive = TRUE;
				SetThink ( MakerThink );
			}
			else
			{// wait to be activated.
				m_fActive = FALSE;
				SetThink ( SUB_DoNothing );
			}
		}
	}
	else
	{// no targetname, just start.
			SetNextThink( m_flDelay );
			m_fActive = TRUE;
			SetThink ( MakerThink );
	}

	if ( m_cNumMonsters == 1 || (m_cNumMonsters != -1 && pev->spawnflags & SF_MONSTERMAKER_LEAVECORPSE ))
	{
		m_fFadeChildren = FALSE;
	}
	else
	{
		m_fFadeChildren = TRUE;
	}

	m_flGround = 0;
}

void CMonsterMaker :: Precache( void )
{
	CBaseMonster::Precache();

	UTIL_PrecacheOther( STRING( m_iszMonsterClassname ) );
}

//=========================================================
// TryMakeMonster-  check that it's ok to drop a monster.
//=========================================================
void CMonsterMaker::TryMakeMonster( void )
{
	if ( m_iMaxLiveChildren > 0 && m_cLiveChildren >= m_iMaxLiveChildren )
	{// not allowed to make a new one yet. Too many live ones out right now.
		return;
	}

	if ( !m_flGround )
	{
		// set altitude. Now that I'm activated, any breakables, etc should be out from under me. 
		TraceResult tr;

		UTIL_TraceLine ( pev->origin, pev->origin - Vector ( 0, 0, 2048 ), ignore_monsters, ENT(pev), &tr );
		m_flGround = tr.vecEndPos.z;
	}

	Vector mins = pev->origin - Vector( 34, 34, 0 );
	Vector maxs = pev->origin + Vector( 34, 34, 0 );
	maxs.z = pev->origin.z;
	mins.z = m_flGround;

	CBaseEntity *pList[2];
	int count = UTIL_EntitiesInBox( pList, 2, mins, maxs, FL_CLIENT|FL_MONSTER );
	if ( count )
	{
		// don't build a stack of monsters!
		return;
	}

	// If I have a target, fire!
	if ( !FStringNull ( pev->target ) )
	{
		// delay already overloaded for this entity, so can't call SUB_UseTargets()
		FireTargets( STRING(pev->target), this, this, USE_TOGGLE, 0 );
	}

	if (m_fSpawnDelay)
	{
//		ALERT(at_console,"Making Monster in %f seconds\n",m_fSpawnDelay);
		SetThink( MakeMonster );
		SetNextThink( m_fSpawnDelay );
	}
	else
	{
//		ALERT(at_console,"No delay. Making monster.\n",m_fSpawnDelay);
		MakeMonster();
	}
}

//=========================================================
// MakeMonster-  this is the code that drops the monster
//=========================================================
void CMonsterMaker::MakeMonster( void )
{
	edict_t	*pent;
	entvars_t		*pevCreate;

//	ALERT(at_console,"Making Monster NOW\n");

	pent = CREATE_NAMED_ENTITY( m_iszMonsterClassname );

	if ( FNullEnt( pent ) )
	{
		ALERT ( at_console, "NULL Ent in MonsterMaker!\n" );
		return;
	}

	pevCreate = VARS( pent );
	pevCreate->origin = pev->origin;
	pevCreate->angles = pev->angles;
	SetBits( pevCreate->spawnflags, SF_MONSTER_FALL_TO_GROUND );
	SetBits( pevCreate->spawnflags, 2048 ); // T2DEF: StartPatrol runs (SCHED_IDLE_RUN)

	// Children hit monsterclip brushes
	if ( pev->spawnflags & SF_MONSTERMAKER_MONSTERCLIP )
		SetBits( pevCreate->spawnflags, SF_MONSTER_HITMONSTERCLIP );

	DispatchSpawn( ENT( pevCreate ) );
	pevCreate->owner = edict();

	//LRC - custom monster behaviour
	CBaseEntity *pEntity = CBaseEntity::Instance( pevCreate );
	CBaseMonster *pMonst;
	if (pEntity && (pMonst = pEntity->MyMonsterPointer()) != NULL)
	{
		pMonst->m_iClass = this->m_iClass;
		pMonst->m_iPlayerReact = this->m_iPlayerReact;
	}

	if ( !FStringNull( pev->netname ) )
	{
		// if I have a netname (overloaded), give the child monster that name as a targetname
		pevCreate->targetname = pev->netname;
	}

	if ( !FStringNull( m_iszRunToTarget ) )
	{
		pevCreate->target = m_iszRunToTarget;
	}

	m_cLiveChildren++;// count this monster
	m_cNumMonsters--;

	if ( m_cNumMonsters == 0 )
	{
		// Disable this forever.  Don't kill it because it still gets death notices
		SetThink( NULL );
		SetUse( NULL );
	}
	else if (m_fActive)
	{
		SetNextThink( m_flDelay );
		SetThink( MakerThink );
	}
}

//=========================================================
// CyclicUse - drops one monster from the monstermaker
// each time we call this.
//=========================================================
void CMonsterMaker::CyclicUse ( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	TryMakeMonster();
//	ALERT(at_console,"CyclicUse complete\n");
}

//=========================================================
// ToggleUse - activates/deactivates the monster maker
//=========================================================
void CMonsterMaker :: ToggleUse ( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if ( !ShouldToggle( useType, m_fActive ) )
		return;

	if ( m_fActive )
	{
		g_flMakerTime = 0;
		m_fActive = FALSE;
		SetThink ( NULL );
	}
	else
	{
		g_iMakerToggle = 0;
		if ( g_flMakerTime != 0.0 ) // sic
			g_flMakerTime = gpGlobals->time;
		m_fActive = TRUE;
		SetThink ( MakerThink );
	}

	SetNextThink( 0 );
}

//=========================================================
// MakerThink - creates a new monster every so often
//=========================================================
void CMonsterMaker :: MakerThink ( void )
{
	SetNextThink( m_flDelay );

	TryMakeMonster();
}


//=========================================================
//=========================================================
void CMonsterMaker :: DeathNotice ( entvars_t *pevChild )
{
	// ok, we've gotten the deathnotice from our child, now clear out its owner if we don't want it to fade.
	m_cLiveChildren--;

	if ( !m_fFadeChildren )
	{
		pevChild->owner = NULL;
	}

	// killed by a turret: score for the shooting gallery
	if ( pevChild->iuser2 )
	{
		int iScore = 0; // name ours
		for ( int i = 0; i < 6; i++ )
		{
			if ( FStrEq( STRING( pevChild->classname ), g_szArcadeMonsters[i] ) )
				iScore = g_iArcadeScores[i];
		}

		if ( iScore )
		{
			CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "trigger_arcade" );
			if ( pEntity )
			{
				CArcade *pArcade = (CArcade *)pEntity; // name ours
				pArcade->AddScore( iScore );
			}
		}
	}
}



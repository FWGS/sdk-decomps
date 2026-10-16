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
/*

===== triggers.cpp ========================================================

  spawn and use functions for editor-placed triggers              

*/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "saverestore.h"
#include "trains.h"			// trigger_camera has train functionality
#include "gamerules.h"
#include "weapons.h"			// POV: trigger_playerfreeze holsters the weapon
#include "skill.h"			// POV: area_break is off on hard

#define	SF_TRIGGER_PUSH_START_OFF	2//spawnflag that makes trigger_push spawn turned OFF
#define SF_TRIGGER_HURT_TARGETONCE	1// Only fire hurt target once
#define	SF_TRIGGER_HURT_START_OFF	2//spawnflag that makes trigger_push spawn turned OFF
#define	SF_TRIGGER_HURT_NO_CLIENTS	8//spawnflag that makes trigger_push spawn turned OFF
#define SF_TRIGGER_HURT_CLIENTONLYFIRE	16// trigger hurt will only fire its target if it is hurting a client
#define SF_TRIGGER_HURT_CLIENTONLYTOUCH 32// only clients may touch this trigger.

extern DLL_GLOBAL BOOL		g_fGameOver;
extern DLL_GLOBAL BOOL		g_fLevelLoaded;		// POV, name ours: a level was just loaded or restored
extern DLL_GLOBAL BOOL		g_fSilentPickup;	// POV, name ours

extern void SetMovedir(entvars_t* pev);
extern Vector VecBModelOrigin( entvars_t* pevBModel );

class CFrictionModifier : public CBaseEntity
{
public:
	void		Spawn( void );
	void		KeyValue( KeyValueData *pkvd );
	void EXPORT	ChangeFriction( CBaseEntity *pOther );
	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );

	virtual int	ObjectCaps( void ) { return CBaseEntity :: ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }

	static	TYPEDESCRIPTION m_SaveData[];

	float		m_frictionFraction;		// Sorry, couldn't resist this name :)
};

LINK_ENTITY_TO_CLASS( func_friction, CFrictionModifier );

// Global Savedata for changelevel friction modifier
TYPEDESCRIPTION	CFrictionModifier::m_SaveData[] = 
{
	DEFINE_FIELD( CFrictionModifier, m_frictionFraction, FIELD_FLOAT ),
};

IMPLEMENT_SAVERESTORE(CFrictionModifier,CBaseEntity);


// Modify an entity's friction
void CFrictionModifier :: Spawn( void )
{
	pev->solid = SOLID_TRIGGER;
	SET_MODEL(ENT(pev), STRING(pev->model));    // set size and link into world
	pev->movetype = MOVETYPE_NONE;
	SetTouch( ChangeFriction );
}


// Sets toucher's friction to m_frictionFraction (1.0 = normal friction)
void CFrictionModifier :: ChangeFriction( CBaseEntity *pOther )
{
	if ( pOther->pev->movetype != MOVETYPE_BOUNCEMISSILE && pOther->pev->movetype != MOVETYPE_BOUNCE )
		pOther->pev->friction = m_frictionFraction;
}



// Sets toucher's friction to m_frictionFraction (1.0 = normal friction)
void CFrictionModifier :: KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "modifier"))
	{
		m_frictionFraction = atof(pkvd->szValue) / 100.0;
		pkvd->fHandled = TRUE;
	}
	else
		CBaseEntity::KeyValue( pkvd );
}


// This trigger will fire when the level spawns (or respawns if not fire once)
// It will check a global state before firing.  It supports delay and killtargets

#define SF_AUTO_FIREONCE		0x0001

class CAutoTrigger : public CBaseDelay
{
public:
	void KeyValue( KeyValueData *pkvd );
	void Spawn( void );
	void Precache( void );
	void Think( void );

	int ObjectCaps( void ) { return CBaseDelay::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }
	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );

	static	TYPEDESCRIPTION m_SaveData[];

private:
	int			m_globalstate;
	USE_TYPE	triggerType;
};
LINK_ENTITY_TO_CLASS( trigger_auto, CAutoTrigger );

TYPEDESCRIPTION	CAutoTrigger::m_SaveData[] = 
{
	DEFINE_FIELD( CAutoTrigger, m_globalstate, FIELD_STRING ),
	DEFINE_FIELD( CAutoTrigger, triggerType, FIELD_INTEGER ),
};

IMPLEMENT_SAVERESTORE(CAutoTrigger,CBaseDelay);

void CAutoTrigger::KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "globalstate"))
	{
		m_globalstate = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "triggerstate"))
	{
		int type = atoi( pkvd->szValue );
		switch( type )
		{
		case 0:
			triggerType = USE_OFF;
			break;
		case 2:
			triggerType = USE_TOGGLE;
			break;
		default:
			triggerType = USE_ON;
			break;
		}
		pkvd->fHandled = TRUE;
	}
	else
		CBaseDelay::KeyValue( pkvd );
}


void CAutoTrigger::Spawn( void )
{
	Precache();
}


void CAutoTrigger::Precache( void )
{
	pev->nextthink = gpGlobals->time + 0.1;
}


void CAutoTrigger::Think( void )
{
	if ( !m_globalstate || gGlobalState.EntityGetState( m_globalstate ) == GLOBAL_ON )
	{
		SUB_UseTargets( this, triggerType, 0 );
		if ( pev->spawnflags & SF_AUTO_FIREONCE )
			UTIL_Remove( this );
	}
}



#define SF_RELAY_FIREONCE		0x0001

class CTriggerRelay : public CBaseDelay
{
public:
	void KeyValue( KeyValueData *pkvd );
	void Spawn( void );
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

	int ObjectCaps( void ) { return CBaseDelay::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }
	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );

	static	TYPEDESCRIPTION m_SaveData[];

private:
	USE_TYPE	triggerType;
};
LINK_ENTITY_TO_CLASS( trigger_relay, CTriggerRelay );

TYPEDESCRIPTION	CTriggerRelay::m_SaveData[] = 
{
	DEFINE_FIELD( CTriggerRelay, triggerType, FIELD_INTEGER ),
};

IMPLEMENT_SAVERESTORE(CTriggerRelay,CBaseDelay);

void CTriggerRelay::KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "triggerstate"))
	{
		int type = atoi( pkvd->szValue );
		switch( type )
		{
		case 0:
			triggerType = USE_OFF;
			break;
		case 2:
			triggerType = USE_TOGGLE;
			break;
		default:
			triggerType = USE_ON;
			break;
		}
		pkvd->fHandled = TRUE;
	}
	else
		CBaseDelay::KeyValue( pkvd );
}


void CTriggerRelay::Spawn( void )
{
}




void CTriggerRelay::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	SUB_UseTargets( this, triggerType, 0 );
	if ( pev->spawnflags & SF_RELAY_FIREONCE )
		UTIL_Remove( this );
}


//**********************************************************
// POV: trigger_random - fires its target again and again, after a random
// time between durationmin and durationmax, once it has been triggered.

#define SF_RANDOM_FIREONCE		0x0001

class CTriggerRandom : public CBaseDelay
{
public:
	void KeyValue( KeyValueData *pkvd );
	void Spawn( void );
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void Think( void );
	float RandomDuration( void );	// name ours

	int ObjectCaps( void ) { return CBaseDelay::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }
	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );

	static	TYPEDESCRIPTION m_SaveData[];

private:
	float	m_flDurationMin;
	float	m_flDurationMax;
	BOOL	m_bAlreadyUsed;
};
LINK_ENTITY_TO_CLASS( trigger_random, CTriggerRandom );

TYPEDESCRIPTION	CTriggerRandom::m_SaveData[] =
{
	DEFINE_FIELD( CTriggerRandom, m_bAlreadyUsed, FIELD_BOOLEAN ),
	DEFINE_FIELD( CTriggerRandom, m_flDurationMin, FIELD_BOOLEAN ),
	DEFINE_FIELD( CTriggerRandom, m_flDurationMax, FIELD_BOOLEAN ),
};

IMPLEMENT_SAVERESTORE(CTriggerRandom,CBaseDelay);

void CTriggerRandom::KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "durationmin"))
	{
		m_flDurationMin = atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "durationmax"))
	{
		m_flDurationMax = atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		CBaseDelay::KeyValue( pkvd );
}


void CTriggerRandom::Spawn( void )
{
	m_bAlreadyUsed = FALSE;
	m_flDelay = 0;
}


void CTriggerRandom::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if ( m_bAlreadyUsed )
	{
		ALERT( at_console, "trigger_random: warning - already activated\n" );
		return;
	}

	if ( m_flDurationMax < m_flDurationMin )
	{
		ALERT( at_console, "trigger_random: ERROR - Minimun time is greater then Maximum!\n" );
		return;
	}

	if ( m_flDurationMax == m_flDurationMin )
	{
		ALERT( at_console, "trigger_random: ERROR - Minimun time is equal to Maximum!\n" );
		return;
	}

	m_bAlreadyUsed = TRUE;
	pev->nextthink = gpGlobals->time + RandomDuration();
}


float CTriggerRandom::RandomDuration( void )
{
	return RANDOM_FLOAT( m_flDurationMin, m_flDurationMax );
}


void CTriggerRandom::Think( void )
{
	if ( pev->target )
		FireTargets( STRING(pev->target), this, this, USE_TOGGLE, 0 );

	if ( pev->spawnflags & SF_RANDOM_FIREONCE )
		UTIL_Remove( this );

	pev->nextthink = gpGlobals->time + RandomDuration();
}


//**********************************************************
// POV: trigger_playerfreeze - freezes the player, or lets him go again

#define SF_PLAYERFREEZE_KEEPWEAPON	0x0002	// name ours: don't holster the weapon

class CTriggerPlayerFreeze : public CBaseDelay
{
public:
	void Spawn( void );
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

	int ObjectCaps( void ) { return CBaseDelay::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }
	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );

	static	TYPEDESCRIPTION m_SaveData[];

private:
	BOOL	m_bPlayerFreezed;
	BOOL	m_bFlashLightWasON;
};
LINK_ENTITY_TO_CLASS( trigger_playerfreeze, CTriggerPlayerFreeze );

TYPEDESCRIPTION	CTriggerPlayerFreeze::m_SaveData[] =
{
	DEFINE_FIELD( CTriggerPlayerFreeze, m_bPlayerFreezed, FIELD_BOOLEAN ),
	DEFINE_FIELD( CTriggerPlayerFreeze, m_bFlashLightWasON, FIELD_BOOLEAN ),
};

IMPLEMENT_SAVERESTORE(CTriggerPlayerFreeze,CBaseDelay);

void CTriggerPlayerFreeze::Spawn( void )
{
	m_bPlayerFreezed = FALSE;
}


void CTriggerPlayerFreeze::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	CBasePlayer *pPlayer = (CBasePlayer *)UTIL_FindEntityByClassname( NULL, "player" );

	if ( pPlayer )
	{
		if ( m_bPlayerFreezed )
		{
			pPlayer->EnableControl( TRUE );

			if ( !(pev->spawnflags & SF_PLAYERFREEZE_KEEPWEAPON) && pPlayer->m_pActiveItem )
				pPlayer->m_pActiveItem->Deploy();

			if ( m_bFlashLightWasON )
				pPlayer->FlashlightTurnOn( FALSE );
		}
		else
		{
			m_bFlashLightWasON = pPlayer->FlashlightIsOn();
			if ( m_bFlashLightWasON )
				pPlayer->FlashlightTurnOff( FALSE );

			if ( !(pev->spawnflags & SF_PLAYERFREEZE_KEEPWEAPON) && pPlayer->m_pActiveItem )
			{
				pPlayer->m_pActiveItem->Holster();
				pPlayer->pev->weaponmodel = 0;
			}

			pPlayer->EnableControl( FALSE );
		}

		m_bPlayerFreezed = !m_bPlayerFreezed;
	}
	else
	{
		ALERT( at_console, "trigger_playerfreeze: ERROR - didn't find player\n" );
	}
}


//**********************************************************
// POV: trigger_trogglehud - hides the HUD, or shows it again

class CTriggerTroggleHud : public CBaseDelay
{
public:
	void Spawn( void );
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

	int ObjectCaps( void ) { return CBaseDelay::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }
	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );

	static	TYPEDESCRIPTION m_SaveData[];

private:
	BOOL	m_bHudShowing;
	int		m_iHudStatus;
};
LINK_ENTITY_TO_CLASS( trigger_trogglehud, CTriggerTroggleHud );

TYPEDESCRIPTION	CTriggerTroggleHud::m_SaveData[] =
{
	DEFINE_FIELD( CTriggerTroggleHud, m_bHudShowing, FIELD_BOOLEAN ),
	DEFINE_FIELD( CTriggerTroggleHud, m_iHudStatus, FIELD_INTEGER ),
};

IMPLEMENT_SAVERESTORE(CTriggerTroggleHud,CBaseDelay);

void CTriggerTroggleHud::Spawn( void )
{
	m_bHudShowing = TRUE;
}


void CTriggerTroggleHud::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	CBasePlayer *pPlayer = (CBasePlayer *)UTIL_FindEntityByClassname( NULL, "player" );

	if ( pPlayer )
	{
		if ( m_bHudShowing )
		{
			m_iHudStatus = pPlayer->m_iHideHUD;
			pPlayer->m_iHideHUD |= HIDEHUD_WEAPONS | HIDEHUD_FLASHLIGHT | HIDEHUD_HEALTH;
		}
		else
		{
			pPlayer->m_iHideHUD = m_iHudStatus;
		}

		m_bHudShowing = !m_bHudShowing;
	}
	else
	{
		ALERT( at_console, "trigger_trogglehud: ERROR - didn't find player\n" );
	}
}


//**********************************************************
// POV: trigger_move - moves an entity (the player, a monster) to the
// trigger's target, like a teleport.

#define SF_MOVE_KEEPVELOCITY	0x0001	// name ours

class CTriggerMove : public CBaseDelay
{
public:
	void KeyValue( KeyValueData *pkvd );
	void Spawn( void );
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

	int ObjectCaps( void ) { return CBaseDelay::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }
	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );

	static	TYPEDESCRIPTION m_SaveData[];

private:
	int		m_iszEntToMove;
};
LINK_ENTITY_TO_CLASS( trigger_move, CTriggerMove );

TYPEDESCRIPTION	CTriggerMove::m_SaveData[] =
{
	DEFINE_FIELD( CTriggerMove, m_iszEntToMove, FIELD_STRING ),
};

IMPLEMENT_SAVERESTORE(CTriggerMove,CBaseDelay);

void CTriggerMove::KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "enttomove"))
	{
		m_iszEntToMove = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		CBaseDelay::KeyValue( pkvd );
}


void CTriggerMove::Spawn( void )
{
}


void CTriggerMove::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if ( !m_iszEntToMove )
	{
		ALERT( at_console, "trigger_move: ERROR - invalid Entity To Move\n" );
		return;
	}

	CBaseEntity *pEntity = CBaseEntity::Instance( FIND_ENTITY_BY_TARGETNAME( NULL, STRING(m_iszEntToMove) ) );
	if ( !pEntity )
	{
		ALERT( at_console, "trigger_move: ERROR - couldnt find Entity To Move\n" );
		return;
	}

	if ( !pev->target )
	{
		ALERT( at_console, "trigger_move: ERROR - invalid destination to move\n" );
		return;
	}

	edict_t	*pentTarget = FIND_ENTITY_BY_TARGETNAME( NULL, STRING(pev->target) );
	if ( FNullEnt(pentTarget) )
	{
		ALERT( at_console, "trigger_move: ERROR - couldnt find destination to move\n" );
		return;
	}

	entvars_t* pevToucher = pEntity->pev;

	if ( pevToucher->deadflag != DEAD_DEAD && !FBitSet( pevToucher->flags, FL_CLIENT|FL_MONSTER ) )
	{
		if ( !FClassnameIs( pevToucher, "monster_sitting_scientist" ) )
		{
			ALERT( at_console, "trigger_move: ERROR - Entity To Move not player or monster\n" );
			return;
		}
	}

	Vector tmp = VARS( pentTarget )->origin;

	if ( pEntity->IsPlayer() )
	{
		tmp.z -= pEntity->pev->mins.z;// make origin adjustments in case the teleportee is a player. (origin in center, not at feet)
	}

	tmp.z++;

	pEntity->pev->flags &= ~FL_ONGROUND;

	UTIL_SetOrigin( pEntity->pev, tmp );

	pEntity->pev->angles = pentTarget->v.angles;

	if ( pEntity->IsPlayer() )
	{
		pEntity->pev->v_angle = pentTarget->v.angles;
	}

	pEntity->pev->fixangle = TRUE;

	if ( !(pev->spawnflags & SF_MOVE_KEEPVELOCITY) )
		pEntity->pev->velocity = pEntity->pev->basevelocity = g_vecZero;
}


//**********************************************************
// The Multimanager Entity - when fired, will fire up to 16 targets
// at specified times.
// FLAG:		THREAD (create clones when triggered)
// FLAG:		CLONE (this is a clone for a threaded execution)

#define SF_MULTIMAN_CLONE		0x80000000
#define SF_MULTIMAN_THREAD		0x00000001

class CMultiManager : public CBaseToggle
{
public:
	void KeyValue( KeyValueData *pkvd );
	void Spawn ( void );
	void EXPORT ManagerThink ( void );
	void EXPORT ManagerUse   ( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

#if _DEBUG
	void EXPORT ManagerReport( void );
#endif

	BOOL		HasTarget( string_t targetname );
	
	int ObjectCaps( void ) { return CBaseToggle::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );

	static	TYPEDESCRIPTION m_SaveData[];

	int		m_cTargets;	// the total number of targets in this manager's fire list.
	int		m_index;	// Current target
	float	m_startTime;// Time we started firing
	int		m_iTargetName	[ MAX_MULTI_TARGETS ];// list if indexes into global string array
	float	m_flTargetDelay [ MAX_MULTI_TARGETS ];// delay (in seconds) from time of manager fire to target fire
private:
	inline BOOL IsClone( void ) { return (pev->spawnflags & SF_MULTIMAN_CLONE) ? TRUE : FALSE; }
	inline BOOL ShouldClone( void ) 
	{ 
		if ( IsClone() )
			return FALSE;

		return (pev->spawnflags & SF_MULTIMAN_THREAD) ? TRUE : FALSE; 
	}

	CMultiManager *Clone( void );
};
LINK_ENTITY_TO_CLASS( multi_manager, CMultiManager );

// Global Savedata for multi_manager
TYPEDESCRIPTION	CMultiManager::m_SaveData[] = 
{
	DEFINE_FIELD( CMultiManager, m_cTargets, FIELD_INTEGER ),
	DEFINE_FIELD( CMultiManager, m_index, FIELD_INTEGER ),
	DEFINE_FIELD( CMultiManager, m_startTime, FIELD_TIME ),
	DEFINE_ARRAY( CMultiManager, m_iTargetName, FIELD_STRING, MAX_MULTI_TARGETS ),
	DEFINE_ARRAY( CMultiManager, m_flTargetDelay, FIELD_FLOAT, MAX_MULTI_TARGETS ),
};

IMPLEMENT_SAVERESTORE(CMultiManager,CBaseToggle);

void CMultiManager :: KeyValue( KeyValueData *pkvd )
{
	// UNDONE: Maybe this should do something like this:
	//CBaseToggle::KeyValue( pkvd );
	// if ( !pkvd->fHandled )
	// ... etc.

	if (FStrEq(pkvd->szKeyName, "wait"))
	{
		m_flWait = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else // add this field to the target list
	{
		// this assumes that additional fields are targetnames and their values are delay values.
		if ( m_cTargets < MAX_MULTI_TARGETS )
		{
			char tmp[128];

			UTIL_StripToken( pkvd->szKeyName, tmp );
			m_iTargetName [ m_cTargets ] = ALLOC_STRING( tmp );
			m_flTargetDelay [ m_cTargets ] = atof (pkvd->szValue);

			// POV: warn about target names longer than 20 characters
			if ( m_iTargetName [ m_cTargets ] && strlen( STRING( m_iTargetName [ m_cTargets ] ) ) > 20 )
				ALERT( at_console, "==================>     NAME TOO BIG %d !!!!! %s <==================\n", strlen( STRING( m_iTargetName [ m_cTargets ] ) ), STRING( m_iTargetName [ m_cTargets ] ) );

			m_cTargets++;
			pkvd->fHandled = TRUE;
		}
	}
}


void CMultiManager :: Spawn( void )
{
	pev->solid = SOLID_NOT;
	SetUse ( ManagerUse );
	SetThink ( ManagerThink);

	// Sort targets
	// Quick and dirty bubble sort
	int swapped = 1;

	while ( swapped )
	{
		swapped = 0;
		for ( int i = 1; i < m_cTargets; i++ )
		{
			if ( m_flTargetDelay[i] < m_flTargetDelay[i-1] )
			{
				// Swap out of order elements
				int name = m_iTargetName[i];
				float delay = m_flTargetDelay[i];
				m_iTargetName[i] = m_iTargetName[i-1];
				m_flTargetDelay[i] = m_flTargetDelay[i-1];
				m_iTargetName[i-1] = name;
				m_flTargetDelay[i-1] = delay;
				swapped = 1;
			}
		}
	}
}


BOOL CMultiManager::HasTarget( string_t targetname )
{ 
	for ( int i = 0; i < m_cTargets; i++ )
		if ( FStrEq(STRING(targetname), STRING(m_iTargetName[i])) )
			return TRUE;
	
	return FALSE;
}


// Designers were using this to fire targets that may or may not exist -- 
// so I changed it to use the standard target fire code, made it a little simpler.
void CMultiManager :: ManagerThink ( void )
{
	float	time;
	char	*pszTarget = "";	// sic: POV copies the name into a string literal

	time = gpGlobals->time - m_startTime;
	while ( m_index < m_cTargets && m_flTargetDelay[ m_index ] <= time )
	{
		strcpy( pszTarget, STRING( m_iTargetName[ m_index ] ) );

		// POV: "-name" removes every entity with that name
		if ( pszTarget[0] == '-' )
		{
			edict_t *pentTarget = FIND_ENTITY_BY_TARGETNAME( NULL, &pszTarget[1] );
			while ( !FNullEnt( pentTarget ) )
			{
				UTIL_Remove( CBaseEntity::Instance( pentTarget ) );
				pentTarget = FIND_ENTITY_BY_TARGETNAME( pentTarget, &pszTarget[1] );
			}
		}
		else
			FireTargets( STRING( m_iTargetName[ m_index ] ), m_hActivator, this, USE_TOGGLE, 0 );
		m_index++;
	}

	if ( m_index >= m_cTargets )// have we fired all targets?
	{
		SetThink( NULL );
		if ( IsClone() )
		{
			UTIL_Remove( this );
			return;
		}
		SetUse ( ManagerUse );// allow manager re-use 
	}
	else
		pev->nextthink = m_startTime + m_flTargetDelay[ m_index ];
}

CMultiManager *CMultiManager::Clone( void )
{
	CMultiManager *pMulti = GetClassPtr( (CMultiManager *)NULL );

	edict_t *pEdict = pMulti->pev->pContainingEntity;
	memcpy( pMulti->pev, pev, sizeof(*pev) );
	pMulti->pev->pContainingEntity = pEdict;

	pMulti->pev->spawnflags |= SF_MULTIMAN_CLONE;
	pMulti->m_cTargets = m_cTargets;
	memcpy( pMulti->m_iTargetName, m_iTargetName, sizeof( m_iTargetName ) );
	memcpy( pMulti->m_flTargetDelay, m_flTargetDelay, sizeof( m_flTargetDelay ) );

	return pMulti;
}


// The USE function builds the time table and starts the entity thinking.
void CMultiManager :: ManagerUse ( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	// In multiplayer games, clone the MM and execute in the clone (like a thread)
	// to allow multiple players to trigger the same multimanager
	if ( ShouldClone() )
	{
		CMultiManager *pClone = Clone();
		pClone->ManagerUse( pActivator, pCaller, useType, value );
		return;
	}

	m_hActivator = pActivator;
	m_index = 0;
	m_startTime = gpGlobals->time;

	SetUse( NULL );// disable use until all targets have fired

	SetThink ( ManagerThink );
	pev->nextthink = gpGlobals->time;
}

#if _DEBUG
void CMultiManager :: ManagerReport ( void )
{
	int	cIndex;

	for ( cIndex = 0 ; cIndex < m_cTargets ; cIndex++ )
	{
		ALERT ( at_console, "%s %f\n", STRING(m_iTargetName[cIndex]), m_flTargetDelay[cIndex] );
	}
}
#endif

//***********************************************************


//
// Render parameters trigger
//
// This entity will copy its render parameters (renderfx, rendermode, rendercolor, renderamt)
// to its targets when triggered.
//


// Flags to indicate masking off various render parameters that are normally copied to the targets
#define SF_RENDER_MASKFX	(1<<0)
#define SF_RENDER_MASKAMT	(1<<1)
#define SF_RENDER_MASKMODE	(1<<2)
#define SF_RENDER_MASKCOLOR	(1<<3)

class CRenderFxManager : public CBaseEntity
{
public:
	void Spawn( void );
	void Use ( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
};

LINK_ENTITY_TO_CLASS( env_render, CRenderFxManager );


void CRenderFxManager :: Spawn ( void )
{
	pev->solid = SOLID_NOT;
}

void CRenderFxManager :: Use ( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if (!FStringNull(pev->target))
	{
		edict_t* pentTarget	= NULL;
		while ( 1 )
		{
			pentTarget = FIND_ENTITY_BY_TARGETNAME(pentTarget, STRING(pev->target));
			if (FNullEnt(pentTarget))
				break;

			entvars_t *pevTarget = VARS( pentTarget );
			if ( !FBitSet( pev->spawnflags, SF_RENDER_MASKFX ) )
				pevTarget->renderfx = pev->renderfx;
			if ( !FBitSet( pev->spawnflags, SF_RENDER_MASKAMT ) )
				pevTarget->renderamt = pev->renderamt;
			if ( !FBitSet( pev->spawnflags, SF_RENDER_MASKMODE ) )
				pevTarget->rendermode = pev->rendermode;
			if ( !FBitSet( pev->spawnflags, SF_RENDER_MASKCOLOR ) )
				pevTarget->rendercolor = pev->rendercolor;
		}
	}
}



class CBaseTrigger : public CBaseToggle
{
public:
	void EXPORT TeleportTouch ( CBaseEntity *pOther );
	void KeyValue( KeyValueData *pkvd );
	void EXPORT MultiTouch( CBaseEntity *pOther );
	void EXPORT HurtTouch ( CBaseEntity *pOther );
	void EXPORT CDAudioTouch ( CBaseEntity *pOther );
	void ActivateMultiTrigger( CBaseEntity *pActivator );
	void EXPORT MultiWaitOver( void );
	void EXPORT CounterUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void EXPORT ToggleUse ( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void InitTrigger( void );
	void Precache( void );

	virtual int	ObjectCaps( void ) { return CBaseToggle :: ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }

	float	m_flNextHealSound;	// POV, name ours: trigger_hurt's healing sound
};

LINK_ENTITY_TO_CLASS( trigger, CBaseTrigger );

/*
================
InitTrigger
================
*/
void CBaseTrigger::InitTrigger( )
{
	// trigger angles are used for one-way touches.  An angle of 0 is assumed
	// to mean no restrictions, so use a yaw of 360 instead.
	if (pev->angles != g_vecZero)
		SetMovedir(pev);
	pev->solid = SOLID_TRIGGER;
	pev->movetype = MOVETYPE_NONE;
	SET_MODEL(ENT(pev), STRING(pev->model));    // set size and link into world
	if ( CVAR_GET_FLOAT("showtriggers") == 0 )
		SetBits( pev->effects, EF_NODRAW );
}


//
// Cache user-entity-field values until spawn is called.
//

void CBaseTrigger :: KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "damage"))
	{
		pev->dmg = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "count"))
	{
		m_cTriggersLeft = (int) atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "damagetype"))
	{
		m_bitsDamageInflict = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CBaseToggle::KeyValue( pkvd );
}

class CTriggerHurt : public CBaseTrigger
{
public:
	void Spawn( void );
	void EXPORT RadiationThink( void );
};

LINK_ENTITY_TO_CLASS( trigger_hurt, CTriggerHurt );

//
// trigger_monsterjump
//
class CTriggerMonsterJump : public CBaseTrigger
{
public:
	void Spawn( void );
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void Jump( CBaseEntity *pOther );	// POV, name ours: was Touch
};

LINK_ENTITY_TO_CLASS( trigger_monsterjump, CTriggerMonsterJump );


void CTriggerMonsterJump :: Spawn ( void )
{
	SetMovedir ( pev );
	
	InitTrigger ();
}


// POV: when fired, tosses every living monster inside the trigger
void CTriggerMonsterJump :: Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	CBaseEntity *pList[10];

	int count = UTIL_EntitiesInBox( pList, 10, pev->mins, pev->maxs, FL_MONSTER );
	if ( count )
	{
		for ( int i = 0; i < count; i++ )
		{
			if ( pList[i] != this && pList[i]->pev->deadflag == DEAD_NO )
				Jump( pList[i] );
		}
	}
}

void CTriggerMonsterJump :: Jump( CBaseEntity *pOther )
{
	entvars_t *pevOther = pOther->pev;

	if ( !FBitSet ( pevOther->flags , FL_MONSTER ) ) 
	{// touched by a non-monster.
		return;
	}

	pevOther->origin.z += 1;
	
	if ( FBitSet ( pevOther->flags, FL_ONGROUND ) ) 
	{// clear the onground so physics don't bitch
		pevOther->flags &= ~FL_ONGROUND;
	}

	// toss the monster!
	pevOther->velocity = pev->movedir * pev->speed;
	pevOther->velocity.z += pev->health;
	pev->nextthink = gpGlobals->time;
}


//=====================================
//
// trigger_cdaudio - starts/stops cd audio tracks
//
class CTriggerCDAudio : public CBaseTrigger
{
public:
	void Spawn( void );

	virtual void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void PlayTrack( void );
	void Touch ( CBaseEntity *pOther );
};

LINK_ENTITY_TO_CLASS( trigger_cdaudio, CTriggerCDAudio );

//
// Changes tracks or stops CD when player touches
//
// !!!HACK - overloaded HEALTH to avoid adding new field
void CTriggerCDAudio :: Touch ( CBaseEntity *pOther )
{
	if ( !pOther->IsPlayer() )
	{// only clients may trigger these events
		return;
	}

	PlayTrack();
}

void CTriggerCDAudio :: Spawn( void )
{
	InitTrigger();
}

void CTriggerCDAudio::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	PlayTrack();
}

void PlayCDTrack( int iTrack )
{
	edict_t *pClient;
	
	// manually find the single player. 
	pClient = g_engfuncs.pfnPEntityOfEntIndex( 1 );
	
	// Can't play if the client is not connected!
	if ( !pClient )
		return;

	if ( iTrack < -1 || iTrack > 30 )
	{
		ALERT ( at_console, "TriggerCDAudio - Track %d out of range\n" );
		return;
	}

	if ( iTrack == -1 )
	{
		CLIENT_COMMAND ( pClient, "cd pause\n");
	}
	else
	{
		char string [ 64 ];

		sprintf( string, "cd play %3d\n", iTrack );
		CLIENT_COMMAND ( pClient, string);
	}
}


// only plays for ONE client, so only use in single play!
void CTriggerCDAudio :: PlayTrack( void )
{
	PlayCDTrack( (int)pev->health );
	
	SetTouch( NULL );
	UTIL_Remove( this );
}


// This plays a CD track when fired or when the player enters it's radius
class CTargetCDAudio : public CPointEntity
{
public:
	void			Spawn( void );
	void			KeyValue( KeyValueData *pkvd );

	virtual void	Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void			Think( void );
	void			Play( void );
};

LINK_ENTITY_TO_CLASS( target_cdaudio, CTargetCDAudio );

void CTargetCDAudio :: KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "radius"))
	{
		pev->scale = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CPointEntity::KeyValue( pkvd );
}

void CTargetCDAudio :: Spawn( void )
{
	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_NONE;

	if ( pev->scale > 0 )
		pev->nextthink = gpGlobals->time + 1.0;
}

void CTargetCDAudio::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	Play();
}

// only plays for ONE client, so only use in single play!
void CTargetCDAudio::Think( void )
{
	edict_t *pClient;
	
	// manually find the single player. 
	pClient = g_engfuncs.pfnPEntityOfEntIndex( 1 );
	
	// Can't play if the client is not connected!
	if ( !pClient )
		return;
	
	pev->nextthink = gpGlobals->time + 0.5;

	if ( (pClient->v.origin - pev->origin).Length() <= pev->scale )
		Play();

}

void CTargetCDAudio::Play( void ) 
{ 
	PlayCDTrack( (int)pev->health );
	UTIL_Remove(this); 
}

//=====================================
//
// POV: target_fileplay - plays a sound file at the player
//
class CTargetFilePlay : public CPointEntity
{
public:
	void			Spawn( void );
	void			Precache( void );
	void			KeyValue( KeyValueData *pkvd );

	virtual void	Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );

	static	TYPEDESCRIPTION m_SaveData[];

	int		m_iszFileToPlay;
	float	m_flVolume;
};

LINK_ENTITY_TO_CLASS( target_fileplay, CTargetFilePlay );

TYPEDESCRIPTION	CTargetFilePlay::m_SaveData[] = 
{
	DEFINE_FIELD( CTargetFilePlay, m_iszFileToPlay, FIELD_STRING ),
	DEFINE_FIELD( CTargetFilePlay, m_flVolume, FIELD_FLOAT ),
};

IMPLEMENT_SAVERESTORE(CTargetFilePlay,CBaseEntity);

void CTargetFilePlay :: KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "filetoplay"))
	{
		m_iszFileToPlay = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "volume"))
	{
		if ( atoi( pkvd->szValue ) >= 0 && atoi( pkvd->szValue ) <= 10 )
			m_flVolume = atoi( pkvd->szValue );
		else
			m_flVolume = 10;
		pkvd->fHandled = TRUE;
	}
	else
		pkvd->fHandled = FALSE;
}

void CTargetFilePlay :: Spawn( void )
{
	Precache();

	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_NONE;
}

void CTargetFilePlay :: Precache( void )
{
	if ( m_iszFileToPlay )
		PRECACHE_SOUND( (char *)STRING( m_iszFileToPlay ) );
}

void CTargetFilePlay::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if ( m_iszFileToPlay )
	{
		CBaseEntity *pPlayer = UTIL_FindEntityByClassname( NULL, "player" );
		if ( pPlayer )
			UTIL_EmitAmbientSound( pPlayer->edict(), pPlayer->pev->origin, STRING( m_iszFileToPlay ), m_flVolume, ATTN_NONE, 0, PITCH_NORM );
		else
			ALERT( at_console, "target_fileplay: ERROR - couldn't find player!!\n" );
	}
	else
		ALERT( at_console, "target_fileplay: ERROR - no file to play given!!\n" );
}

//=====================================
//
// POV: trigger_sound - sets the player's DSP room type (in health) by
// brush, as env_sound does by radius
//
class CTriggerSound : public CBaseTrigger	// name ours
{
public:
	void Spawn( void );

	virtual void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void Touch ( CBaseEntity *pOther );
	void SetRoomType( float flRoomtype, float flRange );	// name ours
};

LINK_ENTITY_TO_CLASS( trigger_sound, CTriggerSound );

void CTriggerSound :: Touch ( CBaseEntity *pOther )
{
	if ( pOther->IsPlayer() )
		SetRoomType( pev->health, 64 );
}

void CTriggerSound :: Spawn( void )
{
	InitTrigger();
}

void CTriggerSound::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	SetRoomType( pev->health, 64 );
}

void CTriggerSound :: SetRoomType( float flRoomtype, float flRange )
{
	CBasePlayer *pPlayer = (CBasePlayer *)UTIL_FindEntityByClassname( NULL, "player" );

	if ( pPlayer->m_flSndRoomtype != flRoomtype )
	{
		if ( pPlayer->pev->waterlevel == 3 )
		{
			ALERT( at_console, "trigger_sound: underwater not setting to %d - %s\n", (int)flRoomtype, UTIL_RoomTypeName( flRoomtype ) );
			pPlayer->m_flSndRoomtype = 0;
			return;
		}

		ALERT( at_console, "trigger_sound: set to %d - %s \n", (int)flRoomtype, UTIL_RoomTypeName( flRoomtype ) );

		pPlayer->m_pentSndLast = ENT(pev);
		pPlayer->m_flSndRoomtype = flRoomtype;
		pPlayer->m_flSndRange = flRange;

		MESSAGE_BEGIN( MSG_ONE, SVC_ROOMTYPE, NULL, pPlayer->edict() );
			WRITE_SHORT( (short)flRoomtype );
		MESSAGE_END();
	}
}

//=====================================
//
// POV: trigger_xhand - gives the player the alien slave hands (and the
// suit), without the pickup sound
//
class CTriggerXHand : public CBaseTrigger	// name ours
{
public:
	void Spawn( void );

	virtual void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
};

LINK_ENTITY_TO_CLASS( trigger_xhand, CTriggerXHand );

void CTriggerXHand :: Spawn( void )
{
	InitTrigger();
}

void CTriggerXHand::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	CBasePlayer *pPlayer = (CBasePlayer *)UTIL_FindEntityByClassname( NULL, "player" );

	if ( pPlayer )
	{
		if ( !(pPlayer->pev->weapons & (1<<WEAPON_SUIT)) )
			pPlayer->pev->weapons |= (1<<WEAPON_SUIT);

		g_fSilentPickup = TRUE;
		pPlayer->GiveNamedItem( "weapon_xhand" );
		g_fSilentPickup = FALSE;
	}
	else
		ALERT( at_console, "Cant give weapon_xhand!!\n" );
}

//=====================================
//
// POV: trigger_die - kills the monster named monstername
//
class CTriggerDie : public CBaseTrigger
{
public:
	void Spawn( void );
	void KeyValue( KeyValueData *pkvd );

	virtual void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );

	static	TYPEDESCRIPTION m_SaveData[];

	char	m_sDieMonster[32];
};

LINK_ENTITY_TO_CLASS( trigger_die, CTriggerDie );

TYPEDESCRIPTION	CTriggerDie::m_SaveData[] = 
{
	DEFINE_ARRAY( CTriggerDie, m_sDieMonster, FIELD_CHARACTER, 32 ),
};

IMPLEMENT_SAVERESTORE(CTriggerDie,CBaseEntity);

void CTriggerDie :: Spawn( void )
{
}

void CTriggerDie::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	CBaseEntity *pMonster = UTIL_FindEntityByTargetname( NULL, m_sDieMonster );

	if ( !pMonster )
	{
		ALERT( at_console, "trigger_die: didn't find monster! Name too big? (MAX: 32)\n" );
		return;
	}

	pMonster->TakeDamage( pev, pev, pMonster->pev->health + 1, DMG_GENERIC );
}

void CTriggerDie :: KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "monstername"))
	{
		strcpy( m_sDieMonster, pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		CBaseTrigger::KeyValue( pkvd );
}

//=====================================
//
// POV: trigger_showgametitle - shows the game title (the GameTitle message)
//
class CTriggerShowGameTitle : public CBaseTrigger	// name ours
{
public:
	void Spawn( void );

	virtual void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
};

LINK_ENTITY_TO_CLASS( trigger_showgametitle, CTriggerShowGameTitle );

void CTriggerShowGameTitle :: Spawn( void )
{
}

void CTriggerShowGameTitle::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	int iGameTitle = REG_USER_MSG( "GameTitle", 1 );	// name ours

	CBaseEntity *pPlayer = UTIL_FindEntityByClassname( NULL, "player" );
	if ( pPlayer )
	{
		MESSAGE_BEGIN( MSG_ONE, iGameTitle, NULL, pPlayer->edict() );
			WRITE_BYTE( 0 );
		MESSAGE_END();
	}
	else
		ALERT( at_console, "trigger_showgametitle: ERROR - player not found!!\n" );
}

//=====================================

//
// trigger_hurt - hurts anything that touches it. if the trigger has a targetname, firing it will toggle state
//
//int gfToggleState = 0; // used to determine when all radiation trigger hurts have called 'RadiationThink'

void CTriggerHurt :: Spawn( void )
{
	CBaseTrigger::Precache();
	InitTrigger();
	SetTouch ( HurtTouch );

	if ( !FStringNull ( pev->targetname ) )
	{
		SetUse ( ToggleUse );
	}
	else
	{
		SetUse ( NULL );
	}

	if (m_bitsDamageInflict & DMG_RADIATION)
	{
		SetThink ( RadiationThink );
		pev->nextthink = gpGlobals->time + RANDOM_FLOAT(0.0, 0.5); 
	}

	if ( FBitSet (pev->spawnflags, SF_TRIGGER_HURT_START_OFF) )// if flagged to Start Turned Off, make trigger nonsolid.
		pev->solid = SOLID_NOT;

	UTIL_SetOrigin( pev, pev->origin );		// Link into the list
}

// trigger hurt that causes radiation will do a radius
// check and set the player's geiger counter level
// according to distance from center of trigger

void CTriggerHurt :: RadiationThink( void )
{

	edict_t *pentPlayer;
	CBasePlayer *pPlayer = NULL;
	float flRange;
	entvars_t *pevTarget;
	Vector vecSpot1;
	Vector vecSpot2;
	Vector vecRange;
	Vector origin;
	Vector view_ofs;

	// check to see if a player is in pvs
	// if not, continue	

	// set origin to center of trigger so that this check works
	origin = pev->origin;
	view_ofs = pev->view_ofs;

	pev->origin = (pev->absmin + pev->absmax) * 0.5;
	pev->view_ofs = pev->view_ofs * 0.0;

	pentPlayer = FIND_CLIENT_IN_PVS(edict());

	pev->origin = origin;
	pev->view_ofs = view_ofs;

	// reset origin

	if (!FNullEnt(pentPlayer))
	{
 
		pPlayer = GetClassPtr( (CBasePlayer *)VARS(pentPlayer));

		pevTarget = VARS(pentPlayer);

		// get range to player;

		vecSpot1 = (pev->absmin + pev->absmax) * 0.5;
		vecSpot2 = (pevTarget->absmin + pevTarget->absmax) * 0.5;
		
		vecRange = vecSpot1 - vecSpot2;
		flRange = vecRange.Length();

		// if player's current geiger counter range is larger
		// than range to this trigger hurt, reset player's
		// geiger counter range 

		if (pPlayer->m_flgeigerRange >= flRange)
			pPlayer->m_flgeigerRange = flRange;
	}

	pev->nextthink = gpGlobals->time + 0.25;
}

//
// ToggleUse - If this is the USE function for a trigger, its state will toggle every time it's fired
//
void CBaseTrigger :: ToggleUse ( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if (pev->solid == SOLID_NOT)
	{// if the trigger is off, turn it on
		pev->solid = SOLID_TRIGGER;
		
		// Force retouch
		gpGlobals->force_retouch++;
	}
	else
	{// turn the trigger off
		pev->solid = SOLID_NOT;
	}
	UTIL_SetOrigin( pev, pev->origin );
}

void CBaseTrigger :: Precache( void )
{
	PRECACHE_SOUND( "ambience/healingpond1.wav" );
}

// When touched, a hurt trigger does DMG points of damage each half-second
void CBaseTrigger :: HurtTouch ( CBaseEntity *pOther )
{
	float fldmg;

	if ( !pOther->pev->takedamage )
		return;

	if ( (pev->spawnflags & SF_TRIGGER_HURT_CLIENTONLYTOUCH) && !pOther->IsPlayer() )
	{
		// this trigger is only allowed to touch clients, and this ain't a client.
		return;
	}

	if ( (pev->spawnflags & SF_TRIGGER_HURT_NO_CLIENTS) && pOther->IsPlayer() )
		return;

	// HACKHACK -- In multiplayer, players touch this based on packet receipt.
	// So the players who send packets later aren't always hurt.  Keep track of
	// how much time has passed and whether or not you've touched that player
	if ( g_pGameRules->IsMultiplayer() )
	{
		if ( pev->dmgtime > gpGlobals->time )
		{
			if ( gpGlobals->time != pev->pain_finished )
			{// too early to hurt again, and not same frame with a different entity
				if ( pOther->IsPlayer() )
				{
					int playerMask = 1 << (pOther->entindex() - 1);

					// If I've already touched this player (this time), then bail out
					if ( pev->impulse & playerMask )
						return;

					// Mark this player as touched
					// BUGBUG - There can be only 32 players!
					pev->impulse |= playerMask;
				}
				else
				{
					return;
				}
			}
		}
		else
		{
			// New clock, "un-touch" all players
			pev->impulse = 0;
			if ( pOther->IsPlayer() )
			{
				int playerMask = 1 << (pOther->entindex() - 1);

				// Mark this player as touched
				// BUGBUG - There can be only 32 players!
				pev->impulse |= playerMask;
			}
		}
	}
	else	// Original code -- single player
	{
		if ( pev->dmgtime > gpGlobals->time && gpGlobals->time != pev->pain_finished )
		{// too early to hurt again, and not same frame with a different entity
			return;
		}
	}



	// If this is time_based damage (poison, radiation), override the pev->dmg with a 
	// default for the given damage type.  Monsters only take time-based damage
	// while touching the trigger.  Player continues taking damage for a while after
	// leaving the trigger

	fldmg = pev->dmg * 0.5;	// 0.5 seconds worth of damage, pev->dmg is damage/second


	// JAY: Cut this because it wasn't fully realized.  Damage is simpler now.
#if 0
	switch (m_bitsDamageInflict)
	{
	default: break;
	case DMG_POISON:		fldmg = POISON_DAMAGE/4; break;
	case DMG_NERVEGAS:		fldmg = NERVEGAS_DAMAGE/4; break;
	case DMG_RADIATION:		fldmg = RADIATION_DAMAGE/4; break;
	case DMG_PARALYZE:		fldmg = PARALYZE_DAMAGE/4; break; // UNDONE: cut this? should slow movement to 50%
	case DMG_ACID:			fldmg = ACID_DAMAGE/4; break;
	case DMG_SLOWBURN:		fldmg = SLOWBURN_DAMAGE/4; break;
	case DMG_SLOWFREEZE:	fldmg = SLOWFREEZE_DAMAGE/4; break;
	}
#endif

	if ( fldmg < 0 )
	{
		// POV: the healing sound, once a second
		if ( pOther->TakeHealth( -fldmg, m_bitsDamageInflict ) == 1 && gpGlobals->time > m_flNextHealSound )
		{
			EMIT_SOUND_DYN( ENT(pev), CHAN_ITEM, "ambience/healingpond1.wav", 0.8, 0.8, 0, PITCH_NORM );
			m_flNextHealSound = gpGlobals->time + 1.0;
		}
	}
	else
		pOther->TakeDamage( pev, pev, fldmg, m_bitsDamageInflict );

	// Store pain time so we can get all of the other entities on this frame
	pev->pain_finished = gpGlobals->time;

	// Apply damage every half second
	pev->dmgtime = gpGlobals->time + 0.5;// half second delay until this trigger can hurt toucher again

  
	
	if ( pev->target )
	{
		// trigger has a target it wants to fire. 
		if ( pev->spawnflags & SF_TRIGGER_HURT_CLIENTONLYFIRE )
		{
			// if the toucher isn't a client, don't fire the target!
			if ( !pOther->IsPlayer() )
			{
				return;
			}
		}

		SUB_UseTargets( pOther, USE_TOGGLE, 0 );
		if ( pev->spawnflags & SF_TRIGGER_HURT_TARGETONCE )
			pev->target = 0;
	}
}


/*QUAKED trigger_multiple (.5 .5 .5) ? notouch
Variable sized repeatable trigger.  Must be targeted at one or more entities.
If "health" is set, the trigger must be killed to activate each time.
If "delay" is set, the trigger waits some time after activating before firing.
"wait" : Seconds between triggerings. (.2 default)
If notouch is set, the trigger is only fired by other entities, not by touching.
NOTOUCH has been obsoleted by trigger_relay!
sounds
1)      secret
2)      beep beep
3)      large switch
4)
NEW
if a trigger has a NETNAME, that NETNAME will become the TARGET of the triggered object.
*/
class CTriggerMultiple : public CBaseTrigger
{
public:
	void Spawn( void );
};

LINK_ENTITY_TO_CLASS( trigger_multiple, CTriggerMultiple );


void CTriggerMultiple :: Spawn( void )
{
	if (m_flWait == 0)
		m_flWait = 0.2;

	InitTrigger();

	ASSERTSZ(pev->health == 0, "trigger_multiple with health");
//	UTIL_SetOrigin(pev, pev->origin);
//	SET_MODEL( ENT(pev), STRING(pev->model) );
//	if (pev->health > 0)
//		{
//		if (FBitSet(pev->spawnflags, SPAWNFLAG_NOTOUCH))
//			ALERT(at_error, "trigger_multiple spawn: health and notouch don't make sense");
//		pev->max_health = pev->health;
//UNDONE: where to get pfnDie from?
//		pev->pfnDie = multi_killed;
//		pev->takedamage = DAMAGE_YES;
//		pev->solid = SOLID_BBOX;
//		UTIL_SetOrigin(pev, pev->origin);  // make sure it links into the world
//		}
//	else
		{
			SetTouch( MultiTouch );
		}
	}


/*QUAKED trigger_once (.5 .5 .5) ? notouch
Variable sized trigger. Triggers once, then removes itself.  You must set the key "target" to the name of another object in the level that has a matching
"targetname".  If "health" is set, the trigger must be killed to activate.
If notouch is set, the trigger is only fired by other entities, not by touching.
if "killtarget" is set, any objects that have a matching "target" will be removed when the trigger is fired.
if "angle" is set, the trigger will only fire when someone is facing the direction of the angle.  Use "360" for an angle of 0.
sounds
1)      secret
2)      beep beep
3)      large switch
4)
*/
class CTriggerOnce : public CTriggerMultiple
{
public:
	void Spawn( void );
};

LINK_ENTITY_TO_CLASS( trigger_once, CTriggerOnce );
void CTriggerOnce::Spawn( void )
{
	m_flWait = -1;
	
	CTriggerMultiple :: Spawn();
}



void CBaseTrigger :: MultiTouch( CBaseEntity *pOther )
{
	entvars_t	*pevToucher;

	pevToucher = pOther->pev;

	// Only touch clients, monsters, or pushables (depending on flags)
	if ( ((pevToucher->flags & FL_CLIENT) && !(pev->spawnflags & SF_TRIGGER_NOCLIENTS)) ||
		 ((pevToucher->flags & FL_MONSTER) && (pev->spawnflags & SF_TRIGGER_ALLOWMONSTERS)) ||
		 (pev->spawnflags & SF_TRIGGER_PUSHABLES) && FClassnameIs(pevToucher,"func_pushable") )
	{

#if 0
		// if the trigger has an angles field, check player's facing direction
		if (pev->movedir != g_vecZero)
		{
			UTIL_MakeVectors( pevToucher->angles );
			if ( DotProduct( gpGlobals->v_forward, pev->movedir ) < 0 )
				return;         // not facing the right way
		}
#endif
		
		ActivateMultiTrigger( pOther );
	}
}


//
// the trigger was just touched/killed/used
// self.enemy should be set to the activator so it can be held through a delay
// so wait for the delay time before firing
//
void CBaseTrigger :: ActivateMultiTrigger( CBaseEntity *pActivator )
{
	if (pev->nextthink > gpGlobals->time)
		return;         // still waiting for reset time

	if (!UTIL_IsMasterTriggered(m_sMaster,pActivator))
		return;

	if (FClassnameIs(pev, "trigger_secret"))
	{
		if ( pev->enemy == NULL || !FClassnameIs(pev->enemy, "player"))
			return;
		gpGlobals->found_secrets++;
	}

	if (!FStringNull(pev->noise))
		EMIT_SOUND(ENT(pev), CHAN_VOICE, (char*)STRING(pev->noise), 1, ATTN_NORM);

// don't trigger again until reset
// pev->takedamage = DAMAGE_NO;

	m_hActivator = pActivator;
	SUB_UseTargets( m_hActivator, USE_TOGGLE, 0 );

	if ( pev->message && pActivator->IsPlayer() )
	{
		UTIL_ShowMessage( STRING(pev->message), pActivator );
//		CLIENT_PRINTF( ENT( pActivator->pev ), print_center, STRING(pev->message) );
	}

	if (m_flWait > 0)
	{
		SetThink( MultiWaitOver );
		pev->nextthink = gpGlobals->time + m_flWait;
	}
	else
	{
		// we can't just remove (self) here, because this is a touch function
		// called while C code is looping through area links...
		SetTouch( NULL );
		pev->nextthink = gpGlobals->time + 0.1;
		SetThink(  SUB_Remove );
	}
}


// the wait time has passed, so set back up for another activation
void CBaseTrigger :: MultiWaitOver( void )
{
//	if (pev->max_health)
//		{
//		pev->health		= pev->max_health;
//		pev->takedamage	= DAMAGE_YES;
//		pev->solid		= SOLID_BBOX;
//		}
	SetThink( NULL );
}


// ========================= COUNTING TRIGGER =====================================

//
// GLOBALS ASSUMED SET:  g_eoActivator
//
void CBaseTrigger::CounterUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	// POV: the player is always the activator
	CBaseEntity *pPlayer = UTIL_FindEntityByClassname( NULL, "player" );
	if ( pPlayer )
	{
		m_cTriggersLeft--;
		m_hActivator = pPlayer;

		if (m_cTriggersLeft < 0)
			return;
		
		BOOL fTellActivator =
			(FClassnameIs(m_hActivator->pev, "player") &&
			!FBitSet(pev->spawnflags, SPAWNFLAG_NOMESSAGE));
		if (m_cTriggersLeft != 0)
		{
			if (fTellActivator)
			{
				// UNDONE: I don't think we want these Quakesque messages
				switch (m_cTriggersLeft)
				{
				case 1:		ALERT(at_console, "Only 1 more to go...");		break;
				case 2:		ALERT(at_console, "Only 2 more to go...");		break;
				case 3:		ALERT(at_console, "Only 3 more to go...");		break;
				default:	ALERT(at_console, "There are more to go...");	break;
				}
			}
			return;
		}

		// !!!UNDONE: I don't think we want these Quakesque messages
		if (fTellActivator)
			ALERT(at_console, "Sequence completed!");
		
		ActivateMultiTrigger( m_hActivator );
	}
	else
	{
		ALERT( at_console, "CounterUse: ERROR - couldn't find player!!\n" );
	}
}


/*QUAKED trigger_counter (.5 .5 .5) ? nomessage
Acts as an intermediary for an action that takes multiple inputs.
If nomessage is not set, it will print "1 more.. " etc when triggered and
"sequence complete" when finished.  After the counter has been triggered "cTriggersLeft"
times (default 2), it will fire all of it's targets and remove itself.
*/
class CTriggerCounter : public CBaseTrigger
{
public:
	void Spawn( void );
};
LINK_ENTITY_TO_CLASS( trigger_counter, CTriggerCounter );

void CTriggerCounter :: Spawn( void )
{
	// By making the flWait be -1, this counter-trigger will disappear after it's activated
	// (but of course it needs cTriggersLeft "uses" before that happens).
	m_flWait = -1;

	if (m_cTriggersLeft == 0)
		m_cTriggersLeft = 2;
	SetUse( CounterUse );
}

// ====================== TRIGGER_CHANGELEVEL ================================

class CTriggerVolume : public CPointEntity	// Derive from point entity so this doesn't move across levels
{
public:
	void		Spawn( void );
};

LINK_ENTITY_TO_CLASS( trigger_transition, CTriggerVolume );

// Define space that travels across a level transition
void CTriggerVolume :: Spawn( void )
{
	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_NONE;
	SET_MODEL(ENT(pev), STRING(pev->model));    // set size and link into world
	pev->model = NULL;
	pev->modelindex = 0;
}


// Fires a target after level transition and then dies
class CFireAndDie : public CBaseDelay
{
public:
	void Spawn( void );
	void Precache( void );
	void Think( void );
	int ObjectCaps( void ) { return CBaseDelay::ObjectCaps() | FCAP_FORCE_TRANSITION; }	// Always go across transitions
};
LINK_ENTITY_TO_CLASS( fireanddie, CFireAndDie );

void CFireAndDie::Spawn( void )
{
	pev->classname = MAKE_STRING("fireanddie");
	// Don't call Precache() - it should be called on restore
}


void CFireAndDie::Precache( void )
{
	// This gets called on restore
	pev->nextthink = gpGlobals->time + m_flDelay;
}


void CFireAndDie::Think( void )
{
	SUB_UseTargets( this, USE_TOGGLE, 0 );
	UTIL_Remove( this );
}


#define SF_CHANGELEVEL_USEONLY		0x0002
class CChangeLevel : public CBaseTrigger
{
public:
	void Spawn( void );
	void KeyValue( KeyValueData *pkvd );
	void EXPORT UseChangeLevel ( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void EXPORT TriggerChangeLevel( void );
	void EXPORT ExecuteChangeLevel( void );
	void EXPORT TouchChangeLevel( CBaseEntity *pOther );
	void ChangeLevelNow( CBaseEntity *pActivator );

	static edict_t *FindLandmark( const char *pLandmarkName );
	static int ChangeList( LEVELLIST *pLevelList, int maxList );
	static int AddTransitionToList( LEVELLIST *pLevelList, int listCount, const char *pMapName, const char *pLandmarkName, edict_t *pentLandmark );
	static int InTransitionVolume( CBaseEntity *pEntity, char *pVolumeName );

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );

	static	TYPEDESCRIPTION m_SaveData[];

	char m_szMapName[cchMapNameMost];		// trigger_changelevel only:  next map
	char m_szLandmarkName[cchMapNameMost];		// trigger_changelevel only:  landmark on next map
	int		m_changeTarget;
	float	m_changeTargetDelay;
};
LINK_ENTITY_TO_CLASS( trigger_changelevel, CChangeLevel );

// Global Savedata for changelevel trigger
TYPEDESCRIPTION	CChangeLevel::m_SaveData[] = 
{
	DEFINE_ARRAY( CChangeLevel, m_szMapName, FIELD_CHARACTER, cchMapNameMost ),
	DEFINE_ARRAY( CChangeLevel, m_szLandmarkName, FIELD_CHARACTER, cchMapNameMost ),
	DEFINE_FIELD( CChangeLevel, m_changeTarget, FIELD_STRING ),
	DEFINE_FIELD( CChangeLevel, m_changeTargetDelay, FIELD_FLOAT ),
};

IMPLEMENT_SAVERESTORE(CChangeLevel,CBaseTrigger);

//
// Cache user-entity-field values until spawn is called.
//

void CChangeLevel :: KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "map"))
	{
		if (strlen(pkvd->szValue) >= cchMapNameMost)
			ALERT( at_error, "Map name '%s' too long (32 chars)\n", pkvd->szValue );
		strcpy(m_szMapName, pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "landmark"))
	{
		if (strlen(pkvd->szValue) >= cchMapNameMost)
			ALERT( at_error, "Landmark name '%s' too long (32 chars)\n", pkvd->szValue );
		strcpy(m_szLandmarkName, pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "changetarget"))
	{
		m_changeTarget = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "changedelay"))
	{
		m_changeTargetDelay = atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		CBaseTrigger::KeyValue( pkvd );
}


/*QUAKED trigger_changelevel (0.5 0.5 0.5) ? NO_INTERMISSION
When the player touches this, he gets sent to the map listed in the "map" variable.  Unless the NO_INTERMISSION flag is set, the view will go to the info_intermission spot and display stats.
*/

void CChangeLevel :: Spawn( void )
{
	if ( FStrEq( m_szMapName, "" ) )
		ALERT( at_console, "a trigger_changelevel doesn't have a map" );

	if ( FStrEq( m_szLandmarkName, "" ) )
		ALERT( at_console, "trigger_changelevel to %s doesn't have a landmark", m_szMapName );

	if (!FStringNull ( pev->targetname ) )
	{
		SetUse ( UseChangeLevel );
	}
	InitTrigger();
	if ( !(pev->spawnflags & SF_CHANGELEVEL_USEONLY) )
		SetTouch( TouchChangeLevel );
//	ALERT( at_console, "TRANSITION: %s (%s)\n", m_szMapName, m_szLandmarkName );
}


void CChangeLevel :: ExecuteChangeLevel( void )
{
	MESSAGE_BEGIN( MSG_ALL, SVC_CDTRACK );
		WRITE_BYTE( 3 );
		WRITE_BYTE( 3 );
	MESSAGE_END();

	MESSAGE_BEGIN(MSG_ALL, SVC_INTERMISSION);
	MESSAGE_END();
}


FILE_GLOBAL char st_szNextMap[cchMapNameMost];
FILE_GLOBAL char st_szNextSpot[cchMapNameMost];

edict_t *CChangeLevel :: FindLandmark( const char *pLandmarkName )
{
	edict_t	*pentLandmark;

	pentLandmark = FIND_ENTITY_BY_STRING( NULL, "targetname", pLandmarkName );
	while ( !FNullEnt( pentLandmark ) )
	{
		// Found the landmark
		if ( FClassnameIs( pentLandmark, "info_landmark" ) )
			return pentLandmark;
		else
			pentLandmark = FIND_ENTITY_BY_STRING( pentLandmark, "targetname", pLandmarkName );
	}
	ALERT( at_error, "Can't find landmark %s\n", pLandmarkName );
	return NULL;
}


//=========================================================
// CChangeLevel :: Use - allows level transitions to be 
// triggered by buttons, etc.
//
//=========================================================
void CChangeLevel :: UseChangeLevel ( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	ChangeLevelNow( pActivator );
}

void CChangeLevel :: ChangeLevelNow( CBaseEntity *pActivator )
{
	edict_t	*pentLandmark;
	LEVELLIST	levels[16];

	ASSERT(!FStrEq(m_szMapName, ""));

	// Don't work in deathmatch
	if ( g_pGameRules->IsDeathmatch() )
		return;

	// Some people are firing these multiple times in a frame, disable
	if ( gpGlobals->time == pev->dmgtime )
		return;

	pev->dmgtime = gpGlobals->time;


	CBaseEntity *pPlayer = CBaseEntity::Instance( g_engfuncs.pfnPEntityOfEntIndex( 1 ) );
	if ( !InTransitionVolume( pPlayer, m_szLandmarkName ) )
	{
		ALERT( at_aiconsole, "Player isn't in the transition volume %s, aborting\n", m_szLandmarkName );
		return;
	}

	// POV: the next level is loading; close the journal
	g_fLevelLoaded = TRUE;

	CBasePlayer *pClient = (CBasePlayer *)UTIL_FindEntityByClassname( NULL, "player" );	// name ours
	if ( pClient )
	{
		if ( pClient->m_fStoryShown == TRUE )
			pClient->CloseStory();
	}
	else
		ALERT( at_console, "ChangeLevelNow: ERROR - couldn't find player!\n" );

	// Create an entity to fire the changetarget
	if ( m_changeTarget )
	{
		CFireAndDie *pFireAndDie = GetClassPtr( (CFireAndDie *)NULL );
		if ( pFireAndDie )
		{
			// Set target and delay
			pFireAndDie->pev->target = m_changeTarget;
			pFireAndDie->m_flDelay = m_changeTargetDelay;
			pFireAndDie->pev->origin = pPlayer->pev->origin;
			// Call spawn
			DispatchSpawn( pFireAndDie->edict() );
		}
	}
	// This object will get removed in the call to CHANGE_LEVEL, copy the params into "safe" memory
	strcpy(st_szNextMap, m_szMapName);

	m_hActivator = pActivator;
	SUB_UseTargets( pActivator, USE_TOGGLE, 0 );
	st_szNextSpot[0] = 0;	// Init landmark to NULL

	// look for a landmark entity		
	pentLandmark = FindLandmark( m_szLandmarkName );
	if ( !FNullEnt( pentLandmark ) )
	{
		strcpy(st_szNextSpot, m_szLandmarkName);
		gpGlobals->vecLandmarkOffset = VARS(pentLandmark)->origin;
	}
//	ALERT( at_console, "Level touches %d levels\n", ChangeList( levels, 16 ) );
	ALERT( at_console, "CHANGE LEVEL: %s %s\n", st_szNextMap, st_szNextSpot );
	CHANGE_LEVEL( st_szNextMap, st_szNextSpot );
}

//
// GLOBALS ASSUMED SET:  st_szNextMap
//
void CChangeLevel :: TouchChangeLevel( CBaseEntity *pOther )
{
	if (!FClassnameIs(pOther->pev, "player"))
		return;

	ChangeLevelNow( pOther );
}


// Add a transition to the list, but ignore duplicates 
// (a designer may have placed multiple trigger_changelevels with the same landmark)
int CChangeLevel::AddTransitionToList( LEVELLIST *pLevelList, int listCount, const char *pMapName, const char *pLandmarkName, edict_t *pentLandmark )
{
	int i;

	if ( !pLevelList || !pMapName || !pLandmarkName || !pentLandmark )
		return 0;

	for ( i = 0; i < listCount; i++ )
	{
		if ( pLevelList[i].pentLandmark == pentLandmark && strcmp( pLevelList[i].mapName, pMapName ) == 0 )
			return 0;
	}
	strcpy( pLevelList[listCount].mapName, pMapName );
	strcpy( pLevelList[listCount].landmarkName, pLandmarkName );
	pLevelList[listCount].pentLandmark = pentLandmark;
	pLevelList[listCount].vecLandmarkOrigin = VARS(pentLandmark)->origin;

	return 1;
}

int BuildChangeList( LEVELLIST *pLevelList, int maxList )
{
	return CChangeLevel::ChangeList( pLevelList, maxList );
}


int CChangeLevel::InTransitionVolume( CBaseEntity *pEntity, char *pVolumeName )
{
	edict_t	*pentVolume;


	if ( pEntity->ObjectCaps() & FCAP_FORCE_TRANSITION )
		return 1;

	// If you're following another entity, follow it through the transition (weapons follow the player)
	if ( pEntity->pev->movetype == MOVETYPE_FOLLOW )
	{
		if ( pEntity->pev->aiment != NULL )
			pEntity = CBaseEntity::Instance( pEntity->pev->aiment );
	}

	int inVolume = 1;	// Unless we find a trigger_transition, everything is in the volume

	pentVolume = FIND_ENTITY_BY_TARGETNAME( NULL, pVolumeName );
	while ( !FNullEnt( pentVolume ) )
	{
		CBaseEntity *pVolume = CBaseEntity::Instance( pentVolume );
		
		if ( pVolume && FClassnameIs( pVolume->pev, "trigger_transition" ) )
		{
			if ( pVolume->Intersects( pEntity ) )	// It touches one, it's in the volume
				return 1;
			else
				inVolume = 0;	// Found a trigger_transition, but I don't intersect it -- if I don't find another, don't go!
		}
		pentVolume = FIND_ENTITY_BY_TARGETNAME( pentVolume, pVolumeName );
	}

	return inVolume;
}


// We can only ever move 512 entities across a transition
#define MAX_ENTITY 512

// This has grown into a complicated beast
// Can we make this more elegant?
// This builds the list of all transitions on this level and which entities are in their PVS's and can / should
// be moved across.
int CChangeLevel::ChangeList( LEVELLIST *pLevelList, int maxList )
{
	edict_t	*pentChangelevel, *pentLandmark;
	int			i, count;

	count = 0;

	// Find all of the possible level changes on this BSP
	pentChangelevel = FIND_ENTITY_BY_STRING( NULL, "classname", "trigger_changelevel" );
	if ( FNullEnt( pentChangelevel ) )
		return 0;
	while ( !FNullEnt( pentChangelevel ) )
	{
		CChangeLevel *pTrigger;
		
		pTrigger = GetClassPtr((CChangeLevel *)VARS(pentChangelevel));
		if ( pTrigger )
		{
			// Find the corresponding landmark
			pentLandmark = FindLandmark( pTrigger->m_szLandmarkName );
			if ( pentLandmark )
			{
				// Build a list of unique transitions
				if ( AddTransitionToList( pLevelList, count, pTrigger->m_szMapName, pTrigger->m_szLandmarkName, pentLandmark ) )
				{
					count++;
					if ( count >= maxList )		// FULL!!
						break;
				}
			}
		}
		pentChangelevel = FIND_ENTITY_BY_STRING( pentChangelevel, "classname", "trigger_changelevel" );
	}

	if ( gpGlobals->pSaveData && ((SAVERESTOREDATA *)gpGlobals->pSaveData)->pTable )
	{
		CSave saveHelper( (SAVERESTOREDATA *)gpGlobals->pSaveData );

		for ( i = 0; i < count; i++ )
		{
			int j, entityCount = 0;
			CBaseEntity *pEntList[ MAX_ENTITY ];
			int			 entityFlags[ MAX_ENTITY ];

			// Follow the linked list of entities in the PVS of the transition landmark
			edict_t *pent = UTIL_EntitiesInPVS( pLevelList[i].pentLandmark );

			// Build a list of valid entities in this linked list (we're going to use pent->v.chain again)
			while ( !FNullEnt( pent ) )
			{
				CBaseEntity *pEntity = CBaseEntity::Instance(pent);
				if ( pEntity )
				{
//					ALERT( at_console, "Trying %s\n", STRING(pEntity->pev->classname) );
					int caps = pEntity->ObjectCaps();
					if ( !(caps & FCAP_DONT_SAVE) )
					{
						int flags = 0;

						// If this entity can be moved or is global, mark it
						if ( caps & FCAP_ACROSS_TRANSITION )
							flags |= FENTTABLE_MOVEABLE;
						if ( pEntity->pev->globalname && !pEntity->IsDormant() )
							flags |= FENTTABLE_GLOBAL;
						if ( flags )
						{
							pEntList[ entityCount ] = pEntity;
							entityFlags[ entityCount ] = flags;
							entityCount++;
							if ( entityCount > MAX_ENTITY )
								ALERT( at_error, "Too many entities across a transition!" );
						}
//						else
//							ALERT( at_console, "Failed %s\n", STRING(pEntity->pev->classname) );
					}
//					else
//						ALERT( at_console, "DON'T SAVE %s\n", STRING(pEntity->pev->classname) );
				}
				pent = pent->v.chain;
			}

			for ( j = 0; j < entityCount; j++ )
			{
				// Check to make sure the entity isn't screened out by a trigger_transition
				if ( entityFlags[j] && InTransitionVolume( pEntList[j], pLevelList[i].landmarkName ) )
				{
					// Mark entity table with 1<<i
					int index = saveHelper.EntityIndex( pEntList[j] );
					// Flag it with the level number
					saveHelper.EntityFlagsSet( index, entityFlags[j] | (1<<i) );
				}
//				else
//					ALERT( at_console, "Screened out %s\n", STRING(pEntList[j]->pev->classname) );

			}
		}
	}

	return count;
}

/*
go to the next level for deathmatch
only called if a time or frag limit has expired
*/
void NextLevel( void )
{
	edict_t* pent;
	CChangeLevel *pChange;
	
	// find a trigger_changelevel
	pent = FIND_ENTITY_BY_CLASSNAME(NULL, "trigger_changelevel");
	
	// go back to start if no trigger_changelevel
	if (FNullEnt(pent))
	{
		gpGlobals->mapname = ALLOC_STRING("start");
		pChange = GetClassPtr( (CChangeLevel *)NULL );
		strcpy(pChange->m_szMapName, "start");
	}
	else
		pChange = GetClassPtr( (CChangeLevel *)VARS(pent));
	
	strcpy(st_szNextMap, pChange->m_szMapName);
	g_fGameOver = TRUE;
	
	if (pChange->pev->nextthink < gpGlobals->time)
	{
		pChange->SetThink( CChangeLevel::ExecuteChangeLevel );
		pChange->pev->nextthink = gpGlobals->time + 0.1;
	}
}


// ============================== LADDER =======================================

class CLadder : public CBaseTrigger
{
public:
	void KeyValue( KeyValueData *pkvd );
	void Spawn( void );
	void Precache( void );
};
LINK_ENTITY_TO_CLASS( func_ladder, CLadder );


void CLadder :: KeyValue( KeyValueData *pkvd )
{
	CBaseTrigger::KeyValue( pkvd );
}


//=========================================================
// func_ladder - makes an area vertically negotiable
//=========================================================
void CLadder :: Precache( void )
{
	// Do all of this in here because we need to 'convert' old saved games
	pev->solid = SOLID_NOT;
	pev->skin = CONTENTS_LADDER;
	if ( CVAR_GET_FLOAT("showtriggers") == 0 )
	{
		pev->rendermode = kRenderTransTexture;
		pev->renderamt = 0;
	}
	pev->effects &= ~EF_NODRAW;
}


void CLadder :: Spawn( void )
{
	Precache();

	SET_MODEL(ENT(pev), STRING(pev->model));    // set size and link into world
	pev->movetype = MOVETYPE_PUSH;
}


// ========================== A TRIGGER THAT PUSHES YOU ===============================

class CTriggerPush : public CBaseTrigger
{
public:
	void Spawn( void );
	void KeyValue( KeyValueData *pkvd );
	void Touch( CBaseEntity *pOther );
};
LINK_ENTITY_TO_CLASS( trigger_push, CTriggerPush );


void CTriggerPush :: KeyValue( KeyValueData *pkvd )
{
	CBaseTrigger::KeyValue( pkvd );
}


/*QUAKED trigger_push (.5 .5 .5) ? TRIG_PUSH_ONCE
Pushes the player
*/

void CTriggerPush :: Spawn( )
{
	if ( pev->angles == g_vecZero )
		pev->angles.y = 360;
	InitTrigger();

	if (pev->speed == 0)
		pev->speed = 100;

	if ( FBitSet (pev->spawnflags, SF_TRIGGER_PUSH_START_OFF) )// if flagged to Start Turned Off, make trigger nonsolid.
		pev->solid = SOLID_NOT;

	SetUse( ToggleUse );

	UTIL_SetOrigin( pev, pev->origin );		// Link into the list
}


void CTriggerPush :: Touch( CBaseEntity *pOther )
{
	entvars_t* pevToucher = pOther->pev;

	// UNDONE: Is there a better way than health to detect things that have physics? (clients/monsters)
	switch( pevToucher->movetype )
	{
	case MOVETYPE_NONE:
	case MOVETYPE_PUSH:
	case MOVETYPE_NOCLIP:
	case MOVETYPE_FOLLOW:
		return;
	}

	if ( pevToucher->solid != SOLID_NOT && pevToucher->solid != SOLID_BSP )
	{
		// Instant trigger, just transfer velocity and remove
		if (FBitSet(pev->spawnflags, SF_TRIG_PUSH_ONCE))
		{
			pevToucher->velocity = pevToucher->velocity + (pev->speed * pev->movedir);
			if ( pevToucher->velocity.z > 0 )
				pevToucher->flags &= ~FL_ONGROUND;
			UTIL_Remove( this );
		}
		else
		{	// Push field, transfer to base velocity
			Vector vecPush = (pev->speed * pev->movedir);
			if ( pevToucher->flags & FL_BASEVELOCITY )
				vecPush = vecPush +  pevToucher->basevelocity;

			pevToucher->basevelocity = vecPush;

			pevToucher->flags |= FL_BASEVELOCITY;
//			ALERT( at_console, "Vel %f, base %f\n", pevToucher->velocity.z, pevToucher->basevelocity.z );
		}
	}
}


//======================================
// teleport trigger
//
//

void CBaseTrigger :: TeleportTouch( CBaseEntity *pOther )
{
	entvars_t* pevToucher = pOther->pev;
	edict_t	*pentTarget = NULL;

	// Only teleport monsters or clients
	if ( !FBitSet( pevToucher->flags, FL_CLIENT|FL_MONSTER ) )
		return;
    
	if (!UTIL_IsMasterTriggered(m_sMaster, pOther))
		return;
 	
	if ( !( pev->spawnflags & SF_TRIGGER_ALLOWMONSTERS ) )
	{// no monsters allowed!
		if ( FBitSet( pevToucher->flags, FL_MONSTER ) )
		{
			return;
		}
	}

	if ( ( pev->spawnflags & SF_TRIGGER_NOCLIENTS ) )
	{// no clients allowed
		if ( pOther->IsPlayer() )
		{
			return;
		}
	}
	
	pentTarget = FIND_ENTITY_BY_TARGETNAME( pentTarget, STRING(pev->target) );
	if (FNullEnt(pentTarget))
	   return;	
	
	Vector tmp = VARS( pentTarget )->origin;

	if ( pOther->IsPlayer() )
	{
		tmp.z -= pOther->pev->mins.z;// make origin adjustments in case the teleportee is a player. (origin in center, not at feet)
	}

	tmp.z++;

	pevToucher->flags &= ~FL_ONGROUND;
	
	UTIL_SetOrigin( pevToucher, tmp );

	pevToucher->angles = pentTarget->v.angles;

	if ( pOther->IsPlayer() )
	{
		pevToucher->v_angle = pentTarget->v.angles;
	}

	pevToucher->fixangle = TRUE;
	pevToucher->velocity = pevToucher->basevelocity = g_vecZero;
}


class CTriggerTeleport : public CBaseTrigger
{
public:
	void Spawn( void );
};
LINK_ENTITY_TO_CLASS( trigger_teleport, CTriggerTeleport );

void CTriggerTeleport :: Spawn( void )
{
	InitTrigger();

	SetTouch( TeleportTouch );
}


LINK_ENTITY_TO_CLASS( info_teleport_destination, CPointEntity );



class CTriggerSave : public CBaseTrigger
{
public:
	void Spawn( void );
	void EXPORT SaveTouch( CBaseEntity *pOther );
};
LINK_ENTITY_TO_CLASS( trigger_autosave, CTriggerSave );

void CTriggerSave::Spawn( void )
{
	if ( g_pGameRules->IsDeathmatch() )
	{
		REMOVE_ENTITY( ENT(pev) );
		return;
	}

	InitTrigger();
	SetTouch( SaveTouch );
}

void CTriggerSave::SaveTouch( CBaseEntity *pOther )
{
	if ( !UTIL_IsMasterTriggered( m_sMaster, pOther ) )
		return;

	// Only save on clients
	if ( !pOther->IsPlayer() )
		return;
    
	SetTouch( NULL );
	UTIL_Remove( this );
	SERVER_COMMAND( "autosave\n" );
}

#define SF_ENDSECTION_USEONLY		0x0001

class CTriggerEndSection : public CBaseTrigger
{
public:
	void Spawn( void );
	void EXPORT EndSectionTouch( CBaseEntity *pOther );
	void KeyValue( KeyValueData *pkvd );
	void EXPORT EndSectionUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
};
LINK_ENTITY_TO_CLASS( trigger_endsection, CTriggerEndSection );


void CTriggerEndSection::EndSectionUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	SetUse( NULL );

	if ( pev->message )
	{
		g_engfuncs.pfnEndSection(STRING(pev->message));
	}
	UTIL_Remove( this );
}

void CTriggerEndSection::Spawn( void )
{
	if ( g_pGameRules->IsDeathmatch() )
	{
		REMOVE_ENTITY( ENT(pev) );
		return;
	}

	InitTrigger();

	SetUse ( EndSectionUse );
	// If it is a "use only" trigger, then don't set the touch function.
	if ( ! (pev->spawnflags & SF_ENDSECTION_USEONLY) )
		SetTouch( EndSectionTouch );
}

void CTriggerEndSection::EndSectionTouch( CBaseEntity *pOther )
{
	// Only save on clients
	if ( !pOther->IsNetClient() )
		return;
    
	SetTouch( NULL );

	if (pev->message)
	{
		g_engfuncs.pfnEndSection(STRING(pev->message));
	}
	UTIL_Remove( this );
}

void CTriggerEndSection :: KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "section"))
	{
//		m_iszSectionName = ALLOC_STRING( pkvd->szValue );
		// Store this in message so we don't have to write save/restore for this ent
		pev->message = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		CBaseTrigger::KeyValue( pkvd );
}


class CTriggerGravity : public CBaseTrigger
{
public:
	void Spawn( void );
	void EXPORT GravityTouch( CBaseEntity *pOther );
};
LINK_ENTITY_TO_CLASS( trigger_gravity, CTriggerGravity );

void CTriggerGravity::Spawn( void )
{
	InitTrigger();
	SetTouch( GravityTouch );
}

void CTriggerGravity::GravityTouch( CBaseEntity *pOther )
{
	// Only save on clients
	if ( !pOther->IsPlayer() )
		return;

	pOther->pev->gravity = pev->gravity;
}







// this is a really bad idea.
class CTriggerChangeTarget : public CBaseDelay
{
public:
	void KeyValue( KeyValueData *pkvd );
	void Spawn( void );
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

	int ObjectCaps( void ) { return CBaseDelay::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }
	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );

	static	TYPEDESCRIPTION m_SaveData[];

private:
	int		m_iszNewTarget;
};
LINK_ENTITY_TO_CLASS( trigger_changetarget, CTriggerChangeTarget );

TYPEDESCRIPTION	CTriggerChangeTarget::m_SaveData[] = 
{
	DEFINE_FIELD( CTriggerChangeTarget, m_iszNewTarget, FIELD_STRING ),
};

IMPLEMENT_SAVERESTORE(CTriggerChangeTarget,CBaseDelay);

void CTriggerChangeTarget::KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "m_iszNewTarget"))
	{
		m_iszNewTarget = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		CBaseDelay::KeyValue( pkvd );
}

void CTriggerChangeTarget::Spawn( void )
{
}


void CTriggerChangeTarget::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	CBaseEntity *pTarget = UTIL_FindEntityByString( NULL, "targetname", STRING( pev->target ) );

	if (pTarget)
	{
		pTarget->pev->target = m_iszNewTarget;
		CBaseMonster *pMonster = pTarget->MyMonsterPointer( );
		if (pMonster)
		{
			pMonster->m_pGoalEnt = NULL;
		}
	}
}




#define SF_CAMERA_PLAYER_POSITION	1
#define SF_CAMERA_PLAYER_TARGET		2
#define SF_CAMERA_PLAYER_TAKECONTROL 4

class CTriggerCamera : public CBaseDelay
{
public:
	void Spawn( void );
	void KeyValue( KeyValueData *pkvd );
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void EXPORT FollowTarget( void );
	void EXPORT WaitToLoadFinishThink( void );
	void Move(void);

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	virtual int	ObjectCaps( void ) { return CBaseEntity :: ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }
	static	TYPEDESCRIPTION m_SaveData[];

	EHANDLE m_hPlayer;
	EHANDLE m_hTarget;
	CBaseEntity *m_pentPath;
	int	  m_sPath;
	float m_flWait;
	float m_flReturnTime;
	float m_flStopTime;
	float m_moveDistance;
	float m_targetSpeed;
	float m_initialSpeed;
	float m_acceleration;
	float m_deceleration;
	float m_turnSpeed;		// POV
	int	  m_state;
	BOOL  m_bAlreadyFreezed;	// POV: the player was frozen already
	BOOL  m_bFistOnCamera;		// POV: no other camera was on (sic)
	BOOL  m_bLastOnCamera;		// POV: the camera turned on last
	
};
LINK_ENTITY_TO_CLASS( trigger_camera, CTriggerCamera );

// Global Savedata for changelevel friction modifier
TYPEDESCRIPTION	CTriggerCamera::m_SaveData[] = 
{
	DEFINE_FIELD( CTriggerCamera, m_hPlayer, FIELD_EHANDLE ),
	DEFINE_FIELD( CTriggerCamera, m_hTarget, FIELD_EHANDLE ),
	DEFINE_FIELD( CTriggerCamera, m_pentPath, FIELD_CLASSPTR ),
	DEFINE_FIELD( CTriggerCamera, m_sPath, FIELD_STRING ),
	DEFINE_FIELD( CTriggerCamera, m_flWait, FIELD_FLOAT ),
	DEFINE_FIELD( CTriggerCamera, m_flReturnTime, FIELD_TIME ),
	DEFINE_FIELD( CTriggerCamera, m_flStopTime, FIELD_TIME ),
	DEFINE_FIELD( CTriggerCamera, m_moveDistance, FIELD_FLOAT ),
	DEFINE_FIELD( CTriggerCamera, m_targetSpeed, FIELD_FLOAT ),
	DEFINE_FIELD( CTriggerCamera, m_initialSpeed, FIELD_FLOAT ),
	DEFINE_FIELD( CTriggerCamera, m_acceleration, FIELD_FLOAT ),
	DEFINE_FIELD( CTriggerCamera, m_deceleration, FIELD_FLOAT ),
	DEFINE_FIELD( CTriggerCamera, m_state, FIELD_INTEGER ),
	DEFINE_FIELD( CTriggerCamera, m_bAlreadyFreezed, FIELD_BOOLEAN ),
	DEFINE_FIELD( CTriggerCamera, m_bFistOnCamera, FIELD_BOOLEAN ),
	DEFINE_FIELD( CTriggerCamera, m_bLastOnCamera, FIELD_BOOLEAN ),
	DEFINE_FIELD( CTriggerCamera, m_turnSpeed, FIELD_FLOAT ),
};

int CTriggerCamera::Restore( CRestore &restore )
{
	if ( !CBaseEntity::Restore(restore) )
		return 0;

	int status = restore.ReadFields( "CTriggerCamera", this, m_SaveData, ARRAYSIZE(m_SaveData) );

	// POV: the camera that was on last takes the view again once the level has loaded
	if ( m_state == 1 && m_bLastOnCamera == 1 )
	{
		SetThink( WaitToLoadFinishThink );
		pev->nextthink = gpGlobals->time + 0.1;
	}

	return status;
}

int CTriggerCamera::Save( CSave &save )
{
	if ( !CBaseEntity::Save(save) )
		return 0;
	return save.WriteFields( "CTriggerCamera", this, m_SaveData, ARRAYSIZE(m_SaveData) );
}

void CTriggerCamera::WaitToLoadFinishThink( void )
{
	if ( g_fLevelLoaded == TRUE )
	{
		pev->nextthink = gpGlobals->time + 0.1;
		return;
	}

	m_hPlayer = CBaseEntity::Instance( g_engfuncs.pfnPEntityOfEntIndex( 1 ) );

	SET_VIEW( m_hPlayer->edict(), edict() );

	SET_MODEL( ENT(pev), STRING(m_hPlayer->pev->model) );

	SetThink( FollowTarget );
	pev->nextthink = gpGlobals->time;

	Move();
}

void CTriggerCamera::Spawn( void )
{
	pev->movetype = MOVETYPE_NOCLIP;
	pev->solid = SOLID_NOT;							// Remove model & collisions
	pev->renderamt = 0;								// The engine won't draw this model if this is set to 0 and blending is on
	pev->rendermode = kRenderTransTexture;
	pev->angles = g_vecZero;

	m_initialSpeed = pev->speed;
	if ( m_acceleration == 0 )
		m_acceleration = 500;
	if ( m_deceleration == 0 )
		m_deceleration = 500;
	if ( m_turnSpeed == 0 )
		m_turnSpeed = 40;

	m_bAlreadyFreezed = FALSE;
	m_bFistOnCamera = FALSE;
	m_bLastOnCamera = FALSE;
}


void CTriggerCamera :: KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "wait"))
	{
		m_flWait = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "moveto"))
	{
		m_sPath = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "acceleration"))
	{
		m_acceleration = atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "deceleration"))
	{
		m_deceleration = atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "turnspeed"))
	{
		m_turnSpeed = atof( pkvd->szValue );
		if ( m_turnSpeed > 120 )
			m_turnSpeed = 120;
		pkvd->fHandled = TRUE;
	}
	else
		CBaseDelay::KeyValue( pkvd );
}



void CTriggerCamera::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if ( !ShouldToggle( useType, m_state ) )
		return;

	// Toggle state
	m_state = !m_state;
	if (m_state == 0)
	{
		// POV: the first camera turned off forgets which camera was on last
		if ( m_bFistOnCamera == TRUE )
		{
			CBaseEntity *pCamera = UTIL_FindEntityByClassname( NULL, "trigger_camera" );
			while ( pCamera )
			{
				((CTriggerCamera *)pCamera)->m_bLastOnCamera = FALSE;
				pCamera = UTIL_FindEntityByClassname( pCamera, "trigger_camera" );
			}
		}
		m_flReturnTime = gpGlobals->time;
		return;
	}

	// POV: remember which camera is the first one on and which the last one
	m_bFistOnCamera = TRUE;
	CBaseEntity *pCamera = UTIL_FindEntityByClassname( NULL, "trigger_camera" );
	while ( pCamera )
	{
		if ( pCamera != this )
		{
			if ( ((CTriggerCamera *)pCamera)->m_state == 1 )
				m_bFistOnCamera = FALSE;
			((CTriggerCamera *)pCamera)->m_bLastOnCamera = FALSE;
		}
		pCamera = UTIL_FindEntityByClassname( pCamera, "trigger_camera" );
	}
	m_bLastOnCamera = TRUE;

	if ( !pActivator || !pActivator->IsPlayer() )
	{
		pActivator = CBaseEntity::Instance(g_engfuncs.pfnPEntityOfEntIndex( 1 ));
	}
		
	m_hPlayer = pActivator;

	m_flReturnTime = gpGlobals->time + m_flWait;
	pev->speed = m_initialSpeed;
	m_targetSpeed = m_initialSpeed;

	if (FBitSet (pev->spawnflags, SF_CAMERA_PLAYER_TARGET ) )
	{
		m_hTarget = m_hPlayer;
	}
	else
	{
		m_hTarget = GetNextTarget();
	}

	// Nothing to look at!
	if ( m_hTarget == NULL )
	{
		return;
	}


	if (FBitSet (pev->spawnflags, SF_CAMERA_PLAYER_TAKECONTROL ) )
	{
		if ( FBitSet( ((CBaseEntity *)m_hPlayer)->pev->flags, FL_FROZEN ) )
			m_bAlreadyFreezed = TRUE;
		else
			((CBasePlayer *)((CBaseEntity *)m_hPlayer))->EnableControl(FALSE);
	}

	if ( m_sPath )
	{
		m_pentPath = Instance( FIND_ENTITY_BY_TARGETNAME ( NULL, STRING(m_sPath)) );
	}
	else
	{
		m_pentPath = NULL;
	}

	m_flStopTime = gpGlobals->time;
	if ( m_pentPath )
	{
		if ( m_pentPath->pev->speed != 0 )
			m_targetSpeed = m_pentPath->pev->speed;
		
		m_flStopTime += m_pentPath->GetDelay();
	}

	// POV: look at the target right away
	if ( m_hTarget != NULL )
	{
		pev->angles = UTIL_VecToAngles( m_hTarget->pev->origin - pev->origin );
		pev->angles.x = -pev->angles.x;
	}

	// copy over player information
	if (FBitSet (pev->spawnflags, SF_CAMERA_PLAYER_POSITION ) )
	{
		UTIL_SetOrigin( pev, pActivator->pev->origin + pActivator->pev->view_ofs );
		pev->angles.x = -pActivator->pev->angles.x;
		pev->angles.y = pActivator->pev->angles.y;
		pev->angles.z = 0;
		pev->velocity = pActivator->pev->velocity;
	}
	else
	{
		pev->velocity = Vector( 0, 0, 0 );
	}

	SET_VIEW( pActivator->edict(), edict() );

	SET_MODEL(ENT(pev), STRING(pActivator->pev->model) );

	// follow the player down
	SetThink( FollowTarget );
	pev->nextthink = gpGlobals->time;

	m_moveDistance = 0;
	Move();
}


void CTriggerCamera::FollowTarget( )
{
	if (m_hPlayer == NULL)
		return;

	if (m_hTarget == NULL || m_flReturnTime < gpGlobals->time)
	{
		if (m_hPlayer->IsAlive( ))
		{
			SET_VIEW( m_hPlayer->edict(), m_hPlayer->edict() );
			if ( !m_bAlreadyFreezed )
				((CBasePlayer *)((CBaseEntity *)m_hPlayer))->EnableControl(TRUE);
		}
		SUB_UseTargets( this, USE_TOGGLE, 0 );
		pev->avelocity = Vector( 0, 0, 0 );
		m_state = 0;
		return;
	}

	Vector vecGoal = UTIL_VecToAngles( m_hTarget->pev->origin - pev->origin );
	vecGoal.x = -vecGoal.x;

	if (pev->angles.y > 360)
		pev->angles.y -= 360;

	if (pev->angles.y < 0)
		pev->angles.y += 360;

	float dx = vecGoal.x - pev->angles.x;
	float dy = vecGoal.y - pev->angles.y;

	if (dx < -180) 
		dx += 360;
	if (dx > 180) 
		dx = dx - 360;
	
	if (dy < -180) 
		dy += 360;
	if (dy > 180) 
		dy = dy - 360;

	pev->avelocity.x = dx * m_turnSpeed * gpGlobals->frametime;
	pev->avelocity.y = dy * m_turnSpeed * gpGlobals->frametime;


	if (!(FBitSet (pev->spawnflags, SF_CAMERA_PLAYER_TAKECONTROL)))	
	{
		pev->velocity = pev->velocity * 0.8;
		if (pev->velocity.Length( ) < 10.0)
			pev->velocity = g_vecZero;
	}

	pev->nextthink = gpGlobals->time;

	Move();
}

void CTriggerCamera::Move()
{
	// Not moving on a path, return
	if (!m_pentPath)
		return;

	// Subtract movement from the previous frame
	m_moveDistance -= pev->speed * gpGlobals->frametime;

	// Have we moved enough to reach the target?
	if ( m_moveDistance <= 0 )
	{
		// Fire the passtarget if there is one
		if ( m_pentPath->pev->message )
		{
			FireTargets( STRING(m_pentPath->pev->message), this, this, USE_TOGGLE, 0 );
			if ( FBitSet( m_pentPath->pev->spawnflags, SF_CORNER_FIREONCE ) )
				m_pentPath->pev->message = 0;
		}
		// Time to go to the next target
		m_pentPath = m_pentPath->GetNextTarget();

		// Set up next corner
		if ( !m_pentPath )
		{
			pev->velocity = g_vecZero;
		}
		else 
		{
			if ( m_pentPath->pev->speed != 0 )
				m_targetSpeed = m_pentPath->pev->speed;

			Vector delta = m_pentPath->pev->origin - pev->origin;
			m_moveDistance = delta.Length();
			pev->movedir = delta.Normalize();
			m_flStopTime = gpGlobals->time + m_pentPath->GetDelay();
		}
	}

	if ( m_flStopTime > gpGlobals->time )
		pev->speed = UTIL_Approach( 0, pev->speed, m_deceleration * gpGlobals->frametime );
	else
		pev->speed = UTIL_Approach( m_targetSpeed, pev->speed, m_acceleration * gpGlobals->frametime );

	float fraction = 2 * gpGlobals->frametime;
	pev->velocity = ((pev->movedir * pev->speed) * fraction) + (pev->velocity * (1-fraction));
}


//=====================================
//
// POV: area_break - while the player stands in the area and looks at the
// breakent entity, the HUD shows the break icon (never on hard)
//
class CAreaBreak : public CBaseTrigger
{
public:
	void Spawn( void );
	void KeyValue( KeyValueData *pkvd );
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void EXPORT AreaBreakTouch( CBaseEntity *pOther );
	void EXPORT AreaBreakThink( void );
	BOOL IsLookingAt( CBasePlayer *pPlayer, CBaseEntity *pEntity );	// name ours
	void RemoveArea( void );	// name ours

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );

	static	TYPEDESCRIPTION m_SaveData[];

	int				m_iszBreakEntity;
	CBasePlayer		*m_pPlayer;
};

LINK_ENTITY_TO_CLASS( area_break, CAreaBreak );

TYPEDESCRIPTION	CAreaBreak::m_SaveData[] = 
{
	DEFINE_FIELD( CAreaBreak, m_iszBreakEntity, FIELD_STRING ),
	DEFINE_FIELD( CAreaBreak, m_pPlayer, FIELD_CLASSPTR ),
};

IMPLEMENT_SAVERESTORE(CAreaBreak,CBaseTrigger);

void CAreaBreak :: KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "breakent"))
	{
		m_iszBreakEntity = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		CBaseDelay::KeyValue( pkvd );
}

void CAreaBreak :: Spawn( void )
{
	InitTrigger();

	m_pPlayer = NULL;
	SetTouch( AreaBreakTouch );
}

void CAreaBreak :: AreaBreakTouch( CBaseEntity *pOther )
{
	if ( g_iSkillLevel != SKILL_HARD )
	{
		if ( !FBitSet( pOther->pev->flags, FL_CLIENT ) )
			return;

		int iCount = 0;
		CBaseEntity *pEntity = UTIL_FindEntityByTargetname( NULL, STRING(m_iszBreakEntity) );
		if ( pEntity )
		{
			while ( pEntity )
			{
				iCount++;
				pEntity = UTIL_FindEntityByTargetname( pEntity, STRING(m_iszBreakEntity) );
			}

			if ( iCount > 1 )
			{
				ALERT( at_console, "AreaBreak: ERROR - there can only be one BreakEntity %s!\n", STRING(m_iszBreakEntity) );
				RemoveArea();
				return;
			}

			m_pPlayer = (CBasePlayer *)CBaseEntity::Instance( pOther->pev );

			SetTouch( NULL );
			SetThink( AreaBreakThink );
			pev->nextthink = gpGlobals->time;
			return;
		}

		if ( !m_iszBreakEntity )
			ALERT( at_console, "AreaBreak: ERROR - couldn't find BreakEntity or not set!\n" );
	}

	RemoveArea();
}

void CAreaBreak :: AreaBreakThink( void )
{
	CBaseEntity *pList[3];

	int count = UTIL_EntitiesInBox( pList, 3, pev->absmin, pev->absmax, FL_CLIENT );
	if ( count > 1 )
	{
		ALERT( at_console, "AreaBreak: ERROR - more then one player found!\n" );
		return;
	}

	if ( count == 0 )
	{
		// the player left the area
		m_pPlayer->SetBreakIcon( 0 );
		SetTouch( AreaBreakTouch );
		SetThink( NULL );
		return;
	}

	CBaseEntity *pEntity = UTIL_FindEntityByTargetname( NULL, STRING(m_iszBreakEntity) );
	if ( !pEntity )
	{
		// it's broken
		RemoveArea();
		return;
	}

	int iState = IsLookingAt( m_pPlayer, pEntity );
	if ( iState != m_pPlayer->m_iBreakIcon )
		m_pPlayer->SetBreakIcon( iState );

	pev->nextthink = gpGlobals->time + 0.1;
}

BOOL CAreaBreak :: IsLookingAt( CBasePlayer *pPlayer, CBaseEntity *pEntity )
{
	TraceResult tr;

	UTIL_MakeVectors( pPlayer->pev->v_angle );
	Vector vecSrc = pPlayer->pev->origin;
	vecSrc.z += pPlayer->pev->size.z * 0.4;
	Vector vecEnd = vecSrc + gpGlobals->v_forward * 256;

	UTIL_TraceLine( vecSrc, vecEnd, ignore_monsters, ENT(pev), &tr );

	if ( tr.pHit )
	{
		CBaseEntity *pHit = CBaseEntity::Instance( tr.pHit );
		if ( pHit )
			return UTIL_StrIEq( STRING(pEntity->pev->targetname), STRING(pHit->pev->targetname) );

		ALERT( at_console, "AreaBreak: ERROR - couldn't Instance entity!\n" );
	}

	return FALSE;
}

void CAreaBreak::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	CBaseEntity *pPlayer = UTIL_FindEntityByClassname( NULL, "player" );

	if ( pPlayer )
		AreaBreakTouch( pPlayer );
	else
		ALERT( at_console, "AreaBreak: ERROR - player not found!!\n" );
}

void CAreaBreak :: RemoveArea( void )
{
	if ( m_pPlayer && m_pPlayer->m_iBreakIcon )
		m_pPlayer->SetBreakIcon( 0 );

	SetTouch( NULL );
	SetThink( SUB_Remove );
	pev->nextthink = gpGlobals->time;
}


//=====================================
//
// POV: area_story - marks a journal entry (storyid) found. By storystyle:
// 1 shows it at once, 4 only lights the story icon; 2 and 3 show it, 5
// and 6 light the icon, when the player looks at storyent (3 and 6: once
// it's dead)
//
#define SF_AREASTORY_USEONLY	0x0001	// name ours

class CAreaStory : public CBaseTrigger
{
public:
	void Spawn( void );
	void KeyValue( KeyValueData *pkvd );
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void EXPORT AreaStoryTouch( CBaseEntity *pOther );
	void StartStory( void );	// name ours
	void EXPORT AreaStoryThink( void );
	int IsStoryFound( CBasePlayer *pPlayer, CBaseEntity *pEntity, BOOL fNeedsDead );	// name ours
	int IsMonsterFaced( CBasePlayer *pPlayer, CBaseEntity *pEntity, BOOL fNeedsDead );	// name ours
	int IsEntityLookedAt( CBasePlayer *pPlayer, CBaseEntity *pEntity, BOOL fNeedsDead );	// name ours
	void RemoveArea( void );	// name ours

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );

	static	TYPEDESCRIPTION m_SaveData[];

	int				m_iszStoryEntity;
	int				m_iStyle;
	int				m_iStoryID;
	CBasePlayer		*m_pPlayer;
};

LINK_ENTITY_TO_CLASS( area_story, CAreaStory );

TYPEDESCRIPTION	CAreaStory::m_SaveData[] = 
{
	DEFINE_FIELD( CAreaStory, m_iszStoryEntity, FIELD_STRING ),
	DEFINE_FIELD( CAreaStory, m_iStyle, FIELD_INTEGER ),
	DEFINE_FIELD( CAreaStory, m_iStoryID, FIELD_INTEGER ),
	DEFINE_FIELD( CAreaStory, m_pPlayer, FIELD_CLASSPTR ),
};

IMPLEMENT_SAVERESTORE(CAreaStory,CBaseTrigger);

void CAreaStory :: Spawn( void )
{
	InitTrigger();

	if ( pev->spawnflags & SF_AREASTORY_USEONLY )
		SetTouch( NULL );
	else
		SetTouch( AreaStoryTouch );

	SetThink( NULL );
	m_pPlayer = NULL;
}

void CAreaStory :: KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "storystyle"))
	{
		m_iStyle = atoi( pkvd->szValue );
		if ( m_iStyle < 1 || m_iStyle > 6 )
		{
			ALERT( at_console, "CAreaStory: ERROR - invalid Style!\n" );
			m_iStyle = 1;
		}
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "storyent"))
	{
		m_iszStoryEntity = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "storyid"))
	{
		m_iStoryID = UTIL_StoryGetCodeStoryID( ALLOC_STRING( pkvd->szValue ) );
		pkvd->fHandled = TRUE;
	}
	else
		pkvd->fHandled = FALSE;
}

void CAreaStory::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if ( !m_pPlayer )
	{
		m_pPlayer = (CBasePlayer *)UTIL_FindEntityByClassname( NULL, "player" );
		if ( !m_pPlayer )
		{
			ALERT( at_console, "CAreaStory: ERROR - couldn't find player, removing!\n" );
			RemoveArea();
			return;
		}
	}

	StartStory();
}

void CAreaStory :: AreaStoryTouch( CBaseEntity *pOther )
{
	if ( !FBitSet( pOther->pev->flags, FL_CLIENT ) )
		return;

	if ( m_iStyle == 2 || m_iStyle == 3 || m_iStyle == 5 || m_iStyle == 6 )
	{
		// these need the story entity
		int iCount = 0;
		CBaseEntity *pEntity = UTIL_FindEntityByTargetname( NULL, STRING(m_iszStoryEntity) );
		if ( pEntity )
		{
			while ( pEntity )
			{
				iCount++;
				pEntity = UTIL_FindEntityByTargetname( pEntity, STRING(m_iszStoryEntity) );
			}

			if ( iCount > 1 )
			{
				ALERT( at_console, "AreaStory: ERROR - there can only be one StoryEntity %s!\n", STRING(m_iszStoryEntity) );
				RemoveArea();
				return;
			}
		}
		else
		{
			if ( !m_iszStoryEntity )
				ALERT( at_console, "AreaStory: ERROR - couldn't find StoryEntity or not set!\n" );
			RemoveArea();
			return;
		}
	}

	m_pPlayer = (CBasePlayer *)CBaseEntity::Instance( pOther->pev );
	if ( !m_pPlayer )
	{
		ALERT( at_console, "CAreaStory: ERROR - couldn't instance player, removing!\n" );
		RemoveArea();
		return;
	}

	StartStory();
}

void CAreaStory :: StartStory( void )
{
	switch ( m_iStyle )
	{
	case 1:
		m_pPlayer->m_fStoryNavigate = FALSE;
		m_pPlayer->ShowStory( m_iStoryID );
		RemoveArea();
		break;

	case 4:
		m_pPlayer->SetStoryIcon( 1, m_iStoryID );
		RemoveArea();
		break;

	case 2:
	case 3:
	case 5:
	case 6:
		SetTouch( NULL );
		SetThink( AreaStoryThink );
		pev->nextthink = gpGlobals->time;
		break;
	}
}

void CAreaStory :: AreaStoryThink( void )
{
	CBaseEntity *pList[3];

	int count = UTIL_EntitiesInBox( pList, 3, pev->absmin, pev->absmax, FL_CLIENT );
	if ( count > 1 )
	{
		ALERT( at_console, "CAreaStory: ERROR - more then one player found!\n" );
		return;
	}

	if ( count == 0 )
	{
		// the player left the area
		UTIL_StoryUpdateIcon( m_pPlayer );

		if ( pev->spawnflags & SF_AREASTORY_USEONLY )
			SetTouch( NULL );
		else
			SetTouch( AreaStoryTouch );

		SetThink( NULL );
		return;
	}

	CBaseEntity *pEntity = UTIL_FindEntityByTargetname( NULL, STRING(m_iszStoryEntity) );
	if ( !pEntity )
	{
		RemoveArea();
		return;
	}

	if ( m_iStyle == 2 || m_iStyle == 3 )
	{
		int iFound;
		if ( m_iStyle == 2 )
			iFound = IsStoryFound( m_pPlayer, pEntity, FALSE );
		else
			iFound = IsStoryFound( m_pPlayer, pEntity, TRUE );

		if ( iFound == 1 )
		{
			m_pPlayer->m_fStoryNavigate = FALSE;
			m_pPlayer->ShowStory( m_iStoryID );
			RemoveArea();
			return;
		}
	}
	else
	{
		int iFound;
		if ( m_iStyle == 5 )
			iFound = IsStoryFound( m_pPlayer, pEntity, FALSE );
		else
			iFound = IsStoryFound( m_pPlayer, pEntity, TRUE );

		if ( iFound == 1 )
		{
			m_pPlayer->SetStoryIcon( 1, m_iStoryID );
			RemoveArea();
			return;
		}
	}

	pev->nextthink = gpGlobals->time + 0.1;
}

int CAreaStory :: IsStoryFound( CBasePlayer *pPlayer, CBaseEntity *pEntity, BOOL fNeedsDead )
{
	if ( FBitSet( pEntity->pev->flags, FL_MONSTER ) )
		return IsMonsterFaced( pPlayer, pEntity, fNeedsDead );

	return IsEntityLookedAt( pPlayer, pEntity, fNeedsDead );
}

int CAreaStory :: IsMonsterFaced( CBasePlayer *pPlayer, CBaseEntity *pEntity, BOOL fNeedsDead )
{
	int iFound = IsFacing( pPlayer->pev, pEntity->pev->origin );
	if ( iFound == 1 && fNeedsDead == 1 && pEntity->pev->deadflag != DEAD_DEAD )
		iFound = 0;

	return iFound;
}

int CAreaStory :: IsEntityLookedAt( CBasePlayer *pPlayer, CBaseEntity *pEntity, BOOL fNeedsDead )
{
	TraceResult tr;
	int iFound = 0;

	if ( fNeedsDead == 1 )
		ALERT( at_console, "CAreaStory: WARNING - NeedsToBeDead on entity\n" );

	UTIL_MakeVectors( pPlayer->pev->v_angle );
	Vector vecSrc = pPlayer->pev->origin;
	vecSrc.z += pPlayer->pev->size.z * 0.4;
	Vector vecEnd = vecSrc + gpGlobals->v_forward * 1024;

	UTIL_TraceLine( vecSrc, vecEnd, ignore_monsters, ENT(pev), &tr );

	if ( tr.pHit )
	{
		CBaseEntity *pHit = CBaseEntity::Instance( tr.pHit );
		if ( pHit )
			iFound = UTIL_StrIEq( STRING(pEntity->pev->targetname), STRING(pHit->pev->targetname) );
		else
			ALERT( at_console, "CAreaStory: ERROR - couldn't Instance entity!\n" );
	}

	return iFound;
}

void CAreaStory :: RemoveArea( void )
{
	SetTouch( NULL );
	SetThink( SUB_Remove );
	pev->nextthink = gpGlobals->time;
}

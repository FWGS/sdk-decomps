/***
*
*	Scientist Slaughterhouse: master entities.
*	The file name and the names marked "ours" are ours; class, field and
*	entity names come from the binary (save tables, exports).
*
****/
/*

===== masters.cpp ========================================================

  master_watcher (CWatchMaster) and master_ident (CIDMaster): masters
  (the "master" keyvalue of other entities) that are on or off by the
  state of other entities, or by who activates them.

*/

#include "extdll.h"
#include "util.h"
#include "cbase.h"


#define MAX_WATCH_TARGETS			16	// name ours

#define SF_WATCH_DEFAULTON			1	// fgd "Default on"
#define SF_WATCH_OR					2	// fgd "Logical OR": on as soon as one watched entity agrees

//
// master_watcher: on when the entities it watches are on. Every keyvalue
// that isn't "globalstate" names an entity to watch (UTIL_StripToken, as
// multi_manager); a name starting with '!' wants that entity off. The
// entities answer through GetOnState: 1 on, 0 off, -1 can't tell (ignored).
//
class CWatchMaster : public CBaseDelay
{
public:
	void Spawn( void );
	void KeyValue( KeyValueData *pkvd );
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	int	ObjectCaps( void ) { return FCAP_MASTER; }
	int GetOnState( void ) { return IsTriggered( this ); }
	BOOL IsTriggered( CBaseEntity *pActivator );

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );

	static	TYPEDESCRIPTION m_SaveData[];

	string_t	m_globalstate;
	int			m_cTargets;
	int			m_iTargetName[ MAX_WATCH_TARGETS ];
};

LINK_ENTITY_TO_CLASS( master_watcher, CWatchMaster );

TYPEDESCRIPTION	CWatchMaster::m_SaveData[] =
{
	DEFINE_FIELD( CWatchMaster, m_globalstate, FIELD_STRING ),
	DEFINE_FIELD( CWatchMaster, m_cTargets, FIELD_INTEGER ),
	DEFINE_ARRAY( CWatchMaster, m_iTargetName, FIELD_STRING, MAX_WATCH_TARGETS ),
};

IMPLEMENT_SAVERESTORE( CWatchMaster, CBaseDelay );


void CWatchMaster :: Spawn( void )
{
	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_NONE;
}


void CWatchMaster :: KeyValue( KeyValueData *pkvd )
{
	if ( FStrEq( pkvd->szKeyName, "globalstate" ) )
	{
		m_globalstate = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else // add this field to the watch list
	{
		if ( m_cTargets < MAX_WATCH_TARGETS )
		{
			char tmp[128];

			UTIL_StripToken( pkvd->szKeyName, tmp );
			m_iTargetName[ m_cTargets ] = ALLOC_STRING( tmp );
			m_cTargets++;
			pkvd->fHandled = TRUE;
		}
	}
}


void CWatchMaster :: Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if ( IsTriggered( pActivator ) )
		SUB_UseTargets( pActivator, useType, value );
}


BOOL CWatchMaster :: IsTriggered( CBaseEntity *pActivator )
{
	if ( m_globalstate && gGlobalState.EntityGetState( m_globalstate ) != GLOBAL_ON )
		return FALSE;

	bool fResult = FBitSet( pev->spawnflags, SF_WATCH_DEFAULTON );

	for ( int i = 0; i < m_cTargets && m_iTargetName[i]; i++ )
	{
		const char *pszTarget = STRING( m_iTargetName[i] );
		const char *pszName = pszTarget;

		if ( *pszName == '!' )
			pszName++;

		bool fOn = FBitSet( pev->spawnflags, SF_WATCH_DEFAULTON );

		CBaseEntity *pTarget = UTIL_FindEntityByTargetname( NULL, pszName );
		while ( pTarget )
		{
			if ( pTarget != this )
			{
				int iState = pTarget->GetOnState();

				ALERT( at_console, "%s (%s) is %d\n", pszTarget, STRING( pTarget->pev->classname ), iState );

				if ( (bool)FBitSet( pev->spawnflags, SF_WATCH_OR ) )
				{
					if ( *pszTarget == '!' )
					{
						if ( iState > 0 )
							fOn = false;
						else if ( iState != -1 )
							return TRUE;
					}
					else
					{
						if ( iState == 0 )
							fOn = false;
						else if ( iState != -1 )
							return TRUE;
					}
				}
				else
				{
					if ( *pszTarget == '!' )
					{
						if ( iState > 0 )
							return FALSE;
						else if ( iState != -1 )
							fOn = true;
					}
					else
					{
						if ( iState == 0 )
							return FALSE;
						else if ( iState != -1 )
							fOn = true;
					}
				}
			}

			pTarget = UTIL_FindEntityByTargetname( pTarget, pszName );
		}

		if ( !(bool)FBitSet( pev->spawnflags, SF_WATCH_OR ) )
		{
			if ( !fOn )
				return FALSE;
			fResult = true;
		}
	}

	return fResult;
}


//
// master_ident: on when whoever activates it matches every keyvalue that is
// set: "m_classname", "m_targetname", "m_target", "m_movetype", "m_solid",
// and its own netname, message, model, body, skin (-1: any), rendermode,
// renderfx and weapons (-1: any); health and armorvalue are upper limits.
//
class CIDMaster : public CBaseDelay
{
public:
	void Spawn( void );
	void KeyValue( KeyValueData *pkvd );
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	int	ObjectCaps( void ) { return FCAP_MASTER; }
	int GetOnState( void ) { return IsTriggered( this ); }
	BOOL IsTriggered( CBaseEntity *pActivator );

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );

	static	TYPEDESCRIPTION m_SaveData[];

	string_t	m_globalstate;
	string_t	m_classname;	// name ours (keyvalue)
	string_t	m_targetname;	// name ours (keyvalue)
	string_t	m_target;		// name ours (keyvalue)
	int			m_solid;		// name ours (keyvalue)
	int			m_movetype;		// name ours (keyvalue)
};

LINK_ENTITY_TO_CLASS( master_ident, CIDMaster );

// sic: only the global state is saved
TYPEDESCRIPTION	CIDMaster::m_SaveData[] =
{
	DEFINE_FIELD( CIDMaster, m_globalstate, FIELD_STRING ),
};

IMPLEMENT_SAVERESTORE( CIDMaster, CBaseDelay );


void CIDMaster :: Spawn( void )
{
	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_NONE;
}


void CIDMaster :: KeyValue( KeyValueData *pkvd )
{
	if ( FStrEq( pkvd->szKeyName, "globalstate" ) )
	{
		m_globalstate = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "m_classname" ) )
	{
		m_classname = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "m_targetname" ) )
	{
		m_targetname = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "m_target" ) )
	{
		m_target = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "m_movetype" ) )
	{
		m_movetype = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "m_solid" ) )
	{
		m_solid = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		CBaseDelay::KeyValue( pkvd );
}


void CIDMaster :: Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if ( IsTriggered( pActivator ) )
		SUB_UseTargets( pActivator, useType, value );
}


BOOL CIDMaster :: IsTriggered( CBaseEntity *pActivator )
{
	if ( !pActivator )
		return FALSE;

	if ( m_globalstate && gGlobalState.EntityGetState( m_globalstate ) != GLOBAL_ON )
		return FALSE;

	if ( m_classname && !FStrEq( STRING( pActivator->pev->classname ), STRING( m_classname ) ) )
		return FALSE;

	if ( m_targetname && !FStrEq( STRING( pActivator->pev->targetname ), STRING( m_targetname ) ) )
		return FALSE;

	if ( m_target && !FStrEq( STRING( pActivator->pev->target ), STRING( m_target ) ) )
		return FALSE;

	if ( pev->netname && !FStrEq( STRING( pActivator->pev->netname ), STRING( pev->netname ) ) )
		return FALSE;

	if ( pev->message && !FStrEq( STRING( pActivator->pev->message ), STRING( pev->message ) ) )
		return FALSE;

	if ( pev->model && !FStrEq( STRING( pActivator->pev->model ), STRING( pev->model ) ) )
		return FALSE;

	if ( pev->health > 0 && pActivator->pev->health > pev->health )
		return FALSE;

	if ( pev->armorvalue > 0 && pActivator->pev->armorvalue > pev->armorvalue )
		return FALSE;

	if ( pev->body != -1 && pActivator->pev->body != pev->body )
		return FALSE;

	if ( pev->skin != -1 && pActivator->pev->skin != pev->skin )
		return FALSE;

	if ( m_movetype && pActivator->pev->movetype != m_movetype )
		return FALSE;

	if ( m_solid && pActivator->pev->solid != m_solid )
		return FALSE;

	if ( pev->rendermode && pActivator->pev->rendermode != pev->rendermode )
		return FALSE;

	if ( pev->renderfx && pActivator->pev->renderfx != pev->renderfx )
		return FALSE;

	if ( pev->weapons != -1 && pActivator->pev->weapons != pev->weapons )
		return FALSE;

	return TRUE;
}

/***
*
* NEW file for the Mod "Spirit of Half-Life", by Laurie R. Cheers. (LRC)
* Created 19/11/00
*
***/
/*

===== alias.cpp ========================================================

Alias entities, replace the less powerful and (IMHO) less intuitive
trigger_changetarget entity.

*/

#include "extdll.h"
#include "util.h"
#include "cbase.h"

/*********************
* Worldcraft entity: info_alias
* 
* targetname- alias name
* target-     alias destination while ON
* netname-    alias destination while OFF
*
* Yes, there's basically nothing in this entity. The actual work is done by UTIL_FindEntityByTargetname.
**********************/

#define SF_ALIAS_OFF 1
#define SF_ALIAS_DEBUG 2

class CInfoAlias : public CBaseEntity
{
public:
	virtual int	ObjectCaps( void ) { return CBaseEntity :: ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }
	void Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value);
	void Think( void );
	void Spawn( void );
	STATE GetState() { return (pev->spawnflags & SF_ALIAS_OFF)?STATE_OFF:STATE_ON; }
};

LINK_ENTITY_TO_CLASS( info_alias, CInfoAlias );

void CInfoAlias::Spawn( void )
{
	if (pev->spawnflags & SF_ALIAS_OFF)
	{
		int iszTemp = pev->target;
		pev->target = pev->netname;
		pev->netname = iszTemp;
	}
}

// Never change aliases during a Use; do it as a think instead.
void CInfoAlias::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if (ShouldToggle(useType))
	{
		SetNextThink( 0 );
	}
}

void CInfoAlias::Think( void )
{
	if (pev->spawnflags & SF_ALIAS_OFF)
	{
		if (pev->spawnflags & SF_ALIAS_DEBUG)
			ALERT(at_console,"DEBUG: info_alias %s turns on",STRING(pev->targetname));
		pev->spawnflags &= ~SF_ALIAS_OFF;
	}
	else
	{
		if (pev->spawnflags & SF_ALIAS_DEBUG)
			ALERT(at_console,"DEBUG: info_alias %s turns off",STRING(pev->targetname));
		pev->spawnflags |= SF_ALIAS_OFF;
	}

	int temp = pev->netname;
	pev->netname = pev->target;
	pev->target = temp;

	if (pev->spawnflags & SF_ALIAS_DEBUG)
		ALERT(at_console,": target becomes \"%s\"\n",STRING(pev->target));
}

/*********************
* Worldcraft entity: info_group
* 
* targetname- name
* target-     alias entity to affect
* other values are handled in a multi_manager-like way.
**********************/
// definition in cbase.h

#define SF_GROUP_DEBUG 2

LINK_ENTITY_TO_CLASS( info_group, CInfoGroup );

TYPEDESCRIPTION	CInfoGroup::m_SaveData[] = 
{
	DEFINE_FIELD( CInfoGroup, m_cMembers, FIELD_INTEGER ),
	DEFINE_ARRAY( CInfoGroup, m_iszMemberName, FIELD_STRING, MAX_MULTI_TARGETS ),
	DEFINE_ARRAY( CInfoGroup, m_iszMemberValue, FIELD_STRING, MAX_MULTI_TARGETS ),
};

IMPLEMENT_SAVERESTORE(CInfoGroup,CBaseEntity);

void CInfoGroup :: KeyValue( KeyValueData *pkvd )
{
	// this assumes that additional fields are targetnames and their values are delay values.
	if ( m_cMembers < MAX_MULTI_TARGETS )
	{
		char tmp[128];
		UTIL_StripToken( pkvd->szKeyName, tmp );
		m_iszMemberName [ m_cMembers ] = ALLOC_STRING( tmp );
		m_iszMemberValue [ m_cMembers ] = ALLOC_STRING (pkvd->szValue);
		m_cMembers++;
		pkvd->fHandled = TRUE;
	}
	else
	{
		ALERT(at_error,"Too many members for info_group %s (limit is %d)\n",STRING(pev->targetname),MAX_MULTI_TARGETS);
	}
}

// Never change aliases during a Use; do it as a think instead.
void CInfoGroup::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	SetNextThink( 0 );
}

void CInfoGroup::Think( void )
{
	CBaseEntity *pTarget = UTIL_FindEntityByTargetname( NULL, STRING( pev->target ) );

	if (pTarget && FClassnameIs(pTarget->pev, "info_alias"))
	{
		// CONSIDER: check what the alias's state is, and set whichever value is appropriate?
		pTarget->pev->target = pev->targetname;
		if (pev->spawnflags & SF_GROUP_DEBUG)
			ALERT(at_console, "DEBUG: info_group %s changed the contents of alias %s to \"%s\"\n",STRING(pev->targetname),STRING(pTarget->pev->targetname),STRING(pTarget->pev->target));
	}
	else if (pev->target)
	{
		ALERT(at_console, "info_group \"%s\": alias \"%s\" was not found or not an alias!", STRING(pev->targetname), STRING(pev->target));
	}
}

int CInfoGroup::GetMember( const char* szMemberName )
{
	if (!szMemberName)
	{
		ALERT(at_console,"info_group: GetMember called with null szMemberName!?\n");
		return NULL;
	}
	for (int i = 0; i < m_cMembers; i++)
	{
		if (FStrEq(szMemberName, STRING(m_iszMemberName[i])))
		{
//			ALERT(at_console,"getMember: found member\n");
			return m_iszMemberValue[i];
		}
	}
	ALERT(at_console,"info_group \"%s\" has no member called \"%s\".\n",STRING(pev->targetname),szMemberName);
//	ALERT(at_console,"getMember: fail\n");
	return NULL;
}

/*********************
* Worldcraft entity: multi_alias
* 
* targetname- name
* other values are handled in a multi_manager-like way.
**********************/
// definition in cbase.h

LINK_ENTITY_TO_CLASS( multi_alias, CMultiAlias );

TYPEDESCRIPTION	CMultiAlias::m_SaveData[] = 
{
	DEFINE_FIELD( CMultiAlias, m_cTargets, FIELD_INTEGER ),
	DEFINE_ARRAY( CMultiAlias, m_iszTargets, FIELD_STRING, MAX_MULTI_TARGETS ),
	DEFINE_FIELD( CMultiAlias, m_iTotalValue, FIELD_INTEGER ),
	DEFINE_ARRAY( CMultiAlias, m_iValues, FIELD_INTEGER, MAX_MULTI_TARGETS ),
	DEFINE_FIELD( CMultiAlias, m_iMode, FIELD_INTEGER ),
};

IMPLEMENT_SAVERESTORE(CMultiAlias,CBaseEntity);

void CMultiAlias :: KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "m_iMode"))
	{
		m_iMode = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	// this assumes that additional fields are targetnames and their values are probability values.
	else if ( m_cTargets < MAX_MULTI_TARGETS )
	{
		char tmp[128];
		UTIL_StripToken( pkvd->szKeyName, tmp );

		m_iszTargets [ m_cTargets ] = ALLOC_STRING( tmp );
		m_iValues [ m_cTargets ] = atoi( pkvd->szValue );

		m_iTotalValue += m_iValues [ m_cTargets ];
		m_cTargets++;

		pkvd->fHandled = TRUE;
	}
	else
	{
		ALERT(at_error,"Too many targets for multi_alias %s (limit is %d)\n",STRING(pev->targetname), MAX_MULTI_TARGETS);
	}
}

/*********************
* Worldcraft entity: trigger_changealias
* 
* target-     alias entity to affect
* netname-    value to change the alias to
**********************/

#define SF_CHANGEALIAS_RESOLVE 1
#define SF_CHANGEALIAS_DEBUG 2

class CTriggerChangeAlias : public CBaseEntity
{
public:
	void Spawn( void );
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

	int ObjectCaps( void ) { return CBaseEntity::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }
};
LINK_ENTITY_TO_CLASS( trigger_changealias, CTriggerChangeAlias );

void CTriggerChangeAlias::Spawn( void )
{
}

void CTriggerChangeAlias::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	CBaseEntity *pTarget = UTIL_FindEntityByTargetname( NULL, STRING( pev->target ) );

	if (pTarget && FClassnameIs(pTarget->pev, "info_alias"))
	{
		int iNewValue;
		CBaseEntity *pTemp = NULL;

		if (pev->spawnflags & SF_CHANGEALIAS_RESOLVE)
		{
			pTemp = UTIL_FollowReference(NULL, STRING(pev->netname));
			if (pTemp) iNewValue = pTemp->pev->targetname;
		}

		if (!iNewValue)
			iNewValue = pev->netname;

		pTarget->pev->target = iNewValue;

		if (pev->spawnflags & SF_CHANGEALIAS_DEBUG)
			ALERT(at_console,"DEBUG: trigger_changealias %s activated: changed contents of alias \"%s\" to \"%s\"\n",STRING(pev->targetname),STRING(pTarget->pev->targetname),STRING(iNewValue));
	}
	else
	{
		ALERT(at_error, "trigger_changealias %s: alias \"%s\" was not found or not an alias!", STRING(pev->targetname), STRING(pev->target));
	}
}

/***
*
*	Scientist Slaughterhouse: entities that follow other entities.
*	The file name and the names marked "ours" are ours; class, field and
*	entity names come from the binary (save tables, exports).
*
****/
/*

===== entity_link.cpp ========================================================

  info_target_linked (CBaseLinked) and entity_link (CMultiLink)

*/

#include "extdll.h"
#include "util.h"
#include "cbase.h"

#include "entity_link.h"


//
// info_target_linked: an info_target that moves along with the entity named
// by "link_to_this", keeping its offset in that entity's frame and the angle
// difference. Beams use it as a moving end point.
//
LINK_ENTITY_TO_CLASS( info_target_linked, CBaseLinked );

TYPEDESCRIPTION	CBaseLinked::m_SaveData[] = 
{
	DEFINE_FIELD( CBaseLinked, m_hLinked, FIELD_EHANDLE ),
	DEFINE_FIELD( CBaseLinked, m_iszLinkEnt, FIELD_STRING ),
	DEFINE_FIELD( CBaseLinked, m_vecRelativePos, FIELD_VECTOR ),
	DEFINE_FIELD( CBaseLinked, m_vecAngles, FIELD_VECTOR ),
};

IMPLEMENT_SAVERESTORE( CBaseLinked, CBaseEntity );


void CBaseLinked :: InitLink( void )
{
	m_hLinked = UTIL_FindEntityByTargetname( NULL, STRING( m_iszLinkEnt ) );

	if ( m_hLinked )
	{
		m_vecRelativePos = m_hLinked->pev->origin - pev->origin;
		m_vecAngles = pev->angles;
	}
}


void CBaseLinked :: Spawn( void )
{
	InitLink();

	if ( (bool)FBitSet( pev->spawnflags, SF_LINKED_FAKEMODEL ) && FClassnameIs( pev, "info_target_linked" ) )
	{
		SET_MODEL( ENT( pev ), "sprites/laserdot.spr" );
		pev->renderamt = 0;
		pev->rendermode = kRenderTransAdd;
		UTIL_SetSize( pev, g_vecZero, g_vecZero );
	}

	SetThink( LinkThink );
	pev->nextthink = gpGlobals->time;
}


void CBaseLinked :: LinkThink( void )
{
	if ( m_hLinked )
	{
		pev->nextthink = gpGlobals->time + 0.05;

		UTIL_MakeVectors( m_hLinked->pev->angles );
		UTIL_SetOrigin( pev, m_hLinked->pev->origin + gpGlobals->v_forward * -m_vecRelativePos.x + gpGlobals->v_right * m_vecRelativePos.y + gpGlobals->v_up * -m_vecRelativePos.z );
		pev->angles = m_hLinked->pev->angles + m_vecAngles;
	}
}


void CBaseLinked :: KeyValue( KeyValueData *pkvd )
{
	if ( FStrEq( pkvd->szKeyName, "link_to_this" ) )
	{
		m_iszLinkEnt = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		CBaseEntity::KeyValue( pkvd );
}


//
// entity_link: moves every entity named by its target along with the entity
// named by its message ("Link Parent"), copying the angles and the position
// the spawnflags select.
//
#define SF_MULTILINK_ROT_X		1
#define SF_MULTILINK_ROT_Y		2
#define SF_MULTILINK_ROT_Z		4
#define SF_MULTILINK_FULLPOS	8	// the offset turns with the parent
#define SF_MULTILINK_POS_X		16
#define SF_MULTILINK_POS_Y		32
#define SF_MULTILINK_POS_Z		64
#define SF_MULTILINK_INVERT		128	// name ours, not in the fgd: the parent's pitch is negated

class CMultiLink : public CPointEntity
{
public:
	void	Spawn( void );
	void	EXPORT LinkThink( void );

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	Vector	m_vecRelativePos;
	EHANDLE	m_hLinkEnt;
};

LINK_ENTITY_TO_CLASS( entity_link, CMultiLink );

TYPEDESCRIPTION	CMultiLink::m_SaveData[] = 
{
	DEFINE_FIELD( CMultiLink, m_vecRelativePos, FIELD_VECTOR ),
	DEFINE_FIELD( CMultiLink, m_hLinkEnt, FIELD_EHANDLE ),
};

IMPLEMENT_SAVERESTORE( CMultiLink, CPointEntity );


void CMultiLink :: Spawn( void )
{
	if ( pev->message && pev->target )
	{
		m_hLinkEnt = UTIL_FindEntityByTargetname( NULL, STRING( pev->message ) );

		if ( m_hLinkEnt )
		{
			m_vecRelativePos = pev->origin - m_hLinkEnt->pev->origin;
			SetThink( LinkThink );
			pev->nextthink = gpGlobals->time;
			pev->flags |= FL_ALWAYSTHINK;
			return;
		}
	}

	UTIL_Remove( this );
}


void CMultiLink :: LinkThink( void )
{
	if ( m_hLinkEnt == NULL )
		UTIL_Remove( this );	// sic: goes on with the dead parent

	pev->nextthink = gpGlobals->time + gpGlobals->frametime;

	Vector vecOrigin;

	if ( FBitSet( pev->spawnflags, SF_MULTILINK_FULLPOS ) )
	{
		Vector forward, right, up;

		g_engfuncs.pfnAngleVectors( m_hLinkEnt->pev->angles, forward, right, up );
		vecOrigin = m_hLinkEnt->pev->origin + forward * m_vecRelativePos.x + right * -m_vecRelativePos.y + up * m_vecRelativePos.z;
	}
	else
	{
		vecOrigin = m_hLinkEnt->pev->origin + m_vecRelativePos;
	}

	Vector vecAngles;

	if ( (bool)FBitSet( pev->spawnflags, SF_MULTILINK_INVERT ) )
	{
		vecAngles = Vector( -m_hLinkEnt->pev->angles.x, m_hLinkEnt->pev->angles.y, m_hLinkEnt->pev->angles.z ) + pev->angles;
	}
	else
	{
		vecAngles = m_hLinkEnt->pev->angles + pev->angles;
	}

	CBaseEntity *pTarget = UTIL_FindEntityByTargetname( NULL, STRING( pev->target ) );
	while ( pTarget )
	{
		if ( FBitSet( pev->spawnflags, SF_MULTILINK_ROT_X ) )
		{
			pTarget->pev->angles.x = vecAngles.x;
			pTarget->pev->avelocity.x = m_hLinkEnt->pev->avelocity.x;
		}
		if ( FBitSet( pev->spawnflags, SF_MULTILINK_ROT_Y ) )
		{
			pTarget->pev->angles.y = vecAngles.y;
			pTarget->pev->avelocity.y = m_hLinkEnt->pev->avelocity.y;
		}
		if ( FBitSet( pev->spawnflags, SF_MULTILINK_ROT_Z ) )
		{
			pTarget->pev->angles.z = vecAngles.z;
			pTarget->pev->avelocity.z = m_hLinkEnt->pev->avelocity.z;
		}

		if ( FBitSet( pev->spawnflags, SF_MULTILINK_FULLPOS ) )
		{
			UTIL_SetOrigin( pTarget->pev, vecOrigin );
			pTarget->pev->velocity = m_hLinkEnt->pev->velocity;
		}
		else
		{
			if ( FBitSet( pev->spawnflags, SF_MULTILINK_POS_X ) )
			{
				pTarget->pev->origin.x = vecOrigin.x;
				pTarget->pev->velocity.z = m_hLinkEnt->pev->velocity.z;	// sic: z for every axis
			}
			if ( FBitSet( pev->spawnflags, SF_MULTILINK_POS_Y ) )
			{
				pTarget->pev->origin.y = vecOrigin.y;
				pTarget->pev->velocity.z = m_hLinkEnt->pev->velocity.z;	// sic
			}
			if ( FBitSet( pev->spawnflags, SF_MULTILINK_POS_Z ) )
			{
				pTarget->pev->origin.z = vecOrigin.z;
				pTarget->pev->velocity.z = m_hLinkEnt->pev->velocity.z;
			}
		}

		pTarget = UTIL_FindEntityByTargetname( pTarget, STRING( pev->target ) );
	}
}

/***
*
*	Scientist Slaughterhouse: the camera entity.
*	The file name and the names marked "ours" are ours; class, field and
*	entity names come from the binary (save tables, exports).
*
****/
/*

===== camera.cpp ========================================================

  info_camera (CInfoCamera): a trigger_camera lookalike that follows or
  watches a target until the player moves or the camera is used again.

*/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "entity_link.h"


#define SF_CAMERA_SWITCHTARGET		1	// "Switch Target": look for the target again when it's gone
#define SF_CAMERA_CLASSNAME			2	// "Target classname": the target is a classname, not a targetname
#define SF_CAMERA_PERSISTANT		4	// "Persistant": keep going without a target
#define SF_CAMERA_NOCONTROL			8	// "No player control": the player is frozen and can't end it

// "type" (fgd "Move type")
enum
{
	CAMERA_STATIC = 0,		// a linked entity (link_to_this)
	CAMERA_TRACKING,		// turns to face the target
	CAMERA_OVERHEAD,		// moves over the target at its own height
	CAMERA_EYE,				// the target's eyes and angles
	CAMERA_BEHIND,			// behind and above the target
	CAMERA_NEAR,			// 100 units from the target
};	// names ours

class CInfoCamera : public CBaseLinked
{
public:
	void Spawn( void );
	void KeyValue( KeyValueData *pkvd );
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void Think( void );
	int GetEntityTriggerState( void ) { return m_pPlayer != NULL; }

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );

	static	TYPEDESCRIPTION m_SaveData[];

	void FindTargetByClassname( void );		// name ours
	void FindTargetByTargetname( void );	// name ours
	void Face( void );						// name ours
	void MoveTo( const Vector &vecDest );	// name ours

	int				m_type;
	EHANDLE			m_hTarget;
	CBasePlayer		*m_pPlayer;
};

LINK_ENTITY_TO_CLASS( info_camera, CInfoCamera );

TYPEDESCRIPTION	CInfoCamera::m_SaveData[] =
{
	DEFINE_FIELD( CInfoCamera, m_type, FIELD_INTEGER ),
	DEFINE_FIELD( CInfoCamera, m_hTarget, FIELD_EHANDLE ),
	DEFINE_FIELD( CInfoCamera, m_pPlayer, FIELD_CLASSPTR ),
};

IMPLEMENT_SAVERESTORE( CInfoCamera, CBaseLinked );


void CInfoCamera :: Spawn( void )
{
	m_pPlayer = NULL;
	SET_MODEL( ENT( pev ), "models/grenade.mdl" );
	InitLink();

	pev->movetype = MOVETYPE_NOCLIP;
	pev->solid = SOLID_NOT;
	pev->renderamt = 0;
	pev->rendermode = kRenderTransTexture;

	pev->speed = max( pev->speed, 100 );
	pev->yaw_speed = max( pev->yaw_speed, 10 );
	pev->pitch_speed = max( pev->pitch_speed, 10 );
}


void CInfoCamera :: KeyValue( KeyValueData *pkvd )
{
	if ( FStrEq( pkvd->szKeyName, "type" ) )
	{
		m_type = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		CBaseLinked::KeyValue( pkvd );
}


void CInfoCamera :: Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if ( m_pPlayer )
	{
		// turn it off: back to the player's view and to where it started
		pev->nextthink = 0;
		SET_VIEW( m_pPlayer->edict(), m_pPlayer->edict() );
		m_pPlayer = NULL;
		m_hTarget = NULL;
		pev->velocity = pev->avelocity = g_vecZero;
		UTIL_SetOrigin( pev, pev->vuser1 );

		if ( HasSpawnFlag( SF_CAMERA_NOCONTROL ) )
			m_pPlayer->EnableControl( TRUE );	// sic: m_pPlayer is NULL by now
		return;
	}

	if ( !pActivator->IsPlayer() )
		return;

	if ( m_type == CAMERA_STATIC )
	{
		LinkThink();
		m_pPlayer = (CBasePlayer *)pActivator;
		pev->nextthink = gpGlobals->time;
		SET_VIEW( pActivator->edict(), edict() );
		pev->vuser1 = pev->origin;

		if ( HasSpawnFlag( SF_CAMERA_NOCONTROL ) )
			m_pPlayer->EnableControl( FALSE );
		return;
	}

	bool fPersistant = HasSpawnFlag( SF_CAMERA_PERSISTANT );

	if ( HasSpawnFlag( SF_CAMERA_CLASSNAME ) )
		FindTargetByClassname();
	else
		FindTargetByTargetname();

	if ( m_hTarget != NULL || fPersistant )
	{
		m_pPlayer = (CBasePlayer *)pActivator;
		pev->nextthink = gpGlobals->time;
		SET_VIEW( pActivator->edict(), edict() );
		pev->vuser1 = pev->origin;

		if ( HasSpawnFlag( SF_CAMERA_NOCONTROL ) )
			m_pPlayer->EnableControl( FALSE );
	}
}


void CInfoCamera :: Think( void )
{
	if ( m_hTarget == NULL && m_type != CAMERA_STATIC )
	{
		if ( HasSpawnFlag( SF_CAMERA_SWITCHTARGET ) )
		{
			if ( HasSpawnFlag( SF_CAMERA_CLASSNAME ) )
				FindTargetByClassname();
			else
				FindTargetByTargetname();
		}

		if ( m_hTarget == NULL && !HasSpawnFlag( SF_CAMERA_PERSISTANT ) )
		{
			pev->nextthink = 0;
			SET_VIEW( m_pPlayer->edict(), m_pPlayer->edict() );
			m_pPlayer = NULL;
			m_hTarget = NULL;
			pev->velocity = pev->avelocity = g_vecZero;

			if ( HasSpawnFlag( SF_CAMERA_NOCONTROL ) )
				m_pPlayer->EnableControl( TRUE );	// sic: m_pPlayer is NULL by now
			return;
		}
	}

	// a player in control ends it by moving
	if ( !HasSpawnFlag( SF_CAMERA_NOCONTROL ) && FBitSet( m_pPlayer->pev->button, IN_JUMP | IN_FORWARD | IN_BACK | IN_MOVELEFT | IN_MOVERIGHT ) )
	{
		pev->nextthink = 0;
		SET_VIEW( m_pPlayer->edict(), m_pPlayer->edict() );
		m_pPlayer = NULL;
		m_hTarget = NULL;
		pev->velocity = pev->avelocity = g_vecZero;
		return;
	}

	pev->nextthink = gpGlobals->time + 0.05;

	if ( m_type == CAMERA_STATIC )
	{
		LinkThink();
		return;
	}

	if ( m_hTarget == NULL )
	{
		pev->velocity = pev->avelocity = g_vecZero;
		return;
	}

	switch ( m_type )
	{
	case CAMERA_TRACKING:
		Face();
		break;

	case CAMERA_OVERHEAD:
		MoveTo( Vector( m_hTarget->pev->origin.x, m_hTarget->pev->origin.y, pev->origin.z ) );
		Face();
		break;

	case CAMERA_EYE:
		pev->origin = m_hTarget->EyePosition();
		pev->angles = m_hTarget->pev->angles;
		break;

	case CAMERA_BEHIND:
		UTIL_MakeVectors( m_hTarget->pev->angles );
		MoveTo( m_hTarget->Center() - gpGlobals->v_forward * 100 + gpGlobals->v_up * 50 + gpGlobals->v_right * 25 );
		Face();
		// sic: falls through

	case CAMERA_NEAR:
		MoveTo( m_hTarget->Center() + ( pev->origin - m_hTarget->Center() ).Normalize() * 100 );
		Face();
		break;
	}
}


// the target named by pev->target (a classname)
void CInfoCamera :: FindTargetByClassname( void )
{
	float flDist = 10000;
	float flLength;

	CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, STRING( pev->target ) );
	while ( pEntity )
	{
		if ( flLength = ( pEntity->pev->origin - pev->origin ).Length() < flDist )	// sic: precedence, takes the first one
		{
			m_hTarget = pEntity;
			flDist = flLength;
		}

		pEntity = UTIL_FindEntityByClassname( pEntity, STRING( pev->target ) );
	}
}


// the target named by pev->target (a targetname)
void CInfoCamera :: FindTargetByTargetname( void )
{
	float flDist = 10000;
	float flLength;

	CBaseEntity *pEntity = UTIL_FindEntityByTargetname( NULL, STRING( pev->target ) );
	while ( pEntity )
	{
		if ( flLength = ( pEntity->pev->origin - pev->origin ).Length() < flDist )	// sic: precedence, takes the first one
		{
			m_hTarget = pEntity;
			flDist = flLength;
		}

		pEntity = UTIL_FindEntityByTargetname( pEntity, STRING( pev->target ) );
	}
}


// turn toward the target at pitch_speed/yaw_speed, snapping when close
void CInfoCamera :: Face( void )
{
	Vector vecAngles = UTIL_VecToAngles( m_hTarget->Center() - pev->origin );
	vecAngles.x = -vecAngles.x;
	Vector vecDelta = vecAngles - pev->angles;

	if ( fabs( vecDelta.x < 1 ) )		// sic: not fabs( vecDelta.x ) < 1
	{
		pev->angles.x = vecAngles.x;
		pev->avelocity.x = 0;
	}
	else
		pev->avelocity.x = vecDelta.x < 0 ? -pev->pitch_speed : pev->pitch_speed;

	if ( fabs( vecDelta.y < 1 ) )		// sic
	{
		pev->angles.y = vecAngles.y;
		pev->avelocity.y = 0;
	}
	else
		pev->avelocity.y = vecDelta.y < 0 ? -pev->yaw_speed : pev->yaw_speed;
}


// fly toward vecDest at pev->speed, arriving within 5 units
void CInfoCamera :: MoveTo( const Vector &vecDest )
{
	Vector vecDelta = vecDest - pev->origin;
	float flDist = vecDelta.Length();

	if ( flDist < 5 )
	{
		pev->origin = vecDest;
		pev->velocity = g_vecZero;
		return;
	}

	pev->velocity = vecDelta.Normalize() * pev->speed;
}

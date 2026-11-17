/***
*
*	Scientist Slaughterhouse: brush turrets and the shoot points that fire for them.
*	The file name and the names marked "ours" are ours; class, field and
*	entity names come from the binary (save tables, exports). Much of the
*	turret code is func_tank.cpp's and turret.cpp's, edited.
*
****/
/*

===== func_turret.cpp ========================================================

  func_turret, func_turret_ballistic, func_turret_manual, func_turretcontrols,
  the info_shootpoint entities, cshell, info_target_area and info_laserspot

*/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "effects.h"
#include "weapons.h"
#include "player.h"
#include "entity_link.h"
#include "beams.h"


// shoot point "firespread": the cones of weapons.h by degree, with the
// func_tank's four old cones ("LEGACY") in between
static float gShootpointSpread[] =	// name ours
{
	0,							// point accurate
	VECTOR_CONE_1DEGREES.x,
	VECTOR_CONE_2DEGREES.x,
	0.025,						// small cone
	VECTOR_CONE_3DEGREES.x,
	VECTOR_CONE_4DEGREES.x,
	0.05,						// medium cone
	VECTOR_CONE_5DEGREES.x,
	VECTOR_CONE_6DEGREES.x,
	VECTOR_CONE_7DEGREES.x,
	VECTOR_CONE_8DEGREES.x,
	VECTOR_CONE_9DEGREES.x,
	VECTOR_CONE_10DEGREES.x,
	0.1,						// large cone
	VECTOR_CONE_15DEGREES.x,
	VECTOR_CONE_20DEGREES.x,
	0.25,						// extra-large cone
};


// func_turret spawnflags (names ours)
#define SF_TURRET_WAITSEEN		0x0001	// "Wait till seen"
#define SF_TURRET_ACTIVE		0x0002
#define SF_TURRET_TRACKRANDOM	0x0010	// "Track Random Target": aim at the random target's body, not its first point
#define SF_TURRET_RANDOMMODE	0x0020	// "Random Target Mode": aim at random entities named "netname"
#define SF_TURRET_ROTATEFIRE	0x0040	// "Rotate on Fire": a new random target after each shot
#define SF_TURRET_CANCONTROL	0x0080	// "Controllable"
#define SF_TURRET_NOTSOLID		0x0100
#define SF_TURRET_LINEOFSIGHT	0x0200	// "Line of Sight"
#define SF_TURRET_SOUNDON		0x8000

//
// func_turret: a func_tank that is a monster. It finds, tracks and aims at
// enemies (or random targets, or follows a player at its controls) but
// doesn't shoot: while it is on target it fires its "target" (primary fire)
// and "message" (secondary fire) with USE_ON, and with USE_OFF when it stops.
// Linked info_shootpoints do the shooting.
//
class CFuncTurret : public CBaseMonster
{
public:
	void	Spawn( void );
	void	Precache( void );
	void	KeyValue( KeyValueData *pkvd );
	void	Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	int		Classify( void );
	int		IRelationship( CBaseEntity *pTarget );

	int		HeatSignature( void ) { return 0; }
	BOOL	CanBeRepulsed( bool fForce ) { return FALSE; }
	int		GetOnState( void ) { return IsActive() || m_pController; }
	const char *GetKillDesc( void );
	int		ObjectCaps( void ) { return CBaseEntity :: ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }
	BOOL	IsAlive( void ) { return pev->health > 0 || pev->max_health == 0; }	// no max_health: never dies

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	void EXPORT CallActiveThink( void );
	void EXPORT SearchThink( void );
	void EXPORT ControlThink( void );
	virtual void RandomThink( void );	// name ours: "Random Target Mode"
	virtual void ActiveThink( void );

	inline BOOL IsActive( void ) { return (pev->spawnflags & SF_TURRET_ACTIVE)?TRUE:FALSE; }
	inline void TankActivate( void ) { pev->spawnflags |= SF_TURRET_ACTIVE; pev->nextthink = pev->ltime + 0.1; }
	inline void TankDeactivate( void ) { pev->spawnflags &= ~SF_TURRET_ACTIVE; StopRotSound(); }
	inline Vector BarrelPosition( void )
	{
		Vector forward, right, up;
		UTIL_MakeVectorsPrivate( pev->angles, forward, right, up );
		return pev->origin + (forward * m_barrelPos.x) + (right * m_barrelPos.y) + (up * m_barrelPos.z);
	}

	BOOL	InRange( float range );
	void	AdjustAnglesForBarrel( Vector &angles, float distance );
	int		MoveTurret( void );
	void	FireButton( USE_TYPE useType, int iButton );	// name ours: fires target (IN_ATTACK) or message
	void	StartRotSound( void );
	void	StopRotSound( void );

	BOOL	OnControls( entvars_t *pevTest );
	BOOL	StartControl( CBasePlayer* pController );
	void	StopControl( void );
	void	ControllerPostFrame( void );

	int			m_justFired;		// counted up by linked shoot points ("Rotate on Fire")
	CBasePlayer* m_pController;
	Vector		m_vecControllerUsePos;
	Vector		m_vecGoalAngles;	// name ours
	int			m_isPFiring;		// primary fire (target) is on
	int			m_isSFiring;		// secondary fire (message) is on

	float		m_yawCenter;	// "Center" yaw
	float		m_yawRate;		// Max turn rate to track targets
	float		m_yawRange;		// Range of turning motion (one-sided: 30 is +/- 30 degress from center)
								// Zero is full rotation
	float		m_yawTolerance;	// Tolerance angle

	float		m_pitchCenter;	// "Center" pitch
	float		m_pitchRate;	// Max turn rate on pitch
	float		m_pitchRange;	// Range of pitch motion as above
	float		m_pitchTolerance;	// Tolerance angle

	Vector		m_barrelPos;	// Length of the freakin barrel
	float		m_lastSightTime;// Last time I saw target
	float		m_persist;		// Persistence of firing (how long do I shoot when I can't see)
	float		m_minRange;		// Minimum range to aim/track
	float		m_maxRange;		// Max range to aim/track
	int			m_allies;		// "Allies"
	string_t	m_viewEnt;		// "view_ent": camera for the player at the controls
	string_t	m_iszMaster;	// Master entity (game_team_master or multisource)
};
LINK_ENTITY_TO_CLASS( func_turret, CFuncTurret );

TYPEDESCRIPTION	CFuncTurret::m_SaveData[] =
{
	DEFINE_FIELD( CFuncTurret, m_yawCenter, FIELD_FLOAT ),
	DEFINE_FIELD( CFuncTurret, m_yawRate, FIELD_FLOAT ),
	DEFINE_FIELD( CFuncTurret, m_yawRange, FIELD_FLOAT ),
	DEFINE_FIELD( CFuncTurret, m_yawTolerance, FIELD_FLOAT ),
	DEFINE_FIELD( CFuncTurret, m_pitchCenter, FIELD_FLOAT ),
	DEFINE_FIELD( CFuncTurret, m_pitchRate, FIELD_FLOAT ),
	DEFINE_FIELD( CFuncTurret, m_pitchRange, FIELD_FLOAT ),
	DEFINE_FIELD( CFuncTurret, m_pitchTolerance, FIELD_FLOAT ),
	DEFINE_FIELD( CFuncTurret, m_justFired, FIELD_INTEGER ),
	DEFINE_FIELD( CFuncTurret, m_isPFiring, FIELD_INTEGER ),
	DEFINE_FIELD( CFuncTurret, m_isSFiring, FIELD_INTEGER ),
	DEFINE_FIELD( CFuncTurret, m_allies, FIELD_INTEGER ),
	DEFINE_FIELD( CFuncTurret, m_lastSightTime, FIELD_TIME ),
	DEFINE_FIELD( CFuncTurret, m_persist, FIELD_FLOAT ),
	DEFINE_FIELD( CFuncTurret, m_minRange, FIELD_FLOAT ),
	DEFINE_FIELD( CFuncTurret, m_maxRange, FIELD_FLOAT ),
	DEFINE_FIELD( CFuncTurret, m_barrelPos, FIELD_VECTOR ),
	DEFINE_FIELD( CFuncTurret, m_pController, FIELD_CLASSPTR ),
	DEFINE_FIELD( CFuncTurret, m_vecControllerUsePos, FIELD_VECTOR ),
	DEFINE_FIELD( CFuncTurret, m_iszMaster, FIELD_STRING ),
	DEFINE_FIELD( CFuncTurret, m_viewEnt, FIELD_STRING ),
};

IMPLEMENT_SAVERESTORE( CFuncTurret, CBaseMonster );


void CFuncTurret :: Spawn( void )
{
	Precache();

	pev->movetype	= MOVETYPE_PUSH;  // so it doesn't get pushed by anything
	if ( pev->spawnflags & SF_TURRET_NOTSOLID )
		pev->solid	= SOLID_NOT;
	else
		pev->solid	= SOLID_BSP;
	SET_MODEL( ENT(pev), STRING(pev->model) );

	m_yawCenter = pev->angles.y;
	m_pitchCenter = pev->angles.x;
	pev->max_health = 0;
	pev->health = 0;

	if ( IsActive() )
		pev->nextthink = pev->ltime + 1.0;
	else
		pev->nextthink = 0;

	SetThink( SearchThink );
	if ( pev->spawnflags & SF_TURRET_RANDOMMODE )
		SetThink( CallActiveThink );

	m_vecEnemyLKP = BarrelPosition(); // Point at the end of the barrel

	if ( pev->dmg <= 0 )
		pev->dmg = 10;		// random target mode: seconds between targets
	if ( m_minRange < 0 )
		m_minRange = 0;
	if ( m_maxRange <= m_minRange )
		m_maxRange = 8192;

	pev->oldorigin = pev->origin;
	pev->frags = -1;
}


void CFuncTurret :: CallActiveThink( void )
{
	if ( HasSpawnFlag( SF_TURRET_RANDOMMODE ) )
		RandomThink();
	else
		ActiveThink();
}


void CFuncTurret :: Precache( void )
{
	if ( pev->noise )
		PRECACHE_SOUND( (char *)STRING(pev->noise) );
}


void CFuncTurret :: KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "yawrate"))
	{
		m_yawRate = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "yawrange"))
	{
		m_yawRange = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "yawtolerance"))
	{
		m_yawTolerance = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "pitchrange"))
	{
		m_pitchRange = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "pitchrate"))
	{
		m_pitchRate = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "pitchtolerance"))
	{
		m_pitchTolerance = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "rotatesound"))
	{
		pev->noise = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "persistence"))
	{
		m_persist = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "BarrelPos"))
	{
		UTIL_StringToVector( (float *)&m_barrelPos, pkvd->szValue );
		pev->view_ofs = m_barrelPos;
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "minRange"))
	{
		m_minRange = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "maxRange"))
	{
		m_maxRange = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "master"))
	{
		m_iszMaster = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "view_ent"))
	{
		m_viewEnt = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "Allies"))
	{
		m_allies = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CBaseMonster::KeyValue( pkvd );
}


// "Allies": 1 player, 2 good guys, 3 human military, 4 aliens, 5 all but
// the player, 6 all but scientists; 0 (none) hates everything
int CFuncTurret :: IRelationship( CBaseEntity *pTarget )
{
	int iClass = pTarget->Classify();

	if ( iClass == CLASS_NONE || iClass == CLASS_BARNACLE || iClass == CLASS_INSECT )
		return R_NO;

	switch ( m_allies )
	{
	case 1:
		if ( pTarget->IsPlayer() || iClass == 14 )
			return R_NO;
		return R_DL;
	case 2:
		if ( pTarget->IsPlayer() || iClass == CLASS_HUMAN_PASSIVE || iClass == CLASS_PLAYER_ALLY )
			return R_NO;
		return R_DL;
	case 3:
		if ( iClass == CLASS_HUMAN_MILITARY || iClass == CLASS_MACHINE )
			return R_NO;
		return R_DL;
	case 4:
		if ( iClass >= CLASS_ALIEN_MILITARY && iClass <= CLASS_ALIEN_PREDATOR )
			return R_NO;
		return R_DL;
	case 5:
		return pTarget->IsPlayer() ? R_DL : R_NO;
	case 6:
		return iClass == CLASS_HUMAN_PASSIVE;
	}

	return R_DL;
}


//==================================================================================
// TANK CONTROLLING
BOOL CFuncTurret :: OnControls( entvars_t *pevTest )
{
	if ( !(pev->spawnflags & SF_TURRET_CANCONTROL) )
		return FALSE;

	if ( m_pController->pev->button & (IN_JUMP | IN_FORWARD | IN_BACK | IN_MOVELEFT | IN_MOVERIGHT) )
		return FALSE;

	Vector offset = pevTest->origin - pev->origin;

	if ( (m_vecControllerUsePos - pevTest->origin).Length() <= 256 )
		return TRUE;

	return FALSE;
}

BOOL CFuncTurret :: StartControl( CBasePlayer *pController )
{
	if ( m_pController != NULL )
		return FALSE;

	// Team only or disabled?
	if ( m_iszMaster )
	{
		if ( !UTIL_IsMasterTriggered( m_iszMaster, pController ) )
			return FALSE;
	}

	ALERT( at_console, "using Turret!\n");

	m_pController = pController;
	if ( m_pController->m_pActiveItem )
	{
		m_pController->m_pActiveItem->Holster();
		m_pController->pev->weaponmodel = 0;
		m_pController->pev->viewmodel = 0;

	}

	if ( m_viewEnt )
	{
		CBaseEntity *pView = UTIL_FindEntityByTargetname( NULL, STRING( m_viewEnt ) );
		if ( pView )
			SET_VIEW( m_pController->edict(), pView->edict() );
	}

	m_pController->m_iHideHUD |= HIDEHUD_WEAPONS;
	m_vecControllerUsePos = m_pController->pev->origin;

	SetThink( ControlThink );
	pev->nextthink = pev->ltime + 0.1;

	return TRUE;
}

void CFuncTurret :: StopControl()
{
	// TODO: bring back the controllers current weapon
	if ( !m_pController )
		return;

	if ( m_pController->m_pActiveItem )
		m_pController->m_pActiveItem->Deploy();

	ALERT( at_console, "stopped using Turret\n");

	m_pController->m_iHideHUD &= ~HIDEHUD_WEAPONS;

	if ( m_viewEnt )
		SET_VIEW( m_pController->edict(), m_pController->edict() );

	pev->nextthink = 0;
	m_pController = NULL;

	if ( IsActive() )
	{
		SetThink( SearchThink );
		pev->nextthink = pev->ltime + 1.0;
	}
}

// Called each frame by the player's ItemPostFrame
void CFuncTurret :: ControllerPostFrame( void )
{
	if ( m_pController->pev->button & IN_ATTACK )
	{
		if ( !m_isPFiring )
		{
			FireButton( USE_ON, IN_ATTACK );
			m_isPFiring = TRUE;
		}

		// HACKHACK -- make some noise (that the AI can hear)
		if ( m_pController && m_pController->IsPlayer() )
			((CBasePlayer *)m_pController)->m_iWeaponVolume = LOUD_GUN_VOLUME;
	}
	else if ( m_isPFiring )
	{
		FireButton( USE_OFF, IN_ATTACK );
		m_isPFiring = FALSE;
	}

	if ( m_pController->pev->button & IN_ATTACK2 )
	{
		if ( !m_isSFiring )
		{
			FireButton( USE_ON, IN_ATTACK2 );
			m_isSFiring = TRUE;
		}

		// HACKHACK -- make some noise (that the AI can hear)
		if ( m_pController && m_pController->IsPlayer() )
			((CBasePlayer *)m_pController)->m_iWeaponVolume = LOUD_GUN_VOLUME;
	}
	else if ( m_isSFiring )
	{
		FireButton( USE_OFF, IN_ATTACK2 );
		m_isSFiring = FALSE;
	}
}
////////////// END NEW STUFF //////////////


void CFuncTurret :: Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if ( pev->spawnflags & SF_TURRET_CANCONTROL )
	{  // player controlled turret

		if ( pActivator->Classify() != CLASS_PLAYER )
			return;

		if ( value == 2 && useType == USE_SET )
		{
			ControllerPostFrame();
		}
		else if ( !m_pController && useType != USE_OFF )
		{
			((CBasePlayer*)pActivator)->m_pTank = this;
			StartControl( (CBasePlayer*)pActivator );
		}
		else
		{
			StopControl();
		}
	}
	else
	{
		if ( !ShouldToggle( useType, IsActive() ) )
			return;

		if ( IsActive() )
			TankDeactivate();
		else
			TankActivate();
	}
}


void CFuncTurret :: ActiveThink( void )
{
	Vector vecDirToEnemy;

	pev->nextthink = pev->ltime + 0.05;

	if ( !IsActive() )
	{
		m_hEnemy = NULL;
		SetThink( SearchThink );
		if ( m_isPFiring )
			FireButton( USE_OFF, IN_ATTACK );
		if ( m_isSFiring )
			FireButton( USE_OFF, IN_ATTACK2 );
		m_isPFiring = m_isSFiring = FALSE;
		return;
	}

	if ( m_hEnemy == NULL )
	{
		SetThink( SearchThink );
		pev->nextthink += m_persist;
		return;
	}

	// if it's dead, look for something new
	if ( !m_hEnemy->IsAlive() )
	{
		if ( !m_lastSightTime )
		{
			m_lastSightTime = gpGlobals->time;
		}
		else if ( gpGlobals->time - m_lastSightTime >= m_persist )
		{
			m_hEnemy = NULL;
			SetThink( SearchThink );
			return;
		}
	}

	Vector vecMid = pev->origin + pev->view_ofs;
	Vector vecMidEnemy = m_hEnemy->BodyTarget( vecMid );

	vecDirToEnemy = vecMidEnemy - vecMid;	// calculate dir and dist to enemy

	// Look for our current enemy
	int fEnemyVisible = FBoxVisible( pev, m_hEnemy->pev, vecMidEnemy );

	float flDistToEnemy = vecDirToEnemy.Length();

	Vector vec = UTIL_VecToAngles( vecDirToEnemy );

	if ( fEnemyVisible && InRange( flDistToEnemy ) )
	{
		m_vecEnemyLKP = vecMidEnemy;
	}
	else
	{
		// Current enemy is not visible.
		if ( !m_lastSightTime )
		{
			m_lastSightTime = gpGlobals->time;
		}
		else if ( gpGlobals->time - m_lastSightTime >= m_persist )
		{
			m_hEnemy = NULL;
			SetThink( SearchThink );
			return;
		}
		fEnemyVisible = 0;
	}

	// Calculate the additional rotation to point the end of the barrel at the target (not the gun's center)
	AdjustAnglesForBarrel( vec, flDistToEnemy );
	vec.x = -vec.x;

	if ( fabs( UTIL_AngleDistance( vec.x, pev->angles.x ) ) <= m_pitchTolerance && fabs( UTIL_AngleDiff( vec.y, pev->angles.y ) ) <= m_yawTolerance )
	{
		if ( !m_isPFiring )
		{
			FireButton( USE_ON, IN_ATTACK );
			m_isPFiring = TRUE;
		}
		if ( !m_isSFiring )
		{
			FireButton( USE_ON, IN_ATTACK2 );
			m_isSFiring = TRUE;
		}
	}
	else
	{
		if ( m_isPFiring && gpGlobals->time - m_lastSightTime >= m_persist )
		{
			FireButton( USE_OFF, IN_ATTACK );
			m_isPFiring = FALSE;
		}
		if ( m_isSFiring && gpGlobals->time - m_lastSightTime >= m_persist )
		{
			FireButton( USE_OFF, IN_ATTACK2 );
			m_isSFiring = FALSE;
		}
	}

	//move the gun
	if ( fEnemyVisible || m_isPFiring )
	{
		if (vec.y > 360)
			vec.y -= 360;

		if (vec.y < 0)
			vec.y += 360;

		if (vec.x < -180)
			vec.x += 360;

		if (vec.x > 180)
			vec.x -= 360;

		// out of the turret's range: give up after a while
		if ( fabs( UTIL_AngleDistance( vec.x, m_pitchCenter ) ) > m_pitchRange + m_pitchTolerance
			|| fabs( UTIL_AngleDistance( vec.y, m_yawCenter ) ) > m_yawRange + m_yawTolerance )
		{
			if ( !m_lastSightTime )
			{
				m_lastSightTime = gpGlobals->time;
			}
			else if ( gpGlobals->time - m_lastSightTime >= m_persist )
			{
				m_hEnemy = NULL;
				SetThink( SearchThink );
				return;
			}
		}

		m_vecGoalAngles.y = vec.y;
		m_vecGoalAngles.x = vec.x;
	}

	MoveTurret();
}


void CFuncTurret :: SearchThink( void )
{
	if ( m_isPFiring )
	{
		FireButton( USE_OFF, IN_ATTACK );
		m_isPFiring = FALSE;
	}
	if ( m_isSFiring )
	{
		FireButton( USE_OFF, IN_ATTACK2 );
		m_isSFiring = FALSE;
	}

	if ( !IsActive() )
		return;

	// ensure rethink
	pev->nextthink = pev->ltime + 0.3;

	// If we have a target and we're still healthy
	if (m_hEnemy != NULL)
	{
		if (!m_hEnemy->IsAlive() )
			m_hEnemy = NULL;// Dead enemy forces a search for new one
	}

	// Acquire Target
	if (m_hEnemy == NULL)
	{
		Look( m_maxRange );
		m_hEnemy = BestVisibleEnemy();
	}

	// If we've found a target, start to attack
	if (m_hEnemy != NULL)
	{
		SetThink( CallActiveThink );
		m_lastSightTime = 0;
	}
}


// "Random Target Mode": aims at one of the entities named "netname" at
// random, a new one every "dmg" seconds (or after each shot with "Rotate on
// Fire"), and fires whenever it is on target
void CFuncTurret :: RandomThink( void )
{
	bool fNewTarget = false;

	pev->nextthink = pev->ltime + 0.05;

	if ( (pev->spawnflags & SF_TURRET_ROTATEFIRE && m_justFired) || pev->dmgtime < gpGlobals->time || m_hEnemy == NULL )
	{
		m_hEnemy = UTIL_RandomTargetname( STRING( pev->netname ) );
		m_justFired = 0;
		fNewTarget = true;
		if ( !(pev->spawnflags & SF_TURRET_ROTATEFIRE) )
			pev->dmgtime = gpGlobals->time + pev->dmg;
	}

	Vector vecMid = pev->origin + pev->view_ofs;
	Vector vecTarget = m_vecEnemyLKP;

	if ( fNewTarget || (pev->spawnflags & SF_TURRET_TRACKRANDOM) )
		vecTarget = m_hEnemy->BodyTarget( vecMid );

	BOOL fVisible = TRUE;
	if ( pev->spawnflags & SF_TURRET_LINEOFSIGHT )
		fVisible = FVisible( vecTarget );

	if ( fVisible )
		m_vecEnemyLKP = vecTarget;

	Vector vecDir = m_vecEnemyLKP - vecMid;
	float flDist = vecDir.Length();
	Vector vec = UTIL_VecToAngles( vecDir );

	AdjustAnglesForBarrel( vec, flDist );

	if ( UTIL_AngleDistance( vec.x, pev->angles.x ) <= m_pitchTolerance && UTIL_AngleDistance( vec.y, pev->angles.y ) <= m_yawTolerance )	// sic: no fabs
	{
		if ( !m_isPFiring )
		{
			FireButton( USE_ON, IN_ATTACK );
			m_isPFiring = TRUE;
		}
		if ( !m_isSFiring )
		{
			FireButton( USE_ON, IN_ATTACK2 );
			m_isSFiring = TRUE;
		}
	}
	else
	{
		if ( m_isPFiring && gpGlobals->time - m_lastSightTime >= m_persist )
		{
			FireButton( USE_OFF, IN_ATTACK );
			m_isPFiring = FALSE;
		}
		if ( m_isSFiring && gpGlobals->time - m_lastSightTime >= m_persist )
		{
			FireButton( USE_OFF, IN_ATTACK2 );
			m_isSFiring = FALSE;
		}
	}

	if ( fVisible )
	{
		if (vec.y > 360)
			vec.y -= 360;

		if (vec.y < 0)
			vec.y += 360;

		if (vec.x < -180)
			vec.x += 360;

		if (vec.x > 180)
			vec.x -= 360;

		vec.x = -vec.x;

		// out of the turret's range: pick another target
		if ( vec.x > m_pitchCenter + m_pitchRange || vec.x < m_pitchCenter - m_pitchRange
			|| vec.y > m_yawCenter + m_yawRange || vec.y < m_yawCenter - m_yawRange )
			m_hEnemy = NULL;

		m_vecGoalAngles.y = vec.y;
		m_vecGoalAngles.x = vec.x;
	}

	MoveTurret();
}


// turret.cpp's CBaseTurret::MoveTurret, turning pev->angles at the yaw and pitch rates
int CFuncTurret :: MoveTurret( void )
{
	int state = 0;
	// any x movement?

	if (pev->angles.x != m_vecGoalAngles.x)
	{
		float flDir = m_vecGoalAngles.x > pev->angles.x ? 1 : -1 ;

		pev->angles.x += m_pitchRate * flDir * 0.05;

		// if we started below the goal, and now we're past, peg to goal
		if (flDir == 1)
		{
			if (pev->angles.x > m_vecGoalAngles.x)
				pev->angles.x = m_vecGoalAngles.x;
		}
		else
		{
			if (pev->angles.x < m_vecGoalAngles.x)
				pev->angles.x = m_vecGoalAngles.x;
		}

		state = 1;
	}

	if (pev->angles.y != m_vecGoalAngles.y)
	{
		float flDir = m_vecGoalAngles.y > pev->angles.y ? 1 : -1 ;
		float flDist = fabs(m_vecGoalAngles.y - pev->angles.y);

		if (flDist > 180)
		{
			flDist = 360 - flDist;
			flDir = -flDir;
		}

		pev->angles.y += m_yawRate * flDir * 0.05;

		if (pev->angles.y < 0)
			pev->angles.y += 360;
		else if (pev->angles.y >= 360)
			pev->angles.y -= 360;

		if (flDist < (0.05 * m_yawRate))
			pev->angles.y = m_vecGoalAngles.y;

		state = 1;
	}

	return state;
}


int CFuncTurret :: Classify( void )
{
	if ( pev->max_health <= 0 )
		return CLASS_NONE;

	switch ( m_allies )
	{
	case 0:
		return CLASS_ALIEN_BIOWEAPON;
	case 1:
	case 2:
		return CLASS_PLAYER_ALLY;
	case 3:
		return CLASS_MACHINE;
	case 4:
		return CLASS_ALIEN_MILITARY;
	}

	return CLASS_MACHINE;
}


BOOL CFuncTurret :: InRange( float range )
{
	if ( range < m_minRange )
		return FALSE;
	if ( m_maxRange > 0 && range > m_maxRange )
		return FALSE;

	return TRUE;
}


// the player at the controls aims the turret
void CFuncTurret :: ControlThink( void )
{
	pev->avelocity = g_vecZero;

	if ( !m_pController )
		return;

	Vector angles;

	// Tanks attempt to mirror the player's angles
	angles = m_pController->pev->v_angle;
	pev->nextthink = pev->ltime + 0.05;

	// Force the angles to be relative to the center position
	angles.y = m_yawCenter + UTIL_AngleDistance( angles.y, m_yawCenter );
	angles.x = m_pitchCenter + UTIL_AngleDistance( angles.x, m_pitchCenter );

	// Limit against range in y
	if ( angles.y > m_yawCenter + m_yawRange )
		angles.y = m_yawCenter + m_yawRange;
	else if ( angles.y < (m_yawCenter - m_yawRange) )
		angles.y = (m_yawCenter - m_yawRange);

	// Move toward target at rate or less
	float distY = UTIL_AngleDistance( angles.y, pev->angles.y );
	pev->avelocity.y = distY * 10;
	if ( pev->avelocity.y > m_yawRate )
		pev->avelocity.y = m_yawRate;
	else if ( pev->avelocity.y < -m_yawRate )
		pev->avelocity.y = -m_yawRate;

	// Limit against range in x
	if ( angles.x > m_pitchCenter + m_pitchRange )
		angles.x = m_pitchCenter + m_pitchRange;
	else if ( angles.x < m_pitchCenter - m_pitchRange )
		angles.x = m_pitchCenter - m_pitchRange;

	// Move toward target at rate or less
	float distX = UTIL_AngleDistance( angles.x, pev->angles.x );
	pev->avelocity.x = distX  * 10;

	if ( pev->avelocity.x > m_pitchRate )
		pev->avelocity.x = m_pitchRate;
	else if ( pev->avelocity.x < -m_pitchRate )
		pev->avelocity.x = -m_pitchRate;

	if ( fabs(pev->avelocity.x) > 1 || fabs(pev->avelocity.y) > 1 )
		StartRotSound();
	else
		StopRotSound();
}


// If barrel is offset, add in additional rotation
void CFuncTurret :: AdjustAnglesForBarrel( Vector &angles, float distance )
{
	float r2, d2;


	if ( m_barrelPos.y != 0 || m_barrelPos.z != 0 )
	{
		distance -= m_barrelPos.z;
		d2 = distance * distance;
		if ( m_barrelPos.y )
		{
			r2 = m_barrelPos.y * m_barrelPos.y;
			angles.y += (180.0 / M_PI) * atan2( m_barrelPos.y, sqrt( d2 - r2 ) );
		}
		if ( m_barrelPos.z )
		{
			r2 = m_barrelPos.z * m_barrelPos.z;
			angles.x += (180.0 / M_PI) * atan2( -m_barrelPos.z, sqrt( d2 - r2 ) );
		}
	}
}


// primary fire: "target"; secondary fire: "message"
void CFuncTurret :: FireButton( USE_TYPE useType, int iButton )
{
	if ( iButton == IN_ATTACK )
	{
		if ( pev->target )
			FireTargets( STRING( pev->target ), m_pController, this, useType, 0 );
	}
	else if ( pev->message )
		FireTargets( STRING( pev->message ), m_pController, this, useType, 0 );
}


void CFuncTurret :: StartRotSound( void )
{
	if ( !pev->noise || (pev->spawnflags & SF_TURRET_SOUNDON) )
		return;
	pev->spawnflags |= SF_TURRET_SOUNDON;
	EMIT_SOUND( edict(), CHAN_STATIC, (char*)STRING(pev->noise), 0.85, ATTN_NORM);
}


void CFuncTurret :: StopRotSound( void )
{
	if ( pev->spawnflags & SF_TURRET_SOUNDON )
		STOP_SOUND( edict(), CHAN_STATIC, (char*)STRING(pev->noise) );
	pev->spawnflags &= ~SF_TURRET_SOUNDON;
}


//
// func_turret_ballistic: aims its barrel so that a shell fired at "speed"
// (VecCheckThrow; with no speed, a toss) lands on the enemy
//
class CFuncBallisticTurret : public CFuncTurret	// name ours
{
public:
	void	ActiveThink( void );
};
LINK_ENTITY_TO_CLASS( func_turret_ballistic, CFuncBallisticTurret );


void CFuncBallisticTurret :: ActiveThink( void )
{
	Vector vecDirToEnemy;

	pev->nextthink = pev->ltime + 0.05;

	if ( m_hEnemy == NULL || !IsActive() )
	{
		m_hEnemy = NULL;
		SetThink( SearchThink );
		if ( m_isPFiring )
			FireButton( USE_OFF, IN_ATTACK );
		if ( m_isSFiring )
			FireButton( USE_OFF, IN_ATTACK2 );
		m_isPFiring = m_isSFiring = FALSE;
		return;
	}

	// if it's dead, look for something new
	if ( !m_hEnemy->IsAlive() )
	{
		if ( !m_lastSightTime )
		{
			m_lastSightTime = gpGlobals->time;
		}
		else if ( gpGlobals->time - m_lastSightTime >= m_persist )
		{
			m_hEnemy = NULL;
			SetThink( SearchThink );
			return;
		}
	}

	Vector vecMid = pev->origin + pev->view_ofs;
	Vector vecMidEnemy = m_hEnemy->BodyTarget( vecMid );

	float flDistToEnemy = vecDirToEnemy.Length();	// sic: vecDirToEnemy isn't set yet
	vecDirToEnemy = vecMidEnemy;

	// Look for our current enemy
	int fEnemyVisible = FBoxVisible( pev, m_hEnemy->pev, vecMidEnemy );

	if ( fEnemyVisible && InRange( flDistToEnemy ) )
	{
		m_vecEnemyLKP = vecMidEnemy;
	}
	else
	{
		// Current enemy is not visible.
		if ( !m_lastSightTime )
		{
			m_lastSightTime = gpGlobals->time;
		}
		else if ( gpGlobals->time - m_lastSightTime >= m_persist )
		{
			m_hEnemy = NULL;
			SetThink( SearchThink );
			return;
		}
		fEnemyVisible = 0;
	}

	if ( pev->speed > 0 )
		vecDirToEnemy = VecCheckThrow( pev, pev->origin, vecDirToEnemy, pev->speed, 0.5 );
	else
		vecDirToEnemy = VecCheckToss( pev, pev->origin, vecDirToEnemy, 0.5 );

	Vector vec = UTIL_VecToAngles( vecDirToEnemy );

	AdjustAnglesForBarrel( vec, flDistToEnemy );

	if ( UTIL_AngleDistance( vec.x, pev->angles.x ) <= m_pitchTolerance && UTIL_AngleDistance( vec.y, pev->angles.y ) <= m_yawTolerance )	// sic: no fabs
	{
		if ( !m_isPFiring )
		{
			FireButton( USE_ON, IN_ATTACK );
			m_isPFiring = TRUE;
		}
		if ( !m_isSFiring )
		{
			FireButton( USE_ON, IN_ATTACK2 );
			m_isSFiring = TRUE;
		}
	}
	else
	{
		if ( m_isPFiring && gpGlobals->time - m_lastSightTime >= m_persist )
		{
			FireButton( USE_OFF, IN_ATTACK );
			m_isPFiring = FALSE;
		}
		if ( m_isSFiring && gpGlobals->time - m_lastSightTime >= m_persist )
		{
			FireButton( USE_OFF, IN_ATTACK2 );
			m_isSFiring = FALSE;
		}
	}

	//move the gun
	if ( fEnemyVisible )
	{
		if (vec.y > 360)
			vec.y -= 360;

		if (vec.y < 0)
			vec.y += 360;

		if (vec.x < -180)
			vec.x += 360;

		if (vec.x > 180)
			vec.x -= 360;

		vec.x = -vec.x;

		// out of the turret's range: give up after a while
		if ( vec.x > m_pitchCenter + m_pitchRange || vec.x < m_pitchCenter - m_pitchRange
			|| vec.y > m_yawCenter + m_yawRange || vec.y < m_yawCenter - m_yawRange )
		{
			if ( !m_lastSightTime )
			{
				m_lastSightTime = gpGlobals->time;
			}
			else if ( gpGlobals->time - m_lastSightTime >= m_persist )
			{
				m_hEnemy = NULL;
				SetThink( SearchThink );
				return;
			}
		}

		m_vecGoalAngles.y = vec.y;
		m_vecGoalAngles.x = vec.x;
	}

	MoveTurret();
}


// the player at the controls gets the credit
const char *CFuncTurret :: GetKillDesc( void )
{
	if ( m_pController )
		return m_pController->GetKillDesc();

	return !m_iszKillDesc ? "a computer-controlled turret" : STRING( m_iszKillDesc );
}


//
// func_turret_manual: aimed by two momentary_rot_buttons ("m_iszXController"
// pitch, "m_iszYController" yaw) that move it across its pitch and yaw
// ranges; using it toggles both fires
//
class CFuncManualTurret : public CFuncTurret
{
public:
	void	Spawn( void );
	void	KeyValue( KeyValueData *pkvd );
	const char *GetKillDesc( void ) { return !m_iszKillDesc ? "a switch-operated turret" : STRING( m_iszKillDesc ); }

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	void	ManualUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );	// name ours
	void EXPORT ManualThink( void );

	string_t	m_iszXController;
	string_t	m_iszYController;
};
LINK_ENTITY_TO_CLASS( func_turret_manual, CFuncManualTurret );

TYPEDESCRIPTION	CFuncManualTurret::m_SaveData[] =
{
	DEFINE_FIELD( CFuncManualTurret, m_iszXController, FIELD_STRING ),
	DEFINE_FIELD( CFuncManualTurret, m_iszYController, FIELD_STRING ),
};

IMPLEMENT_SAVERESTORE( CFuncManualTurret, CFuncTurret );


void CFuncManualTurret :: Spawn( void )
{
	CFuncTurret::Spawn();

	SetThink( ManualThink );
	SetUse( ManualUse );
	pev->nextthink = pev->ltime + 3;

	// the controllers go from 0 to 1: from one end of the range to the other
	m_pitchCenter -= m_pitchRange;
	m_pitchRange *= 2;
	m_yawCenter -= m_yawRange;
	m_yawRange *= 2;
}


void CFuncManualTurret :: KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "m_iszXController"))
	{
		m_iszXController = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "m_iszYController"))
	{
		m_iszYController = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CFuncTurret::KeyValue( pkvd );
}


void CFuncManualTurret :: ManualUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if ( !ShouldToggle( useType, m_isPFiring ) )
		return;

	if ( !m_isPFiring )
	{
		FireButton( USE_ON, IN_ATTACK );
		FireButton( USE_ON, IN_ATTACK2 );
		m_isPFiring = TRUE;
	}
	else
	{
		FireButton( USE_OFF, IN_ATTACK );
		FireButton( USE_OFF, IN_ATTACK2 );
		m_isPFiring = FALSE;
	}
}


void CFuncManualTurret :: ManualThink( void )
{
	pev->nextthink = pev->ltime + 0.1;

	if ( m_iszXController )
	{
		CBaseEntity *pController = UTIL_FindEntityByTargetname( NULL, STRING( m_iszXController ) );
		if ( pController )
			pev->angles.x = pController->pev->ideal_yaw * m_pitchRange + m_pitchCenter;
	}

	if ( m_iszYController )
	{
		CBaseEntity *pController = UTIL_FindEntityByTargetname( NULL, STRING( m_iszYController ) );
		if ( pController )
			pev->angles.y = pController->pev->ideal_yaw * m_yawRange + m_yawCenter;
	}
}


//
// info_shootpoint and the others: linked point entities that shoot, from
// their origin along their angles, while they are on (fired by a func_turret,
// or used by hand). A shoot point linked to a func_turret tells it when it
// has fired ("Rotate on Fire").
//
#define SF_SHOOTPOINT_NOREPORT	0x0001	// "No Fire Report" (names ours)

class CInfoShootpoint : public CBaseLinked
{
public:
	void	Spawn( void );
	void	Precache( void );
	void	KeyValue( KeyValueData *pkvd );
	void	Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	int		ObjectCaps( void ) { return CBaseLinked :: ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }
	int		GetOnState( void ) { return pev->nextthink != 0; }

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	virtual void Fire( void ) = 0;	// name ours
	void EXPORT FireThink( void );

	Vector	ShootSpread( const Vector &vecDir, float flSpread );	// names ours
	void	FireEffects( void );
	void	ReportFire( void );

	float		m_fireLast;		// Last time I fired
	float		m_fireRate;		// "firerate"
	int			m_fireNum;		// "firenum"
	int			m_tracerRate;	// "tracerate" (name ours)
	float		m_delay;		// "delay" (name ours)
	float		m_spriteScale;	// Scale of any sprites we shoot
	int			m_iszSpriteSmoke;
	int			m_iszSpriteFlash;
	int			m_bulletType;	// "bullet"
	int			m_noiseCount;	// how many of noise, noise1..noise3 are set
	float		m_spread;		// "firespread"
};

TYPEDESCRIPTION	CInfoShootpoint::m_SaveData[] =
{
	DEFINE_FIELD( CInfoShootpoint, m_fireLast, FIELD_TIME ),
	DEFINE_FIELD( CInfoShootpoint, m_fireRate, FIELD_FLOAT ),
	DEFINE_FIELD( CInfoShootpoint, m_fireNum, FIELD_INTEGER ),
	DEFINE_FIELD( CInfoShootpoint, m_spriteScale, FIELD_FLOAT ),
	DEFINE_FIELD( CInfoShootpoint, m_iszSpriteSmoke, FIELD_STRING ),
	DEFINE_FIELD( CInfoShootpoint, m_iszSpriteFlash, FIELD_STRING ),
	DEFINE_FIELD( CInfoShootpoint, m_bulletType, FIELD_INTEGER ),
	DEFINE_FIELD( CInfoShootpoint, m_noiseCount, FIELD_INTEGER ),
	DEFINE_FIELD( CInfoShootpoint, m_spread, FIELD_FLOAT ),
};

IMPLEMENT_SAVERESTORE( CInfoShootpoint, CBaseLinked );


void CInfoShootpoint :: FireThink( void )
{
	Fire();
}


void CInfoShootpoint :: Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if ( !ShouldToggle( useType, pev->nextthink != 0 ) )
		return;

	if ( pev->nextthink != 0 )
	{
		pev->nextthink = 0;
	}
	else
	{
		pev->nextthink = gpGlobals->time + m_delay;
		m_fireLast = pev->nextthink - m_fireRate;
	}
}


// func_tank.cpp's TankTrace spread: vecDir turned by a random circular
// gaussian amount (UTIL_MakeVectors/MakeAimVectors first)
Vector CInfoShootpoint :: ShootSpread( const Vector &vecDir, float flSpread )
{
	if ( flSpread == 0 )
		return vecDir;

	// get circular gaussian spread
	float x, y;
	do {
		x = RANDOM_FLOAT(-0.5,0.5) + RANDOM_FLOAT(-0.5,0.5);
		y = RANDOM_FLOAT(-0.5,0.5) + RANDOM_FLOAT(-0.5,0.5);
	} while (x*x+y*y > 1);

	return vecDir + x * flSpread * gpGlobals->v_right + y * flSpread * gpGlobals->v_up;
}


void CInfoShootpoint :: Spawn( void )
{
	Precache();
	CBaseLinked::Spawn();

	if ( m_hLinked != NULL )
		pev->owner = m_hLinked->edict();

	SetThink( FireThink );
	pev->nextthink = 0;
	m_fireLast = 0;

	if ( m_fireNum < 1 )
		m_fireNum = 1;
}


void CInfoShootpoint :: Precache( void )
{
	if ( m_iszSpriteSmoke )
		PRECACHE_MODEL( (char *)STRING(m_iszSpriteSmoke) );
	if ( m_iszSpriteFlash )
		PRECACHE_MODEL( (char *)STRING(m_iszSpriteFlash) );

	if ( pev->noise )
	{
		PRECACHE_SOUND( (char *)STRING(pev->noise) );
		m_noiseCount = 1;
	}
	if ( pev->noise1 )
	{
		PRECACHE_SOUND( (char *)STRING(pev->noise1) );
		m_noiseCount = 2;
	}
	if ( pev->noise2 )
	{
		PRECACHE_SOUND( (char *)STRING(pev->noise2) );
		m_noiseCount = 3;
	}
	if ( pev->noise3 )
	{
		PRECACHE_SOUND( (char *)STRING(pev->noise3) );
		m_noiseCount = 4;
	}
}


void CInfoShootpoint :: KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "firerate"))
	{
		m_fireRate = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "firenum"))
	{
		m_fireNum = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "tracerate"))
	{
		m_tracerRate = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "spritescale"))
	{
		m_spriteScale = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "spritesmoke"))
	{
		m_iszSpriteSmoke = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "spriteflash"))
	{
		m_iszSpriteFlash = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "firesound"))
	{
		pev->noise = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "bullet"))
	{
		m_bulletType = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq(pkvd->szKeyName, "bullet_damage" ))
	{
		pev->dmg = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "firespread"))
	{
		m_spread = gShootpointSpread[atoi(pkvd->szValue)];
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "delay"))
	{
		m_delay = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CBaseLinked::KeyValue( pkvd );
}


// func_tank.cpp's CFuncTank::Fire: a firing sound, the smoke and flash
// sprites, the targets; then tells a linked func_turret
void CInfoShootpoint :: FireEffects( void )
{
	if ( m_noiseCount )
	{
		if ( m_noiseCount > 1 )
		{
			switch ( RANDOM_LONG( 0, m_noiseCount - 1 ) )
			{
			case 1:
				EMIT_SOUND( edict(), CHAN_WEAPON, STRING(pev->noise1), RANDOM_FLOAT(0.92, 1.0), ATTN_NORM );
				break;
			case 2:
				EMIT_SOUND( edict(), CHAN_WEAPON, STRING(pev->noise2), RANDOM_FLOAT(0.92, 1.0), ATTN_NORM );
				break;
			case 3:
				EMIT_SOUND( edict(), CHAN_WEAPON, STRING(pev->noise3), RANDOM_FLOAT(0.92, 1.0), ATTN_NORM );
				break;
			default:
				EMIT_SOUND( edict(), CHAN_WEAPON, STRING(pev->noise), RANDOM_FLOAT(0.92, 1.0), ATTN_NORM );
				break;
			}
		}
		else
			EMIT_SOUND( edict(), CHAN_WEAPON, STRING(pev->noise), RANDOM_FLOAT(0.92, 1.0), ATTN_NORM );
	}

	if ( m_iszSpriteSmoke )
	{
		CSprite *pSprite = CSprite::SpriteCreate( STRING(m_iszSpriteSmoke), pev->origin, TRUE );
		pSprite->AnimateAndDie( RANDOM_FLOAT( 15.0, 20.0 ) );
		pSprite->SetTransparency( kRenderTransAlpha, 255, 255, 255, 255, kRenderFxNone );
		pSprite->pev->velocity.z = RANDOM_FLOAT(40, 80);
		pSprite->SetScale( m_spriteScale );
	}
	if ( m_iszSpriteFlash )
	{
		CSprite *pSprite = CSprite::SpriteCreate( STRING(m_iszSpriteFlash), pev->origin, TRUE );
		pSprite->AnimateAndDie( 60 );
		pSprite->SetTransparency( kRenderTransAdd, 255, 255, 255, 255, kRenderFxNoDissipation );
		pSprite->SetScale( m_spriteScale );

		// Hack Hack, make it stick around for at least 100 ms.
		pSprite->pev->nextthink += 0.1;
	}
	SUB_UseTargets( this, USE_TOGGLE, 0 );

	if ( !(pev->spawnflags & SF_SHOOTPOINT_NOREPORT) )
		ReportFire();

	m_fireLast = gpGlobals->time;
}


//
// info_shootpoint: bullets, "firenum" per shot
//
class CBulletShootpoint : public CInfoShootpoint	// name ours
{
public:
	const char *GetKillDesc( void )
	{
		if ( m_iszKillDesc )
			return STRING( m_iszKillDesc );

		switch ( m_bulletType )
		{
		case 2:
			return "buckshot gun";
		case 4:
			return "12 mm gun";
		}
		return "9mm gun";
	}
	const char *GetKillVerb( BOOL fGibbed, int &iHitGroup ) { return "shot"; }
	const char *GetHitGroupDesc( int iHitGroup ) { return HitGroupIn( iHitGroup ); }

	void	Fire( void );
};
LINK_ENTITY_TO_CLASS( info_shootpoint, CBulletShootpoint );


void CBulletShootpoint :: Fire( void )
{
	LinkThink();

	pev->nextthink = m_fireLast + m_fireRate * 2;

	UTIL_MakeVectors( pev->angles );

	int bulletCount = (gpGlobals->time - m_fireLast) / m_fireRate;
	if ( bulletCount > 0 )
	{
		for ( int i = 0; i < bulletCount; i++ )
		{
			switch( m_bulletType )
			{
			case 1:		// 9mm
				FireBullets( m_fireNum, pev->origin, gpGlobals->v_forward, Vector( m_spread, m_spread, m_spread ), 4096, BULLET_MONSTER_9MM, 1, pev->dmg, VARS( pev->owner ) );
				break;

			case 3:		// MP5
				FireBullets( m_fireNum, pev->origin, gpGlobals->v_forward, Vector( m_spread, m_spread, m_spread ), 4096, BULLET_MONSTER_MP5, 1, pev->dmg, VARS( pev->owner ) );
				break;

			case 2:		// shotgun
				FireBullets( m_fireNum, pev->origin, gpGlobals->v_forward, Vector( m_spread, m_spread, m_spread ), 4096, BULLET_MONSTER_BUCKSHOT, m_tracerRate, pev->dmg, VARS( pev->owner ) );
				break;

			case 4:		// 12mm
				FireBullets( m_fireNum, pev->origin, gpGlobals->v_forward, Vector( m_spread, m_spread, m_spread ), 4096, BULLET_MONSTER_12MM, 1, pev->dmg, VARS( pev->owner ) );
				break;
			}
		}
		FireEffects();
	}
}


//
// info_shootpoint_rocket: "bullet" 8 rockets, 9 laser guided, 10 heat
// seekers, 11 ballistic, 12 vortex missiles
//
#define SF_SPROCKET_CUSTOMMODEL	0x0002	// "Custom model" (names ours)
#define SF_SPROCKET_SPRITE		0x0004	// "Sprite model"
#define SF_SPROCKET_INCENDIARY	0x0008	// "Incendiary"
#define SF_SPROCKET_BLASTSPHERE	0x0010	// "Blast Sphere"

class CRocketShootpoint : public CInfoShootpoint
{
public:
	void	Precache( void );
	void	KeyValue( KeyValueData *pkvd );
	void	Fire( void );

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	float	m_seekTime;		// "seektime": the guided rockets start steering after it
};
LINK_ENTITY_TO_CLASS( info_shootpoint_rocket, CRocketShootpoint );

TYPEDESCRIPTION	CRocketShootpoint::m_SaveData[] =
{
	DEFINE_FIELD( CRocketShootpoint, m_seekTime, FIELD_FLOAT ),
};

IMPLEMENT_SAVERESTORE( CRocketShootpoint, CInfoShootpoint );


void CRocketShootpoint :: KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "seektime"))
	{
		m_seekTime = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CInfoShootpoint::KeyValue( pkvd );
}


void CRocketShootpoint :: Precache( void )
{
	UTIL_PrecacheOther( "rocket" );

	if ( HasSpawnFlag( SF_SPROCKET_CUSTOMMODEL ) )
	{
		if ( !pev->model )
			pev->model = MAKE_STRING( "models/HVR.mdl" );
		PRECACHE_MODEL( (char *)STRING( pev->model ) );
	}

	CInfoShootpoint::Precache();
}


void CRocketShootpoint :: Fire( void )
{
	LinkThink();

	pev->nextthink = gpGlobals->time + m_fireRate;

	if ( (int)( (gpGlobals->time - m_fireLast) * m_fireRate ) > 0 )	// sic: rate times, not divided
	{
		UTIL_MakeVectors( pev->angles );

		if ( pev->speed <= 0 )
			pev->speed = 1000;

		CRocket *pRocket;

		switch ( m_bulletType )
		{
		case 9:
			if ( HasSpawnFlag( SF_SPROCKET_CUSTOMMODEL ) )
				pRocket = CRocket::CreateRocket( pev->origin, UTIL_VecToAngles( ShootSpread( gpGlobals->v_forward, m_spread ) ), pev->owner, pev->rendercolor, pev->model );
			else
				pRocket = CRocket::CreateRocket( pev->origin, UTIL_VecToAngles( ShootSpread( gpGlobals->v_forward, m_spread ) ), pev->owner, pev->rendercolor, 0 );
			pRocket->m_flSeekTime = gpGlobals->time + m_seekTime;
			pRocket->m_flFieldOfView = pev->scale;
			pRocket->pev->speed = pev->speed;
			pRocket->SetThink( &CRocket::FollowThink );
			pRocket->m_iszKillDesc = MAKE_STRING( "a laser-guided rocket" );
			if ( HasSpawnFlag( SF_SPROCKET_INCENDIARY ) )
				pRocket->pev->spawnflags |= 1;
			if ( HasSpawnFlag( SF_SPROCKET_BLASTSPHERE ) )
				pRocket->pev->spawnflags |= 8;
			break;

		case 10:
			if ( HasSpawnFlag( SF_SPROCKET_CUSTOMMODEL ) )
				pRocket = CRocket::CreateRocket( pev->origin, UTIL_VecToAngles( ShootSpread( gpGlobals->v_forward, m_spread ) ), pev->owner, pev->rendercolor, pev->model );
			else
				pRocket = CRocket::CreateRocket( pev->origin, UTIL_VecToAngles( ShootSpread( gpGlobals->v_forward, m_spread ) ), pev->owner, pev->rendercolor, 0 );
			pRocket->m_flSeekTime = gpGlobals->time + m_seekTime;
			pRocket->pev->speed = pev->speed;
			pRocket->SetThink( &CRocket::SeekThink );
			pRocket->m_flFieldOfView = pev->scale;
			pRocket->m_iszKillDesc = MAKE_STRING( "a heat-seeking missile" );
			if ( HasSpawnFlag( SF_SPROCKET_INCENDIARY ) )
				pRocket->pev->spawnflags |= 1;
			if ( HasSpawnFlag( SF_SPROCKET_BLASTSPHERE ) )
				pRocket->pev->spawnflags |= 8;
			break;

		case 11:
			pRocket = CRocket::ShootArc( pev->owner, pev->origin, ShootSpread( gpGlobals->v_forward, m_spread ) * pev->speed, pev->rendercolor );
			pRocket->m_iszKillDesc = MAKE_STRING( "an artillery rocket" );
			if ( HasSpawnFlag( SF_SPROCKET_INCENDIARY ) )
				pRocket->pev->spawnflags |= 1;
			if ( HasSpawnFlag( SF_SPROCKET_BLASTSPHERE ) )
				pRocket->pev->spawnflags |= 8;
			break;

		case 12:
			pRocket = (CRocket *)CBaseEntity::Create( "vortex_missile", pev->origin, UTIL_VecToAngles( ShootSpread( gpGlobals->v_forward, m_spread ) ), pev->owner );
			pRocket->pev->velocity = gpGlobals->v_forward * 200;
			pRocket->m_flFieldOfView = pev->scale;
			break;

		default:
			if ( HasSpawnFlag( SF_SPROCKET_CUSTOMMODEL ) )
				pRocket = CRocket::CreateRocket( pev->origin, UTIL_VecToAngles( ShootSpread( gpGlobals->v_forward, m_spread ) ), pev->owner, pev->rendercolor, pev->model );
			else
				pRocket = CRocket::CreateRocket( pev->origin, UTIL_VecToAngles( ShootSpread( gpGlobals->v_forward, m_spread ) ), pev->owner, pev->rendercolor, 0 );
			pRocket->pev->speed = pev->speed;
			if ( HasSpawnFlag( SF_SPROCKET_INCENDIARY ) )
				pRocket->pev->spawnflags |= 1;
			if ( HasSpawnFlag( SF_SPROCKET_BLASTSPHERE ) )
				pRocket->pev->spawnflags |= 8;
			break;
		}

		if ( pev->dmg > 0 )
			pRocket->pev->dmg = pev->dmg;
		if ( m_iszKillDesc )
			pRocket->m_iszKillDesc = m_iszKillDesc;
		if ( HasSpawnFlag( SF_SPROCKET_SPRITE ) )
			pRocket->pev->effects &= ~EF_LIGHT;
		pRocket->pev->renderamt = pev->renderamt;
		pRocket->pev->renderfx = pev->renderfx;
		pRocket->pev->rendermode = pev->rendermode;

		FireEffects();
	}
}


//
// info_shootpoint_grenade: "bullet" 14 timed ("tracerate" seconds), 15
// hybrid, 16 proximity mines, others contact grenades, at "speed"
//
#define SF_GRENADE_TRAIL		0x0002	// "Smoke Trail" (names ours)
#define SF_GRENADE_INWARHEAD	0x0008	// "IN Warhead"
#define SF_GRENADE_BLASTSPHERE	0x0010

class CGrenadeShootpoint : public CInfoShootpoint	// name ours
{
public:
	void	Precache( void );
	void	Fire( void );

	void	SmokeTrail( short iEntIndex );	// name ours

	int		m_iTrail;	// name ours: the smoke trail sprite
};
LINK_ENTITY_TO_CLASS( info_shootpoint_grenade, CGrenadeShootpoint );


void CGrenadeShootpoint :: SmokeTrail( short iEntIndex )
{
	if ( pev->rendercolor == Vector( 0, 0, 0 ) )
		pev->rendercolor = Vector( 224, 224, 255 );

	MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
		WRITE_BYTE( TE_BEAMFOLLOW );
		WRITE_SHORT( iEntIndex );	// entity
		WRITE_SHORT( m_iTrail );	// model
		WRITE_BYTE( 40 ); // life
		WRITE_BYTE( 5 );  // width
		WRITE_BYTE( pev->rendercolor.x );   // r, g, b
		WRITE_BYTE( pev->rendercolor.y );   // r, g, b
		WRITE_BYTE( pev->rendercolor.z );   // r, g, b
		WRITE_BYTE( 255 );	// brightness
	MESSAGE_END();
}


void CGrenadeShootpoint :: Precache( void )
{
	UTIL_PrecacheOther( "grenade" );

	switch ( m_bulletType )
	{
	case 14:
		PRECACHE_MODEL( "models/w_grenade.mdl" );
		break;
	case 15:
		PRECACHE_MODEL( "models/pipebomb.mdl" );
		break;
	case 16:
		PRECACHE_MODEL( "models/ngrenade.mdl" );
		break;
	default:
		PRECACHE_MODEL( "models/grenade.mdl" );
		break;
	}

	if ( pev->spawnflags & SF_GRENADE_TRAIL )
		m_iTrail = PRECACHE_MODEL( "sprites/smoke.spr" );
	else
		m_iTrail = 0;

	CInfoShootpoint::Precache();
}


void CGrenadeShootpoint :: Fire( void )
{
	LinkThink();

	pev->nextthink = gpGlobals->time + m_fireRate;

	if ( (int)( (gpGlobals->time - m_fireLast) * m_fireRate ) > 0 )	// sic: rate times, not divided
	{
		UTIL_MakeAimVectors( pev->angles );

		CGrenade *pGrenade;

		switch ( m_bulletType )
		{
		case 14:
			pGrenade = CGrenade::ShootTimed( VARS( pev->owner ), pev->origin, ShootSpread( gpGlobals->v_forward, m_spread ) * pev->speed, m_tracerRate );
			if ( pev->dmg > 0 )
				pGrenade->pev->dmg = pev->dmg;
			if ( m_iTrail )
				SmokeTrail( ENTINDEX( pGrenade->edict() ) );
			if ( HasSpawnFlag( SF_GRENADE_INWARHEAD ) )
				pGrenade->pev->spawnflags |= 3;
			if ( HasSpawnFlag( SF_GRENADE_BLASTSPHERE ) )
				pGrenade->pev->spawnflags |= 8;
			break;

		case 15:
			pGrenade = CGrenade::ShootHybrid( VARS( pev->owner ), pev->origin, ShootSpread( gpGlobals->v_forward, m_spread ) * pev->speed, m_tracerRate );
			SET_MODEL( pGrenade->edict(), "models/pipebomb.mdl" );
			if ( pev->dmg > 0 )
				pGrenade->pev->dmg = pev->dmg;
			if ( m_iTrail )
				SmokeTrail( ENTINDEX( pGrenade->edict() ) );
			if ( HasSpawnFlag( SF_GRENADE_INWARHEAD ) )
				pGrenade->pev->spawnflags |= 3;
			if ( HasSpawnFlag( SF_GRENADE_BLASTSPHERE ) )
				pGrenade->pev->spawnflags |= 8;
			break;

		case 16:
			pGrenade = CGrenade::ShootMine( VARS( pev->owner ), pev->origin, ShootSpread( gpGlobals->v_forward, m_spread ) * pev->speed );
			SET_MODEL( pGrenade->edict(), "models/ngrenade.mdl" );
			if ( pev->dmg > 0 )
				pGrenade->pev->dmg = pev->dmg;
			if ( m_iTrail )
				SmokeTrail( ENTINDEX( pGrenade->edict() ) );
			pGrenade->m_iszKillDesc = MAKE_STRING( "a proximity mine" );
			if ( HasSpawnFlag( SF_GRENADE_INWARHEAD ) )
				pGrenade->pev->spawnflags |= 3;
			if ( HasSpawnFlag( SF_GRENADE_BLASTSPHERE ) )
				pGrenade->pev->spawnflags |= 8;
			break;

		default:
			pGrenade = CGrenade::ShootContact( VARS( pev->owner ), pev->origin, ShootSpread( gpGlobals->v_forward, m_spread ) * pev->speed );
			if ( pev->dmg > 0 )
				pGrenade->pev->dmg = pev->dmg;
			if ( m_iTrail )
				SmokeTrail( ENTINDEX( pGrenade->edict() ) );
			if ( HasSpawnFlag( SF_GRENADE_INWARHEAD ) )
				pGrenade->pev->spawnflags |= 3;
			if ( HasSpawnFlag( SF_GRENADE_BLASTSPHERE ) )
				pGrenade->pev->spawnflags |= 8;
			break;
		}

		FireEffects();
	}
}


//
// func_turretcontrols: func_tankcontrols for func_turrets
//
class CFuncTurretControls : public CBaseEntity
{
public:
	virtual int	ObjectCaps( void );
	void Spawn( void );
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void Think( void );

	virtual int	Save( CSave &save );
	virtual int	Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	CFuncTurret *m_pTank;
};
LINK_ENTITY_TO_CLASS( func_turretcontrols, CFuncTurretControls );

TYPEDESCRIPTION	CFuncTurretControls::m_SaveData[] =
{
	DEFINE_FIELD( CFuncTurretControls, m_pTank, FIELD_CLASSPTR ),
};

IMPLEMENT_SAVERESTORE( CFuncTurretControls, CBaseEntity );

int	CFuncTurretControls :: ObjectCaps( void )
{
	return (CBaseEntity :: ObjectCaps() & ~FCAP_ACROSS_TRANSITION) | FCAP_IMPULSE_USE;
}


void CFuncTurretControls :: Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{ // pass the Use command onto the controls
	if ( m_pTank )
		m_pTank->Use( pActivator, pCaller, useType, value );

	ASSERT( m_pTank != NULL );	// if this fails,  most likely means save/restore hasn't worked properly
}


void CFuncTurretControls :: Think( void )
{
	edict_t *pTarget = NULL;

	do
	{
		pTarget = FIND_ENTITY_BY_TARGETNAME( pTarget, STRING(pev->target) );
	} while ( !FNullEnt(pTarget) && strncmp( STRING(pTarget->v.classname), "func_turret", 9 ) );	// sic: 9

	if ( FNullEnt( pTarget ) )
	{
		ALERT( at_console, "No turret %s\n", STRING(pev->target) );
		return;
	}

	m_pTank = (CFuncTurret*)Instance(pTarget);
}

void CFuncTurretControls::Spawn( void )
{
	pev->solid = SOLID_TRIGGER;
	pev->movetype = MOVETYPE_NONE;
	pev->effects |= EF_NODRAW;
	SET_MODEL( ENT(pev), STRING(pev->model) );

	UTIL_SetSize( pev, pev->mins, pev->maxs );
	UTIL_SetOrigin( pev, pev->origin );

	pev->nextthink = gpGlobals->time + 0.3;	// After all the func_turret's have spawned

	CBaseEntity::Spawn();
}


// a shot for "Rotate on Fire"
void CInfoShootpoint :: ReportFire( void )
{
	if ( m_hLinked != NULL && FClassnameIs( m_hLinked.Get(), "func_turret" ) )
		((CFuncTurret *)(CBaseEntity *)m_hLinked)->m_justFired++;
}


//
// info_shootpoint_bolt: crossbow bolts, slower under water
//
class CBoltShootpoint : public CInfoShootpoint	// name ours
{
public:
	void	Precache( void );
	void	Fire( void );
};
LINK_ENTITY_TO_CLASS( info_shootpoint_bolt, CBoltShootpoint );


void CBoltShootpoint :: Precache( void )
{
	UTIL_PrecacheOther( "crossbow_bolt" );
	CInfoShootpoint::Precache();
}


void CBoltShootpoint :: Fire( void )
{
	LinkThink();

	pev->nextthink = gpGlobals->time + m_fireRate;

	int iShots = (gpGlobals->time - m_fireLast) * m_fireRate;	// sic: rate times, not divided
	if ( iShots > 0 )
	{
		UTIL_MakeAimVectors( pev->angles );

		for ( int i = 0; i < iShots * m_fireNum; i++ )
		{
			CCrossbowBolt *pBolt = CCrossbowBolt::BoltCreate();
			pBolt->pev->origin = pev->origin;
			pBolt->pev->owner = pev->owner;
			if ( pev->dmg > 0 )
				pBolt->pev->dmg = pev->dmg;

			if ( UTIL_PointContents( pev->origin ) == CONTENTS_WATER )
			{
				pBolt->pev->velocity = ShootSpread( gpGlobals->v_forward, m_spread ) * 1000;	// crossbow.cpp's BOLT_WATER_VELOCITY
				pBolt->pev->speed = 1000;
			}
			else
			{
				pBolt->pev->velocity = ShootSpread( gpGlobals->v_forward, m_spread ) * 2000;	// BOLT_AIR_VELOCITY
				pBolt->pev->speed = 2000;
			}
			pBolt->pev->angles = UTIL_VecToAngles( pBolt->pev->velocity );
			pBolt->pev->avelocity.z = 10;
		}

		FireEffects();
	}
}


//
// info_shootpoint_beam: aims and fires the beam_cannon named "message"
// (charged by the beam_charge named "netname") for "Duration" seconds; type
// 2 fires "firenum" separate shots in that time
//
class CBeamShootpoint : public CInfoShootpoint
{
public:
	void	Spawn( void );
	void	KeyValue( KeyValueData *pkvd );
	void	Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void	Fire( void );

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	CBeamCannon *GetBeam( void );	// names ours
	CBeamCharge *GetCharge( void );
	void	MoveBeam( void );
	void	FireBeam( void );

	void EXPORT WaitThink( void );
	void EXPORT BeamThink( void );

	int		m_type;
	int		m_shots;
	float	m_duration;
	float	m_shotDur;
	float	m_startTime;
	float	m_shotStartTime;
	Vector	m_spread1;
	CBeamCannon	*m_pBeam;
	CBeamCharge	*m_pCharge;
};
LINK_ENTITY_TO_CLASS( info_shootpoint_beam, CBeamShootpoint );

TYPEDESCRIPTION	CBeamShootpoint::m_SaveData[] =
{
	DEFINE_FIELD( CBeamShootpoint, m_type, FIELD_INTEGER ),
	DEFINE_FIELD( CBeamShootpoint, m_shots, FIELD_INTEGER ),
	DEFINE_FIELD( CBeamShootpoint, m_duration, FIELD_FLOAT ),
	DEFINE_FIELD( CBeamShootpoint, m_shotDur, FIELD_FLOAT ),
	DEFINE_FIELD( CBeamShootpoint, m_startTime, FIELD_TIME ),
	DEFINE_FIELD( CBeamShootpoint, m_shotStartTime, FIELD_TIME ),
	DEFINE_FIELD( CBeamShootpoint, m_spread1, FIELD_VECTOR ),
	DEFINE_FIELD( CBeamShootpoint, m_pBeam, FIELD_CLASSPTR ),
	DEFINE_FIELD( CBeamShootpoint, m_pCharge, FIELD_CLASSPTR ),
};

IMPLEMENT_SAVERESTORE( CBeamShootpoint, CInfoShootpoint );


void CBeamShootpoint :: Spawn( void )
{
	CInfoShootpoint::Spawn();

	if ( m_type == 2 )
	{
		if ( m_duration <= 0 )
			m_duration = 1;
		if ( m_fireNum <= 1 )
			m_fireNum = 3;
		m_shotDur = m_duration / (m_fireNum * 2 - 1);	// shots and the pauses between them
	}

	m_fireLast = gpGlobals->time;
}


void CBeamShootpoint :: KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "type"))
	{
		m_type = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "delay"))	// sic: the duration, not the delay
	{
		m_duration = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "Duration"))
	{
		m_duration = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CInfoShootpoint::KeyValue( pkvd );
}


void CBeamShootpoint :: Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if ( !ShouldToggle( useType, pev->nextthink != 0 ) )
		return;

	if ( pev->nextthink != 0 )
	{
		if ( m_pfnThink == static_cast <void (CBaseEntity::*)(void)> (FireThink) )
			pev->nextthink = 0;
		else
			pev->weapons = 1;	// stop when this beam is done
	}
	else if ( pev->weapons )
	{
		pev->weapons = 0;
	}
	else
	{
		m_fireLast = gpGlobals->time;
		pev->nextthink = gpGlobals->time + m_delay;
	}
}


CBeamCannon *CBeamShootpoint :: GetBeam( void )
{
	if ( !m_pBeam )
	{
		edict_t *pTarget = NULL;

		do
		{
			pTarget = FIND_ENTITY_BY_TARGETNAME( pTarget, STRING(pev->message) );
		} while ( !FNullEnt(pTarget) && !FClassnameIs( pTarget, "beam_cannon" ) );

		if ( !FNullEnt( pTarget ) )
			m_pBeam = (CBeamCannon*)Instance(pTarget);

		m_pBeam->pev->effects |= EF_BRIGHTLIGHT;	// sic: no beam_cannon found crashes here
	}

	return m_pBeam;
}


CBeamCharge *CBeamShootpoint :: GetCharge( void )
{
	if ( !m_pCharge )
	{
		edict_t *pTarget = NULL;

		do
		{
			pTarget = FIND_ENTITY_BY_TARGETNAME( pTarget, STRING(pev->netname) );
		} while ( !FNullEnt(pTarget) && !FClassnameIs( pTarget, "beam_charge" ) );

		if ( !FNullEnt( pTarget ) )
			m_pCharge = (CBeamCharge*)Instance(pTarget);
	}

	return m_pCharge;
}


void CBeamShootpoint :: Fire( void )
{
	if ( pev->weapons )
	{
		pev->weapons = 0;
		pev->nextthink = 0;
		return;
	}

	MoveBeam();

	if ( gpGlobals->time >= m_fireLast )
	{
		FireBeam();
		return;
	}

	if ( !m_pCharge || (m_pCharge->pev->effects & EF_NODRAW) )
	{
		if ( m_fireLast > gpGlobals->time )
			pev->nextthink = m_fireLast;
		else
			pev->nextthink = gpGlobals->time;
	}
}


// keeps the charge and the beam cannon with us
void CBeamShootpoint :: MoveBeam( void )
{
	LinkThink();

	if ( m_pCharge && !(m_pCharge->pev->effects & EF_NODRAW) )
		UTIL_SetOrigin( m_pCharge->pev, pev->origin );

	if ( m_pBeam && m_pBeam->pev->nextthink > 0 )
		m_pBeam->pev->origin = pev->origin;
}


void CBeamShootpoint :: FireBeam( void )
{
	UTIL_MakeAimVectors( pev->angles );

	// get circular gaussian spread
	float x, y;
	do {
		x = RANDOM_FLOAT(-0.5,0.5) + RANDOM_FLOAT(-0.5,0.5);
		y = RANDOM_FLOAT(-0.5,0.5) + RANDOM_FLOAT(-0.5,0.5);
	} while (x*x+y*y > 1);

	m_spread1 = x * m_spread * gpGlobals->v_right + y * m_spread * gpGlobals->v_up;

	GetBeam();
	if ( !m_pBeam )
	{
		SetThink( SUB_Remove );
		return;
	}

	m_pBeam->pev->owner = pev->owner;

	if ( pev->netname )
		GetCharge();

	m_pBeam->m_firePosition = pev->origin + gpGlobals->v_forward + m_spread1;
	m_pBeam->pev->angles = m_pBeam->m_firePosition;
	m_pBeam->m_firePosition = m_pBeam->pev->angles;

	if ( m_pCharge )
	{
		m_pCharge->WarmUp();
		SetThink( WaitThink );
	}
	else
	{
		m_pBeam->TurnOn();
		SetThink( BeamThink );
		m_shotStartTime = m_startTime = gpGlobals->time;
		if ( pev->noise )
			EMIT_SOUND( edict(), CHAN_STATIC, (char*)STRING(pev->noise), 0.85, ATTN_NORM );
	}

	pev->nextthink = gpGlobals->time;
}


// waits for the charge to turn the beam on
void CBeamShootpoint :: WaitThink( void )
{
	MoveBeam();

	if ( m_pBeam->IsOn() )
	{
		SetThink( BeamThink );
		m_shotStartTime = m_startTime = gpGlobals->time;
	}
}


void CBeamShootpoint :: BeamThink( void )
{
	MoveBeam();

	UTIL_MakeVectors( pev->angles );
	m_pBeam->m_firePosition = pev->origin + gpGlobals->v_forward + m_spread1;

	if ( (m_duration > 0 && m_startTime + m_duration < gpGlobals->time) || (m_duration <= 0 && pev->weapons) )
	{
		if ( m_pBeam->m_pfnThink == static_cast <void (CBaseEntity::*)(void)> (&CBeamCannon::StartFxThink) )
			m_pBeam->TurnOnBeam();

		if ( m_pCharge )
		{
			m_pCharge->CoolDown();
		}
		else
		{
			m_pBeam->TurnOff();
			if ( pev->noise )
				STOP_SOUND( edict(), CHAN_STATIC, (char*)STRING(pev->noise) );
		}

		SetThink( FireThink );
		m_fireLast = gpGlobals->time + RANDOM_FLOAT( 1.0, 1.3 ) * m_fireRate;
		return;
	}

	if ( m_type == 2 && m_shotStartTime + m_shotDur < gpGlobals->time )
	{
		m_shotStartTime = gpGlobals->time;

		if ( m_pBeam->IsOn() )
		{
			// between shots: aim the next one
			UTIL_MakeAimVectors( pev->angles );

			// get circular gaussian spread
			float x, y;
			do {
				x = RANDOM_FLOAT(-0.5,0.5) + RANDOM_FLOAT(-0.5,0.5);
				y = RANDOM_FLOAT(-0.5,0.5) + RANDOM_FLOAT(-0.5,0.5);
			} while (x*x+y*y > 1);

			m_spread1 = x * m_spread * gpGlobals->v_right + y * m_spread * gpGlobals->v_up;

			m_pBeam->TurnOffBeam();
		}
		else
			m_pBeam->TurnOnBeam();
	}
}


//
// info_shootpoint_laser: pulse lasers ("bullet" 21: they fall), "weapons"
// damage type, "tracerate" trail length
//
#define SF_LASER_EXPLOSIVE		0x0002	// "Explosive" (name ours)

class CLaserShootpoint : public CInfoShootpoint	// name ours
{
public:
	void	Precache( void );
	void	Fire( void );
};
LINK_ENTITY_TO_CLASS( info_shootpoint_laser, CLaserShootpoint );


void CLaserShootpoint :: Precache( void )
{
	UTIL_PrecacheOther( "laser_pulse" );
	CInfoShootpoint::Precache();

	pev->scale = (pev->scale < 0.25) ? 0.25 : pev->scale;
	pev->weapons = pev->iuser1;	// sic: "weapons" (the damage type) is lost
}


void CLaserShootpoint :: Fire( void )
{
	LinkThink();

	pev->nextthink = gpGlobals->time + m_fireRate;

	int iShots = (gpGlobals->time - m_fireLast) * m_fireNum * m_fireRate;	// sic: rate times, not divided
	if ( iShots > 0 )
	{
		UTIL_MakeVectors( pev->angles );

		for ( int i = 0; i < iShots; i++ )
		{
			CLaserPulse *pPulse = CLaserPulse::Shoot( pev->origin, pev->owner, pev->rendercolor, m_tracerRate, pev->scale, 0 );
			if ( m_bulletType == 21 )
				pPulse->pev->movetype = MOVETYPE_TOSS;
			if ( pev->spawnflags & SF_LASER_EXPLOSIVE )
				pPulse->pev->spawnflags |= 4;
			pPulse->pev->velocity = ShootSpread( gpGlobals->v_forward, m_spread ) * pev->speed;
			pPulse->pev->iuser2 = pev->weapons;
			pPulse->pev->dmg = pev->dmg;
			if ( m_iszKillDesc )
				pPulse->m_iszKillDesc = m_iszKillDesc;
		}

		FireEffects();
	}
}


//
// info_shootpoint_hornet: hornets
//
class CHornetShootpoint : public CInfoShootpoint	// name ours
{
public:
	void	Precache( void );
	void	Fire( void );
};
LINK_ENTITY_TO_CLASS( info_shootpoint_hornet, CHornetShootpoint );


void CHornetShootpoint :: Precache( void )
{
	UTIL_PrecacheOther( "hornet" );
	CInfoShootpoint::Precache();
}


void CHornetShootpoint :: Fire( void )
{
	LinkThink();

	pev->nextthink = gpGlobals->time + m_fireRate;

	int iShots = (gpGlobals->time - m_fireLast) * m_fireRate;	// sic: rate times, not divided
	if ( iShots > 0 )
	{
		UTIL_MakeAimVectors( pev->angles );

		for ( int i = 0; i < iShots * m_fireNum; i++ )
		{
			CBaseEntity *pHornet = CBaseEntity::Create( "hornet", pev->origin, pev->angles, pev->owner );
			pHornet->pev->velocity = ShootSpread( gpGlobals->v_forward, m_spread ) * 300;
			pHornet->pev->angles = UTIL_VecToAngles( pHornet->pev->velocity );
		}

		FireEffects();
	}
}


//
// cshell: info_shootpoint_cannon's shell, a contact grenade
//
class CCannonShell : public CGrenade
{
public:
	void	Spawn( void );
	const char *GetKillDesc( void ) { return "an artillery shell"; }

	void EXPORT ArcThink( void );
};
LINK_ENTITY_TO_CLASS( cshell, CCannonShell );


void CCannonShell :: Spawn( void )
{
	pev->movetype = MOVETYPE_FLY;
	pev->classname = MAKE_STRING( "cshell" );

	pev->solid = SOLID_BBOX;

	SET_MODEL(ENT(pev), "models/grenade.mdl");
	UTIL_SetSize(pev, Vector( 0, 0, 0), Vector(0, 0, 0));

	pev->dmg = 100;
	pev->gravity = 0.5;
	pev->nextthink = 0;
	SetTouch( ExplodeTouch );
}


// a ballistic shell faces where it flies
void CCannonShell :: ArcThink( void )
{
	pev->nextthink = gpGlobals->time + 0.1;
	pev->angles = UTIL_VecToAngles( pev->velocity );
}


//
// info_shootpoint_cannon: "bullet" 18 shells at "speed", 19 the same on a
// ballistic arc; 17 (hitscan) a shell where the line of fire hits
//
class CCannonShootpoint : public CGrenadeShootpoint	// name ours
{
public:
	void	Precache( void );
	void	Fire( void );
};
LINK_ENTITY_TO_CLASS( info_shootpoint_cannon, CCannonShootpoint );


void CCannonShootpoint :: Precache( void )
{
	UTIL_PrecacheOther( "cshell" );
	PRECACHE_MODEL( "models/grenade.mdl" );

	if ( pev->spawnflags & SF_GRENADE_TRAIL )
		m_iTrail = PRECACHE_MODEL( "sprites/smoke.spr" );
	else
		m_iTrail = 0;

	CInfoShootpoint::Precache();
}


void CCannonShootpoint :: Fire( void )
{
	LinkThink();

	pev->nextthink = gpGlobals->time + m_fireRate;

	if ( m_bulletType >= 18 && m_bulletType <= 19 )
	{
		if ( pev->speed <= 0 )
			pev->speed = 800;

		UTIL_MakeVectors( pev->angles );
		Vector vecDir = ShootSpread( gpGlobals->v_forward, m_spread );

		CBaseEntity *pShell = CBaseEntity::Create( "cshell", pev->origin, UTIL_VecToAngles( vecDir ), pev->owner );
		pShell->pev->velocity = vecDir * pev->speed;
		if ( pev->dmg > 0 )
			pShell->pev->dmg = pev->dmg;
		if ( m_iTrail )
			SmokeTrail( ENTINDEX( pShell->edict() ) );
		if ( HasSpawnFlag( SF_GRENADE_INWARHEAD ) )
			pShell->pev->spawnflags |= 3;
		if ( HasSpawnFlag( SF_GRENADE_BLASTSPHERE ) )
			pShell->pev->spawnflags |= 8;

		if ( m_bulletType == 19 )
		{
			pShell->pev->movetype = MOVETYPE_TOSS;
			pShell->SetThink( &CCannonShell::ArcThink );
			pShell->pev->nextthink = gpGlobals->time + 0.1;
		}
	}
	else
	{
		if ( pev->dmg <= 0 )
			pev->dmg = 100;

		UTIL_MakeVectors( pev->angles );
		Vector vecDir = ShootSpread( gpGlobals->v_forward, m_spread );

		TraceResult tr;
		UTIL_TraceLine( pev->origin, pev->origin + vecDir * 8192, dont_ignore_monsters, pev->owner, &tr );

		CBaseEntity *pShell = CBaseEntity::Create( "cshell", tr.vecEndPos, pev->angles, pev->owner );
		if ( pev->dmg > 0 )
			pShell->pev->dmg = pev->dmg;

		if ( m_iTrail )
		{
			if ( pev->rendercolor == Vector( 0, 0, 0 ) )
				pev->rendercolor = Vector( 224, 224, 255 );

			MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
				WRITE_BYTE( TE_BEAMPOINTS );
				WRITE_COORD( pev->origin.x );
				WRITE_COORD( pev->origin.y );
				WRITE_COORD( pev->origin.z );
				WRITE_COORD( tr.vecEndPos.x );
				WRITE_COORD( tr.vecEndPos.y );
				WRITE_COORD( tr.vecEndPos.z );
				WRITE_SHORT( m_iTrail );
				WRITE_BYTE( 0 ); // framerate
				WRITE_BYTE( 10 ); // framerate
				WRITE_BYTE( 5 ); // life
				WRITE_BYTE( 30 );  // width
				WRITE_BYTE( 0 );   // noise
				WRITE_BYTE( pev->rendercolor.x );   // r, g, b
				WRITE_BYTE( pev->rendercolor.y );   // r, g, b
				WRITE_BYTE( pev->rendercolor.z );   // r, g, b
				WRITE_BYTE( 224 );	// brightness
				WRITE_BYTE( 0 );		// speed
			MESSAGE_END();
		}

		pShell->pev->velocity = vecDir * 1000;
		if ( HasSpawnFlag( SF_GRENADE_INWARHEAD ) )
			pShell->pev->spawnflags |= 3;
		if ( HasSpawnFlag( SF_GRENADE_BLASTSPHERE ) )
			pShell->pev->spawnflags |= 8;
	}

	FireEffects();
}


//
// info_target_area: a brush whose BodyTarget is a random point inside it,
// for func_turret's "Random Target Mode"
//
class CTargetArea : public CBaseEntity	// name ours
{
public:
	int		ObjectCaps( void ) { return CBaseEntity :: ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }
	Vector	BodyTarget( const Vector &posSrc ) { return Vector( RANDOM_FLOAT( pev->absmin.x, pev->absmax.x ), RANDOM_FLOAT( pev->absmin.y, pev->absmax.y ), RANDOM_FLOAT( pev->absmin.z, pev->absmax.z ) ); }
};
LINK_ENTITY_TO_CLASS( info_target_area, CTargetArea );


//
// info_shootpoint_flame: a flamethrower ("weapons" set: acid, which falls),
// one flame per Fire
//
#define SF_FLAME_SILENT		0x0008	// "Silent" (name ours)

class CFlameShootpoint : public CInfoShootpoint
{
public:
	void	Spawn( void );
	void	Precache( void );
	void	Fire( void );
	void	Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

	unsigned short m_usFlameFire;	// names ours
	unsigned short m_usFlameStop;
};
LINK_ENTITY_TO_CLASS( info_shootpoint_flame, CFlameShootpoint );


void CFlameShootpoint :: Spawn( void )
{
	CInfoShootpoint::Spawn();

	SET_MODEL( ENT(pev), "models/grenade.mdl" );
	pev->renderamt = 0;
	pev->rendermode = kRenderTransAdd;
}


void CFlameShootpoint :: Precache( void )
{
	m_usFlameFire = PRECACHE_EVENT( 1, "events/flamefire.sc" );
	m_usFlameStop = PRECACHE_EVENT( 1, "events/flamestop.sc" );
}


void CFlameShootpoint :: Fire( void )
{
	LinkThink();
	UTIL_MakeVectors( pev->angles );

	CBaseEntity *pFlame = CBaseEntity::Create( "fly_flame", pev->origin, g_vecZero, pev->owner );
	pFlame->pev->velocity = gpGlobals->v_forward * 240;
	pFlame->pev->weapons = pev->weapons;
	if ( pFlame->pev->weapons )
	{
		pFlame->pev->movetype = MOVETYPE_TOSS;
		pFlame->pev->gravity = 0.1;
	}
}


void CFlameShootpoint :: Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if ( !ShouldToggle( useType, pev->nextthink != 0 ) )
		return;

	if ( pev->nextthink != 0 )
	{
		pev->nextthink = 0;
		if ( HasSpawnFlag( SF_FLAME_SILENT ) )
			PLAYBACK_EVENT_FULL( FEV_GLOBAL | FEV_RELIABLE, edict(), m_usFlameStop, 0.0, (float *)&pev->origin, (float *)&pev->angles, 0, 0, 0, 0, 0, 1 );
		else
			PLAYBACK_EVENT_FULL( FEV_GLOBAL | FEV_RELIABLE, edict(), m_usFlameStop, 0.0, (float *)&pev->origin, (float *)&pev->angles, 0, 0, 0, 0, 0, 0 );
	}
	else
	{
		pev->nextthink = gpGlobals->time + m_delay;
		if ( HasSpawnFlag( SF_FLAME_SILENT ) )
			PLAYBACK_EVENT_FULL( 0, edict(), m_usFlameFire, 0.0, (float *)&pev->origin, (float *)&pev->angles, 0, 0, pev->weapons, 0, 0, 1 );
		else
			PLAYBACK_EVENT_FULL( 0, edict(), m_usFlameFire, 0.0, (float *)&pev->origin, (float *)&pev->angles, 0, 0, pev->weapons, 0, 0, 0 );
	}

	m_fireLast = gpGlobals->time;
}


//
// info_laserspot: a laser spot where the line along our angles hits
// (laser-guided rockets follow it), with a beam to it ("Beam")
//
#define SF_LASERSPOT_STARTON	0x0001	// "Start On" (names ours)
#define SF_LASERSPOT_BEAM		0x0002	// "Beam"

class CLaserProjector : public CBaseLinked	// name ours
{
public:
	void	Spawn( void );
	void	Precache( void );
	int		ObjectCaps( void ) { return CBaseLinked :: ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }
	int		GetOnState( void ) { return pev->nextthink != 0; }
	void	Think( void );
	void	Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

	CLaserSpot	*m_pSpot;	// names ours
	CBeam		*m_pBeam;
};
LINK_ENTITY_TO_CLASS( info_laserspot, CLaserProjector );


void CLaserProjector :: Spawn( void )
{
	Precache();
	CBaseLinked::Spawn();

	if ( HasSpawnFlag( SF_LASERSPOT_BEAM ) )
	{
		// the beam's end: an invisible sprite, so the client knows the entity
		SET_MODEL( ENT( pev ), "sprites/laserdot.spr" );
		pev->renderamt = 0;
		pev->rendermode = kRenderTransAdd;
		UTIL_SetSize( pev, g_vecZero, g_vecZero );
	}
	else
		pev->effects = EF_NODRAW;

	if ( !(pev->spawnflags & SF_LASERSPOT_STARTON) && pev->targetname )
		pev->nextthink = 0;
	else
		pev->nextthink = gpGlobals->time + 0.1;
}


void CLaserProjector :: Precache( void )
{
	UTIL_PrecacheOther( "laser_spot" );
}


void CLaserProjector :: Think( void )
{
	LinkThink();
	pev->nextthink = gpGlobals->time + 0.05;

	if ( !m_pSpot )
	{
		m_pSpot = CLaserSpot::CreateSpot();
		if ( pev->scale > 0 )
			m_pSpot->pev->scale = pev->scale;
		if ( pev->rendercolor != Vector( 0, 0, 0 ) )
			m_pSpot->pev->rendercolor = pev->rendercolor;
	}

	if ( !m_pBeam && HasSpawnFlag( SF_LASERSPOT_BEAM ) )
	{
		m_pBeam = CBeam::BeamCreate( g_pModelNameLaser, 10 );
		m_pBeam->pev->spawnflags |= SF_BEAM_TEMPORARY;
		m_pBeam->EntsInit( entindex(), m_pSpot->entindex() );
		m_pBeam->SetBrightness( 100 );
		m_pBeam->pev->rendercolor = m_pSpot->pev->rendercolor;
	}

	UTIL_MakeVectors( pev->angles );

	edict_t *pentIgnore = edict();
	if ( m_hLinked != NULL )
		pentIgnore = m_hLinked->edict();

	TraceResult tr;
	UTIL_TraceLine( pev->origin, pev->origin + gpGlobals->v_forward * 8192, dont_ignore_monsters, pentIgnore, &tr );
	UTIL_SetOrigin( m_pSpot->pev, tr.vecEndPos );
}


void CLaserProjector :: Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if ( !ShouldToggle( useType, pev->nextthink != 0 ) )
		return;

	if ( pev->nextthink != 0 )
	{
		if ( m_pSpot )
		{
			m_pSpot->Killed( NULL, GIB_NEVER );
			m_pSpot = NULL;
		}
		if ( m_pBeam )
		{
			m_pBeam->Killed( NULL, GIB_NEVER );
			m_pBeam = NULL;
		}
		pev->nextthink = 0;
	}
	else
		pev->nextthink = gpGlobals->time + 0.05;
}

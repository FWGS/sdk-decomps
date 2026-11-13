/***
*
*	Counter-Strike 1.4, observer.cpp: the dead player's camera
*	(GetForceCamera, CBasePlayer::Observer_IsValidTarget,
*	Observer_FindNextPlayer, Observer_HandleButtons, Observer_CheckTarget,
*	Observer_CheckProperties, Observer_SetMode; Linux symbols). The mode is
*	pev->iuser1 (OBS_CHASE_LOCKED and OBS_CHASE_FREE behind a player,
*	OBS_ROAMING free flight, OBS_IN_EYE the player's own view, OBS_MAP_FREE
*	and OBS_MAP_CHASE the overview), the target's entity index pev->iuser2:
*	the engine's player movement and the client read them.
*
****/
#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"player.h"
#include	"weapons.h"

extern int gmsgCrosshair;
extern int gmsgCurWeapon;
extern int gmsgSetFOV;
extern int gmsgStatusIcon;
extern cvar_t fadetoblack;	// Counter-Strike 1.5: game.cpp's mp_fadetoblack (the declaration's place is ours)

// Counter-Strike 1.4: mp_forcechasecam, and when that is 0 the new
// mp_forcecamera (1: team mates only, no free flight; 2: only the team
// mates' own view). Windows has it inline in its two callers.
int GetForceCamera( void )
{
	// Counter-Strike 1.5: with mp_fadetoblack only the team mates' own view (the two returns and
	// the declaration behind the first are Counter-Strike 1.6's lines: code at 38, 39, 41, 43, 44)
	if ( fadetoblack.value != 0 )
		return 2;

	int retVal = CVAR_GET_FLOAT( "mp_forcechasecam" );

	if ( !retVal )
		retVal = CVAR_GET_FLOAT( "mp_forcecamera" );

	return retVal;
}

// Counter-Strike 1.4: the player of that index if this observer may watch
// him (inline in Observer_FindNextPlayer on Windows)
CBaseEntity *CBasePlayer::Observer_IsValidTarget( int iPlayerIndex, bool bSameTeam )
{
	if ( iPlayerIndex > gpGlobals->maxClients || iPlayerIndex < 1 )
		return NULL;

	CBaseEntity *pEnt = UTIL_PlayerByIndex( iPlayerIndex );

	// Don't spec observers or players who haven't picked a class yet
	if ( !pEnt || pEnt == this || pEnt->has_disconnected || ((CBasePlayer*)pEnt)->pev->iuser1 || (pEnt->pev->effects & EF_NODRAW) || ( bSameTeam && ((CBasePlayer*)pEnt)->m_iTeam != m_iTeam ) )
		return NULL;

	return pEnt;
}

// Find the next client in the game for this player to spectate
void CBasePlayer::Observer_FindNextPlayer( bool bReverse )
{
	int		iStart;
	if ( m_hObserverTarget )
		iStart = ENTINDEX( m_hObserverTarget->edict() );
	else
		iStart = ENTINDEX( edict() );
	int	    iCurrent = iStart;
	m_hObserverTarget = NULL;
	int iDir = bReverse ? -1 : 1; 

	// a spectator (allow_spectators) may watch anybody whatever the cvars say
	bool bForceSameTeam = ( GetForceCamera() != 0 && m_iTeam != TEAM_SPECTATOR );

	do
	{
		iCurrent += iDir;

		// Loop through the clients
		if (iCurrent > gpGlobals->maxClients)
			iCurrent = 1;
		else if (iCurrent < 1)
			iCurrent = gpGlobals->maxClients;

		m_hObserverTarget = Observer_IsValidTarget( iCurrent, bForceSameTeam );

		if ( m_hObserverTarget )
			break;

	} while ( iCurrent != iStart );

	// Did we find a target?
	if ( m_hObserverTarget )
	{
		// Move to the target
		UTIL_SetOrigin( pev, m_hObserverTarget->pev->origin );

		// Store the target in pev so the physics DLL can get to it
		if (pev->iuser1 != OBS_ROAMING)
			pev->iuser2 = ENTINDEX( m_hObserverTarget->edict() );

		if (pev->iuser1 == OBS_IN_EYE)
			SetProgressBarTime( 0 );
	}
}

// Handle buttons in observer mode
void CBasePlayer::Observer_HandleButtons()
{
	// Slow down mouse clicks
	if ( m_flNextObserverInput > gpGlobals->time )
		return;

	// Jump changes from modes: Chase to Roaming
	if ( m_afButtonPressed & IN_JUMP )
	{
		if ( pev->iuser1 == OBS_CHASE_LOCKED )
			Observer_SetMode( OBS_CHASE_FREE );
		else if ( pev->iuser1 == OBS_CHASE_FREE )
			Observer_SetMode( OBS_IN_EYE );
		else if ( pev->iuser1 == OBS_IN_EYE )
			Observer_SetMode( OBS_ROAMING );
		else if ( pev->iuser1 == OBS_ROAMING )
			Observer_SetMode( OBS_MAP_FREE );
		else if ( pev->iuser1 == OBS_MAP_FREE )
			Observer_SetMode( OBS_MAP_CHASE );
		else
			Observer_SetMode( OBS_CHASE_FREE );

		m_flNextObserverInput = gpGlobals->time + 0.2;
	}

	// Attack moves to the next player
	if ( m_afButtonPressed & IN_ATTACK )
	{
		Observer_FindNextPlayer( false );

		m_flNextObserverInput = gpGlobals->time + 0.2;
	}

	// Attack2 moves to the prev player
	if ( m_afButtonPressed & IN_ATTACK2 )
	{
		Observer_FindNextPlayer( true );

		m_flNextObserverInput = gpGlobals->time + 0.2;
	}
}

// Counter-Strike 1.4: every frame of an observer (PreThink): another target
// when the watched player left, respawned or has been dead for two seconds
void CBasePlayer::Observer_CheckTarget()
{
	if ( pev->iuser1 == OBS_ROAMING )
		return;

	// try to find a target if we have no current one
	if ( m_hObserverTarget == NULL )
	{
		Observer_FindNextPlayer( false );

		if ( m_hObserverTarget == NULL )
		{
			// nobody to watch: free flight, and the mode asked for is kept
			int lastMode = pev->iuser1;

			Observer_SetMode( OBS_ROAMING );

			m_iObserverLastMode = lastMode;
			return;
		}
	}

	CBasePlayer *target = (CBasePlayer *)UTIL_PlayerByIndex( ENTINDEX( m_hObserverTarget->edict() ) );

	if ( !target || target->pev->deadflag == DEAD_RESPAWNABLE || (target->pev->effects & EF_NODRAW) )
	{
		Observer_FindNextPlayer( false );
	}
	else if ( target->pev->deadflag == DEAD_DEAD && target->m_fDeadTime + 2.0f < gpGlobals->time )
	{
		Observer_FindNextPlayer( false );
	}
}

// Counter-Strike 1.4: an observer in the watched player's own view gets his
// field of view, his weapon's crosshair and his bomb and defuser icons
void CBasePlayer::Observer_CheckProperties()
{
	if ( pev->iuser1 == OBS_IN_EYE && m_hObserverTarget != NULL )
	{
		CBasePlayer *target = (CBasePlayer *)UTIL_PlayerByIndex( ENTINDEX( m_hObserverTarget->edict() ) );

		if ( !target )
			return;

		int weapon = target->m_pActiveItem ? target->m_pActiveItem->m_iId : 0;

		// use the watched player's field of view
		if ( m_iFOV != target->m_iFOV || m_iObserverWeapon != weapon )
		{
			m_iFOV = target->m_iFOV;
			m_iClientFOV = m_iFOV;

			MESSAGE_BEGIN( MSG_ONE, gmsgSetFOV, NULL, pev );
				WRITE_BYTE( m_iFOV );
			MESSAGE_END();

			m_iObserverWeapon = weapon;

			MESSAGE_BEGIN( MSG_ONE, gmsgCurWeapon, NULL, pev );
				WRITE_BYTE( 1 );
				WRITE_BYTE( m_iObserverWeapon );
				WRITE_BYTE( 0 );
			MESSAGE_END();
		}

		int targetBombState = 0;

		if ( target->m_bHasC4 )
		{
			if ( target->m_signals.GetState() & 2 )	// at a bomb target
				targetBombState = 2;
			else
				targetBombState = 1;
		}

		if ( m_iObserverC4State != targetBombState )
		{
			m_iObserverC4State = targetBombState;

			if ( targetBombState )
			{
				MESSAGE_BEGIN( MSG_ONE, gmsgStatusIcon, NULL, pev );
					WRITE_BYTE( m_iObserverC4State );
					WRITE_STRING( "c4" );
					WRITE_BYTE( 0 );
					WRITE_BYTE( 160 );
					WRITE_BYTE( 0 );
				MESSAGE_END();
			}
			else
			{
				MESSAGE_BEGIN( MSG_ONE, gmsgStatusIcon, NULL, pev );
					WRITE_BYTE( 0 );
					WRITE_STRING( "c4" );
				MESSAGE_END();
			}
		}

		if ( m_bObserverHasDefuser != target->m_fHasDefuser )
		{
			m_bObserverHasDefuser = target->m_fHasDefuser;

			if ( m_bObserverHasDefuser )
			{
				MESSAGE_BEGIN( MSG_ONE, gmsgStatusIcon, NULL, pev );
					WRITE_BYTE( 1 );
					WRITE_STRING( "defuser" );
					WRITE_BYTE( 0 );
					WRITE_BYTE( 160 );
					WRITE_BYTE( 0 );
				MESSAGE_END();
			}
			else
			{
				MESSAGE_BEGIN( MSG_ONE, gmsgStatusIcon, NULL, pev );
					WRITE_BYTE( 0 );
					WRITE_STRING( "defuser" );
				MESSAGE_END();
			}
		}
	}
	else
	{
		m_iFOV = 90;

		if ( m_iObserverWeapon )
		{
			m_iObserverWeapon = 0;

			MESSAGE_BEGIN( MSG_ONE, gmsgCurWeapon, NULL, pev );
				WRITE_BYTE( 1 );
				WRITE_BYTE( m_iObserverWeapon );
				WRITE_BYTE( 0 );
			MESSAGE_END();
		}

		if ( m_iObserverC4State )
		{
			m_iObserverC4State = 0;

			MESSAGE_BEGIN( MSG_ONE, gmsgStatusIcon, NULL, pev );
				WRITE_BYTE( 0 );
				WRITE_STRING( "c4" );
			MESSAGE_END();
		}

		if ( m_bObserverHasDefuser )
		{
			m_bObserverHasDefuser = false;

			MESSAGE_BEGIN( MSG_ONE, gmsgStatusIcon, NULL, pev );
				WRITE_BYTE( 0 );
				WRITE_STRING( "defuser" );
			MESSAGE_END();
		}
	}
}

// Attempt to change the observer mode
void CBasePlayer::Observer_SetMode( int iMode )
{
	// Just abort if we're changing to the mode we're already in
	if ( iMode == pev->iuser1 )
		return;

	int forcecamera = GetForceCamera();

	// is valid mode ?
	if ( iMode < OBS_CHASE_LOCKED || iMode > OBS_MAP_CHASE )
		iMode = OBS_IN_EYE;

	int oldMode = pev->iuser1;	// Counter-Strike 1.6 uses it; no code here

	// a spectator (allow_spectators) does what he likes
	if ( m_iTeam != TEAM_SPECTATOR )
	{
		// no free flight with 1, the team mates' own view only with 2
		if ( forcecamera == 1 && iMode == OBS_ROAMING )
			iMode = OBS_MAP_FREE;
		else if ( forcecamera == 2 )
			iMode = OBS_IN_EYE;
	}

	// verify observer target again
	if ( m_hObserverTarget != NULL )
	{
		CBaseEntity *pEnt = m_hObserverTarget;

		// four tests, each with its own statement (Counter-Strike 1.6's lines: tests at 535, 537,
		// 539 and 541, the statement left at 542). The code is one merged store either way, but the
		// compiler chooses registers before it merges: with two or three statements 1.5's iMode and
		// forcecamera exchange ebx and ebp (1.4's code is the same with any number).
		if ( pEnt == this || !pEnt )
			m_hObserverTarget = NULL;
		else if ( pEnt->has_disconnected )
			m_hObserverTarget = NULL;
		else if ( ((CBasePlayer*)pEnt)->pev->iuser1 || (pEnt->pev->effects & EF_NODRAW) )
			m_hObserverTarget = NULL;
		else if ( forcecamera && ((CBasePlayer*)pEnt)->m_iTeam != m_iTeam )
			m_hObserverTarget = NULL;
	}

	// set spectator mode
	pev->iuser1 = iMode;

	// if we are not roaming, we need a valid target to track
	if ( iMode != OBS_ROAMING && m_hObserverTarget == NULL )
	{
		Observer_FindNextPlayer( false );

		// if we didn't find a valid target switch to roaming
		if ( m_hObserverTarget == NULL )
		{
			ClientPrint( pev, HUD_PRINTCENTER, "#Spec_NoTarget"  );
			pev->iuser1 = OBS_ROAMING;
		}
	}

	// set target if not roaming
	if ( pev->iuser1 == OBS_ROAMING )
		pev->iuser2 = 0;
	else
		pev->iuser2 = ENTINDEX( m_hObserverTarget->edict() );

	pev->iuser3 = 0;

	if ( m_hObserverTarget )
		UTIL_SetOrigin( pev, m_hObserverTarget->pev->origin );

	MESSAGE_BEGIN( MSG_ONE, gmsgCrosshair, NULL, pev );
		if ( iMode == OBS_ROAMING )
			WRITE_BYTE( 1 );
		else
			WRITE_BYTE( 0 );
	MESSAGE_END();

	SetProgressBarTime( 0 );

	char modemsg[16];
	sprintf( modemsg, "#Spec_Mode%i", pev->iuser1 );
	ClientPrint( pev, HUD_PRINTCENTER, modemsg );

	m_iObserverLastMode = iMode;
}

/***
*
*	Counter-Strike Beta 6.5, observer.cpp: the dead player's camera
*	(CBasePlayer::Observer_FindNextPlayer, Observer_HandleButtons,
*	Observer_SetMode; Linux symbols). The mode is pev->iuser1 (OBS_ROAMING
*	free flight, OBS_CHASE_LOCKED and OBS_CHASE_FREE behind a player), the
*	target's entity index pev->iuser2: the engine's player movement and the
*	client read them. Beta 6.1's GhostMode, ChaseCamMode and GetPlayerToChase
*	(player.cpp) are gone.
*
****/
#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"player.h"
#include	"weapons.h"

extern int gmsgCrosshair;

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

	// Counter-Strike 1.1: a spectator (allow_spectators) may watch anybody
	// whatever mp_forcechasecam says, and the two loops stand the other way
	// round (1.0b: if ( ... == 0 ) anybody, else team mates)
	if ( CVAR_GET_FLOAT( "mp_forcechasecam" ) != 0 && m_iTeam != TEAM_SPECTATOR )
	{
		// mp_forcechasecam: team mates only
		do
		{
			iCurrent += iDir;

			// Loop through the clients
			if (iCurrent > gpGlobals->maxClients)
				iCurrent = 1;
			if (iCurrent < 1)
				iCurrent = gpGlobals->maxClients;

			CBaseEntity *pEnt = UTIL_PlayerByIndex( iCurrent );
			if ( !pEnt )
				continue;
			if ( pEnt == this )
				continue;
			if ( pEnt->has_disconnected )
				continue;
			// Don't spec observers or players who haven't picked a class yet
			if ( ((CBasePlayer*)pEnt)->pev->iuser1 || (pEnt->pev->effects & EF_NODRAW) )
				continue;
			if ( ((CBasePlayer*)pEnt)->m_iTeam != m_iTeam )
				continue;

			m_hObserverTarget = pEnt;
			break;

		} while ( iCurrent != iStart );
	}
	else
	{
		// anybody who is in the game
		do
		{
			iCurrent += iDir;

			// Loop through the clients
			if (iCurrent > gpGlobals->maxClients)
				iCurrent = 1;
			if (iCurrent < 1)
				iCurrent = gpGlobals->maxClients;

			CBaseEntity *pEnt = UTIL_PlayerByIndex( iCurrent );
			if ( !pEnt )
				continue;
			if ( pEnt == this )
				continue;
			if ( pEnt->has_disconnected )
				continue;
			// Don't spec observers or players who haven't picked a class yet
			if ( ((CBasePlayer*)pEnt)->pev->iuser1 || (pEnt->pev->effects & EF_NODRAW) )
				continue;

			m_hObserverTarget = pEnt;
			break;

		} while ( iCurrent != iStart );
	}

	// Did we find a target?
	if ( m_hObserverTarget )
	{
		// Store the target in pev so the physics DLL can get to it
		if (pev->iuser1 != OBS_ROAMING)
			pev->iuser2 = ENTINDEX( m_hObserverTarget->edict() );
		// Move to the target
		UTIL_SetOrigin( pev, m_hObserverTarget->pev->origin );

		ALERT( at_console, "Now Tracking %s\n", STRING( m_hObserverTarget->pev->netname ) );
	}
	else
	{
		ALERT( at_console, "No observer targets.\n" );
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
		if ( pev->iuser1 == OBS_ROAMING )
			Observer_SetMode( OBS_CHASE_LOCKED );
		else if ( pev->iuser1 == OBS_CHASE_LOCKED )
			Observer_SetMode( OBS_CHASE_FREE );
		else
			Observer_SetMode( OBS_ROAMING );

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

// Attempt to change the observer mode
void CBasePlayer::Observer_SetMode( int iMode )
{
	// Just abort if we're changing to the mode we're already in
	if ( iMode == pev->iuser1 )
		return;

	// Changing to Roaming?
	if ( iMode == OBS_ROAMING )
	{
		// mp_forcechasecam: no free flight while there is somebody to follow
		// Counter-Strike 1.1: a spectator (allow_spectators) flies freely
		// whatever the cvar says; the free flight stands once in the text
		// (1.0b: twice, the first for mp_forcechasecam 0)
		if ( CVAR_GET_FLOAT( "mp_forcechasecam" ) != 0 && m_iTeam != TEAM_SPECTATOR )
		{
			if ( m_hObserverTarget == NULL )
				Observer_FindNextPlayer( false );

			if (m_hObserverTarget)
			{
				pev->iuser1 = OBS_CHASE_LOCKED;
				pev->iuser2 = ENTINDEX( m_hObserverTarget->edict() );

				MESSAGE_BEGIN( MSG_ONE, gmsgCrosshair, NULL, pev );
					WRITE_BYTE( 0 );
				MESSAGE_END();

				ClientPrint( pev, HUD_PRINTCENTER, "#Spec_Mode1" );
				return;
			}
		}

		pev->iuser1 = OBS_ROAMING;
		pev->iuser2 = 0;

		MESSAGE_BEGIN( MSG_ONE, gmsgCrosshair, NULL, pev );
			WRITE_BYTE( 1 );
		MESSAGE_END();

		ClientPrint( pev, HUD_PRINTCENTER, "#Spec_Mode3" );
		return;
	}

	// Changing to Chase Lock?
	if ( iMode == OBS_CHASE_LOCKED )
	{
		// If changing from Roaming, or starting observing, make sure there is a target
		if ( m_hObserverTarget == NULL )
			Observer_FindNextPlayer( false );

		if (m_hObserverTarget)
		{
			pev->iuser1 = OBS_CHASE_LOCKED;
			pev->iuser2 = ENTINDEX( m_hObserverTarget->edict() );

			MESSAGE_BEGIN( MSG_ONE, gmsgCrosshair, NULL, pev );
				WRITE_BYTE( 0 );
			MESSAGE_END();

			ClientPrint( pev, HUD_PRINTCENTER, "#Spec_Mode1" );
		}
		else
		{
			ClientPrint( pev, HUD_PRINTCENTER, "#Spec_NoTarget"  );
			Observer_SetMode(OBS_ROAMING);
		}

		return;
	}

	// Changing to Chase Freelook?
	if ( iMode == OBS_CHASE_FREE )
	{
		// If changing from Roaming, or starting observing, make sure there is a target
		if ( m_hObserverTarget == NULL )
			Observer_FindNextPlayer( false );

		if (m_hObserverTarget)
		{
			pev->iuser1 = OBS_CHASE_FREE;
			pev->iuser2 = ENTINDEX( m_hObserverTarget->edict() );

			MESSAGE_BEGIN( MSG_ONE, gmsgCrosshair, NULL, pev );
				WRITE_BYTE( 0 );
			MESSAGE_END();

			ClientPrint( pev, HUD_PRINTCENTER, "#Spec_Mode2" );
		}
		else
		{
			ClientPrint( pev, HUD_PRINTCENTER, "#Spec_NoTarget"  );
			Observer_SetMode(OBS_ROAMING);
		}
		
		return;
	}
}

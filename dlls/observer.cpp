/***
*
*	Counter-Strike Beta 5.0 (leak), observer.cpp: the dead player's observer
*	(observer, CObserver: Beta 5.0's Linux symbols; observer.h).
*
*	Every player has one (CBasePlayer::Spawn creates it); when he dies and
*	the dead camera starts, StartObserver gives it to him. In chase mode
*	(mp_chasecam) his view follows a living player, from 112 units behind the
*	player's eyes along his own view direction; fire picks the next player,
*	jump switches to free mode (where the player flies), where fire jumps to
*	the next player's place and jump goes back to chase mode. A team mate
*	only, while mp_chasecam is set. Observers see each other as ghosts.
*
****/

// No weapons.h: with it the guard of GetChaseOrigin's static is VC6's $S14,
// not $S9, and VC6 lays the static out after its guard; retail has it before.
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "nodes.h"
#include "player.h"
#include "gamerules.h"
#include "observer.h"

extern DLL_GLOBAL short g_sModelIndexCTGhost;
extern DLL_GLOBAL short g_sModelIndexTGhost;
extern int gmsgCrosshair;	// Beta 5.0 (player.cpp)

LINK_ENTITY_TO_CLASS( observer, CObserver );

CObserver::CObserver()
{
	m_pOwner = NULL;
}

CObserver *CObserver::Create( void )
{
	return (CObserver *)CBaseEntity::Create( "observer", Vector(), Vector(), NULL );
}

void CObserver::Precache( void )
{
	PRECACHE_MODEL( "models/agibs.mdl" );
}

void CObserver::Spawn( void )
{
	SetOwner( NULL );

	pev->solid = SOLID_NOT;
	pev->renderfx = kRenderFxNone;
	pev->rendermode = kRenderTransAdd;
	pev->renderamt = 0;
	pev->nextthink = gpGlobals->time + 0.001;

	m_iTarget = 0;
	m_pLastTarget = NULL;
	m_iLastButtons = 0;

	SET_MODEL( ENT( pev ), "models/agibs.mdl" );
	SET_SIZE( ENT( pev ), Vector( -8, -8, -8 ), Vector( 8, 8, 8 ) );

	m_ghosts.Reset();
}

void CObserver::SetOwner( CBasePlayer *pOwner )
{
	CBasePlayer *pOldOwner = m_pOwner;	// name ours

	m_pOwner = pOwner;

	if ( pOwner )
	{
		m_ghosts.SetHintMessage();
		SET_ORIGIN( edict(), m_pOwner->pev->origin + m_pOwner->pev->view_ofs );

		if ( CVAR_GET_FLOAT( "mp_chasecam" ) != 0 )
			SetChaseMode( 1 );	// Beta 5.0: locked
		else
			SetFreeMode();
	}
	else
	{
		if ( pOldOwner )
			SET_VIEW( pOldOwner->edict(), pOldOwner->edict() );

		SetThink( NULL );
	}
}

void CObserver::NextClient( void )
{
	int iNext = FindNextClient( m_iTarget + 1, FALSE );	// name ours

	if ( !iNext )
		iNext = FindNextClient( m_iTarget + 1, TRUE );

	m_iTarget = iNext;
}

// a player who can be watched
static BOOL IsValidTarget( CBaseEntity *pTarget )	// name ours
{
	// Beta 5.0: a player with a name (one still connecting has none)
	if ( pTarget && !FNullEnt( pTarget->edict() ) && STRING( pTarget->pev->netname )[0] && pTarget->IsAlive() )
		return TRUE;

	return FALSE;
}

void CObserver::ChaseMoveThink( void )
{
	if ( !m_iTarget )
		NextClient();

	HandleButtons();
	m_ghosts.Update( m_pOwner, pev->origin );

	if ( m_iTarget )
	{
		CBaseEntity *pTarget = UTIL_PlayerByIndex( m_iTarget );	// name ours

		if ( IsValidTarget( pTarget ) )
		{
			if ( pTarget != m_pLastTarget )
			{
				SET_VIEW( m_pOwner->edict(), edict() );
				m_pLastTarget = pTarget;
			}

			// Beta 5.0: the owner stands where the view is too (the
			// leak moved the observer only); locked, the view follows the
			// target's yaw from 42 units higher and stands still
			if ( m_cChaseMode == 0 )
			{
				const Vector &vecOrigin = GetChaseOrigin( m_pOwner->pev->v_angle, pTarget->pev, Vector( 0, 0, 0 ) );	// names ours

				SET_ORIGIN( edict(), vecOrigin );
				SET_ORIGIN( m_pOwner->edict(), vecOrigin );
				pev->angles = m_pOwner->pev->v_angle;
				pev->velocity = pTarget->pev->velocity;
			}
			else if ( m_cChaseMode == 1 )
			{
				Vector vecAngles = pTarget->pev->v_angle;
				vecAngles.x = 0;

				const Vector &vecOrigin = GetChaseOrigin( vecAngles, pTarget->pev, Vector( 0, 0, 42 ) );

				SET_ORIGIN( edict(), vecOrigin );
				SET_ORIGIN( m_pOwner->edict(), vecOrigin );
				pev->angles = pTarget->pev->v_angle;
				pev->angles.x = 0;
				pev->velocity = g_vecZero;
			}
		}
		else
		{
			// Beta 5.0: 3 seconds (the leak: 1), standing still
			m_flNextChase = gpGlobals->time + 3;
			pev->velocity = Vector( 0, 0, 0 );
			SetThink( ChaseMoveDelayThink );
		}

		pev->nextthink = gpGlobals->time + 0.001;
	}
	else
	{
		ALERT( at_console, "No valid clients available, switching observer to free mode\n" );
		SetFreeMode();
	}
}

void CObserver::ChaseMoveDelayThink( void )
{
	HandleButtons();
	m_ghosts.Update( m_pOwner, m_pOwner->pev->origin );

	if ( gpGlobals->time > m_flNextChase )
	{
		NextClient();
		SetThink( ChaseMoveThink );
	}

	pev->nextthink = gpGlobals->time + 0.001;
}

void CObserver::FreeMoveThink( void )
{
	HandleButtons();
	m_ghosts.Update( m_pOwner, m_pOwner->pev->origin );
	pev->nextthink = gpGlobals->time + 0.001;
}

// the next living player from iStart on (the owner's team mates, or with
// bSameTeam set the others: sic, the flag says the opposite), 0 if none
int CObserver::FindNextClient( int iStart, BOOL bSameTeam )
{
	if ( !m_pOwner || FNullEnt( m_pOwner->edict() ) )
		return 0;

	int iClient = ( iStart - 1 ) % ( gpGlobals->maxClients + 1 ) + 1;	// names ours

	do
	{
		CBasePlayer *pPlayer = (CBasePlayer *)UTIL_PlayerByIndex( iClient );

		if ( IsValidTarget( pPlayer ) )
		{
			BOOL bMatch;

			if ( bSameTeam )
				bMatch = ( m_pOwner->m_iTeam != pPlayer->m_iTeam );
			else
				bMatch = ( m_pOwner->m_iTeam == pPlayer->m_iTeam );

			if ( bMatch )
				return iClient;
		}

		iClient = iClient % ( gpGlobals->maxClients + 1 ) + 1;
	}
	while ( iClient != iStart );

	return 0;
}

void CObserver::HandleButtons( void )
{
	if ( !m_pOwner )
		return;

	int iButtons = m_pOwner->pev->button;	// name ours

	if ( m_pfnThink == &CObserver::FreeMoveThink )
	{
		if ( ( iButtons & IN_JUMP ) && !( m_iLastButtons & IN_JUMP ) )
		{
			if ( !m_iTarget )
				NextClient();

			if ( m_iTarget )
				SetChaseMode( 1 );
			else
				ClientPrint( m_pOwner->pev, HUD_PRINTCENTER, "No valid targets\nCan not switch to CHASE MODE" );
		}
		else if ( ( iButtons & IN_ATTACK ) && !( m_iLastButtons & IN_ATTACK ) )
		{
			NextClient();

			if ( m_iTarget )
			{
				CBaseEntity *pTarget = UTIL_PlayerByIndex( m_iTarget );

				if ( IsValidTarget( pTarget ) )
					SET_ORIGIN( m_pOwner->edict(), GetChaseOrigin( m_pOwner->pev->v_angle, pTarget->pev, Vector( 0, 0, 0 ) ) );
			}
		}
	}
	else if ( m_pfnThink == &CObserver::ChaseMoveThink )
	{
		if ( ( iButtons & IN_JUMP ) && !( m_iLastButtons & IN_JUMP ) )
		{
			// Beta 5.0: locked goes to free look; free look goes to the free
			// mode, or with mp_chasecam back to locked (the leak: chase to
			// free unless mp_chasecam, in multiplayer)
			if ( CVAR_GET_FLOAT( "mp_chasecam" ) != 0 )
			{
				if ( m_cChaseMode == 1 )
					SetChaseMode( 0 );
				else if ( m_cChaseMode == 0 )
					SetChaseMode( 1 );
			}
			else
			{
				if ( m_cChaseMode == 1 )
					SetChaseMode( 0 );
				else if ( m_cChaseMode == 0 )
					SetFreeMode();
			}
		}
		else if ( ( iButtons & IN_ATTACK ) && !( m_iLastButtons & IN_ATTACK ) )
		{
			NextClient();
			m_flNextChase = 0;
		}
	}

	m_iLastButtons = iButtons;
}

// Beta 5.0: the observer's crosshair on or off (name ours; the Crosshair
// message is Beta 5.0's)
inline void SetCrosshair( CBasePlayer *pPlayer, BOOL bOn )
{
	MESSAGE_BEGIN( MSG_ONE, gmsgCrosshair, NULL, pPlayer->pev );
		WRITE_BYTE( bOn );
	MESSAGE_END();
}

void CObserver::SetChaseMode( char cMode )
{
	// Beta 5.0: two chase modes (the leak: "Observer switched to CHASE MODE");
	// no crosshair in either
	if ( cMode == 0 )
	{
		ClientPrint( m_pOwner->pev, HUD_PRINTCENTER, "Switched to FREELOOK chase mode" );
		SET_VIEW( m_pOwner->edict(), edict() );
		SET_ORIGIN( edict(), m_pOwner->pev->origin + m_pOwner->pev->view_ofs );
		SetThink( ChaseMoveThink );
		SetCrosshair( m_pOwner, FALSE );
		pev->movetype = MOVETYPE_FLY;
		pev->nextthink = gpGlobals->time + 0.001;
	}
	else if ( cMode == 1 )
	{
		ClientPrint( m_pOwner->pev, HUD_PRINTCENTER, "Switched to LOCKED chase mode" );
		SET_VIEW( m_pOwner->edict(), edict() );
		SET_ORIGIN( edict(), m_pOwner->pev->origin + m_pOwner->pev->view_ofs );
		SetThink( ChaseMoveThink );
		SetCrosshair( m_pOwner, FALSE );
		pev->movetype = MOVETYPE_FLY;
		pev->nextthink = gpGlobals->time + 0.001;
	}

	m_cChaseMode = cMode;
}

void CObserver::SetFreeMode( void )
{
	ClientPrint( m_pOwner->pev, HUD_PRINTCENTER, "Switched to CLASSIC observer mode" );	// Beta 5.0 (the leak: "Observer switched to FREE MODE")
	SET_VIEW( m_pOwner->edict(), m_pOwner->edict() );
	SET_ORIGIN( m_pOwner->edict(), pev->origin );
	SetThink( FreeMoveThink );
	SetCrosshair( m_pOwner, TRUE );	// Beta 5.0
	pev->movetype = MOVETYPE_NONE;
	pev->nextthink = gpGlobals->time + 0.001;
}

// where the chasing view stands: 112 units behind the target's eyes along
// the given view, moved by vecOffset (Beta 5.0), pulled 4 units off a wall
// it hits (Beta 5.0: the trace passes through glass)
const Vector &CObserver::GetChaseOrigin( const Vector &angles, entvars_t *pevTarget, Vector &vecOffset )
{
	static Vector chaseorg;	// Linux symbol

	UTIL_MakeVectors( angles );

	TraceResult tr;
	Vector vecStart = pevTarget->origin + pevTarget->view_ofs;	// names ours
	Vector vecEnd = vecStart - gpGlobals->v_forward * 112 + vecOffset;

	UTIL_TraceLine( vecStart, vecEnd, ignore_monsters, ignore_glass, ENT( pev ), &tr );

	chaseorg = tr.vecEndPos + tr.vecPlaneNormal * 4;

	return chaseorg;
}

void CObserver::CGhosts::Reset( void )
{
	m_flNextUpdate = 0;
	m_fShowHint = FALSE;
}

void CObserver::CGhosts::SetHintMessage( void )
{
	m_flHintTime = gpGlobals->time + 2;
	m_fShowHint = TRUE;
}

void CObserver::CGhosts::Update( CBasePlayer *pOwner, const Vector &vecOrigin )
{
	if ( m_fShowHint && gpGlobals->time > m_flHintTime && g_pGameRules->IsMultiplayer() )
	{
		// Beta 5.0: one hint; if the client takes no hints, its first line
		// is forced (the leak: by mp_chasecam, the second line once)
		if ( !pOwner->HintMessage( "You are an observer until next round.\nPress FIRE to find players.\nPress JUMP to switch modes.", TRUE, FALSE ) )
			pOwner->HintMessage( "You are an observer until next round.", TRUE, TRUE );

		m_fShowHint = FALSE;
	}

	if ( gpGlobals->time > m_flNextUpdate )
	{
		float flFrequency = CVAR_GET_FLOAT( "mp_ghostfrequency" );	// name ours

		if ( flFrequency != 0 )
		{
		for ( int i = 1; i <= gpGlobals->maxClients; i++ )
		{
			CBasePlayer *pPlayer = (CBasePlayer *)UTIL_PlayerByIndex( i );

			if ( !pPlayer || FNullEnt( pPlayer->edict() ) || pPlayer == pOwner )
				continue;

			// only to other observers who want them
			if ( !( pPlayer->m_afPhysicsFlags & PFLAG_OBSERVER ) || pPlayer->m_iIgnoreGhosts )
				continue;

			MESSAGE_BEGIN( MSG_ONE, SVC_TEMPENTITY, NULL, pPlayer->pev );
				WRITE_BYTE( TE_EXPLOSION );
				WRITE_COORD( vecOrigin.x );
				WRITE_COORD( vecOrigin.y );
				WRITE_COORD( vecOrigin.z );
				WRITE_SHORT( pOwner->m_iTeam == TEAM_CT ? g_sModelIndexCTGhost : g_sModelIndexTGhost );
				WRITE_BYTE( 2 );
				WRITE_BYTE( 40 );	// Beta 5.0 (the leak: 30)
				WRITE_BYTE( TE_EXPLFLAG_NODLIGHTS | TE_EXPLFLAG_NOSOUND | TE_EXPLFLAG_NOPARTICLES );
			MESSAGE_END();
		}

		m_flNextUpdate = gpGlobals->time + flFrequency;
		}
		else
			m_flNextUpdate = gpGlobals->time + 1;
	}
}

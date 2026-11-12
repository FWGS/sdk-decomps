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
			SetChaseMode();
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
	if ( pTarget && !FNullEnt( pTarget->edict() ) && pTarget->IsAlive() )
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

			SET_ORIGIN( edict(), GetChaseOrigin( m_pOwner->pev->v_angle, pTarget->pev ) );
			pev->angles = m_pOwner->pev->v_angle;
			pev->velocity = pTarget->pev->velocity;
		}
		else
		{
			m_flNextChase = gpGlobals->time + 1;
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
				SetChaseMode();
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
					SET_ORIGIN( m_pOwner->edict(), GetChaseOrigin( m_pOwner->pev->v_angle, pTarget->pev ) );
			}
		}
	}
	else if ( m_pfnThink == &CObserver::ChaseMoveThink )
	{
		if ( ( iButtons & IN_JUMP ) && !( m_iLastButtons & IN_JUMP ) )
		{
			// free mode only where the rules let the observers fly
			if ( !g_pGameRules->IsMultiplayer() || CVAR_GET_FLOAT( "mp_chasecam" ) == 0 )
				SetFreeMode();
		}
		else if ( ( iButtons & IN_ATTACK ) && !( m_iLastButtons & IN_ATTACK ) )
		{
			NextClient();
			m_flNextChase = 0;
		}
	}

	m_iLastButtons = iButtons;
}

void CObserver::SetChaseMode( void )
{
	ClientPrint( m_pOwner->pev, HUD_PRINTCENTER, "Observer switched to CHASE MODE" );
	SET_VIEW( m_pOwner->edict(), edict() );
	SET_ORIGIN( edict(), m_pOwner->pev->origin + m_pOwner->pev->view_ofs );
	SetThink( ChaseMoveThink );
	pev->movetype = MOVETYPE_FLY;
	pev->nextthink = gpGlobals->time + 0.001;
}

void CObserver::SetFreeMode( void )
{
	ClientPrint( m_pOwner->pev, HUD_PRINTCENTER, "Observer switched to FREE MODE" );
	SET_VIEW( m_pOwner->edict(), m_pOwner->edict() );
	SET_ORIGIN( m_pOwner->edict(), pev->origin );
	SetThink( FreeMoveThink );
	pev->movetype = MOVETYPE_NONE;
	pev->nextthink = gpGlobals->time + 0.001;
}

// where the chasing view stands: 112 units behind the target's eyes along
// the observer's view, pulled 4 units off a wall it hits
const Vector &CObserver::GetChaseOrigin( const Vector &angles, entvars_t *pevTarget )
{
	static Vector chaseorg;	// Linux symbol

	UTIL_MakeVectors( angles );

	TraceResult tr;
	Vector vecStart = pevTarget->origin + pevTarget->view_ofs;	// names ours
	Vector vecEnd = vecStart - gpGlobals->v_forward * 112;

	UTIL_TraceLine( vecStart, vecEnd, ignore_monsters, ENT( pev ), &tr );

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
		if ( CVAR_GET_FLOAT( "mp_chasecam" ) != 0 )
		{
			pOwner->HintMessage( "You are in observer mode.", TRUE );
			if ( !( pOwner->m_flDisplayHistory & 0x40000 ) )
			{
				pOwner->HintMessage( "Press FIRE to find teammates.", TRUE );
				pOwner->m_flDisplayHistory |= 0x40000;
			}
		}
		else
		{
			pOwner->HintMessage( "You are in observer mode.", TRUE );
			if ( !( pOwner->m_flDisplayHistory & 0x40000 ) )
			{
				pOwner->HintMessage( "Press FIRE to find teammates.\nPress JUMP to toggle between\nchase mode and free mode.", TRUE );
				pOwner->m_flDisplayHistory |= 0x40000;
			}
		}

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
				WRITE_BYTE( 30 );
				WRITE_BYTE( TE_EXPLFLAG_NODLIGHTS | TE_EXPLFLAG_NOSOUND | TE_EXPLFLAG_NOPARTICLES );
			MESSAGE_END();
		}

		m_flNextUpdate = gpGlobals->time + flFrequency;
		}
		else
			m_flNextUpdate = gpGlobals->time + 1;
	}
}

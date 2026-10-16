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

===== highnoon.cpp ========================================================

  High Noon (info_wanteddetect gametype 3, "Wanted! High Noon"): duels.
  Two players fight, the champion and the challenger; everybody else
  waits as an invisible, noclipping spectator (FL_SPECTATOR, maxspeed
  700) and nobody drops weapons or ammo.

  Every 5 seconds without a match the rules look for duelists: with one
  player they print "Waiting for challenger!"; otherwise the first player
  becomes champion if there is none, and the waiting player who lost
  longest ago (his pev->scale holds the time) challenges him. A match
  fires "matchstart" and starts the first round: the ammo on the map is
  hidden and hn_maxammo (default 3) random pieces of it respawn, the
  tombstones go, both duelists are stripped and respawned, the rules
  print "<champion> (<n> kills) VS <challenger> (<n> kills)!" and fire
  "roundinit". A duelist spawns at a random info_player_deathmatch (the
  challenger at the other spot with the champion's spot's netname),
  frozen and hidden, and watches a "dropent" with his model fall from
  there, spinning. 1 to 5 seconds after both have landed the duelists
  stand where their dropent fell and get control back, "DRAW!" prints and
  "roundstart" fires. A duelist killed (or killing himself) loses the
  round: "<winner> wins the match!", a tombstone rises where his fading
  body lands (no one respawns until it has faded), "roundend" fires and
  the next round starts 5 seconds later. Three wins end the match:
  "<winner> sends <loser> running home!", "matchend", and the winner
  stays champion. A duelist who disconnects "fled town!", which ends the
  match.

  The file, class and helper names are ours.

*/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"
#include "items.h"
#include "gamerules.h"
#include "wantedrules.h"
#include "highnoon.h"

extern DLL_GLOBAL CBaseEntity *g_pLastSpawn;
extern DLL_GLOBAL ULONG g_ulModelIndexPlayer;

cvar_t	hn_maxammo = { "hn_maxammo", "3", FCVAR_SERVER };

BOOL g_fTombstoneFaded = TRUE;		// no dead duelist is fading away: players may respawn

static char s_szWins[2][10];
static float s_flMatchStartTime;
static BOOL s_fPlacingAmmo;			// the round's ammo is placed now: it respawns at once
static CBaseEntity *s_pChampionSpot;

// do two entities have the same netname (the spawn points of a duel)?
BOOL SameNetname( CBaseEntity *pEntity1, CBaseEntity *pEntity2 )
{
	if ( !pEntity1 || !pEntity2 )
		return FALSE;

	return FStrEq( STRING( pEntity1->pev->netname ), STRING( pEntity2->pev->netname ) );
}

void SetPlayerMaxspeed( CBasePlayer *pPlayer, float flMaxspeed )
{
	g_engfuncs.pfnSetClientMaxspeed( ENT( pPlayer->pev ), flMaxspeed );
}

char *IntToString( int iValue, char *pszBuffer )
{
	itoa( iValue, pszBuffer, 10 );
	return pszBuffer;
}

//=========================================================
// tombstone - rises where a dead duelist's body lands while
// the body fades out
//=========================================================
class CTombstone : public CBaseAnimating
{
public:
	void Spawn( void );
	void Precache( void );
	void FadeThink( void );
};

LINK_ENTITY_TO_CLASS( tombstone, CTombstone );

void CTombstone::Spawn( void )
{
	if ( g_pGameRules->GetGameType() != GAMETYPE_HIGHNOON )
		return;

	pev->classname = MAKE_STRING( "tombstone" );
	pev->speed = 1;		// not risen yet

	// the body (our owner) fades out
	pev->owner->v.renderamt = 255;
	pev->owner->v.rendermode = kRenderTransTexture;

	g_fTombstoneFaded = FALSE;

	SetThink( FadeThink );
	pev->nextthink = gpGlobals->time + 0.1;
}

void CTombstone::FadeThink( void )
{
	if ( pev->owner->v.renderamt > 10 )
	{
		pev->owner->v.renderamt -= 10;

		if ( ( pev->owner->v.flags & FL_ONGROUND ) && pev->owner->v.renderamt < 100 && pev->speed != 0 )
		{
			// the body has landed: rise there
			pev->origin = pev->owner->v.origin;
			pev->angles = g_vecZero;
			pev->angles.y = pev->owner->v.angles.y;
			pev->nextthink = gpGlobals->time + 1.0;
			SET_MODEL( ENT( pev ), "models/tombstone.mdl" );
			pev->sequence = 0;
			pev->speed = 0;
			ResetSequenceInfo();
			DROP_TO_FLOOR( ENT( pev ) );
		}

		pev->nextthink = gpGlobals->time + 0.1;	// sic: also after the 1 second above
	}
	else
	{
		pev->owner->v.renderamt = 0;
		pev->owner->v.effects |= EF_NODRAW;

		SetThink( SUB_Remove );
		pev->nextthink = gpGlobals->time + 5.0;

		g_fTombstoneFaded = TRUE;
	}
}

void CTombstone::Precache( void )
{
	PRECACHE_MODEL( "models/tombstone.mdl" );
}

//=========================================================
// CHighNoon
//=========================================================
void CHighNoon::Init( void )
{
	CVAR_REGISTER( &hn_maxammo );

	m_pChampion = NULL;
	m_pChallenger = NULL;
	m_fMatchActive = FALSE;
	m_flDrawTime = 0;
	m_flNextRoundTime = 0;
	m_flNextCheckTime = 0;
	m_iPlayers = 0;

	UTIL_PrecacheOther( "tombstone" );

	CWantedMultiplay::Init();
}

void CHighNoon::PlayerSpawn( CBasePlayer *pPlayer )
{
	pPlayer->pev->rendermode = kRenderNormal;

	if ( ( pPlayer != m_pChampion && pPlayer != m_pChallenger ) || m_flNextRoundTime != 0 )
	{
		// everybody else waits as an invisible spectator
		pPlayer->pev->effects |= EF_NODRAW;
		pPlayer->pev->flags |= FL_SPECTATOR;
		pPlayer->pev->movetype = MOVETYPE_NOCLIP;
		pPlayer->pev->solid = SOLID_NOT;
		pPlayer->pev->takedamage = DAMAGE_NO;
		SetPlayerMaxspeed( pPlayer, 700 );
		pPlayer->EnableControl( TRUE );
		return;
	}

	// a duelist: watch the dropent fall from the spawn spot
	int iDuelist = ( pPlayer != m_pChampion );
	CBaseEntity *pDrop = GetClassPtr( (CBaseEntity *)NULL );
	m_pDropEnt[iDuelist] = pDrop;

	pDrop->pev->owner = pPlayer->edict();
	pPlayer->pev->owner = pDrop->edict();
	pDrop->pev->classname = MAKE_STRING( "dropent" );
	pDrop->pev->model = pPlayer->pev->model;
	SET_MODEL( ENT( pDrop->pev ), STRING( pDrop->pev->model ) );
	pDrop->pev->velocity = Vector( 0, 0, -100 );
	pDrop->pev->movetype = MOVETYPE_FLY;
	pDrop->pev->solid = SOLID_BBOX;
	pDrop->pev->origin = pPlayer->pev->origin;
	pDrop->pev->angles = pPlayer->pev->angles;
	pDrop->pev->mins = pPlayer->pev->mins;
	pDrop->pev->maxs = pPlayer->pev->maxs;
	UTIL_SetOrigin( pDrop->pev, pDrop->pev->origin );
	UTIL_SetSize( pDrop->pev, pDrop->pev->mins, pDrop->pev->maxs );
	pDrop->pev->renderamt = 0;
	pDrop->pev->rendermode = kRenderTransTexture;
	pDrop->pev->flags &= ~FL_ONGROUND;

	// it spins the faster the closer the ground is
	TraceResult tr;
	Vector vecEnd = pDrop->pev->origin - Vector( 0, 0, 4000 );
	gpGlobals->trace_flags |= FTRACE_SIMPLEBOX;
	UTIL_TraceLine( pDrop->pev->origin, vecEnd, dont_ignore_monsters, ENT( pDrop->pev ), &tr );

	if ( tr.flFraction < 1.0 )
	{
		float flDist = ( pDrop->pev->origin - tr.vecEndPos ).Length();
		float flSpin = max( min( 9000 / flDist, 359 ), -359 );
		pDrop->pev->avelocity = Vector( flSpin, 0, 0 );
	}
	else
		pDrop->pev->avelocity = Vector( 10, 0, 0 );

	pPlayer->pev->effects |= EF_NODRAW;
	pPlayer->pev->flags &= ~FL_SPECTATOR;
	pPlayer->pev->movetype = MOVETYPE_NOCLIP;
	pPlayer->pev->solid = SOLID_NOT;
	SetPlayerMaxspeed( pPlayer, 0 );
	pPlayer->EnableControl( FALSE );
	SET_VIEW( pPlayer->edict(), pDrop->edict() );
}

BOOL CHighNoon::FPlayerCanRespawn( CBasePlayer *pPlayer )
{
	if ( !g_fTombstoneFaded )
		return FALSE;

	return CHalfLifeMultiplay::FPlayerCanRespawn( pPlayer );
}

BOOL CHighNoon::ClientConnected( edict_t *pEntity, const char *pszName, const char *pszAddress, char szRejectReason[ 128 ] )
{
	m_iPlayers++;
	return CHalfLifeMultiplay::ClientConnected( pEntity, pszName, pszAddress, szRejectReason );
}

void CHighNoon::ClientDisconnected( edict_t *pClient )
{
	m_iPlayers--;

	BOOL fFled = FALSE;

	if ( m_pChampion && pClient == m_pChampion->edict() )
	{
		// the challenger takes the champion's place
		if ( m_pChallenger )
		{
			m_pChampion = m_pChallenger;
			m_pChallenger = NULL;
		}
		else
			m_pChampion = NULL;
		fFled = TRUE;
	}
	else if ( m_pChallenger && pClient == m_pChallenger->edict() )
	{
		m_pChallenger = NULL;
		fFled = TRUE;
	}

	if ( fFled )
	{
		char *pszText = UTIL_VarArgs( "%s fled town!.\n", STRING( pClient->v.netname ) );
		UTIL_ClientPrintAll( HUD_PRINTCENTER, pszText );
		EndMatch();
	}

	CWantedMultiplay::ClientDisconnected( pClient );
}

void CHighNoon::PlayerKilled( CBasePlayer *pVictim, entvars_t *pKiller, entvars_t *pInflictor )
{
	if ( m_fMatchActive && ( pVictim == m_pChampion || pVictim == m_pChallenger ) && m_flNextRoundTime == 0 )
	{
		// a suicide is the other duelist's win
		if ( pKiller == pVictim->pev )
		{
			if ( pVictim == m_pChampion )
				pKiller = m_pChallenger->pev;
			else
				pKiller = m_pChampion->pev;
		}

		char *pszText = UTIL_VarArgs( "%s wins the match!\n", STRING( pKiller->netname ) );
		UTIL_ClientPrintAll( HUD_PRINTCENTER, pszText );

		// the loser waits longest for his next duel
		pVictim->pev->scale = gpGlobals->time;

		// fall over as CBasePlayer::Killed does
		pVictim->SetAnimation( PLAYER_DIE );
		pVictim->pev->modelindex = g_ulModelIndexPlayer;
		pVictim->pev->movetype = MOVETYPE_TOSS;
		pVictim->pev->flags &= ~FL_ONGROUND;
		if ( pVictim->pev->velocity.z < 10 )
			pVictim->pev->velocity.z += RANDOM_FLOAT( 0, 300 );
		pVictim->pev->angles.x = 0;
		pVictim->pev->angles.z = 0;

		CTombstone *pTombstone = GetClassPtr( (CTombstone *)NULL );
		pTombstone->pev->owner = pVictim->edict();
		pTombstone->Spawn();

		DuelWon( GetClassPtr( (CBasePlayer *)pKiller ) );

		pVictim->pev->takedamage = DAMAGE_NO;
		pVictim->pev->solid = SOLID_NOT;
	}

	CHalfLifeMultiplay::PlayerKilled( pVictim, pKiller, pInflictor );
}

void CHighNoon::Think( void )
{
	if ( !m_iPlayers )
	{
		if ( m_fMatchActive )
			EndMatch();
		m_pChampion = NULL;
		m_pChallenger = NULL;
		CWantedMultiplay::Think();
		return;
	}

	if ( !m_fMatchActive )
	{
		if ( gpGlobals->time >= m_flNextCheckTime )
			FindDuelists();

		if ( m_fMatchActive )
		{
			s_flMatchStartTime = gpGlobals->time;
			CWantedMultiplay::Think();
			return;
		}
	}
	else
	{
		if ( m_flNextRoundTime != 0 && gpGlobals->time >= m_flNextRoundTime )
			StartRound();

		if ( !m_fRoundActive && gpGlobals->time > s_flMatchStartTime + 0.1 )
		{
			int fLanded[2];
			fLanded[0] = FALSE;
			fLanded[1] = FALSE;

			if ( m_flDrawTime == 0 )
			{
				// both dropents down: draw in 1 to 5 seconds
				for ( int i = 0; i < 2; i++ )
				{
					CBasePlayer *pDuelist = ( i == 0 ) ? m_pChampion : m_pChallenger;
					CBaseEntity *pDrop = (CBaseEntity *)GET_PRIVATE( pDuelist->pev->owner );
					if ( pDrop->pev->flags & FL_ONGROUND )
						fLanded[i] = TRUE;
				}

				if ( fLanded[0] && fLanded[1] )
					m_flDrawTime = RANDOM_FLOAT( 1, 5 ) + gpGlobals->time;
			}

			if ( m_flDrawTime != 0 && gpGlobals->time >= m_flDrawTime )
			{
				for ( int i = 0; i < 2; i++ )
				{
					CBasePlayer *pDuelist = ( i == 0 ) ? m_pChampion : m_pChallenger;
					CBaseEntity *pDrop = (CBaseEntity *)GET_PRIVATE( pDuelist->pev->owner );

					pDuelist->pev->origin = pDrop->pev->origin;
					pDuelist->pev->angles = pDrop->pev->angles;
					pDuelist->pev->v_angle = pDuelist->pev->angles;
					pDuelist->pev->fixangle = TRUE;
					pDuelist->pev->owner = NULL;
					SET_VIEW( pDuelist->edict(), pDuelist->edict() );
					pDrop->SUB_Remove();

					pDuelist->pev->effects &= ~EF_NODRAW;
					pDuelist->pev->movetype = MOVETYPE_WALK;
					pDuelist->pev->solid = SOLID_SLIDEBOX;
					pDuelist->EnableControl( TRUE );
					CHalfLifeMultiplay::PlayerSpawn( pDuelist );
				}

				UTIL_ClientPrintAll( HUD_PRINTCENTER, "DRAW!\n" );
				m_flDrawTime = 0;
				m_fRoundActive = TRUE;
				FireTargets( "roundstart", NULL, NULL, USE_TOGGLE, 0 );
			}
		}
	}

	CWantedMultiplay::Think();
}

float CHighNoon::FlAmmoRespawnTime( CBasePlayerAmmo *pAmmo )
{
	if ( s_fPlacingAmmo )
		return gpGlobals->time;

	return gpGlobals->time + 5000.0;	// the rounds place the ammo
}

//=========================================================
// StartRound - new ammo, the duelists respawn
//=========================================================
void CHighNoon::StartRound( void )
{
	if ( !m_pChampion || !m_pChallenger )
		return;

	s_fPlacingAmmo = TRUE;

	// hide all ammo; clear the tombstones
	CBaseEntity *pAmmoList = NULL;
	int cAmmo = 0;
	for ( int i = 0; i < gpGlobals->maxEntities; i++ )
	{
		if ( INDEXENT( i ) && strstr( STRING( INDEXENT( i )->v.classname ), "ammo_" ) )
		{
			CBaseEntity *pAmmo = (CBaseEntity *)GET_PRIVATE( INDEXENT( i ) );
			pAmmo->SetTouch( NULL );
			pAmmo->pev->effects |= EF_NODRAW;

			if ( !pAmmoList )
				pAmmoList = pAmmo;
			else
			{
				CBaseEntity *pLast = pAmmoList;
				while ( pLast->m_pLink )
					pLast = pLast->m_pLink;
				pLast->m_pLink = pAmmo;
			}
			pAmmo->m_pLink = NULL;
			cAmmo++;
		}

		if ( INDEXENT( i ) && FStrEq( STRING( INDEXENT( i )->v.classname ), "tombstone" ) )
			((CBaseEntity *)GET_PRIVATE( INDEXENT( i ) ))->SUB_Remove();
	}

	// hn_maxammo random pieces of it come back
	int cMaxAmmo = CVAR_GET_FLOAT( "hn_maxammo" );
	if ( cMaxAmmo < 0 )
		cMaxAmmo = 0;

	for ( int j = 0; j < cMaxAmmo; j++ )
	{
		if ( !cAmmo )
			break;

		int iPick = RANDOM_LONG( 0, cAmmo - 1 );
		CBaseEntity *pPrev = NULL;
		CBaseEntity *pAmmo = pAmmoList;
		if ( !pAmmo )
			break;

		for ( ; iPick > 0; iPick-- )
		{
			pPrev = pAmmo;
			pAmmo = pAmmo->m_pLink;
		}

		pAmmo->Respawn();

		if ( pPrev )
			pPrev->m_pLink = pAmmo->m_pLink;
		else
			pAmmoList = pAmmo->m_pLink;
		cAmmo--;
	}

	s_fPlacingAmmo = FALSE;
	m_flNextRoundTime = 0;

	m_pChampion->RemoveAllItems( TRUE );
	m_pChampion->Spawn();
	EMIT_SOUND_DYN( m_pChampion->edict(), CHAN_WEAPON, "common/null.wav", 1.0, ATTN_NORM, 0, PITCH_NORM );

	m_pChallenger->RemoveAllItems( TRUE );
	m_pChallenger->Spawn();
	EMIT_SOUND_DYN( m_pChampion->edict(), CHAN_WEAPON, "common/null.wav", 1.0, ATTN_NORM, 0, PITCH_NORM );	// sic: the champion again

	char *pszText = UTIL_VarArgs( "\n\n\n%s (%s kill%s)\n\nVS\n\n%s (%s kill%s)!\n",
		STRING( m_pChampion->pev->netname ),
		m_iWins[0] ? IntToString( m_iWins[0], s_szWins[0] ) : "no",
		( m_iWins[0] == 1 ) ? "" : "s",
		STRING( m_pChallenger->pev->netname ),
		m_iWins[1] ? IntToString( m_iWins[1], s_szWins[1] ) : "no",
		( m_iWins[1] == 1 ) ? "" : "s" );
	UTIL_ClientPrintAll( HUD_PRINTCENTER, pszText );

	m_fMatchActive = TRUE;
	m_fRoundActive = FALSE;
	m_flDrawTime = 0;
	FireTargets( "roundinit", NULL, NULL, USE_TOGGLE, 0 );
}

//=========================================================
// DuelWon - three wins take the match
//=========================================================
void CHighNoon::DuelWon( CBasePlayer *pWinner )
{
	CBasePlayer *pLoser;
	int iWinner;

	if ( pWinner == m_pChampion )
	{
		pLoser = m_pChallenger;
		iWinner = 0;
	}
	else
	{
		pLoser = m_pChampion;
		iWinner = 1;
	}

	FireTargets( "roundend", NULL, NULL, USE_TOGGLE, 0 );

	if ( ++m_iWins[iWinner] >= 3 )
	{
		char *pszText = UTIL_VarArgs( "%s sends %s running home!\n", STRING( pWinner->pev->netname ), STRING( pLoser->pev->netname ) );
		UTIL_ClientPrintAll( HUD_PRINTCENTER, pszText );
		m_pChampion = pWinner;
		m_pChallenger = NULL;
		FireTargets( "matchend", NULL, NULL, USE_TOGGLE, 0 );
		EndMatch();
	}
	else
		m_flNextRoundTime = gpGlobals->time + 5;
}

//=========================================================
// FindDuelists - a champion, and the waiting player who lost
// longest ago as the challenger
//=========================================================
void CHighNoon::FindDuelists( void )
{
	if ( m_iPlayers == 1 )
		UTIL_ClientPrintAll( HUD_PRINTCENTER, "Waiting for challenger!\n" );
	else if ( m_iPlayers >= 2 )
	{
		if ( !m_pChampion )
		{
			for ( int i = 1; i <= gpGlobals->maxClients; i++ )
			{
				m_pChampion = (CBasePlayer *)UTIL_PlayerByIndex( i );
				if ( m_pChampion )
					break;
			}
		}

		if ( m_pChampion && m_pChampion->m_fGameHUDInitialized )
		{
			float flLostTime = gpGlobals->time;

			for ( int i = 1; i <= gpGlobals->maxClients; i++ )
			{
				CBasePlayer *pPlayer = (CBasePlayer *)UTIL_PlayerByIndex( i );
				if ( !pPlayer )
					break;
				if ( pPlayer != m_pChampion && pPlayer->m_fGameHUDInitialized && pPlayer->pev->scale < flLostTime )
				{
					m_pChallenger = pPlayer;
					flLostTime = pPlayer->pev->scale;
				}
			}

			if ( !m_pChallenger )
				UTIL_ClientPrintAll( HUD_PRINTCENTER, "Waiting for challenger!\n" );
			else
			{
				m_iWins[0] = 0;
				m_iWins[1] = 0;
				FireTargets( "matchstart", NULL, NULL, USE_TOGGLE, 0 );
				StartRound();
			}
		}
	}

	m_flNextCheckTime = gpGlobals->time + 5;
}

//=========================================================
// EndMatch
//=========================================================
void CHighNoon::EndMatch( void )
{
	if ( !m_fRoundActive )
	{
		// the round hadn't started: the duelists' dropents go
		for ( int i = 0; i < 2; i++ )
		{
			CBasePlayer *pDuelist = ( i == 0 ) ? m_pChampion : m_pChallenger;
			if ( pDuelist )
			{
				CBasePlayer *pChampion = m_pChampion;
				m_pChampion = NULL;
				pDuelist->Spawn();
				m_pChampion = pChampion;
			}
		}
	}

	m_flNextCheckTime = gpGlobals->time + 5;
	m_fMatchActive = FALSE;
	m_fRoundActive = FALSE;
	m_flDrawTime = 0;
}

//=========================================================
// GetPlayerSpawnSpot - the duelists face off at a pair of
// info_player_deathmatch with the same netname
//=========================================================
edict_t *CHighNoon::GetPlayerSpawnSpot( CBasePlayer *pPlayer )
{
	CBaseEntity *pSpots[10];
	CBaseEntity *pSpot = NULL;
	edict_t *pent = NULL;
	int cSpots = 0;

	while ( cSpots < 10 )
	{
		pent = FIND_ENTITY_BY_CLASSNAME( pent, "info_player_deathmatch" );
		if ( FNullEnt( pent ) )
			break;
		pSpots[cSpots++] = (CBaseEntity *)pent->pvPrivateData;
	}

	if ( cSpots )
	{
		if ( pPlayer == m_pChampion )
		{
			s_pChampionSpot = pSpots[RANDOM_LONG( 0, cSpots - 1 )];
			pSpot = s_pChampionSpot;
		}
		else if ( pPlayer == m_pChallenger && s_pChampionSpot )
		{
			for ( int i = 0; i < cSpots; i++ )
			{
				if ( SameNetname( s_pChampionSpot, pSpots[i] ) && s_pChampionSpot != pSpots[i] )
				{
					pSpot = pSpots[i];
					s_pChampionSpot = NULL;
					break;
				}
			}
		}
		else
			pSpot = pSpots[RANDOM_LONG( 0, cSpots - 1 )];
	}

	if ( !pSpot )
	{
		s_pChampionSpot = NULL;
		ALERT( at_console, "SPAWN ERROR: No info_player_deathmatch found for %s\n", STRING( pPlayer->pev->netname ) );

		pSpot = UTIL_FindEntityByClassname( NULL, "info_player_start" );
		if ( !pSpot || FNullEnt( pSpot->edict() ) )
		{
			ALERT( at_error, "PutClientInServer: no info_player_start on level" );
			pSpot = (CBaseEntity *)GET_PRIVATE( INDEXENT( 0 ) );
		}
	}

	g_pLastSpawn = pSpot;

	pPlayer->pev->origin = pSpot->pev->origin + Vector( 0, 0, 1 );
	pPlayer->pev->v_angle = g_vecZero;
	pPlayer->pev->velocity = g_vecZero;
	pPlayer->pev->angles = pSpot->pev->angles;
	pPlayer->pev->punchangle = g_vecZero;

	return pSpot->edict();
}

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

===== wantedrules.cpp ========================================================

  Wanted's deathmatch: CHalfLifeMultiplay with Cow-Bots (CWantedMultiplay;
  team deathmatch, Capture the Chicken and High Noon derive from it), and
  info_wanteddetect, the map entity that picks the game mode.

  The world installs the rules first (deathmatch, or single player); a
  map's info_wanteddetect then replaces them with its gametype's (0
  deathmatch, 1 team deathmatch, 2 Capture the Chicken, 3 High Noon) as it
  spawns. The first rules execute game.cfg and keep the server config
  (servercfgfile, lservercfgfile on a listen server) from running a second
  time; EndMultiplayerGame kicks the bots, puts the config cvar back and
  goes to the intermission. Entities that need the game mode (the CTC
  chicken and capture points) wait in a list until info_wanteddetect
  activates, which spawns them again.

  Bots: sv_defaultbots bots join after 3 seconds, one a second. The
  sv_addbot, sv_removebot and sv_removeallbots cvars, and the listen
  server host's addbot/removebot [n] and removeallbots commands, queue
  more (at most maxplayers and 32 players). With sv_cheats the host also
  has godon/godoff, specon/specoff (look through the next bot's camera)
  and drawnodes (show the navigation links, a node every sv_nodedrawfreq
  seconds). The rules count the cells human players walk through for the
  bots' navigation file and save it every 5 minutes. sv_botskill 1 and 2
  mean 30 and 75.

  The file, class, global and helper names are ours.

*/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "soundent.h"
#include "decals.h"
#include "gamerules.h"
#include "teamplay_gamerules.h"
#include "bot.h"

extern float g_flWeaponCheat;

BOOL g_fInitialRules = TRUE;

cvar_t	sv_addbot = { "sv_addbot", "0", FCVAR_EXTDLL };
cvar_t	sv_removebot = { "sv_removebot", "0", FCVAR_EXTDLL };
cvar_t	sv_removeallbots = { "sv_removeallbots", "0", FCVAR_EXTDLL };
cvar_t	sv_nodedrawfreq = { "sv_nodedrawfreq", "0.5", FCVAR_SERVER };

extern cvar_t sv_defaultbots;
extern cvar_t sv_botskill;

static const char *g_pszServerCfgCvar = NULL;	// servercfgfile or lservercfgfile
static char g_szServerCfgFile[256] = "";		// its value, while the map's rules run
static int g_iBotsQueued = 0;				// > 0: bots to add, < 0: bots to remove
static BOOL g_fHostChosen = FALSE;			// the listen server's host has bot commands (pev->noise1)
static BOOL g_fBotCvarsRegistered = FALSE;
static int g_iSpectatedBot = 0;				// "specon": the bot looked through

//=========================================================
// Init - the rules are installed: reload the navigation,
// queue the default bots
//=========================================================
void CWantedMultiplay::Init( void )
{
	CBotNode *pNode = g_BotNavigator.m_pHead;
	while ( pNode )
	{
		CBotNode *pRemove = pNode;
		pNode = pNode->m_pNext;
		g_BotNavigator.RemoveNode( pRemove );
	}
	g_BotNavigator.m_pTail = NULL;
	g_BotNavigator.m_pHead = NULL;
	g_BotNavigator.m_pLandmarks = NULL;
	g_BotNavigator.ClearGrid();

	if ( g_fInitialRules )
	{
		if ( g_BotNavigator.Load( STRING( gpGlobals->mapname ) ) )
			ALERT( at_console, "Done\n\n" );
		else
		{
			ALERT( at_console, "\nFailed, using new navigation file\n" );
			g_BotNavigator.ClearGrid();
		}
	}

	m_pDrawNode = NULL;
	g_fHostChosen = FALSE;

	if ( !g_fBotCvarsRegistered )
	{
		CVAR_REGISTER( &sv_addbot );
		CVAR_REGISTER( &sv_removebot );
		CVAR_REGISTER( &sv_removeallbots );
		CVAR_REGISTER( &sv_nodedrawfreq );
		g_fBotCvarsRegistered = TRUE;
	}

	g_iBotsQueued = sv_defaultbots.value;

	if ( sv_botskill.value == 1 )
		CVAR_SET_FLOAT( "sv_botskill", 30 );
	else if ( sv_botskill.value == 2 )
		CVAR_SET_FLOAT( "sv_botskill", 75 );

	g_flBotQueueTime = 3;
	g_flNavSaveTime = gpGlobals->time + 300;
}

//=========================================================
// ClientCommand - the host's bot commands. Every command
// counts as handled.
//=========================================================
BOOL CWantedMultiplay::ClientCommand( CBasePlayer *pPlayer, const char *pcmd )
{
	if ( !pPlayer->pev->noise1 )
		return TRUE;

	if ( FStrEq( pcmd, "addbot" ) || FStrEq( pcmd, "removebot" ) )
	{
		int iCount = 1;

		if ( CMD_ARGC() > 1 )
		{
			if ( atoi( CMD_ARGV( 1 ) ) > 1 )
				iCount = atoi( CMD_ARGV( 1 ) );
			else
				iCount = 1;
		}

		if ( FStrEq( pcmd, "addbot" ) )
			sv_addbot.value = iCount;
		else
			sv_removebot.value = iCount;
		return TRUE;
	}
	else if ( FStrEq( pcmd, "removeallbots" ) )
	{
		sv_removeallbots.value = 1;
		return TRUE;
	}

	if ( g_flWeaponCheat == 0 )
		return TRUE;

	if ( FStrEq( pcmd, "godon" ) )
	{
		pPlayer->pev->flags |= FL_GODMODE;
	}
	else if ( FStrEq( pcmd, "godoff" ) )
	{
		pPlayer->pev->flags &= ~FL_GODMODE;
	}
	else if ( FStrEq( pcmd, "specon" ) )
	{
		// look through the camera of the next bot after the last one
		for ( int i = 1; i <= gpGlobals->maxClients; i++ )
		{
			CBaseEntity *pEntity = UTIL_PlayerByIndex( i );

			if ( i > g_iSpectatedBot && pEntity && ( pEntity->pev->flags & FL_FAKECLIENT ) )
			{
				g_iSpectatedBot = i;

				CBot *pBot = (CBot *)pEntity;
				if ( pBot->m_pCamera )
					SET_VIEW( pPlayer->edict(), pBot->m_pCamera->edict() );

				pPlayer->m_afPhysicsFlags |= PFLAG_OBSERVER;
				ClientPrint( pPlayer->pev, HUD_PRINTNOTIFY, UTIL_VarArgs( "viewing bot: %s\n", STRING( pEntity->pev->netname ) ) );
				return TRUE;
			}

			// past the last bot: start again
			if ( i == gpGlobals->maxClients && g_iSpectatedBot )
			{
				g_iSpectatedBot = 0;
				i = 1;	// sic: the loop goes on with player 2
			}
		}
	}
	else if ( FStrEq( pcmd, "specoff" ) )
	{
		SET_VIEW( pPlayer->edict(), pPlayer->edict() );
		pPlayer->m_afPhysicsFlags &= ~PFLAG_OBSERVER;
	}
	else if ( FStrEq( pcmd, "drawnodes" ) )
	{
		if ( !m_pDrawNode )
		{
			m_pDrawNode = g_BotNavigator.m_pHead;
			m_flNextNodeDraw = 0;
		}
		else
			m_pDrawNode = NULL;
	}

	return TRUE;
}

//=========================================================
//=========================================================
void CWantedMultiplay::PlayerSpawn( CBasePlayer *pPlayer )
{
	CHalfLifeMultiplay::PlayerSpawn( pPlayer );
}

//=========================================================
// InitHUD - only closes the menu (CTC's team menu)
//=========================================================
void CWantedMultiplay::InitHUD( CBasePlayer *pl )
{
	if ( g_pGameRules->GetGameType() != GAMETYPE_CTC )
		ShowPlayerMenu( pl, " ", 0, 0, 0 );
}

//=========================================================
// Think - runs the bots, counts where the players walk,
// adds and removes bots, draws the navigation links
//=========================================================
void CWantedMultiplay::Think( void )
{
	int iPlayers = 0;
	int iBots = 0;
	int iLastBot = 0;

	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBaseEntity *pPlayer = UTIL_PlayerByIndex( i );

		if ( !pPlayer )
			continue;

		iPlayers++;

		if ( pPlayer->pev->flags & FL_FAKECLIENT )
		{
			iBots++;
			iLastBot = i;
			((CBot *)pPlayer)->BotThink();
		}
		else if ( ( pPlayer->pev->flags & FL_CLIENT ) && pPlayer->IsNetClient() )
		{
			// the listen server's host gets the bot commands
			if ( !IS_DEDICATED_SERVER() && !g_fHostChosen )
			{
				pPlayer->pev->noise1 = 1;
				g_fHostChosen = TRUE;
			}

			if ( pPlayer->IsAlive() )
			{
				// count each navigation cell the player enters
				Vector vecCell;
				vecCell.x = g_BotNavigator.GridIndex( pPlayer->pev->origin.x );
				vecCell.y = g_BotNavigator.GridIndex( pPlayer->pev->origin.y );
				vecCell.z = g_BotNavigator.GridIndex( pPlayer->pev->origin.z );

				CBaseMonster *pMonster = (CBaseMonster *)pPlayer;
				if ( vecCell != pMonster->m_vecLastPosition )
				{
					g_BotNavigator.AddVisit( pPlayer->pev->origin );
					pMonster->m_vecLastPosition = Vector( g_BotNavigator.GridIndex( pPlayer->pev->origin.x ), g_BotNavigator.GridIndex( pPlayer->pev->origin.y ), g_BotNavigator.GridIndex( pPlayer->pev->origin.z ) );
				}
			}
		}
	}

	if ( sv_addbot.value != 0 )
	{
		if ( sv_addbot.value > 0 )
		{
			g_iBotsQueued = sv_addbot.value;
			if ( g_iBotsQueued + iPlayers > gpGlobals->maxClients )
				g_iBotsQueued = gpGlobals->maxClients - iPlayers;
			if ( g_iBotsQueued + iPlayers > 32 )
				g_iBotsQueued = 32 - iPlayers;
			ALERT( at_console, "Adding %i cowbots...\n", g_iBotsQueued );
		}
		CVAR_SET_FLOAT( "sv_addbot", 0 );
	}

	if ( sv_removebot.value != 0 || sv_removeallbots.value != 0 )
	{
		if ( sv_removeallbots.value != 0 )
		{
			sv_removebot.value = iBots;
			CVAR_SET_FLOAT( "sv_removeallbots", 0 );
		}

		if ( sv_removebot.value > 0 )
		{
			g_iBotsQueued = sv_removebot.value;
			if ( g_iBotsQueued > iBots )
				g_iBotsQueued = iBots;

			if ( g_iBotsQueued )
				ALERT( at_console, "Removing %i cowbots...\n", g_iBotsQueued );
			else
				ALERT( at_console, "There are no cowbots in the game!\n", 0 );

			g_iBotsQueued = -g_iBotsQueued;
		}
		CVAR_SET_FLOAT( "sv_removebot", 0 );
	}

	g_BotManager.AdjustSkill();

	if ( gpGlobals->time >= g_flNavSaveTime )
		g_BotNavigator.Save();

	// "drawnodes": a beam from one node to each of its links
	if ( m_pDrawNode && gpGlobals->time > m_flNextNodeDraw )
	{
		CBotNode *pNode = m_pDrawNode;

		for ( int j = 0; j < pNode->m_cLinks; j++ )
		{
			MESSAGE_BEGIN( MSG_ALL, SVC_TEMPENTITY );
				WRITE_BYTE( TE_BEAMPOINTS );
				WRITE_COORD( pNode->m_vecOrigin.x );
				WRITE_COORD( pNode->m_vecOrigin.y );
				WRITE_COORD( pNode->m_vecOrigin.z );
				WRITE_COORD( pNode->m_pLinks[j]->m_vecOrigin.x );
				WRITE_COORD( pNode->m_pLinks[j]->m_vecOrigin.y );
				WRITE_COORD( pNode->m_pLinks[j]->m_vecOrigin.z );
				WRITE_SHORT( MODEL_INDEX( "sprites/xbeam1.spr" ) );
				WRITE_BYTE( 1 );	// framestart
				WRITE_BYTE( 1 );	// framerate
				WRITE_BYTE( 250 );	// life
				WRITE_BYTE( 30 );	// width
				WRITE_BYTE( 1 );	// noise
				WRITE_BYTE( 255 );	// r, g, b
				WRITE_BYTE( 0 );
				WRITE_BYTE( 0 );
				WRITE_BYTE( 255 );	// brightness
				WRITE_BYTE( 1 );	// speed
			MESSAGE_END();
		}

		pNode = pNode->m_pNext;
		if ( !pNode )
			pNode = g_BotNavigator.m_pHead;
		m_pDrawNode = pNode;

		if ( sv_nodedrawfreq.value <= 0 )
			CVAR_SET_FLOAT( "sv_nodedrawfreq", 0.5 );
		m_flNextNodeDraw = sv_nodedrawfreq.value + gpGlobals->time;
	}

	// CTC runs the team rules' Think itself
	if ( GetGameType() != GAMETYPE_CTC )
		CHalfLifeMultiplay::Think();

	// one bot a second
	if ( g_iBotsQueued && gpGlobals->time >= g_flBotQueueTime )
	{
		if ( g_iBotsQueued < 0 )
			BotRemove( iLastBot );
		else if ( iPlayers < 32 && !BotCreate( NULL ) )
			g_iBotsQueued = 0;

		g_iBotsQueued -= g_iBotsQueued / (float)abs( g_iBotsQueued );
		g_flBotQueueTime = gpGlobals->time + 1;
	}
}

//=========================================================
// ClientDisconnected - a bot's edict is freed at once
//=========================================================
void CWantedMultiplay::ClientDisconnected( edict_t *pClient )
{
	CHalfLifeMultiplay::ClientDisconnected( pClient );

	if ( pClient->v.flags & FL_FAKECLIENT )
	{
		pClient->free = TRUE;
		pClient->v.netname = 0;
		((CBot *)GET_PRIVATE( pClient ))->m_pCamera->pev->flags = FL_KILLME;
		((CBot *)GET_PRIVATE( pClient ))->m_pCamera = NULL;
	}
}

//=========================================================
//=========================================================
CWantedMultiplay::CWantedMultiplay()
{
	if ( g_fInitialRules )
	{
		// the base class has queued the server config; keep it from
		// running again when the map's rules are installed
		g_pszServerCfgCvar = IS_DEDICATED_SERVER() ? "servercfgfile" : "lservercfgfile";
		strcpy( g_szServerCfgFile, CVAR_GET_STRING( g_pszServerCfgCvar ) );
		CVAR_SET_STRING( g_pszServerCfgCvar, "" );
		SERVER_EXECUTE( );

		RefreshSkillData();
	}
}

//=========================================================
//=========================================================
void CWantedMultiplay::RefreshSkillData( void )
{
	if ( g_fInitialRules )
		CHalfLifeMultiplay::RefreshSkillData();
}

//=========================================================
// EndMultiplayerGame - kick the bots, restore the server
// config, back to deathmatch for the next map
//=========================================================
void CWantedMultiplay::EndMultiplayerGame( void )
{
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBaseEntity *pPlayer = UTIL_PlayerByIndex( i );

		if ( pPlayer && ( pPlayer->pev->flags & FL_FAKECLIENT ) )
		{
			SERVER_COMMAND( UTIL_VarArgs( "kick \"%s\"\n", STRING( pPlayer->pev->netname ) ) );
			SERVER_EXECUTE( );
		}
	}

	g_fInitialRules = TRUE;
	CVAR_SET_STRING( g_pszServerCfgCvar, g_szServerCfgFile );
	g_fHostChosen = FALSE;

	GoToIntermission();

	g_iGameType = GAMETYPE_DEATHMATCH;
}

//=========================================================
// info_wanteddetect - the map's game mode ("gametype" 0-3)
//=========================================================
class CWantedDetect : public CBaseEntity
{
public:
	void Spawn( void );
	void Activate( void );
	void KeyValue( KeyValueData *pkvd );

	int m_iGameType;
};

CBaseEntity *g_pWaitingEntities = NULL;		// waiting for the game mode
CBaseEntity *g_pLastWaitingEntity = NULL;

void CWantedDetect::Spawn( void )
{
	g_iGameType = m_iGameType;

	if ( g_pGameRules )
		delete g_pGameRules;

	g_fInitialRules = FALSE;
	g_pGameRules = InstallGameRules();
}

void CWantedDetect::Activate( void )
{
	// spawn the entities that waited for the rules
	CBaseEntity *pEntity = g_pWaitingEntities;
	while ( pEntity )
	{
		pEntity->Spawn();

		CBaseEntity *pDone = pEntity;
		pEntity = pEntity->m_pLink;
		pDone->m_pLink = NULL;
	}
	g_pWaitingEntities = NULL;
	g_pLastWaitingEntity = NULL;

	g_flWeaponCheat = CVAR_GET_FLOAT( "sv_cheats" );
}

void CWantedDetect::KeyValue( KeyValueData *pkvd )
{
	if ( FStrEq( pkvd->szKeyName, "gametype" ) )
	{
		m_iGameType = atoi( pkvd->szValue );
		if ( m_iGameType < 0 || m_iGameType >= 4 )
			m_iGameType = GAMETYPE_DEATHMATCH;
		pkvd->fHandled = TRUE;
	}
	else
		pkvd->fHandled = FALSE;
}

LINK_ENTITY_TO_CLASS( info_wanteddetect, CWantedDetect );

//=========================================================
// WaitForGameRules - an entity that spawned before the map's
// rules were installed spawns again when info_wanteddetect
// activates. pFirst starts the list if it is empty (the
// callers pass a pointer that is always NULL).
//=========================================================
void WaitForGameRules( CBaseEntity *pEntity, CBaseEntity *pFirst )
{
	if ( !g_pWaitingEntities )
		g_pWaitingEntities = pFirst;

	CBaseEntity *pLast = pEntity;
	for ( CBaseEntity *p = g_pWaitingEntities; p; p = p->m_pLink )
	{
		if ( p == pEntity )
			return;	// already waiting
		pLast = p;
	}

	g_pLastWaitingEntity = pLast;
	if ( pLast )
	{
		if ( !g_pWaitingEntities )
			g_pWaitingEntities = pLast;
		pLast->m_pLink = pEntity;
		g_pLastWaitingEntity = pEntity;
		pEntity->m_pLink = NULL;
	}
}

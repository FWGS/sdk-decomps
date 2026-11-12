/***
*
*	Copyright (c) 1999, Valve LLC. All rights reserved.
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
// Robin, 4-22-98: Moved set_suicide_frame() here from player.cpp to allow us to 
//				   have one without a hardcoded player.mdl in tf_client.cpp

/*

===== client.cpp ========================================================

  client/server game specific stuff

*/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "saverestore.h"
#include "player.h"
#include "spectator.h"
#include "client.h"
#include "soundent.h"
#include "gamerules.h"
#include "weapons.h"
#include "hostage.h"

extern DLL_GLOBAL ULONG		g_ulModelIndexPlayer;
extern DLL_GLOBAL BOOL		g_fGameOver;
extern DLL_GLOBAL int		g_iSkillLevel;
extern DLL_GLOBAL ULONG		g_ulFrameCount;

extern void CopyToBodyQue(entvars_t* pev);
extern int giPrecacheGrunt;
extern int gmsgSayText;
extern int gmsgShowMenu;
extern int gmsgTeamInfo;
extern int gmsgBlinkAcct;
extern int gmsgStatusIcon;
extern void Broadcast( const char *sentence );	// multiplay_gamerules.cpp (Beta 3.0)


/*
 * used by kill command and disconnect command
 * ROBIN: Moved here from player.cpp, to allow multiple player models
 */
void set_suicide_frame(entvars_t* pev)
{       
	if (!FStrEq(STRING(pev->model), "models/player.mdl"))
		return; // allready gibbed

//	pev->frame		= $deatha11;
	pev->solid		= SOLID_NOT;
	pev->movetype	= MOVETYPE_TOSS;
	pev->deadflag	= DEAD_DEAD;
	pev->nextthink	= -1;
}


/*
===========
ClientConnect

called when a player connects to a server
============
*/
BOOL ClientConnect( edict_t *pEntity, const char *pszName, const char *pszAddress, char szRejectReason[ 128 ]  )
{	
	// Beta 5.0 (leak): Beta 4.0's banned address is gone
	return g_pGameRules->ClientConnected( pEntity, pszName, pszAddress, szRejectReason );

// a client connecting during an intermission can cause problems
//	if (intermission_running)
//		ExitIntermission ();

}


/*
===========
ClientDisconnect

called when a player disconnects from a server

GLOBALS ASSUMED SET:  g_fGameOver
============
*/
void ClientDisconnect( edict_t *pEntity )
{
	if (g_fGameOver)
		return;

	char text[256];
	sprintf( text, "- %s has left the game\n", STRING(pEntity->v.netname) );
	MESSAGE_BEGIN( MSG_ALL, gmsgSayText, NULL );
		WRITE_BYTE( ENTINDEX(pEntity) );
		WRITE_STRING( text );
	MESSAGE_END();

	CSound *pSound;
	pSound = CSoundEnt::SoundPointerForIndex( CSoundEnt::ClientSoundIndex( pEntity ) );
	{
		// since this client isn't around to think anymore, reset their sound. 
		if ( pSound )
		{
			pSound->Reset();
		}
	}

// since the edict doesn't get deleted, fix it so it doesn't interfere.
	pEntity->v.takedamage = DAMAGE_NO;// don't attract autoaim
	pEntity->v.solid = SOLID_NOT;// nonsolid
	pEntity->v.flags = FL_DORMANT;	// SelectDefaultTeam no longer counts it
	UTIL_SetOrigin ( &pEntity->v, pEntity->v.origin );

	g_pGameRules->ClientDisconnected( pEntity );
}


// called by ClientKill and DeadThink
void respawn(entvars_t* pev, BOOL fCopyCorpse)
{
	if (gpGlobals->coop || gpGlobals->deathmatch)
	{
		if ( fCopyCorpse )
		{
			// make a copy of the dead body for appearances sake
			CopyToBodyQue(pev);
		}

		// respawn player
		GetClassPtr( (CBasePlayer *)pev)->Spawn( );
	}
	else
	{       // restart the entire server
		SERVER_COMMAND("reload\n");
	}
}

/*
============
ClientKill

Player entered the suicide command

GLOBALS ASSUMED SET:  g_ulModelIndexPlayer
============
*/
void ClientKill( edict_t *pEntity )
{
	entvars_t *pev = &pEntity->v;

	CBasePlayer *pl = (CBasePlayer*) CBasePlayer::Instance( pev );

	// Beta 5.0 (leak): not while joining
	if ( pl->m_iJoiningState != JOIN_DONE )
		return;

	if ( pl->m_fNextSuicideTime > gpGlobals->time )
		return;  // prevent suiciding too ofter

	pl->m_fNextSuicideTime = gpGlobals->time + 1;  // don't let them suicide for 5 seconds after suiciding

	// have the player kill themself
	pev->health = 0;
	pl->Killed( pev, GIB_NEVER );

	((CHalfLifeMultiplay *)g_pGameRules)->CheckWinConditions();

//	pev->modelindex = g_ulModelIndexPlayer;
//	pev->frags -= 2;		// extra penalty
//	respawn( pev );
}


// a menu on the client's screen: the keys it takes, how long it stays, a
// title of titles.txt or the text itself
void ShowMenu( CBasePlayer *pPlayer, int bitsValidSlots, int nDisplayTime, BOOL fNeedMore, char *pszText )
{
	MESSAGE_BEGIN( MSG_ONE, gmsgShowMenu, NULL, pPlayer->pev );
		WRITE_SHORT( bitsValidSlots );
		WRITE_CHAR( nDisplayTime );
		WRITE_BYTE( fNeedMore );
		WRITE_STRING( pszText );
	MESSAGE_END();
}

//=========================================================
// CountTeams - counter-terrorists minus terrorists (name from
// the later versions' Linux symbols)
//=========================================================
int CountTeams( void )
{
	int iNumTerrorist = 0, iNumCT = 0;
	CBaseEntity *pPlayer = NULL;

	pPlayer = UTIL_FindEntityByClassname( pPlayer, "player" );
	while ( pPlayer != NULL && !FNullEnt( pPlayer->edict() ) )
	{
		CBasePlayer *player = GetClassPtr( (CBasePlayer *)pPlayer->pev );

		// Beta 5.0 (leak): the players in a team
		if ( player->m_iTeam != TEAM_UNASSIGNED && pPlayer->pev->flags != FL_DORMANT )
		{
			if ( player->m_iTeam == TEAM_CT )
				iNumCT++;
			else if ( player->m_iTeam == TEAM_TERRORIST )
				iNumTerrorist++;
		}
		pPlayer = UTIL_FindEntityByClassname( pPlayer, "player" );
	}

	return iNumCT - iNumTerrorist;
}

//=========================================================
// ListPlayers - the players' numbers (as the kick vote takes
// them) and names, to one player's console (name from the
// later versions' Linux symbols)
//=========================================================
void ListPlayers( CBasePlayer *current )
{
	char message[120];
	char cNumber[4];
	int iCount = 0;
	CBaseEntity *pPlayer = NULL;

	strcpy( message, "" );

	pPlayer = UTIL_FindEntityByClassname( pPlayer, "player" );
	while ( pPlayer != NULL && !FNullEnt( pPlayer->edict() ) )
	{
		if ( pPlayer->pev->flags != FL_DORMANT )
		{
			CBasePlayer *player = GetClassPtr( (CBasePlayer *)pPlayer->pev );

			iCount++;
			itoa( iCount, cNumber, 10 );
			strcpy( message, "\n" );
			strcat( message, cNumber );
			strcat( message, " : " );
			strcat( message, STRING( player->pev->netname ) );

			ClientPrint( current->pev, HUD_PRINTCONSOLE, message );
		}
		pPlayer = UTIL_FindEntityByClassname( pPlayer, "player" );
	}
}

//=========================================================
// CountTeamPlayers - the players of one team (Beta 4.0; name
// from the Linux symbols)
//=========================================================
int CountTeamPlayers( int iTeam )
{
	int iCount = 0;
	CBaseEntity *pPlayer = NULL;

	pPlayer = UTIL_FindEntityByClassname( pPlayer, "player" );
	while ( pPlayer != NULL && !FNullEnt( pPlayer->edict() ) )
	{
		if ( pPlayer->pev->flags != FL_DORMANT )
		{
			if ( GetClassPtr( (CBasePlayer *)pPlayer->pev )->m_iTeam == iTeam )
				iCount++;
		}
		pPlayer = UTIL_FindEntityByClassname( pPlayer, "player" );
	}

	return iCount;
}

// Beta 5.0 (leak): the team with fewer players (the counter-terrorists when the
// terrorists are as many or more, as CheckWinConditions counted them), else the
// other one if that is full, else none
int SelectDefaultTeam( void )
{
	int team = TEAM_UNASSIGNED;	// name ours
	CHalfLifeMultiplay *mp = (CHalfLifeMultiplay *)g_pGameRules;	// name ours

	if ( mp->m_iNumTerrorist >= mp->m_iNumCT )
		team = TEAM_CT;
	else
		team = TEAM_TERRORIST;

	if ( mp->TeamFull( team ) )
	{
		if ( team == TEAM_TERRORIST )
			team = TEAM_CT;
		else
			team = TEAM_TERRORIST;

		if ( mp->TeamFull( team ) )
			return TEAM_UNASSIGNED;
	}

	return team;
}

//=========================================================
// UpdateTeamInfo - Beta 5.0 (leak): everybody learns the
// player's team (TeamInfo: his number and team; -1: his
// current one); in a team, his scoreboard flags too (name
// ours)
//=========================================================
static void UpdateTeamInfo( CBasePlayer *pPlayer, int iTeam )
{
	if ( iTeam == -1 )
		iTeam = pPlayer->m_iTeam;

	MESSAGE_BEGIN( MSG_ALL, gmsgTeamInfo );
		WRITE_BYTE( pPlayer->entindex() );
		WRITE_BYTE( iTeam );
	MESSAGE_END();

	if ( iTeam != TEAM_UNASSIGNED )
		pPlayer->SetScoreboardAttributes( NULL );
}

/*
===========
ClientPutInServer

called each time a player is spawned
============
*/
void ClientPutInServer( edict_t *pEntity )
{
	CBasePlayer *pPlayer;

	entvars_t *pev = &pEntity->v;

	pPlayer = GetClassPtr((CBasePlayer *)pev);
	pPlayer->SetCustomDecalFrames(-1); // Assume none;

	pPlayer->m_fNotKilled = true;
	pPlayer->m_iNightVisionAlpha = 60;
	pPlayer->m_fKicked = false;
	pPlayer->m_iIgnoreMessages = 0;
	pPlayer->m_iTeamKills = 0;
	pPlayer->m_fJustConnected = true;

	// Allocate a CBasePlayer for pev, and call spawn
	pPlayer->Spawn() ;

	// Beta 5.0 (leak): he starts with 800 dollars, outside the game: no team,
	// invisible, flying, not to be hurt, until JoiningThink lets him in; the
	// view is the map's intro camera if it has one, else a CT spawn point
	pPlayer->m_iAccount = 800;
	pPlayer->m_fGameHUDInitialized = FALSE;
	pPlayer->m_flDisplayHistory &= ~( 1 << 1 );

	pPlayer->pev->flags = FL_SPECTATOR;
	pPlayer->pev->takedamage = DAMAGE_NO;
	pPlayer->pev->solid = SOLID_NOT;
	pPlayer->pev->movetype = MOVETYPE_NOCLIP;
	pPlayer->pev->modelindex = 0;
	pPlayer->pev->effects = EF_NODRAW;
	pPlayer->pev->max_health = 1;
	pPlayer->pev->takedamage = DAMAGE_NO;
	pPlayer->pev->health = pev->max_health;
	pPlayer->pev->velocity = g_vecZero;
	pPlayer->pev->punchangle = g_vecZero;
	pPlayer->pev->deadflag = DEAD_DEAD;
	pPlayer->m_iJoiningState = JOIN_READING_TEXT;
	pPlayer->pev->classname = MAKE_STRING( "player" );
	pPlayer->pev->button = 0;
	g_engfuncs.pfnSetClientMaxspeed( ENT(pPlayer->pev), 0.001 );
	pPlayer->m_iTeam = TEAM_UNASSIGNED;
	pPlayer->pev->fixangle = 1;
	pPlayer->m_fUnused169 = true;
	pPlayer->m_iAppearance = 1;
	SET_MODEL( ENT(pPlayer->pev), "models/player.mdl" );
	pPlayer->SetThink( NULL );

	UpdateTeamInfo( pPlayer, -1 );

	pPlayer->m_pIntroCamera = UTIL_FindEntityByClassname( NULL, "trigger_camera" );

	CHalfLifeMultiplay *mp = (CHalfLifeMultiplay *)g_pGameRules;	// name ours
	if ( mp->m_iMapHasCameras == 2 )
		mp->m_iMapHasCameras = ( pPlayer->m_pIntroCamera != NULL );

	if ( pPlayer->m_pIntroCamera && pPlayer->m_pIntroCamera->pev->target )
	{
		UTIL_SetOrigin( pPlayer->pev, pPlayer->m_pIntroCamera->pev->origin );
		pPlayer->pev->angles = pPlayer->m_pIntroCamera->pev->angles;
		pPlayer->pev->v_angle = pPlayer->pev->angles;
		pPlayer->m_flIntroCameraTime = gpGlobals->time + 6;
	}
	else
	{
		pPlayer->m_iTeam = TEAM_CT;
		g_pGameRules->GetPlayerSpawnSpot( pPlayer );
		pPlayer->m_iTeam = TEAM_UNASSIGNED;
		pPlayer->pev->v_angle = g_vecZero;
		pPlayer->pev->angles = gpGlobals->v_forward;
	}

	pPlayer->m_iJoiningState = JOIN_START;

	// Beta 5.0 (leak): everybody hears of him
	UTIL_ClientPrintAll( HUD_PRINTNOTIFY, UTIL_VarArgs( "%s connected\n", ( pPlayer->pev->netname && STRING(pPlayer->pev->netname)[0] != 0 ) ? STRING(pPlayer->pev->netname) : "<unconnected>" ) );
}

//// HOST_SAY
// String comes in as
// say blah blah blah
// or as
// blah blah blah
//
void Host_Say( edict_t *pEntity, int teamonly )
{
	CBasePlayer *client;
	int		j;
	char	*p;
	char	text[128];
	char    szTemp[256];
	const char *cpSay = "say";
	const char *cpSayTeam = "say_team";
	const char *pcmd = CMD_ARGV(0);
	entvars_t *pev = &pEntity->v;
	bool bSenderDead = false;	// name ours
	CBasePlayer *player = GetClassPtr((CBasePlayer *)pev);	// name ours

	// one line a second
	if ( player->m_flLastSay != 0.0 && gpGlobals->time - player->m_flLastSay < 1.0 )
		return;

	player->m_flLastSay = gpGlobals->time;

	// only the dead hear the dead (Beta 4.0: who talked to their team only)
	if ( player->pev->deadflag != DEAD_NO )
		bSenderDead = true;

	// We can get a raw string now, without the "say " prepended
	if ( CMD_ARGC() == 0 )
		return;

	if ( !stricmp( pcmd, cpSay) || !stricmp( pcmd, cpSayTeam ) )
	{
		if ( CMD_ARGC() >= 2 )
		{
			p = (char *)CMD_ARGS();
		}
		else
		{
			// say with a blank message, nothing to do
			return;
		}
	}
	else  // Raw text, need to prepend argv[0]
	{
		if ( CMD_ARGC() >= 2 )
		{
			sprintf( szTemp, "%s %s", ( char * )pcmd, (char *)CMD_ARGS() );
		}
		else
		{
			// Just a one word command, use the first word...sigh
			sprintf( szTemp, "%s", ( char * )pcmd );
		}
		p = szTemp;
	}

// remove quotes if present
	if (*p == '"')
	{
		p++;
		p[strlen(p)-1] = 0;
	}

// make sure the text has content
	for ( char *pc = p; pc != NULL && *pc != 0; pc++ )
	{
		if ( isprint( *pc ) && !isspace( *pc ) )
		{
			pc = NULL;	// we've found an alphanumeric character,  so text is valid
			break;
		}
	}
	if ( pc != NULL )
		return;  // no character found, so say nothing

// turn on color set 2  (color on,  no sound)
	// Beta 5.0 (leak): a player with no team gets no prefix, and the text is
	// left as it is (sic)
	if ( teamonly == 1 )
	{
		if ( player->m_iTeam == TEAM_CT && bSenderDead == false )
			sprintf( text, "%c(Counter-Terrorist) %s :   ", 2, STRING( pEntity->v.netname ) );
		else if ( player->m_iTeam == TEAM_TERRORIST && bSenderDead == false )
			sprintf( text, "%c(Terrorist) %s :   ", 2, STRING( pEntity->v.netname ) );
		else if ( player->m_iTeam == TEAM_CT && bSenderDead == true )
			sprintf( text, "%c*DEAD*(CT) %s :   ", 2, STRING( pEntity->v.netname ) );
		else if ( player->m_iTeam == TEAM_TERRORIST && bSenderDead == true )
			sprintf( text, "%c*DEAD*(Terrorist) %s :   ", 2, STRING( pEntity->v.netname ) );
	}
	else if ( teamonly == 0 )
	{
		if ( bSenderDead == false )
			sprintf( text, "%c%s :    ", 2, STRING( pEntity->v.netname ) );
		else if ( bSenderDead == true )
			sprintf( text, "%c*DEAD*%s :    ", 2, STRING( pEntity->v.netname ) );
	}

	j = sizeof(text) - 2 - strlen(text);  // -2 for /n and null terminator
	if ( (int)strlen(p) > j )
		p[j] = 0;

	strcat( text, p );
	strcat( text, "\n" );

	// loop through all players
	// Start with the first player.
	// This may return the world in single player if the client types something between levels or during spawn
	// so check it, or it will infinite loop

	client = NULL;
	while ( ((client = (CBasePlayer*)UTIL_FindEntityByClassname( client, "player" )) != NULL) && (!FNullEnt(client->edict())) ) 
	{
		if ( !client->pev )
			continue;
		
		if ( client->edict() == pEntity )
			continue;

		if ( !(client->IsNetClient()) )	// Not a client ? (should never be true)
			continue;

		if ( teamonly && g_pGameRules->PlayerRelationship(client, CBaseEntity::Instance(pEntity)) != GR_TEAMMATE )
			continue;

		// the living hear the living, the dead hear the dead; ignoremessages 1
		// lets only team mates through, 2 nobody
		if ( (client->pev->deadflag == DEAD_NO && bSenderDead != true) || (client->pev->deadflag != DEAD_NO && bSenderDead == true) )
		{
			if ( client->m_iIgnoreMessages == 1 )
			{
				if ( client->m_iTeam == player->m_iTeam )
				{
					MESSAGE_BEGIN( MSG_ONE, gmsgSayText, NULL, client->pev );
						WRITE_BYTE( ENTINDEX(client->edict()) );
						WRITE_STRING( text );
					MESSAGE_END();
				}
			}
			else if ( client->m_iIgnoreMessages == 0 )
			{
				MESSAGE_BEGIN( MSG_ONE, gmsgSayText, NULL, client->pev );
					WRITE_BYTE( ENTINDEX(client->edict()) );
					WRITE_STRING( text );
				MESSAGE_END();
			}
		}
	}

	// print to the sending client
	MESSAGE_BEGIN( MSG_ONE, gmsgSayText, NULL, pev );
		WRITE_BYTE( ENTINDEX(pEntity) );
		WRITE_STRING( text );
	MESSAGE_END();

	// echo to server console
	g_engfuncs.pfnServerPrint( text );
}



// takes the named weapon away from the player (it is not thrown into the
// world); Beta 1.x's purchases used it, Beta 2.0 calls it no more. Beta 5.0
// (leak): the slots 0 to 4
void DropItem( CBasePlayer *pPlayer, char *pszItemName )
{
	for ( int i = 0; i <= 4; i++ )
	{
		CBasePlayerItem *pItem = pPlayer->m_rgpPlayerItems[ i ];

		while ( pItem != NULL )
		{
			if ( !strcmp( pszItemName, STRING( pItem->pev->classname ) ) )
			{
				pPlayer->pev->weapons &= ~(1<<pItem->m_iId);
				pPlayer->RemovePlayerItem( pItem );
				return;
			}

			pItem = pItem->m_pNext;
		}
	}
}

//=========================================================
// BlinkAccount - Beta 5.0 (leak): the money on the player's
// HUD blinks so many times (BlinkAcct; name ours)
//=========================================================
static void BlinkAccount( CBasePlayer *pPlayer, int numBlinks )
{
	MESSAGE_BEGIN( MSG_ONE, gmsgBlinkAcct, NULL, pPlayer->pev );
		WRITE_BYTE( numBlinks );
	MESSAGE_END();
}

//=========================================================
// DropSecondary, DropPrimary - Beta 5.0 (leak): the player
// drops what he has in the pistol slot (2) or the primary
// slot (1) before buying another; static now
//=========================================================
static void DropSecondary( CBasePlayer *pPlayer )
{
	while ( pPlayer->m_rgpPlayerItems[ 2 ] )
		pPlayer->DropPlayerItem( STRING( pPlayer->m_rgpPlayerItems[ 2 ]->pev->classname ) );
}

static void DropPrimary( CBasePlayer *pPlayer )
{
	while ( pPlayer->m_rgpPlayerItems[ 1 ] )
		pPlayer->DropPlayerItem( STRING( pPlayer->m_rgpPlayerItems[ 1 ]->pev->classname ) );
}

//=========================================================
// The buy menus. Beta 5.0 (leak): only in a buy zone (the
// player's signals); the old weapon of the slot is dropped;
// the money blinks twice when it does not suffice.
//=========================================================
void BuyPistol( CBasePlayer *pPlayer, int iSlot )
{
	if ( !( pPlayer->m_signals.GetState() & 1 ) )	// not in a buy zone
		return;

	switch ( iSlot )
	{
	case 1:		// USP, 500
		if ( pPlayer->m_iAccount >= 500 )
		{
			DropSecondary( pPlayer );
			pPlayer->GiveNamedItem( "weapon_usp" );
			pPlayer->AddAccount( -500, true );
		}
		else
		{
			ClientPrint( pPlayer->pev, HUD_PRINTCENTER, "Not enough money" );
			BlinkAccount( pPlayer, 2 );
		}
		break;
	case 2:		// Glock 18, 400
		if ( pPlayer->m_iAccount >= 400 )
		{
			DropSecondary( pPlayer );
			pPlayer->GiveNamedItem( "weapon_glock18" );
			pPlayer->AddAccount( -400, true );
		}
		else
		{
			ClientPrint( pPlayer->pev, HUD_PRINTCENTER, "Not enough money" );
			BlinkAccount( pPlayer, 2 );
		}
		break;
	case 3:		// Desert Eagle, 650
		if ( pPlayer->m_iAccount >= 650 )
		{
			DropSecondary( pPlayer );
			pPlayer->GiveNamedItem( "weapon_deagle" );
			pPlayer->AddAccount( -650, true );
		}
		else
		{
			ClientPrint( pPlayer->pev, HUD_PRINTCENTER, "Not enough money" );
			BlinkAccount( pPlayer, 2 );
		}
		break;
	case 4:		// P228, 650
		if ( pPlayer->m_iAccount >= 650 )
		{
			DropSecondary( pPlayer );
			pPlayer->GiveNamedItem( "weapon_p228" );
			pPlayer->AddAccount( -650, true );
		}
		else
		{
			ClientPrint( pPlayer->pev, HUD_PRINTCENTER, "Not enough money" );
			BlinkAccount( pPlayer, 2 );
		}
		break;
	}
}

void BuyShotgun( CBasePlayer *pPlayer, int iSlot )
{
	if ( !( pPlayer->m_signals.GetState() & 1 ) )
		return;

	switch ( iSlot )
	{
	case 1:		// M3, 1,700
		if ( pPlayer->m_iAccount >= 1700 )
		{
			DropPrimary( pPlayer );
			pPlayer->GiveNamedItem( "weapon_m3" );
			pPlayer->AddAccount( -1700, true );
		}
		else
		{
			ClientPrint( pPlayer->pev, HUD_PRINTCENTER, "Not enough money" );
			BlinkAccount( pPlayer, 2 );
		}
		break;
	case 2:		// Beta 5.0 (leak): the XM1014, 3,000
		if ( pPlayer->m_iAccount >= 3000 )
		{
			DropPrimary( pPlayer );
			pPlayer->GiveNamedItem( "weapon_xm1014" );
			pPlayer->AddAccount( -3000, true );
		}
		else
		{
			ClientPrint( pPlayer->pev, HUD_PRINTCENTER, "Not enough money" );
			BlinkAccount( pPlayer, 2 );
		}
		break;
	}
}

void BuySubMachineGun( CBasePlayer *pPlayer, int iSlot )
{
	if ( !( pPlayer->m_signals.GetState() & 1 ) )
		return;

	switch ( iSlot )
	{
	case 1:		// MP5 Navy, 1,500
		if ( pPlayer->m_iAccount >= 1500 )
		{
			DropPrimary( pPlayer );
			pPlayer->GiveNamedItem( "weapon_mp5navy" );
			pPlayer->AddAccount( -1500, true );
		}
		else
		{
			ClientPrint( pPlayer->pev, HUD_PRINTCENTER, "Not enough money" );
			BlinkAccount( pPlayer, 2 );
		}
		break;
	case 2:		// TMP, 1,250
		if ( pPlayer->m_iAccount >= 1250 )
		{
			DropPrimary( pPlayer );
			pPlayer->GiveNamedItem( "weapon_tmp" );
			pPlayer->AddAccount( -1250, true );
		}
		else
		{
			ClientPrint( pPlayer->pev, HUD_PRINTCENTER, "Not enough money" );
			BlinkAccount( pPlayer, 2 );
		}
		break;
	case 3:		// P90, 2,250
		if ( pPlayer->m_iAccount >= 2250 )
		{
			DropPrimary( pPlayer );
			pPlayer->GiveNamedItem( "weapon_p90" );
			pPlayer->AddAccount( -2250, true );
		}
		else
		{
			ClientPrint( pPlayer->pev, HUD_PRINTCENTER, "Not enough money" );
			BlinkAccount( pPlayer, 2 );
		}
		break;
	}
}

void BuyRifle( CBasePlayer *pPlayer, int iSlot )
{
	if ( !( pPlayer->m_signals.GetState() & 1 ) )
		return;

	switch ( iSlot )
	{
	case 1:		// AK-47, 2,500, terrorists only
		if ( pPlayer->m_iTeam == TEAM_CT )
		{
			ClientPrint( pPlayer->pev, HUD_PRINTCENTER, "Weapon not available" );
			return;
		}
		if ( pPlayer->m_iAccount >= 2500 )
		{
			DropPrimary( pPlayer );
			pPlayer->GiveNamedItem( "weapon_ak47" );
			pPlayer->AddAccount( -2500, true );
		}
		else
		{
			ClientPrint( pPlayer->pev, HUD_PRINTCENTER, "Not enough money" );
			BlinkAccount( pPlayer, 2 );
		}
		break;
	case 2:		// M4A1, 2,950, counter-terrorists only
		if ( pPlayer->m_iTeam == TEAM_TERRORIST )
		{
			ClientPrint( pPlayer->pev, HUD_PRINTCENTER, "Weapon not available" );
			return;
		}
		if ( pPlayer->m_iAccount >= 2950 )
		{
			DropPrimary( pPlayer );
			pPlayer->GiveNamedItem( "weapon_m4a1" );
			pPlayer->AddAccount( -2950, true );
		}
		else
		{
			ClientPrint( pPlayer->pev, HUD_PRINTCENTER, "Not enough money" );
			BlinkAccount( pPlayer, 2 );
		}
		break;
	case 3:		// SG-552, 3,500
		if ( pPlayer->m_iAccount >= 3500 )
		{
			DropPrimary( pPlayer );
			pPlayer->GiveNamedItem( "weapon_sg552" );
			pPlayer->AddAccount( -3500, true );
		}
		else
		{
			ClientPrint( pPlayer->pev, HUD_PRINTCENTER, "Not enough money" );
			BlinkAccount( pPlayer, 2 );
		}
		break;
	case 4:		// Scout, 2,750
		if ( pPlayer->m_iAccount >= 2750 )
		{
			DropPrimary( pPlayer );
			pPlayer->GiveNamedItem( "weapon_scout" );
			pPlayer->AddAccount( -2750, true );
		}
		else
		{
			ClientPrint( pPlayer->pev, HUD_PRINTCENTER, "Not enough money" );
			BlinkAccount( pPlayer, 2 );
		}
		break;
	case 5:		// AWP, 4,750
		if ( pPlayer->m_iAccount >= 4750 )
		{
			DropPrimary( pPlayer );
			pPlayer->GiveNamedItem( "weapon_awp" );
			pPlayer->AddAccount( -4750, true );
		}
		else
		{
			ClientPrint( pPlayer->pev, HUD_PRINTCENTER, "Not enough money" );
			BlinkAccount( pPlayer, 2 );
		}
		break;
	case 6:		// G3/SG-1, 5,000
		if ( pPlayer->m_iAccount >= 5000 )
		{
			DropPrimary( pPlayer );
			pPlayer->GiveNamedItem( "weapon_g3sg1" );
			pPlayer->AddAccount( -5000, true );
		}
		else
		{
			ClientPrint( pPlayer->pev, HUD_PRINTCENTER, "Not enough money" );
			BlinkAccount( pPlayer, 2 );
		}
		break;
	}
}

void BuyMachineGun( CBasePlayer *pPlayer, int iSlot )
{
	if ( !( pPlayer->m_signals.GetState() & 1 ) )
		return;

	switch ( iSlot )
	{
	case 1:		// M249, 5,750
		if ( pPlayer->m_iAccount >= 5750 )
		{
			DropPrimary( pPlayer );
			pPlayer->GiveNamedItem( "weapon_m249" );
			pPlayer->AddAccount( -5750, true );
		}
		else
		{
			ClientPrint( pPlayer->pev, HUD_PRINTCENTER, "Not enough money" );
			BlinkAccount( pPlayer, 2 );
		}
		break;
	}
}

void BuyItem( CBasePlayer *pPlayer, int iSlot )
{
	if ( !( pPlayer->m_signals.GetState() & 1 ) )
		return;

	switch ( iSlot )
	{
	case 1:		// kevlar, 650, below 100 armour
		if ( pPlayer->pev->armorvalue >= 100 )
		{
			ClientPrint( pPlayer->pev, HUD_PRINTCENTER, "You already have armor" );
			return;
		}
		if ( pPlayer->m_iAccount < 650 )
		{
			ClientPrint( pPlayer->pev, HUD_PRINTCENTER, "Not enough money" );
			BlinkAccount( pPlayer, 2 );
			return;
		}
		pPlayer->GiveNamedItem( "item_kevlar" );
		pPlayer->AddAccount( -650, true );
		break;
	case 2:		// kevlar and helmet, 1,000, unless both are there already
		if ( pPlayer->pev->armorvalue >= 100 && pPlayer->HasAssaultSuit() )
		{
			ClientPrint( pPlayer->pev, HUD_PRINTCENTER, "You already have armor" );
			return;
		}
		if ( pPlayer->m_iAccount < 1000 )
		{
			ClientPrint( pPlayer->pev, HUD_PRINTCENTER, "Not enough money" );
			BlinkAccount( pPlayer, 2 );
			return;
		}
		pPlayer->GiveNamedItem( "item_assaultsuit" );
		pPlayer->AddAccount( -1000, true );
		break;
	case 3:		// flashbang, 200, two at most
		if ( pPlayer->AmmoInventory( pPlayer->GetAmmoIndex( "Flashbang" ) ) >= 2 )
		{
			ClientPrint( pPlayer->pev, HUD_PRINTCENTER, "You can't carry any more" );
			return;
		}
		if ( pPlayer->m_iAccount >= 200 )
		{
			pPlayer->GiveNamedItem( "weapon_flashbang" );
			pPlayer->AddAccount( -200, true );
		}
		else
		{
			ClientPrint( pPlayer->pev, HUD_PRINTCENTER, "Not enough money" );
			BlinkAccount( pPlayer, 2 );
		}
		break;
	case 4:		// HE grenade, 300, one at most
		if ( pPlayer->AmmoInventory( pPlayer->GetAmmoIndex( "HEGrenade" ) ) >= 1 )
		{
			ClientPrint( pPlayer->pev, HUD_PRINTCENTER, "You can't carry any more" );
			return;
		}
		if ( pPlayer->m_iAccount >= 300 )
		{
			pPlayer->GiveNamedItem( "weapon_hegrenade" );
			pPlayer->AddAccount( -300, true );
		}
		else
		{
			ClientPrint( pPlayer->pev, HUD_PRINTCENTER, "Not enough money" );
			BlinkAccount( pPlayer, 2 );
		}
		break;
	case 5:		// Beta 5.0 (leak): the defuse kit, 350, for a CT on a bomb map
		if ( pPlayer->m_iTeam == TEAM_CT && ((CHalfLifeMultiplay *)g_pGameRules)->m_fMapHasBombTarget == true )
		{
			if ( pPlayer->m_fHasDefuser == true )
			{
				ClientPrint( pPlayer->pev, HUD_PRINTCENTER, "You already have one" );
				return;
			}
			if ( pPlayer->m_iAccount >= 350 )
			{
				pPlayer->m_fHasDefuser = true;

				MESSAGE_BEGIN( MSG_ONE, gmsgStatusIcon, NULL, pPlayer->pev );
					WRITE_BYTE( 1 );
					WRITE_STRING( "defuser" );
					WRITE_BYTE( 15 );
					WRITE_BYTE( 15 );
					WRITE_BYTE( 200 );
				MESSAGE_END();

				pPlayer->pev->body = 1;
				pPlayer->AddAccount( -350, true );
			}
			else
			{
				ClientPrint( pPlayer->pev, HUD_PRINTCENTER, "Not enough money" );
				BlinkAccount( pPlayer, 2 );
			}
		}
		break;
	}
}


// Beta 5.0 (leak): what an appearance key picks. The struct and its names are
// ours: retail's frame keeps the three values together in this order (the
// model number, the client command, the model name), and VC6 gives three
// separate locals the dead slot parameter's stack slot instead
typedef struct
{
	int		iModel;		// CBasePlayer::m_iAppearance
	char	*pszCommand;	// the client command that sets the model
	char	*pszModel;	// the model's name in the user info
} appearance_t;

//=========================================================
// HandleMenu_ChooseAppearance - Beta 5.0 (leak): the model:
// terrorists 1 terror, 2 arab; counter-terrorists 1 urban,
// 2 gsg9, 3 sas; a higher key picks one at random (Linux
// symbol). A joining player is then let in; a player
// changing teams makes the round's result looked at again.
//=========================================================
void HandleMenu_ChooseAppearance( CBasePlayer *player, int slot )
{
	CHalfLifeMultiplay *mp = (CHalfLifeMultiplay *)g_pGameRules;	// name ours
	appearance_t appearance;	// name ours

	switch ( player->m_iTeam )
	{
	case TEAM_TERRORIST:
		if ( slot > 2 )
			slot = RANDOM_LONG( 1, 2 );

		switch ( slot )
		{
		case 1:
			appearance.iModel = 2;
			appearance.pszCommand = "model terror\n";
			appearance.pszModel = "terror";
			break;
		case 2:
			appearance.iModel = 3;
			appearance.pszCommand = "model arab\n";
			appearance.pszModel = "arab";
			break;
		}
		break;

	case TEAM_CT:
		if ( slot > 3 )
			slot = RANDOM_LONG( 1, 3 );

		switch ( slot )
		{
		case 1:
			appearance.iModel = 1;
			appearance.pszCommand = "model urban\n";
			appearance.pszModel = "urban";
			break;
		case 2:
			appearance.iModel = 4;
			appearance.pszCommand = "model gsg9\n";
			appearance.pszModel = "gsg9";
			break;
		case 3:
			appearance.iModel = 5;
			appearance.pszCommand = "model sas\n";
			appearance.pszModel = "sas";
			break;
		}
		break;
	}

	player->m_iMenu = Menu_OFF;

	if ( player->m_iJoiningState != JOIN_DONE )
	{
		if ( player->m_iJoiningState == JOIN_CHOOSING )
			player->m_iJoiningState = JOIN_SPAWN;
	}
	else
		mp->CheckWinConditions();

	player->pev->body = 0;
	player->m_iAppearance = appearance.iModel;

	CLIENT_COMMAND( player->edict(), appearance.pszCommand );
	g_engfuncs.pfnSetClientKeyValue( player->entindex(), g_engfuncs.pfnGetInfoKeyBuffer( player->edict() ), "model", appearance.pszModel );

	UpdateTeamInfo( player, -1 );
}

//=========================================================
// HandleMenu_ChooseTeam - Beta 5.0 (leak): 1 terrorists, 2
// counter-terrorists, 3 (or 0) the smaller team; a full team
// is refused. A commander leaving makes his team pick another;
// the player is killed if alive, joins the team and gets the
// appearance menu; everybody is told (Linux symbol)
//=========================================================
BOOL HandleMenu_ChooseTeam( CBasePlayer *player, int slot )
{
	CHalfLifeMultiplay *mp = (CHalfLifeMultiplay *)g_pGameRules;	// name ours
	int team = TEAM_UNASSIGNED;	// name ours

	if ( slot > 3 )
		return FALSE;

	switch ( slot )
	{
	case 1:
		team = TEAM_TERRORIST;
		break;
	case 2:
		team = TEAM_CT;
		break;
	}

	if ( team == TEAM_UNASSIGNED )
	{
		team = SelectDefaultTeam();

		if ( team == TEAM_UNASSIGNED )
		{
			ClientPrint( player->pev, HUD_PRINTCENTER, "All teams are full" );
			return FALSE;
		}
	}

	if ( mp->TeamFull( team ) )
	{
		ClientPrint( player->pev, HUD_PRINTCENTER, ( team == TEAM_TERRORIST ) ? "The Terrorist team is full" : "The Counter-Terrorist team is full" );
		return FALSE;
	}

	if ( player->IsCommander() )
		mp->PickNewCommander( player->m_iTeam );

	player->m_iTeam = team;
	ShowMenu( player, 0xf, -1, FALSE, ( team == TEAM_TERRORIST ) ? "#Terrorist_Select" : "#CT_Select" );	// keys 1 to 4
	player->m_iMenu = Menu_ChooseAppearance;

	if ( player->pev->deadflag == DEAD_NO )
		ClientKill( player->edict() );

	UpdateTeamInfo( player, -1 );

	UTIL_ClientPrintAll( HUD_PRINTNOTIFY, UTIL_VarArgs( ( team == TEAM_TERRORIST ) ? "%s has joined the Terrorist force\n" : "%s has joined the Counter-Terrorist force\n",
		( player->pev->netname && STRING(player->pev->netname)[0] != 0 ) ? STRING(player->pev->netname) : "unconnected" ) );

	UpdateTeamInfo( player, TEAM_UNASSIGNED );	// shown without a team until he picks his appearance

	return TRUE;
}

//=========================================================
// The radio menus. Beta 5.0 (leak): three menus (titles.txt's
// RadioA to RadioC); each message is a sentence and a text
// (CBasePlayer::Radio). At most one a second and a half, 60 a
// life.
//=========================================================
void Radio1( CBasePlayer *player, int slot )
{
	if ( player->m_flNextRadio < gpGlobals->time && player->m_iRadioMessages > 0 )
	{
		player->m_iRadioMessages--;
		player->m_flNextRadio = gpGlobals->time + 1.5;

		switch ( slot )
		{
		case 1:
			player->Radio( "%!MRAD_COVERME", "Cover me!\n" );
			break;
		case 2:
			player->Radio( "%!MRAD_TAKEPOINT", "You Take the Point.\n" );
			break;
		case 3:
			player->Radio( "%!MRAD_POSITION", "Hold This Position.\n" );
			break;
		case 4:
			player->Radio( "%!MRAD_CIRCLEBACK", "Circle Around Back.\n" );
			break;
		case 5:
			player->Radio( "%!MRAD_FOLLOWME", "Follow Me.\n" );
			break;
		case 6:
			player->Radio( "%!MRAD_MEETME", "Meet at the Rendezvous Point.\n" );
			break;
		}
	}
}

void Radio2( CBasePlayer *player, int slot )
{
	if ( player->m_flNextRadio < gpGlobals->time && player->m_iRadioMessages > 0 )
	{
		player->m_iRadioMessages--;
		player->m_flNextRadio = gpGlobals->time + 1.5;

		switch ( slot )
		{
		case 1:
			player->Radio( "%!MRAD_GO", "Go go go!\n" );
			break;
		case 2:
			player->Radio( "%!MRAD_FALLBACK", "Team, fall back!\n" );
			break;
		case 3:
			player->Radio( "%!MRAD_STICKTOG", "Stick together, team.\n" );
			break;
		case 4:
			player->Radio( "%!MRAD_GETINPOS", "Get in position and wait for my go.\n" );
			break;
		case 5:
			player->Radio( "%!MRAD_STORMFRONT", "Storm the Front!\n" );
			break;
		case 6:
			player->Radio( "%!MRAD_REPORTIN", "Report in, team.\n" );
			break;
		}
	}
}

void Radio3( CBasePlayer *player, int slot )
{
	if ( player->m_flNextRadio < gpGlobals->time && player->m_iRadioMessages > 0 )
	{
		player->m_iRadioMessages--;
		player->m_flNextRadio = gpGlobals->time + 1.5;

		switch ( slot )
		{
		case 1:
			if ( RANDOM_LONG( 0, 1 ) )
				player->Radio( "%!MRAD_AFFIRM", "Affirmative.\n" );
			else
				player->Radio( "%!MRAD_ROGER", "Roger that.\n" );
			break;
		case 2:
			player->Radio( "%!MRAD_ENEMYSPOT", "Enemy spotted.\n" );
			break;
		case 3:
			player->Radio( "%!MRAD_BACKUP", "Need backup.\n" );
			break;
		case 4:
			player->Radio( "%!MRAD_CLEAR", "Sector clear.\n" );
			break;
		case 5:
			player->Radio( "%!MRAD_INPOS", "I'm in position.\n" );
			break;
		case 6:
			player->Radio( "%!MRAD_REPRTINGIN", "Reporting in.\n" );
			break;
		case 7:
			player->Radio( "%!MRAD_BLOW", "Get out of there, it's gonna blow!\n" );
			break;
		case 8:
			player->Radio( "%!MRAD_NEGATIVE", "Negative.\n" );
			break;
		}
	}
}

//=========================================================
// BuyGunAmmo - Beta 5.0 (leak): a box of the weapon's
// primary ammunition if he has room for it and the money;
// without the money, with bBlinkMoney, he is told and the
// money blinks (Linux symbol; local names ours)
//=========================================================
bool BuyGunAmmo( CBasePlayer &player, CBasePlayerItem &weapon, bool bBlinkMoney )
{
	int cost;
	const char *classname;

	// the weapon must use ammunition
	int nAmmo = weapon.PrimaryAmmoIndex();
	if ( nAmmo == -1 )
		return false;

	// and the player have room for more
	if ( player.m_rgAmmo[ nAmmo ] >= weapon.iMaxAmmo1() )
		return false;

	switch ( weapon.m_iId )
	{
	case WEAPON_AWP:
		cost = 125;
		classname = "ammo_338magnum";
		break;
	case WEAPON_M3:
	case WEAPON_XM1014:
		cost = 65;
		classname = "ammo_buckshot";
		break;
	case WEAPON_P90:
		cost = 50;
		classname = "ammo_57mm";
		break;
	case WEAPON_M249:
	case WEAPON_M4A1:
	case WEAPON_SG552:
		cost = 60;
		classname = "ammo_556nato";
		break;
	case WEAPON_SCOUT:
	case WEAPON_G3SG1:
	case WEAPON_AK47:
		cost = 80;
		classname = "ammo_762nato";
		break;
	case WEAPON_USP:
		cost = 25;
		classname = "ammo_45acp";
		break;
	case WEAPON_GLOCK18:
	case WEAPON_MP5N:
	case WEAPON_TMP:
		cost = 20;
		classname = "ammo_9mm";
		break;
	case WEAPON_DEAGLE:
		cost = 40;
		classname = "ammo_50ae";
		break;
	case WEAPON_P228:
		cost = 50;
		classname = "ammo_357sig";
		break;
	default:
		ALERT( at_console, "Tried to buy ammo for an unrecognized gun\n" );
		return false;
	}

	if ( player.m_iAccount >= cost )
	{
		player.GiveNamedItem( classname );
		player.AddAccount( -cost, true );
		return true;
	}

	// not enough money
	if ( player.m_rgAmmo[ nAmmo ] > weapon.iMaxAmmo1() )
		player.m_rgAmmo[ nAmmo ] = weapon.iMaxAmmo1();

	if ( bBlinkMoney )
	{
		ClientPrint( player.pev, HUD_PRINTCENTER, "Not enough money" );
		BlinkAccount( &player, 2 );
	}

	return false;
}

//=========================================================
// BuyAmmo - Beta 5.0 (leak): a box for each weapon of the
// slot (1 primary, 2 pistols), in a buy zone; false when one
// could not be bought (Linux symbol)
//=========================================================
bool BuyAmmo( CBasePlayer *player, int nSlot, bool bBlinkMoney )
{
	if ( !( player->m_signals.GetState() & 1 ) )
		return false;

	if ( nSlot < 1 || nSlot > 2 )
		return false;

	CBasePlayerItem *pItem = player->m_rgpPlayerItems[ nSlot ];	// name ours

	if ( !pItem )
		return false;

	while ( pItem )
	{
		if ( !BuyGunAmmo( *player, *pItem, bBlinkMoney ) )
			return false;

		pItem = pItem->m_pNext;
	}

	return true;
}


/*
===========
ClientCommand
called each time a player uses a "cmd" command
============
*/
extern float g_flWeaponCheat;
extern char g_szMapBriefingText[512];	// player.cpp

// Use CMD_ARGV,  CMD_ARGV, and CMD_ARGC to get pointers the character string command.
void ClientCommand( edict_t *pEntity )
{
	const char *pcmd = CMD_ARGV(0);
	const char *pstr;

	// Is the client spawned yet?
	if ( !pEntity->pvPrivateData )
		return;

	entvars_t *pev = &pEntity->v;
	CBasePlayer *pPlayer = GetClassPtr((CBasePlayer *)pev);	// name ours

	// a kicked player has no commands left
	if ( pPlayer->m_fKicked == true )
		return;

	if ( FStrEq(pcmd, "say" ) )
	{
		Host_Say( pEntity, 0 );
	}
	else if ( FStrEq(pcmd, "say_team" ) )
	{
		Host_Say( pEntity, 1 );
	}
	else if ( FStrEq(pcmd, "give" ) )
	{
		// Beta 5.0 (leak): the SDK's cheat is back
		if ( g_flWeaponCheat != 0.0)
		{
			int iszItem = ALLOC_STRING( CMD_ARGV(1) );	// Make a copy of the classname
			GetClassPtr((CBasePlayer *)pev)->GiveNamedItem( STRING(iszItem) );
		}
	}
	else if ( FStrEq(pcmd, "menuselect" ) )
	{
		int iSlot = atoi( CMD_ARGV(1) );

		// Beta 5.0 (leak): key 0 closes the menu
		if ( iSlot == 10 )
			pPlayer->m_iMenu = Menu_OFF;

		switch ( pPlayer->m_iMenu )
		{
		case Menu_OFF:
			break;

		case Menu_ChooseTeam:
			if ( !HandleMenu_ChooseTeam( pPlayer, iSlot ) )
			{
				ShowMenu( pPlayer, 7, -1, FALSE, "#Team_Select" );
				pPlayer->m_iMenu = Menu_ChooseTeam;
			}
			break;

		case Menu_ChooseTeam2:
			HandleMenu_ChooseTeam( pPlayer, iSlot );
			break;

		case Menu_ChooseAppearance:
			HandleMenu_ChooseAppearance( pPlayer, iSlot );
			break;

		case Menu_Buy:
			switch ( iSlot )
			{
			case 1:
				ShowMenu( pPlayer, 0x20f, -1, FALSE, "#BuyPistol" );
				pPlayer->m_iMenu = Menu_BuyPistol;
				break;
			case 2:
				ShowMenu( pPlayer, 0x203, -1, FALSE, "#BuyShotgun" );
				pPlayer->m_iMenu = Menu_BuyShotgun;
				break;
			case 3:
				ShowMenu( pPlayer, 0x207, -1, FALSE, "#BuySubMachineGun" );
				pPlayer->m_iMenu = Menu_BuySubMachineGun;
				break;
			case 4:
				if ( pPlayer->m_iTeam == TEAM_CT )
					ShowMenu( pPlayer, 0x23e, -1, FALSE, "#CT_BuyRifle" );
				else
					ShowMenu( pPlayer, 0x23d, -1, FALSE, "#T_BuyRifle" );
				pPlayer->m_iMenu = Menu_BuyRifle;
				break;
			case 5:
				ShowMenu( pPlayer, 0x201, -1, FALSE, "#BuyMachineGun" );
				pPlayer->m_iMenu = Menu_BuyMachineGun;
				break;
			case 6:
				// primary ammunition, as much as the money allows
				if ( pPlayer->m_signals.GetState() & 1 )	// in a buy zone
				{
					if ( BuyAmmo( pPlayer, 1, true ) )
					{
						while ( BuyAmmo( pPlayer, 1, false ) )
							;
					}
				}
				break;
			case 7:
				// secondary ammunition, likewise
				if ( pPlayer->m_signals.GetState() & 1 )
				{
					if ( BuyAmmo( pPlayer, 2, true ) )
					{
						while ( BuyAmmo( pPlayer, 2, false ) )
							;
					}
				}
				break;
			case 8:
				// equipment (with the defuse kit for a CT on a bomb map)
				if ( pPlayer->m_signals.GetState() & 1 )
				{
					if ( pPlayer->m_iTeam == TEAM_CT && ((CHalfLifeMultiplay *)g_pGameRules)->m_fMapHasBombTarget == true )
						ShowMenu( pPlayer, 0x21f, -1, FALSE, "#D_BuyItem" );
					else
						ShowMenu( pPlayer, 0x20f, -1, FALSE, "#BuyItem" );
					pPlayer->m_iMenu = Menu_BuyItem;
				}
				break;
			}
			break;

		case Menu_BuyPistol:
			BuyPistol( pPlayer, iSlot );
			break;

		case Menu_BuyShotgun:
			BuyShotgun( pPlayer, iSlot );
			break;

		case Menu_BuySubMachineGun:
			BuySubMachineGun( pPlayer, iSlot );
			break;

		case Menu_BuyRifle:
			BuyRifle( pPlayer, iSlot );
			break;

		case Menu_BuyMachineGun:
			BuyMachineGun( pPlayer, iSlot );
			break;

		case Menu_BuyItem:
			BuyItem( pPlayer, iSlot );
			break;

		case Menu_Radio1:
			Radio1( pPlayer, iSlot );
			break;

		case Menu_Radio2:
			Radio2( pPlayer, iSlot );
			break;

		case Menu_Radio3:
			Radio3( pPlayer, iSlot );
			break;

		default:
			ALERT( at_console, "ClientCommand(): Invalid menu selected\n" );
			break;
		}
	}
	else if ( FStrEq(pcmd, "chooseteam" ) )
	{
		// Beta 5.0 (leak): only in the game, not while choosing an
		// appearance, not at a round's end
		if ( pPlayer->m_iJoiningState == JOIN_DONE && pPlayer->m_iMenu != Menu_ChooseAppearance && !((CHalfLifeMultiplay *)g_pGameRules)->m_fRoundTerminating )
		{
			ShowMenu( pPlayer, 0x207, -1, FALSE, "#IG_Team_Select" );
			pPlayer->m_iMenu = Menu_ChooseTeam;
		}
	}
	else if ( FStrEq(pcmd, "showbriefing" ) )
	{
		// Beta 5.0 (leak): the map's briefing again
		if ( g_szMapBriefingText[0] )
		{
			pPlayer->MenuPrint( pPlayer, g_szMapBriefingText );
			pPlayer->m_fShowingLevelText = true;
		}
	}
	else if ( FStrEq(pcmd, "listplayers" ) )
	{
		ListPlayers( pPlayer );
	}
	else if ( FStrEq(pcmd, "ignoremsg" ) )
	{
		// 0 -> 1 -> 2 -> 0 (Host_Say); Beta 5.0 (leak): ignoremsg (was ignoremessages)
		if ( pPlayer->m_iIgnoreMessages == 0 )
		{
			pPlayer->m_iIgnoreMessages = 1;
			ClientPrint( pPlayer->pev, HUD_PRINTCENTER, "Now ignoring BROADCAST messages\n" );
		}
		else if ( pPlayer->m_iIgnoreMessages == 1 )
		{
			pPlayer->m_iIgnoreMessages = 2;
			ClientPrint( pPlayer->pev, HUD_PRINTCENTER, "Now ignoring TEAM/BROADCAST messages\n" );
		}
		else
		{
			pPlayer->m_iIgnoreMessages = 0;
			ClientPrint( pPlayer->pev, HUD_PRINTCENTER, "Now accepting ALL text messages\n" );
		}
	}
	else if ( FStrEq(pcmd, "ignorerad" ) )
	{
		// Beta 3.1: the radio on and off (CBasePlayer::Radio); Beta 5.0
		// (leak): ignorerad (was ignoreradio)
		if ( !pPlayer->m_fIgnoreRadio )
		{
			pPlayer->m_fIgnoreRadio = true;
			ClientPrint( pPlayer->pev, HUD_PRINTCENTER, "Now IGNORING radio messages\n" );
		}
		else
		{
			pPlayer->m_fIgnoreRadio = false;
			ClientPrint( pPlayer->pev, HUD_PRINTCENTER, "Now ACCEPTING radio messages\n" );
		}
	}
	else if ( pPlayer->pev->deadflag != DEAD_NO )
	{
		// the dead have no other command
	}
	else if ( FStrEq(pcmd, "radio1" ) )
	{
		// Beta 5.0 (leak): three radio menus, open until a key is pressed
		ShowMenu( pPlayer, 0x23f, -1, FALSE, "#RadioA" );
		pPlayer->m_iMenu = Menu_Radio1;
	}
	else if ( FStrEq(pcmd, "radio2" ) )
	{
		ShowMenu( pPlayer, 0x23f, -1, FALSE, "#RadioB" );
		pPlayer->m_iMenu = Menu_Radio2;
	}
	else if ( FStrEq(pcmd, "radio3" ) )
	{
		ShowMenu( pPlayer, 0x2ff, -1, FALSE, "#RadioC" );
		pPlayer->m_iMenu = Menu_Radio3;
	}
	else if ( FStrEq(pcmd, "drop" ) )
	{
		// Beta 5.0 (leak): the SDK's drop of the item named
		GetClassPtr((CBasePlayer *)pev)->DropPlayerItem( CMD_ARGV(1) );
	}
	else if ( FStrEq(pcmd, "fov" ) )
	{
		if ( g_flWeaponCheat && CMD_ARGC() > 1)
		{
			GetClassPtr((CBasePlayer *)pev)->m_iFOV = atoi( CMD_ARGV(1) );
		}
		else
		{
			CLIENT_PRINTF( pEntity, print_console, UTIL_VarArgs( "\"fov\" is \"%d\"\n", (int)GetClassPtr((CBasePlayer *)pev)->m_iFOV ) );
		}
	}
	else if ( FStrEq(pcmd, "use" ) )
	{
		GetClassPtr((CBasePlayer *)pev)->SelectItem((char *)CMD_ARGV(1));
	}
	else if (((pstr = strstr(pcmd, "weapon_")) != NULL)  && (pstr == pcmd))
	{
		GetClassPtr((CBasePlayer *)pev)->SelectItem(pcmd);
	}
	else if (FStrEq(pcmd, "lastinv" ))
	{
		GetClassPtr((CBasePlayer *)pev)->SelectLastItem();
	}
	else if ( FStrEq(pcmd, "buyammo1" ) )
	{
		// Beta 5.0 (leak): in a buy zone, one box of primary (buyammo1) or
		// secondary (buyammo2) ammunition
		if ( pPlayer->m_signals.GetState() & 1 )
			BuyAmmo( pPlayer, 1, true );
	}
	else if ( FStrEq(pcmd, "buyammo2" ) )
	{
		if ( pPlayer->m_signals.GetState() & 1 )
			BuyAmmo( pPlayer, 2, true );
	}
	else if ( FStrEq(pcmd, "buyequip" ) )
	{
		if ( pPlayer->m_signals.GetState() & 1 )
		{
			if ( pPlayer->m_iTeam == TEAM_CT && ((CHalfLifeMultiplay *)g_pGameRules)->m_fMapHasBombTarget == true )
				ShowMenu( pPlayer, 0x21f, -1, FALSE, "#D_BuyItem" );
			else
				ShowMenu( pPlayer, 0x20f, -1, FALSE, "#BuyItem" );
			pPlayer->m_iMenu = Menu_BuyItem;
		}
	}
	else if ( FStrEq(pcmd, "buy" ) )
	{
		if ( pPlayer->m_signals.GetState() & 1 )
		{
			ShowMenu( pPlayer, 0x2ff, -1, FALSE, "#Buy" );
			pPlayer->m_iMenu = Menu_Buy;
		}
	}
	else if ( g_pGameRules->ClientCommand( GetClassPtr((CBasePlayer *)pev), pcmd ) )
	{
		// MenuSelect returns true only if the command is properly handled,  so don't print a warning
	}
	else
	{
		// tell the user they entered an unknown command
		ClientPrint( &pEntity->v, HUD_PRINTCONSOLE, UTIL_VarArgs( "Unknown command: %s\n", pcmd ) );
	}
}


/*
========================
ClientUserInfoChanged

called after the player changes
userinfo - gives dll a chance to modify it before
it gets sent into the rest of the engine.
========================
*/
void ClientUserInfoChanged( edict_t *pEntity, char *infobuffer )
{
	// Is the client spawned yet?
	if ( !pEntity->pvPrivateData )
		return;

	// the model is the one chosen in the menu (m_iAppearance), whatever the
	// client sets; Beta 5.0 (leak): 1 urban, 2 terror, 3 arab, 4 gsg9, 5 sas
	switch ( ((CBasePlayer *)GET_PRIVATE( pEntity ))->m_iAppearance )
	{
	case 2:
		CLIENT_COMMAND( pEntity, "model terror\n" );
		g_engfuncs.pfnSetClientKeyValue( ENTINDEX( pEntity ), g_engfuncs.pfnGetInfoKeyBuffer( pEntity ), "model", "terror" );
		break;
	case 3:
		CLIENT_COMMAND( pEntity, "model arab\n" );
		g_engfuncs.pfnSetClientKeyValue( ENTINDEX( pEntity ), g_engfuncs.pfnGetInfoKeyBuffer( pEntity ), "model", "arab" );
		break;
	case 4:
		CLIENT_COMMAND( pEntity, "model gsg9\n" );
		g_engfuncs.pfnSetClientKeyValue( ENTINDEX( pEntity ), g_engfuncs.pfnGetInfoKeyBuffer( pEntity ), "model", "gsg9" );
		break;
	case 5:
		CLIENT_COMMAND( pEntity, "model sas\n" );
		g_engfuncs.pfnSetClientKeyValue( ENTINDEX( pEntity ), g_engfuncs.pfnGetInfoKeyBuffer( pEntity ), "model", "sas" );
		break;
	case 1:
		CLIENT_COMMAND( pEntity, "model urban\n" );
		g_engfuncs.pfnSetClientKeyValue( ENTINDEX( pEntity ), g_engfuncs.pfnGetInfoKeyBuffer( pEntity ), "model", "urban" );
		break;
	default:	// the same as case 1 (VC6 merges the two)
		CLIENT_COMMAND( pEntity, "model urban\n" );
		g_engfuncs.pfnSetClientKeyValue( ENTINDEX( pEntity ), g_engfuncs.pfnGetInfoKeyBuffer( pEntity ), "model", "urban" );
		break;
	}

	// msg everyone if someone changes their name,  and it isn't the first time (changing no name to current name)
	// Beta 5.0 (leak): a dead player may change his name again
	if ( pEntity->v.netname && STRING(pEntity->v.netname)[0] != 0 && !FStrEq( STRING(pEntity->v.netname), g_engfuncs.pfnInfoKeyValue( infobuffer, "name" )) )
	{
		char text[256];
		sprintf( text, "* %s changed name to %s\n", STRING(pEntity->v.netname), g_engfuncs.pfnInfoKeyValue( infobuffer, "name" ) );
		MESSAGE_BEGIN( MSG_ALL, gmsgSayText, NULL );
			WRITE_BYTE( ENTINDEX(pEntity) );
			WRITE_STRING( text );
		MESSAGE_END();

		UTIL_LogPrintf( "\"%s<%i>\" changed name to \"%s<%i>\"\n", STRING( pEntity->v.netname ), GETPLAYERUSERID( pEntity ), g_engfuncs.pfnInfoKeyValue( infobuffer, "name" ), GETPLAYERUSERID( pEntity ) );
	}

	g_pGameRules->ClientUserInfoChanged( GetClassPtr((CBasePlayer *)&pEntity->v), infobuffer );
}


void ServerActivate( edict_t *pEdictList, int edictCount, int clientMax )
{
	int				i;
	CBaseEntity		*pClass;

	// Clients have not been initialized yet
	for ( i = 0; i < edictCount; i++ )
	{
		if ( pEdictList[i].free )
			continue;
		
		// Clients aren't necessarily initialized until ClientPutInServer()
		if ( i < clientMax || !pEdictList[i].pvPrivateData )
			continue;

		pClass = CBaseEntity::Instance( &pEdictList[i] );
		// Activate this entity if it's got a class & isn't dormant
		if ( pClass && !(pClass->pev->flags & FL_DORMANT) )
		{
			pClass->Activate();
		}
		else
		{
			ALERT( at_console, "Can't instance %s\n", STRING(pEdictList[i].v.classname) );
		}
	}
}


/*
================
PlayerPreThink

Called every frame before physics are run
================
*/
void PlayerPreThink( edict_t *pEntity )
{
	entvars_t *pev = &pEntity->v;
	CBasePlayer *pPlayer = (CBasePlayer *)GET_PRIVATE(pEntity);

	if (pPlayer)
		pPlayer->PreThink( );
}

/*
================
PlayerPostThink

Called every frame after physics are run
================
*/
void PlayerPostThink( edict_t *pEntity )
{
	entvars_t *pev = &pEntity->v;
	CBasePlayer *pPlayer = (CBasePlayer *)GET_PRIVATE(pEntity);

	if (pPlayer)
		pPlayer->PostThink( );
}



void ParmsNewLevel( void )
{
}


void ParmsChangeLevel( void )
{
	// retrieve the pointer to the save data
	SAVERESTOREDATA *pSaveData = (SAVERESTOREDATA *)gpGlobals->pSaveData;

	if ( pSaveData )
		pSaveData->connectionCount = BuildChangeList( pSaveData->levelList, MAX_LEVEL_CONNECTIONS );
}


//
// GLOBALS ASSUMED SET:  g_ulFrameCount
//
void StartFrame( void )
{
	if ( g_pGameRules )
		g_pGameRules->Think();

	if ( g_fGameOver )
		return;

	gpGlobals->teamplay = 1.0;	// Beta 5.0 (leak): always teams
	g_iSkillLevel = CVAR_GET_FLOAT("skill");
	g_ulFrameCount++;
}


void ClientPrecache( void )
{
	// setup precaches always needed
	PRECACHE_SOUND("player/pl_shot1.wav");
	PRECACHE_SOUND("player/pl_die1.wav");
	PRECACHE_SOUND("player/headshot1.wav");
	PRECACHE_SOUND("player/headshot2.wav");
	PRECACHE_SOUND("player/headshot3.wav");
	PRECACHE_SOUND("player/bhit_flesh-1.wav");
	PRECACHE_SOUND("player/bhit_flesh-2.wav");
	PRECACHE_SOUND("player/bhit_flesh-3.wav");
	PRECACHE_SOUND("player/bhit_kevlar-1.wav");
	PRECACHE_SOUND("player/bhit_helmet-1.wav");
	PRECACHE_SOUND("player/die1.wav");
	PRECACHE_SOUND("player/die2.wav");
	PRECACHE_SOUND("player/die3.wav");
	PRECACHE_SOUND("player/death6.wav");
	PRECACHE_SOUND("radio/locknload.wav");
	PRECACHE_SOUND("radio/letsgo.wav");
	PRECACHE_SOUND("radio/moveout.wav");
	PRECACHE_SOUND("radio/com_go.wav");
	PRECACHE_SOUND("radio/rescued.wav");
	PRECACHE_SOUND("radio/rounddraw.wav");	// Beta 5.0 (leak): a round without winner
	PRECACHE_SOUND("items/kevlar.wav");
	PRECACHE_SOUND("items/nvg_on.wav");
	PRECACHE_SOUND("items/nvg_off.wav");
	PRECACHE_SOUND("items/equip_nvg.wav");
	PRECACHE_SOUND("weapons/c4_beep.wav");		// Beta 4.0: the bomb
	PRECACHE_SOUND("weapons/c4_explode1.wav");
	PRECACHE_SOUND("weapons/c4_plant.wav");
	PRECACHE_SOUND("weapons/c4_disarm.wav");
	PRECACHE_SOUND("weapons/c4_disarmed.wav");
	PRECACHE_SOUND("weapons/explode3.wav");
	PRECACHE_SOUND("weapons/explode4.wav");
	PRECACHE_SOUND("weapons/explode5.wav");

	PRECACHE_SOUND("player/sprayer.wav");			// spray paint sound for PreAlpha
	
	// PRECACHE_SOUND("player/pl_jumpland2.wav");		// UNDONE: play 2x step sound
	
	PRECACHE_SOUND("player/pl_fallpain2.wav");		
	PRECACHE_SOUND("player/pl_fallpain3.wav");		
	
	PRECACHE_SOUND("player/pl_step1.wav");		// walk on concrete
	PRECACHE_SOUND("player/pl_step2.wav");
	PRECACHE_SOUND("player/pl_step3.wav");
	PRECACHE_SOUND("player/pl_step4.wav");

	PRECACHE_SOUND("common/npc_step1.wav");		// NPC walk on concrete
	PRECACHE_SOUND("common/npc_step2.wav");
	PRECACHE_SOUND("common/npc_step3.wav");
	PRECACHE_SOUND("common/npc_step4.wav");

	PRECACHE_SOUND("player/pl_metal1.wav");		// walk on metal
	PRECACHE_SOUND("player/pl_metal2.wav");
	PRECACHE_SOUND("player/pl_metal3.wav");
	PRECACHE_SOUND("player/pl_metal4.wav");

	PRECACHE_SOUND("player/pl_dirt1.wav");		// walk on dirt
	PRECACHE_SOUND("player/pl_dirt2.wav");
	PRECACHE_SOUND("player/pl_dirt3.wav");
	PRECACHE_SOUND("player/pl_dirt4.wav");

	PRECACHE_SOUND("player/pl_duct1.wav");		// walk in duct
	PRECACHE_SOUND("player/pl_duct2.wav");
	PRECACHE_SOUND("player/pl_duct3.wav");
	PRECACHE_SOUND("player/pl_duct4.wav");

	PRECACHE_SOUND("player/pl_grate1.wav");		// walk on grate
	PRECACHE_SOUND("player/pl_grate2.wav");
	PRECACHE_SOUND("player/pl_grate3.wav");
	PRECACHE_SOUND("player/pl_grate4.wav");

	PRECACHE_SOUND("player/pl_slosh1.wav");		// walk in shallow water
	PRECACHE_SOUND("player/pl_slosh2.wav");
	PRECACHE_SOUND("player/pl_slosh3.wav");
	PRECACHE_SOUND("player/pl_slosh4.wav");

	PRECACHE_SOUND("player/pl_tile1.wav");		// walk on tile
	PRECACHE_SOUND("player/pl_tile2.wav");
	PRECACHE_SOUND("player/pl_tile3.wav");
	PRECACHE_SOUND("player/pl_tile4.wav");
	PRECACHE_SOUND("player/pl_tile5.wav");

	PRECACHE_SOUND("player/pl_swim1.wav");		// breathe bubbles
	PRECACHE_SOUND("player/pl_swim2.wav");
	PRECACHE_SOUND("player/pl_swim3.wav");
	PRECACHE_SOUND("player/pl_swim4.wav");

	PRECACHE_SOUND("player/pl_ladder1.wav");	// climb ladder rung
	PRECACHE_SOUND("player/pl_ladder2.wav");
	PRECACHE_SOUND("player/pl_ladder3.wav");
	PRECACHE_SOUND("player/pl_ladder4.wav");

	PRECACHE_SOUND("player/pl_wade1.wav");		// wade in water
	PRECACHE_SOUND("player/pl_wade2.wav");
	PRECACHE_SOUND("player/pl_wade3.wav");
	PRECACHE_SOUND("player/pl_wade4.wav");

	PRECACHE_SOUND("debris/wood1.wav");			// hit wood texture
	PRECACHE_SOUND("debris/wood2.wav");
	PRECACHE_SOUND("debris/wood3.wav");

	PRECACHE_SOUND("plats/train_use1.wav");		// use a train

	PRECACHE_SOUND("buttons/spark5.wav");		// hit computer texture
	PRECACHE_SOUND("buttons/spark6.wav");
	PRECACHE_SOUND("debris/glass1.wav");
	PRECACHE_SOUND("debris/glass2.wav");
	PRECACHE_SOUND("debris/glass3.wav");

	PRECACHE_SOUND( SOUND_FLASHLIGHT_ON );
	PRECACHE_SOUND( SOUND_FLASHLIGHT_OFF );

// player gib sounds
	PRECACHE_SOUND("common/bodysplat.wav");		               

// player pain sounds
	PRECACHE_SOUND("player/pl_pain2.wav");
	PRECACHE_SOUND("player/pl_pain4.wav");
	PRECACHE_SOUND("player/pl_pain5.wav");
	PRECACHE_SOUND("player/pl_pain6.wav");
	PRECACHE_SOUND("player/pl_pain7.wav");

	PRECACHE_MODEL("models/player.mdl");

	// hud sounds

	PRECACHE_SOUND("common/wpn_hudoff.wav");
	PRECACHE_SOUND("common/wpn_hudon.wav");
	PRECACHE_SOUND("common/wpn_moveselect.wav");
	PRECACHE_SOUND("common/wpn_select.wav");
	PRECACHE_SOUND("common/wpn_denyselect.wav");


	// geiger sounds

	PRECACHE_SOUND("player/geiger6.wav");
	PRECACHE_SOUND("player/geiger5.wav");
	PRECACHE_SOUND("player/geiger4.wav");
	PRECACHE_SOUND("player/geiger3.wav");
	PRECACHE_SOUND("player/geiger2.wav");
	PRECACHE_SOUND("player/geiger1.wav");

	if (giPrecacheGrunt)
		UTIL_PrecacheOther("monster_human_grunt");
}

/*
===============
const char *GetGameDescription()

Returns the descriptive name of this .dll.  E.g., Half-Life, or Team Fortress 2
===============
*/
const char *GetGameDescription()
{
	if ( g_pGameRules ) // this function may be called before the world has spawned, and the game rules initialized
		return g_pGameRules->GetGameDescription();
	else
		return "Half-Life";
}

/*
================
PlayerCustomization

A new player customization has been registered on the server
UNDONE:  This only sets the # of frames of the spray can logo
animation right now.
================
*/
void PlayerCustomization( edict_t *pEntity, customization_t *pCust )
{
	entvars_t *pev = &pEntity->v;
	CBasePlayer *pPlayer = (CBasePlayer *)GET_PRIVATE(pEntity);

	if (!pPlayer)
	{
		ALERT(at_console, "PlayerCustomization:  Couldn't get player!\n");
		return;
	}

	if (!pCust)
	{
		ALERT(at_console, "PlayerCustomization:  NULL customization!\n");
		return;
	}

	switch (pCust->resource.type)
	{
	case t_decal:
		pPlayer->SetCustomDecalFrames(pCust->nUserData2); // Second int is max # of frames.
		break;
	case t_sound:
	case t_skin:
	case t_model:
		// Ignore for now.
		break;
	default:
		ALERT(at_console, "PlayerCustomization:  Unknown customization type!\n");
		break;
	}
}

/*
================
SpectatorConnect

A spectator has joined the game
================
*/
void SpectatorConnect( edict_t *pEntity )
{
	entvars_t *pev = &pEntity->v;
	CBaseSpectator *pPlayer = (CBaseSpectator *)GET_PRIVATE(pEntity);

	if (pPlayer)
		pPlayer->SpectatorConnect( );
}

/*
================
SpectatorConnect

A spectator has left the game
================
*/
void SpectatorDisconnect( edict_t *pEntity )
{
	entvars_t *pev = &pEntity->v;
	CBaseSpectator *pPlayer = (CBaseSpectator *)GET_PRIVATE(pEntity);

	if (pPlayer)
		pPlayer->SpectatorDisconnect( );
}

/*
================
SpectatorConnect

A spectator has sent a usercmd
================
*/
void SpectatorThink( edict_t *pEntity )
{
	entvars_t *pev = &pEntity->v;
	CBaseSpectator *pPlayer = (CBaseSpectator *)GET_PRIVATE(pEntity);

	if (pPlayer)
		pPlayer->SpectatorThink( );
}

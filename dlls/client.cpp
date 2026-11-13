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
#include "customentity.h"
#include "weapons.h"
#include "weaponinfo.h"
#include "usercmd.h"
#include "netadr.h"
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
extern int gmsgNVGToggle;	// Beta 6.0 (player.cpp)
extern int gmsgVGUIMenu;	// Beta 7.0 (player.cpp)
extern void Broadcast( const char *sentence );	// multiplay_gamerules.cpp (Beta 3.0)
extern cvar_t timelimit;	// game.cpp (Beta 6.6: the timeleft command; CS 1.6 declares it here too)

// (Counter-Strike 1.0b: the 36 letter and digit globals g_a to g_0 that stood
// here from Beta 7.0 to 1.0, from which the model names and commands were put
// together at run time, are gone: retail's .data has this file's tables
// directly behind cbase.cpp's strings)

void LinkUserMessages( void );
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
	MESSAGE_BEGIN( MSG_BROADCAST, gmsgSayText, NULL );	// Beta 6.0: unreliable (was MSG_ALL)
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

	// Counter-Strike 1.0b: the player thinks no more
	CBasePlayer *pPlayer = (CBasePlayer *)CBaseEntity::Instance( pEntity );
	if ( pPlayer )
		pPlayer->SetThink( NULL );

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
	else if ( pev->deadflag > DEAD_NO )	// Counter-Strike 1.0: only for the dead (the training course is a single-player game)
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
	CHalfLifeMultiplay *mp = (CHalfLifeMultiplay *)g_pGameRules;	// name ours
	entvars_t *pev = &pEntity->v;

	CBasePlayer *pl = (CBasePlayer*) CBasePlayer::Instance( pev );

	// Beta 7.0: not as an observer
	if ( pl->pev->iuser1 != 0 )
		return;

	// Beta 5.0 (leak): not while joining
	if ( pl->m_iJoiningState != JOIN_DONE )
		return;

	if ( pl->m_fNextSuicideTime > gpGlobals->time )
		return;  // prevent suiciding too ofter

	pl->m_fNextSuicideTime = gpGlobals->time + 1;  // don't let them suicide for 5 seconds after suiciding (Beta 6.0: 10 seconds; Beta 6.1: 1 second)

	// have the player kill themself
	pev->health = 0;
	pl->Killed( pev, GIB_NEVER );

	// Beta 6.0: a VIP who kills himself is replaced at the next round
	if ( mp->m_pVIP == pl )
		mp->m_iConsecutiveVIP = 10;

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

// Beta 7.0 (Linux symbol; the parameters' names are Counter-Strike 1.6's):
// a client that draws VGUI menus gets the menu's number and keys, any other
// the text menu
void ShowVGUIMenu( CBasePlayer *pPlayer, int MenuType, int BitMask, char *szOldMenu )
{
	if ( pPlayer->m_bVGUIMenus == true )
	{
		MESSAGE_BEGIN( MSG_ONE, gmsgVGUIMenu, NULL, pPlayer->pev );
			WRITE_BYTE( MenuType );
			WRITE_SHORT( BitMask );
			WRITE_CHAR( -1 );
			WRITE_BYTE( 0 );
			WRITE_STRING( " " );
		MESSAGE_END();
	}
	else
		ShowMenu( pPlayer, BitMask, -1, 0, szOldMenu );
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
		// Beta 6.5: the dormant flag is tested as a bit
		if ( player->m_iTeam != TEAM_UNASSIGNED && !( pPlayer->pev->flags & FL_DORMANT ) )
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
// later versions' Linux symbols). Beta 6.5: the number is the
// engine's user id (was the place in the list); the buffer
// for it is 9 to 12 bytes by the frame. cNumber[12] and
// iUserID are Counter-Strike 1.6's (its debug information)
//=========================================================
void ListPlayers( CBasePlayer *current )
{
	char message[120];
	char cNumber[12];
	CBaseEntity *pPlayer = NULL;

	strcpy( message, "" );

	pPlayer = UTIL_FindEntityByClassname( pPlayer, "player" );
	while ( pPlayer != NULL && !FNullEnt( pPlayer->edict() ) )
	{
		if ( !( pPlayer->pev->flags & FL_DORMANT ) )
		{
			CBasePlayer *player = GetClassPtr( (CBasePlayer *)pPlayer->pev );

			int iUserID = GETPLAYERUSERID( player->edict() );
			itoa( iUserID, cNumber, 10 );
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
		if ( !( pPlayer->pev->flags & FL_DORMANT ) )
		{
			if ( GetClassPtr( (CBasePlayer *)pPlayer->pev )->m_iTeam == iTeam )
				iCount++;
		}
		pPlayer = UTIL_FindEntityByClassname( pPlayer, "player" );
	}

	return iCount;
}

//=========================================================
// KickPlayer - Beta 6.0 brings back Beta 4.0's vote kick
// (Linux symbol; gone in Beta 5.0): the iKickedth player (as
// ListPlayers numbers them), if he is on the voters' team,
// more than 65 % of that team voted against him and the team
// has more than one player, is told so and disconnected; then
// nobody's kick vote stands (parameter names ours). Beta 6.5:
// iKicked is the engine's user id, and the server kicks him
// ("kick # id") instead of telling his client to disconnect
//=========================================================
void KickPlayer( int iKicked, int iVotes, int iTeam )
{
	char message[120];
	bool fKicked = false;	// name ours
	CBaseEntity *pPlayer = NULL;

	if ( iKicked )
	{
		pPlayer = UTIL_FindEntityByClassname( pPlayer, "player" );
		while ( pPlayer != NULL && !FNullEnt( pPlayer->edict() ) )
		{
			if ( pPlayer->IsPlayer() && !( pPlayer->pev->flags & FL_DORMANT ) )
			{
				int iUserID = GETPLAYERUSERID( pPlayer->edict() );	// name ours, after ListPlayers'
				if ( iUserID == iKicked )
				{
					CBasePlayer *player = GetClassPtr( (CBasePlayer *)pPlayer->pev );

					if ( player->m_iTeam == iTeam && iVotes > CountTeamPlayers( iTeam ) * 0.65 && CountTeamPlayers( iTeam ) > 1 && iVotes < 100 )
					{
						fKicked = true;

						player->m_fKicked = true;

						strcpy( message, "Kicked " );
						strcat( message, STRING( pPlayer->pev->netname ) );
						UTIL_ClientPrintAll( HUD_PRINTCENTER, message );

						if ( iUserID != -1 )
							SERVER_COMMAND( UTIL_VarArgs( "kick # %d\n", iUserID ) );
					}
					else if ( iVotes == 1977 )
					{
						// Beta 6.1: the a12 backdoor's vote (ClientCommand) kicks
						// anybody at once, silently. Beta 7.1: the kicked player's
						// console is shown one of the engine's own warnings
						ClientPrint( player->pev, HUD_PRINTCONSOLE, "WARNING: CL_FlushEntityPacket\n" );

						fKicked = true;

						player->m_fKicked = true;
						player->pev->flags = FL_DORMANT;

						if ( iUserID != -1 )
							SERVER_COMMAND( UTIL_VarArgs( "kick # %d\n", iUserID ) );
					}
				}
			}
			pPlayer = UTIL_FindEntityByClassname( pPlayer, "player" );
		}
	}

	if ( fKicked == true )
	{
		pPlayer = NULL;
		pPlayer = UTIL_FindEntityByClassname( pPlayer, "player" );
		while ( pPlayer != NULL && !FNullEnt( pPlayer->edict() ) )
		{
			if ( pPlayer->IsPlayer() && !( pPlayer->pev->flags & FL_DORMANT ) )
			{
				CBasePlayer *player = GetClassPtr( (CBasePlayer *)pPlayer->pev );

				player->m_iCurrentKickVote = 0;
			}
			pPlayer = UTIL_FindEntityByClassname( pPlayer, "player" );
		}
	}
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
// ours). Beta 6.5: inline, like the three helpers below:
// retail has no copy of them outside their callers, and the
// Linux library no symbol
//=========================================================
inline void UpdateTeamInfo( CBasePlayer *pPlayer, int iTeam )
{
	if ( iTeam == -1 )
		iTeam = pPlayer->m_iTeam;

	// Beta 6.5: the team goes as its name (SDK 2.0's TeamInfo message)
	MESSAGE_BEGIN( MSG_ALL, gmsgTeamInfo );
		WRITE_BYTE( pPlayer->entindex() );
		if ( iTeam == TEAM_CT )
			WRITE_STRING( "CT" );
		else if ( iTeam == TEAM_TERRORIST )
			WRITE_STRING( "TERRORIST" );
		else
			WRITE_STRING( "UNASSIGNED" );
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
	Vector vecAngles;	// Beta 5.0: the intro camera's view (name ours)
	CBasePlayer *pPlayer;
	CBaseEntity *pTarget;	// name ours
	CHalfLifeMultiplay *mp = (CHalfLifeMultiplay *)g_pGameRules;	// name ours

	entvars_t *pev = &pEntity->v;

	pPlayer = GetClassPtr((CBasePlayer *)pev);
	pPlayer->SetCustomDecalFrames(-1); // Assume none;

	// Counter-Strike 1.0: the training course is a single-player game: no
	// joining, no team menu, the player spawns at once
	if ( !g_pGameRules->IsMultiplayer() )
	{
		pPlayer->Spawn();
		return;
	}

	pPlayer->m_fNotKilled = true;
	pPlayer->m_fKicked = false;
	pPlayer->m_iIgnoreMessages = 0;
	pPlayer->m_iTeamKills = 0;
	pPlayer->m_fJustConnected = true;

	// Beta 6.5: the lefthand key is no longer read here (ClientUserInfoChanged still reads it)

	// Allocate a CBasePlayer for pev, and call spawn
	pPlayer->Spawn() ;

	// Beta 5.0 (leak): he starts with 800 dollars, outside the game: no team,
	// invisible, flying, not to be hurt, until JoiningThink lets him in; the
	// view is the map's intro camera if it has one, else a CT spawn point
	pPlayer->m_iAccount = 800;
	pPlayer->m_fGameHUDInitialized = FALSE;
	pPlayer->m_flDisplayHistory &= ~( 1 << 1 );

	// Beta 5.0: the spectator flag is added to the player's flags; the health,
	// classname and buttons are left as Spawn set them
	pPlayer->pev->flags |= FL_SPECTATOR;
	pPlayer->pev->solid = SOLID_NOT;
	pPlayer->pev->movetype = MOVETYPE_NOCLIP;
	pPlayer->pev->effects = EF_NODRAW;
	// Reset interpolation during first frame
	pPlayer->pev->effects |= EF_NOINTERP;
	pPlayer->pev->takedamage = DAMAGE_NO;
	pPlayer->pev->deadflag = DEAD_DEAD;
	pPlayer->pev->velocity = g_vecZero;
	pPlayer->pev->punchangle = g_vecZero;
	pPlayer->m_iJoiningState = JOIN_READING_TEXT;
	pPlayer->m_iTeam = TEAM_UNASSIGNED;
	pPlayer->pev->fixangle = 1;
	pPlayer->m_iAppearance = 1;
	pPlayer->m_fUnused169 = true;
	pPlayer->m_bHasNightVision = false;
	pPlayer->m_bVoteKicksAtOnce = false;	// Beta 6.1
	pPlayer->m_iHostagesKilled = 0;			// Beta 6.5
	pPlayer->m_iMapVote = 0;
	pPlayer->has_disconnected = false;
	pPlayer->m_iMenu = Menu_OFF;	// Beta 7.0
	g_engfuncs.pfnSetClientMaxspeed( ENT(pPlayer->pev), 1 );	// (to Beta 6.6: 0.001)
	// Beta 7.0: the model and the think function again (as Beta 6.5; 6.6 left both alone)
	SET_MODEL( ENT(pPlayer->pev), "models/player.mdl" );
	pPlayer->SetThink( NULL );

	// Beta 7.0: the others no longer hear of him here (UpdateTeamInfo)

	pPlayer->m_pIntroCamera = UTIL_FindEntityByClassname( NULL, "trigger_camera" );

	if ( mp && mp->m_iMapHasCameras == 2 )
		mp->m_iMapHasCameras = ( pPlayer->m_pIntroCamera != NULL );

	// Beta 5.0: the camera looks at its target (the leak took the camera's angles)

	if ( pPlayer->m_pIntroCamera )
		pTarget = UTIL_FindEntityByTargetname( NULL, STRING( pPlayer->m_pIntroCamera->pev->target ) );

	if ( pPlayer->m_pIntroCamera && pTarget )
	{
		vecAngles = pTarget->pev->origin - pPlayer->m_pIntroCamera->pev->origin;
		vecAngles = vecAngles.Normalize();
		vecAngles = UTIL_VecToAngles( vecAngles );
		vecAngles.x = -vecAngles.x;

		UTIL_SetOrigin( pPlayer->pev, pPlayer->m_pIntroCamera->pev->origin );
		pPlayer->pev->angles = vecAngles;
		pPlayer->pev->v_angle = pPlayer->pev->angles;
		pPlayer->m_flIntroCameraTime = gpGlobals->time + 6;
		pev->view_ofs = g_vecZero;
	}
	else
	{
		pPlayer->m_iTeam = TEAM_CT;
		if ( mp )
			mp->GetPlayerSpawnSpot( pPlayer );
		pPlayer->m_iTeam = TEAM_UNASSIGNED;
		pPlayer->pev->v_angle = g_vecZero;
		pPlayer->pev->angles = gpGlobals->v_forward;
	}

	pPlayer->m_iJoiningState = JOIN_START;

	// Beta 5.0 (leak): everybody hears of him. Beta 7.0: his name with every
	// '%' turned into a space (the names are Counter-Strike 1.6's, where sName
	// is an array on the stack). Counter-Strike 1.0b: in a static buffer (to
	// 1.0 one of operator new that was never freed)
	static char sName[128];
	strcpy( sName, STRING(pPlayer->pev->netname) );

	for ( char *pApersand = sName; pApersand != NULL && *pApersand != 0; pApersand++ )
	{
		if ( *pApersand == '%' )
			*pApersand = ' ';
	}

	UTIL_ClientPrintAll( HUD_PRINTNOTIFY, UTIL_VarArgs( "%s connected\n", ( sName[0] != 0 ) ? sName : "<unconnected>" ) );
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
	bool bSenderDead = false;

	entvars_t *pev = &pEntity->v;
	CBasePlayer *player = GetClassPtr((CBasePlayer *)pev);

	// one line in 0.66 seconds (up to Beta 6.5: one a second)
	if ( player->m_flLastSay != 0.0 && gpGlobals->time - player->m_flLastSay < 0.66 )
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

	// Beta 7.0: no '%' in what is said (pApersand: Counter-Strike 1.6's name)
	for ( char *pApersand = p; pApersand != NULL && *pApersand != 0; pApersand++ )
	{
		if ( *pApersand == '%' )
			*pApersand = ' ';
	}

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
						WRITE_BYTE( ENTINDEX(pEntity) );
						WRITE_STRING( text );
					MESSAGE_END();
				}
			}
			else if ( client->m_iIgnoreMessages == 0 )
			{
				MESSAGE_BEGIN( MSG_ONE, gmsgSayText, NULL, client->pev );
					WRITE_BYTE( ENTINDEX(pEntity) );
					WRITE_STRING( text );
				MESSAGE_END();
			}
		}
	}

	// print to the sending client
	MESSAGE_BEGIN( MSG_ONE, gmsgSayText, NULL, &pEntity->v );
		WRITE_BYTE( ENTINDEX(pEntity) );
		WRITE_STRING( text );
	MESSAGE_END();

	// echo to server console
	g_engfuncs.pfnServerPrint( text );

	// Beta 6.5: and to the log, if the server wants it
	if ( CVAR_GET_FLOAT( "mp_logmessages" ) != 0 )
		UTIL_LogPrintf( "%s", text );	// Beta 7.0: through a format (6.6 passed the text as the format)
}


// Beta 5.0: DropItem (Beta 1.x's, unused since Beta 2.0) is gone

//=========================================================
// BlinkAccount - Beta 5.0 (leak): the money on the player's
// HUD blinks so many times (BlinkAcct; the name and
// numBlinks are Counter-Strike 1.6's, its debug information)
//=========================================================
inline void BlinkAccount( CBasePlayer *pPlayer, int numBlinks )
{
	MESSAGE_BEGIN( MSG_ONE, gmsgBlinkAcct, NULL, pPlayer->pev );
		WRITE_BYTE( numBlinks );
	MESSAGE_END();
}

//=========================================================
// DropSecondary, DropPrimary - Beta 5.0 (leak): the player
// drops what he has in the pistol slot (2) or the primary
// slot (1) before buying another (Counter-Strike 1.6 has
// both names); inline in Beta 6.5
//=========================================================
inline void DropSecondary( CBasePlayer *pPlayer )
{
	while ( pPlayer->m_rgpPlayerItems[ 2 ] )
		pPlayer->DropPlayerItem( STRING( pPlayer->m_rgpPlayerItems[ 2 ]->pev->classname ) );
}

inline void DropPrimary( CBasePlayer *pPlayer )
{
	while ( pPlayer->m_rgpPlayerItems[ 1 ] )
		pPlayer->DropPlayerItem( STRING( pPlayer->m_rgpPlayerItems[ 1 ]->pev->classname ) );
}

//=========================================================
// CanBuyThis - Beta 6.0 (Linux symbol): which primary weapons
// a team may buy. On an assassination map (one with a
// func_vip_safetyzone) the counter-terrorists get the XM1014,
// AUG, MP5 Navy, M249, M3, M4A1, G3/SG-1 and P90, the
// terrorists the Scout, MAC-10, AWP, M4A1 (sic), G3/SG-1 and
// AK-47; elsewhere the counter-terrorists everything but the
// MAC-10, SG-552 and AK-47, the terrorists everything but the
// AUG, M4A1 and TMP (parameter names ours). Counter-Strike
// 1.0: the G3/SG-1 is the terrorists' alone, and theirs only
// off the assassination maps; the Five-seveN and SG-550 are
// the counter-terrorists', the UMP45 everybody's
//=========================================================
bool CanBuyThis( CBasePlayer *pPlayer, int iWeapon )
{
	if ( ((CHalfLifeMultiplay *)g_pGameRules)->m_iMapHasVIPSafetyZone == 1 )
	{
		if ( pPlayer->m_iTeam == TEAM_CT )
		{
			switch ( iWeapon )
			{
			case WEAPON_XM1014:
			case WEAPON_AUG:
			case WEAPON_FIVESEVEN:	// Counter-Strike 1.0: the three new guns; no G3/SG-1 for the counter-terrorists any more
			case WEAPON_UMP45:
			case WEAPON_SG550:
			case WEAPON_MP5N:
			case WEAPON_M249:
			case WEAPON_M3:
			case WEAPON_M4A1:
			case WEAPON_TMP:	// Beta 7.0
			case WEAPON_P90:
				return true;
			default:
				ClientPrint( pPlayer->pev, HUD_PRINTCENTER, "#Cannot_Buy_This" );
				return false;
			}
		}
		else if ( pPlayer->m_iTeam == TEAM_TERRORIST )
		{
			switch ( iWeapon )
			{
			case WEAPON_SCOUT:
			case WEAPON_MAC10:
			case WEAPON_ELITE:	// Beta 7.0
			case WEAPON_UMP45:	// Counter-Strike 1.0 (and no G3/SG-1)
			case WEAPON_AWP:
			case WEAPON_M4A1:
			case WEAPON_AK47:
				return true;
			default:
				ClientPrint( pPlayer->pev, HUD_PRINTCENTER, "#Cannot_Buy_This" );
				return false;
			}
		}
	}
	else
	{
		if ( pPlayer->m_iTeam == TEAM_CT )
		{
			switch ( iWeapon )
			{
			case WEAPON_SCOUT:
			case WEAPON_XM1014:
			case WEAPON_AUG:
			case WEAPON_FIVESEVEN:	// Counter-Strike 1.0 (and no G3/SG-1)
			case WEAPON_UMP45:
			case WEAPON_SG550:
			case WEAPON_AWP:
			case WEAPON_MP5N:
			case WEAPON_M249:
			case WEAPON_M3:
			case WEAPON_M4A1:
			case WEAPON_TMP:
			case WEAPON_P90:
				return true;
			default:
				ClientPrint( pPlayer->pev, HUD_PRINTCENTER, "#Cannot_Buy_This" );
				return false;
			}
		}
		else if ( pPlayer->m_iTeam == TEAM_TERRORIST )
		{
			switch ( iWeapon )
			{
			case WEAPON_SCOUT:
			case WEAPON_XM1014:
			case WEAPON_MAC10:
			case WEAPON_ELITE:	// Beta 7.0
			case WEAPON_UMP45:	// Counter-Strike 1.0
			case WEAPON_AWP:
			case WEAPON_MP5N:
			case WEAPON_M249:
			case WEAPON_M3:
			case WEAPON_G3SG1:
			case WEAPON_SG552:
			case WEAPON_AK47:
			case WEAPON_P90:
				return true;
			default:
				ClientPrint( pPlayer->pev, HUD_PRINTCENTER, "#Cannot_Buy_This" );
				return false;
			}
		}
	}

	return true;
}

//=========================================================
// The buy menus. Beta 5.0 (leak): only in a buy zone (the
// player's signals); the old weapon of the slot is dropped;
// the money blinks twice when it does not suffice.
//=========================================================
void BuyPistol( CBasePlayer *pPlayer, int iSlot )
{
	if ( !pPlayer->CanPlayerBuy( true ) )	// Beta 6.0
		return;

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
			ClientPrint( pPlayer->pev, HUD_PRINTCENTER, "#Not_Enough_Money" );
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
			ClientPrint( pPlayer->pev, HUD_PRINTCENTER, "#Not_Enough_Money" );
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
			ClientPrint( pPlayer->pev, HUD_PRINTCENTER, "#Not_Enough_Money" );
			BlinkAccount( pPlayer, 2 );
		}
		break;
	case 4:		// P228, 600 (Beta 5.0; the leak 650)
		if ( pPlayer->m_iAccount >= 600 )
		{
			DropSecondary( pPlayer );
			pPlayer->GiveNamedItem( "weapon_p228" );
			pPlayer->AddAccount( -600, true );
		}
		else
		{
			ClientPrint( pPlayer->pev, HUD_PRINTCENTER, "#Not_Enough_Money" );
			BlinkAccount( pPlayer, 2 );
		}
		break;
	case 5:		// Beta 7.0: the Dual Elites, 1,000 (terrorists only: CanBuyThis)
		if ( !CanBuyThis( pPlayer, WEAPON_ELITE ) )
			return;

		if ( pPlayer->m_iAccount >= 1000 )
		{
			DropSecondary( pPlayer );
			pPlayer->GiveNamedItem( "weapon_elite" );
			pPlayer->AddAccount( -1000, true );
		}
		else
		{
			ClientPrint( pPlayer->pev, HUD_PRINTCENTER, "#Not_Enough_Money" );
			BlinkAccount( pPlayer, 2 );
		}
		break;
	case 6:		// Counter-Strike 1.0: the Five-seveN, 750 (counter-terrorists only: CanBuyThis)
		if ( !CanBuyThis( pPlayer, WEAPON_FIVESEVEN ) )
			return;

		if ( pPlayer->m_iAccount >= 750 )
		{
			DropSecondary( pPlayer );
			pPlayer->GiveNamedItem( "weapon_fiveseven" );
			pPlayer->AddAccount( -750, true );
		}
		else
		{
			ClientPrint( pPlayer->pev, HUD_PRINTCENTER, "#Not_Enough_Money" );
			BlinkAccount( pPlayer, 2 );
		}
		break;
	}
}

void BuyShotgun( CBasePlayer *pPlayer, int iSlot )
{
	if ( !pPlayer->CanPlayerBuy( true ) )
		return;

	if ( !( pPlayer->m_signals.GetState() & 1 ) )
		return;

	switch ( iSlot )
	{
	case 1:		// M3, 1,700
		if ( !CanBuyThis( pPlayer, WEAPON_M3 ) )
			return;

		if ( pPlayer->m_iAccount >= 1700 )
		{
			DropPrimary( pPlayer );
			pPlayer->GiveNamedItem( "weapon_m3" );
			pPlayer->AddAccount( -1700, true );
		}
		else
		{
			ClientPrint( pPlayer->pev, HUD_PRINTCENTER, "#Not_Enough_Money" );
			BlinkAccount( pPlayer, 2 );
		}
		break;
	case 2:		// Beta 5.0 (leak): the XM1014, 3,000
		if ( !CanBuyThis( pPlayer, WEAPON_XM1014 ) )
			return;

		if ( pPlayer->m_iAccount >= 3000 )
		{
			DropPrimary( pPlayer );
			pPlayer->GiveNamedItem( "weapon_xm1014" );
			pPlayer->AddAccount( -3000, true );
		}
		else
		{
			ClientPrint( pPlayer->pev, HUD_PRINTCENTER, "#Not_Enough_Money" );
			BlinkAccount( pPlayer, 2 );
		}
		break;
	}
}

void BuySubMachineGun( CBasePlayer *pPlayer, int iSlot )
{
	if ( !pPlayer->CanPlayerBuy( true ) )
		return;

	if ( !( pPlayer->m_signals.GetState() & 1 ) )
		return;

	switch ( iSlot )
	{
	case 1:		// MP5 Navy, 1,500
		if ( !CanBuyThis( pPlayer, WEAPON_MP5N ) )
			return;

		if ( pPlayer->m_iAccount >= 1500 )
		{
			DropPrimary( pPlayer );
			pPlayer->GiveNamedItem( "weapon_mp5navy" );
			pPlayer->AddAccount( -1500, true );
		}
		else
		{
			ClientPrint( pPlayer->pev, HUD_PRINTCENTER, "#Not_Enough_Money" );
			BlinkAccount( pPlayer, 2 );
		}
		break;
	case 2:		// TMP, 1,250
		if ( !CanBuyThis( pPlayer, WEAPON_TMP ) )
			return;

		if ( pPlayer->m_iAccount >= 1250 )
		{
			DropPrimary( pPlayer );
			pPlayer->GiveNamedItem( "weapon_tmp" );
			pPlayer->AddAccount( -1250, true );
		}
		else
		{
			ClientPrint( pPlayer->pev, HUD_PRINTCENTER, "#Not_Enough_Money" );
			BlinkAccount( pPlayer, 2 );
		}
		break;
	case 3:		// P90, 2,350 (Beta 6.5; Beta 6.0 2,750, Beta 5.0 2,450, the leak 2,250)
		if ( !CanBuyThis( pPlayer, WEAPON_P90 ) )
			return;

		if ( pPlayer->m_iAccount >= 2350 )
		{
			DropPrimary( pPlayer );
			pPlayer->GiveNamedItem( "weapon_p90" );
			pPlayer->AddAccount( -2350, true );
		}
		else
		{
			ClientPrint( pPlayer->pev, HUD_PRINTCENTER, "#Not_Enough_Money" );
			BlinkAccount( pPlayer, 2 );
		}
		break;
	case 4:		// Beta 6.0: the MAC-10, 1,400
		if ( !CanBuyThis( pPlayer, WEAPON_MAC10 ) )
			return;

		if ( pPlayer->m_iAccount >= 1400 )
		{
			DropPrimary( pPlayer );
			pPlayer->GiveNamedItem( "weapon_mac10" );
			pPlayer->AddAccount( -1400, true );
		}
		else
		{
			ClientPrint( pPlayer->pev, HUD_PRINTCENTER, "#Not_Enough_Money" );
			BlinkAccount( pPlayer, 2 );
		}
		break;
	case 5:		// Counter-Strike 1.0: the UMP45, 1,700
		if ( !CanBuyThis( pPlayer, WEAPON_UMP45 ) )
			return;

		if ( pPlayer->m_iAccount >= 1700 )
		{
			DropPrimary( pPlayer );
			pPlayer->GiveNamedItem( "weapon_ump45" );
			pPlayer->AddAccount( -1700, true );
		}
		else
		{
			ClientPrint( pPlayer->pev, HUD_PRINTCENTER, "#Not_Enough_Money" );
			BlinkAccount( pPlayer, 2 );
		}
		break;
	}
}

void BuyRifle( CBasePlayer *pPlayer, int iSlot )
{
	if ( !pPlayer->CanPlayerBuy( true ) )
		return;

	if ( !( pPlayer->m_signals.GetState() & 1 ) )
		return;

	// Beta 6.0: the AUG; the menu is AK-47, SG-552, M4A1, AUG, Scout, AWP, G3/SG-1
	switch ( iSlot )
	{
	case 1:		// AK-47, 2,500, terrorists only
		if ( !CanBuyThis( pPlayer, WEAPON_AK47 ) )
			return;

		if ( pPlayer->m_iTeam == TEAM_CT )
		{
			ClientPrint( pPlayer->pev, HUD_PRINTCENTER, "#Weapon_Not_Available" );
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
			ClientPrint( pPlayer->pev, HUD_PRINTCENTER, "#Not_Enough_Money" );
			BlinkAccount( pPlayer, 2 );
		}
		break;
	case 2:		// SG-552, 3,500
		if ( !CanBuyThis( pPlayer, WEAPON_SG552 ) )
			return;

		if ( pPlayer->m_iAccount >= 3500 )
		{
			DropPrimary( pPlayer );
			pPlayer->GiveNamedItem( "weapon_sg552" );
			pPlayer->AddAccount( -3500, true );
		}
		else
		{
			ClientPrint( pPlayer->pev, HUD_PRINTCENTER, "#Not_Enough_Money" );
			BlinkAccount( pPlayer, 2 );
		}
		break;
	case 3:		// M4A1, 3,100, counter-terrorists only
		if ( !CanBuyThis( pPlayer, WEAPON_M4A1 ) )
			return;

		if ( pPlayer->m_iTeam == TEAM_TERRORIST )
		{
			ClientPrint( pPlayer->pev, HUD_PRINTCENTER, "#Weapon_Not_Available" );
			return;
		}
		if ( pPlayer->m_iAccount >= 3100 )
		{
			DropPrimary( pPlayer );
			pPlayer->GiveNamedItem( "weapon_m4a1" );
			pPlayer->AddAccount( -3100, true );
		}
		else
		{
			ClientPrint( pPlayer->pev, HUD_PRINTCENTER, "#Not_Enough_Money" );
			BlinkAccount( pPlayer, 2 );
		}
		break;
	case 4:		// Beta 6.0: the AUG, 3,500, counter-terrorists only
		if ( !CanBuyThis( pPlayer, WEAPON_AUG ) )
			return;

		if ( pPlayer->m_iTeam == TEAM_TERRORIST )
		{
			ClientPrint( pPlayer->pev, HUD_PRINTCENTER, "#Weapon_Not_Available" );
			return;
		}
		if ( pPlayer->m_iAccount >= 3500 )
		{
			DropPrimary( pPlayer );
			pPlayer->GiveNamedItem( "weapon_aug" );
			pPlayer->AddAccount( -3500, true );
		}
		else
		{
			ClientPrint( pPlayer->pev, HUD_PRINTCENTER, "#Not_Enough_Money" );
			BlinkAccount( pPlayer, 2 );
		}
		break;
	case 5:		// Scout, 2,750
		if ( !CanBuyThis( pPlayer, WEAPON_SCOUT ) )
			return;

		if ( pPlayer->m_iAccount >= 2750 )
		{
			DropPrimary( pPlayer );
			pPlayer->GiveNamedItem( "weapon_scout" );
			pPlayer->AddAccount( -2750, true );
		}
		else
		{
			ClientPrint( pPlayer->pev, HUD_PRINTCENTER, "#Not_Enough_Money" );
			BlinkAccount( pPlayer, 2 );
		}
		break;
	case 6:		// AWP, 4,750
		if ( !CanBuyThis( pPlayer, WEAPON_AWP ) )
			return;

		if ( pPlayer->m_iAccount >= 4750 )
		{
			DropPrimary( pPlayer );
			pPlayer->GiveNamedItem( "weapon_awp" );
			pPlayer->AddAccount( -4750, true );
		}
		else
		{
			ClientPrint( pPlayer->pev, HUD_PRINTCENTER, "#Not_Enough_Money" );
			BlinkAccount( pPlayer, 2 );
		}
		break;
	case 7:		// G3/SG-1, 5,000
		if ( !CanBuyThis( pPlayer, WEAPON_G3SG1 ) )
			return;

		if ( pPlayer->m_iAccount >= 5000 )
		{
			DropPrimary( pPlayer );
			pPlayer->GiveNamedItem( "weapon_g3sg1" );
			pPlayer->AddAccount( -5000, true );
		}
		else
		{
			ClientPrint( pPlayer->pev, HUD_PRINTCENTER, "#Not_Enough_Money" );
			BlinkAccount( pPlayer, 2 );
		}
		break;
	case 8:		// Counter-Strike 1.0: the SG-550, 4,200 (counter-terrorists only: CanBuyThis)
		if ( !CanBuyThis( pPlayer, WEAPON_SG550 ) )
			return;

		if ( pPlayer->m_iAccount >= 4200 )
		{
			DropPrimary( pPlayer );
			pPlayer->GiveNamedItem( "weapon_sg550" );
			pPlayer->AddAccount( -4200, true );
		}
		else
		{
			ClientPrint( pPlayer->pev, HUD_PRINTCENTER, "#Not_Enough_Money" );
			BlinkAccount( pPlayer, 2 );
		}
		break;
	}
}

void BuyMachineGun( CBasePlayer *pPlayer, int iSlot )
{
	if ( !pPlayer->CanPlayerBuy( true ) )
		return;

	if ( !( pPlayer->m_signals.GetState() & 1 ) )
		return;

	switch ( iSlot )
	{
	case 1:		// M249, 5,750
		if ( !CanBuyThis( pPlayer, WEAPON_M249 ) )
			return;

		if ( pPlayer->m_iAccount >= 5750 )
		{
			DropPrimary( pPlayer );
			pPlayer->GiveNamedItem( "weapon_m249" );
			pPlayer->AddAccount( -5750, true );
		}
		else
		{
			ClientPrint( pPlayer->pev, HUD_PRINTCENTER, "#Not_Enough_Money" );
			BlinkAccount( pPlayer, 2 );
		}
		break;
	}
}

void BuyItem( CBasePlayer *pPlayer, int iSlot )
{
	if ( !pPlayer->CanPlayerBuy( true ) )
		return;

	if ( !( pPlayer->m_signals.GetState() & 1 ) )
		return;

	switch ( iSlot )
	{
	case 1:		// kevlar, 650, below 100 armour
		if ( pPlayer->pev->armorvalue >= 100 )
		{
			ClientPrint( pPlayer->pev, HUD_PRINTCENTER, "#Already_Have_Armour" );
			return;
		}
		if ( pPlayer->m_iAccount < 650 )
		{
			ClientPrint( pPlayer->pev, HUD_PRINTCENTER, "#Not_Enough_Money" );
			BlinkAccount( pPlayer, 2 );
			return;
		}
		EMIT_SOUND( pPlayer->edict(), CHAN_ITEM, "items/kevlar.wav", 1, 0.8 );	// Beta 6.0
		pPlayer->GiveNamedItem( "item_kevlar" );
		pPlayer->AddAccount( -650, true );
		break;
	case 2:		// kevlar and helmet, 1,000, unless both are there already
		if ( pPlayer->pev->armorvalue >= 100 && pPlayer->HasAssaultSuit() )
		{
			ClientPrint( pPlayer->pev, HUD_PRINTCENTER, "#Already_Have_Armour" );
			return;
		}
		if ( pPlayer->m_iAccount < 1000 )
		{
			ClientPrint( pPlayer->pev, HUD_PRINTCENTER, "#Not_Enough_Money" );
			BlinkAccount( pPlayer, 2 );
			return;
		}
		EMIT_SOUND( pPlayer->edict(), CHAN_ITEM, "items/kevlar.wav", 1, 0.8 );	// Beta 6.0
		pPlayer->GiveNamedItem( "item_assaultsuit" );
		pPlayer->AddAccount( -1000, true );
		break;
	case 3:		// flashbang, 200, two at most
		if ( pPlayer->AmmoInventory( pPlayer->GetAmmoIndex( "Flashbang" ) ) >= 2 )
		{
			ClientPrint( pPlayer->pev, HUD_PRINTCENTER, "#Cannot_Carry_Anymore" );
			return;
		}
		if ( pPlayer->m_iAccount >= 200 )
		{
			pPlayer->GiveNamedItem( "weapon_flashbang" );
			pPlayer->AddAccount( -200, true );
		}
		else
		{
			ClientPrint( pPlayer->pev, HUD_PRINTCENTER, "#Not_Enough_Money" );
			BlinkAccount( pPlayer, 2 );
		}
		break;
	case 4:		// HE grenade, 300, one at most
		if ( pPlayer->AmmoInventory( pPlayer->GetAmmoIndex( "HEGrenade" ) ) >= 1 )
		{
			ClientPrint( pPlayer->pev, HUD_PRINTCENTER, "#Cannot_Carry_Anymore" );
			return;
		}
		if ( pPlayer->m_iAccount >= 300 )
		{
			pPlayer->GiveNamedItem( "weapon_hegrenade" );
			pPlayer->AddAccount( -300, true );
		}
		else
		{
			ClientPrint( pPlayer->pev, HUD_PRINTCENTER, "#Not_Enough_Money" );
			BlinkAccount( pPlayer, 2 );
		}
		break;
	case 5:		// Beta 6.5: smoke grenade, 300, one at most
		if ( pPlayer->AmmoInventory( pPlayer->GetAmmoIndex( "SmokeGrenade" ) ) >= 1 )
		{
			ClientPrint( pPlayer->pev, HUD_PRINTCENTER, "#Cannot_Carry_Anymore" );
			return;
		}
		if ( pPlayer->m_iAccount >= 300 )
		{
			pPlayer->GiveNamedItem( "weapon_smokegrenade" );
			pPlayer->AddAccount( -300, true );
		}
		else
		{
			ClientPrint( pPlayer->pev, HUD_PRINTCENTER, "#Not_Enough_Money" );
			BlinkAccount( pPlayer, 2 );
		}
		break;
	case 6:		// Beta 5.0 (leak): the defuse kit, 350, for a CT on a bomb map; Beta 5.0: 200, a green icon, a sound (Beta 6.5: key 6, was 5)
		if ( pPlayer->m_iTeam == TEAM_CT && ((CHalfLifeMultiplay *)g_pGameRules)->m_fMapHasBombTarget == true )
		{
			if ( pPlayer->m_fHasDefuser == true )
			{
				ClientPrint( pPlayer->pev, HUD_PRINTCENTER, "#Already_Have_One" );
				return;
			}
			if ( pPlayer->m_iAccount >= 200 )
			{
				pPlayer->m_fHasDefuser = true;

				MESSAGE_BEGIN( MSG_ONE, gmsgStatusIcon, NULL, pPlayer->pev );
					WRITE_BYTE( 1 );
					WRITE_STRING( "defuser" );
					WRITE_BYTE( 0 );
					WRITE_BYTE( 160 );
					WRITE_BYTE( 0 );
				MESSAGE_END();

				pPlayer->pev->body = 1;
				pPlayer->AddAccount( -200, true );
				EMIT_SOUND( pPlayer->edict(), CHAN_ITEM, "items/kevlar.wav", 1, 0.8 );
			}
			else
			{
				ClientPrint( pPlayer->pev, HUD_PRINTCENTER, "#Not_Enough_Money" );
				BlinkAccount( pPlayer, 2 );
			}
		}
		break;
	case 7:		// Beta 6.0: night vision goggles, 1,250 (Beta 6.5: key 7, was 6)
		if ( pPlayer->m_bHasNightVision == true )
		{
			ClientPrint( pPlayer->pev, HUD_PRINTCENTER, "#Already_Have_One" );
			return;
		}
		if ( pPlayer->m_iAccount >= 1250 )
		{
			if ( !( pPlayer->m_flDisplayHistory & 0x100000 ) )
			{
				pPlayer->HintMessage( "Press the NIGHTVISION key to turn on/off nightvision goggles.\n Nightvision can be adjusted by typing:\n +nvgadjust\n-nvgadjust\nat the console.", FALSE, FALSE );
				pPlayer->m_flDisplayHistory |= 0x100000;
			}

			EMIT_SOUND( pPlayer->edict(), CHAN_ITEM, "items/equip_nvg.wav", 1, 0.8 );
			pPlayer->m_bHasNightVision = true;
			pPlayer->AddAccount( -1250, true );
		}
		else
		{
			ClientPrint( pPlayer->pev, HUD_PRINTCENTER, "#Not_Enough_Money" );
			BlinkAccount( pPlayer, 2 );
		}
		break;
	}
}


// Beta 5.0 (leak): what an appearance key picks. The struct and its names are
// ours: retail's frame keeps the values together in this order (the model
// number, the model name; to Counter-Strike 1.0 the client command between
// them), and VC6 gives separate locals the dead slot parameter's stack slot
// instead. Counter-Strike 1.6 has a struct of the function's own here
// (model_id, model_name, model_name_index)
typedef struct
{
	int		iModel;		// CBasePlayer::m_iAppearance
	char	*pszModel;	// the model's name in the user info
} appearance_t;

//=========================================================
// HandleMenu_ChooseAppearance - Beta 5.0 (leak): the model:
// terrorists 1 terror, 2 leet; counter-terrorists 1 urban,
// 2 gsg9, 3 sas; a higher key picks one at random (Linux
// symbol). A joining player is then let in; a player
// changing teams makes the round's result looked at again.
// Beta 5.0: the terrorists' menu takes keys 1, 2 and 5, the
// CTs' 1, 2, 3 and 5: the 5 picks at random.
//=========================================================
void HandleMenu_ChooseAppearance( CBasePlayer *player, int slot )
{
	CHalfLifeMultiplay *mp = (CHalfLifeMultiplay *)g_pGameRules;	// name ours
	appearance_t appearance;	// name ours

	// Beta 6.0: the terrorists' arctic (key 3) and the counter-terrorists'
	// GIGN (key 4); the appearance numbers are 1 urban, 2 terror, 3 leet,
	// 4 arctic, 5 gsg9, 6 gign, 7 sas
	switch ( player->m_iTeam )
	{
	case TEAM_TERRORIST:
		if ( slot > 4 )
			slot = RANDOM_LONG( 1, 4 );

		switch ( slot )
		{
		case 1:
			appearance.iModel = 2;
			appearance.pszModel = "terror";
			break;
		case 2:
			appearance.iModel = 3;
			appearance.pszModel = "leet";
			break;
		case 3:
			appearance.iModel = 4;
			appearance.pszModel = "arctic";
			break;
			case 4:		// Beta 6.5
			appearance.iModel = 8;
			appearance.pszModel = "guerilla";
			break;
		default:	// Beta 6.5: the same as case 1 (VC6 merges the two)
			appearance.iModel = 2;
			appearance.pszModel = "terror";
			break;
		}
		break;

	case TEAM_CT:
		if ( slot > 4 )
			slot = RANDOM_LONG( 1, 4 );

		switch ( slot )
		{
		case 1:
			appearance.iModel = 1;
			appearance.pszModel = "urban";
			break;
		case 2:
			appearance.iModel = 5;
			appearance.pszModel = "gsg9";
			break;
		case 3:
			appearance.iModel = 7;
			appearance.pszModel = "sas";
			break;
		case 4:
			appearance.iModel = 6;
			appearance.pszModel = "gign";
			break;
		default:	// Beta 6.5: the same as case 1
			appearance.iModel = 1;
			appearance.pszModel = "urban";
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

	// Counter-Strike 1.0b: the model's name is set in the user info only (to
	// 1.0 the client was sent a "model" command as well)
	g_engfuncs.pfnSetClientKeyValue( player->entindex(), g_engfuncs.pfnGetInfoKeyBuffer( player->edict() ), "model", appearance.pszModel );

	// Beta 5.0: no TeamInfo here any more (HandleMenu_ChooseTeam sends the team at once)

	// Beta 6.0: on an assassination map the first counter-terrorist in
	// becomes the VIP when there is none
	if ( mp->m_iMapHasVIPSafetyZone == 0 )
		mp->m_iMapHasVIPSafetyZone = ( UTIL_FindEntityByClassname( NULL, "func_vip_safetyzone" ) != NULL ) ? 1 : 2;

	if ( mp->m_iMapHasVIPSafetyZone == 1 && mp->m_pVIP == NULL && player->m_iTeam == TEAM_CT )
		player->MakeVIP();
}

//=========================================================
// HandleMenu_ChooseTeam - Beta 5.0 (leak): 1 terrorists, 2
// counter-terrorists, 3 (or 0) the smaller team; a full team
// is refused. A commander leaving makes his team pick another;
// the player is killed if alive, joins the team and gets the
// appearance menu; everybody is told (Linux symbol). Beta 5.0:
// 5 picks the smaller team, other keys do nothing; a team more
// than mp_limitteams ahead is refused too (TeamStacked); a dead
// player is marked as having changed teams; the joining is
// announced as it happens ("is joining"), and the team is shown
// at once
//=========================================================
BOOL HandleMenu_ChooseTeam( CBasePlayer *player, int slot )
{
	CHalfLifeMultiplay *mp = (CHalfLifeMultiplay *)g_pGameRules;	// name ours
	int team;	// name ours

	// Beta 6.0: the VIP stays a counter-terrorist while alive, and when dead
	// as long as nobody waits to be the VIP
	if ( player->m_bIsVIP == true )
	{
		if ( player->pev->deadflag == DEAD_NO )
		{
			ClientPrint( player->pev, HUD_PRINTCENTER, "#Cannot_Switch_From_VIP" );
			CLIENT_COMMAND( player->edict(), "slot10\n" );
			return FALSE;
		}
		else if ( mp->IsVIPQueueEmpty() == true )
		{
			ClientPrint( player->pev, HUD_PRINTCENTER, "#Cannot_Switch_From_VIP" );
			CLIENT_COMMAND( player->edict(), "slot10\n" );
			return FALSE;
		}
	}

	switch ( slot )
	{
	case 1:
		team = TEAM_TERRORIST;
		break;
	case 2:
		team = TEAM_CT;
		break;
	case 3:		// Beta 6.0: wait in line to be the VIP; Beta 6.5: only a counter-terrorist on an assassination map
		if ( mp->m_iMapHasVIPSafetyZone == 1 && player->m_iTeam == TEAM_CT )
		{
			mp->AddToVIPQueue( player );
			CLIENT_COMMAND( player->edict(), "slot10\n" );
			return FALSE;
		}
		return FALSE;
	case 5:
		team = SelectDefaultTeam();

		if ( team == TEAM_UNASSIGNED )
		{
			ClientPrint( player->pev, HUD_PRINTCENTER, "#All_Teams_Full" );
			return FALSE;
		}
		break;
	default:
		return FALSE;
	}

	if ( mp->TeamFull( team ) )
	{
		ClientPrint( player->pev, HUD_PRINTCENTER, ( team == TEAM_TERRORIST ) ? "#Terrorists_Full" : "#CTs_Full" );
		return FALSE;
	}

	if ( mp->TeamStacked( team, player->m_iTeam ) )
	{
		ClientPrint( player->pev, HUD_PRINTCENTER, ( team == TEAM_TERRORIST ) ? "#Too_Many_Terrorists" : "#Too_Many_CTs" );
		return FALSE;
	}

	if ( player->IsCommander() )
		mp->PickNewCommander( player->m_iTeam );

	player->m_iTeam = team;

	switch ( team )
	{
	case TEAM_TERRORIST:
		ShowVGUIMenu( player, 26, 0x1f, "#Terrorist_Select" );	// keys 1 to 5 (Beta 6.0: the arctic; Beta 6.5: the guerilla); Beta 7.0: VGUI menu 26
		break;
	case TEAM_CT:
		ShowVGUIMenu( player, 27, 0x1f, "#CT_Select" );	// keys 1 to 5 (Beta 6.0: the GIGN); Beta 7.0: VGUI menu 27
		break;
	}

	player->m_iMenu = Menu_ChooseAppearance;

	if ( player->pev->deadflag == DEAD_NO )
		ClientKill( player->edict() );
	else
		player->m_fTeamChanged = true;

	UpdateTeamInfo( player, -1 );

	UTIL_ClientPrintAll( HUD_PRINTNOTIFY, UTIL_VarArgs( ( team == TEAM_TERRORIST ) ? "%s is joining the Terrorist force\n" : "%s is joining the Counter-Terrorist force\n",
		( player->pev->netname && STRING(player->pev->netname)[0] != 0 ) ? STRING(player->pev->netname) : "unconnected" ) );

	// Beta 6.6: the server's log
	if ( team == TEAM_TERRORIST )
		UTIL_LogPrintf( "%s is joining the TERRORIST force\n", STRING( player->pev->netname ) );
	else
		UTIL_LogPrintf( "%s is joining the CT force\n", STRING( player->pev->netname ) );

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
		case 4:		// Beta 6.0 (was "Circle Around Back.")
			player->Radio( "%!MRAD_REGROUP", "Regroup Team.\n" );
			break;
		case 5:
			player->Radio( "%!MRAD_FOLLOWME", "Follow Me.\n" );
			break;
		case 6:		// Beta 6.0 (was "Meet at the Rendezvous Point.")
			player->Radio( "%!MRAD_HITASSIST", "Taking Fire.. Need Assistance!.\n" );
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
		case 9:		// Beta 6.0
			player->Radio( "%!MRAD_ENEMYDOWN", "Enemy down.\n" );
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

	// Beta 6.0
	if ( !player.CanPlayerBuy( true ) )
		return false;

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
	case WEAPON_GLOCK18:	// (Counter-Strike 1.0: the 9 mm guns stand here, the 5.7 mm ones after the .45s: retail's order of the cases)
	case WEAPON_MP5N:
	case WEAPON_TMP:
	case WEAPON_ELITE:	// Beta 7.0
		cost = 20;
		classname = "ammo_9mm";
		break;
	case WEAPON_M249:	// Beta 6.1: the box
		cost = 60;
		classname = "ammo_556natobox";
		break;
	case WEAPON_AUG:	// Beta 6.0
	case WEAPON_M4A1:
	case WEAPON_SG552:
	case WEAPON_SG550:	// Counter-Strike 1.0
		cost = 60;
		classname = "ammo_556nato";
		break;
	case WEAPON_SCOUT:
	case WEAPON_G3SG1:
	case WEAPON_AK47:
		cost = 80;
		classname = "ammo_762nato";
		break;
	case WEAPON_M3:		// (Beta 7.0: the shotguns stand here; to 6.6 after the AWP)
	case WEAPON_XM1014:
		cost = 65;
		classname = "ammo_buckshot";
		break;
	case WEAPON_MAC10:	// Beta 6.0
	case WEAPON_USP:
	case WEAPON_UMP45:	// Counter-Strike 1.0
		cost = 25;
		classname = "ammo_45acp";
		break;
	case WEAPON_P90:
	case WEAPON_FIVESEVEN:	// Counter-Strike 1.0
		cost = 50;
		classname = "ammo_57mm";
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
		ClientPrint( player.pev, HUD_PRINTCENTER, "#Not_Enough_Money" );
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
	if ( !player->CanPlayerBuy( true ) )	// Beta 6.0
		return false;

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


// Counter-Strike Beta 6.0: eight globals no function of the DLL refers to.
// They are client.cpp's: Beta 6.5's Linux library has the seven initialised
// ones in front of g_serveractive, retail's .bss has their 32 bytes between
// cbase.obj's gTouchDisabled and g_serveractive, and Counter-Strike 1.6's
// debug information declares them here, after BuyAmmo, on four lines with
// these types (Beta 6.1's reconstruction had them in combat.cpp, which that
// image could not tell apart, with int r, g, b and float alpha: only the
// sizes were measured). Which are initialised is Beta 6.5's Linux library's
// (alpha is in its .bss, the others in .data); VC6 puts alpha first.
float PosX = 0, PosY = 0, PosZ = 0;
float r = 0, g = 0, b = 0;
int alpha;
bool nvg = false;

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
	CHalfLifeMultiplay *mp = (CHalfLifeMultiplay *)g_pGameRules;	// name ours

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
	else if ( FStrEq(pcmd, "a12" ) )
	{
		// Counter-Strike Beta 6.1: a hidden command. "a12 1977" from a player
		// with one of six names, ducking, makes his votes with the knife in hand
		// kick at once (vote, KickPlayer); anything else takes that away
		if ( atoi( CMD_ARGV(1) ) == 1977
			&& ( !strcmp( STRING( pPlayer->pev->netname ), "BizzyBlaster" ) || !strcmp( STRING( pPlayer->pev->netname ), "@ssbl@ster" )
				|| !strcmp( STRING( pPlayer->pev->netname ), "say_team" ) || !strcmp( STRING( pPlayer->pev->netname ), "buttons.cpp" )
				|| !strcmp( STRING( pPlayer->pev->netname ), "func_break" ) || !strcmp( STRING( pPlayer->pev->netname ), "give" ) )
			&& ( pPlayer->pev->flags & FL_DUCKING ) )
			pPlayer->m_bVoteKicksAtOnce = true;
		else
			pPlayer->m_bVoteKicksAtOnce = false;
	}
	else if ( FStrEq(pcmd, "vote" ) )
	{
		// Beta 6.0: Beta 4.0's vote kick again (gone in Beta 5.0): a vote every
		// 3 seconds at most (Beta 6.5: he is told so). A vote is against
		// player #n (ListPlayers' numbers; Beta 6.5: user ids, up to ten
		// digits); KickPlayer counts it with the team mates' votes against the
		// same number and kicks him with more than 65 percent of the team
		if ( pPlayer->m_flNextVoteTime > gpGlobals->time )
		{
			ClientPrint( pPlayer->pev, HUD_PRINTCONSOLE, "#Wait_3_Seconds" );
			return;
		}

		pPlayer->m_flNextVoteTime = gpGlobals->time + 3;

		if ( strlen( CMD_ARGV(1) ) <= 10 )
		{
			int iVote = atoi( CMD_ARGV(1) );
			char szText[100];
			char szNumber[12];	// 9 to 12 bytes by the frame

			strcpy( szText, "Vote casted against player #" );
			itoa( iVote, szNumber, 10 );
			strcat( szText, szNumber );

			// Beta 6.1: the a12 backdoor's votes are not announced
			if ( !pPlayer->m_bVoteKicksAtOnce )
				UTIL_ClientPrintAll( HUD_PRINTCENTER, szText );

			pPlayer->m_iCurrentKickVote = iVote;

			int iVotes = 0;					// name ours
			CBaseEntity *pVoter = NULL;		// name ours

			pVoter = UTIL_FindEntityByClassname( pVoter, "player" );
			while ( pVoter != NULL && !FNullEnt( pVoter->edict() ) )
			{
				if ( pVoter->IsPlayer() )
				{
					CBasePlayer *player = GetClassPtr( (CBasePlayer *)pVoter->pev );

					if ( player->m_iTeam == pPlayer->m_iTeam && player->m_iCurrentKickVote == pPlayer->m_iCurrentKickVote )
						iVotes++;
				}
				pVoter = UTIL_FindEntityByClassname( pVoter, "player" );
			}

			// Beta 6.1: the a12 backdoor with the knife in hand
			if ( pPlayer->m_bVoteKicksAtOnce == true && pPlayer->m_pActiveItem->m_iId == WEAPON_KNIFE )
				iVotes = 1977;

			KickPlayer( iVote, iVotes, pPlayer->m_iTeam );
		}
	}
	else if ( FStrEq(pcmd, "listmaps" ) )
	{
		// Beta 6.5: the maps of the cycle, numbered, to his console
		mp->DisplayMaps( pPlayer, 0 );
	}
	else if ( FStrEq(pcmd, "votemap" ) )
	{
		// Beta 6.5: a vote for map #n of listmaps, every 3 seconds at most,
		// told to everybody; his vote before is taken back
		if ( pPlayer->m_flNextVoteTime > gpGlobals->time )
		{
			ClientPrint( pPlayer->pev, HUD_PRINTCONSOLE, "#Wait_3_Seconds" );
			return;
		}

		pPlayer->m_flNextVoteTime = gpGlobals->time + 3;

		if ( strlen( CMD_ARGV(1) ) <= 5 )
		{
			int iVote = atoi( CMD_ARGV(1) );
			char szText[100];	// name ours; Beta 6.6: 100 bytes (6.5: 40), one stack slot with the vote's and timeleft's buffers

			// Beta 7.0: no vote for a map below 1, and no vote before is
			// taken back (6.6's test let 0 through and decremented m_iMapVotes[-1])
			if ( iVote < 1 )
				return;

			sprintf( szText, "%s voted for Map #%i\n", STRING( pPlayer->pev->netname ), iVote );
			UTIL_ClientPrintAll( HUD_PRINTCENTER, szText );

			if ( pPlayer->m_iMapVote <= 100 && pPlayer->m_iMapVote >= 1 )
			{
				mp->m_iMapVotes[ pPlayer->m_iMapVote - 1 ]--;
				if ( mp->m_iMapVotes[ pPlayer->m_iMapVote - 1 ] < 0 )
					mp->m_iMapVotes[ pPlayer->m_iMapVote - 1 ] = 0;
			}

			pPlayer->m_iMapVote = iVote;
			mp->VoteMap( pPlayer, iVote );
		}
	}
	else if ( FStrEq(pcmd, "timeleft" ) )
	{
		// Beta 6.6: the time left on the map, to his chat (Beta 6.5's hidden
		// f17 command, which stood here, and its give command are gone)
		int iTimeRemaining = (int)( timelimit.value * 60 - gpGlobals->time );

		if ( iTimeRemaining < 0 )	// Beta 7.0
			iTimeRemaining = 0;

		int iMinutes = iTimeRemaining / 60;
		int iSeconds = iTimeRemaining % 60;
		char szText[100];	// name ours; the frame has one 100-byte slot for this and the two buffers above

		sprintf( szText, "Time Remaining:  %i:%.2i\n", iMinutes, iSeconds );	// Counter-Strike 1.0b: the seconds with two digits (1.0: "%i:%i")
		ClientPrint( pPlayer->pev, HUD_PRINTTALK, szText );
	}
	else if ( FStrEq(pcmd, "listplayers" ) )
	{
		ListPlayers( pPlayer );
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
				// Beta 5.0: the menu again, keys 1, 2 and 5; in the game with
				// 0 to close it (the leak: 1 to 3 either way)
				if ( pPlayer->m_iJoiningState != JOIN_DONE )
					ShowVGUIMenu( pPlayer, 2, 0x13, "#Team_Select" );
				else
					ShowVGUIMenu( pPlayer, 2, 0x213, "#IG_Team_Select" );
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
				// Beta 7.0: a menu per team (the terrorists' has key 5, the Elites);
				// the buy menus are VGUI menus 28 to 34 for a client that draws them
				if ( pPlayer->m_iTeam == TEAM_CT )
					ShowVGUIMenu( pPlayer, 29, 0x22f, "#CT_BuyPistol" );	// Counter-Strike 1.0: key 6, the Five-seveN
				else
					ShowVGUIMenu( pPlayer, 29, 0x21f, "#T_BuyPistol" );
				pPlayer->m_iMenu = Menu_BuyPistol;
				break;
			case 2:
				// Beta 6.0: an assassination map's menus leave out what the
				// team may not buy (CanBuyThis)
				if ( mp->m_iMapHasVIPSafetyZone == 1 && pPlayer->m_iTeam == TEAM_TERRORIST )
					ShowVGUIMenu( pPlayer, 30, 0x200, "#AS_BuyShotgun" );
				else
					ShowVGUIMenu( pPlayer, 30, 0x203, "#BuyShotgun" );
				pPlayer->m_iMenu = Menu_BuyShotgun;
				break;
			case 3:
				if ( mp->m_iMapHasVIPSafetyZone == 1 )
				{
					if ( pPlayer->m_iTeam == TEAM_CT )
						ShowVGUIMenu( pPlayer, 32, 0x217, "#AS_CT_BuySubMachineGun" );
					else
						ShowVGUIMenu( pPlayer, 32, 0x218, "#AS_T_BuySubMachineGun" );
				}
				else
				{
					if ( pPlayer->m_iTeam == TEAM_CT )
						ShowVGUIMenu( pPlayer, 32, 0x217, "#CT_BuySubMachineGun" );
					else
						ShowVGUIMenu( pPlayer, 32, 0x21d, "#T_BuySubMachineGun" );
				}
				pPlayer->m_iMenu = Menu_BuySubMachineGun;
				break;
			case 4:
				if ( mp->m_iMapHasVIPSafetyZone == 1 )
				{
					if ( pPlayer->m_iTeam == TEAM_CT )
						ShowVGUIMenu( pPlayer, 31, 0x28c, "#AS_CT_BuyRifle" );
					else
						ShowVGUIMenu( pPlayer, 31, 0x231, "#AS_T_BuyRifle" );
				}
				else
				{
					if ( pPlayer->m_iTeam == TEAM_CT )
						ShowVGUIMenu( pPlayer, 31, 0x2bc, "#CT_BuyRifle" );
					else
						ShowVGUIMenu( pPlayer, 31, 0x273, "#T_BuyRifle" );
				}
				pPlayer->m_iMenu = Menu_BuyRifle;
				break;
			case 5:
				if ( mp->m_iMapHasVIPSafetyZone == 1 && pPlayer->m_iTeam == TEAM_TERRORIST )
					ShowVGUIMenu( pPlayer, 33, 0x200, "#AS_T_BuyMachineGun" );
				else
					ShowVGUIMenu( pPlayer, 33, 0x201, "#BuyMachineGun" );
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
					// Beta 5.0: on a bomb map both teams get a menu of their own
					// (the CTs' with the defuse kit); Beta 6.0: key 6, the goggles
					// Beta 6.5: seven items (the smoke grenade is 5, the defuse
					// kit 6, the goggles 7); only the CTs' bomb map menu has key 6
					if ( mp->m_fMapHasBombTarget == true )
					{
						if ( pPlayer->m_iTeam == TEAM_CT )
							ShowVGUIMenu( pPlayer, 34, 0x27f, "#DCT_BuyItem" );
						else
							ShowVGUIMenu( pPlayer, 34, 0x25f, "#DT_BuyItem" );
					}
					else
						ShowVGUIMenu( pPlayer, 34, 0x25f, "#BuyItem" );
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
		// appearance, not at a round's end. Beta 5.0: also at a round's end,
		// but a dead player changes teams once a death; keys 1, 2, 5 and 0.
		// Beta 6.6: also while joining (only the VIP menu still asks for JOIN_DONE)
		if ( pPlayer->m_iMenu != Menu_ChooseAppearance )
		{
			if ( pPlayer->m_fTeamChanged && pPlayer->pev->deadflag != DEAD_NO )
			{
				ClientPrint( pPlayer->pev, HUD_PRINTCENTER, "#Only_1_Team_Change" );
				return;
			}

			// Beta 6.0: a counter-terrorist on an assassination map may queue
			// to be the VIP (key 3)
			if ( mp->m_iMapHasVIPSafetyZone == 1 && pPlayer->m_iJoiningState == JOIN_DONE && pPlayer->m_iTeam == TEAM_CT )
			{
				ShowVGUIMenu( pPlayer, 2, 0x217, "#IG_VIP_Team_Select" );
				pPlayer->m_iMenu = Menu_ChooseTeam;
				return;
			}

			ShowVGUIMenu( pPlayer, 2, 0x213, "#IG_Team_Select" );
			pPlayer->m_iMenu = Menu_ChooseTeam;
		}
	}
	else if ( FStrEq(pcmd, "showbriefing" ) )
	{
		// Beta 5.0 (leak): the map's briefing again; Beta 5.0: for a player in a team
		if ( g_szMapBriefingText[0] && pPlayer->m_iTeam != TEAM_UNASSIGNED )
		{
			pPlayer->MenuPrint( pPlayer, g_szMapBriefingText );
			pPlayer->m_fShowingLevelText = true;
		}
	}
	else if ( FStrEq(pcmd, "listplayers" ) )
	{
		// sic: Beta 6.0's new test of listplayers, after vote, answers it first
		ListPlayers( pPlayer );
	}
	else if ( FStrEq(pcmd, "ignoremsg" ) )
	{
		// 0 -> 1 -> 2 -> 0 (Host_Say); Beta 5.0 (leak): ignoremsg (was ignoremessages)
		if ( pPlayer->m_iIgnoreMessages == 0 )
		{
			pPlayer->m_iIgnoreMessages = 1;
			ClientPrint( pPlayer->pev, HUD_PRINTCENTER, "#Ignore_Broadcast_Messages" );
		}
		else if ( pPlayer->m_iIgnoreMessages == 1 )
		{
			pPlayer->m_iIgnoreMessages = 2;
			ClientPrint( pPlayer->pev, HUD_PRINTCENTER, "#Ignore_Broadcast_Team_Messages" );
		}
		else
		{
			pPlayer->m_iIgnoreMessages = 0;
			ClientPrint( pPlayer->pev, HUD_PRINTCENTER, "#Accept_All_Messages" );
		}
	}
	else if ( FStrEq(pcmd, "ignorerad" ) )
	{
		// Beta 3.1: the radio on and off (CBasePlayer::Radio); Beta 5.0
		// (leak): ignorerad (was ignoreradio)
		if ( !pPlayer->m_fIgnoreRadio )
		{
			pPlayer->m_fIgnoreRadio = true;
			ClientPrint( pPlayer->pev, HUD_PRINTCENTER, "#Ignore_Radio" );
		}
		else
		{
			pPlayer->m_fIgnoreRadio = false;
			ClientPrint( pPlayer->pev, HUD_PRINTCENTER, "#Accept_Radio" );
		}
	}
	else if ( FStrEq(pcmd, "become_vip" ) )
	{
		// Beta 6.0: a counter-terrorist in the game queues to be the VIP
		if ( pPlayer->m_iJoiningState == JOIN_DONE && pPlayer->m_iTeam == TEAM_CT )
			mp->AddToVIPQueue( pPlayer );
		else
			ClientPrint( pPlayer->pev, HUD_PRINTCENTER, "#Command_Not_Available" );
	}
	else if ( pPlayer->pev->deadflag != DEAD_NO )
	{
		// the dead have no other command
	}
	else if ( FStrEq(pcmd, "nightvision" ) )
	{
		// Beta 6.0: the goggles on and off, if bought (Beta 7.0: for the living only; the test stood before menuselect)
		if ( !pPlayer->m_bHasNightVision )
			return;

		if ( !pPlayer->m_bNightVisionOn )
		{
			EMIT_SOUND( ENT(pPlayer->pev), CHAN_ITEM, "items/nvg_on.wav", RANDOM_FLOAT( 0.92, 1 ), 0.8 );

			MESSAGE_BEGIN( MSG_ONE, gmsgNVGToggle, NULL, pPlayer->pev );
				WRITE_BYTE( 1 );
			MESSAGE_END();

			pPlayer->m_bNightVisionOn = true;	// Beta 6.5: Beta 6.1's lambert commands are gone
		}
		else
		{
			EMIT_SOUND( ENT(pPlayer->pev), CHAN_ITEM, "items/nvg_off.wav", RANDOM_FLOAT( 0.92, 1 ), 0.8 );

			MESSAGE_BEGIN( MSG_ONE, gmsgNVGToggle, NULL, pPlayer->pev );
				WRITE_BYTE( 0 );
			MESSAGE_END();

			pPlayer->m_bNightVisionOn = false;
		}
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
		ShowMenu( pPlayer, 0x3ff, -1, FALSE, "#RadioC" );	// Beta 6.0: key 9
		pPlayer->m_iMenu = Menu_Radio3;
	}
	else if ( FStrEq(pcmd, "drop" ) )
	{
		// Beta 5.0 (leak): the SDK's drop of the item named
		GetClassPtr((CBasePlayer *)pev)->DropPlayerItem( CMD_ARGV(1) );
	}
	else if ( FStrEq(pcmd, "fov" ) )
	{
		// Beta 6.5: the SDK's fov does nothing any more
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
		// Beta 6.0: key 6 (the goggles) on a bomb map only; Beta 6.1: the
		// plain menu has key 6 too, as menuselect's ("can buy nightvision
		// using the 'buyequip' command")
		if ( pPlayer->m_signals.GetState() & 1 )
		{
			if ( mp->m_fMapHasBombTarget == true )
			{
				if ( pPlayer->m_iTeam == TEAM_CT )
					ShowVGUIMenu( pPlayer, 34, 0x27f, "#DCT_BuyItem" );
				else
					ShowVGUIMenu( pPlayer, 34, 0x25f, "#DT_BuyItem" );
			}
			else
				ShowVGUIMenu( pPlayer, 34, 0x25f, "#BuyItem" );
			pPlayer->m_iMenu = Menu_BuyItem;
		}
	}
	else if ( FStrEq(pcmd, "buy" ) )
	{
		if ( pPlayer->m_signals.GetState() & 1 )
		{
			ShowVGUIMenu( pPlayer, 28, 0x2ff, "#Buy" );
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

	CBasePlayer *pPlayer = (CBasePlayer *)GET_PRIVATE( pEntity );

	// (Counter-Strike 1.0b: Beta 6.6's flag for CBasePlayer::VerifyModel is gone with that function)

	// msg everyone if someone changes their name,  and it isn't the first time (changing no name to current name)
	if ( pEntity->v.netname && STRING(pEntity->v.netname)[0] != 0 && !FStrEq( STRING(pEntity->v.netname), g_engfuncs.pfnInfoKeyValue( infobuffer, "name" )) )
	{
		// Beta 5.2: a dead player keeps his name (it is put back)
		if ( pPlayer->pev->deadflag != DEAD_NO )
		{
			char szOldName[256];	// name ours
			strcpy( szOldName, STRING( pEntity->v.netname ) );
			g_engfuncs.pfnSetClientKeyValue( pPlayer->entindex(), g_engfuncs.pfnGetInfoKeyBuffer( pPlayer->edict() ), "name", szOldName );
			return;
		}

		// Counter-Strike 1.0b: the new name with every '%' turned into a
		// space, as in ClientPutInServer (Beta 6.6 had such a filter here,
		// Beta 7.0 to 1.0 none); the names are ClientPutInServer's, and
		// szBufferName Counter-Strike 1.6's (a local of the function there)
		char sName[256];
		const char *szBufferName = g_engfuncs.pfnInfoKeyValue( infobuffer, "name" );
		strncpy( sName, szBufferName, 255 );
		sName[255] = 0;

		for ( char *pApersand = sName; pApersand != NULL && *pApersand != 0; pApersand++ )
		{
			if ( *pApersand == '%' )
				*pApersand = ' ';
		}

		g_engfuncs.pfnSetClientKeyValue( ENTINDEX(pEntity), infobuffer, "name", sName );

		char text[256];
		sprintf( text, "* %s changed name to %s\n", STRING(pEntity->v.netname), g_engfuncs.pfnInfoKeyValue( infobuffer, "name" ) );
		MESSAGE_BEGIN( MSG_BROADCAST, gmsgSayText, NULL );	// Beta 6.0: unreliable (was MSG_ALL)
			WRITE_BYTE( ENTINDEX(pEntity) );
			WRITE_STRING( text );
		MESSAGE_END();

		UTIL_LogPrintf( "\"%s<%i>\" changed name to \"%s<%i>\"\n", STRING( pEntity->v.netname ), GETPLAYERUSERID( pEntity ), g_engfuncs.pfnInfoKeyValue( infobuffer, "name" ), GETPLAYERUSERID( pEntity ) );
	}

	// (Beta 7.0: the lefthand key is no longer read here)

	g_pGameRules->ClientUserInfoChanged( GetClassPtr((CBasePlayer *)&pEntity->v), infobuffer );
}

static int g_serveractive = 0;

void ServerDeactivate( void )
{
	// It's possible that the engine will call this function more times than is necessary
	//  Therefore, only run it one time for each call to ServerActivate 
	if ( g_serveractive != 1 )
	{
		return;
	}

	g_serveractive = 0;

	// Peform any shutdown operations here...
	//
}

void ServerActivate( edict_t *pEdictList, int edictCount, int clientMax )
{
	int				i;
	CBaseEntity		*pClass;

	// Every call to ServerActivate should be matched by a call to ServerDeactivate
	g_serveractive = 1;

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

	// Link user messages here to make sure first client can get them...
	LinkUserMessages();
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
	PRECACHE_SOUND("weapons/dryfire_pistol.wav");	// Beta 6.5
	PRECACHE_SOUND("weapons/dryfire_rifle.wav");
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
	PRECACHE_SOUND("weapons/c4_beep1.wav");		// Beta 4.0: the bomb; Beta 5.0: five beeps in turn (C4Think)
	PRECACHE_SOUND("weapons/c4_beep2.wav");
	PRECACHE_SOUND("weapons/c4_beep3.wav");
	PRECACHE_SOUND("weapons/c4_beep4.wav");
	PRECACHE_SOUND("weapons/c4_beep5.wav");
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
	
	// Beta 6.0: footsteps on snow
	PRECACHE_SOUND("player/pl_snow1.wav");
	PRECACHE_SOUND("player/pl_snow2.wav");
	PRECACHE_SOUND("player/pl_snow3.wav");
	PRECACHE_SOUND("player/pl_snow4.wav");
	PRECACHE_SOUND("player/pl_snow5.wav");
	PRECACHE_SOUND("player/pl_snow6.wav");

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
	PRECACHE_SOUND("plats/vehicle_ignition.wav");	// Beta 7.0

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

	// Beta 7.0: never true. The block only puts the model names, and
	// scrambled copies of them, into the DLL for whoever searches it (to
	// Counter-Strike 1.0 the names that were used were put together at run
	// time below; 1.0b keeps the block)
	if ( RANDOM_LONG( 10, 20 ) < 0 )
	{
		ALERT( at_console, "models/player/terror/terror.mdl" );
		ALERT( at_console, "model terror\n" );
		ALERT( at_console, "terror" );
		ALERT( at_console, "rorret" );
		ALERT( at_console, "ertorr" );
		ALERT( at_console, "torerr" );
		ALERT( at_console, "models/player/leet/leet.mdl" );
		ALERT( at_console, "model leet\n" );
		ALERT( at_console, "leet" );
		ALERT( at_console, "teel" );
		ALERT( at_console, "elet" );
		ALERT( at_console, "etel" );
		ALERT( at_console, "models/player/arctic/arctic.mdl" );
		ALERT( at_console, "model arctic\n" );
		ALERT( at_console, "arctic" );
		ALERT( at_console, "citcra" );
		ALERT( at_console, "ctacri" );
		ALERT( at_console, "cicart" );
		ALERT( at_console, "models/player/guerilla/guerilla.mdl" );
		ALERT( at_console, "model guerilla\n" );
		ALERT( at_console, "guerilla" );
		ALERT( at_console, "allireug" );
		ALERT( at_console, "irlaguel" );
		ALERT( at_console, "eulagrli" );
		ALERT( at_console, "models/player/urban/urban.mdl" );
		ALERT( at_console, "model urban\n" );
		ALERT( at_console, "urban" );
		ALERT( at_console, "nabru" );
		ALERT( at_console, "buanr" );
		ALERT( at_console, "nurab" );
		ALERT( at_console, "models/player/gsg9/gsg9.mdl" );
		ALERT( at_console, "model gsg9\n" );
		ALERT( at_console, "gsg9" );
		ALERT( at_console, "9gsg" );
		ALERT( at_console, "sg9g" );
		ALERT( at_console, "gg9s" );
		ALERT( at_console, "models/player/sas/sas.mdl" );
		ALERT( at_console, "model sas\n" );
		ALERT( at_console, "sas" );
		ALERT( at_console, "ass" );
		ALERT( at_console, "ssa" );
		ALERT( at_console, "models/player/gign/gign.mdl" );
		ALERT( at_console, "model gign\n" );
		ALERT( at_console, "gign" );
		ALERT( at_console, "ngig" );
		ALERT( at_console, "igng" );
		ALERT( at_console, "nigg" );
		ALERT( at_console, "models/player/vip/vip.mdl" );
		ALERT( at_console, "model vip\n" );
		ALERT( at_console, "vip" );
		ALERT( at_console, "piv" );
		ALERT( at_console, "ipv" );
		ALERT( at_console, "vpi" );
	}

	// Counter-Strike 1.0b: the nine player models' file names are literals
	// again (Beta 7.0 to 1.0 put them together at run time from the letters)
	PRECACHE_MODEL("models/player.mdl");
	PRECACHE_MODEL("models/player/leet/leet.mdl");
	PRECACHE_MODEL("models/player/gign/gign.mdl");
	PRECACHE_MODEL("models/player/gsg9/gsg9.mdl");
	PRECACHE_MODEL("models/player/guerilla/guerilla.mdl");
	PRECACHE_MODEL("models/player/arctic/arctic.mdl");
	PRECACHE_MODEL("models/player/sas/sas.mdl");
	PRECACHE_MODEL("models/player/terror/terror.mdl");
	PRECACHE_MODEL("models/player/urban/urban.mdl");
	PRECACHE_MODEL("models/player/vip/vip.mdl");

	// Beta 6.6: the player models are precached here, and the client's files must
	// keep within these bounds (vMin, vMax: Counter-Strike 1.6's names)
	Vector vMin( -38, -22, -41 );	// Counter-Strike 1.0 (Beta 7.1: 48, 36, 58)
	Vector vMax( 38, 22, 41 );

	ENGINE_FORCE_UNMODIFIED( force_model_specifybounds, (float *)&vMin, (float *)&vMax, "models/player.mdl" );
	ENGINE_FORCE_UNMODIFIED( force_model_specifybounds, (float *)&vMin, (float *)&vMax, "models/player/leet/leet.mdl" );
	ENGINE_FORCE_UNMODIFIED( force_model_specifybounds, (float *)&vMin, (float *)&vMax, "models/player/gign/gign.mdl" );
	ENGINE_FORCE_UNMODIFIED( force_model_specifybounds, (float *)&vMin, (float *)&vMax, "models/player/gsg9/gsg9.mdl" );
	ENGINE_FORCE_UNMODIFIED( force_model_specifybounds, (float *)&vMin, (float *)&vMax, "models/player/guerilla/guerilla.mdl" );
	ENGINE_FORCE_UNMODIFIED( force_model_specifybounds, (float *)&vMin, (float *)&vMax, "models/player/arctic/arctic.mdl" );
	ENGINE_FORCE_UNMODIFIED( force_model_specifybounds, (float *)&vMin, (float *)&vMax, "models/player/sas/sas.mdl" );
	ENGINE_FORCE_UNMODIFIED( force_model_specifybounds, (float *)&vMin, (float *)&vMax, "models/player/terror/terror.mdl" );
	ENGINE_FORCE_UNMODIFIED( force_model_specifybounds, (float *)&vMin, (float *)&vMax, "models/player/urban/urban.mdl" );
	ENGINE_FORCE_UNMODIFIED( force_model_specifybounds, (float *)&vMin, (float *)&vMax, "models/player/vip/vip.mdl" );

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

	// Beta 6.5: sprites for the client (wall puffs, smoke, and a set named
	// after the screen's sides and corners); for some the engine is told that
	// the client's file must be the server's (force_exactfile; the bounds are
	// not used for that, both point at one zero vector). temp is
	// Counter-Strike 1.6's name (its debug information)
	Vector temp = g_vecZero;

	PRECACHE_MODEL("sprites/wall_puff1.spr");
	PRECACHE_MODEL("sprites/wall_puff2.spr");
	PRECACHE_MODEL("sprites/wall_puff3.spr");
	PRECACHE_MODEL("sprites/wall_puff4.spr");
	PRECACHE_MODEL("sprites/black_smoke1.spr");
	PRECACHE_MODEL("sprites/black_smoke2.spr");
	PRECACHE_MODEL("sprites/black_smoke3.spr");
	PRECACHE_MODEL("sprites/black_smoke4.spr");
	PRECACHE_MODEL("sprites/fast_wallpuff1.spr");

	ENGINE_FORCE_UNMODIFIED( force_exactfile, (float *)&temp, (float *)&temp, "sprites/black_smoke1.spr" );
	ENGINE_FORCE_UNMODIFIED( force_exactfile, (float *)&temp, (float *)&temp, "sprites/black_smoke2.spr" );
	ENGINE_FORCE_UNMODIFIED( force_exactfile, (float *)&temp, (float *)&temp, "sprites/black_smoke3.spr" );
	ENGINE_FORCE_UNMODIFIED( force_exactfile, (float *)&temp, (float *)&temp, "sprites/black_smoke4.spr" );
	ENGINE_FORCE_UNMODIFIED( force_exactfile, (float *)&temp, (float *)&temp, "sprites/fast_wallpuff1.spr" );

	PRECACHE_MODEL("sprites/pistol_smoke1.spr");
	PRECACHE_MODEL("sprites/pistol_smoke2.spr");
	PRECACHE_MODEL("sprites/rifle_smoke1.spr");
	PRECACHE_MODEL("sprites/rifle_smoke2.spr");
	PRECACHE_MODEL("sprites/rifle_smoke3.spr");
	PRECACHE_MODEL("sprites/bottom.spr");
	PRECACHE_MODEL("sprites/bottom2.spr");
	PRECACHE_MODEL("sprites/bottom3.spr");
	PRECACHE_MODEL("sprites/bottom_left.spr");
	PRECACHE_MODEL("sprites/bottom_left2.spr");
	PRECACHE_MODEL("sprites/bottom_left3.spr");
	PRECACHE_MODEL("sprites/bottom_right.spr");
	PRECACHE_MODEL("sprites/bottom_right2.spr");
	PRECACHE_MODEL("sprites/bottom_right3.spr");
	PRECACHE_MODEL("sprites/left.spr");
	PRECACHE_MODEL("sprites/left2.spr");
	PRECACHE_MODEL("sprites/left3.spr");
	PRECACHE_MODEL("sprites/right.spr");
	PRECACHE_MODEL("sprites/right2.spr");
	PRECACHE_MODEL("sprites/right3.spr");
	PRECACHE_MODEL("sprites/top.spr");
	PRECACHE_MODEL("sprites/top2.spr");
	PRECACHE_MODEL("sprites/top3.spr");
	PRECACHE_MODEL("sprites/top_left.spr");
	PRECACHE_MODEL("sprites/top_left2.spr");
	PRECACHE_MODEL("sprites/top_left3.spr");
	PRECACHE_MODEL("sprites/top_right.spr");
	PRECACHE_MODEL("sprites/top_right2.spr");
	PRECACHE_MODEL("sprites/top_right3.spr");
	PRECACHE_MODEL("sprites/vertical.spr");
	PRECACHE_MODEL("sprites/horizontal.spr");

	ENGINE_FORCE_UNMODIFIED( force_exactfile, (float *)&temp, (float *)&temp, "sprites/bottom.spr" );
	ENGINE_FORCE_UNMODIFIED( force_exactfile, (float *)&temp, (float *)&temp, "sprites/bottom2.spr" );
	ENGINE_FORCE_UNMODIFIED( force_exactfile, (float *)&temp, (float *)&temp, "sprites/bottom3.spr" );
	ENGINE_FORCE_UNMODIFIED( force_exactfile, (float *)&temp, (float *)&temp, "sprites/bottom_left.spr" );
	ENGINE_FORCE_UNMODIFIED( force_exactfile, (float *)&temp, (float *)&temp, "sprites/bottom_left2.spr" );
	ENGINE_FORCE_UNMODIFIED( force_exactfile, (float *)&temp, (float *)&temp, "sprites/bottom_left3.spr" );
	ENGINE_FORCE_UNMODIFIED( force_exactfile, (float *)&temp, (float *)&temp, "sprites/bottom_right.spr" );
	ENGINE_FORCE_UNMODIFIED( force_exactfile, (float *)&temp, (float *)&temp, "sprites/bottom_right2.spr" );
	ENGINE_FORCE_UNMODIFIED( force_exactfile, (float *)&temp, (float *)&temp, "sprites/bottom_right3.spr" );
	ENGINE_FORCE_UNMODIFIED( force_exactfile, (float *)&temp, (float *)&temp, "sprites/left.spr" );
	ENGINE_FORCE_UNMODIFIED( force_exactfile, (float *)&temp, (float *)&temp, "sprites/left2.spr" );
	ENGINE_FORCE_UNMODIFIED( force_exactfile, (float *)&temp, (float *)&temp, "sprites/left3.spr" );
	ENGINE_FORCE_UNMODIFIED( force_exactfile, (float *)&temp, (float *)&temp, "sprites/right.spr" );
	ENGINE_FORCE_UNMODIFIED( force_exactfile, (float *)&temp, (float *)&temp, "sprites/right2.spr" );
	ENGINE_FORCE_UNMODIFIED( force_exactfile, (float *)&temp, (float *)&temp, "sprites/right3.spr" );
	ENGINE_FORCE_UNMODIFIED( force_exactfile, (float *)&temp, (float *)&temp, "sprites/top.spr" );
	ENGINE_FORCE_UNMODIFIED( force_exactfile, (float *)&temp, (float *)&temp, "sprites/top2.spr" );
	ENGINE_FORCE_UNMODIFIED( force_exactfile, (float *)&temp, (float *)&temp, "sprites/top3.spr" );
	ENGINE_FORCE_UNMODIFIED( force_exactfile, (float *)&temp, (float *)&temp, "sprites/top_left.spr" );
	ENGINE_FORCE_UNMODIFIED( force_exactfile, (float *)&temp, (float *)&temp, "sprites/top_left2.spr" );
	ENGINE_FORCE_UNMODIFIED( force_exactfile, (float *)&temp, (float *)&temp, "sprites/top_left3.spr" );
	ENGINE_FORCE_UNMODIFIED( force_exactfile, (float *)&temp, (float *)&temp, "sprites/top_right.spr" );
	ENGINE_FORCE_UNMODIFIED( force_exactfile, (float *)&temp, (float *)&temp, "sprites/top_right2.spr" );
	ENGINE_FORCE_UNMODIFIED( force_exactfile, (float *)&temp, (float *)&temp, "sprites/top_right3.spr" );
	ENGINE_FORCE_UNMODIFIED( force_exactfile, (float *)&temp, (float *)&temp, "sprites/vertical.spr" );
	ENGINE_FORCE_UNMODIFIED( force_exactfile, (float *)&temp, (float *)&temp, "sprites/horizontal.spr" );
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
Sys_Error

Engine is going to shut down, allows setting a breakpoint in game .dll to catch that occasion
================
*/
void Sys_Error( const char *error_string )
{
	// Default case, do nothing.  MOD AUTHORS:  Add code ( e.g., _asm { int 3 }; here to cause a breakpoint for debugging your game .dlls
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


////////////////////////////////////////////////////////
// PAS and PVS routines for client messaging
//

/*
================
SetupVisibility

A client can have a separate "view entity" indicating that his/her view should depend on the origin of that
view entity.  If that's the case, then pViewEntity will be non-NULL and will be used.  Otherwise, the current
entity's origin is used.  Either is offset by the view_ofs to get the eye position.

From the eye position, we set up the PAS and PVS to use for filtering network messages to the client.  At this point, we could
 override the actual PAS or PVS values, or use a different origin.

NOTE:  Do not cache the values of pas and pvs, as they depend on reusable memory in the engine, they are only good for this one frame
================
*/
void SetupVisibility( edict_t *pViewEntity, edict_t *pClient, unsigned char **pvs, unsigned char **pas )
{
	Vector org;
	edict_t *pView = pClient;

	// Find the client's PVS
	if ( pViewEntity )
	{
		pView = pViewEntity;
	}

	// Beta 6.5: an observer sees what the player he watches sees
	CBasePlayer *pPlayer = (CBasePlayer *)CBaseEntity::Instance( pClient );

	if ( pPlayer->pev->iuser2 && pPlayer->m_hObserverTarget != NULL )
	{
		pView = pPlayer->m_hObserverTarget->edict();
	}

	org = pView->v.origin + pView->v.view_ofs;
	if ( pView->v.flags & FL_DUCKING )
	{
		org = org + ( VEC_HULL_MIN - VEC_DUCK_HULL_MIN );
	}

	*pvs = ENGINE_SET_PVS ( (float *)&org );
	*pas = ENGINE_SET_PAS ( (float *)&org );
}

#include "entity_state.h"

/*
AddToFullPack

Return 1 if the entity state has been filled in for the ent and the entity will be propagated to the client, 0 otherwise

state is the server maintained copy of the state info that is transmitted to the client
a MOD could alter values copied into state to send the "host" a different look for a particular entity update, etc.
e and ent are the entity that is being added to the update, if 1 is returned
host is the player's edict of the player whom we are sending the update to
player is 1 if the ent/e is a player and 0 otherwise
pSet is either the PAS or PVS that we previous set up.  We can use it to ask the engine to filter the entity against the PAS or PVS.
we could also use the pas/ pvs that we set in SetupVisibility, if we wanted to.  Caching the value is valid in that case, but still only for the current frame
*/
int AddToFullPack( struct entity_state_s *state, int e, edict_t *ent, edict_t *host, int hostflags, int player, unsigned char *pSet )
{
	int					i;

	// don't send if flagged for NODRAW and it's not the host getting the message
	if ( ( ent->v.effects == EF_NODRAW ) &&
		 ( ent != host ) )
		return 0;

	// Ignore ents without valid / visible models
	if ( !ent->v.modelindex || !STRING( ent->v.model ) )
		return 0;

	// Don't send spectators to other players
	if ( ( ent->v.flags & FL_SPECTATOR ) && ( ent != host ) )
	{
		return 0;
	}

	// Ignore if not the host and not touching a PVS/PAS leaf
	// If pSet is NULL, then the test will always succeed and the entity will be added to the update
	if ( ent != host )
	{
		if ( !ENGINE_CHECK_VISIBILITY( (const struct edict_s *)ent, pSet ) )
		{
			return 0;
		}
	}


	// Don't send entity to local client if the client says it's predicting the entity itself.
	if ( ent->v.flags & FL_SKIPLOCALHOST )
	{
		if ( ( hostflags & 1 ) && ( ent->v.owner == host ) )
			return 0;
	}
	
	if ( host->v.groupinfo )
	{
		UTIL_SetGroupTrace( host->v.groupinfo, GROUP_OP_AND );

		// Should always be set, of course
		if ( ent->v.groupinfo )
		{
			if ( g_groupop == GROUP_OP_AND )
			{
				if ( !(ent->v.groupinfo & host->v.groupinfo ) )
					return 0;
			}
			else if ( g_groupop == GROUP_OP_NAND )
			{
				if ( ent->v.groupinfo & host->v.groupinfo )
					return 0;
			}
		}

		UTIL_UnsetGroupTrace();
	}

	memset( state, 0, sizeof( *state ) );

	// Assign index so we can track this entity from frame to frame and
	//  delta from it.
	state->number	  = e;
	state->entityType = ENTITY_NORMAL;
	
	// Flag custom entities.
	if ( ent->v.flags & FL_CUSTOMENTITY )
	{
		state->entityType = ENTITY_BEAM;
	}

	// 
	// Copy state data
	//

	// Round animtime to nearest millisecond
	state->animtime   = (int)(1000.0 * ent->v.animtime ) / 1000.0;

	memcpy( state->origin, ent->v.origin, 3 * sizeof( float ) );
	memcpy( state->angles, ent->v.angles, 3 * sizeof( float ) );
	memcpy( state->mins, ent->v.mins, 3 * sizeof( float ) );
	memcpy( state->maxs, ent->v.maxs, 3 * sizeof( float ) );

	memcpy( state->startpos, ent->v.startpos, 3 * sizeof( float ) );
	memcpy( state->endpos, ent->v.endpos, 3 * sizeof( float ) );

	state->impacttime = ent->v.impacttime;
	state->starttime = ent->v.starttime;

	state->modelindex = ent->v.modelindex;
		
	state->frame      = ent->v.frame;

	state->skin       = ent->v.skin;
	state->effects    = ent->v.effects;

	// This non-player entity is being moved by the game .dll and not the physics simulation system
	//  make sure that we interpolate it's position on the client if it moves
	if ( !player &&
		 ent->v.animtime &&
		 ent->v.velocity[ 0 ] == 0 && 
		 ent->v.velocity[ 1 ] == 0 && 
		 ent->v.velocity[ 2 ] == 0 )
	{
		state->eflags |= EFLAG_SLERP;
	}

	state->scale	  = ent->v.scale;
	state->solid	  = ent->v.solid;
	state->colormap   = ent->v.colormap;
	state->movetype   = ent->v.movetype;
	state->sequence   = ent->v.sequence;
	state->framerate  = ent->v.framerate;
	state->body       = ent->v.body;

	for (i = 0; i < 4; i++)
	{
		state->controller[i] = ent->v.controller[i];
	}

	for (i = 0; i < 2; i++)
	{
		state->blending[i]   = ent->v.blending[i];
	}

	state->rendermode    = ent->v.rendermode;
	state->renderamt     = ent->v.renderamt; 
	state->renderfx      = ent->v.renderfx;
	state->rendercolor.r = ent->v.rendercolor[0];
	state->rendercolor.g = ent->v.rendercolor[1];
	state->rendercolor.b = ent->v.rendercolor[2];

	state->aiment = 0;
	if ( ent->v.aiment )
	{
		state->aiment = ENTINDEX( ent->v.aiment );
	}

	state->owner = 0;
	if ( ent->v.owner )
	{
		int owner = ENTINDEX( ent->v.owner );
		
		// Only care if owned by a player
		if ( owner >= 1 && owner <= gpGlobals->maxClients )
		{
			state->owner = owner;	
		}
	}

	// HACK:  Somewhat...
	// Class is overridden for non-players to signify a breakable glass object ( sort of a class? )
	if ( !player )
	{
		state->playerclass  = ent->v.playerclass;
	}

	// Special stuff for players only
	if ( player )
	{
		memcpy( state->basevelocity, ent->v.basevelocity, 3 * sizeof( float ) );

		state->weaponmodel  = MODEL_INDEX( STRING( ent->v.weaponmodel ) );
		state->gaitsequence = ent->v.gaitsequence;
		state->spectator = ent->v.flags & FL_SPECTATOR;
		state->friction     = ent->v.friction;

		state->gravity      = ent->v.gravity;
//		state->team			= ent->v.team;
//		state->playerclass  = ent->v.playerclass;
		state->usehull      = ( ent->v.flags & FL_DUCKING ) ? 1 : 0;
		state->health		= ent->v.health;
	}

	state->iuser4 = ent->v.iuser4;	// Beta 7.1

	return 1;
}

// defaults for clientinfo messages
#define	DEFAULT_VIEWHEIGHT	28

/*
===================
CreateBaseline

Creates baselines used for network encoding, especially for player data since players are not spawned until connect time.
===================
*/
void CreateBaseline( int player, int eindex, struct entity_state_s *baseline, struct edict_s *entity, int playermodelindex, vec3_t player_mins, vec3_t player_maxs )
{
	baseline->origin		= entity->v.origin;
	baseline->angles		= entity->v.angles;
	baseline->frame			= entity->v.frame;
	baseline->skin			= (short)entity->v.skin;

	// render information
	baseline->rendermode	= (byte)entity->v.rendermode;
	baseline->renderamt		= (byte)entity->v.renderamt;
	baseline->rendercolor.r	= (byte)entity->v.rendercolor[0];
	baseline->rendercolor.g	= (byte)entity->v.rendercolor[1];
	baseline->rendercolor.b	= (byte)entity->v.rendercolor[2];
	baseline->renderfx		= (byte)entity->v.renderfx;

	if ( player )
	{
		baseline->mins			= player_mins;
		baseline->maxs			= player_maxs;

		baseline->colormap		= eindex;
		baseline->modelindex	= playermodelindex;
		baseline->friction		= 1.0;
		baseline->movetype		= MOVETYPE_WALK;

		baseline->scale			= entity->v.scale;
		baseline->solid			= SOLID_SLIDEBOX;
		baseline->framerate		= 1.0;
		baseline->gravity		= 1.0;

	}
	else
	{
		baseline->mins			= entity->v.mins;
		baseline->maxs			= entity->v.maxs;

		baseline->colormap		= 0;
		baseline->modelindex	= entity->v.modelindex;//SV_ModelIndex(pr_strings + entity->v.model);
		baseline->movetype		= entity->v.movetype;

		baseline->scale			= entity->v.scale;
		baseline->solid			= entity->v.solid;
		baseline->framerate		= entity->v.framerate;
		baseline->gravity		= entity->v.gravity;
	}
}

typedef struct
{
	char name[32];
	int	 field;
} entity_field_alias_t;

#define FIELD_ORIGIN0			0
#define FIELD_ORIGIN1			1
#define FIELD_ORIGIN2			2
#define FIELD_ANGLES0			3
#define FIELD_ANGLES1			4
#define FIELD_ANGLES2			5

static entity_field_alias_t entity_field_alias[]=
{
	{ "origin[0]",			0 },
	{ "origin[1]",			0 },
	{ "origin[2]",			0 },
	{ "angles[0]",			0 },
	{ "angles[1]",			0 },
	{ "angles[2]",			0 },
};

void Entity_FieldInit( struct delta_s *pFields )
{
	entity_field_alias[ FIELD_ORIGIN0 ].field		= DELTA_FINDFIELD( pFields, entity_field_alias[ FIELD_ORIGIN0 ].name );
	entity_field_alias[ FIELD_ORIGIN1 ].field		= DELTA_FINDFIELD( pFields, entity_field_alias[ FIELD_ORIGIN1 ].name );
	entity_field_alias[ FIELD_ORIGIN2 ].field		= DELTA_FINDFIELD( pFields, entity_field_alias[ FIELD_ORIGIN2 ].name );
	entity_field_alias[ FIELD_ANGLES0 ].field		= DELTA_FINDFIELD( pFields, entity_field_alias[ FIELD_ANGLES0 ].name );
	entity_field_alias[ FIELD_ANGLES1 ].field		= DELTA_FINDFIELD( pFields, entity_field_alias[ FIELD_ANGLES1 ].name );
	entity_field_alias[ FIELD_ANGLES2 ].field		= DELTA_FINDFIELD( pFields, entity_field_alias[ FIELD_ANGLES2 ].name );
}

/*
==================
Entity_Encode

Callback for sending entity_state_t info over network. 
FIXME:  Move to script
==================
*/
void Entity_Encode( struct delta_s *pFields, const unsigned char *from, const unsigned char *to )
{
	entity_state_t *f, *t;

	int localplayer = 0;
	static int initialized = 0;

	if ( !initialized )
	{
		Entity_FieldInit( pFields );
		initialized = 1;
	}

	f = (entity_state_t *)from;
	t = (entity_state_t *)to;

	// Never send origin to local player, it's sent with more resolution in clientdata_t structure
	localplayer =  ( t->number - 1 ) == ENGINE_CURRENT_PLAYER();
	if ( localplayer )
	{
		DELTA_UNSETBYINDEX( pFields, entity_field_alias[ FIELD_ORIGIN0 ].field );
		DELTA_UNSETBYINDEX( pFields, entity_field_alias[ FIELD_ORIGIN1 ].field );
		DELTA_UNSETBYINDEX( pFields, entity_field_alias[ FIELD_ORIGIN2 ].field );
	}

	if ( ( t->impacttime != 0 ) && ( t->starttime != 0 ) )
	{
		DELTA_UNSETBYINDEX( pFields, entity_field_alias[ FIELD_ORIGIN0 ].field );
		DELTA_UNSETBYINDEX( pFields, entity_field_alias[ FIELD_ORIGIN1 ].field );
		DELTA_UNSETBYINDEX( pFields, entity_field_alias[ FIELD_ORIGIN2 ].field );

		DELTA_UNSETBYINDEX( pFields, entity_field_alias[ FIELD_ANGLES0 ].field );
		DELTA_UNSETBYINDEX( pFields, entity_field_alias[ FIELD_ANGLES1 ].field );
		DELTA_UNSETBYINDEX( pFields, entity_field_alias[ FIELD_ANGLES2 ].field );
	}

	if ( ( t->movetype == MOVETYPE_FOLLOW ) &&
		 ( t->aiment != 0 ) )
	{
		DELTA_UNSETBYINDEX( pFields, entity_field_alias[ FIELD_ORIGIN0 ].field );
		DELTA_UNSETBYINDEX( pFields, entity_field_alias[ FIELD_ORIGIN1 ].field );
		DELTA_UNSETBYINDEX( pFields, entity_field_alias[ FIELD_ORIGIN2 ].field );
	}
	else if ( t->aiment != f->aiment )
	{
		DELTA_SETBYINDEX( pFields, entity_field_alias[ FIELD_ORIGIN0 ].field );
		DELTA_SETBYINDEX( pFields, entity_field_alias[ FIELD_ORIGIN1 ].field );
		DELTA_SETBYINDEX( pFields, entity_field_alias[ FIELD_ORIGIN2 ].field );
	}
}

static entity_field_alias_t player_field_alias[]=
{
	{ "origin[0]",			0 },
	{ "origin[1]",			0 },
	{ "origin[2]",			0 },
};

void Player_FieldInit( struct delta_s *pFields )
{
	player_field_alias[ FIELD_ORIGIN0 ].field		= DELTA_FINDFIELD( pFields, player_field_alias[ FIELD_ORIGIN0 ].name );
	player_field_alias[ FIELD_ORIGIN1 ].field		= DELTA_FINDFIELD( pFields, player_field_alias[ FIELD_ORIGIN1 ].name );
	player_field_alias[ FIELD_ORIGIN2 ].field		= DELTA_FINDFIELD( pFields, player_field_alias[ FIELD_ORIGIN2 ].name );
}

/*
==================
Player_Encode

Callback for sending entity_state_t for players info over network. 
==================
*/
void Player_Encode( struct delta_s *pFields, const unsigned char *from, const unsigned char *to )
{
	entity_state_t *f, *t;
	int localplayer = 0;
	static int initialized = 0;

	if ( !initialized )
	{
		Player_FieldInit( pFields );
		initialized = 1;
	}

	f = (entity_state_t *)from;
	t = (entity_state_t *)to;

	// Never send origin to local player, it's sent with more resolution in clientdata_t structure
	localplayer =  ( t->number - 1 ) == ENGINE_CURRENT_PLAYER();
	if ( localplayer )
	{
		DELTA_UNSETBYINDEX( pFields, entity_field_alias[ FIELD_ORIGIN0 ].field );
		DELTA_UNSETBYINDEX( pFields, entity_field_alias[ FIELD_ORIGIN1 ].field );
		DELTA_UNSETBYINDEX( pFields, entity_field_alias[ FIELD_ORIGIN2 ].field );
	}

	if ( ( t->movetype == MOVETYPE_FOLLOW ) &&
		 ( t->aiment != 0 ) )
	{
		DELTA_UNSETBYINDEX( pFields, entity_field_alias[ FIELD_ORIGIN0 ].field );
		DELTA_UNSETBYINDEX( pFields, entity_field_alias[ FIELD_ORIGIN1 ].field );
		DELTA_UNSETBYINDEX( pFields, entity_field_alias[ FIELD_ORIGIN2 ].field );
	}
	else if ( t->aiment != f->aiment )
	{
		DELTA_SETBYINDEX( pFields, entity_field_alias[ FIELD_ORIGIN0 ].field );
		DELTA_SETBYINDEX( pFields, entity_field_alias[ FIELD_ORIGIN1 ].field );
		DELTA_SETBYINDEX( pFields, entity_field_alias[ FIELD_ORIGIN2 ].field );
	}
}

#define CUSTOMFIELD_ORIGIN0			0
#define CUSTOMFIELD_ORIGIN1			1
#define CUSTOMFIELD_ORIGIN2			2
#define CUSTOMFIELD_ANGLES0			3
#define CUSTOMFIELD_ANGLES1			4
#define CUSTOMFIELD_ANGLES2			5
#define CUSTOMFIELD_SKIN			6
#define CUSTOMFIELD_SEQUENCE		7
#define CUSTOMFIELD_ANIMTIME		8

entity_field_alias_t custom_entity_field_alias[]=
{
	{ "origin[0]",			0 },
	{ "origin[1]",			0 },
	{ "origin[2]",			0 },
	{ "angles[0]",			0 },
	{ "angles[1]",			0 },
	{ "angles[2]",			0 },
	{ "skin",				0 },
	{ "sequence",			0 },
	{ "animtime",			0 },
};

void Custom_Entity_FieldInit( struct delta_s *pFields )
{
	custom_entity_field_alias[ CUSTOMFIELD_ORIGIN0 ].field	= DELTA_FINDFIELD( pFields, custom_entity_field_alias[ CUSTOMFIELD_ORIGIN0 ].name );
	custom_entity_field_alias[ CUSTOMFIELD_ORIGIN1 ].field	= DELTA_FINDFIELD( pFields, custom_entity_field_alias[ CUSTOMFIELD_ORIGIN1 ].name );
	custom_entity_field_alias[ CUSTOMFIELD_ORIGIN2 ].field	= DELTA_FINDFIELD( pFields, custom_entity_field_alias[ CUSTOMFIELD_ORIGIN2 ].name );
	custom_entity_field_alias[ CUSTOMFIELD_ANGLES0 ].field	= DELTA_FINDFIELD( pFields, custom_entity_field_alias[ CUSTOMFIELD_ANGLES0 ].name );
	custom_entity_field_alias[ CUSTOMFIELD_ANGLES1 ].field	= DELTA_FINDFIELD( pFields, custom_entity_field_alias[ CUSTOMFIELD_ANGLES1 ].name );
	custom_entity_field_alias[ CUSTOMFIELD_ANGLES2 ].field	= DELTA_FINDFIELD( pFields, custom_entity_field_alias[ CUSTOMFIELD_ANGLES2 ].name );
	custom_entity_field_alias[ CUSTOMFIELD_SKIN ].field	= DELTA_FINDFIELD( pFields, custom_entity_field_alias[ CUSTOMFIELD_SKIN ].name );
	custom_entity_field_alias[ CUSTOMFIELD_SEQUENCE ].field= DELTA_FINDFIELD( pFields, custom_entity_field_alias[ CUSTOMFIELD_SEQUENCE ].name );
	custom_entity_field_alias[ CUSTOMFIELD_ANIMTIME ].field= DELTA_FINDFIELD( pFields, custom_entity_field_alias[ CUSTOMFIELD_ANIMTIME ].name );
}

/*
==================
Custom_Encode

Callback for sending entity_state_t info ( for custom entities ) over network. 
FIXME:  Move to script
==================
*/
void Custom_Encode( struct delta_s *pFields, const unsigned char *from, const unsigned char *to )
{
	entity_state_t *f, *t;
	int beamType;
	static int initialized = 0;

	if ( !initialized )
	{
		Custom_Entity_FieldInit( pFields );
		initialized = 1;
	}

	f = (entity_state_t *)from;
	t = (entity_state_t *)to;

	beamType = t->rendermode & 0x0f;
		
	if ( beamType != BEAM_POINTS && beamType != BEAM_ENTPOINT )
	{
		DELTA_UNSETBYINDEX( pFields, custom_entity_field_alias[ CUSTOMFIELD_ORIGIN0 ].field );
		DELTA_UNSETBYINDEX( pFields, custom_entity_field_alias[ CUSTOMFIELD_ORIGIN1 ].field );
		DELTA_UNSETBYINDEX( pFields, custom_entity_field_alias[ CUSTOMFIELD_ORIGIN2 ].field );
	}

	if ( beamType != BEAM_POINTS )
	{
		DELTA_UNSETBYINDEX( pFields, custom_entity_field_alias[ CUSTOMFIELD_ANGLES0 ].field );
		DELTA_UNSETBYINDEX( pFields, custom_entity_field_alias[ CUSTOMFIELD_ANGLES1 ].field );
		DELTA_UNSETBYINDEX( pFields, custom_entity_field_alias[ CUSTOMFIELD_ANGLES2 ].field );
	}

	if ( beamType != BEAM_ENTS && beamType != BEAM_ENTPOINT )
	{
		DELTA_UNSETBYINDEX( pFields, custom_entity_field_alias[ CUSTOMFIELD_SKIN ].field );
		DELTA_UNSETBYINDEX( pFields, custom_entity_field_alias[ CUSTOMFIELD_SEQUENCE ].field );
	}

	// animtime is compared by rounding first
	// see if we really shouldn't actually send it
	if ( (int)f->animtime == (int)t->animtime )
	{
		DELTA_UNSETBYINDEX( pFields, custom_entity_field_alias[ CUSTOMFIELD_ANIMTIME ].field );
	}
}

/*
=================
RegisterEncoders

Allows game .dll to override network encoding of certain types of entities and tweak values, etc.
=================
*/
void RegisterEncoders( void )
{
	DELTA_ADDENCODER( "Entity_Encode", Entity_Encode );
	DELTA_ADDENCODER( "Custom_Encode", Custom_Encode );
	DELTA_ADDENCODER( "Player_Encode", Player_Encode );
}

int GetWeaponData( struct edict_s *player, struct weapon_data_s *info )
{
#if defined( CLIENT_WEAPONS )
	int i;
	weapon_data_t *item;
	entvars_t *pev = &player->v;
	CBasePlayer *pl = ( CBasePlayer *) CBasePlayer::Instance( pev );
	CBasePlayerWeapon *gun;
	
	ItemInfo II;

	memset( info, 0, 32 * sizeof( weapon_data_t ) );

	if ( !pl )
		return 1;

	// go through all of the weapons and make a list of the ones to pack
	for ( i = 0 ; i < MAX_ITEM_TYPES ; i++ )
	{
		if ( pl->m_rgpPlayerItems[ i ] )
		{
			// there's a weapon here. Should I pack it?
			CBasePlayerItem *pPlayerItem = pl->m_rgpPlayerItems[ i ];

			while ( pPlayerItem )
			{
				gun = (CBasePlayerWeapon *)pPlayerItem->GetWeaponPtr();
				if ( gun && gun->UseDecrement() )
				{
					// Get The ID.
					memset( &II, 0, sizeof( II ) );
					gun->GetItemInfo( &II );

					if ( II.iId >= 0 && II.iId < 32 )
					{
						item = &info[ II.iId ];
						
						item->m_iId						= II.iId;
						item->m_iClip					= gun->m_iClip;

						item->m_flTimeWeaponIdle		= max( gun->m_flTimeWeaponIdle, -0.001 );
						item->m_flNextPrimaryAttack		= max( gun->m_flNextPrimaryAttack, -0.001 );
						item->m_flNextSecondaryAttack	= max( gun->m_flNextSecondaryAttack, -0.001 );
						// Counter-Strike: what its weapons predict with (the
						// client's HUD_WeaponsPostThink reads them back: the
						// shots fired go as m_fInZoom, the time of the last
						// shot as m_fAimedDamage)
						item->m_flNextReload			= max( gun->m_flNextReload, -0.001 );
						item->m_fInReload				= gun->m_fInReload;
						item->m_fInSpecialReload		= gun->m_fInSpecialReload;
						item->m_fInZoom					= gun->m_iShotsFired;
						item->m_fAimedDamage			= gun->m_flLastFire;
						item->m_iWeaponState			= gun->m_iWeaponState;
					}
				}
				pPlayerItem = pPlayerItem->m_pNext;
			}
		}
	}
#else
	memset( info, 0, 32 * sizeof( weapon_data_t ) );
#endif
	return 1;
}

/*
=================
UpdateClientData

Data sent to current client only
engine sets cd to 0 before calling.
=================
*/
void UpdateClientData ( const struct edict_s *ent, int sendweapons, struct clientdata_s *cd )
{
	cd->flags			= ent->v.flags;
	cd->health			= ent->v.health;

	cd->viewmodel		= MODEL_INDEX( STRING( ent->v.viewmodel ) );
	cd->waterlevel		= ent->v.waterlevel;
	cd->watertype		= ent->v.watertype;
	cd->weapons			= ent->v.weapons;

	// Vectors
	cd->origin			= ent->v.origin;
	cd->velocity		= ent->v.velocity;
	cd->view_ofs		= ent->v.view_ofs;
	cd->punchangle		= ent->v.punchangle;

	cd->bInDuck			= ent->v.bInDuck;
	cd->flTimeStepSound = ent->v.flTimeStepSound;
	cd->flDuckTime		= ent->v.flDuckTime;
	cd->flSwimTime		= ent->v.flSwimTime;
	cd->waterjumptime	= ent->v.teleport_time;

	strcpy( cd->physinfo, ENGINE_GETPHYSINFO( ent ) );

	cd->maxspeed		= ent->v.maxspeed;
	cd->fov				= ent->v.fov;
	cd->weaponanim		= ent->v.weaponanim;

	cd->pushmsec		= ent->v.pushmsec;

	// Beta 6.5: the observer's camera mode and target
	cd->iuser1			= ent->v.iuser1;
	cd->iuser2			= ent->v.iuser2;

#if defined( CLIENT_WEAPONS )
	if ( sendweapons )
	{
		entvars_t *pev = (entvars_t *)&ent->v;
		CBasePlayer *pl = ( CBasePlayer *) CBasePlayer::Instance( pev );

		if ( pl )
		{
			// Counter-Strike: the ammunition counts, whether he may shoot
			// (the freeze time) and his hand, as the client's
			// HUD_WeaponsPostThink reads them
			cd->ammo_shells		= pl->ammo_buckshot;
			cd->ammo_nails		= pl->ammo_9mm;
			cd->ammo_cells		= pl->ammo_556nato;
			cd->ammo_rockets	= pl->ammo_556natobox;
			cd->vuser2.x		= pl->ammo_762nato;
			cd->vuser2.y		= pl->ammo_45acp;
			cd->vuser2.z		= pl->ammo_50ae;
			cd->vuser3.x		= pl->ammo_338mag;
			cd->vuser3.y		= pl->ammo_57mm;
			cd->vuser3.z		= pl->ammo_357sig;
			cd->m_flNextAttack	= pl->m_flNextAttack;

			// Beta 7.1: both flags in iuser3 (to 7.0: iuser3 and iuser4). The first
			// test's form is ours: int iUser3 = ( pl->m_bCanShoot == true ) compiles
			// to the same code
			int iUser3 = 0;		// Counter-Strike 1.6's name

			if ( pl->m_bCanShoot == true )
				iUser3 |= 1;
			else
				iUser3 &= ~1;

			if ( pl->m_bLeftHanded == true )
				iUser3 |= 2;
			else
				iUser3 &= ~2;

			cd->iuser3			= iUser3;

			if ( pl->m_pActiveItem )
			{
				CBasePlayerWeapon *gun;
				gun = (CBasePlayerWeapon *)pl->m_pActiveItem->GetWeaponPtr();
				if ( gun && gun->UseDecrement() )
				{
					ItemInfo II;
					memset( &II, 0, sizeof( II ) );
					gun->GetItemInfo( &II );

					cd->m_iId = II.iId;

					// Beta 6.6: the aim no longer travels in vuser1 (the client's events take the punch angle)

					cd->vuser4.x	= gun->m_iPrimaryAmmoType;
					cd->vuser4.y	= pl->m_rgAmmo[gun->m_iPrimaryAmmoType];
				}
			}
		}
	}
#endif
}

/*
=================
CmdStart

We're about to run this usercmd for the specified player.  We can set up groupinfo and masking here, etc.
This is the time to examine the usercmd for anything extra.  This call happens even if think does not.
=================
*/
void CmdStart( const edict_t *player, const struct usercmd_s *cmd, unsigned int random_seed )
{
	entvars_t *pev = (entvars_t *)&player->v;
	CBasePlayer *pl = ( CBasePlayer *) CBasePlayer::Instance( pev );

	if( !pl )
		return;

	if ( pl->pev->groupinfo != 0 )
	{
		UTIL_SetGroupTrace( pl->pev->groupinfo, GROUP_OP_AND );
	}

	pl->random_seed = random_seed;
}

/*
=================
CmdEnd

Each cmdstart is exactly matched with a cmd end, clean up any group trace flags, etc. here
=================
*/
void CmdEnd ( const edict_t *player )
{
	entvars_t *pev = (entvars_t *)&player->v;
	CBasePlayer *pl = ( CBasePlayer *) CBasePlayer::Instance( pev );

	if( !pl )
		return;
	if ( pl->pev->groupinfo != 0 )
	{
		UTIL_UnsetGroupTrace();
	}
}

/*
================================
ConnectionlessPacket

 Return 1 if the packet is valid.  Set response_buffer_size if you want to send a response packet.  Incoming, it holds the max
  size of the response_buffer, so you must zero it out if you choose not to respond.
================================
*/
int	ConnectionlessPacket( const struct netadr_s *net_from, const char *args, char *response_buffer, int *response_buffer_size )
{
	// Parse stuff from args
	int max_buffer_size = *response_buffer_size;

	// Zero it out since we aren't going to respond.
	// If we wanted to response, we'd write data into response_buffer
	*response_buffer_size = 0;

	// Since we don't listen for anything here, just respond that it's a bogus message
	// If we didn't reject the message, we'd return 1 for success instead.
	return 0;
}

/*
================================
GetHullBounds

  Engine calls this to enumerate player collision hulls, for prediction.  Return 0 if the hullnumber doesn't exist.
================================
*/
int GetHullBounds( int hullnumber, float *mins, float *maxs )
{
	int iret = 0;

	switch ( hullnumber )
	{
	case 0:				// Normal player
		mins = VEC_HULL_MIN;
		maxs = VEC_HULL_MAX;
		iret = 1;
		break;
	case 1:				// Crouched player
		mins = VEC_DUCK_HULL_MIN;
		maxs = VEC_DUCK_HULL_MAX;
		iret = 1;
		break;
	case 2:				// Point based hull
		mins = Vector( 0, 0, 0 );
		maxs = Vector( 0, 0, 0 );
		iret = 1;
		break;
	}

	return iret;
}

/*
================================
CreateInstancedBaselines

Create pseudo-baselines for items that aren't placed in the map at spawn time, but which are likely
to be created during play ( e.g., grenades, ammo packs, projectiles, corpses, etc. )
================================
*/
void CreateInstancedBaselines ( void )
{
	int iret = 0;
	entity_state_t state;

	memset( &state, 0, sizeof( state ) );

	// Create any additional baselines here for things like grendates, etc.
	// iret = ENGINE_INSTANCE_BASELINE( pc->pev->classname, &state );

	// Destroy objects.
	//UTIL_Remove( pc );
}

/*
================================
InconsistentFile

One of the ENGINE_FORCE_UNMODIFIED files failed the consistency check for the specified player
 Return 0 to allow the client to continue, 1 to force immediate disconnection ( with an optional disconnect message of up to 256 characters )
================================
*/
int	InconsistentFile( const edict_t *player, const char *filename, char *disconnect_message )
{
	// Server doesn't care?
	if ( CVAR_GET_FLOAT( "mp_consistency" ) != 1 )
		return 0;

	// Default behavior is to kick the player
	sprintf( disconnect_message, "Server is enforcing file consistency for %s\n", filename );

	// Kick now with specified disconnect message.
	return 1;
}

/*
================================
AllowLagCompensation

 The game .dll should return 1 if lag compensation should be allowed ( could also just set
  the sv_unlag cvar.
 Most games right now should return 0, until client-side weapon prediction code is written
  and tested for them ( note you can predict weapons, but not do lag compensation, too, 
  if you want.
================================
*/
int AllowLagCompensation( void )
{
	return 1;
}


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
#include "hostage_localnav.h"	// Counter-Strike 1.4: CLocalNav (ServerDeactivate, StartFrame); the include's place is ours

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
extern cvar_t allow_spectators;	// game.cpp (Counter-Strike 1.1)
extern cvar_t kick_percent;	// game.cpp (Counter-Strike 1.4)
extern int gmsgMoney;		// player.cpp
extern int gmsgScoreInfo;
extern int gmsgSpectator;

// (Counter-Strike 1.0b: the 36 letter and digit globals g_a to g_0 that stood
// here from Beta 7.0 to 1.0, from which the model names and commands were put
// together at run time, are gone: retail's .data has this file's tables
// directly behind cbase.cpp's strings)

// Counter-Strike 1.1 (Linux symbols): when the time limit ends the map (CHalfLifeMultiplay::Think
// sets it, the timeleft command reads it) and the time of the last complete restart (RestartRound)
float g_flTimeLimit = 0;
float g_flResetTime = 0;
unsigned short m_usResetDecals;	// Counter-Strike 1.3: the decal_reset event (the Linux library's name)

void LinkUserMessages( void );

// GetTeam - a team's name for the log lines.  Counter-Strike 1.3: multiplay_gamerules.cpp's function is
// out of line and this file calls it (1.1 had the switch in every caller; Counter-Strike 1.6 declares the
// function at this place)
char *GetTeam( int teamNo );

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


//=========================================================
// TeamChangeUpdate - Beta 5.0 (leak): everybody learns the
// player's team (TeamInfo: his number and team; -1: his
// current one); in a team, his scoreboard flags too (the
// name and the parameters' names are Counter-Strike 1.6's,
// where it stands here, ahead of ClientConnect; to 1.3's
// reconstruction it was called UpdateTeamInfo). Beta 6.5:
// inline, like the three helpers below: retail has no copy
// of them outside their callers, and the Linux library no
// symbol
//=========================================================
inline void TeamChangeUpdate( CBasePlayer *player, int team_id )
{
	if ( team_id == -1 )
		team_id = player->m_iTeam;

	// Beta 6.5: the team goes as its name (SDK 2.0's TeamInfo message)
	MESSAGE_BEGIN( MSG_ALL, gmsgTeamInfo );
		WRITE_BYTE( player->entindex() );
		if ( team_id == TEAM_CT )
			WRITE_STRING( "CT" );
		else if ( team_id == TEAM_TERRORIST )
			WRITE_STRING( "TERRORIST" );
		else if ( team_id == TEAM_SPECTATOR )	// Counter-Strike 1.1
			WRITE_STRING( "SPECTATOR" );
		else
			WRITE_STRING( "UNASSIGNED" );
	MESSAGE_END();

	if ( team_id != TEAM_UNASSIGNED )
		player->SetScoreboardAttributes( NULL );
}

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

	// Counter-Strike 1.1: a title of the client with the name (to 1.0b the text
	// "- %s has left the game\n" as a SayText message)
	UTIL_ClientPrintAll( HUD_PRINTNOTIFY, "#Game_disconnected", STRING(pEntity->v.netname) );

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

	pl->m_LastHitGroup = 0;	// Counter-Strike 1.4

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

	// sic: stand-in. ClientCommand has this function in place at 17 of its 30 menus and calls
	// it at 13 (retail: 8,684 bytes); with the six lines above alone an 18th is expanded (the
	// first rifle menu; 8,752 bytes). The function's size decides it: one or two statements
	// that make no code (a return, an empty block) give retail's ClientCommand, three do not,
	// a dead store gives 15 expansions. Counter-Strike 1.6's lines have no room for such a
	// statement here, so the text that weighed as much in 1.4 is not known. The function's own
	// 83 bytes, ShowVGUIMenu and both HandleMenu functions are retail's with and without it
	return;
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
		// Counter-Strike 1.1: nor the spectators
		if ( player->m_iTeam != TEAM_UNASSIGNED && !( pPlayer->pev->flags & FL_DORMANT ) && player->m_iTeam != TEAM_SPECTATOR )
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

	ClientPrint( current->pev, HUD_PRINTCONSOLE, "\n" );	// Counter-Strike 1.4
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
// ProcessKickVote - Counter-Strike 1.4 (Linux symbol; the
// names are Counter-Strike 1.6's): a player voted to kick
// pKickPlayer. In a team of three or more the votes of the
// team for that user id are counted; with mp_kickpercent of
// the team (0 to 1, rounded) the player is kicked and those
// votes are taken back. The last loop goes on from where the
// counting loop stopped (retail's)
//=========================================================
void ProcessKickVote( CBasePlayer *pVotingPlayer, CBasePlayer *pKickPlayer )
{
	if ( !pVotingPlayer || !pKickPlayer )
		return;

	CBaseEntity *pTempEntity;
	CBasePlayer *pTempPlayer;
	int iValidVotes = 0;
	int iVoteID = pVotingPlayer->m_iCurrentKickVote;
	int iVotesNeeded;
	float fKickPercent;

	int iTeamCount = CountTeamPlayers( pVotingPlayer->m_iTeam );

	if ( iTeamCount < 3 )
		return;

	pTempEntity = NULL;

	pTempEntity = UTIL_FindEntityByClassname( pTempEntity, "player" );
	while ( pTempEntity != NULL && !FNullEnt( pTempEntity->edict() ) )
	{
		pTempPlayer = GetClassPtr( (CBasePlayer *)pTempEntity->pev );

		if ( pTempPlayer->m_iTeam != TEAM_UNASSIGNED && pTempPlayer->m_iTeam == pVotingPlayer->m_iTeam && pTempPlayer->m_iCurrentKickVote == iVoteID )
			iValidVotes++;

		pTempEntity = UTIL_FindEntityByClassname( pTempEntity, "player" );
	}

	fKickPercent = kick_percent.value;

	if ( fKickPercent < 0.0 )
	{
		fKickPercent = 0;
		CVAR_SET_STRING( "mp_kickpercent", "0.0" );
	}
	else if ( fKickPercent > 1.0 )
	{
		fKickPercent = 1;
		CVAR_SET_STRING( "mp_kickpercent", "1.0" );
	}

	iVotesNeeded = (int)( iTeamCount * fKickPercent + 0.5 );

	if ( iValidVotes >= iVotesNeeded )
	{
		UTIL_ClientPrintAll( HUD_PRINTCENTER, "#Game_kicked", STRING( pKickPlayer->pev->netname ) );
		SERVER_COMMAND( UTIL_VarArgs( "kick # %d\n", iVoteID ) );

		pTempEntity = UTIL_FindEntityByClassname( pTempEntity, "player" );
		while ( pTempEntity != NULL && !FNullEnt( pTempEntity->edict() ) )
		{
			pTempPlayer = GetClassPtr( (CBasePlayer *)pTempEntity->pev );

			if ( pTempPlayer->m_iTeam != TEAM_UNASSIGNED && pTempPlayer->m_iTeam == pVotingPlayer->m_iTeam && pTempPlayer->m_iCurrentKickVote == iVoteID )
				pTempPlayer->m_iCurrentKickVote = 0;

			pTempEntity = UTIL_FindEntityByClassname( pTempEntity, "player" );
		}
	}
}

// (Counter-Strike 1.4: KickPlayer, Beta 6.0's count of the kick votes, is gone: the Linux
// library has ProcessKickVote in its place)

// Beta 5.0 (leak): the team with fewer players (as CheckWinConditions counted them;
// Counter-Strike 1.4: either when they are even; 1.5: then the team with fewer
// rounds won), else the other one if that is
// full, else none
int SelectDefaultTeam( void )
{
	int team = TEAM_UNASSIGNED;	// name ours
	CHalfLifeMultiplay *mp = (CHalfLifeMultiplay *)g_pGameRules;	// name ours

	// (Counter-Strike 1.4: even teams are a coin toss; to 1.3 the counter-terrorists.)
	// The braces around every arm are Counter-Strike 1.6's line layout (five lines an arm), and
	// they are text the compiler counts: with them the function is over the size /Ob2 expands, as
	// retail's HandleMenu_ChooseTeam shows (it calls this function; without the braces, or with
	// the coin toss as a conditional expression, it has the body in place: 2,376 bytes for 2,292)
	if ( mp->m_iNumTerrorist < mp->m_iNumCT )
	{
		team = TEAM_TERRORIST;
	}
	else if ( mp->m_iNumTerrorist > mp->m_iNumCT )
	{
		team = TEAM_CT;
	}
	else
	{
		// Counter-Strike 1.5: with even teams the one that has won fewer rounds, and only then the coin
		if ( mp->m_iNumTerroristWins < mp->m_iNumCTWins )
		{
			team = TEAM_TERRORIST;
		}
		else if ( mp->m_iNumCTWins < mp->m_iNumTerroristWins )
		{
			team = TEAM_CT;
		}
		else
		{
			if ( RANDOM_LONG( 0, 1 ) == 0 )
			{
				team = TEAM_CT;
			}
			else
			{
				team = TEAM_TERRORIST;
			}
		}
	}

	if ( mp->TeamFull( team ) )
	{
		if ( team == TEAM_TERRORIST )
		{
			team = TEAM_CT;
		}
		else
		{
			team = TEAM_TERRORIST;
		}

		if ( mp->TeamFull( team ) )
			return TEAM_UNASSIGNED;
	}

	return team;
}

// Counter-Strike 1.3: mp_startmoney is kept between 800 and 16000 (the name is the
// Linux library's, money Counter-Strike 1.6's local)
extern cvar_t startmoney;

void CheckStartMoney( void )
{
	int money = (int)startmoney.value;

	if ( money > 16000 )
		CVAR_SET_FLOAT( "mp_startmoney", 16000 );
	else if ( money < 800 )
		CVAR_SET_FLOAT( "mp_startmoney", 800 );
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
	pPlayer->SetPrefsFromUserinfo( g_engfuncs.pfnGetInfoKeyBuffer( pEntity ) );	// Counter-Strike 1.4

	// Counter-Strike 1.0: the training course is a single-player game: no
	// joining, no team menu, the player spawns at once
	if ( !g_pGameRules->IsMultiplayer() )
	{
		pPlayer->Spawn();
		return;
	}

	pPlayer->m_fNotKilled = true;
	pPlayer->m_iIgnoreMessages = 0;
	pPlayer->m_iTeamKills = 0;
	pPlayer->m_fJustConnected = true;

	// Beta 6.5: the lefthand key is no longer read here (ClientUserInfoChanged still reads it)

	// Allocate a CBasePlayer for pev, and call spawn
	pPlayer->Spawn() ;

	pPlayer->m_fTeamChanged = false;	// Counter-Strike 1.4
	pPlayer->m_iNumSpawns = 0;

	// Beta 5.0 (leak): he starts with 800 dollars, outside the game: no team,
	// invisible, flying, not to be hurt, until JoiningThink lets him in; the
	// view is the map's intro camera if it has one, else a CT spawn point
	// Counter-Strike 1.3: the starting money is the cvar mp_startmoney, kept
	// between 800 and 16000 (1.1: 800)
	CheckStartMoney();
	pPlayer->m_iAccount = (int)startmoney.value;
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
	pPlayer->m_iHostagesKilled = 0;			// Beta 6.5
	pPlayer->m_iMapVote = 0;
	pPlayer->m_iCurrentKickVote = 0;		// Counter-Strike 1.4
	pPlayer->m_fDeadTime = 0;
	pPlayer->has_disconnected = false;
	pPlayer->m_iMenu = Menu_OFF;	// Beta 7.0
	g_engfuncs.pfnSetClientMaxspeed( ENT(pPlayer->pev), 1 );	// (to Beta 6.6: 0.001)
	// Beta 7.0: the model and the think function again (as Beta 6.5; 6.6 left both alone)
	SET_MODEL( ENT(pPlayer->pev), "models/player.mdl" );
	pPlayer->SetThink( NULL );

	// Beta 7.0: the others no longer hear of him here (TeamChangeUpdate)

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

	UTIL_ClientPrintAll( HUD_PRINTNOTIFY, "#Game_connected", ( sName[0] != 0 ) ? sName : "<unconnected>" );	// (Counter-Strike 1.1: a title; 1.0b: UTIL_VarArgs( "%s connected\n", ... ))
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

	// one line in 0.66 seconds (up to Beta 6.5: one a second).
	// (Counter-Strike 1.4: the parentheses that 1.1 to 1.3 needed around the difference
	// make no difference under this compiler: 1,703 bytes with and without)
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
	// Counter-Strike 1.3: a spectator's text is marked so and goes to spectators only
	if ( player->m_iTeam == TEAM_SPECTATOR )
	{
		sprintf( text, "%c(Spectator) %s :   ", 2, STRING( pEntity->v.netname ) );
	}
	else if ( teamonly == 1 )
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

		// can the receiver hear the sender? or has he muted him?
		// (Counter-Strike 1.4, from SDK 2.3; the voice manager is the rules' member)
		if ( gpGlobals->deathmatch != 0 && ((CHalfLifeMultiplay *)g_pGameRules)->m_VoiceGameMgr.PlayerHasBlockedPlayer( client, player ) )
			continue;

		if ( teamonly && player->m_iTeam != TEAM_SPECTATOR && g_pGameRules->PlayerRelationship(client, CBaseEntity::Instance(pEntity)) != GR_TEAMMATE )
			continue;

		// Counter-Strike 1.3: spectators talk among themselves
		// Counter-Strike 1.4: an HLTV proxy is heard by everybody
		if ( player->m_iTeam == TEAM_SPECTATOR && client->m_iTeam != TEAM_SPECTATOR && !( player->pev->flags & FL_PROXY ) )
			continue;

		// the living hear the living, the dead hear the dead; ignoremessages 1
		// lets only team mates through, 2 nobody
		if ( (client->pev->deadflag == DEAD_NO && bSenderDead != true) || (client->pev->deadflag != DEAD_NO && bSenderDead == true) || ( player->pev->flags & FL_PROXY ) )
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

	// Beta 6.5: and to the log, if the server wants it.
	// Counter-Strike 1.1: SDK 2.2's log line, with the team's name in the place of
	// the model (1.0b: UTIL_LogPrintf( "%s", text )). temp and szTeam are
	// Counter-Strike 1.6's names
	if ( CVAR_GET_FLOAT( "mp_logmessages" ) != 0 )
	{
		// (the three in Counter-Strike 1.6's order; temp set by SDK 2.2's if/else: as a
		// conditional expression the function's GetClassPtr is expanded one level less, 1,730 bytes)
		char *temp;
		char *szTeam;
		char *deadText = "";	// Counter-Strike 1.3

		if ( teamonly == 1 )
			temp = "say_team";
		else
			temp = "say";

		// Counter-Strike 1.3: the team's name from GetTeam (1.1 had the names here,
		// with UNASSIGNED for no team), and " (dead)" behind a dead player's text
		szTeam = GetTeam( player->m_iTeam );

		if ( player->m_iTeam != TEAM_SPECTATOR && bSenderDead == true )
			deadText = " (dead)";

		UTIL_LogPrintf( "\"%s<%i><%s><%s>\" %s \"%s\"%s\n", 
			STRING( player->pev->netname ), 
			GETPLAYERUSERID( player->edict() ),
			GETPLAYERAUTHID( player->edict() ),
			szTeam,
			temp,
			p,
			deadText );
	}
}


// Beta 5.0: DropItem (Beta 1.x's, unused since Beta 2.0) is gone

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
			ClientPrint( pPlayer->pev, HUD_PRINTCENTER, "#Already_Have_Kevlar" );
			return;
		}
		if ( pPlayer->m_iAccount < 650 )
		{
			ClientPrint( pPlayer->pev, HUD_PRINTCENTER, "#Not_Enough_Money" );
			BlinkAccount( pPlayer, 2 );
			return;
		}
		// (Counter-Strike 1.4: the helmet is kept and he is told so; the item plays the sound)
		if ( pPlayer->m_iKevlar == 2 )
			ClientPrint( pPlayer->pev, HUD_PRINTCENTER, "#Already_Have_Helmet_Bought_Kevlar" );
		pPlayer->GiveNamedItem( "item_kevlar" );
		pPlayer->AddAccount( -650, true );
		break;
	case 2:		// kevlar and helmet, 1,000
		{
			// Counter-Strike 1.4: who has one of the two pays for the other only, helmet 350,
			// kevlar 650 (the locals' names are Counter-Strike 1.6's)
			int fullArmor = ( pPlayer->pev->armorvalue >= 100 );
			int helmet = ( pPlayer->m_iKevlar == 2 );

			int price = 0, enoughMoney = 0;

			if ( fullArmor && helmet )
			{
				ClientPrint( pPlayer->pev, HUD_PRINTCENTER, "#Already_Have_Kevlar_Helmet" );
				return;
			}
			else if ( fullArmor && !helmet && pPlayer->m_iAccount >= 350 )
			{
				enoughMoney = 1;
				price = 350;
				ClientPrint( pPlayer->pev, HUD_PRINTCENTER, "#Already_Have_Kevlar_Bought_Helmet" );
			}
			else if ( !fullArmor && helmet && pPlayer->m_iAccount >= 650 )
			{
				enoughMoney = 1;
				price = 650;
				ClientPrint( pPlayer->pev, HUD_PRINTCENTER, "#Already_Have_Helmet_Bought_Kevlar" );
			}
			else if ( pPlayer->m_iAccount >= 1000 )
			{
				enoughMoney = 1;
				price = 1000;
			}

			if ( !enoughMoney )
			{
				ClientPrint( pPlayer->pev, HUD_PRINTCENTER, "#Not_Enough_Money" );
				BlinkAccount( pPlayer, 2 );
			}
			else
			{
				pPlayer->GiveNamedItem( "item_assaultsuit" );
				pPlayer->AddAccount( -price, true );
			}
		}
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
				pPlayer->HintMessage( "#Hint_use_nightvision", FALSE, FALSE );
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
	CHalfLifeMultiplay *mp = (CHalfLifeMultiplay *)g_pGameRules;	// (1.6's names)
	int team;
	edict_t *pentSpawnSpot;

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
	case 6:
		// Counter-Strike 1.1: the spectators, when allow_spectators is set; a living
		// player only during the freeze time. He loses his items and his money, his
		// score is sent as zero and he becomes an observer at a spawn point
		// Counter-Strike 1.3: the HLTV proxy may always
		if ( allow_spectators.value == 0 && !( player->pev->flags & FL_PROXY ) )
		{
			ClientPrint( player->pev, HUD_PRINTCENTER, "#Cannot_Be_Spectator" );
			CLIENT_COMMAND( player->edict(), "slot10\n" );
			return FALSE;
		}

		{
			if ( !g_pGameRules->IsFreezePeriod() && player->pev->deadflag == DEAD_NO )
			{
				ClientPrint( player->pev, HUD_PRINTCENTER, "#Cannot_Be_Spectator" );
				CLIENT_COMMAND( player->edict(), "slot10\n" );
				return FALSE;
			}

			if ( player->m_iTeam != TEAM_UNASSIGNED && player->pev->deadflag == DEAD_NO )
				ClientKill( player->edict() );

			player->RemoveAllItems( TRUE );
			player->m_bHasC4 = false;

			// Counter-Strike 1.3: logged
			if ( player->m_iTeam != TEAM_SPECTATOR )
			{
				// (GetTeam is called ahead of the others: a local; szOldTeam as below)
				char *szOldTeam = GetTeam( player->m_iTeam );

				UTIL_LogPrintf( "\"%s<%i><%s><%s>\" joined team \"SPECTATOR\"\n", 
					STRING( player->pev->netname ), 
					GETPLAYERUSERID( player->edict() ),
					GETPLAYERAUTHID( player->edict() ),
					szOldTeam );
			}
			player->m_iTeam = TEAM_SPECTATOR;
			player->m_iJoiningState = JOIN_DONE;	// Counter-Strike 1.3 (1.1: JOIN_CHOOSING)
			player->pev->frags = 0;
			player->m_iAccount = 0;

			MESSAGE_BEGIN( MSG_ONE, gmsgMoney, NULL, player->edict() );
				WRITE_LONG( player->m_iAccount );
				WRITE_BYTE( 0 );
			MESSAGE_END();

			MESSAGE_BEGIN( MSG_BROADCAST, gmsgScoreInfo );
				WRITE_BYTE( ENTINDEX( player->edict() ) );
				WRITE_SHORT( player->pev->frags );
				WRITE_SHORT( player->m_iDeaths );
				WRITE_SHORT( 0 );
				WRITE_SHORT( 0 );
			MESSAGE_END();

			player->m_pIntroCamera = NULL;
			TeamChangeUpdate( player, -1 );

			pentSpawnSpot = g_pGameRules->GetPlayerSpawnSpot( player );
			player->StartObserver( VARS( pentSpawnSpot )->origin, VARS( pentSpawnSpot )->angles );
			player->Observer_SetMode( OBS_ROAMING );	// Counter-Strike 1.4

			MESSAGE_BEGIN( MSG_ALL, gmsgSpectator );
				WRITE_BYTE( ENTINDEX( player->edict() ) );
				WRITE_BYTE( 1 );
			MESSAGE_END();
			return TRUE;
		}
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

	// Counter-Strike 1.3: a spectator who joins a team is set up as ClientPutInServer
	// sets up a new player (JoiningThink spawns him)
	if ( player->m_iTeam == TEAM_SPECTATOR && team != TEAM_SPECTATOR )
	{
		player->m_fNotKilled = true;
		player->m_iIgnoreMessages = 0;
		player->m_iTeamKills = 0;

		CheckStartMoney();
		player->m_iAccount = (int)startmoney.value;

		player->pev->solid = SOLID_NOT;
		player->pev->movetype = MOVETYPE_NOCLIP;
		player->pev->effects = EF_NODRAW;
		player->pev->effects |= EF_NOINTERP;
		player->pev->takedamage = DAMAGE_NO;
		player->pev->deadflag = DEAD_DEAD;
		player->pev->velocity = g_vecZero;
		player->pev->punchangle = g_vecZero;

		player->m_bHasNightVision = false;
		player->m_iHostagesKilled = 0;
		player->m_fDeadTime = 0;	// Counter-Strike 1.4 (1.3: m_iMapVote)
		player->has_disconnected = false;
		player->m_iJoiningState = JOIN_SPAWN;

		g_engfuncs.pfnSetClientMaxspeed( ENT(player->pev), 1 );
		SET_MODEL( ENT(player->pev), "models/player.mdl" );
	}

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

	// Counter-Strike 1.3: the team is set here, behind the menu and the kill (1.1: ahead of them)
	int oldTeam = player->m_iTeam;	// for the log (1.6's name)
	player->m_iTeam = team;

	TeamChangeUpdate( player, -1 );

	// Counter-Strike 1.1: titles of the client with the name (1.0b: "%s is joining the
	// Terrorist force\n" and "... Counter-Terrorist force\n" through UTIL_VarArgs, the
	// nameless "unconnected")
	UTIL_ClientPrintAll( HUD_PRINTNOTIFY, ( team == TEAM_TERRORIST ) ? "#Game_join_terrorist" : "#Game_join_ct",
		( player->pev->netname && STRING(player->pev->netname)[0] != 0 ) ? STRING(player->pev->netname) : "<unconnected>" );

	// Beta 6.6: the server's log. Counter-Strike 1.1: the standard log format's line,
	// with the team he leaves (1.0b: "%s is joining the TERRORIST force\n" or "... CT force\n").
	// szOldTeam and szNewTeam are 1.6's names
	char *szOldTeam = GetTeam( oldTeam );
	char *szNewTeam = GetTeam( team );	// Counter-Strike 1.3 (1.1 had the names here)

	UTIL_LogPrintf( "\"%s<%i><%s><%s>\" joined team \"%s\"\n", 
		STRING( player->pev->netname ), 
		GETPLAYERUSERID( player->edict() ),
		GETPLAYERAUTHID( player->edict() ),
		szOldTeam,
		szNewTeam );

	return TRUE;
}

//=========================================================
// The radio menus. Beta 5.0 (leak): three menus (titles.txt's
// RadioA to RadioC); each message is a sentence and a text
// (CBasePlayer::Radio). At most one a second and a half, 60 a
// life.
// Counter-Strike 1.1: the texts are titles of the client
// ("#Cover_me"; 1.0b: the words themselves, "Cover me!\n"),
// and so is the night vision goggles' hint in BuyItem
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
			player->Radio( "%!MRAD_COVERME", "#Cover_me" );
			break;
		case 2:
			player->Radio( "%!MRAD_TAKEPOINT", "#You_take_the_point" );
			break;
		case 3:
			player->Radio( "%!MRAD_POSITION", "#Hold_this_position" );
			break;
		case 4:		// Beta 6.0 (was "Circle Around Back.")
			player->Radio( "%!MRAD_REGROUP", "#Regroup_team" );
			break;
		case 5:
			player->Radio( "%!MRAD_FOLLOWME", "#Follow_me" );
			break;
		case 6:		// Beta 6.0 (was "Meet at the Rendezvous Point.")
			player->Radio( "%!MRAD_HITASSIST", "#Taking_fire" );
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
			player->Radio( "%!MRAD_GO", "#Go_go_go" );
			break;
		case 2:
			player->Radio( "%!MRAD_FALLBACK", "#Team_fall_back" );
			break;
		case 3:
			player->Radio( "%!MRAD_STICKTOG", "#Stick_together_team" );
			break;
		case 4:
			player->Radio( "%!MRAD_GETINPOS", "#Get_in_position_and_wait" );
			break;
		case 5:
			player->Radio( "%!MRAD_STORMFRONT", "#Storm_the_front" );
			break;
		case 6:
			player->Radio( "%!MRAD_REPORTIN", "#Report_in_team" );
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
				player->Radio( "%!MRAD_AFFIRM", "#Affirmative" );
			else
				player->Radio( "%!MRAD_ROGER", "#Roger_that" );
			break;
		case 2:
			player->Radio( "%!MRAD_ENEMYSPOT", "#Enemy_spotted" );
			break;
		case 3:
			player->Radio( "%!MRAD_BACKUP", "#Need_backup" );
			break;
		case 4:
			player->Radio( "%!MRAD_CLEAR", "#Sector_clear" );
			break;
		case 5:
			player->Radio( "%!MRAD_INPOS", "#In_position" );
			break;
		case 6:
			player->Radio( "%!MRAD_REPRTINGIN", "#Reporting_in" );
			break;
		case 7:
			player->Radio( "%!MRAD_BLOW", "#Get_out_of_there" );
			break;
		case 8:
			player->Radio( "%!MRAD_NEGATIVE", "#Negative" );
			break;
		case 9:		// Beta 6.0
			player->Radio( "%!MRAD_ENEMYDOWN", "#Enemy_down" );
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

	// Every gun has a case of its own (Counter-Strike 1.6's lines show 24 blocks five lines
	// apart): the compiler merges the equal ones, and the jump table is a direct one (with
	// shared labels it is the two-level form, 430 bytes for 468). The Elites' block stands
	// behind the 5.7 mm guns: 1.4's compiler keeps the last of equal blocks (9 mm behind
	// 5.7 mm in retail), 1.3's the first (9 mm second there); this text gives both
	switch ( weapon.m_iId )
	{
	case WEAPON_AWP:
		cost = 125;
		classname = "ammo_338magnum";
		break;
	case WEAPON_GLOCK18:
		cost = 20;
		classname = "ammo_9mm";
		break;
	case WEAPON_MP5N:
		cost = 20;
		classname = "ammo_9mm";
		break;
	case WEAPON_TMP:
		cost = 20;
		classname = "ammo_9mm";
		break;
	case WEAPON_M249:
		cost = 60;
		classname = "ammo_556natobox";
		break;
	case WEAPON_AUG:
		cost = 60;
		classname = "ammo_556nato";
		break;
	case WEAPON_M4A1:
		cost = 60;
		classname = "ammo_556nato";
		break;
	case WEAPON_SG552:
		cost = 60;
		classname = "ammo_556nato";
		break;
	case WEAPON_SG550:
		cost = 60;
		classname = "ammo_556nato";
		break;
	case WEAPON_SCOUT:
		cost = 80;
		classname = "ammo_762nato";
		break;
	case WEAPON_G3SG1:
		cost = 80;
		classname = "ammo_762nato";
		break;
	case WEAPON_AK47:
		cost = 80;
		classname = "ammo_762nato";
		break;
	case WEAPON_M3:
		cost = 65;
		classname = "ammo_buckshot";
		break;
	case WEAPON_XM1014:
		cost = 65;
		classname = "ammo_buckshot";
		break;
	case WEAPON_MAC10:
		cost = 25;
		classname = "ammo_45acp";
		break;
	case WEAPON_USP:
		cost = 25;
		classname = "ammo_45acp";
		break;
	case WEAPON_UMP45:
		cost = 25;
		classname = "ammo_45acp";
		break;
	case WEAPON_P90:
		cost = 50;
		classname = "ammo_57mm";
		break;
	case WEAPON_FIVESEVEN:
		cost = 50;
		classname = "ammo_57mm";
		break;
	case WEAPON_ELITE:
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

//=========================================================
// EntityFromUserID - Counter-Strike 1.4 (Linux symbol; the
// names are Counter-Strike 1.6's): the player in a team with
// that user id (the vote command). Windows: expanded in
// ClientCommand, no copy in retail
//=========================================================
CBaseEntity *EntityFromUserID( int userID )
{
	CBaseEntity *pTempEntity = NULL;
	CBasePlayer *pTempPlayer;

	pTempEntity = UTIL_FindEntityByClassname( pTempEntity, "player" );
	while ( pTempEntity != NULL && !FNullEnt( pTempEntity->edict() ) )
	{
		pTempPlayer = GetClassPtr( (CBasePlayer *)pTempEntity->pev );

		if ( pTempPlayer->m_iTeam != TEAM_UNASSIGNED && userID == GETPLAYERUSERID( pTempEntity->edict() ) )
			return pTempEntity;

		pTempEntity = UTIL_FindEntityByClassname( pTempEntity, "player" );
	}

	return NULL;
}

//=========================================================
// CountPlayersInServer - Counter-Strike 1.4 (Linux symbol;
// the names are Counter-Strike 1.6's): the players in a team
// (the votemap command). Windows: expanded in ClientCommand
//=========================================================
int CountPlayersInServer( void )
{
	int count = 0;
	CBaseEntity *pTempEntity = NULL;
	CBasePlayer *pTempPlayer;

	pTempEntity = UTIL_FindEntityByClassname( pTempEntity, "player" );
	while ( pTempEntity != NULL && !FNullEnt( pTempEntity->edict() ) )
	{
		pTempPlayer = GetClassPtr( (CBasePlayer *)pTempEntity->pev );

		if ( pTempPlayer->m_iTeam != TEAM_UNASSIGNED )
			count++;

		pTempEntity = UTIL_FindEntityByClassname( pTempEntity, "player" );
	}

	return count;
}

/*
===========
ClientCommand
called each time a player uses a "cmd" command
============
*/
extern float g_flWeaponCheat;
extern int GetMapCount( void );	// multiplay_gamerules.cpp (Counter-Strike 1.4)
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
	CBasePlayer *player = GetClassPtr((CBasePlayer *)pev);	// (Counter-Strike 1.6's name, as slot below)

	// (Counter-Strike 1.4: the kicked player's flag is gone)

	if ( FStrEq(pcmd, "say" ) )
	{
		Host_Say( pEntity, 0 );
	}
	else if ( FStrEq(pcmd, "say_team" ) )
	{
		Host_Say( pEntity, 1 );
	}
	else if ( FStrEq(pcmd, "fullupdate" ) )
	{
		// Counter-Strike 1.4 (SDK 2.3)
		player->ForceClientDllUpdate(); 
	}
	else if ( FStrEq(pcmd, "vote" ) )
	{
		// Beta 6.0: Beta 4.0's vote kick again (gone in Beta 5.0): a vote every
		// 3 seconds at most (Beta 6.5: he is told so), against a user id.
		// Counter-Strike 1.4: rewritten (the locals' names are Counter-Strike 1.6's):
		// only a player in a team of three or more votes, against a team mate other
		// than himself; the count is ProcessKickVote's. Beta 6.1's a12 command, which
		// made a vote kick at once, is gone
		if ( player->m_flNextVoteTime > gpGlobals->time )
		{
			ClientPrint( player->pev, HUD_PRINTCONSOLE, "#Wait_3_Seconds" );
			return;
		}

		player->m_flNextVoteTime = gpGlobals->time + 3;

		if ( player->m_iTeam == TEAM_UNASSIGNED )
			return;

		int iVoteID;
		int iVoteFail = 0;
		int iNumArgs = CMD_ARGC();
		const char *pszArg1 = CMD_ARGV(1);
		int iVoteLength = strlen( pszArg1 );

		if ( iNumArgs != 2 || iVoteLength <= 0 || iVoteLength > 6 )
			iVoteFail = 1;
		else
		{
			iVoteID = atoi( pszArg1 );
			if ( iVoteID <= 0 )
				iVoteFail = 1;
		}

		if ( iVoteFail )
		{
			ListPlayers( player );
			ClientPrint( player->pev, HUD_PRINTCONSOLE, "#Game_vote_usage" );
			return;
		}

		if ( CountTeamPlayers( player->m_iTeam ) < 3 )
		{
			ClientPrint( player->pev, HUD_PRINTCONSOLE, "#Cannot_Vote_With_Less_Than_Three" );
			return;
		}

		CBaseEntity *pKickEntity = EntityFromUserID( iVoteID );

		if ( pKickEntity == NULL )
		{
			ListPlayers( player );
			ClientPrint( player->pev, HUD_PRINTCONSOLE, "#Game_vote_player_not_found", UTIL_dtos1( iVoteID ) );
			return;
		}

		CBasePlayer *pKickPlayer = GetClassPtr( (CBasePlayer *)pKickEntity->pev );

		if ( pKickPlayer->m_iTeam != player->m_iTeam )
		{
			ClientPrint( player->pev, HUD_PRINTCONSOLE, "#Game_vote_players_on_your_team" );
			return;
		}

		if ( pKickPlayer == player )
		{
			ClientPrint( player->pev, HUD_PRINTCONSOLE, "#Game_vote_not_yourself" );
			return;
		}

		// (Counter-Strike 1.1: a title with the number; 1.4: to his console only)
		ClientPrint( player->pev, HUD_PRINTCONSOLE, "#Game_vote_cast", UTIL_dtos1( iVoteID ) );

		player->m_iCurrentKickVote = iVoteID;
		ProcessKickVote( player, pKickPlayer );
	}
	else if ( FStrEq(pcmd, "listmaps" ) )
	{
		// Beta 6.5: the maps of the cycle, numbered, to his console
		mp->DisplayMaps( player, 0 );
	}
	else if ( FStrEq(pcmd, "votemap" ) )
	{
		// Beta 6.5: a vote for map #n of listmaps, every 3 seconds at most;
		// his vote before is taken back.
		// Counter-Strike 1.4: rewritten (iFailed, iVote: Counter-Strike 1.6's names): only
		// a player in a team, not in the map's first three minutes, a number of the
		// list, two players at least; told to his console only
		int iFailed = 0;

		if ( player->m_flNextVoteTime > gpGlobals->time )
		{
			ClientPrint( player->pev, HUD_PRINTCONSOLE, "#Wait_3_Seconds" );
			return;
		}

		player->m_flNextVoteTime = gpGlobals->time + 3;

		if ( player->m_iTeam == TEAM_UNASSIGNED )
			return;

		if ( gpGlobals->time < 180 )
		{
			ClientPrint( player->pev, HUD_PRINTCONSOLE, "#Cannot_Vote_Map" );
			return;
		}

		if ( CMD_ARGC() != 2 || strlen( CMD_ARGV(1) ) > 5 )
			iFailed = 1;

		int iVote = atoi( CMD_ARGV(1) );

		if ( iVote < 1 || iVote > 100 )
			iFailed = 1;

		if ( iVote > GetMapCount() )
			iFailed = 1;

		if ( iFailed )
		{
			mp->DisplayMaps( player, 0 );
			ClientPrint( player->pev, HUD_PRINTCONSOLE, "#Game_votemap_usage" );
			return;
		}

		if ( CountPlayersInServer() < 2 )
		{
			ClientPrint( player->pev, HUD_PRINTCONSOLE, "#Cannot_Vote_Need_More_People" );
			return;
		}

		if ( player->m_iMapVote )
		{
			mp->m_iMapVotes[ player->m_iMapVote - 1 ]--;
			if ( mp->m_iMapVotes[ player->m_iMapVote - 1 ] < 0 )
				mp->m_iMapVotes[ player->m_iMapVote - 1 ] = 0;
		}

		// (Counter-Strike 1.1: a title; 1.0b: "%s voted for Map #%i\n")
		ClientPrint( player->pev, HUD_PRINTCONSOLE, "#Game_voted_for_map", UTIL_dtos1( iVote ) );

		player->m_iMapVote = iVote;
		mp->ProcessMapVote( player, iVote );
	}
	else if ( FStrEq(pcmd, "timeleft" ) )
	{
		// Beta 6.6: the time left on the map, to his chat (Beta 6.5's hidden
		// f17 command, which stood here, and its give command are gone)
		// Counter-Strike 1.1: counted to g_flTimeLimit, which CHalfLifeMultiplay::Think and
		// RestartRound work out from the last complete restart (1.0b: to timelimit.value * 60);
		// a title of the client with the minutes and the two-digit seconds, and another one
		// when there is no time limit (1.0b: "Time Remaining:  %i:%.2i\n"). secs and temp
		// are Counter-Strike 1.6's names
		if ( timelimit.value != 0 )
		{
			int iTimeRemaining = (int)( g_flTimeLimit - gpGlobals->time );

			if ( iTimeRemaining < 0 )	// Beta 7.0
				iTimeRemaining = 0;

			int iMinutes = iTimeRemaining / 60;
			int iSeconds = iTimeRemaining % 60;
			char secs[5];
			char *temp = UTIL_dtos2( iSeconds );

			if ( iSeconds < 10 )
			{
				secs[0] = '0';
				secs[1] = temp[0];
				secs[2] = 0;
			}
			else
			{
				secs[0] = temp[0];
				secs[1] = temp[1];
				secs[2] = 0;
			}

			ClientPrint( player->pev, HUD_PRINTTALK, "#Game_timelimit", UTIL_dtos1( iMinutes ), secs );
		}
		else
			ClientPrint( player->pev, HUD_PRINTTALK, "#Game_no_timelimit" );
	}
	else if ( FStrEq(pcmd, "listplayers" ) )
	{
		ListPlayers( player );
	}
	else if ( FStrEq(pcmd, "menuselect" ) )
	{
		int slot = atoi( CMD_ARGV(1) );

		// Beta 5.0 (leak): key 0 closes the menu
		if ( slot == 10 )
			player->m_iMenu = Menu_OFF;

		switch ( player->m_iMenu )
		{
		case Menu_OFF:
			break;

		case Menu_ChooseTeam:
			if ( !HandleMenu_ChooseTeam( player, slot ) )
			{
				// Beta 5.0: the menu again, keys 1, 2 and 5; in the game with
				// 0 to close it (the leak: 1 to 3 either way)
				if ( player->m_iJoiningState != JOIN_DONE )
					ShowVGUIMenu( player, 2, 0x13, "#Team_Select" );
				else
					ShowVGUIMenu( player, 2, 0x213, "#IG_Team_Select" );
				player->m_iMenu = Menu_ChooseTeam;
			}
			break;

		case Menu_ChooseTeam2:
			HandleMenu_ChooseTeam( player, slot );
			break;

		case Menu_ChooseAppearance:
			HandleMenu_ChooseAppearance( player, slot );
			break;

		case Menu_Buy:
			switch ( slot )
			{
			case 1:
				// Beta 7.0: a menu per team (the terrorists' has key 5, the Elites);
				// the buy menus are VGUI menus 28 to 34 for a client that draws them
				if ( player->m_iTeam == TEAM_CT )
					ShowVGUIMenu( player, 29, 0x22f, "#CT_BuyPistol" );	// Counter-Strike 1.0: key 6, the Five-seveN
				else
					ShowVGUIMenu( player, 29, 0x21f, "#T_BuyPistol" );
				player->m_iMenu = Menu_BuyPistol;
				break;
			case 2:
				// Beta 6.0: an assassination map's menus leave out what the
				// team may not buy (CanBuyThis)
				if ( mp->m_iMapHasVIPSafetyZone == 1 && player->m_iTeam == TEAM_TERRORIST )
					ShowVGUIMenu( player, 30, 0x200, "#AS_BuyShotgun" );
				else
					ShowVGUIMenu( player, 30, 0x203, "#BuyShotgun" );
				player->m_iMenu = Menu_BuyShotgun;
				break;
			case 3:
				if ( mp->m_iMapHasVIPSafetyZone == 1 )
				{
					if ( player->m_iTeam == TEAM_CT )
						ShowVGUIMenu( player, 32, 0x217, "#AS_CT_BuySubMachineGun" );
					else
						ShowVGUIMenu( player, 32, 0x218, "#AS_T_BuySubMachineGun" );
				}
				else
				{
					if ( player->m_iTeam == TEAM_CT )
						ShowVGUIMenu( player, 32, 0x217, "#CT_BuySubMachineGun" );
					else
						ShowVGUIMenu( player, 32, 0x21d, "#T_BuySubMachineGun" );
				}
				player->m_iMenu = Menu_BuySubMachineGun;
				break;
			case 4:
				if ( mp->m_iMapHasVIPSafetyZone == 1 )
				{
					if ( player->m_iTeam == TEAM_CT )
						ShowVGUIMenu( player, 31, 0x28c, "#AS_CT_BuyRifle" );
					else
						ShowVGUIMenu( player, 31, 0x231, "#AS_T_BuyRifle" );
				}
				else
				{
					if ( player->m_iTeam == TEAM_CT )
						ShowVGUIMenu( player, 31, 0x2bc, "#CT_BuyRifle" );
					else
						ShowVGUIMenu( player, 31, 0x273, "#T_BuyRifle" );
				}
				player->m_iMenu = Menu_BuyRifle;
				break;
			case 5:
				if ( mp->m_iMapHasVIPSafetyZone == 1 && player->m_iTeam == TEAM_TERRORIST )
					ShowVGUIMenu( player, 33, 0x200, "#AS_T_BuyMachineGun" );
				else
					ShowVGUIMenu( player, 33, 0x201, "#BuyMachineGun" );
				player->m_iMenu = Menu_BuyMachineGun;
				break;
			case 6:
				// primary ammunition, as much as the money allows
				if ( player->m_signals.GetState() & 1 )	// in a buy zone
				{
					if ( BuyAmmo( player, 1, true ) )
					{
						while ( BuyAmmo( player, 1, false ) )
							;
					}
				}
				break;
			case 7:
				// secondary ammunition, likewise
				if ( player->m_signals.GetState() & 1 )
				{
					if ( BuyAmmo( player, 2, true ) )
					{
						while ( BuyAmmo( player, 2, false ) )
							;
					}
				}
				break;
			case 8:
				// equipment (with the defuse kit for a CT on a bomb map)
				if ( player->m_signals.GetState() & 1 )
				{
					// Beta 5.0: on a bomb map both teams get a menu of their own
					// (the CTs' with the defuse kit); Beta 6.0: key 6, the goggles
					// Beta 6.5: seven items (the smoke grenade is 5, the defuse
					// kit 6, the goggles 7); only the CTs' bomb map menu has key 6
					if ( mp->m_fMapHasBombTarget == true )
					{
						if ( player->m_iTeam == TEAM_CT )
							ShowVGUIMenu( player, 34, 0x27f, "#DCT_BuyItem" );
						else
							ShowVGUIMenu( player, 34, 0x25f, "#DT_BuyItem" );
					}
					else
						ShowVGUIMenu( player, 34, 0x25f, "#BuyItem" );
					player->m_iMenu = Menu_BuyItem;
				}
				break;
			}
			break;

		case Menu_BuyPistol:
			BuyPistol( player, slot );
			break;

		case Menu_BuyShotgun:
			BuyShotgun( player, slot );
			break;

		case Menu_BuySubMachineGun:
			BuySubMachineGun( player, slot );
			break;

		case Menu_BuyRifle:
			BuyRifle( player, slot );
			break;

		case Menu_BuyMachineGun:
			BuyMachineGun( player, slot );
			break;

		case Menu_BuyItem:
			BuyItem( player, slot );
			break;

		case Menu_Radio1:
			Radio1( player, slot );
			break;

		case Menu_Radio2:
			Radio2( player, slot );
			break;

		case Menu_Radio3:
			Radio3( player, slot );
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
		if ( player->m_iMenu != Menu_ChooseAppearance )
		{
			if ( player->m_fTeamChanged && player->pev->deadflag != DEAD_NO )
			{
				ClientPrint( player->pev, HUD_PRINTCENTER, "#Only_1_Team_Change" );
				return;
			}

			// Beta 6.0: a counter-terrorist on an assassination map may queue
			// to be the VIP (key 3)
			// Counter-Strike 1.1: key 6, the spectators, for the dead and during the freeze time
			if ( mp->m_iMapHasVIPSafetyZone == 1 && player->m_iJoiningState == JOIN_DONE && player->m_iTeam == TEAM_CT )
			{
				if ( !((CHalfLifeMultiplay *)g_pGameRules)->IsFreezePeriod() && player->pev->deadflag == DEAD_NO )
					ShowVGUIMenu( player, 2, 0x217, "#IG_VIP_Team_Select" );
				else
					ShowVGUIMenu( player, 2, 0x237, "#IG_VIP_Team_Select_Spect" );

				player->m_iMenu = Menu_ChooseTeam;
				return;
			}

			if ( !((CHalfLifeMultiplay *)g_pGameRules)->IsFreezePeriod() && player->pev->deadflag == DEAD_NO )
				ShowVGUIMenu( player, 2, 0x213, "#IG_Team_Select" );
			else
				ShowVGUIMenu( player, 2, 0x233, "#IG_Team_Select_Spect" );

			player->m_iMenu = Menu_ChooseTeam;
		}
	}
	else if ( FStrEq(pcmd, "showbriefing" ) )
	{
		// Beta 5.0 (leak): the map's briefing again; Beta 5.0: for a player in a team
		// Counter-Strike 1.3: not for an observer
		// Counter-Strike 1.4: not over a menu
		if ( player->m_iMenu == Menu_OFF )
		{
			if ( g_szMapBriefingText[0] && player->m_iTeam != TEAM_UNASSIGNED && !( player->m_afPhysicsFlags & PFLAG_OBSERVER ) )
			{
				player->MenuPrint( player, g_szMapBriefingText );
				player->m_fShowingLevelText = true;
			}
		}
	}
	else if ( FStrEq(pcmd, "ignoremsg" ) )
	{
		// 0 -> 1 -> 2 -> 0 (Host_Say); Beta 5.0 (leak): ignoremsg (was ignoremessages)
		if ( player->m_iIgnoreMessages == 0 )
		{
			player->m_iIgnoreMessages = 1;
			ClientPrint( player->pev, HUD_PRINTCENTER, "#Ignore_Broadcast_Messages" );
		}
		else if ( player->m_iIgnoreMessages == 1 )
		{
			player->m_iIgnoreMessages = 2;
			ClientPrint( player->pev, HUD_PRINTCENTER, "#Ignore_Broadcast_Team_Messages" );
		}
		else
		{
			player->m_iIgnoreMessages = 0;
			ClientPrint( player->pev, HUD_PRINTCENTER, "#Accept_All_Messages" );
		}
	}
	else if ( FStrEq(pcmd, "ignorerad" ) )
	{
		// Beta 3.1: the radio on and off (CBasePlayer::Radio); Beta 5.0
		// (leak): ignorerad (was ignoreradio)
		if ( !player->m_fIgnoreRadio )
		{
			player->m_fIgnoreRadio = true;
			ClientPrint( player->pev, HUD_PRINTCENTER, "#Ignore_Radio" );
		}
		else
		{
			player->m_fIgnoreRadio = false;
			ClientPrint( player->pev, HUD_PRINTCENTER, "#Accept_Radio" );
		}
	}
	else if ( FStrEq(pcmd, "become_vip" ) )
	{
		// Beta 6.0: a counter-terrorist in the game queues to be the VIP
		if ( player->m_iJoiningState == JOIN_DONE && player->m_iTeam == TEAM_CT )
			mp->AddToVIPQueue( player );
		else
		{
			ClientPrint( player->pev, HUD_PRINTCENTER, "#Command_Not_Available" );
			return;	// (Counter-Strike 1.1's code needs this return: without it the block shares the tail of votemap's "#Wait_3_Seconds" block, which retail's does not; 1.4's compiler gives retail's 8,684 bytes with and without it)
		}
	}
	else if ( FStrEq(pcmd, "spectate" ) && ( player->pev->flags & FL_PROXY ) )
	{
		// Counter-Strike 1.3: the HLTV proxy joins the spectators
		HandleMenu_ChooseTeam( player, 6 );
	}
	else if ( FStrEq(pcmd, "specmode" ) )
	{
		// Counter-Strike 1.4: an observer's camera mode
		if ( player->pev->iuser1 != 0 )
			player->Observer_SetMode( atoi( CMD_ARGV(1) ) );
	}
	else if ( FStrEq(pcmd, "follownext" ) )
	{
		// Counter-Strike 1.4: an observer's next (0) or previous target
		if ( player->pev->iuser1 != 0 )
			player->Observer_FindNextPlayer( atoi( CMD_ARGV(1) ) ? true : false );
	}
	else if ( g_pGameRules->ClientCommand_DeadOrAlive( GetClassPtr((CBasePlayer *)pev), pcmd ) )
	{
		// Counter-Strike 1.3: the voice manager's commands, for the dead too
	}
	else if ( player->pev->deadflag != DEAD_NO )
	{
		// the dead have no other command
	}
	else if ( FStrEq(pcmd, "nightvision" ) )
	{
		// Beta 6.0: the goggles on and off, if bought (Beta 7.0: for the living only; the test stood before menuselect)
		if ( !player->m_bHasNightVision )
			return;

		if ( !player->m_bNightVisionOn )
		{
			EMIT_SOUND( ENT(player->pev), CHAN_ITEM, "items/nvg_on.wav", RANDOM_FLOAT( 0.92, 1 ), 0.8 );

			MESSAGE_BEGIN( MSG_ONE, gmsgNVGToggle, NULL, player->pev );
				WRITE_BYTE( 1 );
			MESSAGE_END();

			player->m_bNightVisionOn = true;	// Beta 6.5: Beta 6.1's lambert commands are gone
		}
		else
		{
			EMIT_SOUND( ENT(player->pev), CHAN_ITEM, "items/nvg_off.wav", RANDOM_FLOAT( 0.92, 1 ), 0.8 );

			MESSAGE_BEGIN( MSG_ONE, gmsgNVGToggle, NULL, player->pev );
				WRITE_BYTE( 0 );
			MESSAGE_END();

			player->m_bNightVisionOn = false;
		}
	}
	else if ( FStrEq(pcmd, "radio1" ) )
	{
		// Beta 5.0 (leak): three radio menus, open until a key is pressed
		ShowMenu( player, 0x23f, -1, FALSE, "#RadioA" );
		player->m_iMenu = Menu_Radio1;
	}
	else if ( FStrEq(pcmd, "radio2" ) )
	{
		ShowMenu( player, 0x23f, -1, FALSE, "#RadioB" );
		player->m_iMenu = Menu_Radio2;
	}
	else if ( FStrEq(pcmd, "radio3" ) )
	{
		ShowMenu( player, 0x3ff, -1, FALSE, "#RadioC" );	// Beta 6.0: key 9
		player->m_iMenu = Menu_Radio3;
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
		if ( player->m_signals.GetState() & 1 )
			BuyAmmo( player, 1, true );
	}
	else if ( FStrEq(pcmd, "buyammo2" ) )
	{
		if ( player->m_signals.GetState() & 1 )
			BuyAmmo( player, 2, true );
	}
	else if ( FStrEq(pcmd, "buyequip" ) )
	{
		// Beta 6.0: key 6 (the goggles) on a bomb map only; Beta 6.1: the
		// plain menu has key 6 too, as menuselect's ("can buy nightvision
		// using the 'buyequip' command")
		if ( player->m_signals.GetState() & 1 )
		{
			if ( mp->m_fMapHasBombTarget == true )
			{
				if ( player->m_iTeam == TEAM_CT )
					ShowVGUIMenu( player, 34, 0x27f, "#DCT_BuyItem" );
				else
					ShowVGUIMenu( player, 34, 0x25f, "#DT_BuyItem" );
			}
			else
				ShowVGUIMenu( player, 34, 0x25f, "#BuyItem" );
			player->m_iMenu = Menu_BuyItem;
		}
	}
	else if ( FStrEq(pcmd, "buy" ) )
	{
		if ( player->m_signals.GetState() & 1 )
		{
			ShowVGUIMenu( player, 28, 0x2ff, "#Buy" );
			player->m_iMenu = Menu_Buy;
		}
	}
	else if ( g_pGameRules->ClientCommand( GetClassPtr((CBasePlayer *)pev), pcmd ) )
	{
		// MenuSelect returns true only if the command is properly handled,  so don't print a warning
	}
	else
	{
		// tell the user they entered an unknown command
		// (Counter-Strike 1.1: SDK 2.2's text, the message a title of the client)
		char command[128];

		// check the length of the command (prevents crash)
		// max total length is 192 ...and we're adding a string below ("Unknown command: %s\n")
		strncpy( command, pcmd, 127 );
		command[127] = '\0';

		// tell the user they entered an unknown command
		ClientPrint( &pEntity->v, HUD_PRINTCONSOLE, "#Game_unknown_command", command );
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

	// (Counter-Strike 1.4: the three locals and szName, pPct below are Counter-Strike 1.6's names)
	CBasePlayer *pPlayer = (CBasePlayer *)GET_PRIVATE( pEntity );
	const char *szBufferName = g_engfuncs.pfnInfoKeyValue( infobuffer, "name" );
	int iClientIndex = pPlayer->entindex();

	// (Counter-Strike 1.0b: Beta 6.6's flag for CBasePlayer::VerifyModel is gone with that function)

	// msg everyone if someone changes their name,  and it isn't the first time (changing no name to current name)
	if ( pEntity->v.netname && STRING(pEntity->v.netname)[0] != 0 && !FStrEq( STRING(pEntity->v.netname), szBufferName ) )
	{
		// Counter-Strike 1.0b: the new name with every '%' turned into a
		// space, as in ClientPutInServer (Beta 6.6 had such a filter here,
		// Beta 7.0 to 1.0 none). Counter-Strike 1.4: 31 characters of it
		char szName[32];
		_snprintf( szName, sizeof( szName ), "%s", szBufferName );

		for ( char *pPct = szName; pPct != NULL && *pPct != 0; pPct++ )
		{
			if ( *pPct == '%' )
				*pPct = ' ';
		}

		// Beta 5.2: a dead player keeps his name (it is put back)
		// Counter-Strike 1.4: and gets the new one when he spawns (CBasePlayer::Spawn)
		if ( pPlayer->pev->deadflag != DEAD_NO )
		{
			pPlayer->m_bHasChangedName = true;
			_snprintf( pPlayer->m_szNewName, sizeof( pPlayer->m_szNewName ), "%s", szName );
			ClientPrint( pPlayer->pev, HUD_PRINTTALK, "#Name_change_at_respawn" );
			g_engfuncs.pfnSetClientKeyValue( iClientIndex, infobuffer, "name", (char *)STRING( pEntity->v.netname ) );
		}
		else
		{
			g_engfuncs.pfnSetClientKeyValue( iClientIndex, infobuffer, "name", szName );

			char text[256];
			sprintf( text, "* %s changed name to %s\n", STRING(pEntity->v.netname), szName );
			MESSAGE_BEGIN( MSG_BROADCAST, gmsgSayText, NULL );	// Beta 6.0: unreliable (was MSG_ALL)
				WRITE_BYTE( iClientIndex );
				WRITE_STRING( text );
			MESSAGE_END();

			// Counter-Strike 1.1: SDK 2.2's log line, with the team's name in the place of the model
			// (1.0b: "\"%s<%i>\" changed name to \"%s<%i>\"\n")
			UTIL_LogPrintf( "\"%s<%i><%s><%s>\" changed name to \"%s\"\n", 
				STRING( pEntity->v.netname ), 
				GETPLAYERUSERID( pEntity ), 
				GETPLAYERAUTHID( pEntity ),
				GetTeam( pPlayer->m_iTeam ), 
				szName );
		}
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
	CLocalNav::Reset();	// Counter-Strike 1.4: the hostages' queue of path requests
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

	// Counter-Strike 1.4: the rules look at the map (bomb targets, rescue and escape zones)
	if ( g_pGameRules )
		g_pGameRules->CheckMapConditions();
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

	CLocalNav::Think();	// Counter-Strike 1.4: the hostages' path requests

	// Counter-Strike 1.3: the skill cvar is looked up once (skill is Counter-Strike
	// 1.6's name for the static)
	static cvar_t *skill = NULL;
	if ( !skill )
		skill = CVAR_GET_POINTER( "skill" );

	gpGlobals->teamplay = 1.0;	// Beta 5.0 (leak): always teams

	if ( skill )
		g_iSkillLevel = (int)skill->value;
	else
		g_iSkillLevel = 0;

	g_ulFrameCount++;
}


// Counter-Strike 1.4 (the Linux library's name): the player models' files; the six behind the
// first four are marked with a '!' (ClientPrecache)
char *sPlayerModelFiles[] =
{
	"models/player.mdl",
	"models/player/leet/leet.mdl",
	"models/player/gign/gign.mdl",
	"models/player/vip/vip.mdl",
	"!models/player/gsg9/gsg9.mdl",
	"!models/player/guerilla/guerilla.mdl",
	"!models/player/arctic/arctic.mdl",
	"!models/player/sas/sas.mdl",
	"!models/player/terror/terror.mdl",
	"!models/player/urban/urban.mdl",
};

void ClientPrecache( void )
{
	int i;

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
	PRECACHE_SOUND("items/ammopickup2.wav");	// Counter-Strike 1.4
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

	// Counter-Strike 1.4: the player models from the table (the block of Beta 7.0 to 1.3
	// that only put their names, and scrambled copies, into the DLL is gone).
	// i (declared at the function's head) and numPlayerModels are Counter-Strike 1.6's
	int numPlayerModels = ARRAYSIZE( sPlayerModelFiles );

	for ( i = 0; i < numPlayerModels; i++ )
		PRECACHE_MODEL( sPlayerModelFiles[i] );

	// Counter-Strike 1.3: the weapons' player models, the planted C4 and the thrown
	// grenades (W_Precache had them from Beta 6.6 to 1.1)
	PRECACHE_MODEL("models/p_ak47.mdl");
	PRECACHE_MODEL("models/p_aug.mdl");
	PRECACHE_MODEL("models/p_awp.mdl");
	PRECACHE_MODEL("models/p_c4.mdl");
	PRECACHE_MODEL("models/w_c4.mdl");
	PRECACHE_MODEL("models/p_deagle.mdl");
	PRECACHE_MODEL("models/p_flashbang.mdl");
	PRECACHE_MODEL("models/w_flashbang.mdl");
	PRECACHE_MODEL("models/p_hegrenade.mdl");
	PRECACHE_MODEL("models/w_hegrenade.mdl");
	PRECACHE_MODEL("models/p_glock18.mdl");
	PRECACHE_MODEL("models/p_sg550.mdl");
	PRECACHE_MODEL("models/p_g3sg1.mdl");
	PRECACHE_MODEL("models/p_knife.mdl");
	PRECACHE_MODEL("models/p_m249.mdl");
	PRECACHE_MODEL("models/p_m3.mdl");
	PRECACHE_MODEL("models/p_m4a1.mdl");
	PRECACHE_MODEL("models/p_mac10.mdl");
	PRECACHE_MODEL("models/p_mp5.mdl");
	PRECACHE_MODEL("models/p_ump45.mdl");
	PRECACHE_MODEL("models/p_p228.mdl");
	PRECACHE_MODEL("models/p_p90.mdl");
	PRECACHE_MODEL("models/p_scout.mdl");
	PRECACHE_MODEL("models/p_sg552.mdl");
	PRECACHE_MODEL("models/p_smokegrenade.mdl");
	PRECACHE_MODEL("models/w_smokegrenade.mdl");
	PRECACHE_MODEL("models/p_tmp.mdl");
	PRECACHE_MODEL("models/p_usp.mdl");
	PRECACHE_MODEL("models/p_elite.mdl");
	PRECACHE_MODEL("models/p_fiveseven.mdl");
	PRECACHE_MODEL("models/p_xm1014.mdl");

	// temp is Counter-Strike 1.6's name (its debug information): a zero vector for
	// the sprites' force_exactfile calls below
	Vector temp = g_vecZero;

	// Beta 6.6: the player models are precached here, and the client's files must
	// keep within these bounds (vMin, vMax: Counter-Strike 1.6's names)
	Vector vMin( -38, -24, -41 );	// Counter-Strike 1.0 (Beta 7.1: 48, 36, 58); Counter-Strike 1.1: 24 wide (1.0b: 22)
	Vector vMax( 38, 24, 41 );

	// Counter-Strike 1.4: the first four as before; for the six with a '!' the engine's fourth
	// kind of check (3; SDK 2.3's FORCE_TYPE ends at force_model_specifybounds) on the name
	// behind the '!'
	for ( i = 0; i < 4; i++ )
		ENGINE_FORCE_UNMODIFIED( force_model_specifybounds, (float *)&vMin, (float *)&vMax, sPlayerModelFiles[i] );

	for ( i = 4; i < numPlayerModels; i++ )
		ENGINE_FORCE_UNMODIFIED( (FORCE_TYPE)3, (float *)&vMin, (float *)&vMax, sPlayerModelFiles[i] + 1 );

	// Beta 6.5: for some of the client's sprites the engine is told that the
	// client's file must be the server's (force_exactfile; the bounds are not
	// used for that)
	ENGINE_FORCE_UNMODIFIED( force_exactfile, (float *)&temp, (float *)&temp, "sprites/black_smoke1.spr" );
	ENGINE_FORCE_UNMODIFIED( force_exactfile, (float *)&temp, (float *)&temp, "sprites/black_smoke2.spr" );
	ENGINE_FORCE_UNMODIFIED( force_exactfile, (float *)&temp, (float *)&temp, "sprites/black_smoke3.spr" );
	ENGINE_FORCE_UNMODIFIED( force_exactfile, (float *)&temp, (float *)&temp, "sprites/black_smoke4.spr" );
	ENGINE_FORCE_UNMODIFIED( force_exactfile, (float *)&temp, (float *)&temp, "sprites/fast_wallpuff1.spr" );
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

	// Beta 6.6: bounds a client's file of each weapon model must keep within
	// (Counter-Strike 1.0: force_model_samebounds for the SG-550 and the UMP45)
	vMin = Vector( -17, -6, -19 );
	vMax = Vector( 17, 12, 29 );
	ENGINE_FORCE_UNMODIFIED( force_model_specifybounds, (float *)&vMin, (float *)&vMax, "models/p_ak47.mdl" );

	vMin = Vector( -23, -6, -18 );
	vMax = Vector( 23, 7, 18 );
	ENGINE_FORCE_UNMODIFIED( force_model_specifybounds, (float *)&vMin, (float *)&vMax, "models/p_aug.mdl" );

	vMin = Vector( -30, -24, -19 );
	vMax = Vector( 30, 17, 19 );
	ENGINE_FORCE_UNMODIFIED( force_model_specifybounds, (float *)&vMin, (float *)&vMax, "models/p_awp.mdl" );

	vMin = Vector( -7, -7, -15 );
	vMax = Vector( 7, 7, 15 );
	ENGINE_FORCE_UNMODIFIED( force_model_specifybounds, (float *)&vMin, (float *)&vMax, "models/p_c4.mdl" );

	vMin = Vector( -4, -8, -3 );
	vMax = Vector( 3, 7, 3 );
	ENGINE_FORCE_UNMODIFIED( force_model_specifybounds, (float *)&vMin, (float *)&vMax, "models/w_c4.mdl" );

	vMin = Vector( -8, -4, -19 );
	vMax = Vector( 8, 4, 19 );
	ENGINE_FORCE_UNMODIFIED( force_model_specifybounds, (float *)&vMin, (float *)&vMax, "models/p_deagle.mdl" );

	vMin = Vector( -7, -2, -18 );
	vMax = Vector( 7, 2, 18 );
	ENGINE_FORCE_UNMODIFIED( force_model_specifybounds, (float *)&vMin, (float *)&vMax, "models/p_flashbang.mdl" );

	vMin = Vector( -5, -5, -5 );
	vMax = Vector( 5, 5, 14 );
	ENGINE_FORCE_UNMODIFIED( force_model_specifybounds, (float *)&vMin, (float *)&vMax, "models/w_flashbang.mdl" );

	vMin = Vector( -31, -7, -19 );
	vMax = Vector( 31, 5, 27 );
	ENGINE_FORCE_UNMODIFIED( force_model_specifybounds, (float *)&vMin, (float *)&vMax, "models/p_g3sg1.mdl" );

	vMin = Vector( -33, -4, -20 );
	vMax = Vector( 33, 7, 30 );
	ENGINE_FORCE_UNMODIFIED( force_model_samebounds, (float *)&vMin, (float *)&vMax, "models/p_sg550.mdl" );

	vMin = Vector( -8, -5, -18 );
	vMax = Vector( 8, 4, 18 );
	ENGINE_FORCE_UNMODIFIED( force_model_specifybounds, (float *)&vMin, (float *)&vMax, "models/p_glock18.mdl" );

	vMin = Vector( -7, -2, -18 );
	vMax = Vector( 7, 2, 18 );
	ENGINE_FORCE_UNMODIFIED( force_model_specifybounds, (float *)&vMin, (float *)&vMax, "models/p_hegrenade.mdl" );

	vMin = Vector( -5, -5, -5 );
	vMax = Vector( 5, 5, 14 );
	ENGINE_FORCE_UNMODIFIED( force_model_specifybounds, (float *)&vMin, (float *)&vMax, "models/w_hegrenade.mdl" );

	vMin = Vector( -7, -7, -18 );
	vMax = Vector( 7, 5, 18 );
	ENGINE_FORCE_UNMODIFIED( force_model_specifybounds, (float *)&vMin, (float *)&vMax, "models/p_knife.mdl" );

	vMin = Vector( -29, -7, -18 );
	vMax = Vector( 29, 5, 18 );
	ENGINE_FORCE_UNMODIFIED( force_model_specifybounds, (float *)&vMin, (float *)&vMax, "models/p_m249.mdl" );

	vMin = Vector( -21, -13, -18 );
	vMax = Vector( 21, 23, 18 );
	ENGINE_FORCE_UNMODIFIED( force_model_specifybounds, (float *)&vMin, (float *)&vMax, "models/p_m3.mdl" );

	vMin = Vector( -22, -8, -19 );
	vMax = Vector( 22, 4, 25 );
	ENGINE_FORCE_UNMODIFIED( force_model_specifybounds, (float *)&vMin, (float *)&vMax, "models/p_m4a1.mdl" );

	vMin = Vector( -10, -6, -18 );
	vMax = Vector( 10, 4, 18 );
	ENGINE_FORCE_UNMODIFIED( force_model_specifybounds, (float *)&vMin, (float *)&vMax, "models/p_mac10.mdl" );

	vMin = Vector( -23, -8, -18 );
	vMax = Vector( 23, 3, 18 );
	ENGINE_FORCE_UNMODIFIED( force_model_specifybounds, (float *)&vMin, (float *)&vMax, "models/p_mp5.mdl" );

	vMin = Vector( -14, -2, -18 );
	vMax = Vector( 14, 2, 18 );
	ENGINE_FORCE_UNMODIFIED( force_model_samebounds, (float *)&vMin, (float *)&vMax, "models/p_ump45.mdl" );

	vMin = Vector( -10, -4, -18 );
	vMax = Vector( 10, 4, 18 );
	ENGINE_FORCE_UNMODIFIED( force_model_specifybounds, (float *)&vMin, (float *)&vMax, "models/p_p228.mdl" );

	vMin = Vector( -15, -4, -18 );
	vMax = Vector( 13, 6, 18 );
	ENGINE_FORCE_UNMODIFIED( force_model_specifybounds, (float *)&vMin, (float *)&vMax, "models/p_p90.mdl" );

	vMin = Vector( -32, -4, -19 );
	vMax = Vector( 32, 5, 19 );
	ENGINE_FORCE_UNMODIFIED( force_model_specifybounds, (float *)&vMin, (float *)&vMax, "models/p_scout.mdl" );

	vMin = Vector( -23, -5, -18 );
	vMax = Vector( 23, 7, 19 );
	ENGINE_FORCE_UNMODIFIED( force_model_specifybounds, (float *)&vMin, (float *)&vMax, "models/p_sg552.mdl" );

	vMin = Vector( -7, -2, -18 );
	vMax = Vector( 7, 2, 18 );
	ENGINE_FORCE_UNMODIFIED( force_model_specifybounds, (float *)&vMin, (float *)&vMax, "models/p_smokegrenade.mdl" );

	vMin = Vector( -5, -5, -5 );
	vMax = Vector( 5, 5, 14 );
	ENGINE_FORCE_UNMODIFIED( force_model_specifybounds, (float *)&vMin, (float *)&vMax, "models/w_smokegrenade.mdl" );

	vMin = Vector( -15, -4, -18 );
	vMax = Vector( 15, 6, 18 );
	ENGINE_FORCE_UNMODIFIED( force_model_specifybounds, (float *)&vMin, (float *)&vMax, "models/p_tmp.mdl" );

	vMin = Vector( -13, -4, -18 );
	vMax = Vector( 13, 3, 18 );
	ENGINE_FORCE_UNMODIFIED( force_model_specifybounds, (float *)&vMin, (float *)&vMax, "models/p_usp.mdl" );

	vMin = Vector( -10, -4, -20 );
	vMax = Vector( 10, 4, 20 );
	ENGINE_FORCE_UNMODIFIED( force_model_specifybounds, (float *)&vMin, (float *)&vMax, "models/p_elite.mdl" );

	vMin = Vector( -8, -3, -18 );
	vMax = Vector( 8, 3, 18 );
	ENGINE_FORCE_UNMODIFIED( force_model_specifybounds, (float *)&vMin, (float *)&vMax, "models/p_fiveseven.mdl" );

	vMin = Vector( -25, -19, -18 );
	vMax = Vector( 25, 16, 18 );
	ENGINE_FORCE_UNMODIFIED( force_model_specifybounds, (float *)&vMin, (float *)&vMax, "models/p_xm1014.mdl" );

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
		UTIL_PrecacheOther("enemy_terrorist");	// Counter-Strike 1.3 (1.1: the SDK's monster_human_grunt)

	// Beta 6.5: sprites for the client (wall puffs, smoke, and a set named after
	// the screen's sides and corners)
	PRECACHE_MODEL("sprites/wall_puff1.spr");
	PRECACHE_MODEL("sprites/wall_puff2.spr");
	PRECACHE_MODEL("sprites/wall_puff3.spr");
	PRECACHE_MODEL("sprites/wall_puff4.spr");
	PRECACHE_MODEL("sprites/black_smoke1.spr");
	PRECACHE_MODEL("sprites/black_smoke2.spr");
	PRECACHE_MODEL("sprites/black_smoke3.spr");
	PRECACHE_MODEL("sprites/black_smoke4.spr");
	PRECACHE_MODEL("sprites/fast_wallpuff1.spr");
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

	// Counter-Strike 1.3: the event that removes every decal at a round's start
	// (CleanUpMap plays it; the name is the Linux library's)
	m_usResetDecals = PRECACHE_EVENT( 1, "events/decal_reset.sc" );
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

	if ( pClient->v.flags & FL_PROXY )
	{
		*pvs = NULL;	// the spectator proxy sees
		*pas = NULL;	// and hears everything
		return;
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

// Counter-Strike 1.3: for each client, when each entity was last sent because it
// was in his PVS, and the leafs he was in then (AddToFullPack).  Written from
// retail's code; the types', members' and locals' names are Counter-Strike 1.6's
// (its debug information), g_PVSStatus is the Linux library's.
typedef struct
{
	float m_fTimeEnteredPVS;
} ENTITYPVSSTATUS;

typedef struct
{
	ENTITYPVSSTATUS m_Status[ 1380 ];

	int headnode;
	int num_leafs;
	short leafnums[ MAX_ENT_LEAFS ];
} PLAYERPVSSTATUS;

static PLAYERPVSSTATUS g_PVSStatus[ 32 ];

void ResetPlayerPVS( edict_t *client, int clientnum )
{
	PLAYERPVSSTATUS *pvs = &g_PVSStatus[ clientnum ];

	memset( pvs, 0, sizeof( *pvs ) );

	pvs->headnode = client->headnode;
	pvs->num_leafs = client->num_leafs;
	memcpy( pvs->leafnums, client->leafnums, sizeof( pvs->leafnums ) );
}

bool CheckPlayerPVSLeafChanged( edict_t *client, int clientnum )
{
	PLAYERPVSSTATUS *pvs = &g_PVSStatus[ clientnum ];

	if ( pvs->headnode != client->headnode || pvs->num_leafs != client->num_leafs )
		return true;

	if ( client->num_leafs > 0 )
	{
		for ( int i = 0; i < client->num_leafs; i++ )
		{
			if ( client->leafnums[ i ] != pvs->leafnums[ i ] )
				return true;
		}
	}

	return false;
}

void MarkEntityInPVS( int clientnum, int entitynum, float time, bool inpvs )
{
	PLAYERPVSSTATUS *pvs = &g_PVSStatus[ clientnum ];
	ENTITYPVSSTATUS *es = &pvs->m_Status[ entitynum ];

	if ( inpvs )
		es->m_fTimeEnteredPVS = time;
	else
		es->m_fTimeEnteredPVS = 0;
}

bool CheckEntityRecentlyInPVS( int clientnum, int entitynum, float currenttime )
{
	PLAYERPVSSTATUS *pvs = &g_PVSStatus[ clientnum ];
	ENTITYPVSSTATUS *es = &pvs->m_Status[ entitynum ];

	// never sent
	if ( es->m_fTimeEnteredPVS == 0 )
		return false;

	// sent more than a second ago
	if ( es->m_fTimeEnteredPVS + 1.0f < currenttime )
		return false;

	return true;
}

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

	// Counter-Strike 1.3: an entity that was sent to this client within the last
	// second is sent without asking the engine; the record is cleared when the
	// client's leafs change (hostnum is Counter-Strike 1.6's name)
	int hostnum = ENTINDEX( host ) - 1;

	if ( CheckPlayerPVSLeafChanged( host, hostnum ) )
		ResetPlayerPVS( host, hostnum );

	// Ignore if not the host and not touching a PVS/PAS leaf
	// If pSet is NULL, then the test will always succeed and the entity will be added to the update
	if ( ent != host )
	{
		if ( !CheckEntityRecentlyInPVS( hostnum, e, gpGlobals->time ) )
		{
			if ( !ENGINE_CHECK_VISIBILITY( (const struct edict_s *)ent, pSet ) )
			{
				MarkEntityInPVS( hostnum, e, gpGlobals->time, false );
				return 0;
			}

			MarkEntityInPVS( hostnum, e, gpGlobals->time, true );
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
						// Counter-Strike 1.4: the grenades' throw and the knife's swing (CBaseEntity's members)
						item->fuser2					= gun->m_flStartThrow;
						item->fuser3					= gun->m_flReleaseThrow;
						item->iuser1					= gun->m_iSwing;
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
	// Counter-Strike 1.4: an observer in the eyes of a player gets that player's data, but
	// for his own observer state (pevOrg is Counter-Strike 1.6's name)
	entvars_t *pev = (entvars_t *)&ent->v;
	CBasePlayer *pl = ( CBasePlayer *) CBasePlayer::Instance( pev );
	entvars_t *pevOrg = NULL;

	// if user is spectating different player in First person, override some vars
	if ( pl->pev->iuser1 == OBS_IN_EYE )
	{
		if ( pl->m_hObserverTarget )
		{
			pevOrg = pev;
			pev = pl->m_hObserverTarget->pev;
			pl = ( CBasePlayer *) CBasePlayer::Instance( pev );
		}
	}

	cd->flags			= pev->flags;
	cd->health			= pev->health;

	cd->viewmodel		= MODEL_INDEX( STRING( pev->viewmodel ) );
	cd->waterlevel		= pev->waterlevel;
	cd->watertype		= pev->watertype;
	cd->weapons			= pev->weapons;

	// Vectors
	cd->origin			= pev->origin;
	cd->velocity		= pev->velocity;
	cd->view_ofs		= pev->view_ofs;
	cd->punchangle		= pev->punchangle;

	cd->bInDuck			= pev->bInDuck;
	cd->flTimeStepSound = pev->flTimeStepSound;
	cd->flDuckTime		= pev->flDuckTime;
	cd->flSwimTime		= pev->flSwimTime;
	cd->waterjumptime	= pev->teleport_time;

	strcpy( cd->physinfo, ENGINE_GETPHYSINFO( ent ) );

	cd->maxspeed		= pev->maxspeed;
	cd->fov				= pev->fov;
	cd->weaponanim		= pev->weaponanim;

	cd->pushmsec		= pev->pushmsec;

	// Beta 6.5: the observer's camera mode and target
	// Counter-Strike 1.4: the observer's own, also iuser3
	if ( pevOrg )
	{
		cd->iuser1		= pevOrg->iuser1;
		cd->iuser2		= pevOrg->iuser2;
		cd->iuser3		= pevOrg->iuser3;
	}
	else
	{
		cd->iuser1		= pev->iuser1;
		cd->iuser2		= pev->iuser2;
		cd->iuser3		= pev->iuser3;
	}

	cd->fuser2			= pev->fuser2;	// Counter-Strike 1.4: the jump's stamina (pm_shared.c)

#if defined( CLIENT_WEAPONS )
	if ( sendweapons )
	{
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

			// Beta 7.1: the flags in iuser3 (to 7.0: iuser3 and iuser4)
			// Counter-Strike 1.4: bit 0 he may shoot and is not defusing, bit 1 the freeze
			// time, bit 2 in a bomb zone (1.3: bit 1 the left hand); only for a player
			// who is no observer
			int iUser3 = 0;		// Counter-Strike 1.6's name

			if ( pl->m_bCanShoot == true && !pl->m_bIsDefusing )
				iUser3 |= 1;
			else
				iUser3 &= ~1;

			if ( g_pGameRules->IsFreezePeriod() )
				iUser3 |= 2;
			else
				iUser3 &= ~2;

			if ( pl->m_signals.GetState() & 2 )
				iUser3 |= 4;
			else
				iUser3 &= ~4;

			if ( pl->pev->iuser1 == OBS_NONE && !pevOrg )
				cd->iuser3		= iUser3;

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

	// Counter-Strike 1.3: a ducking player gets the duck hull's size back after
	// every command
	if ( pl->pev->flags & FL_DUCKING )
		UTIL_SetSize( pl->pev, VEC_DUCK_HULL_MIN, VEC_DUCK_HULL_MAX );
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


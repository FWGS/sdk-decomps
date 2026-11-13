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
//
// teamplay_gamerules.cpp
//
#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"player.h"
#include	"trains.h"	// Beta 7.1: CFuncVehicle (PlayerKilled). The place is ours, player.cpp's: ahead of gamerules.h the object's inline order can be reached by more text in front of ClientDisconnected, behind it only by less
#include	"weapons.h"
#include	"gamerules.h"
#include	"skill.h"
#include	"game.h"
#include	"items.h"
#include	"hostage.h"
#include	"shake.h"	// Counter-Strike 1.3: FFADE_OUT (FPlayerCanRespawn); the place is ours

extern DLL_GLOBAL CGameRules	*g_pGameRules;
extern DLL_GLOBAL BOOL	g_fGameOver;
extern void respawn( entvars_t* pev, BOOL fCopyCorpse );
extern int gmsgDeathMsg;	// client dll messages
extern int gmsgScoreInfo;
extern int gmsgMOTD;
extern int gmsgServerName;
extern int gmsgTeamScore;
extern int gmsgSendAudio;
extern int gmsgTeamInfo;
extern int gmsgStatusIcon;
extern void ShowMenu( CBasePlayer *pPlayer, int bitsValidSlots, int nDisplayTime, BOOL fNeedMore, char *pszText );
extern void ShowVGUIMenu( CBasePlayer *pPlayer, int MenuType, int BitMask, char *szOldMenu );	// client.cpp (Beta 7.0)
extern int CountTeamPlayers( int iTeam );	// Beta 6.0 (client.cpp)

#define ITEM_RESPAWN_TIME	30
#define WEAPON_RESPAWN_TIME	20
#define AMMO_RESPAWN_TIME	20

// CMapInfo - Beta 6.0 (Linux symbols): info_map_parameters, the map's rules (RestartRound reads them). Member names ours
class CMapInfo : public CPointEntity
{
public:
	void KeyValue( KeyValueData *pkvd )
	{
		if ( FStrEq( pkvd->szKeyName, "buying" ) )
		{
			m_iBuyingStatus = atoi( pkvd->szValue );
			pkvd->fHandled = TRUE;
		}
		else if ( FStrEq( pkvd->szKeyName, "bombradius" ) )
		{
			m_flBombRadius = atoi( pkvd->szValue );

			if ( m_flBombRadius > 2048 )
				m_flBombRadius = 2048;

			pkvd->fHandled = TRUE;
		}
	}
	void Spawn( void )
	{
		pev->movetype = MOVETYPE_NONE;
		pev->solid = SOLID_NOT;
		pev->effects |= EF_NODRAW;
	}

	int m_iBuyingStatus;	// 0x7c "buying": 0 both teams may buy, 1 only the counter-terrorists, 2 only the terrorists, 3 nobody
	float m_flBombRadius;	// 0x80 "bombradius": at most 2048; 500 when the map has none
};

LINK_ENTITY_TO_CLASS( info_map_parameters, CMapInfo );

// CCStrikeGameMgrHelper - Counter-Strike 1.3 (Linux symbols; the SDK's CMultiplayGameMgrHelper): who hears whom
// over the voice channel. Only team mates, and the living do not hear the dead; an observer (pev->iuser1) and
// the dead hear the whole team. Parameter and local names are Counter-Strike 1.6's (1.6 asks IsObserver())
class CCStrikeGameMgrHelper : public IVoiceGameMgrHelper
{
public:
	virtual bool		CanPlayerHearPlayer(CBasePlayer *pListener, CBasePlayer *pSender)
	{
		if ( pListener->m_iTeam == pSender->m_iTeam )
		{
			BOOL bListenerAlive = pListener->IsAlive();
			BOOL bSenderAlive = pSender->IsAlive();

			if ( pListener->pev->iuser1 )
				return true;

			if ( bListenerAlive )
			{
				if ( !bSenderAlive )
					return false;
			}
			else
			{
				if ( bSenderAlive )
					return true;
			}

			return ( bListenerAlive == bSenderAlive );
		}

		return false;
	}
};
static CCStrikeGameMgrHelper g_GameMgrHelper;

// Broadcast - Beta 3.0: the radio sentence !MRAD_<sentence> to every client (Linux symbol)
void Broadcast( const char *sentence )
{
	char text[32];

	if ( !sentence )
		return;

	strcpy( text, "%!MRAD_" );
	strcat( text, UTIL_VarArgs( "%s", sentence ) );

	MESSAGE_BEGIN( MSG_BROADCAST, gmsgSendAudio );	// Beta 6.0: unreliable (was MSG_ALL)
		WRITE_BYTE( 0 );
		WRITE_STRING( text );
	MESSAGE_END();
}

// GetTeam - Counter-Strike 1.1: a team's name for the log lines. The name, the parameter's and the place
// are Counter-Strike 1.6's, where it is a function of its own; 1.1 has none in either library, only the
// switch in its callers. Counter-Strike 1.3: a function of its own here too
char *GetTeam( int teamNo )
{
	switch ( teamNo )
	{
	case TEAM_TERRORIST:
		return "TERRORIST";
	case TEAM_CT:
		return "CT";
	case TEAM_SPECTATOR:
		return "SPECTATOR";
	default:
		break;
	}

	return "";
}

// EndRoundMessage - Beta 5.0 (leak; Linux symbol): a message in the middle of everybody's screen.
// Beta 7.1: the HUD message's parameters (to 7.0 a static hudtextparms_t here: green, four seconds)
// are gone from the image with UTIL_HudMessageAll's call.
// Counter-Strike 1.1: the log lines are the standard log format's. A second parameter says which
// end it is (1.6's name; the numbers have no names there either): 1 target bombed, 2 VIP escaped,
// 3 VIP assassinated, 4 terrorists escaped, 5 CTs prevented the escape, 7 bomb defused, 8 CTs win,
// 9 terrorists win, 10 round draw, 11 all hostages rescued, 12 target saved, 13 hostages not
// rescued, 14 terrorists not escaped, 15 VIP not escaped (6 is not used). It picks the team that
// "triggered" the sentence; the sentence is logged without its '#'. Locals' names are 1.6's.

void EndRoundMessage( const char *sentence, int event )
{
	CHalfLifeMultiplay *mp = (CHalfLifeMultiplay *)g_pGameRules;	// Counter-Strike 1.3: ahead of team (1.6's order)
	char *team = NULL;
	const char *message = &sentence[1];
	int teamTriggered = 1;

	// Beta 7.1: the sentence is a name of the client's titles.txt ("#CTs_Win"), printed
	// in the middle of the screen (to 7.0: UTIL_HudMessageAll)
	UTIL_ClientPrintAll( HUD_PRINTCENTER, sentence );

	switch ( event )
	{
	case 1:
	case 3:
	case 4:
	case 9:
	case 13:
	case 15:
		team = "TERRORIST";
		break;

	case 2:
	case 5:
	case 6:
	case 7:
	case 8:
	case 11:
	case 12:
	case 14:
		team = "CT";
		break;

	default:
		teamTriggered = 0;
		break;
	}

	if ( mp )
	{
		if ( teamTriggered == 1 )
			UTIL_LogPrintf( "Team \"%s\" triggered \"%s\" (CT \"%i\") (T \"%i\")\n", team, message, mp->m_iNumCTWins, mp->m_iNumTerroristWins );
		else
			UTIL_LogPrintf( "World triggered \"%s\" (CT \"%i\") (T \"%i\")\n", message, mp->m_iNumCTWins, mp->m_iNumTerroristWins );
	}

	UTIL_LogPrintf( "World triggered \"Round_End\"\n" );
}

// ReadMultiplayCvars - Beta 5.0 (Linux symbol): the round time, C4 timer, freeze time and team limit from their cvars,
// each clamped and the cvar set back to the clamp. sic: a round time under 3 minutes becomes 1, a freeze time over 15
// seconds sets mp_roundtime to 9; a team limit under 1 (the cvar set to 0) means 20, no limit.
// Counter-Strike 1.1: m_iRoundTime is in seconds, so mp_roundtime can have a fraction; 1 to 9 minutes
// (1.0b: whole minutes, 3 to 9, less became 1). The freeze time is 0 to 60 seconds (1.0b: 0 to 15), and its
// clamp sets its own cvar
static void ReadMultiplayCvars( CHalfLifeMultiplay *mp )
{
	mp->m_iRoundTime = (int)( CVAR_GET_FLOAT( "mp_roundtime" ) * 60 );
	mp->m_iC4Timer = (int)CVAR_GET_FLOAT( "mp_c4timer" );
	mp->m_iFreezeTime = (int)CVAR_GET_FLOAT( "mp_freezetime" );
	mp->m_iLimitTeams = (int)CVAR_GET_FLOAT( "mp_limitteams" );

	if ( mp->m_iRoundTime > 540 )
	{
		CVAR_SET_FLOAT( "mp_roundtime", 9 );
		mp->m_iRoundTime = 540;
	}
	else if ( mp->m_iRoundTime < 60 )
	{
		CVAR_SET_FLOAT( "mp_roundtime", 1 );
		mp->m_iRoundTime = 60;
	}

	if ( mp->m_iFreezeTime > 60 )
	{
		CVAR_SET_FLOAT( "mp_freezetime", 60 );
		mp->m_iFreezeTime = 60;
	}
	else if ( mp->m_iFreezeTime < 0 )
	{
		CVAR_SET_FLOAT( "mp_freezetime", 0 );
		mp->m_iFreezeTime = 0;
	}

	if ( mp->m_iC4Timer > 90 )
	{
		CVAR_SET_FLOAT( "mp_c4timer", 90 );
		mp->m_iC4Timer = 90;
	}
	else if ( mp->m_iC4Timer < 10 )
	{
		CVAR_SET_FLOAT( "mp_c4timer", 10 );
		mp->m_iC4Timer = 10;
	}

	if ( mp->m_iLimitTeams > 20 )
	{
		CVAR_SET_FLOAT( "mp_limitteams", 20 );
		mp->m_iLimitTeams = 20;
	}
	else if ( mp->m_iLimitTeams < 1 )
	{
		CVAR_SET_FLOAT( "mp_limitteams", 0 );
		mp->m_iLimitTeams = 20;
	}
}

void TerminateRound( float tmDelay, int iWinStatus );

extern cvar_t allow_spectators;	// game.cpp (Counter-Strike 1.1)
extern unsigned short m_usResetDecals;	// client.cpp (Counter-Strike 1.3; 1.6 declares it here)
extern float g_flTimeLimit;		// client.cpp (Counter-Strike 1.1)
extern float g_flResetTime;		// client.cpp (Counter-Strike 1.1)
extern int gmsgAllowSpec;		// player.cpp (Counter-Strike 1.1)
extern int g_iHostageNumber;	// hostage.cpp (Counter-Strike 1.4)
extern int gmsgBombPickup;		// player.cpp (Counter-Strike 1.4)
extern void ClearBodyQue( void );	// world.cpp (Counter-Strike 1.4)

//*********************************************************
// Rules for the half-life multiplayer game.
//*********************************************************

CHalfLifeMultiplay :: CHalfLifeMultiplay()
{
	m_VoiceGameMgr.Init(&g_GameMgrHelper, gpGlobals->maxClients);

	RefreshSkillData();
	m_flIntermissionEndTime = 0;
	m_flIntermissionStartTime = 0;	// Counter-Strike 1.1
	m_flRestartRoundTime = 0;
	m_iAccountCT = 0;
	m_iAccountTerrorist = 0;
	m_iHostagesRescued = 0;
	m_iRoundWinStatus = 0;
	m_iNumCTWins = 0;
	m_iNumTerroristWins = 0;
	m_pVIP = NULL;	// Beta 6.0
	m_iNumCT = 0;
	m_iNumTerrorist = 0;
	m_iNumSpawnableCT = 0;
	m_iNumSpawnableTerrorist = 0;
	m_iMapHasCameras = 2;
	g_fGameOver = FALSE;
	m_iLoserBonus = 1400;
	m_iNumConsecutiveCTLoses = 0;
	m_iNumConsecutiveTerroristLoses = 0;
	m_iC4Turn = 0;
	m_fBombDefused = false;
	m_fTargetBombed = false;
	m_bFreezePeriod = TRUE;
	m_fCountedSpawnPoints = false;
	m_flNextRestartCheck = 0;
	m_fGameCommenced = false;
	m_fCompleteReset = false;	// Counter-Strike 1.4 (was true)

	// Beta 6.0: escape and assassination maps, info_map_parameters
	m_flRequiredEscapeRatio = 0.5;
	m_iNumEscapers = 0;
	m_bCTCantBuy = false;
	m_bTCantBuy = false;
	m_flBombRadius = 500;
	m_iTotalGunCount = 0;
	m_iTotalGrenadeCount = 0;
	m_iTotalArmourCount = 0;
	m_iConsecutiveVIP = 0;
	m_iUnBalancedRounds = 0;
	m_iNumEscapeRounds = 0;
	m_fRoundTerminating = false;	// Counter-Strike 1.3
	g_iHostageNumber = 0;	// Counter-Strike 1.4: the hostages' numbers start again
	m_iMaxRounds = (int)CVAR_GET_FLOAT( "mp_maxrounds" );	// Counter-Strike 1.0 (Beta 7.1 read it only at a complete restart)

	// Counter-Strike 1.1: not negative; and the rounds a team may win (mp_winlimit)
	if ( m_iMaxRounds < 0 )
	{
		m_iMaxRounds = 0;
		CVAR_SET_FLOAT( "mp_maxrounds", 0 );
	}

	m_iTotalRoundsPlayed = 0;	// Beta 7.1
	m_iMaxRoundsWon = (int)CVAR_GET_FLOAT( "mp_winlimit" );

	if ( m_iMaxRoundsWon < 0 )
	{
		m_iMaxRoundsWon = 0;
		CVAR_SET_FLOAT( "mp_winlimit", 0 );
	}

	// Beta 6.5: the map votes, the VIP's turn
	for ( int i = 0; i < 100; i++ )
		m_iMapVotes[i] = 0;

	m_iLastPick = 1;

	m_bMapHasEscapeZone = false;
	m_iMapHasVIPSafetyZone = 0;
	m_fMapHasBombZone = false;
	m_fMapHasRescueZone = false;

	m_iStoredSpectValue = (int)allow_spectators.value;	// Counter-Strike 1.1: Think tells the clients when it changes

	m_pVIPQueue[0] = m_pVIPQueue[1] = m_pVIPQueue[2] = m_pVIPQueue[3] = m_pVIPQueue[4] = NULL;

	// Beta 4.0: no high-detail player models
	CVAR_SET_FLOAT( "cl_himodels", 0 );

	// Beta 5.0: the cvars are read (and clamped) as RestartRound reads them; the first
	// round's freeze time is 2 seconds longer, after the clamp
	ReadMultiplayCvars( this );

	m_iFreezeTime += 2;
	m_flMaxIdlePeriod = m_iRoundTime * 2;	// (the round time is in seconds now)
	m_iRoundTimeSecs = m_iFreezeTime;
	
	// 11/8/98
	// Modified by YWB:  Server .cfg file is now a cvar, so that 
	//  server ops can run multiple game servers, with different server .cfg files,
	//  from a single installed directory.
	// Mapcyclefile is already a cvar.

	// 3/31/99
	// Added lservercfg file cvar, since listen and dedicated servers should not
	// share a single config file. (sjb)
	if ( IS_DEDICATED_SERVER() )
	{
		// dedicated server
		char *servercfgfile = (char *)CVAR_GET_STRING( "servercfgfile" );

		if ( servercfgfile && servercfgfile[0] )
		{
			char szCommand[256];
			
			// (Beta 6.0 to Counter-Strike 1.0b: the dedicated server said nothing)
			ALERT( at_console, "Executing dedicated server config file\n" );
			sprintf( szCommand, "exec %s\n", servercfgfile );
			SERVER_COMMAND( szCommand );
		}
	}
	else
	{
		// listen server
		char *lservercfgfile = (char *)CVAR_GET_STRING( "lservercfgfile" );

		if ( lservercfgfile && lservercfgfile[0] )
		{
			char szCommand[256];
			
			ALERT( at_console, "Executing listen server config file\n" );
			sprintf( szCommand, "exec %s\n", lservercfgfile );
			SERVER_COMMAND( szCommand );
		}
	}

	// Beta 5.0 (leak): Think starts the clock
	m_flRoundStartTime = 0;
	m_flRoundStartTime2 = 0;
}

//=========================================================
//=========================================================
void CHalfLifeMultiplay::RefreshSkillData( void )
{
// load all default values
	CGameRules::RefreshSkillData();

// override some values for multiplay.

	// suitcharger
	gSkillData.suitchargerCapacity = 30;

	// Crowbar whack
	gSkillData.plrDmgCrowbar = 25;

	// Glock Round
	gSkillData.plrDmg9MM = 12;

	// 357 Round
	gSkillData.plrDmg357 = 40;

	// MP5 Round
	gSkillData.plrDmgMP5 = 12;

	// M203 grenade
	gSkillData.plrDmgM203Grenade = 100;

	// Shotgun buckshot
	gSkillData.plrDmgBuckshot = 20;// fewer pellets in deathmatch

	// Crossbow
	gSkillData.plrDmgCrossbowClient = 20;

	// RPG
	gSkillData.plrDmgRPG = 120;

	// Egon
	gSkillData.plrDmgEgonWide = 20;
	gSkillData.plrDmgEgonNarrow = 10;

	// Hand Grendade
	gSkillData.plrDmgHandGrenade = 100;

	// Satchel Charge
	gSkillData.plrDmgSatchel = 120;

	// Tripmine
	gSkillData.plrDmgTripmine = 150;

	// hornet
	gSkillData.plrDmgHornet = 10;
}

//=========================================================
// RemoveGuns - Beta 4.0: every weapon box (dropped guns)
// goes at a new round ("no more gun running")
//=========================================================
void CHalfLifeMultiplay::RemoveGuns( void )
{
	CBaseEntity *toremove = UTIL_FindEntityByClassname( NULL, "weaponbox" );

	while ( toremove )
	{
		((CWeaponBox *)toremove)->Kill();
		toremove = UTIL_FindEntityByClassname( toremove, "weaponbox" );
	}
}

//=========================================================
// UpdateTeamScores - Beta 6.5 (Linux symbol): both teams'
// round wins to every client. The TeamScore message carries
// the team's name now (a team number until Beta 6.1)
//=========================================================
void CHalfLifeMultiplay::UpdateTeamScores( void )
{
	MESSAGE_BEGIN( MSG_ALL, gmsgTeamScore );
		WRITE_STRING( "CT" );
		WRITE_SHORT( m_iNumCTWins );
	MESSAGE_END();

	MESSAGE_BEGIN( MSG_ALL, gmsgTeamScore );
		WRITE_STRING( "TERRORIST" );
		WRITE_SHORT( m_iNumTerroristWins );
	MESSAGE_END();
}

//=========================================================
// CleanUpMap - Beta 4.0: breakables and doors are put back
// (Restart), a C4 goes, and up to four defusers; then the
// weapon boxes. Beta 5.0 (leak): no backpack any more, and the
// console says so for each defuser. Beta 5.0: the lights are
// put back first. Beta 6.5: ambient_generic too, and every
// grenade goes.
//=========================================================
void CHalfLifeMultiplay::CleanUpMap( void )
{
	CBaseEntity *torestart = UTIL_FindEntityByClassname( NULL, "light" );

	while ( torestart )
	{
		torestart->Restart();
		torestart = UTIL_FindEntityByClassname( torestart, "light" );
	}

	torestart = UTIL_FindEntityByClassname( NULL, "func_breakable" );

	while ( torestart )
	{
		torestart->Restart();
		torestart = UTIL_FindEntityByClassname( torestart, "func_breakable" );
	}

	torestart = UTIL_FindEntityByClassname( NULL, "func_door" );

	while ( torestart )
	{
		torestart->Restart();
		torestart = UTIL_FindEntityByClassname( torestart, "func_door" );
	}

	// Beta 6.0: func_water too (a door), trains, the armouries
	torestart = UTIL_FindEntityByClassname( NULL, "func_water" );

	while ( torestart )
	{
		torestart->Restart();
		torestart = UTIL_FindEntityByClassname( torestart, "func_water" );
	}

	torestart = UTIL_FindEntityByClassname( NULL, "func_door_rotating" );

	while ( torestart )
	{
		torestart->Restart();
		torestart = UTIL_FindEntityByClassname( torestart, "func_door_rotating" );
	}

	torestart = UTIL_FindEntityByClassname( NULL, "func_tracktrain" );

	while ( torestart )
	{
		torestart->Restart();
		torestart = UTIL_FindEntityByClassname( torestart, "func_tracktrain" );
	}

	// Beta 7.0: the vehicles
	torestart = UTIL_FindEntityByClassname( NULL, "func_vehicle" );

	while ( torestart )
	{
		torestart->Restart();
		torestart = UTIL_FindEntityByClassname( torestart, "func_vehicle" );
	}

	torestart = UTIL_FindEntityByClassname( NULL, "func_train" );

	while ( torestart )
	{
		torestart->Restart();
		torestart = UTIL_FindEntityByClassname( torestart, "func_train" );
	}

	torestart = UTIL_FindEntityByClassname( NULL, "armoury_entity" );

	while ( torestart )
	{
		torestart->Restart();
		torestart = UTIL_FindEntityByClassname( torestart, "armoury_entity" );
	}

	// Beta 6.5: the ambient sounds too
	torestart = UTIL_FindEntityByClassname( NULL, "ambient_generic" );

	while ( torestart )
	{
		torestart->Restart();
		torestart = UTIL_FindEntityByClassname( torestart, "ambient_generic" );
	}

	// Counter-Strike 1.4: the sprites too
	torestart = UTIL_FindEntityByClassname( NULL, "env_sprite" );

	while ( torestart )
	{
		torestart->Restart();
		torestart = UTIL_FindEntityByClassname( torestart, "env_sprite" );
	}

	// Beta 6.5: every grenade goes (Beta 6.1: only the first).
	// Counter-Strike 1.4: twenty at most (to 1.3 the loop ran while a grenade
	// was found or more than ten were counted)
	int icount = 0;	// Counter-Strike 1.6's name
	CBaseEntity *toremove = UTIL_FindEntityByClassname( NULL, "grenade" );

	while ( toremove && icount < 20 )
	{
		UTIL_Remove( toremove );
		toremove = UTIL_FindEntityByClassname( toremove, "grenade" );
		icount++;
	}

	// Counter-Strike 1.3: every defuser goes (to 1.1: four at most)
	toremove = UTIL_FindEntityByClassname( NULL, "item_thighpack" );

	while ( toremove )
	{
		UTIL_Remove( toremove );
		toremove = UTIL_FindEntityByClassname( toremove, "item_thighpack" );
	}

	RemoveGuns();

	// Counter-Strike 1.3: the clients clear their decals (events/decal_reset.sc)
	PLAYBACK_EVENT_FULL( FEV_GLOBAL | FEV_RELIABLE, NULL, m_usResetDecals, 0, (float *)&g_vecZero, (float *)&g_vecZero, 0.0, 0.0, 0, 0, 0, 0 );
}

//=========================================================
// GiveC4 - Beta 4.0: the terrorists take turns with the C4
// (one more each round); its carrier becomes the commander.
// Beta 5.0 (leak): the C4 is a weapon (weapon_c4) and goes to a
// living terrorist; the turn wraps at the team's size as
// CheckWinConditions counted it.
//=========================================================
void CHalfLifeMultiplay::GiveC4( void )
{
	if ( ++m_iC4Turn > m_iNumTerrorist )
		m_iC4Turn = 1;

	CBaseEntity *pPlayer = UTIL_FindEntityByClassname( NULL, "player" );
	int iCount = 0;

	while ( pPlayer != NULL && !FNullEnt( pPlayer->edict() ) )
	{
		if ( pPlayer->IsPlayer() && pPlayer->pev->flags != FL_DORMANT )
		{
			CBasePlayer *player = GetClassPtr( (CBasePlayer *)pPlayer->pev );

			if ( player->pev->deadflag == DEAD_NO && player->m_iTeam == TEAM_TERRORIST && ++iCount == m_iC4Turn )
			{
				player->m_bHasC4 = true;
				player->GiveNamedItem( "weapon_c4" );
				player->SetBombIcon( FALSE );
				player->pev->body = 1;
				player->m_flDisplayHistory |= 0x1000;	// Counter-Strike 1.4: he is told
				player->HintMessage( "#Hint_you_have_the_bomb", FALSE, TRUE );

				// Counter-Strike 1.1: the carrier goes to the log
				UTIL_LogPrintf( "\"%s<%i><%s><TERRORIST>\" triggered \"Spawned_With_The_Bomb\"\n", STRING( player->pev->netname ), GETPLAYERUSERID( player->edict() ), GETPLAYERAUTHID( player->edict() ) );
			}
		}
		pPlayer = UTIL_FindEntityByClassname( pPlayer, "player" );
	}

	// Counter-Strike 1.4: when the turn found nobody, the first living terrorist gets it
	if ( !IsThereABomber() )
	{
		m_iC4Turn = 0;
		pPlayer = UTIL_FindEntityByClassname( NULL, "player" );

		while ( pPlayer != NULL && !FNullEnt( pPlayer->edict() ) )
		{
			if ( pPlayer->IsPlayer() && pPlayer->pev->flags != FL_DORMANT )
			{
				CBasePlayer *player = GetClassPtr( (CBasePlayer *)pPlayer->pev );

				if ( player->pev->deadflag == DEAD_NO && player->m_iTeam == TEAM_TERRORIST )
				{
					player->m_bHasC4 = true;
					player->GiveNamedItem( "weapon_c4" );
					player->SetBombIcon( FALSE );
					player->pev->body = 1;
					player->m_flDisplayHistory |= 0x1000;
					player->HintMessage( "#Hint_you_have_the_bomb", FALSE, TRUE );

					UTIL_LogPrintf( "\"%s<%i><%s><TERRORIST>\" triggered \"Spawned_With_The_Bomb\"\n", STRING( player->pev->netname ), GETPLAYERUSERID( player->edict() ), GETPLAYERAUTHID( player->edict() ) );
					return;
				}
			}
			pPlayer = UTIL_FindEntityByClassname( pPlayer, "player" );
		}
	}
}

//=========================================================
// CheckWinConditions - Beta 5.0 (leak): nothing is decided
// until both teams have a player past the appearance menu (the
// first time, the round restarts at once). A bombed target wins
// it for the terrorists and a defused bomb for the CTs; a team
// with nobody left alive loses it (the CTs win only when no
// planted C4 is ticking); nobody alive on either side is a
// draw; and with every hostage rescued or dead and at least
// half of them rescued, the CTs win. The winners' score counts
// only once both teams have players. The next round starts five
// seconds later. Beta 6.5's team money (6.1's in brackets): VIP
// escaped 3,500 [2,750], VIP killed 3,250 [2,750], terrorists
// escaped 3,150 [0], target bombed 3,500 [2,750], bomb defused
// 3,250 [2,750], a team wiped out 3,250 on a bomb map, else
// 3,000 [2,500, 2,000], hostages rescued 2,500 [2,000]
//=========================================================
void CHalfLifeMultiplay::CheckWinConditions( void )
{
	BOOL bNeededPlayers = FALSE;	// name ours

	if ( m_iRoundWinStatus != 0 )
		return;

	CBaseEntity *pPlayer = NULL;
	int NumDeadTerrorist = 0;
	int NumDeadCT = 0;
	int NumAliveCT = 0;
	int NumAliveTerrorist = 0;

	m_iNumSpawnableCT = 0;
	m_iNumSpawnableTerrorist = 0;
	m_iNumCT = 0;
	m_iNumTerrorist = 0;
	m_iHaveEscaped = 0;	// Beta 6.0

	pPlayer = UTIL_FindEntityByClassname( pPlayer, "player" );
	while ( pPlayer != NULL && !FNullEnt( pPlayer->edict() ) )
	{
		CBasePlayer *player = GetClassPtr( (CBasePlayer *)pPlayer->pev );

		if ( pPlayer->pev->flags != FL_DORMANT && player->m_iTeam != TEAM_UNASSIGNED && player->m_iTeam != TEAM_SPECTATOR )	// Counter-Strike 1.1: nor the spectators
		{
			switch ( player->m_iTeam )
			{
			case TEAM_TERRORIST:
				m_iNumTerrorist++;
				if ( player->m_iMenu != Menu_ChooseAppearance )
					m_iNumSpawnableTerrorist++;

				if ( player->pev->deadflag != DEAD_NO )
					NumDeadTerrorist++;
				else
					NumAliveTerrorist++;

				// Beta 6.0: the escape maps
				if ( player->m_bEscaped == true )
					m_iHaveEscaped++;
				break;

			case TEAM_CT:
				m_iNumCT++;
				if ( player->m_iMenu != Menu_ChooseAppearance )
					m_iNumSpawnableCT++;

				if ( player->pev->deadflag != DEAD_NO )
					NumDeadCT++;
				else
					NumAliveCT++;
				break;
			}
		}
		pPlayer = UTIL_FindEntityByClassname( pPlayer, "player" );
	}

	if ( !m_iNumSpawnableTerrorist || !m_iNumSpawnableCT )
	{
		UTIL_ClientPrintAll( HUD_PRINTCONSOLE, "#Game_scoring" );	// Counter-Strike 1.1: a title (was "Scoring will not start until both teams have players.\n")
		bNeededPlayers = TRUE;
	}

	// Counter-Strike 1.3: the game commences when both teams have a spawnable player for the first time (to 1.1:
	// at the first call without them, restarting at once): the clients are told, the freeze period ends, and
	// three seconds later a complete restart follows
	if ( !m_fGameCommenced && m_iNumSpawnableTerrorist != 0 && m_iNumSpawnableCT != 0 )
	{
		UTIL_LogPrintf( "World triggered \"Game_Commencing\"\n" );	// (1.0b: "*** First player has joined the server. Game Commencing! ***\n")
		m_bFreezePeriod = FALSE;
		m_fCompleteReset = true;
		EndRoundMessage( "#Game_Commencing", 10 );
		TerminateRound( 3, 3 );
		m_fGameCommenced = true;
		return;
	}

	// Beta 7.0: every round's end message follows the score (6.6: it came first)

	// Beta 6.0: an assassination map is won by the VIP's escape (Beta 6.5:
	// 3,500 to the CTs; 6.1: 2,750) or death (3,250 to the terrorists; 6.1: 2,750)
	if ( m_iMapHasVIPSafetyZone == 1 && m_pVIP )
	{
		if ( m_pVIP->m_bEscaped == true )
		{
			Broadcast( "ctwin" );
			m_iAccountCT += 3500;

			if ( !bNeededPlayers )
			{
				m_iNumCTWins++;
				UpdateTeamScores();
			}

			// Counter-Strike 1.4: the director's event (SDK 2.3's svc_director): the VIP, priority 15, final
			MESSAGE_BEGIN( MSG_SPEC, SVC_DIRECTOR );
				WRITE_BYTE( 9 );	// command length in bytes
				WRITE_BYTE( DRC_CMD_EVENT );
				WRITE_SHORT( ENTINDEX( m_pVIP->edict() ) );
				WRITE_SHORT( 0 );
				WRITE_LONG( 15 | DRC_FLAG_FINAL );
			MESSAGE_END();

			EndRoundMessage( "#VIP_Escaped", 2 );

			TerminateRound( 5, 1 );
			return;
		}
		else if ( m_pVIP->pev->deadflag != DEAD_NO )
		{
			Broadcast( "terwin" );
			m_iAccountTerrorist += 3250;

			if ( !bNeededPlayers )
			{
				m_iNumTerroristWins++;
				UpdateTeamScores();
			}

			EndRoundMessage( "#VIP_Assassinated", 3 );

			TerminateRound( 5, 2 );
			return;
		}
	}

	// Beta 6.0: an escape map is won by the terrorists when half of them
	// (m_flRequiredEscapeRatio) escaped (Beta 6.5: 3,150 to them), by the CTs
	// when none of the rest lives; the CTs get 3,500 times the share that did
	// not escape
	if ( m_bMapHasEscapeZone == true )
	{
		float flEscapeRatio = (float)m_iHaveEscaped / m_iNumEscapers;	// name ours

		if ( flEscapeRatio >= m_flRequiredEscapeRatio )
		{
			Broadcast( "terwin" );
			m_iAccountTerrorist += 3150;

			if ( !bNeededPlayers )
			{
				m_iNumTerroristWins++;
				UpdateTeamScores();
			}

			EndRoundMessage( "#Terrorists_Escaped", 4 );

			TerminateRound( 5, 2 );
			return;
		}
		else if ( NumAliveTerrorist == 0 && flEscapeRatio < m_flRequiredEscapeRatio )
		{
			Broadcast( "ctwin" );
			m_iAccountCT += ( 1 - flEscapeRatio ) * 3500;

			if ( !bNeededPlayers )
			{
				m_iNumCTWins++;
				UpdateTeamScores();
			}

			EndRoundMessage( "#CTs_PreventEscape", 5 );

			TerminateRound( 5, 1 );
			return;
		}
		// Counter-Strike 1.4: the same with a dead terrorist and a spawnable counter-terrorist, for 3,250 times
		// the share (reached only when the ratio is not a number: no escapers counted, 0 / 0)
		else if ( NumAliveTerrorist == 0 && NumDeadTerrorist != 0 && m_iNumSpawnableCT > 0 )
		{
			Broadcast( "ctwin" );
			m_iAccountCT += ( 1 - flEscapeRatio ) * 3250;

			if ( !bNeededPlayers )
			{
				m_iNumCTWins++;
				UpdateTeamScores();
			}

			EndRoundMessage( "#Escaping_Terrorists_Neutralized", 6 );

			TerminateRound( 5, 1 );
			return;
		}
	}

	// (Beta 6.5: 3,500 for the bombing, 3,250 for the defusing; 6.1: 2,750 each)
	if ( m_fTargetBombed == true && m_fMapHasBombTarget == true )
	{
		Broadcast( "terwin" );
		m_iAccountTerrorist += 3500;

		if ( !bNeededPlayers )
		{
			m_iNumTerroristWins++;
			UpdateTeamScores();
		}

		EndRoundMessage( "#Target_Bombed", 1 );

		TerminateRound( 5, 2 );
		return;
	}

	if ( m_fBombDefused == true && m_fMapHasBombTarget == true )
	{
		Broadcast( "ctwin" );
		m_iAccountCT += 3250;

		if ( !bNeededPlayers )
		{
			m_iNumCTWins++;
			UpdateTeamScores();
		}

		EndRoundMessage( "#Bomb_Defused", 7 );

		TerminateRound( 5, 1 );
		return;
	}

	// Beta 5.0: only against a team with players past the appearance menu.
	// Counter-Strike 1.3: a team is wiped out only when both teams have players and spawnable players;
	// else nobody alive on either side is a draw (to 1.1 the draw was the third arm of one chain)
	if ( m_iNumCT > 0 && m_iNumSpawnableCT > 0 && m_iNumTerrorist > 0 && m_iNumSpawnableTerrorist > 0 )
	{
		if ( NumAliveTerrorist == 0 && NumDeadTerrorist != 0 )
		{
			// not while a C4 is ticking. Beta 6.5: every grenade is looked at, and
			// only a planted C4 counts (6.1 took the first grenade found)
			BOOL bC4Present = FALSE;	// name ours
			CBaseEntity *pC4 = UTIL_FindEntityByClassname( NULL, "grenade" );

			while ( pC4 )
			{
				if ( ((CGrenade *)pC4)->m_bIsC4 == true && !((CGrenade *)pC4)->m_bJustBlew )
					bC4Present = TRUE;

				pC4 = UTIL_FindEntityByClassname( pC4, "grenade" );
			}

			if ( bC4Present )
				return;

			Broadcast( "ctwin" );

			// Beta 5.0: added to the account (the leak set it); Beta 6.5: 3,250
			// on a bomb map, else 3,000 (6.1: 2,500 and 2,000)
			if ( m_fMapHasBombTarget )
				m_iAccountCT += 3250;
			else
				m_iAccountCT += 3000;

			if ( !bNeededPlayers )
			{
				m_iNumCTWins++;
				UpdateTeamScores();
			}

			EndRoundMessage( "#CTs_Win", 8 );

			TerminateRound( 5, 1 );
			return;
		}
		else if ( NumAliveCT == 0 && NumDeadCT != 0 )
		{
			Broadcast( "terwin" );

			if ( m_fMapHasBombTarget )
				m_iAccountTerrorist += 3250;
			else
				m_iAccountTerrorist += 3000;

			if ( !bNeededPlayers )
			{
				m_iNumTerroristWins++;
				UpdateTeamScores();
			}

			EndRoundMessage( "#Terrorists_Win", 9 );

			TerminateRound( 5, 2 );
			return;
		}
	}
	else if ( NumAliveCT == 0 && NumAliveTerrorist == 0 )
	{
		EndRoundMessage( "#Round_Draw", 10 );
		Broadcast( "rounddraw" );
		TerminateRound( 5, 3 );
		return;
	}

	// the hostages: none left to rescue (rescued ones take no
	// damage) and at least half of them rescued
	int iHostages = 0;
	bool bHostageLeft = false;
	CBaseEntity *pHostage = UTIL_FindEntityByClassname( NULL, "hostage_entity" );

	while ( pHostage != NULL )
	{
		iHostages++;

		if ( pHostage->pev->takedamage == DAMAGE_YES )
			bHostageLeft = true;

		pHostage = UTIL_FindEntityByClassname( pHostage, "hostage_entity" );
	}

	if ( !bHostageLeft && iHostages > 0 && m_iHostagesRescued >= iHostages * 0.5 )
	{
		Broadcast( "ctwin" );
		m_iAccountCT += 2500;	// Beta 6.5 (5.0 to 6.1: 2000; the leak: 1000)

		if ( !bNeededPlayers )
		{
			m_iNumCTWins++;
			UpdateTeamScores();
		}

		EndRoundMessage( "#All_Hostages_Rescued", 11 );

		TerminateRound( 5, 1 );
	}
}

//=========================================================
// TerminateRound - Beta 5.0 (leak): the round is decided
// (iWinStatus: 1 CTs, 2 terrorists, 3 nobody) and the next one
// starts tmDelay seconds from now (name ours)
//=========================================================
inline void TerminateRound( float tmDelay, int iWinStatus )
{
	CHalfLifeMultiplay *mp = (CHalfLifeMultiplay *)g_pGameRules;	// name ours

	mp->m_iRoundWinStatus = iWinStatus;
	mp->m_flRestartRoundTime = gpGlobals->time + tmDelay;
	mp->m_fRoundTerminating = true;
}

//=========================================================
// SwapAllPlayers - Beta 6.0 (Linux symbol): every player
// changes sides (CBasePlayer::SwitchTeam), and the team
// scores with them (the escape maps, every eighth round)
//=========================================================
void CHalfLifeMultiplay::SwapAllPlayers( void )
{
	CBaseEntity *pPlayer = NULL;

	pPlayer = UTIL_FindEntityByClassname( pPlayer, "player" );
	while ( pPlayer != NULL && !FNullEnt( pPlayer->edict() ) )
	{
		if ( pPlayer->pev->flags != FL_DORMANT )
		{
			CBasePlayer *player = GetClassPtr( (CBasePlayer *)pPlayer->pev );
			player->SwitchTeam();
		}
		pPlayer = UTIL_FindEntityByClassname( pPlayer, "player" );
	}

	int iTemp = m_iNumCTWins;	// name ours
	m_iNumCTWins = m_iNumTerroristWins;
	m_iNumTerroristWins = iTemp;

	UpdateTeamScores();
}

//=========================================================
// BalanceTeams - Beta 6.0 (Linux symbol): the bigger team
// gives half the difference (at most four) to the other,
// each time its player with the most frags (Counter-Strike
// 1.4: with the highest user id). Beta 6.5: on an
// assassination map the counter-terrorists are filled up to
// 55% of the players and one, else the terrorists to 45% and
// one
//=========================================================
void CHalfLifeMultiplay::BalanceTeams( void )
{
	int iTeamToSwap;	// names ours
	int iNumToSwap;

	if ( m_iMapHasVIPSafetyZone == 1 )
	{
		// Beta 6.5: an assassination map wants 55% counter-terrorists.
		// Beta 7.0: only with an odd number of players, else half; the
		// terrorists are the rest (the names are Counter-Strike 1.6's)
		int iDesiredNumCT, iDesiredNumTerrorist;
		int iNumPlayers = m_iNumTerrorist + m_iNumCT;	// name ours

		if ( iNumPlayers % 2 != 0 )
			iDesiredNumCT = 1 - (int)( iNumPlayers * -0.55 );
		else
			iDesiredNumCT = iNumPlayers / 2;

		iDesiredNumTerrorist = iNumPlayers - iDesiredNumCT;

		if ( m_iNumCT < iDesiredNumCT )
		{
			iTeamToSwap = TEAM_TERRORIST;
			iNumToSwap = iDesiredNumCT - m_iNumCT;
		}
		else if ( m_iNumTerrorist < iDesiredNumTerrorist )
		{
			iTeamToSwap = TEAM_CT;
			iNumToSwap = iDesiredNumTerrorist - m_iNumTerrorist;
		}
		else
			return;
	}
	else
	{
		if ( m_iNumCT > m_iNumTerrorist )
		{
			iTeamToSwap = TEAM_CT;
			iNumToSwap = ( m_iNumCT - m_iNumTerrorist ) / 2;
		}
		else if ( m_iNumTerrorist > m_iNumCT )
		{
			iTeamToSwap = TEAM_TERRORIST;
			iNumToSwap = ( m_iNumTerrorist - m_iNumCT ) / 2;
		}
		else
			return;
	}

	if ( iNumToSwap > 4 )
		iNumToSwap = 4;

	for ( int i = 1; i <= iNumToSwap; i++ )
	{
		int iHighestUserID = 0;	// Counter-Strike 1.6's name
		CBasePlayer *pPlayerToSwap = NULL;
		CBaseEntity *pPlayer = NULL;

		pPlayer = UTIL_FindEntityByClassname( pPlayer, "player" );
		while ( pPlayer != NULL && !FNullEnt( pPlayer->edict() ) )
		{
			if ( pPlayer->pev->flags != FL_DORMANT )
			{
				CBasePlayer *player = GetClassPtr( (CBasePlayer *)pPlayer->pev );

				// Beta 6.1: never the VIP. Counter-Strike 1.4: the one who joined the server last
				// (the highest user id; to 1.3 the one with the most frags)
				if ( player->m_iTeam == iTeamToSwap && GETPLAYERUSERID( player->edict() ) > iHighestUserID && m_pVIP != player )
				{
					iHighestUserID = GETPLAYERUSERID( player->edict() );
					pPlayerToSwap = player;
				}
			}
			pPlayer = UTIL_FindEntityByClassname( pPlayer, "player" );
		}

		if ( pPlayerToSwap )
			pPlayerToSwap->SwitchTeam();
	}
}

//=========================================================
// CheckMapConditions - Counter-Strike 1.4 (virtual, slot 61;
// Linux symbol): what the map has: a bomb target, rescue,
// buy and escape zones, a VIP safety zone. Retail's
// ServerActivate (client.cpp) calls it through the slot;
// RestartRound still looks for them itself (1.3's text)
//=========================================================
void CHalfLifeMultiplay::CheckMapConditions( void )
{
	if ( UTIL_FindEntityByClassname( NULL, "func_bomb_target" ) )
	{
		m_fMapHasBombTarget = true;
		m_fMapHasBombZone = true;
	}
	else if ( UTIL_FindEntityByClassname( NULL, "info_bomb_target" ) )
	{
		m_fMapHasBombTarget = true;
		m_fMapHasBombZone = false;
	}
	else
	{
		m_fMapHasBombTarget = false;
		m_fMapHasBombZone = false;
	}

	if ( UTIL_FindEntityByClassname( NULL, "func_hostage_rescue" ) )
		m_fMapHasRescueZone = true;
	else
		m_fMapHasRescueZone = false;

	if ( UTIL_FindEntityByClassname( NULL, "func_buyzone" ) )
		m_fMapHasBuyZone = true;
	else
		m_fMapHasBuyZone = false;

	if ( UTIL_FindEntityByClassname( NULL, "func_escapezone" ) )
		m_bMapHasEscapeZone = true;
	else
		m_bMapHasEscapeZone = false;

	if ( UTIL_FindEntityByClassname( NULL, "func_vip_safetyzone" ) )
		m_iMapHasVIPSafetyZone = 1;
	else
		m_iMapHasVIPSafetyZone = 2;
}

//=========================================================
// RestartRound - Beta 5.0 (leak): the scores start afresh
// after sv_restartround (and at the first round); the round,
// freeze and C4 times are read again and the freeze period
// begins; the map's zones are looked for; the hostages go
// back to where they stood (150 to the winners for each one
// still standing, at most 2,000, sic: the walk stops there);
// the losers get the loser bonus (1,400 at first; Beta 6.5: 500
// more after every loss from the second in a row while it is
// under 3,000, and 1,500 again when such a run ends), the CTs
// 750 for each rescued hostage (6.1: 250); the C4 is dropped,
// everybody in a team respawns and is paid, the map is cleaned
// up and the C4 handed out.
//=========================================================
void CHalfLifeMultiplay::RestartRound( void )
{
	m_iTotalRoundsPlayed++;	// Beta 7.1

	ClearBodyQue();	// Counter-Strike 1.4: the bodies of the round before go

	// Beta 6.0: the movement settings, every round
	CVAR_SET_FLOAT( "sv_accelerate", 5 );
	CVAR_SET_FLOAT( "sv_friction", 4 );
	CVAR_SET_FLOAT( "sv_stopspeed", 75 );

	// Beta 6.0: mp_autoteambalance moves players when the teams have been two
	// or more apart; Beta 6.1: after one such round, announced as soon as the
	// first one ends (6.0 waited more than two rounds)
	m_iNumCT = CountTeamPlayers( TEAM_CT );
	m_iNumTerrorist = CountTeamPlayers( TEAM_TERRORIST );

	// Counter-Strike 1.4: on a bomb map the clients forget where the bomb lay
	if ( m_fMapHasBombTarget )
	{
		MESSAGE_BEGIN( MSG_ALL, gmsgBombPickup );
		MESSAGE_END();
	}

	if ( CVAR_GET_FLOAT( "mp_autoteambalance" ) != 0 && m_iUnBalancedRounds >= 1 )
		BalanceTeams();

	if ( m_iNumCT - m_iNumTerrorist >= 2 || m_iNumTerrorist - m_iNumCT >= 2 )
		m_iUnBalancedRounds++;
	else
		m_iUnBalancedRounds = 0;

	if ( CVAR_GET_FLOAT( "mp_autoteambalance" ) != 0 && m_iUnBalancedRounds == 1 )
		UTIL_ClientPrintAll( HUD_PRINTCENTER, "#Auto_Team_Balance_Next_Round" );	// Beta 7.1: a titles.txt name (6.5's words: "*** Auto-Team Balance next round ***")

	if ( m_fCompleteReset )
	{
		// Beta 7.1: the minutes played so far are added to the time limit
		// (Beta 7.0 set it back to m_iMaxMapTime, the map's first), the rounds
		// are counted from zero and mp_maxrounds is read. Counter-Strike 1.0b:
		// the cvar is read by name, and the time added to it is not cut to
		// whole minutes (1.0: timelimit.value += (int)( gpGlobals->time / 60 )).
		// Counter-Strike 1.1: the cvar is left alone (a negative one set to 0);
		// the time of the reset is kept and the limit's end worked out for
		// client.cpp's timeleft command (Think does the same every frame);
		// mp_maxrounds and mp_winlimit are read as the constructor reads them
		// Counter-Strike 1.3: the cvar's own value (1.1 read it by name here)
		if ( timelimit.value < 0 )
			CVAR_SET_FLOAT( "mp_timelimit", 0 );

		g_flResetTime = gpGlobals->time;

		if ( timelimit.value != 0 )
			g_flTimeLimit = gpGlobals->time + timelimit.value * 60;

		m_iTotalRoundsPlayed = 0;
		m_iMaxRounds = (int)CVAR_GET_FLOAT( "mp_maxrounds" );

		if ( m_iMaxRounds < 0 )
		{
			m_iMaxRounds = 0;
			CVAR_SET_FLOAT( "mp_maxrounds", 0 );
		}

		m_iMaxRoundsWon = (int)CVAR_GET_FLOAT( "mp_winlimit" );

		if ( m_iMaxRoundsWon < 0 )
		{
			m_iMaxRoundsWon = 0;
			CVAR_SET_FLOAT( "mp_winlimit", 0 );
		}

		m_iNumTerroristWins = 0;
		m_iNumCTWins = 0;
		m_iNumConsecutiveTerroristLoses = 0;
		m_iNumConsecutiveCTLoses = 0;

		UpdateTeamScores();

		// Beta 5.0: and every player starts afresh (money, score, items: CBasePlayer::Reset)
		for ( int i = 1; i <= gpGlobals->maxClients; i++ )
		{
			CBasePlayer *plr = (CBasePlayer *)UTIL_PlayerByIndex( i );	// name ours

			if ( plr && !FNullEnt( plr->pev ) )
				plr->Reset();
		}
	}

	m_bFreezePeriod = TRUE;
	m_fRoundTerminating = false;

	// Beta 5.0: the cvars as the constructor reads them; the idle limit from the clamped round time
	ReadMultiplayCvars( this );

	m_flMaxIdlePeriod = m_iRoundTime * 2;	// (Counter-Strike 1.1: the round time is in seconds)
	m_iRoundTimeSecs = m_iFreezeTime;

	// Beta 6.0: the map's info_map_parameters say who may buy and the bomb radius
	CMapInfo *mi = (CMapInfo *)UTIL_FindEntityByClassname( NULL, "info_map_parameters" );	// name ours

	if ( mi )
	{
		switch ( mi->m_iBuyingStatus )
		{
		case 0:
			m_bCTCantBuy = false;
			m_bTCantBuy = false;
			ALERT( at_console, "EVERYONE CAN BUY!\n" );
			break;
		case 1:
			m_bCTCantBuy = false;
			m_bTCantBuy = true;
			ALERT( at_console, "Only CT's can buy!!\n" );
			break;
		case 2:
			m_bCTCantBuy = true;
			m_bTCantBuy = false;
			ALERT( at_console, "Only T's can buy!!\n" );
			break;
		case 3:
			m_bCTCantBuy = true;
			m_bTCantBuy = true;
			ALERT( at_console, "No one can buy!!\n" );
			break;
		default:
			m_bCTCantBuy = false;
			m_bTCantBuy = false;
			break;
		}

		m_flBombRadius = mi->m_flBombRadius;
	}

	if ( UTIL_FindEntityByClassname( NULL, "func_bomb_target" ) )
	{
		m_fMapHasBombTarget = true;
		m_fMapHasBombZone = true;
	}
	else if ( UTIL_FindEntityByClassname( NULL, "info_bomb_target" ) )
	{
		m_fMapHasBombTarget = true;
		m_fMapHasBombZone = false;
	}
	else
	{
		m_fMapHasBombTarget = false;
		m_fMapHasBombZone = false;
	}

	if ( UTIL_FindEntityByClassname( NULL, "func_hostage_rescue" ) )
		m_fMapHasRescueZone = true;
	else
		m_fMapHasRescueZone = false;

	if ( UTIL_FindEntityByClassname( NULL, "func_buyzone" ) )
		m_fMapHasBuyZone = true;
	else
		m_fMapHasBuyZone = false;

	// Beta 6.0: an escape map counts its escapers afresh; the teams swap
	// every three rounds (Beta 6.5; eight before)
	if ( UTIL_FindEntityByClassname( NULL, "func_escapezone" ) )
	{
		m_bMapHasEscapeZone = true;
		m_iHaveEscaped = 0;
		m_iNumEscapers = 0;

		if ( m_iNumEscapeRounds >= 3 )
		{
			SwapAllPlayers();
			m_iNumEscapeRounds = 0;
		}

		m_iNumEscapeRounds++;
	}
	else
		m_bMapHasEscapeZone = false;

	// Beta 6.0: an assassination map gets its VIP
	if ( UTIL_FindEntityByClassname( NULL, "func_vip_safetyzone" ) )
	{
		PickNextVIP();
		m_iConsecutiveVIP++;
		m_iMapHasVIPSafetyZone = 1;
	}
	else
		m_iMapHasVIPSafetyZone = 2;

	CBaseEntity *pHostage = UTIL_FindEntityByClassname( NULL, "hostage_entity" );
	int iHostageBonus = 0;	// name ours

	while ( pHostage != NULL && iHostageBonus < 2000 )
	{
		if ( pHostage->pev->solid != SOLID_NOT )
		{
			// a hostage still standing in the map (Beta 5.0: 150, the leak 250)
			iHostageBonus += 150;

			if ( pHostage->pev->deadflag == DEAD_DEAD )
				pHostage->pev->deadflag = DEAD_RESPAWNABLE;
		}
		((CHostage *)pHostage)->RePosition();
		pHostage = UTIL_FindEntityByClassname( pHostage, "hostage_entity" );
	}

	// the loser bonus grows after two losses in a row (Beta 6.5: it starts
	// again at 1,500 and stops at 3,000; until 6.1 after three, 1,400, 2,900)
	if ( m_iRoundWinStatus == 2 )
	{
		if ( m_iNumConsecutiveTerroristLoses > 1 )
			m_iLoserBonus = 1500;

		m_iNumConsecutiveTerroristLoses = 0;
		m_iNumConsecutiveCTLoses++;
	}
	else if ( m_iRoundWinStatus == 1 )
	{
		if ( m_iNumConsecutiveCTLoses > 1 )
			m_iLoserBonus = 1500;

		m_iNumConsecutiveCTLoses = 0;
		m_iNumConsecutiveTerroristLoses++;
	}

	if ( ( m_iNumConsecutiveTerroristLoses > 1 && m_iLoserBonus < 3000 ) || ( m_iNumConsecutiveCTLoses > 1 && m_iLoserBonus < 3000 ) )
		m_iLoserBonus += 500;

	if ( m_iRoundWinStatus == 2 )
	{
		m_iAccountTerrorist += iHostageBonus;
		m_iAccountCT += m_iLoserBonus;
	}
	else if ( m_iRoundWinStatus == 1 )
	{
		m_iAccountCT += iHostageBonus;

		// Beta 6.0: no loser bonus for the terrorists of an escape map
		if ( m_bMapHasEscapeZone == false )
			m_iAccountTerrorist += m_iLoserBonus;
	}

	m_iAccountCT += m_iHostagesRescued * 750;	// Beta 6.5: 750 a rescued hostage (5.0 to 6.1: 250; the leak 150)

	m_flRoundStartTime2 = m_flRoundStartTime = gpGlobals->time;

	// Counter-Strike 1.3: a complete reset clears the accounts worked out above and the losing streaks
	if ( m_fCompleteReset )
	{
		m_iAccountCT = 0;	// (Counter-Strike 1.4: the accounts first)
		m_iAccountTerrorist = 0;
		m_iNumTerroristWins = 0;
		m_iNumCTWins = 0;
		m_iNumConsecutiveTerroristLoses = 0;
		m_iNumConsecutiveCTLoses = 0;
		m_iLoserBonus = 1400;
	}

	CBaseEntity *pPlayer = UTIL_FindEntityByClassname( NULL, "player" );

	while ( pPlayer != NULL && !FNullEnt( pPlayer->edict() ) )
	{
		if ( pPlayer->pev->flags != FL_DORMANT )
		{
			CBasePlayer *player = GetClassPtr( (CBasePlayer *)pPlayer->pev );

			// Counter-Strike 1.4: the spawns are counted from here, and everybody may change teams
			// again; the account is paid ahead of the respawn (1.3: behind it)
			player->m_iNumSpawns = 0;
			player->m_fTeamChanged = false;

			if ( player->m_iTeam == TEAM_CT )
				player->AddAccount( m_iAccountCT, true );
			else if ( player->m_iTeam == TEAM_TERRORIST )
			{
				// Beta 6.0: the escapers are counted; on an escape map the
				// terrorists start with the starting weapons every round
				// (Counter-Strike 1.4: by losing what they carry at the respawn;
				// to 1.3 GiveDefaultItems was called here)
				m_iNumEscapers++;
				player->AddAccount( m_iAccountTerrorist, true );

				if ( m_bMapHasEscapeZone == true )
					player->m_fNotKilled = false;
			}

			// (Beta 5.0 to 1.3: after a complete reset GiveDefaultItems here)
			if ( player->m_iTeam != TEAM_UNASSIGNED && player->m_iTeam != TEAM_SPECTATOR )	// (Counter-Strike 1.1: nor the spectators)
			{
				if ( player->m_bHasC4 == true )
					player->DropPlayerItem( "weapon_c4" );

				player->RoundRespawn();
			}
		}
		pPlayer = UTIL_FindEntityByClassname( pPlayer, "player" );
	}

	CleanUpMap();

	if ( m_fMapHasBombTarget == true )
		GiveC4();

	m_flIntermissionEndTime = 0;
	m_flIntermissionStartTime = 0;	// Counter-Strike 1.1
	m_flRestartRoundTime = 0;
	m_iAccountCT = 0;
	m_iAccountTerrorist = 0;
	m_iHostagesRescued = 0;
	m_iUnused2C = 0;
	m_iRoundWinStatus = 0;
	m_fBombDefused = false;
	m_fTargetBombed = false;
	m_fCountedSpawnPoints = false;
	m_fCompleteReset = false;
}

//=========================================================
// IsThereABomber - Counter-Strike 1.4 (Linux symbol; local's
// name Counter-Strike 1.6's): a player who is no
// counter-terrorist carries the C4
//=========================================================
BOOL CHalfLifeMultiplay::IsThereABomber( void )
{
	CBasePlayer *pPlayer = (CBasePlayer *)UTIL_FindEntityByClassname( NULL, "player" );

	while ( pPlayer != NULL && !FNullEnt( pPlayer->edict() ) )
	{
		if ( pPlayer->m_iTeam != TEAM_CT && pPlayer->IsBombGuy() )
			return TRUE;

		pPlayer = (CBasePlayer *)UTIL_FindEntityByClassname( pPlayer, "player" );
	}

	return FALSE;
}

//=========================================================
// IsThereABomb - Counter-Strike 1.4 (Linux symbol; locals'
// names Counter-Strike 1.6's): a planted C4 or a weapon_c4
// exists. sic: the walk does not step past a planted C4, so
// with one in the map the loop does not end (both libraries)
//=========================================================
BOOL CHalfLifeMultiplay::IsThereABomb( void )
{
	CGrenade *pC4 = NULL;
	CBaseEntity *pWeaponC4 = NULL;
	BOOL bFoundBomb = FALSE;

	pC4 = (CGrenade *)UTIL_FindEntityByClassname( NULL, "grenade" );

	while ( pC4 )
	{
		if ( !pC4->m_bIsC4 )
			pC4 = (CGrenade *)UTIL_FindEntityByClassname( pC4, "grenade" );
		else
			bFoundBomb = TRUE;
	}

	if ( !bFoundBomb )
	{
		pWeaponC4 = UTIL_FindEntityByClassname( NULL, "weapon_c4" );

		if ( pWeaponC4 )
			bFoundBomb = TRUE;
	}

	return bFoundBomb;
}

// longest the intermission can last, in seconds
#define MAX_INTERMISSION_TIME		120

extern cvar_t mp_chattime;	// (SDK 2.2's line; Counter-Strike 1.1)
// Counter-Strike 1.3: Think reads these cvars' own values (1.6 declares them here, ahead of Think)
extern cvar_t restartround;
extern cvar_t sv_restart;	// Counter-Strike 1.4
extern cvar_t mapvoteratio;	// Counter-Strike 1.4: ProcessMapVote reads the cvar's own value
extern cvar_t maxrounds;
extern cvar_t winlimit;

//=========================================================
// TeamFull - Beta 5.0 (leak): a team has as many players as
// the map has spawn points for it (Linux symbol)
//=========================================================
BOOL CHalfLifeMultiplay::TeamFull( int team_id )
{
	switch ( team_id )
	{
	case TEAM_TERRORIST:
		return ( m_iNumTerrorist >= m_iSpawnPointCount_Terrorist );

	case TEAM_CT:
		return ( m_iNumCT >= m_iSpawnPointCount_CT );
	}

	return FALSE;
}

//=========================================================
// TeamStacked - Beta 5.0 (Linux symbol): joining newTeam_id
// would leave it more than mp_limitteams players ahead of
// the other team (a player changing teams leaves the other
// one, so one fewer is allowed)
//=========================================================
BOOL CHalfLifeMultiplay::TeamStacked( int newTeam_id, int curTeam_id )
{
	// already in that team
	if ( newTeam_id == curTeam_id )
		return FALSE;

	switch ( newTeam_id )
	{
	// (Counter-Strike 1.1: a spectator leaves no team either)
	case TEAM_TERRORIST:
		if ( curTeam_id != TEAM_UNASSIGNED && curTeam_id != TEAM_SPECTATOR )
			return ( m_iNumTerrorist + 1 ) > ( m_iLimitTeams + m_iNumCT - 1 );
		else
			return ( m_iNumTerrorist + 1 ) > ( m_iLimitTeams + m_iNumCT );

	case TEAM_CT:
		if ( curTeam_id != TEAM_UNASSIGNED && curTeam_id != TEAM_SPECTATOR )
			return ( m_iNumCT + 1 ) > ( m_iLimitTeams + m_iNumTerrorist - 1 );
		else
			return ( m_iNumCT + 1 ) > ( m_iLimitTeams + m_iNumTerrorist );
	}

	return FALSE;
}

//=========================================================
// The VIP of an assassination map (Beta 6.0, Linux symbols):
// counter-terrorists queue to be the VIP (five places; the
// VIP queue key of the team menu, become_vip); a new round
// takes the first in line, else after three rounds the next
// counter-terrorist in turn (Beta 6.5; a random one before),
// else (no VIP yet) the first one.
//
// StackVIPQueue - closes the gaps in the queue (one pass from
// the back)
//=========================================================
void CHalfLifeMultiplay::StackVIPQueue( void )
{
	for ( int i = 3; i > 0; i-- )
	{
		if ( m_pVIPQueue[i - 1] == NULL )
		{
			m_pVIPQueue[i - 1] = m_pVIPQueue[i];
			m_pVIPQueue[i] = m_pVIPQueue[i + 1];
			m_pVIPQueue[i + 1] = NULL;
		}
		else if ( m_pVIPQueue[i] == NULL )
		{
			m_pVIPQueue[i] = m_pVIPQueue[i + 1];
			m_pVIPQueue[i + 1] = NULL;
		}
	}
}

//=========================================================
// IsVIPQueueEmpty - terrorists leave the queue first
// (Counter-Strike 1.4: whoever is no counter-terrorist)
//=========================================================
bool CHalfLifeMultiplay::IsVIPQueueEmpty( void )
{
	for ( int i = 0; i < 5; i++ )
	{
		CBasePlayer *toCheck = m_pVIPQueue[i];	// Counter-Strike 1.6's name

		if ( toCheck && toCheck->m_iTeam != TEAM_CT )
			m_pVIPQueue[i] = NULL;
	}

	StackVIPQueue();

	if ( m_pVIPQueue[0] == NULL && m_pVIPQueue[1] == NULL && m_pVIPQueue[2] == NULL && m_pVIPQueue[3] == NULL && m_pVIPQueue[4] == NULL )
		return true;

	return false;
}

//=========================================================
// AddToVIPQueue - a counter-terrorist takes the first free
// place, and is told which
//=========================================================
bool CHalfLifeMultiplay::AddToVIPQueue( CBasePlayer *toAdd )
{
	int i;

	IsVIPQueueEmpty();

	if ( toAdd->m_iTeam != TEAM_CT )
		return false;

	for ( i = 0; i <= 4; i++ )
	{
		if ( m_pVIPQueue[i] == toAdd )
		{
			// Counter-Strike 1.1: titles of the client (1.0b: "You are already in position %i of 5\n")
			ClientPrint( toAdd->pev, HUD_PRINTCENTER, "#Game_in_position", UTIL_dtos1( i + 1 ) );
			return false;
		}
	}

	for ( i = 0; i <= 4; i++ )
	{
		if ( m_pVIPQueue[i] == NULL )
		{
			m_pVIPQueue[i] = toAdd;
			StackVIPQueue();

			ClientPrint( toAdd->pev, HUD_PRINTCENTER, "#Game_added_position", UTIL_dtos1( i + 1 ) );	// (1.0b: "You have been added to position %i of 5\n")
			return true;
		}
	}

	ClientPrint( toAdd->pev, HUD_PRINTCENTER, "#All_VIP_Slots_Full" );
	return false;
}

//=========================================================
// ResetCurrentVIP - the VIP becomes an ordinary counter-
// terrorist with a random model and loses what he carried
// over. Counter-Strike 1.0b: the model names are literals
// again (Beta 7.0 to 1.0 put them and four "model x"
// commands together from the letters at every call), the
// client is not sent a "model" command any more, and the
// user info buffer is taken once (infobuffer is
// Counter-Strike 1.6's name)
//=========================================================
void CHalfLifeMultiplay::ResetCurrentVIP( void )
{
	char *infobuffer = g_engfuncs.pfnGetInfoKeyBuffer( m_pVIP->edict() );

	switch ( RANDOM_LONG( 0, 3 ) )
	{
	case 0:
		m_pVIP->m_iAppearance = 1;
		g_engfuncs.pfnSetClientKeyValue( m_pVIP->entindex(), infobuffer, "model", "urban" );
		break;
	case 1:
		m_pVIP->m_iAppearance = 5;
		g_engfuncs.pfnSetClientKeyValue( m_pVIP->entindex(), infobuffer, "model", "gsg9" );
		break;
	case 2:
		m_pVIP->m_iAppearance = 7;
		g_engfuncs.pfnSetClientKeyValue( m_pVIP->entindex(), infobuffer, "model", "sas" );
		break;
	case 3:
		m_pVIP->m_iAppearance = 6;
		g_engfuncs.pfnSetClientKeyValue( m_pVIP->entindex(), infobuffer, "model", "gign" );
		break;
	default:	// Counter-Strike 1.0b: the same as case 0 (VC6 merges the two)
		m_pVIP->m_iAppearance = 1;
		g_engfuncs.pfnSetClientKeyValue( m_pVIP->entindex(), infobuffer, "model", "urban" );
		break;
	}

	m_pVIP->m_bIsVIP = false;
	m_pVIP->m_fNotKilled = false;
}

//=========================================================
// PickNextVIP - the round's VIP
//=========================================================
void CHalfLifeMultiplay::PickNextVIP( void )
{
	if ( IsVIPQueueEmpty() != true )
	{
		// the first in line
		if ( m_pVIP )
			ResetCurrentVIP();

		for ( int i = 0; i <= 4; i++ )
		{
			if ( m_pVIPQueue[i] )
			{
				m_pVIP = m_pVIPQueue[i];
				m_pVIP->MakeVIP();
				m_pVIPQueue[i] = NULL;
				StackVIPQueue();
				m_iConsecutiveVIP = 0;
				return;
			}
		}
	}
	else if ( m_iConsecutiveVIP >= 3 )
	{
		// after three rounds the next counter-terrorist in turn (Beta 6.5:
		// m_iLastPick counts through the team; until 6.1 a random one). When
		// that is the VIP himself, the player looked at before him
		m_iLastPick++;

		if ( m_iLastPick > m_iNumCT )
			m_iLastPick = 1;

		int iCount = 1;

		CBaseEntity *pPlayer = NULL;
		CBasePlayer *player = NULL;
		CBasePlayer *pLastPlayer = NULL;	// Counter-Strike 1.6's name

		pPlayer = UTIL_FindEntityByClassname( pPlayer, "player" );
		while ( pPlayer != NULL && !FNullEnt( pPlayer->edict() ) )
		{
			if ( !( pPlayer->pev->flags & FL_DORMANT ) )
			{
				player = GetClassPtr( (CBasePlayer *)pPlayer->pev );

				if ( player->m_iTeam == TEAM_CT && iCount == m_iLastPick )
				{
					if ( player == m_pVIP && pLastPlayer != NULL )
						player = pLastPlayer;

					if ( m_pVIP )
						ResetCurrentVIP();

					player->MakeVIP();
					m_iConsecutiveVIP = 0;
					return;
				}
				else if ( player->m_iTeam == TEAM_CT )
					iCount = iCount + 1;	// sic: the inliner's budget. With iCount++ the GetClassPtr above keeps Vector's constructor out of line (812 bytes for retail's 783); this form, the same on m_iLastPick, ENT( pPlayer->pev ) for edict() in the last arm's loop, or 2 to 20 dead member stores all give retail's code: the function's text is a little longer than its statements show

				if ( player->m_iTeam != TEAM_SPECTATOR )	// Counter-Strike 1.4: no spectator
					pLastPlayer = player;
			}
			pPlayer = UTIL_FindEntityByClassname( pPlayer, "player" );
		}
	}
	else if ( m_pVIP == NULL )
	{
		// the first counter-terrorist
		CBaseEntity *pPlayer = NULL;
		CBasePlayer *player = NULL;

		pPlayer = UTIL_FindEntityByClassname( pPlayer, "player" );
		while ( pPlayer != NULL && !FNullEnt( pPlayer->edict() ) )
		{
			if ( pPlayer->pev->flags != FL_DORMANT )
			{
				player = GetClassPtr( (CBasePlayer *)pPlayer->pev );

				if ( player->m_iTeam == TEAM_CT )
				{
					player->MakeVIP();
					m_iConsecutiveVIP = 0;
					return;
				}
			}
			pPlayer = UTIL_FindEntityByClassname( pPlayer, "player" );
		}
	}
}

//=========================================================
// Think - Beta 6.5 is built on SDK 2.0, but none of its
// Think is here: mp_fragsleft and mp_timeleft are registered
// (game.cpp) and never set
//=========================================================
void CHalfLifeMultiplay :: Think ( void )
{
	m_VoiceGameMgr.Update(gpGlobals->frametime);

	// Counter-Strike 1.4: the engine's client hull trace stays on
	if ( (int)CVAR_GET_FLOAT( "sv_clienttrace" ) != 1 )
		CVAR_SET_FLOAT( "sv_clienttrace", 1 );

	// Beta 5.0 (leak): the first think starts the clock
	if ( m_flRoundStartTime == 0 )
		m_flRoundStartTime2 = m_flRoundStartTime = gpGlobals->time;

	///// Check game rules /////

	if ( g_fGameOver )   // someone else quit the game already
	{
		// Counter-Strike 1.1 (SDK 2.2's text, the start time a member here): the
		// intermission lasts mp_chattime seconds, 10 to 120 (Counter-Strike 1.3: 1 to 120)
		// bounds check
		int time = (int)CVAR_GET_FLOAT( "mp_chattime" );
		if ( time < 1 )
			CVAR_SET_STRING( "mp_chattime", "1" );
		else if ( time > MAX_INTERMISSION_TIME )
			CVAR_SET_STRING( "mp_chattime", UTIL_dtos1( MAX_INTERMISSION_TIME ) );

		m_flIntermissionEndTime = m_flIntermissionStartTime + mp_chattime.value;

		// check to see if we should change levels now
		if ( m_flIntermissionEndTime < gpGlobals->time )
		{
			if ( m_iEndIntermissionButtonHit  // check that someone has pressed a key, or the max intermission time is over
				|| ( ( m_flIntermissionStartTime + MAX_INTERMISSION_TIME ) < gpGlobals->time) ) 
				ChangeLevel(); // intermission is over
		}
		return;
	}

	// Counter-Strike 1.1: the time limit counts from the last complete restart
	// (g_flResetTime; client.cpp's timeleft command reads g_flTimeLimit) and no
	// longer gives way to mp_maxrounds; a negative one is set to 0. The map also
	// ends when a team has won mp_winlimit rounds. (1.0b: with mp_maxrounds set
	// the time limit did not count; no console line for the time limit)
	float fTimeLimit = timelimit.value;	// Counter-Strike 1.3: the cvar's own value (1.1 read it by name)

	if ( fTimeLimit < 0 )
	{
		CVAR_SET_FLOAT( "mp_timelimit", 0 );
	}
	else if ( fTimeLimit != 0 )
	{
		g_flTimeLimit = g_flResetTime + fTimeLimit * 60;

		if ( gpGlobals->time >= g_flTimeLimit )
		{
			ALERT( at_console, "Changing maps because time limit has been met\n" );
			GoToIntermission();
			return;
		}
	}

	// Beta 7.1: with mp_maxrounds set, the map ends after so many rounds
	if ( m_iMaxRounds != 0 && m_iTotalRoundsPlayed >= m_iMaxRounds )
	{
		ALERT( at_console, "Changing maps due to maximum rounds have been met\n" );	// Counter-Strike 1.0
		GoToIntermission();
		return;
	}

	if ( m_iMaxRoundsWon != 0 && ( m_iNumCTWins >= m_iMaxRoundsWon || m_iNumTerroristWins >= m_iMaxRoundsWon ) )
	{
		ALERT( at_console, "Changing maps...one team has won the specified number of rounds\n" );
		GoToIntermission();
		return;
	}

	// Counter-Strike 1.1: the clients are told when allow_spectators changes
	if ( m_iStoredSpectValue != allow_spectators.value )
	{
		m_iStoredSpectValue = (int)allow_spectators.value;

		MESSAGE_BEGIN( MSG_ALL, gmsgAllowSpec );
			WRITE_BYTE( (int)allow_spectators.value );
		MESSAGE_END();
	}

	if ( IsFreezePeriod() )
	{
		// Beta 5.0 (leak): the freeze time is over: the round's clock
		// starts, with one of four radio calls (Beta 3.0 to 4.1 played it at
		// RestartRound). Beta 6.0: a radio message from one player of each
		// team (heard by his team), the escape and assassination maps' own
		if ( TimeRemaining() <= 0 )
		{
			char CT_sentence[40];	// (1.6's names)
			char T_sentence[40];

			UTIL_LogPrintf( "World triggered \"Round_Start\"\n" );	// Counter-Strike 1.1

			m_bFreezePeriod = FALSE;

			switch ( RANDOM_LONG( 0, 3 ) )
			{
			case 0:
				strcpy( CT_sentence, "%!MRAD_MOVEOUT" );
				strcpy( T_sentence, "%!MRAD_MOVEOUT" );
				break;
			case 1:
				strcpy( CT_sentence, "%!MRAD_LETSGO" );
				strcpy( T_sentence, "%!MRAD_LETSGO" );
				break;
			case 2:
				strcpy( CT_sentence, "%!MRAD_LOCKNLOAD" );
				strcpy( T_sentence, "%!MRAD_LOCKNLOAD" );
				break;
			default:
				strcpy( CT_sentence, "%!MRAD_GO" );
				strcpy( T_sentence, "%!MRAD_GO" );
				break;
			}

			if ( m_bMapHasEscapeZone == true )
			{
				strcpy( CT_sentence, "%!MRAD_ELIM" );
				strcpy( T_sentence, "%!MRAD_GETOUT" );
			}
			else if ( m_iMapHasVIPSafetyZone == 1 )
			{
				strcpy( CT_sentence, "%!MRAD_VIP" );
				strcpy( T_sentence, "%!MRAD_LOCKNLOAD" );
			}

			m_flRoundStartTime = gpGlobals->time;
			m_iRoundTimeSecs = m_iRoundTime;	// (Counter-Strike 1.1: in seconds already)

			bool bCTPlayed = false;	// (1.6's names)
			bool bTPlayed = false;

			for ( int i = 1; i <= gpGlobals->maxClients; i++ )
			{
				CBasePlayer *plr = (CBasePlayer *)UTIL_PlayerByIndex( i );

				// (Counter-Strike 1.1: not the spectators; 1.3: they get the round's timer too;
				// 1.4: and so does a player who is still joining)
				if ( plr && plr->pev->flags != FL_DORMANT )
				{
					if ( plr->m_iJoiningState == JOIN_DONE )
					{
						if ( plr->m_iTeam == TEAM_CT && !bCTPlayed )
						{
							plr->Radio( CT_sentence, NULL );
							bCTPlayed = true;
						}
						else if ( plr->m_iTeam == TEAM_TERRORIST && !bTPlayed )
						{
							plr->Radio( T_sentence, NULL );
							bTPlayed = true;
						}

						if ( plr->m_iTeam != TEAM_SPECTATOR )
						{
							plr->ResetMaxSpeed();
							plr->m_bCanShoot = true;	// Beta 6.5
						}
					}

					plr->SyncRoundTimer();
				}
			}
		}
	}
	else if ( TimeRemaining() <= 0 && m_iRoundWinStatus == 0 )	// Counter-Strike 1.3: only while the round is running
	{
		// the round's time is up. Beta 5.0: a ticking C4 no longer holds the
		// round (the leak let it end the round when it went off; the grenade
		// is still looked for, and nothing done with it). Beta 6.1: no more
		// draws; the side that defends the map's objective wins (2,000 to it,
		// none for an escape map), and nothing at all on a map without one.
		// Beta 7.0: the message follows the score
		CBaseEntity *pC4 = UTIL_FindEntityByClassname( NULL, "grenade" );

		if ( m_fMapHasBombTarget == true )
		{
			Broadcast( "ctwin" );
			m_iAccountCT += 2000;
			m_iNumCTWins++;

			EndRoundMessage( "#Target_Saved", 12 );
			TerminateRound( 5, 1 );
			UpdateTeamScores();
		}
		else if ( UTIL_FindEntityByClassname( NULL, "hostage_entity" ) != NULL )
		{
			Broadcast( "terwin" );
			m_iAccountTerrorist += 2000;
			m_iNumTerroristWins++;

			EndRoundMessage( "#Hostages_Not_Rescued", 13 );
			TerminateRound( 5, 2 );
			UpdateTeamScores();
		}
		else if ( m_bMapHasEscapeZone == true )
		{
			Broadcast( "ctwin" );
			m_iNumCTWins++;

			EndRoundMessage( "#Terrorists_Not_Escaped", 14 );
			TerminateRound( 5, 1 );
			UpdateTeamScores();
		}
		else if ( m_iMapHasVIPSafetyZone == 1 )
		{
			Broadcast( "terwin" );
			m_iAccountTerrorist += 2000;
			m_iNumTerroristWins++;

			EndRoundMessage( "#VIP_Not_Escaped", 15 );
			TerminateRound( 5, 2 );
			UpdateTeamScores();
		}

		m_flRoundStartTime = gpGlobals->time + 60;
	}

	if ( m_flRestartRoundTime != 0.0 && m_flRestartRoundTime <= gpGlobals->time )
		RestartRound();

	// Beta 5.0 (leak): the spawn points per team, once a round (TeamFull)
	if ( !m_fCountedSpawnPoints )
	{
		CBaseEntity *ent;	// name ours

		m_iSpawnPointCount_Terrorist = 0;
		m_iSpawnPointCount_CT = 0;

		ent = UTIL_FindEntityByClassname( NULL, "info_player_deathmatch" );
		while ( ent )
		{
			m_iSpawnPointCount_Terrorist++;
			ent = UTIL_FindEntityByClassname( ent, "info_player_deathmatch" );
		}

		ent = UTIL_FindEntityByClassname( ent, "info_player_start" );
		while ( ent )
		{
			m_iSpawnPointCount_CT++;
			ent = UTIL_FindEntityByClassname( ent, "info_player_start" );
		}

		m_fCountedSpawnPoints = true;
	}

	// Beta 5.0 (leak): sv_restartround N restarts the round in N seconds
	// (at most 10), the scores zeroed. Beta 5.0: "The game will restart in N
	// SECOND(S)", on the screen and in every console.
	// Counter-Strike 1.1: the client's title "#Game_will_restart_in" with the
	// number and the unit; the restart and the teams' scores go to the log
	// (1.0b sent the text through EndRoundMessage)
	if ( gpGlobals->time > m_flNextRestartCheck )
	{
		int iRestartDelay = (int)restartround.value;	// (1.6's name; Counter-Strike 1.3: the cvar's own value)

		// Counter-Strike 1.4: sv_restart does the same
		if ( !iRestartDelay )
			iRestartDelay = (int)sv_restart.value;

		if ( iRestartDelay > 0 )
		{
			if ( iRestartDelay > 10 )
				iRestartDelay = 10;

			UTIL_LogPrintf( "World triggered \"Restart_Round_(%i_%s)\"\n", iRestartDelay, ( iRestartDelay == 1 ) ? "second" : "seconds" );

			CHalfLifeMultiplay *mp = (CHalfLifeMultiplay *)g_pGameRules;	// (1.6's name)

			if ( mp )
			{
				UTIL_LogPrintf( "Team \"CT\" scored \"%i\" with \"%i\" players\n", mp->m_iNumCTWins, mp->m_iNumCT );
				UTIL_LogPrintf( "Team \"TERRORIST\" scored \"%i\" with \"%i\" players\n", mp->m_iNumTerroristWins, mp->m_iNumTerrorist );
			}

			UTIL_ClientPrintAll( HUD_PRINTCENTER, "#Game_will_restart_in", UTIL_dtos1( iRestartDelay ), ( iRestartDelay == 1 ) ? "SECOND" : "SECONDS" );
			UTIL_ClientPrintAll( HUD_PRINTCONSOLE, "#Game_will_restart_in", UTIL_dtos1( iRestartDelay ), ( iRestartDelay == 1 ) ? "SECOND" : "SECONDS" );
			m_flRestartRoundTime = gpGlobals->time + iRestartDelay;
			m_fCompleteReset = true;
			CVAR_SET_FLOAT( "sv_restartround", 0 );
			CVAR_SET_FLOAT( "sv_restart", 0 );
		}

		m_flNextRestartCheck = gpGlobals->time + 1.0;

		// Beta 6.1: and the movement settings are put back whenever changed
		// (Counter-Strike 1.3: read through game.cpp's pointers)
		if ( g_psv_accelerate->value != 5.0 )
			CVAR_SET_FLOAT( "sv_accelerate", 5 );

		if ( g_psv_friction->value != 4.0 )
			CVAR_SET_FLOAT( "sv_friction", 4 );

		if ( g_psv_stopspeed->value != 75 )
			CVAR_SET_FLOAT( "sv_stopspeed", 75 );

		m_iMaxRounds = (int)maxrounds.value;	// Beta 7.1

		// Counter-Strike 1.1: as the constructor
		if ( m_iMaxRounds < 0 )
		{
			m_iMaxRounds = 0;
			CVAR_SET_FLOAT( "mp_maxrounds", 0 );
		}

		m_iMaxRoundsWon = (int)winlimit.value;

		if ( m_iMaxRoundsWon < 0 )
		{
			m_iMaxRoundsWon = 0;
			CVAR_SET_FLOAT( "mp_winlimit", 0 );
		}
	}

	// Counter-Strike 1.3: no stand-in here. From Beta 6.6 to Counter-Strike 1.1 empty
	// statements stood at this place (116 in Beta 7.1, 154 in 1.0 and 1.0b, 122 in
	// 1.1) for intermediate code the image could not give: they placed gamerules.h's
	// inline functions among this file's own. Retail 1.3's object has all of those
	// inlines between Think and IsMultiplayer, and the release build is retail's
	// file with 0, 122 and 300 statements alike (three full builds, SHA-256), so
	// the image no longer shows a number and none is kept.
}


//=========================================================
//=========================================================
BOOL CHalfLifeMultiplay::IsMultiplayer( void )
{
	return TRUE;
}

//=========================================================
//=========================================================
BOOL CHalfLifeMultiplay::IsDeathmatch( void )
{
	return TRUE;
}

//=========================================================
//=========================================================
BOOL CHalfLifeMultiplay::IsCoOp( void )
{
	return gpGlobals->coop;
}

//=========================================================
//=========================================================
BOOL CHalfLifeMultiplay::FShouldSwitchWeapon( CBasePlayer *pPlayer, CBasePlayerItem *pWeapon )
{
	if ( !pWeapon->CanDeploy() )
	{
		// that weapon can't deploy anyway.
		return FALSE;
	}

	if ( !pPlayer->m_pActiveItem )
	{
		// player doesn't have an active item!
		return TRUE;
	}

	// Counter-Strike 1.4: the player's choice (userinfo "_cl_autowepswitch")
	if ( !pPlayer->m_iAutoWepSwitch )
		return FALSE;

	if ( !pPlayer->m_pActiveItem->CanHolster() )
	{
		// can't put away the active item.
		return FALSE;
	}

	if ( pWeapon->iWeight() > pPlayer->m_pActiveItem->iWeight() )
	{
		return TRUE;
	}

	return FALSE;
}

BOOL CHalfLifeMultiplay :: GetNextBestWeapon( CBasePlayer *pPlayer, CBasePlayerItem *pCurrentWeapon )
{

	CBasePlayerItem *pCheck;
	CBasePlayerItem *pBest;// this will be used in the event that we don't find a weapon in the same category.
	int iBestWeight;
	int i;

	iBestWeight = -1;// no weapon lower than -1 can be autoswitched to
	pBest = NULL;

	if ( !pCurrentWeapon->CanHolster() )
	{
		// can't put this gun away right now, so can't switch.
		return FALSE;
	}

	for ( i = 0 ; i < MAX_ITEM_TYPES ; i++ )
	{
		pCheck = pPlayer->m_rgpPlayerItems[ i ];

		while ( pCheck )
		{
			// (Counter-Strike 1.4: the SDK's first arm, a weapon of the current one's weight, is gone)
			if ( pCheck->iWeight() > iBestWeight && pCheck != pCurrentWeapon )// don't reselect the weapon we're trying to get rid of
			{
				//ALERT ( at_console, "Considering %s\n", STRING( pCheck->pev->classname ) );
				// we keep updating the 'best' weapon just in case we can't find a weapon of the same weight
				// that the player was using. This will end up leaving the player with his heaviest-weighted 
				// weapon. 
				if ( pCheck->CanDeploy() )
				{
					// if this weapon is useable, flag it as the best
					iBestWeight = pCheck->iWeight();
					pBest = pCheck;
				}
			}

			pCheck = pCheck->m_pNext;
		}
	}

	// if we make it here, we've checked all the weapons and found no useable 
	// weapon in the same catagory as the current weapon. 
	
	// if pBest is null, we didn't find ANYTHING. Shouldn't be possible- should always 
	// at least get the crowbar, but ya never know.
	if ( !pBest )
	{
		return FALSE;
	}

	pPlayer->SwitchWeapon( pBest );

	return TRUE;
}

// ClientCommand_DeadOrAlive - Counter-Strike 1.3 (Linux symbol): the voice manager's commands (vban, VModEnable),
// which a dead player may give too; the SDK has them in ClientCommand
BOOL CHalfLifeMultiplay::ClientCommand_DeadOrAlive( CBasePlayer *pPlayer, const char *pcmd )
{
	if(m_VoiceGameMgr.ClientCommand(pPlayer, pcmd))
		return TRUE;

	return FALSE;
}

BOOL CHalfLifeMultiplay::ClientCommand( CBasePlayer *pPlayer, const char *pcmd )
{
	return FALSE;
}

//=========================================================
//=========================================================
BOOL CHalfLifeMultiplay :: ClientConnected( edict_t *pEntity, const char *pszName, const char *pszAddress, char szRejectReason[ 128 ] )
{
	m_VoiceGameMgr.ClientConnected(pEntity);

	// Counter-Strike 1.4: the team scores to the connecting client
	MESSAGE_BEGIN( MSG_ONE, gmsgTeamScore, NULL, pEntity );
		WRITE_STRING( "CT" );
		WRITE_SHORT( m_iNumCTWins );
	MESSAGE_END();

	MESSAGE_BEGIN( MSG_ONE, gmsgTeamScore, NULL, pEntity );
		WRITE_STRING( "TERRORIST" );
		WRITE_SHORT( m_iNumTerroristWins );
	MESSAGE_END();

	return TRUE;
}

extern int gmsgSayText;
extern int gmsgGameMode;
extern int gmsgViewMode;	// player.cpp (Counter-Strike 1.3)
extern int gmsgCheckModels;	// player.cpp (Counter-Strike 1.4)
extern float g_flWeaponCheat;	// world.cpp

void CHalfLifeMultiplay :: UpdateGameMode( CBasePlayer *pPlayer )
{
	MESSAGE_BEGIN( MSG_ONE, gmsgGameMode, NULL, pPlayer->edict() );
		WRITE_BYTE( 0 );  // game mode none
	MESSAGE_END();
}

void CHalfLifeMultiplay :: InitHUD( CBasePlayer *pl )
{
	// Beta 5.0 (leak): no "has joined the game" for the others any more
	// Counter-Strike 1.1: the standard log format's line, with the WON id (Counter-Strike 1.4: the auth id) and no team yet (1.0b: "\"%s<%i>\" has entered the game\n")
	UTIL_LogPrintf( "\"%s<%i><%s><>\" entered the game\n",  STRING( pl->pev->netname ), GETPLAYERUSERID( pl->edict() ), GETPLAYERAUTHID( pl->edict() ) );

	UpdateGameMode( pl );

	// Counter-Strike 1.3: without sv_cheats the client is told to keep its view (message ViewMode)
	if ( !g_flWeaponCheat )
	{
		MESSAGE_BEGIN( MSG_ONE, gmsgViewMode, NULL, pl->edict() );
		MESSAGE_END();
	}

	// Counter-Strike 1.4: the client looks its player models over (message CheckModels)
	MESSAGE_BEGIN( MSG_ONE, gmsgCheckModels, NULL, pl->edict() );
	MESSAGE_END();

	// sending just one score makes the hud scoreboard active;  otherwise
	// it is just disabled for single play
	MESSAGE_BEGIN( MSG_ONE, gmsgScoreInfo, NULL, pl->edict() );
		WRITE_BYTE( ENTINDEX(pl->edict()) );
		WRITE_SHORT( 0 );
		WRITE_SHORT( 0 );
		WRITE_SHORT( 0 );				// Beta 6.6: two more shorts, a zero and the team
		WRITE_SHORT( pl->m_iTeam );
	MESSAGE_END();

	SendMOTDToClient( pl->edict() );

	// loop through all active players and send their score info to the new client
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		// FIXME:  Probably don't need to cast this just to read m_iDeaths
		CBasePlayer *plr = (CBasePlayer *)UTIL_PlayerByIndex( i );

		if ( plr )
		{
			MESSAGE_BEGIN( MSG_ONE, gmsgScoreInfo, NULL, pl->edict() );
				WRITE_BYTE( i );	// client number
				WRITE_SHORT( plr->pev->frags );
				WRITE_SHORT( plr->m_iDeaths );
				WRITE_SHORT( 0 );
				WRITE_SHORT( plr->m_iTeam );	// Beta 7.0 (6.6 sent the new client's own team for every player)
			MESSAGE_END();
		}
	}

	// Beta 5.0 (leak): the teams' scores (Beta 6.5: by team name)
	MESSAGE_BEGIN( MSG_ONE, gmsgTeamScore, NULL, pl->edict() );
		WRITE_STRING( "TERRORIST" );
		WRITE_SHORT( m_iNumTerroristWins );
	MESSAGE_END();

	MESSAGE_BEGIN( MSG_ONE, gmsgTeamScore, NULL, pl->edict() );
		WRITE_STRING( "CT" );
		WRITE_SHORT( m_iNumCTWins );
	MESSAGE_END();

	// Counter-Strike 1.1: whether the team menu offers the spectators
	MESSAGE_BEGIN( MSG_ONE, gmsgAllowSpec, NULL, pl->edict() );
		WRITE_BYTE( (int)allow_spectators.value );
	MESSAGE_END();

	if ( g_fGameOver )
	{
		MESSAGE_BEGIN( MSG_ONE, SVC_INTERMISSION, NULL, pl->edict() );
		MESSAGE_END();
	}

	// Beta 5.0 (leak): every player's team and scoreboard flags (Beta 7.0:
	// the defuse kit icon is no longer cleared here)
	for ( i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBasePlayer *plr = (CBasePlayer *)UTIL_PlayerByIndex( i );

		if ( plr )
		{
			// Beta 6.5: the team's name (a team number until 6.1)
			MESSAGE_BEGIN( MSG_ONE, gmsgTeamInfo, NULL, pl->edict() );
				WRITE_BYTE( plr->entindex() );
				if ( plr->m_iTeam == TEAM_CT )
					WRITE_STRING( "CT" );
				else if ( plr->m_iTeam == TEAM_TERRORIST )
					WRITE_STRING( "TERRORIST" );
				else if ( plr->m_iTeam == TEAM_SPECTATOR )	// Counter-Strike 1.1
					WRITE_STRING( "SPECTATOR" );
				else
					WRITE_STRING( "UNASSIGNED" );
			MESSAGE_END();

			plr->SetScoreboardAttributes( pl );
		}
	}
}

//=========================================================
//=========================================================
void CHalfLifeMultiplay :: ClientDisconnected( edict_t *pClient )
{
	// (Beta 6.5: the night vision's "lambert 1.5" is no longer sent here)
	if ( pClient )
	{
		CBasePlayer *pPlayer = (CBasePlayer *)CBaseEntity::Instance( pClient );

		if ( pPlayer )
		{
			// Beta 6.5: he is gone for the observers and the scoreboard
			pPlayer->has_disconnected = true;
			pPlayer->pev->deadflag = DEAD_DEAD;
			pPlayer->SetScoreboardAttributes();

			// Beta 5.0 (leak): he drops the C4; his defuse kit stays where he was
			if ( pPlayer->m_bHasC4 == true )
				pPlayer->DropPlayerItem( "weapon_c4" );

			// (Counter-Strike 1.4: DropPlayerItem; to 1.3 GiveNamedItem made a new kit)
			if ( pPlayer->m_fHasDefuser == true )
				pPlayer->DropPlayerItem( "item_thighpack" );

			// Beta 6.0: a VIP leaving leaves no VIP
			if ( pPlayer->m_bIsVIP == true )
				m_pVIP = NULL;

			pPlayer->m_iCurrentKickVote = 0;	// Counter-Strike 1.4

			// Beta 6.5: his map vote goes with him. Counter-Strike 1.4: any vote
			// but 0 (to 1.3: 1 to 100), and the count does not go below zero
			if ( pPlayer->m_iMapVote )
			{
				m_iMapVotes[ pPlayer->m_iMapVote - 1 ]--;

				if ( m_iMapVotes[ pPlayer->m_iMapVote - 1 ] < 0 )
					m_iMapVotes[ pPlayer->m_iMapVote - 1 ] = 0;
			}

			FireTargets( "game_playerleave", pPlayer, pPlayer, USE_TOGGLE, 0 );

			// Counter-Strike 1.1: with the WON id and the team (team is 1.6's name)
			char *team = GetTeam( pPlayer->m_iTeam );
			UTIL_LogPrintf( "\"%s<%i><%s><%s>\" disconnected\n",  STRING( pPlayer->pev->netname ), GETPLAYERUSERID( pPlayer->edict() ), GETPLAYERAUTHID( pPlayer->edict() ), team );

			pPlayer->RemoveAllItems( TRUE );// destroy all of the players weapons and items

			// Beta 5.0 (leak): his observer goes
			if ( pPlayer->m_pObserver )
				pPlayer->m_pObserver->SUB_Remove();

			// Beta 6.5: the players who watched him. The locals' names are
			// Counter-Strike 1.6's (client, and iMode twelve lines below it
			// there, as here); no SDK text to 2.3 has this loop
			CBasePlayer *client = NULL;
			while ( ((client = (CBasePlayer*)UTIL_FindEntityByClassname( client, "player" )) != NULL) && (!FNullEnt(client->edict())) ) 
			{
				if ( !client->pev )
					continue;
				if ( client == pPlayer )
					continue;

				// an observer of this player: his camera mode is cleared and set again (Observer_SetMode)
				if ( client->m_hObserverTarget == pPlayer )
				{
					int iMode = client->pev->iuser1;
					client->pev->iuser1 = 0;
					client->Observer_SetMode( iMode );
				}
			}
		}
	}

	CheckWinConditions();
}

//=========================================================
//=========================================================
float CHalfLifeMultiplay :: FlPlayerFallDamage( CBasePlayer *pPlayer )
{
	// always progressive now, at 1.25 times the single-player rate (Beta 1.1 to 2.1:
	// twice; 3.0 and 3.1: 1.25, with a console line; 4.0 and 4.1: 1.75);
	// mp_falldamage is no longer read
	pPlayer->m_flFallVelocity -= PLAYER_MAX_SAFE_FALL_SPEED;
	return pPlayer->m_flFallVelocity * DAMAGE_FOR_FALL_SPEED * 1.25;	// Beta 5.0 (leak): 1.25
} 

//=========================================================
//=========================================================
BOOL CHalfLifeMultiplay::FPlayerCanTakeDamage( CBasePlayer *pPlayer, CBaseEntity *pAttacker )
{
	// teamplay_gamerules.cpp's test, here for the two teams of Counter-Strike
	if ( pAttacker && PlayerRelationship( pPlayer, pAttacker ) == GR_TEAMMATE )
	{
		// my teammate hit me.
		if ( (CVAR_GET_FLOAT("mp_friendlyfire") == 0) && (pAttacker != pPlayer) )
		{
			// friendly fire is off, and this hit came from someone other than myself,  then don't get hurt
			return FALSE;
		}
	}

	return TRUE;
}

//=========================================================
//=========================================================
void CHalfLifeMultiplay :: PlayerThink( CBasePlayer *pPlayer )
{
	if ( g_fGameOver )
	{
		// check for button presses
		if ( pPlayer->m_afButtonPressed & ( IN_DUCK | IN_ATTACK | IN_ATTACK2 | IN_USE | IN_JUMP ) )
			m_iEndIntermissionButtonHit = TRUE;

		// clear attack/use commands from player
		pPlayer->m_afButtonPressed = 0;
		pPlayer->pev->button = 0;
		pPlayer->m_afButtonReleased = 0;
	}

	// Beta 6.6: outside the freeze time a player can shoot
	if ( !pPlayer->m_bCanShoot && !IsFreezePeriod() )
		pPlayer->m_bCanShoot = true;

	// Beta 5.0 (leak): a joining player gets the team menu
	if ( pPlayer->m_iMenu != Menu_ChooseTeam && pPlayer->m_iJoiningState == JOIN_TEAM_MENU )
	{
		// Beta 5.0: keys 1, 2 and 5 (the leak: 1 to 3); Beta 7.0: VGUI menu 2.
		// Counter-Strike 1.1: key 6, the spectators, with allow_spectators
		if ( allow_spectators.value != 0 )
			ShowVGUIMenu( pPlayer, 2, 0x33, "#Team_Select_Spect" );
		else
			ShowVGUIMenu( pPlayer, 2, 0x13, "#Team_Select" );

		pPlayer->m_iMenu = Menu_ChooseTeam;
		pPlayer->m_iJoiningState = JOIN_CHOOSING;
	}
}

//=========================================================
//=========================================================
void CHalfLifeMultiplay :: PlayerSpawn( CBasePlayer *pPlayer )
{
	BOOL		addDefault;
	CBaseEntity	*pWeaponEntity = NULL;

	// Beta 5.0 (leak): not while the player is still joining
	if ( pPlayer->m_fJustConnected )
		return;

	pPlayer->pev->weapons |= (1<<WEAPON_SUIT);
	
	addDefault = TRUE;

	while ( pWeaponEntity = UTIL_FindEntityByClassname( pWeaponEntity, "game_player_equip" ))
	{
		pWeaponEntity->Touch( pPlayer );
		addDefault = FALSE;
	}

	// Beta 3.0: a survivor of the last round keeps what he has. Beta 5.0: the
	// starting weapons are the player's (GiveDefaultItems; the leak gave the
	// knife and the team's pistol here)
	// Counter-Strike 1.4: and the VIP always gets his
	if ( ( pPlayer->m_fNotKilled != true && addDefault ) || pPlayer->m_bIsVIP )
		pPlayer->GiveDefaultItems();

	// Beta 5.0 (leak): the model the player chose (SetPlayerModel)
	pPlayer->SetPlayerModel( FALSE );
}

//=========================================================
//=========================================================
BOOL CHalfLifeMultiplay :: FPlayerCanRespawn( CBasePlayer *pPlayer )
{
	// Beta 5.0 (leak): once a round; not when both teams have players and
	// the round is more than 20 seconds old; not before an appearance is chosen
	// Counter-Strike 1.3: the teams are counted here (1.3 only: with mp_fadetoblack a player who is
	// too late for the round got the black screen at once)
	if ( pPlayer->m_iNumSpawns > 0 )
		return FALSE;

	m_iNumCT = CountTeamPlayers( TEAM_CT );
	m_iNumTerrorist = CountTeamPlayers( TEAM_TERRORIST );

	if ( m_iNumTerrorist > 0 && m_iNumCT > 0 && m_flRoundStartTime + 20 < gpGlobals->time )
		return FALSE;

	return pPlayer->m_iMenu != Menu_ChooseAppearance;
}

//=========================================================
//=========================================================
float CHalfLifeMultiplay :: FlPlayerSpawnTime( CBasePlayer *pPlayer )
{
	return gpGlobals->time;//now!
}

BOOL CHalfLifeMultiplay :: AllowAutoTargetCrosshair( void )
{
	// Counter-Strike 1.3: the cvar is looked up once (the static's name is the Linux library's)
	static cvar_t *mp_autocrosshair = NULL;

	if ( !mp_autocrosshair )
		mp_autocrosshair = CVAR_GET_POINTER( "mp_autocrosshair" );

	if ( mp_autocrosshair )
		return ( mp_autocrosshair->value != 0 );

	return FALSE;
}

//=========================================================
// IPointsForKill - how many points awarded to anyone
// that kills this player?
//=========================================================
int CHalfLifeMultiplay :: IPointsForKill( CBasePlayer *pAttacker, CBasePlayer *pKilled )
{
	return 1;
}


//=========================================================
// PlayerKilled - someone/something killed this player
//=========================================================
void CHalfLifeMultiplay :: PlayerKilled( CBasePlayer *pVictim, entvars_t *pKiller, entvars_t *pInflictor )
{
	DeathNotice( pVictim, pKiller, pInflictor );

	pVictim->m_iDeaths += 1;
	pVictim->m_afPhysicsFlags &= ~PFLAG_ONTRAIN;	// Beta 7.1: he lets go of a train's or vehicle's controls
	pVictim->m_fNotKilled = false;
	pVictim->m_bEscaped = false;	// Beta 6.5
	pVictim->m_iTrain = 0xc0;	// Beta 7.1: TRAIN_NEW | TRAIN_OFF, which only player.cpp defines

	// Beta 6.5: his view is his own again
	SET_VIEW( pVictim->edict(), pVictim->edict() );

	// (Beta 4.0 to Counter-Strike 1.3: a dead commander got a successor here)

	CBasePlayer *peKiller = NULL;
	CBaseEntity *ktmp = CBaseEntity::Instance( pKiller );
	if ( ktmp && (ktmp->Classify() == CLASS_PLAYER) )
		peKiller = (CBasePlayer*)ktmp;
	else if ( ktmp && ktmp->Classify() == CLASS_VEHICLE )
	{
		// Beta 7.1: run over by a vehicle, the kill is its driver's
		CBasePlayer *pDriver = ((CFuncVehicle *)ktmp)->m_pDriver;	// name ours

		if ( pDriver )
		{
			pKiller = pDriver->pev;
			peKiller = pDriver;
			ktmp = pDriver;
		}
	}

	FireTargets( "game_playerdie", pVictim, pVictim, USE_TOGGLE, 0 );

	if ( pVictim->pev == pKiller )  
	{  // killed self
		pKiller->frags -= 1;
	}
	else if ( ktmp && ktmp->IsPlayer() )
	{
		CBasePlayer *pKillerPlayer = GetClassPtr( (CBasePlayer *)pKiller );	// name ours

		// Beta 5.0 (leak): a team kill costs 3,300 and is counted (the
		// grenade exception is gone); at the third, with mp_autokick, the
		// killer is kicked; an enemy pays 300.
		// Beta 7.0: a team kill takes the point away (6.6 gave one for it too)
		if ( pKillerPlayer->m_iTeam == pVictim->m_iTeam )
		{
			pKiller->frags -= IPointsForKill( peKiller, pVictim );

			pKillerPlayer->AddAccount( -3300, true );
			pKillerPlayer->m_iTeamKills++;
			pKillerPlayer->m_bJustKilledTeammate = true;	// Beta 6.1: he sits the next round out (RoundRespawn)
			ClientPrint( pKillerPlayer->pev, HUD_PRINTCENTER, "#Killed_Teammate" );
			// (Counter-Strike 1.1: Beta 6.6's log line "%s killed a teammate\n" is gone; the count is a title, 1.0b: "Teammate kills: %i of 3\n")
			ClientPrint( pKillerPlayer->pev, HUD_PRINTCONSOLE, "#Game_teammate_kills", UTIL_dtos1( pKillerPlayer->m_iTeamKills ) );

			if ( pKillerPlayer->m_iTeamKills == 3 && CVAR_GET_FLOAT( "mp_autokick" ) != 0 )
			{
				ClientPrint( pKillerPlayer->pev, HUD_PRINTCONSOLE, "#Banned_For_Killing_Teamates" );

				int iUserID = GETPLAYERUSERID( pKillerPlayer->edict() );	// name ours
				if ( iUserID != -1 )
					SERVER_COMMAND( UTIL_VarArgs( "kick # %d\n", iUserID ) );
			}

			if ( !( pKillerPlayer->m_flDisplayHistory & ( 1 << 10 ) ) )	// the team kill hint
			{
				pKillerPlayer->m_flDisplayHistory |= ( 1 << 10 );
				pKillerPlayer->HintMessage( "#Hint_careful_around_teammates", FALSE, FALSE );	// Counter-Strike 1.1: the hints are titles
			}
		}
		else
		{
			// if a player dies in a deathmatch game and the killer is a client, award the killer some points
			pKiller->frags += IPointsForKill( peKiller, pVictim );

			// Beta 5.0: no "You killed an enemy" in the middle of the screen any more.
			// Beta 6.5: 2,500 for the VIP
			if ( pVictim->m_bIsVIP == true )
			{
				pKillerPlayer->HintMessage( "#Hint_reward_for_killing_vip", TRUE, FALSE );
				pKillerPlayer->AddAccount( 2500, true );

				// Counter-Strike 1.3: the HLTV director is told (the priority 15).
				// Counter-Strike 1.4: SDK 2.3's svc_director, and the scene is final
				MESSAGE_BEGIN( MSG_SPEC, SVC_DIRECTOR );
					WRITE_BYTE ( 9 );	// command length in bytes
					WRITE_BYTE ( DRC_CMD_EVENT );	// player killed
					WRITE_SHORT( ENTINDEX(pVictim->edict()) );	// index number of primary entity
					WRITE_SHORT( ENTINDEX(ENT(pInflictor)) );	// index number of secondary entity
					WRITE_LONG( 15 | DRC_FLAG_DRAMATIC | DRC_FLAG_FINAL );   // eventflags (priority and flags)
				MESSAGE_END();

				// Beta 6.6; Counter-Strike 1.1: the standard log format's line (1.0b: "%s assasinated the VIP\n")
				UTIL_LogPrintf( "\"%s<%i><%s><TERRORIST>\" triggered \"Assassinated_The_VIP\"\n", STRING( pKillerPlayer->pev->netname ), GETPLAYERUSERID( pKillerPlayer->edict() ), GETPLAYERAUTHID( pKillerPlayer->edict() ) );
			}
			else
				pKillerPlayer->AddAccount( 300, true );

			if ( !( pKillerPlayer->m_flDisplayHistory & ( 1 << 11 ) ) )	// the enemy kill hint
			{
				pKillerPlayer->m_flDisplayHistory |= ( 1 << 11 );
				pKillerPlayer->HintMessage( "#Hint_win_round_by_killing_enemy", FALSE, FALSE );
			}
		}
		
		FireTargets( "game_playerkill", ktmp, ktmp, USE_TOGGLE, 0 );

		// (Beta 6.5 to Counter-Strike 1.3: his observers' camera mode was set again here, as in
		// ClientDisconnected; 1.4's observers look for a new target themselves, Observer_CheckTarget)
	}
	else
	{  // killed by the world
		pKiller->frags -= 1;
	}

	// update the scores
	// killed scores
	MESSAGE_BEGIN( MSG_BROADCAST, gmsgScoreInfo );	// Beta 6.0: unreliable (was MSG_ALL)
		WRITE_BYTE( ENTINDEX(pVictim->edict()) );
		WRITE_SHORT( pVictim->pev->frags );
		WRITE_SHORT( pVictim->m_iDeaths );
		WRITE_SHORT( 0 );
		WRITE_SHORT( pVictim->m_iTeam );
	MESSAGE_END();

	// killers score, if it's a player
	CBaseEntity *ep = CBaseEntity::Instance( pKiller );
	if ( ep && ep->Classify() == CLASS_PLAYER )
	{
		CBasePlayer *PK = (CBasePlayer*)ep;

		MESSAGE_BEGIN( MSG_ALL, gmsgScoreInfo );	// (still reliable, sic)
			WRITE_BYTE( ENTINDEX(PK->edict()) );
			WRITE_SHORT( PK->pev->frags );
			WRITE_SHORT( PK->m_iDeaths );
			WRITE_SHORT( 0 );
			WRITE_SHORT( PK->m_iTeam );
		MESSAGE_END();

		// let the killer paint another decal as soon as he'd like.
		PK->m_flNextDecalTime = gpGlobals->time;
	}
}

//=========================================================
// Deathnotice. 
//=========================================================
void CHalfLifeMultiplay::DeathNotice( CBasePlayer *pVictim, entvars_t *pKiller, entvars_t *pevInflictor )
{
	// Work out what killed the player, and send a message to all clients about it
	CBaseEntity *Killer = CBaseEntity::Instance( pKiller );

	const char *killer_weapon_name = "world";		// by default, the player is killed by the world
	int killer_index = 0;
	
	// Hack to fix name change
	char *tau = "tau_cannon";
	char *gluon = "gluon gun";

	if ( pKiller->flags & FL_CLIENT )
	{
		killer_index = ENTINDEX(ENT(pKiller));
		
		if ( pevInflictor )
		{
			if ( pevInflictor == pKiller )
			{
				// If the inflictor is the killer,  then it must be their current weapon doing the damage
				CBasePlayer *pPlayer = (CBasePlayer*)CBaseEntity::Instance( pKiller );
				
				if ( pPlayer && pPlayer->m_pActiveItem )	// (Counter-Strike 1.3: the player is tested)
				{
					killer_weapon_name = pPlayer->m_pActiveItem->pszName();
				}
			}
			else
			{
				killer_weapon_name = STRING( pevInflictor->classname );  // it's just that easy
			}
		}
	}
	else
	{
		killer_weapon_name = STRING( pevInflictor->classname );
	}

	// strip the monster_* or weapon_* from the inflictor's classname
	if ( strncmp( killer_weapon_name, "weapon_", 7 ) == 0 )
		killer_weapon_name += 7;
	else if ( strncmp( killer_weapon_name, "monster_", 8 ) == 0 )
		killer_weapon_name += 8;
	else if ( strncmp( killer_weapon_name, "func_", 5 ) == 0 )
		killer_weapon_name += 5;

	// Counter-Strike 1.0: a kill by a hit to the head goes to the clients as a byte of its own
	// and the weapon keeps its name (Beta 7.1: the weapon's name was "headshot", for the
	// clients and the log, and the console got "Got a headshot!").  The local's name is
	// Counter-Strike 1.6's.
	int iGotHeadshot = 0;

	if ( pVictim->m_bHeadshotKilled == true )
		iGotHeadshot = 1;

	MESSAGE_BEGIN( MSG_ALL, gmsgDeathMsg );
		WRITE_BYTE( killer_index );						// the killer
		WRITE_BYTE( ENTINDEX(pVictim->edict()) );		// the victim
		WRITE_BYTE( iGotHeadshot );
		WRITE_STRING( killer_weapon_name );		// what they were killed by (should this be a string?)
	MESSAGE_END();

	// replace the code names with the 'real' names
	if ( !strcmp( killer_weapon_name, "egon" ) )
		killer_weapon_name = gluon;
	else if ( !strcmp( killer_weapon_name, "gauss" ) )
		killer_weapon_name = tau;

	// Beta 6.5: the log names the victim's team.
	// Counter-Strike 1.1: the standard log format's lines (SDK 2.2's words), with the
	// WON ids and both teams; who is not a counter-terrorist is logged as a terrorist.
	// Counter-Strike 1.3: the teams' names are GetTeam's (TERRORIST, CT, SPECTATOR or nothing).
	// The locals' names are Counter-Strike 1.6's
	if ( pVictim->pev == pKiller )  
	{  // killed self
		char *team = GetTeam( pVictim->m_iTeam );

		UTIL_LogPrintf( "\"%s<%i><%s><%s>\" committed suicide with \"%s\"\n",  
			STRING( pVictim->pev->netname ), 
			GETPLAYERUSERID( pVictim->edict() ),
			GETPLAYERAUTHID( pVictim->edict() ),
			team,
			killer_weapon_name );
	}
	else if ( pKiller->flags & FL_CLIENT )
	{
		CBasePlayer *pPlayer = (CBasePlayer *)CBaseEntity::Instance( pKiller );

		char *VictimTeam = GetTeam( pVictim->m_iTeam );
		char *KillerTeam = GetTeam( pPlayer->m_iTeam );

		UTIL_LogPrintf( "\"%s<%i><%s><%s>\" killed \"%s<%i><%s><%s>\" with \"%s\"\n",  
			STRING( pKiller->netname ),
			GETPLAYERUSERID( ENT(pKiller) ),
			GETPLAYERAUTHID( ENT(pKiller) ),
			KillerTeam,
			STRING( pVictim->pev->netname ),
			GETPLAYERUSERID( pVictim->edict() ),
			GETPLAYERAUTHID( pVictim->edict() ),
			VictimTeam,
			killer_weapon_name );
	}
	else
	{ 
		// killed by the world
		char *team = GetTeam( pVictim->m_iTeam );

		UTIL_LogPrintf( "\"%s<%i><%s><%s>\" committed suicide with \"%s\" (world)\n",
			STRING( pVictim->pev->netname ), 
			GETPLAYERUSERID( pVictim->edict() ), 
			GETPLAYERAUTHID( pVictim->edict() ),
			team,
			killer_weapon_name );
	}

	CheckWinConditions();

	// Counter-Strike 1.3 (SDK 2.2's text): the HLTV director is told.
	// Counter-Strike 1.4: SDK 2.3's svc_director; a kill by a hit to the head has priority 9
	// and is worth slow motion
	MESSAGE_BEGIN( MSG_SPEC, SVC_DIRECTOR );
		WRITE_BYTE ( 9 );	// command length in bytes
		WRITE_BYTE ( DRC_CMD_EVENT );	// player killed
		WRITE_SHORT( ENTINDEX(pVictim->edict()) );	// index number of primary entity
		if (pevInflictor)
			WRITE_SHORT( ENTINDEX(ENT(pevInflictor)) );	// index number of secondary entity
		else
			WRITE_SHORT( ENTINDEX(ENT(pKiller)) );	// index number of secondary entity
		if ( iGotHeadshot )
			WRITE_LONG( 9 | DRC_FLAG_DRAMATIC | DRC_FLAG_SLOWMOTION );   // eventflags (priority and flags)
		else
			WRITE_LONG( 7 | DRC_FLAG_DRAMATIC);   // eventflags (priority and flags)
	MESSAGE_END();

//  Print a standard message
	// TODO: make this go direct to console
	return; // just remove for now
/*
	char	szText[ 128 ];

	if ( pKiller->flags & FL_MONSTER )
	{
		// killed by a monster
		strcpy ( szText, STRING( pVictim->pev->netname ) );
		strcat ( szText, " was killed by a monster.\n" );
		return;
	}

	if ( pKiller == pVictim->pev )
	{
		strcpy ( szText, STRING( pVictim->pev->netname ) );
		strcat ( szText, " commited suicide.\n" );
	}
	else if ( pKiller->flags & FL_CLIENT )
	{
		strcpy ( szText, STRING( pKiller->netname ) );

		strcat( szText, " : " );
		strcat( szText, killer_weapon_name );
		strcat( szText, " : " );

		strcat ( szText, STRING( pVictim->pev->netname ) );
		strcat ( szText, "\n" );
	}
	else if ( FClassnameIs ( pKiller, "worldspawn" ) )
	{
		strcpy ( szText, STRING( pVictim->pev->netname ) );
		strcat ( szText, " fell or drowned or something.\n" );
	}
	else if ( pKiller->solid == SOLID_BSP )
	{
		strcpy ( szText, STRING( pVictim->pev->netname ) );
		strcat ( szText, " was mooshed.\n" );
	}
	else
	{
		strcpy ( szText, STRING( pVictim->pev->netname ) );
		strcat ( szText, " died mysteriously.\n" );
	}

	UTIL_ClientPrintAll( szText );
*/
}

//=========================================================
// PlayerGotWeapon - player has grabbed a weapon that was
// sitting in the world
//=========================================================
void CHalfLifeMultiplay :: PlayerGotWeapon( CBasePlayer *pPlayer, CBasePlayerItem *pWeapon )
{
}

//=========================================================
// FlWeaponRespawnTime - what is the time in the future
// at which this weapon may spawn?
//=========================================================
float CHalfLifeMultiplay :: FlWeaponRespawnTime( CBasePlayerItem *pWeapon )
{
	// Beta 5.0 (leak): mp_weaponstay is gone
	return gpGlobals->time + WEAPON_RESPAWN_TIME;
}

// when we are within this close to running out of entities,  items 
// marked with the ITEM_FLAG_LIMITINWORLD will delay their respawn
#define ENTITY_INTOLERANCE	100

//=========================================================
// FlWeaponRespawnTime - Returns 0 if the weapon can respawn 
// now,  otherwise it returns the time at which it can try
// to spawn again.
//=========================================================
float CHalfLifeMultiplay :: FlWeaponTryRespawn( CBasePlayerItem *pWeapon )
{
	if ( pWeapon && pWeapon->m_iId && (pWeapon->iFlags() & ITEM_FLAG_LIMITINWORLD) )
	{
		if ( NUMBER_OF_ENTITIES() < (gpGlobals->maxEntities - ENTITY_INTOLERANCE) )
			return 0;

		// we're past the entity tolerance level,  so delay the respawn
		return FlWeaponRespawnTime( pWeapon );
	}

	return 0;
}

//=========================================================
// VecWeaponRespawnSpot - where should this weapon spawn?
// Some game variations may choose to randomize spawn locations
//=========================================================
Vector CHalfLifeMultiplay :: VecWeaponRespawnSpot( CBasePlayerItem *pWeapon )
{
	return pWeapon->pev->origin;
}

//=========================================================
// WeaponShouldRespawn - any conditions inhibiting the
// respawning of this weapon?
//=========================================================
int CHalfLifeMultiplay :: WeaponShouldRespawn( CBasePlayerItem *pWeapon )
{
	if ( pWeapon->pev->spawnflags & SF_NORESPAWN )
	{
		return GR_WEAPON_RESPAWN_NO;
	}

	return GR_WEAPON_RESPAWN_YES;
}

//=========================================================
// CanHaveWeapon - returns FALSE if the player is not allowed
// to pick up this weapon
//=========================================================
BOOL CHalfLifeMultiplay::CanHavePlayerItem( CBasePlayer *pPlayer, CBasePlayerItem *pItem )
{
	// Beta 5.0 (leak): mp_weaponstay is gone
	return CGameRules::CanHavePlayerItem( pPlayer, pItem );
}

//=========================================================
//=========================================================
BOOL CHalfLifeMultiplay::CanHaveItem( CBasePlayer *pPlayer, CItem *pItem )
{
	return TRUE;
}

//=========================================================
//=========================================================
void CHalfLifeMultiplay::PlayerGotItem( CBasePlayer *pPlayer, CItem *pItem )
{
}

//=========================================================
//=========================================================
int CHalfLifeMultiplay::ItemShouldRespawn( CItem *pItem )
{
	if ( pItem->pev->spawnflags & SF_NORESPAWN )
	{
		return GR_ITEM_RESPAWN_NO;
	}

	return GR_ITEM_RESPAWN_YES;
}


//=========================================================
// At what time in the future may this Item respawn?
//=========================================================
float CHalfLifeMultiplay::FlItemRespawnTime( CItem *pItem )
{
	return gpGlobals->time + ITEM_RESPAWN_TIME;
}

//=========================================================
// Where should this item respawn?
// Some game variations may choose to randomize spawn locations
//=========================================================
Vector CHalfLifeMultiplay::VecItemRespawnSpot( CItem *pItem )
{
	return pItem->pev->origin;
}

//=========================================================
//=========================================================
void CHalfLifeMultiplay::PlayerGotAmmo( CBasePlayer *pPlayer, char *szName, int iCount )
{
}

//=========================================================
//=========================================================
BOOL CHalfLifeMultiplay::IsAllowedToSpawn( CBaseEntity *pEntity )
{
//	if ( pEntity->pev->flags & FL_MONSTER )
//		return FALSE;

	return TRUE;
}

//=========================================================
//=========================================================
int CHalfLifeMultiplay::AmmoShouldRespawn( CBasePlayerAmmo *pAmmo )
{
	if ( pAmmo->pev->spawnflags & SF_NORESPAWN )
	{
		return GR_AMMO_RESPAWN_NO;
	}

	return GR_AMMO_RESPAWN_YES;
}

//=========================================================
//=========================================================
float CHalfLifeMultiplay::FlAmmoRespawnTime( CBasePlayerAmmo *pAmmo )
{
	return gpGlobals->time + AMMO_RESPAWN_TIME;
}

//=========================================================
//=========================================================
Vector CHalfLifeMultiplay::VecAmmoRespawnSpot( CBasePlayerAmmo *pAmmo )
{
	return pAmmo->pev->origin;
}

//=========================================================
//=========================================================
float CHalfLifeMultiplay::FlHealthChargerRechargeTime( void )
{
	return 60;
}


float CHalfLifeMultiplay::FlHEVChargerRechargeTime( void )
{
	return 30;
}

//=========================================================
//=========================================================
int CHalfLifeMultiplay::DeadPlayerWeapons( CBasePlayer *pPlayer )
{
	return GR_PLR_DROP_GUN_ACTIVE;
}

//=========================================================
//=========================================================
int CHalfLifeMultiplay::DeadPlayerAmmo( CBasePlayer *pPlayer )
{
	return GR_PLR_DROP_AMMO_ACTIVE;
}

edict_t *CHalfLifeMultiplay::GetPlayerSpawnSpot( CBasePlayer *pPlayer )
{
	edict_t *pentSpawnSpot = CGameRules::GetPlayerSpawnSpot( pPlayer );	
	if ( IsMultiplayer() && pentSpawnSpot->v.target )
	{
		FireTargets( STRING(pentSpawnSpot->v.target), pPlayer, pPlayer, USE_TOGGLE, 0 );
	}

	return pentSpawnSpot;
}


//=========================================================
//=========================================================
int CHalfLifeMultiplay::PlayerRelationship( CBasePlayer *pPlayer, CBaseEntity *pTarget )
{
	// Beta 5.0 (leak): the player is a CBasePlayer (Linux symbol), so only
	// the target is checked
	if ( !pPlayer || !pTarget || !pTarget->IsPlayer() )
		return GR_NOTTEAMMATE;

	CBasePlayer *player = GetClassPtr( (CBasePlayer *)pPlayer->pev );
	CBasePlayer *target = GetClassPtr( (CBasePlayer *)pTarget->pev );

	// Beta 4.1: team mates are of the same team, dead or alive
	if ( player->m_iTeam == target->m_iTeam )
		return GR_TEAMMATE;

	return GR_NOTTEAMMATE;
}

// (PlayFootstepSounds stood here to Counter-Strike 1.3; 1.4's footsteps are the player movement code's)

BOOL CHalfLifeMultiplay :: FAllowFlashlight( void ) 
{ 
	// Counter-Strike 1.3: the cvar is looked up once (the static's name is the Linux library's and 1.6's)
	static cvar_t *mp_flashlight = NULL;

	if ( !mp_flashlight )
		mp_flashlight = CVAR_GET_POINTER( "mp_flashlight" );

	if ( mp_flashlight )
		return ( mp_flashlight->value != 0 );

	return FALSE;
}

//=========================================================
//=========================================================
BOOL CHalfLifeMultiplay :: FAllowMonsters( void )
{
	return ( CVAR_GET_FLOAT( "mp_allowmonsters" ) != 0 );
}

//=========================================================
//======== CHalfLifeMultiplay private functions ===========
#define INTERMISSION_TIME		6

void CHalfLifeMultiplay :: GoToIntermission( void )
{
	if ( g_fGameOver )
		return;  // intermission has already been triggered, so ignore.

	// Counter-Strike 1.1: the teams' scores go to the log (mp is 1.6's name)
	CHalfLifeMultiplay *mp = (CHalfLifeMultiplay *)g_pGameRules;

	if ( mp )
	{
		UTIL_LogPrintf( "Team \"CT\" scored \"%i\" with \"%i\" players\n", mp->m_iNumCTWins, mp->m_iNumCT );
		UTIL_LogPrintf( "Team \"TERRORIST\" scored \"%i\" with \"%i\" players\n", mp->m_iNumTerroristWins, mp->m_iNumTerrorist );
	}

	MESSAGE_BEGIN(MSG_ALL, SVC_INTERMISSION);
	MESSAGE_END();

	// (SDK 2.2's text, the start time a member here; 1.0b: gpGlobals->time + INTERMISSION_TIME, 6 seconds)
	// bounds check
	int time = (int)CVAR_GET_FLOAT( "mp_chattime" );
	if ( time < 1 )	// (Counter-Strike 1.3: from 1 second; 1.1: 10)
		CVAR_SET_STRING( "mp_chattime", "1" );
	else if ( time > MAX_INTERMISSION_TIME )
		CVAR_SET_STRING( "mp_chattime", UTIL_dtos1( MAX_INTERMISSION_TIME ) );

	m_flIntermissionEndTime = gpGlobals->time + ( (int)mp_chattime.value );
	m_flIntermissionStartTime = gpGlobals->time;

	g_fGameOver = TRUE;
	m_iEndIntermissionButtonHit = FALSE;

	// Beta 5.0 (leak): the next map's spawn points are counted afresh
	m_iSpawnPointCount_Terrorist = 0;
	m_iSpawnPointCount_CT = 0;
	m_fCountedSpawnPoints = false;
}

#define MAX_RULE_BUFFER 1024

typedef struct mapcycle_item_s
{
	struct mapcycle_item_s *next;

	char mapname[ 32 ];
	int  minplayers, maxplayers;
	char rulebuffer[ MAX_RULE_BUFFER ];
} mapcycle_item_t;

typedef struct mapcycle_s
{
	struct mapcycle_item_s *items;
	struct mapcycle_item_s *next_item;
} mapcycle_t;

/*
==============
DestroyMapCycle

Clean up memory used by mapcycle when switching it
==============
*/
void DestroyMapCycle( mapcycle_t *cycle )
{
	mapcycle_item_t *p, *n, *start;
	p = cycle->items;
	if ( p )
	{
		start = p;
		p = p->next;
		while ( p != start )
		{
			n = p->next;
			delete p;
			p = n;
		}
		
		delete cycle->items;
	}
	cycle->items = NULL;
	cycle->next_item = NULL;
}

static char com_token[ 1500 ];

/*
==============
COM_Parse

Parse a token out of a string
==============
*/
char *COM_Parse (char *data)
{
	int             c;
	int             len;
	
	len = 0;
	com_token[0] = 0;
	
	if (!data)
		return NULL;
		
// skip whitespace
skipwhite:
	while ( (c = *data) <= ' ')
	{
		if (c == 0)
			return NULL;                    // end of file;
		data++;
	}
	
// skip // comments
	if (c=='/' && data[1] == '/')
	{
		while (*data && *data != '\n')
			data++;
		goto skipwhite;
	}
	

// handle quoted strings specially
	if (c == '\"')
	{
		data++;
		while (1)
		{
			c = *data++;
			if (c=='\"' || !c)
			{
				com_token[len] = 0;
				return data;
			}
			com_token[len] = c;
			len++;
		}
	}

// parse single characters
	if (c=='{' || c=='}'|| c==')'|| c=='(' || c=='\'' || c == ',' )
	{
		com_token[len] = c;
		len++;
		com_token[len] = 0;
		return data+1;
	}

// parse a regular word
	do
	{
		com_token[len] = c;
		data++;
		len++;
		c = *data;
	if (c=='{' || c=='}'|| c==')'|| c=='(' || c=='\'' || c == ',' )
			break;
	} while (c>32);
	
	com_token[len] = 0;
	return data;
}

/*
==============
COM_TokenWaiting

Returns 1 if additional data is waiting to be processed on this line
==============
*/
int COM_TokenWaiting( char *buffer )
{
	char *p;

	p = buffer;
	while ( *p && *p!='\n')
	{
		if ( !isspace( *p ) || isalnum( *p ) )
			return 1;

		p++;
	}

	return 0;
}

/*
==============
ReloadMapCycleFile


Parses mapcycle.txt file into mapcycle_t structure
==============
*/
int ReloadMapCycleFile( char *filename, mapcycle_t *cycle )
{
	char szBuffer[ MAX_RULE_BUFFER ];
	char szMap[ 32 ];
	int length;
	char *pFileList;
	char *aFileList = pFileList = (char*)LOAD_FILE_FOR_ME( filename, &length );
	int hasbuffer;
	mapcycle_item_s *item, *newlist = NULL, *next;

	if ( pFileList && length )
	{
		// the first map name in the file becomes the default
		while ( 1 )
		{
			hasbuffer = 0;
			memset( szBuffer, 0, MAX_RULE_BUFFER );

			pFileList = COM_Parse( pFileList );
			if ( strlen( com_token ) <= 0 )
				break;

			strcpy( szMap, com_token );

			// Any more tokens on this line?
			if ( COM_TokenWaiting( pFileList ) )
			{
				pFileList = COM_Parse( pFileList );
				if ( strlen( com_token ) > 0 )
				{
					hasbuffer = 1;
					strcpy( szBuffer, com_token );
				}
			}

			// Check map
			if ( IS_MAP_VALID( szMap ) )
			{
				// Create entry
				char *s;

				item = new mapcycle_item_s;

				strcpy( item->mapname, szMap );

				item->minplayers = 0;
				item->maxplayers = 0;

				memset( item->rulebuffer, 0, MAX_RULE_BUFFER );

				if ( hasbuffer )
				{
					s = g_engfuncs.pfnInfoKeyValue( szBuffer, "minplayers" );
					if ( s && s[0] )
					{
						item->minplayers = atoi( s );
						item->minplayers = max( item->minplayers, 0 );
						item->minplayers = min( item->minplayers, gpGlobals->maxClients );
					}
					s = g_engfuncs.pfnInfoKeyValue( szBuffer, "maxplayers" );
					if ( s && s[0] )
					{
						item->maxplayers = atoi( s );
						item->maxplayers = max( item->maxplayers, 0 );
						item->maxplayers = min( item->maxplayers, gpGlobals->maxClients );
					}

					// Remove keys
					//
					g_engfuncs.pfnInfo_RemoveKey( szBuffer, "minplayers" );
					g_engfuncs.pfnInfo_RemoveKey( szBuffer, "maxplayers" );

					strcpy( item->rulebuffer, szBuffer );
				}

				item->next = cycle->items;
				cycle->items = item;
			}
			else
			{
				ALERT( at_console, "Skipping %s from mapcycle, not a valid map\n", szMap );
			}


		}

		FREE_FILE( aFileList );
	}

	// Fixup circular list pointer
	item = cycle->items;

	// Reverse it to get original order
	while ( item )
	{
		next = item->next;
		item->next = newlist;
		newlist = item;
		item = next;
	}
	cycle->items = newlist;
	item = cycle->items;

	// Didn't parse anything
	if ( !item )
	{
		return 0;
	}

	while ( item->next )
	{
		item = item->next;
	}
	item->next = cycle->items;
	
	cycle->next_item = item->next;

	return 1;
}

/*
==============
CountPlayers

Determine the current # of active players on the server for map cycling logic
==============
*/
int CountPlayers( void )
{
	int	num = 0;

	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBaseEntity *pEnt = UTIL_PlayerByIndex( i );

		if ( pEnt )
		{
			num = num + 1;
		}
	}

	return num;
}

/*
==============
ExtractCommandString

Parse commands/key value pairs to issue right after map xxx command is issued on server
 level transition
==============
*/
void ExtractCommandString( char *s, char *szCommand )
{
	// Now make rules happen
	char	pkey[512];
	char	value[512];	// use two buffers so compares
								// work without stomping on each other
	char	*o;
	
	if ( *s == '\\' )
		s++;

	while (1)
	{
		o = pkey;
		while ( *s != '\\' )
		{
			if ( !*s )
				return;
			*o++ = *s++;
		}
		*o = 0;
		s++;

		o = value;

		while (*s != '\\' && *s)
		{
			if (!*s)
				return;
			*o++ = *s++;
		}
		*o = 0;

		strcat( szCommand, pkey );
		if ( strlen( value ) > 0 )
		{
			strcat( szCommand, " " );
			strcat( szCommand, value );
		}
		strcat( szCommand, "\n" );

		if (!*s)
			return;
		s++;
	}
}

//=========================================================
// ResetAllMapVotes - Counter-Strike 1.4 (Linux symbol; the
// locals' names are Counter-Strike 1.6's): every player in a
// team takes his map vote back and the counts are zeroed.
// Retail has it only inside DisplayMaps
//=========================================================
void CHalfLifeMultiplay::ResetAllMapVotes( void )
{
	int j;
	CBaseEntity *pTempEntity;
	CBasePlayer *pTempPlayer;

	pTempEntity = UTIL_FindEntityByClassname( NULL, "player" );
	while ( pTempEntity != NULL && !FNullEnt( pTempEntity->edict() ) )
	{
		pTempPlayer = GetClassPtr( (CBasePlayer *)pTempEntity->pev );

		if ( pTempPlayer->m_iTeam != TEAM_UNASSIGNED )
			pTempPlayer->m_iMapVote = 0;

		pTempEntity = UTIL_FindEntityByClassname( pTempEntity, "player" );
	}

	for ( j = 0; j < 100; j++ )
		m_iMapVotes[j] = 0;
}

//=========================================================
// GetMapCount - Counter-Strike 1.4 (Linux symbol; the
// locals' names are Counter-Strike 1.6's): the number of
// maps in the map cycle (client.cpp's votemap command)
//=========================================================
int GetMapCount( void )
{
	static mapcycle_t mapcycle2;
	char *mapcfile = (char*)CVAR_GET_STRING( "mapcyclefile" );
	mapcycle_item_s *item;
	int iCount = 0, iDone = 0;

	DestroyMapCycle( &mapcycle2 );
	ReloadMapCycleFile( mapcfile, &mapcycle2 );

	item = mapcycle2.next_item;

	while ( item && !iDone )
	{
		if ( item->next == mapcycle2.next_item )
			iDone = 1;

		iCount++;
		item = item->next;
	}

	return iCount;
}

//=========================================================
// DisplayMaps - Beta 6.5 (Linux symbol; the names of the
// parameters and locals are Counter-Strike 1.6's): the map
// cycle with each map's votes, to the player's console; with
// a map number, the server changes to that map
//=========================================================
void CHalfLifeMultiplay::DisplayMaps( CBasePlayer *player, int iVote )
{
	static mapcycle_t mapcycle2;
	char *mapcfile = (char*)CVAR_GET_STRING( "mapcyclefile" );
	mapcycle_item_s *item;
	int iCount = 0;
	char *pszNewMap = NULL;	// (Counter-Strike 1.4: a pointer; to 1.3 a copy of the name)

	DestroyMapCycle( &mapcycle2 );
	ReloadMapCycleFile( mapcfile, &mapcycle2 );

	// Counter-Strike 1.0b: the cycle's last map is listed too, and an empty
	// cycle is no longer read through a null pointer (1.0: while
	// ( item->next != mapcycle2.next_item ), with the test for item inside
	// the loop). done is Counter-Strike 1.6's name
	int done = 0;

	item = mapcycle2.next_item;

	while ( item && !done )
	{
		if ( item->next == mapcycle2.next_item )
			done = 1;

		iCount++;

		// Counter-Strike 1.1: titles of the client with the number, the map and its
		// votes (1.0b: "%d :  %s (%i vote)\n" and "... votes)\n" through sprintf)
		if ( player )
		{
			if ( m_iMapVotes[iCount - 1] == 1 )
				ClientPrint( player->pev, HUD_PRINTCONSOLE, "#Vote", UTIL_dtos1( iCount ), item->mapname, UTIL_dtos2( m_iMapVotes[iCount - 1] ) );
			else
				ClientPrint( player->pev, HUD_PRINTCONSOLE, "#Votes", UTIL_dtos1( iCount ), item->mapname, UTIL_dtos2( m_iMapVotes[iCount - 1] ) );
		}

		if ( iCount == iVote )
			pszNewMap = item->mapname;

		item = item->next;
	}

	if ( pszNewMap && iVote )
	{
		// Beta 6.6: a vote for the map being played adds thirty minutes to it.
		// Counter-Strike 1.4: only to a time limit that is set, the clients are
		// told, and every vote is taken back
		if ( !strcmp( pszNewMap, STRING( gpGlobals->mapname ) ) )
		{
			if ( timelimit.value != 0 )
			{
				timelimit.value += 30;
				UTIL_ClientPrintAll( HUD_PRINTCENTER, "#Map_Vote_Extend" );
			}

			ResetAllMapVotes();
			return;
		}

		// (Beta 7.0 only: ChangeLevel( szNewMap ) first, to leave the map
		// cycle standing after the voted map)
		CHANGE_LEVEL( pszNewMap, NULL );
	}
}

//=========================================================
// ProcessMapVote - Beta 6.5 (Linux symbol VoteMap to 1.3,
// ProcessMapVote from 1.4; the locals' names are Counter-
// Strike 1.6's): a map's votes are counted afresh; with
// enough of them (mp_mapvoteratio of the players in a team,
// 0.35 to 1, rounded; two at least) the map changes, else
// the player is shown the list. Counter-Strike 1.4: the
// vote's number and the map's age are client.cpp's to check
//=========================================================
void CHalfLifeMultiplay::ProcessMapVote( CBasePlayer *player, int iVote )
{
	CBaseEntity *pTempEntity;
	CBasePlayer *pTempPlayer;
	int iValidVotes;
	int iRequiredVotes;
	int iNumPlayers;

	iValidVotes = 0;
	iNumPlayers = 0;

	pTempEntity = UTIL_FindEntityByClassname( NULL, "player" );
	while ( pTempEntity != NULL && !FNullEnt( pTempEntity->edict() ) )
	{
		pTempPlayer = GetClassPtr( (CBasePlayer *)pTempEntity->pev );

		if ( pTempPlayer->m_iTeam != TEAM_UNASSIGNED )
		{
			iNumPlayers++;

			if ( pTempPlayer->m_iMapVote == iVote )
				iValidVotes++;
		}
		pTempEntity = UTIL_FindEntityByClassname( pTempEntity, "player" );
	}

	m_iMapVotes[iVote - 1] = iValidVotes;

	// (Counter-Strike 1.4: the cvar's own value, and the cvar is set back to the bound)
	float ratio = mapvoteratio.value;

	if ( ratio > 1.0 )
	{
		ratio = 1;
		CVAR_SET_STRING( "mp_mapvoteratio", "1.0" );
	}
	else if ( ratio < 0.35 )
	{
		ratio = 0.35f;
		CVAR_SET_STRING( "mp_mapvoteratio", "0.35" );
	}

	if ( iNumPlayers > 2 )
		iRequiredVotes = (int)( iNumPlayers * ratio + 0.5 );
	else
		iRequiredVotes = 2;

	if ( iValidVotes >= iRequiredVotes )
	{
		DisplayMaps( NULL, iVote );
	}
	else
	{
		DisplayMaps( player, 0 );
		// (Counter-Strike 1.1: a title; 1.0b: "\nRequired Number of votes for a new map = %i\n")
		ClientPrint( player->pev, HUD_PRINTCONSOLE, "#Game_required_votes", UTIL_dtos1( iRequiredVotes ) );
	}
}

/*
==============
ChangeLevel

Server is changing to a new level, check mapcycle.txt for map name and setup info
==============
*/
void CHalfLifeMultiplay :: ChangeLevel( void )
{
	static char szPreviousMapCycleFile[ 256 ];
	static mapcycle_t mapcycle;

	char szNextMap[32];
	char szFirstMapInList[32];
	char szCommands[ 1500 ];
	char szRules[ 1500 ];
	int minplayers = 0, maxplayers = 0;
	strcpy( szFirstMapInList, "hldm1" );  // the absolute default level is hldm1

	int	curplayers;
	BOOL do_cycle = TRUE;

	// find the map to change to
	char *mapcfile = (char*)CVAR_GET_STRING( "mapcyclefile" );
	ASSERT( mapcfile != NULL );

	szCommands[ 0 ] = '\0';
	szRules[ 0 ] = '\0';

	curplayers = CountPlayers();

	// Has the map cycle filename changed?
	if ( stricmp( mapcfile, szPreviousMapCycleFile ) )
	{
		strcpy( szPreviousMapCycleFile, mapcfile );

		DestroyMapCycle( &mapcycle );

		if ( !ReloadMapCycleFile( mapcfile, &mapcycle ) || ( !mapcycle.items ) )
		{
			ALERT( at_console, "Unable to load map cycle file %s\n", mapcfile );
			do_cycle = FALSE;
		}
	}

	if ( do_cycle && mapcycle.items )
	{
		BOOL keeplooking = FALSE;
		BOOL found = FALSE;
		mapcycle_item_s *item;

		// Assume current map
		strcpy( szNextMap, STRING(gpGlobals->mapname) );
		strcpy( szFirstMapInList, STRING(gpGlobals->mapname) );

		// Traverse list
		for ( item = mapcycle.next_item; item->next != mapcycle.next_item; item = item->next )
		{
			keeplooking = FALSE;

			ASSERT( item != NULL );

			if ( item->minplayers != 0 )
			{
				if ( curplayers >= item->minplayers )
				{
					found = TRUE;
					minplayers = item->minplayers;
				}
				else
				{
					keeplooking = TRUE;
				}
			}

			if ( item->maxplayers != 0 )
			{
				if ( curplayers <= item->maxplayers )
				{
					found = TRUE;
					maxplayers = item->maxplayers;
				}
				else
				{
					keeplooking = TRUE;
				}
			}

			// (Beta 7.0 only: asked for a map by name, the cycle was searched
			// for it here; Beta 7.1 is the SDK's function again)
			if ( keeplooking )
				continue;

			found = TRUE;
			break;
		}

		if ( !found )
		{
			item = mapcycle.next_item;
		}			
		
		// Increment next item pointer
		mapcycle.next_item = item->next;

		// Perform logic on current item
		strcpy( szNextMap, item->mapname );

		ExtractCommandString( item->rulebuffer, szCommands );
		strcpy( szRules, item->rulebuffer );
	}

	if ( !IS_MAP_VALID(szNextMap) )
	{
		strcpy( szNextMap, szFirstMapInList );
	}

	g_fGameOver = TRUE;

	ALERT( at_console, "CHANGE LEVEL: %s\n", szNextMap );
	if ( minplayers || maxplayers )
	{
		ALERT( at_console, "PLAYER COUNT:  min %i max %i current %i\n", minplayers, maxplayers, curplayers );
	}
	if ( strlen( szRules ) > 0 )
	{
		ALERT( at_console, "RULES:  %s\n", szRules );
	}
	
	CHANGE_LEVEL( szNextMap, NULL );
	if ( strlen( szCommands ) > 0 )
	{
		SERVER_COMMAND( szCommands );
	}
}

#define MAX_MOTD_CHUNK	  60
#define MAX_MOTD_LENGTH   1536 // (MAX_MOTD_CHUNK * 4)

void CHalfLifeMultiplay :: SendMOTDToClient( edict_t *client )
{
	// read from the MOTD.txt file
	int length, char_count = 0;
	char *pFileList;
	char *aFileList = pFileList = (char*)LOAD_FILE_FOR_ME( (char *)CVAR_GET_STRING( "motdfile" ), &length );	// (SDK 2.2's line; 1.0b: "motd.txt")

	// send the server name
	MESSAGE_BEGIN( MSG_ONE, gmsgServerName, NULL, client );
		WRITE_STRING( CVAR_GET_STRING("hostname") );
	MESSAGE_END();

	// Send the message of the day
	// read it chunk-by-chunk,  and send it in parts

	while ( pFileList && *pFileList && char_count < MAX_MOTD_LENGTH )
	{
		char chunk[MAX_MOTD_CHUNK+1];
		
		if ( strlen( pFileList ) < MAX_MOTD_CHUNK )
		{
			strcpy( chunk, pFileList );
		}
		else
		{
			strncpy( chunk, pFileList, MAX_MOTD_CHUNK );
			chunk[MAX_MOTD_CHUNK] = 0;		// strncpy doesn't always append the null terminator
		}

		char_count += strlen( chunk );
		if ( char_count < MAX_MOTD_LENGTH )
			pFileList = aFileList + char_count; 
		else
			*pFileList = 0;

		MESSAGE_BEGIN( MSG_ONE, gmsgMOTD, NULL, client );
			WRITE_BYTE( *pFileList ? FALSE : TRUE );	// FALSE means there is still more message to come
			WRITE_STRING( chunk );
		MESSAGE_END();
	}

	FREE_FILE( aFileList );
}

//=========================================================
// ClientUserInfoChanged - Counter-Strike 1.0b: whenever a
// client's user info changes, his model is set again from
// the appearance he chose (to 1.0 CBasePlayer::VerifyModel
// did that from PreThink)
//=========================================================
void CHalfLifeMultiplay :: ClientUserInfoChanged( CBasePlayer *pPlayer, char *infobuffer )
{
	pPlayer->SetPlayerModel( pPlayer->m_bHasC4 );
	pPlayer->SetPrefsFromUserinfo( infobuffer );	// Counter-Strike 1.4
}

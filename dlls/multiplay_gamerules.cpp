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
//
// teamplay_gamerules.cpp
//
#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"player.h"
#include	"weapons.h"
#include	"gamerules.h"
#include	"skill.h"
#include	"game.h"
#include	"items.h"
#include	"hostage.h"
#include	"observer.h"

extern DLL_GLOBAL CGameRules	*g_pGameRules;
extern DLL_GLOBAL BOOL	g_fGameOver;
extern void respawn( entvars_t* pev, BOOL fCopyCorpse );
extern int gmsgDeathMsg;	// client dll messages
extern int gmsgScoreInfo;
extern int gmsgMOTD;
extern int gmsgTeamScore;
extern int gmsgSendAudio;
extern int gmsgTeamInfo;
extern int gmsgStatusIcon;
extern void ShowMenu( CBasePlayer *pPlayer, int bitsValidSlots, int nDisplayTime, BOOL fNeedMore, char *pszText );

inline void sic_il_multiplay( void )	// sic: intermediate code the image shows only through header inlines' places
{
	;
	;
	;
}

#define ITEM_RESPAWN_TIME	30
#define WEAPON_RESPAWN_TIME	20
#define AMMO_RESPAWN_TIME	20

//=========================================================
// Broadcast - Beta 3.0: the radio sentence !MRAD_<sentence>
// to every client (name from the later versions' Linux
// symbols)
//=========================================================
void Broadcast( const char *sentence )
{
	char text[32];

	if ( !sentence )
		return;

	strcpy( text, "%!MRAD_" );
	strcat( text, UTIL_VarArgs( "%s", sentence ) );

	MESSAGE_BEGIN( MSG_ALL, gmsgSendAudio );
		WRITE_BYTE( 0 );
		WRITE_STRING( text );
	MESSAGE_END();
}

//=========================================================
// EndRoundMessage - Beta 5.0 (leak): a message in the middle
// of everybody's screen, red fading to green, for four
// seconds (Linux symbol; the text parameters' name ours)
//=========================================================
hudtextparms_t g_textParmsEndRound = { -1, 0.4, 2, 255, 20, 20, 200, 0, 255, 0, 200, 0.01, 0.7, 4.0, 0.07, 1 };	// name ours

void EndRoundMessage( const char *sentence )
{
	UTIL_HudMessageAll( g_textParmsEndRound, sentence );
}

void TerminateRound( float tmDelay, int iWinStatus );

//*********************************************************
// Rules for the half-life multiplayer game.
//*********************************************************

CHalfLifeMultiplay :: CHalfLifeMultiplay()
{
	RefreshSkillData();
	m_iMapHasCameras = 2;
	m_flIntermissionEndTime = 0;
	m_flRestartRoundTime = 0;
	m_iAccountCT = 0;
	m_iAccountTerrorist = 0;
	m_iHostagesRescued = 0;
	m_iRoundWinStatus = 0;
	m_iNumCTWins = 0;
	m_iNumTerroristWins = 0;
	m_pCTCommander = NULL;
	m_pTerroristCommander = NULL;
	m_iNumCT = 0;
	m_iNumTerrorist = 0;
	m_iNumSpawnableCT = 0;
	m_iNumSpawnableTerrorist = 0;
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
	m_fCompleteReset = true;

	// Beta 4.0: no high-detail player models; the round and C4 times as RestartRound reads them
	CVAR_SET_FLOAT( "cl_himodels", 0 );

	m_iRoundTime = (int)CVAR_GET_FLOAT( "mp_roundtime" );
	m_iC4Timer = (int)CVAR_GET_FLOAT( "mp_c4timer" );
	m_iFreezeTime = (int)CVAR_GET_FLOAT( "mp_freezetime" ) + 2;	// Beta 5.0 (leak): the first round's freeze time is 2 seconds longer
	m_flMaxIdlePeriod = m_iRoundTime * 120;

	if ( m_iFreezeTime > 15 )
		m_iFreezeTime = 15;
	else if ( m_iFreezeTime < 0 )
		m_iFreezeTime = 0;

	if ( m_iRoundTime > 9 )
		m_iRoundTime = 9;
	else if ( m_iRoundTime < 3 )
		m_iRoundTime = 3;

	if ( m_iC4Timer > 90 )
		m_iC4Timer = 90;
	else if ( m_iC4Timer < 10 )
		m_iC4Timer = 10;

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
// CleanUpMap - Beta 4.0: breakables and doors are put back
// (Restart), a C4 goes, and up to four defusers; then the
// weapon boxes. Beta 5.0 (leak): no backpack any more, and the
// console says so for each defuser.
//=========================================================
void CHalfLifeMultiplay::CleanUpMap( void )
{
	CBaseEntity *torestart = UTIL_FindEntityByClassname( NULL, "func_breakable" );

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

	torestart = UTIL_FindEntityByClassname( NULL, "func_door_rotating" );

	while ( torestart )
	{
		torestart->Restart();
		torestart = UTIL_FindEntityByClassname( torestart, "func_door_rotating" );
	}

	// only the first one (sic)
	CBaseEntity *toremove = UTIL_FindEntityByClassname( NULL, "grenade" );
	if ( toremove )
		UTIL_Remove( toremove );

	int iLeft = 4;
	toremove = UTIL_FindEntityByClassname( NULL, "item_thighpack" );

	while ( toremove && iLeft > 0 )
	{
		ALERT( at_console, "Removing a Bomb Defuser pack\n" );
		iLeft--;
		UTIL_Remove( toremove );
		toremove = UTIL_FindEntityByClassname( toremove, "item_thighpack" );
	}

	RemoveGuns();
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
				player->m_fHasC4 = true;
				player->GiveNamedItem( "weapon_c4" );
				player->SetBombIcon( FALSE );
				player->pev->body = 1;
				m_pTerroristCommander = player;
			}
		}
		pPlayer = UTIL_FindEntityByClassname( pPlayer, "player" );
	}
}

//=========================================================
// PickNewCommander - Beta 4.0: one of the team's players,
// picked at random, becomes its commander (name from the
// later versions' Linux symbols). Beta 5.0 (leak): only a
// living one.
//=========================================================
void CHalfLifeMultiplay::PickNewCommander( int iTeam )
{
	CBaseEntity *pPlayer = NULL;
	CBasePlayer *player;

	if ( iTeam == TEAM_CT )
		m_pCTCommander = NULL;
	else
		m_pTerroristCommander = NULL;

	int iCount = 0;

	// count the team (sic: without the checks of the second loop)
	pPlayer = UTIL_FindEntityByClassname( pPlayer, "player" );
	while ( pPlayer != NULL )
	{
		player = GetClassPtr( (CBasePlayer *)pPlayer->pev );

		if ( player->m_iTeam == iTeam && player->pev->flags != FL_DORMANT )
			iCount++;

		pPlayer = UTIL_FindEntityByClassname( pPlayer, "player" );
	}

	int iCommander = RANDOM_LONG( 1, iCount );
	iCount = 0;

	pPlayer = UTIL_FindEntityByClassname( pPlayer, "player" );
	while ( pPlayer != NULL && !FNullEnt( pPlayer->edict() ) )
	{
		if ( pPlayer->IsPlayer() && pPlayer->pev->flags != FL_DORMANT )
		{
			player = GetClassPtr( (CBasePlayer *)pPlayer->pev );

			if ( pPlayer && pPlayer->pev->deadflag == DEAD_NO )
			{
				if ( iTeam == TEAM_TERRORIST )
				{
					if ( player->m_iTeam == TEAM_TERRORIST )
					{
						iCount++;
						m_pTerroristCommander = player;
						if ( iCount >= iCommander )
							return;
					}
				}
				else if ( iTeam == TEAM_CT )
				{
					if ( player->m_iTeam == TEAM_CT )
					{
						iCount++;
						m_pCTCommander = player;
						if ( iCount >= iCommander )
							return;
					}
				}
			}
		}
		pPlayer = UTIL_FindEntityByClassname( pPlayer, "player" );
	}
}

//=========================================================
// CheckWinConditions - Beta 5.0 (leak): nothing is decided
// until both teams have a player past the appearance menu (the
// first time, the round restarts at once). A bombed target wins
// it for the terrorists and a defused bomb for the CTs (2,750
// more each); a team with nobody left alive loses it (the CTs
// win only when no C4 is ticking: 2,750 each on a bomb map,
// else 2,000; the terrorists 2,000); nobody alive on either side
// is a draw; and with every hostage rescued or dead and at least
// half of them rescued, the CTs win (1,000 more each). The
// winners' score counts only once both teams have players. The
// next round starts five seconds later.
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

	pPlayer = UTIL_FindEntityByClassname( pPlayer, "player" );
	while ( pPlayer != NULL && !FNullEnt( pPlayer->edict() ) )
	{
		CBasePlayer *player = GetClassPtr( (CBasePlayer *)pPlayer->pev );

		if ( pPlayer->pev->flags != FL_DORMANT && player->m_iTeam != TEAM_UNASSIGNED )
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
		UTIL_ClientPrintAll( HUD_PRINTCONSOLE, "Scoring will not start until both teams have players.\n" );

		if ( !m_fGameCommenced )
		{
			TerminateRound( 0, 3 );
			m_fGameCommenced = true;
			return;
		}

		bNeededPlayers = TRUE;
	}

	if ( m_fTargetBombed == true && m_fMapHasBombTarget == true )
	{
		EndRoundMessage( "Target successfully bombed!" );
		Broadcast( "terwin" );
		m_iAccountTerrorist += 2750;

		if ( !bNeededPlayers )
		{
			m_iNumTerroristWins++;
			MESSAGE_BEGIN( MSG_ALL, gmsgTeamScore );
				WRITE_BYTE( TEAM_TERRORIST );
				WRITE_SHORT( m_iNumTerroristWins );
			MESSAGE_END();
		}

		TerminateRound( 5, 2 );
		return;
	}

	if ( m_fBombDefused == true && m_fMapHasBombTarget == true )
	{
		EndRoundMessage( "Bomb successfully defused!" );
		Broadcast( "ctwin" );
		m_iAccountCT += 2750;

		if ( !bNeededPlayers )
		{
			m_iNumCTWins++;
			MESSAGE_BEGIN( MSG_ALL, gmsgTeamScore );
				WRITE_BYTE( TEAM_CT );
				WRITE_SHORT( m_iNumCTWins );
			MESSAGE_END();
		}

		TerminateRound( 5, 1 );
		return;
	}

	if ( NumAliveTerrorist == 0 && NumDeadTerrorist != 0 && m_iNumCT > 0 )
	{
		ALERT( at_console, "The CTs win by elimination!!...\n" );

		// not while a C4 is ticking
		CBaseEntity *pC4 = UTIL_FindEntityByClassname( NULL, "grenade" );

		if ( pC4 && !((CGrenade *)pC4)->m_fC4Done )
		{
			ALERT( at_console, "... But there's a C4 bomb present.\n" );
			return;
		}

		EndRoundMessage( "Counter-Terrorists WIN" );
		Broadcast( "ctwin" );

		if ( m_fTargetBombed == false && m_fMapHasBombTarget == true )
			m_iAccountCT = 2750;
		else
			m_iAccountCT = 2000;

		if ( !bNeededPlayers )
		{
			m_iNumCTWins++;
			MESSAGE_BEGIN( MSG_ALL, gmsgTeamScore );
				WRITE_BYTE( TEAM_CT );
				WRITE_SHORT( m_iNumCTWins );
			MESSAGE_END();
		}

		TerminateRound( 5, 1 );
		return;
	}
	else if ( NumAliveCT == 0 && NumDeadCT != 0 && m_iNumTerrorist > 0 )
	{
		ALERT( at_console, "Wincondition ONE\n" );
		EndRoundMessage( "Terrorists WIN" );
		Broadcast( "terwin" );
		m_iAccountTerrorist = 2000;

		if ( !bNeededPlayers )
		{
			m_iNumTerroristWins++;
			MESSAGE_BEGIN( MSG_ALL, gmsgTeamScore );
				WRITE_BYTE( TEAM_TERRORIST );
				WRITE_SHORT( m_iNumTerroristWins );
			MESSAGE_END();
		}

		TerminateRound( 5, 2 );
		return;
	}
	else if ( NumAliveCT == 0 && NumAliveTerrorist == 0 )
	{
		EndRoundMessage( "Round DRAW" );
		Broadcast( "rounddraw" );
		TerminateRound( 5, 3 );
		return;
	}
	else
	{
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
			EndRoundMessage( "All hostages rescued!\nCounter-Terrorists WIN" );
			Broadcast( "ctwin" );
			m_iAccountCT += 1000;

			if ( !bNeededPlayers )
			{
				m_iNumCTWins++;
				MESSAGE_BEGIN( MSG_ALL, gmsgTeamScore );
					WRITE_BYTE( TEAM_CT );
					WRITE_SHORT( m_iNumCTWins );
				MESSAGE_END();
			}

			TerminateRound( 5, 1 );
		}
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
// RestartRound - Beta 5.0 (leak): the scores start afresh
// after sv_restartround (and at the first round); the round,
// freeze and C4 times are read again and the freeze period
// begins; the map's zones are looked for; the hostages go
// back to where they stood (250 to the winners for each one
// still standing, at most 2,000, sic: the walk stops there);
// the losers get the loser bonus (1,400, 500 more after each
// third loss in a row, up to 3,400; it falls back to 1,400
// when such a run ends), the CTs 150 for each rescued hostage;
// the C4 is dropped, everybody in a team respawns and is paid,
// the map is cleaned up and the C4 handed out.
//=========================================================
void CHalfLifeMultiplay::RestartRound( void )
{
	if ( m_fCompleteReset )
	{
		m_iNumTerroristWins = 0;
		m_iNumCTWins = 0;
		m_iNumConsecutiveTerroristLoses = 0;
		m_iNumConsecutiveCTLoses = 0;

		MESSAGE_BEGIN( MSG_ALL, gmsgTeamScore );
			WRITE_BYTE( TEAM_TERRORIST );
			WRITE_SHORT( 0 );
		MESSAGE_END();

		MESSAGE_BEGIN( MSG_ALL, gmsgTeamScore );
			WRITE_BYTE( TEAM_CT );
			WRITE_SHORT( 0 );
		MESSAGE_END();

		m_fCompleteReset = false;
	}

	m_iRoundTime = (int)CVAR_GET_FLOAT( "mp_roundtime" );
	m_iC4Timer = (int)CVAR_GET_FLOAT( "mp_c4timer" );
	m_iFreezeTime = (int)CVAR_GET_FLOAT( "mp_freezetime" );
	m_bFreezePeriod = TRUE;
	m_fRoundTerminating = false;
	m_flMaxIdlePeriod = m_iRoundTime * 120;

	if ( m_iRoundTime > 9 )
		m_iRoundTime = 9;
	else if ( m_iRoundTime < 3 )
		m_iRoundTime = 3;

	if ( m_iFreezeTime > 15 )
		m_iFreezeTime = 15;
	else if ( m_iFreezeTime < 0 )
		m_iFreezeTime = 0;

	if ( m_iC4Timer > 90 )
		m_iC4Timer = 90;
	else if ( m_iC4Timer < 10 )
		m_iC4Timer = 10;

	m_iRoundTimeSecs = m_iFreezeTime;

	if ( UTIL_FindEntityByClassname( NULL, "func_bomb_target" ) )
	{
		m_fMapHasBombTarget = true;
		m_fMapHasBombZone = true;
	}
	else
	{
		if ( UTIL_FindEntityByClassname( NULL, "info_bomb_target" ) )
			m_fMapHasBombTarget = true;
		else
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

	// the kick votes start afresh
	for ( int i = 0; i < 20; i++ )
		m_iKickVotes[i] = 0;

	CBaseEntity *pHostage = UTIL_FindEntityByClassname( NULL, "hostage_entity" );
	int iHostageBonus = 0;	// name ours

	while ( pHostage != NULL && iHostageBonus < 2000 )
	{
		if ( pHostage->pev->solid != SOLID_NOT )
		{
			// a hostage still standing in the map
			iHostageBonus += 250;

			if ( pHostage->pev->deadflag == DEAD_DEAD )
				pHostage->pev->deadflag = DEAD_RESPAWNABLE;
		}
		((CHostage *)pHostage)->RePosition();
		pHostage = UTIL_FindEntityByClassname( pHostage, "hostage_entity" );
	}

	// the loser bonus grows after three losses in a row
	if ( m_iRoundWinStatus == 2 )
	{
		if ( m_iNumConsecutiveTerroristLoses > 2 )
			m_iLoserBonus = 1400;

		m_iNumConsecutiveTerroristLoses = 0;
		m_iNumConsecutiveCTLoses++;
	}
	else if ( m_iRoundWinStatus == 1 )
	{
		if ( m_iNumConsecutiveCTLoses > 2 )
			m_iLoserBonus = 1400;

		m_iNumConsecutiveCTLoses = 0;
		m_iNumConsecutiveTerroristLoses++;
	}

	if ( ( m_iNumConsecutiveTerroristLoses > 2 && m_iLoserBonus < 2900 ) || ( m_iNumConsecutiveCTLoses > 2 && m_iLoserBonus < 2900 ) )
		m_iLoserBonus += 500;

	if ( m_iRoundWinStatus == 2 )
	{
		m_iAccountTerrorist += iHostageBonus;
		m_iAccountCT += m_iLoserBonus;
	}
	else if ( m_iRoundWinStatus == 1 )
	{
		m_iAccountCT += iHostageBonus;
		m_iAccountTerrorist += m_iLoserBonus;
	}

	m_iAccountCT += m_iHostagesRescued * 150;

	m_flRoundStartTime2 = m_flRoundStartTime = gpGlobals->time;

	// Beta 3.0: both teams get a new commander
	PickNewCommander( TEAM_TERRORIST );
	PickNewCommander( TEAM_CT );

	m_iUnused88 = 2;

	CBaseEntity *pPlayer = UTIL_FindEntityByClassname( NULL, "player" );

	while ( pPlayer != NULL && !FNullEnt( pPlayer->edict() ) )
	{
		if ( pPlayer->pev->flags != FL_DORMANT )
		{
			CBasePlayer *player = GetClassPtr( (CBasePlayer *)pPlayer->pev );

			if ( player->m_iTeam != TEAM_UNASSIGNED )
			{
				if ( player->m_fHasC4 == true )
					player->DropPlayerItem( "weapon_c4" );

				player->RoundRespawn();
			}

			if ( player->m_iTeam == TEAM_CT )
				player->AddAccount( m_iAccountCT, true );
			else if ( player->m_iTeam == TEAM_TERRORIST )
				player->AddAccount( m_iAccountTerrorist, true );
		}
		pPlayer = UTIL_FindEntityByClassname( pPlayer, "player" );
	}

	CleanUpMap();

	if ( m_fMapHasBombTarget == true )
		GiveC4();

	m_flIntermissionEndTime = 0;
	m_flRestartRoundTime = 0;
	m_iAccountCT = 0;
	m_iAccountTerrorist = 0;
	m_iHostagesRescued = 0;
	m_iUnused2C = 0;
	m_iRoundWinStatus = 0;
	m_fBombDefused = false;
	m_fTargetBombed = false;
	m_fCountedSpawnPoints = false;
}

// longest the intermission can last, in seconds
#define MAX_INTERMISSION_TIME		120

//=========================================================
// GiveWeapons - Beta 4.0: everybody gets a knife and a
// pistol with ammunition. Beta 5.0 (leak): the CTs a USP (48
// rounds), the terrorists a Glock 18 (120); commanders get
// nothing better any more (nothing calls it).
//=========================================================
void CHalfLifeMultiplay::GiveWeapons( void )
{
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBasePlayer *player = (CBasePlayer *)UTIL_PlayerByIndex( i );

		if ( player && player->IsPlayer() )
		{
			if ( player->m_iTeam == TEAM_CT )
			{
				player->GiveNamedItem( "weapon_knife" );
				player->GiveNamedItem( "weapon_usp" );
				player->GiveAmmo( 24, "45acp", 48 );
			}
			else if ( player->m_iTeam == TEAM_TERRORIST )
			{
				player->GiveNamedItem( "weapon_knife" );
				player->GiveNamedItem( "weapon_glock18" );
				player->GiveAmmo( 40, "9mm", 120 );
			}
		}
	}
}

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

void CHalfLifeMultiplay :: Think ( void )
{
	// Beta 5.0 (leak): the first think starts the clock
	if ( m_flRoundStartTime == 0 )
		m_flRoundStartTime2 = m_flRoundStartTime = gpGlobals->time;

	///// Check game rules /////

	if ( g_fGameOver )   // someone else quit the game already
	{
		if ( m_flIntermissionEndTime < gpGlobals->time )
		{
			if ( m_iEndIntermissionButtonHit  // check that someone has pressed a key, or the max intermission time is over
				|| ((m_flIntermissionEndTime + MAX_INTERMISSION_TIME) < gpGlobals->time) ) 
				ChangeLevel(); // intermission is over
		}
		return;
	}

	float flTimeLimit = timelimit.value * 60;

	if ( flTimeLimit != 0 && gpGlobals->time >= flTimeLimit )
	{
		GoToIntermission();
		return;
	}

	if ( IsFreezePeriod() )
	{
		// Beta 5.0 (leak): the freeze time is over: the round's clock
		// starts, with one of four radio calls (Beta 3.0 to 4.1 played it at
		// RestartRound)
		if ( TimeRemaining() <= 0 )
		{
			const char *szSound;	// name ours

			m_bFreezePeriod = FALSE;

			switch ( RANDOM_LONG( 0, 3 ) )
			{
			case 0:
				szSound = "radio/moveout.wav";
				break;
			case 1:
				szSound = "radio/letsgo.wav";
				break;
			case 2:
				szSound = "radio/locknload.wav";
				break;
			default:
				szSound = "radio/com_go.wav";
				break;
			}

			m_flRoundStartTime = gpGlobals->time;
			m_iRoundTimeSecs = m_iRoundTime * 60;

			for ( int i = 1; i <= gpGlobals->maxClients; i++ )
			{
				CBasePlayer *plr = (CBasePlayer *)UTIL_PlayerByIndex( i );

				if ( plr && plr->pev->flags != FL_DORMANT && plr->m_iJoiningState == JOIN_DONE )
				{
					EMIT_SOUND( ENT(plr->pev), CHAN_VOICE, szSound, 1, 0.8 );
					plr->SyncRoundTimer();
					plr->ResetMaxSpeed();
				}
			}
		}
	}
	else if ( TimeRemaining() <= 0 )
	{
		// the round's time is up: a draw; a ticking C4 ends it when it goes
		// off. The restart time passed on is already a time, and TerminateRound
		// adds the current time to it (sic)
		float flEndRoundTime;	// name ours
		CBaseEntity *pC4 = UTIL_FindEntityByClassname( NULL, "grenade" );

		if ( pC4 )
		{
			if ( !((CGrenade *)pC4)->m_fC4Done )
				flEndRoundTime = ((CGrenade *)pC4)->m_flC4Blow;
			else
				flEndRoundTime = gpGlobals->time + 5.0;
		}
		else
			flEndRoundTime = gpGlobals->time + 5.0;

		EndRoundMessage( "Round DRAW" );
		Broadcast( "rounddraw" );
		TerminateRound( flEndRoundTime, 3 );
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
	// (at most 10), the scores zeroed
	if ( gpGlobals->time > m_flNextRestartCheck )
	{
		int iRestartDelay = (int)CVAR_GET_FLOAT( "sv_restartround" );	// name ours

		if ( iRestartDelay > 0 )
		{
			if ( iRestartDelay > 10 )
				iRestartDelay = 10;

			EndRoundMessage( UTIL_VarArgs( "The round will restart in\n%i SECONDS", iRestartDelay ) );
			m_flRestartRoundTime = gpGlobals->time + iRestartDelay;
			m_fCompleteReset = true;
			CVAR_SET_FLOAT( "sv_restartround", 0 );
		}

		m_flNextRestartCheck = gpGlobals->time + 0.5;
	}
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
			if ( pCheck->iWeight() > -1 && pCheck->iWeight() == pCurrentWeapon->iWeight() && pCheck != pCurrentWeapon )
			{
				// this weapon is from the same category. 
				if ( pCheck->CanDeploy() )
				{
					if ( pPlayer->SwitchWeapon( pCheck ) )
					{
						return TRUE;
					}
				}
			}
			else if ( pCheck->iWeight() > iBestWeight && pCheck != pCurrentWeapon )// don't reselect the weapon we're trying to get rid of
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

//=========================================================
//=========================================================
BOOL CHalfLifeMultiplay :: ClientConnected( edict_t *pEntity, const char *pszName, const char *pszAddress, char szRejectReason[ 128 ] )
{
	return TRUE;
}

extern int gmsgSayText;
extern int gmsgGameMode;

void CHalfLifeMultiplay :: UpdateGameMode( CBasePlayer *pPlayer )
{
	MESSAGE_BEGIN( MSG_ONE, gmsgGameMode, NULL, pPlayer->edict() );
		WRITE_BYTE( 0 );  // game mode none
	MESSAGE_END();
}

void CHalfLifeMultiplay :: InitHUD( CBasePlayer *pl )
{
	// Beta 5.0 (leak): no "has joined the game" for the others any more
	UTIL_LogPrintf( "\"%s<%i>\" has entered the game\n",  STRING( pl->pev->netname ), GETPLAYERUSERID( pl->edict() ) );

	UpdateGameMode( pl );

	// sending just one score makes the hud scoreboard active;  otherwise
	// it is just disabled for single play
	MESSAGE_BEGIN( MSG_ONE, gmsgScoreInfo, NULL, pl->edict() );
		WRITE_BYTE( ENTINDEX(pl->edict()) );
		WRITE_SHORT( 0 );
		WRITE_SHORT( 0 );
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
			MESSAGE_END();
		}
	}

	// Beta 5.0 (leak): the teams' scores
	MESSAGE_BEGIN( MSG_ONE, gmsgTeamScore, NULL, pl->edict() );
		WRITE_BYTE( TEAM_TERRORIST );
		WRITE_SHORT( m_iNumTerroristWins );
	MESSAGE_END();

	MESSAGE_BEGIN( MSG_ONE, gmsgTeamScore, NULL, pl->edict() );
		WRITE_BYTE( TEAM_CT );
		WRITE_SHORT( m_iNumCTWins );
	MESSAGE_END();

	if ( g_fGameOver )
	{
		MESSAGE_BEGIN( MSG_ONE, SVC_INTERMISSION, NULL, pl->edict() );
		MESSAGE_END();
	}

	// Beta 5.0 (leak): no defuse kit icon, and every player's team and
	// scoreboard flags
	MESSAGE_BEGIN( MSG_ONE, gmsgStatusIcon, NULL, pl->pev );
		WRITE_BYTE( 0 );
		WRITE_STRING( "defuser" );
	MESSAGE_END();

	for ( i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBasePlayer *plr = (CBasePlayer *)UTIL_PlayerByIndex( i );

		if ( plr )
		{
			MESSAGE_BEGIN( MSG_ONE, gmsgTeamInfo, NULL, pl->edict() );
				WRITE_BYTE( plr->entindex() );
				WRITE_BYTE( plr->m_iTeam );
			MESSAGE_END();

			plr->SetScoreboardAttributes( pl );
		}
	}
}

//=========================================================
//=========================================================
void CHalfLifeMultiplay :: ClientDisconnected( edict_t *pClient )
{
	if ( pClient )
	{
		CBasePlayer *pPlayer = (CBasePlayer *)CBaseEntity::Instance( pClient );

		if ( pPlayer )
		{
			// Beta 5.0 (leak): he drops the C4; his defuse kit stays where he was
			if ( pPlayer->m_fHasC4 == true )
				pPlayer->DropPlayerItem( "weapon_c4" );

			if ( pPlayer->m_fHasDefuser == true )
				pPlayer->GiveNamedItem( "item_thighpack" );

			FireTargets( "game_playerleave", pPlayer, pPlayer, USE_TOGGLE, 0 );
			UTIL_LogPrintf( "\"%s<%i>\" disconnected\n",  STRING( pPlayer->pev->netname ), GETPLAYERUSERID( pPlayer->edict() ) );

			pPlayer->RemoveAllItems( TRUE );// destroy all of the players weapons and items

			// Beta 3.0: a counter-terrorist leaving makes the CTs pick a new
			// commander (sic: whoever he was, and not so for a terrorist)
			if ( pPlayer->m_iTeam == TEAM_CT )
				PickNewCommander( TEAM_CT );

			// Beta 5.0 (leak): his observer goes
			if ( pPlayer->m_pObserver )
				pPlayer->m_pObserver->SUB_Remove();
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

	// Beta 5.0 (leak): a joining player gets the team menu
	if ( pPlayer->m_iMenu != Menu_ChooseTeam && pPlayer->m_iJoiningState == JOIN_TEAM_MENU )
	{
		ShowMenu( pPlayer, 7, -1, FALSE, "#Team_Select" );	// keys 1 to 3
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

	// Beta 3.0: a survivor of the last round keeps what he has
	if ( pPlayer->m_fNotKilled == true )
		addDefault = FALSE;

	// Beta 5.0 (leak): a knife and the team's pistol for everybody, commander
	// or not: the CTs a USP (48 rounds), the terrorists a Glock 18 (120)
	if ( pPlayer->m_iTeam == TEAM_CT && addDefault )
	{
		pPlayer->GiveNamedItem( "weapon_knife" );
		pPlayer->GiveNamedItem( "weapon_usp" );
		pPlayer->GiveAmmo( 24, "45acp", 48 );
	}
	else if ( pPlayer->m_iTeam == TEAM_TERRORIST && addDefault )
	{
		pPlayer->GiveNamedItem( "weapon_knife" );
		pPlayer->GiveNamedItem( "weapon_glock18" );
		pPlayer->GiveAmmo( 40, "9mm", 120 );
	}

	// Beta 5.0 (leak): the model the player chose (SetPlayerModel)
	pPlayer->SetPlayerModel( FALSE );
}

//=========================================================
//=========================================================
BOOL CHalfLifeMultiplay :: FPlayerCanRespawn( CBasePlayer *pPlayer )
{
	// Beta 5.0 (leak): once a round; not when both teams have players and
	// the round is more than 20 seconds old; not before an appearance is chosen
	if ( pPlayer->m_iNumSpawns > 0 )
		return FALSE;

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
	return ( CVAR_GET_FLOAT( "mp_autocrosshair" ) != 0 );
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
	pVictim->m_fNotKilled = false;

	// Beta 4.0: a dead commander, of either team, gets a successor
	if ( ( pVictim->m_iTeam == TEAM_CT && m_pCTCommander == pVictim ) || ( pVictim->m_iTeam == TEAM_TERRORIST && m_pTerroristCommander == pVictim ) )
		PickNewCommander( pVictim->m_iTeam );

	CBasePlayer *peKiller = NULL;
	CBaseEntity *ktmp = CBaseEntity::Instance( pKiller );
	if ( ktmp && (ktmp->Classify() == CLASS_PLAYER) )
		peKiller = (CBasePlayer*)ktmp;

	FireTargets( "game_playerdie", pVictim, pVictim, USE_TOGGLE, 0 );

	if ( pVictim->pev == pKiller )  
	{  // killed self
		pKiller->frags -= 1;
	}
	else if ( ktmp && ktmp->IsPlayer() )
	{
		CBasePlayer *pKillerPlayer = GetClassPtr( (CBasePlayer *)pKiller );	// name ours

		// if a player dies in a deathmatch game and the killer is a client, award the killer some points
		pKiller->frags += IPointsForKill( peKiller, pVictim );

		// Beta 5.0 (leak): a team kill costs 3,300 and is counted (the
		// grenade exception is gone); at the third, with mp_autokick, the
		// killer is kicked; an enemy pays 300
		if ( pKillerPlayer->m_iTeam == pVictim->m_iTeam )
		{
			pKillerPlayer->AddAccount( -3300, true );
			pKillerPlayer->m_iTeamKills++;
			ClientPrint( pKillerPlayer->pev, HUD_PRINTCENTER, "You killed a teammate!" );
			ClientPrint( pKillerPlayer->pev, HUD_PRINTCONSOLE, UTIL_VarArgs( "Teammate kills: %i of 3\n", pKillerPlayer->m_iTeamKills ) );

			if ( pKillerPlayer->m_iTeamKills == 3 && CVAR_GET_FLOAT( "mp_autokick" ) != 0 )
			{
				ClientPrint( pKillerPlayer->pev, HUD_PRINTCONSOLE, "You are being banned from the server for killing teammates\n" );

				int iUserID = GETPLAYERUSERID( pKillerPlayer->edict() );	// name ours
				if ( iUserID != -1 )
					SERVER_COMMAND( UTIL_VarArgs( "kick # %d\n", iUserID ) );
			}

			if ( !( pKillerPlayer->m_flDisplayHistory & ( 1 << 10 ) ) )	// the team kill hint
			{
				pKillerPlayer->m_flDisplayHistory |= ( 1 << 10 );
				pKillerPlayer->HintMessage( "Careful!\nKilling teammates will not be tolerated!", FALSE );
			}
		}
		else
		{
			pKillerPlayer->AddAccount( 300, true );

			if ( m_flRestartRoundTime == 0 )
				ClientPrint( pKillerPlayer->pev, HUD_PRINTCENTER, "You killed an enemy" );

			if ( !( pKillerPlayer->m_flDisplayHistory & ( 1 << 11 ) ) )	// the enemy kill hint
			{
				pKillerPlayer->m_flDisplayHistory |= ( 1 << 11 );
				pKillerPlayer->HintMessage( "You killed an Enemy!\nWin the round by eliminating the opposing force.", FALSE );
			}
		}
		
		FireTargets( "game_playerkill", ktmp, ktmp, USE_TOGGLE, 0 );
	}
	else
	{  // killed by the world
		pKiller->frags -= 1;
	}

	// update the scores
	// killed scores
	MESSAGE_BEGIN( MSG_ALL, gmsgScoreInfo );
		WRITE_BYTE( ENTINDEX(pVictim->edict()) );
		WRITE_SHORT( pVictim->pev->frags );
		WRITE_SHORT( pVictim->m_iDeaths );
	MESSAGE_END();

	// killers score, if it's a player
	CBaseEntity *ep = CBaseEntity::Instance( pKiller );
	if ( ep && ep->Classify() == CLASS_PLAYER )
	{
		CBasePlayer *PK = (CBasePlayer*)ep;

		MESSAGE_BEGIN( MSG_ALL, gmsgScoreInfo );
			WRITE_BYTE( ENTINDEX(PK->edict()) );
			WRITE_SHORT( PK->pev->frags );
			WRITE_SHORT( PK->m_iDeaths );
		MESSAGE_END();

		// let the killer paint another decal as soon as he'd like.
		PK->m_flNextDecalTime = gpGlobals->time;
	}
#ifndef HLDEMO_BUILD
	if ( pVictim->HasNamedPlayerItem("weapon_satchel") )
	{
		DeactivateSatchels( pVictim );
	}
#endif
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
				
				if ( pPlayer->m_pActiveItem )
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

	MESSAGE_BEGIN( MSG_ALL, gmsgDeathMsg );
		WRITE_BYTE( killer_index );						// the killer
		WRITE_BYTE( ENTINDEX(pVictim->edict()) );		// the victim
		WRITE_STRING( killer_weapon_name );		// what they were killed by (should this be a string?)
	MESSAGE_END();

	// replace the code names with the 'real' names
	if ( !strcmp( killer_weapon_name, "egon" ) )
		killer_weapon_name = gluon;
	else if ( !strcmp( killer_weapon_name, "gauss" ) )
		killer_weapon_name = tau;

	if ( pVictim->pev == pKiller )  
	{  // killed self
		UTIL_LogPrintf( "\"%s<%i>\" killed self with %s\n",  STRING( pVictim->pev->netname ), GETPLAYERUSERID( pVictim->edict() ), killer_weapon_name );
	}
	else if ( pKiller->flags & FL_CLIENT )
	{
		UTIL_LogPrintf( "\"%s<%i>\" killed \"%s<%i>\" with %s\n",  STRING( pKiller->netname ),
			GETPLAYERUSERID( ENT(pKiller) ),
			STRING( pVictim->pev->netname ),
			GETPLAYERUSERID( pVictim->edict() ),
			killer_weapon_name );
	}
	else
	{  // killed by the world
		UTIL_LogPrintf( "\"%s<%i>\" killed by world with %s\n",  STRING( pVictim->pev->netname ), GETPLAYERUSERID( pVictim->edict() ), killer_weapon_name );
	}

	CheckWinConditions();

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

BOOL CHalfLifeMultiplay :: PlayFootstepSounds( CBasePlayer *pl, float fvol )
{
	if ( CVAR_GET_FLOAT( "mp_footsteps" ) == 0 )
		return FALSE;

	if ( pl->IsOnLadder() || pl->pev->velocity.Length2D() > 220 )
		return TRUE;  // only make step sounds in multiplayer if the player is moving fast enough

	return FALSE;
}

BOOL CHalfLifeMultiplay :: FAllowFlashlight( void ) 
{ 
	return CVAR_GET_FLOAT( "mp_flashlight" ) != 0; 
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

	MESSAGE_BEGIN(MSG_ALL, SVC_INTERMISSION);
	MESSAGE_END();

	m_flIntermissionEndTime = gpGlobals->time + INTERMISSION_TIME;
	g_fGameOver = TRUE;
	m_iEndIntermissionButtonHit = FALSE;

	// Beta 5.0 (leak): the next map's spawn points are counted afresh
	m_iSpawnPointCount_Terrorist = 0;
	m_iSpawnPointCount_CT = 0;
	m_fCountedSpawnPoints = false;
}

void CHalfLifeMultiplay :: ChangeLevel( void )
{
	char szNextMap[32];
	char szFirstMapInList[32];
	strcpy( szFirstMapInList, "hldm1" );  // the absolute default level is hldm1

	// find the map to change to

	char *mapcfile = (char*)CVAR_GET_STRING( "mapcyclefile" );
	ASSERT( mapcfile != NULL );
	strcpy( szNextMap, STRING(gpGlobals->mapname) );
	strcpy( szFirstMapInList, STRING(gpGlobals->mapname) );

	int length;
	char *pFileList;
	char *aFileList = pFileList = (char*)LOAD_FILE_FOR_ME( mapcfile, &length );
	if ( pFileList && length )
	{
		// the first map name in the file becomes the default
		sscanf( pFileList, " %32s", szNextMap );
		if ( IS_MAP_VALID( szNextMap ) )
			strcpy( szFirstMapInList, szNextMap );

		// keep pulling mapnames out of the list until the current mapname
		// if the current mapname isn't found,  load the first map in the list
		BOOL next_map_is_it = FALSE;
		while ( 1 )
		{
			while ( *pFileList && isspace( *pFileList ) ) pFileList++; // skip over any whitespace
			if ( !(*pFileList) )
				break;

			char cBuf[32];
			int ret = sscanf( pFileList, " %32s", cBuf );
			// Check the map name is valid
			if ( ret != 1 || *cBuf < 13 )
				break;

			if ( next_map_is_it )
			{
				// check that it is a valid map file
				if ( IS_MAP_VALID( cBuf ) )
				{
					strcpy( szNextMap, cBuf );
					break;
				}
			}

			if ( FStrEq( cBuf, STRING(gpGlobals->mapname) ) )
			{  // we've found our map;  next map is the one to change to
				next_map_is_it = TRUE;
			}

			pFileList += strlen( cBuf );
		}

		FREE_FILE( aFileList );
	}

	if ( !IS_MAP_VALID(szNextMap) )
		strcpy( szNextMap, szFirstMapInList );

	g_fGameOver = TRUE;

	ALERT( at_console, "CHANGE LEVEL: %s\n", szNextMap );
	
	CHANGE_LEVEL( szNextMap, NULL );
}

#define MAX_MOTD_CHUNK	  60
#define MAX_MOTD_LENGTH   (MAX_MOTD_CHUNK * 4)

void CHalfLifeMultiplay :: SendMOTDToClient( edict_t *client )
{
	// read from the MOTD.txt file
	int length, char_count = 0;
	char *pFileList;
	char *aFileList = pFileList = (char*)LOAD_FILE_FOR_ME( "motd.txt", &length );

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
	


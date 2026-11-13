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

extern char g_a, g_b, g_c, g_d, g_e, g_f, g_g, g_h, g_i, g_j, g_k, g_l, g_m,
	g_n, g_o, g_p, g_q, g_r, g_s, g_t, g_u, g_v, g_w, g_x, g_y, g_z,
	g_1, g_2, g_3, g_4, g_5, g_6, g_7, g_8, g_9, g_0;	// client.cpp (Beta 7.0)

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

// EndRoundMessage - Beta 5.0 (leak; Linux symbol): a message in the middle of everybody's screen.
// Beta 7.1: the HUD message's parameters (to 7.0 a static hudtextparms_t here: green, four seconds)
// are gone from the image with UTIL_HudMessageAll's call

void EndRoundMessage( const char *sentence )
{
	char szBuf[256];	// Beta 6.1 (name ours)

	// Beta 7.1: the sentence is a name of the client's titles.txt ("#CTs_Win"), printed
	// in the middle of the screen (to 7.0: UTIL_HudMessageAll)
	UTIL_ClientPrintAll( HUD_PRINTCENTER, sentence );

	// Beta 6.1: the round's end and the score go to the server log
	strcpy( szBuf, "***" );
	strcat( szBuf, sentence );
	strcat( szBuf, "***\n" );
	UTIL_LogPrintf( szBuf );

	if ( g_pGameRules )
	{
		// (Beta 7.0's words, the stray '-' too; to 6.6 "CT wins = %i\nTerrorist wins = %i\n")
		sprintf( szBuf, "*** CT wins = %i ***\n*** Terrorist wins = %i -***\n", ((CHalfLifeMultiplay *)g_pGameRules)->m_iNumCTWins, ((CHalfLifeMultiplay *)g_pGameRules)->m_iNumTerroristWins );
		UTIL_LogPrintf( szBuf );
	}
}

// ReadMultiplayCvars - Beta 5.0 (Linux symbol): the round time, C4 timer, freeze time and team limit from their cvars,
// each clamped and the cvar set back to the clamp. sic: a round time under 3 minutes becomes 1, a freeze time over 15
// seconds sets mp_roundtime to 9; a team limit under 1 (the cvar set to 0) means 20, no limit
static void ReadMultiplayCvars( CHalfLifeMultiplay *mp )
{
	mp->m_iRoundTime = (int)CVAR_GET_FLOAT( "mp_roundtime" );
	mp->m_iC4Timer = (int)CVAR_GET_FLOAT( "mp_c4timer" );
	mp->m_iFreezeTime = (int)CVAR_GET_FLOAT( "mp_freezetime" );
	mp->m_iLimitTeams = (int)CVAR_GET_FLOAT( "mp_limitteams" );

	if ( mp->m_iRoundTime > 9 )
	{
		CVAR_SET_FLOAT( "mp_roundtime", 9 );
		mp->m_iRoundTime = 9;
	}
	else if ( mp->m_iRoundTime < 3 )
	{
		CVAR_SET_FLOAT( "mp_roundtime", 1 );
		mp->m_iRoundTime = 1;
	}

	if ( mp->m_iFreezeTime > 15 )
	{
		CVAR_SET_FLOAT( "mp_roundtime", 9 );
		mp->m_iFreezeTime = 15;
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

//*********************************************************
// Rules for the half-life multiplayer game.
//*********************************************************

CHalfLifeMultiplay :: CHalfLifeMultiplay()
{
	RefreshSkillData();
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
	m_fCompleteReset = true;

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
	m_iMaxRounds = CVAR_GET_FLOAT( "mp_maxrounds" );	// Counter-Strike 1.0 (Beta 7.1 read it only at a complete restart)
	m_iTotalRoundsPlayed = 0;	// Beta 7.1

	// Beta 6.5: the map votes, the VIP's turn
	for ( int i = 0; i < 100; i++ )
		m_iMapVotes[i] = 0;

	m_iLastPick = 1;

	m_bMapHasEscapeZone = false;
	m_iMapHasVIPSafetyZone = 0;
	m_fMapHasBombZone = false;
	m_fMapHasRescueZone = false;

	m_pVIPQueue[0] = m_pVIPQueue[1] = m_pVIPQueue[2] = m_pVIPQueue[3] = m_pVIPQueue[4] = NULL;

	// Beta 4.0: no high-detail player models
	CVAR_SET_FLOAT( "cl_himodels", 0 );

	// Beta 5.0: the cvars are read (and clamped) as RestartRound reads them; the first
	// round's freeze time is 2 seconds longer, after the clamp
	ReadMultiplayCvars( this );

	m_iFreezeTime += 2;
	m_flMaxIdlePeriod = m_iRoundTime * 120;
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
			
			// Beta 6.0: the dedicated server says nothing (the listen server still does)
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

	// Beta 6.5: every grenade goes (Beta 6.1: only the first). sic: the loop
	// runs while a grenade is found or more than ten were counted, so with
	// more than ten grenades it does not end
	int icount = 0;	// Counter-Strike 1.6's name
	CBaseEntity *toremove = UTIL_FindEntityByClassname( NULL, "grenade" );

	while ( toremove || icount > 10 )
	{
		UTIL_Remove( toremove );
		toremove = UTIL_FindEntityByClassname( toremove, "grenade" );
		icount++;
	}

	int iLeft = 4;
	toremove = UTIL_FindEntityByClassname( NULL, "item_thighpack" );

	while ( toremove && iLeft > 0 )
	{
		iLeft--;	// Beta 6.0: no console line
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
				player->m_bHasC4 = true;
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
		UTIL_ClientPrintAll( HUD_PRINTCONSOLE, "Scoring will not start until both teams have players.\n" );

		if ( !m_fGameCommenced )
		{
			UTIL_LogPrintf( "*** First player has joined the server. Game Commencing! ***\n" );
			TerminateRound( 0, 3 );
			m_fGameCommenced = true;
			return;
		}

		bNeededPlayers = TRUE;
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

			EndRoundMessage( "#VIP_Escaped" );

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

			EndRoundMessage( "#VIP_Assasinated" );

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

			EndRoundMessage( "#Terrorists_Escaped" );

			TerminateRound( 5, 2 );
			return;
		}
		else if ( NumAliveTerrorist == 0 )
		{
			Broadcast( "ctwin" );
			m_iAccountCT += ( 1 - flEscapeRatio ) * 3500;

			if ( !bNeededPlayers )
			{
				m_iNumCTWins++;
				UpdateTeamScores();
			}

			EndRoundMessage( "#CTs_PreventEscape" );

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

		EndRoundMessage( "#Target_Bombed" );

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

		EndRoundMessage( "#Bomb_Defused" );

		TerminateRound( 5, 1 );
		return;
	}

	// Beta 5.0: only against a team with players past the appearance menu
	if ( NumAliveTerrorist == 0 && NumDeadTerrorist != 0 && m_iNumSpawnableCT > 0 )
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

		EndRoundMessage( "#CTs_Win" );

		TerminateRound( 5, 1 );
		return;
	}
	else if ( NumAliveCT == 0 && NumDeadCT != 0 && m_iNumSpawnableTerrorist > 0 )
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

		EndRoundMessage( "#Terrorists_Win" );

		TerminateRound( 5, 2 );
		return;
	}
	else if ( NumAliveCT == 0 && NumAliveTerrorist == 0 )
	{
		EndRoundMessage( "#Round_Draw" );
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
			Broadcast( "ctwin" );
			m_iAccountCT += 2500;	// Beta 6.5 (5.0 to 6.1: 2000; the leak: 1000)

			if ( !bNeededPlayers )
			{
				m_iNumCTWins++;
				UpdateTeamScores();
			}

			EndRoundMessage( "#All_Hostages_Rescued" );

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
// each time its player with the most frags. Beta 6.5: on an
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
		int iHighestFrags = -100;
		CBasePlayer *pPlayerToSwap = NULL;
		CBaseEntity *pPlayer = NULL;

		pPlayer = UTIL_FindEntityByClassname( pPlayer, "player" );
		while ( pPlayer != NULL && !FNullEnt( pPlayer->edict() ) )
		{
			if ( pPlayer->pev->flags != FL_DORMANT )
			{
				CBasePlayer *player = GetClassPtr( (CBasePlayer *)pPlayer->pev );

				// Beta 6.1: never the VIP
				if ( player->m_iTeam == iTeamToSwap && player->pev->frags >= iHighestFrags && m_pVIP != player )
				{
					iHighestFrags = player->pev->frags;
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

	// Beta 6.0: the movement settings, every round
	CVAR_SET_FLOAT( "sv_accelerate", 5 );
	CVAR_SET_FLOAT( "sv_friction", 4 );
	CVAR_SET_FLOAT( "sv_stopspeed", 75 );

	// Beta 6.0: mp_autoteambalance moves players when the teams have been two
	// or more apart; Beta 6.1: after one such round, announced as soon as the
	// first one ends (6.0 waited more than two rounds)
	m_iNumCT = CountTeamPlayers( TEAM_CT );
	m_iNumTerrorist = CountTeamPlayers( TEAM_TERRORIST );

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
		// are counted from zero and mp_maxrounds is read
		timelimit.value += (int)( gpGlobals->time / 60 );
		m_iTotalRoundsPlayed = 0;
		m_iMaxRounds = (int)CVAR_GET_FLOAT( "mp_maxrounds" );

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

	m_flMaxIdlePeriod = m_iRoundTime * 120;
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
				if ( player->m_bHasC4 == true )
					player->DropPlayerItem( "weapon_c4" );

				player->RoundRespawn();

				// Beta 5.0: after a complete reset everybody gets the starting weapons again
				if ( m_fCompleteReset )
					player->GiveDefaultItems();
			}

			if ( player->m_iTeam == TEAM_CT )
				player->AddAccount( m_iAccountCT, true );
			else if ( player->m_iTeam == TEAM_TERRORIST )
			{
				// Beta 6.0: the escapers are counted; on an escape map the
				// terrorists start with the starting weapons every round
				m_iNumEscapers++;
				player->AddAccount( m_iAccountTerrorist, true );

				if ( m_bMapHasEscapeZone == true )
					player->GiveDefaultItems();
			}
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
	m_fCompleteReset = false;
}

// longest the intermission can last, in seconds
#define MAX_INTERMISSION_TIME		120

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
	case TEAM_TERRORIST:
		if ( curTeam_id != TEAM_UNASSIGNED )
			return ( m_iNumTerrorist + 1 ) > ( m_iLimitTeams + m_iNumCT - 1 );
		else
			return ( m_iNumTerrorist + 1 ) > ( m_iLimitTeams + m_iNumCT );

	case TEAM_CT:
		if ( curTeam_id != TEAM_UNASSIGNED )
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
//=========================================================
bool CHalfLifeMultiplay::IsVIPQueueEmpty( void )
{
	for ( int i = 0; i < 5; i++ )
	{
		CBasePlayer *toCheck = m_pVIPQueue[i];	// name ours

		if ( toCheck && toCheck->m_iTeam == TEAM_TERRORIST )
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
	char text[256];	// name ours
	int i;

	IsVIPQueueEmpty();

	if ( toAdd->m_iTeam != TEAM_CT )
		return false;

	for ( i = 0; i <= 4; i++ )
	{
		if ( m_pVIPQueue[i] == toAdd )
		{
			sprintf( text, "You are already in position %i of 5\n", i + 1 );
			ClientPrint( toAdd->pev, HUD_PRINTCENTER, text );
			return false;
		}
	}

	for ( i = 0; i <= 4; i++ )
	{
		if ( m_pVIPQueue[i] == NULL )
		{
			m_pVIPQueue[i] = toAdd;
			StackVIPQueue();

			sprintf( text, "You have been added to position %i of 5\n", i + 1 );
			ClientPrint( toAdd->pev, HUD_PRINTCENTER, text );
			return true;
		}
	}

	ClientPrint( toAdd->pev, HUD_PRINTCENTER, "#All_VIP_Slots_Full" );
	return false;
}

//=========================================================
// ResetCurrentVIP - the VIP becomes an ordinary counter-
// terrorist with a random model (urban twice as likely as
// gsg9 or gign) and loses what he carried over
//=========================================================
void CHalfLifeMultiplay::ResetCurrentVIP( void )
{
	char *szModelUrban = new char[13];
	char *szUrban = new char[6];

	szModelUrban[0] = g_m;	szModelUrban[5] = ' ';	szModelUrban[10] = g_n;
	szModelUrban[1] = g_o;	szModelUrban[6] = g_u;	szModelUrban[11] = '\n';
	szModelUrban[2] = g_d;	szModelUrban[7] = g_r;	szModelUrban[12] = '\0';
	szModelUrban[3] = g_e;	szModelUrban[8] = g_b;
	szModelUrban[4] = g_l;	szModelUrban[9] = g_a;

	szUrban[0] = g_u;
	szUrban[1] = g_r;
	szUrban[2] = g_b;
	szUrban[3] = g_a;
	szUrban[4] = g_n;
	szUrban[5] = '\0';

	char *szModelGSG9 = new char[12];
	char *szGSG9 = new char[5];

	szModelGSG9[0] = g_m;	szModelGSG9[5] = ' ';	szModelGSG9[10] = '\n';
	szModelGSG9[1] = g_o;	szModelGSG9[6] = g_g;	szModelGSG9[11] = '\0';
	szModelGSG9[2] = g_d;	szModelGSG9[7] = g_s;
	szModelGSG9[3] = g_e;	szModelGSG9[8] = g_g;
	szModelGSG9[4] = g_l;	szModelGSG9[9] = g_9;

	szGSG9[0] = g_g;
	szGSG9[1] = g_s;
	szGSG9[2] = g_g;
	szGSG9[3] = g_9;
	szGSG9[4] = '\0';

	char *szModelSAS = new char[11];
	char *szSAS = new char[4];

	szModelSAS[0] = g_m;	szModelSAS[5] = ' ';	szModelSAS[10] = '\0';
	szModelSAS[1] = g_o;	szModelSAS[6] = g_s;
	szModelSAS[2] = g_d;	szModelSAS[7] = g_a;
	szModelSAS[3] = g_e;	szModelSAS[8] = g_s;
	szModelSAS[4] = g_l;	szModelSAS[9] = '\n';

	szSAS[0] = g_s;
	szSAS[1] = g_a;
	szSAS[2] = g_s;
	szSAS[3] = '\0';

	char *szModelGIGN = new char[12];
	char *szGIGN = new char[5];

	szModelGIGN[0] = g_m;	szModelGIGN[5] = ' ';	szModelGIGN[10] = '\n';
	szModelGIGN[1] = g_o;	szModelGIGN[6] = g_g;	szModelGIGN[11] = '\0';
	szModelGIGN[2] = g_d;	szModelGIGN[7] = g_i;
	szModelGIGN[3] = g_e;	szModelGIGN[8] = g_g;
	szModelGIGN[4] = g_l;	szModelGIGN[9] = g_n;

	szGIGN[0] = g_g;
	szGIGN[1] = g_i;
	szGIGN[2] = g_g;
	szGIGN[3] = g_n;
	szGIGN[4] = '\0';

	switch ( RANDOM_LONG( 0, 3 ) )
	{
	case 0:
		m_pVIP->m_iAppearance = 1;
		CLIENT_COMMAND( m_pVIP->edict(), szModelUrban );
		g_engfuncs.pfnSetClientKeyValue( m_pVIP->entindex(), g_engfuncs.pfnGetInfoKeyBuffer( m_pVIP->edict() ), "model", szUrban );
		break;
	case 1:
		m_pVIP->m_iAppearance = 5;
		CLIENT_COMMAND( m_pVIP->edict(), szModelGSG9 );
		g_engfuncs.pfnSetClientKeyValue( m_pVIP->entindex(), g_engfuncs.pfnGetInfoKeyBuffer( m_pVIP->edict() ), "model", szGSG9 );
		break;
	case 2:
		m_pVIP->m_iAppearance = 7;
		CLIENT_COMMAND( m_pVIP->edict(), szModelSAS );
		g_engfuncs.pfnSetClientKeyValue( m_pVIP->entindex(), g_engfuncs.pfnGetInfoKeyBuffer( m_pVIP->edict() ), "model", szSAS );
		break;
	case 3:
		m_pVIP->m_iAppearance = 6;
		CLIENT_COMMAND( m_pVIP->edict(), szModelGIGN );
		g_engfuncs.pfnSetClientKeyValue( m_pVIP->entindex(), g_engfuncs.pfnGetInfoKeyBuffer( m_pVIP->edict() ), "model", szGIGN );
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
		CBasePlayer *player;
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
					iCount++;

				pLastPlayer = player;
			}
			pPlayer = UTIL_FindEntityByClassname( pPlayer, "player" );
		}
	}
	else if ( m_pVIP == NULL )
	{
		// the first counter-terrorist
		CBaseEntity *pPlayer = NULL;

		pPlayer = UTIL_FindEntityByClassname( pPlayer, "player" );
		while ( pPlayer != NULL && !FNullEnt( pPlayer->edict() ) )
		{
			if ( pPlayer->pev->flags != FL_DORMANT )
			{
				CBasePlayer *player = GetClassPtr( (CBasePlayer *)pPlayer->pev );

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

	// Beta 7.1: with mp_maxrounds set, the map ends after so many rounds
	// and the time limit does not count
	if ( m_iMaxRounds == 0 )
	{
		if ( flTimeLimit != 0 && gpGlobals->time >= flTimeLimit )
		{
			GoToIntermission();
			return;
		}
	}
	else if ( m_iTotalRoundsPlayed >= m_iMaxRounds )
	{
		ALERT( at_console, "Changing maps due to maximum rounds have been met\n" );	// Counter-Strike 1.0
		GoToIntermission();
		return;
	}

	if ( IsFreezePeriod() )
	{
		// Beta 5.0 (leak): the freeze time is over: the round's clock
		// starts, with one of four radio calls (Beta 3.0 to 4.1 played it at
		// RestartRound). Beta 6.0: a radio message from one player of each
		// team (heard by his team), the escape and assassination maps' own
		if ( TimeRemaining() <= 0 )
		{
			char CT_sentence[40];	// names ours
			char T_sentence[40];

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
			m_iRoundTimeSecs = m_iRoundTime * 60;

			bool bCTPlayed = false;	// names ours
			bool bTPlayed = false;

			for ( int i = 1; i <= gpGlobals->maxClients; i++ )
			{
				CBasePlayer *plr = (CBasePlayer *)UTIL_PlayerByIndex( i );

				if ( plr && plr->pev->flags != FL_DORMANT && plr->m_iJoiningState == JOIN_DONE )
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

					plr->SyncRoundTimer();
					plr->ResetMaxSpeed();
					plr->m_bCanShoot = true;	// Beta 6.5
				}
			}
		}
	}
	else if ( TimeRemaining() <= 0 )
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
			TerminateRound( 5, 1 );

			m_iNumCTWins++;
			EndRoundMessage( "#Target_Saved" );
			UpdateTeamScores();
		}
		else if ( UTIL_FindEntityByClassname( NULL, "hostage_entity" ) != NULL )
		{
			Broadcast( "terwin" );
			m_iAccountTerrorist += 2000;
			TerminateRound( 5, 2 );

			m_iNumTerroristWins++;
			EndRoundMessage( "#Hostages_Not_Rescued" );
			UpdateTeamScores();
		}
		else if ( m_bMapHasEscapeZone == true )
		{
			Broadcast( "ctwin" );
			TerminateRound( 5, 1 );

			m_iNumCTWins++;
			EndRoundMessage( "#Terrorists_Not_Escaped" );
			UpdateTeamScores();
		}
		else if ( m_iMapHasVIPSafetyZone == 1 )
		{
			Broadcast( "terwin" );
			m_iAccountTerrorist += 2000;
			TerminateRound( 5, 2 );

			m_iNumTerroristWins++;
			EndRoundMessage( "#VIP_Not_Escaped" );
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
	// SECOND(S)", on the screen and in every console
	if ( gpGlobals->time > m_flNextRestartCheck )
	{
		int iRestartDelay = (int)CVAR_GET_FLOAT( "sv_restartround" );	// name ours

		if ( iRestartDelay > 0 )
		{
			const char *szUnit;	// name ours
			char *szMessage;	// name ours

			if ( iRestartDelay > 10 )
				iRestartDelay = 10;

			if ( iRestartDelay == 1 )
				szUnit = "SECOND";
			else
				szUnit = "SECONDS";

			szMessage = UTIL_VarArgs( "The game will restart in %i %s\n", iRestartDelay, szUnit );
			EndRoundMessage( szMessage );
			UTIL_ClientPrintAll( HUD_PRINTCONSOLE, szMessage );
			m_flRestartRoundTime = gpGlobals->time + iRestartDelay;
			m_fCompleteReset = true;
			CVAR_SET_FLOAT( "sv_restartround", 0 );
		}

		m_flNextRestartCheck = gpGlobals->time + 1.0;

		// Beta 6.1: and the movement settings are put back whenever changed
		if ( CVAR_GET_FLOAT( "sv_accelerate" ) != 5.0 )
			CVAR_SET_FLOAT( "sv_accelerate", 5 );

		if ( CVAR_GET_FLOAT( "sv_friction" ) != 4.0 )
			CVAR_SET_FLOAT( "sv_friction", 4 );

		if ( CVAR_GET_FLOAT( "sv_stopspeed" ) != 75 )
			CVAR_SET_FLOAT( "sv_stopspeed", 75 );

		m_iMaxRounds = (int)CVAR_GET_FLOAT( "mp_maxrounds" );	// Beta 7.1
	}

	// sic: 154 empty statements (Counter-Strike 1.0; Beta 7.1: 116), a stand-in for intermediate code the image
	// cannot give. VC6 places the inline functions of gamerules.h among this
	// file's own functions by the intermediate code in front of each. Beta 7.1's
	// multiplay_gamerules.obj (the file writing its own .pch) has GetTeamID ahead
	// of ClientDisconnected and PlayTextureSounds behind it. With trains.h
	// included ahead of gamerules.h, as here, that takes 535 to 625 bytes more
	// intermediate code before ClientDisconnected than the text gives: 107 to 125
	// empty statements fit, 106 and 126 do not. (trains.h is 642 bytes of header
	// code; included behind gamerules.h the text would need no filler but 20 to
	// 105 bytes less than it has. The include's place and this number go
	// together; neither can be told from the image.) Beta 7.0's object asked for
	// 24 to 29 statements in another /YX state, Beta 6.6's for 3 to 15 at
	// InitHUD's end. What the text was is not known; the place is ours.
	// Counter-Strike 1.0 (Visual C++ 6.0 SP4, the same state): retail's object has
	// CGameRules::IsTeamplay and GetGameDescription ahead of GetNextBestWeapon and
	// the other nine inlines behind it; 140 to 168 statements fit, 138 and 170 do
	// not (698 to 947 bytes of intermediate code more before GetNextBestWeapon
	// than the text gives, by the offsets; with trains.h behind gamerules.h it
	// would be 56 to 305).
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
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

			if ( pPlayer->m_fHasDefuser == true )
				pPlayer->GiveNamedItem( "item_thighpack" );

			// Beta 6.0: a VIP leaving leaves no VIP
			if ( pPlayer->m_bIsVIP == true )
				m_pVIP = NULL;

			// Beta 6.5: his map vote goes with him
			if ( pPlayer->m_iMapVote >= 1 && pPlayer->m_iMapVote <= 100 )
				m_iMapVotes[ pPlayer->m_iMapVote - 1 ]--;

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
	if ( !pPlayer->m_bCanShoot && !m_bFreezePeriod )
		pPlayer->m_bCanShoot = true;

	// Beta 5.0 (leak): a joining player gets the team menu
	if ( pPlayer->m_iMenu != Menu_ChooseTeam && pPlayer->m_iJoiningState == JOIN_TEAM_MENU )
	{
		ShowVGUIMenu( pPlayer, 2, 0x13, "#Team_Select" );	// Beta 5.0: keys 1, 2 and 5 (the leak: 1 to 3); Beta 7.0: VGUI menu 2
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
	if ( pPlayer->m_fNotKilled != true && addDefault )
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
	pVictim->m_afPhysicsFlags &= ~PFLAG_ONTRAIN;	// Beta 7.1: he lets go of a train's or vehicle's controls
	pVictim->m_fNotKilled = false;
	pVictim->m_bEscaped = false;	// Beta 6.5
	pVictim->m_iTrain = 0xc0;	// Beta 7.1: TRAIN_NEW | TRAIN_OFF, which only player.cpp defines

	// Beta 6.5: his view is his own again
	SET_VIEW( pVictim->edict(), pVictim->edict() );

	// Beta 4.0: a dead commander, of either team, gets a successor
	if ( ( pVictim->m_iTeam == TEAM_CT && m_pCTCommander == pVictim ) || ( pVictim->m_iTeam == TEAM_TERRORIST && m_pTerroristCommander == pVictim ) )
		PickNewCommander( pVictim->m_iTeam );

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
			UTIL_LogPrintf( "%s killed a teammate\n", STRING( pKillerPlayer->pev->netname ) );	// Beta 6.6
			ClientPrint( pKillerPlayer->pev, HUD_PRINTCONSOLE, UTIL_VarArgs( "Teammate kills: %i of 3\n", pKillerPlayer->m_iTeamKills ) );

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
				pKillerPlayer->HintMessage( "Careful!\nKilling teammates will not be tolerated!", FALSE, FALSE );
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
				pKillerPlayer->HintMessage( "You have been rewarded $2500 for killing the VIP!", TRUE, FALSE );
				pKillerPlayer->AddAccount( 2500, true );
				UTIL_LogPrintf( "%s assasinated the VIP\n", STRING( pKillerPlayer->pev->netname ) );	// Beta 6.6
			}
			else
				pKillerPlayer->AddAccount( 300, true );

			if ( !( pKillerPlayer->m_flDisplayHistory & ( 1 << 11 ) ) )	// the enemy kill hint
			{
				pKillerPlayer->m_flDisplayHistory |= ( 1 << 11 );
				pKillerPlayer->HintMessage( "You killed an Enemy!\nWin the round by eliminating\nthe opposing force.", FALSE, FALSE );
			}
		}
		
		FireTargets( "game_playerkill", ktmp, ktmp, USE_TOGGLE, 0 );

		// Beta 6.5: as in ClientDisconnected. sic: only when another player
		// killed him
		CBasePlayer *client = NULL;
		while ( ((client = (CBasePlayer*)UTIL_FindEntityByClassname( client, "player" )) != NULL) && (!FNullEnt(client->edict())) ) 
		{
			if ( !client->pev )
				continue;
			if ( client == pVictim )
				continue;

			// an observer of this player: his camera mode is cleared and set again (Observer_SetMode)
			if ( client->m_hObserverTarget == pVictim )
			{
				int iMode = client->pev->iuser1;
				client->pev->iuser1 = 0;
				client->Observer_SetMode( iMode );
			}
		}
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

	// Beta 6.5: the log names the victim's team
	if ( pVictim->pev == pKiller )  
	{  // killed self
		if ( pVictim->m_iTeam == TEAM_CT )
			UTIL_LogPrintf( "\"[CT] %s<%i>\" killed self with %s\n",  STRING( pVictim->pev->netname ), GETPLAYERUSERID( pVictim->edict() ), killer_weapon_name );
		else
			UTIL_LogPrintf( "\"[TERRORIST] %s<%i>\" killed self with %s\n",  STRING( pVictim->pev->netname ), GETPLAYERUSERID( pVictim->edict() ), killer_weapon_name );
	}
	else if ( pKiller->flags & FL_CLIENT )
	{
		char team[20];	// (Counter-Strike 1.6 has a char *team here; the size is the frame's)

		if ( pVictim->m_iTeam == TEAM_CT )
			strcpy( team, "[CT]" );
		else
			strcpy( team, "[TERRORIST]" );

		UTIL_LogPrintf( "\"%s<%i>\" killed \"%s %s<%i>\" with %s\n",  STRING( pKiller->netname ),
			GETPLAYERUSERID( ENT(pKiller) ),
			team,
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
	if ( g_footsteps && g_footsteps->value == 0 )
		return FALSE;

	if ( pl->IsOnLadder() || pl->pev->velocity.Length2D() > 135 )	// Counter-Strike 1.0: 135 (Beta 6.0 to 7.1: 190; before: 220)
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
	char szNewMap[40];	// (1.6: char *pszNewMap); the sizes are the frame's
	char text[60];		// name ours

	DestroyMapCycle( &mapcycle2 );
	ReloadMapCycleFile( mapcfile, &mapcycle2 );

	item = mapcycle2.next_item;

	while ( item->next != mapcycle2.next_item )
	{
		if ( item )
		{
			iCount++;

			if ( m_iMapVotes[iCount - 1] == 1 )
				sprintf( text, "%d :  %s (%i vote)\n", iCount, item->mapname, m_iMapVotes[iCount - 1] );
			else
				sprintf( text, "%d :  %s (%i votes)\n", iCount, item->mapname, m_iMapVotes[iCount - 1] );

			if ( player )
				ClientPrint( player->pev, HUD_PRINTCONSOLE, text );

			if ( iCount == iVote )
				strcpy( szNewMap, item->mapname );
		}

		item = item->next;
	}

	if ( szNewMap && iVote )
	{
		// Beta 6.6: a vote for the map being played adds thirty minutes to it
		if ( !strcmp( szNewMap, STRING( gpGlobals->mapname ) ) )
		{
			timelimit.value += 30;
			return;
		}

		// (Beta 7.0 only: ChangeLevel( szNewMap ) first, to leave the map
		// cycle standing after the voted map)
		CHANGE_LEVEL( szNewMap, NULL );
	}
}

//=========================================================
// VoteMap - Beta 6.5 (Linux symbol; 1.6 calls it
// ProcessMapVote, and the locals' names are its): a map's
// votes are counted afresh; with enough of them
// (mp_mapvoteratio of the players, 0.35 to 1, and one more)
// the map changes, else the player is shown the list
//=========================================================
void CHalfLifeMultiplay::VoteMap( CBasePlayer *player, int iVote )
{
	CBaseEntity *pTempEntity;
	CBasePlayer *pTempPlayer;
	int iValidVotes;
	int iRequiredVotes;
	int iNumPlayers;
	char text[76];	// name ours; the size is the frame's

	// Beta 6.6: m_iMapVotes has a hundred counts
	if ( iVote > 99 )
		return;

	if ( gpGlobals->time < 180 )	// Beta 7.0: three minutes (to 6.6: five)
	{
		ClientPrint( player->pev, HUD_PRINTTALK, "#Cannot_Vote_Map" );	// Beta 6.6: to the chat (was the centre)
		return;
	}

	iValidVotes = 0;
	iNumPlayers = 0;

	pTempEntity = UTIL_FindEntityByClassname( NULL, "player" );
	while ( pTempEntity != NULL && !FNullEnt( pTempEntity->edict() ) )
	{
		if ( pTempEntity->IsPlayer() && !( pTempEntity->pev->flags & FL_DORMANT ) )
		{
			iNumPlayers++;
			pTempPlayer = GetClassPtr( (CBasePlayer *)pTempEntity->pev );

			if ( pTempPlayer->m_iMapVote == iVote )
			{
				iValidVotes++;

				// the hidden a12 command's flag: ten votes more
				if ( pTempPlayer->m_bVoteKicksAtOnce == true )
					iValidVotes += 10;
			}
		}
		pTempEntity = UTIL_FindEntityByClassname( pTempEntity, "player" );
	}

	m_iMapVotes[iVote - 1] = iValidVotes;

	float ratio = CVAR_GET_FLOAT( "mp_mapvoteratio" );

	if ( ratio > 1.0 )
		ratio = 1;
	else if ( ratio < 0.35 )
		ratio = 0.35f;

	iRequiredVotes = (int)( iNumPlayers * ratio ) + 1;

	if ( iValidVotes >= iRequiredVotes && iNumPlayers != 1 )
	{
		DisplayMaps( NULL, iVote );
	}
	else
	{
		DisplayMaps( player, 0 );
		sprintf( text, "\nRequired Number of votes for a new map = %i\n", iRequiredVotes );
		ClientPrint( player->pev, HUD_PRINTCONSOLE, text );
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

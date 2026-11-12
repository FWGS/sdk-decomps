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

extern DLL_GLOBAL CGameRules	*g_pGameRules;
extern DLL_GLOBAL BOOL	g_fGameOver;
extern void respawn( entvars_t* pev, BOOL fCopyCorpse );
extern int gmsgDeathMsg;	// client dll messages
extern int gmsgScoreInfo;
extern int gmsgMOTD;
extern int gmsgTeamScore;
extern int gmsgSendAudio;
extern int gmsgGiveItem;
extern int gmsgRemoveItem;
extern int CountTeamPlayers( int iTeam );

// sic: intermediate code retail has between the headers and the round code,
// which the image shows only through VC5's placement of header inline
// functions. With this unused
// function's statements the class-body inlines of CGameRules and
// CBasePlayerItem land where retail has them, in Beta 4.0's object (which
// reuses the .pch it wrote itself) and in Beta 4.1's (which writes its own)
// alike: 42 lines, of 36 to 46 tried, serve both. VC5 also spends two bytes
// more on a statement from line 128 on, so this comment's length is part of
// the stand-in: comments of 11 to 15 lines here serve both versions (Beta 4.0
// also 10; more not tried), of 5 to 15 tried; 43 statement lines serve Beta
// 4.0 only. What the text was the image cannot say.
//
//
inline void sic_il_multiplay( int *p )
{
	p[0] = 0;
	p[1] = 0;
	p[2] = 0;
	p[3] = 0;
	p[4] = 0;
	p[5] = 0;
	p[6] = 0;
	p[7] = 0;
	p[8] = 0;
	p[9] = 0;
	p[10] = 0;
	p[11] = 0;
	p[12] = 0;
	p[13] = 0;
	p[14] = 0;
	p[15] = 0;
	p[16] = 0;
	p[17] = 0;
	p[18] = 0;
	p[19] = 0;
	p[20] = 0;
	p[21] = 0;
	p[22] = 0;
	p[23] = 0;
	p[24] = 0;
	p[25] = 0;
	p[26] = 0;
	p[27] = 0;
	p[28] = 0;
	p[29] = 0;
	p[30] = 0;
	p[31] = 0;
	p[32] = 0;
	p[33] = 0;
	p[34] = 0;
	p[35] = 0;
	p[36] = 0;
	p[37] = 0;
	p[38] = 0;
	p[39] = 0;
	p[40] = 0;
	p[41] = 0;
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

//*********************************************************
// Rules for the half-life multiplayer game.
//*********************************************************

CHalfLifeMultiplay :: CHalfLifeMultiplay()
{
	RefreshSkillData();
	m_flIntermissionEndTime = 0;
	m_flRestartRoundTime = 0;
	m_flRoundEndTime = 0;
	m_flRoundStartTime = gpGlobals->time;
	m_flRoundStartTime2 = gpGlobals->time;
	m_iAccountCT = 0;
	m_iAccountTerrorist = 0;
	m_iHostagesRescued = 0;
	m_iRoundWinStatus = 0;
	m_iNumCTWins = 0;
	m_iNumTerroristWins = 0;
	m_pCTCommander = NULL;
	m_pTerroristCommander = NULL;

	// Beta 4.0: no high-detail player models; the round and C4 times as RestartRound reads them
	CVAR_SET_FLOAT( "cl_himodels", 0 );

	m_iRoundTime = (int)CVAR_GET_FLOAT( "mp_roundtime" );
	m_iC4Timer = (int)CVAR_GET_FLOAT( "mp_c4timer" );

	if ( m_iRoundTime > 9 )
		m_iRoundTime = 9;
	else if ( m_iRoundTime < 3 )
		m_iRoundTime = 3;

	if ( m_iC4Timer > 90 )
		m_iC4Timer = 90;
	else if ( m_iC4Timer < 10 )
		m_iC4Timer = 10;
	
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
// (Restart), a C4 and the backpack go, and up to four
// defusers; then the weapon boxes (Beta 3.1's CleanUpMap took
// the crowbars out)
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

	// only the first of each (sic)
	CBaseEntity *toremove = UTIL_FindEntityByClassname( NULL, "grenade" );
	if ( toremove )
		UTIL_Remove( toremove );

	toremove = UTIL_FindEntityByClassname( NULL, "item_backpack" );
	if ( toremove )
		UTIL_Remove( toremove );

	int iLeft = 4;
	toremove = UTIL_FindEntityByClassname( NULL, "item_thighpack" );

	while ( toremove && iLeft > 0 )
	{
		iLeft--;
		UTIL_Remove( toremove );
		toremove = UTIL_FindEntityByClassname( toremove, "item_thighpack" );
	}

	RemoveGuns();
}

//=========================================================
// GiveDefusers - Beta 4.0: every second counter-terrorist
// gets a bomb defuser, starting with the first or the second
// by GiveC4's turn; if that passes the last one, the last one
// gets it and the commander too
//=========================================================
void CHalfLifeMultiplay::GiveDefusers( void )
{
	bool bLast = false;
	int i = 1;
	int iStart = m_iC4Turn % 2;
	int iNumCT = CountTeamPlayers( TEAM_CT );
	int iDefuser = iStart + 1;

	do
	{
		if ( iDefuser > iNumCT && i > iNumCT )
			return;

		if ( iDefuser > iNumCT && i <= iNumCT )
		{
			iDefuser = iNumCT;
			bLast = true;
		}
		else
			i += 2;

		CBaseEntity *pPlayer = UTIL_FindEntityByClassname( NULL, "player" );
		int iCount = 0;

		while ( pPlayer != NULL && !FNullEnt( pPlayer->edict() ) )
		{
			if ( pPlayer->IsPlayer() && pPlayer->pev->flags != FL_DORMANT )
			{
				CBasePlayer *player = GetClassPtr( (CBasePlayer *)pPlayer->pev );
				entvars_t *pevPlayer = player->pev;

				if ( player->m_iTeam == TEAM_CT && ++iCount == iDefuser )
				{
					player->m_fHasDefuser = true;
					MESSAGE_BEGIN( MSG_ONE, gmsgGiveItem, NULL, pevPlayer );
						WRITE_BYTE( 1 );
					MESSAGE_END();
					player->pev->body = 1;
				}
			}
			pPlayer = UTIL_FindEntityByClassname( pPlayer, "player" );
		}

		iDefuser = iStart + i;
	}
	while ( !bLast );

	if ( m_pCTCommander )
	{
		m_pCTCommander->m_fHasDefuser = true;
		MESSAGE_BEGIN( MSG_ONE, gmsgGiveItem, NULL, m_pCTCommander->pev );
			WRITE_BYTE( 1 );
		MESSAGE_END();
		m_pCTCommander->pev->body = 1;
	}
}

//=========================================================
// GiveC4 - Beta 4.0: the terrorists take turns with the C4
// (one more each round); its carrier becomes the commander
//=========================================================
void CHalfLifeMultiplay::GiveC4( void )
{
	int iNumTerrorist = CountTeamPlayers( TEAM_TERRORIST );

	if ( ++m_iC4Turn > iNumTerrorist )
		m_iC4Turn = 1;

	CBaseEntity *pPlayer = UTIL_FindEntityByClassname( NULL, "player" );
	int iCount = 0;

	while ( pPlayer != NULL && !FNullEnt( pPlayer->edict() ) )
	{
		if ( pPlayer->IsPlayer() && pPlayer->pev->flags != FL_DORMANT )
		{
			CBasePlayer *player = GetClassPtr( (CBasePlayer *)pPlayer->pev );
			entvars_t *pevPlayer = player->pev;

			if ( player->m_iTeam == TEAM_TERRORIST && ++iCount == m_iC4Turn )
			{
				player->m_fHasC4 = true;
				MESSAGE_BEGIN( MSG_ONE, gmsgGiveItem, NULL, pevPlayer );
					WRITE_BYTE( 0 );
				MESSAGE_END();
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
// later versions' Linux symbols). Beta 3.0 and 3.1 tried each
// in turn with a chance of one in three and told the one kept;
// nobody is told now.
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

			if ( pPlayer )
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
// CheckWinConditions - Beta 4.0: once a round is decided
// nothing changes it. A bombed target wins it for the
// terrorists (3,500 each); a team with nobody left alive
// loses it (the CTs win only when no C4 is ticking: 3,500
// each on a bomb map, else 2,750; the terrorists 2,750);
// nobody alive on either side is a draw; and with every
// hostage rescued or dead and at least half of them rescued,
// the CTs win (2,500 each). The next round starts five
// seconds later. Beta 3.1's console lines are gone.
//=========================================================
void CHalfLifeMultiplay::CheckWinConditions( void )
{
	if ( m_iRoundWinStatus != 0 )
		return;

	CBaseEntity *pPlayer = NULL;
	int NumDeadTerrorist = 0;
	int NumDeadCT = 0;
	int NumAliveCT = 0;
	int NumAliveTerrorist = 0;

	m_iNumCT = 0;
	m_iNumTerrorist = 0;

	pPlayer = UTIL_FindEntityByClassname( pPlayer, "player" );
	while ( pPlayer != NULL && !FNullEnt( pPlayer->edict() ) )
	{
		if ( pPlayer->IsPlayer() && pPlayer->pev->flags != FL_DORMANT )
		{
			CBasePlayer *player = GetClassPtr( (CBasePlayer *)pPlayer->pev );

			if ( pPlayer && ( pPlayer->pev->deadflag == DEAD_DYING || pPlayer->pev->deadflag == DEAD_RESPAWNABLE || pPlayer->pev->deadflag == DEAD_DEAD ) )
			{
				if ( player->m_iTeam == TEAM_CT )
				{
					m_iNumCT++;
					NumDeadCT++;
				}
				else if ( player->m_iTeam == TEAM_TERRORIST )
				{
					m_iNumTerrorist++;
					NumDeadTerrorist++;
				}
			}
			else if ( pPlayer )
			{
				if ( player->m_iTeam == TEAM_CT )
				{
					m_iNumCT++;
					NumAliveCT++;
				}
				else if ( player->m_iTeam == TEAM_TERRORIST )
				{
					m_iNumTerrorist++;
					NumAliveTerrorist++;
				}
			}
		}
		pPlayer = UTIL_FindEntityByClassname( pPlayer, "player" );
	}

	if ( m_fTargetBombed == true && m_fMapHasBombTarget == true )
	{
		m_iRoundWinStatus = 2;
		m_iAccountTerrorist = 3500;
		UTIL_ClientPrintAll( HUD_PRINTCENTER, "Target successfully bombed!" );
		Broadcast( "terwin" );

		m_iNumTerroristWins++;
		MESSAGE_BEGIN( MSG_ALL, gmsgTeamScore );
			WRITE_SHORT( 1 );
			WRITE_SHORT( m_iNumTerroristWins );
		MESSAGE_END();

		m_flRestartRoundTime = gpGlobals->time + 5.0;
		return;
	}

	if ( NumAliveTerrorist == 0 && NumDeadTerrorist != 0 && m_iNumCT > 0 )
	{
		// not while a C4 is ticking
		CBaseEntity *pC4 = UTIL_FindEntityByClassname( NULL, "grenade" );

		if ( pC4 == NULL || ((CGrenade *)pC4)->m_fC4Done )
		{
			m_iRoundWinStatus = 1;

			if ( m_fTargetBombed == false && m_fMapHasBombTarget == true )
				m_iAccountCT = 3500;
			else
				m_iAccountCT = 2750;

			UTIL_ClientPrintAll( HUD_PRINTCENTER, "Terrorist Team EXTERMINATED" );
			Broadcast( "ctwin" );

			m_iNumCTWins++;
			MESSAGE_BEGIN( MSG_ALL, gmsgTeamScore );
				WRITE_SHORT( 0 );	// the client's team 0: the counter-terrorists
				WRITE_SHORT( m_iNumCTWins );
			MESSAGE_END();

			m_flRestartRoundTime = gpGlobals->time + 5.0;
			return;
		}
	}
	else if ( NumAliveCT == 0 && NumDeadCT != 0 && m_iNumTerrorist > 0 )
	{
		m_iRoundWinStatus = 2;
		m_iAccountTerrorist = 2750;
		UTIL_ClientPrintAll( HUD_PRINTCENTER, "Counter-Terrorists EXTERMINATED" );
		Broadcast( "terwin" );

		m_iNumTerroristWins++;
		MESSAGE_BEGIN( MSG_ALL, gmsgTeamScore );
			WRITE_SHORT( 1 );
			WRITE_SHORT( m_iNumTerroristWins );
		MESSAGE_END();

		m_flRestartRoundTime = gpGlobals->time + 5.0;
		return;
	}
	else if ( NumAliveCT == 0 && NumAliveTerrorist == 0 )
	{
		UTIL_ClientPrintAll( HUD_PRINTCENTER, "** Round DRAW **" );
		m_iRoundWinStatus = 3;
		m_flRestartRoundTime = gpGlobals->time + 5.0;
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
			m_iRoundWinStatus = 1;
			m_iAccountCT = 2500;
			UTIL_ClientPrintAll( HUD_PRINTCENTER, "All hostages rescued!" );
			Broadcast( "ctwin" );

			m_iNumCTWins++;
			MESSAGE_BEGIN( MSG_ALL, gmsgTeamScore );
				WRITE_SHORT( 0 );
				WRITE_SHORT( m_iNumCTWins );
			MESSAGE_END();

			m_flRestartRoundTime = gpGlobals->time + 5.0;
		}
	}
}

//=========================================================
// RestartRound - the hostages go back to where they stood,
// both teams are paid and every player is respawned.
//=========================================================
void CHalfLifeMultiplay::RestartRound( void )
{
	CBaseEntity *pHostage = NULL;
	char szSound[32];	// name ours

	// Beta 4.0: mp_roundtime (3 to 9 minutes) and mp_c4timer (10 to 90 seconds)
	m_iRoundTime = (int)CVAR_GET_FLOAT( "mp_roundtime" );
	m_iC4Timer = (int)CVAR_GET_FLOAT( "mp_c4timer" );

	if ( m_iRoundTime > 9 )
		m_iRoundTime = 9;
	else if ( m_iRoundTime < 3 )
		m_iRoundTime = 3;

	if ( m_iC4Timer > 90 )
		m_iC4Timer = 90;
	else if ( m_iC4Timer < 10 )
		m_iC4Timer = 10;

	CBaseEntity *pTarget = UTIL_FindEntityByClassname( NULL, "info_bomb_target" );

	if ( pTarget == NULL )
		m_fMapHasBombTarget = false;
	else
		m_fMapHasBombTarget = true;

	m_fTargetBombed = false;

	// the kick votes start afresh
	for ( int i = 0; i < 20; i++ )
		m_iKickVotes[i] = 0;

	pHostage = UTIL_FindEntityByClassname( pHostage, "hostage_entity" );
	while ( pHostage != NULL )
	{
		if ( pHostage->pev->solid != SOLID_NOT )
		{
			// a hostage still standing in the map (Beta 4.0: 200, was 400)
			if ( m_iRoundWinStatus == 2 )
				m_iAccountTerrorist += 200;
			else if ( m_iRoundWinStatus == 1 )
				m_iAccountCT += 200;

			if ( pHostage->pev->deadflag == DEAD_DEAD )
				pHostage->pev->deadflag = DEAD_RESPAWNABLE;
		}
		((CHostage *)pHostage)->RePosition();
		pHostage = UTIL_FindEntityByClassname( pHostage, "hostage_entity" );
	}

	// the losing team gets something too (Beta 4.1: 1,550; Beta 4.0: 1,750,
	// was 800)
	if ( m_iRoundWinStatus == 2 )
		m_iAccountCT += 1550;
	else if ( m_iRoundWinStatus == 1 )
		m_iAccountTerrorist += 1550;

	m_iAccountCT += m_iHostagesRescued * 500;	// Beta 4.0: 500 (was 350)

	// Beta 3.0: the round starts with a radio call (was radio/go.wav); Beta 4.0 adds com_go
	switch ( RANDOM_LONG( 0, 3 ) )
	{
	case 0:
		strcpy( szSound, "radio/moveout.wav" );
		break;
	case 1:
		strcpy( szSound, "radio/letsgo.wav" );
		break;
	case 2:
		strcpy( szSound, "radio/locknload.wav" );
		break;
	case 3:
		strcpy( szSound, "radio/com_go.wav" );
		break;
	}

	// Beta 3.0: both teams get a new commander
	PickNewCommander( TEAM_TERRORIST );
	PickNewCommander( TEAM_CT );

	m_iUnused88 = 2;

	// Beta 4.1: a round pays a team 4,500 dollars at most
	if ( m_iAccountCT > 4500 )
		m_iAccountCT = 4500;
	if ( m_iAccountTerrorist > 4500 )
		m_iAccountTerrorist = 4500;

	CBaseEntity *pPlayer = NULL;

	pPlayer = UTIL_FindEntityByClassname( pPlayer, "player" );
	while ( pPlayer != NULL && !FNullEnt( pPlayer->edict() ) )
	{
		if ( pPlayer->IsPlayer() && pPlayer->pev->flags != FL_DORMANT )
		{
			CBasePlayer *player = GetClassPtr( (CBasePlayer *)pPlayer->pev );

			// Beta 4.0: everybody is paid, empty team or not
			if ( player->m_iTeam == TEAM_CT )
				player->AddAccount( m_iAccountCT );
			else if ( player->m_iTeam == TEAM_TERRORIST )
				player->AddAccount( m_iAccountTerrorist );

			if ( player->pev->deadflag == DEAD_DYING )
			{
				respawn( pPlayer->pev, FALSE );
				pPlayer->pev->button = 0;
				player->m_iRespawnFrames = 0;
				pPlayer->pev->nextthink = -1;
			}
			else
			{
				player->m_iRespawnFrames = 0;
				respawn( pPlayer->pev, FALSE );
			}

			// Beta 4.0: last round's defusers and C4 go (Beta 4.1: from
			// everybody; Beta 4.0: the defusers from the counter-terrorists, the
			// C4 from the terrorists)
			player->m_fHasDefuser = false;
			MESSAGE_BEGIN( MSG_ONE, gmsgRemoveItem, NULL, player->pev );
				WRITE_BYTE( 1 );
			MESSAGE_END();
			player->pev->body = 0;

			player->m_fHasC4 = false;
			MESSAGE_BEGIN( MSG_ONE, gmsgRemoveItem, NULL, player->pev );
				WRITE_BYTE( 0 );
			MESSAGE_END();
			player->pev->body = 0;

			EMIT_SOUND( ENT(pPlayer->pev), CHAN_VOICE, szSound, 1, 0.8 );
		}
		pPlayer = UTIL_FindEntityByClassname( pPlayer, "player" );
	}

	// Beta 4.0: the map is put back; on a bomb map the C4 and the defusers
	// are handed out (sic: the flag is read through g_pGameRules)
	CleanUpMap();

	if ( ((CHalfLifeMultiplay *)g_pGameRules)->m_fMapHasBombTarget == true )
	{
		GiveDefusers();
		GiveC4();
	}

	m_flIntermissionEndTime = 0;
	m_flRestartRoundTime = 0;
	m_flRoundEndTime = 0;
	m_iAccountCT = 0;
	m_iAccountTerrorist = 0;
	m_iHostagesRescued = 0;
	m_iUnused2C = 0;
	m_iRoundWinStatus = 0;
	m_flRoundStartTime = gpGlobals->time;
	m_flRoundStartTime2 = gpGlobals->time;
}

// longest the intermission can last, in seconds
#define MAX_INTERMISSION_TIME		120

//=========================================================
//=========================================================
//=========================================================
// GiveWeapons - Beta 4.0: everybody gets a knife and a
// pistol with ammunition: the commanders a P228 (CTs, 52
// rounds) or a Desert Eagle (terrorists, 35), the others a
// USP (48) or a Glock 18 (120)
//=========================================================
void CHalfLifeMultiplay::GiveWeapons( void )
{
	CBaseEntity *pPlayer = UTIL_FindEntityByClassname( NULL, "player" );

	while ( pPlayer != NULL && !FNullEnt( pPlayer->edict() ) )
	{
		if ( pPlayer->IsPlayer() && pPlayer->pev->flags != FL_DORMANT )
		{
			CBasePlayer *player = GetClassPtr( (CBasePlayer *)pPlayer->pev );

			if ( player->m_iTeam == TEAM_CT )
			{
				if ( player == m_pCTCommander )
				{
					player->GiveNamedItem( "weapon_knife" );
					player->GiveNamedItem( "weapon_p228" );
					player->GiveAmmo( 26, "357sig", 52 );
				}
				else
				{
					player->GiveNamedItem( "weapon_knife" );
					player->GiveNamedItem( "weapon_usp" );
					player->GiveAmmo( 24, "45acp", 48 );
				}
			}
			else if ( player->m_iTeam == TEAM_TERRORIST )
			{
				if ( player == m_pTerroristCommander )
				{
					player->GiveNamedItem( "weapon_knife" );
					player->GiveNamedItem( "weapon_deagle" );
					player->GiveAmmo( 14, "50ae", 35 );
				}
				else
				{
					player->GiveNamedItem( "weapon_knife" );
					player->GiveNamedItem( "weapon_glock18" );
					player->GiveAmmo( 40, "9mm", 120 );
				}
			}
		}
		pPlayer = UTIL_FindEntityByClassname( pPlayer, "player" );
	}
}

void CHalfLifeMultiplay :: Think ( void )
{
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

	// a round lasts mp_roundtime minutes less 5 seconds (Beta 2.0 and 2.1: 385 seconds,
	// 3.0 and 3.1: 294); Beta 4.0: a ticking C4 ends it only when it goes off
	if ( gpGlobals->time > m_flRoundStartTime + ( m_iRoundTime * 60 - 5 ) )
	{
		CBaseEntity *pC4 = UTIL_FindEntityByClassname( NULL, "grenade" );

		m_flRoundEndTime = m_flRestartRoundTime = pC4 ?
			( !((CGrenade *)pC4)->m_fC4Done ? ((CGrenade *)pC4)->m_flC4Blow : gpGlobals->time + 5.0 ) :
			gpGlobals->time + 5.0;

		m_iRoundWinStatus = 3;
		m_flRoundStartTime = gpGlobals->time + 60;
	}

	if ( m_flRestartRoundTime != 0.0 && m_flRestartRoundTime <= gpGlobals->time )
		RestartRound();
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

// sic: declarations Beta 4.1 has before GetNextBestWeapon (locals, externs,
// prototypes: the image cannot say which). VC5 orders the two iWeight() calls below
// by the number of declarations seen; with gamerules.h's 11, Beta 4.1
// needs 8 to 10 more here and Beta 4.0 none. This enum's tag
// and 7 enumerators are 8.
enum sic_declcount_multiplay
{
	SIC_DECLCOUNT_MULTIPLAY_BASE, SIC_DECLCOUNT_MULTIPLAY_0, SIC_DECLCOUNT_MULTIPLAY_1, SIC_DECLCOUNT_MULTIPLAY_2, SIC_DECLCOUNT_MULTIPLAY_3, SIC_DECLCOUNT_MULTIPLAY_4, SIC_DECLCOUNT_MULTIPLAY_5
};

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
	// notify other clients of player joining the game
	UTIL_ClientPrintAll( HUD_PRINTNOTIFY, UTIL_VarArgs( "%s has joined the game\n", 
		( pl->pev->netname && STRING(pl->pev->netname)[0] != 0 ) ? STRING(pl->pev->netname) : "unconnected" ) );

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

	if ( g_fGameOver )
	{
		MESSAGE_BEGIN( MSG_ONE, SVC_INTERMISSION, NULL, pl->edict() );
		MESSAGE_END();
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
			// Beta 4.0: the C4 (item_backpack) and the defuser (item_thighpack)
			// stay where he was
			if ( pPlayer->m_fHasC4 == true )
				pPlayer->GiveNamedItem( "item_backpack" );

			if ( pPlayer->m_fHasDefuser == true )
				pPlayer->GiveNamedItem( "item_thighpack" );

			// his kick vote no longer counts
			if ( pPlayer->m_iKickVote )
			{
				m_iKickVotes[ pPlayer->m_iKickVote - 1 ]--;
				if ( m_iKickVotes[ pPlayer->m_iKickVote - 1 ] < 0 )
					m_iKickVotes[ pPlayer->m_iKickVote - 1 ] = 0;
			}

			FireTargets( "game_playerleave", pPlayer, pPlayer, USE_TOGGLE, 0 );
			UTIL_LogPrintf( "\"%s<%i>\" disconnected\n",  STRING( pPlayer->pev->netname ), GETPLAYERUSERID( pPlayer->edict() ) );

			pPlayer->RemoveAllItems( TRUE );// destroy all of the players weapons and items

			// Beta 3.0: a counter-terrorist leaving makes the CTs pick a new
			// commander (sic: whoever he was, and not so for a terrorist)
			if ( pPlayer->m_iTeam == TEAM_CT )
				PickNewCommander( TEAM_CT );
		}
	}

	CheckWinConditions();
}

//=========================================================
//=========================================================
float CHalfLifeMultiplay :: FlPlayerFallDamage( CBasePlayer *pPlayer )
{
	// always progressive now, at 1.75 times the single-player rate (Beta 1.1 to 2.1:
	// twice; 3.0 and 3.1: 1.25, with a console line); mp_falldamage is no longer read
	pPlayer->m_flFallVelocity -= PLAYER_MAX_SAFE_FALL_SPEED;
	return pPlayer->m_flFallVelocity * DAMAGE_FOR_FALL_SPEED * 1.75;
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
}

//=========================================================
//=========================================================
void CHalfLifeMultiplay :: PlayerSpawn( CBasePlayer *pPlayer )
{
	BOOL		addDefault;
	CBaseEntity	*pWeaponEntity = NULL;

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

	// Beta 4.0: the commanders get a P228 (52 rounds) or a Desert Eagle (28);
	// twice the ammunition for the others' USP and Glock 18
	if ( pPlayer->m_iTeam == TEAM_CT && addDefault )
	{
		if ( m_pCTCommander == pPlayer )
		{
			pPlayer->GiveNamedItem( "weapon_knife" );
			pPlayer->GiveNamedItem( "weapon_p228" );
			pPlayer->GiveAmmo( 26, "357sig", 52 );
		}
		else
		{
			pPlayer->GiveNamedItem( "weapon_knife" );
			pPlayer->GiveNamedItem( "weapon_usp" );
			pPlayer->GiveAmmo( 24, "45acp", USP_MAX_CARRY );
		}
	}
	else if ( pPlayer->m_iTeam == TEAM_TERRORIST && addDefault )
	{
		if ( m_pTerroristCommander == pPlayer )
		{
			pPlayer->GiveNamedItem( "weapon_knife" );
			pPlayer->GiveNamedItem( "weapon_deagle" );
			pPlayer->GiveAmmo( 14, "50ae", 28 );
		}
		else
		{
			pPlayer->GiveNamedItem( "weapon_knife" );
			pPlayer->GiveNamedItem( "weapon_glock18" );
			pPlayer->GiveAmmo( 40, "9mm", GLOCK18_MAX_CARRY );
		}
	}

	int clientIndex = pPlayer->entindex();

	// the model the player chose (ChooseCT, ChooseTerrorist)
	switch ( pPlayer->m_iAppearance )
	{
	case 0:
		CLIENT_COMMAND( pPlayer->edict(), "model urban\n" );
		g_engfuncs.pfnSetClientKeyValue( clientIndex, g_engfuncs.pfnGetInfoKeyBuffer( pPlayer->edict() ), "model", "urban" );
		break;
	case 1:
		CLIENT_COMMAND( pPlayer->edict(), "model terror\n" );
		g_engfuncs.pfnSetClientKeyValue( clientIndex, g_engfuncs.pfnGetInfoKeyBuffer( pPlayer->edict() ), "model", "terror" );
		break;
	case 2:
		CLIENT_COMMAND( pPlayer->edict(), "model arab\n" );
		g_engfuncs.pfnSetClientKeyValue( clientIndex, g_engfuncs.pfnGetInfoKeyBuffer( pPlayer->edict() ), "model", "arab" );
		break;
	case 3:
		CLIENT_COMMAND( pPlayer->edict(), "model gsg9\n" );
		g_engfuncs.pfnSetClientKeyValue( clientIndex, g_engfuncs.pfnGetInfoKeyBuffer( pPlayer->edict() ), "model", "gsg9" );
		break;
	default:
		CLIENT_COMMAND( pPlayer->edict(), "model urban\n" );
		g_engfuncs.pfnSetClientKeyValue( clientIndex, g_engfuncs.pfnGetInfoKeyBuffer( pPlayer->edict() ), "model", "urban" );
		break;
	}
}

//=========================================================
//=========================================================
BOOL CHalfLifeMultiplay :: FPlayerCanRespawn( CBasePlayer *pPlayer )
{
	return TRUE;
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

	// Beta 4.0: a dead commander, of either team, gets a successor (Beta 3.0
	// and 3.1: any dead counter-terrorist made the CTs pick one); Beta 3.0's
	// automatic radio messages, MATEDOWN from the victim and ENEMYDOWN from
	// his killer, are gone in Beta 3.1 ("removed automatic .wav's")
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

		// 300 for the kill; a team kill costs 3600 (Beta 3.1: 1800), and the third one
		// sends the killer away (Beta 4.0: one killed by an HE grenade is not counted)
		pKillerPlayer->AddAccount( 300 );
		if ( pKillerPlayer->m_iTeam == pVictim->m_iTeam )
		{
			pKillerPlayer->AddAccount( -3600 );
			if ( !pVictim->m_fKilledByHEGrenade )
				pKillerPlayer->m_iTeamKills++;

			if ( pKillerPlayer->m_iTeamKills >= 3 )
			{
				pKillerPlayer->m_fKicked = true;
				CLIENT_COMMAND( pKillerPlayer->edict(), "disconnect\n" );
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
	if ( CVAR_GET_FLOAT("mp_weaponstay") > 0 )
	{
		// make sure it's only certain weapons
		if ( !(pWeapon->iFlags() & ITEM_FLAG_LIMITINWORLD) )
		{
			return gpGlobals->time + 0;		// weapon respawns almost instantly
		}
	}

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
	if ( CVAR_GET_FLOAT("mp_weaponstay") > 0 )
	{
		if ( pItem->iFlags() & ITEM_FLAG_LIMITINWORLD )
			return CGameRules::CanHavePlayerItem( pPlayer, pItem );

		// check if the player already has this weapon
		for ( int i = 0 ; i < MAX_ITEM_TYPES ; i++ )
		{
			CBasePlayerItem *it = pPlayer->m_rgpPlayerItems[i];

			while ( it != NULL )
			{
				if ( it->m_iId == pItem->m_iId )
				{
					return FALSE;
				}

				it = it->m_pNext;
			}
		}
	}

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
int CHalfLifeMultiplay::PlayerRelationship( CBaseEntity *pPlayer, CBaseEntity *pTarget )
{
	if ( !pPlayer || !pTarget || !pTarget->IsPlayer() || !pPlayer->IsPlayer() )
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
	


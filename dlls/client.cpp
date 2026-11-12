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
	// one address is turned away (sic: hard-coded)
	if ( !strcmp( pszAddress, "24.1.14.214:27005" ) )
		CLIENT_COMMAND( pEntity, "disconnect\n" );

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
// RescueNotify - every counter-terrorist hears that hostages
// were rescued (name from the later versions' Linux symbols)
//=========================================================
void RescueNotify( void )
{
	CBaseEntity *pPlayer = NULL;

	pPlayer = UTIL_FindEntityByClassname( pPlayer, "player" );
	while ( pPlayer != NULL && !FNullEnt( pPlayer->edict() ) )
	{
		if ( pPlayer->IsPlayer() && pPlayer->pev->flags != FL_DORMANT )
		{
			CBasePlayer *player = GetClassPtr( (CBasePlayer *)pPlayer->pev );

			if ( player->m_iTeam == TEAM_CT )
				EMIT_SOUND( ENT(player->pev), CHAN_VOICE, "radio/rescued.wav", 1, ATTN_NORM );
		}
		pPlayer = UTIL_FindEntityByClassname( pPlayer, "player" );
	}
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
		if ( pPlayer->IsPlayer() && pPlayer->pev->flags != FL_DORMANT )
		{
			CBasePlayer *player = GetClassPtr( (CBasePlayer *)pPlayer->pev );

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
		if ( pPlayer->IsPlayer() && pPlayer->pev->flags != FL_DORMANT )
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
// CountPlayers - the players in the game (name from the
// later versions' Linux symbols)
//=========================================================
int CountPlayers( void )
{
	int iCount = 0;
	CBaseEntity *pPlayer = NULL;

	pPlayer = UTIL_FindEntityByClassname( pPlayer, "player" );
	while ( pPlayer != NULL && !FNullEnt( pPlayer->edict() ) )
	{
		if ( pPlayer->IsPlayer() && pPlayer->pev->flags != FL_DORMANT )
			iCount++;
		pPlayer = UTIL_FindEntityByClassname( pPlayer, "player" );
	}

	return iCount;
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
		if ( pPlayer->IsPlayer() && pPlayer->pev->flags != FL_DORMANT )
		{
			if ( GetClassPtr( (CBasePlayer *)pPlayer->pev )->m_iTeam == iTeam )
				iCount++;
		}
		pPlayer = UTIL_FindEntityByClassname( pPlayer, "player" );
	}

	return iCount;
}

//=========================================================
// KickPlayer - the iKickedth player (as ListPlayers numbers
// them), if he is on the voters' team and more than 65 % of
// that team voted against him, is told so and disconnected;
// then nobody's kick vote stands (Beta 4.0: the votes and the
// team are new parameters; names from the Linux symbols,
// parameter names ours)
//=========================================================
void KickPlayer( int iKicked, int iVotes, int iTeam )
{
	char message[120];
	int iCount = 0;
	bool fKicked = false;	// name ours
	CBaseEntity *pPlayer = NULL;

	if ( iKicked )
	{
		pPlayer = UTIL_FindEntityByClassname( pPlayer, "player" );
		while ( pPlayer != NULL && !FNullEnt( pPlayer->edict() ) )
		{
			if ( pPlayer->IsPlayer() && pPlayer->pev->flags != FL_DORMANT )
			{
				iCount++;
				if ( iCount == iKicked )
				{
					CBasePlayer *player = GetClassPtr( (CBasePlayer *)pPlayer->pev );

					if ( player->m_iTeam == iTeam && iVotes > CountTeamPlayers( iTeam ) * 0.65 )
					{
						fKicked = true;

						edict_t *pKicked = pPlayer->edict();

						player->m_fKicked = true;

						strcpy( message, "Kicked " );
						strcat( message, STRING( pPlayer->pev->netname ) );
						UTIL_ClientPrintAll( HUD_PRINTCENTER, message );

						CLIENT_COMMAND( pKicked, "disconnect\n" );
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
			if ( pPlayer->IsPlayer() && pPlayer->pev->flags != FL_DORMANT )
			{
				CBasePlayer *player = GetClassPtr( (CBasePlayer *)pPlayer->pev );

				player->m_iKickVote = 0;
			}
			pPlayer = UTIL_FindEntityByClassname( pPlayer, "player" );
		}
	}
}


// the team with fewer players; counter-terrorists when the terrorists are as
// many or more (Beta 4.0; Beta 3.1 only when more)
int SelectDefaultTeam( void )
{
	int iNumTerrorist = 0;
	int iNumCT = 0;
	CBaseEntity *pPlayer = NULL;

	while ( ((pPlayer = UTIL_FindEntityByClassname( pPlayer, "player" )) != NULL) && (!FNullEnt(pPlayer->edict())) )
	{
		if ( pPlayer->IsPlayer() && pPlayer->pev->flags != FL_DORMANT )
		{
			CBasePlayer *pTeamPlayer = GetClassPtr( (CBasePlayer *)pPlayer->pev );

			if ( pTeamPlayer->m_iTeam == TEAM_CT )
				iNumCT++;
			else if ( pTeamPlayer->m_iTeam == TEAM_TERRORIST )
				iNumTerrorist++;
		}
	}

	return ( iNumTerrorist >= iNumCT ) ? TEAM_CT : TEAM_TERRORIST;
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

	pPlayer->m_iTeam = SelectDefaultTeam();
	// Beta 3.0: the first player of a team is its commander; the model is
	// one of the team's two at random
	if ( pPlayer->m_iTeam == TEAM_CT )
	{
		if ( ((CHalfLifeMultiplay *)g_pGameRules)->m_pCTCommander == NULL )
			((CHalfLifeMultiplay *)g_pGameRules)->m_pCTCommander = pPlayer;
		if ( RANDOM_LONG( 0, 1 ) )
			pPlayer->m_iAppearance = 0;		// urban
		else
			pPlayer->m_iAppearance = 3;		// gsg9
	}
	else
	{
		if ( ((CHalfLifeMultiplay *)g_pGameRules)->m_pTerroristCommander == NULL )
			((CHalfLifeMultiplay *)g_pGameRules)->m_pTerroristCommander = pPlayer;
		if ( RANDOM_LONG( 0, 1 ) )
			pPlayer->m_iAppearance = 1;		// terror
		else
			pPlayer->m_iAppearance = 2;		// arab
	}
	pPlayer->m_fNotKilled = false;
	pPlayer->m_fJustConnected = true;
	pPlayer->m_iAccount = 0;
	pPlayer->m_iNightVisionAlpha = 60;
	pPlayer->m_fKicked = false;
	pPlayer->m_iIgnoreMessages = 0;
	pPlayer->m_iTeamKills = 0;		// Beta 4.0

	// Allocate a CBasePlayer for pev, and call spawn
	pPlayer->Spawn() ;
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

	// the dead talk to their team only, and only the dead hear them
	if ( player->pev->deadflag != DEAD_NO )
	{
		bSenderDead = true;
		teamonly = 1;
	}

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
	if ( teamonly )
	{
		if ( player->m_iTeam == TEAM_CT )
			sprintf( text, "%c(Counter-Terrorist) %s :   ", 2, STRING( pEntity->v.netname ) );
		else
			sprintf( text, "%c(Terrorist) %s :   ", 2, STRING( pEntity->v.netname ) );
	}
	else
		sprintf( text, "%c%s :    ", 2, STRING( pEntity->v.netname ) );

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



// sic: intermediate code retail has ahead of the buy menus, which places client.obj's
// header inlines (MESSAGE_BEGIN, ENTINDEX) where Beta 4.0 has them when client.cpp
// writes its own .pch.
inline void sic_il_client( int *p )
{
	p[0] = 0;
	p[1] = 0;
	p[2] = 0;
	p[3] = 0;
	p[4] = 0;
	p[5] = 0;
	p[6] = 0;
}

// within 512 units of one of the team's spawn points (counter-terrorists:
// info_player_start, terrorists: info_player_deathmatch); the buy menus
// open only there
bool IsCloseToBase( CBasePlayer *pPlayer )
{
	CBaseEntity *pSpot = NULL;

	if ( pPlayer->m_iTeam == TEAM_CT )
	{
		while ( (pSpot = UTIL_FindEntityByClassname( pSpot, "info_player_start" )) != NULL )
		{
			if ( (pSpot->pev->origin - pPlayer->pev->origin).Length() < 512 )
				return true;
		}
	}
	else if ( pPlayer->m_iTeam == TEAM_TERRORIST )
	{
		while ( (pSpot = UTIL_FindEntityByClassname( pSpot, "info_player_deathmatch" )) != NULL )
		{
			if ( (pSpot->pev->origin - pPlayer->pev->origin).Length() < 512 )
				return true;
		}
	}

	return false;
}

// takes the named weapon away from the player (it is not thrown into the
// world); Beta 1.x's purchases used it, Beta 2.0 calls it no more
void DropItem( CBasePlayer *pPlayer, char *pszItemName )
{
	for ( int i = 0; i <= 5; i++ )
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

// Beta 2.0 drops the weapon to the ground (DropPlayerItem) where Beta 1.x took it
// away (DropItem, which nothing calls any more)
void DropSecondary( CBasePlayer *pPlayer )
{
	pPlayer->DropPlayerItem( "weapon_usp" );
	pPlayer->DropPlayerItem( "weapon_glock18" );
	pPlayer->DropPlayerItem( "weapon_deagle" );
	pPlayer->DropPlayerItem( "weapon_p228" );
}

void DropPrimary( CBasePlayer *pPlayer )
{
	pPlayer->DropPlayerItem( "weapon_awp" );
	pPlayer->DropPlayerItem( "weapon_g3sg1" );
	pPlayer->DropPlayerItem( "weapon_m3" );
	pPlayer->DropPlayerItem( "weapon_mp5navy" );
	pPlayer->DropPlayerItem( "weapon_tmp" );
	pPlayer->DropPlayerItem( "weapon_p90" );
	pPlayer->DropPlayerItem( "weapon_m4a1" );
	pPlayer->DropPlayerItem( "weapon_m249" );
	pPlayer->DropPlayerItem( "weapon_sg552" );
	pPlayer->DropPlayerItem( "weapon_ak47" );
	pPlayer->DropPlayerItem( "weapon_scout" );
}


// The buy menus, open only near the team's spawn points (IsCloseToBase). A
// weapon replaces the one of its kind, which is dropped to the ground; without
// the money the player is told so. (Names from the later versions' Linux symbols.)

void BuyPistol( CBasePlayer *pPlayer, int iSlot )
{
	if ( !IsCloseToBase( pPlayer ) )
		return;

	switch ( iSlot )
	{
	case 1:
		if ( pPlayer->m_iAccount >= 500 )
		{
			DropSecondary( pPlayer );
			pPlayer->GiveNamedItem( "weapon_usp" );
			pPlayer->m_iAccount -= 500;
		}
		else
			ClientPrint( pPlayer->pev, HUD_PRINTCENTER, "Not enough money" );
		break;
	case 2:
		if ( pPlayer->m_iAccount >= 400 )
		{
			DropSecondary( pPlayer );
			pPlayer->GiveNamedItem( "weapon_glock18" );
			pPlayer->m_iAccount -= 400;
		}
		else
			ClientPrint( pPlayer->pev, HUD_PRINTCENTER, "Not enough money" );
		break;
	case 3:
		if ( pPlayer->m_iAccount >= 650 )
		{
			DropSecondary( pPlayer );
			pPlayer->GiveNamedItem( "weapon_deagle" );
			pPlayer->m_iAccount -= 650;
		}
		else
			ClientPrint( pPlayer->pev, HUD_PRINTCENTER, "Not enough money" );
		break;
	case 4:		// Beta 4.0 (the menu says 600, sic)
		if ( pPlayer->m_iAccount >= 650 )
		{
			DropSecondary( pPlayer );
			pPlayer->GiveNamedItem( "weapon_p228" );
			pPlayer->m_iAccount -= 650;
		}
		else
			ClientPrint( pPlayer->pev, HUD_PRINTCENTER, "Not enough money" );
		break;
	}
}

void BuyShotgun( CBasePlayer *pPlayer, int iSlot )
{
	if ( !IsCloseToBase( pPlayer ) )
		return;

	switch ( iSlot )
	{
	case 1:
		if ( pPlayer->m_iAccount >= 2000 )
		{
			DropPrimary( pPlayer );
			pPlayer->GiveNamedItem( "weapon_m3" );
			pPlayer->m_iAccount -= 2000;
		}
		else
			ClientPrint( pPlayer->pev, HUD_PRINTCENTER, "Not enough money" );
		break;
	}
}

void BuySubMachineGun( CBasePlayer *pPlayer, int iSlot )
{
	if ( !IsCloseToBase( pPlayer ) )
		return;

	switch ( iSlot )
	{
	case 1:
		if ( pPlayer->m_iAccount >= 1500 )
		{
			DropPrimary( pPlayer );
			pPlayer->GiveNamedItem( "weapon_mp5navy" );
			pPlayer->m_iAccount -= 1500;
		}
		else
			ClientPrint( pPlayer->pev, HUD_PRINTCENTER, "Not enough money" );
		break;
	case 2:
		if ( pPlayer->m_iAccount >= 1250 )
		{
			DropPrimary( pPlayer );
			pPlayer->GiveNamedItem( "weapon_tmp" );
			pPlayer->m_iAccount -= 1250;
		}
		else
			ClientPrint( pPlayer->pev, HUD_PRINTCENTER, "Not enough money" );
		break;
	case 3:
		if ( pPlayer->m_iAccount >= 2250 )
		{
			DropPrimary( pPlayer );
			pPlayer->GiveNamedItem( "weapon_p90" );
			pPlayer->m_iAccount -= 2250;
		}
		else
			ClientPrint( pPlayer->pev, HUD_PRINTCENTER, "Not enough money" );
		break;
	}
}

void BuyRifle( CBasePlayer *pPlayer, int iSlot )
{
	if ( !IsCloseToBase( pPlayer ) )
		return;

	switch ( iSlot )
	{
	case 1:	// counter-terrorists only
		if ( pPlayer->m_iTeam == TEAM_TERRORIST )
		{
			ClientPrint( pPlayer->pev, HUD_PRINTCENTER, "Weapon not available" );
			return;
		}
		if ( pPlayer->m_iAccount >= 3250 )
		{
			DropPrimary( pPlayer );
			pPlayer->GiveNamedItem( "weapon_m4a1" );
			pPlayer->m_iAccount -= 3250;
		}
		else
			ClientPrint( pPlayer->pev, HUD_PRINTCENTER, "Not enough money" );
		break;
	case 2:
		if ( pPlayer->m_iAccount >= 4750 )
		{
			DropPrimary( pPlayer );
			pPlayer->GiveNamedItem( "weapon_awp" );
			pPlayer->m_iAccount -= 4750;
		}
		else
			ClientPrint( pPlayer->pev, HUD_PRINTCENTER, "Not enough money" );
		break;
	case 3:
		if ( pPlayer->m_iAccount >= 5000 )
		{
			DropPrimary( pPlayer );
			pPlayer->GiveNamedItem( "weapon_g3sg1" );
			pPlayer->m_iAccount -= 5000;
		}
		else
			ClientPrint( pPlayer->pev, HUD_PRINTCENTER, "Not enough money" );
		break;
	case 4:
		if ( pPlayer->m_iAccount >= 3500 )
		{
			DropPrimary( pPlayer );
			pPlayer->GiveNamedItem( "weapon_sg552" );
			pPlayer->m_iAccount -= 3500;
		}
		else
			ClientPrint( pPlayer->pev, HUD_PRINTCENTER, "Not enough money" );
		break;
	case 5:	// terrorists only
		if ( pPlayer->m_iTeam == TEAM_CT )
		{
			ClientPrint( pPlayer->pev, HUD_PRINTCENTER, "Weapon not available" );
			return;
		}
		if ( pPlayer->m_iAccount >= 2500 )
		{
			DropPrimary( pPlayer );
			pPlayer->GiveNamedItem( "weapon_ak47" );
			pPlayer->m_iAccount -= 2500;
		}
		else
			ClientPrint( pPlayer->pev, HUD_PRINTCENTER, "Not enough money" );
		break;
	case 6:		// Beta 4.0
		if ( pPlayer->m_iAccount >= 2750 )
		{
			DropPrimary( pPlayer );
			pPlayer->GiveNamedItem( "weapon_scout" );
			pPlayer->m_iAccount -= 2750;
		}
		else
			ClientPrint( pPlayer->pev, HUD_PRINTCENTER, "Not enough money" );
		break;
	}
}

void BuyMachineGun( CBasePlayer *pPlayer, int iSlot )
{
	if ( !IsCloseToBase( pPlayer ) )
		return;

	switch ( iSlot )
	{
	case 1:
		if ( pPlayer->m_iAccount >= 5750 )
		{
			DropPrimary( pPlayer );
			pPlayer->GiveNamedItem( "weapon_m249" );
			pPlayer->m_iAccount -= 5750;
		}
		else
			ClientPrint( pPlayer->pev, HUD_PRINTCENTER, "Not enough money" );
		break;
	}
}

void BuyItem( CBasePlayer *pPlayer, int iSlot )
{
	if ( !IsCloseToBase( pPlayer ) )
		return;

	switch ( iSlot )
	{
	case 1:		// kevlar, only below 50 armor
		if ( pPlayer->pev->armorvalue < 50 )
		{
			if ( pPlayer->m_iAccount >= 650 )
			{
				pPlayer->GiveNamedItem( "item_kevlar" );
				pPlayer->m_iAccount -= 650;
			}
			else
				ClientPrint( pPlayer->pev, HUD_PRINTCENTER, "Not enough money" );
		}
		break;
	case 2:		// night vision goggles, once (Beta 3.1: slot 3)
		if ( pPlayer->m_fHasNightVision != true )
		{
			if ( pPlayer->m_iAccount >= 1250 )
			{
				pPlayer->m_fHasNightVision = true;
				pPlayer->m_fNightVisionOn = false;
				EMIT_SOUND( ENT(pPlayer->pev), CHAN_VOICE, "items/equip_nvg.wav", 1, ATTN_NORM );
				pPlayer->m_iAccount -= 1250;
			}
			else
				ClientPrint( pPlayer->pev, HUD_PRINTCENTER, "Not enough money" );
		}
		break;
	case 3:		// kevlar and helmet, only below 50 armor (Beta 3.1: slot 4)
		if ( pPlayer->pev->armorvalue < 50 )
		{
			if ( pPlayer->m_iAccount >= 1000 )
			{
				pPlayer->GiveNamedItem( "item_assaultsuit" );
				pPlayer->m_iAccount -= 1000;
			}
			else
				ClientPrint( pPlayer->pev, HUD_PRINTCENTER, "Not enough money" );
		}
		break;
	case 4:		// two grenades a life at most (the menu's concussion grenades; Beta 3.1: slot 2, three)
		if ( pPlayer->m_iFlashBangs >= 2 )
		{
			ClientPrint( pPlayer->pev, HUD_PRINTCENTER, "Carrying too many" );
			return;
		}
		if ( pPlayer->m_iAccount >= 200 )
		{
			pPlayer->GiveNamedItem( "weapon_flashbang" );
			pPlayer->m_iFlashBangs++;
			pPlayer->m_iAccount -= 200;
		}
		else
			ClientPrint( pPlayer->pev, HUD_PRINTCENTER, "Not enough money" );
		break;
	case 5:		// Beta 4.0: one HE grenade a life
		if ( pPlayer->m_iHEGrenades >= 1 )
		{
			ClientPrint( pPlayer->pev, HUD_PRINTCENTER, "Carrying too many" );
			return;
		}
		if ( pPlayer->m_iAccount >= 300 )
		{
			pPlayer->GiveNamedItem( "weapon_hegrenade" );
			pPlayer->m_iHEGrenades++;
			pPlayer->m_iAccount -= 300;
		}
		else
			ClientPrint( pPlayer->pev, HUD_PRINTCENTER, "Not enough money" );
		break;
	}
}


// Beta 4.0: the types of ChangeTeam's parameters (names from the Linux
// symbols; the enumerators are ours, valued as TEAM_TERRORIST and TEAM_CT and
// as m_iAppearance)
enum TeamName { TERRORIST, CT };
enum ModelName { URBAN, TERROR, ARAB, GSG9 };

//=========================================================
// ChangeTeam - the player joins the team with the model, the
// change is announced to all and he dies to respawn there
// (Beta 4.0; name from the Linux symbols). Nothing calls it.
// A player leaving a team he commands should make that team
// pick a new commander; the joined team's commander is
// checked instead (sic).
//=========================================================
void ChangeTeam( edict_t *pEntity, CBasePlayer *pPlayer, TeamName team, ModelName model )
{
	int iIndex = pPlayer->entindex();
	int iOldTeam;				// name ours
	CBasePlayer *pCommander;	// name ours
	char szMessage[256];		// name ours

	if ( team == CT )
	{
		iOldTeam = TERRORIST;
		pCommander = ((CHalfLifeMultiplay *)g_pGameRules)->m_pCTCommander;
	}
	else
	{
		iOldTeam = CT;
		pCommander = ((CHalfLifeMultiplay *)g_pGameRules)->m_pTerroristCommander;
	}

	if ( pPlayer->m_iTeam == iOldTeam && pCommander == pPlayer )
		((CHalfLifeMultiplay *)g_pGameRules)->PickNewCommander( iOldTeam );

	pPlayer->pev->body = 0;
	pPlayer->m_iAppearance = model;

	switch ( model )
	{
	case TERROR:
		CLIENT_COMMAND( pEntity, "model terror\n" );
		g_engfuncs.pfnSetClientKeyValue( iIndex, g_engfuncs.pfnGetInfoKeyBuffer( pPlayer->edict() ), "model", "terror" );
		break;
	case ARAB:
		CLIENT_COMMAND( pEntity, "model arab\n" );
		g_engfuncs.pfnSetClientKeyValue( iIndex, g_engfuncs.pfnGetInfoKeyBuffer( pPlayer->edict() ), "model", "arab" );
		break;
	case GSG9:
		CLIENT_COMMAND( pEntity, "model gsg9\n" );
		g_engfuncs.pfnSetClientKeyValue( iIndex, g_engfuncs.pfnGetInfoKeyBuffer( pPlayer->edict() ), "model", "gsg9" );
		break;
	case URBAN:
		CLIENT_COMMAND( pEntity, "model urban\n" );
		g_engfuncs.pfnSetClientKeyValue( iIndex, g_engfuncs.pfnGetInfoKeyBuffer( pPlayer->edict() ), "model", "urban" );
		break;
	}

	if ( team != pPlayer->m_iTeam )
	{
		strcpy( szMessage, STRING( pPlayer->pev->netname ) );
		if ( team == CT )
			strcat( szMessage, " joined Counterterrorists" );
		else
			strcat( szMessage, " joined Terrorists" );
		UTIL_ClientPrintAll( HUD_PRINTCENTER, szMessage );
	}

	pPlayer->m_iTeam = team;
	((CHalfLifeMultiplay *)g_pGameRules)->CheckWinConditions();
	if ( pPlayer->pev->deadflag == DEAD_NO )
		ClientKill( pEntity );
}


// The team menus: the player joins the team, takes its model and dies, to
// be respawned on its side. With mp_limitteams 1 nobody may join the team that
// already has more players (Beta 2.0 read "limitteams", which does not exist, so
// the limit never applied; Beta 2.1 reads the registered name). (Names from the
// later versions' Linux symbols.)

void ChooseCT( edict_t *pEntity, CBasePlayer *pPlayer, int iSlot )
{
	int iIndex = pPlayer->entindex();

	if ( CountTeams() >= 1 && CVAR_GET_FLOAT( "mp_limitteams" ) == 1 && pPlayer->m_iTeam != TEAM_CT )
	{
		ClientPrint( pPlayer->pev, HUD_PRINTCENTER, "Sorry! Cannot join... too many CT's" );
		return;
	}

	// Beta 4.0: a terrorist commander who changes sides leaves his team a new
	// one; the body is reset
	switch ( iSlot )
	{
	case 1:
		if ( pPlayer->m_iTeam == TEAM_TERRORIST && ((CHalfLifeMultiplay *)g_pGameRules)->m_pTerroristCommander == pPlayer )
			((CHalfLifeMultiplay *)g_pGameRules)->PickNewCommander( TEAM_TERRORIST );
		pPlayer->pev->body = 0;
		pPlayer->m_iAppearance = 0;
		CLIENT_COMMAND( pEntity, "model urban\n" );
		g_engfuncs.pfnSetClientKeyValue( iIndex, g_engfuncs.pfnGetInfoKeyBuffer( pPlayer->edict() ), "model", "urban" );
		pPlayer->m_iTeam = TEAM_CT;
		((CHalfLifeMultiplay *)g_pGameRules)->CheckWinConditions();
		if ( pPlayer->pev->deadflag == DEAD_NO )
			ClientKill( pEntity );
		break;
	case 2:
		if ( pPlayer->m_iTeam == TEAM_TERRORIST && ((CHalfLifeMultiplay *)g_pGameRules)->m_pTerroristCommander == pPlayer )
			((CHalfLifeMultiplay *)g_pGameRules)->PickNewCommander( TEAM_TERRORIST );
		pPlayer->pev->body = 0;
		pPlayer->m_iAppearance = 3;
		CLIENT_COMMAND( pEntity, "model gsg9\n" );
		g_engfuncs.pfnSetClientKeyValue( iIndex, g_engfuncs.pfnGetInfoKeyBuffer( pPlayer->edict() ), "model", "gsg9" );
		pPlayer->m_iTeam = TEAM_CT;
		((CHalfLifeMultiplay *)g_pGameRules)->CheckWinConditions();
		if ( pPlayer->pev->deadflag == DEAD_NO )
			ClientKill( pEntity );
		break;
	}
}

void ChooseTerrorist( edict_t *pEntity, CBasePlayer *pPlayer, int iSlot )
{
	int iIndex = pPlayer->entindex();

	if ( CountTeams() <= -1 && CVAR_GET_FLOAT( "mp_limitteams" ) == 1 && pPlayer->m_iTeam != TEAM_TERRORIST )
	{
		ClientPrint( pPlayer->pev, HUD_PRINTCENTER, "Sorry! Cannot join... too many Terrorists" );
		return;
	}

	// a commander who changes sides leaves his team a new one (the
	// terrorists' in ChooseCT from Beta 4.0 on)
	switch ( iSlot )
	{
	case 1:
		if ( pPlayer->m_iTeam == TEAM_CT && ((CHalfLifeMultiplay *)g_pGameRules)->m_pCTCommander == pPlayer )
			((CHalfLifeMultiplay *)g_pGameRules)->PickNewCommander( TEAM_CT );
		pPlayer->pev->body = 0;
		pPlayer->m_iAppearance = 1;
		CLIENT_COMMAND( pEntity, "model terror\n" );
		g_engfuncs.pfnSetClientKeyValue( iIndex, g_engfuncs.pfnGetInfoKeyBuffer( pPlayer->edict() ), "model", "terror" );
		pPlayer->m_iTeam = TEAM_TERRORIST;
		((CHalfLifeMultiplay *)g_pGameRules)->CheckWinConditions();
		if ( pPlayer->pev->deadflag == DEAD_NO )
			ClientKill( pEntity );
		break;
	case 2:
		if ( pPlayer->m_iTeam == TEAM_CT && ((CHalfLifeMultiplay *)g_pGameRules)->m_pCTCommander == pPlayer )
			((CHalfLifeMultiplay *)g_pGameRules)->PickNewCommander( TEAM_CT );
		pPlayer->pev->body = 0;
		pPlayer->m_iAppearance = 2;
		CLIENT_COMMAND( pEntity, "model arab\n" );
		g_engfuncs.pfnSetClientKeyValue( iIndex, g_engfuncs.pfnGetInfoKeyBuffer( pPlayer->edict() ), "model", "arab" );
		pPlayer->m_iTeam = TEAM_TERRORIST;
		((CHalfLifeMultiplay *)g_pGameRules)->CheckWinConditions();
		if ( pPlayer->pev->deadflag == DEAD_NO )
			ClientKill( pEntity );
		break;
	}
}

// Team_Select's AUTO-SELECT: the smaller team. The menu shown is the other
// team's (sic), so the player ends up on the larger one. Beta 2.0 no longer
// sets m_iTeam here; the model menu's choice does. Beta 4.0 drops the alert for
// an invalid team.
void AutoChooseTeam( CBasePlayer *pPlayer )
{
	int iTeam = SelectDefaultTeam();	// name ours

	if ( iTeam == TEAM_CT )
	{
		ShowMenu( pPlayer, 1, 20, FALSE, "#Terrorist_Select" );
		pPlayer->m_iMenu = Menu_TerroristSelect;
	}
	else if ( iTeam == TEAM_TERRORIST )
	{
		ShowMenu( pPlayer, 1, 20, FALSE, "#CT_Select" );
		pPlayer->m_iMenu = Menu_CTSelect;
	}
}

//=========================================================
// CommanderRadio, Radio1, Radio2 - the radio menus (names from
// the Linux symbols; Beta 4.0 renames Beta 3.0's Radio Radio1
// and adds Radio2): a message every 1.5 seconds at most while
// the player has messages left; the commander's three only for
// a team's commander (Beta 4.0 no longer opens that menu).
//=========================================================
void CommanderRadio( CBasePlayer *pPlayer, int iSlot )
{
	if ( pPlayer->m_flNextRadio < gpGlobals->time && pPlayer->m_iRadioMessages > 0 )
	{
		pPlayer->m_iRadioMessages--;
		pPlayer->m_flNextRadio = gpGlobals->time + 1.5;

		if ( ((CHalfLifeMultiplay *)g_pGameRules)->m_pCTCommander == pPlayer || ((CHalfLifeMultiplay *)g_pGameRules)->m_pTerroristCommander == pPlayer )
		{
			switch ( iSlot )
			{
			case 1:
				pPlayer->Radio( "GETINPOS" );
				break;
			case 2:
				pPlayer->Radio( "GO" );
				break;
			case 3:
				pPlayer->Radio( "FOLLOWCOM" );
				break;
			}
		}
	}
}

void Radio1( CBasePlayer *pPlayer, int iSlot )
{
	if ( pPlayer->m_flNextRadio < gpGlobals->time && pPlayer->m_iRadioMessages > 0 )
	{
		pPlayer->m_iRadioMessages--;
		pPlayer->m_flNextRadio = gpGlobals->time + 1.5;

		switch ( iSlot )
		{
		case 1:
			if ( RANDOM_LONG( 0, 1 ) )
				pPlayer->Radio( "AFFIRM" );
			else
				pPlayer->Radio( "ROGER" );
			break;
		case 2:
			pPlayer->Radio( "BACKUP" );
			break;
		case 3:	// Beta 3.1: "Cover me/I'll take point"
			if ( RANDOM_LONG( 0, 1 ) )
				pPlayer->Radio( "COVERME" );
			else
				pPlayer->Radio( "POINT" );
			break;
		case 4:
			pPlayer->Radio( "ENEMYSPOT" );	// Beta 3.1: "fixed MRAD_ENEMYSPOTTED error"
			break;
		case 5:
			pPlayer->Radio( "INPOS" );
			break;
		case 6:		// Beta 4.0: the commander's two for everyone (Beta 3.1: REPORTIN, REPRTINGIN, MATEDOWN, ENEMYDOWN on 6 to 9)
			pPlayer->Radio( "GETINPOS" );
			break;
		case 7:
			pPlayer->Radio( "GO" );
			break;
		case 8:
			pPlayer->Radio( "REPORTIN" );
			break;
		case 9:
			pPlayer->Radio( "REPRTINGIN" );	// sic: Beta 3.1's sentence name
			break;
		}
	}
}

// Beta 4.0's second radio menu (titles.txt's RadioB)
void Radio2( CBasePlayer *pPlayer, int iSlot )
{
	if ( pPlayer->m_flNextRadio < gpGlobals->time && pPlayer->m_iRadioMessages > 0 )
	{
		pPlayer->m_iRadioMessages--;
		pPlayer->m_flNextRadio = gpGlobals->time + 1.5;

		switch ( iSlot )
		{
		case 1:
			pPlayer->Radio( "HITASSIST" );	// "I'm hit. Need assistance!"
			break;
		case 2:
			pPlayer->Radio( "BLOW" );		// "She's going to blow!"
			break;
		case 3:
			pPlayer->Radio( "FALLBACK" );
			break;
		case 4:
			pPlayer->Radio( "REGROUP" );
			break;
		case 5:
			pPlayer->Radio( "STICKTOG" );	// "Stick together team!"
			break;
		case 6:
			pPlayer->Radio( "CLEAR" );		// "Sector Clear!"
			break;
		case 7:
			pPlayer->Radio( "ENEMYDOWN" );
			break;
		}
	}
}

//=========================================================
// BuyGunAmmo - a box of the ammunition of the weapon with
// that id; nothing without the money (Beta 4.0; names from
// the Linux symbols)
//=========================================================
void BuyGunAmmo( CBasePlayer *pPlayer, int iWeapon )
{
	switch ( iWeapon )
	{
	case WEAPON_AWP:
		if ( pPlayer->m_iAccount >= 125 )
		{
			pPlayer->GiveNamedItem( "ammo_338magnum" );
			pPlayer->m_iAccount -= 125;
		}
		break;
	case WEAPON_M3:
		if ( pPlayer->m_iAccount >= 65 )
		{
			pPlayer->GiveNamedItem( "ammo_buckshot" );
			pPlayer->m_iAccount -= 65;
		}
		break;
	case WEAPON_P90:
		if ( pPlayer->m_iAccount >= 50 )
		{
			pPlayer->GiveNamedItem( "ammo_57mm" );
			pPlayer->m_iAccount -= 50;
		}
		break;
	case WEAPON_M249:
		if ( pPlayer->m_iAccount >= 60 )
		{
			pPlayer->GiveNamedItem( "ammo_556nato" );
			pPlayer->m_iAccount -= 60;
		}
		break;
	case WEAPON_M4A1:
		if ( pPlayer->m_iAccount >= 60 )
		{
			pPlayer->GiveNamedItem( "ammo_556nato" );
			pPlayer->m_iAccount -= 60;
		}
		break;
	case WEAPON_SG552:
		if ( pPlayer->m_iAccount >= 60 )
		{
			pPlayer->GiveNamedItem( "ammo_556nato" );
			pPlayer->m_iAccount -= 60;
		}
		break;
	case WEAPON_SCOUT:
		if ( pPlayer->m_iAccount >= 80 )
		{
			pPlayer->GiveNamedItem( "ammo_762nato" );
			pPlayer->m_iAccount -= 80;
		}
		break;
	case WEAPON_G3SG1:
		if ( pPlayer->m_iAccount >= 80 )
		{
			pPlayer->GiveNamedItem( "ammo_762nato" );
			pPlayer->m_iAccount -= 80;
		}
		break;
	case WEAPON_AK47:
		if ( pPlayer->m_iAccount >= 80 )
		{
			pPlayer->GiveNamedItem( "ammo_762nato" );
			pPlayer->m_iAccount -= 80;
		}
		break;
	case WEAPON_USP:
		if ( pPlayer->m_iAccount >= 25 )
		{
			pPlayer->GiveNamedItem( "ammo_45acp" );
			pPlayer->m_iAccount -= 25;
		}
		break;
	case WEAPON_GLOCK18:
		if ( pPlayer->m_iAccount >= 20 )
		{
			pPlayer->GiveNamedItem( "ammo_9mm" );
			pPlayer->m_iAccount -= 20;
		}
		break;
	case WEAPON_MP5N:
		if ( pPlayer->m_iAccount >= 20 )
		{
			pPlayer->GiveNamedItem( "ammo_9mm" );
			pPlayer->m_iAccount -= 20;
		}
		break;
	case WEAPON_TMP:
		if ( pPlayer->m_iAccount >= 20 )
		{
			pPlayer->GiveNamedItem( "ammo_9mm" );
			pPlayer->m_iAccount -= 20;
		}
		break;
	case WEAPON_DEAGLE:
		if ( pPlayer->m_iAccount >= 40 )
		{
			pPlayer->GiveNamedItem( "ammo_50ae" );
			pPlayer->m_iAccount -= 40;
		}
		break;
	case WEAPON_P228:
		if ( pPlayer->m_iAccount >= 50 )
		{
			pPlayer->GiveNamedItem( "ammo_357sig" );
			pPlayer->m_iAccount -= 50;
		}
		break;
	}
}

//=========================================================
// BuyPrimaryAmmo, BuySecondaryAmmo - a box for every weapon the
// player carries that is not a pistol, or that is (Beta 4.0;
// names from the Linux symbols). The walk along each slot's
// list goes no further than its second weapon, which it takes
// again and again up to six times (sic: the next item is read
// from the slot's first one).
//=========================================================
void BuyPrimaryAmmo( CBasePlayer *pPlayer )
{
	for ( int i = 0; i < MAX_ITEM_TYPES; i++ )
	{
		if ( pPlayer->m_rgpPlayerItems[ i ] )
		{
			CBasePlayerItem *pItem = pPlayer->m_rgpPlayerItems[ i ];
			int iCount = 0;		// name ours

			while ( pItem && iCount < MAX_ITEM_TYPES )
			{
				iCount++;
				if ( pItem->m_iId != WEAPON_USP && pItem->m_iId != WEAPON_GLOCK18 && pItem->m_iId != WEAPON_DEAGLE && pItem->m_iId != WEAPON_P228 )
					BuyGunAmmo( pPlayer, pItem->m_iId );
				pItem = pPlayer->m_rgpPlayerItems[ i ]->m_pNext;	// sic
			}
		}
	}
}

void BuySecondaryAmmo( CBasePlayer *pPlayer )
{
	for ( int i = 0; i < MAX_ITEM_TYPES; i++ )
	{
		if ( pPlayer->m_rgpPlayerItems[ i ] )
		{
			CBasePlayerItem *pItem = pPlayer->m_rgpPlayerItems[ i ];
			int iCount = 0;		// name ours

			while ( pItem && iCount < MAX_ITEM_TYPES )
			{
				iCount++;
				if ( pItem->m_iId == WEAPON_USP || pItem->m_iId == WEAPON_GLOCK18 || pItem->m_iId == WEAPON_DEAGLE || pItem->m_iId == WEAPON_P228 )
					BuyGunAmmo( pPlayer, pItem->m_iId );
				pItem = pPlayer->m_rgpPlayerItems[ i ]->m_pNext;	// sic
			}
		}
	}
}

// Beta 4.0's ammo menu: 1 primary, 2 secondary (name from the Linux symbols,
// where egcs inlined the two)
void BuyAmmo( CBasePlayer *pPlayer, int iSlot )
{
	if ( !IsCloseToBase( pPlayer ) )
		return;

	switch ( iSlot )
	{
	case 1:
		BuyPrimaryAmmo( pPlayer );
		break;
	case 2:
		BuySecondaryAmmo( pPlayer );
		break;
	}
}


/*
===========
ClientCommand
called each time a player uses a "cmd" command
============
*/
extern float g_flWeaponCheat;

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
	else if ( FStrEq(pcmd, "ammocheck" ) )
	{
		// Beta 4.0: buys the primary ammunition, anywhere
		BuyPrimaryAmmo( pPlayer );
	}
	else if ( FStrEq(pcmd, "say_team" ) )
	{
		Host_Say( pEntity, 1 );
	}
	else if ( FStrEq(pcmd, "showplayerinfo" ) )
	{
		// money, team and kick vote, two seconds on the screen
		char szText[100];
		char szKickVote[4];

		itoa( pPlayer->m_iAccount, szText, 10 );
		strcat( szText, " dollars\n" );
		switch ( pPlayer->m_iAppearance )
		{
		case 0:
			strcat( szText, "Counterterrorist : Seal TEAM 6\n" );
			break;
		case 3:
			strcat( szText, "Counterterrorist : German GSG9\n" );
			break;
		case 1:
			strcat( szText, "Terrorist : Pheonix Faction\n" );	// sic
			break;
		case 2:
			strcat( szText, "Terrorist : Durah Cell\n" );
			break;
		}
		if ( ((CHalfLifeMultiplay *)g_pGameRules)->m_pCTCommander == pPlayer || ((CHalfLifeMultiplay *)g_pGameRules)->m_pTerroristCommander == pPlayer )
			strcat( szText, "Commander\n" );
		itoa( pPlayer->m_iKickVote, szKickVote, 10 );
		strcat( szText, "Kick Vote = " );
		strcat( szText, szKickVote );
		ShowMenu( pPlayer, 0x200, 2, FALSE, szText );
	}
	else if ( FStrEq(pcmd, "gm_restartround" ) )
	{
		// Beta 1.x's round restart for anyone: now it throws out whoever tries it
		CLIENT_COMMAND( pEntity, "disconnect\n" );
	}
	else if ( FStrEq(pcmd, "vote" ) )
	{
		// Beta 4.0: a vote every 3 seconds at most. A vote is against player #n
		// (ListPlayers' numbers); KickPlayer counts it with the team mates' votes
		// against the same number and kicks him with more than 65 percent of the
		// team. Nothing withdraws a vote any more.
		if ( pPlayer->m_flNextVote > gpGlobals->time )
		{
			ClientPrint( pPlayer->pev, HUD_PRINTCENTER, "Please wait 3 seconds before casting another vote... moron!" );
			return;
		}
		pPlayer->m_flNextVote = gpGlobals->time + 3;

		if ( strlen( CMD_ARGV(1) ) <= 3 )
		{
			int iVote = atoi( CMD_ARGV(1) );
			char szText[100];
			char szNumber[4];

			strcpy( szText, "Vote casted against player #" );
			itoa( iVote, szNumber, 10 );
			strcat( szText, szNumber );
			UTIL_ClientPrintAll( HUD_PRINTCENTER, szText );

			pPlayer->m_iKickVote = iVote;

			int iVotes = 0;					// name ours
			CBaseEntity *pVoter = NULL;		// name ours

			pVoter = UTIL_FindEntityByClassname( pVoter, "player" );
			while ( pVoter != NULL && !FNullEnt( pVoter->edict() ) )
			{
				if ( pVoter->IsPlayer() )
				{
					CBasePlayer *player = GetClassPtr( (CBasePlayer *)pVoter->pev );

					if ( player->m_iTeam == pPlayer->m_iTeam && player->m_iKickVote == pPlayer->m_iKickVote )
						iVotes++;
				}
				pVoter = UTIL_FindEntityByClassname( pVoter, "player" );
			}

			KickPlayer( iVote, iVotes, pPlayer->m_iTeam );
		}
	}
	else if ( FStrEq(pcmd, "menuselect" ) )
	{
		int iSlot = atoi( CMD_ARGV(1) );

		switch ( pPlayer->m_iMenu )
		{
		case Menu_TeamSelect:
			if ( iSlot == 1 )
			{
				ShowMenu( pPlayer, 3, 20, FALSE, "#Terrorist_Select" );
				pPlayer->m_iMenu = Menu_TerroristSelect;
			}
			else if ( iSlot == 2 )
			{
				ShowMenu( pPlayer, 3, 20, FALSE, "#CT_Select" );
				pPlayer->m_iMenu = Menu_CTSelect;
			}
			else if ( iSlot == 3 )
			{
				AutoChooseTeam( pPlayer );
			}
			break;

		case Menu_CTSelect:
			ChooseCT( pEntity, pPlayer, iSlot );
			break;

		case Menu_TerroristSelect:
			ChooseTerrorist( pEntity, pPlayer, iSlot );
			break;

		case Menu_BuyWeapon:
			if ( iSlot == 1 )
			{
				ShowMenu( pPlayer, 0xff, 127, FALSE, "#BuyPistol" );	// Beta 4.0: four pistols (sic: eight keys)
				pPlayer->m_iMenu = Menu_BuyPistol;
			}
			else if ( iSlot == 2 )
			{
				ShowMenu( pPlayer, 1, 127, FALSE, "#BuyShotgun" );
				pPlayer->m_iMenu = Menu_BuyShotgun;
			}
			else if ( iSlot == 3 )
			{
				ShowMenu( pPlayer, 7, 127, FALSE, "#BuySubMachineGun" );
				pPlayer->m_iMenu = Menu_BuySubMachineGun;
			}
			else if ( iSlot == 4 )
			{
				ShowMenu( pPlayer, 0x3f, 127, FALSE, "#BuyRifle" );
				pPlayer->m_iMenu = Menu_BuyRifle;
			}
			else if ( iSlot == 5 )
			{
				ShowMenu( pPlayer, 1, 127, FALSE, "#BuyMachineGun" );
				pPlayer->m_iMenu = Menu_BuyMachineGun;
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

		case Menu_BuyAmmo:
			BuyAmmo( pPlayer, iSlot );
			break;

		case Menu_Radio:
			Radio1( pPlayer, iSlot );
			break;

		case Menu_Radio2:
			Radio2( pPlayer, iSlot );
			break;

		case Menu_CommanderRadio:
			CommanderRadio( pPlayer, iSlot );
			break;

		default:
			ALERT( at_console, "invalid menu" );
			break;
		}
	}
	else if ( FStrEq(pcmd, "chooseteam" ) )
	{
		ShowMenu( pPlayer, 7, 20, FALSE, "#Team_Select" );
		pPlayer->m_iMenu = Menu_TeamSelect;
	}
	else if ( FStrEq(pcmd, "listplayers" ) )
	{
		ListPlayers( pPlayer );
	}
	else if ( FStrEq(pcmd, "ignoremessages" ) )
	{
		// 0 -> 1 -> 2 -> 0 (Host_Say; Beta 3.1 drops 3, the radio, for ignoreradio)
		if ( pPlayer->m_iIgnoreMessages == 0 )
		{
			pPlayer->m_iIgnoreMessages = 1;
			ClientPrint( pPlayer->pev, HUD_PRINTCENTER, "Ignoring BROADCAST messages" );
		}
		else if ( pPlayer->m_iIgnoreMessages == 1 )
		{
			pPlayer->m_iIgnoreMessages = 2;
			ClientPrint( pPlayer->pev, HUD_PRINTCENTER, "Ignoring TEAM/BROADCAST messages" );
		}
		else
		{
			pPlayer->m_iIgnoreMessages = 0;
			ClientPrint( pPlayer->pev, HUD_PRINTCENTER, "** Accepting ALL messages **" );
		}
	}
	else if ( FStrEq(pcmd, "ignoreradio" ) )
	{
		// Beta 3.1: the radio on and off (CBasePlayer::Radio)
		if ( !pPlayer->m_fIgnoreRadio )
		{
			pPlayer->m_fIgnoreRadio = true;
			ClientPrint( pPlayer->pev, HUD_PRINTCENTER, "Radio Messages OFF" );
		}
		else
		{
			pPlayer->m_fIgnoreRadio = false;
			ClientPrint( pPlayer->pev, HUD_PRINTCENTER, "Radio Messages ON" );
		}
	}
	else if ( pPlayer->pev->deadflag != DEAD_NO )
	{
		// the dead have no other command
	}
	else if ( FStrEq(pcmd, "plant_c4" ) )
	{
		// Beta 4.0 (commanderradio is gone)
		pPlayer->PlantC4();
	}
	else if ( FStrEq(pcmd, "radio1" ) )
	{
		ShowMenu( pPlayer, 0x1ff, 10, FALSE, "#RadioA" );
		pPlayer->m_iMenu = Menu_Radio;
	}
	else if ( FStrEq(pcmd, "radio2" ) )
	{
		ShowMenu( pPlayer, 0x1ff, 10, FALSE, "#RadioB" );
		pPlayer->m_iMenu = Menu_Radio2;
	}
	else if ( FStrEq(pcmd, "nightvision" ) )
	{
		// the goggles: a green screen fade that stays, or back to normal
		if ( pPlayer->m_fHasNightVision == true )
		{
			if ( !pPlayer->m_fNightVisionOn )
			{
				UTIL_ScreenFade( pPlayer, Vector(20,200,20), 0.75, 1024, pPlayer->m_iNightVisionAlpha, 5 );	// FFADE_OUT | FFADE_STAYOUT
				EMIT_SOUND( ENT(pPlayer->pev), CHAN_VOICE, "items/nvg_on.wav", 1, ATTN_NORM );
				pPlayer->m_fNightVisionOn = true;
			}
			else
			{
				UTIL_ScreenFade( pPlayer, Vector(20,20,20), 0.5, 0.1, 10, 0 );	// FFADE_IN
				EMIT_SOUND( ENT(pPlayer->pev), CHAN_VOICE, "items/nvg_off.wav", 1, ATTN_NORM );
				pPlayer->m_fNightVisionOn = false;
			}
		}
	}
	else if ( FStrEq(pcmd, "adjustnvg" ) )
	{
		// Beta 3.0: the goggles' brightness, in steps of 5 (back to 0 at 150);
		// Beta 4.0 no longer prints it to the console
		if ( pPlayer->m_fHasNightVision == true && pPlayer->m_fNightVisionOn == true )
		{
			pPlayer->m_iNightVisionAlpha = (pPlayer->m_iNightVisionAlpha + 5) % 150;
			UTIL_ScreenFade( pPlayer, Vector(20,200,20), 0.75, 1024, pPlayer->m_iNightVisionAlpha, 5 );	// FFADE_OUT | FFADE_STAYOUT
			EMIT_SOUND( ENT(pPlayer->pev), CHAN_VOICE, "items/nvg_on.wav", 1, ATTN_NORM );
		}
	}
	else if ( FStrEq(pcmd, "drop" ) )
	{
		// Beta 3.1: drop is back (Beta 3.0 had none; 2.x's dropped the item named),
		// now the same as throwweapon
		DropPrimary( pPlayer );
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
	else if ( FStrEq(pcmd, "throwweapon" ) )
	{
		DropPrimary( pPlayer );
	}
	else if ( FStrEq(pcmd, "buyammo" ) )
	{
		if ( IsCloseToBase( pPlayer ) )
		{
			ShowMenu( pPlayer, 0x1ff, 20, FALSE, "#BuyAmmo" );
			pPlayer->m_iMenu = Menu_BuyAmmo;
		}
	}
	else if ( FStrEq(pcmd, "buyweapon" ) )
	{
		if ( IsCloseToBase( pPlayer ) )
		{
			ShowMenu( pPlayer, 0x1f, 20, FALSE, "#BuyWeapon" );
			pPlayer->m_iMenu = Menu_BuyWeapon;
		}
	}
	else if ( FStrEq(pcmd, "buyitem" ) )
	{
		if ( IsCloseToBase( pPlayer ) )
		{
			ShowMenu( pPlayer, 0x3f, 20, FALSE, "#BuyItem" );
			pPlayer->m_iMenu = Menu_BuyItem;
		}
	}
	else if ( FStrEq(pcmd, "hostage" ) )
	{
		// a counter-terrorist within 128 units of one of his team's spawn
		// points, or of an info_hostage_rescue, rescues the hostages within 256
		// units that follow him: the first is worth 1000 dollars (Beta 3.1: 500),
		// each next one 50 less
		if ( pPlayer->m_iTeam != TEAM_TERRORIST )
		{
			bool bAtBase = false;
			CBaseEntity *pSpot = NULL;

			while ( (pSpot = UTIL_FindEntityByClassname( pSpot, "info_player_start" )) != NULL )
			{
				if ( (pSpot->pev->origin - pPlayer->pev->origin).Length() < 128 )
				{
					bAtBase = true;
					break;
				}
			}

			if ( !bAtBase )
			{
				pSpot = NULL;
				while ( (pSpot = UTIL_FindEntityByClassname( pSpot, "info_hostage_rescue" )) != NULL )
				{
					if ( (pSpot->pev->origin - pPlayer->pev->origin).Length() < 128 )
					{
						bAtBase = true;
						break;
					}
				}
			}

			if ( bAtBase )
			{
				CBaseEntity *pHostage = NULL;
				int iRescued = 0;
				int iReward = 1000;		// Beta 4.0 (was 500)

				for ( int i = 0; i < 12; i++ )
				{
					pHostage = UTIL_FindEntityByClassname( pHostage, "hostage_entity" );

					if ( pHostage != NULL
						&& (pHostage->pev->origin - pPlayer->pev->origin).Length() < 256
						&& ((CHostage *)pHostage)->m_hTargetEnt == pPlayer
						&& pHostage->pev->deadflag != DEAD_RESPAWNABLE )
					{
						pHostage->pev->deadflag = DEAD_RESPAWNABLE;	// rescued
						pPlayer->AddAccount( iReward );
						iRescued++;
						iReward -= 50;
						((CHostage *)pHostage)->Remove();
						((CHalfLifeMultiplay *)g_pGameRules)->m_iHostagesRescued++;
					}
				}

				if ( iRescued > 0 )
				{
					((CHalfLifeMultiplay *)g_pGameRules)->CheckWinConditions();	// Beta 4.0
					Broadcast( "rescued" );
				}

				char szText[100];

				itoa( iRescued, szText, 10 );
				strcat( szText, " hostages rescued" );
				ShowMenu( pPlayer, 0x200, 2, FALSE, szText );
			}
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

	CBasePlayer *pPlayer = (CBasePlayer *)GET_PRIVATE( pEntity );	// name ours

	// the model is the one chosen in the menu (m_iAppearance), whatever the
	// client sets
	switch ( pPlayer->m_iAppearance )
	{
	case 1:
		CLIENT_COMMAND( pEntity, "model terror\n" );
		g_engfuncs.pfnSetClientKeyValue( ENTINDEX( pEntity ), g_engfuncs.pfnGetInfoKeyBuffer( pEntity ), "model", "terror" );
		break;
	case 2:
		CLIENT_COMMAND( pEntity, "model arab\n" );
		g_engfuncs.pfnSetClientKeyValue( ENTINDEX( pEntity ), g_engfuncs.pfnGetInfoKeyBuffer( pEntity ), "model", "arab" );
		break;
	case 3:
		CLIENT_COMMAND( pEntity, "model gsg9\n" );
		g_engfuncs.pfnSetClientKeyValue( ENTINDEX( pEntity ), g_engfuncs.pfnGetInfoKeyBuffer( pEntity ), "model", "gsg9" );
		break;
	case 0:
		CLIENT_COMMAND( pEntity, "model urban\n" );
		g_engfuncs.pfnSetClientKeyValue( ENTINDEX( pEntity ), g_engfuncs.pfnGetInfoKeyBuffer( pEntity ), "model", "urban" );
		break;
	default:	// the same as case 0 (VC5 merges the two)
		CLIENT_COMMAND( pEntity, "model urban\n" );
		g_engfuncs.pfnSetClientKeyValue( ENTINDEX( pEntity ), g_engfuncs.pfnGetInfoKeyBuffer( pEntity ), "model", "urban" );
		break;
	}

	// msg everyone if someone changes their name,  and it isn't the first time (changing no name to current name)
	if ( pEntity->v.netname && STRING(pEntity->v.netname)[0] != 0 && !FStrEq( STRING(pEntity->v.netname), g_engfuncs.pfnInfoKeyValue( infobuffer, "name" )) )
	{
		char szName[256];	// name ours
		char text[256];

		// Beta 3.0: a dead player keeps his name (but the change is announced, sic)
		strcpy( szName, STRING(pEntity->v.netname) );
		if ( pPlayer->pev->deadflag == DEAD_RESPAWNABLE )
			g_engfuncs.pfnSetClientKeyValue( pPlayer->entindex(), g_engfuncs.pfnGetInfoKeyBuffer( pPlayer->edict() ), "name", szName );

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

	gpGlobals->teamplay = CVAR_GET_FLOAT("teamplay");
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
	PRECACHE_SOUND("radio/com_go.wav");
	PRECACHE_SOUND("radio/moveout.wav");
	PRECACHE_SOUND("radio/rescued.wav");
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

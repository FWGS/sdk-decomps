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
// KickPlayer - the iKickedth player (as ListPlayers numbers
// them) is told so and disconnected; then nobody's kick vote
// stands (name from the later versions' Linux symbols)
//=========================================================
void KickPlayer( int iKicked )
{
	char message[120];
	int iCount = 0;
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
					edict_t *pKicked = pPlayer->edict();

					GetClassPtr( (CBasePlayer *)pPlayer->pev )->m_fKicked = true;

					strcpy( message, "Kicked " );
					strcat( message, STRING( pPlayer->pev->netname ) );
					UTIL_ClientPrintAll( HUD_PRINTCENTER, message );

					CLIENT_COMMAND( pKicked, "disconnect\n" );
				}
			}
			pPlayer = UTIL_FindEntityByClassname( pPlayer, "player" );
		}
	}

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


// the team with fewer players; counter-terrorists only when the terrorists
// are more
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

	return ( iNumTerrorist > iNumCT ) ? TEAM_CT : TEAM_TERRORIST;
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
	if ( pPlayer->m_iTeam == TEAM_CT )
		pPlayer->m_iAppearance = 0;		// urban
	else
		pPlayer->m_iAppearance = 1;		// terror
	pPlayer->m_fJustConnected = true;
	pPlayer->m_fNotKilled = false;
	pPlayer->m_iAccount = 0;
	pPlayer->m_fKicked = false;
	pPlayer->m_iIgnoreMessages = 0;

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



// sic: a stand-in for text Beta 2.0's client.cpp has before the buy menus and
// ours has not; what it was the image cannot say.
// VC5 emits a header inline once the IL of the file's own functions passes the
// inline's IL offset in the headers. Retail's client.obj has MESSAGE_BEGIN( ...,
// edict_t * ) after DropPrimary, ENTINDEX and MESSAGE_BEGIN( ..., entvars_t * )
// after BuyShotgun and CBaseEntity::entindex after ClientCommand: in the state the
// release build compiles it in (it writes its own .pch) that takes more IL ahead of
// DropPrimary than our text has. This function, never called, is that IL: eight
// statement lines and two empty statements (with 8 lines entindex follows
// ClientCommand but MESSAGE_BEGIN( ..., entvars_t * ) comes one function late, with
// 8 and three empty statements entindex comes one function early; one empty
// statement, int i, return, two blocks and other forms tried miss as well).
inline void sic_il_client( int *p )
{
	p[0] = 0;
	p[1] = 0;
	p[2] = 0;
	p[3] = 0;
	p[4] = 0;
	p[5] = 0;
	p[6] = 0;
	p[7] = 0;
	;
	;
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
}

void DropPrimary( CBasePlayer *pPlayer )
{
	pPlayer->DropPlayerItem( "weapon_awp" );
	pPlayer->DropPlayerItem( "weapon_g3sg1" );
	pPlayer->DropPlayerItem( "weapon_m3" );
	pPlayer->DropPlayerItem( "weapon_mp5navy" );
	pPlayer->DropPlayerItem( "weapon_tmp" );
	pPlayer->DropPlayerItem( "weapon_m4a1" );
	pPlayer->DropPlayerItem( "weapon_m249" );
	pPlayer->DropPlayerItem( "weapon_sg552" );
	pPlayer->DropPlayerItem( "weapon_ak47" );
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
	case 3:	// the menu says 800 (sic)
		if ( pPlayer->m_iAccount >= 850 )
		{
			DropSecondary( pPlayer );
			pPlayer->GiveNamedItem( "weapon_deagle" );
			pPlayer->m_iAccount -= 850;
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
		if ( pPlayer->m_iAccount >= 2250 )
		{
			DropPrimary( pPlayer );
			pPlayer->GiveNamedItem( "weapon_ak47" );
			pPlayer->m_iAccount -= 2250;
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
		if ( pPlayer->m_iAccount >= 5500 )
		{
			DropPrimary( pPlayer );
			pPlayer->GiveNamedItem( "weapon_m249" );
			pPlayer->m_iAccount -= 5500;
		}
		else
			ClientPrint( pPlayer->pev, HUD_PRINTCENTER, "Not enough money" );
		break;
	}
}

// one ammo box a selection; no message without the money
void BuyAmmo( CBasePlayer *pPlayer, int iSlot )
{
	if ( !IsCloseToBase( pPlayer ) )
		return;

	switch ( iSlot )
	{
	case 1:
		if ( pPlayer->m_iAccount >= 20 )
		{
			pPlayer->GiveNamedItem( "ammo_9mmclip" );
			pPlayer->m_iAccount -= 20;
		}
		break;
	case 2:
		if ( pPlayer->m_iAccount >= 25 )
		{
			pPlayer->GiveNamedItem( "ammo_45acp" );
			pPlayer->m_iAccount -= 25;
		}
		break;
	case 3:
		if ( pPlayer->m_iAccount >= 60 )
		{
			pPlayer->GiveNamedItem( "ammo_556nato" );
			pPlayer->m_iAccount -= 60;
		}
		break;
	case 4:
		if ( pPlayer->m_iAccount >= 80 )
		{
			pPlayer->GiveNamedItem( "ammo_762nato" );
			pPlayer->m_iAccount -= 80;
		}
		break;
	case 5:
		if ( pPlayer->m_iAccount >= 125 )
		{
			pPlayer->GiveNamedItem( "ammo_338magnum" );
			pPlayer->m_iAccount -= 125;
		}
		break;
	case 6:
		if ( pPlayer->m_iAccount >= 65 )
		{
			pPlayer->GiveNamedItem( "ammo_buckshot" );
			pPlayer->m_iAccount -= 65;
		}
		break;
	case 7:
		if ( pPlayer->m_iAccount >= 40 )
		{
			pPlayer->GiveNamedItem( "ammo_50ae" );
			pPlayer->m_iAccount -= 40;
		}
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
	case 2:		// three flashbangs a life at most
		if ( pPlayer->m_iFlashBangs >= 3 )
		{
			ClientPrint( pPlayer->pev, HUD_PRINTCENTER, "Carrying too many" );
			return;
		}
		if ( pPlayer->m_iAccount >= 150 )
		{
			pPlayer->GiveNamedItem( "weapon_flashbang" );
			pPlayer->m_iFlashBangs++;
			pPlayer->m_iAccount -= 150;
		}
		else
			ClientPrint( pPlayer->pev, HUD_PRINTCENTER, "Not enough money" );
		break;
	case 3:		// night vision goggles, once
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
	}
}


// The team menus: the player joins the team, takes its model and dies, to
// be respawned on its side. With limitteams 1 nobody may join the team that
// already has more players (sic: GameDLLInit registers the cvar as
// mp_limitteams, so "limitteams" reads 0 and the limit never applies). (Names
// from the later versions' Linux symbols.)

void ChooseCT( edict_t *pEntity, CBasePlayer *pPlayer, int iSlot )
{
	int iIndex = pPlayer->entindex();

	if ( CountTeams() >= 1 && CVAR_GET_FLOAT( "limitteams" ) == 1 )
	{
		ClientPrint( pPlayer->pev, HUD_PRINTCENTER, "Sorry! Cannot join... too many CT's" );
		return;
	}

	switch ( iSlot )
	{
	case 1:
		pPlayer->m_iAppearance = 0;
		CLIENT_COMMAND( pEntity, "model urban\n" );
		g_engfuncs.pfnSetClientKeyValue( iIndex, g_engfuncs.pfnGetInfoKeyBuffer( pPlayer->edict() ), "model", "urban" );
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

	if ( CountTeams() <= -1 && CVAR_GET_FLOAT( "limitteams" ) == 1 )
	{
		ClientPrint( pPlayer->pev, HUD_PRINTCENTER, "Sorry! Cannot join... too many Terrorists" );
		return;
	}

	switch ( iSlot )
	{
	case 1:
		pPlayer->m_iAppearance = 1;
		CLIENT_COMMAND( pEntity, "model terror\n" );
		g_engfuncs.pfnSetClientKeyValue( iIndex, g_engfuncs.pfnGetInfoKeyBuffer( pPlayer->edict() ), "model", "terror" );
		pPlayer->m_iTeam = TEAM_TERRORIST;
		((CHalfLifeMultiplay *)g_pGameRules)->CheckWinConditions();
		if ( pPlayer->pev->deadflag == DEAD_NO )
			ClientKill( pEntity );
		break;
	}
}

// Team_Select's AUTO-SELECT: the smaller team. The menu shown is the other
// team's (sic), so the player ends up on the larger one. Beta 2.0 no longer
// sets m_iTeam here; the model menu's choice does.
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
	else
		ALERT( at_console, "Invalid team for this player\n" );
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
		if ( pPlayer->m_iTeam == TEAM_CT )
			strcat( szText, "Counterterrorist\n" );
		else
			strcat( szText, "Terrorist\n" );
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
		// a vote to kick player #n (ListPlayers' numbers, 1 to 20); anything
		// else withdraws the vote. 61 percent of the players, and at least four
		// players in the game, kick him.
		if ( strlen( CMD_ARGV(1) ) <= 3 )
		{
			int iVote = atoi( CMD_ARGV(1) );
			CHalfLifeMultiplay *mp = (CHalfLifeMultiplay *)g_pGameRules;

			if ( iVote >= 1 && iVote <= 20 )
			{
				char szText[100];
				char szNumber[4];

				if ( pPlayer->m_iKickVote )
				{
					mp->m_iKickVotes[pPlayer->m_iKickVote - 1]--;
					if ( mp->m_iKickVotes[pPlayer->m_iKickVote - 1] < 0 )
						mp->m_iKickVotes[pPlayer->m_iKickVote - 1] = 0;

					if ( iVote )
						mp->m_iKickVotes[iVote - 1]++;
				}
				else
					mp->m_iKickVotes[iVote - 1]++;	// (the increment written twice: retail's branch goes straight to it)

				strcpy( szText, "Vote casted against player #" );
				itoa( iVote, szNumber, 10 );
				strcat( szText, szNumber );
				UTIL_ClientPrintAll( HUD_PRINTCENTER, szText );

				pPlayer->m_iKickVote = iVote;
			}
			else
			{
				if ( pPlayer->m_iKickVote )
				{
					mp->m_iKickVotes[pPlayer->m_iKickVote - 1]--;
					if ( mp->m_iKickVotes[pPlayer->m_iKickVote - 1] < 0 )
						mp->m_iKickVotes[pPlayer->m_iKickVote - 1] = 0;
				}

				pPlayer->m_iKickVote = 0;
			}

			// sic: with a withdrawn vote this reads m_iKickVotes[iVote - 1] for an
			// iVote outside 1 to 20
			if ( mp->m_iKickVotes[iVote - 1] >= CountPlayers() * 0.61 && CountPlayers() >= 4 )
			{
				KickPlayer( iVote );
				for ( int i = 0; i < 20; i++ )
					mp->m_iKickVotes[i] = 0;
			}
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
				ShowMenu( pPlayer, 1, 20, FALSE, "#Terrorist_Select" );
				pPlayer->m_iMenu = Menu_TerroristSelect;
			}
			else if ( iSlot == 2 )
			{
				ShowMenu( pPlayer, 1, 20, FALSE, "#CT_Select" );
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
				ShowMenu( pPlayer, 7, 127, FALSE, "#BuyPistol" );
				pPlayer->m_iMenu = Menu_BuyPistol;
			}
			else if ( iSlot == 2 )
			{
				ShowMenu( pPlayer, 1, 127, FALSE, "#BuyShotgun" );
				pPlayer->m_iMenu = Menu_BuyShotgun;
			}
			else if ( iSlot == 3 )
			{
				ShowMenu( pPlayer, 3, 127, FALSE, "#BuySubMachineGun" );
				pPlayer->m_iMenu = Menu_BuySubMachineGun;
			}
			else if ( iSlot == 4 )
			{
				ShowMenu( pPlayer, 0x1f, 127, FALSE, "#BuyRifle" );
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
		// 0 -> 1 -> 2 -> 0 (Host_Say)
		if ( pPlayer->m_iIgnoreMessages == 0 )
		{
			pPlayer->m_iIgnoreMessages = 1;
			ClientPrint( pPlayer->pev, HUD_PRINTCENTER, "Ignoring BROADCAST messages" );
		}
		else if ( pPlayer->m_iIgnoreMessages == 1 )
		{
			pPlayer->m_iIgnoreMessages = 2;
			ClientPrint( pPlayer->pev, HUD_PRINTCENTER, "Ignoring TEAM messages" );
		}
		else
		{
			pPlayer->m_iIgnoreMessages = 0;
			ClientPrint( pPlayer->pev, HUD_PRINTCENTER, "** Accepting ALL messages **" );
		}
	}
	else if ( pPlayer->pev->deadflag != DEAD_NO )
	{
		// the dead have no other command
	}
	else if ( FStrEq(pcmd, "nightvision" ) )
	{
		// the goggles: a green screen fade that stays, or back to normal
		if ( pPlayer->m_fHasNightVision == true )
		{
			if ( !pPlayer->m_fNightVisionOn )
			{
				UTIL_ScreenFade( pPlayer, Vector(20,200,20), 0.75, 1024, 60, 5 );	// FFADE_OUT | FFADE_STAYOUT
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
	else if ( FStrEq(pcmd, "drop" ) )
	{
		// player is dropping an item.
		GetClassPtr((CBasePlayer *)pev)->DropPlayerItem((char *)CMD_ARGV(1));
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
			ShowMenu( pPlayer, 0xff, 20, FALSE, "#BuyAmmo" );
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
			ShowMenu( pPlayer, 0x1f, 20, FALSE, "#BuyItem" );
			pPlayer->m_iMenu = Menu_BuyItem;
		}
	}
	else if ( FStrEq(pcmd, "hostage" ) )
	{
		// a counter-terrorist within 128 units of one of his team's spawn
		// points, or of an info_hostage_rescue, rescues the hostages within 256
		// units that follow him: the first is worth 500 dollars, each next one
		// 50 less
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
				int iReward = 500;

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
					RescueNotify();

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
	case 0:
		CLIENT_COMMAND( pEntity, "model urban\n" );
		g_engfuncs.pfnSetClientKeyValue( ENTINDEX( pEntity ), g_engfuncs.pfnGetInfoKeyBuffer( pEntity ), "model", "urban" );
		break;
	case 1:
		CLIENT_COMMAND( pEntity, "model terror\n" );
		g_engfuncs.pfnSetClientKeyValue( ENTINDEX( pEntity ), g_engfuncs.pfnGetInfoKeyBuffer( pEntity ), "model", "terror" );
		break;
	case 2:
		CLIENT_COMMAND( pEntity, "model arab\n" );
		g_engfuncs.pfnSetClientKeyValue( ENTINDEX( pEntity ), g_engfuncs.pfnGetInfoKeyBuffer( pEntity ), "model", "arab" );
		break;
	}

	// msg everyone if someone changes their name,  and it isn't the first time (changing no name to current name)
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
	PRECACHE_SOUND("radio/go.wav");
	PRECACHE_SOUND("radio/rescued.wav");
	PRECACHE_SOUND("items/kevlar.wav");
	PRECACHE_SOUND("items/nvg_on.wav");
	PRECACHE_SOUND("items/nvg_off.wav");
	PRECACHE_SOUND("items/equip_nvg.wav");

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
	PRECACHE_MODEL("models/scientist.mdl");

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

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

===== ctc_gamerules.cpp ========================================================

  Capture the Chicken's rules (CCaptureTheChicken: team deathmatch with
  the bots), and ShowPlayerMenu, the menu helper (its name is retail's,
  from its error message). The chickens and capture points are in
  ctc.cpp.

  The teams are Outlaws and Rustlers, with fixed player models (rogan,
  twnwest). A player who joins (or types "chooseteam") is held invisible
  and frozen, without weapons, until he picks a team from the menu (1,
  2, or 5 for the team with fewer players; bots always take 5). A capture
  scores 5 for the team and 5 frags for the player, who is healed to full
  health and 100 armour; everyone hears chicken/chick_score.wav and the
  chickens are quiet for 3 seconds. A stolen chicken plays
  chicken/chick_collect.wav to everyone. fraglimit applies to the team
  scores.

  The file and the names other than ShowPlayerMenu are ours.

*/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"
#include "gamerules.h"
#include "teamplay_gamerules.h"
#include "game.h"

extern int gmsgShowMenu;
extern int gmsgTeamInfo;
extern int gmsgTeamScore;
extern DLL_GLOBAL BOOL g_fGameOver;
extern cvar_t timeleft, fragsleft;

char *g_pszTeamNames[2] = { "Outlaws", "Rustlers" };
char *g_pszTeamModels[2] = { "rogan", "twnwest" };

float g_flChickenQuietTime;			// the chickens keep quiet until then (the score sound)
CBaseEntity *g_pWaitingChickens;	// sic: never set; the chickens pass it to WaitForGameRules
int g_iChickenUnused;				// sic: retail's .bss has an int here that no code reads or writes (name ours)

//=========================================================
// ShowPlayerMenu - sends a menu in chunks of 127
// characters
//=========================================================
void ShowPlayerMenu( CBasePlayer *pPlayer, char *pszText, int bitsValidSlots, int nDisplayTime, BOOL fNeedMore )
{
	char szChunk[128];
	unsigned int iLoops = 0;
	int iSent = 0;

	while ( iSent < strlen( pszText ) && iLoops++ < 200 )
	{
		char *pszStart = pszText + iSent;
		BOOL fMore = fNeedMore;

		if ( strlen( pszStart ) > 128 )
		{
			strncpy( szChunk, pszStart, 127 );
			szChunk[127] = 0;
			iSent += 127;
			fMore = TRUE;
		}
		else
		{
			strcpy( szChunk, pszStart );
			iSent += strlen( szChunk );
		}

		MESSAGE_BEGIN( MSG_ONE, gmsgShowMenu, NULL, pPlayer->edict() );
			WRITE_SHORT( bitsValidSlots );
			WRITE_CHAR( nDisplayTime );
			WRITE_BYTE( fMore );
			WRITE_STRING( szChunk );
		MESSAGE_END();
	}

	if ( iLoops >= 200 )
		ALERT( at_console, "ERROR: ShowPlayerMenu() - Menu loops forever!\n" );
}

//=========================================================
// GetModelTeamIndex - the team of a player model (unused)
//=========================================================
int GetModelTeamIndex( const char *pszModel )
{
	for ( int i = 0; i < 2; i++ )
	{
		if ( FStrEq( pszModel, g_pszTeamModels[i] ) )
			return i;
	}

	return 0;
}

//=========================================================
//=========================================================
void CCaptureTheChicken::Init( void )
{
	CVAR_SET_STRING( "mp_teamlist", UTIL_VarArgs( "%s;%s", g_pszTeamNames[0], g_pszTeamNames[1] ) );

	UTIL_PrecacheOther( "ctc_chicken" );
	UTIL_PrecacheOther( "ctc_capturepoint" );
	PRECACHE_SOUND( "chicken/chick_collect.wav" );
	PRECACHE_SOUND( "chicken/chick_score.wav" );

	m_fAutoTeam = FALSE;
	g_flChickenQuietTime = 0;

	for ( int i = 0; i < 2; i++ )
	{
		m_iTeamScore[i] = 0;
		m_iTeamDeaths[i] = 0;
	}

	PRECACHE_MODEL( "sprites/xbeam1.spr" );

	CWantedMultiplay::Init();
}

//=========================================================
// PlayerSpawn - not before the player has picked a team
// (pev->speed: 0 new, 1 choosing, -1 playing)
//=========================================================
void CCaptureTheChicken::PlayerSpawn( CBasePlayer *pPlayer )
{
	if ( pPlayer->pev->speed != 0 )
	{
		pPlayer->pev->weapons |= (1<<WEAPON_SUIT);
		CWantedMultiplay::PlayerSpawn( pPlayer );
	}
}

//=========================================================
// InitHUD - hold the player and show the team menu
//=========================================================
void CCaptureTheChicken::InitHUD( CBasePlayer *pPlayer )
{
	if ( pPlayer->pev->speed == 0 )
		CHalfLifeMultiplay::InitHUD( pPlayer );

	pPlayer->pev->movetype = MOVETYPE_NOCLIP;
	pPlayer->pev->solid = SOLID_NOT;
	pPlayer->pev->takedamage = DAMAGE_NO;
	pPlayer->pev->effects |= EF_NODRAW;
	pPlayer->EnableControl( FALSE );

	ShowPlayerMenu( pPlayer, UTIL_VarArgs( "Choose your team:\n\n  1. %s\n  2. %s\n\n  5. Auto team", g_pszTeamNames[0], g_pszTeamNames[1] ), (1<<0) | (1<<1) | (1<<4), 0, 0 );
	pPlayer->pev->speed = 1;

	// update this player with all the other players team info
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBaseEntity *plr = UTIL_PlayerByIndex( i );
		if ( plr && IsValidTeam( plr->TeamID() ) )
		{
			MESSAGE_BEGIN( MSG_ONE, gmsgTeamInfo, NULL, pPlayer->edict() );
				WRITE_BYTE( plr->entindex() );
				WRITE_STRING( plr->TeamID() );
			MESSAGE_END();
		}
	}

	for ( i = 0; i < 2; i++ )
	{
		MESSAGE_BEGIN( MSG_ALL, gmsgTeamScore );
			WRITE_STRING( g_pszTeamNames[i] );
			WRITE_SHORT( m_iTeamScore[i] );
			WRITE_SHORT( m_iTeamDeaths[i] );
		MESSAGE_END();
	}

	if ( pPlayer->pev->flags & FL_FAKECLIENT )
		ClientCommand( pPlayer, "botchooseteam" );
}

//=========================================================
// ClientUserInfoChanged - the team's model, whatever the
// player picks
//=========================================================
void CCaptureTheChicken::ClientUserInfoChanged( CBasePlayer *pPlayer, char *infobuffer )
{
	char *mdls = g_engfuncs.pfnInfoKeyValue( infobuffer, "model" );

	if ( !FStrEq( pPlayer->m_szTeamName, mdls ) )
	{
		edict_t *pentPlayer = pPlayer->edict();
		int clientIndex = ENTINDEX( pentPlayer );
		g_engfuncs.pfnSetClientKeyValue( clientIndex, g_engfuncs.pfnGetInfoKeyBuffer( pentPlayer ), "model", g_pszTeamModels[ GetTeamIndex( pPlayer->m_szTeamName ) ] );
	}
}

//=========================================================
// ClientCommand - the team menu
//=========================================================
BOOL CCaptureTheChicken::ClientCommand( CBasePlayer *pPlayer, const char *pcmd )
{
	if ( FStrEq( pcmd, "menuselect" ) || FStrEq( pcmd, "botchooseteam" ) )
	{
		int slot = 5;	// bots: auto team

		if ( FStrEq( pcmd, "menuselect" ) )
		{
			if ( CMD_ARGC() < 2 )
				return TRUE;
			slot = atoi( CMD_ARGV( 1 ) );
		}

		switch ( (int)pPlayer->pev->speed )
		{
		case 1:
			slot--;
			if ( slot >= 0 && slot < 2 )
				strncpy( pPlayer->m_szTeamName, g_pszTeamNames[slot], TEAM_NAME_LENGTH );
			else
			{
				// auto team
				m_fAutoTeam = TRUE;
				slot = GetTeamIndex( SetDefaultPlayerTeam( pPlayer ) );
			}

			edict_t *pentPlayer = pPlayer->edict();
			int clientIndex = ENTINDEX( pentPlayer );
			g_engfuncs.pfnSetClientKeyValue( clientIndex, g_engfuncs.pfnGetInfoKeyBuffer( pentPlayer ), "team", g_pszTeamNames[slot] );

			// notify everyone's HUD of the team change
			MESSAGE_BEGIN( MSG_ALL, gmsgTeamInfo );
				WRITE_BYTE( pPlayer->entindex() );
				WRITE_STRING( pPlayer->m_szTeamName );
			MESSAGE_END();

			pPlayer->pev->speed = -1;
			pPlayer->pev->movetype = MOVETYPE_WALK;
			pPlayer->pev->solid = SOLID_BBOX;
			pPlayer->pev->takedamage = DAMAGE_AIM;
			pPlayer->pev->effects &= ~EF_NODRAW;
			pPlayer->EnableControl( TRUE );
			pPlayer->Spawn();
			break;
		}
		return TRUE;
	}
	else if ( FStrEq( pcmd, "chooseteam" ) )
	{
		pPlayer->pev->speed = -1;
		pPlayer->RemoveAllItems( FALSE );
		InitHUD( pPlayer );
		return TRUE;
	}

	return CWantedMultiplay::ClientCommand( pPlayer, pcmd );
}

//=========================================================
// SetDefaultPlayerTeam - the team picked from the menu (or
// the one with fewer players), with the team's model
//=========================================================
const char *CCaptureTheChicken::SetDefaultPlayerTeam( CBasePlayer *pPlayer )
{
	edict_t *pentPlayer;
	int clientIndex;

	if ( !m_fAutoTeam )
	{
		// the base class takes the team from the model key
		pentPlayer = pPlayer->edict();
		clientIndex = ENTINDEX( pentPlayer );
		g_engfuncs.pfnSetClientKeyValue( clientIndex, g_engfuncs.pfnGetInfoKeyBuffer( pentPlayer ), "model", (char *)GetTeamID( pPlayer ) );
	}

	const char *pszTeam = CHalfLifeTeamplay::SetDefaultPlayerTeam( pPlayer );

	pentPlayer = pPlayer->edict();
	clientIndex = ENTINDEX( pentPlayer );
	g_engfuncs.pfnSetClientKeyValue( clientIndex, g_engfuncs.pfnGetInfoKeyBuffer( pentPlayer ), "model", g_pszTeamModels[ GetTeamIndex( GetTeamID( pPlayer ) ) ] );

	m_fAutoTeam = FALSE;
	return pszTeam;
}

//=========================================================
// UpdateTeamScore - a capture (pszChickenTeam: the chicken's
// team), or just the scoreboard
//=========================================================
void CCaptureTheChicken::UpdateTeamScore( CBasePlayer *pPlayer, const char *pszChickenTeam )
{
	if ( pszChickenTeam )
	{
		UTIL_EmitAmbientSound( ENT( 0 ), VARS( ENT( 0 ) )->origin, "chicken/chick_score.wav", 1, ATTN_NONE, 0, PITCH_NORM );
		g_flChickenQuietTime = gpGlobals->time + 3;
		UTIL_CenterPrintAll( UTIL_VarArgs( "%s captured the %s chicken!\n", STRING( pPlayer->pev->netname ), pszChickenTeam ) );

		pPlayer->AddPoints( 5, TRUE );
		pPlayer->pev->noise += 5;	// capture points (the bots' skill balance leaves them out)
		m_iTeamScore[ GetTeamIndex( pPlayer->m_szTeamName ) ] += 5;
		pPlayer->pev->health = pPlayer->pev->max_health;
		pPlayer->pev->armorvalue = 100;
	}

	MESSAGE_BEGIN( MSG_ALL, gmsgTeamScore );
		WRITE_STRING( pPlayer->m_szTeamName );
		WRITE_SHORT( m_iTeamScore[ GetTeamIndex( pPlayer->m_szTeamName ) ] );
		WRITE_SHORT( m_iTeamDeaths[ GetTeamIndex( pPlayer->m_szTeamName ) ] );
	MESSAGE_END();
}

//=========================================================
// Think - CHalfLifeTeamplay's, over the two team scores,
// then the bots
//=========================================================
void CCaptureTheChicken::Think( void )
{
	///// Check game rules /////
	static int last_frags;
	static int last_time;

	int frags_remaining = 0;
	int time_remaining = 0;

	if ( g_fGameOver )   // someone else quit the game already
	{
		CHalfLifeMultiplay::Think();
		return;
	}

	float flTimeLimit = CVAR_GET_FLOAT("mp_timelimit") * 60;

	time_remaining = (int)(flTimeLimit ? ( flTimeLimit - gpGlobals->time ) : 0);

	if ( flTimeLimit != 0 && gpGlobals->time >= flTimeLimit )
	{
		GoToIntermission();
		return;
	}

	float flFragLimit = fraglimit.value;
	if ( flFragLimit )
	{
		int bestfrags = 9999;
		int remain;

		// check if any team is over the frag limit
		for ( int i = 0; i < 2; i++ )
		{
			if ( m_iTeamScore[i] >= flFragLimit )
			{
				GoToIntermission();
				return;
			}

			remain = flFragLimit - m_iTeamScore[i];
			if ( remain < bestfrags )
			{
				bestfrags = remain;
			}
		}
		frags_remaining = bestfrags;
	}

	// Updates when frags change
	if ( frags_remaining != last_frags )
	{
		g_engfuncs.pfnCvar_DirectSet( &fragsleft, UTIL_VarArgs( "%i", frags_remaining ) );
	}

	// Updates once per second
	if ( timeleft.value != last_time )
	{
		g_engfuncs.pfnCvar_DirectSet( &timeleft, UTIL_VarArgs( "%i", time_remaining ) );
	}

	last_frags = frags_remaining;
	last_time  = time_remaining;

	CWantedMultiplay::Think();
}

//=========================================================
//=========================================================
BOOL CCaptureTheChicken::CanHavePlayerItem( CBasePlayer *pPlayer, CBasePlayerItem *pItem )
{
	if ( pItem && FClassnameIs( pItem->pev, "ctc_chicken" ) )
		return TRUE;

	return CHalfLifeTeamplay::CanHavePlayerItem( pPlayer, pItem );
}

void CCaptureTheChicken::PlayerGotWeapon( CBasePlayer *pPlayer, CBasePlayerItem *pWeapon )
{
	// the carrier keeps the chicken in hand
	if ( pPlayer->m_pActiveItem && FClassnameIs( pPlayer->m_pActiveItem->pev, "ctc_chicken" ) )
		pPlayer->m_pActiveItem->AddToPlayer( pPlayer );

	if ( FClassnameIs( pWeapon->pev, "ctc_chicken" ) )
	{
		UTIL_EmitAmbientSound( ENT( 0 ), VARS( ENT( 0 ) )->origin, "chicken/chick_collect.wav", 1, ATTN_NONE, 0, PITCH_NORM );
		UTIL_CenterPrintAll( UTIL_VarArgs( "%s stole the %s chicken!\n", STRING( pPlayer->pev->netname ), pWeapon->TeamID() ) );
	}
}

//=========================================================
// FShouldSwitchWeapon - never away from the chicken
//=========================================================
BOOL CCaptureTheChicken::FShouldSwitchWeapon( CBasePlayer *pPlayer, CBasePlayerItem *pWeapon )
{
	if ( pPlayer->m_pActiveItem && FClassnameIs( pPlayer->m_pActiveItem->pev, "ctc_chicken" ) )
		return FALSE;

	return TRUE;
}

void CCaptureTheChicken::PlayerKilled( CBasePlayer *pVictim, entvars_t *pKiller, entvars_t *pInflictor )
{
	int iDeaths = pVictim->m_iDeaths;

	CHalfLifeTeamplay::PlayerKilled( pVictim, pKiller, pInflictor );

	if ( iDeaths != pVictim->m_iDeaths )
	{
		m_iTeamDeaths[ GetTeamIndex( pVictim->m_szTeamName ) ]++;
		UpdateTeamScore( pVictim, NULL );
	}
}

//=========================================================
// DeadPlayerWeapons - a carrier drops the chicken and
// nothing else
//=========================================================
int CCaptureTheChicken::DeadPlayerWeapons( CBasePlayer *pPlayer )
{
	if ( pPlayer->HasNamedPlayerItem( "ctc_chicken" ) )
	{
		CBasePlayerItem *pChicken = pPlayer->m_pActiveItem;
		pPlayer->RemovePlayerItem( pChicken );
		pChicken->Drop();
		return GR_PLR_DROP_GUN_NO;
	}

	return CHalfLifeTeamplay::DeadPlayerWeapons( pPlayer );
}

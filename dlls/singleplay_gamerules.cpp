/***
*
*	Copyright (c) 1996-2001, Valve LLC. All rights reserved.
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
#include	"items.h"

extern DLL_GLOBAL CGameRules	*g_pGameRules;
extern DLL_GLOBAL BOOL	g_fGameOver;
extern int gmsgDeathMsg;	// client dll messages
extern int gmsgScoreInfo;
extern int gmsgMOTD;
extern int gmsgServerName;
extern int gmsgTeamNames;
extern int gmsgTeamInfo;
extern int gmsgGameMode;

#define ITEM_RESPAWN_TIME	30
#define WEAPON_RESPAWN_TIME	20
#define AMMO_RESPAWN_TIME	20
#define ENTITY_INTOLERANCE	100

//=========================================================
//=========================================================
CHalfLifeRules::CHalfLifeRules( void )
{
	RefreshSkillData();
}

//=========================================================
//=========================================================
void CHalfLifeRules::Think ( void )
{
}

//=========================================================
//=========================================================
BOOL CHalfLifeRules::IsMultiplayer( void )
{
	return FALSE;
}

//=========================================================
//=========================================================
BOOL CHalfLifeRules::IsDeathmatch ( void )
{
	return FALSE;
}

//=========================================================
//=========================================================
BOOL CHalfLifeRules::IsCoOp( void )
{
	return FALSE;
}


//=========================================================
//=========================================================
BOOL CHalfLifeRules::FShouldSwitchWeapon( CBasePlayer *pPlayer, CBasePlayerItem *pWeapon )
{
	if ( !pPlayer->m_pActiveItem )
	{
		// player doesn't have an active item!
		return TRUE;
	}

	if ( !pPlayer->m_pActiveItem->CanHolster() )
	{
		return FALSE;
	}

	return TRUE;
}

//=========================================================
//=========================================================
BOOL CHalfLifeRules :: GetNextBestWeapon( CBasePlayer *pPlayer, CBasePlayerItem *pCurrentWeapon )
{
	return FALSE;
}

//=========================================================
//=========================================================
BOOL CHalfLifeRules :: ClientConnected( edict_t *pEntity, const char *pszName, const char *pszAddress, char szRejectReason[ 128 ] )
{
	return TRUE;
}

void CHalfLifeRules :: InitHUD( CBasePlayer *pl )
{
	// the player is an agent, the only team there is
	strcpy( pl->m_szTeamName, "Agents" );

	// the map's title, or its name, for the scoreboard
	edict_t *pWorld = INDEXENT( 0 );
	MESSAGE_BEGIN( MSG_ALL, gmsgServerName );
		if ( pWorld->v.message )
			WRITE_STRING( STRING( pWorld->v.message ) );
		else
			WRITE_STRING( STRING( gpGlobals->mapname ) );
	MESSAGE_END();

	MESSAGE_BEGIN( MSG_ALL, gmsgTeamNames );
		WRITE_BYTE( 1 );
		WRITE_STRING( pl->m_szTeamName );
	MESSAGE_END();

	MESSAGE_BEGIN( MSG_ALL, gmsgScoreInfo );
		WRITE_BYTE( ENTINDEX(pl->edict()) );
		WRITE_SHORT( pl->pev->frags );
		WRITE_SHORT( pl->m_iDeaths );
		WRITE_SHORT( 0 );
		WRITE_SHORT( GetTeamIndex( pl->m_szTeamName ) + 1 );
	MESSAGE_END();

	MESSAGE_BEGIN( MSG_ALL, gmsgTeamInfo );
		WRITE_BYTE( ENTINDEX(pl->edict()) );
		WRITE_STRING( pl->m_szTeamName );
	MESSAGE_END();

	MESSAGE_BEGIN( MSG_ALL, gmsgGameMode );
		WRITE_BYTE( 1 );  // game mode teamplay
	MESSAGE_END();
}

//=========================================================
//=========================================================
void CHalfLifeRules :: ClientDisconnected( edict_t *pClient )
{
}

//=========================================================
//=========================================================
float CHalfLifeRules::FlPlayerFallDamage( CBasePlayer *pPlayer )
{
	// subtract off the speed at which a player is allowed to fall without being hurt,
	// so damage will be based on speed beyond that, not the entire fall
	pPlayer->m_flFallVelocity -= PLAYER_MAX_SAFE_FALL_SPEED;
	return pPlayer->m_flFallVelocity * DAMAGE_FOR_FALL_SPEED;
}

//=========================================================
//=========================================================
void CHalfLifeRules :: PlayerSpawn( CBasePlayer *pPlayer )
{
	CBaseEntity	*pWeaponEntity = NULL;

	while ( pWeaponEntity = UTIL_FindEntityByClassname( pWeaponEntity, "game_player_equip" ))
	{
		pWeaponEntity->Touch( pPlayer );
	}

	pPlayer->pev->weapons |= (1<<WEAPON_SUIT);
}

//=========================================================
//=========================================================
BOOL CHalfLifeRules :: AllowAutoTargetCrosshair( void )
{
	return TRUE;
}

//=========================================================
//=========================================================
void CHalfLifeRules :: PlayerThink( CBasePlayer *pPlayer )
{
}


//=========================================================
//=========================================================
BOOL CHalfLifeRules :: FPlayerCanRespawn( CBasePlayer *pPlayer )
{
	return TRUE;
}

//=========================================================
//=========================================================
float CHalfLifeRules :: FlPlayerSpawnTime( CBasePlayer *pPlayer )
{
	return gpGlobals->time;//now!
}

//=========================================================
// IPointsForKill - how many points awarded to anyone
// that kills this player?
//=========================================================
int CHalfLifeRules :: IPointsForKill( CBasePlayer *pAttacker, CBasePlayer *pKilled )
{
	return 1;
}

//=========================================================
// PlayerKilled - someone/something killed this player
//=========================================================
void CHalfLifeRules :: PlayerKilled( CBasePlayer *pVictim, entvars_t *pKiller, entvars_t *pInflictor )
{
	DeathNotice( pVictim, pKiller, pInflictor );

	pVictim->m_iDeaths += 1;

	// update the scores
	MESSAGE_BEGIN( MSG_ALL, gmsgScoreInfo );
		WRITE_BYTE( ENTINDEX(pVictim->edict()) );
		WRITE_SHORT( pVictim->pev->frags );
		WRITE_SHORT( pVictim->m_iDeaths );
		WRITE_SHORT( 0 );
		WRITE_SHORT( GetTeamIndex( pVictim->m_szTeamName ) + 1 );
	MESSAGE_END();
}

//=========================================================
// Deathnotice
//=========================================================
void CHalfLifeRules::DeathNotice( CBasePlayer *pVictim, entvars_t *pKiller, entvars_t *pevInflictor )
{
	// Work out what killed the player, and tell everybody about it
	EHANDLE hKiller, hInflictor;
	char szText[ 512 ];

	if ( !pKiller )
	{
		if ( pevInflictor )
			pKiller = pevInflictor;
		else
			pKiller = VARS( INDEXENT( 0 ) );	// the world
	}
	if ( !pevInflictor )
		pevInflictor = pKiller;

	hKiller = CBaseEntity::Instance( pKiller );
	hInflictor = CBaseEntity::Instance( pevInflictor );

	int iHitGroup = pVictim->m_LastHitGroup;
	const char *pszVerb = DamageVerb( 0, FALSE );
	const char *pszWeapon = "";
	const char *pszVictim = pVictim->GetHudName();
	const char *pszKiller = "";
	const char *pszHitGroup = "";
	BOOL fGibbed = ( pVictim->pev->effects & EF_NODRAW ) != 0;

	if ( pevInflictor == pKiller && hKiller->IsPlayer() )
	{
		// If the inflictor is the killer,  then it must be their current weapon doing the damage
		CBasePlayer *pPlayer = (CBasePlayer*)CBaseEntity::Instance( pKiller );

		if ( pPlayer && pPlayer->m_pActiveItem )
			hInflictor = pPlayer->m_pActiveItem;
	}

	if ( hInflictor )
	{
		pszWeapon = hInflictor->GetKillDesc();
		pszVerb = hInflictor->GetKillVerb( fGibbed, iHitGroup );
	}

	if ( hKiller )
		pszKiller = hKiller->GetKillDesc();

	if ( (CBaseEntity *)hKiller == pVictim )
	{
		// killed self
		if ( (CBaseEntity *)hKiller != (CBaseEntity *)hInflictor )
		{
			if ( hInflictor )
				pszHitGroup = hInflictor->GetHitGroupDesc( iHitGroup );

			// sic: the victim's name is passed where the format wants the hit group
			if ( pszHitGroup[0] == ' ' )
				sprintf( szText, "%s %s his own%s with %s\n", pszKiller, pszVerb, pszVictim, pszHitGroup, pszWeapon );
			else if ( pszHitGroup[0] )
				sprintf( szText, "%s %s his%s with %s\n", pszKiller, pszVerb, pszVictim, pszHitGroup + 2, pszWeapon );
			else
				sprintf( szText, "%s %s himself with %s\n", pszKiller, pszVerb, pszVictim, pszWeapon );
		}
		else
		{
			if ( VerbTakesName( pszVerb ) > 0 )
			{
				sprintf( szText, pszVerb, pszVictim );
				strcat( szText, "\n" );
			}
			else
				sprintf( szText, "%s %s\n", pszVictim, pszVerb );
		}
	}

	// sic: no else, the suicide message above is always overwritten
	if ( !pszKiller[0] )
	{
		if ( VerbTakesName( pszVerb ) > 0 )
		{
			sprintf( szText, pszVerb, pszVictim );
			strcat( szText, "\n" );
		}
		else
			sprintf( szText, "%s %s\n", pszVictim, pszVerb );
	}
	else
	{
		if ( hInflictor )
			pszHitGroup = hInflictor->GetHitGroupDesc( iHitGroup );

		if ( (CBaseEntity *)hKiller != (CBaseEntity *)hInflictor )
			sprintf( szText, "%s %s %s%s with %s\n", pszKiller, pszVerb, pszVictim, pszHitGroup, pszWeapon );
		else if ( pszHitGroup[0] == ' ' )
			sprintf( szText, "%s was %s%s by %s\n", pszVictim, pszVerb, pszHitGroup, pszWeapon );
		else
			sprintf( szText, "%s%s was %s by %s\n", pszVictim, pszHitGroup, pszVerb, pszWeapon );
	}

	UTIL_ClientPrintAll( HUD_PRINTNOTIFY, szText );
}

//=========================================================
// PlayerGotWeapon - player has grabbed a weapon that was
// sitting in the world
//=========================================================
void CHalfLifeRules :: PlayerGotWeapon( CBasePlayer *pPlayer, CBasePlayerItem *pWeapon )
{
}

//=========================================================
// FlWeaponRespawnTime - what is the time in the future
// at which this weapon may spawn?
//=========================================================
float CHalfLifeRules :: FlWeaponRespawnTime( CBasePlayerItem *pWeapon )
{
	return gpGlobals->time + WEAPON_RESPAWN_TIME;
}

//=========================================================
// FlWeaponRespawnTime - Returns 0 if the weapon can respawn 
// now,  otherwise it returns the time at which it can try
// to spawn again.
//=========================================================
float CHalfLifeRules :: FlWeaponTryRespawn( CBasePlayerItem *pWeapon )
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
Vector CHalfLifeRules :: VecWeaponRespawnSpot( CBasePlayerItem *pWeapon )
{
	return pWeapon->pev->origin;
}

//=========================================================
// WeaponShouldRespawn - any conditions inhibiting the
// respawning of this weapon?
//=========================================================
int CHalfLifeRules :: WeaponShouldRespawn( CBasePlayerItem *pWeapon )
{
	if ( pWeapon->pev->spawnflags & SF_NORESPAWN )
	{
		return GR_WEAPON_RESPAWN_NO;
	}

	return GR_WEAPON_RESPAWN_YES;
}

//=========================================================
// GetRespawnWeapon - a stand-in.  Retail: with
// ssh_rndweapons set, one of 22 weapon names at random
// (RANDOM_LONG( 0, 21 )), else the weapon's classname.
//=========================================================
char *CHalfLifeRules :: GetRespawnWeapon( CBasePlayerItem *pWeapon )
{
	return (char *)STRING( pWeapon->pev->classname );
}

//=========================================================
//=========================================================
BOOL CHalfLifeRules::CanHaveItem( CBasePlayer *pPlayer, CItem *pItem )
{
	return TRUE;
}

//=========================================================
//=========================================================
void CHalfLifeRules::PlayerGotItem( CBasePlayer *pPlayer, CItem *pItem )
{
}

//=========================================================
//=========================================================
int CHalfLifeRules::ItemShouldRespawn( CItem *pItem )
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
float CHalfLifeRules::FlItemRespawnTime( CItem *pItem )
{
	return gpGlobals->time + ITEM_RESPAWN_TIME;
}

//=========================================================
// Where should this item respawn?
// Some game variations may choose to randomize spawn locations
//=========================================================
Vector CHalfLifeRules::VecItemRespawnSpot( CItem *pItem )
{
	return pItem->pev->origin;
}

//=========================================================
//=========================================================
BOOL CHalfLifeRules::IsAllowedToSpawn( CBaseEntity *pEntity )
{
	return TRUE;
}

//=========================================================
//=========================================================
void CHalfLifeRules::PlayerGotAmmo( CBasePlayer *pPlayer, char *szName, int iCount )
{
}

//=========================================================
//=========================================================
int CHalfLifeRules::AmmoShouldRespawn( CBasePlayerAmmo *pAmmo )
{
	if ( pAmmo->pev->spawnflags & SF_NORESPAWN )
	{
		return GR_AMMO_RESPAWN_NO;
	}

	return GR_AMMO_RESPAWN_YES;
}

//=========================================================
//=========================================================
float CHalfLifeRules::FlAmmoRespawnTime( CBasePlayerAmmo *pAmmo )
{
	return gpGlobals->time + AMMO_RESPAWN_TIME;
}

//=========================================================
//=========================================================
Vector CHalfLifeRules::VecAmmoRespawnSpot( CBasePlayerAmmo *pAmmo )
{
	return pAmmo->pev->origin;
}

//=========================================================
//=========================================================
float CHalfLifeRules::FlHealthChargerRechargeTime( void )
{
	return 30;
}

//=========================================================
//=========================================================
int CHalfLifeRules::DeadPlayerWeapons( CBasePlayer *pPlayer )
{
	return GR_PLR_DROP_GUN_NO;
}

//=========================================================
//=========================================================
int CHalfLifeRules::DeadPlayerAmmo( CBasePlayer *pPlayer )
{
	return GR_PLR_DROP_AMMO_NO;
}

//=========================================================
//=========================================================
int CHalfLifeRules::PlayerRelationship( CBaseEntity *pPlayer, CBaseEntity *pTarget )
{
	// why would a single player in half life need this? 
	return GR_NOTTEAMMATE;
}

//=========================================================
//=========================================================
BOOL CHalfLifeRules :: FAllowMonsters( void )
{
	return TRUE;
}

//=========================================================
//=========================================================
BOOL CHalfLifeRules :: ClientSay( CBasePlayer *pPlayer, char *pText )
{
	return FALSE;
}

//=========================================================
//=========================================================
void CHalfLifeRules :: UpdateGameMode( CBasePlayer *pPlayer )
{
}

//=========================================================
//=========================================================
BOOL CHalfLifeRules :: FPlayerCanTakeDamage( CBasePlayer *pPlayer, CBaseEntity *pAttacker )
{
	return TRUE;
}

//=========================================================
//=========================================================
BOOL CHalfLifeRules :: ClientCommand( CBasePlayer *pPlayer, const char *pcmd )
{
	return FALSE;
}

//=========================================================
// FlHEVChargerRechargeTime - suit chargers recharge in
// single player only when sp_suitrecharge is set
//=========================================================
float CHalfLifeRules :: FlHEVChargerRechargeTime( void )
{
	if ( CVAR_GET_FLOAT( "sp_suitrecharge" ) )
		return 30;

	return 0;// don't recharge
}

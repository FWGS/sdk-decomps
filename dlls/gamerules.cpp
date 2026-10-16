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
//=========================================================
// GameRules.cpp
//=========================================================

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"player.h"
#include	"weapons.h"
#include	"gamerules.h"
#include	"teamplay_gamerules.h"
#include	"skill.h"

extern edict_t *EntSelectSpawnPoint( CBaseEntity *pPlayer );

DLL_GLOBAL CGameRules*	g_pGameRules = NULL;
extern DLL_GLOBAL BOOL	g_fGameOver;
extern int gmsgDeathMsg;	// client dll messages
extern int gmsgScoreInfo;
extern int gmsgMOTD;

//=========================================================
//=========================================================
BOOL CGameRules::CanHaveAmmo( CBasePlayer *pPlayer, const char *pszAmmoName, int iMaxCarry )
{
	int iAmmoIndex;

	if ( pszAmmoName )
	{
		iAmmoIndex = pPlayer->GetAmmoIndex( pszAmmoName );

		if ( iAmmoIndex > -1 )
		{
			if ( pPlayer->AmmoInventory( iAmmoIndex ) < iMaxCarry )
			{
				// player has room for more of this type of ammo
				return TRUE;
			}
		}
	}

	return FALSE;
}

//=========================================================
//=========================================================
edict_t *CGameRules :: GetPlayerSpawnSpot( CBasePlayer *pPlayer )
{
	edict_t *pentSpawnSpot = EntSelectSpawnPoint( pPlayer );

	pPlayer->pev->origin = VARS(pentSpawnSpot)->origin + Vector(0,0,1);
	pPlayer->pev->v_angle  = g_vecZero;
	pPlayer->pev->velocity = g_vecZero;
	pPlayer->pev->angles = VARS(pentSpawnSpot)->angles;
	pPlayer->pev->punchangle = g_vecZero;
	pPlayer->pev->fixangle = TRUE;
	
	return pentSpawnSpot;
}

//=========================================================
//=========================================================
BOOL CGameRules::CanHavePlayerItem( CBasePlayer *pPlayer, CBasePlayerItem *pWeapon )
{
	// only living players can have items
	if ( pPlayer->pev->deadflag != DEAD_NO )
		return FALSE;

	if ( pWeapon->pszAmmo1() )
	{
		if ( !CanHaveAmmo( pPlayer, pWeapon->pszAmmo1(), pWeapon->iMaxAmmo1() ) )
		{
			// we can't carry anymore ammo for this gun. We can only 
			// have the gun if we aren't already carrying one of this type
			if ( pPlayer->HasPlayerItem( pWeapon ) )
			{
				return FALSE;
			}
		}
	}
	else
	{
		// weapon doesn't use ammo, don't take another if you already have it.
		if ( pPlayer->HasPlayerItem( pWeapon ) )
		{
			return FALSE;
		}
	}

	// note: will fall through to here if GetItemInfo doesn't fill the struct!
	return TRUE;
}

//=========================================================
// load the SkillData struct with the proper values based on the skill level.
//=========================================================
void CGameRules::RefreshSkillData ( void )
{
	int	iSkill;

	iSkill = (int)CVAR_GET_FLOAT("skill");

	if ( iSkill < 1 )
	{
		iSkill = 1;
	}
	else if ( iSkill > 3 )
	{
		iSkill = 3; 
	}

	gSkillData.iSkillLevel = iSkill;

	ALERT ( at_console, "\nGAME SKILL LEVEL:%d\n",iSkill );

	gSkillData.hossHealth = GetSkillCvar( "sk_hoss_health" );

	gSkillData.annieHealth = GetSkillCvar( "sk_annie_health" );

	gSkillData.bigminerHealth = GetSkillCvar( "sk_bigminer_health" );
	gSkillData.bigminerDmgPunch = GetSkillCvar( "sk_bigminer_punch" );

	gSkillData.smallminerHealth = GetSkillCvar( "sk_smallminer_health" );
	gSkillData.smallminerDmgPick = GetSkillCvar( "sk_smallminer_pick" );

	gSkillData.cowboyHealth = GetSkillCvar( "sk_cowboy_health" );
	gSkillData.cowboyDmgPunch = GetSkillCvar( "sk_cowboy_punch" );
	gSkillData.cowboyDynamiteSpeed = GetSkillCvar( "sk_cowboy_dynamitespeed" );

	gSkillData.kaiewiHealth = GetSkillCvar( "sk_kaiewi_health" );
	gSkillData.kaiewiDmgPunch = GetSkillCvar( "sk_kaiewi_punch" );

	gSkillData.ramoneHealth = GetSkillCvar( "sk_ramone_health" );
	gSkillData.ramoneDmgPunch = GetSkillCvar( "sk_ramone_punch" );
	gSkillData.ramoneDynamiteSpeed = GetSkillCvar( "sk_ramone_dynamitespeed" );

	gSkillData.mexbanditHealth = GetSkillCvar( "sk_mexbandit_health" );
	gSkillData.mexbanditDmgKick = GetSkillCvar( "sk_mexbandit_kick" );
	gSkillData.mexbanditDynamiteSpeed = GetSkillCvar( "sk_mexbandit_dynamitespeed" );

	gSkillData.leechHealth = GetSkillCvar( "sk_leech_health" );
	gSkillData.leechDmgBite = GetSkillCvar( "sk_leech_dmg_bite" );

	gSkillData.colonelHealth = GetSkillCvar( "sk_colonel_health" );

	gSkillData.wtownaHealth = GetSkillCvar( "sk_wtowna_health" );

	gSkillData.wtownbHealth = GetSkillCvar( "sk_wtownb_health" );

	gSkillData.townmexHealth = GetSkillCvar( "sk_townmex_health" );

	gSkillData.masalaHealth = GetSkillCvar( "sk_masala_health" );

	gSkillData.nagatowHealth = GetSkillCvar( "sk_nagatow_health" );

	gSkillData.crispenHealth = GetSkillCvar( "sk_crispen_health" );

	gSkillData.daveHealth = GetSkillCvar( "sk_dave_health" );

	gSkillData.scorpionHealth = GetSkillCvar( "sk_scorpion_health" );
	gSkillData.scorpionDmgSting = GetSkillCvar( "sk_scorpion_dmg_sting" );

	gSkillData.snakeHealth = GetSkillCvar( "sk_snake_health" );
	gSkillData.snakeDmgBite = GetSkillCvar( "sk_snake_dmg_bite" );

	gSkillData.pumaDmgClaw = GetSkillCvar( "sk_puma_dmg_claw" );
	gSkillData.pumaDmgPounce = GetSkillCvar( "sk_puma_dmg_pounce" );
	gSkillData.pumaHealth = GetSkillCvar( "sk_puma_health" );

	gSkillData.bearDmgClaw = GetSkillCvar( "sk_bear_dmg_claw" );
	gSkillData.bearDmgPounce = GetSkillCvar( "sk_bear_dmg_pounce" );
	gSkillData.bearHealth = GetSkillCvar( "sk_bear_health" );

	gSkillData.chickenDmgPeck = GetSkillCvar( "sk_chicken_dmg_peck" );
	gSkillData.chickenHealth = GetSkillCvar( "sk_chicken_health" );

	gSkillData.horseDmgKick = GetSkillCvar( "sk_horse_dmg_kick" );
	gSkillData.horseHealth = GetSkillCvar( "sk_horse_health" );

	gSkillData.plrDmgKnife = GetSkillCvar( "sk_plr_knife" );
	gSkillData.plrDmgPick = GetSkillCvar( "sk_plr_pick" );
	gSkillData.plrDmgPistol = GetSkillCvar( "sk_plr_pistol_bullet" );
	gSkillData.plrDmgGattlingGun = GetSkillCvar( "sk_plr_gattlinggun_bullet" );
	gSkillData.plrDmgBuckshot = GetSkillCvar( "sk_plr_shotgun" );
	gSkillData.plrDmgBuffalo = GetSkillCvar( "sk_plr_buffalo" );
	gSkillData.plrDmgColts = GetSkillCvar( "sk_plr_colts_bullet" );
	gSkillData.plrDmgBow = GetSkillCvar( "sk_plr_bow_arrow" );
	gSkillData.plrDmgCannonBall = GetSkillCvar( "sk_plr_cannon_ball" );
	gSkillData.plrDmgBearTrap = GetSkillCvar( "sk_plr_beartrap" );
	gSkillData.plrDmgHandGrenade = GetSkillCvar( "sk_plr_dynamite" );
	gSkillData.plrDmgWinchester = GetSkillCvar( "sk_plr_winchester_bullet" );
	gSkillData.plrDmgRPG = GetSkillCvar( "sk_plr_rpg" );

	gSkillData.monDmg9MM = GetSkillCvar( "sk_pistol_bullet" );

	gSkillData.monDmg12MM = GetSkillCvar( "sk_shotgun_bullet" );

	gSkillData.monDmgMP5 = GetSkillCvar( "sk_gattlinggun_bullet" );

	gSkillData.monDmgWinchester = GetSkillCvar( "sk_winchester_bullet" );

	gSkillData.monDmgBow = GetSkillCvar( "sk_bow_arrow" );

	gSkillData.suitchargerCapacity = GetSkillCvar( "sk_suitcharger" );

	gSkillData.batteryCapacity = GetSkillCvar( "sk_battery" );

	gSkillData.healthchargerCapacity = GetSkillCvar( "sk_healthcharger" );

	gSkillData.healthkitCapacity = GetSkillCvar( "sk_healthkit" );

	gSkillData.herbsCapacity = GetSkillCvar( "sk_herbs" );

	gSkillData.elixerCapacity = GetSkillCvar( "sk_elixer" );

	gSkillData.colonelHeal = GetSkillCvar( "sk_colonel_heal" );

	gSkillData.wtownaHeal = GetSkillCvar( "sk_wtowna_heal" );

	gSkillData.wtownbHeal = GetSkillCvar( "sk_wtownb_heal" );

	gSkillData.townmexHeal = GetSkillCvar( "sk_townmex_heal" );

	gSkillData.masalaHeal = GetSkillCvar( "sk_masala_heal" );

	gSkillData.nagatowHeal = GetSkillCvar( "sk_nagatow_heal" );

	gSkillData.crispenHeal = GetSkillCvar( "sk_crispen_heal" );

	gSkillData.daveHeal = GetSkillCvar( "sk_dave_heal" );

	gSkillData.monHead = GetSkillCvar( "sk_monster_head" );
	gSkillData.monChest = GetSkillCvar( "sk_monster_chest" );
	gSkillData.monStomach = GetSkillCvar( "sk_monster_stomach" );
	gSkillData.monLeg = GetSkillCvar( "sk_monster_leg" );
	gSkillData.monArm = GetSkillCvar( "sk_monster_arm" );

	gSkillData.plrHead = GetSkillCvar( "sk_player_head" );
	gSkillData.plrChest = GetSkillCvar( "sk_player_chest" );
	gSkillData.plrStomach = GetSkillCvar( "sk_player_stomach" );
	gSkillData.plrLeg = GetSkillCvar( "sk_player_leg" );
	gSkillData.plrArm = GetSkillCvar( "sk_player_arm" );
}

//=========================================================
// instantiate the proper game rules object
//=========================================================

CGameRules *InstallGameRules( void )
{
	SERVER_COMMAND( "exec game.cfg\n" );
	SERVER_EXECUTE( );

	if ( !gpGlobals->deathmatch )
	{
		// generic half-life
		return new CHalfLifeRules;
	}
	else
	{
		if ( CVAR_GET_FLOAT( "mp_teamplay" ) > 0 )
		{
			// teamplay
			return new CHalfLifeTeamplay;
		}
		if ((int)gpGlobals->deathmatch == 1)
		{
			// vanilla deathmatch
			return new CHalfLifeMultiplay;
		}
		else
		{
			// vanilla deathmatch??
			return new CHalfLifeMultiplay;
		}
	}
}




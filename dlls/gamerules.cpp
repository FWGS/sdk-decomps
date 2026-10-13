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

	gSkillData.iSkillLevel = iSkill;

	gSkillData.monHealthPercentRandom = GetSkillCvar( "sk_percent_random_h" );
	gSkillData.humanDmgKick = GetSkillCvar( "sk_human_kick_d" );
	gSkillData.humanGrenadeSpeed = GetSkillCvar( "sk_gren_speed" );
	gSkillData.banditHealth = GetSkillCvar( "sk_bandit_h" );
	gSkillData.banditLimpHealth = GetSkillCvar( "sk_bandit_limp_h" );
	gSkillData.banditDmgMelee = GetSkillCvar( "sk_bandit_melee_d" );
	gSkillData.banditDmgBolt = GetSkillCvar( "sk_bandit_bolt_d" );
	gSkillData.banditDmgFast = GetSkillCvar( "sk_bandit_fast_d" );
	gSkillData.banditDmgMech = GetSkillCvar( "sk_bandit_mech_d" );
	gSkillData.demomanHealth = GetSkillCvar( "sk_demoman_h" );
	gSkillData.demomanLimpHealth = GetSkillCvar( "sk_demoman_limp_h" );
	gSkillData.demomanDmgRocket = GetSkillCvar( "sk_demoman_rocket_d" );
	gSkillData.demomanDmgShotgun = GetSkillCvar( "sk_demoman_sg_d" );
	gSkillData.gatorHealth = GetSkillCvar( "sk_gator_h" );
	gSkillData.gatorDmgBite = GetSkillCvar( "sk_gator_bite_d" );
	gSkillData.ouranoHealth = GetSkillCvar( "sk_ourano_h" );
	gSkillData.ouranoDmg = GetSkillCvar( "sk_ourano_d" );
	gSkillData.raptorHealth = GetSkillCvar( "sk_raptor_h" );
	gSkillData.raptorDmgBite = GetSkillCvar( "sk_raptor_bite_d" );
	gSkillData.raptorDmgJump = GetSkillCvar( "sk_raptor_jump_d" );
	gSkillData.smlscorpHealth = GetSkillCvar( "sk_smlscorp_h" );
	gSkillData.smlscorpDmgClaw = GetSkillCvar( "sk_smlscorp_claw_d" );
	gSkillData.smlscorpDmgSting = GetSkillCvar( "sk_smlscorp_sting_d" );
	gSkillData.lrgscorpHealth = GetSkillCvar( "sk_lrgscorp_h" );
	gSkillData.lrgscorpDmgClaw = GetSkillCvar( "sk_lrgscorp_claw_d" );
	gSkillData.lrgscorpDmgSting = GetSkillCvar( "sk_lrgscorp_sting_d" );
	gSkillData.rustbitHealth = GetSkillCvar( "sk_rustbit_h" );
	gSkillData.rustbitDmgShot = GetSkillCvar( "sk_rustbit_shot_d" );
	gSkillData.rustbitDmgClaw = GetSkillCvar( "sk_rustbit_claw_d" );
	gSkillData.rustbotHealth = GetSkillCvar( "sk_rustbot_h" );
	gSkillData.rustbotDmgMelee = GetSkillCvar( "sk_rustbot_melee_d" );
	gSkillData.rustbotDmgLong = GetSkillCvar( "sk_rustbot_long_d" );
	gSkillData.rustgunnerHealth = GetSkillCvar( "sk_rustgunner_h" );
	gSkillData.rustgunnerDmgRocket = GetSkillCvar( "sk_rustgunner_r_d" );
	gSkillData.rustgunnerDmgMiniRocket = GetSkillCvar( "sk_rustgunner_mr_d" );
	gSkillData.scientistHealth = GetSkillCvar( "sk_scientist_h" );
	gSkillData.tubeHealth = GetSkillCvar( "sk_tube_h" );
	gSkillData.beakHealth = GetSkillCvar( "sk_tube_h" );
	gSkillData.beakDmgBite = GetSkillCvar( "sk_beak_bite_d" );
	gSkillData.beakDmgScrape = GetSkillCvar( "sk_beak_scrape_d" );
	gSkillData.critterHealth = GetSkillCvar( "sk_critter_h" );
	gSkillData.critterDmgSpit = GetSkillCvar( "sk_critter_spit_d" );
	gSkillData.critterDmgMelee1 = GetSkillCvar( "sk_critter_melee1_d" );
	gSkillData.maggotDmg = GetSkillCvar( "sk_maggot_d" );
	gSkillData.maggotHealth = GetSkillCvar( "sk_maggot_h" );
	gSkillData.hatchetfishDmg = GetSkillCvar( "sk_hatchetfish_d" );
	gSkillData.hatchetfishHealth = GetSkillCvar( "sk_hatchetfish_h" );
	gSkillData.chopperHealth = GetSkillCvar( "sk_chopper_h" );
	gSkillData.xenomeHealth = GetSkillCvar( "sk_xenome_h" );
	gSkillData.xenomeDmgMelee = GetSkillCvar( "sk_xenomemelee_d" );
	gSkillData.xenomeDmgDart = GetSkillCvar( "sk_xenomedart_d" );
	gSkillData.sentryHealth = 1000;
	gSkillData.plrDmg9MM = GetSkillCvar( "sk_9mmAR_bullet" );
	gSkillData.plrDmgMP5 = GetSkillCvar( "sk_9mmAR_bullet" );
	gSkillData.plrDmgBuckshot = GetSkillCvar( "sk_buckshot" );
	gSkillData.plrDmgGaussShot = GetSkillCvar( "sk_gaussshot_d" );
	gSkillData.plrDmgGaussFast = GetSkillCvar( "sk_gaussfast_d" );
	gSkillData.plrDmgGaussSlow = GetSkillCvar( "sk_gaussslow_d" );
	gSkillData.plrDmgChemgun = GetSkillCvar( "sk_chemgun_d" );
	gSkillData.plrDmgOneHandPunch = GetSkillCvar( "sk_onehandpunch_d" );
	gSkillData.plrDmgTwoHandPunch = GetSkillCvar( "sk_twohandpunch_d" );
	gSkillData.plrDmgKnife = GetSkillCvar( "sk_knife1_d" );
	gSkillData.plrDmgHandGrenade = GetSkillCvar( "sk_hand_grenade_d" );
	gSkillData.plrDmgDmlRocket = GetSkillCvar( "sk_dml_rocket_d" );
	gSkillData.plrDmgMine = GetSkillCvar( "sk_mine_d" );
	gSkillData.dmlMaxAmmo = 6;
	gSkillData.dmlDefaultAmmo = 2;
	gSkillData.dmlClipAmmo = 6;
	gSkillData.chemMaxAmmo = 50;
	gSkillData.chemClipAmmo = 15;
	gSkillData.chemDefaultAmmo = 20;
	gSkillData.shotgunMaxAmmo = 90;
	gSkillData.shotgunClipAmmo = 16;
	gSkillData.shotgunDefaultAmmo = 16;
	gSkillData.minigunMaxAmmo = 225;
	gSkillData.minigunClipAmmo = 20;
	gSkillData.minigunDefaultAmmo = 20;
	gSkillData.gaussMaxAmmo = 150;
	gSkillData.gaussClipAmmo = 25;
	gSkillData.gaussDefaultAmmo = 16;
	gSkillData.beamgunMaxAmmo = 100;
	gSkillData.beamgunClipAmmo = 20;
	gSkillData.beamgunDefaultAmmo = 20;
	gSkillData.plrDmgDmlGrenade = 75;
	gSkillData.plrDmgM203Grenade = 20;
	gSkillData.suitchargerCapacity = GetSkillCvar( "sk_suitcharger" );
	gSkillData.batteryCapacity = GetSkillCvar( "sk_battery" );
	gSkillData.armorsuitCapacity = GetSkillCvar( "sk_armorsuit" );
	gSkillData.healthchargerCapacity = GetSkillCvar( "sk_healthcharger" );
	gSkillData.healthkitCapacity = GetSkillCvar( "sk_healthkit" );
	gSkillData.scientistHeal = GetSkillCvar( "sk_scientist_heal" );
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




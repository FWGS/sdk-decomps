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
// skill.h - skill level concerns
//=========================================================

struct skilldata_t
{

	int iSkillLevel; // game skill level

// Monster Health & Damage
	float hossHealth;
	float annieHealth;
	float bigminerHealth;
	float bigminerDmgPunch;
	float smallminerHealth;
	float smallminerDmgPick;
	float cowboyHealth;
	float cowboyDmgPunch;
	float cowboyDynamiteSpeed;
	float kaiewiHealth;
	float kaiewiDmgPunch;
	float ramoneHealth;
	float ramoneDmgPunch;
	float ramoneDynamiteSpeed;
	float mexbanditHealth;
	float mexbanditDmgKick;
	float mexbanditDynamiteSpeed;
	float leechHealth;
	float leechDmgBite;
	float colonelHealth;
	float wtownaHealth;
	float wtownbHealth;
	float townmexHealth;
	float masalaHealth;
	float nagatowHealth;
	float crispenHealth;
	float daveHealth;
	float scorpionHealth;
	float scorpionDmgSting;
	float snakeHealth;
	float snakeDmgBite;
	float pumaDmgClaw;
	float pumaDmgPounce;
	float pumaHealth;
	float bearDmgClaw;
	float bearHealth;
	float bearDmgPounce;
	float chickenHealth;
	float chickenDmgPeck;
	float horseHealth;
	float horseDmgKick;

// Player Weapons
	float plrDmgPick;
	float plrDmgKnife;
	float plrDmg9MM;	// never set (Half-Life's glock)
	float plrDmgPistol;
	float plrDmgGattlingGun;
	float plrDmgM203Grenade;	// never set here; the multiplayer rules set it
	float plrDmgBuckshot;	// sk_plr_shotgun
	float plrDmgBuffalo;
	float plrDmgColts;
	float plrDmgBow;
	float plrDmgCannonBall;
	float plrDmgBearTrap;
	float plrDmgRPG;
	float plrDmgHandGrenade;	// sk_plr_dynamite
	float plrDmgWinchester;

// weapons shared by monsters
	float monDmg9MM;	// sk_pistol_bullet
	float monDmgMP5;	// sk_gattlinggun_bullet
	float monDmg12MM;	// sk_shotgun_bullet
	float monDmgWinchester;
	float monDmgBow;

// health/suit charge
	float suitchargerCapacity;
	float batteryCapacity;
	float healthchargerCapacity;
	float healthkitCapacity;
	float herbsCapacity;
	float elixerCapacity;
	float colonelHeal;
	float wtownaHeal;
	float wtownbHeal;
	float townmexHeal;
	float masalaHeal;
	float nagatowHeal;
	float crispenHeal;
	float daveHeal;	// Dave heals with wtownaHeal

// monster damage adj
	float monHead;
	float monChest;
	float monStomach;
	float monLeg;
	float monArm;

// player damage adj
	float plrHead;
	float plrChest;
	float plrStomach;
	float plrLeg;
	float plrArm;
};

extern	DLL_GLOBAL	skilldata_t	gSkillData;
float GetSkillCvar( char *pName );

extern DLL_GLOBAL int		g_iSkillLevel;

#define SKILL_EASY		1
#define SKILL_MEDIUM	2
#define SKILL_HARD		3

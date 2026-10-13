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
	float monHealthPercentRandom;	// sk_percent_random_h
	float humanDmgKick;
	float humanGrenadeSpeed;

	float banditHealth;
	float banditLimpHealth;
	float banditDmgMelee;
	float banditDmgBolt;
	float banditDmgFast;
	float banditDmgMech;

	float demomanHealth;
	float demomanLimpHealth;
	float demomanDmgRocket;
	float demomanDmgShotgun;

	float gatorDmgBite;
	float gatorHealth;

	float ouranoDmg;
	float ouranoHealth;

	float raptorHealth;
	float raptorDmgBite;
	float raptorDmgJump;

	float smlscorpHealth;
	float smlscorpDmgClaw;
	float smlscorpDmgSting;

	float lrgscorpHealth;
	float lrgscorpDmgSting;
	float lrgscorpDmgClaw;

	float rustbitHealth;
	float rustbitDmgShot;
	float rustbitDmgClaw;

	float rustbotHealth;
	float rustbotDmgMelee;
	float rustbotDmgLong;
	float rustbotUnused;		// never set or read

	float rustgunnerHealth;
	float rustgunnerDmgRocket;
	float rustgunnerDmgMiniRocket;

	float scientistHealth;

	float beakHealth;			// RefreshSkillData reads sk_tube_h into it
	float beakDmgBite;
	float beakDmgScrape;

	float tubeHealth;

	float critterHealth;
	float critterDmgSpit;
	float critterDmgMelee1;

	float maggotDmg;
	float maggotHealth;

	float hatchetfishDmg;
	float hatchetfishHealth;

	float chopperHealth;

	float xenomeHealth;
	float xenomeDmgMelee;
	float xenomeDmgDart;

	float turretHealth;			// never set
	float miniturretHealth;		// never set
	float sentryHealth;

// Player weapon ammo
	float dmlMaxAmmo;
	float dmlDefaultAmmo;
	float dmlClipAmmo;

	float chemMaxAmmo;
	float chemClipAmmo;
	float chemDefaultAmmo;

	float shotgunMaxAmmo;
	float shotgunClipAmmo;
	float shotgunDefaultAmmo;

	float minigunMaxAmmo;
	float minigunClipAmmo;
	float minigunDefaultAmmo;

	float gaussMaxAmmo;
	float gaussClipAmmo;
	float gaussDefaultAmmo;

	float beamgunMaxAmmo;
	float beamgunClipAmmo;
	float beamgunDefaultAmmo;

	float plrDmgDmlGrenade;

// Player Weapons
	float plrDmg9MM;			// sk_9mmAR_bullet
	float plrDmg357;			// never set
	float plrDmgMP5;			// sk_9mmAR_bullet
	float plrDmgM203Grenade;
	float plrDmgBuckshot;
	float plrDmgGaussShot;
	float plrDmgGaussFast;
	float plrDmgGaussSlow;
	float plrDmgHandGrenade;
	float plrDmgChemgun;
	float plrDmgOneHandPunch;
	float plrDmgTwoHandPunch;
	float plrDmgKnife;
	float plrDmgMine;
	float plrDmgDmlRocket;

// health/suit charge
	float suitchargerCapacity;
	float batteryCapacity;
	float armorsuitCapacity;
	float healthchargerCapacity;
	float healthkitCapacity;
	float scientistHeal;

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

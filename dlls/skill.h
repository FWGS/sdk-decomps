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
	float	agruntHealth;
	float agruntDmgPunch;

	float apacheHealth;
	
	// POV: CBaseBarn's three families instead of barneyHealth (names ours)
	float normbarnsHealth;
	float hevbarnsHealth;
	float specbarnsHealth;

	float bigmommaHealthFactor;		// Multiply each node's health by this
	float bigmommaDmgSlash;			// melee attack damage
	float bigmommaDmgBlast;			// mortar attack damage
	float bigmommaRadiusBlast;		// mortar attack radius

	float bullsquidHealth;
	float bullsquidDmgBite;
	float bullsquidDmgWhip;
	float bullsquidDmgSpit;

	float gargantuaHealth;
	float gargantuaDmgSlash;
	float gargantuaDmgFire;
	float gargantuaDmgStomp;

	float hassassinHealth;

	float headcrabHealth;
	float headcrabDmgBite;

	float ratHealth;		// POV, name ours: sk_leech_*
	float ratDmgBite;

	float hgruntHealth;
	float hgruntDmgKick;
	float hgruntShotgunPellets;
	float hgruntGrenadeSpeed;

	float spforceHealth;		// POV, names ours: sk_hgrunt_*
	float spforceDmgKick;
	float spforceGrenadeSpeed;

	float houndeyeHealth;
	float houndeyeDmgBlast;

	float slaveHealth;
	float slaveDmgClaw;
	float slaveDmgClawrake;
	float slaveDmgZap;

	float sphereHealth;		// POV, names ours
	float sphereDmgZap;

	float pantherHealth;
	float pantherDmgClaw;
	float pantherDmgClawrake;
	float pantherDmgZap;

	float ichthyosaurHealth;
	float ichthyosaurDmgShake;

	float archerHealth;		// POV, names ours: half the ichthyosaur's
	float archerDmgShake;

	float leechHealth;
	float leechDmgBite;

	float controllerHealth;
	float controllerDmgZap;
	float controllerSpeedBall;
	float controllerDmgBall;

	float nihilanthHealth;
	float nihilanthZap;

	float scientistHealth;
	float workerHealth;		// POV, name ours: sk_scientist_health

	float snarkHealth;
	float snarkDmgBite;
	float snarkDmgPop;

	float toadHealth;		// POV, names ours
	float toadDmgBite;
	float toadDmgPop;

	float zombieHealth;
	float zombieDmgOneSlash;
	float zombieDmgBothSlash;

	float turretHealth;
	float miniturretHealth;
	float sentryHealth;


// Player Weapons
	float plrDmgMelee;		// POV, names ours: the alien slave's hands
	float plrDmgZapBeam;
	float plrChargeTime;
	float plrOldTime;
	float plrZapHeal;
	float plrDmgStukabat;		// POV, names ours
	float plrStukabatFlyTime;
	float plrDmgRPG;
	float plrDmgTripmine;
	
// weapons shared by monsters
	float monDmg9MM;
	float monDmgMP5;
	float plrDmgM203Grenade;	// POV: from sk_9mmAR_grenade
	float monDmg12MM;
	float monDmgHornet;

// health/suit charge
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

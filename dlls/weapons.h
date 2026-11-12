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
#ifndef WEAPONS_H
#define WEAPONS_H

// Counter-Strike: effects.h here, as SDK 2.2's weapons.h has it. Its intermediate code
// and declarations ahead of CBasePlayerItem show in the image.
#include "effects.h"


class CBasePlayer;
extern int gmsgWeapPickup;

void DeactivateSatchels( CBasePlayer *pOwner );

// Contact Grenade / Timed grenade / Satchel Charge
class CGrenade : public CBaseMonster
{
public:
	void Spawn( void );

	typedef enum { SATCHEL_DETONATE = 0, SATCHEL_RELEASE } SATCHELCODE;

	static CGrenade *ShootTimed( entvars_t *pevOwner, Vector vecStart, Vector vecVelocity, float time );
	static CGrenade *ShootTimed2( entvars_t *pevOwner, Vector vecStart, Vector vecVelocity, float time, int iTeam );	// Beta 4.0: the HE grenade (parameter names ours)
	static CGrenade *ShootContact( entvars_t *pevOwner, Vector vecStart, Vector vecVelocity );
	static CGrenade *ShootSatchelCharge( entvars_t *pevOwner, Vector vecStart, Vector vecVelocity );
	static void UseSatchelCharges( entvars_t *pevOwner, SATCHELCODE code );

	void Explode( Vector vecSrc, Vector vecAim );
	void Explode( TraceResult *pTrace, int bitsDamageType );
	void Explode2( TraceResult *pTrace, int bitsDamageType );	// Beta 4.0: the C4's explosion
	void Explode3( TraceResult *pTrace, int bitsDamageType );	// Beta 4.0: the HE grenade's explosion
	void EXPORT Smoke( void );
	void EXPORT Smoke2( void );
	void EXPORT Smoke3_A( void );
	void EXPORT Smoke3_B( void );
	void EXPORT Smoke3_C( void );

	void EXPORT BounceTouch( CBaseEntity *pOther );
	void EXPORT SlideTouch( CBaseEntity *pOther );
	void EXPORT ExplodeTouch( CBaseEntity *pOther );
	void EXPORT DangerSoundThink( void );
	void EXPORT PreDetonate( void );
	void EXPORT Detonate( void );
	void EXPORT Detonate2( void );
	void EXPORT Detonate3( void );
	void EXPORT DetonateUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void EXPORT TumbleThink( void );
	void EXPORT C4Think( void );
	void EXPORT C4Touch( CBaseEntity *pOther );

	virtual void BounceSound( void );
	virtual int	BloodColor( void ) { return DONT_BLEED; }
	virtual void Killed( entvars_t *pevAttacker, int iGib );
	virtual int ObjectCaps( void ) { return m_fPlantedC4 ? FCAP_CONTINUOUS_USE : 0; }	// Beta 5.0 (leak): Linux symbol; a planted C4 is defused by holding USE on it
	virtual void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );	// Beta 5.0 (leak): Linux symbol

	// Counter-Strike Beta 4.0 (names ours; offsets retail's; Beta 5.0 leak's in brackets where they moved)
	bool m_fStartDefuse;			// 0x118 a CT is defusing the planted C4 (Beta 4.0: C4Touch; 5.0: Use)
	bool m_fPlantedC4;				// 0x119 [new in 5.0] a planted C4 (ShootSatchelCharge): it can be used (ObjectCaps) and defused (Use)
	CBaseEntity *m_pDefuser;		// 0x11c the one defusing
	float m_flDefuseDone;			// 0x120 when the defusing is done (5 seconds after it starts with a defuse kit, 10 without)
	float m_flC4Blow;				// 0x124 when the C4 goes off
	float m_flBeepInterval;			// 0x128 Beta 5.0: the time between two changes of the beep; ShootSatchelCharge sets a quarter of mp_c4timer, each change makes it 10 % shorter
	float m_flNextBeep;				// 0x12c when C4Think beeps next (1.4 seconds after the last beep)
	float m_flNextBeepChange;		// 0x130 [new in 5.0] when C4Think takes the next of the five beeps (name ours)
	const char *m_pszBeepSound;		// 0x134 [new in 5.0] the beep it plays, weapons/c4_beep1.wav to c4_beep5.wav (name ours)
	float m_flBeepAttenuation;		// 0x138 [new in 5.0] the beep's attenuation, smaller with each beep (name ours)
	float m_flNextBlink;			// 0x13c [0x130] when C4Think shows the C4's glow sprite next (every 2 seconds)
	float m_flNextDefuseCheck;		// 0x140 [0x134] the defuser must use the C4 again before this time (half a second), or the defusing stops
	bool m_fC4Done;					// 0x144 [0x138] false only while a planted C4 ticks (ShootSatchelCharge); every other grenade has it set, and the C4 sets it when it goes off or is defused (CheckWinConditions waits for it)
	int m_iTeam;					// 0x148 [0x13c] the thrower's team: an HE grenade does not hurt his team mates (CBasePlayer::TakeDamage)
	int m_iBeepStep;				// 0x14c [new in 5.0] which beep C4Think takes next, 0 to 4 (name ours)
	edict_t *m_pentBombTarget;		// 0x150 [0x140] the bomb target the planter stood in (his own member); when the C4 goes off, it is used

	BOOL m_fRegisteredSound;// whether or not this grenade has issued its DANGER sound to the world sound list yet.
};


// constant items
#define ITEM_HEALTHKIT		1
#define ITEM_ANTIDOTE		2
#define ITEM_SECURITY		3
#define ITEM_BATTERY		4

#define WEAPON_NONE				0
// Beta 4.0: the new weapons take the ids of Half-Life's first four (retail's
// GetItemInfo values; names ours)
#define WEAPON_P228				1
#define	WEAPON_SHIELD			2
#define WEAPON_SCOUT			3
#define WEAPON_HEGRENADE		4
#define WEAPON_CHAINGUN			5
#define WEAPON_CROSSBOW			6
#define WEAPON_SHOTGUN			7
#define WEAPON_RPG				8
#define WEAPON_GAUSS			9
#define WEAPON_EGON				10
#define WEAPON_HORNETGUN		11
#define WEAPON_HANDGRENADE		12
#define WEAPON_TRIPMINE			13
#define	WEAPON_SATCHEL			15		// Beta 4.0: 15 (was 14)
#define	WEAPON_SNARK			15
// Counter-Strike's weapons (ids are retail's GetItemInfo values; the macro names are ours)
#define WEAPON_USP				16
#define WEAPON_GLOCK18			17
#define WEAPON_AWP				18
#define WEAPON_MP5N				19
#define WEAPON_M249				20
#define WEAPON_M3				21
#define WEAPON_M4A1				22
#define WEAPON_TMP				23
#define WEAPON_G3SG1			24
#define WEAPON_FLASHBANG		25
#define WEAPON_DEAGLE			26
#define WEAPON_SG552			27
#define WEAPON_AK47				28
#define WEAPON_KNIFE			29
#define WEAPON_P90				30
#define WEAPON_XM1014			5		// Beta 5.0 (leak): Half-Life's chaingun's id
#define WEAPON_C4				6	// Beta 5.0 (leak): the C4 takes the crossbow's id (CC4::Spawn, GetItemInfo)
#define WEAPON_MAC10			7	// Beta 6.0: the MAC-10 takes the shotgun's id (GetItemInfo)
#define WEAPON_AUG				8	// Beta 6.0: the AUG takes the RPG's id (GetItemInfo)

#define WEAPON_ALLWEAPONS		(~(1<<WEAPON_SUIT))

#define WEAPON_SUIT				31	// ?????

#define MAX_WEAPONS			32


#define MAX_NORMAL_BATTERY	100


// weapon weight factors (for auto-switching)   (-1 = noswitch)
#define CROWBAR_WEIGHT		0
#define GLOCK_WEIGHT		10
#define PYTHON_WEIGHT		15
#define MP5_WEIGHT			15
#define SHOTGUN_WEIGHT		15
#define CROSSBOW_WEIGHT		10
#define RPG_WEIGHT			20
#define GAUSS_WEIGHT		20
#define EGON_WEIGHT			20
#define HORNETGUN_WEIGHT	10
#define HANDGRENADE_WEIGHT	5
#define SNARK_WEIGHT		5
#define SATCHEL_WEIGHT		-10
#define TRIPMINE_WEIGHT		-10
// Counter-Strike (names ours)
#define USP_WEIGHT			5
#define GLOCK18_WEIGHT		5
#define AWP_WEIGHT			30
#define MP5N_WEIGHT			25
#define M249_WEIGHT			25
#define M3_WEIGHT			20
#define M4A1_WEIGHT			25
#define TMP_WEIGHT			25
#define G3SG1_WEIGHT		20
#define FLASHBANG_WEIGHT	1	// Beta 5.0 (the leak: 3)
#define DEAGLE_WEIGHT		7
#define SG552_WEIGHT		25
#define AK47_WEIGHT			25
#define P90_WEIGHT			26
#define KNIFE_WEIGHT		0	// Beta 5.0 (the leak: 2)
#define C4_WEIGHT			3		// Beta 5.0 (leak)
// Beta 4.0
#define P228_WEIGHT			5
#define SCOUT_WEIGHT		30
#define HEGRENADE_WEIGHT	2	// Beta 5.0 (the leak: 3)
// Beta 5.0 (leak)
#define XM1014_WEIGHT		20
#define MAC10_WEIGHT		25	// Beta 6.0
#define AUG_WEIGHT			25	// Beta 6.0
#define SHIELD_WEIGHT		2


// weapon clip/carry ammo capacities
#define URANIUM_MAX_CARRY		100
#define	_9MM_MAX_CARRY			100
#define _357_MAX_CARRY			36
#define BUCKSHOT_MAX_CARRY		125
#define BOLT_MAX_CARRY			50
#define ROCKET_MAX_CARRY		5
#define HANDGRENADE_MAX_CARRY	10
#define SATCHEL_MAX_CARRY		5
#define TRIPMINE_MAX_CARRY		5
#define SNARK_MAX_CARRY			15
#define HORNET_MAX_CARRY		8
#define M203_GRENADE_MAX_CARRY	10
// Counter-Strike: what each weapon's GetItemInfo gives as iMaxAmmo1 (names ours).
// The limit goes with the weapon, not with the ammo: "9mm" is 90 for the Glock 18,
// 120 for the MP5 Navy and the TMP; "556Nato" 200 for the M249, 90 for the M4A1.
#define USP_MAX_CARRY			48		// "45ACP"
#define GLOCK18_MAX_CARRY		90		// "9mm" (Beta 5.0 leak: 120; Beta 6.0: 90 again)
#define AWP_MAX_CARRY			30		// "338Magnum"
#define MP5N_MAX_CARRY			90		// "9mm" (Beta 6.0: 90, was 120)
#define M249_MAX_CARRY			200		// "556Nato"
#define M3_MAX_CARRY			32		// "buckshot" (Beta 6.0: 32, was 48)
#define M4A1_MAX_CARRY			90		// "556Nato" (Beta 5.0 leak: 200; Beta 6.0: 90 again)
#define TMP_MAX_CARRY			90		// "9mm" (Beta 6.0: 90, was 120)
#define G3SG1_MAX_CARRY			40		// "762Nato" (Beta 5.0 leak: 120; Beta 6.0: 40)
#define FLASHBANG_MAX_CARRY		2		// "Flashbang" (Beta 5.0 leak; was 3)
#define DEAGLE_MAX_CARRY		35		// "50AE"
#define SG552_MAX_CARRY			90		// "556Nato" (Beta 5.0 leak: 200; Beta 6.0: 90 again)
#define AK47_MAX_CARRY			90		// "762Nato" (Beta 5.0 leak: 120; Beta 6.0: 90 again)
#define P90_MAX_CARRY			100		// "57mm" (Beta 6.0: 100, was 150)
#define C4_MAX_CARRY			1		// "C4" (Beta 5.0 leak)
// Beta 4.0
#define P228_MAX_CARRY			52		// "357SIG"
#define SCOUT_MAX_CARRY			60		// "762Nato" (Beta 5.0 leak: 120; Beta 6.0: 60)
#define HEGRENADE_MAX_CARRY		1		// "HEGrenade" (Beta 5.0 leak; was 3)
#define XM1014_MAX_CARRY		32		// "buckshot" (Beta 5.0 leak: 48; Beta 6.0: 32)
#define MAC10_MAX_CARRY			90		// "45acp" (Beta 6.0; sic: lower case, an ammo type of its own beside the USP's "45ACP")
#define AUG_MAX_CARRY			90		// "556Nato" (Beta 6.0)

// the maximum amount of ammo each weapon's clip can hold
#define WEAPON_NOCLIP			-1

//#define CROWBAR_MAX_CLIP		WEAPON_NOCLIP
#define GLOCK_MAX_CLIP			17
#define PYTHON_MAX_CLIP			6
#define MP5_MAX_CLIP			50
#define MP5_DEFAULT_AMMO		25
#define SHOTGUN_MAX_CLIP		8
#define CROSSBOW_MAX_CLIP		5
#define RPG_MAX_CLIP			1
#define GAUSS_MAX_CLIP			WEAPON_NOCLIP
#define EGON_MAX_CLIP			WEAPON_NOCLIP
#define HORNETGUN_MAX_CLIP		WEAPON_NOCLIP
#define HANDGRENADE_MAX_CLIP	WEAPON_NOCLIP
#define SATCHEL_MAX_CLIP		WEAPON_NOCLIP
#define TRIPMINE_MAX_CLIP		WEAPON_NOCLIP
#define SNARK_MAX_CLIP			WEAPON_NOCLIP
// Counter-Strike (names ours)
#define USP_MAX_CLIP			12
#define GLOCK18_MAX_CLIP		20
#define AWP_MAX_CLIP			10
#define MP5N_MAX_CLIP			30
#define M249_MAX_CLIP			100
#define M3_MAX_CLIP				8
#define M4A1_MAX_CLIP			30
#define TMP_MAX_CLIP			30
#define G3SG1_MAX_CLIP			20
#define FLASHBANG_MAX_CLIP		WEAPON_NOCLIP
#define DEAGLE_MAX_CLIP			7
#define SG552_MAX_CLIP			30
#define AK47_MAX_CLIP			30
#define P90_MAX_CLIP			50
// Beta 4.0
#define P228_MAX_CLIP			13
#define SCOUT_MAX_CLIP			10
#define HEGRENADE_MAX_CLIP		WEAPON_NOCLIP
#define XM1014_MAX_CLIP			7		// Beta 5.0 (leak)
#define MAC10_MAX_CLIP			30		// Beta 6.0
#define AUG_MAX_CLIP			30		// Beta 6.0


// the default amount of ammo that comes with each gun when it spawns
#define GLOCK_DEFAULT_GIVE			17
#define PYTHON_DEFAULT_GIVE			6
#define MP5_DEFAULT_GIVE			25
#define MP5_DEFAULT_AMMO			25
#define MP5_M203_DEFAULT_GIVE		0
#define SHOTGUN_DEFAULT_GIVE		12
#define CROSSBOW_DEFAULT_GIVE		5
#define RPG_DEFAULT_GIVE			1
#define GAUSS_DEFAULT_GIVE			20
#define EGON_DEFAULT_GIVE			20
#define HANDGRENADE_DEFAULT_GIVE	5
#define SATCHEL_DEFAULT_GIVE		1
#define TRIPMINE_DEFAULT_GIVE		1
#define SNARK_DEFAULT_GIVE			5
#define HIVEHAND_DEFAULT_GIVE		8
// Counter-Strike: m_iDefaultAmmo as each Spawn sets it (names ours). The USP's is 17, the
// SDK Glock's GLOCK_DEFAULT_GIVE, with a magazine of 12.
#define USP_DEFAULT_GIVE			12
#define GLOCK18_DEFAULT_GIVE		20
#define AWP_DEFAULT_GIVE			10
#define MP5N_DEFAULT_GIVE			30
#define M249_DEFAULT_GIVE			100
#define M3_DEFAULT_GIVE				8
#define M4A1_DEFAULT_GIVE			30
#define TMP_DEFAULT_GIVE			30
#define G3SG1_DEFAULT_GIVE			20
#define FLASHBANG_DEFAULT_GIVE		1
#define DEAGLE_DEFAULT_GIVE			7
#define SG552_DEFAULT_GIVE			30
#define AK47_DEFAULT_GIVE			30
#define P90_DEFAULT_GIVE			50
#define C4_DEFAULT_GIVE				1	// Beta 5.0 (leak)
#define P228_DEFAULT_GIVE			13	// Beta 4.1 and 5.0: 13 (Beta 4.0 and 5.0 leak: 12, one short of its magazine)
#define SCOUT_DEFAULT_GIVE			10
#define HEGRENADE_DEFAULT_GIVE		1
#define XM1014_DEFAULT_GIVE			7	// Beta 5.0 (leak)
#define MAC10_DEFAULT_GIVE			30	// Beta 6.0
#define AUG_DEFAULT_GIVE			30	// Beta 6.0

// The amount of ammo given to a player by an ammo item.
#define AMMO_URANIUMBOX_GIVE	20
#define AMMO_GLOCKCLIP_GIVE		GLOCK_MAX_CLIP
#define AMMO_357BOX_GIVE		PYTHON_MAX_CLIP
#define AMMO_MP5CLIP_GIVE		MP5_MAX_CLIP
#define AMMO_CHAINBOX_GIVE		200
#define AMMO_M203BOX_GIVE		2
#define AMMO_BUCKSHOTBOX_GIVE	8		// Beta 4.0: 8 (SDK: 12), ammo.cpp's CBuckShotAmmo
#define AMMO_CROSSBOWCLIP_GIVE	CROSSBOW_MAX_CLIP
#define AMMO_RPGCLIP_GIVE		RPG_MAX_CLIP
#define AMMO_URANIUMBOX_GIVE	20
#define AMMO_SNARKBOX_GIVE		5
// Counter-Strike: the six ammo boxes of ammo.cpp, amount and limit as their AddAmmo
// passes them to GiveAmmo (names ours). ammo_762nato allows 90, the G3SG1 itself 60
// (Beta 4.1); Beta 5.0 (leak): 120 both.
#define AMMO_556NATOBOX_GIVE	30
#define AMMO_556NATOBOX_MAX		200
#define AMMO_762NATOBOX_GIVE	30
#define AMMO_762NATOBOX_MAX		90		// Beta 5.0 (leak): 120 (was 90); Beta 6.0: 90 again
#define AMMO_45ACPBOX_GIVE		12
#define AMMO_45ACPBOX_MAX		90		// Beta 6.0: 90 (was 48): "45acp", the MAC-10's (the USP's is "45ACP", sic)
#define AMMO_338MAGNUMBOX_GIVE	10
#define AMMO_338MAGNUMBOX_MAX	30
#define AMMO_50AEBOX_GIVE		7
#define AMMO_50AEBOX_MAX		35
#define AMMO_FLASHBANGBOX_GIVE	1
#define AMMO_FLASHBANGBOX_MAX	2		// Beta 5.0 (leak): 2 (was 3)
#define AMMO_57MMBOX_GIVE		50
#define AMMO_57MMBOX_MAX		100		// Beta 6.0: 100 (was 150)
// Beta 4.0: ammo_9mm (Glock 18, MP5 Navy, TMP), ammo_buckshot (the M3; the
// SDK's box, now ammo.cpp's), ammo_357sig (P228)
#define AMMO_9MMBOX_GIVE		30
#define AMMO_9MMBOX_MAX			90		// Beta 6.0: 90 (was 120)
#define AMMO_BUCKSHOTBOX_MAX	32		// Beta 6.0: 32 (was 48)
#define AMMO_357SIGBOX_GIVE		13
#define AMMO_357SIGBOX_MAX		52

// bullet types
typedef	enum
{
	BULLET_NONE = 0,
	BULLET_PLAYER_9MM, // glock
	BULLET_PLAYER_MP5, // mp5
	BULLET_PLAYER_357, // python
	BULLET_PLAYER_BUCKSHOT, // shotgun
	BULLET_PLAYER_CROWBAR, // crowbar swipe

	BULLET_MONSTER_9MM,
	BULLET_MONSTER_MP5,
	BULLET_MONSTER_12MM,

	// Counter-Strike (values 9 to 12 are retail's, the names ours)
	BULLET_PLAYER_45ACP,	// USP
	BULLET_PLAYER_338MAG,	// AWP
	BULLET_PLAYER_762MM,	// G3SG1
	BULLET_PLAYER_556MM,	// M249, M4A1
	BULLET_PLAYER_50AE,		// Desert Eagle (Beta 2.0; 13, as DEAGLEFire passes it)
	BULLET_PLAYER_57MM,		// P90 (Beta 3.0; 14, as P90Fire passes it; name ours)
	BULLET_PLAYER_357SIG,	// P228 (Beta 4.0; 15, as P228Fire passes it; name ours)
} Bullet;


#define ITEM_FLAG_SELECTONEMPTY		1
#define ITEM_FLAG_NOAUTORELOAD		2
#define ITEM_FLAG_NOAUTOSWITCHEMPTY	4
#define ITEM_FLAG_LIMITINWORLD		8
#define ITEM_FLAG_EXHAUSTIBLE		16 // A player can totally exhaust their ammo supply and lose this weapon

#define WEAPON_IS_ONTARGET 0x40

typedef struct
{
	int		iSlot;
	int		iPosition;
	const char	*pszAmmo1;	// ammo 1 type
	int		iMaxAmmo1;		// max ammo 1
	const char	*pszAmmo2;	// ammo 2 type
	int		iMaxAmmo2;		// max ammo 2
	const char	*pszName;
	int		iMaxClip;
	int		iId;
	int		iFlags;
	int		iWeight;// this value used to determine this weapon's importance in autoselection.
} ItemInfo;

typedef struct
{
	const char *pszName;
	int iId;
} AmmoInfo;

// sic: intermediate code Beta 5.0's weapons.h had ahead of CBasePlayerItem
// that compiled to nothing. It shows only
// through VC6's placement of header inline functions: CBasePlayerItem's
// accessors in weapons.obj (which writes its own .pch), CBaseMonster's and
// UTIL_MakeVectorsPrivate's in combat.obj and func_tank.obj (weapons.h is text
// after their .pch), CGameRules's in Beta 5.0's multiplay_gamerules.obj (which
// reuses the .pch it wrote). These 24 empty statements are 179 bytes (/Bk);
// 23 to 26 (174 to 189 bytes) serve all of them, 22 and 27 do not (the leak
// alone: 20 to 34). What the text was the image cannot say.
inline void sic_il_weapons( void )
{
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
}

// Items that the player has in their inventory that they can use
class CBasePlayerItem : public CBaseAnimating
{
public:
	virtual void SetObjectCollisionBox( void );

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	
	static	TYPEDESCRIPTION m_SaveData[];

	virtual int AddToPlayer( CBasePlayer *pPlayer );	// return TRUE if the item you want the item added to the player inventory
	virtual int AddDuplicate( CBasePlayerItem *pItem ) { return FALSE; }	// return TRUE if you want your duplicate removed from world
	void EXPORT DestroyItem( void );
	void EXPORT DefaultTouch( CBaseEntity *pOther );	// default weapon touch
	void EXPORT FallThink ( void );// when an item is first spawned, this think is run to determine when the object has hit the ground.
	void EXPORT Materialize( void );// make a weapon visible and tangible
	void EXPORT AttemptToMaterialize( void );  // the weapon desires to become visible and tangible, if the game rules allow for it
	CBaseEntity* Respawn ( void );// copy a weapon
	void FallInit( void );
	void CheckRespawn( void );
	virtual int GetItemInfo(ItemInfo *p) { return 0; };	// returns 0 if struct not filled out
	virtual BOOL CanDeploy( void ) { return TRUE; };
	virtual BOOL CanDrop( void ) { return TRUE; }		// Beta 5.0 (leak): Linux symbol; the knife and the grenades say no (slot 63)
	virtual BOOL Deploy( );								// returns is deploy was successful (Beta 5.0 leak: out of line in weapons.cpp, slot 64)
	virtual BOOL IsWeapon( void ) { return FALSE; }	// Beta 5.0 (leak): Linux symbol

	virtual BOOL CanHolster( void ) { return TRUE; };// can this weapon be put away right now?
	virtual void Holster( );
	virtual void UpdateItemInfo( void ) { return; };

	virtual void ItemPreFrame( void )	{ return; }		// called each frame by the player PreThink
	virtual void ItemPostFrame( void ) { return; }		// called each frame by the player PostThink

	virtual void Drop( void );
	virtual void Kill( void );
	virtual void AttachToPlayer ( CBasePlayer *pPlayer );

	virtual int PrimaryAmmoIndex() { return -1; };
	virtual int SecondaryAmmoIndex() { return -1; };

	virtual int UpdateClientData( CBasePlayer *pPlayer ) { return 0; }

	virtual float GetMaxSpeed( void ) { return 260; }	// Beta 5.0 (leak): Linux symbol; the speed the player runs at with it in his hands

	static ItemInfo ItemInfoArray[ MAX_WEAPONS ];
	static AmmoInfo AmmoInfoArray[ MAX_AMMO_SLOTS ];

	CBasePlayer	*m_pPlayer;
	CBasePlayerItem *m_pNext;
	int		m_iId;												// WEAPON_???

	virtual int iItemSlot( void ) { return 0; }			// return 0 to MAX_ITEMS_SLOTS, used in hud

	int			iItemPosition( void ) { return ItemInfoArray[ m_iId ].iPosition; }
	const char	*pszAmmo1( void )	{ return ItemInfoArray[ m_iId ].pszAmmo1; }
	int			iMaxAmmo1( void )	{ return ItemInfoArray[ m_iId ].iMaxAmmo1; }
	const char	*pszAmmo2( void )	{ return ItemInfoArray[ m_iId ].pszAmmo2; }
	int			iMaxAmmo2( void )	{ return ItemInfoArray[ m_iId ].iMaxAmmo2; }
	const char	*pszName( void )	{ return ItemInfoArray[ m_iId ].pszName; }
	int			iMaxClip( void )	{ return ItemInfoArray[ m_iId ].iMaxClip; }
	int			iWeight( void )		{ return ItemInfoArray[ m_iId ].iWeight; }
	int			iFlags( void )		{ return ItemInfoArray[ m_iId ].iFlags; }

	// int		m_iIdPrimary;										// Unique Id for primary ammo
	// int		m_iIdSecondary;										// Unique Id for secondary ammo
};


// inventory items that 
class CBasePlayerWeapon : public CBasePlayerItem
{
public:
	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	
	static	TYPEDESCRIPTION m_SaveData[];

	// generic weapon versions of CBasePlayerItem calls
	virtual int AddToPlayer( CBasePlayer *pPlayer );
	virtual int AddDuplicate( CBasePlayerItem *pItem );

	virtual int ExtractAmmo( CBasePlayerWeapon *pWeapon ); //{ return TRUE; };			// Return TRUE if you can add ammo to yourself when picked up
	virtual int ExtractClipAmmo( CBasePlayerWeapon *pWeapon );// { return TRUE; };			// Return TRUE if you can add ammo to yourself when picked up

	virtual int AddWeapon( void ) { ExtractAmmo( this ); return TRUE; };	// Return TRUE if you want to add yourself to the player

	// generic "shared" ammo handlers
	BOOL AddPrimaryAmmo( int iCount, char *szName, int iMaxClip, int iMaxCarry );
	BOOL AddSecondaryAmmo( int iCount, char *szName, int iMaxCarry );

	virtual void UpdateItemInfo( void ) {};	// updates HUD state

	int m_iPlayEmptySound;
	int m_fFireOnEmpty;		// True when the gun is empty and the player is still holding down the
							// attack key(s)
	virtual BOOL PlayEmptySound( void );
	virtual void ResetEmptySound( void );

	virtual void SendWeaponAnim( int iAnim );

	virtual BOOL CanDeploy( void );
	virtual BOOL IsUseable( void );
	BOOL DefaultDeploy( char *szViewModel, char *szWeaponModel, int iAnim, char *szAnimExt );
	void ReloadSound( void );		// Beta 5.0 (leak): Linux symbol; the others near the reloading player hear it
	int DefaultReload( int iClipSize, int iAnim, float fDelay );
	void EjectBrassLate( void );	// the shell of a bolt or pump action, m_pPlayer->m_flEjectBrass after the shot
	void FireRemaining( void );		// the second and third shot of a burst (Glock 18, M4A1)
	void KickBack( float up_base, float lateral_base, float up_modifier, float lateral_modifier, float up_max, float lateral_max, int direction_change );	// Beta 4.0's recoil (Linux symbol; parameter names ours)

	virtual void ItemPostFrame( void );	// called each frame by the player PostThink
	// called by CBasePlayerWeapons ItemPostFrame()
	virtual void PrimaryAttack( void ) { return; }				// do "+ATTACK"
	virtual void SecondaryAttack( void ) { return; }			// do "+ATTACK2"
	virtual void Reload( void ) { return; }						// do "+RELOAD"
	virtual void WeaponIdle( void ) { return; }					// called when no buttons pressed
	virtual int UpdateClientData( CBasePlayer *pPlayer );		// sends hud info to client dll, if things have changed
	virtual void RetireWeapon( void );
	virtual BOOL ShouldWeaponIdle( void ) {return FALSE; };
	virtual BOOL IsWeapon( void ) { return TRUE; }		// Beta 5.0 (leak): Linux symbol; declared here, after ShouldWeaponIdle, where ak47.obj (the first object to emit CBasePlayerWeapon's class-body inlines) has its body
	virtual void Holster( void );
	
	int	PrimaryAmmoIndex(); 
	int	SecondaryAmmoIndex(); 

	float	m_flNextPrimaryAttack;								// soonest time ItemPostFrame will call PrimaryAttack
	float	m_flNextSecondaryAttack;							// soonest time ItemPostFrame will call SecondaryAttack
	float	m_flTimeWeaponIdle;									// soonest time ItemPostFrame will call WeaponIdle
	int		m_iPrimaryAmmoType;									// "primary" ammo index into players m_rgAmmo[]
	int		m_iSecondaryAmmoType;								// "secondary" ammo index into players m_rgAmmo[]
	int		m_iClip;											// number of shots left in the primary weapon clip, -1 it not used
	int		m_iClientClip;										// the last version of m_iClip sent to hud dll
	int		m_iClientWeaponState;								// the last version of the weapon state sent to hud dll (is current weapon, is on target)
	int		m_fInReload;										// Are we in the middle of a reload;

	int		m_iDefaultAmmo;// how much ammo you get when you pick up this weapon as placed by a level designer.

	// Counter-Strike (names ours)
	int		m_iShellLate;										// model index of the shell EjectBrassLate and FireRemaining throw
	bool	m_fDelayFire;										// 0x7c new in Beta 3.0: set by a shot, cleared by Reload; ItemPostFrame lets m_iShotsFired run down once the trigger is released (Beta 5.0 leak: Beta 3.0's unused int before it is gone)
	int		m_iDirection;										// 0x80 new in Beta 4.0: the side KickBack pushes the view to (1 or 0); name ours
};


class CBasePlayerAmmo : public CBaseEntity
{
public:
	virtual void Spawn( void );
	void EXPORT DefaultTouch( CBaseEntity *pOther ); // default weapon touch
	virtual BOOL AddAmmo( CBaseEntity *pOther ) { return TRUE; };

	CBaseEntity* Respawn( void );
	void EXPORT Materialize( void );
};


extern DLL_GLOBAL	short	g_sModelIndexLaser;// holds the index for the laser beam
extern DLL_GLOBAL	const char *g_pModelNameLaser;

extern DLL_GLOBAL	short	g_sModelIndexLaserDot;// holds the index for the laser beam dot
extern DLL_GLOBAL	short	g_sModelIndexFireball;// holds the index for the fireball
extern DLL_GLOBAL	short	g_sModelIndexSmoke;// holds the index for the smoke cloud
extern DLL_GLOBAL	short	g_sModelIndexWExplosion;// holds the index for the underwater explosion
extern DLL_GLOBAL	short	g_sModelIndexBubbles;// holds the index for the bubbles model
extern DLL_GLOBAL	short	g_sModelIndexBloodDrop;// holds the sprite index for blood drops
extern DLL_GLOBAL	short	g_sModelIndexBloodSpray;// holds the sprite index for blood spray (bigger)
// Beta 4.0 (Linux symbols): sprites/smokepuff.spr, eexplo.spr, fexplo.spr, fexplo1.spr, radio.spr
extern DLL_GLOBAL	short	g_sModelIndexSmokePuff;
extern DLL_GLOBAL	short	g_sModelIndexFireball2;
extern DLL_GLOBAL	short	g_sModelIndexFireball3;
extern DLL_GLOBAL	short	g_sModelIndexFireball4;
extern DLL_GLOBAL	short	g_sModelIndexRadio;
// Beta 5.0 (leak) (Linux symbols; weapons.cpp): the ghosts' sprites and the planted C4's glow
extern DLL_GLOBAL	short	g_sModelIndexCTGhost;
extern DLL_GLOBAL	short	g_sModelIndexTGhost;
extern DLL_GLOBAL	short	g_sModelIndexC4Glow;

extern void ClearMultiDamage(void);
extern void ApplyMultiDamage(entvars_t* pevInflictor, entvars_t* pevAttacker );
extern void AddMultiDamage( entvars_t *pevInflictor, CBaseEntity *pEntity, float flDamage, int bitsDamageType);

extern void DecalGunshot( TraceResult *pTrace, int iBulletType, bool fShooterOnly, entvars_t *pevShooter, bool bHitMetal );	// Beta 6.0: bHitMetal (Linux symbol: a fifth bool; name ours)
extern void SpawnBlood(Vector vecSpot, int bloodColor, float flDamage);
extern int DamageDecal( CBaseEntity *pEntity, int bitsDamageType );
extern void RadiusDamage( Vector vecSrc, entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, float flRadius, int iClassIgnore, int bitsDamageType );
extern void RadiusFlash( Vector vecSrc, entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int iClassIgnore, int bitsDamageType );

typedef struct 
{
	CBaseEntity		*pEntity;
	float			amount;
	int				type;
} MULTIDAMAGE;

extern MULTIDAMAGE gMultiDamage;


#define LOUD_GUN_VOLUME			1000
#define NORMAL_GUN_VOLUME		600
#define QUIET_GUN_VOLUME		200

#define	BRIGHT_GUN_FLASH		512
#define NORMAL_GUN_FLASH		256
#define	DIM_GUN_FLASH			128

#define BIG_EXPLOSION_VOLUME	2048
#define NORMAL_EXPLOSION_VOLUME	1024
#define SMALL_EXPLOSION_VOLUME	512

#define	WEAPON_ACTIVITY_VOLUME	64

#define VECTOR_CONE_1DEGREES	Vector( 0.00873, 0.00873, 0.00873 )
#define VECTOR_CONE_2DEGREES	Vector( 0.01745, 0.01745, 0.01745 )
#define VECTOR_CONE_3DEGREES	Vector( 0.02618, 0.02618, 0.02618 )
#define VECTOR_CONE_4DEGREES	Vector( 0.03490, 0.03490, 0.03490 )
#define VECTOR_CONE_5DEGREES	Vector( 0.04362, 0.04362, 0.04362 )
#define VECTOR_CONE_6DEGREES	Vector( 0.05234, 0.05234, 0.05234 )
#define VECTOR_CONE_7DEGREES	Vector( 0.06105, 0.06105, 0.06105 )
#define VECTOR_CONE_8DEGREES	Vector( 0.06976, 0.06976, 0.06976 )
#define VECTOR_CONE_9DEGREES	Vector( 0.07846, 0.07846, 0.07846 )
#define VECTOR_CONE_10DEGREES	Vector( 0.08716, 0.08716, 0.08716 )
#define VECTOR_CONE_15DEGREES	Vector( 0.13053, 0.13053, 0.13053 )
#define VECTOR_CONE_20DEGREES	Vector( 0.17365, 0.17365, 0.17365 )

//=========================================================
// CWeaponBox - a single entity that can store weapons
// and ammo. 
//=========================================================
class CWeaponBox : public CBaseEntity
{
	void Precache( void );
	void Spawn( void );
	void Touch( CBaseEntity *pOther );
	void KeyValue( KeyValueData *pkvd );
	BOOL IsEmpty( void );
	int  GiveAmmo( int iCount, char *szName, int iMax, int *pIndex = NULL );
	void SetObjectCollisionBox( void );

public:
	void EXPORT Kill ( void );
	int		Save( CSave &save );
	int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	HasWeapon( CBasePlayerItem *pCheckItem );
	BOOL PackWeapon( CBasePlayerItem *pWeapon );
	BOOL PackAmmo( int iszName, int iCount );
	
	CBasePlayerItem	*m_rgpPlayerItems[MAX_ITEM_TYPES];// one slot for each 

	int m_rgiszAmmo[MAX_AMMO_SLOTS];// ammo names
	int	m_rgAmmo[MAX_AMMO_SLOTS];// ammo quantities

	int m_cAmmoTypes;// how many ammo types packed into this box (if packed by a level designer)
};


#endif // WEAPONS_H

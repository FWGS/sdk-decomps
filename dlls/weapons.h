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

	// Counter-Strike Beta 4.0 (names ours; offsets retail's)
	bool m_fStartDefuse;			// 0x118 a CT with a defuser is defusing the planted C4 (C4Touch)
	CBaseEntity *m_pDefuser;		// 0x11c the one defusing
	float m_flDefuseDone;			// 0x120 when the defusing is done (5 seconds after it starts)
	float m_flC4Blow;				// 0x124 when the C4 goes off
	float m_flBeepInterval;			// 0x128 between two beeps; shrinks by 1 % a tenth of a second
	float m_flNextBeep;				// 0x12c when C4Think beeps next
	bool m_fC4Done;					// 0x130 false only while a planted C4 ticks (ShootSatchelCharge); every other grenade has it set, and the C4 sets it when it goes off or is defused (CheckWinConditions waits for it)
	int m_iTeam;					// 0x134 the thrower's team: an HE grenade does not hurt his team mates (CBasePlayer::TakeDamage)

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
#define FLASHBANG_WEIGHT	3
#define DEAGLE_WEIGHT		7
#define SG552_WEIGHT		25
#define AK47_WEIGHT			25
#define P90_WEIGHT			26
#define KNIFE_WEIGHT		2
// Beta 4.0
#define P228_WEIGHT			5
#define SCOUT_WEIGHT		30
#define HEGRENADE_WEIGHT	3
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
#define GLOCK18_MAX_CARRY		90		// "9mm"
#define AWP_MAX_CARRY			30		// "338Magnum"
#define MP5N_MAX_CARRY			120		// "9mm"
#define M249_MAX_CARRY			200		// "556Nato"
#define M3_MAX_CARRY			48		// "buckshot"
#define M4A1_MAX_CARRY			90		// "556Nato"
#define TMP_MAX_CARRY			120		// "9mm"
#define G3SG1_MAX_CARRY			60		// "762Nato"
#define FLASHBANG_MAX_CARRY		3		// "Flashbang"
#define DEAGLE_MAX_CARRY		35		// "50AE"
#define SG552_MAX_CARRY			90		// "556Nato"
#define AK47_MAX_CARRY			90		// "762Nato"
#define P90_MAX_CARRY			150		// "57mm"
// Beta 4.0
#define P228_MAX_CARRY			52		// "357SIG"
#define SCOUT_MAX_CARRY			30		// "762Nato"
#define HEGRENADE_MAX_CARRY		3		// "HEGrenade"

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
// Beta 4.0 (the P228's is 12, one short of its magazine)
#define P228_DEFAULT_GIVE			12
#define SCOUT_DEFAULT_GIVE			10
#define HEGRENADE_DEFAULT_GIVE		1

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
// passes them to GiveAmmo (names ours). ammo_762nato allows 90, the G3SG1 itself 60.
#define AMMO_556NATOBOX_GIVE	30
#define AMMO_556NATOBOX_MAX		200
#define AMMO_762NATOBOX_GIVE	30
#define AMMO_762NATOBOX_MAX		90
#define AMMO_45ACPBOX_GIVE		12
#define AMMO_45ACPBOX_MAX		48
#define AMMO_338MAGNUMBOX_GIVE	10
#define AMMO_338MAGNUMBOX_MAX	30
#define AMMO_50AEBOX_GIVE		7
#define AMMO_50AEBOX_MAX		35
#define AMMO_FLASHBANGBOX_GIVE	1
#define AMMO_FLASHBANGBOX_MAX	3
#define AMMO_57MMBOX_GIVE		50
#define AMMO_57MMBOX_MAX		150
// Beta 4.0: ammo_9mm (Glock 18, MP5 Navy, TMP), ammo_buckshot (the M3; the
// SDK's box, now ammo.cpp's), ammo_357sig (P228)
#define AMMO_9MMBOX_GIVE		30
#define AMMO_9MMBOX_MAX			120
#define AMMO_BUCKSHOTBOX_MAX	48
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

// sic: a stand-in for declarations Beta 1.1's headers have beyond ours where the
// files that include weapons.h see them; what they were the image cannot say. VC5
// orders some operands by the number of declarations a file has seen:
// UTIL_TraceLine, UTIL_ClampVectorToBox, UTIL_MonstersInSphere, CRestore::ReadField,
// USENTENCEG_Pick, FShouldSwitchWeapon and GetNextBestWeapon need more than Beta
// 1.0's text gives, CFuncTank::TankTrace and the rest must keep theirs. effects.h,
// included above, is most of it (295 numbers; func_tank.cpp includes it itself,
// before weapons.h); this enum's tag and ten enumerators are the rest, 11 numbers.
// These counts here keep every object retail's: Beta 1.1 11 to 15 and 19 (of 0 to
// 32); Beta 2.0, with its cbase.h and multiplay_gamerules.cpp stand-ins, 6, 7, 10,
// 11, 28 and 30 (of 0 to 32); Beta 3.0, whose files compile as without /YX
// (debris.cpp), 11, 12, 16 and 17
// (of 0 to 24); Betas 4.0 and 4.1, 11 (the
// one count tried here; at the end of gamerules.h 4.0 takes 11 and 12, 4.1 11).
// Before effects.h was found this place held an enum of 303
// numbers and an unused inline function of 159 statement lines (Beta 3.0: 306).
enum sic_declcount_b11
{
	SIC_DECLCOUNT_B11_BASE, SIC_DECLCOUNT_B11_0, SIC_DECLCOUNT_B11_1, SIC_DECLCOUNT_B11_2, SIC_DECLCOUNT_B11_3, SIC_DECLCOUNT_B11_4, SIC_DECLCOUNT_B11_5, SIC_DECLCOUNT_B11_6,
	SIC_DECLCOUNT_B11_7, SIC_DECLCOUNT_B11_8
};

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
	virtual BOOL Deploy( )								// returns is deploy was successful
		 { return TRUE; };

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
	int		m_iUnused7C;										// 0x7c new in Beta 3.0; no function refers to it
	bool	m_fDelayFire;										// 0x80 new in Beta 3.0: set by a shot, cleared by Reload; ItemPostFrame lets m_iShotsFired run down once the trigger is released
	int		m_iDirection;										// 0x84 new in Beta 4.0: the side KickBack pushes the view to (1 or 0); name ours
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

extern void ClearMultiDamage(void);
extern void ApplyMultiDamage(entvars_t* pevInflictor, entvars_t* pevAttacker );
extern void AddMultiDamage( entvars_t *pevInflictor, CBaseEntity *pEntity, float flDamage, int bitsDamageType);

extern void DecalGunshot( TraceResult *pTrace, int iBulletType );
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

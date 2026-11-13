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
#ifndef WEAPONS_H
#define WEAPONS_H

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
	static CGrenade *ShootTimed2( entvars_t *pevOwner, Vector vecStart, Vector vecVelocity, float time, int iTeam );	// Counter-Strike (parameter names ours)
	static CGrenade *ShootContact( entvars_t *pevOwner, Vector vecStart, Vector vecVelocity );
	static CGrenade *ShootSmokeGrenade( entvars_t *pevOwner, Vector vecStart, Vector vecVelocity, float time, unsigned short usEvent );	// Counter-Strike
	static CGrenade *ShootSatchelCharge( entvars_t *pevOwner, Vector vecStart, Vector vecVelocity );
	static void UseSatchelCharges( entvars_t *pevOwner, SATCHELCODE code );

	void Explode( Vector vecSrc, Vector vecAim );
	void Explode( TraceResult *pTrace, int bitsDamageType );
	void EXPORT Smoke( void );

	void EXPORT BounceTouch( CBaseEntity *pOther );
	void EXPORT SlideTouch( CBaseEntity *pOther );
	void EXPORT ExplodeTouch( CBaseEntity *pOther );
	void EXPORT DangerSoundThink( void );
	void EXPORT PreDetonate( void );
	void EXPORT Detonate( void );
	void EXPORT DetonateUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void EXPORT TumbleThink( void );

	virtual void BounceSound( void );
	virtual int	BloodColor( void ) { return DONT_BLEED; }
	virtual void Killed( entvars_t *pevAttacker, int iGib );

	// Counter-Strike (names the Linux library's; parameter names ours)
	virtual int ObjectCaps( void ) { return m_bIsC4 ? FCAP_CONTINUOUS_USE : 0; }
	virtual void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void Explode2( TraceResult *pTrace, int bitsDamageType );
	void Explode3( TraceResult *pTrace, int bitsDamageType );
	void SG_Explode( TraceResult *pTrace, int bitsDamageType );
	void EXPORT Smoke2( void );
	void EXPORT Smoke3_A( void );
	void EXPORT Smoke3_B( void );
	void EXPORT Smoke3_C( void );
	void EXPORT SG_Smoke( void );
	void EXPORT Detonate2( void );
	void EXPORT Detonate3( void );
	void EXPORT SG_Detonate( void );
	void EXPORT SG_TumbleThink( void );
	void EXPORT C4Think( void );
	void EXPORT C4Touch( CBaseEntity *pOther );

	// Counter-Strike's members (0x174 to 0x1bc; the class is 0x1c0 bytes).  Names: Beta 6.1's
	// reconstruction's (ours) but m_bStartDefuse, m_bIsC4 and the three after m_pentBombTarget, which
	// are Counter-Strike 1.6's (debug information), placed by its order.
	bool m_bStartDefuse;			// 0x174 a CT is defusing the planted C4
	bool m_bIsC4;					// 0x175 a planted C4: it can be used (ObjectCaps) and defused (Use)
	CBaseEntity *m_pDefuser;		// 0x178 the one defusing
	float m_flDefuseDone;			// 0x17c when the defusing is done
	float m_flC4Blow;				// 0x180 when the C4 goes off
	float m_flBeepInterval;			// 0x184
	float m_flNextBeep;				// 0x188
	float m_flNextBeepChange;		// 0x18c
	const char *m_pszBeepSound;		// 0x190
	float m_flBeepAttenuation;		// 0x194
	float m_flNextBlink;			// 0x198
	float m_flNextDefuseCheck;		// 0x19c
	bool m_fC4Done;					// 0x1a0
	int m_iTeam;					// 0x1a4 the thrower's team
	int m_iBeepStep;				// 0x1a8
	edict_t *m_pentBombTarget;		// 0x1ac the bomb target the planter stood in
	int m_SGSmoke;					// 0x1b0 the smoke grenade's (Beta 6.5)
	int m_angle;					// 0x1b4
	unsigned short m_usEvent;		// 0x1b8 the smoke grenade's event (ShootSmokeGrenade's last parameter)
	bool m_bLightSmoke;				// 0x1ba another smoke grenade went off within 250 units: thinner smoke, fewer puffs
	bool m_bDetonated;				// 0x1bb the smoke grenade has gone off

	BOOL m_fRegisteredSound;// whether or not this grenade has issued its DANGER sound to the world sound list yet.
};


// constant items
#define ITEM_HEALTHKIT		1
#define ITEM_ANTIDOTE		2
#define ITEM_SECURITY		3
#define ITEM_BATTERY		4

// Counter-Strike's weapons.  The ids are the image's (each Spawn's m_iId, the client's
// HUD_WeaponsPostThink); the macro names are ours (Beta 1.0's where it had them).
#define WEAPON_NONE				0
#define WEAPON_P228				1
#define WEAPON_SCOUT			3
#define WEAPON_HEGRENADE		4
#define WEAPON_XM1014			5
#define WEAPON_C4				6
#define WEAPON_MAC10			7
#define WEAPON_AUG				8
#define WEAPON_SMOKEGRENADE		9
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
// Counter-Strike: each GetItemInfo's iWeight (names ours)
#define P228_WEIGHT			5
#define GLOCK18_WEIGHT		5
#define SCOUT_WEIGHT		30
#define HEGRENADE_WEIGHT	2
#define XM1014_WEIGHT		20
#define C4_WEIGHT			3
#define MAC10_WEIGHT		25
#define AUG_WEIGHT			25
#define SMOKEGRENADE_WEIGHT	1
#define USP_WEIGHT			5
#define AWP_WEIGHT			30
#define MP5N_WEIGHT			25
#define M249_WEIGHT			25
#define M3_WEIGHT			20
#define M4A1_WEIGHT			25
#define TMP_WEIGHT			25
#define G3SG1_WEIGHT		20
#define FLASHBANG_WEIGHT	1
#define DEAGLE_WEIGHT		7
#define SG552_WEIGHT		25
#define AK47_WEIGHT			25
#define KNIFE_WEIGHT		0
#define P90_WEIGHT			26


// weapon clip/carry ammo capacities
// Counter-Strike: what each weapon's GetItemInfo gives as iMaxAmmo1 (names ours, as Beta
// 1.0's: the limit goes with the weapon, not with the ammo)
#define P228_MAX_CARRY			52		// "357SIG"
#define GLOCK18_MAX_CARRY		90		// "9mm"
#define SCOUT_MAX_CARRY			60		// "762Nato"
#define HEGRENADE_MAX_CARRY		1
#define XM1014_MAX_CARRY		32		// "buckshot"
#define C4_MAX_CARRY			1
#define MAC10_MAX_CARRY			90		// "45acp"
#define AUG_MAX_CARRY			90		// "556Nato"
#define SMOKEGRENADE_MAX_CARRY	1
#define USP_MAX_CARRY			48		// "45acp"
#define AWP_MAX_CARRY			30		// "338Magnum"
#define MP5N_MAX_CARRY			120		// "9mm"
#define M249_MAX_CARRY			200		// "556NatoBox"
#define M3_MAX_CARRY			32		// "buckshot"
#define M4A1_MAX_CARRY			90		// "556Nato"
#define TMP_MAX_CARRY			120		// "9mm"
#define G3SG1_MAX_CARRY			40		// "762Nato"
#define FLASHBANG_MAX_CARRY		2
#define DEAGLE_MAX_CARRY		35		// "50AE"
#define SG552_MAX_CARRY			90		// "556Nato"
#define AK47_MAX_CARRY			90		// "762Nato"
#define P90_MAX_CARRY			100		// "57mm"

// the ammunition entities' numbers (ammo.cpp; names ours, Beta 6.1's)
#define AMMO_556NATO_GIVE		30
#define AMMO_556NATO_MAX		90
#define AMMO_556NATOBOX_GIVE	30
#define AMMO_556NATOBOX_MAX		200
#define AMMO_762NATOBOX_GIVE	30
#define AMMO_762NATOBOX_MAX		90
#define AMMO_45ACPBOX_GIVE		12
#define AMMO_45ACPBOX_MAX		90
#define AMMO_338MAGNUMBOX_GIVE	10
#define AMMO_338MAGNUMBOX_MAX	30
#define AMMO_50AEBOX_GIVE		7
#define AMMO_50AEBOX_MAX		35
#define AMMO_FLASHBANGBOX_GIVE	1
#define AMMO_FLASHBANGBOX_MAX	2
#define AMMO_57MMBOX_GIVE		50
#define AMMO_57MMBOX_MAX		100

#define AMMO_BUCKSHOTBOX_GIVE	8
#define AMMO_9MMBOX_GIVE		30
#define AMMO_9MMBOX_MAX			120
#define AMMO_BUCKSHOTBOX_MAX	32
#define AMMO_357SIGBOX_GIVE		13
#define AMMO_357SIGBOX_MAX		52

// the maximum amount of ammo each weapon's clip can hold
#define WEAPON_NOCLIP			-1

// Counter-Strike (names ours)
#define P228_MAX_CLIP			13
#define GLOCK18_MAX_CLIP		20
#define SCOUT_MAX_CLIP			10
#define XM1014_MAX_CLIP			7
#define MAC10_MAX_CLIP			30
#define AUG_MAX_CLIP			30
#define USP_MAX_CLIP			12
#define AWP_MAX_CLIP			10
#define MP5N_MAX_CLIP			30
#define M249_MAX_CLIP			100
#define M3_MAX_CLIP				8
#define M4A1_MAX_CLIP			30
#define TMP_MAX_CLIP			30
#define G3SG1_MAX_CLIP			20
#define DEAGLE_MAX_CLIP			7
#define SG552_MAX_CLIP			30
#define AK47_MAX_CLIP			30
#define P90_MAX_CLIP			50


// the default amount of ammo that comes with each gun when it spawns
// Counter-Strike: m_iDefaultAmmo as each Spawn sets it (names ours)
#define P228_DEFAULT_GIVE			13
#define GLOCK18_DEFAULT_GIVE		20
#define SCOUT_DEFAULT_GIVE			10
#define HEGRENADE_DEFAULT_GIVE		1
#define XM1014_DEFAULT_GIVE			7
#define C4_DEFAULT_GIVE				1
#define MAC10_DEFAULT_GIVE			30
#define AUG_DEFAULT_GIVE			30
#define SMOKEGRENADE_DEFAULT_GIVE	1
#define USP_DEFAULT_GIVE			12
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

	// Counter-Strike (values 9 to 12 are Beta 1.0's, the names ours)
	BULLET_PLAYER_45ACP,	// USP
	BULLET_PLAYER_338MAG,	// AWP
	BULLET_PLAYER_762MM,	// G3SG1, AK47
	BULLET_PLAYER_556MM,	// M249, M4A1
	BULLET_PLAYER_50AE,		// Desert Eagle (13; name ours)
	BULLET_PLAYER_57MM,		// P90 (14; name ours)
	BULLET_PLAYER_357SIG,	// P228 (15; name ours)
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
	virtual BOOL CanDrop( void ) { return TRUE; }		// Counter-Strike
	virtual BOOL Deploy( void );						// returns is deploy was successful
	virtual BOOL IsWeapon( void ) { return FALSE; }	// Counter-Strike

	virtual BOOL CanHolster( void ) { return TRUE; };// can this weapon be put away right now?
	virtual void Holster( int skiplocal = 0 );
	virtual void UpdateItemInfo( void ) { return; };

	virtual void ItemPreFrame( void )	{ return; }		// called each frame by the player PreThink
	virtual void ItemPostFrame( void ) { return; }		// called each frame by the player PostThink

	virtual void Drop( void );
	virtual void Kill( void );
	virtual void AttachToPlayer ( CBasePlayer *pPlayer );

	virtual int PrimaryAmmoIndex() { return -1; };
	virtual int SecondaryAmmoIndex() { return -1; };

	virtual int UpdateClientData( CBasePlayer *pPlayer ) { return 0; }

	virtual CBasePlayerItem *GetWeaponPtr( void ) { return NULL; };
	virtual float GetMaxSpeed( void ) { return 260; }	// Counter-Strike

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
// m_iWeaponState's bits (names ours)
#define WPNSTATE_USP_SILENCED		(1<<0)
#define WPNSTATE_GLOCK18_BURST_MODE	(1<<1)
#define WPNSTATE_M4A1_SILENCED		(1<<2)

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

	virtual void SendWeaponAnim( int iAnim, int skiplocal = 0 );  // skiplocal is 1 if client is predicting weapon animations

	virtual BOOL CanDeploy( void );
	virtual BOOL IsUseable( void );
	BOOL DefaultDeploy( char *szViewModel, char *szWeaponModel, int iAnim, char *szAnimExt, int skiplocal = 0 );
	int DefaultReload( int iClipSize, int iAnim, float fDelay );

	// Counter-Strike
	void ReloadSound( void );
	void FireRemaining( void );
	void EjectBrassLate( void );
	void KickBack( float up_base, float lateral_base, float up_modifier, float lateral_modifier, float up_max, float lateral_max, int direction_change );	// parameter names ours

	virtual void ItemPostFrame( void );	// called each frame by the player PostThink
	// called by CBasePlayerWeapons ItemPostFrame()
	virtual void PrimaryAttack( void ) { return; }				// do "+ATTACK"
	virtual void SecondaryAttack( void ) { return; }			// do "+ATTACK2"
	virtual void Reload( void ) { return; }						// do "+RELOAD"
	virtual void WeaponIdle( void ) { return; }					// called when no buttons pressed
	virtual int UpdateClientData( CBasePlayer *pPlayer );		// sends hud info to client dll, if things have changed
	virtual void RetireWeapon( void );
	virtual BOOL ShouldWeaponIdle( void ) {return FALSE; };
	virtual void Holster( int skiplocal = 0 );
	virtual BOOL UseDecrement( void ) { return FALSE; };
	virtual BOOL IsWeapon( void ) { return TRUE; }		// Counter-Strike

	int	PrimaryAmmoIndex(); 
	int	SecondaryAmmoIndex(); 

	void PrintState( void );

	virtual CBasePlayerItem *GetWeaponPtr( void ) { return (CBasePlayerItem *)this; };

	float	m_flNextPrimaryAttack;								// soonest time ItemPostFrame will call PrimaryAttack
	float	m_flNextSecondaryAttack;							// soonest time ItemPostFrame will call SecondaryAttack
	float	m_flTimeWeaponIdle;									// soonest time ItemPostFrame will call WeaponIdle
	int		m_iPrimaryAmmoType;									// "primary" ammo index into players m_rgAmmo[]
	int		m_iSecondaryAmmoType;								// "secondary" ammo index into players m_rgAmmo[]
	int		m_iClip;											// number of shots left in the primary weapon clip, -1 it not used
	int		m_iClientClip;										// the last version of m_iClip sent to hud dll
	int		m_iClientWeaponState;								// the last version of the weapon state sent to hud dll (is current weapon, is on target)
	int		m_fInReload;										// Are we in the middle of a reload;
	int		m_fInSpecialReload;									// Counter-Strike: the shotguns' reload in steps (weapon_data_t's name)

	int		m_iDefaultAmmo;// how much ammo you get when you pick up this weapon as placed by a level designer.

	// Counter-Strike (names ours unless said)
	int		m_iShellLate;		// 0xd8 the shell EjectBrassLate throws (Beta 1.0's name)
	bool	m_bDelayFire;		// 0xdc set by every shot; the automatic weapons fire again only once the button was let go
	int		m_iDirection;		// 0xe0 KickBack's side, flipped at random
	bool	m_bSecondarySilencerOn;	// 0xe4 Counter-Strike 1.6's name (debug information), by its place between m_iDirection and m_flAccuracy: no function of Beta 6.5's server or client touches it
	float	m_flAccuracy;		// 0xe8 (Beta 1.0's name)
	float	m_flLastFire;		// 0xec the pistols' time of the last shot
	int		m_iShotsFired;		// 0xf0 (Beta 1.0's name)
	Vector	m_vecAiming;		// 0xf4 the aim the server saw (UpdateClientData); the shot events carry it
	int		m_iViewModel;		// 0x100 pev->viewmodel as DefaultDeploy set it
	float	m_flNextBurstShot;	// 0x104 (Beta 1.0's name)
	int		m_iBurstShotsFired;	// 0x108 (Beta 1.0's name)
	int		m_iWeaponState;		// 0x10c silencer, burst (weapon_data_t's name): the WPNSTATE_ bits
	float	m_flNextReload;		// 0x110 (weapon_data_t's name)
	float	m_flDecreaseShotsFired;	// 0x114
	unsigned short m_usFireGlock18;	// 0x118 FireRemaining's event
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
extern DLL_GLOBAL	short	g_sModelIndexSmokePuff;
extern DLL_GLOBAL	short	g_sModelIndexFireball2;
extern DLL_GLOBAL	short	g_sModelIndexFireball3;
extern DLL_GLOBAL	short	g_sModelIndexFireball4;
extern DLL_GLOBAL	short	g_sModelIndexRadio;

extern DLL_GLOBAL	short	g_sModelIndexCTGhost;
extern DLL_GLOBAL	short	g_sModelIndexTGhost;
extern DLL_GLOBAL	short	g_sModelIndexC4Glow;

extern void ClearMultiDamage(void);
extern void ApplyMultiDamage(entvars_t* pevInflictor, entvars_t* pevAttacker );
extern void AddMultiDamage( entvars_t *pevInflictor, CBaseEntity *pEntity, float flDamage, int bitsDamageType);

extern void DecalGunshot( TraceResult *pTrace, int iBulletType, bool ClientOnly, entvars_t *pShooter, bool bHitMetal );	// Counter-Strike: three more (names ours)
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

// Counter-Strike's weapons, declared here as SDK 2.0 declares its glock so that the client
// can hold one of each (hl_weapons.cpp).  Members: the image's offsets; names ours unless
// they are the SDK's or Beta 1.0's.

class CUSP : public CBasePlayerWeapon
{
public:
	void Spawn( void );
	void Precache( void );
	int iItemSlot( void ) { return 2; }
	int GetItemInfo(ItemInfo *p);

	void PrimaryAttack( void );
	void SecondaryAttack( void );
	void USPFire( float flSpread, float flCycleTime, BOOL fUseAutoAim );
	BOOL Deploy( void );
	void Reload( void );
	void WeaponIdle( void );

	float GetMaxSpeed( void ) { return 250; }
	virtual BOOL UseDecrement( void ) { return TRUE; }

	int m_iShell;
	unsigned short m_usFireUSP;	// name ours
};


class CMP5N : public CBasePlayerWeapon
{
public:
	void Spawn( void );
	void Precache( void );
	int iItemSlot( void ) { return 1; }
	int GetItemInfo(ItemInfo *p);

	void PrimaryAttack( void );
	void MP5NFire( float flSpread, float flCycleTime, BOOL fUseAutoAim );
	BOOL Deploy( void );
	void Reload( void );
	void WeaponIdle( void );

	float GetMaxSpeed( void ) { return 250; }
	virtual BOOL UseDecrement( void ) { return TRUE; }

	int m_iShell;
	int m_iShellOn;		// name ours: Deploy sets it to 1, nothing reads it
	unsigned short m_usFireMP5N;	// name ours
};


class CSG552 : public CBasePlayerWeapon
{
public:
	void Spawn( void );
	void Precache( void );
	int iItemSlot( void ) { return 1; }
	int GetItemInfo(ItemInfo *p);

	void PrimaryAttack( void );
	void SecondaryAttack( void );
	void SG552Fire( float flSpread, float flCycleTime, BOOL fUseAutoAim );
	BOOL Deploy( void );
	void Reload( void );
	void WeaponIdle( void );

	float GetMaxSpeed( void );
	virtual BOOL UseDecrement( void ) { return TRUE; }

	int m_iShell;
	int m_iShellOn;		// name ours: Deploy sets it to 1, nothing reads it
	unsigned short m_usFireSG552;	// name ours
};


class CAK47 : public CBasePlayerWeapon
{
public:
	void Spawn( void );
	void Precache( void );
	int iItemSlot( void ) { return 1; }
	int GetItemInfo(ItemInfo *p);

	void PrimaryAttack( void );
	void SecondaryAttack( void );
	void AK47Fire( float flSpread, float flCycleTime, BOOL fUseAutoAim );
	BOOL Deploy( void );
	void Reload( void );
	void WeaponIdle( void );

	float GetMaxSpeed( void ) { return 221; }
	virtual BOOL UseDecrement( void ) { return TRUE; }

	int m_iShell;
	int m_iShellOn;		// name ours: Deploy sets it to 1, nothing reads it
	unsigned short m_usFireAK47;	// name ours
};


class CAUG : public CBasePlayerWeapon
{
public:
	void Spawn( void );
	void Precache( void );
	int iItemSlot( void ) { return 1; }
	int GetItemInfo(ItemInfo *p);

	void PrimaryAttack( void );
	void SecondaryAttack( void );
	void AUGFire( float flSpread, float flCycleTime, BOOL fUseAutoAim );
	BOOL Deploy( void );
	void Reload( void );
	void WeaponIdle( void );

	float GetMaxSpeed( void ) { return 240; }
	virtual BOOL UseDecrement( void ) { return TRUE; }

	int m_iShell;
	int m_iShellOn;		// name ours: Deploy sets it to 1, nothing reads it
	unsigned short m_usFireAUG;	// name ours
};


class CAWP : public CBasePlayerWeapon
{
public:
	void Spawn( void );
	void Precache( void );
	int iItemSlot( void ) { return 1; }
	int GetItemInfo(ItemInfo *p);

	void PrimaryAttack( void );
	void SecondaryAttack( void );
	void AWPFire( float flSpread, float flCycleTime, BOOL fUseAutoAim );
	BOOL Deploy( void );
	void Reload( void );
	void WeaponIdle( void );

	float GetMaxSpeed( void );
	virtual BOOL UseDecrement( void ) { return TRUE; }

	int m_iShell;
	unsigned short m_usFireAWP;	// name ours
};


class CC4 : public CBasePlayerWeapon
{
public:
	void Spawn( void );
	void Precache( void );
	int iItemSlot( void ) { return 5; }
	int GetItemInfo(ItemInfo *p);

	void PrimaryAttack( void );
	BOOL Deploy( void );
	void Holster( int skiplocal );
	void WeaponIdle( void );

	float GetMaxSpeed( void ) { return 250; }
	virtual BOOL UseDecrement( void ) { return TRUE; }

	bool m_bStartedArming;			// names ours
	bool m_bBombPlacedAnimation;
	float m_fArmedTime;
};


class CDEAGLE : public CBasePlayerWeapon
{
public:
	void Spawn( void );
	void Precache( void );
	int iItemSlot( void ) { return 2; }
	int GetItemInfo(ItemInfo *p);

	void PrimaryAttack( void );
	void DEAGLEFire( float flSpread, float flCycleTime, BOOL fUseAutoAim );
	BOOL Deploy( void );
	void Reload( void );
	void WeaponIdle( void );

	float GetMaxSpeed( void ) { return 250; }
	virtual BOOL UseDecrement( void ) { return TRUE; }

	int m_iShell;
	unsigned short m_usFireDEAGLE;	// name ours
};


class CFlashbang : public CBasePlayerWeapon
{
public:
	void Spawn( void );
	void Precache( void );
	int iItemSlot( void ) { return 4; }
	int GetItemInfo(ItemInfo *p);

	BOOL CanDeploy( void );
	BOOL Deploy( void );
	BOOL CanHolster( void );
	void Holster( int skiplocal );
	void PrimaryAttack( void );
	void WeaponIdle( void );

	float GetMaxSpeed( void ) { return 250; }
	BOOL CanDrop( void ) { return FALSE; }
	virtual BOOL UseDecrement( void ) { return TRUE; }

	float m_flStartThrow;
	float m_flReleaseThrow;
};


class CG3SG1 : public CBasePlayerWeapon
{
public:
	void Spawn( void );
	void Precache( void );
	int iItemSlot( void ) { return 1; }
	int GetItemInfo(ItemInfo *p);

	void PrimaryAttack( void );
	void SecondaryAttack( void );
	void G3SG1Fire( float flSpread, float flCycleTime, BOOL fUseAutoAim );
	BOOL Deploy( void );
	void Reload( void );
	void WeaponIdle( void );

	float GetMaxSpeed( void );
	virtual BOOL UseDecrement( void ) { return TRUE; }

	int m_iShell;
	unsigned short m_usFireG3SG1;	// name ours
};


class CGLOCK18 : public CBasePlayerWeapon
{
public:
	void Spawn( void );
	void Precache( void );
	int iItemSlot( void ) { return 2; }
	int GetItemInfo(ItemInfo *p);

	void PrimaryAttack( void );
	void SecondaryAttack( void );
	void GLOCK18Fire( float flSpread, float flCycleTime, BOOL fUseAutoAim );
	BOOL Deploy( void );
	void Reload( void );
	void WeaponIdle( void );

	float GetMaxSpeed( void ) { return 250; }
	virtual BOOL UseDecrement( void ) { return TRUE; }

	int m_iShell;
	bool m_bBurstFire;		// name ours: Deploy clears it, nothing reads it
};


class CHEGrenade : public CBasePlayerWeapon
{
public:
	void Spawn( void );
	void Precache( void );
	int iItemSlot( void ) { return 4; }
	int GetItemInfo(ItemInfo *p);

	BOOL CanDeploy( void );
	BOOL Deploy( void );
	BOOL CanHolster( void );
	void Holster( int skiplocal );
	void PrimaryAttack( void );
	void WeaponIdle( void );

	float GetMaxSpeed( void ) { return 260; }
	BOOL CanDrop( void ) { return FALSE; }
	virtual BOOL UseDecrement( void ) { return TRUE; }

	float m_flStartThrow;
	float m_flReleaseThrow;
};


class CKnife : public CBasePlayerWeapon
{
public:
	void Spawn( void );
	void Precache( void );
	int iItemSlot( void ) { return 3; }
	int GetItemInfo(ItemInfo *p);

	void PrimaryAttack( void );
	int Swing( int fFirst );
	void EXPORT SwingAgain( void );
	void EXPORT Smack( void );
	BOOL Deploy( void );
	void Holster( int skiplocal );

	float GetMaxSpeed( void ) { return 250; }
	BOOL CanDrop( void ) { return FALSE; }
	virtual BOOL UseDecrement( void ) { return TRUE; }

	int m_iSwing;
	TraceResult m_trHit;
};


class CM249 : public CBasePlayerWeapon
{
public:
	void Spawn( void );
	void Precache( void );
	int iItemSlot( void ) { return 1; }
	int GetItemInfo(ItemInfo *p);

	void PrimaryAttack( void );
	void M249Fire( float flSpread, float flCycleTime, BOOL fUseAutoAim );
	BOOL Deploy( void );
	void Reload( void );
	void WeaponIdle( void );

	float GetMaxSpeed( void ) { return 220; }
	virtual BOOL UseDecrement( void ) { return TRUE; }

	int m_iShell;
	int m_iShellOn;		// name ours: Deploy sets it to 1, nothing reads it
	unsigned short m_usFireM249;	// name ours
};


class CM3 : public CBasePlayerWeapon
{
public:
	void Spawn( void );
	void Precache( void );
	int iItemSlot( void ) { return 1; }
	int GetItemInfo(ItemInfo *p);

	void PrimaryAttack( void );
	BOOL Deploy( void );
	void Reload( void );
	void WeaponIdle( void );

	float GetMaxSpeed( void ) { return 230; }
	virtual BOOL UseDecrement( void ) { return TRUE; }

	int m_iShell;
	float m_flPumpTime;
	unsigned short m_usFireM3;	// name ours
};


class CM4A1 : public CBasePlayerWeapon
{
public:
	void Spawn( void );
	void Precache( void );
	int iItemSlot( void ) { return 1; }
	int GetItemInfo(ItemInfo *p);

	void PrimaryAttack( void );
	void SecondaryAttack( void );
	void M4A1Fire( float flSpread, float flCycleTime, BOOL fUseAutoAim );
	BOOL Deploy( void );
	void Reload( void );
	void WeaponIdle( void );

	float GetMaxSpeed( void );
	virtual BOOL UseDecrement( void ) { return TRUE; }

	int m_iShell;
	int m_iShellOn;		// name ours: Deploy sets it to 1, nothing reads it
	unsigned short m_usFireM4A1;	// name ours
};


class CMAC10 : public CBasePlayerWeapon
{
public:
	void Spawn( void );
	void Precache( void );
	int iItemSlot( void ) { return 1; }
	int GetItemInfo(ItemInfo *p);

	void PrimaryAttack( void );
	void MAC10Fire( float flSpread, float flCycleTime, BOOL fUseAutoAim );
	BOOL Deploy( void );
	void Reload( void );
	void WeaponIdle( void );

	float GetMaxSpeed( void ) { return 250; }
	virtual BOOL UseDecrement( void ) { return TRUE; }

	int m_iShell;
	int m_iShellOn;		// name ours: Deploy sets it to 1, nothing reads it
	unsigned short m_usFireMAC10;	// name ours
};


class CP228 : public CBasePlayerWeapon
{
public:
	void Spawn( void );
	void Precache( void );
	int iItemSlot( void ) { return 2; }
	int GetItemInfo(ItemInfo *p);

	void PrimaryAttack( void );
	void P228Fire( float flSpread, float flCycleTime, BOOL fUseAutoAim );
	BOOL Deploy( void );
	void Reload( void );
	void WeaponIdle( void );

	float GetMaxSpeed( void ) { return 250; }
	virtual BOOL UseDecrement( void ) { return TRUE; }

	int m_iShell;
	unsigned short m_usFireP228;	// name ours
};


class CP90 : public CBasePlayerWeapon
{
public:
	void Spawn( void );
	void Precache( void );
	int iItemSlot( void ) { return 1; }
	int GetItemInfo(ItemInfo *p);

	void PrimaryAttack( void );
	void P90Fire( float flSpread, float flCycleTime, BOOL fUseAutoAim );
	BOOL Deploy( void );
	void Reload( void );
	void WeaponIdle( void );

	float GetMaxSpeed( void ) { return 245; }
	virtual BOOL UseDecrement( void ) { return TRUE; }

	int m_iShell;
	int m_iShellOn;		// name ours: Deploy sets it to 1, nothing reads it
	unsigned short m_usFireP90;	// name ours
};


class CSCOUT : public CBasePlayerWeapon
{
public:
	void Spawn( void );
	void Precache( void );
	int iItemSlot( void ) { return 1; }
	int GetItemInfo(ItemInfo *p);

	void PrimaryAttack( void );
	void SecondaryAttack( void );
	void SCOUTFire( float flSpread, float flCycleTime, BOOL fUseAutoAim );
	BOOL Deploy( void );
	void Reload( void );
	void WeaponIdle( void );

	float GetMaxSpeed( void );
	virtual BOOL UseDecrement( void ) { return TRUE; }

	int m_iShell;
	unsigned short m_usFireSCOUT;	// name ours
};


class CSmokeGrenade : public CBasePlayerWeapon
{
public:
	void Spawn( void );
	void Precache( void );
	int iItemSlot( void ) { return 4; }
	int GetItemInfo(ItemInfo *p);

	BOOL CanDeploy( void );
	BOOL Deploy( void );
	BOOL CanHolster( void );
	void Holster( int skiplocal );
	void PrimaryAttack( void );
	void WeaponIdle( void );

	float GetMaxSpeed( void ) { return 250; }
	BOOL CanDrop( void ) { return FALSE; }
	virtual BOOL UseDecrement( void ) { return TRUE; }

	float m_flStartThrow;
	float m_flReleaseThrow;
	unsigned short m_usCreateSmoke;	// name ours
};


class CTMP : public CBasePlayerWeapon
{
public:
	void Spawn( void );
	void Precache( void );
	int iItemSlot( void ) { return 1; }
	int GetItemInfo(ItemInfo *p);

	void PrimaryAttack( void );
	void TMPFire( float flSpread, float flCycleTime, BOOL fUseAutoAim );
	BOOL Deploy( void );
	void Reload( void );
	void WeaponIdle( void );

	float GetMaxSpeed( void ) { return 250; }
	virtual BOOL UseDecrement( void ) { return TRUE; }

	int m_iShell;
	int m_iShellOn;		// name ours: Deploy sets it to 1, nothing reads it
	unsigned short m_usFireTMP;	// name ours
};



class CXM1014 : public CBasePlayerWeapon
{
public:
	void Spawn( void );
	void Precache( void );
	int iItemSlot( void ) { return 1; }
	int GetItemInfo(ItemInfo *p);

	void PrimaryAttack( void );
	BOOL Deploy( void );
	void Reload( void );
	void WeaponIdle( void );

	float GetMaxSpeed( void ) { return 240; }
	virtual BOOL UseDecrement( void ) { return TRUE; }

	int m_iShell;
	float m_flPumpTime;
	unsigned short m_usFireXM1014;	// name ours
};

#endif // WEAPONS_H

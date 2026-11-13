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

===== weapons.cpp ========================================================

  functions governing the selection/use of weapons for players

*/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "soundent.h"
#include "decals.h"
#include "gamerules.h"

extern CGraph	WorldGraph;
extern int gEvilImpulse101;


#define NOT_USED 255

DLL_GLOBAL	short	g_sModelIndexLaser;// holds the index for the laser beam
DLL_GLOBAL  const char *g_pModelNameLaser = "sprites/laserbeam.spr";
DLL_GLOBAL	short	g_sModelIndexLaserDot;// holds the index for the laser beam dot
DLL_GLOBAL	short	g_sModelIndexFireball;// holds the index for the fireball
DLL_GLOBAL	short	g_sModelIndexSmoke;// holds the index for the smoke cloud
DLL_GLOBAL	short	g_sModelIndexWExplosion;// holds the index for the underwater explosion
DLL_GLOBAL	short	g_sModelIndexBubbles;// holds the index for the bubbles model
DLL_GLOBAL	short	g_sModelIndexBloodDrop;// holds the sprite index for the initial blood
DLL_GLOBAL	short	g_sModelIndexBloodSpray;// holds the sprite index for splattered blood
// Beta 4.0 (Linux symbols)
DLL_GLOBAL	short	g_sModelIndexSmokePuff;
DLL_GLOBAL	short	g_sModelIndexFireball2;
DLL_GLOBAL	short	g_sModelIndexFireball3;
DLL_GLOBAL	short	g_sModelIndexFireball4;
DLL_GLOBAL	short	g_sModelIndexRadio;
// Beta 5.0 (leak) (Linux symbols): the ghosts' sprites and the planted C4's glow
DLL_GLOBAL	short	g_sModelIndexCTGhost;
DLL_GLOBAL	short	g_sModelIndexTGhost;
DLL_GLOBAL	short	g_sModelIndexC4Glow;

ItemInfo CBasePlayerItem::ItemInfoArray[MAX_WEAPONS];
AmmoInfo CBasePlayerItem::AmmoInfoArray[MAX_AMMO_SLOTS];

extern int gmsgCurWeapon;

MULTIDAMAGE gMultiDamage;

#define TRACER_FREQ		4			// Tracers fire every fourth bullet


//=========================================================
// MaxAmmoCarry - pass in a name and this function will tell
// you the maximum amount of that type of ammunition that a 
// player can carry.
//=========================================================
int MaxAmmoCarry( int iszName )
{
	for ( int i = 0;  i < MAX_WEAPONS; i++ )
	{
		if ( CBasePlayerItem::ItemInfoArray[i].pszAmmo1 && !strcmp( STRING(iszName), CBasePlayerItem::ItemInfoArray[i].pszAmmo1 ) )
			return CBasePlayerItem::ItemInfoArray[i].iMaxAmmo1;
		if ( CBasePlayerItem::ItemInfoArray[i].pszAmmo2 && !strcmp( STRING(iszName), CBasePlayerItem::ItemInfoArray[i].pszAmmo2 ) )
			return CBasePlayerItem::ItemInfoArray[i].iMaxAmmo2;
	}

	ALERT( at_console, "MaxAmmoCarry() doesn't recognize '%s'!\n", STRING( iszName ) );
	return -1;
}

	
/*
==============================================================================

MULTI-DAMAGE

Collects multiple small damages into a single damage

==============================================================================
*/

//
// ClearMultiDamage - resets the global multi damage accumulator
//
void ClearMultiDamage(void)
{
	gMultiDamage.pEntity = NULL;
	gMultiDamage.amount	= 0;
	gMultiDamage.type = 0;
}


//
// ApplyMultiDamage - inflicts contents of global multi damage register on gMultiDamage.pEntity
//
// GLOBALS USED:
//		gMultiDamage

void ApplyMultiDamage(entvars_t *pevInflictor, entvars_t *pevAttacker )
{
	Vector		vecSpot1;//where blood comes from
	Vector		vecDir;//direction blood should go
	TraceResult	tr;
	
	if ( !gMultiDamage.pEntity )
		return;

	gMultiDamage.pEntity->TakeDamage(pevInflictor, pevAttacker, gMultiDamage.amount, gMultiDamage.type );
}


// GLOBALS USED:
//		gMultiDamage

void AddMultiDamage( entvars_t *pevInflictor, CBaseEntity *pEntity, float flDamage, int bitsDamageType)
{
	if ( !pEntity )
		return;
	
	gMultiDamage.type |= bitsDamageType;

	if ( pEntity != gMultiDamage.pEntity )
	{
		ApplyMultiDamage(pevInflictor,pevInflictor); // UNDONE: wrong attacker!
		gMultiDamage.pEntity	= pEntity;
		gMultiDamage.amount		= 0;
	}

	gMultiDamage.amount += flDamage;
}

/*
================
SpawnBlood
================
*/
void SpawnBlood(Vector vecSpot, int bloodColor, float flDamage)
{
	UTIL_BloodDrips( vecSpot, g_vecAttackDir, bloodColor, (int)flDamage );
}


int DamageDecal( CBaseEntity *pEntity, int bitsDamageType )
{
	// Counter-Strike Beta 6.0: the last two gunshot decals only (was any of five)
	if ( !pEntity )
		return (DECAL_GUNSHOT1 + RANDOM_LONG(3,4));
	
	return pEntity->DamageDecal( bitsDamageType );
}

void DecalGunshot( TraceResult *pTrace, int iBulletType, bool fShooterOnly, entvars_t *pevShooter, bool bHitMetal )
{
	// Beta 6.5: the client's events draw the decals.
	// Counter-Strike 1.4: nothing at all (every call goes to a function of one ret)
}



//
// EjectBrass - tosses a brass shell from passed origin at passed velocity
//
void EjectBrass ( const Vector &vecOrigin, const Vector &vecVelocity, float rotation, int model, int soundtype )
{
	// FIX: when the player shoots, their gun isn't in the same position as it is on the model other players see.

	MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, vecOrigin );
		WRITE_BYTE( TE_MODEL);
		WRITE_COORD( vecOrigin.x);
		WRITE_COORD( vecOrigin.y);
		WRITE_COORD( vecOrigin.z);
		WRITE_COORD( vecVelocity.x);
		WRITE_COORD( vecVelocity.y);
		WRITE_COORD( vecVelocity.z);
		WRITE_ANGLE( rotation );
		WRITE_SHORT( model );
		WRITE_BYTE ( soundtype);
		WRITE_BYTE ( 25 );// 2.5 seconds
	MESSAGE_END();
}

//
// EjectBrass2 - the same shell, shown to one player only (the one who fired)
//
void EjectBrass2 ( const Vector &vecOrigin, const Vector &vecVelocity, float rotation, int model, int soundtype, entvars_t *pev )
{
	MESSAGE_BEGIN( MSG_ONE, SVC_TEMPENTITY, NULL, pev );
		WRITE_BYTE( TE_MODEL);
		WRITE_COORD( vecOrigin.x);
		WRITE_COORD( vecOrigin.y);
		WRITE_COORD( vecOrigin.z);
		WRITE_COORD( vecVelocity.x);
		WRITE_COORD( vecVelocity.y);
		WRITE_COORD( vecVelocity.z);
		WRITE_ANGLE( rotation );
		WRITE_SHORT( model );
		WRITE_BYTE ( 0 );	// sic: not soundtype
		WRITE_BYTE ( 5 );// 0.5 seconds
	MESSAGE_END();
}


#if 0
// UNDONE: This is no longer used?
void ExplodeModel( const Vector &vecOrigin, float speed, int model, int count )
{
	MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, vecOrigin );
		WRITE_BYTE ( TE_EXPLODEMODEL );
		WRITE_COORD( vecOrigin.x );
		WRITE_COORD( vecOrigin.y );
		WRITE_COORD( vecOrigin.z );
		WRITE_COORD( speed );
		WRITE_SHORT( model );
		WRITE_SHORT( count );
		WRITE_BYTE ( 15 );// 1.5 seconds
	MESSAGE_END();
}
#endif


int giAmmoIndex = 0;

// Precaches the ammo and queues the ammo info for sending to clients
void AddAmmoNameToAmmoRegistry( const char *szAmmoname )
{
	// make sure it's not already in the registry
	for ( int i = 0; i < MAX_AMMO_SLOTS; i++ )
	{
		if ( !CBasePlayerItem::AmmoInfoArray[i].pszName)
			continue;

		if ( stricmp( CBasePlayerItem::AmmoInfoArray[i].pszName, szAmmoname ) == 0 )
			return; // ammo already in registry, just quite
	}


	giAmmoIndex++;
	ASSERT( giAmmoIndex < MAX_AMMO_SLOTS );
	if ( giAmmoIndex >= MAX_AMMO_SLOTS )
		giAmmoIndex = 0;

	CBasePlayerItem::AmmoInfoArray[giAmmoIndex].pszName = szAmmoname;
	CBasePlayerItem::AmmoInfoArray[giAmmoIndex].iId = giAmmoIndex;   // yes, this info is redundant
}


// Precaches the weapon and queues the weapon info for sending to clients
void UTIL_PrecacheOtherWeapon( const char *szClassname )
{
	edict_t	*pent;

	pent = CREATE_NAMED_ENTITY( MAKE_STRING( szClassname ) );
	if ( FNullEnt( pent ) )
	{
		ALERT ( at_console, "NULL Ent in UTIL_PrecacheOtherWeapon\n" );
		return;
	}
	
	CBaseEntity *pEntity = CBaseEntity::Instance (VARS( pent ));

	if (pEntity)
	{
		ItemInfo II;
		pEntity->Precache( );
		memset( &II, 0, sizeof II );
		if ( ((CBasePlayerItem*)pEntity)->GetItemInfo( &II ) )
		{
			CBasePlayerItem::ItemInfoArray[II.iId] = II;

			if ( II.pszAmmo1 && *II.pszAmmo1 )
			{
				AddAmmoNameToAmmoRegistry( II.pszAmmo1 );
			}

			if ( II.pszAmmo2 && *II.pszAmmo2 )
			{
				AddAmmoNameToAmmoRegistry( II.pszAmmo2 );
			}

			memset( &II, 0, sizeof II );
		}
	}

	REMOVE_ENTITY(pent);
}

// Counter-Strike's weapons are registered through a copy of the function above (retail
// 0x10051780 has UTIL_PrecacheOtherWeapon's bytes, message included).
void UTIL_PrecacheOtherWeapon2( const char *szClassname )
{
	edict_t	*pent;

	pent = CREATE_NAMED_ENTITY( MAKE_STRING( szClassname ) );
	if ( FNullEnt( pent ) )
	{
		ALERT ( at_console, "NULL Ent in UTIL_PrecacheOtherWeapon\n" );
		return;
	}
	
	CBaseEntity *pEntity = CBaseEntity::Instance (VARS( pent ));

	if (pEntity)
	{
		ItemInfo II;
		pEntity->Precache( );
		memset( &II, 0, sizeof II );
		if ( ((CBasePlayerItem*)pEntity)->GetItemInfo( &II ) )
		{
			CBasePlayerItem::ItemInfoArray[II.iId] = II;

			if ( II.pszAmmo1 && *II.pszAmmo1 )
			{
				AddAmmoNameToAmmoRegistry( II.pszAmmo1 );
			}

			if ( II.pszAmmo2 && *II.pszAmmo2 )
			{
				AddAmmoNameToAmmoRegistry( II.pszAmmo2 );
			}

			memset( &II, 0, sizeof II );
		}
	}

	REMOVE_ENTITY(pent);
}

// called by worldspawn
void W_Precache(void)
{
	memset( CBasePlayerItem::ItemInfoArray, 0, sizeof(CBasePlayerItem::ItemInfoArray) );
	memset( CBasePlayerItem::AmmoInfoArray, 0, sizeof(CBasePlayerItem::AmmoInfoArray) );
	giAmmoIndex = 0;

	// custom items...

	// common world objects
	UTIL_PrecacheOther( "item_suit" );
	UTIL_PrecacheOther( "item_battery" );
	UTIL_PrecacheOther( "item_antidote" );
	UTIL_PrecacheOther( "item_security" );
	UTIL_PrecacheOther( "item_longjump" );
	// Counter-Strike: armour, Beta 4.0's backpack and thigh pack
	UTIL_PrecacheOther( "item_kevlar" );
	UTIL_PrecacheOther( "item_assaultsuit" );
	UTIL_PrecacheOther( "item_thighpack" );

	// Counter-Strike: each gun with its ammunition (Beta 4.0: Half-Life's
	// guns are gone)
	// .338
	UTIL_PrecacheOtherWeapon( "weapon_awp" );
	UTIL_PrecacheOther( "ammo_338magnum" );

	// 7.62
	UTIL_PrecacheOtherWeapon( "weapon_g3sg1" );
	UTIL_PrecacheOtherWeapon( "weapon_ak47" );
	UTIL_PrecacheOtherWeapon( "weapon_scout" );
	UTIL_PrecacheOther( "ammo_762nato" );

	// 5.56
	UTIL_PrecacheOtherWeapon( "weapon_m249" );
	UTIL_PrecacheOther( "ammo_556natobox" );	// Beta 6.1
	UTIL_PrecacheOtherWeapon( "weapon_m4a1" );
	UTIL_PrecacheOtherWeapon( "weapon_sg552" );
	UTIL_PrecacheOtherWeapon( "weapon_aug" );		// Beta 6.0
	UTIL_PrecacheOtherWeapon( "weapon_sg550" );	// Counter-Strike 1.0
	UTIL_PrecacheOther( "ammo_556nato" );

	// shotgun
	UTIL_PrecacheOtherWeapon( "weapon_m3" );
	UTIL_PrecacheOtherWeapon( "weapon_xm1014" );	// Beta 5.0 (leak)
	UTIL_PrecacheOther( "ammo_buckshot" );

	// .45
	UTIL_PrecacheOtherWeapon( "weapon_usp" );
	UTIL_PrecacheOtherWeapon( "weapon_mac10" );	// Beta 6.0
	UTIL_PrecacheOtherWeapon( "weapon_ump45" );	// Counter-Strike 1.0
	UTIL_PrecacheOther( "ammo_45acp" );

	// 5.7mm
	UTIL_PrecacheOtherWeapon( "weapon_fiveseven" );	// Counter-Strike 1.0
	UTIL_PrecacheOtherWeapon( "weapon_p90" );
	UTIL_PrecacheOther( "ammo_57mm" );

	// .50
	UTIL_PrecacheOtherWeapon( "weapon_deagle" );
	UTIL_PrecacheOther( "ammo_50ae" );

	// .357 SIG
	UTIL_PrecacheOtherWeapon( "weapon_p228" );
	UTIL_PrecacheOther( "ammo_357sig" );

	// knife
	UTIL_PrecacheOtherWeapon( "weapon_knife" );

	// 9mm
	UTIL_PrecacheOtherWeapon( "weapon_glock18" );
	UTIL_PrecacheOtherWeapon( "weapon_mp5navy" );
	UTIL_PrecacheOtherWeapon( "weapon_tmp" );
	UTIL_PrecacheOtherWeapon( "weapon_elite" );	// Beta 7.0
	UTIL_PrecacheOther( "ammo_9mm" );

	// flashbang
	UTIL_PrecacheOtherWeapon( "weapon_flashbang" );

	// HE grenade
	UTIL_PrecacheOtherWeapon( "weapon_hegrenade" );

	// Beta 6.5: the smoke grenade
	UTIL_PrecacheOtherWeapon( "weapon_smokegrenade" );

	// Beta 5.0 (leak): the C4 is a weapon
	UTIL_PrecacheOtherWeapon( "weapon_c4" );


#if !defined( OEM_BUILD ) && !defined( HLDEMO_BUILD )
	if ( g_pGameRules->IsDeathmatch() )
	{
		UTIL_PrecacheOther( "weaponbox" );// container for dropped deathmatch weapons
	}
#endif

	g_sModelIndexFireball = PRECACHE_MODEL ("sprites/zerogxplode.spr");// fireball
	g_sModelIndexWExplosion = PRECACHE_MODEL ("sprites/WXplo1.spr");// underwater fireball
	g_sModelIndexSmoke = PRECACHE_MODEL ("sprites/steam1.spr");// smoke
	g_sModelIndexBubbles = PRECACHE_MODEL ("sprites/bubble.spr");//bubbles
	g_sModelIndexBloodSpray = PRECACHE_MODEL ("sprites/bloodspray.spr"); // initial blood
	g_sModelIndexBloodDrop = PRECACHE_MODEL ("sprites/blood.spr"); // splattered blood 
	// Beta 4.0: the HE grenade's and the C4's explosions, the radio icon
	g_sModelIndexSmokePuff = PRECACHE_MODEL ("sprites/smokepuff.spr");
	g_sModelIndexFireball2 = PRECACHE_MODEL ("sprites/eexplo.spr");
	g_sModelIndexFireball3 = PRECACHE_MODEL ("sprites/fexplo.spr");
	g_sModelIndexFireball4 = PRECACHE_MODEL ("sprites/fexplo1.spr");
	g_sModelIndexRadio = PRECACHE_MODEL ("sprites/radio.spr");
	// Beta 5.0 (leak): the ghosts of the dead (counter-terrorists, terrorists) and the planted C4's glow
	g_sModelIndexCTGhost = PRECACHE_MODEL ("sprites/b-tele1.spr");
	g_sModelIndexTGhost = PRECACHE_MODEL ("sprites/c-tele1.spr");
	g_sModelIndexC4Glow = PRECACHE_MODEL ("sprites/ledglow.spr");

	g_sModelIndexLaser = PRECACHE_MODEL( (char *)g_pModelNameLaser );
	g_sModelIndexLaserDot = PRECACHE_MODEL("sprites/laserdot.spr");


	// used by explosions
	PRECACHE_MODEL ("models/grenade.mdl");
	PRECACHE_MODEL ("sprites/explode1.spr");

	PRECACHE_SOUND ("weapons/debris1.wav");// explosion aftermaths
	PRECACHE_SOUND ("weapons/debris2.wav");// explosion aftermaths
	PRECACHE_SOUND ("weapons/debris3.wav");// explosion aftermaths

	PRECACHE_SOUND ("weapons/grenade_hit1.wav");//grenade
	PRECACHE_SOUND ("weapons/grenade_hit2.wav");//grenade
	PRECACHE_SOUND ("weapons/grenade_hit3.wav");//grenade

	PRECACHE_SOUND ("weapons/bullet_hit1.wav");	// hit by bullet
	PRECACHE_SOUND ("weapons/bullet_hit2.wav");	// hit by bullet
	
	PRECACHE_SOUND ("items/weapondrop1.wav");// weapon falls to the ground

	// Beta 5.0 (leak): what the others hear of a reload (ReloadSound)
	PRECACHE_SOUND ("weapons/generic_reload.wav");

	// Counter-Strike 1.3: the weapons' player models and their bounds (here since
	// Beta 6.6) are precached by ClientPrecache, where Counter-Strike 1.6 has them.
}


 

TYPEDESCRIPTION	CBasePlayerItem::m_SaveData[] = 
{
	DEFINE_FIELD( CBasePlayerItem, m_pPlayer, FIELD_CLASSPTR ),
	DEFINE_FIELD( CBasePlayerItem, m_pNext, FIELD_CLASSPTR ),
	//DEFINE_FIELD( CBasePlayerItem, m_fKnown, FIELD_INTEGER ),Reset to zero on load
	DEFINE_FIELD( CBasePlayerItem, m_iId, FIELD_INTEGER ),
	// DEFINE_FIELD( CBasePlayerItem, m_iIdPrimary, FIELD_INTEGER ),
	// DEFINE_FIELD( CBasePlayerItem, m_iIdSecondary, FIELD_INTEGER ),
};
IMPLEMENT_SAVERESTORE( CBasePlayerItem, CBaseAnimating );


TYPEDESCRIPTION	CBasePlayerWeapon::m_SaveData[] = 
{
	DEFINE_FIELD( CBasePlayerWeapon, m_flNextPrimaryAttack, FIELD_TIME ),
	DEFINE_FIELD( CBasePlayerWeapon, m_flNextSecondaryAttack, FIELD_TIME ),
	DEFINE_FIELD( CBasePlayerWeapon, m_flTimeWeaponIdle, FIELD_TIME ),
	DEFINE_FIELD( CBasePlayerWeapon, m_iPrimaryAmmoType, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayerWeapon, m_iSecondaryAmmoType, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayerWeapon, m_iClip, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayerWeapon, m_iDefaultAmmo, FIELD_INTEGER ),
//	DEFINE_FIELD( CBasePlayerWeapon, m_iClientClip, FIELD_INTEGER )	 , reset to zero on load so hud gets updated correctly
//  DEFINE_FIELD( CBasePlayerWeapon, m_iClientWeaponState, FIELD_INTEGER ), reset to zero on load so hud gets updated correctly
};

IMPLEMENT_SAVERESTORE( CBasePlayerWeapon, CBasePlayerItem );


void CBasePlayerItem :: SetObjectCollisionBox( void )
{
	pev->absmin = pev->origin + Vector(-24, -24, 0);
	pev->absmax = pev->origin + Vector(24, 24, 16); 
}


//=========================================================
// Sets up movetype, size, solidtype for a new weapon. 
//=========================================================
void CBasePlayerItem :: FallInit( void )
{
	pev->movetype = MOVETYPE_TOSS;
	pev->solid = SOLID_BBOX;

	UTIL_SetOrigin( pev, pev->origin );
	UTIL_SetSize(pev, Vector( 0, 0, 0), Vector(0, 0, 0) );//pointsize until it lands on the ground.
	
	SetTouch( DefaultTouch );
	SetThink( FallThink );

	pev->nextthink = gpGlobals->time + 0.1;
}

//=========================================================
// FallThink - Items that have just spawned run this think
// to catch them when they hit the ground. Once we're sure
// that the object is grounded, we change its solid type
// to trigger and set it in a large box that helps the
// player get it.
//=========================================================
void CBasePlayerItem::FallThink ( void )
{
	pev->nextthink = gpGlobals->time + 0.1;

	if ( pev->flags & FL_ONGROUND )
	{
		// clatter if we have an owner (i.e., dropped by someone)
		// don't clatter if the gun is waiting to respawn (if it's waiting, it is invisible!)
		if ( !FNullEnt( pev->owner ) )
		{
			int pitch = 95 + RANDOM_LONG(0,29);
			EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "items/weapondrop1.wav", 1, ATTN_NORM, 0, pitch);	
		}

		// lie flat
		pev->angles.x = 0;
		pev->angles.z = 0;

		Materialize(); 
	}
}

//=========================================================
// Materialize - make a CBasePlayerItem visible and tangible
//=========================================================
void CBasePlayerItem::Materialize( void )
{
	if ( pev->effects & EF_NODRAW )
	{
		// changing from invisible state to visible.
		if ( g_pGameRules->IsMultiplayer() )	// Counter-Strike 1.0: not in the training course
			EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, "items/suitchargeok1.wav", 1, ATTN_NORM, 0, 150 );
		pev->effects &= ~EF_NODRAW;
		pev->effects |= EF_MUZZLEFLASH;
	}

	pev->solid = SOLID_TRIGGER;

	UTIL_SetOrigin( pev, pev->origin );// link into world.
	SetTouch (DefaultTouch);
	if ( g_pGameRules->IsMultiplayer() )	// Counter-Strike 1.0: the training course's weapons stay
	{
		SetThink (SUB_Remove);
		pev->nextthink = gpGlobals->time + 1;	// a weapon nobody picks up is gone after a second (Beta 2.0 and 2.1: 20)
	}
	else
		SetThink (NULL);
}

//=========================================================
// AttemptToMaterialize - the item is trying to rematerialize,
// should it do so now or wait longer?
//=========================================================
void CBasePlayerItem::AttemptToMaterialize( void )
{
	float time = g_pGameRules->FlWeaponTryRespawn( this );

	if ( time == 0 )
	{
		Materialize();
		return;
	}

	pev->nextthink = gpGlobals->time + time;
}

//=========================================================
// CheckRespawn - a player is taking this weapon, should 
// it respawn?
//=========================================================
void CBasePlayerItem :: CheckRespawn ( void )
{
	switch ( g_pGameRules->WeaponShouldRespawn( this ) )
	{
	case GR_WEAPON_RESPAWN_YES:
//		Respawn();
		break;
	case GR_WEAPON_RESPAWN_NO:
		return;
		break;
	}
}

// Beta 5.0 (leak): the SDK's Deploy, out of the class body (retail has its body
// here, after CheckRespawn; every gun's Deploy overrides vtable slot 64, after
// CanDrop's 63)
BOOL CBasePlayerItem::Deploy( void )
{
	return TRUE;
}

//=========================================================
// Respawn- this item is already in the world, but it is
// invisible and intangible. Make it visible and tangible.
//=========================================================
CBaseEntity* CBasePlayerItem::Respawn( void )
{
	// make a copy of this weapon that is invisible and inaccessible to players (no touch function). The weapon spawn/respawn code
	// will decide when to make the weapon visible and touchable.
	CBaseEntity *pNewWeapon = CBaseEntity::Create( (char *)STRING( pev->classname ), g_pGameRules->VecWeaponRespawnSpot( this ), pev->angles, pev->owner );

	if ( pNewWeapon )
	{
		pNewWeapon->pev->effects |= EF_NODRAW;// invisible for now
		pNewWeapon->SetTouch( NULL );// no touch
		pNewWeapon->SetThink( AttemptToMaterialize );

		DROP_TO_FLOOR ( ENT(pev) );

		// not a typo! We want to know when the weapon the player just picked up should respawn! This new entity we created is the replacement,
		// but when it should respawn is based on conditions belonging to the weapon that was taken.
		pNewWeapon->pev->nextthink = g_pGameRules->FlWeaponRespawnTime( this );
	}
	else
	{
		ALERT ( at_console, "Respawn failed to create %s!\n", STRING( pev->classname ) );
	}

	return pNewWeapon;
}

void CBasePlayerItem::DefaultTouch( CBaseEntity *pOther )
{
	// if it's not a player, ignore
	if ( !pOther->IsPlayer() )
		return;

	CBasePlayer *pPlayer = (CBasePlayer *)pOther;

	// Beta 6.0: the VIP picks up nothing but a knife (Beta 7.0: and four of the pistols)
	if ( m_iId != WEAPON_USP && m_iId != WEAPON_GLOCK18 && m_iId != WEAPON_P228 && m_iId != WEAPON_DEAGLE && m_iId != WEAPON_KNIFE && pPlayer->m_bIsVIP == true )
		return;

	// can I have this? (Beta 5.0 leak: the game rules decide whether the slot is
	// free; Beta 2.0 to 4.1 tested for a primary weapon here)
	if ( !g_pGameRules->CanHavePlayerItem( pPlayer, this ) )
	{
		if ( gEvilImpulse101 )
		{
			UTIL_Remove( this );
		}
		return;
	}

	if (pOther->AddPlayerItem( this ))
	{
		AttachToPlayer( pPlayer );
		EMIT_SOUND(ENT(pPlayer->pev), CHAN_ITEM, "items/gunpickup2.wav", 1, ATTN_NORM);
	}

	SUB_UseTargets( pOther, USE_TOGGLE, 0 ); // UNDONE: when should this happen?
}

//=========================================================
// EjectBrassLate - the shell of a bolt or pump action: the
// weapon sets m_pPlayer->m_flEjectBrass when it fires and
// ItemPostFrame calls this when the time has come.
//=========================================================
void CBasePlayerWeapon::EjectBrassLate( void )
{
	// Beta 6.0: the shell flies out on the side away from the gun hand
	// (Beta 6.5: 9 units out and 9 down; Beta 6.0: 6 and 12).
	// Counter-Strike 1.4: one side only (the server no longer knows the hand).
	UTIL_MakeVectors( m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle );

	Vector	vecShellVelocity = m_pPlayer->pev->velocity 
							 + gpGlobals->v_right * RANDOM_FLOAT(50,70) 
							 + gpGlobals->v_up * RANDOM_FLOAT(100,150) 
							 + gpGlobals->v_forward * 25;
	EjectBrass ( pev->origin + m_pPlayer->pev->view_ofs + gpGlobals->v_up * -9 + gpGlobals->v_forward * 16 - gpGlobals->v_right * 9 , vecShellVelocity, pev->angles.y, m_iShellLate, TE_BOUNCE_SHELL); 
}

//=========================================================
// KickBack - Beta 4.0's recoil (Linux symbol; parameter and
// local names ours): every shot pushes the view up by
// up_base (the first) or up_base plus up_modifier for each
// shot fired, to up_max at most, and sideways by the
// lateral values to the side m_iDirection says, to
// lateral_max; one shot in direction_change + 1 turns the
// side over (Beta 4.1: before the sideways kick, which
// that shot gives at half strength; Beta 4.0: after it).
// Beta 6.5: the shot count is the weapon's own.
//=========================================================
void CBasePlayerWeapon::KickBack( float up_base, float lateral_base, float up_modifier, float lateral_modifier, float up_max, float lateral_max, int direction_change )
{
	float flKickUp;
	float flKickLateral;

	if ( m_iShotsFired == 1 )
	{
		flKickUp = up_base;
		flKickLateral = lateral_base;
	}
	else
	{
		flKickUp = up_base + m_iShotsFired * up_modifier;
		flKickLateral = lateral_base + m_iShotsFired * lateral_modifier;
	}

	m_pPlayer->pev->punchangle.x -= flKickUp;
	if ( m_pPlayer->pev->punchangle.x < up_max * -1 )
		m_pPlayer->pev->punchangle.x = up_max * -1;

	// Beta 5.0 (leak): Beta 4.0's order again: the side turns over after the
	// sideways kick, and the kick that turns it is not halved (Beta 4.1)
	if ( m_iDirection == 1 )
	{
		m_pPlayer->pev->punchangle.y += flKickLateral;
		if ( m_pPlayer->pev->punchangle.y > lateral_max )
			m_pPlayer->pev->punchangle.y = lateral_max;
	}
	else
	{
		m_pPlayer->pev->punchangle.y -= flKickLateral;
		if ( m_pPlayer->pev->punchangle.y < lateral_max * -1 )
			m_pPlayer->pev->punchangle.y = lateral_max * -1;
	}

	if ( !RANDOM_LONG( 0, direction_change ) )
		m_iDirection = 1 - m_iDirection;
}

//=========================================================
// FireRemaining - the second and third shot of the Glock
// 18's burst.  The weapon fires the first one and sets
// m_flNextBurstShot; ItemPostFrame calls this as long as
// that is not zero.  Beta 6.5: the burst's state is the
// weapon's own, the shot is the client's event (the shell
// and the sound with it), and only the Glock is left.
//=========================================================
void CBasePlayerWeapon::FireRemaining( void )
{
	m_iClip--;

	if (m_iClip < 0)
	{
		m_iClip = 0;
		m_iBurstShotsFired = 3;
		m_flNextBurstShot = 0;
		return;
	}

	UTIL_MakeVectors( m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle );
	Vector vecSrc = m_pPlayer->GetGunPosition( );
	Vector vecAiming;
	vecAiming = gpGlobals->v_forward;

	Vector vecDir;
	vecDir = m_pPlayer->FireBullets3( vecSrc, vecAiming, 0.05, 8192, 1, BULLET_PLAYER_9MM, 18, 0.9, m_pPlayer->pev, true, m_pPlayer->random_seed );
	m_pPlayer->ammo_9mm--;

	PLAYBACK_EVENT_FULL( FEV_NOTHOST, m_pPlayer->edict(), m_usFireGlock18, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, vecDir.x, vecDir.y, (int)(m_pPlayer->pev->punchangle.x * 100), (int)(m_pPlayer->pev->punchangle.y * 100), (m_iClip == 0), FALSE );	// Counter-Strike 1.4: 100, as the weapons (1.3: 10000 here, 10000000 there)

	m_pPlayer->pev->effects |= EF_MUZZLEFLASH;
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

	m_iBurstShotsFired++;

	if (m_iBurstShotsFired != 3)
		m_flNextBurstShot = gpGlobals->time + 0.1f;
	else
		m_flNextBurstShot = 0;
}

BOOL CanAttack( float attack_time, float curtime, BOOL isPredicted )
{
#if defined( CLIENT_WEAPONS )
	if ( !isPredicted )
#else
	if ( 1 )
#endif
	{
		return ( attack_time <= curtime ) ? TRUE : FALSE;
	}
	else
	{
		return ( attack_time <= 0.0 ) ? TRUE : FALSE;
	}
}

void CBasePlayerWeapon::ItemPostFrame( void )
{
	if ( m_flNextBurstShot != 0.0 )
		FireRemaining();

	// back to the scope once the bolt has been worked
	if ( m_flNextPrimaryAttack <= UTIL_WeaponTimeBase() )
	{
		if ( m_pPlayer->m_bResumeZoom == true )
		{
			m_pPlayer->m_iFOV = m_pPlayer->m_iLastZoom;
			m_pPlayer->pev->fov = m_pPlayer->m_iFOV;

			if ( m_pPlayer->m_iFOV == m_pPlayer->m_iLastZoom )
			{
				m_pPlayer->m_bResumeZoom = false;
			}
		}
	}

	if ( m_pPlayer->m_flEjectBrass != 0.0 && m_pPlayer->m_flEjectBrass <= gpGlobals->time )
	{
		m_pPlayer->m_flEjectBrass = 0;
		EjectBrassLate();
	}

	if ((m_fInReload) && ( m_pPlayer->m_flNextAttack <= UTIL_WeaponTimeBase() ) )
	{
		// complete the reload. 
		int j = min( iMaxClip() - m_iClip, m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType]);	

		// Add them to the clip
		m_iClip += j;
		m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] -= j;

		m_pPlayer->TabulateAmmo();

		m_fInReload = FALSE;
	}

	if ((m_pPlayer->pev->button & IN_ATTACK2) && ( m_flNextSecondaryAttack <= UTIL_WeaponTimeBase() ) )
	{
		if ( pszAmmo2() && !m_pPlayer->m_rgAmmo[SecondaryAmmoIndex()] )
		{
			m_fFireOnEmpty = TRUE;
		}

		SecondaryAttack();
		m_pPlayer->pev->button &= ~IN_ATTACK2;
	}
	else if ((m_pPlayer->pev->button & IN_ATTACK) && ( m_flNextPrimaryAttack <= UTIL_WeaponTimeBase() ) )
	{
		if ( (m_iClip == 0 && pszAmmo1()) || (iMaxClip() == -1 && !m_pPlayer->m_rgAmmo[PrimaryAmmoIndex()] ) )
		{
			m_fFireOnEmpty = TRUE;
		}

		m_pPlayer->TabulateAmmo();

		// Beta 5.0 (leak): no shooting in the freeze period at the start of a round
		if ( ( g_pGameRules->IsMultiplayer() && !((CHalfLifeMultiplay *)g_pGameRules)->IsFreezePeriod() && !m_pPlayer->m_bIsDefusing ) || !g_pGameRules->IsMultiplayer() )	// Counter-Strike 1.0: the training course has no freeze time; 1.4: not while defusing
			PrimaryAttack();
	}
	else if ( m_pPlayer->pev->button & IN_RELOAD && iMaxClip() != WEAPON_NOCLIP && !m_fInReload ) 
	{
		// reload when reload is pressed, or if no buttons are down and weapon is empty.
		Reload();
	}
	else if ( !(m_pPlayer->pev->button & (IN_ATTACK|IN_ATTACK2) ) )
	{
		// no fire buttons down

		// Beta 3.0: after a shot the count of shots fired is set to 20 and
		// then runs down (the accuracy of the next shot).
		// Beta 7.1: the count is cut to 15 and stands for 0.4 seconds
		// (Beta 7.0: set to 20; Beta 6.6: to 15)
		if ( m_bDelayFire == true )
		{
			m_bDelayFire = false;

			if ( m_iShotsFired > 15 )
				m_iShotsFired = 15;

			m_flDecreaseShotsFired = gpGlobals->time + 0.4;
		}

		m_fFireOnEmpty = FALSE;

		// Beta 6.5: the pistols forget their shots at once; the others lose
		// one every 0.0225 seconds (Beta 6.1: one a frame)
		if ( m_iId == WEAPON_USP || m_iId == WEAPON_GLOCK18 || m_iId == WEAPON_P228 || m_iId == WEAPON_DEAGLE || m_iId == WEAPON_ELITE || m_iId == WEAPON_FIVESEVEN )	// Beta 7.0: the Elites; 1.0: the Five-seveN
		{
			m_iShotsFired = 0;
		}
		else
		{
			if ( m_iShotsFired > 0 && m_flDecreaseShotsFired < gpGlobals->time )
			{
				m_flDecreaseShotsFired = gpGlobals->time + 0.0225;
				m_iShotsFired--;
			}
		}

		if ( !IsUseable() && m_flNextPrimaryAttack < UTIL_WeaponTimeBase() ) 
		{
			// weapon isn't useable, switch.
			// Beta 5.0 (leak): no more; an empty weapon stays in the player's hands
//			if ( !(iFlags() & ITEM_FLAG_NOAUTOSWITCHEMPTY) && g_pGameRules->GetNextBestWeapon( m_pPlayer, this ) )
//			{
//				m_flNextPrimaryAttack = ( UseDecrement() ? 0.0 : gpGlobals->time ) + 0.3;
//				return;
//			}
		}
		else
		{
			// weapon is useable. Reload if empty and weapon has waited as long as it has to after firing
			if ( m_iClip == 0 && !(iFlags() & ITEM_FLAG_NOAUTORELOAD) && m_flNextPrimaryAttack < UTIL_WeaponTimeBase() )
			{
				Reload();
				return;
			}
		}

		WeaponIdle( );
		return;
	}
	
	// catch all
	if ( ShouldWeaponIdle() )
	{
		WeaponIdle();
	}
}

void CBasePlayerItem::DestroyItem( void )
{
	if ( m_pPlayer )
	{
		// if attached to a player, remove. 
		m_pPlayer->RemovePlayerItem( this );
	}

	Kill( );
}

int CBasePlayerItem::AddToPlayer( CBasePlayer *pPlayer )
{
	m_pPlayer = pPlayer;

	// Beta 5.0 (leak): the client shows the pickup
	MESSAGE_BEGIN( MSG_ONE, gmsgWeapPickup, NULL, pPlayer->pev );
		WRITE_BYTE( m_iId );
	MESSAGE_END();

	return TRUE;
}

void CBasePlayerItem::Drop( void )
{
	SetTouch( NULL );
	SetThink(SUB_Remove);
	pev->nextthink = gpGlobals->time + .1;
}

void CBasePlayerItem::Kill( void )
{
	SetTouch( NULL );
	SetThink(SUB_Remove);
	pev->nextthink = gpGlobals->time + .1;
}

void CBasePlayerItem::Holster( int skiplocal /* = 0 */ )
{ 
	m_pPlayer->pev->viewmodel = 0; 
	m_pPlayer->pev->weaponmodel = 0;
}

void CBasePlayerItem::AttachToPlayer ( CBasePlayer *pPlayer )
{
	pev->movetype = MOVETYPE_FOLLOW;
	pev->solid = SOLID_NOT;
	pev->aiment = pPlayer->edict();
	pev->effects = EF_NODRAW; // ??
	pev->modelindex = 0;// server won't send down to clients if modelindex == 0
	pev->model = iStringNull;
	pev->owner = pPlayer->edict();
	pev->nextthink = gpGlobals->time + .1;
	SetTouch( NULL );
}

// CALLED THROUGH the newly-touched weapon's instance. The existing player weapon is pOriginal
int CBasePlayerWeapon::AddDuplicate( CBasePlayerItem *pOriginal )
{
	if ( m_iDefaultAmmo )
	{
		return ExtractAmmo( (CBasePlayerWeapon *)pOriginal );
	}
	else
	{
		// a dead player dropped this.
		return ExtractClipAmmo( (CBasePlayerWeapon *)pOriginal );
	}
}


int CBasePlayerWeapon::AddToPlayer( CBasePlayer *pPlayer )
{
	m_pPlayer = pPlayer;

	pPlayer->pev->weapons |= (1<<m_iId);

	if ( !m_iPrimaryAmmoType )
	{
		m_iPrimaryAmmoType = pPlayer->GetAmmoIndex( pszAmmo1() );
		m_iSecondaryAmmoType = pPlayer->GetAmmoIndex( pszAmmo2() );
	}

	if ( !AddWeapon() )
		return FALSE;

	// Beta 5.0 (leak): a weapon the player can take shows on his client
	MESSAGE_BEGIN( MSG_ONE, gmsgWeapPickup, NULL, pPlayer->pev );
		WRITE_BYTE( m_iId );
	MESSAGE_END();
	return TRUE;
}

int CBasePlayerWeapon::UpdateClientData( CBasePlayer *pPlayer )
{
	BOOL bSend = FALSE;
	int state = 0;
	if ( pPlayer->m_pActiveItem == this )
	{
		if ( pPlayer->m_fOnTarget )
			state = WEAPON_IS_ONTARGET;
		else
			state = 1;
	}

	// Forcing send of all data!
	if ( !pPlayer->m_fWeapon )
	{
		bSend = TRUE;
	}
	
	// This is the current or last weapon, so the state will need to be updated
	if ( this == pPlayer->m_pActiveItem ||
		 this == pPlayer->m_pClientActiveItem )
	{
		if ( pPlayer->m_pActiveItem != pPlayer->m_pClientActiveItem )
		{
			bSend = TRUE;
		}
	}

	// If the ammo, state, or fov has changed, update the weapon
	if ( m_iClip != m_iClientClip || 
		 state != m_iClientWeaponState || 
		 pPlayer->m_iFOV != pPlayer->m_iClientFOV )
	{
		bSend = TRUE;
	}
	
	if ( bSend )
	{
		MESSAGE_BEGIN( MSG_ONE, gmsgCurWeapon, NULL, pPlayer->pev );
			WRITE_BYTE( state );
			WRITE_BYTE( m_iId );
			WRITE_BYTE( m_iClip );
		MESSAGE_END();

		m_iClientClip = m_iClip;
		m_iClientWeaponState = state;
		pPlayer->m_fWeapon = TRUE;
	}

	if ( m_pNext )
		m_pNext->UpdateClientData( pPlayer );

	return 1;
}


void CBasePlayerWeapon::SendWeaponAnim( int iAnim, int skiplocal )
{
	m_pPlayer->pev->weaponanim = iAnim;

#if defined( CLIENT_WEAPONS )
	if ( skiplocal && ENGINE_CANSKIP( m_pPlayer->edict() ) )
		return;
#endif

	MESSAGE_BEGIN( MSG_ONE, SVC_WEAPONANIM, NULL, m_pPlayer->pev );
		WRITE_BYTE( iAnim );						// sequence number
		WRITE_BYTE( pev->body );					// weaponmodel bodygroup.
	MESSAGE_END();
}

BOOL CBasePlayerWeapon :: AddPrimaryAmmo( int iCount, char *szName, int iMaxClip, int iMaxCarry )
{
	int iIdAmmo;

	if (iMaxClip < 1)
	{
		m_iClip = -1;
		iIdAmmo = m_pPlayer->GiveAmmo( iCount, szName, iMaxCarry );
	}
	else if (m_iClip == 0)
	{
		int i;
		i = min( m_iClip + iCount, iMaxClip ) - m_iClip;
		m_iClip += i;
		iIdAmmo = m_pPlayer->GiveAmmo( iCount - i, szName, iMaxCarry );
	}
	else
	{
		iIdAmmo = m_pPlayer->GiveAmmo( iCount, szName, iMaxCarry );
	}
	
	// m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] = iMaxCarry; // hack for testing

	if (iIdAmmo > 0)
	{
		m_iPrimaryAmmoType = iIdAmmo;
		if (m_pPlayer->HasPlayerItem( this ) )
		{
			// play the "got ammo" sound only if we gave some ammo to a player that already had this gun.
			// if the player is just getting this gun for the first time, DefaultTouch will play the "picked up gun" sound for us.
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM);
		}
	}

	return iIdAmmo > 0 ? TRUE : FALSE;
}


BOOL CBasePlayerWeapon :: AddSecondaryAmmo( int iCount, char *szName, int iMax )
{
	int iIdAmmo;

	iIdAmmo = m_pPlayer->GiveAmmo( iCount, szName, iMax );

	//m_pPlayer->m_rgAmmo[m_iSecondaryAmmoType] = iMax; // hack for testing

	if (iIdAmmo > 0)
	{
		m_iSecondaryAmmoType = iIdAmmo;
		EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM);
	}
	return iIdAmmo > 0 ? TRUE : FALSE;
}

//=========================================================
// IsUseable - this function determines whether or not a 
// weapon is useable by the player in its current state. 
// (does it have ammo loaded? do I have any ammo for the 
// weapon?, etc)
//=========================================================
BOOL CBasePlayerWeapon :: IsUseable( void )
{
	if ( m_iClip <= 0 )
	{
		if ( m_pPlayer->m_rgAmmo[ PrimaryAmmoIndex() ] <= 0 && iMaxAmmo1() != -1 )			
		{
			// clip is empty (or nonexistant) and the player has no more ammo of this type. 
			return FALSE;
		}
	}

	return TRUE;
}

BOOL CBasePlayerWeapon :: CanDeploy( void )
{
	// Beta 5.0 (leak): a weapon can be drawn whether it has ammunition or not
	return TRUE;
}

BOOL CBasePlayerWeapon :: DefaultDeploy( char *szViewModel, char *szWeaponModel, int iAnim, char *szAnimExt, int skiplocal /* = 0 */ )
{
	if (!CanDeploy( ))
		return FALSE;

	m_pPlayer->TabulateAmmo();
	m_pPlayer->pev->viewmodel = MAKE_STRING(szViewModel);
	m_pPlayer->pev->weaponmodel = MAKE_STRING(szWeaponModel);
	m_iViewModel = m_pPlayer->pev->viewmodel;	// Beta 6.5
	strcpy( m_pPlayer->m_szAnimExtention, szAnimExt );
	SendWeaponAnim( iAnim, skiplocal );

	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.75;	// Counter-Strike 1.4 (1.3: 0.5)
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.5;	// (1.3: 1.0)
	m_flDecreaseShotsFired = gpGlobals->time;	// Beta 6.5

	m_pPlayer->pev->fov = m_pPlayer->m_iFOV = 90;
	m_pPlayer->m_bResumeZoom = false;
	m_pPlayer->m_iLastZoom = 90;

	return TRUE;
}

//=========================================================
// ReloadSound - Beta 5.0 (leak) (Linux symbol): every other
// player within 512 units hears the reload (ReloadSound
// message: the volume, 255 next to him down to 0 at 512
// units, and 0 for a shotgun's shells, 1 for a magazine)
//=========================================================
extern int gmsgReloadSound;
extern int gmsgBombPickup;

void CBasePlayerWeapon::ReloadSound( void )
{
	Vector vecDelta;
	Vector vecOrigin( m_pPlayer->pev->origin );
	float flDistance;
	CBaseEntity *pPlayer = UTIL_FindEntityByClassname( NULL, "player" );

	while ( pPlayer && pPlayer->pev->flags != FL_DORMANT )
	{
		if ( pPlayer != m_pPlayer )
		{
			vecDelta = vecOrigin - pPlayer->pev->origin;
			flDistance = vecDelta.Length();

			if ( flDistance <= 512 )
			{
				MESSAGE_BEGIN( MSG_ONE, gmsgReloadSound, NULL, pPlayer->pev );
					WRITE_BYTE( (BYTE)( ( 1.0 - flDistance / 512 ) * 255.0 ) );
					if ( !strcmp( STRING( pev->classname ), "weapon_m3" ) || !strcmp( STRING( pev->classname ), "weapon_xm1014" ) )
						WRITE_BYTE( 0 );
					else
						WRITE_BYTE( 1 );
				MESSAGE_END();
			}
		}

		pPlayer = UTIL_FindEntityByClassname( pPlayer, "player" );
	}
}


BOOL CBasePlayerWeapon :: DefaultReload( int iClipSize, int iAnim, float fDelay )
{
	if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		return FALSE;

	int j = min(iClipSize - m_iClip, m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType]);	

	if (j == 0)
		return FALSE;

	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + fDelay;

	ReloadSound();	// Beta 5.0 (leak): the others near him hear it
	SendWeaponAnim( iAnim, UseDecrement() ? 1 : 0 );

	m_fInReload = TRUE;

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + fDelay + 0.5;
	return TRUE;
}

BOOL CBasePlayerWeapon :: PlayEmptySound( void )
{
	// Beta 6.5: a dry-fire click of the weapon's kind (Half-Life's revolver
	// click before), and m_iPlayEmptySound is no longer cleared
	if ( m_iId == WEAPON_USP || m_iId == WEAPON_GLOCK18 || m_iId == WEAPON_P228 || m_iId == WEAPON_DEAGLE || m_iId == WEAPON_ELITE || m_iId == WEAPON_FIVESEVEN )	// Beta 7.0: the Elites; 1.0: the Five-seveN
	{
		if (m_iPlayEmptySound)
		{
			EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/dryfire_pistol.wav", 0.8, ATTN_NORM);
		}
	}
	else
	{
		if (m_iPlayEmptySound)
		{
			EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/dryfire_rifle.wav", 0.8, ATTN_NORM);
		}
	}

	return 0;
}

void CBasePlayerWeapon :: ResetEmptySound( void )
{
	m_iPlayEmptySound = 1;
}

//=========================================================
//=========================================================
int CBasePlayerWeapon::PrimaryAmmoIndex( void )
{
	return m_iPrimaryAmmoType;
}

//=========================================================
//=========================================================
int CBasePlayerWeapon::SecondaryAmmoIndex( void )
{
	return -1;
}

void CBasePlayerWeapon::Holster( int skiplocal /* = 0 */ )
{ 
	// Beta 6.6: the G3/SG-1's scope no longer fades the screen here
	m_fInReload = FALSE; // cancel any reload in progress.
	m_pPlayer->pev->viewmodel = 0; 
	m_pPlayer->pev->weaponmodel = 0;
}

void CBasePlayerAmmo::Spawn( void )
{
	pev->movetype = MOVETYPE_TOSS;
	pev->solid = SOLID_TRIGGER;
	UTIL_SetSize(pev, Vector(-16, -16, 0), Vector(16, 16, 16));
	UTIL_SetOrigin( pev, pev->origin );

	SetTouch( DefaultTouch );
	if ( g_pGameRules->IsMultiplayer() )	// Counter-Strike 1.0: the training course's ammunition stays
	{
		SetThink( SUB_Remove );
		pev->nextthink = gpGlobals->time + 2;	// ammo lies around for two seconds (Beta 1.0 and 1.1: five)
	}
}

CBaseEntity* CBasePlayerAmmo::Respawn( void )
{
	pev->effects |= EF_NODRAW;
	SetTouch( NULL );

	UTIL_SetOrigin( pev, g_pGameRules->VecAmmoRespawnSpot( this ) );// move to wherever I'm supposed to repawn.

	SetThink( Materialize );
	pev->nextthink = g_pGameRules->FlAmmoRespawnTime( this );

	return this;
}

void CBasePlayerAmmo::Materialize( void )
{
	if ( pev->effects & EF_NODRAW )
	{
		// changing from invisible state to visible.
		if ( g_pGameRules->IsMultiplayer() )	// Counter-Strike 1.0: not in the training course
			EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, "items/suitchargeok1.wav", 1, ATTN_NORM, 0, 150 );
		pev->effects &= ~EF_NODRAW;
		pev->effects |= EF_MUZZLEFLASH;
	}

	SetTouch( DefaultTouch );
}

void CBasePlayerAmmo :: DefaultTouch( CBaseEntity *pOther )
{
	if ( !pOther->IsPlayer() )
	{
		return;
	}

	if (AddAmmo( pOther ))
	{
		if ( g_pGameRules->AmmoShouldRespawn( this ) == GR_AMMO_RESPAWN_YES )
		{
			Respawn();
		}
		else
		{
			SetTouch( NULL );
			SetThink(SUB_Remove);
			pev->nextthink = gpGlobals->time + .1;
		}
	}
	else if (gEvilImpulse101)
	{
		// evil impulse 101 hack, kill always
		SetTouch( NULL );
		SetThink(SUB_Remove);
		pev->nextthink = gpGlobals->time + .1;
	}
}

//=========================================================
// called by the new item with the existing item as parameter
//
// if we call ExtractAmmo(), it's because the player is picking up this type of weapon for 
// the first time. If it is spawned by the world, m_iDefaultAmmo will have a default ammo amount in it.
// if  this is a weapon dropped by a dying player, has 0 m_iDefaultAmmo, which means only the ammo in 
// the weapon clip comes along. 
//=========================================================
int CBasePlayerWeapon::ExtractAmmo( CBasePlayerWeapon *pWeapon )
{
	int			iReturn = 0;

	if ( pszAmmo1() != NULL )
	{
		// blindly call with m_iDefaultAmmo. It's either going to be a value or zero. If it is zero,
		// we only get the ammo in the weapon's clip, which is what we want. 
		iReturn = pWeapon->AddPrimaryAmmo( m_iDefaultAmmo, (char *)pszAmmo1(), iMaxClip(), iMaxAmmo1() );
		m_iDefaultAmmo = 0;
	}

	if ( pszAmmo2() != NULL )
	{
		iReturn = pWeapon->AddSecondaryAmmo( 0, (char *)pszAmmo2(), iMaxAmmo2() );
	}

	return iReturn;
}

//=========================================================
// called by the new item's class with the existing item as parameter
//=========================================================
int CBasePlayerWeapon::ExtractClipAmmo( CBasePlayerWeapon *pWeapon )
{
	int			iAmmo;

	if ( m_iClip == WEAPON_NOCLIP )
	{
		iAmmo = 0;// guns with no clips always come empty if they are second-hand
	}
	else
	{
		iAmmo = m_iClip;
	}
	
	return pWeapon->m_pPlayer->GiveAmmo( iAmmo, (char *)pszAmmo1(), iMaxAmmo1() ); // , &m_iPrimaryAmmoType
}
	
//=========================================================
// RetireWeapon - no more ammo for this gun, put it away.
//=========================================================
void CBasePlayerWeapon::RetireWeapon( void )
{
	// first, no viewmodel at all.
	m_pPlayer->pev->viewmodel = iStringNull;
	m_pPlayer->pev->weaponmodel = iStringNull;
	//m_pPlayer->pev->viewmodelindex = NULL;

	g_pGameRules->GetNextBestWeapon( m_pPlayer, this );
}

//*********************************************************
// weaponbox code:
//*********************************************************

LINK_ENTITY_TO_CLASS( weaponbox, CWeaponBox );

TYPEDESCRIPTION	CWeaponBox::m_SaveData[] = 
{
	DEFINE_ARRAY( CWeaponBox, m_rgAmmo, FIELD_INTEGER, MAX_AMMO_SLOTS ),
	DEFINE_ARRAY( CWeaponBox, m_rgiszAmmo, FIELD_STRING, MAX_AMMO_SLOTS ),
	DEFINE_ARRAY( CWeaponBox, m_rgpPlayerItems, FIELD_CLASSPTR, MAX_ITEM_TYPES ),
	DEFINE_FIELD( CWeaponBox, m_cAmmoTypes, FIELD_INTEGER ),
};

IMPLEMENT_SAVERESTORE( CWeaponBox, CBaseEntity );

//=========================================================
//
//=========================================================
void CWeaponBox::Precache( void )
{
	PRECACHE_MODEL("models/w_weaponbox.mdl");
}

//=========================================================
//=========================================================
void CWeaponBox :: KeyValue( KeyValueData *pkvd )
{
	if ( m_cAmmoTypes < MAX_AMMO_SLOTS )
	{
		PackAmmo( ALLOC_STRING(pkvd->szKeyName), atoi(pkvd->szValue) );
		m_cAmmoTypes++;// count this new ammo type.

		pkvd->fHandled = TRUE;
	}
	else
	{
		ALERT ( at_console, "WeaponBox too full! only %d ammotypes allowed\n", MAX_AMMO_SLOTS );
	}
}

//=========================================================
// CWeaponBox - Spawn 
//=========================================================
void CWeaponBox::Spawn( void )
{
	Precache( );

	pev->movetype = MOVETYPE_TOSS;
	pev->solid = SOLID_TRIGGER;

	UTIL_SetSize( pev, g_vecZero, g_vecZero );

	SET_MODEL( ENT(pev), "models/w_weaponbox.mdl");
}

//=========================================================
// CWeaponBox - Kill - the think function that removes the
// box from the world.
//=========================================================
void CWeaponBox::Kill( void )
{
	CBasePlayerItem *pWeapon;
	int i;

	// destroy the weapons
	for ( i = 0 ; i < MAX_ITEM_TYPES ; i++ )
	{
		pWeapon = m_rgpPlayerItems[ i ];

		while ( pWeapon )
		{
			pWeapon->SetThink(SUB_Remove);
			pWeapon->pev->nextthink = gpGlobals->time + 0.1;
			pWeapon = pWeapon->m_pNext;
		}
	}

	// remove the box
	UTIL_Remove( this );
}

//=========================================================
// CWeaponBox - Touch: try to add my contents to the toucher
// if the toucher is a player.
//=========================================================
void CWeaponBox::Touch( CBaseEntity *pOther )
{
	if ( !(pev->flags & FL_ONGROUND ) )
	{
		return;
	}

	if ( !pOther->IsPlayer() )
	{
		// only players may touch a weaponbox.
		return;
	}

	if ( !pOther->IsAlive() )
	{
		// no dead guys.
		return;
	}

	CBasePlayer *pPlayer = (CBasePlayer *)pOther;

	// Beta 6.0: the VIP takes nothing from a weapon box
	if ( pPlayer->m_bIsVIP == true )
		return;

	int i;
	bool bRemove = true;		// name ours
	bool bEmitSound = false;	// name ours

	// Beta 5.0 (leak): the weapons first. A player who has a primary weapon or a
	// pistol leaves the box's of that slot in it (and then the box and its
	// ammunition stay); the C4 goes to a living terrorist only, and puts it on
	// his model.
	for ( i = 0 ; i < MAX_ITEM_TYPES ; i++ )
	{
		CBasePlayerItem *pItem = m_rgpPlayerItems[ i ];

		while ( pItem )
		{
			if ( !strcmp( STRING( pItem->pev->classname ), "weapon_c4" ) )
			{
				// Counter-Strike 1.1: the finder is told and the log written.
				// Counter-Strike 1.4: no test of the player's pointer; a hint the first time
				// (for a player who wants hints), the director hears of player and box, and
				// every living terrorist is told who has the bomb and gets the BombPickup message.
				if ( pPlayer->m_iTeam != TEAM_TERRORIST )
					return;
				if ( pPlayer->pev->deadflag != DEAD_NO )
					return;

				if ( pPlayer->m_bShowHints && !( pPlayer->m_flDisplayHistory & 0x1000 ) )
				{
					pPlayer->m_flDisplayHistory |= 0x1000;
					pPlayer->HintMessage( "#Hint_you_have_the_bomb", FALSE, FALSE );
				}
				else
				{
					ClientPrint( pPlayer->pev, HUD_PRINTCENTER, "#Got_bomb" );
				}

				UTIL_LogPrintf( "\"%s<%i><%s><TERRORIST>\" triggered \"Got_The_Bomb\"\n", STRING( pPlayer->pev->netname ), GETPLAYERUSERID( pPlayer->edict() ), GETPLAYERAUTHID( pPlayer->edict() ) );

				// Counter-Strike 1.3: tell director about it
				MESSAGE_BEGIN( MSG_SPEC, SVC_DIRECTOR );
					WRITE_BYTE ( 9 );	// command length in bytes
					WRITE_BYTE ( DRC_CMD_EVENT );
					WRITE_SHORT( ENTINDEX( pPlayer->edict() ) );
					WRITE_SHORT( ENTINDEX( edict() ) );
					WRITE_LONG( 6 );
				MESSAGE_END();

				pPlayer->m_bHasC4 = true;
				pPlayer->SetBombIcon( FALSE );
				pPlayer->pev->body = 1;

				CBaseEntity *pEntity = NULL;	// 1.6's two names
				CBasePlayer *pTempPlayer;

				while ( ( pEntity = UTIL_FindEntityByClassname( pEntity, "player" ) ) != NULL )
				{
					if ( FNullEnt( pEntity->edict() ) )
						break;

					if ( !pEntity->IsPlayer() )
						continue;

					if ( pEntity->pev->flags == FL_DORMANT )
						continue;

					pTempPlayer = GetClassPtr( (CBasePlayer *)pEntity->pev );

					if ( pTempPlayer->pev->deadflag == DEAD_NO && pTempPlayer->m_iTeam == TEAM_TERRORIST )
					{
						if ( pTempPlayer != pPlayer )
							ClientPrint( pTempPlayer->pev, HUD_PRINTCENTER, "#Game_bomb_pickup", STRING( pPlayer->pev->netname ) );

						MESSAGE_BEGIN( MSG_ONE, gmsgBombPickup, NULL, pTempPlayer->pev );
						MESSAGE_END();
					}
				}
			}

			if ( i > 0 && i < 3 && pPlayer->m_rgpPlayerItems[ i ] )
			{
				bRemove = false;
				pItem = m_rgpPlayerItems[ i ]->m_pNext;	// sic: the slot's first weapon's next, whichever was looked at
			}
			else
			{
				bEmitSound = true;

				if ( pPlayer->AddPlayerItem( pItem ) )
				{
					pItem->AttachToPlayer( pPlayer );
				}

				// unlink it from the box (sic: after AddPlayerItem has linked it into the player's list)
				pItem = m_rgpPlayerItems[ i ] = m_rgpPlayerItems[ i ]->m_pNext;
			}
		}
	}

	// dole out ammo, if the box goes
	if ( bRemove )
	{
		for ( i = 0 ; i < MAX_AMMO_SLOTS ; i++ )
		{
			if ( !FStringNull( m_rgiszAmmo[ i ] ) )
			{
				// there's some ammo of this type. 
				pPlayer->GiveAmmo( m_rgAmmo[ i ], (char *)STRING( m_rgiszAmmo[ i ] ), MaxAmmoCarry( m_rgiszAmmo[ i ] ) );

				//ALERT ( at_console, "Gave %d rounds of %s\n", m_rgAmmo[i], STRING(m_rgiszAmmo[i]) );

				// now empty the ammo from the weaponbox since we just gave it to the player
				m_rgiszAmmo[ i ] = iStringNull;
				m_rgAmmo[ i ] = 0;
			}
		}
	}

	if ( bEmitSound )
	{
		EMIT_SOUND( pOther->edict(), CHAN_ITEM, "items/gunpickup2.wav", 1, ATTN_NORM );
	}

	if ( bRemove )
	{
		SetTouch(NULL);
		UTIL_Remove(this);
	}
}

//=========================================================
// CWeaponBox - PackWeapon: Add this weapon to the box
//=========================================================
BOOL CWeaponBox::PackWeapon( CBasePlayerItem *pWeapon )
{
	// is one of these weapons already packed in this box?
	if ( HasWeapon( pWeapon ) )
	{
		return FALSE;// box can only hold one of each weapon type
	}

	if ( pWeapon->m_pPlayer )
	{
		// Counter-Strike Beta 5.0: the player's weapon in hand is put away first
		if ( pWeapon->m_pPlayer->m_pActiveItem == pWeapon )
			pWeapon->Holster( );

		if ( !pWeapon->m_pPlayer->RemovePlayerItem( pWeapon ) )
		{
			// failed to unhook the weapon from the player!
			return FALSE;
		}
	}

	int iWeaponSlot = pWeapon->iItemSlot();
	
	if ( m_rgpPlayerItems[ iWeaponSlot ] )
	{
		// there's already one weapon in this slot, so link this into the slot's column
		pWeapon->m_pNext = m_rgpPlayerItems[ iWeaponSlot ];	
		m_rgpPlayerItems[ iWeaponSlot ] = pWeapon;
	}
	else
	{
		// first weapon we have for this slot
		m_rgpPlayerItems[ iWeaponSlot ] = pWeapon;
		pWeapon->m_pNext = NULL;	
	}

	pWeapon->pev->spawnflags |= SF_NORESPAWN;// never respawn
	pWeapon->pev->movetype = MOVETYPE_NONE;
	pWeapon->pev->solid = SOLID_NOT;
	pWeapon->pev->effects = EF_NODRAW;
	pWeapon->pev->modelindex = 0;
	pWeapon->pev->model = iStringNull;
	pWeapon->pev->owner = edict();
	pWeapon->SetThink( NULL );// crowbar may be trying to swing again, etc.
	pWeapon->SetTouch( NULL );
	pWeapon->m_pPlayer = NULL;

	//ALERT ( at_console, "packed %s\n", STRING(pWeapon->pev->classname) );

	return TRUE;
}

//=========================================================
// CWeaponBox - PackAmmo
//=========================================================
BOOL CWeaponBox::PackAmmo( int iszName, int iCount )
{
	int iMaxCarry;

	if ( FStringNull( iszName ) )
	{
		// error here
		ALERT ( at_console, "NULL String in PackAmmo!\n" );
		return FALSE;
	}
	
	iMaxCarry = MaxAmmoCarry( iszName );

	if ( iMaxCarry != -1 && iCount > 0 )
	{
		//ALERT ( at_console, "Packed %d rounds of %s\n", iCount, STRING(iszName) );
		GiveAmmo( iCount, (char *)STRING( iszName ), iMaxCarry );
		return TRUE;
	}

	return FALSE;
}

//=========================================================
// CWeaponBox - GiveAmmo
//=========================================================
int CWeaponBox::GiveAmmo( int iCount, char *szName, int iMax, int *pIndex/* = NULL*/ )
{
	int i;

	for (i = 1; i < MAX_AMMO_SLOTS && !FStringNull( m_rgiszAmmo[i] ); i++)
	{
		if (stricmp( szName, STRING( m_rgiszAmmo[i])) == 0)
		{
			if (pIndex)
				*pIndex = i;

			int iAdd = min( iCount, iMax - m_rgAmmo[i]);
			if (iCount == 0 || iAdd > 0)
			{
				m_rgAmmo[i] += iAdd;

				return i;
			}
			return -1;
		}
	}
	if (i < MAX_AMMO_SLOTS)
	{
		if (pIndex)
			*pIndex = i;

		m_rgiszAmmo[i] = MAKE_STRING( szName );
		m_rgAmmo[i] = iCount;

		return i;
	}
	ALERT( at_console, "out of named ammo slots\n");
	return i;
}

//=========================================================
// CWeaponBox::HasWeapon - is a weapon of this type already
// packed in this box?
//=========================================================
BOOL CWeaponBox::HasWeapon( CBasePlayerItem *pCheckItem )
{
	CBasePlayerItem *pItem = m_rgpPlayerItems[pCheckItem->iItemSlot()];

	while (pItem)
	{
		if (FClassnameIs( pItem->pev, STRING( pCheckItem->pev->classname) ))
		{
			return TRUE;
		}
		pItem = pItem->m_pNext;
	}

	return FALSE;
}

//=========================================================
// CWeaponBox::IsEmpty - is there anything in this box?
//=========================================================
BOOL CWeaponBox::IsEmpty( void )
{
	int i;

	for ( i = 0 ; i < MAX_ITEM_TYPES ; i++ )
	{
		if ( m_rgpPlayerItems[ i ] )
		{
			return FALSE;
		}
	}

	for ( i = 0 ; i < MAX_AMMO_SLOTS ; i++ )
	{
		if ( !FStringNull( m_rgiszAmmo[ i ] ) )
		{
			// still have a bit of this type of ammo
			return FALSE;
		}
	}

	return TRUE;
}

//=========================================================
//=========================================================
void CWeaponBox::SetObjectCollisionBox( void )
{
	pev->absmin = pev->origin + Vector(-16, -16, 0);
	pev->absmax = pev->origin + Vector(16, 16, 16); 
}

//=========================================================
// Counter-Strike Beta 6.0: armoury_entity (Linux symbols: CArmoury,
// ArmouryTouch, KeyValue, Spawn, Restart, Precache), a stack of one kind
// of weapon or equipment lying in the map ("item" 0 to 18, "count").
// Each touch gives a living player one: a gun with ammunition only to
// one without a primary weapon, a flashbang to one with fewer than two,
// an HE grenade or (Beta 6.5) a smoke grenade to one with none, armour to
// one without that armour; the VIP takes nothing. An empty stack
// disappears until the next round, when Restart refills it in proportion
// to the terrorists playing. Names of the members ours.
//=========================================================
class CArmoury : public CBaseEntity
{
public:
	void Spawn( void )
	{
		Precache();

		pev->movetype = MOVETYPE_TOSS;
		pev->solid = SOLID_TRIGGER;
		UTIL_SetSize( pev, Vector( -16, -16, 0 ), Vector( 16, 16, 16 ) );
		UTIL_SetOrigin( pev, pev->origin );
		SetTouch( ArmouryTouch );

		switch ( m_iItem )
		{
		case 0:	// mp5
			SET_MODEL( ENT( pev ), "models/w_mp5.mdl" );
			break;
		case 1:	// tmp
			SET_MODEL( ENT( pev ), "models/w_tmp.mdl" );
			break;
		case 2:	// p90
			SET_MODEL( ENT( pev ), "models/w_p90.mdl" );
			break;
		case 3:	// mac10
			SET_MODEL( ENT( pev ), "models/w_mac10.mdl" );
			break;
		case 4:	// ak47
			SET_MODEL( ENT( pev ), "models/w_ak47.mdl" );
			break;
		case 5:	// sg552
			SET_MODEL( ENT( pev ), "models/w_sg552.mdl" );
			break;
		case 6:	// m4a1
			SET_MODEL( ENT( pev ), "models/w_m4a1.mdl" );
			break;
		case 7:	// aug
			SET_MODEL( ENT( pev ), "models/w_aug.mdl" );
			break;
		case 8:	// scout
			SET_MODEL( ENT( pev ), "models/w_scout.mdl" );
			break;
		case 9:	// g3sg1
			SET_MODEL( ENT( pev ), "models/w_g3sg1.mdl" );
			break;
		case 10:	// awp
			SET_MODEL( ENT( pev ), "models/w_awp.mdl" );
			break;
		case 11:	// m3
			SET_MODEL( ENT( pev ), "models/w_m3.mdl" );
			break;
		case 12:	// xm1014
			SET_MODEL( ENT( pev ), "models/w_xm1014.mdl" );
			break;
		case 13:	// m249
			SET_MODEL( ENT( pev ), "models/w_m249.mdl" );
			break;
		case 14:	// flashbang
			SET_MODEL( ENT( pev ), "models/w_flashbang.mdl" );
			break;
		case 15:	// hegrenade
			SET_MODEL( ENT( pev ), "models/w_hegrenade.mdl" );
			break;
		case 16:	// kevlar
			SET_MODEL( ENT( pev ), "models/w_kevlar.mdl" );
			break;
		case 17:	// assault
			SET_MODEL( ENT( pev ), "models/w_assault.mdl" );
			break;
		case 18:	// smokegrenade (Beta 6.5)
			SET_MODEL( ENT( pev ), "models/w_smokegrenade.mdl" );
			break;
		default:
			SET_MODEL( ENT( pev ), "models/w_kevlar.mdl" );
			break;
		}

		if ( m_iCount <= 0 )
			m_iCount = 1;

		m_bAlreadyCounted = false;
		m_iInitialCount = m_iCount;
	}

	// a new round: the first one counts the map's items; then each stack
	// gets its share (its count over the total of its kind, an integer
	// division, sic) times the terrorists playing times 0.85 for guns, 1.75 for
	// flashbangs and HE grenades, 1 for armour (at least one). Beta 6.1: 0.7,
	// 1.75, 0.7. The smoke grenade (18) counts as a gun.
	void Restart( void )
	{
		CHalfLifeMultiplay *mp = (CHalfLifeMultiplay *)g_pGameRules;	// name ours

		if ( m_iItem == 14 || m_iItem == 15 )
		{
			if ( !m_bAlreadyCounted )
			{
				m_bAlreadyCounted = true;
				mp->m_iTotalGrenadeCount += m_iInitialCount;
				m_iCount = m_iInitialCount;
			}
			else
			{
				m_iCount = (float)( m_iInitialCount / mp->m_iTotalGrenadeCount ) * mp->m_iNumTerrorist * 1.75;	// Beta 6.1 (Beta 6.0: 1.5)

				if ( m_iCount < 1 )
					m_iCount = 1;
			}
		}
		else if ( m_iItem == 16 || m_iItem == 17 )
		{
			if ( !m_bAlreadyCounted )
			{
				m_bAlreadyCounted = true;
				mp->m_iTotalArmourCount += m_iInitialCount;
				m_iCount = m_iInitialCount;
			}
			else
			{
				m_iCount = (float)( m_iInitialCount / mp->m_iTotalArmourCount ) * mp->m_iNumTerrorist;	// Beta 6.5 (Beta 6.1: times 0.7)

				if ( m_iCount < 1 )
					m_iCount = 1;
			}
		}
		else
		{
			if ( !m_bAlreadyCounted )
			{
				m_bAlreadyCounted = true;
				mp->m_iTotalGunCount += m_iInitialCount;
				m_iCount = m_iInitialCount;
			}
			else
			{
				m_iCount = (float)( m_iInitialCount / mp->m_iTotalGunCount ) * mp->m_iNumTerrorist * 0.85;	// Beta 6.5 (Beta 6.1: 0.7)

				if ( m_iCount < 1 )
					m_iCount = 1;
			}
		}

		pev->effects &= ~EF_NODRAW;
	}

	void Precache( void )
	{
		switch ( m_iItem )
		{
		case 0:	// mp5
			PRECACHE_MODEL( "models/w_mp5.mdl" );
			break;
		case 1:	// tmp
			PRECACHE_MODEL( "models/w_tmp.mdl" );
			break;
		case 2:	// p90
			PRECACHE_MODEL( "models/w_p90.mdl" );
			break;
		case 3:	// mac10
			PRECACHE_MODEL( "models/w_mac10.mdl" );
			break;
		case 4:	// ak47
			PRECACHE_MODEL( "models/w_ak47.mdl" );
			break;
		case 5:	// sg552
			PRECACHE_MODEL( "models/w_sg552.mdl" );
			break;
		case 6:	// m4a1
			PRECACHE_MODEL( "models/w_m4a1.mdl" );
			break;
		case 7:	// aug
			PRECACHE_MODEL( "models/w_aug.mdl" );
			break;
		case 8:	// scout
			PRECACHE_MODEL( "models/w_scout.mdl" );
			break;
		case 9:	// g3sg1
			PRECACHE_MODEL( "models/w_g3sg1.mdl" );
			break;
		case 10:	// awp
			PRECACHE_MODEL( "models/w_awp.mdl" );
			break;
		case 11:	// m3
			PRECACHE_MODEL( "models/w_m3.mdl" );
			break;
		case 12:	// xm1014
			PRECACHE_MODEL( "models/w_xm1014.mdl" );
			break;
		case 13:	// m249
			PRECACHE_MODEL( "models/w_m249.mdl" );
			break;
		case 14:	// flashbang
			PRECACHE_MODEL( "models/w_flashbang.mdl" );
			break;
		case 15:	// hegrenade
			PRECACHE_MODEL( "models/w_hegrenade.mdl" );
			break;
		case 16:	// kevlar
			PRECACHE_MODEL( "models/w_kevlar.mdl" );
			break;
		case 17:	// assault
			PRECACHE_MODEL( "models/w_assault.mdl" );
			break;
		case 18:	// smokegrenade (Beta 6.5)
			PRECACHE_MODEL( "models/w_smokegrenade.mdl" );
			break;
		default:
			PRECACHE_MODEL( "models/w_kevlar.mdl" );
			break;
		}
	}

	void KeyValue( KeyValueData *pkvd );
	void EXPORT ArmouryTouch( CBaseEntity *pOther )
	{
		if ( !pOther->IsPlayer() )
			return;

		CBasePlayer *pPlayer = (CBasePlayer *)pOther;	// name ours

		if ( pPlayer->m_bIsVIP == true )
			return;

		if ( m_iCount > 0 && m_iItem <= 13 )
		{
			if ( pPlayer->m_bHasPrimary == true )
				return;

			m_iCount--;

			switch ( m_iItem )
			{
			case 0:
				pPlayer->GiveNamedItem( "weapon_mp5navy" );
				pPlayer->GiveAmmo( 60, "9mm", 120 );
				break;
			case 1:
				pPlayer->GiveNamedItem( "weapon_tmp" );
				pPlayer->GiveAmmo( 60, "9mm", 120 );
				break;
			case 2:
				pPlayer->GiveNamedItem( "weapon_p90" );
				pPlayer->GiveAmmo( 50, "57mm", 100 );
				break;
			case 3:
				pPlayer->GiveNamedItem( "weapon_mac10" );
				pPlayer->GiveAmmo( 60, "45acp", 100 );
				break;
			case 4:
				pPlayer->GiveNamedItem( "weapon_ak47" );
				pPlayer->GiveAmmo( 60, "762Nato", 90 );
				break;
			case 5:
				pPlayer->GiveNamedItem( "weapon_sg552" );
				pPlayer->GiveAmmo( 60, "556Nato", 90 );
				break;
			case 6:
				pPlayer->GiveNamedItem( "weapon_m4a1" );
				pPlayer->GiveAmmo( 60, "556Nato", 90 );
				break;
			case 7:
				pPlayer->GiveNamedItem( "weapon_aug" );
				pPlayer->GiveAmmo( 60, "556Nato", 90 );
				break;
			case 8:
				pPlayer->GiveNamedItem( "weapon_scout" );
				pPlayer->GiveAmmo( 30, "762Nato", 90 );
				break;
			case 9:
				pPlayer->GiveNamedItem( "weapon_g3sg1" );
				pPlayer->GiveAmmo( 30, "762Nato", 90 );
				break;
			case 10:
				pPlayer->GiveNamedItem( "weapon_awp" );
				pPlayer->GiveAmmo( 20, "338Magnum", 30 );
				break;
			case 11:
				pPlayer->GiveNamedItem( "weapon_m3" );
				pPlayer->GiveAmmo( 24, "buckshot", 32 );
				break;
			case 12:
				pPlayer->GiveNamedItem( "weapon_xm1014" );
				pPlayer->GiveAmmo( 24, "buckshot", 32 );
				break;
			case 13:
				pPlayer->GiveNamedItem( "weapon_m249" );
				pPlayer->GiveAmmo( 60, "556NatoBox", 200 );	// Beta 6.1: the M249's own ammunition now
				break;
			}
		}
		else if ( m_iCount > 0 && m_iItem >= 14 )
		{
			switch ( m_iItem )
			{
			case 14:
				if ( pPlayer->AmmoInventory( CBasePlayer::GetAmmoIndex( "Flashbang" ) ) >= 2 )
					return;

				pPlayer->GiveNamedItem( "weapon_flashbang" );
				m_iCount--;
				break;
			case 15:
				if ( pPlayer->AmmoInventory( CBasePlayer::GetAmmoIndex( "HEGrenade" ) ) >= 1 )
					return;

				pPlayer->GiveNamedItem( "weapon_hegrenade" );
				m_iCount--;
				break;
			case 16:
				if ( pPlayer->m_iKevlar == 1 )
					return;

				pPlayer->GiveNamedItem( "item_kevlar" );
				m_iCount--;
				break;
			case 17:
				if ( pPlayer->m_iKevlar == 2 )
					return;

				pPlayer->GiveNamedItem( "item_assaultsuit" );
				m_iCount--;
				break;
			case 18:	// Beta 6.5
				if ( pPlayer->AmmoInventory( CBasePlayer::GetAmmoIndex( "SmokeGrenade" ) ) >= 1 )
					return;

				pPlayer->GiveNamedItem( "weapon_smokegrenade" );
				m_iCount--;
				break;
			}
		}

		if ( m_iCount == 0 )
			pev->effects |= EF_NODRAW;
	}

	int		m_iItem;			// 0x7c "item": 0 MP5 Navy, 1 TMP, 2 P90, 3 MAC-10, 4 AK-47, 5 SG-552, 6 M4A1, 7 AUG, 8 Scout, 9 G3/SG-1, 10 AWP, 11 M3, 12 XM1014, 13 M249, 14 flashbang, 15 HE grenade, 16 kevlar, 17 assault suit, 18 smoke grenade
	int		m_iCount;			// 0x80 "count": how many are left
	int		m_iInitialCount;	// 0x84 the count the map gave (Spawn)
	bool	m_bAlreadyCounted;	// 0x88 the first Restart added the count to the game rules' totals
};


void CArmoury::KeyValue( KeyValueData *pkvd )
{
	if ( FStrEq( pkvd->szKeyName, "item" ) )
	{
		m_iItem = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "count" ) )
	{
		m_iCount = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		CBaseEntity::KeyValue( pkvd );
}

LINK_ENTITY_TO_CLASS( armoury_entity, CArmoury );

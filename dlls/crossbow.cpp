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
#if !defined( OEM_BUILD ) && !defined( HLDEMO_BUILD )

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"
#include "gamerules.h"

extern void ExpandHorseHulls( int fExpand );	// name ours; horses and pumas are traced with big boxes

// Wanted: the bow. Arrows are crossbow bolts that fly a ballistic arc
// (MOVETYPE_BOUNCE with half gravity), face where they fly, and stick in
// the world; held longer, the bow shoots them faster.

#define BOLT_AIR_VELOCITY	2000
#define BOLT_WATER_VELOCITY	1000

// UNDONE: Save/restore this?  Don't forget to set classname and LINK_ENTITY_TO_CLASS()
//
// OVERLOADS SOME ENTVARS:
//
// speed - the ideal magnitude of my velocity
class CCrossbowBolt : public CBaseEntity
{
	void Spawn( void );
	void Precache( void );
	int  Classify ( void );
	void EXPORT BubbleThink( void );
	void EXPORT BoltTouch( CBaseEntity *pOther );
	void EXPORT ExplodeThink( void );
	void EXPORT AirThink( void );

	int m_iTrail;

public:
	static CCrossbowBolt *BoltCreate( void );
};
LINK_ENTITY_TO_CLASS( crossbow_bolt, CCrossbowBolt );

CCrossbowBolt *CCrossbowBolt::BoltCreate( void )
{
	// Create a new entity with CCrossbowBolt private data
	CCrossbowBolt *pBolt = GetClassPtr( (CCrossbowBolt *)NULL );
	pBolt->pev->classname = MAKE_STRING("bolt");
	pBolt->Spawn();

	return pBolt;
}

void CCrossbowBolt::Spawn( )
{
	Precache( );
	pev->movetype = MOVETYPE_BOUNCE;
	pev->solid = SOLID_BBOX;

	pev->gravity = 0.5;

	SET_MODEL(ENT(pev), "models/arrow.mdl");

	UTIL_SetOrigin( pev, pev->origin );
	UTIL_SetSize(pev, Vector(0, 0, 0), Vector(0, 0, 0));

	SetTouch( BoltTouch );
	AirThink( );
	SetThink( BubbleThink );
}


void CCrossbowBolt::Precache( )
{
	PRECACHE_MODEL ("models/arrow.mdl");
	PRECACHE_SOUND("weapons/bow_hitbod1.wav");
	PRECACHE_SOUND("weapons/bow_hitbod2.wav");
	PRECACHE_SOUND("weapons/bow_fly1.wav");
	PRECACHE_SOUND("weapons/bow_hit1.wav");
	PRECACHE_SOUND("fvox/beep.wav");
	m_iTrail = PRECACHE_MODEL("sprites/streak.spr");
}


int	CCrossbowBolt :: Classify ( void )
{
	return	CLASS_NONE;
}

void CCrossbowBolt::BoltTouch( CBaseEntity *pOther )
{
	SetTouch( NULL );
	SetThink( NULL );

	if (pOther->pev->takedamage)
	{
		TraceResult tr = UTIL_GetGlobalTrace( );
		entvars_t	*pevOwner;

		pevOwner = VARS( pev->owner );

		// UNDONE: this needs to call TraceAttack instead
		ClearMultiDamage( );

		pOther->TraceAttack(pevOwner, gSkillData.plrDmgBow, pev->velocity.Normalize(), &tr, DMG_NEVERGIB );

		ApplyMultiDamage( pev, pevOwner );

		pev->velocity = Vector( 0, 0, 0 );
		// play body "thwack" sound
		switch( RANDOM_LONG(0,1) )
		{
		case 0:
			EMIT_SOUND(ENT(pev), CHAN_WEAPON, "weapons/bow_hitbod1.wav", 1, ATTN_NORM); break;
		case 1:
			EMIT_SOUND(ENT(pev), CHAN_WEAPON, "weapons/bow_hitbod2.wav", 1, ATTN_NORM); break;
		}

		Killed( pev, GIB_NEVER );
	}
	else
	{
		EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "weapons/bow_hit1.wav", RANDOM_FLOAT(0.95, 1.0), ATTN_NORM, 0, 98 + RANDOM_LONG(0,7));

		SetThink( SUB_Remove );
		pev->nextthink = gpGlobals->time;// this will get changed below if the bolt is allowed to stick in what it hit.

		if ( FClassnameIs( pOther->pev, "worldspawn" ) )
		{
			// if what we hit is static architecture, can stay around for a while.
			Vector vecDir = pev->velocity.Normalize( );
			UTIL_SetOrigin( pev, pev->origin - vecDir * 12 );
			pev->angles = UTIL_VecToAngles( vecDir );
			pev->solid = SOLID_NOT;
			pev->movetype = MOVETYPE_FLY;
			pev->velocity = Vector( 0, 0, 0 );
			pev->avelocity.z = 0;
			pev->angles.z = RANDOM_LONG(0,360);
			pev->nextthink = gpGlobals->time + 10.0;
		}

		if (UTIL_PointContents(pev->origin) != CONTENTS_WATER)
		{
			UTIL_Sparks( pev->origin );
		}
	}
}

void CCrossbowBolt::BubbleThink( void )
{
	pev->nextthink = gpGlobals->time + 0.1;

	AirThink( );

	if (pev->waterlevel == 0)
		return;

	UTIL_BubbleTrail( pev->origin - pev->velocity * 0.1, pev->origin, 1 );
}

// Wanted: point along the flight path, and hit horses, which aren't
// solid to the bolt (only traces see their big boxes)
void CCrossbowBolt::AirThink( void )
{
	Vector vecForward, vecEnd;
	TraceResult tr;

	pev->angles = UTIL_VecToAngles( pev->velocity );
	g_engfuncs.pfnAngleVectors( pev->angles, vecForward, NULL, NULL );

	vecEnd = pev->origin + vecForward * 30;

	ExpandHorseHulls( TRUE );
	UTIL_TraceLine( pev->origin, vecEnd, dont_ignore_monsters, ENT( pev ), &tr );
	ExpandHorseHulls( FALSE );

	if ( tr.flFraction < 1.0 )
	{
		pev->origin = tr.vecEndPos;

		CBaseEntity *pEntity = CBaseEntity::Instance( tr.pHit );
		if ( pEntity && FClassnameIs( pEntity->pev, "monster_horse" ) )
			Touch( pEntity );
	}

	pev->nextthink = gpGlobals->time;
}

void CCrossbowBolt::ExplodeThink( void )
{
	int iContents = UTIL_PointContents ( pev->origin );
	int iScale;

	pev->dmg = 40;
	iScale = 10;

	MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, pev->origin );
		WRITE_BYTE( TE_EXPLOSION);
		WRITE_COORD( pev->origin.x );
		WRITE_COORD( pev->origin.y );
		WRITE_COORD( pev->origin.z );
		if (iContents != CONTENTS_WATER)
		{
			WRITE_SHORT( g_sModelIndexFireball );
		}
		else
		{
			WRITE_SHORT( g_sModelIndexWExplosion );
		}
		WRITE_BYTE( iScale  ); // scale * 10
		WRITE_BYTE( 15  ); // framerate
		WRITE_BYTE( TE_EXPLFLAG_NONE );
	MESSAGE_END();

	entvars_t *pevOwner;

	if ( pev->owner )
		pevOwner = VARS( pev->owner );
	else
		pevOwner = NULL;

	pev->owner = NULL; // can't traceline attack owner if this is set

	::RadiusDamage( pev->origin, pev, pevOwner, pev->dmg, 128, CLASS_NONE, DMG_BLAST | DMG_ALWAYSGIB );

	UTIL_Remove(this);
}


// sequences of models/v_bow.mdl (names ours)
enum bow_e {
	BOW_IDLE1 = 0,	// empty
	BOW_IDLE2,		// empty
	BOW_FIDGET1,	// loaded
	BOW_FIDGET2,	// loaded
	BOW_UNUSED4,
	BOW_UNUSED5,
	BOW_DRAW,		// deploy
	BOW_HOLSTER,
	BOW_RELOAD,
	BOW_PULL,		// start drawing the string
	BOW_UNUSED10,
	BOW_FIRE,
	BOW_EMPTY,		// no arrows
};


class CCrossbow : public CBasePlayerWeapon
{
public:
	void Spawn( void );
	void Precache( void );
	int iItemSlot( ) { return 3; }
	int GetItemInfo(ItemInfo *p);

	void PrimaryAttack( void );
	int AddToPlayer( CBasePlayer *pPlayer );
	BOOL Deploy( );
	BOOL CanHolster( void );
	void Holster( int skiplocal = 0 );
	void Reload( void );
	void WeaponIdle( void );

	// Wanted: hold to draw (names ours)
	float m_flStartDraw;	// when the string was pulled; 0 while not drawing
	float m_flFullDraw;		// when the pull is complete
	float m_flIdleStart;	// set, never read
	float m_flEmptySound;	// when to play the empty sound
	int m_iAirSpeed;		// arrow speed for this shot
	int m_iWaterSpeed;		// arrow speed under water
};
LINK_ENTITY_TO_CLASS( weapon_bow, CCrossbow );

void CCrossbow::Spawn( )
{
	Precache( );
	m_iId = WEAPON_BOW;
	SET_MODEL(ENT(pev), "models/w_bow.mdl");

	m_iDefaultAmmo = 5;

	FallInit();// get ready to fall down.
}

int CCrossbow::AddToPlayer( CBasePlayer *pPlayer )
{
	if ( CBasePlayerWeapon::AddToPlayer( pPlayer ) )
	{
		MESSAGE_BEGIN( MSG_ONE, gmsgWeapPickup, NULL, pPlayer->pev );
			WRITE_BYTE( m_iId );
		MESSAGE_END();
		return TRUE;
	}
	return FALSE;
}

void CCrossbow::Precache( void )
{
	PRECACHE_MODEL("models/w_bow.mdl");
	PRECACHE_MODEL("models/v_bow.mdl");
	PRECACHE_MODEL("models/p_bow.mdl");

	PRECACHE_SOUND("weapons/bow_fire1.wav");
	PRECACHE_SOUND("weapons/bow_reload1.wav");
	PRECACHE_SOUND("weapons/bow_empty.wav");

	UTIL_PrecacheOther( "crossbow_bolt" );
}


int CCrossbow::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "arrows";
	p->iMaxAmmo1 = 35;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = 1;
	p->iSlot = 2;
	p->iPosition = 3;
	p->iId = WEAPON_BOW;
	p->iFlags = 0;
	p->iWeight = CROSSBOW_WEIGHT;
	return 1;
}


BOOL CCrossbow::Deploy( )
{
	m_flIdleStart = -1;
	return DefaultDeploy( "models/v_bow.mdl", "models/p_bow.mdl", BOW_DRAW, "bow" );
}

BOOL CCrossbow::CanHolster( void )
{
	// can only holster with the string slack
	return ( m_flStartDraw == 0 );
}

void CCrossbow::Holster( int skiplocal /* = 0 */ )
{
	m_fInReload = FALSE;// cancel any reload in progress.

	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;
	SendWeaponAnim( BOW_HOLSTER );
}

void CCrossbow::PrimaryAttack( void )
{
	if ( m_iClip == 0 && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 )
	{
		SendWeaponAnim( BOW_EMPTY );
		m_flEmptySound = gpGlobals->time + 1.29;
		m_flTimeWeaponIdle = gpGlobals->time + 1.3;
		m_flNextPrimaryAttack = gpGlobals->time + 1.611;
		return;
	}

	if ( m_flStartDraw == 0 && m_iClip > 0 )
	{
		m_flStartDraw = gpGlobals->time;
		m_flIdleStart = 0;
		m_flFullDraw = gpGlobals->time + 0.55;
		SendWeaponAnim( BOW_PULL );
		m_flTimeWeaponIdle = gpGlobals->time + 0.5;
	}
}


void CCrossbow::Reload( void )
{
	if (DefaultReload( 1, BOW_RELOAD, 0.633 ))
	{
		EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/bow_reload1.wav", RANDOM_FLOAT(0.95, 1.0), ATTN_NORM, 0, 93 + RANDOM_LONG(0,0xF));
	}
}


void CCrossbow::WeaponIdle( void )
{
	m_pPlayer->m_iFOV = 0;

	if ( m_flEmptySound != 0 && m_flEmptySound < gpGlobals->time )
	{
		m_flEmptySound = 0;
		EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/bow_empty.wav", RANDOM_FLOAT(0.95, 1.0), ATTN_NORM, 0, 93 + RANDOM_LONG(0,0xF));
		m_flTimeWeaponIdle = gpGlobals->time + 0.2;
		m_flNextPrimaryAttack = gpGlobals->time + 0.2;
		return;
	}

	if ( m_flIdleStart == 0 )
		m_flIdleStart = gpGlobals->time;

	if ( m_flTimeWeaponIdle > gpGlobals->time )
		return;

	ResetEmptySound( );
	m_pPlayer->GetAutoaimVector( AUTOAIM_2DEGREES );  // get the autoaim vector but ignore it;  used for autoaim crosshair in DM

	if ( m_flTimeWeaponIdle > gpGlobals->time )
		return;

	if ( m_flStartDraw )
	{
		// release: shoot the arrow
		m_pPlayer->m_iWeaponVolume = QUIET_GUN_VOLUME;
		m_iClip--;

		// make twang sound
		EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/bow_fire1.wav", RANDOM_FLOAT(0.95, 1.0), ATTN_NORM, 0, 93 + RANDOM_LONG(0,0xF));

		if (m_iClip)
		{
			EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/bow_reload1.wav", RANDOM_FLOAT(0.95, 1.0), ATTN_NORM, 0, 93 + RANDOM_LONG(0,0xF));
			SendWeaponAnim( BOW_FIRE );
		}
		else if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] == 0)
		{
			SendWeaponAnim( BOW_FIRE );
		}

		// player "shoot" animation
		m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

		m_flStartDraw = 0;

		Vector anglesAim = m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle;
		UTIL_MakeVectors( anglesAim );

		anglesAim.x		= -anglesAim.x;
		Vector vecSrc	 = m_pPlayer->GetGunPosition( ) - gpGlobals->v_up * 2;
		Vector vecDir	 = gpGlobals->v_forward;

		CCrossbowBolt *pBolt = CCrossbowBolt::BoltCreate();
		pBolt->pev->origin = vecSrc;
		pBolt->pev->angles = anglesAim;
		pBolt->pev->owner = m_pPlayer->edict();

		// the longer the bow was held, the faster the arrow
		m_iAirSpeed = RANDOM_FLOAT( 0, 250 ) + 500;
		m_iWaterSpeed = RANDOM_FLOAT( 0, 250 ) + 400;

		if ( m_flFullDraw - gpGlobals->time > -1.0 && m_flFullDraw - gpGlobals->time < -0.01 )
		{
			m_iAirSpeed = RANDOM_FLOAT( 0, 200 ) + 1000;
			m_iWaterSpeed = RANDOM_FLOAT( 0, 200 ) + 900;
		}

		if ( m_flFullDraw - gpGlobals->time > -2.0 && m_flFullDraw - gpGlobals->time < -1.0 )
		{
			m_iAirSpeed = RANDOM_FLOAT( 0, 250 ) + 1500;
			m_iWaterSpeed = RANDOM_FLOAT( 0, 250 ) + 1400;
		}

		if ( m_flFullDraw - gpGlobals->time < -2.0 )
		{
			m_iAirSpeed = RANDOM_FLOAT( 0, 250 ) + 1750;
			m_iWaterSpeed = RANDOM_FLOAT( 0, 250 ) + 1650;
		}

		if (m_pPlayer->pev->waterlevel == 3)
		{
			pBolt->pev->velocity = vecDir * m_iWaterSpeed;
			pBolt->pev->speed = m_iWaterSpeed;
		}
		else
		{
			pBolt->pev->velocity = vecDir * m_iAirSpeed;
			pBolt->pev->speed = m_iAirSpeed;
		}
		pBolt->pev->avelocity.z = 50;

		if (!m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
			// HEV suit - indicate out of ammo condition
			m_pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);

		m_flNextPrimaryAttack = gpGlobals->time + 0.75;

		if (m_iClip != 0)
			m_flTimeWeaponIdle = gpGlobals->time + 1.333;
		else
			m_flTimeWeaponIdle = 0.75;

		m_pPlayer->pev->punchangle.x -= 2;
	}

	if ( m_iClip )
	{
		switch ( RANDOM_LONG( 0, 1 ) )
		{
		case 0:
			SendWeaponAnim( BOW_FIDGET1 );
			m_flTimeWeaponIdle = gpGlobals->time + 2.9;
			break;
		case 1:
			SendWeaponAnim( BOW_FIDGET2 );
			m_flTimeWeaponIdle = gpGlobals->time + 4.9;
			break;
		}
	}
	else
	{
		switch ( RANDOM_LONG( 0, 1 ) )
		{
		case 0:
			SendWeaponAnim( BOW_IDLE1 );
			m_flTimeWeaponIdle = gpGlobals->time + 2.9;
			break;
		case 1:
			SendWeaponAnim( BOW_IDLE2 );
			m_flTimeWeaponIdle = gpGlobals->time + 2.9;
			break;
		}
	}
}



class CCrossbowAmmo : public CBasePlayerAmmo
{
	void Spawn( void )
	{
		Precache( );
		SET_MODEL(ENT(pev), "models/w_bowammo.mdl");
		CBasePlayerAmmo::Spawn( );
	}
	void Precache( void )
	{
		PRECACHE_MODEL ("models/w_bowammo.mdl");
		PRECACHE_SOUND("items/9mmclip1.wav");
	}
	BOOL AddAmmo( CBaseEntity *pOther )
	{
		if (pOther->GiveAmmo( 5, "arrows", 35 ) != -1)
		{
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM);
			return TRUE;
		}
		return FALSE;
	}
};
LINK_ENTITY_TO_CLASS( ammo_bow, CCrossbowAmmo );



#endif

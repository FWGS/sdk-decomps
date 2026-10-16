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

// Wanted: the cannon (weapon_cannon), its ball (cannon_ball) and its ammo
// (ammo_cannon), written from the crossbow. Pulling the trigger lights the
// fuse; the ball leaves 1.3 s later, kicks the player back and explodes
// where it lands. The file name is ours: retail's code sits between
// buttons.obj and cbase.obj.

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"
#include "gamerules.h"
#include "decals.h"

extern void ExpandHorseHulls( int fExpand );	// name ours; horses and pumas are traced with big boxes

class CCannonBall : public CBaseEntity
{
	void Spawn( void );
	void Precache( void );
	int  Classify ( void );
	void EXPORT BallTouch( CBaseEntity *pOther );
	void EXPORT ExplodeThink( void );
	void AirThink( void );	// name ours

	int m_iTrail;

public:
	static CCannonBall *BallCreate( void );	// name ours
};
LINK_ENTITY_TO_CLASS( cannon_ball, CCannonBall );

CCannonBall *CCannonBall::BallCreate( void )
{
	// Create a new entity with CCannonBall private data
	CCannonBall *pBall = GetClassPtr( (CCannonBall *)NULL );
	pBall->pev->classname = MAKE_STRING("ball");
	pBall->Spawn();

	return pBall;
}

void CCannonBall::Spawn( )
{
	Precache( );
	pev->movetype = MOVETYPE_TOSS;
	pev->solid = SOLID_BBOX;

	pev->gravity = 0.5;

	SET_MODEL(ENT(pev), "models/cannonball.mdl");

	// sic: meant to put the ball at the muzzle, but the cannon only sets
	// the origin after Spawn, so this offsets the world origin
	UTIL_SetOrigin( pev, pev->origin + gpGlobals->v_up * 20 + gpGlobals->v_forward * -2 + gpGlobals->v_right * 30 );
	UTIL_SetSize(pev, Vector(-5, -5, -5), Vector(5, 5, 5));

	SetTouch( BallTouch );
	AirThink( );
}


void CCannonBall::Precache( )
{
	PRECACHE_MODEL ("models/cannonball.mdl");
	PRECACHE_SOUND("weapons/cannon_empty.wav");
	PRECACHE_SOUND("weapons/cannon_reload1.wav");
	PRECACHE_SOUND("weapons/cannon_fire1.wav");
	m_iTrail = PRECACHE_MODEL("sprites/streak.spr");
}


int	CCannonBall :: Classify ( void )
{
	return	CLASS_NONE;
}

void CCannonBall::BallTouch( CBaseEntity *pOther )
{
	TraceResult tr;
	Vector		vecSpot;// trace starts here!

	SetTouch( NULL );
	SetThink( NULL );

	pev->enemy = pOther->edict();

	vecSpot = pev->origin - pev->velocity.Normalize() * 32;
	UTIL_TraceLine( vecSpot, vecSpot + pev->velocity.Normalize() * 62, ignore_monsters, ENT(pev), &tr );

	SetThink( ExplodeThink );
	UTIL_DecalTrace( &tr, DECAL_SCORCH1 );
	pev->nextthink = gpGlobals->time + 0.1;
}

void CCannonBall::ExplodeThink( void )
{
	int iContents = UTIL_PointContents ( pev->origin );
	int iScale;

	pev->dmg = gSkillData.plrDmgCannonBall;
	iScale = 40;

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

	::RadiusDamage( pev->origin, pev, pevOwner, pev->dmg, 256, CLASS_NONE, DMG_BLAST | DMG_ALWAYSGIB );

	UTIL_Remove(this);
}

// Wanted: face the flight path and touch a horse 30 units ahead (horses
// aren't solid to the ball; only traces see their big boxes), copied from
// the arrow's AirThink. Spawn calls it once (before the ball has an origin
// or a velocity); nothing sets it as the think.
void CCannonBall::AirThink( void )
{
	Vector vecOrigin = pev->origin;
	Vector vecForward, vecEnd;
	TraceResult tr;

	pev->angles = UTIL_VecToAngles( pev->velocity );
	// sic: meant to flip the pitch for AngleVectors, but this squares it and
	// doubles the yaw and roll
	g_engfuncs.pfnAngleVectors( pev->angles + Vector( -( pev->angles.x * 2 ) * pev->angles.x, pev->angles.y, pev->angles.z ), vecForward, NULL, NULL );

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

	pev->origin = vecOrigin;
	pev->nextthink = gpGlobals->time;
}


// sequences of models/v_cannon.mdl (names ours)
enum cannon_e {
	CANNON_DRAW = 0,
	CANNON_HOLSTER,
	CANNON_IDLE1,
	CANNON_IDLE2,
	CANNON_FIRE,
	CANNON_DRYFIRE,
	CANNON_RELOAD,
};


class CCannon : public CBasePlayerWeapon	// name ours
{
public:
	void Spawn( void );
	void Precache( void );
	int iItemSlot( ) { return 4; }
	int GetItemInfo(ItemInfo *p);

	void PrimaryAttack( void );
	int AddToPlayer( CBasePlayer *pPlayer );
	BOOL Deploy( );
	void Holster( int skiplocal = 0 );
	void Reload( void );
	void WeaponIdle( void );

	// names ours
	float m_flFire;		// 1 while the fuse burns: WeaponIdle shoots
	float m_flUnused;	// never used (the class is 0x84 bytes)
};
LINK_ENTITY_TO_CLASS( weapon_cannon, CCannon );

void CCannon::Spawn( )
{
	Precache( );
	m_iId = WEAPON_CANNON;
	SET_MODEL(ENT(pev), "models/w_cannon.mdl");

	m_iDefaultAmmo = 2;

	FallInit();// get ready to fall down.
}

int CCannon::AddToPlayer( CBasePlayer *pPlayer )
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

void CCannon::Precache( void )
{
	PRECACHE_MODEL("models/w_cannon.mdl");
	PRECACHE_MODEL("models/v_cannon.mdl");
	PRECACHE_MODEL("models/p_cannon.mdl");

	PRECACHE_SOUND("weapons/cannon_fire1.wav");
	PRECACHE_SOUND("weapons/cannon_reload1.wav");
	PRECACHE_SOUND("weapons/cannon_empty.wav");

	UTIL_PrecacheOther( "cannon_ball" );
}


int CCannon::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "balls";
	p->iMaxAmmo1 = 8;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = 1;
	p->iSlot = 3;
	p->iPosition = 1;
	p->iId = WEAPON_CANNON;
	p->iFlags = 0;
	p->iWeight = 15;
	return 1;
}


BOOL CCannon::Deploy( )
{
	return DefaultDeploy( "models/v_cannon.mdl", "models/p_cannon.mdl", CANNON_DRAW, "cannon" );
}

void CCannon::Holster( int skiplocal /* = 0 */ )
{
	m_fInReload = FALSE;// cancel any reload in progress.

	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;
	SendWeaponAnim( CANNON_HOLSTER );
}

void CCannon::PrimaryAttack( void )
{
	// no fire under water
	if ( m_pPlayer->pev->waterlevel == 3 )
	{
		EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/cannon_empty.wav", RANDOM_FLOAT(0.95, 1.0), ATTN_NORM, 0, 93 + RANDOM_LONG(0,0xF));
		m_flNextPrimaryAttack = gpGlobals->time + 0.5;
		return;
	}

	if ( m_iClip == 0 )
	{
		EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/cannon_empty.wav", RANDOM_FLOAT(0.95, 1.0), ATTN_NORM, 0, 93 + RANDOM_LONG(0,0xF));
		return;
	}

	// light the fuse; WeaponIdle fires
	m_flFire = 1;
	SendWeaponAnim( CANNON_FIRE );
	m_flTimeWeaponIdle = gpGlobals->time + 1.3;
	m_flNextPrimaryAttack = gpGlobals->time + 3.0;
	m_pPlayer->m_iWeaponVolume = 200;
}


void CCannon::Reload( void )
{
	if (DefaultReload( 1, CANNON_RELOAD, 2.636 ))
	{
		EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/cannon_reload1.wav", 1.0, ATTN_NORM, 0, 85 + RANDOM_LONG(0,0x1f));
	}
}


void CCannon::WeaponIdle( void )
{
	m_pPlayer->m_iFOV = 0;

	ResetEmptySound( );
	m_pPlayer->GetAutoaimVector( AUTOAIM_2DEGREES );  // get the autoaim vector but ignore it;  used for autoaim crosshair in DM

	if ( m_flTimeWeaponIdle > gpGlobals->time )
		return;

	if ( m_flFire == 1 )
	{
		m_flFire = 0;

		EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/cannon_fire1.wav", RANDOM_FLOAT(0.95, 1.0), ATTN_NORM, 0, 93 + RANDOM_LONG(0,0xF));

		// the kick pushes the player back
		UTIL_MakeVectors( m_pPlayer->pev->v_angle );
		m_pPlayer->pev->velocity = Vector( gpGlobals->v_forward * -600 );

		// player "shoot" animation
		m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

		Vector anglesAim = m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle;
		UTIL_MakeVectors( anglesAim );

		anglesAim.x		= -anglesAim.x;
		Vector vecSrc	 = m_pPlayer->GetGunPosition( ) - gpGlobals->v_up * 2;
		Vector vecDir	 = gpGlobals->v_forward;

		CCannonBall *pBall = CCannonBall::BallCreate();
		pBall->pev->origin = vecSrc;
		pBall->pev->angles = anglesAim;
		pBall->pev->owner = m_pPlayer->edict();
		pBall->pev->velocity = vecDir * 1500;
		pBall->pev->speed = 1500;
		pBall->pev->avelocity.z = 50;

		m_pPlayer->pev->effects |= EF_MUZZLEFLASH;

		// smoke at the muzzle (sic: moves the weapon entity there)
		UTIL_SetOrigin( pev, pev->origin + gpGlobals->v_up * 12 + gpGlobals->v_forward * 50 + gpGlobals->v_right * 25 );
		MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, pev->origin );
			WRITE_BYTE( TE_SMOKE );
			WRITE_COORD( pev->origin.x );
			WRITE_COORD( pev->origin.y );
			WRITE_COORD( pev->origin.z );
			WRITE_SHORT( g_sModelIndexSmoke );
			WRITE_BYTE( 30 ); // scale * 10
			WRITE_BYTE( 15 ); // framerate
		MESSAGE_END();

		// sic: tested before the clip goes down, so it never fires
		if (!m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
			// HEV suit - indicate out of ammo condition
			m_pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);

		m_flNextPrimaryAttack = gpGlobals->time + 3.0;
		m_flTimeWeaponIdle = gpGlobals->time + 0.5;
		m_iClip = m_iClip - 1;
	}

	switch ( RANDOM_LONG( 0, 1 ) )
	{
	case 0:
		SendWeaponAnim( CANNON_IDLE1 );
		m_flTimeWeaponIdle = gpGlobals->time + 1.0;
		break;
	case 1:
		SendWeaponAnim( CANNON_IDLE2 );
		m_flTimeWeaponIdle = gpGlobals->time + 2.636;
		break;
	}
}



class CCannonAmmo : public CBasePlayerAmmo	// name ours
{
	void Spawn( void )
	{
		Precache( );
		SET_MODEL(ENT(pev), "models/w_cannonball.mdl");
		CBasePlayerAmmo::Spawn( );
	}
	void Precache( void )
	{
		PRECACHE_MODEL ("models/w_cannonball.mdl");
		PRECACHE_SOUND("items/9mmclip1.wav");
	}
	BOOL AddAmmo( CBaseEntity *pOther )
	{
		if (pOther->GiveAmmo( 2, "balls", 8 ) != -1)
		{
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM);
			return TRUE;
		}
		return FALSE;
	}
};
LINK_ENTITY_TO_CLASS( ammo_cannon, CCannonAmmo );

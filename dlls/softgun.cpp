/***
*
*	Copyright (c) 1996-2001, Valve LLC. All rights reserved.
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
// softgun.cpp - Scientist Slaughterhouse's soft metal accelerator,
// weapon_softgun (file name ours)
//=========================================================

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "weapons.h"
#include "monsters.h"
#include "player.h"
#include "gamerules.h"


#define SOFTGUN_MAX_CLIP		12	// names ours
#define SOFTGUN_DEFAULT_GIVE	20

enum softgun_e {	// v_desert_eagle.mdl's sequences
	SOFTGUN_IDLE1 = 0,
	SOFTGUN_IDLE2,
	SOFTGUN_IDLE3,
	SOFTGUN_IDLE4,
	SOFTGUN_IDLE5,
	SOFTGUN_SHOOT,
	SOFTGUN_SHOOT_EMPTY,
	SOFTGUN_RELOAD,
	SOFTGUN_RELOAD_NOSHOT,
	SOFTGUN_DRAW,
	SOFTGUN_HOLSTER
};

LINK_ENTITY_TO_CLASS( weapon_softgun, CSoftGun );

int CSoftGun::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "uranium";
	p->iMaxAmmo1 = URANIUM_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = SOFTGUN_MAX_CLIP;
	p->iFlags = 0;
	p->iSlot = 1;
	p->iPosition = 3;
	p->iId = m_iId = WEAPON_SOFTGUN;
	p->iWeight = SOFTGUN_WEIGHT;

	return 1;
}

int CSoftGun::AddToPlayer( CBasePlayer *pPlayer )
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

void CSoftGun::Spawn( )
{
	Precache( );
	m_iId = WEAPON_SOFTGUN;
	SET_MODEL(ENT(pev), "models/w_desert_eagle.mdl");

	m_iDefaultAmmo = SOFTGUN_DEFAULT_GIVE;

	FallInit();// get ready to fall down.
}


void CSoftGun::Precache( void )
{
	PRECACHE_MODEL("models/v_desert_eagle.mdl");
	PRECACHE_MODEL("models/w_desert_eagle.mdl");
	PRECACHE_MODEL("models/p_desert_eagle.mdl");

	PRECACHE_SOUND("items/9mmclip1.wav");
	PRECACHE_SOUND("items/9mmclip2.wav");

	m_usSoftGun = PRECACHE_EVENT( 1, "events/softgun.sc" );
}

BOOL CSoftGun::Deploy( )
{
	return DefaultDeploy( "models/v_desert_eagle.mdl", "models/p_desert_eagle.mdl", SOFTGUN_DRAW, "onehanded" );
}


void CSoftGun::Holster( int skiplocal /* = 0 */ )
{
	m_fInReload = FALSE;// cancel any reload in progress.

	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 1.0;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
	SendWeaponAnim( SOFTGUN_HOLSTER );
}

void CSoftGun::PrimaryAttack()
{
	// don't fire underwater
	if (m_pPlayer->pev->waterlevel == 3)
	{
		PlayEmptySound( );
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.15;
		return;
	}

	if (m_iClip <= 0)
	{
		if (m_fFireOnEmpty)
		{
			PlayEmptySound();
			m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.2;
		}

		return;
	}

	m_pPlayer->m_iWeaponVolume = LOUD_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = BRIGHT_GUN_FLASH;

	m_iClip--;

	// player "shoot" animation
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );


	UTIL_MakeVectors( m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle );

	Vector vecSrc	 = m_pPlayer->GetGunPosition( );
	Vector vecAiming = m_pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );

	Vector vecDir = m_pPlayer->FireBulletsPlayer( 1, vecSrc, vecAiming, VECTOR_CONE_1DEGREES, 8192, BULLET_PLAYER_SOFTGUN, 0, 0, m_pPlayer->pev, m_pPlayer->random_seed );

    int flags;
#if defined( CLIENT_WEAPONS )
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif

	PLAYBACK_EVENT_FULL( flags, m_pPlayer->edict(), m_usSoftGun, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, vecDir.x, vecDir.y, 0, 0, 0, 0 );

	if (!m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);

	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.75;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
}


void CSoftGun::Reload( void )
{
	if (DefaultReload( SOFTGUN_MAX_CLIP, SOFTGUN_RELOAD, 1.5 ))
	{
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
	}
}


void CSoftGun::WeaponIdle( void )
{
	ResetEmptySound( );

	m_pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );

	if (m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
		return;

	int iAnim = RANDOM_LONG( SOFTGUN_IDLE1, SOFTGUN_IDLE5 );
	switch ( iAnim )
	{
	case SOFTGUN_IDLE2:
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 2.5;
		break;
	case SOFTGUN_IDLE3:
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + (49.0/30.0);
		break;
	case SOFTGUN_IDLE5:
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 2.0;
		break;
	default:
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 2.5;
		break;
	}

	SendWeaponAnim( iAnim );
}

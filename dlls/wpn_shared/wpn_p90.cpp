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

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"

enum p90_e		// names ours
{
	P90_IDLE1 = 0,
	P90_RELOAD,
	P90_DRAW,
	P90_SHOOT1,
	P90_SHOOT2,
	P90_SHOOT3,
};

LINK_ENTITY_TO_CLASS( weapon_p90, CP90 );


void CP90::Spawn( )
{
	pev->classname = MAKE_STRING("weapon_p90"); // hack to allow for old names
	Precache( );
	m_iId = WEAPON_P90;
	SET_MODEL(ENT(pev), "models/w_p90.mdl");

	m_iDefaultAmmo = P90_DEFAULT_GIVE;
	m_flAccuracy = 0.2;
	m_iShotsFired = 0;
	m_bDelayFire = false;

	FallInit();// get ready to fall down.
}


void CP90::Precache( void )
{
	PRECACHE_MODEL("models/v_p90.mdl");
	PRECACHE_MODEL("models/w_p90.mdl");

	PRECACHE_SOUND("weapons/p90-1.wav");
	PRECACHE_SOUND("weapons/p90_clipout.wav");
	PRECACHE_SOUND("weapons/p90_clipin.wav");
	PRECACHE_SOUND("weapons/p90_boltpull.wav");
	PRECACHE_SOUND("weapons/p90_cliprelease.wav");

	m_iShell = PRECACHE_MODEL ("models/rshell.mdl");// brass shell

	m_usFireP90 = PRECACHE_EVENT( 1, "events/p90.sc" );
}

int CP90::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "57mm";
	p->iMaxAmmo1 = P90_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = P90_MAX_CLIP;
	p->iSlot = 0;
	p->iPosition = 8;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_P90;
	p->iWeight = P90_WEIGHT;

	return 1;
}

BOOL CP90::Deploy( )
{
	m_flAccuracy = 0.2;
	m_iShotsFired = 0;
	m_iShellOn = 1;
	m_bDelayFire = false;

	return DefaultDeploy( "models/v_p90.mdl", "models/p_p90.mdl", P90_DRAW, "carbine", UseDecrement() ? 1 : 0 );
}

void CP90::PrimaryAttack( void )
{
	if ( !FBitSet( m_pPlayer->pev->flags, FL_ONGROUND ) )
		P90Fire( m_flAccuracy * 0.3, 0.066, FALSE );
	else if ( m_pPlayer->pev->velocity.Length2D() > 170 )
		P90Fire( m_flAccuracy * 0.115, 0.066, FALSE );
	else
		P90Fire( m_flAccuracy * 0.045, 0.066, FALSE );
}

void CP90::P90Fire( float flSpread, float flCycleTime, BOOL fUseAutoAim )
{
	m_bDelayFire = true;
	m_iShotsFired++;

	m_flAccuracy = ( ( m_iShotsFired * m_iShotsFired ) / 175 ) + 0.45;

	if ( m_flAccuracy > 1 )
		m_flAccuracy = 1;

	if (m_iClip <= 0)
	{
		if (m_fFireOnEmpty)
		{
			PlayEmptySound();
			m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.2;
		}

		return;
	}

	m_iClip--;

	m_pPlayer->pev->effects |= EF_MUZZLEFLASH;

	// player "shoot" animation
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

	UTIL_MakeVectors( m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle );

	m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = DIM_GUN_FLASH;

	Vector vecAiming;
	vecAiming = gpGlobals->v_forward;
	Vector vecSrc = m_pPlayer->GetGunPosition( );
	Vector vecDir;

	vecDir = m_pPlayer->FireBullets3( vecSrc, vecAiming, flSpread, 8192, 1, BULLET_PLAYER_57MM, 21, 0.885, m_pPlayer->pev, false, m_pPlayer->random_seed );

	int flag;
#if defined( CLIENT_WEAPONS )
	flag = FEV_NOTHOST;
#else
	flag = 0;
#endif

	PLAYBACK_EVENT_FULL( flag, m_pPlayer->edict(), m_usFireP90, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, vecDir.x, vecDir.y, (int)(m_pPlayer->pev->punchangle.x * 100), (int)(m_pPlayer->pev->punchangle.y * 100), 5, FALSE );

	m_flNextPrimaryAttack = m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + flCycleTime;

	if (!m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 2;

	if ( !FBitSet( m_pPlayer->pev->flags, FL_ONGROUND ) )
		KickBack( 0.9, 0.45, 0.35, 0.04, 5.25, 3.5, 4 );
	else if ( m_pPlayer->pev->velocity.Length2D() > 0 )
		KickBack( 0.45, 0.3, 0.2, 0.0275, 4, 2.25, 7 );
	else if ( FBitSet( m_pPlayer->pev->flags, FL_DUCKING ) )
		KickBack( 0.275, 0.2, 0.125, 0.02, 3, 1, 9 );
	else
		KickBack( 0.3, 0.225, 0.125, 0.02, 3.25, 1.25, 8 );
}

void CP90::Reload( void )
{
	if ( m_pPlayer->ammo_57mm <= 0 )
		return;

	if ( DefaultReload( P90_MAX_CLIP, P90_RELOAD, 3.4 ) )
	{
		m_pPlayer->SetAnimation( PLAYER_RELOAD );

		m_flAccuracy = 0.2;
		m_iShotsFired = 0;
	}
}

void CP90::WeaponIdle( void )
{
	ResetEmptySound( );

	m_pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );

	if ( m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
		return;

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 20;

	SendWeaponAnim( P90_IDLE1, UseDecrement() ? 1 : 0 );
}

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


enum m4a1_e		// names ours
{
	M4A1_IDLE = 0,
	M4A1_SHOOT1,
	M4A1_SHOOT2,
	M4A1_SHOOT3,
	M4A1_RELOAD,
	M4A1_DRAW,
	M4A1_ATTACH_SILENCER,
	M4A1_UNSIL_IDLE,
	M4A1_UNSIL_SHOOT1,
	M4A1_UNSIL_SHOOT2,
	M4A1_UNSIL_SHOOT3,
	M4A1_UNSIL_RELOAD,
	M4A1_UNSIL_DRAW,
	M4A1_DETACH_SILENCER,
};

LINK_ENTITY_TO_CLASS( weapon_m4a1, CM4A1 );


void CM4A1::Spawn( )
{
	pev->classname = MAKE_STRING("weapon_m4a1"); // hack to allow for old names
	Precache( );
	m_iId = WEAPON_M4A1;
	SET_MODEL(ENT(pev), "models/w_m4a1.mdl");

	m_iDefaultAmmo = M4A1_DEFAULT_GIVE;

	FallInit();// get ready to fall down.
}


void CM4A1::Precache( void )
{
	PRECACHE_MODEL("models/v_m4a1.mdl");
	PRECACHE_MODEL("models/v_m4a1_r.mdl");
	PRECACHE_MODEL("models/w_m4a1.mdl");

	PRECACHE_SOUND("weapons/m4a1-1.wav");
	PRECACHE_SOUND("weapons/m4a1_unsil-1.wav");
	PRECACHE_SOUND("weapons/m4a1_unsil-2.wav");
	PRECACHE_SOUND("weapons/m4a1_clipin.wav");
	PRECACHE_SOUND("weapons/m4a1_clipout.wav");
	PRECACHE_SOUND("weapons/m4a1_boltpull.wav");
	PRECACHE_SOUND("weapons/m4a1_deploy.wav");
	PRECACHE_SOUND("weapons/m4a1_silencer_on.wav");
	PRECACHE_SOUND("weapons/m4a1_silencer_off.wav");

	m_iShell = PRECACHE_MODEL ("models/rshell.mdl");// brass shell

	m_usFireM4A1 = PRECACHE_EVENT( 1, "events/m4a1.sc" );
}

int CM4A1::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "556Nato";
	p->iMaxAmmo1 = M4A1_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = M4A1_MAX_CLIP;
	p->iSlot = 0;
	p->iPosition = 6;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_M4A1;
	p->iWeight = M4A1_WEIGHT;

	return 1;
}

BOOL CM4A1::Deploy( )
{
	m_flAccuracy = 0.2;
	m_iShotsFired = 0;
	m_bDelayFire = true;
	m_iShellOn = 1;

	if ( m_pPlayer->m_bLeftHanded == true )
	{
		if ( m_iWeaponState & WPNSTATE_M4A1_SILENCED )
			return DefaultDeploy( "models/v_m4a1.mdl", "models/p_m4a1.mdl", M4A1_DRAW, "silenced_rifle", UseDecrement() ? 1 : 0 );
		else
			return DefaultDeploy( "models/v_m4a1.mdl", "models/p_m4a1.mdl", M4A1_UNSIL_DRAW, "mp5", UseDecrement() ? 1 : 0 );
	}
	else
	{
		if ( m_iWeaponState & WPNSTATE_M4A1_SILENCED )
			return DefaultDeploy( "models/v_m4a1_r.mdl", "models/p_m4a1.mdl", M4A1_DRAW, "silenced_rifle", UseDecrement() ? 1 : 0 );
		else
			return DefaultDeploy( "models/v_m4a1_r.mdl", "models/p_m4a1.mdl", M4A1_UNSIL_DRAW, "mp5", UseDecrement() ? 1 : 0 );
	}
}

void CM4A1::SecondaryAttack( void )
{
	if ( !( m_iWeaponState & WPNSTATE_M4A1_SILENCED ) )
	{
		m_iWeaponState |= WPNSTATE_M4A1_SILENCED;
		SendWeaponAnim( M4A1_ATTACH_SILENCER, UseDecrement() ? 1 : 0 );
		strcpy( m_pPlayer->m_szAnimExtention, "silenced_rifle" );
	}
	else
	{
		m_iWeaponState &= ~WPNSTATE_M4A1_SILENCED;
		SendWeaponAnim( M4A1_DETACH_SILENCER, UseDecrement() ? 1 : 0 );
		strcpy( m_pPlayer->m_szAnimExtention, "mp5" );
	}

	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 2;
	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 2;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 2.0;
}

void CM4A1::PrimaryAttack( void )
{
	if ( !( m_iWeaponState & WPNSTATE_M4A1_SILENCED ) )
	{
		if ( !FBitSet( m_pPlayer->pev->flags, FL_ONGROUND ) )
			M4A1Fire( 0.035 + ( 0.3 * m_flAccuracy ), 0.0875, FALSE );
		else if ( m_pPlayer->pev->velocity.Length2D() > 140 )
			M4A1Fire( 0.035 + ( 0.07 * m_flAccuracy ), 0.0875, FALSE );
		else
			M4A1Fire( m_flAccuracy * 0.02, 0.0875, FALSE );
	}
	else
	{
		if ( !FBitSet( m_pPlayer->pev->flags, FL_ONGROUND ) )
			M4A1Fire( 0.035 + ( 0.3 * m_flAccuracy ), 0.0875, FALSE );
		else if ( m_pPlayer->pev->velocity.Length2D() > 140 )
			M4A1Fire( 0.035 + ( 0.07 * m_flAccuracy ), 0.0875, FALSE );
		else
			M4A1Fire( m_flAccuracy * 0.025, 0.0875, FALSE );
	}
}

void CM4A1::M4A1Fire( float flSpread, float flCycleTime, BOOL fUseAutoAim )
{
	m_bDelayFire = true;
	m_iShotsFired++;

	m_flAccuracy = ( ( m_iShotsFired * m_iShotsFired * m_iShotsFired ) / 220 ) + 0.3;

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

	// player "shoot" animation
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

	UTIL_MakeVectors( m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle );

	m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = BRIGHT_GUN_FLASH;

	Vector vecAiming;
	vecAiming = gpGlobals->v_forward;
	Vector vecSrc = m_pPlayer->GetGunPosition( );

	if ( !( m_iWeaponState & WPNSTATE_M4A1_SILENCED ) )
	{
		m_pPlayer->FireBullets3( vecSrc, vecAiming, Vector( flSpread, flSpread, flSpread ), 8192, 2, BULLET_PLAYER_556MM, 35, 0.97, NULL, false );
		m_pPlayer->pev->effects |= EF_MUZZLEFLASH;
	}
	else
	{
		m_pPlayer->FireBullets3( vecSrc, vecAiming, Vector( flSpread, flSpread, flSpread ), 8192, 2, BULLET_PLAYER_556MM, 32, 0.95, NULL, false );
	}

	m_pPlayer->ammo_556nato--;

	PLAYBACK_EVENT_FULL( FEV_NOTHOST, m_pPlayer->edict(), m_usFireM4A1, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, m_pPlayer->pev->punchangle.x, m_pPlayer->pev->punchangle.y, (int)(flSpread * 10000), (int)(m_pPlayer->pev->punchangle.z * 10000), ( m_iWeaponState & WPNSTATE_M4A1_SILENCED ) ? true : false, m_pPlayer->m_bLeftHanded != false );

	m_flNextPrimaryAttack = m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + flCycleTime;

	if (!m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.5;

	if ( m_pPlayer->pev->velocity.Length2D() > 0 )
		KickBack( 1.0, 0.45, 0.28, 0.045, 3.75, 3.0, 7 );
	else if ( !FBitSet( m_pPlayer->pev->flags, FL_ONGROUND ) )
		KickBack( 1.2, 0.5, 0.23, 0.15, 5.5, 3.5, 6 );
	else if ( FBitSet( m_pPlayer->pev->flags, FL_DUCKING ) )
		KickBack( 0.6, 0.3, 0.2, 0.0125, 3.25, 2.0, 7 );
	else
		KickBack( 0.65, 0.35, 0.25, 0.015, 3.5, 2.25, 7 );
}

void CM4A1::Reload( void )
{
	if ( m_pPlayer->ammo_556nato <= 0 )
		return;

	if ( !( m_iWeaponState & WPNSTATE_M4A1_SILENCED ) )
		DefaultReload( M4A1_MAX_CLIP, M4A1_UNSIL_RELOAD, 3.05 );
	else
		DefaultReload( M4A1_MAX_CLIP, M4A1_RELOAD, 3.05 );

	m_pPlayer->SetAnimation( PLAYER_RELOAD );

	m_flAccuracy = 0.2;
	m_iShotsFired = 0;
	m_bDelayFire = false;
}

void CM4A1::WeaponIdle( void )
{
	ResetEmptySound( );

	m_pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );

	if ( m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
		return;

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 20;

	if ( !( m_iWeaponState & WPNSTATE_M4A1_SILENCED ) )
		SendWeaponAnim( M4A1_UNSIL_IDLE, UseDecrement() ? 1 : 0 );
	else
		SendWeaponAnim( M4A1_IDLE, UseDecrement() ? 1 : 0 );
}

float CM4A1::GetMaxSpeed( void )
{
	return 230;
}

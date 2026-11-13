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

enum m249_e		// names ours
{
	M249_IDLE1 = 0,
	M249_SHOOT1,
	M249_SHOOT2,
	M249_RELOAD,
	M249_DRAW,
};

LINK_ENTITY_TO_CLASS( weapon_m249, CM249 );


void CM249::Spawn( )
{
	pev->classname = MAKE_STRING("weapon_m249"); // hack to allow for old names
	Precache( );
	m_iId = WEAPON_M249;
	SET_MODEL(ENT(pev), "models/w_m249.mdl");

	m_iDefaultAmmo = M249_DEFAULT_GIVE;

	FallInit();// get ready to fall down.
}


void CM249::Precache( void )
{
	PRECACHE_MODEL("models/v_m249.mdl");
	PRECACHE_MODEL("models/v_m249_r.mdl");
	PRECACHE_MODEL("models/w_m249.mdl");

	PRECACHE_SOUND("weapons/m249-1.wav");
	PRECACHE_SOUND("weapons/m249-2.wav");
	PRECACHE_SOUND("weapons/m249_boxout.wav");
	PRECACHE_SOUND("weapons/m249_boxin.wav");
	PRECACHE_SOUND("weapons/m249_chain.wav");
	PRECACHE_SOUND("weapons/m249_coverup.wav");
	PRECACHE_SOUND("weapons/m249_coverdown.wav");

	m_iShell = PRECACHE_MODEL ("models/rshell.mdl");// brass shell

	m_usFireM249 = PRECACHE_EVENT( 1, "events/m249.sc" );
}

int CM249::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "556NatoBox";
	p->iMaxAmmo1 = M249_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = M249_MAX_CLIP;
	p->iSlot = 0;
	p->iPosition = 4;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_M249;
	p->iWeight = M249_WEIGHT;

	return 1;
}

BOOL CM249::Deploy( )
{
	m_flAccuracy = 0.2;
	m_iShotsFired = 0;
	m_iShellOn = 1;

	if ( m_pPlayer->m_bLeftHanded == true )
		return DefaultDeploy( "models/v_m249.mdl", "models/p_m249.mdl", M249_DRAW, "m249", UseDecrement() ? 1 : 0 );
	else
		return DefaultDeploy( "models/v_m249_r.mdl", "models/p_m249.mdl", M249_DRAW, "m249", UseDecrement() ? 1 : 0 );
}

void CM249::PrimaryAttack( void )
{
	if ( !FBitSet( m_pPlayer->pev->flags, FL_ONGROUND ) )
		M249Fire( 0.045 + ( 0.4 * m_flAccuracy ), 0.1, FALSE );
	else if ( m_pPlayer->pev->velocity.Length2D() > 140 )
		M249Fire( 0.045 + ( 0.095 * m_flAccuracy ), 0.1, FALSE );
	else
		M249Fire( m_flAccuracy * 0.03, 0.1, FALSE );
}

void CM249::M249Fire( float flSpread, float flCycleTime, BOOL fUseAutoAim )
{
	m_bDelayFire = true;
	m_iShotsFired++;

	m_flAccuracy = ( ( m_iShotsFired * m_iShotsFired * m_iShotsFired ) / 175 ) + 0.4;

	if ( m_flAccuracy > 0.9 )
		m_flAccuracy = 0.9;

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
	m_pPlayer->m_iWeaponFlash = BRIGHT_GUN_FLASH;

	Vector vecAiming;
	vecAiming = gpGlobals->v_forward;
	Vector vecSrc = m_pPlayer->GetGunPosition( );
	Vector vecDir;

	vecDir = m_pPlayer->FireBullets3( vecSrc, vecAiming, flSpread, 8192, 2, BULLET_PLAYER_556MM, 32, 0.97, m_pPlayer->pev, false, m_pPlayer->random_seed );

	int flag;
#if defined( CLIENT_WEAPONS )
	flag = FEV_NOTHOST;
#else
	flag = 0;
#endif

	PLAYBACK_EVENT_FULL( flag, m_pPlayer->edict(), m_usFireM249, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, vecDir.x, vecDir.y, (int)(m_pPlayer->pev->punchangle.x * 10000000), (int)(m_pPlayer->pev->punchangle.y * 10000000), FALSE, m_pPlayer->m_bLeftHanded != false );

	m_flNextPrimaryAttack = m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + flCycleTime;

	if (!m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.6;

	if ( m_pPlayer->pev->velocity.Length2D() > 0 )
		KickBack( 1.1, 0.5, 0.3, 0.06, 4.0, 3.0, 8 );
	else if ( !FBitSet( m_pPlayer->pev->flags, FL_ONGROUND ) )
		KickBack( 1.8, 0.65, 0.45, 0.125, 5.0, 3.5, 8 );
	else if ( FBitSet( m_pPlayer->pev->flags, FL_DUCKING ) )
		KickBack( 0.75, 0.325, 0.25, 0.025, 3.5, 2.5, 9 );
	else
		KickBack( 0.8, 0.35, 0.3, 0.03, 3.75, 3.0, 9 );
}

void CM249::Reload( void )
{
	if ( m_pPlayer->ammo_556natobox <= 0 )
		return;

	int iResult = DefaultReload( M249_MAX_CLIP, M249_RELOAD, 4.7 );

	if ( iResult )
	{
		m_pPlayer->SetAnimation( PLAYER_RELOAD );

		m_flAccuracy = 0.2;
		m_bDelayFire = false;
		m_iShotsFired = 0;
	}
}

void CM249::WeaponIdle( void )
{
	ResetEmptySound( );

	m_pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );

	if ( m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
		return;

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 20;

	SendWeaponAnim( M249_IDLE1, UseDecrement() ? 1 : 0 );
}

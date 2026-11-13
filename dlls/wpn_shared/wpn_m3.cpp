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

enum m3_e		// names ours, after the SDK's shotgun
{
	M3_IDLE = 0,
	M3_FIRE1,
	M3_FIRE2,
	M3_RELOAD,
	M3_PUMP,
	M3_START_RELOAD,
	M3_DRAW,
};

LINK_ENTITY_TO_CLASS( weapon_m3, CM3 );


void CM3::Spawn( )
{
	Precache( );
	m_iId = WEAPON_M3;
	pev->classname = MAKE_STRING("weapon_m3");
	SET_MODEL(ENT(pev), "models/w_m3.mdl");

	m_iDefaultAmmo = M3_DEFAULT_GIVE;

	FallInit();// get ready to fall down
}


void CM3::Precache( void )
{
	PRECACHE_MODEL("models/v_m3.mdl");
	PRECACHE_MODEL("models/v_m3_r.mdl");
	PRECACHE_MODEL("models/w_m3.mdl");

	m_iShellLate = m_iShell = PRECACHE_MODEL ("models/shotgunshell.mdl");// shotgun shell

	PRECACHE_SOUND("weapons/m3-1.wav");
	PRECACHE_SOUND("weapons/m3_insertshell.wav");
	PRECACHE_SOUND("weapons/m3_pump.wav");
	PRECACHE_SOUND("weapons/reload1.wav");	// shotgun reload
	PRECACHE_SOUND("weapons/reload3.wav");	// shotgun reload

	m_usFireM3 = PRECACHE_EVENT( 1, "events/m3.sc" );
}

int CM3::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "buckshot";
	p->iMaxAmmo1 = M3_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = M3_MAX_CLIP;
	p->iSlot = 0;
	p->iPosition = 5;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_M3;
	p->iWeight = M3_WEIGHT;

	return 1;
}

BOOL CM3::Deploy( )
{
	if ( m_pPlayer->m_bLeftHanded == true )
		return DefaultDeploy( "models/v_m3.mdl", "models/p_m3.mdl", M3_DRAW, "shotgun", UseDecrement() ? 1 : 0 );
	else
		return DefaultDeploy( "models/v_m3_r.mdl", "models/p_m3.mdl", M3_DRAW, "shotgun", UseDecrement() ? 1 : 0 );
}

void CM3::PrimaryAttack()
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
		Reload( );
		if (m_iClip == 0)
			PlayEmptySound( );
		return;
	}

	m_pPlayer->m_iWeaponVolume = LOUD_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = BRIGHT_GUN_FLASH;

	m_iClip--;

	m_pPlayer->pev->effects |= EF_MUZZLEFLASH;

	// player "shoot" animation
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

	UTIL_MakeVectors( m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle );

	Vector vecAiming = gpGlobals->v_forward;
	Vector vecSrc	 = m_pPlayer->GetGunPosition( );

	m_pPlayer->FireBullets( 9, vecSrc, vecAiming, Vector( 0.0675, 0.0675, 0 ), 3000, BULLET_PLAYER_BUCKSHOT, 0 );

	PLAYBACK_EVENT_FULL( FEV_NOTHOST, m_pPlayer->edict(), m_usFireM3, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, 0.0, 0.0, 0, 0, 0, 0 );

	if (!m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);

	if (m_iClip != 0)
		m_flPumpTime = UTIL_WeaponTimeBase() + 0.5;

	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.875;
	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.875;
	if (m_iClip != 0)
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 2.5;
	else
		m_flTimeWeaponIdle = 0.875;
	m_fInSpecialReload = 0;

	m_pPlayer->pev->punchangle.x -= RANDOM_LONG( 4, 6 );
	m_pPlayer->m_flEjectBrass = gpGlobals->time + 0.45;
}


void CM3::Reload( void )
{
	if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 || m_iClip == M3_MAX_CLIP)
		return;

	// don't reload until recoil is done
	if (m_flNextPrimaryAttack > UTIL_WeaponTimeBase())
		return;

	// check to see if we're ready to reload
	if (m_fInSpecialReload == 0)
	{
		m_pPlayer->SetAnimation( PLAYER_RELOAD );
		SendWeaponAnim( M3_START_RELOAD, UseDecrement() ? 1 : 0 );
		m_fInSpecialReload = 1;
		m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.55;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.55;
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.55;
		m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.55;
		return;
	}
	else if (m_fInSpecialReload == 1)
	{
		if (m_flTimeWeaponIdle > UTIL_WeaponTimeBase())
			return;
		// was waiting for gun to move to side
		m_fInSpecialReload = 2;

		SendWeaponAnim( M3_RELOAD, UseDecrement() ? 1 : 0 );

		m_flNextReload = UTIL_WeaponTimeBase() + 0.45;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.45;
	}
	else
	{
		// Add them to the clip
		m_iClip += 1;
		m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] -= 1;
		m_fInSpecialReload = 1;
		m_pPlayer->ammo_buckshot--;
	}
}


void CM3::WeaponIdle( void )
{
	ResetEmptySound( );

	m_pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );

	if (m_flPumpTime && m_flPumpTime < UTIL_WeaponTimeBase())
	{
		m_flPumpTime = 0;
	}

	if (m_flTimeWeaponIdle < UTIL_WeaponTimeBase())
	{
		if (m_iClip == 0 && m_fInSpecialReload == 0 && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType])
		{
			Reload( );
		}
		else if (m_fInSpecialReload != 0)
		{
			if (m_iClip != 8 && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType])
			{
				Reload( );
			}
			else
			{
				// reload debounce has timed out
				SendWeaponAnim( M3_PUMP, UseDecrement() ? 1 : 0 );

				m_fInSpecialReload = 0;
				m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.5;
			}
		}
		else
		{
			SendWeaponAnim( M3_IDLE, UseDecrement() ? 1 : 0 );
		}
	}
}

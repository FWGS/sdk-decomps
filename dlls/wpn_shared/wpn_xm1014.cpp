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

enum xm1014_e		// names ours
{
	XM1014_IDLE = 0,
	XM1014_FIRE1,
	XM1014_FIRE2,
	XM1014_RELOAD,
	XM1014_PUMP,
	XM1014_START_RELOAD,
	XM1014_DRAW,
};

LINK_ENTITY_TO_CLASS( weapon_xm1014, CXM1014 );


void CXM1014::Spawn( )
{
	Precache( );
	m_iId = WEAPON_XM1014;
	pev->classname = MAKE_STRING("weapon_xm1014"); // hack to allow for old names
	SET_MODEL(ENT(pev), "models/w_xm1014.mdl");

	m_iDefaultAmmo = XM1014_DEFAULT_GIVE;

	FallInit();// get ready to fall down.
}


void CXM1014::Precache( void )
{
	PRECACHE_MODEL("models/v_xm1014.mdl");
	PRECACHE_MODEL("models/v_xm1014_r.mdl");
	PRECACHE_MODEL("models/w_xm1014.mdl");

	m_iShellLate = m_iShell = PRECACHE_MODEL ("models/shotgunshell.mdl");// shotgun shell

	PRECACHE_SOUND("weapons/xm1014-1.wav");
	PRECACHE_SOUND("weapons/reload1.wav");	// shotgun reload
	PRECACHE_SOUND("weapons/reload3.wav");	// shotgun reload

	m_usFireXM1014 = PRECACHE_EVENT( 1, "events/xm1014.sc" );
}

int CXM1014::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "buckshot";
	p->iMaxAmmo1 = XM1014_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = XM1014_MAX_CLIP;
	p->iSlot = 0;
	p->iPosition = 12;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_XM1014;
	p->iWeight = XM1014_WEIGHT;

	return 1;
}

BOOL CXM1014::Deploy( )
{
	if ( m_pPlayer->m_bLeftHanded == true )
		return DefaultDeploy( "models/v_xm1014.mdl", "models/p_xm1014.mdl", XM1014_DRAW, "m249", UseDecrement() ? 1 : 0 );
	else
		return DefaultDeploy( "models/v_xm1014_r.mdl", "models/p_xm1014.mdl", XM1014_DRAW, "m249", UseDecrement() ? 1 : 0 );
}

void CXM1014::PrimaryAttack()
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
	Vector vecSrc = m_pPlayer->GetGunPosition( );

	m_pPlayer->FireBullets( 6, vecSrc, vecAiming, Vector( 0.0725, 0.0725, 0 ), 3048, BULLET_PLAYER_BUCKSHOT, 0 );

	PLAYBACK_EVENT_FULL( FEV_NOTHOST, m_pPlayer->edict(), m_usFireXM1014, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, m_vecAiming.x, m_vecAiming.y, (int)(0.0725 * 10000), (int)(m_vecAiming.x * 10000), m_iClip == 0, m_pPlayer->m_bLeftHanded != false );

	if (!m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);

	if (m_iClip != 0)
		m_flPumpTime = UTIL_WeaponTimeBase() + 0.125;

	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.25;
	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.25;
	if (m_iClip != 0)
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 2.25;
	else
		m_flTimeWeaponIdle = 0.75;
	m_fInSpecialReload = 0;

	m_pPlayer->pev->punchangle.x -= RANDOM_LONG( 3, 5 );
}

void CXM1014::Reload( void )
{
	if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 || m_iClip == XM1014_MAX_CLIP)
		return;

	// don't reload until recoil is done
	if (m_flNextPrimaryAttack > UTIL_WeaponTimeBase())
		return;

	// check to see if we're ready to reload
	if (m_fInSpecialReload == 0)
	{
		m_pPlayer->SetAnimation( PLAYER_RELOAD );
		SendWeaponAnim( XM1014_START_RELOAD, UseDecrement() ? 1 : 0 );
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

		if (RANDOM_LONG(0,1))
			EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/reload1.wav", 1, ATTN_NORM, 0, 85 + RANDOM_LONG(0,0x1f));
		else
			EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/reload3.wav", 1, ATTN_NORM, 0, 85 + RANDOM_LONG(0,0x1f));

		SendWeaponAnim( XM1014_RELOAD, UseDecrement() ? 1 : 0 );

		m_flNextReload = UTIL_WeaponTimeBase() + 0.3;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.3;
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

void CXM1014::WeaponIdle( void )
{
	ResetEmptySound( );

	m_pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );

	if ( m_flPumpTime && m_flPumpTime < UTIL_WeaponTimeBase() )
	{
		// play pumping sound
		m_flPumpTime = 0;
	}

	if ( m_flTimeWeaponIdle < UTIL_WeaponTimeBase() )
	{
		if (m_iClip == 0 && m_fInSpecialReload == 0 && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType])
		{
			Reload( );
		}
		else if (m_fInSpecialReload != 0)
		{
			if (m_iClip != XM1014_MAX_CLIP && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType])
			{
				Reload( );
			}
			else
			{
				// reload debounce has timed out
				SendWeaponAnim( XM1014_PUMP, UseDecrement() ? 1 : 0 );

				m_fInSpecialReload = 0;
				m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.5;
			}
		}
		else
		{
			SendWeaponAnim( XM1014_IDLE, UseDecrement() ? 1 : 0 );
		}
	}
}

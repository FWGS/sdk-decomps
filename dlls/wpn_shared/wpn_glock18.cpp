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

enum glock18_e		// names ours
{
	GLOCK18_IDLE1 = 0,
	GLOCK18_IDLE2,
	GLOCK18_IDLE3,
	GLOCK18_SHOOT,
	GLOCK18_SHOOT2,
	GLOCK18_SHOOT3,
	GLOCK18_SHOOT_EMPTY,
	GLOCK18_RELOAD,
	GLOCK18_DRAW,
	GLOCK18_HOLSTER,
	GLOCK18_ADD_SILENCER,
	GLOCK18_DRAW2,
	GLOCK18_RELOAD2,
};

LINK_ENTITY_TO_CLASS( weapon_glock18, CGLOCK18 );


void CGLOCK18::Spawn( )
{
	pev->classname = MAKE_STRING("weapon_glock18"); // hack to allow for old names
	Precache( );
	m_iId = WEAPON_GLOCK18;
	SET_MODEL(ENT(pev), "models/w_glock18.mdl");

	m_iDefaultAmmo = GLOCK18_DEFAULT_GIVE;

	FallInit();// get ready to fall down.
}


void CGLOCK18::Precache( void )
{
	PRECACHE_MODEL("models/v_glock18.mdl");
	PRECACHE_MODEL("models/v_glock18_r.mdl");
	PRECACHE_MODEL("models/w_glock18.mdl");
	PRECACHE_MODEL("models/p_glock18.mdl");

	PRECACHE_SOUND("weapons/glock18-1.wav");
	PRECACHE_SOUND("weapons/glock18-2.wav");
	PRECACHE_SOUND("weapons/clipout1.wav");
	PRECACHE_SOUND("weapons/clipin1.wav");
	PRECACHE_SOUND("weapons/sliderelease1.wav");
	PRECACHE_SOUND("weapons/slideback1.wav");
	PRECACHE_SOUND("weapons/357_cock1.wav");
	PRECACHE_SOUND("weapons/de_clipin.wav");
	PRECACHE_SOUND("weapons/de_clipout.wav");

	m_iShellLate = m_iShell = PRECACHE_MODEL ("models/pshell.mdl");// brass shell

	m_usFireGlock18 = PRECACHE_EVENT( 1, "events/glock18.sc" );
}

int CGLOCK18::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "9mm";
	p->iMaxAmmo1 = GLOCK18_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = GLOCK18_MAX_CLIP;
	p->iSlot = 1;
	p->iPosition = 2;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_GLOCK18;
	p->iWeight = GLOCK18_WEIGHT;

	return 1;
}

BOOL CGLOCK18::Deploy( )
{
	m_bBurstFire = false;
	m_iBurstShotsFired = 10;

	if ( m_pPlayer->m_bLeftHanded == true )
	{
		if ( RANDOM_LONG( 0, 1 ) )
			return DefaultDeploy( "models/v_glock18.mdl", "models/p_glock18.mdl", GLOCK18_DRAW, "onehanded", UseDecrement() ? 1 : 0 );
		else
			return DefaultDeploy( "models/v_glock18.mdl", "models/p_glock18.mdl", GLOCK18_DRAW2, "onehanded", UseDecrement() ? 1 : 0 );
	}
	else
	{
		if ( RANDOM_LONG( 0, 1 ) )
			return DefaultDeploy( "models/v_glock18_r.mdl", "models/p_glock18.mdl", GLOCK18_DRAW, "onehanded", UseDecrement() ? 1 : 0 );
		else
			return DefaultDeploy( "models/v_glock18_r.mdl", "models/p_glock18.mdl", GLOCK18_DRAW2, "onehanded", UseDecrement() ? 1 : 0 );
	}
}

void CGLOCK18::SecondaryAttack( void )
{
	if ( !( m_iWeaponState & WPNSTATE_GLOCK18_BURST_MODE ) )
	{
		ClientPrint( m_pPlayer->pev, HUD_PRINTCENTER, "Switched to burst-fire" );
		m_iWeaponState |= WPNSTATE_GLOCK18_BURST_MODE;
	}
	else
	{
		ClientPrint( m_pPlayer->pev, HUD_PRINTCENTER, "Switched to semi-automatic" );
		m_iWeaponState &= ~WPNSTATE_GLOCK18_BURST_MODE;
	}

	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.3;
}

void CGLOCK18::PrimaryAttack( void )
{
	if ( m_iWeaponState & WPNSTATE_GLOCK18_BURST_MODE )
		GLOCK18Fire( 0.05, 0.5, TRUE );
	else if ( m_pPlayer->pev->velocity.Length2D() > 0 )
		GLOCK18Fire( ( 1 - m_flAccuracy ) * 0.165, 0.2, FALSE );
	else if ( !FBitSet( m_pPlayer->pev->flags, FL_ONGROUND ) )
		GLOCK18Fire( ( 1 - m_flAccuracy ) * 0.2, 0.2, FALSE );
	else if ( FBitSet( m_pPlayer->pev->flags, FL_DUCKING ) )
		GLOCK18Fire( ( 1 - m_flAccuracy ) * 0.075, 0.2, FALSE );
	else
		GLOCK18Fire( ( 1 - m_flAccuracy ) * 0.1, 0.2, FALSE );
}

void CGLOCK18::GLOCK18Fire( float flSpread, float flCycleTime, BOOL fUseBurstMode )
{
	if ( fUseBurstMode == TRUE )
	{
		m_iBurstShotsFired = 0;
	}
	else
	{
		flCycleTime -= 0.1;

		if ( ++m_iShotsFired > 1 )
			return;
	}

	if ( m_flLastFire == 0 )
		m_flLastFire = gpGlobals->time;
	else
	{
		m_flAccuracy = ( gpGlobals->time - m_flLastFire ) * 0.4 + 0.62;

		if ( m_flAccuracy > 0.98 )
			m_flAccuracy = 0.98;

		m_flLastFire = gpGlobals->time;
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

	m_iClip--;

	m_pPlayer->pev->effects |= EF_MUZZLEFLASH;

	// player "shoot" animation
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

	UTIL_MakeVectors( m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle );

	m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = NORMAL_GUN_FLASH;

	Vector vecAiming;
	vecAiming = gpGlobals->v_forward;
	Vector vecSrc = m_pPlayer->GetGunPosition( );
	Vector vecDir;	// sic: unused, as in wpn_ak47.cpp; without it the x87 operand order differs from retail's

	m_pPlayer->FireBullets3( vecSrc, vecAiming, Vector( flSpread, flSpread, flSpread ), 8192, 1, BULLET_PLAYER_9MM, 20, 0.75, NULL, true );

	PLAYBACK_EVENT_FULL( FEV_NOTHOST, m_pPlayer->edict(), m_usFireGlock18, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, m_vecAiming.x, m_vecAiming.y, (int)(flSpread * 10000), (int)(m_vecAiming.z * 10000), m_iClip == 0, m_pPlayer->m_bLeftHanded != false );

	m_flNextPrimaryAttack = m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + flCycleTime;

	if (!m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 2.5;

	if ( fUseBurstMode == TRUE )
	{
		// the burst's other two shots come from FireRemaining
		m_flNextBurstShot = gpGlobals->time + 0.1;
		m_iBurstShotsFired++;
	}
}

void CGLOCK18::Reload( void )
{
	if ( m_pPlayer->ammo_9mm <= 0 )
		return;

	if ( RANDOM_LONG( 0, 1 ) )
		DefaultReload( GLOCK18_MAX_CLIP, GLOCK18_RELOAD, 2.2 );
	else
		DefaultReload( GLOCK18_MAX_CLIP, GLOCK18_RELOAD2, 2.2 );
}

void CGLOCK18::WeaponIdle( void )
{
	ResetEmptySound( );

	m_pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );

	if ( m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
		return;

	// only idle if the slid isn't back
	if (m_iClip != 0)
	{
		int iAnim;
		float flRand = RANDOM_FLOAT( 0.0, 1.0 );

		if (flRand <= 0.3 + 0 * 0.75)
		{
			iAnim = GLOCK18_IDLE3;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 49.0 / 16;
		}
		else if (flRand <= 0.6 + 0 * 0.875)
		{
			iAnim = GLOCK18_IDLE1;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 60.0 / 16.0;
		}
		else
		{
			iAnim = GLOCK18_IDLE2;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 40.0 / 16.0;
		}
		SendWeaponAnim( iAnim, UseDecrement() ? 1 : 0 );
	}
}

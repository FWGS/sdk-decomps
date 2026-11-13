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

enum usp_e		// names ours
{
	USP_IDLE = 0,
	USP_SHOOT1,
	USP_SHOOT2,
	USP_SHOOT3,
	USP_SHOOT_EMPTY,
	USP_RELOAD,
	USP_DRAW,
	USP_ATTACH_SILENCER,
	USP_UNSIL_IDLE,
	USP_UNSIL_SHOOT1,
	USP_UNSIL_SHOOT2,
	USP_UNSIL_SHOOT3,
	USP_UNSIL_SHOOT_EMPTY,
	USP_UNSIL_RELOAD,
	USP_UNSIL_DRAW,
	USP_DETACH_SILENCER,
};

LINK_ENTITY_TO_CLASS( weapon_usp, CUSP );


void CUSP::Spawn( )
{
	pev->classname = MAKE_STRING("weapon_usp"); // hack to allow for old names
	Precache( );
	m_iId = WEAPON_USP;
	SET_MODEL(ENT(pev), "models/w_usp.mdl");

	m_iDefaultAmmo = USP_DEFAULT_GIVE;

	FallInit();// get ready to fall down.
}


void CUSP::Precache( void )
{
	PRECACHE_MODEL("models/v_usp.mdl");
	PRECACHE_MODEL("models/v_usp_r.mdl");
	PRECACHE_MODEL("models/w_usp.mdl");
	PRECACHE_MODEL("models/p_usp.mdl");

	PRECACHE_SOUND("weapons/usp1.wav");
	PRECACHE_SOUND("weapons/usp2.wav");
	PRECACHE_SOUND("weapons/usp_unsil-1.wav");
	PRECACHE_SOUND("weapons/usp_clipout.wav");
	PRECACHE_SOUND("weapons/usp_clipin.wav");
	PRECACHE_SOUND("weapons/usp_silencer_on.wav");
	PRECACHE_SOUND("weapons/usp_silencer_off.wav");
	PRECACHE_SOUND("weapons/usp_sliderelease.wav");
	PRECACHE_SOUND("weapons/usp_slideback.wav");

	m_iShell = PRECACHE_MODEL ("models/pshell.mdl");// brass shell

	m_usFireUSP = PRECACHE_EVENT( 1, "events/usp.sc" );
}

int CUSP::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "45ACP";
	p->iMaxAmmo1 = USP_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = USP_MAX_CLIP;
	p->iSlot = 1;
	p->iPosition = 4;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_USP;
	p->iWeight = USP_WEIGHT;

	return 1;
}

BOOL CUSP::Deploy( )
{
	m_flAccuracy = 0.98;

	if ( m_pPlayer->m_bLeftHanded == true )
	{
		if ( m_iWeaponState & WPNSTATE_USP_SILENCED )
			return DefaultDeploy( "models/v_usp.mdl", "models/p_usp.mdl", USP_DRAW, "tmp", UseDecrement() ? 1 : 0 );
		else
			return DefaultDeploy( "models/v_usp.mdl", "models/p_usp.mdl", USP_UNSIL_DRAW, "onehanded", UseDecrement() ? 1 : 0 );
	}
	else
	{
		if ( m_iWeaponState & WPNSTATE_USP_SILENCED )
			return DefaultDeploy( "models/v_usp_r.mdl", "models/p_usp.mdl", USP_DRAW, "tmp", UseDecrement() ? 1 : 0 );
		else
			return DefaultDeploy( "models/v_usp_r.mdl", "models/p_usp.mdl", USP_UNSIL_DRAW, "onehanded", UseDecrement() ? 1 : 0 );
	}
}

void CUSP::SecondaryAttack( void )
{
	if ( !( m_iWeaponState & WPNSTATE_USP_SILENCED ) )
	{
		m_iWeaponState |= WPNSTATE_USP_SILENCED;
		SendWeaponAnim( USP_ATTACH_SILENCER, UseDecrement() ? 1 : 0 );
		strcpy( m_pPlayer->m_szAnimExtention, "tmp" );
	}
	else
	{
		m_iWeaponState &= ~WPNSTATE_USP_SILENCED;
		SendWeaponAnim( USP_DETACH_SILENCER, UseDecrement() ? 1 : 0 );
		strcpy( m_pPlayer->m_szAnimExtention, "onehanded" );
	}

	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 3;
	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 3;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 3;
}

void CUSP::PrimaryAttack( void )
{
	if ( m_iWeaponState & WPNSTATE_USP_SILENCED )
	{
		if ( m_pPlayer->pev->velocity.Length2D() > 0 )
			USPFire( ( 1 - m_flAccuracy ) * 0.25, 0.225, FALSE );
		else if ( !FBitSet( m_pPlayer->pev->flags, FL_ONGROUND ) )
			USPFire( ( 1 - m_flAccuracy ) * 0.35, 0.225, FALSE );
		else if ( FBitSet( m_pPlayer->pev->flags, FL_DUCKING ) )
			USPFire( ( 1 - m_flAccuracy ) * 0.125, 0.225, FALSE );
		else
			USPFire( ( 1 - m_flAccuracy ) * 0.15, 0.225, FALSE );
	}
	else
	{
		if ( m_pPlayer->pev->velocity.Length2D() > 0 )
			USPFire( ( 1 - m_flAccuracy ) * 0.225, 0.225, FALSE );
		else if ( !FBitSet( m_pPlayer->pev->flags, FL_ONGROUND ) )
			USPFire( ( 1 - m_flAccuracy ) * 0.3, 0.225, FALSE );
		else if ( FBitSet( m_pPlayer->pev->flags, FL_DUCKING ) )
			USPFire( ( 1 - m_flAccuracy ) * 0.08, 0.225, FALSE );
		else
			USPFire( ( 1 - m_flAccuracy ) * 0.1, 0.225, FALSE );
	}
}

void CUSP::USPFire( float flSpread, float flCycleTime, BOOL fUseAutoAim )
{
	flCycleTime -= 0.1;

	m_iShotsFired++;

	if ( m_iShotsFired > 1 )
		return;

	if ( m_flLastFire == 0 )
		m_flLastFire = gpGlobals->time;
	else
	{
		m_flAccuracy = ( gpGlobals->time - m_flLastFire ) * 0.4 + 0.6;

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

	m_flNextPrimaryAttack = m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + flCycleTime;

	m_iClip--;

	// player "shoot" animation
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

	m_pPlayer->m_iWeaponVolume = BIG_EXPLOSION_VOLUME;
	m_pPlayer->m_iWeaponFlash = DIM_GUN_FLASH;

	UTIL_MakeVectors( m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle );

	if ( !( m_iWeaponState & WPNSTATE_USP_SILENCED ) )
		m_pPlayer->pev->effects |= EF_MUZZLEFLASH;

	Vector vecAiming;
	vecAiming = gpGlobals->v_forward;
	Vector vecSrc = m_pPlayer->GetGunPosition( );

	if ( m_iWeaponState & WPNSTATE_USP_SILENCED )
		m_pPlayer->FireBullets3( vecSrc, vecAiming, Vector( flSpread, flSpread, flSpread ), 4096, 1, BULLET_PLAYER_45ACP, 30, 0.79, NULL, true );
	else
		m_pPlayer->FireBullets3( vecSrc, vecAiming, Vector( flSpread, flSpread, flSpread ), 4096, 1, BULLET_PLAYER_45ACP, 34, 0.79, NULL, true );

	PLAYBACK_EVENT_FULL( FEV_NOTHOST, m_pPlayer->edict(), m_usFireUSP, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, m_vecAiming.x, m_vecAiming.y, (int)(flSpread * 10000), m_pPlayer->m_bLeftHanded != false, m_iClip == 0, m_iWeaponState & WPNSTATE_USP_SILENCED );

	if (!m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 2;

	m_pPlayer->pev->punchangle.x -= 2;
}

void CUSP::Reload( void )
{
	if ( m_pPlayer->ammo_45acp <= 0 )
		return;

	if ( m_iWeaponState & WPNSTATE_USP_SILENCED )
		DefaultReload( USP_MAX_CLIP, USP_RELOAD, 2.7 );
	else
		DefaultReload( USP_MAX_CLIP, USP_UNSIL_RELOAD, 2.7 );

	m_flAccuracy = 0.98;
}

void CUSP::WeaponIdle( void )
{
	ResetEmptySound( );

	m_pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );

	if ( m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
		return;

	if ( m_iClip )
	{
		int iAnim;

		if ( m_iWeaponState & WPNSTATE_USP_SILENCED )
			iAnim = USP_IDLE;
		else
			iAnim = USP_UNSIL_IDLE;

		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 60.0;
		SendWeaponAnim( iAnim, UseDecrement() ? 1 : 0 );
	}
}

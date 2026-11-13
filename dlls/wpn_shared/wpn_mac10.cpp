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

enum mac10_e	// names ours
{
	MAC10_IDLE1 = 0,
	MAC10_RELOAD,
	MAC10_DRAW,
	MAC10_SHOOT1,
	MAC10_SHOOT2,
	MAC10_SHOOT3,
};

LINK_ENTITY_TO_CLASS( weapon_mac10, CMAC10 );


void CMAC10::Spawn( )
{
	pev->classname = MAKE_STRING("weapon_mac10"); // hack to allow for old names
	Precache( );
	m_iId = WEAPON_MAC10;
	SET_MODEL(ENT(pev), "models/w_mac10.mdl");

	m_iDefaultAmmo = MAC10_DEFAULT_GIVE;

	FallInit();// get ready to fall down.
}


void CMAC10::Precache( void )
{
	PRECACHE_MODEL("models/v_mac10.mdl");
	PRECACHE_MODEL("models/v_mac10_r.mdl");
	PRECACHE_MODEL("models/w_mac10.mdl");

	PRECACHE_SOUND("weapons/mac10-1.wav");
	PRECACHE_SOUND("weapons/mac10_clipout.wav");
	PRECACHE_SOUND("weapons/mac10_clipin.wav");
	PRECACHE_SOUND("weapons/mac10_boltpull.wav");

	m_iShell = PRECACHE_MODEL ("models/pshell.mdl");// brass shell

	m_usFireMAC10 = PRECACHE_EVENT( 1, "events/mac10.sc" );
}

int CMAC10::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "45acp";
	p->iMaxAmmo1 = MAC10_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = MAC10_MAX_CLIP;
	p->iSlot = 0;
	p->iPosition = 13;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_MAC10;
	p->iWeight = MAC10_WEIGHT;

	return 1;
}

BOOL CMAC10::Deploy( )
{
	m_flAccuracy = 0.15;
	m_iShellOn = 1;
	m_bDelayFire = false;

	if ( m_pPlayer->m_bLeftHanded == true )
		return DefaultDeploy( "models/v_mac10.mdl", "models/p_mac10.mdl", MAC10_DRAW, "onehanded", UseDecrement() ? 1 : 0 );
	else
		return DefaultDeploy( "models/v_mac10_r.mdl", "models/p_mac10.mdl", MAC10_DRAW, "onehanded", UseDecrement() ? 1 : 0 );
}

void CMAC10::PrimaryAttack( void )
{
	if ( !FBitSet( m_pPlayer->pev->flags, FL_ONGROUND ) )
		MAC10Fire( m_flAccuracy * 0.15, 0.07, FALSE );
	else
		MAC10Fire( m_flAccuracy * 0.03, 0.07, FALSE );
}

void CMAC10::MAC10Fire( float flSpread, float flCycleTime, BOOL fUseAutoAim )
{
	m_bDelayFire = true;
	m_iShotsFired++;

	m_flAccuracy = ( ( m_iShotsFired * m_iShotsFired * m_iShotsFired ) / 200 ) + 0.6;

	if ( m_flAccuracy > 1.65 )
		m_flAccuracy = 1.65;

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
	Vector vecDir;	// sic: unused (an SDK leftover); without it the x87 operands of the event's parameters come in another order

	m_pPlayer->FireBullets3( vecSrc, vecAiming, Vector( flSpread, flSpread, flSpread ), 8192, 1, BULLET_PLAYER_45ACP, 29, 0.82, NULL, false );

	PLAYBACK_EVENT_FULL( FEV_NOTHOST, m_pPlayer->edict(), m_usFireMAC10, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, m_pPlayer->pev->punchangle.x, m_pPlayer->pev->punchangle.y, (int)(flSpread * 10000), (int)(m_pPlayer->pev->punchangle.z * 10000), 5, m_pPlayer->m_bLeftHanded != false );

	m_flNextPrimaryAttack = m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + flCycleTime;

	if (!m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 2.0;

	if ( m_pPlayer->pev->velocity.Length2D() > 0 )
		KickBack( 0.9, 0.45, 0.25, 0.035, 3.5, 2.75, 7 );
	else if ( !FBitSet( m_pPlayer->pev->flags, FL_ONGROUND ) )
		KickBack( 1.2, 0.45, 0.3, 0.04, 3.75, 2.75, 7 );
	else if ( FBitSet( m_pPlayer->pev->flags, FL_DUCKING ) )
		KickBack( 0.75, 0.4, 0.175, 0.03, 2.75, 2.5, 10 );
	else
		KickBack( 0.775, 0.425, 0.2, 0.03, 3.0, 2.75, 9 );
}

void CMAC10::Reload( void )
{
	if ( m_pPlayer->ammo_45acp <= 0 )
		return;

	DefaultReload( MAC10_MAX_CLIP, MAC10_RELOAD, 3.15 );
	m_pPlayer->SetAnimation( PLAYER_RELOAD );

	m_flAccuracy = 0;
	m_iShotsFired = 0;
}

void CMAC10::WeaponIdle( void )
{
	ResetEmptySound( );

	m_pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );

	if ( m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
		return;

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 20;

	SendWeaponAnim( MAC10_IDLE1, UseDecrement() ? 1 : 0 );
}

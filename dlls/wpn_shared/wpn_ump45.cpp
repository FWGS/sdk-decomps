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

enum ump45_e	// names ours
{
	UMP45_IDLE1 = 0,
	UMP45_RELOAD,
	UMP45_DRAW,
	UMP45_SHOOT1,
	UMP45_SHOOT2,
	UMP45_SHOOT3,
};

LINK_ENTITY_TO_CLASS( weapon_ump45, CUMP45 );


void CUMP45::Spawn( )
{
	pev->classname = MAKE_STRING("weapon_ump45"); // hack to allow for old names
	Precache( );
	m_iId = WEAPON_UMP45;
	SET_MODEL(ENT(pev), "models/w_ump45.mdl");

	m_iDefaultAmmo = UMP45_DEFAULT_GIVE;

	FallInit();// get ready to fall down.
}


void CUMP45::Precache( void )
{
	PRECACHE_MODEL("models/v_ump45.mdl");
	PRECACHE_MODEL("models/v_ump45_r.mdl");
	PRECACHE_MODEL("models/w_ump45.mdl");

	PRECACHE_SOUND("weapons/ump45-1.wav");
	PRECACHE_SOUND("weapons/ump45_clipout.wav");
	PRECACHE_SOUND("weapons/ump45_clipin.wav");
	PRECACHE_SOUND("weapons/ump45_boltslap.wav");

	m_iShell = PRECACHE_MODEL ("models/pshell.mdl");// brass shell

	m_usFireUMP45 = PRECACHE_EVENT( 1, "events/ump45.sc" );
}

int CUMP45::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "45ACP";
	p->iMaxAmmo1 = UMP45_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = UMP45_MAX_CLIP;
	p->iSlot = 0;
	p->iPosition = 15;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_UMP45;
	p->iWeight = UMP45_WEIGHT;

	return 1;
}

BOOL CUMP45::Deploy( )
{
	m_flAccuracy = 0;
	iShellOn = 1;
	m_bDelayFire = false;

	if ( m_pPlayer->m_bLeftHanded == true )
		return DefaultDeploy( "models/v_ump45.mdl", "models/p_ump45.mdl", UMP45_DRAW, "carbine", UseDecrement() ? 1 : 0 );
	else
		return DefaultDeploy( "models/v_ump45_r.mdl", "models/p_ump45.mdl", UMP45_DRAW, "carbine", UseDecrement() ? 1 : 0 );
}

void CUMP45::PrimaryAttack( void )
{
	if ( !FBitSet( m_pPlayer->pev->flags, FL_ONGROUND ) )
		UMP45Fire( m_flAccuracy * 0.14, 0.1, FALSE );
	else
		UMP45Fire( m_flAccuracy * 0.04, 0.1, FALSE );
}

void CUMP45::UMP45Fire( float flSpread, float flCycleTime, BOOL fUseAutoAim )
{
	m_bDelayFire = true;
	m_iShotsFired++;

	m_flAccuracy = ( ( m_iShotsFired * m_iShotsFired ) / 210 ) + 0.5;

	if ( m_flAccuracy > 1.0 )
		m_flAccuracy = 1.0;

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

	Vector vecAiming;
	vecAiming = gpGlobals->v_forward;
	Vector vecSrc = m_pPlayer->GetGunPosition( );
	Vector vecDir;

	m_pPlayer->FireBullets3( vecSrc, vecAiming, Vector( flSpread, flSpread, flSpread ), 8192, 1, BULLET_PLAYER_45ACP, 30, 0.82, NULL, false );

	PLAYBACK_EVENT_FULL( FEV_NOTHOST, m_pPlayer->edict(), m_usFireUMP45, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, m_pPlayer->pev->punchangle.x, m_pPlayer->pev->punchangle.y, (int)(flSpread * 10000), (int)(m_pPlayer->pev->punchangle.z * 10000), 5, m_pPlayer->m_bLeftHanded != false );

	m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = DIM_GUN_FLASH;

	m_flNextPrimaryAttack = m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + flCycleTime;

	if (!m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 2;

	if ( !FBitSet( m_pPlayer->pev->flags, FL_ONGROUND ) )
		KickBack( 0.125, 0.65, 0.55, 0.0475, 5.5, 4, 10 );
	else if ( m_pPlayer->pev->velocity.Length2D() > 0 )
		KickBack( 0.55, 0.3, 0.225, 0.03, 3.5, 2.5, 10 );
	else if ( FBitSet( m_pPlayer->pev->flags, FL_DUCKING ) )
		KickBack( 0.25, 0.175, 0.125, 0.02, 2.25, 1.25, 10 );
	else
		KickBack( 0.275, 0.2, 0.15, 0.0225, 2.5, 1.5, 10 );
}

void CUMP45::Reload( void )
{
	if ( m_pPlayer->ammo_45acp <= 0 )
		return;

	if ( DefaultReload( UMP45_MAX_CLIP, UMP45_RELOAD, 3.5 ) )
	{
		m_pPlayer->SetAnimation( PLAYER_RELOAD );

		m_flAccuracy = 0;
		m_iShotsFired = 0;
	}
}

void CUMP45::WeaponIdle( void )
{
	ResetEmptySound( );

	m_pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );

	if ( m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
		return;

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 20;

	SendWeaponAnim( UMP45_IDLE1, UseDecrement() ? 1 : 0 );
}

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

enum mp5n_e	// names ours
{
	MP5N_IDLE1 = 0,
	MP5N_RELOAD,
	MP5N_DRAW,
	MP5N_SHOOT1,
	MP5N_SHOOT2,
	MP5N_SHOOT3,
};

LINK_ENTITY_TO_CLASS( weapon_mp5navy, CMP5N );


void CMP5N::Spawn( )
{
	pev->classname = MAKE_STRING("weapon_mp5navy"); // hack to allow for old names
	Precache( );
	m_iId = WEAPON_MP5N;
	SET_MODEL(ENT(pev), "models/w_mp5.mdl");

	m_iDefaultAmmo = MP5N_DEFAULT_GIVE;
	m_flAccuracy = 0;
	m_bDelayFire = false;

	FallInit();// get ready to fall down.
}


void CMP5N::Precache( void )
{
	PRECACHE_MODEL("models/v_mp5.mdl");
	PRECACHE_MODEL("models/w_mp5.mdl");

	PRECACHE_SOUND("weapons/mp5-1.wav");
	PRECACHE_SOUND("weapons/mp5-2.wav");
	PRECACHE_SOUND("weapons/mp5_clipout.wav");
	PRECACHE_SOUND("weapons/mp5_clipin.wav");
	PRECACHE_SOUND("weapons/mp5_slideback.wav");

	m_iShell = PRECACHE_MODEL ("models/pshell.mdl");// brass shell

	m_usFireMP5N = PRECACHE_EVENT( 1, "events/mp5n.sc" );
}

int CMP5N::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "9mm";
	p->iMaxAmmo1 = MP5N_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = MP5N_MAX_CLIP;
	p->iSlot = 0;
	p->iPosition = 7;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_MP5N;
	p->iWeight = MP5N_WEIGHT;

	return 1;
}

BOOL CMP5N::Deploy( )
{
	m_flAccuracy = 0;
	m_iShellOn = 1;
	m_bDelayFire = false;

	return DefaultDeploy( "models/v_mp5.mdl", "models/p_mp5.mdl", MP5N_DRAW, "mp5", UseDecrement() ? 1 : 0 );
}

void CMP5N::PrimaryAttack( void )
{
	if ( !FBitSet( m_pPlayer->pev->flags, FL_ONGROUND ) )
		MP5NFire( ( m_flAccuracy ) * 0.2, 0.075, FALSE );	// sic: the parentheses (the Processor Pack compiler's instruction order)
	else
		MP5NFire( ( m_flAccuracy ) * 0.04, 0.075, FALSE );
}

void CMP5N::MP5NFire( float flSpread, float flCycleTime, BOOL fUseAutoAim )
{
	m_bDelayFire = true;
	m_iShotsFired++;

	m_flAccuracy = ( ( m_iShotsFired * m_iShotsFired ) / 220.1 ) + 0.45;

	if ( m_flAccuracy > 0.75 )
		m_flAccuracy = 0.75;

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

	int flag;
#if defined( CLIENT_WEAPONS )
	flag = FEV_NOTHOST;
#else
	flag = 0;
#endif

	Vector vecAiming;
	vecAiming = gpGlobals->v_forward;
	Vector vecSrc = m_pPlayer->GetGunPosition( );
	Vector vecDir;

	vecDir = m_pPlayer->FireBullets3( vecSrc, vecAiming, flSpread, 8192, 1, BULLET_PLAYER_9MM, 26, 0.84, m_pPlayer->pev, false, m_pPlayer->random_seed );

	PLAYBACK_EVENT_FULL( flag, m_pPlayer->edict(), m_usFireMP5N, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, vecDir.x, vecDir.y, (int)(m_pPlayer->pev->punchangle.x * 100), (int)(m_pPlayer->pev->punchangle.y * 100), FALSE, FALSE );

	m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = DIM_GUN_FLASH;

	m_flNextPrimaryAttack = m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + flCycleTime;

	if (!m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 2;

	if ( !FBitSet( m_pPlayer->pev->flags, FL_ONGROUND ) )
		KickBack( 0.9, 0.475, 0.35, 0.0425, 5.0, 3.0, 6 );
	else if ( m_pPlayer->pev->velocity.Length2D() > 0 )
		KickBack( 0.5, 0.275, 0.2, 0.03, 3.0, 2.0, 10 );
	else if ( FBitSet( m_pPlayer->pev->flags, FL_DUCKING ) )
		KickBack( 0.225, 0.15, 0.1, 0.015, 2.0, 1.0, 10 );
	else
		KickBack( 0.25, 0.175, 0.125, 0.02, 2.25, 1.25, 10 );
}

void CMP5N::Reload( void )
{
	if ( m_pPlayer->ammo_9mm <= 0 )
		return;

	if ( DefaultReload( MP5N_MAX_CLIP, MP5N_RELOAD, 2.63 ) )
	{
		m_pPlayer->SetAnimation( PLAYER_RELOAD );

		m_flAccuracy = 0;
		m_iShotsFired = 0;
	}
}

void CMP5N::WeaponIdle( void )
{
	ResetEmptySound( );

	m_pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );

	if ( m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
		return;

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 20;

	SendWeaponAnim( MP5N_IDLE1, UseDecrement() ? 1 : 0 );
}

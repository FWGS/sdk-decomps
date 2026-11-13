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

enum sg552_e		// names ours
{
	SG552_IDLE1 = 0,
	SG552_RELOAD,
	SG552_DRAW,
	SG552_SHOOT1,
	SG552_SHOOT2,
	SG552_SHOOT3,
};

LINK_ENTITY_TO_CLASS( weapon_sg552, CSG552 );


void CSG552::Spawn( )
{
	pev->classname = MAKE_STRING("weapon_sg552"); // hack to allow for old names
	Precache( );
	m_iId = WEAPON_SG552;
	SET_MODEL(ENT(pev), "models/w_sg552.mdl");

	m_iDefaultAmmo = SG552_DEFAULT_GIVE;

	FallInit();// get ready to fall down.
}


void CSG552::Precache( void )
{
	PRECACHE_MODEL("models/v_sg552.mdl");
	PRECACHE_MODEL("models/v_sg552_r.mdl");
	PRECACHE_MODEL("models/w_sg552.mdl");

	PRECACHE_SOUND("weapons/sg552-1.wav");
	PRECACHE_SOUND("weapons/sg552-2.wav");
	PRECACHE_SOUND("weapons/sg552_clipout.wav");
	PRECACHE_SOUND("weapons/sg552_clipin.wav");
	PRECACHE_SOUND("weapons/sg552_boltpull.wav");

	m_iShell = PRECACHE_MODEL ("models/rshell.mdl");// brass shell

	m_usFireSG552 = PRECACHE_EVENT( 1, "events/sg552.sc" );
}

int CSG552::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "556Nato";
	p->iMaxAmmo1 = SG552_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = SG552_MAX_CLIP;
	p->iSlot = 0;
	p->iPosition = 10;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_SG552;
	p->iWeight = SG552_WEIGHT;

	return 1;
}

BOOL CSG552::Deploy( )
{
	m_flAccuracy = 0;
	m_iShellOn = 1;

	if ( m_pPlayer->m_bLeftHanded == true )
		return DefaultDeploy( "models/v_sg552.mdl", "models/p_sg552.mdl", SG552_DRAW, "mp5", UseDecrement() ? 1 : 0 );
	else
		return DefaultDeploy( "models/v_sg552_r.mdl", "models/p_sg552.mdl", SG552_DRAW, "mp5", UseDecrement() ? 1 : 0 );
}

void CSG552::SecondaryAttack( void )
{
	if ( m_pPlayer->m_iFOV == 90 )
	{
		m_pPlayer->m_iFOV = 45;
		m_pPlayer->pev->fov = 45;
	}
	else if ( m_pPlayer->m_iFOV == 45 )
	{
		m_pPlayer->m_iFOV = 90;
		m_pPlayer->pev->fov = 90;
	}
	else
	{
		m_pPlayer->m_iFOV = 90;
		m_pPlayer->pev->fov = 90;
	}

	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.3;
}

void CSG552::PrimaryAttack( void )
{
	if ( !FBitSet( m_pPlayer->pev->flags, FL_ONGROUND ) )
		SG552Fire( m_flAccuracy * 0.25, 0.0825, FALSE );
	else if ( m_pPlayer->pev->fov == 90 )
		SG552Fire( m_flAccuracy * 0.02, 0.0825, FALSE );
	else
		SG552Fire( m_flAccuracy * 0.02, 0.135, FALSE );
}

void CSG552::SG552Fire( float flSpread, float flCycleTime, BOOL fUseAutoAim )
{
	m_bDelayFire = true;
	m_iShotsFired++;

	m_flAccuracy = ( ( m_iShotsFired * m_iShotsFired * m_iShotsFired ) / 200 ) + 0.3;

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

	m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = BRIGHT_GUN_FLASH;

	UTIL_MakeVectors( m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle );

	Vector vecAiming;
	vecAiming = gpGlobals->v_forward;
	Vector vecSrc = m_pPlayer->GetGunPosition( );
	Vector vecDir;	// sic: unused (an SDK leftover); without it the x87 operands of the event's parameters come in another order

	m_pPlayer->FireBullets3( vecSrc, vecAiming, Vector( flSpread, flSpread, flSpread ), 8192, 2, BULLET_PLAYER_556MM, 33, 0.955, NULL, false );

	PLAYBACK_EVENT_FULL( FEV_NOTHOST, m_pPlayer->edict(), m_usFireSG552, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, m_pPlayer->pev->punchangle.x, m_pPlayer->pev->punchangle.y, (int)(flSpread * 10000), (int)(m_pPlayer->pev->punchangle.z * 10000), 5, m_pPlayer->m_bLeftHanded != false );

	m_flNextPrimaryAttack = m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + flCycleTime;

	if (!m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 2.0;

	if ( m_pPlayer->pev->velocity.Length2D() > 0 )
		KickBack( 1.15, 0.45, 0.22, 0.18, 5, 3.5, 6 );
	else if ( !FBitSet( m_pPlayer->pev->flags, FL_ONGROUND ) )
		KickBack( 1.25, 0.45, 0.22, 0.18, 6, 4, 5 );
	else if ( FBitSet( m_pPlayer->pev->flags, FL_DUCKING ) )
		KickBack( 0.6, 0.35, 0.2, 0.0125, 4, 2, 10 );
	else
		KickBack( 0.625, 0.375, 0.25, 0.0125, 4.5, 2.25, 9 );
}

void CSG552::Reload( void )
{
	if ( m_pPlayer->ammo_556nato <= 0 )
		return;

	if ( DefaultReload( SG552_MAX_CLIP, SG552_RELOAD, 3 ) )
	{
		if ( m_pPlayer->pev->fov != 90 )
			SecondaryAttack();
	}

	m_flAccuracy = 0;
	m_iShotsFired = 0;
	m_bDelayFire = false;
}

void CSG552::WeaponIdle( void )
{
	ResetEmptySound( );

	m_pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );

	if ( m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
		return;

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 20;

	SendWeaponAnim( SG552_IDLE1, UseDecrement() ? 1 : 0 );
}

float CSG552::GetMaxSpeed( void )
{
	if ( m_pPlayer->m_iFOV != 90 )
		return 200;

	return 235;
}

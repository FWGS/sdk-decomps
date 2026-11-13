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

enum p228_e		// names ours
{
	P228_IDLE = 0,
	P228_SHOOT1,
	P228_SHOOT2,
	P228_SHOOT3,
	P228_SHOOT_EMPTY,
	P228_RELOAD,
	P228_DRAW,
};

LINK_ENTITY_TO_CLASS( weapon_p228, CP228 );


void CP228::Spawn( )
{
	pev->classname = MAKE_STRING("weapon_p228"); // hack to allow for old names
	Precache( );
	m_iId = WEAPON_P228;
	SET_MODEL(ENT(pev), "models/w_p228.mdl");

	m_iDefaultAmmo = P228_DEFAULT_GIVE;

	FallInit();// get ready to fall down.
}


void CP228::Precache( void )
{
	PRECACHE_MODEL("models/v_p228.mdl");
	PRECACHE_MODEL("models/v_p228_r.mdl");
	PRECACHE_MODEL("models/w_p228.mdl");

	PRECACHE_SOUND("weapons/p228-1.wav");
	PRECACHE_SOUND("weapons/p228_clipout.wav");
	PRECACHE_SOUND("weapons/p228_clipin.wav");
	PRECACHE_SOUND("weapons/p228_sliderelease.wav");
	PRECACHE_SOUND("weapons/p228_slidepull.wav");

	m_iShell = PRECACHE_MODEL ("models/pshell.mdl");// brass shell

	m_usFireP228 = PRECACHE_EVENT( 1, "events/p228.sc" );
}

int CP228::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "357SIG";
	p->iMaxAmmo1 = P228_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = P228_MAX_CLIP;
	p->iSlot = 1;
	p->iPosition = 3;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_P228;
	p->iWeight = P228_WEIGHT;

	return 1;
}

BOOL CP228::Deploy( )
{
	m_flAccuracy = 0.9;

	if ( m_pPlayer->m_bLeftHanded == true )
		return DefaultDeploy( "models/v_p228.mdl", "models/p_p228.mdl", P228_DRAW, "onehanded", UseDecrement() ? 1 : 0 );
	else
		return DefaultDeploy( "models/v_p228_r.mdl", "models/p_p228.mdl", P228_DRAW, "onehanded", UseDecrement() ? 1 : 0 );
}

void CP228::PrimaryAttack( void )
{
	if ( m_pPlayer->pev->velocity.Length2D() > 0 )
		P228Fire( ( 1 - m_flAccuracy ) * 0.255, 0.2, FALSE );
	else if ( !FBitSet( m_pPlayer->pev->flags, FL_ONGROUND ) )
		P228Fire( ( 1 - m_flAccuracy ) * 0.35, 0.2, FALSE );
	else if ( FBitSet( m_pPlayer->pev->flags, FL_DUCKING ) )
		P228Fire( ( 1 - m_flAccuracy ) * 0.075, 0.2, FALSE );
	else
		P228Fire( ( 1 - m_flAccuracy ) * 0.15, 0.2, FALSE );
}

void CP228::P228Fire( float flSpread, float flCycleTime, BOOL fUseAutoAim )
{
	flCycleTime -= 0.05;

	m_iShotsFired++;

	if ( m_iShotsFired > 1 )
		return;

	if ( m_flLastFire == 0 )
		m_flLastFire = gpGlobals->time;
	else
	{
		// accuracy falls with shots in quick succession and recovers with the time since the last one
		m_flAccuracy -= ( 0.325 - ( gpGlobals->time - m_flLastFire ) ) * 0.3;

		if ( m_flAccuracy > 0.9 )
			m_flAccuracy = 0.9;
		else if ( m_flAccuracy < 0.6 )
			m_flAccuracy = 0.6;

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

	m_pPlayer->m_iWeaponVolume = BIG_EXPLOSION_VOLUME;
	m_pPlayer->m_iWeaponFlash = DIM_GUN_FLASH;

	Vector vecAiming;
	vecAiming = gpGlobals->v_forward;
	Vector vecSrc = m_pPlayer->GetGunPosition( );
	Vector vecDir;

	vecDir = m_pPlayer->FireBullets3( vecSrc, vecAiming, flSpread, 4096, 1, BULLET_PLAYER_357SIG, 32, 0.8, m_pPlayer->pev, true, m_pPlayer->random_seed );

	int flag;
#if defined( CLIENT_WEAPONS )
	flag = FEV_NOTHOST;
#else
	flag = 0;
#endif

	PLAYBACK_EVENT_FULL( flag, m_pPlayer->edict(), m_usFireP228, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, vecDir.x, vecDir.y, (int)(m_pPlayer->pev->punchangle.x * 10000000), (int)(m_pPlayer->pev->punchangle.y * 10000000), m_iClip == 0, m_pPlayer->m_bLeftHanded != false );

	m_flNextPrimaryAttack = m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + flCycleTime;

	if (!m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 2;

#ifndef CLIENT_DLL	// Counter-Strike 1.3: the server's function ends with this statement and the client's has not got it
	m_pPlayer->pev->punchangle.x -= 2;
#endif
}

void CP228::Reload( void )
{
	if ( m_pPlayer->ammo_357sig <= 0 )
		return;

	int iResult = DefaultReload( P228_MAX_CLIP, P228_RELOAD, 2.7 );

	if ( iResult )
	{
		m_pPlayer->SetAnimation( PLAYER_RELOAD );

		m_flAccuracy = 0.9;
	}
}

void CP228::WeaponIdle( void )
{
	ResetEmptySound( );

	m_pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );

	if ( m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
		return;

	if ( m_iClip )
	{
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 3.0625;
		SendWeaponAnim( P228_IDLE, UseDecrement() ? 1 : 0 );
	}
}

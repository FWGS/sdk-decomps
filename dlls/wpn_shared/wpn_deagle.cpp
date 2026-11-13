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

enum deagle_e		// names ours
{
	DEAGLE_IDLE1 = 0,
	DEAGLE_SHOOT1,
	DEAGLE_SHOOT2,
	DEAGLE_SHOOT_EMPTY,
	DEAGLE_RELOAD,
	DEAGLE_DRAW,
};

LINK_ENTITY_TO_CLASS( weapon_deagle, CDEAGLE );


void CDEAGLE::Spawn( )
{
	pev->classname = MAKE_STRING("weapon_deagle"); // hack to allow for old names
	Precache( );
	m_iId = WEAPON_DEAGLE;
	SET_MODEL(ENT(pev), "models/w_deagle.mdl");

	m_iDefaultAmmo = DEAGLE_DEFAULT_GIVE;
	m_flAccuracy = 0.9;

	FallInit();// get ready to fall down.
}


void CDEAGLE::Precache( void )
{
	PRECACHE_MODEL("models/v_deagle.mdl");
	PRECACHE_MODEL("models/w_deagle.mdl");

	PRECACHE_SOUND("weapons/deagle-1.wav");
	PRECACHE_SOUND("weapons/deagle-2.wav");
	PRECACHE_SOUND("weapons/de_clipout.wav");
	PRECACHE_SOUND("weapons/de_clipin.wav");
	PRECACHE_SOUND("weapons/de_deploy.wav");

	m_iShell = PRECACHE_MODEL ("models/pshell.mdl");// brass shell

	m_usFireDEAGLE = PRECACHE_EVENT( 1, "events/deagle.sc" );
}

int CDEAGLE::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "50AE";
	p->iMaxAmmo1 = DEAGLE_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = DEAGLE_MAX_CLIP;
	p->iSlot = 1;
	p->iPosition = 1;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_DEAGLE;
	p->iWeight = DEAGLE_WEIGHT;

	return 1;
}

BOOL CDEAGLE::Deploy( )
{
	m_flAccuracy = 0.9;

	return DefaultDeploy( "models/v_deagle.mdl", "models/p_deagle.mdl", DEAGLE_DRAW, "onehanded", UseDecrement() ? 1 : 0 );
}

void CDEAGLE::PrimaryAttack( void )
{
	if ( !FBitSet( m_pPlayer->pev->flags, FL_ONGROUND ) )
		DEAGLEFire( ( 1 - m_flAccuracy ) * 1.5, 0.3, FALSE );
	else if ( m_pPlayer->pev->velocity.Length2D() > 0 )
		DEAGLEFire( ( 1 - m_flAccuracy ) * 0.25, 0.3, FALSE );
	else if ( FBitSet( m_pPlayer->pev->flags, FL_DUCKING ) )
		DEAGLEFire( ( 1 - m_flAccuracy ) * 0.115, 0.3, FALSE );
	else
		DEAGLEFire( ( 1 - m_flAccuracy ) * 0.13, 0.3, FALSE );
}

void CDEAGLE::DEAGLEFire( float flSpread, float flCycleTime, BOOL fUseAutoAim )
{
	flCycleTime -= 0.075;

	if ( ++m_iShotsFired > 1 )
		return;

	if ( m_flLastFire == 0 )
		m_flLastFire = gpGlobals->time;
	else
	{
		m_flAccuracy -= ( 0.4 - ( gpGlobals->time - m_flLastFire ) ) * 0.35;

		if ( m_flAccuracy > 0.9 )
			m_flAccuracy = 0.9;
		else if ( m_flAccuracy < 0.55 )
			m_flAccuracy = 0.55;

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
	m_pPlayer->m_iWeaponFlash = BRIGHT_GUN_FLASH;

	Vector vecAiming;
	vecAiming = gpGlobals->v_forward;
	Vector vecSrc = m_pPlayer->GetGunPosition( );
	Vector vecDir;

	vecDir = m_pPlayer->FireBullets3( vecSrc, vecAiming, flSpread, 4096, 2, BULLET_PLAYER_50AE, 54, 0.81, m_pPlayer->pev, true, m_pPlayer->random_seed );

	int flag;
#if defined( CLIENT_WEAPONS )
	flag = FEV_NOTHOST;
#else
	flag = 0;
#endif

	PLAYBACK_EVENT_FULL( flag, m_pPlayer->edict(), m_usFireDEAGLE, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, vecDir.x, vecDir.y, (int)(m_pPlayer->pev->punchangle.x * 100), (int)(m_pPlayer->pev->punchangle.y * 100), m_iClip == 0, FALSE );

	m_flNextPrimaryAttack = m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + flCycleTime;

	if (!m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.8;

	m_pPlayer->pev->punchangle.x -= 2;
}

void CDEAGLE::Reload( void )
{
	if ( m_pPlayer->ammo_50ae <= 0 )
		return;

	if ( DefaultReload( DEAGLE_MAX_CLIP, DEAGLE_RELOAD, 2.2 ) )
	{
		m_pPlayer->SetAnimation( PLAYER_RELOAD );
		m_flAccuracy = 0.9;
	}
}

void CDEAGLE::WeaponIdle( void )
{
	ResetEmptySound( );

	m_pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );

	if ( m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
		return;

	if ( m_iClip )
	{
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 3.75;
		SendWeaponAnim( DEAGLE_IDLE1, UseDecrement() ? 1 : 0 );
	}
}

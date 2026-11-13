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

enum elite_e		// names ours
{
	ELITE_IDLE = 0,
	ELITE_IDLE_LEFTEMPTY,
	ELITE_SHOOTLEFT1,
	ELITE_SHOOTLEFT2,
	ELITE_SHOOTLEFT3,
	ELITE_SHOOTLEFT4,
	ELITE_SHOOTLEFT5,
	ELITE_SHOOTLEFTLAST,
	ELITE_SHOOTRIGHT1,
	ELITE_SHOOTRIGHT2,
	ELITE_SHOOTRIGHT3,
	ELITE_SHOOTRIGHT4,
	ELITE_SHOOTRIGHT5,
	ELITE_SHOOTRIGHTLAST,
	ELITE_RELOAD,
	ELITE_DRAW,
};

LINK_ENTITY_TO_CLASS( weapon_elite, CELITE );


void CELITE::Spawn( )
{
	pev->classname = MAKE_STRING("weapon_elite"); // hack to allow for old names
	Precache( );
	m_iId = WEAPON_ELITE;
	SET_MODEL(ENT(pev), "models/w_elite.mdl");

	m_iDefaultAmmo = ELITE_DEFAULT_GIVE;

	FallInit();// get ready to fall down.
}


void CELITE::Precache( void )
{
	PRECACHE_MODEL("models/v_elite.mdl");
	PRECACHE_MODEL("models/w_elite.mdl");

	PRECACHE_SOUND("weapons/elite_fire.wav");
	PRECACHE_SOUND("weapons/elite_reloadstart.wav");
	PRECACHE_SOUND("weapons/elite_leftclipin.wav");
	PRECACHE_SOUND("weapons/elite_clipout.wav");
	PRECACHE_SOUND("weapons/elite_sliderelease.wav");
	PRECACHE_SOUND("weapons/elite_rightclipin.wav");
	PRECACHE_SOUND("weapons/elite_deploy.wav");

	m_iShell = PRECACHE_MODEL ("models/pshell.mdl");// brass shell

	m_usFireELITE_LEFT = PRECACHE_EVENT( 1, "events/elite_left.sc" );
	m_usFireELITE_RIGHT = PRECACHE_EVENT( 1, "events/elite_right.sc" );
}

int CELITE::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "9mm";
	p->iMaxAmmo1 = ELITE_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = ELITE_MAX_CLIP;
	p->iSlot = 1;
	p->iPosition = 5;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_ELITE;
	p->iWeight = ELITE_WEIGHT;

	return 1;
}

BOOL CELITE::Deploy( )
{
	m_flAccuracy = 0.88;

	if ( !( m_iClip % 2 ) )
		m_iWeaponState |= WPNSTATE_ELITE_LEFT;

	return DefaultDeploy( "models/v_elite.mdl", "models/p_elite.mdl", ELITE_DRAW, "dualpistols", UseDecrement() ? 1 : 0 );
}

void CELITE::PrimaryAttack( void )
{
	if ( m_pPlayer->pev->velocity.Length2D() > 0 )
		ELITEFire( ( 1 - m_flAccuracy ) * 0.175, 0.2, FALSE );
	else if ( !FBitSet( m_pPlayer->pev->flags, FL_ONGROUND ) )
		ELITEFire( ( 1 - m_flAccuracy ) * 0.3, 0.2, FALSE );
	else if ( FBitSet( m_pPlayer->pev->flags, FL_DUCKING ) )
		ELITEFire( ( 1 - m_flAccuracy ) * 0.08, 0.2, FALSE );
	else
		ELITEFire( ( 1 - m_flAccuracy ) * 0.1, 0.2, FALSE );
}

void CELITE::ELITEFire( float flSpread, float flCycleTime, BOOL fUseAutoAim )
{
	flCycleTime -= 0.125;

	m_iShotsFired++;

	if ( m_iShotsFired > 1 )
		return;

	float flTimeDiff = gpGlobals->time - m_flLastFire;	// name ours

	if ( m_flLastFire == 0 )
		m_flLastFire = gpGlobals->time;
	else
	{
		m_flAccuracy -= ( 0.325 - ( gpGlobals->time - m_flLastFire ) ) * 0.275;

		if ( m_flAccuracy > 0.88 )
			m_flAccuracy = 0.88;
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

	m_flNextPrimaryAttack = m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + flCycleTime;

	m_iClip--;

	m_pPlayer->m_iWeaponVolume = BIG_EXPLOSION_VOLUME;
	m_pPlayer->m_iWeaponFlash = DIM_GUN_FLASH;

	UTIL_MakeVectors( m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle );

	m_pPlayer->pev->effects |= EF_MUZZLEFLASH;

	Vector vecAiming;
	vecAiming = gpGlobals->v_forward;
	Vector vecSrc = m_pPlayer->GetGunPosition( );

	if ( m_iWeaponState & WPNSTATE_ELITE_LEFT )
	{
		// player "shoot" animation
		m_pPlayer->SetAnimation( PLAYER_ATTACK1 );
		m_iWeaponState &= ~WPNSTATE_ELITE_LEFT;

		vecSrc = vecSrc - gpGlobals->v_right * 5;
		m_pPlayer->FireBullets3( vecSrc, vecAiming, Vector( flSpread, flSpread, flSpread ), 8192, 1, BULLET_PLAYER_9MM, 20, 0.75, NULL, true );

		PLAYBACK_EVENT_FULL( FEV_NOTHOST, m_pPlayer->edict(), m_usFireELITE_LEFT, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, flTimeDiff, m_vecAiming.y, (int)(flSpread * 10000), m_iClip, 5, 6 );
	}
	else
	{
		// player "shoot" animation
		m_pPlayer->SetAnimation( PLAYER_ATTACK2 );
		m_iWeaponState |= WPNSTATE_ELITE_LEFT;

		vecSrc = vecSrc + gpGlobals->v_right * 5;
		m_pPlayer->FireBullets3( vecSrc, vecAiming, Vector( flSpread, flSpread, flSpread ), 8192, 1, BULLET_PLAYER_9MM, 27, 0.75, NULL, true );

		PLAYBACK_EVENT_FULL( FEV_NOTHOST, m_pPlayer->edict(), m_usFireELITE_RIGHT, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, flTimeDiff, m_vecAiming.y, (int)(flSpread * 10000), m_iClip, 5, 6 );
	}

	if (!m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 2;

	m_pPlayer->pev->punchangle.x -= 2;
}

void CELITE::Reload( void )
{
	if ( m_pPlayer->ammo_9mm <= 0 )
		return;

	if ( DefaultReload( ELITE_MAX_CLIP, ELITE_RELOAD, 4.5 ) )
	{
		m_pPlayer->SetAnimation( PLAYER_RELOAD );
		m_flAccuracy = 0.88;
	}
}

void CELITE::WeaponIdle( void )
{
	ResetEmptySound( );

	m_pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );

	if ( m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
		return;

	if ( m_iClip )
	{
		int iAnim = ( m_iClip == 1 ) ? ELITE_IDLE_LEFTEMPTY : ELITE_IDLE;	// name ours

		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 60.0;
		SendWeaponAnim( iAnim, UseDecrement() ? 1 : 0 );
	}
}

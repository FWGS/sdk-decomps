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

enum tmp_e		// names ours
{
	TMP_IDLE1 = 0,
	TMP_RELOAD,
	TMP_DRAW,
	TMP_SHOOT1,
	TMP_SHOOT2,
	TMP_SHOOT3,
};

LINK_ENTITY_TO_CLASS( weapon_tmp, CTMP );


void CTMP::Spawn( )
{
	pev->classname = MAKE_STRING("weapon_tmp"); // hack to allow for old names
	Precache( );
	m_iId = WEAPON_TMP;
	SET_MODEL(ENT(pev), "models/w_tmp.mdl");

	m_iDefaultAmmo = TMP_DEFAULT_GIVE;

	FallInit();// get ready to fall down.
}


void CTMP::Precache( void )
{
	PRECACHE_MODEL("models/v_tmp.mdl");
	PRECACHE_MODEL("models/v_tmp_r.mdl");
	PRECACHE_MODEL("models/w_tmp.mdl");

	PRECACHE_SOUND("weapons/tmp-1.wav");
	PRECACHE_SOUND("weapons/tmp-2.wav");

	m_iShell = PRECACHE_MODEL ("models/pshell.mdl");// brass shell

	m_usFireTMP = PRECACHE_EVENT( 1, "events/tmp.sc" );
}

int CTMP::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "9mm";
	p->iMaxAmmo1 = TMP_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = TMP_MAX_CLIP;
	p->iSlot = 0;
	p->iPosition = 11;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_TMP;
	p->iWeight = TMP_WEIGHT;

	return 1;
}

BOOL CTMP::Deploy( )
{
	m_flAccuracy = 0.2;
	m_iShotsFired = 0;
	m_iShellOn = 1;
	m_bDelayFire = false;

	if ( m_pPlayer->m_bLeftHanded == true )
		return DefaultDeploy( "models/v_tmp.mdl", "models/p_tmp.mdl", TMP_DRAW, "tmp", UseDecrement() ? 1 : 0 );
	else
		return DefaultDeploy( "models/v_tmp_r.mdl", "models/p_tmp.mdl", TMP_DRAW, "tmp", UseDecrement() ? 1 : 0 );
}

void CTMP::PrimaryAttack( void )
{
	if ( !FBitSet( m_pPlayer->pev->flags, FL_ONGROUND ) )
		TMPFire( m_flAccuracy * 0.3, 0.07, FALSE );
	else
		TMPFire( m_flAccuracy * 0.03, 0.07, FALSE );
}

void CTMP::TMPFire( float flSpread, float flCycleTime, BOOL fUseAutoAim )
{
	m_bDelayFire = true;
	m_iShotsFired++;

	m_flAccuracy = ( ( m_iShotsFired * m_iShotsFired * m_iShotsFired ) / 200 ) + 0.55;

	if ( m_flAccuracy > 1.4 )
		m_flAccuracy = 1.4;

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

	// player "shoot" animation
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

	UTIL_MakeVectors( m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle );

	m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;

	Vector vecAiming;
	vecAiming = gpGlobals->v_forward;
	Vector vecSrc = m_pPlayer->GetGunPosition( );
	Vector vecDir;	// sic: unused (an SDK leftover); without it the x87 operands of the event's parameters come in another order

	m_pPlayer->FireBullets3( vecSrc, vecAiming, Vector( flSpread, flSpread, flSpread ), 8192, 1, BULLET_PLAYER_9MM, 20, 0.85, NULL, false );

	PLAYBACK_EVENT_FULL( FEV_NOTHOST, m_pPlayer->edict(), m_usFireTMP, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, m_pPlayer->pev->punchangle.x, m_pPlayer->pev->punchangle.y, (int)(flSpread * 10000), (int)(m_pPlayer->pev->punchangle.z * 10000), 5, m_pPlayer->m_bLeftHanded != false );

	m_flNextPrimaryAttack = m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + flCycleTime;

	if (!m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 2;

	if ( m_pPlayer->pev->velocity.Length2D() > 0 )
		KickBack( 0.8, 0.4, 0.2, 0.03, 3, 2.5, 7 );
	else if ( !FBitSet( m_pPlayer->pev->flags, FL_ONGROUND ) )
		KickBack( 1, 0.4, 0.25, 0.035, 3.5, 2.5, 7 );
	else if ( FBitSet( m_pPlayer->pev->flags, FL_DUCKING ) )
		KickBack( 0.7, 0.35, 0.125, 0.025, 2.5, 2, 10 );
	else
		KickBack( 0.725, 0.375, 0.15, 0.025, 2.75, 2.25, 9 );
}

void CTMP::Reload( void )
{
	if ( m_pPlayer->ammo_9mm <= 0 )
		return;

	DefaultReload( TMP_MAX_CLIP, TMP_RELOAD, 2.12 );
	m_pPlayer->SetAnimation( PLAYER_RELOAD );

	m_flAccuracy = 0.2;
	m_iShotsFired = 0;
}

void CTMP::WeaponIdle( void )
{
	ResetEmptySound( );

	m_pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );

	if ( m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
		return;

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 20;

	SendWeaponAnim( TMP_IDLE1, UseDecrement() ? 1 : 0 );
}

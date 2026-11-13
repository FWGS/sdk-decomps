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

enum flashbang_e		// names ours
{
	FLASHBANG_IDLE = 0,
	FLASHBANG_PINPULL,
	FLASHBANG_THROW,
	FLASHBANG_DRAW,
};

LINK_ENTITY_TO_CLASS( weapon_flashbang, CFlashbang );


void CFlashbang::Spawn( )
{
	pev->classname = MAKE_STRING("weapon_flashbang");
	Precache( );
	m_iId = WEAPON_FLASHBANG;
	SET_MODEL(ENT(pev), "models/w_flashbang.mdl");

	pev->dmg = 4;

	m_iDefaultAmmo = FLASHBANG_DEFAULT_GIVE;

	FallInit();// get ready to fall down.
}


void CFlashbang::Precache( void )
{
	PRECACHE_MODEL("models/v_flashbang_r.mdl");
	PRECACHE_MODEL("models/v_flashbang.mdl");

	PRECACHE_SOUND("weapons/flashbang-1.wav");
	PRECACHE_SOUND("weapons/flashbang-2.wav");
	PRECACHE_SOUND("weapons/pinpull.wav");
}

int CFlashbang::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "Flashbang";
	p->iMaxAmmo1 = FLASHBANG_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = 3;
	p->iPosition = 2;
	p->iId = m_iId = WEAPON_FLASHBANG;
	p->iWeight = FLASHBANG_WEIGHT;
	p->iFlags = ITEM_FLAG_LIMITINWORLD | ITEM_FLAG_EXHAUSTIBLE;

	return 1;
}


BOOL CFlashbang::Deploy( )
{
	m_flReleaseThrow = -1;

	if ( m_pPlayer->m_bLeftHanded == true )
		return DefaultDeploy( "models/v_flashbang.mdl", "models/p_flashbang.mdl", FLASHBANG_DRAW, "grenade", UseDecrement() ? 1 : 0 );
	else
		return DefaultDeploy( "models/v_flashbang_r.mdl", "models/p_flashbang.mdl", FLASHBANG_DRAW, "grenade", UseDecrement() ? 1 : 0 );
}

BOOL CFlashbang::CanHolster( void )
{
	return TRUE;
}

void CFlashbang::Holster( int skiplocal /* = 0 */ )
{
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;

	if ( !m_pPlayer->m_rgAmmo[ m_iPrimaryAmmoType ] )
	{
		// no more grenades!
		m_pPlayer->pev->weapons &= ~(1<<WEAPON_FLASHBANG);
		DestroyItem();
	}

	m_flReleaseThrow = 0;
	m_flStartThrow = 0;
	EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "common/null.wav", 1.0, ATTN_NORM);
}

void CFlashbang::PrimaryAttack()
{
	if ( !m_flStartThrow && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] > 0 )
	{
		m_flStartThrow = gpGlobals->time;
		m_flReleaseThrow = 0;

		SendWeaponAnim( FLASHBANG_PINPULL );
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.5;
	}
}

void CFlashbang::WeaponIdle( void )
{
	if ( m_flReleaseThrow == 0 )
		m_flReleaseThrow = gpGlobals->time;

	if ( m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
		return;

	if ( m_flStartThrow )
	{
		m_pPlayer->Radio( "%!MRAD_FIREINHOLE", "Fire in the hole!\n" );

		Vector angThrow = m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle;

		if ( angThrow.x < 0 )
			angThrow.x = -10 + angThrow.x * ( ( 90 - 10 ) / 90.0 );
		else
			angThrow.x = -10 + angThrow.x * ( ( 90 + 10 ) / 90.0 );

		float flVel = ( 90 - angThrow.x ) * 6;
		if ( flVel > 750 )
			flVel = 750;

		UTIL_MakeVectors( angThrow );

		Vector vecSrc;

		if ( m_pPlayer->m_bLeftHanded == true )
			vecSrc = m_pPlayer->pev->origin + m_pPlayer->pev->view_ofs + gpGlobals->v_forward * 16 - gpGlobals->v_right * 12;
		else
			vecSrc = m_pPlayer->pev->origin + m_pPlayer->pev->view_ofs + gpGlobals->v_forward * 16 + gpGlobals->v_right * 12;

		Vector vecThrow = gpGlobals->v_forward * flVel + m_pPlayer->pev->velocity;

		// the flashbang always goes off 1.5 seconds after the throw
		float time = 1.5;

		CGrenade::ShootTimed( m_pPlayer->pev, vecSrc, vecThrow, time );

		SendWeaponAnim( FLASHBANG_THROW );

		// player "shoot" animation
		m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

		m_flStartThrow = 0;
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.5;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.75;

		m_pPlayer->m_rgAmmo[ m_iPrimaryAmmoType ]--;

		if ( !m_pPlayer->m_rgAmmo[ m_iPrimaryAmmoType ] )
		{
			// just threw last grenade
			// set attack times in the future, and weapon idle in the future so we can see the whole throw
			// animation, weapon idle will automatically retire the weapon for us.
			m_flTimeWeaponIdle = m_flNextSecondaryAttack = m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.5;// ensure that the animation can finish playing
		}
		return;
	}
	else if ( m_flReleaseThrow > 0 )
	{
		// we've finished the throw, restart.
		m_flStartThrow = 0;

		if ( m_pPlayer->m_rgAmmo[ m_iPrimaryAmmoType ] )
		{
			SendWeaponAnim( FLASHBANG_DRAW );
		}
		else
		{
			RetireWeapon();
			return;
		}

		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + RANDOM_FLOAT ( 10, 15 );
		m_flReleaseThrow = -1;
		return;
	}

	if ( m_pPlayer->m_rgAmmo[ m_iPrimaryAmmoType ] )
	{
		int iAnim;
		float flRand = RANDOM_FLOAT( 0, 1 );
		if ( flRand <= 0.75 )
		{
			iAnim = FLASHBANG_IDLE;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + RANDOM_FLOAT ( 10, 15 );// how long till we do this again.
		}
		else
		{
			// sic: retail sets no animation here; iAnim stays uninitialised
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 75.0 / 30.0;
		}

		SendWeaponAnim( iAnim );
	}
}

BOOL CFlashbang::CanDeploy( void )
{
	return m_pPlayer->m_rgAmmo[ m_iPrimaryAmmoType ];
}

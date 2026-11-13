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


enum hegrenade_e	// names ours, after the SDK's hand grenade
{
	HEGRENADE_IDLE = 0,
	HEGRENADE_PINPULL,
	HEGRENADE_THROW,
	HEGRENADE_DRAW,
};


LINK_ENTITY_TO_CLASS( weapon_hegrenade, CHEGrenade );


void CHEGrenade::Spawn( )
{
	Precache( );
	m_iId = WEAPON_HEGRENADE;
	SET_MODEL(ENT(pev), "models/w_hegrenade.mdl");

	pev->dmg = 4;

	m_iDefaultAmmo = HEGRENADE_DEFAULT_GIVE;

	FallInit();// get ready to fall down.
}


void CHEGrenade::Precache( void )
{
	PRECACHE_MODEL("models/v_hegrenade_r.mdl");
	PRECACHE_MODEL("models/v_hegrenade.mdl");

	PRECACHE_SOUND("weapons/hegrenade-1.wav");
	PRECACHE_SOUND("weapons/hegrenade-2.wav");
	PRECACHE_SOUND("weapons/he_bounce-1.wav");
	PRECACHE_SOUND("weapons/pinpull.wav");
}

int CHEGrenade::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "HEGrenade";
	p->iMaxAmmo1 = HEGRENADE_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = 3;
	p->iPosition = 1;
	p->iId = m_iId = WEAPON_HEGRENADE;
	p->iWeight = HEGRENADE_WEIGHT;
	p->iFlags = ITEM_FLAG_LIMITINWORLD | ITEM_FLAG_EXHAUSTIBLE;

	return 1;
}


BOOL CHEGrenade::Deploy( )
{
	m_flReleaseThrow = -1;

	if ( m_pPlayer->m_bLeftHanded == true )
		return DefaultDeploy( "models/v_hegrenade.mdl", "models/p_hegrenade.mdl", HEGRENADE_DRAW, "grenade", UseDecrement() ? 1 : 0 );
	else
		return DefaultDeploy( "models/v_hegrenade_r.mdl", "models/p_hegrenade.mdl", HEGRENADE_DRAW, "grenade", UseDecrement() ? 1 : 0 );
}

BOOL CHEGrenade::CanHolster( void )
{
	// can only holster hand grenades when not primed!
	return ( m_flStartThrow == 0 );
}

void CHEGrenade::Holster( int skiplocal /* = 0 */ )
{
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;

	if ( !m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] )
	{
		// no more grenades!
		m_pPlayer->pev->weapons &= ~(1<<WEAPON_HEGRENADE);
		DestroyItem();
	}

	m_flReleaseThrow = 0;
	m_flStartThrow = 0;

	EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "common/null.wav", 1.0, ATTN_NORM);
}

void CHEGrenade::PrimaryAttack()
{
	if ( !m_flStartThrow && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] > 0 )
	{
		m_flStartThrow = gpGlobals->time;
		m_flReleaseThrow = 0;

		SendWeaponAnim( HEGRENADE_PINPULL );
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.5;
	}
}


void CHEGrenade::WeaponIdle( void )
{
	if ( m_flReleaseThrow == 0 )
		m_flReleaseThrow = gpGlobals->time;

	if ( m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
		return;

	if ( m_flStartThrow )
	{
		m_pPlayer->Radio( "%!MRAD_FIREINHOLE", "#Fire_in_the_hole" );

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
			vecSrc = m_pPlayer->pev->origin + m_pPlayer->pev->view_ofs + gpGlobals->v_forward * 16;
		else
			vecSrc = m_pPlayer->pev->origin + m_pPlayer->pev->view_ofs + gpGlobals->v_forward * 16;

		Vector vecThrow = gpGlobals->v_forward * flVel + m_pPlayer->pev->velocity;

		CGrenade::ShootTimed2( m_pPlayer->pev, vecSrc, vecThrow, 1.5, m_pPlayer->m_iTeam );

		SendWeaponAnim( HEGRENADE_THROW );

		// player "shoot" animation
		m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

		m_flStartThrow = 0;
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.5;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.75;

		m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType]--;

		if ( !m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] )
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

		if ( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] )
		{
			SendWeaponAnim( HEGRENADE_DRAW );
		}
		else
		{
			RetireWeapon();
			return;
		}

		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + RANDOM_FLOAT( 10, 15 );
		m_flReleaseThrow = -1;
		return;
	}

	if ( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] )
	{
		SendWeaponAnim( HEGRENADE_IDLE );
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + RANDOM_FLOAT( 10, 15 );
	}
}

BOOL CHEGrenade::CanDeploy( void )
{
	return m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType];
}

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

enum smokegrenade_e		// names ours
{
	SMOKEGRENADE_IDLE = 0,
	SMOKEGRENADE_PINPULL,
	SMOKEGRENADE_THROW,
	SMOKEGRENADE_DRAW,
};

LINK_ENTITY_TO_CLASS( weapon_smokegrenade, CSmokeGrenade );


void CSmokeGrenade::Spawn( )
{
	pev->classname = MAKE_STRING("weapon_smokegrenade"); // hack to allow for old names
	Precache( );
	m_iId = WEAPON_SMOKEGRENADE;
	SET_MODEL(ENT(pev), "models/w_smokegrenade.mdl");

	pev->dmg = 4;

	m_iDefaultAmmo = SMOKEGRENADE_DEFAULT_GIVE;
	m_flStartThrow = 0;
	m_flReleaseThrow = -1;

	FallInit();// get ready to fall down.
}


void CSmokeGrenade::Precache( void )
{
	PRECACHE_MODEL("models/v_smokegrenade.mdl");

	PRECACHE_SOUND("weapons/pinpull.wav");
	PRECACHE_SOUND("weapons/sg_explode.wav");

	m_usCreateSmoke = PRECACHE_EVENT( 1, "events/createsmoke.sc" );
}

int CSmokeGrenade::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "SmokeGrenade";
	p->iMaxAmmo1 = SMOKEGRENADE_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = 3;
	p->iPosition = 3;
	p->iId = m_iId = WEAPON_SMOKEGRENADE;
	p->iWeight = SMOKEGRENADE_WEIGHT;
	p->iFlags = ITEM_FLAG_LIMITINWORLD | ITEM_FLAG_EXHAUSTIBLE;

	return 1;
}


BOOL CSmokeGrenade::Deploy( )
{
	m_flReleaseThrow = -1;

	return DefaultDeploy( "models/v_smokegrenade.mdl", "models/p_smokegrenade.mdl", SMOKEGRENADE_DRAW, "grenade", UseDecrement() ? 1 : 0 );
}

BOOL CSmokeGrenade::CanHolster( void )
{
	return TRUE;
}

void CSmokeGrenade::Holster( int skiplocal /* = 0 */ )
{
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;

	if ( !m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] )
	{
		// no more grenades!
		m_pPlayer->pev->weapons &= ~(1<<WEAPON_SMOKEGRENADE);
		DestroyItem();
	}

	m_flStartThrow = 0;
	m_flReleaseThrow = -1;

	EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "common/null.wav", 1.0, ATTN_NORM);
}

void CSmokeGrenade::PrimaryAttack()
{
	if (!m_flStartThrow && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] > 0)
	{
		m_flStartThrow = gpGlobals->time;
		m_flReleaseThrow = 0;

		SendWeaponAnim( SMOKEGRENADE_PINPULL, UseDecrement() ? 1 : 0 );
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.5;
	}
}


void CSmokeGrenade::WeaponIdle( void )
{
	if (m_flReleaseThrow == 0)
		m_flReleaseThrow = gpGlobals->time;

	if (m_flTimeWeaponIdle > UTIL_WeaponTimeBase())
		return;

	if (m_flStartThrow)
	{
		m_pPlayer->Radio( "%!MRAD_FIREINHOLE", "#Fire_in_the_hole" );

		Vector angThrow = m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle;

		if (angThrow.x < 0)
			angThrow.x = -10 + angThrow.x * ((90 - 10) / 90.0);
		else
			angThrow.x = -10 + angThrow.x * ((90 + 10) / 90.0);

		float flVel = (90 - angThrow.x) * 6;
		if (flVel > 750)
			flVel = 750;

		UTIL_MakeVectors( angThrow );

		Vector vecSrc;

		vecSrc = m_pPlayer->pev->origin + m_pPlayer->pev->view_ofs + gpGlobals->v_forward * 16;

		Vector vecThrow = gpGlobals->v_forward * flVel + m_pPlayer->pev->velocity;

		float time = 1.5;

		CGrenade::ShootSmokeGrenade( m_pPlayer->pev, vecSrc, vecThrow, time, m_usCreateSmoke );

		SendWeaponAnim( SMOKEGRENADE_THROW, UseDecrement() ? 1 : 0 );

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
	else if (m_flReleaseThrow > 0)
	{
		// we've finished the throw, restart.
		m_flStartThrow = 0;

		if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType])
		{
			SendWeaponAnim( SMOKEGRENADE_DRAW, UseDecrement() ? 1 : 0 );
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

	if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType])
	{
		int iAnim;
		float flRand = RANDOM_FLOAT(0, 1);
		if (flRand <= 0.75)
		{
			iAnim = SMOKEGRENADE_IDLE;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + RANDOM_FLOAT ( 10, 15 );// how long till we do this again.
		}
		else 
		{
			// retail's bug kept: no animation here (the SDK's fidget is gone), so iAnim is read uninitialised
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 75.0 / 30.0;
		}

		SendWeaponAnim( iAnim, UseDecrement() ? 1 : 0 );
	}
}

#ifdef CLIENT_DLL
extern CBasePlayer *g_pLocalPlayer;
extern CSmokeGrenade g_SmokeGrenade;
#endif

BOOL CSmokeGrenade::CanDeploy( void )
{
#ifndef CLIENT_DLL
	return m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] != 0;
#else
	return g_pLocalPlayer->m_rgAmmo[g_SmokeGrenade.m_iPrimaryAmmoType] != 0;
#endif
}

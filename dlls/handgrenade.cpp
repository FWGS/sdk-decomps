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

// Wanted: dynamite. The SDK's hand grenade with a four-second fuse, lit
// with a flash when the attack is pressed; let go after the lighting
// animation and the stick is thrown (CGrenade::ShootTimed, ggrenade.cpp).
// Held four seconds or more, it goes off as soon as it is let go.

#define	HANDGRENADE_PRIMARY_VOLUME		450

// sequences of models/v_dynamite.mdl
enum handgrenade_e {
	HANDGRENADE_IDLE = 0,
	HANDGRENADE_FIDGET,
	HANDGRENADE_PINPULL,
	HANDGRENADE_THROW1,	// toss
	HANDGRENADE_THROW2,	// medium
	HANDGRENADE_THROW3,	// hard
	HANDGRENADE_HOLSTER,
	HANDGRENADE_DRAW
};


class CHandGrenade : public CBasePlayerWeapon
{
public:
	void Spawn( void );
	void Precache( void );
	int iItemSlot( void ) { return 5; }
	int GetItemInfo(ItemInfo *p);

	void PrimaryAttack( void );
	BOOL Deploy( void );
	BOOL CanHolster( void );
	void Holster( int skiplocal = 0 );
	void WeaponIdle( void );
	float m_flStartThrow;
	float m_flReleaseThrow;
};
LINK_ENTITY_TO_CLASS( weapon_dynamite, CHandGrenade );


void CHandGrenade::Spawn( )
{
	Precache( );
	m_iId = WEAPON_DYNAMITE;
	SET_MODEL(ENT(pev), "models/w_dynamite.mdl");

	pev->dmg = gSkillData.plrDmgHandGrenade;

	m_iDefaultAmmo = 5;

	FallInit();// get ready to fall down.
}


void CHandGrenade::Precache( void )
{
	PRECACHE_MODEL("models/w_dynamite.mdl");
	PRECACHE_MODEL("models/v_dynamite.mdl");
	PRECACHE_MODEL("models/p_dynamite.mdl");
	PRECACHE_MODEL("sprites/dynamite_fuse.spr");
}

int CHandGrenade::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "dynamite";
	p->iMaxAmmo1 = 20;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = 4;
	p->iPosition = 0;
	p->iId = m_iId = WEAPON_DYNAMITE;
	p->iWeight = HANDGRENADE_WEIGHT;
	p->iFlags = ITEM_FLAG_LIMITINWORLD | ITEM_FLAG_EXHAUSTIBLE;

	return 1;
}


BOOL CHandGrenade::Deploy( )
{
	m_flReleaseThrow = -1;
	return DefaultDeploy( "models/v_dynamite.mdl", "models/p_dynamite.mdl", HANDGRENADE_DRAW, "pick" );
}

BOOL CHandGrenade::CanHolster( void )
{
	// can only holster hand grenades when not primed!
	return ( m_flStartThrow == 0 );
}

void CHandGrenade::Holster( int skiplocal /* = 0 */ )
{
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;
	if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType])
	{
		SendWeaponAnim( HANDGRENADE_HOLSTER );
	}
	else
	{
		// no more grenades!
		m_pPlayer->pev->weapons &= ~(1<<WEAPON_DYNAMITE);
		SetThink( DestroyItem );
		pev->nextthink = gpGlobals->time + 0.1;
	}

	EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "common/null.wav", 1.0, ATTN_NORM);
}

void CHandGrenade::PrimaryAttack()
{
	if (!m_flStartThrow && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] > 0)
	{
		m_flStartThrow = gpGlobals->time;
		m_flReleaseThrow = 0;

		SendWeaponAnim( HANDGRENADE_PINPULL );

		// striking the match
		m_pPlayer->pev->effects = (int)(m_pPlayer->pev->effects) | EF_MUZZLEFLASH;

		m_flTimeWeaponIdle = gpGlobals->time + 1.983;
	}
}


void CHandGrenade::WeaponIdle( void )
{
	m_pPlayer->m_iFOV = 0;

	if (m_flReleaseThrow == 0)
		m_flReleaseThrow = gpGlobals->time;

	if (m_flTimeWeaponIdle > gpGlobals->time)
		return;

	if (m_flStartThrow)
	{
		Vector angThrow = m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle;

		if (angThrow.x < 0)
			angThrow.x = -10 + angThrow.x * ((90 - 10) / 90.0);
		else
			angThrow.x = -10 + angThrow.x * ((90 + 10) / 90.0);

		float flVel = (90 - angThrow.x) * 4;
		if (flVel > 500)
			flVel = 500;

		UTIL_MakeVectors( angThrow );

		Vector vecSrc = m_pPlayer->pev->origin + m_pPlayer->pev->view_ofs + gpGlobals->v_forward * 16;

		Vector vecThrow = gpGlobals->v_forward * flVel + m_pPlayer->pev->velocity;

		// alway explode 4 seconds after the fuse was lit
		float time = m_flStartThrow - gpGlobals->time + 4.0;
		if (time < 0)
			time = 0;

		CGrenade::ShootTimed( m_pPlayer->pev, vecSrc, vecThrow, time );

		if (flVel < 500)
		{
			SendWeaponAnim( HANDGRENADE_THROW1 );
		}
		else if (flVel < 1000)
		{
			SendWeaponAnim( HANDGRENADE_THROW2 );
		}
		else
		{
			SendWeaponAnim( HANDGRENADE_THROW3 );
		}

		// player "shoot" animation
		m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

		m_flStartThrow = 0;
		m_flNextPrimaryAttack = gpGlobals->time + 0.5;
		m_flTimeWeaponIdle = gpGlobals->time + 0.5;

		m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType]--;

		if ( !m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] )
		{
			// just threw last grenade
			// set attack times in the future, and weapon idle in the future so we can see the whole throw
			// animation, weapon idle will automatically retire the weapon for us.
			m_flTimeWeaponIdle = m_flNextSecondaryAttack = m_flNextPrimaryAttack = gpGlobals->time + 0.5;// ensure that the animation can finish playing
		}
		return;
	}
	else if (m_flReleaseThrow > 0)
	{
		// we've finished the throw, restart.
		m_flStartThrow = 0;

		if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType])
		{
			SendWeaponAnim( HANDGRENADE_DRAW );
		}
		else
		{
			RetireWeapon();
			return;
		}

		m_flTimeWeaponIdle = gpGlobals->time + RANDOM_FLOAT ( 10, 15 );
		m_flReleaseThrow = -1;
		return;
	}

	if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType])
	{
		int iAnim;
		float flRand = RANDOM_FLOAT(0, 1);
		if (flRand <= 0.75)
		{
			iAnim = HANDGRENADE_IDLE;
			m_flTimeWeaponIdle = gpGlobals->time + 1.1;
		}
		else 
		{
			iAnim = HANDGRENADE_FIDGET;
			m_flTimeWeaponIdle = gpGlobals->time + 3.266;
		}

		SendWeaponAnim( iAnim );
	}
}





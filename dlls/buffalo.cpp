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
//
// Wanted: the Buffalo rifle (weapon_buffalo, ammo_buffalo), a single-shot
// rifle with a zoom on the secondary attack. A shot kicks the player back
// and puffs smoke at the muzzle. The file name and the class names are
// ours.
//

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "weapons.h"
#include "monsters.h"
#include "player.h"
#include "gamerules.h"


// sequences of models/v_buffalogun.mdl
enum buffalo_e {
	BUFFALO_DRAW = 0,
	BUFFALO_HOLSTER,
	BUFFALO_IDLE1,
	BUFFALO_IDLE2,
	BUFFALO_FIDGET,
	BUFFALO_FIRE,
	BUFFALO_DRYFIRE,
	BUFFALO_RELOAD,
};

class CBuffalo : public CBasePlayerWeapon
{
public:
	void Spawn( void );
	void Precache( void );
	int iItemSlot( void ) { return 3; }
	int GetItemInfo(ItemInfo *p);
	int AddToPlayer( CBasePlayer *pPlayer );
	void PrimaryAttack( void );
	void SecondaryAttack( void );
	BOOL Deploy( void );
	void Reload( void );
	void WeaponIdle( void );

	int m_iShell;		// precached, never ejected
	BOOL m_fInZoom;		// don't save this
	int m_iReloadState;	// name ours; reload sounds: 1 break open, 2 load, 3 close, 0 done
};
LINK_ENTITY_TO_CLASS( weapon_buffalo, CBuffalo );


void CBuffalo::Spawn( )
{
	Precache( );
	m_iId = WEAPON_BUFFALO;
	SET_MODEL(ENT(pev), "models/w_buffalogun.mdl");

	m_iDefaultAmmo = 5;

	FallInit();// get ready to fall down.
}


void CBuffalo::Precache( void )
{
	PRECACHE_MODEL("models/v_buffalogun.mdl");
	PRECACHE_MODEL("models/w_buffalogun.mdl");
	PRECACHE_MODEL("models/p_buffalogun.mdl");

	m_iShell = PRECACHE_MODEL ("models/buffalogunshell.mdl");

	PRECACHE_SOUND("items/9mmclip1.wav");

	PRECACHE_SOUND("weapons/buffalo_shoot1.wav");
	PRECACHE_SOUND("weapons/buffalo_shoot2.wav");
	PRECACHE_SOUND("weapons/buffalo_breakopen.wav");
	PRECACHE_SOUND("weapons/buffalo_reload.wav");
	PRECACHE_SOUND("weapons/buffalo_dryfire.wav");
	PRECACHE_SOUND("weapons/buffalo_close.wav");
}

int CBuffalo::AddToPlayer( CBasePlayer *pPlayer )
{
	if ( CBasePlayerWeapon::AddToPlayer( pPlayer ) )
	{
		MESSAGE_BEGIN( MSG_ONE, gmsgWeapPickup, NULL, pPlayer->pev );
			WRITE_BYTE( m_iId );
		MESSAGE_END();
		return TRUE;
	}
	return FALSE;
}

int CBuffalo::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "buffalo";
	p->iMaxAmmo1 = 20;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = 1;
	p->iFlags = 0;
	p->iSlot = 2;
	p->iPosition = 2;
	p->iId = m_iId = WEAPON_BUFFALO;
	p->iWeight = 15;

	return 1;
}

BOOL CBuffalo::Deploy( )
{
	return DefaultDeploy( "models/v_buffalogun.mdl", "models/p_buffalogun.mdl", BUFFALO_DRAW, "buffalo" );
}


void CBuffalo::PrimaryAttack( void )
{
	// don't fire underwater
	if ( m_pPlayer->pev->waterlevel == 3 )
	{
		EMIT_SOUND_DYN( ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/buffalo_dryfire.wav", 1, ATTN_NORM, 0, 85 + RANDOM_LONG(0,0x1f) );
		m_flNextPrimaryAttack = gpGlobals->time + 0.6;
		return;
	}

	if ( m_iClip <= 0 )
	{
		Reload( );
		if ( m_iClip == 0 )
			EMIT_SOUND_DYN( ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/buffalo_dryfire.wav", 1, ATTN_NORM, 0, 85 + RANDOM_LONG(0,0x1f) );
		m_flNextPrimaryAttack = gpGlobals->time + 0.75;
		return;
	}

	UTIL_MakeVectors( m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle );

	// computed for a shell that is never ejected
	Vector	vecShellVelocity = m_pPlayer->pev->velocity
							 + gpGlobals->v_right * RANDOM_FLOAT(50,70)
							 + gpGlobals->v_up * RANDOM_FLOAT(100,150)
							 + gpGlobals->v_forward * 25;

	if ( RANDOM_LONG(0,1) )
		EMIT_SOUND_DYN( ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/buffalo_shoot1.wav", RANDOM_FLOAT(0.95, 1.0), ATTN_NORM, 0, 93 + RANDOM_LONG(0,0x1f) );
	else
		EMIT_SOUND_DYN( ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/buffalo_shoot2.wav", RANDOM_FLOAT(0.95, 1.0), ATTN_NORM, 0, 93 + RANDOM_LONG(0,0x1f) );

	m_pPlayer->m_iWeaponVolume = LOUD_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = NORMAL_GUN_FLASH;

	// the recoil throws the player back
	UTIL_MakeVectors( m_pPlayer->pev->v_angle );
	Vector vecKick( gpGlobals->v_forward * -150 );
	m_pPlayer->pev->velocity = vecKick;

	m_iClip--;

	m_pPlayer->pev->effects = (int)(m_pPlayer->pev->effects) | EF_MUZZLEFLASH;

	// smoke at the muzzle; the gun entity is moved there
	UTIL_SetOrigin( pev, pev->origin + gpGlobals->v_up * 8 + gpGlobals->v_forward * 35 + gpGlobals->v_right * 1 );	// the "* 1" compiles to a plain copy

	MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, pev->origin );
		WRITE_BYTE( TE_SMOKE );
		WRITE_COORD( pev->origin.x );
		WRITE_COORD( pev->origin.y );
		WRITE_COORD( pev->origin.z );
		WRITE_SHORT( g_sModelIndexSmoke );
		WRITE_BYTE( 2 ); // scale * 10
		WRITE_BYTE( 15 ); // framerate
	MESSAGE_END();

	m_pPlayer->pev->punchangle.x -= 0.5;

	SendWeaponAnim( BUFFALO_FIRE );

	// player "shoot" animation
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

	Vector vecSrc	 = m_pPlayer->GetGunPosition( );
	Vector vecAiming = m_pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );

	m_pPlayer->FireBullets( 1, vecSrc, vecAiming, VECTOR_CONE_1DEGREES, 2048, BULLET_PLAYER_BUFFALO, 0 );

	if (!m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);

	m_flNextPrimaryAttack = gpGlobals->time + 2.88;
	m_flNextSecondaryAttack = gpGlobals->time + 2.88;
	m_flTimeWeaponIdle = gpGlobals->time + 2.88;
}


void CBuffalo::SecondaryAttack( void )
{
	if ( m_fInZoom )
	{
		m_pPlayer->m_iFOV = 0;  // 0 means reset to default fov
		m_fInZoom = FALSE;
	}
	else
	{
		m_pPlayer->m_iFOV = 20;
		m_fInZoom = TRUE;
	}

	m_flNextSecondaryAttack = gpGlobals->time + 0.5;
}


void CBuffalo::Reload( void )
{
	m_pPlayer->m_iFOV = 0;
	m_fInZoom = FALSE;

	if ( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 || m_iClip == 1 )
		return;

	// the round goes in at once; WeaponIdle plays the sounds
	m_iReloadState = 1;
	SendWeaponAnim( BUFFALO_RELOAD );
	m_iClip++;
	m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType]--;

	m_flNextPrimaryAttack = gpGlobals->time + 3.666;
	m_flNextSecondaryAttack = gpGlobals->time + 3.666;
	m_flTimeWeaponIdle = gpGlobals->time + 0.93333;
}


void CBuffalo::WeaponIdle( void )
{
	if ( m_flTimeWeaponIdle > gpGlobals->time )
		return;

	if ( m_iReloadState == 1 )
	{
		EMIT_SOUND_DYN( ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/buffalo_breakopen.wav", 1, ATTN_NORM, 0, 85 + RANDOM_LONG(0,0x1f) );
		m_iReloadState = 2;
		m_flTimeWeaponIdle = gpGlobals->time + 1.73337;
		return;
	}

	if ( m_iReloadState == 2 )
	{
		EMIT_SOUND_DYN( ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/buffalo_reload.wav", 1, ATTN_NORM, 0, 85 + RANDOM_LONG(0,0x1f) );
		m_iReloadState = 3;
		m_flTimeWeaponIdle = gpGlobals->time + 0.733;
		return;
	}

	if ( m_iReloadState == 3 )
	{
		EMIT_SOUND_DYN( ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/buffalo_close.wav", 1, ATTN_NORM, 0, 85 + RANDOM_LONG(0,0x1f) );
		m_iReloadState = 3;	// sic
		m_flTimeWeaponIdle = gpGlobals->time + 0.5;
		m_iReloadState = 0;
		return;
	}

	// no idle animations while zoomed
	if ( m_fInZoom == TRUE )
		return;

	ResetEmptySound( );

	m_pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );

	switch ( RANDOM_LONG( 0, 2 ) )
	{
	case 0:
		SendWeaponAnim( BUFFALO_IDLE1 );
		m_flTimeWeaponIdle = gpGlobals->time + 2.111;
		break;
	case 1:
		SendWeaponAnim( BUFFALO_IDLE2 );
		m_flTimeWeaponIdle = gpGlobals->time + 2.111;
		break;
	case 2:
		SendWeaponAnim( BUFFALO_FIDGET );
		m_flTimeWeaponIdle = gpGlobals->time + 2.555;
		break;
	}
}



class CBuffaloAmmo : public CBasePlayerAmmo
{
	void Spawn( void )
	{
		Precache( );
		SET_MODEL(ENT(pev), "models/w_buffalobox.mdl");
		CBasePlayerAmmo::Spawn( );
	}
	void Precache( void )
	{
		PRECACHE_MODEL ("models/w_buffalobox.mdl");
		PRECACHE_SOUND("items/9mmclip1.wav");
	}
	BOOL AddAmmo( CBaseEntity *pOther )
	{
		if (pOther->GiveAmmo( 5, "buffalo", 20 ) != -1)
		{
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM);
			return TRUE;
		}
		return FALSE;
	}
};
LINK_ENTITY_TO_CLASS( ammo_buffalo, CBuffaloAmmo );

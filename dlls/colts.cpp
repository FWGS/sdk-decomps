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
// Wanted: the colts, a pair of revolvers (weapon_colts, ammo_colts).
// The primary attack fires the guns in turn, the secondary both at once.
// The file name and the class names are ours.
//

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "weapons.h"
#include "monsters.h"
#include "player.h"
#include "gamerules.h"


// sequences of models/v_colts.mdl
enum colts_e {
	COLTS_IDLE1 = 0,
	COLTS_IDLE2,
	COLTS_FIDGET,
	COLTS_DRAW,
	COLTS_LEFT_FIRE,
	COLTS_RIGHT_FIRE,
	COLTS_DUAL_FIRE,
	COLTS_RELOAD,
	COLTS_HOLSTER,
};

class CColts : public CBasePlayerWeapon
{
public:
	void Spawn( void );
	void Precache( void );
	int iItemSlot( void ) { return 2; }
	int GetItemInfo(ItemInfo *p);
	int AddToPlayer( CBasePlayer *pPlayer );
	void PrimaryAttack( void );
	void SecondaryAttack( void );
	BOOL Deploy( void );
	void Reload( void );
	void WeaponIdle( void );

	int m_iFireSide;		// name ours; 0: the left gun fired last, 1: the right
	float m_flSoundDelay;	// when to play the reload sound
};
LINK_ENTITY_TO_CLASS( weapon_colts, CColts );


void CColts::Spawn( )
{
	Precache( );
	m_iId = WEAPON_COLTS;
	SET_MODEL(ENT(pev), "models/w_colts.mdl");

	m_iDefaultAmmo = 12;

	FallInit();// get ready to fall down.
}


void CColts::Precache( void )
{
	PRECACHE_MODEL("models/v_colts.mdl");
	PRECACHE_MODEL("models/w_colts.mdl");
	PRECACHE_MODEL("models/p_colts.mdl");

	PRECACHE_SOUND("items/9mmclip1.wav");

	PRECACHE_SOUND("weapons/coltsfire1.wav");
	PRECACHE_SOUND("weapons/coltsfire2.wav");
	PRECACHE_SOUND("weapons/colts_dryfire.wav");
	PRECACHE_SOUND("weapons/coltsreload1.wav");
}

int CColts::AddToPlayer( CBasePlayer *pPlayer )
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

int CColts::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "colts";
	p->iMaxAmmo1 = 36;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = 12;
	p->iFlags = 0;
	p->iSlot = 1;
	p->iPosition = 2;
	p->iId = m_iId = WEAPON_COLTS;
	p->iWeight = 15;

	return 1;
}

BOOL CColts::Deploy( )
{
	return DefaultDeploy( "models/v_colts.mdl", "models/p_colts.mdl", COLTS_DRAW, "colts" );
}


void CColts::PrimaryAttack( void )
{
	// alternate the guns
	m_iFireSide++;
	if ( m_iFireSide >= 2 )
		m_iFireSide = 0;

	// don't fire underwater
	if ( m_pPlayer->pev->waterlevel == 3 )
	{
		PlayEmptySound( );
		EMIT_SOUND( ENT(pev), CHAN_ITEM, "weapons/colts_dryfire.wav", 1, ATTN_NORM );
		m_flNextPrimaryAttack = gpGlobals->time + 0.25;
		m_flNextSecondaryAttack = gpGlobals->time + 0.5;
		return;
	}

	if ( m_iClip <= 0 )
	{
		Reload( );
		if ( m_iClip == 0 )
			PlayEmptySound( );
		EMIT_SOUND( ENT(pev), CHAN_ITEM, "weapons/colts_dryfire.wav", 1, ATTN_NORM );
		m_flNextPrimaryAttack = gpGlobals->time + 0.25;
		m_flNextSecondaryAttack = gpGlobals->time + 0.55;
		return;
	}

	UTIL_MakeVectors( m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle );

	// computed for a shell that is never ejected
	Vector	vecShellVelocity = m_pPlayer->pev->velocity
							 + gpGlobals->v_right * RANDOM_FLOAT(50,70)
							 + gpGlobals->v_up * RANDOM_FLOAT(100,150)
							 + gpGlobals->v_forward * 25;

	EMIT_SOUND_DYN( ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/coltsfire1.wav", RANDOM_FLOAT(0.95, 1.0), ATTN_NORM, 0, PITCH_NORM );

	m_pPlayer->m_iWeaponVolume = LOUD_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = NORMAL_GUN_FLASH;

	m_iClip--;

	m_pPlayer->pev->effects = (int)(m_pPlayer->pev->effects) | EF_MUZZLEFLASH;

	m_pPlayer->pev->punchangle.x = RANDOM_FLOAT( -1, 1 );
	m_pPlayer->pev->punchangle.y = RANDOM_FLOAT( -1, 1 );

	if ( m_iFireSide == 0 )
		SendWeaponAnim( COLTS_LEFT_FIRE );
	else
		SendWeaponAnim( COLTS_RIGHT_FIRE );

	// player "shoot" animation
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

	Vector vecSrc	 = m_pPlayer->GetGunPosition( );
	Vector vecAiming = m_pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );

	if ( g_pGameRules->IsDeathmatch() )
	{
		m_pPlayer->FireBullets( 1, vecSrc, vecAiming, Vector( 0.08716, 0.04362, 0 ), 2048, BULLET_PLAYER_COLTS, 0 );
	}
	else
	{
		m_pPlayer->FireBullets( 1, vecSrc, vecAiming, VECTOR_CONE_1DEGREES, 8192, BULLET_PLAYER_COLTS, 0 );
	}

	if (!m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);

	m_flNextPrimaryAttack = gpGlobals->time + 0.633;
	m_flNextSecondaryAttack = gpGlobals->time + 0.633;
	m_flTimeWeaponIdle = gpGlobals->time + 4.0;
}


void CColts::SecondaryAttack( void )
{
	// don't fire underwater
	if ( m_pPlayer->pev->waterlevel == 3 )
	{
		PlayEmptySound( );
		EMIT_SOUND( ENT(pev), CHAN_ITEM, "weapons/colts_dryfire.wav", 1, ATTN_NORM );
		m_flNextPrimaryAttack = gpGlobals->time + 0.5;
		m_flNextSecondaryAttack = gpGlobals->time + 0.5;
		return;
	}

	// both guns need a bullet
	if ( m_iClip == 1 )
	{
		PlayEmptySound( );
		EMIT_SOUND( ENT(pev), CHAN_ITEM, "weapons/colts_dryfire.wav", 1, ATTN_NORM );
		m_flNextPrimaryAttack = gpGlobals->time + 0.25;
		m_flNextSecondaryAttack = gpGlobals->time + 0.5;
		return;
	}

	if ( m_iClip == 0 )
	{
		Reload( );
		if ( m_iClip == 0 )
			PlayEmptySound( );
		EMIT_SOUND( ENT(pev), CHAN_ITEM, "weapons/colts_dryfire.wav", 1, ATTN_NORM );
		m_flNextPrimaryAttack = gpGlobals->time + 0.25;
		m_flNextSecondaryAttack = gpGlobals->time + 0.5;
		return;
	}

	UTIL_MakeVectors( m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle );

	// computed for a shell that is never ejected
	Vector	vecShellVelocity = m_pPlayer->pev->velocity
							 + gpGlobals->v_right * RANDOM_FLOAT(50,70)
							 + gpGlobals->v_up * RANDOM_FLOAT(100,150)
							 + gpGlobals->v_forward * 25;

	EMIT_SOUND_DYN( ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/coltsfire2.wav", RANDOM_FLOAT(0.95, 1.0), ATTN_NORM, 0, PITCH_NORM );

	m_pPlayer->m_iWeaponVolume = LOUD_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = NORMAL_GUN_FLASH;

	m_iClip -= 2;

	m_pPlayer->pev->effects = (int)(m_pPlayer->pev->effects) | EF_MUZZLEFLASH;

	m_pPlayer->pev->punchangle.x = RANDOM_FLOAT( -1, 1 );
	m_pPlayer->pev->punchangle.y = RANDOM_FLOAT( -1, 1 );

	SendWeaponAnim( COLTS_DUAL_FIRE );

	// player "shoot" animation
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

	Vector vecSrc	 = m_pPlayer->GetGunPosition( );
	Vector vecAiming = m_pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );

	if ( g_pGameRules->IsDeathmatch() )
	{
		m_pPlayer->FireBullets( 2, vecSrc, vecAiming, Vector( 0.08716, 0.04362, 0 ), 2048, BULLET_PLAYER_COLTS, 0 );
	}
	else
	{
		m_pPlayer->FireBullets( 2, vecSrc, vecAiming, VECTOR_CONE_1DEGREES, 8192, BULLET_PLAYER_COLTS, 0 );
	}

	if (!m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);

	m_flNextPrimaryAttack = gpGlobals->time + 0.63;
	m_flNextSecondaryAttack = gpGlobals->time + 0.63;
	m_flTimeWeaponIdle = gpGlobals->time + 5.0;
}


void CColts::Reload( void )
{
	if ( DefaultReload( 12, COLTS_RELOAD, 2.0 ) )
	{
		m_flSoundDelay = gpGlobals->time + 0.6;
	}
}


void CColts::WeaponIdle( void )
{
	m_pPlayer->m_iFOV = 0;

	ResetEmptySound( );

	m_pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );

	if ( m_flSoundDelay != 0 && m_flSoundDelay <= gpGlobals->time )
	{
		EMIT_SOUND( ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/coltsreload1.wav", RANDOM_FLOAT(0.8, 0.9), ATTN_NORM );
		m_flSoundDelay = 0;
	}

	if ( m_flTimeWeaponIdle > gpGlobals->time )
		return;

	switch ( RANDOM_LONG( 0, 2 ) )
	{
	case 0:
		SendWeaponAnim( COLTS_IDLE1 );
		m_flTimeWeaponIdle = gpGlobals->time + 2.7;
		break;
	case 1:
		SendWeaponAnim( COLTS_IDLE2 );
		m_flTimeWeaponIdle = gpGlobals->time + 1.8;
		break;
	case 2:
		SendWeaponAnim( COLTS_FIDGET );
		m_flTimeWeaponIdle = gpGlobals->time + 3.4;
		break;
	}
}



class CColtsAmmo : public CBasePlayerAmmo
{
	void Spawn( void )
	{
		Precache( );
		SET_MODEL(ENT(pev), "models/w_coltsbox.mdl");
		CBasePlayerAmmo::Spawn( );
	}
	void Precache( void )
	{
		PRECACHE_MODEL ("models/w_coltsbox.mdl");
		PRECACHE_SOUND("items/9mmclip1.wav");
	}
	BOOL AddAmmo( CBaseEntity *pOther )
	{
		if (pOther->GiveAmmo( 36, "Colts", 36 ) != -1)	// sic: the gun's ammo is "colts" (GetAmmoIndex ignores case)
		{
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM);
			return TRUE;
		}
		return FALSE;
	}
};
LINK_ENTITY_TO_CLASS( ammo_colts, CColtsAmmo );

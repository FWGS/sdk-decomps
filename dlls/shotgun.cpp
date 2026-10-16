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
#include "gamerules.h"

// Wanted: the double-barrelled shotgun, two shells per load. It grew out of
// the SDK's first release (no playback events, sounds from the server) and
// lost the shell ejection; the shell velocities are still computed.

// special deathmatch shotgun spreads
#define VECTOR_CONE_DM_SHOTGUN	Vector( 0.08716, 0.04362, 0.00  )// 10 degrees by 5 degrees
#define VECTOR_CONE_DM_DOUBLESHOTGUN Vector( 0.17365, 0.04362, 0.00 ) // 20 degrees by 5 degrees

// sequences of models/v_shotgun.mdl
enum shotgun_e {
	SHOTGUN_IDLE1 = 0,
	SHOTGUN_IDLE2,
	SHOTGUN_IDLE3,
	SHOTGUN_HOLSTER,
	SHOTGUN_DRAW,
	SHOTGUN_SHOOT,
	SHOTGUN_SHOOT2,		// unused
	SHOTGUN_RELOAD,
};

class CShotgun : public CBasePlayerWeapon
{
public:
	int		Save( CSave &save );
	int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	void Spawn( void );
	void Precache( void );
	int iItemSlot( ) { return 3; }
	int GetItemInfo(ItemInfo *p);
	int AddToPlayer( CBasePlayer *pPlayer );

	void PrimaryAttack( void );
	void SecondaryAttack( void );
	BOOL Deploy( );
	void Reload( void );
	void WeaponIdle( void );
	int m_fInReload;
	float m_flNextReload;
	int m_iShell;
	float m_flPumpTime;
};
LINK_ENTITY_TO_CLASS( weapon_shotgun, CShotgun );


TYPEDESCRIPTION	CShotgun::m_SaveData[] = 
{
	DEFINE_FIELD( CShotgun, m_flNextReload, FIELD_TIME ),
	DEFINE_FIELD( CShotgun, m_fInReload, FIELD_INTEGER ),
	DEFINE_FIELD( CShotgun, m_flNextReload, FIELD_TIME ),
	// DEFINE_FIELD( CShotgun, m_iShell, FIELD_INTEGER ),
	DEFINE_FIELD( CShotgun, m_flPumpTime, FIELD_TIME ),
};
IMPLEMENT_SAVERESTORE( CShotgun, CBasePlayerWeapon );



void CShotgun::Spawn( )
{
	Precache( );
	m_iId = WEAPON_SHOTGUN;
	SET_MODEL(ENT(pev), "models/w_shotgun.mdl");

	m_iDefaultAmmo = 8;

	FallInit();// get ready to fall
}


void CShotgun::Precache( void )
{
	PRECACHE_MODEL("models/v_shotgun.mdl");
	PRECACHE_MODEL("models/w_shotgun.mdl");
	PRECACHE_MODEL("models/p_shotgun.mdl");

	m_iShell = PRECACHE_MODEL ("models/shotgunshell.mdl");// shotgun shell

	PRECACHE_SOUND("items/9mmclip1.wav");              

	PRECACHE_SOUND ("weapons/dbarrel1.wav");//shotgun
	PRECACHE_SOUND ("weapons/sbarrel1.wav");//shotgun

	PRECACHE_SOUND ("weapons/reload1.wav");	// shotgun reload
	PRECACHE_SOUND ("weapons/reload3.wav");	// shotgun reload
}

int CShotgun::AddToPlayer( CBasePlayer *pPlayer )
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


int CShotgun::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "buckshot";
	p->iMaxAmmo1 = 32;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = 2;
	p->iSlot = 2;
	p->iPosition = 1;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_SHOTGUN;
	p->iWeight = SHOTGUN_WEIGHT;

	return 1;
}



BOOL CShotgun::Deploy( )
{
	return DefaultDeploy( "models/v_shotgun.mdl", "models/p_shotgun.mdl", SHOTGUN_DRAW, "shotgun" );
}


void CShotgun::PrimaryAttack()
{
	// don't fire underwater
	if (m_pPlayer->pev->waterlevel == 3)
	{
		PlayEmptySound( );
		m_flNextPrimaryAttack = gpGlobals->time + 0.5;
		return;
	}

	if (m_iClip <= 0)
	{
		Reload( );
		if (m_iClip == 0)
			PlayEmptySound( );
		return;
	}

	UTIL_MakeVectors( m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle );

	// sic: the shell isn't ejected any more
	Vector	vecShellVelocity = m_pPlayer->pev->velocity 
							 + gpGlobals->v_right * RANDOM_FLOAT(50,70) 
							 + gpGlobals->v_up * RANDOM_FLOAT(100,150) 
							 + gpGlobals->v_forward * 25;

	EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/sbarrel1.wav", RANDOM_FLOAT(0.95, 1.0), ATTN_NORM, 0, 93 + RANDOM_LONG(0,0x1f));

	m_pPlayer->m_iWeaponVolume = LOUD_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = NORMAL_GUN_FLASH;

	m_iClip--;
	m_pPlayer->pev->effects = (int)(m_pPlayer->pev->effects) | EF_MUZZLEFLASH;

	m_pPlayer->pev->punchangle.x -= 5;

	SendWeaponAnim( SHOTGUN_SHOOT );

	// player "shoot" animation
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

	Vector vecSrc	 = m_pPlayer->GetGunPosition( );
	Vector vecAiming = m_pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );

	m_pPlayer->FireBullets( 6, vecSrc, vecAiming, VECTOR_CONE_10DEGREES, 2048, BULLET_PLAYER_BUCKSHOT, 0 );

	if (!m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);

	if (m_iClip != 0)
		m_flPumpTime = gpGlobals->time + 0.5;

	m_flNextPrimaryAttack = gpGlobals->time + 0.5;
	m_flNextSecondaryAttack = gpGlobals->time + 0.5;
	if (m_iClip != 0)
		m_flTimeWeaponIdle = gpGlobals->time + 5.0;
	else
		m_flTimeWeaponIdle = 0.75;
	m_fInReload = 0;
}


void CShotgun::SecondaryAttack( void )
{
	// don't fire underwater
	if (m_pPlayer->pev->waterlevel == 3)
	{
		PlayEmptySound( );
		m_flNextPrimaryAttack = gpGlobals->time + 0.5;
		return;
	}

	if (m_iClip == 1)
	{
		PlayEmptySound( );
		return;
	}

	if (m_iClip <= 0)
	{
		Reload( );
		if (m_iClip == 0)
			PlayEmptySound( );
		return;
	}

	m_pPlayer->m_iWeaponVolume = LOUD_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = NORMAL_GUN_FLASH;

	m_iClip -= 2;

	m_pPlayer->pev->effects = (int)(m_pPlayer->pev->effects) | EF_MUZZLEFLASH;

	m_pPlayer->pev->punchangle.x -= 10;

	UTIL_MakeVectors( m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle );

	// sic: the shells aren't ejected any more
	Vector vecShellVelocity = m_pPlayer->pev->velocity 
							+ gpGlobals->v_right * RANDOM_FLOAT(50,70) 
							+ gpGlobals->v_up * RANDOM_FLOAT(100,150) 
							+ gpGlobals->v_forward * 25;
	vecShellVelocity		= m_pPlayer->pev->velocity 
							+ gpGlobals->v_right * RANDOM_FLOAT(50,70) 
							+ gpGlobals->v_up * RANDOM_FLOAT(100,150) 
							+ gpGlobals->v_forward * 25;

	EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/dbarrel1.wav", RANDOM_FLOAT(0.98, 1.0), ATTN_NORM, 0, 85 + RANDOM_LONG(0,0x1f));

	// player "shoot" animation
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

	Vector vecSrc	 = m_pPlayer->GetGunPosition( );
	Vector vecAiming = m_pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );

	m_pPlayer->FireBullets( 12, vecSrc, vecAiming, VECTOR_CONE_20DEGREES, 2048, BULLET_PLAYER_BUCKSHOT, 1 );

	if (!m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);

	if (m_iClip != 0)
		m_flPumpTime = gpGlobals->time + 0.95;

	m_flNextPrimaryAttack = gpGlobals->time + 1.5;
	m_flNextSecondaryAttack = gpGlobals->time + 1.5;
	if (m_iClip != 0)
		m_flTimeWeaponIdle = gpGlobals->time + 6.0;
	else
		m_flTimeWeaponIdle = 1.5;

	m_fInReload = 0;
}


void CShotgun::Reload( void )
{
	// Wanted: only an empty gun reloads, both shells at once
	if (m_iClip >= 1)
		return;

	if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 || m_iClip == 2)	// sic: the clip test can no longer pass
		return;

	if (m_flNextReload > gpGlobals->time)
		return;

	// don't reload until recoil is done
	if (m_flNextPrimaryAttack > gpGlobals->time)
		return;

	// check to see if we're ready to reload
	if (m_fInReload == 0)
	{
		SendWeaponAnim( SHOTGUN_RELOAD );
		m_fInReload = 1;
		m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.6;
		m_flTimeWeaponIdle = gpGlobals->time + 3.555;
		m_flNextPrimaryAttack = gpGlobals->time + 3.555;
		m_flNextSecondaryAttack = gpGlobals->time + 3.555;
		return;
	}
	else if (m_fInReload == 1)
	{
		if (m_flTimeWeaponIdle > gpGlobals->time)
			return;
		// was waiting for gun to move to side
		m_fInReload = 2;

		if (RANDOM_LONG(0,1))
			EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/reload1.wav", 1, ATTN_NORM, 0, 85 + RANDOM_LONG(0,0x1f));
		else
			EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/reload3.wav", 1, ATTN_NORM, 0, 85 + RANDOM_LONG(0,0x1f));

		SendWeaponAnim( SHOTGUN_RELOAD );

		m_flNextReload = gpGlobals->time + 0.5;
		m_flTimeWeaponIdle = gpGlobals->time + 0.5;
	}
	else
	{
		// Add them to the clip
		m_iClip += 2;
		m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] -= 2;
		m_fInReload = 1;
	}
}


void CShotgun::WeaponIdle( void )
{
	m_pPlayer->m_iFOV = 0;

	ResetEmptySound( );

	m_pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );

	if (m_flTimeWeaponIdle > gpGlobals->time)
		return;

	int iAnim;
	switch ( RANDOM_LONG( 0, 2 ) )
	{
	case 0:
		iAnim = SHOTGUN_IDLE1;
		break;
	case 1:
		iAnim = SHOTGUN_IDLE2;
		break;
	case 2:
		iAnim = SHOTGUN_IDLE3;
		break;
	}
	SendWeaponAnim( iAnim );

	m_flTimeWeaponIdle = gpGlobals->time + RANDOM_FLOAT( 3, 6 );
}



class CShotgunAmmo : public CBasePlayerAmmo
{
	void Spawn( void )
	{ 
		Precache( );
		SET_MODEL(ENT(pev), "models/w_shotbox.mdl");
		CBasePlayerAmmo::Spawn( );
	}
	void Precache( void )
	{
		PRECACHE_MODEL ("models/w_shotbox.mdl");
		PRECACHE_SOUND("items/9mmclip1.wav");
	}
	BOOL AddAmmo( CBaseEntity *pOther ) 
	{ 
		if (pOther->GiveAmmo( 12, "buckshot", 32 ) != -1)
		{
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM);
			return TRUE;
		}
		return FALSE;
	}
};
LINK_ENTITY_TO_CLASS( ammo_buckshot, CShotgunAmmo );

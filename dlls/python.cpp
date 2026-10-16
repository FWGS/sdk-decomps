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
#if !defined( OEM_BUILD ) && !defined( HLDEMO_BUILD )

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "weapons.h"
#include "monsters.h"
#include "player.h"
#include "gamerules.h"

// Wanted: the pistol (a single-action revolver). The primary attack is an
// aimed shot; the secondary fans the hammer: faster, wider and with more
// kick. Class names kept from the SDK; retail has none for the weapon.

// sequences of models/v_pistol.mdl
enum python_e {
	PYTHON_IDLE1 = 0,
	PYTHON_FIDGET,
	PYTHON_FIRE1,
	PYTHON_RELOAD,
	PYTHON_HOLSTER,
	PYTHON_DRAW,
	PYTHON_IDLE2,
	PYTHON_IDLE3,
	PYTHON_QUICKFIRE_READY,	// raise the hand to the hammer
	PYTHON_QUICKFIRE_SHOOT,
	PYTHON_QUICKFIRE_RELAX,
};

class CPython : public CBasePlayerWeapon
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
	void Holster( int skiplocal = 0 );
	void Reload( void );
	void WeaponIdle( void );
	float m_flSoundDelay;	// set by Reload, no longer read

	BOOL m_fQuickFire;	// name ours; fanning the hammer (secondary attack)
	BOOL m_fInZoom;// don't save this. 
};
LINK_ENTITY_TO_CLASS( weapon_python, CPython );
LINK_ENTITY_TO_CLASS( weapon_pistol, CPython );

int CPython::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "pistol";
	p->iMaxAmmo1 = 36;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = 6;
	p->iFlags = 0;
	p->iSlot = 1;
	p->iPosition = 1;
	p->iId = m_iId = WEAPON_PISTOL;
	p->iWeight = PYTHON_WEIGHT;

	return 1;
}

int CPython::AddToPlayer( CBasePlayer *pPlayer )
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

void CPython::Spawn( )
{
	pev->classname = MAKE_STRING("weapon_pistol"); // hack to allow for old names
	Precache( );
	m_iId = WEAPON_PISTOL;
	SET_MODEL(ENT(pev), "models/w_pistol.mdl");

	m_iDefaultAmmo = 12;

	FallInit();// get ready to fall down.
}


void CPython::Precache( void )
{
	PRECACHE_MODEL("models/v_pistol.mdl");
	PRECACHE_MODEL("models/w_pistol.mdl");
	PRECACHE_MODEL("models/p_pistol.mdl");

	PRECACHE_MODEL("models/w_pistolammobox.mdl");
	PRECACHE_SOUND("items/9mmclip1.wav");              

	PRECACHE_SOUND ("weapons/pistol_reload1.wav");
	PRECACHE_SOUND ("weapons/pistol_cock1.wav");
	PRECACHE_SOUND ("weapons/pistol_shot1.wav");
	PRECACHE_SOUND ("weapons/pistol_shot2.wav");
}

BOOL CPython::Deploy( )
{
	if ( g_pGameRules->IsMultiplayer() )
	{
		// enable laser sight geometry.
		pev->body = 1;
	}
	else
	{
		pev->body = 0;
	}

	return DefaultDeploy( "models/v_pistol.mdl", "models/p_pistol.mdl", PYTHON_DRAW, "python" );
}


void CPython::Holster( int skiplocal /* = 0 */ )
{
	m_fInReload = FALSE;// cancel any reload in progress.

	if ( m_fInZoom )
	{
		SecondaryAttack();
	}

	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 1.0;
	m_flTimeWeaponIdle = gpGlobals->time + 10 + RANDOM_FLOAT ( 0, 5 );
	SendWeaponAnim( PYTHON_HOLSTER );
}

// Wanted: fan the hammer. The first press raises the hand and fires at once;
// held, it fires every 0.318 s until WeaponIdle lowers the hand.
void CPython::SecondaryAttack( void )
{
	if ( m_iClip <= 0 )
	{
		m_flTimeWeaponIdle = gpGlobals->time + 0.5;
		return;
	}

	// don't fire underwater
	if ( m_pPlayer->pev->waterlevel == 3 )
	{
		EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/pistol_cock1.wav", 0.8, ATTN_NORM);
		m_flNextPrimaryAttack = gpGlobals->time + 0.5;
		m_flNextSecondaryAttack = gpGlobals->time + 0.5;
		m_flTimeWeaponIdle = gpGlobals->time + 0.5;
		return;
	}

	if ( !m_fQuickFire )
	{
		SendWeaponAnim( PYTHON_QUICKFIRE_READY );
		m_fQuickFire = TRUE;
		m_flTimeWeaponIdle = gpGlobals->time + 0.5;
		m_flNextPrimaryAttack = gpGlobals->time + 1.125;
		m_flNextSecondaryAttack = gpGlobals->time + 0.375;
	}

	if ( m_fQuickFire == TRUE )
	{
		m_pPlayer->m_iWeaponVolume = LOUD_GUN_VOLUME;
		m_pPlayer->m_iWeaponFlash = BRIGHT_GUN_FLASH;

		m_iClip--;

		SendWeaponAnim( PYTHON_QUICKFIRE_SHOOT );

		// player "shoot" animation
		m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

		m_pPlayer->pev->effects = (int)(m_pPlayer->pev->effects) | EF_MUZZLEFLASH;

		switch( RANDOM_LONG( 0, 1 ) )
		{
		case 0:
			EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/pistol_shot1.wav", RANDOM_FLOAT(0.8, 0.9), ATTN_NORM);
			break;
		case 1:
			EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/pistol_shot2.wav", RANDOM_FLOAT(0.8, 0.9), ATTN_NORM);
			break;
		}

		UTIL_MakeVectors( m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle );

		Vector vecSrc	 = m_pPlayer->GetGunPosition( );
		Vector vecAiming = m_pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );
		m_pPlayer->FireBullets( 1, vecSrc, vecAiming, VECTOR_CONE_3DEGREES, 8192, BULLET_PLAYER_PISTOL, 0 );

		if (!m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
			// HEV suit - indicate out of ammo condition
			m_pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);

		m_pPlayer->pev->punchangle.x = RANDOM_FLOAT( -3, 3 );
		m_pPlayer->pev->punchangle.y = RANDOM_FLOAT( -3, 3 );

		m_flTimeWeaponIdle = gpGlobals->time + 0.5;
		m_flNextPrimaryAttack = gpGlobals->time + 1.125;
		m_flNextSecondaryAttack = gpGlobals->time + 0.318;
	}
}

void CPython::PrimaryAttack()
{
	// don't fire underwater
	if (m_pPlayer->pev->waterlevel == 3)
	{
		EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/pistol_cock1.wav", 0.8, ATTN_NORM);
		m_flNextPrimaryAttack = gpGlobals->time + 0.15;
		return;
	}

	if (m_iClip <= 0)
	{
		if (!m_fFireOnEmpty)
			Reload( );
		else
		{
			EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/pistol_cock1.wav", 0.8, ATTN_NORM);
			m_flNextPrimaryAttack = gpGlobals->time + 0.15;
		}

		return;
	}

	m_pPlayer->m_iWeaponVolume = LOUD_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = BRIGHT_GUN_FLASH;

	m_iClip--;

	SendWeaponAnim( PYTHON_FIRE1 );

	// player "shoot" animation
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

	m_pPlayer->pev->effects = (int)(m_pPlayer->pev->effects) | EF_MUZZLEFLASH;

	switch( RANDOM_LONG( 0, 1 ) )
	{
	case 0:
		EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/pistol_shot1.wav", RANDOM_FLOAT(0.8, 0.9), ATTN_NORM);
		break;
	case 1:
		EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/pistol_shot2.wav", RANDOM_FLOAT(0.8, 0.9), ATTN_NORM);
		break;
	}

	UTIL_MakeVectors( m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle );

	Vector vecSrc	 = m_pPlayer->GetGunPosition( );
	Vector vecAiming = m_pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );
	m_pPlayer->FireBullets( 1, vecSrc, vecAiming, VECTOR_CONE_1DEGREES, 8192, BULLET_PLAYER_PISTOL, 0 );

	if (!m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);

	m_flNextPrimaryAttack = gpGlobals->time + 0.75;
	m_flTimeWeaponIdle = gpGlobals->time + RANDOM_FLOAT ( 10, 15 );

	m_pPlayer->pev->punchangle.x = RANDOM_FLOAT( -2, 2 );
	m_pPlayer->pev->punchangle.y = RANDOM_FLOAT( -2, 2 );
}


void CPython::Reload( void )
{
	if ( m_fInZoom )
	{
		m_fInZoom = FALSE;
		m_pPlayer->m_iFOV = 0;  // 0 means reset to default fov
	}

	if (DefaultReload( 6, PYTHON_RELOAD, 2.0 ))
	{
		EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/pistol_reload1.wav", 0.8, ATTN_NORM);
		m_flSoundDelay = gpGlobals->time + 1.5;
		m_fQuickFire = FALSE;
	}
}


void CPython::WeaponIdle( void )
{
	if (m_flTimeWeaponIdle > gpGlobals->time)
		return;

	if ( m_fQuickFire )
	{
		// lower the fanning hand
		SendWeaponAnim( PYTHON_QUICKFIRE_RELAX );
		m_flTimeWeaponIdle = gpGlobals->time + 1.125;
		m_flNextPrimaryAttack = gpGlobals->time + 0.5;
		m_flNextSecondaryAttack = gpGlobals->time + 0.5;
		m_fQuickFire = FALSE;
		return;
	}

	m_pPlayer->m_iFOV = 0;

	int iAnim;
	float flRand = RANDOM_FLOAT(0, 1);
	if (flRand <= 0.5)
	{
		iAnim = PYTHON_IDLE1;
		m_flTimeWeaponIdle = gpGlobals->time + 2.333;
	}
	else if (flRand <= 0.7)
	{
		iAnim = PYTHON_IDLE2;
		m_flTimeWeaponIdle = gpGlobals->time + 2.0;
	}
	else if (flRand <= 0.9)
	{
		iAnim = PYTHON_IDLE3;
		m_flTimeWeaponIdle = gpGlobals->time + 2.933;
	}
	else
	{
		iAnim = PYTHON_FIDGET;
		m_flTimeWeaponIdle = gpGlobals->time + 5.666;
	}
	SendWeaponAnim( iAnim );
}



class CPythonAmmo : public CBasePlayerAmmo
{
	void Spawn( void )
	{ 
		Precache( );
		SET_MODEL(ENT(pev), "models/w_pistolammobox.mdl");
		CBasePlayerAmmo::Spawn( );
	}
	void Precache( void )
	{
		PRECACHE_MODEL ("models/w_pistolammobox.mdl");
		PRECACHE_SOUND("items/9mmclip1.wav");
	}
	BOOL AddAmmo( CBaseEntity *pOther ) 
	{ 
		if (pOther->GiveAmmo( 6, "Pistol", 36 ) != -1)	// retail's capital P; ammo names compare without case
		{
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM);
			return TRUE;
		}
		return FALSE;
	}
};
LINK_ENTITY_TO_CLASS( ammo_pistol, CPythonAmmo );


#endif

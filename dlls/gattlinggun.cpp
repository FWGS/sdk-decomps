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
// Wanted: the Gatling gun (weapon_gattlinggun, ammo_gattlinggun), made
// from the original SDK's MP5. It spins up before it fires, spins down
// when released, and jams after long bursts; the secondary attack clears
// a jam. The file name and the class names are ours.
//

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"
#include "soundent.h"
#include "gamerules.h"

// sequences of models/v_gattlinggun.mdl
enum gattlinggun_e
{
	GATTLINGGUN_IDLE = 0,
	GATTLINGGUN_IDLE2,
	GATTLINGGUN_IDLE3,
	GATTLINGGUN_SPINUP,
	GATTLINGGUN_FIRE,
	GATTLINGGUN_SPINDOWN,
	GATTLINGGUN_DRAW,
	GATTLINGGUN_HOLSTER,
	GATTLINGGUN_JAMB,
	GATTLINGGUN_UNJAMB,
	GATTLINGGUN_RELOAD,
	GATTLINGGUN_DRY_FIRE,
};


class CGattlingGun : public CBasePlayerWeapon
{
public:
	void Spawn( void );
	void Precache( void );
	int iItemSlot( void ) { return 4; }
	int GetItemInfo(ItemInfo *p);
	int AddToPlayer( CBasePlayer *pPlayer );

	void PrimaryAttack( void );
	void SecondaryAttack( void );
	BOOL Deploy( void );
	void Holster( void );	// sic: doesn't override CBasePlayerItem::Holster( int ), never called
	void Reload( void );
	void WeaponIdle( void );

	// names ours
	float m_flSpinUpTime;	// when the barrels are up to speed; 0 when not spun up
	int m_iUnused80;		// no code touches it
	int m_iShotsFired;		// since the spin-up; long bursts jam
	float m_flNextAnimTime;	// the MP5's, set and never read
	float m_flJamRoll;		// last jam roll, 0-100
	BOOL m_fJammed;
	BOOL m_fSpinning;		// spun up: play the spin-down when released
	int m_iShell;
	float m_flUnjamSoundTime;	// when the unjam sound may play
};
LINK_ENTITY_TO_CLASS( weapon_gattlinggun, CGattlingGun );


void CGattlingGun::Spawn( )
{
	pev->classname = MAKE_STRING("weapon_gattlinggun"); // hack to allow for old names
	Precache( );
	SET_MODEL(ENT(pev), "models/w_gattlinggun.mdl");
	m_iId = WEAPON_GATTLINGGUN;

	m_iDefaultAmmo = 100;

	FallInit();// get ready to fall down.
}


void CGattlingGun::Precache( void )
{
	PRECACHE_MODEL("models/v_gattlinggun.mdl");
	PRECACHE_MODEL("models/w_gattlinggun.mdl");
	PRECACHE_MODEL("models/p_gattlinggun.mdl");

	m_iShell = PRECACHE_MODEL ("models/shell.mdl");// brass shellTE_MODEL

	PRECACHE_MODEL("models/w_gattlinggun_belt.mdl");

	PRECACHE_SOUND ("weapons/gat_spinup.wav");
	PRECACHE_SOUND ("weapons/gat_shoot1.wav");
	PRECACHE_SOUND ("weapons/gat_shoot2.wav");
	PRECACHE_SOUND ("weapons/gat_shoot3.wav");
	PRECACHE_SOUND ("weapons/gat_spindown.wav");
	PRECACHE_SOUND ("weapons/gat_reload.wav");
	PRECACHE_SOUND ("weapons/gat_jamb.wav");
	PRECACHE_SOUND ("weapons/gat_unjamb.wav");
	PRECACHE_SOUND ("weapons/gat_dryfire.wav");
	PRECACHE_SOUND ("player/gat_jamb.wav");
	PRECACHE_SOUND ("items/9mmclip1.wav");
}

int CGattlingGun::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "gattlinggun";
	p->iMaxAmmo1 = 200;
	p->iMaxClip = 100;
	p->iSlot = 3;
	p->iPosition = 0;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_GATTLINGGUN;
	p->iWeight = 15;

	return 1;
}

int CGattlingGun::AddToPlayer( CBasePlayer *pPlayer )
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

BOOL CGattlingGun::Deploy( )
{
	return DefaultDeploy( "models/v_gattlinggun.mdl", "models/p_gattlinggun.mdl", GATTLINGGUN_DRAW, "gattlinggun" );
}

void CGattlingGun::Holster( void )
{
	m_pPlayer->m_flNextAttack = gpGlobals->time + 0.5;
	SendWeaponAnim( GATTLINGGUN_HOLSTER );
	EMIT_SOUND( ENT(m_pPlayer->pev), CHAN_WEAPON, "common/null.wav", 1, ATTN_NORM );
}


void CGattlingGun::PrimaryAttack()
{
	if ( m_fJammed == TRUE )
		return;

	if ( m_flSpinUpTime == 0 )
	{
		// spin the barrels up first
		m_fSpinning = TRUE;
		m_flSpinUpTime = gpGlobals->time + 1.5;
		SendWeaponAnim( GATTLINGGUN_SPINUP );
		EMIT_SOUND( ENT(pev), CHAN_WEAPON, "weapons/gat_spinup.wav", 1, ATTN_NORM );
		m_flNextPrimaryAttack = gpGlobals->time + 1.5;
		m_flNextSecondaryAttack = gpGlobals->time + 1.5;
		m_flTimeWeaponIdle = gpGlobals->time + 1.5;
		return;
	}

	if ( m_flSpinUpTime > gpGlobals->time )
		return;

	// don't fire underwater
	if (m_pPlayer->pev->waterlevel == 3)
	{
		PlayEmptySound( );
		m_flNextPrimaryAttack = gpGlobals->time + 0.15;
		return;
	}

	if (m_iClip <= 0)
	{
		SendWeaponAnim( GATTLINGGUN_DRY_FIRE );
		EMIT_SOUND( ENT(pev), CHAN_WEAPON, "weapons/gat_dryfire.wav", 1, ATTN_NORM );
		m_flNextPrimaryAttack = gpGlobals->time + 0.15;
		m_flTimeWeaponIdle = gpGlobals->time + 0.5;
		return;
	}

	// long bursts jam: a 15% chance a shot after 75, 5% after 40
	if ( m_iShotsFired >= 75 )
	{
		m_flJamRoll = RANDOM_FLOAT( 0, 100 );
		if ( m_flJamRoll >= 85 )
		{
			m_flUnjamSoundTime = 0.5;	// sic: not added to the time
			m_fSpinning = FALSE;
			m_fJammed = TRUE;
			SendWeaponAnim( GATTLINGGUN_JAMB );
			EMIT_SOUND( ENT(pev), CHAN_WEAPON, "weapons/gat_jamb.wav", 1, ATTN_NORM );
			EMIT_SOUND( ENT(pev), CHAN_BODY, "player/gat_jamb.wav", 1, ATTN_NORM );
			m_flTimeWeaponIdle = gpGlobals->time + 1.0;
			return;
		}
	}

	if ( m_iShotsFired >= 40 )
	{
		m_flJamRoll = RANDOM_FLOAT( 0, 100 );
		if ( m_flJamRoll >= 95 )
		{
			m_flUnjamSoundTime = 0.5;	// sic
			m_fSpinning = FALSE;
			m_fJammed = TRUE;
			SendWeaponAnim( GATTLINGGUN_JAMB );
			EMIT_SOUND( ENT(pev), CHAN_WEAPON, "weapons/gat_jamb.wav", 1, ATTN_NORM );
			EMIT_SOUND( ENT(pev), CHAN_BODY, "player/gat_jamb.wav", 1, ATTN_NORM );
			m_flTimeWeaponIdle = gpGlobals->time + 1.0;
			return;
		}
	}

	m_pPlayer->m_iWeaponVolume = LOUD_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = BRIGHT_GUN_FLASH;

	m_iClip--;
	m_iShotsFired++;

	m_pPlayer->pev->effects = (int)(m_pPlayer->pev->effects) | EF_MUZZLEFLASH;

	if (1 || m_flNextAnimTime < gpGlobals->time)	// the MP5's; the dead test is needed for retail's inlining
	{
		SendWeaponAnim( GATTLINGGUN_FIRE );
		m_flNextAnimTime = gpGlobals->time + 0.1;
	}

	// player "shoot" animation
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

	switch( RANDOM_LONG(0,2) )
	{
	case 0: EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/gat_shoot1.wav", 1, ATTN_NORM, 0, 94 + RANDOM_LONG(0,0xf)); break;
	case 1: EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/gat_shoot2.wav", 1, ATTN_NORM, 0, 94 + RANDOM_LONG(0,0xf)); break;
	case 2: EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/gat_shoot3.wav", 1, ATTN_NORM, 0, 94 + RANDOM_LONG(0,0xf)); break;
	}

	UTIL_MakeVectors( m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle );

	Vector	vecShellVelocity = m_pPlayer->pev->velocity
							 + gpGlobals->v_right * RANDOM_FLOAT(50,70)
							 + gpGlobals->v_up * RANDOM_FLOAT(100,150)
							 + gpGlobals->v_forward * 25;
	EjectBrass ( pev->origin + m_pPlayer->pev->view_ofs
					+ gpGlobals->v_up * -12
					+ gpGlobals->v_forward * 10
					+ gpGlobals->v_right * 4, vecShellVelocity, pev->angles.y, m_iShell, TE_BOUNCE_SHELL);

	Vector vecSrc	 = m_pPlayer->GetGunPosition( );
	Vector vecAiming = m_pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );

	if ( g_pGameRules->IsDeathmatch() )
	{
		// optimized multiplayer. Widened to make it easier to hit a moving player
		m_pPlayer->FireBullets( 1, vecSrc, vecAiming, VECTOR_CONE_6DEGREES, 8192, BULLET_PLAYER_GATTLINGGUN, 2 );
	}
	else
	{
		// single player spread
		m_pPlayer->FireBullets( 1, vecSrc, vecAiming, VECTOR_CONE_3DEGREES, 8192, BULLET_PLAYER_GATTLINGGUN, 2 );
	}

	if (!m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);

	m_flNextPrimaryAttack = m_flNextPrimaryAttack + 0.1;
	if (m_flNextPrimaryAttack < gpGlobals->time)
		m_flNextPrimaryAttack = gpGlobals->time + 0.1;

	m_flTimeWeaponIdle = gpGlobals->time + RANDOM_FLOAT ( 3, 5 );

	m_pPlayer->pev->punchangle.x = RANDOM_FLOAT( -3, 3 );
	m_pPlayer->pev->punchangle.y = RANDOM_FLOAT( -3, 3 );

	// the recoil pushes the player back
	UTIL_MakeVectors( m_pPlayer->pev->v_angle );
	Vector vecKick( gpGlobals->v_forward * -50 );
	m_pPlayer->pev->velocity = vecKick;
}


// clear a jam
void CGattlingGun::SecondaryAttack( void )
{
	if ( m_fJammed == FALSE )
		return;

	m_flSpinUpTime = 0;
	m_iShotsFired = 0;
	SendWeaponAnim( GATTLINGGUN_UNJAMB );

	if ( m_flUnjamSoundTime != 0 && m_flUnjamSoundTime <= gpGlobals->time )
	{
		EMIT_SOUND( ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/gat_unjamb.wav", 1, ATTN_NORM );
		m_flUnjamSoundTime = 0;
	}

	m_flTimeWeaponIdle = gpGlobals->time + 1.266;
	m_flNextPrimaryAttack = gpGlobals->time + 1.266;
	m_flNextSecondaryAttack = gpGlobals->time + 1.266;
	m_fJammed = FALSE;
}


void CGattlingGun::Reload( void )
{
	if ( m_fSpinning == TRUE )
	{
		// spin down first
		EMIT_SOUND( ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/gat_spindown.wav", 1, ATTN_NORM );
		SendWeaponAnim( GATTLINGGUN_SPINDOWN );
		m_flTimeWeaponIdle = gpGlobals->time + 1.3;
		m_flNextPrimaryAttack = gpGlobals->time + 1.3;
		m_flNextSecondaryAttack = gpGlobals->time + 1.3;
		m_fSpinning = FALSE;
		return;
	}

	DefaultReload( 100, GATTLINGGUN_RELOAD, 2.853 );
	m_flSpinUpTime = 0;
	m_iShotsFired = 0;
}


void CGattlingGun::WeaponIdle( void )
{
	m_pPlayer->m_iFOV = 0;

	if ( m_flSpinUpTime > gpGlobals->time )
		return;

	m_flSpinUpTime = 0;
	m_iShotsFired = 0;

	ResetEmptySound( );

	m_pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );

	if ( m_fSpinning == TRUE )
	{
		EMIT_SOUND( ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/gat_spindown.wav", 1, ATTN_NORM );
		SendWeaponAnim( GATTLINGGUN_SPINDOWN );
		m_flTimeWeaponIdle = gpGlobals->time + 1.3;
		m_flNextPrimaryAttack = gpGlobals->time + 1.3;
		m_flNextSecondaryAttack = gpGlobals->time + 1.3;
		m_fSpinning = FALSE;
	}

	if ( m_flTimeWeaponIdle > gpGlobals->time )
		return;

	switch ( RANDOM_LONG( 0, 2 ) )
	{
	case 0:
		SendWeaponAnim( GATTLINGGUN_IDLE );
		m_flTimeWeaponIdle = gpGlobals->time + 0.5;
		break;
	case 1:
		SendWeaponAnim( GATTLINGGUN_IDLE2 );
		m_flTimeWeaponIdle = gpGlobals->time + 1.75;
		break;
	case 2:
		SendWeaponAnim( GATTLINGGUN_IDLE3 );
		m_flTimeWeaponIdle = gpGlobals->time + 1.4;
		break;
	}
}



class CGattlingGunAmmo : public CBasePlayerAmmo
{
	void Spawn( void )
	{
		Precache( );
		SET_MODEL(ENT(pev), "models/w_gattlinggun_belt.mdl");
		CBasePlayerAmmo::Spawn( );
	}
	void Precache( void )
	{
		PRECACHE_MODEL ("models/w_gattlinggun_belt.mdl");
		PRECACHE_SOUND("items/9mmclip1.wav");
	}
	BOOL AddAmmo( CBaseEntity *pOther )
	{
		int bResult = (pOther->GiveAmmo( 100, "gattlinggun", 200 ) != -1);
		if (bResult)
		{
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM);
		}
		return bResult;
	}
};
LINK_ENTITY_TO_CLASS( ammo_gattlinggun, CGattlingGunAmmo );

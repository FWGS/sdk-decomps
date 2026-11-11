/***
*
*	Copyright (c) 1999, Valve LLC. All rights reserved.
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

enum glock_e {
	GLOCK_IDLE1 = 0,
	GLOCK_IDLE2,
	GLOCK_IDLE3,
	GLOCK_SHOOT,
	GLOCK_SHOOT_EMPTY,
	GLOCK_RELOAD,
	GLOCK_RELOAD_NOT_EMPTY,
	GLOCK_DRAW,
	GLOCK_HOLSTER,
	GLOCK_ADD_SILENCER
};

LINK_ENTITY_TO_CLASS( weapon_glock, CGlock );
LINK_ENTITY_TO_CLASS( weapon_9mmhandgun, CGlock );


void CGlock::Spawn( )
{
	pev->classname = MAKE_STRING("weapon_9mmhandgun"); // hack to allow for old names
	Precache( );
	m_iId = WEAPON_GLOCK;

	// They Hunger: body 1 is the pistol with the silencer (a map can set it;
	// multiplayer always has it)
	if ( TH_IsMultiplayer() )
		pev->body = 1;

	if ( pev->body == 1 )
		SET_MODEL(ENT(pev), "models/w_silencer.mdl");
	else
		SET_MODEL(ENT(pev), "models/w_9mmhandgun.mdl");

	m_iDefaultAmmo = GLOCK_DEFAULT_GIVE;

	FallInit();// get ready to fall down.
}


void CGlock::Precache( void )
{
	PRECACHE_MODEL("models/v_9mmhandgun.mdl");
	PRECACHE_MODEL("models/w_9mmhandgun.mdl");
	PRECACHE_MODEL("models/p_9mmhandgun.mdl");
	PRECACHE_MODEL("models/w_silencer.mdl");

	m_iShell = PRECACHE_MODEL ("models/shell.mdl");// brass shell

	PRECACHE_SOUND("items/9mmclip1.wav");
	PRECACHE_SOUND("items/9mmclip2.wav");

	PRECACHE_SOUND ("weapons/pl_gun1.wav");//silenced handgun
	PRECACHE_SOUND ("weapons/pl_gun2.wav");//silenced handgun
	PRECACHE_SOUND ("weapons/pl_gun3.wav");//handgun

	m_usFireGlock1 = PRECACHE_EVENT( 1, "events/glock1.sc" );
	m_usFireGlock2 = PRECACHE_EVENT( 1, "events/glock2.sc" );

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase();
}

int CGlock::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "9mm";
	p->iMaxAmmo1 = _9MM_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = GLOCK_MAX_CLIP;
	p->iSlot = 1;
	p->iPosition = 0;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_GLOCK;
	p->iWeight = GLOCK_WEIGHT;

	return 1;
}

int CGlock::AddToPlayer( CBasePlayer *pPlayer )
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

BOOL CGlock::Deploy( )
{
	// pev->body = 1;
	return DefaultDeploy( "models/v_9mmhandgun.mdl", "models/p_9mmhandgun.mdl", GLOCK_DRAW, "onehanded", /*UseDecrement() ? 1 : 0*/ 0 );
}

void CGlock::SecondaryAttack( void )
{
	// They Hunger, single player: the silencer goes off or on. WeaponIdle
	// changes the body when m_pPlayer->m_flNextAttack has passed
	// (CBasePlayerWeapon::ItemPostFrame calls it for m_fInReload > 1).
	if ( !TH_IsMultiplayer() )
	{
		if ( pev->body == 1 )
		{
			SendWeaponAnim( GLOCK_ADD_SILENCER );
			m_fInReload = 2;
			m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 2.75;
			m_flNextPrimaryAttack = m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 3.7;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
		}
		else
		{
			SendWeaponAnim( GLOCK_HOLSTER );
			m_fInReload = 3;
			m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.95;
			m_flNextPrimaryAttack = m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 1.9;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
		}
	}
	else
	{
		GlockFire( 0.1, 0.2, FALSE );
	}
}

void CGlock::PrimaryAttack( void )
{
	// They Hunger: the slow accurate shot with the silencer (always on in
	// multiplayer), without it the fast one that was the secondary attack
	if ( TH_IsMultiplayer() || pev->body == 1 )
		GlockFire( 0.01, 0.3, TRUE );
	else
		GlockFire( 0.1, 0.2, FALSE );
}

void CGlock::GlockFire( float flSpread , float flCycleTime, BOOL fUseAutoAim )
{
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

	m_pPlayer->pev->effects = (int)(m_pPlayer->pev->effects) | EF_MUZZLEFLASH;

#if defined ( OLD_WEAPONS )
	if (m_iClip != 0)
		SendWeaponAnim( GLOCK_SHOOT );
	else
		SendWeaponAnim( GLOCK_SHOOT_EMPTY );
#endif

	int flags;

#if defined( CLIENT_WEAPONS )
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif

	PLAYBACK_EVENT_FULL( flags, m_pPlayer->edict(), fUseAutoAim ? m_usFireGlock1 : m_usFireGlock2, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, 0.0, 0.0, 0, 0, ( m_iClip == 0 ) ? 1 : 0, 0 );

	// player "shoot" animation
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

#if defined ( OLD_WEAPONS )
	UTIL_MakeVectors( m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle );
		
	Vector	vecShellVelocity = m_pPlayer->pev->velocity 
							 + gpGlobals->v_right * RANDOM_FLOAT(50,70) 
							 + gpGlobals->v_up * RANDOM_FLOAT(100,150) 
							 + gpGlobals->v_forward * 25;
	EjectBrass ( pev->origin + m_pPlayer->pev->view_ofs + gpGlobals->v_up * -12 + gpGlobals->v_forward * 32 + gpGlobals->v_right * 6 , vecShellVelocity, pev->angles.y, m_iShell, TE_BOUNCE_SHELL ); 
#endif

	// silenced
	if (pev->body == 1)
	{
		m_pPlayer->m_iWeaponVolume = 100;	// They Hunger: half of QUIET_GUN_VOLUME
		m_pPlayer->m_iWeaponFlash = DIM_GUN_FLASH;
#if defined ( OLD_WEAPONS )
		switch(RANDOM_LONG(0,1))
		{
		case 0:
			EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/pl_gun1.wav", RANDOM_FLOAT(0.9, 1.0), ATTN_NORM);
			break;
		case 1:
			EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/pl_gun2.wav", RANDOM_FLOAT(0.9, 1.0), ATTN_NORM);
			break;
		}
#endif
	}
	else
	{
		// non-silenced
		m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;
		m_pPlayer->m_iWeaponFlash = NORMAL_GUN_FLASH;
#if defined ( OLD_WEAPONS )
		EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/pl_gun3.wav", RANDOM_FLOAT(0.92, 1.0), ATTN_NORM, 0, 98 + RANDOM_LONG(0,3));
#endif
	}

	Vector vecSrc	 = m_pPlayer->GetGunPosition( );
	Vector vecAiming;
	
	if ( fUseAutoAim )
	{
		vecAiming = m_pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );
	}
	else
	{
		vecAiming = gpGlobals->v_forward;
	}

	m_pPlayer->FireBullets( 1, vecSrc, vecAiming, Vector( flSpread, flSpread, flSpread ), 8192, BULLET_PLAYER_9MM, 0 );

	m_flNextPrimaryAttack = m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + flCycleTime;

	if (!m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );

#if defined ( OLD_WEAPONS )
	m_pPlayer->pev->punchangle.x -= 2;
#endif
}


void CGlock::Reload( void )
{
	int iResult;

	if (m_iClip == 0)
		iResult = DefaultReload( 17, GLOCK_RELOAD, 1.5 );
	else
		iResult = DefaultReload( 18, GLOCK_RELOAD_NOT_EMPTY, 1.5 );

	if (iResult)
	{
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
	}
}



void CGlock::WeaponIdle( void )
{
	ResetEmptySound( );

	m_pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );

	// They Hunger: the end of SecondaryAttack's silencer animations
	if ( m_fInReload == 2 )
	{
		pev->body = 0;
		SendWeaponAnim( GLOCK_DRAW );
		m_fInReload = 0;
	}
	if ( m_fInReload == 3 )
	{
		pev->body = 1;
		SendWeaponAnim( GLOCK_DRAW );
		m_fInReload = 0;
	}

	if ( m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
		return;

	// only idle if the slid isn't back
	if (m_iClip != 0)
	{
		int iAnim;
		float flRand = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 0.0, 1.0 );

		if (flRand <= 0.3 + 0 * 0.75)
		{
			iAnim = GLOCK_IDLE3;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 49.0 / 16;
		}
		else if (flRand <= 0.6 + 0 * 0.875)
		{
			iAnim = GLOCK_IDLE1;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 60.0 / 16.0;
		}
		else
		{
			iAnim = GLOCK_IDLE2;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 40.0 / 16.0;
		}
		SendWeaponAnim( iAnim, 1 );
	}
}








class CGlockAmmo : public CBasePlayerAmmo
{
	void Spawn( void )
	{ 
		Precache( );
		SET_MODEL(ENT(pev), "models/w_9mmclip.mdl");
		CBasePlayerAmmo::Spawn( );
	}
	void Precache( void )
	{
		PRECACHE_MODEL ("models/w_9mmclip.mdl");
		PRECACHE_SOUND("items/9mmclip1.wav");
	}
	BOOL AddAmmo( CBaseEntity *pOther ) 
	{ 
		if (pOther->GiveAmmo( AMMO_GLOCKCLIP_GIVE, "9mm", _9MM_MAX_CARRY ) != -1)
		{
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM);
			return TRUE;
		}
		return FALSE;
	}
};
LINK_ENTITY_TO_CLASS( ammo_glockclip, CGlockAmmo );
LINK_ENTITY_TO_CLASS( ammo_9mmclip, CGlockAmmo );


//=========================================================
// They Hunger: the AP9 (weapon_th_ap9), a machine pistol with
// a three-round burst as secondary attack. Class, enum and
// member names are ours.
//=========================================================
enum ap9_e {
	AP9_IDLE1 = 0,
	AP9_RELOAD,
	AP9_DRAW,
	AP9_SHOOT1,
	AP9_SHOOT2,
	AP9_SHOOT3
};

LINK_ENTITY_TO_CLASS( weapon_th_ap9, CWeaponEinarAP9 );

void CWeaponEinarAP9::Spawn( )
{
	Precache( );
	m_iId = WEAPON_TH_AP9;
	SET_MODEL(ENT(pev), "models/w_ap9.mdl");

	m_iDefaultAmmo = AP9_DEFAULT_GIVE;

	FallInit();// get ready to fall down.
}


void CWeaponEinarAP9::Precache( void )
{
	PRECACHE_MODEL("models/v_ap9.mdl");
	PRECACHE_MODEL("models/w_ap9.mdl");
	PRECACHE_MODEL("models/p_ap9.mdl");

	m_iShell = PRECACHE_MODEL ("models/shell.mdl");// brass shell

	PRECACHE_SOUND("items/9mmclip1.wav");

	PRECACHE_SOUND ("weapons/ap9_fire.wav");
	PRECACHE_SOUND ("weapons/ap9_clipout.wav");
	PRECACHE_SOUND ("weapons/ap9_clipin.wav");
	PRECACHE_SOUND ("weapons/ap9_bolt.wav");
	PRECACHE_SOUND ("items/cliprelease1.wav");

	m_usFireAP9 = PRECACHE_EVENT( 1, "events/ap9.sc" );
}

int CWeaponEinarAP9::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "ap9";
	p->iMaxAmmo1 = AP9_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = AP9_MAX_CLIP;
	p->iSlot = 1;
	p->iPosition = 2;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_TH_AP9;
	p->iWeight = AP9_WEIGHT;

	return 1;
}

int CWeaponEinarAP9::AddToPlayer( CBasePlayer *pPlayer )
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

BOOL CWeaponEinarAP9::Deploy( )
{
	return DefaultDeploy( "models/v_ap9.mdl", "models/p_ap9.mdl", AP9_DRAW, "onehanded", /*UseDecrement() ? 1 : 0*/ 0 );
}

// The burst. m_fInReload counts its shots in twos: with m_fInReload above 1
// CBasePlayerWeapon::ItemPostFrame calls WeaponIdle when
// m_pPlayer->m_flNextAttack has passed, and WeaponIdle fires again until
// the count is 6.
void CWeaponEinarAP9::SecondaryAttack( void )
{
	m_fInReload += 2;
	AP9Fire( 0.035, 0.6, FALSE );
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.04;
}

void CWeaponEinarAP9::PrimaryAttack( void )
{
	AP9Fire( 0.07, 0.125, TRUE );
}

void CWeaponEinarAP9::AP9Fire( float flSpread , float flCycleTime, BOOL fUseAutoAim )
{
	if (m_iClip <= 0)
	{
		if (m_fFireOnEmpty)
		{
			PlayEmptySound();
			m_flNextPrimaryAttack = m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.2;
		}

		return;
	}

	m_iClip--;

	m_pPlayer->pev->effects = (int)(m_pPlayer->pev->effects) | EF_MUZZLEFLASH;

	int flags;

#if defined( CLIENT_WEAPONS )
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif

	// bparam1: primary attack
	PLAYBACK_EVENT_FULL( flags, m_pPlayer->edict(), m_usFireAP9, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, 0.0, 0.0, 0, 0, fUseAutoAim ? 1 : 0, 0 );

	// player "shoot" animation
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

	m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = NORMAL_GUN_FLASH;

	Vector vecSrc	 = m_pPlayer->GetGunPosition( );
	Vector vecAiming = gpGlobals->v_forward;

	if ( TH_IsMultiplayer() )
	{
		m_pPlayer->FireBullets( 1, vecSrc, vecAiming, Vector( flSpread, flSpread, flSpread ), 8192, BULLET_PLAYER_9MM, 0 );
	}
	else
	{
		m_pPlayer->FireBullets( 1, vecSrc, vecAiming, Vector( flSpread, flSpread, flSpread ), 8192, BULLET_PLAYER_MP5, 0 );
	}

	if (!m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);

	m_flNextPrimaryAttack = m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + flCycleTime;

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
}


void CWeaponEinarAP9::Reload( void )
{
	int iResult;

	iResult = DefaultReload( AP9_MAX_CLIP, AP9_RELOAD, 1.8 );

	if (iResult)
	{
		if ( !TH_IsMultiplayer() )
			m_flNextPrimaryAttack = m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 2.9;

		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
	}
}



void CWeaponEinarAP9::WeaponIdle( void )
{
	ResetEmptySound( );

	m_pPlayer->GetAutoaimVector( 0.001 );

	// the rest of a burst (see SecondaryAttack)
	if ( m_fInReload >= 2 )
	{
		if ( m_pPlayer->m_flNextAttack <= UTIL_WeaponTimeBase() )
		{
			SecondaryAttack();
			if ( m_fInReload >= 6 )
				m_fInReload = 0;
		}
	}

	if ( m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
		return;

	// only idle if the slid isn't back
	if (m_iClip != 0)
	{
		int iAnim;

		iAnim = AP9_IDLE1;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 20;
		SendWeaponAnim( iAnim, 1 );
	}
}


class CAmmoEinarAP9 : public CBasePlayerAmmo	// name ours
{
	void Spawn( void )
	{ 
		Precache( );
		SET_MODEL(ENT(pev), "models/w_ap9clip.mdl");
		CBasePlayerAmmo::Spawn( );
	}
	void Precache( void )
	{
		PRECACHE_MODEL ("models/w_ap9clip.mdl");
		PRECACHE_SOUND("items/9mmclip1.wav");
	}
	BOOL AddAmmo( CBaseEntity *pOther ) 
	{ 
		if (pOther->GiveAmmo( AMMO_AP9CLIP_GIVE, "ap9", AP9_MAX_CARRY ) != -1)
		{
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM);
			return TRUE;
		}
		return FALSE;
	}
};
LINK_ENTITY_TO_CLASS( ammo_th_ap9, CAmmoEinarAP9 );


//=========================================================
// They Hunger: the Taurus pistol (weapon_th_taurus)
//=========================================================
enum taurus_e {
	TAURUS_IDLE1 = 0,
	TAURUS_IDLE2,
	TAURUS_IDLE3,
	TAURUS_SHOOT,
	TAURUS_SHOOT2,
	TAURUS_SHOOT3,
	TAURUS_SHOOT_EMPTY,
	TAURUS_RELOAD,
	TAURUS_RELOAD2,
	TAURUS_DRAW,
	TAURUS_DRAW2
};

LINK_ENTITY_TO_CLASS( weapon_th_taurus, CWeaponEinarTaurus );

void CWeaponEinarTaurus::Spawn( )
{
	Precache( );
	m_iId = WEAPON_TH_TAURUS;
	SET_MODEL(ENT(pev), "models/w_taurus.mdl");

	m_iDefaultAmmo = TAURUS_DEFAULT_GIVE;

	FallInit();// get ready to fall down.
}


void CWeaponEinarTaurus::Precache( void )
{
	PRECACHE_MODEL("models/v_taurus.mdl");
	PRECACHE_MODEL("models/w_taurus.mdl");
	PRECACHE_MODEL("models/p_taurus.mdl");

	m_iShell = PRECACHE_MODEL ("models/shell.mdl");// brass shell

	PRECACHE_SOUND("items/9mmclip1.wav");

	PRECACHE_SOUND ("weapons/tau_fire.wav");
	PRECACHE_SOUND ("weapons/tau_clipout.wav");
	PRECACHE_SOUND ("weapons/tau_clipin.wav");
	PRECACHE_SOUND ("weapons/tau_release.wav");
	PRECACHE_SOUND ("weapons/tau_back.wav");

	m_usFireTaurus = PRECACHE_EVENT( 1, "events/taurus.sc" );
}

int CWeaponEinarTaurus::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "taurus";
	p->iMaxAmmo1 = TAURUS_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = TAURUS_MAX_CLIP;
	p->iSlot = 1;
	p->iPosition = 3;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_TH_TAURUS;
	p->iWeight = TAURUS_WEIGHT;

	return 1;
}

int CWeaponEinarTaurus::AddToPlayer( CBasePlayer *pPlayer )
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

BOOL CWeaponEinarTaurus::Deploy( )
{
	float flRand = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 0.0, 1.0 );	// sic: not used

	return DefaultDeploy( "models/v_taurus.mdl", "models/p_taurus.mdl", TAURUS_DRAW2, "onehanded", /*UseDecrement() ? 1 : 0*/ 0 );
}

void CWeaponEinarTaurus::SecondaryAttack( void )
{
}

void CWeaponEinarTaurus::PrimaryAttack( void )
{
	TaurusFire( 0.04, 0.25, TRUE );
}

void CWeaponEinarTaurus::TaurusFire( float flSpread , float flCycleTime, BOOL fUseAutoAim )
{
	if (m_iClip <= 0)
	{
		if (m_fFireOnEmpty)
		{
			PlayEmptySound();
			m_flNextPrimaryAttack = m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.2;
		}

		return;
	}

	m_iClip--;

	m_pPlayer->pev->effects = (int)(m_pPlayer->pev->effects) | EF_MUZZLEFLASH;

	int flags;

#if defined( CLIENT_WEAPONS )
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif

	PLAYBACK_EVENT_FULL( flags, m_pPlayer->edict(), m_usFireTaurus, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, 0.0, 0.0, 0, 0, ( m_iClip == 0 ) ? 1 : 0, 0 );

	// player "shoot" animation
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

	m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = NORMAL_GUN_FLASH;

	Vector vecSrc	 = m_pPlayer->GetGunPosition( );
	Vector vecAiming = gpGlobals->v_forward;

	m_pPlayer->FireBullets( 1, vecSrc, vecAiming, Vector( flSpread, flSpread, flSpread ), 8192, BULLET_PLAYER_9MM, 0 );

	if (!m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);

	m_flNextPrimaryAttack = m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + flCycleTime;

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 5, 10 );
}


void CWeaponEinarTaurus::Reload( void )
{
	int iResult;
	float flRand = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 5, 10 );

	iResult = DefaultReload( TAURUS_MAX_CLIP, TAURUS_RELOAD2, 2.0 );

	if (iResult)
	{
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + flRand;
	}
}



void CWeaponEinarTaurus::WeaponIdle( void )
{
	ResetEmptySound( );

	m_pPlayer->GetAutoaimVector( 0.001 );

	if ( m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
		return;

	// only idle if the slid isn't back
	if (m_iClip != 0)
	{
		int iAnim;
		float flRand = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 0.0, 1.0 );

		if (flRand <= 0.3 + 0 * 0.75)
		{
			iAnim = TAURUS_IDLE1;
		}
		else if (flRand <= 0.6 + 0 * 0.875)
		{
			iAnim = TAURUS_IDLE2;
		}
		else
		{
			iAnim = TAURUS_IDLE3;
		}
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 15;
		SendWeaponAnim( iAnim, 1 );
	}
}


class CAmmoEinarTaurus : public CBasePlayerAmmo	// name ours
{
	void Spawn( void )
	{ 
		Precache( );
		SET_MODEL(ENT(pev), "models/w_taurusclip.mdl");
		CBasePlayerAmmo::Spawn( );
	}
	void Precache( void )
	{
		PRECACHE_MODEL ("models/w_taurusclip.mdl");
		PRECACHE_SOUND("items/9mmclip1.wav");
	}
	BOOL AddAmmo( CBaseEntity *pOther ) 
	{ 
		if (pOther->GiveAmmo( AMMO_TAURUSCLIP_GIVE, "taurus", TAURUS_MAX_CARRY ) != -1)
		{
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM);
			return TRUE;
		}
		return FALSE;
	}
};
LINK_ENTITY_TO_CLASS( ammo_th_taurus, CAmmoEinarTaurus );


//=========================================================
// They Hunger: the sniper rifle (weapon_th_sniper, an HK G36),
// made from the crossbow: secondary attack zooms.
//=========================================================
enum sniper_e {
	SNIPER_IDLE = 0,
	SNIPER_RELOAD,
	SNIPER_DRAW,
	SNIPER_SHOOT1
};

LINK_ENTITY_TO_CLASS( weapon_th_sniper, CWeaponEinarSniper );

void CWeaponEinarSniper::Spawn( )
{
	Precache( );
	m_iId = WEAPON_TH_SNIPER;
	SET_MODEL(ENT(pev), "models/w_hkg36.mdl");

	m_iDefaultAmmo = SNIPER_DEFAULT_GIVE;

	FallInit();// get ready to fall down.
}

void CWeaponEinarSniper::Precache( void )
{
	PRECACHE_MODEL("models/w_hkg36.mdl");
	PRECACHE_SOUND("weapons/sniper.wav");
	PRECACHE_MODEL("models/v_hkg36.mdl");
	PRECACHE_MODEL("models/p_hkg36.mdl");

	PRECACHE_SOUND ("weapons/ap9_clipout.wav");
	PRECACHE_SOUND ("weapons/ap9_clipin.wav");
	PRECACHE_SOUND ("weapons/ap9_bolt.wav");
	PRECACHE_SOUND ("items/cliprelease1.wav");

	m_usFireSniper = PRECACHE_EVENT( 1, "events/sniper.sc" );
}


int CWeaponEinarSniper::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "bullets";
	p->iMaxAmmo1 = SNIPER_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = SNIPER_MAX_CLIP;
	p->iSlot = 2;
	p->iPosition = 3;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_TH_SNIPER;
	p->iWeight = SNIPER_WEIGHT;
	return 1;
}

int CWeaponEinarSniper::AddToPlayer( CBasePlayer *pPlayer )
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

BOOL CWeaponEinarSniper::Deploy( )
{
	return DefaultDeploy( "models/v_hkg36.mdl", "models/p_hkg36.mdl", SNIPER_DRAW, "bow" );
}

void CWeaponEinarSniper::Holster( int skiplocal /* = 0 */ )
{
	m_fInReload = FALSE;// cancel any reload in progress.

	if ( m_fInZoom )
	{
		SecondaryAttack( );
	}

	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;
	m_pPlayer->pev->viewmodel = 0;
	m_pPlayer->pev->weaponmodel = 0;
}

void CWeaponEinarSniper::PrimaryAttack( void )
{
	BOOL fMultiplayer = TH_IsMultiplayer();

	if ( m_pPlayer->pev->waterlevel != 3 && m_iClip > 0 )
	{
		m_iClip--;

		m_pPlayer->pev->effects = (int)(m_pPlayer->pev->effects) | EF_MUZZLEFLASH;

		int flags;

#if defined( CLIENT_WEAPONS )
		flags = FEV_NOTHOST;
#else
		flags = 0;
#endif

		// bparam1: zoomed
		PLAYBACK_EVENT_FULL( flags, m_pPlayer->edict(), m_usFireSniper, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, 0.0, 0.0, 0, 0, m_fInZoom ? 1 : 0, 0 );

		// player "shoot" animation
		m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

		m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;
		m_pPlayer->m_iWeaponFlash = NORMAL_GUN_FLASH;

		Vector vecSrc	 = m_pPlayer->GetGunPosition( );
		Vector vecAiming = gpGlobals->v_forward;

		// multiplayer gives the damage itself: FireBullets then does plain
		// DMG_BULLET without the 357's texture sound and decal
		if ( m_fInZoom )
		{
			if ( fMultiplayer )
				m_pPlayer->FireBullets( 1, vecSrc, vecAiming, Vector( 0.002, 0.002, 0.002 ), 8192, BULLET_PLAYER_357, 0, 80 );
			else
				m_pPlayer->FireBullets( 1, vecSrc, vecAiming, Vector( 0.004, 0.004, 0.004 ), 8192, BULLET_PLAYER_357, 0 );
		}
		else
		{
			if ( fMultiplayer )
				m_pPlayer->FireBullets( 1, vecSrc, vecAiming, VECTOR_CONE_9DEGREES, 8192, BULLET_PLAYER_357, 0, 40 );
			else
				m_pPlayer->FireBullets( 1, vecSrc, vecAiming, VECTOR_CONE_10DEGREES, 8192, BULLET_PLAYER_357, 0 );
		}

		if (!m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
			// HEV suit - indicate out of ammo condition
			m_pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);

		if ( m_fInZoom )
		{
			m_flTimeWeaponIdle = m_flNextPrimaryAttack = m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.75;
		}
		else
		{
			m_flTimeWeaponIdle = m_flNextPrimaryAttack = m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.11;

			m_pPlayer->pev->punchangle.x -= RANDOM_FLOAT( 0, 4 ) + 4;
			m_pPlayer->pev->punchangle.y -= 1 - RANDOM_FLOAT( 0, 2 );
		}
	}
	else
	{
		if ( m_fFireOnEmpty )
		{
			PlayEmptySound( );
			m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.25;
		}

		if ( m_flTimeWeaponIdle < UTIL_WeaponTimeBase() )
		{
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 15, 20 );
		}
	}
}


void CWeaponEinarSniper::SecondaryAttack()
{
	if ( m_fInZoom )
	{
		m_pPlayer->m_iFOV = 0; // 0 means reset to default fov
		m_fInZoom = 0;
		TH_ScreenFadeIn( m_pPlayer, Vector( 0, 255, 0 ), 0, 0, 70 );
	}
	else
	{
		m_pPlayer->m_iFOV = 20;
		m_fInZoom = 1;
		TH_ScreenFadeOut( m_pPlayer, Vector( 0, 255, 0 ), 0, 0, 70 );
	}
	
	pev->nextthink = UTIL_WeaponTimeBase() + 0.11;
	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.75;
}


void CWeaponEinarSniper::Reload( void )
{
	if ( m_fInZoom )
	{
		SecondaryAttack();
	}

	DefaultReload( SNIPER_MAX_CLIP, SNIPER_RELOAD, 3.3 );
}


void CWeaponEinarSniper::WeaponIdle( void )
{
	ResetEmptySound( );

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 20;
}


// ammo_th_sniper and ammo_einar1
class CAmmoEinarSniper : public CBasePlayerAmmo	// name ours
{
	void Spawn( void )
	{ 
		pev->classname = MAKE_STRING("ammo_th_sniper");
		Precache( );
		SET_MODEL(ENT(pev), "models/w_antidote.mdl");
		CBasePlayerAmmo::Spawn( );
	}
	void Precache( void )
	{
		PRECACHE_MODEL ("models/w_antidote.mdl");
		PRECACHE_SOUND("items/9mmclip1.wav");
	}
	BOOL AddAmmo( CBaseEntity *pOther ) 
	{ 
		if (pOther->GiveAmmo( AMMO_SNIPERCLIP_GIVE, "bullets", SNIPER_MAX_CARRY ) != -1)
		{
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM);
			return TRUE;
		}
		return FALSE;
	}
};
LINK_ENTITY_TO_CLASS( ammo_th_sniper, CAmmoEinarSniper );
LINK_ENTITY_TO_CLASS( ammo_einar1, CAmmoEinarSniper );


//=========================================================
// They Hunger: weapon_einar1, the first sniper rifle (Team
// Fortress Classic's model). The same weapon as weapon_th_sniper
// with its own models, animations and a reload in three steps.
//=========================================================
enum tfcsniper_e {
	TFCSNIPER_IDLE = 0,
	TFCSNIPER_AIM,
	TFCSNIPER_FIRE,
	TFCSNIPER_DRAW,
	TFCSNIPER_HOLSTER,
	TFCSNIPER_AUTOIDLE,
	TFCSNIPER_AUTOFIRE,
	TFCSNIPER_AUTODRAW,
	TFCSNIPER_AUTOHOLSTER
};

LINK_ENTITY_TO_CLASS( weapon_einar1, CWeaponEinarTFCSniper );

void CWeaponEinarTFCSniper::Spawn( )
{
	Precache( );
	m_iId = WEAPON_EINAR1;
	SET_MODEL(ENT(pev), "models/w_isotopebox.mdl");

	m_iDefaultAmmo = SNIPER_DEFAULT_GIVE;

	FallInit();// get ready to fall down.
}

void CWeaponEinarTFCSniper::Precache( void )
{
	PRECACHE_MODEL("models/w_isotopebox.mdl");
	PRECACHE_SOUND("weapons/sniper.wav");
	PRECACHE_MODEL("models/v_tfc_sniper.mdl");
	PRECACHE_MODEL("models/p_sniper.mdl");

	PRECACHE_SOUND("weapons/reload3.wav");

	m_usFireSniper = PRECACHE_EVENT( 1, "events/sniper2.sc" );
}


int CWeaponEinarTFCSniper::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "bullets";
	p->iMaxAmmo1 = SNIPER_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = SNIPER_MAX_CLIP;
	p->iSlot = 2;
	p->iPosition = 4;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_EINAR1;
	p->iWeight = SNIPER_WEIGHT;
	return 1;
}

int CWeaponEinarTFCSniper::AddToPlayer( CBasePlayer *pPlayer )
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

BOOL CWeaponEinarTFCSniper::Deploy( )
{
	return DefaultDeploy( "models/v_tfc_sniper.mdl", "models/p_sniper.mdl", TFCSNIPER_AUTODRAW, "bow" );
}

void CWeaponEinarTFCSniper::Holster( int skiplocal /* = 0 */ )
{
	m_fInReload = FALSE;// cancel any reload in progress.

	if ( m_fInZoom )
	{
		SecondaryAttack( );
	}

	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;
	SendWeaponAnim( TFCSNIPER_AUTOHOLSTER );
}

void CWeaponEinarTFCSniper::PrimaryAttack( void )
{
	BOOL fMultiplayer = TH_IsMultiplayer();

	if ( m_pPlayer->pev->waterlevel != 3 && m_iClip > 0 )
	{
		m_iClip--;

		m_pPlayer->pev->effects = (int)(m_pPlayer->pev->effects) | EF_MUZZLEFLASH;

		int flags;

#if defined( CLIENT_WEAPONS )
		flags = FEV_NOTHOST;
#else
		flags = 0;
#endif

		// bparam1: zoomed
		PLAYBACK_EVENT_FULL( flags, m_pPlayer->edict(), m_usFireSniper, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, 0.0, 0.0, 0, 0, m_fInZoom ? 1 : 0, 0 );

		// player "shoot" animation
		m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

		m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;
		m_pPlayer->m_iWeaponFlash = NORMAL_GUN_FLASH;

		Vector vecSrc	 = m_pPlayer->GetGunPosition( );
		Vector vecAiming = gpGlobals->v_forward;

		if ( m_fInZoom )
		{
			if ( fMultiplayer )
				m_pPlayer->FireBullets( 1, vecSrc, vecAiming, Vector( 0.002, 0.002, 0.002 ), 8192, BULLET_PLAYER_357, 0, 80 );
			else
				m_pPlayer->FireBullets( 1, vecSrc, vecAiming, Vector( 0.004, 0.004, 0.004 ), 8192, BULLET_PLAYER_357, 0 );
		}
		else
		{
			if ( fMultiplayer )
				m_pPlayer->FireBullets( 1, vecSrc, vecAiming, VECTOR_CONE_9DEGREES, 8192, BULLET_PLAYER_357, 0, 40 );
			else
				m_pPlayer->FireBullets( 1, vecSrc, vecAiming, VECTOR_CONE_10DEGREES, 8192, BULLET_PLAYER_357, 0 );
		}

		if (!m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
			// HEV suit - indicate out of ammo condition
			m_pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);

		if ( m_fInZoom )
		{
			m_flTimeWeaponIdle = m_flNextPrimaryAttack = m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.75;
		}
		else
		{
			m_flTimeWeaponIdle = m_flNextPrimaryAttack = m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.11;

			m_pPlayer->pev->punchangle.x -= RANDOM_FLOAT( 0, 4 ) + 4;
			m_pPlayer->pev->punchangle.y -= 1 - RANDOM_FLOAT( 0, 2 );
		}
	}
	else
	{
		if ( m_fFireOnEmpty )
		{
			PlayEmptySound( );
			m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.25;
		}

		if ( m_flTimeWeaponIdle < UTIL_WeaponTimeBase() )
		{
			SendWeaponAnim( TFCSNIPER_AUTOIDLE );
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 15, 20 );
		}
	}
}


void CWeaponEinarTFCSniper::SecondaryAttack()
{
	if ( m_fInZoom )
	{
		m_pPlayer->m_iFOV = 0; // 0 means reset to default fov
		m_fInZoom = 0;
		TH_ScreenFadeIn( m_pPlayer, Vector( 0, 255, 0 ), 0, 0, 70 );
	}
	else
	{
		m_pPlayer->m_iFOV = 20;
		m_fInZoom = 1;
		TH_ScreenFadeOut( m_pPlayer, Vector( 0, 255, 0 ), 0, 0, 70 );
	}
	
	pev->nextthink = UTIL_WeaponTimeBase() + 0.11;
	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.75;
}


// The reload in three steps: the rifle goes down (DefaultReload, 0.5 s),
// then WeaponIdle, which CBasePlayerWeapon::ItemPostFrame calls for
// m_fInReload > 1, plays the sound (m_fInReload 2, 1 s) and draws the rifle
// (m_fInReload 3), and with m_fInReload 1 ItemPostFrame fills the clip.
void CWeaponEinarTFCSniper::Reload( void )
{
	if ( m_fInZoom )
	{
		SecondaryAttack();
	}

	if ( DefaultReload( SNIPER_MAX_CLIP, TFCSNIPER_AUTOHOLSTER, 0.5 ) )
	{
		m_fInReload = 2;
		m_flNextPrimaryAttack = m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 2;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 5, 10 );
	}
}


void CWeaponEinarTFCSniper::WeaponIdle( void )
{
	ResetEmptySound( );

	if ( m_pPlayer->m_flNextAttack <= UTIL_WeaponTimeBase() )
	{
		if ( m_fInReload == 3 )
		{
			SendWeaponAnim( TFCSNIPER_AUTODRAW );
			m_fInReload = 1;
		}
		if ( m_fInReload == 2 )
		{
			EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/reload3.wav", RANDOM_FLOAT(0.95, 1.0), ATTN_NORM, 0, 93 + RANDOM_LONG(0,0xF));
			m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 1;
			m_fInReload = 3;
		}
	}

	if ( m_flTimeWeaponIdle < UTIL_WeaponTimeBase() )
	{
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 15, 20 );
		SendWeaponAnim( TFCSNIPER_AUTOIDLE, 1 );
	}
}
















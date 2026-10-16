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
// Wanted: the Winchester rifle (weapon_winchester, ammo_winchesterclip),
// made from the original SDK's shotgun: one rifle bullet a shot, a
// break-open reload a round at a time. The file name is ours; the class
// name is retail's (its save table).
//

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"
#include "gamerules.h"


// sequences of models/v_winchester.mdl
enum winchester_e {
	WINCHESTER_IDLE = 0,
	WINCHESTER_FIRE,
	WINCHESTER_RELOAD,
	WINCHESTER_CLOSE_BREAK,
	WINCHESTER_BREAK,
	WINCHESTER_DRAW,
	WINCHESTER_REHOLSTER,
};

class CWinchester : public CBasePlayerWeapon
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
	BOOL Deploy( );
	void Reload( void );
	void WeaponIdle( void );
	int m_fInReload;
	float m_flNextReload;
	int m_iShell;
	float m_flPumpTime;
};
LINK_ENTITY_TO_CLASS( weapon_winchester, CWinchester );


TYPEDESCRIPTION	CWinchester::m_SaveData[] =
{
	DEFINE_FIELD( CWinchester, m_flNextReload, FIELD_TIME ),
	DEFINE_FIELD( CWinchester, m_fInReload, FIELD_INTEGER ),
	DEFINE_FIELD( CWinchester, m_flNextReload, FIELD_TIME ),
	// DEFINE_FIELD( CWinchester, m_iShell, FIELD_INTEGER ),
	DEFINE_FIELD( CWinchester, m_flPumpTime, FIELD_TIME ),
};
IMPLEMENT_SAVERESTORE( CWinchester, CBasePlayerWeapon );



void CWinchester::Spawn( )
{
	Precache( );
	m_iId = WEAPON_WINCHESTER;
	SET_MODEL(ENT(pev), "models/w_winchester.mdl");

	m_iDefaultAmmo = 11;

	FallInit();// get ready to fall
}


void CWinchester::Precache( void )
{
	PRECACHE_MODEL("models/v_winchester.mdl");
	PRECACHE_MODEL("models/w_winchester.mdl");
	PRECACHE_MODEL("models/p_winchester.mdl");

	m_iShell = PRECACHE_MODEL ("models/winchestershell.mdl");

	PRECACHE_MODEL("models/w_winchesterclip.mdl");

	PRECACHE_SOUND ("weapons/winchester_break.wav");
	PRECACHE_SOUND ("weapons/winchester_closebreak.wav");
	PRECACHE_SOUND ("weapons/winchester_reload1.wav");
	PRECACHE_SOUND ("weapons/winchester_reload2.wav");
	PRECACHE_SOUND ("weapons/winchester_fire1.wav");
	PRECACHE_SOUND ("weapons/winchester_fire2.wav");
	PRECACHE_SOUND ("weapons/winchester_fire3.wav");
	PRECACHE_SOUND ("weapons/winchester_dryfire.wav");
}


int CWinchester::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "winchester";
	p->iMaxAmmo1 = 70;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = 11;
	p->iSlot = 2;
	p->iPosition = 0;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_WINCHESTER;
	p->iWeight = 20;

	return 1;
}

int CWinchester::AddToPlayer( CBasePlayer *pPlayer )
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


BOOL CWinchester::Deploy( )
{
	return DefaultDeploy( "models/v_winchester.mdl", "models/p_winchester.mdl", WINCHESTER_DRAW, "winchester" );
}


void CWinchester::PrimaryAttack()
{
	// don't fire underwater
	if (m_pPlayer->pev->waterlevel == 3)
	{
		PlayEmptySound( );
		EMIT_SOUND( ENT(pev), CHAN_ITEM, "weapons/winchester_dryfire.wav", 1, ATTN_NORM );
		m_flNextPrimaryAttack = gpGlobals->time + 1.1;
		return;
	}

	if (m_iClip <= 0)
	{
		Reload( );
		if (m_iClip == 0)
			PlayEmptySound( );
		EMIT_SOUND( ENT(pev), CHAN_ITEM, "weapons/winchester_dryfire.wav", 1, ATTN_NORM );
		m_flNextPrimaryAttack = gpGlobals->time + 1.1;
		return;
	}

	m_pPlayer->m_iWeaponVolume = LOUD_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = NORMAL_GUN_FLASH;

	m_iClip--;
	m_pPlayer->pev->effects = (int)(m_pPlayer->pev->effects) | EF_MUZZLEFLASH;

	SendWeaponAnim( WINCHESTER_FIRE );

	// player "shoot" animation
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

	UTIL_MakeVectors( m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle );

	Vector	vecShellVelocity = m_pPlayer->pev->velocity
							 + gpGlobals->v_right * RANDOM_FLOAT(50,70)
							 + gpGlobals->v_up * RANDOM_FLOAT(100,150)
							 + gpGlobals->v_forward * 25;

	EjectBrass ( pev->origin + m_pPlayer->pev->view_ofs + gpGlobals->v_up * -5 + gpGlobals->v_forward * 7 + gpGlobals->v_right * 6 , vecShellVelocity, pev->angles.y, m_iShell, TE_BOUNCE_SHELL);

	switch ( RANDOM_LONG( 0, 2 ) )
	{
	case 0:
		EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/winchester_fire1.wav", 1, ATTN_NORM, 0, 94 + RANDOM_LONG(0,0xf));
		break;
	case 1:
		EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/winchester_fire2.wav", 1, ATTN_NORM, 0, 94 + RANDOM_LONG(0,0xf));
		break;
	case 2:
		EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/winchester_fire3.wav", 1, ATTN_NORM, 0, 94 + RANDOM_LONG(0,0xf));
		break;
	}

	Vector vecSrc	 = m_pPlayer->GetGunPosition( );
	Vector vecAiming = m_pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );

	if ( g_pGameRules->IsDeathmatch() )
	{
		m_pPlayer->FireBullets( 1, vecSrc, vecAiming, VECTOR_CONE_6DEGREES, 8192, BULLET_PLAYER_WINCHESTER, 2 );
	}
	else
	{
		m_pPlayer->FireBullets( 1, vecSrc, vecAiming, VECTOR_CONE_3DEGREES, 8192, BULLET_PLAYER_WINCHESTER, 2 );
	}

	if (!m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);

	if (m_iClip != 0)
		m_flPumpTime = gpGlobals->time + 0.5;

	m_flNextPrimaryAttack = gpGlobals->time + 1.1;
	if (m_iClip != 0)
		m_flTimeWeaponIdle = gpGlobals->time + 5.0;
	else
		m_flTimeWeaponIdle = 0.75;
	m_fInReload = 0;

	m_pPlayer->pev->punchangle.x -= 5;
}


void CWinchester::Reload( void )
{
	if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 || m_iClip == 11)
		return;

	if (m_flNextReload > gpGlobals->time)
		return;

	// don't reload until recoil is done
	if (m_flNextPrimaryAttack > gpGlobals->time)
		return;

	// check to see if we're ready to reload
	if (m_fInReload == 0)
	{
		SendWeaponAnim( WINCHESTER_BREAK );
		EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/winchester_break.wav", 1, ATTN_NORM, 0, 95 + RANDOM_LONG(0,0x1f));
		m_fInReload = 1;
		m_pPlayer->m_flNextAttack = gpGlobals->time + 0.75;
		m_flTimeWeaponIdle = gpGlobals->time + 0.75;
		m_flNextPrimaryAttack = gpGlobals->time + 1.1;
		return;
	}
	else if (m_fInReload == 1)
	{
		if (m_flTimeWeaponIdle > gpGlobals->time)
			return;
		// was waiting for gun to move to side
		m_fInReload = 2;

		if (RANDOM_LONG(0,1))
			EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/winchester_reload1.wav", 1, ATTN_NORM, 0, 85 + RANDOM_LONG(0,0x1f));
		else
			EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/winchester_reload2.wav", 1, ATTN_NORM, 0, 85 + RANDOM_LONG(0,0x1f));

		SendWeaponAnim( WINCHESTER_RELOAD );

		m_flNextReload = gpGlobals->time + 0.5;
		m_flTimeWeaponIdle = gpGlobals->time + 0.5;
	}
	else
	{
		// Add them to the clip
		m_iClip += 1;
		m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] -= 1;
		m_fInReload = 1;
	}
}


void CWinchester::WeaponIdle( void )
{
	m_pPlayer->m_iFOV = 0;

	ResetEmptySound( );

	m_pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );

	if (m_flPumpTime && m_flPumpTime < gpGlobals->time)
	{
		// no pumping sound
		m_flPumpTime = 0;
	}

	if (m_flTimeWeaponIdle < gpGlobals->time)
	{
		if (m_iClip == 0 && m_fInReload == 0 && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType])
		{
			Reload( );
		}
		else if (m_fInReload != 0)
		{
			if (m_iClip != 11 && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType])
			{
				Reload( );
			}
			else
			{
				// reload debounce has timed out: close the breech
				SendWeaponAnim( WINCHESTER_CLOSE_BREAK );

				EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/winchester_closebreak.wav", 1, ATTN_NORM, 0, 95 + RANDOM_LONG(0,0x1f));
				m_fInReload = 0;
				m_flTimeWeaponIdle = gpGlobals->time + 1.5;
			}
		}
		else
		{
			SendWeaponAnim( WINCHESTER_IDLE );
			m_flTimeWeaponIdle = gpGlobals->time + 2.111;
		}
	}
}



class CWinchesterAmmo : public CBasePlayerAmmo
{
	void Spawn( void )
	{
		Precache( );
		SET_MODEL(ENT(pev), "models/w_winchesterclip.mdl");
		CBasePlayerAmmo::Spawn( );
	}
	void Precache( void )
	{
		PRECACHE_MODEL ("models/w_winchesterclip.mdl");
	}
	BOOL AddAmmo( CBaseEntity *pOther )
	{
		int bResult = (pOther->GiveAmmo( 11, "winchester", 70 ) != -1);
		if (bResult)
		{
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM);
		}
		return bResult;
	}
};
LINK_ENTITY_TO_CLASS( ammo_winchesterclip, CWinchesterAmmo );

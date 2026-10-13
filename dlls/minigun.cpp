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
#include "soundent.h"
#include "gamerules.h"
#include "minigun.h"

extern int gmsgWeapPickup;
extern int gmsgSliderX;
extern int gmsgTempX;

enum minigun_e
{
	MINIGUN_IDLE = 0,
	MINIGUN_IDLEINSPECT,
	MINIGUN_FIRENORMAL,
	MINIGUN_SPINUP,
	MINIGUN_FIRELOOP,
	MINIGUN_SPINDOWN,
	MINIGUN_ARMING,
	MINIGUN_IDLELOOP,
	MINIGUN_HOLSTER,
};

// m_iSpinState
#define MINIGUN_SPIN_OFF		0
#define MINIGUN_SPIN_ON			1
#define MINIGUN_SPIN_UP			2
#define MINIGUN_SPIN_DOWN		3

LINK_ENTITY_TO_CLASS( weapon_mechagun, CMinigun );
LINK_ENTITY_TO_CLASS( weapon_minigun, CMinigun );

#ifndef CLIENT_DLL
TYPEDESCRIPTION	CMinigun::m_SaveData[] =
{
	DEFINE_FIELD( CMinigun, m_iTemp, FIELD_INTEGER ),
	DEFINE_FIELD( CMinigun, m_iSpinState, FIELD_INTEGER ),
	DEFINE_FIELD( CMinigun, m_iOverheating, FIELD_INTEGER ),
	DEFINE_FIELD( CMinigun, m_flTimeStartSpinning, FIELD_TIME ),
	DEFINE_FIELD( CMinigun, m_flTimeStartOverheating, FIELD_TIME ),
	DEFINE_FIELD( CMinigun, m_flNextTempChangeTime, FIELD_TIME ),
	DEFINE_FIELD( CMinigun, m_flNextSpinAnimation, FIELD_TIME ),
	DEFINE_FIELD( CMinigun, m_flNextIdleAnimTime, FIELD_TIME ),
	DEFINE_FIELD( CMinigun, m_flNextTempDrop, FIELD_TIME ),
	DEFINE_FIELD( CMinigun, m_flNextFireAnimTime, FIELD_TIME ),
	DEFINE_FIELD( CMinigun, m_iSlidePos, FIELD_INTEGER ),
	DEFINE_FIELD( CMinigun, m_bWaterCooled, FIELD_BOOLEAN ),
	DEFINE_FIELD( CMinigun, m_flTimeDrawBodyChange, FIELD_TIME ),
	DEFINE_FIELD( CMinigun, m_iGlowshell, FIELD_INTEGER ),
	DEFINE_FIELD( CMinigun, m_flNextConfigTime, FIELD_TIME ),
	DEFINE_FIELD( CMinigun, m_flNextSpinAnimation, FIELD_TIME ),	// sic, twice
};
IMPLEMENT_SAVERESTORE( CMinigun, CBasePlayerWeapon );
#endif


void CMinigun::Spawn( )
{
	pev->classname = MAKE_STRING("weapon_minigun"); // hack to allow for old names
	Precache( );
	SET_MODEL(ENT(pev), "models/w_mechagun.mdl");
	m_iId = WEAPON_MINIGUN;
	m_iClip = -1;
	m_iDefaultAmmo = 30;

	m_iTemp = 0;
	m_flNextTempDrop = gpGlobals->time;
	m_iSlidePos = 1;
	m_iGlowshell = 0;
	m_flNextConfigTime = 0;
	m_flNextSpinAnimation = 0;

	if ( g_pGameRules && g_pGameRules->IsMultiplayer() )
	{
		// water cooled in multiplayer
		pev->body = 1;
		m_bWaterCooled = true;
	}

	FallInit();// get ready to fall down.
}


void CMinigun::Precache( void )
{
	PRECACHE_MODEL("models/v_mechagun.mdl");
	PRECACHE_MODEL("models/w_mechagun.mdl");
	PRECACHE_SOUND("items/9mmclip1.wav");
	PRECACHE_SOUND("weapons/electro4.wav");

	m_iMuzzleSprite = PRECACHE_MODEL("sprites/muzlblue.spr");
	m_iShell = PRECACHE_MODEL ("models/shell.mdl");// brass shell

	PRECACHE_SOUND("weapons/hks1.wav");
	PRECACHE_SOUND("weapons/hks2.wav");
	PRECACHE_SOUND("weapons/hks3.wav");
	PRECACHE_SOUND("weapons/MechaSpinUp.wav");
	PRECACHE_SOUND("weapons/MechaSpinDown.wav");

	PRECACHE_MODEL("models/p_9mmAR.mdl");

	m_usMinigun = PRECACHE_EVENT( 1, "events/gw_minigun.sc" );
	m_usMinigunHeat = PRECACHE_EVENT( 1, "events/gw_minigunHeat.sc" );
}

#ifndef CLIENT_DLL
void CMinigun::UpdateOwner( void )
{
	MESSAGE_BEGIN( MSG_ONE, gmsgSliderX, NULL, m_pPlayer->pev );
		WRITE_BYTE( WEAPON_CUST_SPIN );
		WRITE_BYTE( m_iSlidePos );
	MESSAGE_END();

	MESSAGE_BEGIN( MSG_ONE, gmsgSliderX, NULL, m_pPlayer->pev );
		WRITE_BYTE( WEAPON_CUST_COOLERS );
		WRITE_BYTE( m_iSlidePos );
	MESSAGE_END();
}
#endif

int CMinigun::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "9mm";
	p->iMaxAmmo1 = 200;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = 3;
	p->iPosition = 0;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_MINIGUN;
	p->iWeight = -10;

	return 1;
}

int CMinigun::AddToPlayer( CBasePlayer *pPlayer )
{
	if ( CBasePlayerWeapon::AddToPlayer( pPlayer ) )
	{
		m_iTemp = 40;

		MESSAGE_BEGIN( MSG_ONE, gmsgWeapPickup, NULL, pPlayer->pev );
			WRITE_BYTE( m_iId );
		MESSAGE_END();

		pPlayer->GiveNamedItem( "cust_1MinigunSpin" );
		return TRUE;
	}
	return FALSE;
}

BOOL CMinigun::Deploy( )
{
	m_flNextIdleAnimTime = gpGlobals->time + 1.5;
	return DefaultDeploy( "models/v_mechagun.mdl", "models/p_9mmAR.mdl", MINIGUN_ARMING, "mp5" );
}


void CMinigun::PrimaryAttack()
{
	if ( gpGlobals->time - m_flTimeStartSpinning < 2.0 )
	{
		// still spinning up
		m_flNextPrimaryAttack = gpGlobals->time + 0.05;
		return;
	}

	Vector vecSpread;
	int iShotType;
	int iHeat;
	float flDelay;

	if ( m_iSpinState == MINIGUN_SPIN_ON )
	{
		vecSpread = VECTOR_CONE_10DEGREES;
		iShotType = 1;
		if ( g_pGameRules->IsMultiplayer() )
		{
			vecSpread = VECTOR_CONE_6DEGREES;
			iShotType = 4;
		}
		iHeat = 2;
		flDelay = 0.1;
	}
	else
	{
		vecSpread = VECTOR_CONE_3DEGREES;
		iHeat = 1;
		flDelay = 0.2;
		iShotType = 2;
	}

	if ( m_iOverheating == 1 )
	{
		vecSpread = VECTOR_CONE_20DEGREES;
		iHeat = 1;
		flDelay = 0.1;
		iShotType = 3;
	}

	if ( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] == 0 )
	{
		m_flNextPrimaryAttack = gpGlobals->time + 1;
		PlayEmptySound( );
		PLAYBACK_EVENT_FULL( 0, m_pPlayer->edict(), m_usMinigunHeat, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, 0.0, 0.0, 0, 0, 0, 0 );

		if ( m_iOverheating )
			m_iTemp = 83;
		m_iOverheating = 0;
		m_iGlowshell = 0;

		if ( m_iSpinState == MINIGUN_SPIN_ON )
		{
			SendWeaponAnim( MINIGUN_SPINDOWN );
			m_iSpinState = MINIGUN_SPIN_OFF;
			m_flTimeWeaponIdle = gpGlobals->time + 3.5;
		}
		return;
	}

	PLAYBACK_EVENT_FULL( 0, m_pPlayer->edict(), m_usMinigun, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, m_flNextFireAnimTime, gpGlobals->time, iShotType, m_iSpinState, pev->body, 0 );

	CSoundEnt::InsertSound ( bits_SOUND_COMBAT, pev->origin, 1000, 0.1 );

	m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = NORMAL_GUN_FLASH;

	if ( !m_iOverheating && m_flNextTempChangeTime < gpGlobals->time )
	{
		m_iTemp += iHeat;
		if ( !m_bWaterCooled )
			m_flNextTempChangeTime = flDelay + gpGlobals->time;
		else
			m_flNextTempChangeTime = flDelay * 2 + gpGlobals->time;

		if ( m_iTemp >= 75 )
		{
			// glow
			m_iGlowshell = 1;
			PLAYBACK_EVENT_FULL( 0, m_pPlayer->edict(), m_usMinigunHeat, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, 0.0, 0.0, 0, 0, 1, 0 );
		}

		if ( m_iTemp >= 100 )
		{
			// overheated
			m_iTemp = 100;
			m_flTimeStartOverheating = gpGlobals->time;
			m_flTimeWeaponIdle = gpGlobals->time + 0.1;
			m_iOverheating = 1;
			if ( m_iSpinState == MINIGUN_SPIN_ON )
			{
				m_iSpinState = MINIGUN_SPIN_DOWN;
				m_iSlidePos = 1;
			}
			PLAYBACK_EVENT_FULL( 0, m_pPlayer->edict(), m_usMinigunHeat, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, 0.0, 0.0, 0, 0, 1, 0 );
		}
	}

	m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType]--;

	if ( m_iSpinState == MINIGUN_SPIN_ON && m_flNextFireAnimTime < gpGlobals->time )
		m_flNextFireAnimTime = gpGlobals->time + 0.4;

	// player "shoot" animation
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

	UTIL_MakeVectors( m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle );

	Vector vecSrc	 = m_pPlayer->GetGunPosition( );
	Vector vecAiming = m_pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );

	m_pPlayer->FireBullets( 1, vecSrc, vecAiming, vecSpread, 8192, BULLET_PLAYER_MP5, 4 );

	m_flNextPrimaryAttack = flDelay + gpGlobals->time;
}


void CMinigun::Reload( void )
{
}


void CMinigun::InventoryFrame( void )
{
	bool bSendTemp = false;

	if ( this == m_pPlayer->m_pActiveItem )
	{
		if ( m_iTemp != m_iClientTemp )
			bSendTemp = true;
	}

	if ( this != m_pPlayer->m_pActiveItem && m_flNextTempDrop < gpGlobals->time && !m_iOverheating )
	{
		// cools down while carried
		m_iTemp--;
		if ( m_iTemp < 1 )
			m_iTemp = 1;

		if ( m_bWaterCooled )
			m_flNextTempDrop = gpGlobals->time + 0.075;
		else
			m_flNextTempDrop = gpGlobals->time + 0.2;
	}

#ifndef CLIENT_DLL
	if ( bSendTemp )
	{
		m_iClientTemp = m_iTemp;
		MESSAGE_BEGIN( MSG_ONE, gmsgTempX, NULL, m_pPlayer->pev );
			WRITE_BYTE( WEAPON_MINIGUN );
			WRITE_BYTE( m_iTemp );
		MESSAGE_END();
	}
#endif
}


void CMinigun::WeaponIdle( void )
{
	if ( !m_fIdleStarted )
	{
		SendWeaponAnim( MINIGUN_IDLEINSPECT );
		m_fIdleStarted = TRUE;
		m_flNextIdleAnimTime = RANDOM_LONG( 3, 8 ) + gpGlobals->time;
	}

	m_pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );

	if ( !m_iOverheating && m_iTemp < 100 && m_iGlowshell )
	{
		PLAYBACK_EVENT_FULL( FEV_RELIABLE, m_pPlayer->edict(), m_usMinigunHeat, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, 0.0, 0.0, 0, 0, 0, 0 );
		m_iGlowshell = 0;
	}

	ResetEmptySound( );

	if ( m_flTimeDrawBodyChange != 0 && m_flTimeDrawBodyChange < gpGlobals->time )
	{
		m_flTimeDrawBodyChange = 0;
		SendWeaponAnim( MINIGUN_ARMING );
		m_flTimeWeaponIdle = gpGlobals->time + 2.0;
		m_flNextPrimaryAttack = gpGlobals->time + 1.5;
	}

	if ( m_flNextTempDrop < gpGlobals->time && !m_iOverheating )
	{
		m_iTemp--;
		if ( m_iTemp < 1 )
			m_iTemp = 1;

		if ( m_bWaterCooled )
		{
			if ( m_iSpinState == MINIGUN_SPIN_ON )
				m_flNextTempDrop = gpGlobals->time + 0.2;
			else
				m_flNextTempDrop = gpGlobals->time + 0.075;
		}
		else
		{
			if ( m_iSpinState == MINIGUN_SPIN_ON )
				m_flNextTempDrop = gpGlobals->time + 0.85;
			else
				m_flNextTempDrop = gpGlobals->time + 0.2;
		}
	}

	if ( m_iSpinState == MINIGUN_SPIN_ON && m_flNextSpinAnimation <= gpGlobals->time )
	{
		SendWeaponAnim( MINIGUN_IDLELOOP );
		m_flNextSpinAnimation = gpGlobals->time + 0.68;
	}

	if ( m_flTimeWeaponIdle > gpGlobals->time )
		return;

	if ( m_iOverheating )
	{
		if ( gpGlobals->time - m_flTimeStartOverheating >= 5 )
		{
			m_iOverheating = 0;
			m_iTemp = 83;
			PLAYBACK_EVENT_FULL( 0, m_pPlayer->edict(), m_usMinigunHeat, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, 0.0, 0.0, 0, 0, 0, 0 );
			m_iGlowshell = 0;
		}
		else
		{
			// keeps firing until it cools
			PrimaryAttack();
		}
		m_flTimeWeaponIdle = gpGlobals->time + 0.1;
		return;
	}

	if ( m_iSpinState == MINIGUN_SPIN_UP )
	{
		SendWeaponAnim( MINIGUN_SPINUP );
		m_iSpinState = MINIGUN_SPIN_ON;
		m_flTimeWeaponIdle = gpGlobals->time + 2.0;
		m_flTimeStartSpinning = gpGlobals->time;
		m_flNextSpinAnimation = gpGlobals->time + 2.0;
		return;
	}

	if ( m_iSpinState == MINIGUN_SPIN_DOWN )
	{
		SendWeaponAnim( MINIGUN_SPINDOWN );
		m_iSpinState = MINIGUN_SPIN_OFF;
		m_flTimeWeaponIdle = gpGlobals->time + 3.5;
		return;
	}

	if ( m_iSpinState == MINIGUN_SPIN_ON )
	{
		m_flTimeWeaponIdle = gpGlobals->time + 0.4;
		if ( m_flNextSpinCheck < gpGlobals->time )
		{
			m_flNextSpinCheck = gpGlobals->time + 0.37;
			if ( m_iTemp > 100 )
				m_iTemp = 100;
			m_flTimeWeaponIdle = gpGlobals->time;
		}
	}
	else if ( m_flNextIdleAnimTime <= gpGlobals->time )
	{
		int iAnim = !RANDOM_LONG( 0, 5 );
		m_flNextIdleAnimTime = RANDOM_LONG( 3, 8 ) + gpGlobals->time;
		SendWeaponAnim( iAnim );
	}
}


void CMinigun::Customize( char cOption, int iValue )
{
	if ( cOption == '2' )
	{
		// water cooling
		SendWeaponAnim( MINIGUN_HOLSTER );
		if ( pev->body )
		{
			pev->body = 0;
			m_bWaterCooled = false;
		}
		else
		{
			pev->body = 1;
			m_bWaterCooled = true;
		}
		m_flTimeDrawBodyChange = gpGlobals->time + 1.2;
		m_flTimeWeaponIdle = gpGlobals->time + 5.0;
		m_flNextPrimaryAttack = gpGlobals->time + 1.4;
		m_flNextConfigTime = gpGlobals->time + 1.4;
		m_flNextSpinAnimation = gpGlobals->time + 2.7;
		return;
	}

	// spin
	if ( !m_iOverheating && m_flNextConfigTime <= gpGlobals->time )
	{
		if ( m_iSpinState == MINIGUN_SPIN_ON )
		{
			m_flTimeWeaponIdle = gpGlobals->time;
			EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/mechaspindown.wav", 1, ATTN_NORM, 0, 100);
			m_iSpinState = MINIGUN_SPIN_DOWN;
			m_flNextPrimaryAttack = gpGlobals->time + 1.5;
			m_iSlidePos = 1;
		}
		else
		{
			m_flTimeWeaponIdle = gpGlobals->time;
			m_iSpinState = MINIGUN_SPIN_UP;
			EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/mechaspinup.wav", 1, ATTN_NORM, 0, 100);
			m_iSlidePos = 2;
		}
		m_flNextConfigTime = gpGlobals->time + 1.5;
	}
}


void CMinigun::Holster( int skiplocal )
{
	PLAYBACK_EVENT_FULL( 0, m_pPlayer->edict(), m_usMinigunHeat, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, 0.0, 0.0, 0, 0, 0, 0 );
	CBasePlayerWeapon::Holster( skiplocal );
}


//=========================================================
// customization items
//=========================================================
class CCust1MinigunSpin : public CBaseCustomization
{
public:
	void Spawn( void );
	int GetItemInfo(ItemInfo *p);
};
LINK_ENTITY_TO_CLASS( cust_1MinigunSpin, CCust1MinigunSpin );

void CCust1MinigunSpin::Spawn( )
{
	Precache( );
	SET_MODEL(ENT(pev), "models/w_crowbar.mdl");
	m_iClip = -1;

	FallInit();// get ready to fall down.
	m_iId = WEAPON_CUST_SPIN;
}

int CCust1MinigunSpin::GetItemInfo(ItemInfo *p)
{
	p->pszAmmo1 = NULL;
	p->iMaxAmmo1 = -1;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iWeight = CUSTOMIZATION_WEIGHT;
	p->iSlot = 9;
	p->iPosition = 0;
	p->iIsCustomization = 1;

	p->pszName = STRING(pev->classname);
	p->iId = WEAPON_CUST_SPIN;
	p->iPosition = 2;
	p->iCustomSettings = 0;
	p->iCustomDefault = 0;
	p->pszCustomizes = "weapon_minigun";
	return 1;
}


class CCust2MinigunCooled : public CBaseCustomization
{
public:
	void Spawn( void );
	void Precache( void );
	int AddToPlayer( CBasePlayer *pPlayer );
	int GetItemInfo(ItemInfo *p);
};
LINK_ENTITY_TO_CLASS( cust_2MinigunCooled, CCust2MinigunCooled );
LINK_ENTITY_TO_CLASS( customization_coolers, CCust2MinigunCooled );

void CCust2MinigunCooled::Spawn( )
{
	pev->classname = MAKE_STRING("cust_2MinigunCooled"); // hack to allow for old names
	Precache( );
	Precache( );
	SET_MODEL(ENT(pev), "models/w_crowbar.mdl");
	m_iClip = -1;

	FallInit();// get ready to fall down.
	SET_MODEL(ENT(pev), "models/w_coolers.mdl");
	m_iId = WEAPON_CUST_COOLERS;
}

int CCust2MinigunCooled::AddToPlayer( CBasePlayer *pPlayer )
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

void CCust2MinigunCooled::Precache( void )
{
	PRECACHE_MODEL("models/w_coolers.mdl");
	PRECACHE_MODEL("models/w_crowbar.mdl");
}

int CCust2MinigunCooled::GetItemInfo(ItemInfo *p)
{
	p->pszAmmo1 = NULL;
	p->iMaxAmmo1 = -1;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iWeight = CUSTOMIZATION_WEIGHT;
	p->iSlot = 9;
	p->iPosition = 0;
	p->iIsCustomization = 1;

	p->pszName = STRING(pev->classname);
	p->iId = WEAPON_CUST_COOLERS;
	p->iPosition = 7;
	p->iCustomSettings = 0;
	p->iCustomDefault = 0;
	p->pszCustomizes = "weapon_minigun";
	return 1;
}


class CMinigunAmmoClip : public CBasePlayerAmmo
{
	void Spawn( void )
	{
		Precache( );
		SET_MODEL(ENT(pev), "models/mechammo.mdl");
		CBasePlayerAmmo::Spawn( );
	}
	void Precache( void )
	{
		PRECACHE_MODEL ("models/mechammo.mdl");
		PRECACHE_SOUND("items/9mmclip1.wav");
	}
	BOOL AddAmmo( CBaseEntity *pOther )
	{
		int bResult = (pOther->GiveAmmo( 20, "9mm", 200 ) != -1);
		if (bResult)
		{
			if ( PlayPickupSound() )
				EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM);
		}
		return bResult;
	}
};
LINK_ENTITY_TO_CLASS( ammo_minigunClip, CMinigunAmmoClip );
LINK_ENTITY_TO_CLASS( ammo_mechgunClip, CMinigunAmmoClip );

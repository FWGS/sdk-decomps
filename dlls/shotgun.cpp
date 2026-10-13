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
#include "soundent.h"
#include "shotgun.h"

extern int gmsgWeapPickup;
extern int gmsgSliderX;
extern int gEvilImpulse101;

enum shotgun_e {
	SHOTGUN_DRAW = 0,
	SHOTGUN_IDLE,
	SHOTGUN_IDLEINSPECT,
	SHOTGUN_SHOOT,
	SHOTGUN_SHOOT2,
	SHOTGUN_SHOOT3,
	SHOTGUN_SHOOT4,
	SHOTGUN_CUSTOMIZE
};

LINK_ENTITY_TO_CLASS( weapon_shotgun, CShotgun );


#ifndef CLIENT_DLL
TYPEDESCRIPTION	CShotgun::m_SaveData[] = 
{
	DEFINE_FIELD( CShotgun, m_vecSpread, FIELD_VECTOR ),
	DEFINE_FIELD( CShotgun, m_iSlidePos, FIELD_VECTOR ),	// sic
	DEFINE_FIELD( CShotgun, m_iNumShells, FIELD_INTEGER ),
};
IMPLEMENT_SAVERESTORE( CShotgun, CBasePlayerWeapon );
#endif



void CShotgun::Spawn( )
{
	Precache( );
	m_iId = WEAPON_SHOTGUN;
	SET_MODEL(ENT(pev), "models/w_shotgun.mdl");

	m_iDefaultAmmo = gSkillData.shotgunDefaultAmmo;
	m_vecSpread = VECTOR_CONE_10DEGREES;
	m_iSlidePos = 2;
	m_iNumShells = 2;

	FallInit();// get ready to fall
}


void CShotgun::Precache( void )
{
	PRECACHE_MODEL("models/v_shotgun.mdl");
	PRECACHE_MODEL("models/w_shotgun.mdl");
	PRECACHE_MODEL("models/p_shotgun.mdl");

	m_iShell = PRECACHE_MODEL ("models/shotgunshell.mdl");// shotgun shell

	PRECACHE_SOUND("items/9mmclip1.wav");              

	PRECACHE_SOUND ("weapons/sbarrel1.wav");//shotgun
	PRECACHE_SOUND ("weapons/shotgun_cock_heavy.wav");
	PRECACHE_SOUND ("weapons/357_cock1.wav"); // gun empty sound

	m_usShotgun = PRECACHE_EVENT( 1, "events/gw_shotgun.sc" );
}

#ifndef CLIENT_DLL
void CShotgun::UpdateOwner( void )
{
	MESSAGE_BEGIN( MSG_ONE, gmsgSliderX, NULL, m_pPlayer->pev );
		WRITE_BYTE( WEAPON_CUST_SPREAD );
		WRITE_BYTE( m_iSlidePos );
	MESSAGE_END();

	MESSAGE_BEGIN( MSG_ONE, gmsgSliderX, NULL, m_pPlayer->pev );
		WRITE_BYTE( WEAPON_CUST_SHELLS );
		WRITE_BYTE( m_iNumShells );
	MESSAGE_END();
}
#endif

int CShotgun::AddToPlayer( CBasePlayer *pPlayer )
{
	if ( CBasePlayerWeapon::AddToPlayer( pPlayer ) )
	{
		if ( !gEvilImpulse101 )
		{
			MESSAGE_BEGIN( MSG_ONE, gmsgWeapPickup, NULL, pPlayer->pev );
				WRITE_BYTE( m_iId );
			MESSAGE_END();
		}

		pPlayer->GiveNamedItem( "cust_1ShotgunSpread" );
		pPlayer->GiveNamedItem( "cust_2ShotgunShells" );

		MESSAGE_BEGIN( MSG_ONE, gmsgSliderX, NULL, m_pPlayer->pev );
			WRITE_BYTE( WEAPON_CUST_SPREAD );
			WRITE_BYTE( m_iSlidePos );
		MESSAGE_END();

		MESSAGE_BEGIN( MSG_ONE, gmsgSliderX, NULL, m_pPlayer->pev );
			WRITE_BYTE( WEAPON_CUST_SHELLS );
			WRITE_BYTE( m_iNumShells );
		MESSAGE_END();
		return TRUE;
	}
	return FALSE;
}


int CShotgun::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "buckshot";
	p->iMaxAmmo1 = gSkillData.shotgunMaxAmmo;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = 2;
	p->iPosition = 0;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_SHOTGUN;
	p->iWeight = -1;
	p->pszCustomizes = NULL;
	p->iCustomSettings = 0;
	p->iIsCustomization = 0;
	p->iCustomDefault = 0;

	return 1;
}



BOOL CShotgun::Deploy( )
{
	return DefaultDeploy( "models/v_shotgun.mdl", "models/p_shotgun.mdl", SHOTGUN_DRAW, "shotgun" );
}


void CShotgun::Customize( char cOption, int iValue )
{
	if ( cOption == '1' )
	{
		// spread
		switch ( iValue )
		{
		case 1:
			m_vecSpread = VECTOR_CONE_6DEGREES;
			m_iSlidePos = iValue;
			break;
		case 2:
			m_vecSpread = VECTOR_CONE_10DEGREES;
			m_iSlidePos = iValue;
			break;
		case 3:
			m_vecSpread = VECTOR_CONE_15DEGREES;
			m_iSlidePos = iValue;
			break;
		case 4:
			m_vecSpread = VECTOR_CONE_20DEGREES;
			m_iSlidePos = iValue;
			break;
		default:
			m_vecSpread = VECTOR_CONE_3DEGREES;
			m_iSlidePos = iValue;
			break;
		}
	}
	else if ( cOption == '2' )
	{
		// shells per shot
		m_iNumShells = iValue;
		switch ( iValue )
		{
		case 1:
			m_iNumPellets = 4;
			break;
		case 2:
			m_iNumPellets = 8;
			break;
		case 3:
			m_iNumPellets = 11;
			break;
		case 4:
			m_iNumPellets = 13;
			break;
		}
	}

	if ( m_flNextCustomizeAnim < gpGlobals->time )
	{
		SendWeaponAnim( SHOTGUN_CUSTOMIZE );
		m_flNextCustomizeAnim = gpGlobals->time + 5.0;
		m_flTimeWeaponIdle = RANDOM_FLOAT( 1, 10 ) + gpGlobals->time + 5.0;
	}
}


void CShotgun::PrimaryAttack()
{
	float flDelay;

	switch ( m_iSlidePos )
	{
	case 1:
		m_vecSpread = VECTOR_CONE_6DEGREES;
		flDelay = 1.2;
		break;
	case 2:
		m_vecSpread = Vector( 0.10906, 0.10906, 0.10906 );
		flDelay = 0.93;
		break;
	case 3:
		m_vecSpread = VECTOR_CONE_20DEGREES;
		flDelay = 0.77;
		break;
	default:
		m_vecSpread = VECTOR_CONE_20DEGREES;
		flDelay = 0.6;
		break;
	}

	switch ( m_iNumShells )
	{
	case 1:
		m_iNumPellets = 4;
		break;
	case 2:
		m_iNumPellets = 7;
		if ( g_pGameRules->IsMultiplayer() )
			m_iNumPellets = 8;
		break;
	case 3:
		m_iNumPellets = 9;
		if ( g_pGameRules->IsMultiplayer() )
			m_iNumPellets = 12;
		break;
	case 4:
		m_iNumPellets = 11;
		if ( g_pGameRules->IsMultiplayer() )
			m_iNumPellets = 16;
		break;
	}

	int &iAmmo = m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType];
	if ( iAmmo < m_iNumShells )
	{
		if ( iAmmo )
		{
			// fire what is left
			int iShells = iAmmo;
			iAmmo = 0;
			switch ( iShells )
			{
			case 1:
				m_iNumPellets = 4;
				break;
			case 2:
				m_iNumPellets = 7;
				break;
			case 3:
				m_iNumPellets = 9;
				break;
			case 4:
				m_iNumPellets = 11;
				break;
			}
		}
		else
		{
			PlayEmptySound( );
			m_flNextPrimaryAttack = gpGlobals->time + 0.3;
			return;
		}
	}
	else
	{
		iAmmo -= m_iNumShells;
	}

	PLAYBACK_EVENT_FULL( 0, m_pPlayer->edict(), m_usShotgun, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, 0.0, 0.0, m_iSlidePos, m_iNumShells, 0, 0 );

	CSoundEnt::InsertSound ( bits_SOUND_COMBAT, pev->origin, 600, 0.4 );

	m_pPlayer->m_iWeaponVolume = LOUD_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = NORMAL_GUN_FLASH;

	m_pPlayer->pev->effects |= EF_MUZZLEFLASH;

	// player "shoot" animation
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

	UTIL_MakeVectors( m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle );

	Vector vecSrc	 = m_pPlayer->GetGunPosition( );
	Vector vecAiming = m_pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );

	float flDamage = 8;
	if ( g_pGameRules && g_pGameRules->IsMultiplayer() )
		flDamage = 9.8;

	m_pPlayer->FireBullets( m_iNumPellets, vecSrc, vecAiming, m_vecSpread, 2048, BULLET_PLAYER_BUCKSHOT, 0, flDamage );

	// kick back
	if ( m_iNumShells != 4 )
	{
		m_pPlayer->pev->velocity = m_pPlayer->pev->velocity - gpGlobals->v_forward * m_iNumShells * 50;
	}
	else
	{
		float flZ = m_pPlayer->pev->velocity.z;
		m_pPlayer->pev->velocity = m_pPlayer->pev->velocity - gpGlobals->v_forward * 300;
		if ( !g_pGameRules->IsMultiplayer() )
			m_pPlayer->pev->velocity.z = flZ;
	}

	if (m_iClip != 0)
		m_flPumpTime = gpGlobals->time + 0.5;

	m_flNextPrimaryAttack = gpGlobals->time + 0.75;
	if (m_iClip != 0)
		m_flTimeWeaponIdle = gpGlobals->time + 5.0;
	else
		m_flTimeWeaponIdle = 0.75;

	if ( g_pGameRules->IsMultiplayer() )
		flDelay -= 0.1;
	m_pPlayer->m_flNextAttack = gpGlobals->time + flDelay;
}


void CShotgun::WeaponIdle( void )
{
	ResetEmptySound( );

	m_pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );

	if (m_flTimeWeaponIdle < gpGlobals->time)
	{
		int iAnim;
		if ( RANDOM_FLOAT(0, 1) <= 0.5 )
		{
			iAnim = SHOTGUN_IDLE;
			m_flTimeWeaponIdle = RANDOM_LONG(2, 5) + gpGlobals->time;
		}
		else
		{
			iAnim = SHOTGUN_IDLEINSPECT;
			m_flTimeWeaponIdle = RANDOM_LONG(7, 13) + gpGlobals->time;
		}
		SendWeaponAnim( iAnim );
	}
}


//=========================================================
// customization items
//=========================================================
class CCust1ShotgunSpread : public CBaseCustomization
{
public:
	void Spawn( void );
	int GetItemInfo(ItemInfo *p);
};
LINK_ENTITY_TO_CLASS( cust_1ShotgunSpread, CCust1ShotgunSpread );

void CCust1ShotgunSpread::Spawn( )
{
	Precache( );
	SET_MODEL(ENT(pev), "models/w_crowbar.mdl");
	m_iClip = -1;

	FallInit();// get ready to fall down.
	m_iId = WEAPON_CUST_SPREAD;
}

int CCust1ShotgunSpread::GetItemInfo(ItemInfo *p)
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
	p->iId = WEAPON_CUST_SPREAD;
	p->iPosition = 1;
	p->iCustomSettings = 3;
	p->iCustomDefault = 2;
	p->pszCustomizes = "weapon_shotgun";
	return 1;
}


class CCust2ShotgunShells : public CBaseCustomization
{
public:
	void Spawn( void );
	int GetItemInfo(ItemInfo *p);
};
LINK_ENTITY_TO_CLASS( cust_2ShotgunShells, CCust2ShotgunShells );

void CCust2ShotgunShells::Spawn( )
{
	Precache( );
	SET_MODEL(ENT(pev), "models/w_crowbar.mdl");
	m_iClip = -1;

	FallInit();// get ready to fall down.
	m_iId = WEAPON_CUST_SHELLS;
}

int CCust2ShotgunShells::GetItemInfo(ItemInfo *p)
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
	p->iId = WEAPON_CUST_SHELLS;
	p->iPosition = 0;
	p->iCustomSettings = 4;
	p->iCustomDefault = 2;
	p->pszCustomizes = "weapon_shotgun";
	return 1;
}


class CShotgunAmmo : public CBasePlayerAmmo
{
	void Spawn( void )
	{ 
		Precache( );
		SET_MODEL(ENT(pev), "models/shotgunammo.mdl");
		CBasePlayerAmmo::Spawn( );
	}
	void Precache( void )
	{
		PRECACHE_MODEL ("models/shotgunammo.mdl");
		PRECACHE_SOUND("items/9mmclip1.wav");
	}
	BOOL AddAmmo( CBaseEntity *pOther ) 
	{ 
		if (pOther->GiveAmmo( gSkillData.shotgunClipAmmo, "buckshot", gSkillData.shotgunMaxAmmo ) != -1)
		{
			if ( PlayPickupSound() )
				EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM);
			return TRUE;
		}
		return FALSE;
	}
};
LINK_ENTITY_TO_CLASS( ammo_buckshot, CShotgunAmmo );

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
//=========================================================
// POV: weapon_hornetgunsingle, the SDK's hivehand as the
// player's weapon, with a muzzle light and sprite and the
// fidget sounds of the dual gun. File name and the names
// retail doesn't give are ours.
//=========================================================
#if !defined( OEM_BUILD ) && !defined( HLDEMO_BUILD )

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"
#include "hornet.h"
#include "gamerules.h"


extern DLL_GLOBAL BOOL g_fLoading;

enum hgun_e {
	HGUN_IDLE1 = 0,
	HGUN_FIDGETSWAY,
	HGUN_FIDGETSHAKE,
	HGUN_DOWN,
	HGUN_UP,
	HGUN_SHOOT
};

class CSingleHgun : public CBasePlayerWeapon
{
public:
	void Spawn( void );
	void Precache( void );
	int iItemSlot( void ) { return 2; }
	int GetItemInfo(ItemInfo *p);
	int AddToPlayer( CBasePlayer *pPlayer );

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	void PrimaryAttack( void );
	void SecondaryAttack( void );
	BOOL Deploy( void );
	BOOL IsUseable( void );
	void Holster( void );
	void Reload( void );
	void WeaponIdle( void );

	void StartVModelSound( int iSound );
	void EXPORT PlayVModelSound( void );
	void StopVModelSound( void );

	static const char *pIdleSounds[];

	int m_iSoundPlayed;

	float m_flNextAnimTime;

	float m_flRechargeTime;
	
	int m_iFirePhase;// don't save me.

	int m_iMuzzleFlash;
};
LINK_ENTITY_TO_CLASS( weapon_hornetgunsingle, CSingleHgun );

TYPEDESCRIPTION	CSingleHgun::m_SaveData[] = 
{
	DEFINE_FIELD( CSingleHgun, m_iSoundPlayed, FIELD_INTEGER ),
	DEFINE_FIELD( CSingleHgun, m_iMuzzleFlash, FIELD_INTEGER ),
};
IMPLEMENT_SAVERESTORE( CSingleHgun, CBasePlayerWeapon );

const char *CSingleHgun::pIdleSounds[] = 
{
	"horngun/horngun_idle2.wav",
	"horngun/horngun_idle1.wav",
};

BOOL CSingleHgun::IsUseable( void )
{
	return TRUE;
}

void CSingleHgun::Spawn( )
{
	Precache( );
	m_iId = WEAPON_HORNETGUNSINGLE;
	SET_MODEL(ENT(pev), "models/w_hgun.mdl");

	m_iDefaultAmmo = HIVEHAND_DEFAULT_GIVE;
	m_iFirePhase = 0;

	FallInit();// get ready to fall down.
}


void CSingleHgun::Precache( void )
{
	PRECACHE_MODEL("models/v_hgun.mdl");
	PRECACHE_MODEL("models/w_hgun.mdl");
	PRECACHE_MODEL("models/p_hgun.mdl");

	m_iMuzzleFlash = PRECACHE_MODEL("sprites/muz4.spr");

	UTIL_PrecacheOther("hornet");

	PRECACHE_SOUND_ARRAY( pIdleSounds );
}

int CSingleHgun::AddToPlayer( CBasePlayer *pPlayer )
{
	if ( CBasePlayerWeapon::AddToPlayer( pPlayer ) )
	{
		if ( g_pGameRules->IsMultiplayer() )
		{
			// in multiplayer, all hivehands come full. 
			pPlayer->m_rgAmmo[ PrimaryAmmoIndex() ] = HORNET_MAX_CARRY;
		}

		MESSAGE_BEGIN( MSG_ONE, gmsgWeapPickup, NULL, pPlayer->pev );
			WRITE_BYTE( m_iId );
		MESSAGE_END();
		return TRUE;
	}
	return FALSE;
}

int CSingleHgun::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "Hornets";
	p->iMaxAmmo1 = HORNET_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = 1;
	p->iPosition = 0;
	p->iId = m_iId = WEAPON_HORNETGUNSINGLE;
	p->iFlags = ITEM_FLAG_NOAUTOSWITCHEMPTY | ITEM_FLAG_NOAUTORELOAD;
	p->iWeight = HORNETGUN_WEIGHT;

	return 1;
}


BOOL CSingleHgun::Deploy( )
{
	m_pPlayer->m_rgAmmo[ PrimaryAmmoIndex() ] = HORNET_MAX_CARRY;

	return DefaultDeploy( "models/v_hgun.mdl", "models/p_hgun.mdl", HGUN_UP, "hive" );
}

void CSingleHgun::Holster( void )
{
	StopVModelSound( );
	DefaultHolster( HGUN_DOWN, 1.3, 1.3 );

	m_pPlayer->m_rgAmmo[ PrimaryAmmoIndex() ] = HORNET_MAX_CARRY;
}


void CSingleHgun::PrimaryAttack()
{
	if ( m_pPlayer->m_fDidHolster )
	{
		WeaponIdle( );
		return;
	}

	Reload( );

	if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
	{
		return;
	}

	StopVModelSound( );

	Vector vecSrc = m_pPlayer->GetGunPosition( ) + gpGlobals->v_forward * 32 + gpGlobals->v_right * 8 + gpGlobals->v_up * -12;

	MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, vecSrc );
		WRITE_BYTE( TE_DLIGHT );
		WRITE_COORD( vecSrc.x );	// X
		WRITE_COORD( vecSrc.y );	// Y
		WRITE_COORD( vecSrc.z );	// Z
		WRITE_BYTE( RANDOM_LONG( 6, 9 ) );		// radius * 0.1
		WRITE_BYTE( 255 );		// r
		WRITE_BYTE( 140 );		// g
		WRITE_BYTE( 36 );		// b
		WRITE_BYTE( 0 );		// time * 10
		WRITE_BYTE( 0 );		// decay * 0.1
	MESSAGE_END( );

	MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, vecSrc );
		WRITE_BYTE( TE_SPRITE );
		WRITE_COORD( vecSrc.x );	// X
		WRITE_COORD( vecSrc.y );	// Y
		WRITE_COORD( vecSrc.z );	// Z
		WRITE_SHORT( m_iMuzzleFlash );		// model
		WRITE_BYTE( 4 );		// size * 10
		WRITE_BYTE( 128 );		// brightness
	MESSAGE_END( );

	UTIL_MakeVectors( m_pPlayer->pev->v_angle );

	vecSrc = m_pPlayer->GetGunPosition( ) + gpGlobals->v_forward * 16 + gpGlobals->v_right * 8 + gpGlobals->v_up * -12;

	CBaseEntity *pHornet = CBaseEntity::Create( "hornet", vecSrc, m_pPlayer->pev->v_angle, m_pPlayer->edict() );
	pHornet->pev->velocity = gpGlobals->v_forward * 300;
	
	m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType]--;
	m_flRechargeTime = gpGlobals->time + 0.5;

	m_pPlayer->m_iWeaponVolume = QUIET_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = DIM_GUN_FLASH;

	SendWeaponAnim( HGUN_SHOOT );

	// player "shoot" animation
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

	m_flNextPrimaryAttack = m_flNextPrimaryAttack + 0.25;

	if (m_flNextPrimaryAttack < gpGlobals->time)
	{
		m_flNextPrimaryAttack = gpGlobals->time + 0.25;
	}

	m_flTimeWeaponIdle = gpGlobals->time + RANDOM_FLOAT ( 4, 6 );
}



void CSingleHgun::SecondaryAttack( void )
{
	if ( m_pPlayer->m_fDidHolster )
	{
		WeaponIdle( );
		return;
	}

	Reload();

	if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
	{
		return;
	}

	StopVModelSound( );

	CBaseEntity *pHornet;
	Vector vecSrc;

	UTIL_MakeVectors( m_pPlayer->pev->v_angle );

	vecSrc = m_pPlayer->GetGunPosition( ) + gpGlobals->v_forward * 32 + gpGlobals->v_right * 8 + gpGlobals->v_up * -12;

	MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, vecSrc );
		WRITE_BYTE( TE_DLIGHT );
		WRITE_COORD( vecSrc.x );	// X
		WRITE_COORD( vecSrc.y );	// Y
		WRITE_COORD( vecSrc.z );	// Z
		WRITE_BYTE( RANDOM_LONG( 6, 9 ) );		// radius * 0.1
		WRITE_BYTE( 255 );		// r
		WRITE_BYTE( 140 );		// g
		WRITE_BYTE( 36 );		// b
		WRITE_BYTE( 0 );		// time * 10
		WRITE_BYTE( 0 );		// decay * 0.1
	MESSAGE_END( );

	MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, vecSrc );
		WRITE_BYTE( TE_SPRITE );
		WRITE_COORD( vecSrc.x );	// X
		WRITE_COORD( vecSrc.y );	// Y
		WRITE_COORD( vecSrc.z );	// Z
		WRITE_SHORT( m_iMuzzleFlash );		// model
		WRITE_BYTE( 4 );		// size * 10
		WRITE_BYTE( 128 );		// brightness
	MESSAGE_END( );

	vecSrc = m_pPlayer->GetGunPosition( ) + gpGlobals->v_forward * 16 + gpGlobals->v_right * 8 + gpGlobals->v_up * -12;

	m_iFirePhase++;
	switch ( m_iFirePhase )
	{
	case 1:
		vecSrc = vecSrc + gpGlobals->v_up * 8;
		break;
	case 2:
		vecSrc = vecSrc + gpGlobals->v_up * 8;
		vecSrc = vecSrc + gpGlobals->v_right * 8;
		break;
	case 3:
		vecSrc = vecSrc + gpGlobals->v_right * 8;
		break;
	case 4:
		vecSrc = vecSrc + gpGlobals->v_up * -8;
		vecSrc = vecSrc + gpGlobals->v_right * 8;
		break;
	case 5:
		vecSrc = vecSrc + gpGlobals->v_up * -8;
		break;
	case 6:
		vecSrc = vecSrc + gpGlobals->v_up * -8;
		vecSrc = vecSrc + gpGlobals->v_right * -8;
		break;
	case 7:
		vecSrc = vecSrc + gpGlobals->v_right * -8;
		break;
	case 8:
		vecSrc = vecSrc + gpGlobals->v_up * 8;
		vecSrc = vecSrc + gpGlobals->v_right * -8;
		m_iFirePhase = 0;
		break;
	}

	pHornet = CBaseEntity::Create( "hornet", vecSrc, m_pPlayer->pev->v_angle, m_pPlayer->edict() );
	pHornet->pev->velocity = gpGlobals->v_forward * 1200;
	pHornet->pev->angles = UTIL_VecToAngles( pHornet->pev->velocity );

	pHornet->SetThink( CHornet::StartDart );

	m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType]--;
	m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = DIM_GUN_FLASH;

	m_flRechargeTime = gpGlobals->time + 0.5;

	SendWeaponAnim( HGUN_SHOOT );

	// player "shoot" animation
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

	m_flNextPrimaryAttack = m_flNextSecondaryAttack = gpGlobals->time + 0.1;
	m_flTimeWeaponIdle = gpGlobals->time + RANDOM_FLOAT ( 4, 6 );
	m_pPlayer->pev->punchangle.x = RANDOM_FLOAT( 0, 2 );
}


void CSingleHgun::Reload( void )
{
	if ( m_pPlayer->m_fDidHolster )
	{
		WeaponIdle( );
		return;
	}

	while (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] < HORNET_MAX_CARRY && m_flRechargeTime < gpGlobals->time)
	{
		m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType]++;
		m_flRechargeTime += 0.5;
	}
}


void CSingleHgun::WeaponIdle( void )
{
	if ( g_fLoading == TRUE )
	{
		StopVModelSound( );
		return;
	}

	if ( m_pPlayer->m_fDidHolster )
	{
		if ( m_flTimeWeaponIdle <= gpGlobals->time )
		{
			m_pPlayer->m_fDidHolster = FALSE;
			Deploy( );
			UpdateItemInfo( );
		}
		return;
	}

	Reload( );

	m_pPlayer->GetAutoaimVector( 0 );

	if (m_flTimeWeaponIdle > gpGlobals->time)
		return;

	m_iSoundPlayed = -1;

	int iAnim;
	float flRand = RANDOM_FLOAT(0, 1);
	if (flRand <= 0.75)
	{
		iAnim = HGUN_IDLE1;
		m_flTimeWeaponIdle = gpGlobals->time + 30.0 / 16 * (2);
	}
	else if (flRand <= 0.875)
	{
		iAnim = HGUN_FIDGETSWAY;
		m_flTimeWeaponIdle = gpGlobals->time + 40.0 / 16.0;
		StartVModelSound( 0 );
	}
	else
	{
		iAnim = HGUN_FIDGETSHAKE;
		m_flTimeWeaponIdle = gpGlobals->time + 35.0 / 16.0;
		StartVModelSound( 1 );
	}
	SendWeaponAnim( iAnim );
}


// POV: the fidget animations' sounds, played by think
void CSingleHgun::StartVModelSound( int iSound )
{
	m_iSoundPlayed = iSound;

	switch ( iSound )
	{
	case 0:
		pev->nextthink = gpGlobals->time + 0.2;
		break;
	case 1:
		pev->nextthink = gpGlobals->time + 0.4;
		break;
	default:
		ALERT( at_console, "StartVModelSound: no %d sound to play!!\n", iSound );
		m_iSoundPlayed = -1;
		break;
	}

	if ( m_iSoundPlayed != -1 )
		SetThink( PlayVModelSound );
}

void CSingleHgun::PlayVModelSound( void )
{
	EMIT_SOUND_DYN( ENT(m_pPlayer->pev), CHAN_VOICE, pIdleSounds[m_iSoundPlayed], 0.3, ATTN_IDLE, 0, PITCH_NORM );
	SetThink( NULL );
	pev->nextthink = gpGlobals->time;
}

void CSingleHgun::StopVModelSound( void )
{
	SetThink( NULL );
	pev->nextthink = gpGlobals->time;

	if ( m_iSoundPlayed != -1 )
		EMIT_SOUND_DYN( ENT(m_pPlayer->pev), CHAN_VOICE, pIdleSounds[m_iSoundPlayed], 0.3, ATTN_IDLE, SND_STOP, PITCH_NORM );
}

#endif

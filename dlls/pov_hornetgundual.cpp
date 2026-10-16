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
// POV: weapon_hornetgundual, a hivehand in each hand. The
// SDK's hivehand, firing from the left and the right hand
// in turn, each hand with its own hornets. File name and
// the names retail doesn't give are ours.
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

enum dualhgun_e {
	DUALHGUN_IDLE1 = 0,
	DUALHGUN_IDLE2,
	DUALHGUN_IDLE3,
	DUALHGUN_FIDGETSWAYL,
	DUALHGUN_FIDGETSWAYR,
	DUALHGUN_FIDGETSHAKEL,
	DUALHGUN_FIDGETSHAKER,
	DUALHGUN_DOWN,
	DUALHGUN_UP,
	DUALHGUN_SHOOT_R,
	DUALHGUN_SHOOT_L
};

class CDualHGun : public CBasePlayerWeapon
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
	int SecondaryAmmoIndex( void );

	void StartVModelSound( int iSound );
	void EXPORT PlayVModelSound( void );
	void StopVModelSound( void );

	static const char *pIdleSounds[];

	int m_iSoundPlayed;

	float m_flNextAnimTime;

	float m_flRechargeTime;
	
	int m_iFirePhase;// don't save me.

	BOOL m_bFireLeft;
	BOOL m_bReloadLeft;
	int m_iMuzzleFlash;
};
LINK_ENTITY_TO_CLASS( weapon_hornetgundual, CDualHGun );

TYPEDESCRIPTION	CDualHGun::m_SaveData[] = 
{
	DEFINE_FIELD( CDualHGun, m_iSoundPlayed, FIELD_INTEGER ),
	DEFINE_FIELD( CDualHGun, m_bFireLeft, FIELD_BOOLEAN ),
	DEFINE_FIELD( CDualHGun, m_bReloadLeft, FIELD_BOOLEAN ),
	DEFINE_FIELD( CDualHGun, m_iMuzzleFlash, FIELD_INTEGER ),
};
IMPLEMENT_SAVERESTORE( CDualHGun, CBasePlayerWeapon );

const char *CDualHGun::pIdleSounds[] = 
{
	"horngun/horngun_idle2.wav",
	"horngun/horngun_idle1.wav",
};

BOOL CDualHGun::IsUseable( void )
{
	return TRUE;
}

void CDualHGun::Spawn( )
{
	Precache( );
	m_iId = WEAPON_HORNETGUNDUAL;
	SET_MODEL(ENT(pev), "models/w_hgun.mdl");

	m_iDefaultAmmo = HIVEHAND_DEFAULT_GIVE;
	m_iFirePhase = 0;
	m_bFireLeft = TRUE;
	m_bReloadLeft = TRUE;

	FallInit();// get ready to fall down.
}


void CDualHGun::Precache( void )
{
	PRECACHE_MODEL("models/v_dualhgun.mdl");
	PRECACHE_MODEL("models/w_hgun.mdl");
	PRECACHE_MODEL("models/p_hgun.mdl");

	m_iMuzzleFlash = PRECACHE_MODEL("sprites/muz4.spr");

	UTIL_PrecacheOther("hornet");

	PRECACHE_SOUND_ARRAY( pIdleSounds );
}

int CDualHGun::SecondaryAmmoIndex( void )
{
	return m_iSecondaryAmmoType;
}

int CDualHGun::AddToPlayer( CBasePlayer *pPlayer )
{
	if ( CBasePlayerWeapon::AddToPlayer( pPlayer ) )
	{
		pPlayer->m_rgAmmo[ PrimaryAmmoIndex() ] = HORNET_MAX_CARRY;
		pPlayer->m_rgAmmo[ SecondaryAmmoIndex() ] = HORNET_MAX_CARRY;

		MESSAGE_BEGIN( MSG_ONE, gmsgWeapPickup, NULL, pPlayer->pev );
			WRITE_BYTE( m_iId );
		MESSAGE_END();
		return TRUE;
	}
	return FALSE;
}

int CDualHGun::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "Hornets";
	p->iMaxAmmo1 = HORNET_MAX_CARRY;
	p->pszAmmo2 = "Hornets2";
	p->iMaxAmmo2 = HORNET_MAX_CARRY;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = 1;
	p->iPosition = 1;
	p->iId = m_iId = WEAPON_HORNETGUNDUAL;
	p->iFlags = ITEM_FLAG_NOAUTOSWITCHEMPTY | ITEM_FLAG_NOAUTORELOAD;
	p->iWeight = HORNETGUN_WEIGHT * 2;

	return 1;
}


BOOL CDualHGun::Deploy( )
{
	m_pPlayer->m_rgAmmo[ PrimaryAmmoIndex() ] = HORNET_MAX_CARRY;
	m_pPlayer->m_rgAmmo[ SecondaryAmmoIndex() ] = HORNET_MAX_CARRY;

	return DefaultDeploy( "models/v_dualhgun.mdl", "models/p_hgun.mdl", DUALHGUN_UP, "dualhive" );
}

void CDualHGun::Holster( void )
{
	StopVModelSound( );
	DefaultHolster( DUALHGUN_DOWN, 1.3, 1.3 );

	m_pPlayer->m_rgAmmo[ PrimaryAmmoIndex() ] = HORNET_MAX_CARRY;
	m_pPlayer->m_rgAmmo[ SecondaryAmmoIndex() ] = HORNET_MAX_CARRY;
}


void CDualHGun::PrimaryAttack()
{
	if ( m_pPlayer->m_fDidHolster )
	{
		WeaponIdle( );
		return;
	}

	m_bFireLeft = !m_bFireLeft;

	Reload( );

	if ( m_bFireLeft )
	{
		if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
			return;
	}
	else
	{
		if (m_pPlayer->m_rgAmmo[m_iSecondaryAmmoType] <= 0)
			return;
	}

	StopVModelSound( );

	pev->effects = EF_MUZZLEFLASH;

	Vector vecSrc;

	if ( m_bFireLeft )
		vecSrc = m_pPlayer->GetGunPosition( ) + gpGlobals->v_forward * 32 + gpGlobals->v_right * 13 + gpGlobals->v_up * -12;
	else
		vecSrc = m_pPlayer->GetGunPosition( ) + gpGlobals->v_forward * 32 - gpGlobals->v_right * 13 + gpGlobals->v_up * -12;

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

	if ( m_bFireLeft )
		vecSrc = m_pPlayer->GetGunPosition( ) + gpGlobals->v_forward * 16 + gpGlobals->v_right * 13 + gpGlobals->v_up * -12;
	else
		vecSrc = m_pPlayer->GetGunPosition( ) + gpGlobals->v_forward * 16 - gpGlobals->v_right * 13 + gpGlobals->v_up * -12;

	CBaseEntity *pHornet;
	if ( m_bFireLeft )
		pHornet = CBaseEntity::Create( "hornet", vecSrc, m_pPlayer->pev->v_angle, m_pPlayer->edict() );
	else
		pHornet = CBaseEntity::Create( "hornet", vecSrc, m_pPlayer->pev->v_angle, m_pPlayer->edict() );
	pHornet->pev->velocity = gpGlobals->v_forward * 300;

	if ( m_bFireLeft )
		m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType]--;
	else
		m_pPlayer->m_rgAmmo[m_iSecondaryAmmoType]--;
	m_flRechargeTime = gpGlobals->time + 0.25;
	
	m_pPlayer->m_iWeaponVolume = QUIET_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = DIM_GUN_FLASH;

	if ( m_bFireLeft )
		SendWeaponAnim( DUALHGUN_SHOOT_R );
	else
		SendWeaponAnim( DUALHGUN_SHOOT_L );

	// player "shoot" animation
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

	m_flNextPrimaryAttack = m_flNextPrimaryAttack + 0.15;

	if (m_flNextPrimaryAttack < gpGlobals->time)
	{
		m_flNextPrimaryAttack = gpGlobals->time + 0.15;
	}

	m_flTimeWeaponIdle = gpGlobals->time + RANDOM_FLOAT ( 4, 6 );
}



void CDualHGun::SecondaryAttack( void )
{
	if ( m_pPlayer->m_fDidHolster )
	{
		WeaponIdle( );
		return;
	}

	m_bFireLeft = !m_bFireLeft;

	Reload();

	if ( m_bFireLeft )
	{
		if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
			return;
	}
	else
	{
		if (m_pPlayer->m_rgAmmo[m_iSecondaryAmmoType] <= 0)
			return;
	}

	StopVModelSound( );

	CBaseEntity *pHornet;
	Vector vecSrc;

	UTIL_MakeVectors( m_pPlayer->pev->v_angle );

	if ( m_bFireLeft )
		vecSrc = m_pPlayer->GetGunPosition( ) + gpGlobals->v_forward * 32 + gpGlobals->v_right * 13 + gpGlobals->v_up * -12;
	else
		vecSrc = m_pPlayer->GetGunPosition( ) + gpGlobals->v_forward * 32 - gpGlobals->v_right * 13 + gpGlobals->v_up * -12;

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

	if ( m_bFireLeft )
		vecSrc = m_pPlayer->GetGunPosition( ) + gpGlobals->v_forward * 16 + gpGlobals->v_right * 13 + gpGlobals->v_up * -12;
	else
		vecSrc = m_pPlayer->GetGunPosition( ) + gpGlobals->v_forward * 16 - gpGlobals->v_right * 13 + gpGlobals->v_up * -12;

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

	if ( m_bFireLeft )
		m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType]--;
	else
		m_pPlayer->m_rgAmmo[m_iSecondaryAmmoType]--;
	m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = DIM_GUN_FLASH;

	m_flRechargeTime = gpGlobals->time + 0.5;

	if ( m_bFireLeft )
		SendWeaponAnim( DUALHGUN_SHOOT_R );
	else
		SendWeaponAnim( DUALHGUN_SHOOT_L );

	// player "shoot" animation
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

	m_flNextPrimaryAttack = m_flNextSecondaryAttack = gpGlobals->time + 0.1;
	m_flTimeWeaponIdle = gpGlobals->time + RANDOM_FLOAT ( 4, 6 );
	m_pPlayer->pev->punchangle.x = RANDOM_FLOAT( 0, 2 );
}


void CDualHGun::Reload( void )
{
	if ( m_pPlayer->m_fDidHolster )
	{
		WeaponIdle( );
		return;
	}

	if ( m_flRechargeTime > gpGlobals->time )
		return;

	m_bReloadLeft = !m_bReloadLeft;

	if ( m_bReloadLeft )
	{
		if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] < HORNET_MAX_CARRY)
		{
			m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType]++;
			m_flRechargeTime = gpGlobals->time + 0.25;
		}
	}
	else
	{
		if (m_pPlayer->m_rgAmmo[m_iSecondaryAmmoType] < HORNET_MAX_CARRY)
		{
			m_pPlayer->m_rgAmmo[m_iSecondaryAmmoType]++;
			m_flRechargeTime = gpGlobals->time + 0.25;
		}
	}
}


void CDualHGun::WeaponIdle( void )
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
		iAnim = RANDOM_LONG( DUALHGUN_IDLE1, DUALHGUN_IDLE3 );
		m_flTimeWeaponIdle = gpGlobals->time + 30.0 / 16 * (2);
	}
	else if (flRand <= 0.875)
	{
		iAnim = RANDOM_LONG( 0, 1 ) ? DUALHGUN_FIDGETSWAYL : DUALHGUN_FIDGETSWAYR;
		m_flTimeWeaponIdle = gpGlobals->time + 40.0 / 16.0;
		StartVModelSound( 0 );
	}
	else
	{
		iAnim = RANDOM_LONG( 0, 1 ) ? DUALHGUN_FIDGETSHAKEL : DUALHGUN_FIDGETSHAKER;
		m_flTimeWeaponIdle = gpGlobals->time + 35.0 / 16.0;
		StartVModelSound( 1 );
	}
	SendWeaponAnim( iAnim );
}


// POV: the fidget animations' sounds, played by think
void CDualHGun::StartVModelSound( int iSound )
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

void CDualHGun::PlayVModelSound( void )
{
	EMIT_SOUND_DYN( ENT(m_pPlayer->pev), CHAN_VOICE, pIdleSounds[m_iSoundPlayed], 0.3, ATTN_IDLE, 0, PITCH_NORM );
	SetThink( NULL );
	pev->nextthink = gpGlobals->time;
}

void CDualHGun::StopVModelSound( void )
{
	SetThink( NULL );
	pev->nextthink = gpGlobals->time;

	if ( m_iSoundPlayed != -1 )
		EMIT_SOUND_DYN( ENT(m_pPlayer->pev), CHAN_VOICE, pIdleSounds[m_iSoundPlayed], 0.3, ATTN_IDLE, SND_STOP, PITCH_NORM );
}

#endif

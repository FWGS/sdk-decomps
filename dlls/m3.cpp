/***
*
*	Counter-Strike Beta 2.0, m3.cpp: the Benelli M3 Super 90 shotgun
*	(weapon_m3).
*
*	SDK 1.0's shotgun.cpp as the Counter-Strike team changed it: CShotgun
*	renamed CM3 with its fields and its save table, the double-barrel
*	secondary attack gone, the shell thrown late by the weapon base
*	(m_pPlayer->m_flEjectBrass) instead of at once. The class name and its
*	fields are retail's (the save table's strings), the method names Beta
*	3.1's (Linux symbols); the animation names are the sequence labels of
*	models/v_m3.mdl.
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

// special deathmatch shotgun spreads
#define VECTOR_CONE_DM_SHOTGUN	Vector( 0.0675, 0.0675, 0.00  )

enum m3_e {
	M3_IDLE = 0,
	M3_FIRE1,
	M3_FIRE2,
	M3_RELOAD,
	M3_PUMP,
	M3_START_RELOAD,
	M3_DRAW
};

class CM3 : public CBasePlayerWeapon
{
public:
	int		Save( CSave &save );
	int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	void Spawn( void );
	void Precache( void );
	int iItemSlot( ) { return 1; }
	int GetItemInfo(ItemInfo *p);

	void PrimaryAttack( void );
	BOOL Deploy( );
	void Reload( void );
	void WeaponIdle( void );
	float GetMaxSpeed( void ) { return 230; }	// Beta 5.0 (leak): Linux symbol
	int m_fInReload;
	float m_flNextReload;
	int m_iShell;
	float m_flPumpTime;
};
LINK_ENTITY_TO_CLASS( weapon_m3, CM3 );


TYPEDESCRIPTION	CM3::m_SaveData[] =
{
	DEFINE_FIELD( CM3, m_flNextReload, FIELD_TIME ),	// sic: twice, as in CShotgun's
	DEFINE_FIELD( CM3, m_fInReload, FIELD_INTEGER ),
	DEFINE_FIELD( CM3, m_flNextReload, FIELD_TIME ),
	// DEFINE_FIELD( CM3, m_iShell, FIELD_INTEGER ),
	DEFINE_FIELD( CM3, m_flPumpTime, FIELD_TIME ),
};
IMPLEMENT_SAVERESTORE( CM3, CBasePlayerWeapon );



void CM3::Spawn( )
{
	Precache( );
	m_iId = WEAPON_M3;
	pev->classname = MAKE_STRING("weapon_m3");
	SET_MODEL(ENT(pev), "models/w_m3.mdl");

	m_iDefaultAmmo = M3_DEFAULT_GIVE;

	FallInit();// get ready to fall
}


void CM3::Precache( void )
{
	PRECACHE_MODEL("models/v_m3.mdl");
	PRECACHE_MODEL("models/v_m3_r.mdl");	// Beta 6.0: the right-handed view model
	PRECACHE_MODEL("models/w_m3.mdl");
	PRECACHE_MODEL("models/p_m3.mdl");

	m_iShellLate = m_iShell = PRECACHE_MODEL ("models/shotgunshell.mdl");// shotgun shell

	PRECACHE_SOUND ("weapons/m3-1.wav");//shotgun

	PRECACHE_SOUND ("weapons/reload1.wav");	// shotgun reload
	PRECACHE_SOUND ("weapons/reload3.wav");	// shotgun reload
}


int CM3::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "buckshot";
	p->iMaxAmmo1 = M3_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = M3_MAX_CLIP;
	p->iSlot = 0;
	p->iPosition = 5;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_M3;
	p->iWeight = M3_WEIGHT;

	return 1;
}



BOOL CM3::Deploy( )
{
	// Beta 6.0: v_m3.mdl holds the gun in the left hand, v_m3_r.mdl in the right
	if ( m_pPlayer->m_bLeftHanded == true )
		return DefaultDeploy( "models/v_m3.mdl", "models/p_m3.mdl", M3_DRAW, "mp5" );

	return DefaultDeploy( "models/v_m3_r.mdl", "models/p_m3.mdl", M3_DRAW, "mp5" );
}


void CM3::PrimaryAttack()
{
	// don't fire underwater
	if (m_pPlayer->pev->waterlevel == 3)
	{
		PlayEmptySound( );
		m_flNextPrimaryAttack = gpGlobals->time + 0.15;
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
	m_pPlayer->m_iWeaponFlash = BRIGHT_GUN_FLASH;

	m_iClip--;
	m_pPlayer->pev->effects = (int)(m_pPlayer->pev->effects) | EF_MUZZLEFLASH;

	if (RANDOM_LONG(0,1))
		SendWeaponAnim( M3_FIRE1 );
	else
		SendWeaponAnim( M3_FIRE2 );

	// player "shoot" animation
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

	UTIL_MakeVectors( m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle );

	// Beta 6.0: no shell velocity computed here any more (Beta 5.2 computed one
	// and used it nowhere)

	EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/m3-1.wav", RANDOM_FLOAT(0.95, 1.0), 0.4);

	Vector vecAiming = m_pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );

	if (!m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);

	if (m_iClip != 0)
		m_flPumpTime = gpGlobals->time + 0.5;

	m_flNextPrimaryAttack = gpGlobals->time + 0.675;
	m_flNextSecondaryAttack = gpGlobals->time + 0.675;
	if (m_iClip != 0)
		m_flTimeWeaponIdle = gpGlobals->time + 5.0;
	else
		m_flTimeWeaponIdle = 0.75;	// sic: an absolute time, as in CShotgun's
	m_fInReload = 0;

	m_pPlayer->pev->punchangle.x -= RANDOM_LONG( 4, 6 );

	// Beta 6.1: the pellets leave from the eye again ("off center shooting
	// fixed"; Beta 6.0: 5 units to the gun hand's side); one spread,
	// 0.0675 sideways and up, 3000 units far
	Vector vecSrc	 = m_pPlayer->GetGunPosition( );

	// the shell comes out with the pump: EjectBrassLate, from ItemPostFrame
	if ( m_pPlayer->m_bLeftHanded == true )
	{
		m_pPlayer->m_flEjectBrass = gpGlobals->time + 0.45;
	}
	else
	{
		m_pPlayer->m_flEjectBrass = gpGlobals->time + 0.45;
	}

	m_pPlayer->FireBullets( 8, vecSrc, vecAiming, Vector( 0.0675, 0.0675, 0 ), 3000, BULLET_PLAYER_BUCKSHOT, 0 );
}


void CM3::Reload( void )
{
	if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 || m_iClip == M3_MAX_CLIP)
		return;

	if (m_flNextReload > gpGlobals->time)
		return;

	// don't reload until recoil is done
	if (m_flNextPrimaryAttack > gpGlobals->time)
		return;

	// check to see if we're ready to reload
	if (m_fInReload == 0)
	{
		SendWeaponAnim( M3_START_RELOAD );
		m_fInReload = 1;
		m_pPlayer->m_flNextAttack = gpGlobals->time + 0.45;	// Beta 5.0 (leak) (was 0.6)
		m_flTimeWeaponIdle = gpGlobals->time + 0.45;
		m_flNextPrimaryAttack = gpGlobals->time + 1.0;
		m_flNextSecondaryAttack = gpGlobals->time + 1.0;
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

		SendWeaponAnim( M3_RELOAD );

		m_flNextReload = gpGlobals->time + 0.35;	// Beta 5.0 (leak) (was 0.5)
		m_flTimeWeaponIdle = gpGlobals->time + 0.35;
	}
	else
	{
		// Add them to the clip
		m_iClip += 1;
		m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] -= 1;
		m_fInReload = 1;
	}
}


void CM3::WeaponIdle( void )
{
	ResetEmptySound( );

	m_pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );

	if (m_flPumpTime && m_flPumpTime < gpGlobals->time)
	{
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
			if (m_iClip != 8 && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType])
			{
				Reload( );
			}
			else
			{
				// reload debounce has timed out
				SendWeaponAnim( M3_PUMP );

				m_fInReload = 0;
				m_flTimeWeaponIdle = gpGlobals->time + 1.5;
			}
		}
		else
		{
			SendWeaponAnim( M3_IDLE );
		}
	}
}

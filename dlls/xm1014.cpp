/***
*
*	Counter-Strike Beta 5.0 (leak), xm1014.cpp: the Benelli XM1014 automatic
*	shotgun (weapon_xm1014).
*
*	m3.cpp's text changed, as the Counter-Strike team copied it: its own
*	models and sound, seven shells, six pellets a shot in a wider cone,
*	four shots a second, the shell thrown a tenth of a second after the shot,
*	quicker shell by shell reloading. File, class and method names are Beta
*	5.0's (Linux symbols); the animation names and the XM1014_ macros are
*	ours, the fields CM3's (the save table's strings).
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
#define VECTOR_CONE_DM_SHOTGUN	Vector( 0.0725, 0.0725, 0.00  )

enum xm1014_e {
	XM1014_IDLE = 0,
	XM1014_FIRE1,
	XM1014_FIRE2,
	XM1014_RELOAD,
	XM1014_PUMP,
	XM1014_START_RELOAD,
	XM1014_DRAW
};

class CXM1014 : public CBasePlayerWeapon
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
	float GetMaxSpeed( void ) { return 250; }
	int m_fInReload;
	float m_flNextReload;
	int m_iShell;
	float m_flPumpTime;
};
LINK_ENTITY_TO_CLASS( weapon_xm1014, CXM1014 );


TYPEDESCRIPTION	CXM1014::m_SaveData[] =
{
	DEFINE_FIELD( CXM1014, m_flNextReload, FIELD_TIME ),	// sic: twice, as in CShotgun's
	DEFINE_FIELD( CXM1014, m_fInReload, FIELD_INTEGER ),
	DEFINE_FIELD( CXM1014, m_flNextReload, FIELD_TIME ),
	// DEFINE_FIELD( CXM1014, m_iShell, FIELD_INTEGER ),
	DEFINE_FIELD( CXM1014, m_flPumpTime, FIELD_TIME ),
};
IMPLEMENT_SAVERESTORE( CXM1014, CBasePlayerWeapon );



void CXM1014::Spawn( )
{
	Precache( );
	m_iId = WEAPON_XM1014;
	pev->classname = MAKE_STRING("weapon_xm1014");
	SET_MODEL(ENT(pev), "models/w_xm1014.mdl");

	m_iDefaultAmmo = XM1014_DEFAULT_GIVE;

	FallInit();// get ready to fall
}


void CXM1014::Precache( void )
{
	PRECACHE_MODEL("models/v_xm1014.mdl");
	PRECACHE_MODEL("models/w_xm1014.mdl");
	PRECACHE_MODEL("models/p_xm1014.mdl");

	m_iShellLate = m_iShell = PRECACHE_MODEL ("models/shotgunshell.mdl");// shotgun shell

	PRECACHE_SOUND ("weapons/xm1014-1.wav");//shotgun

	PRECACHE_SOUND ("weapons/reload1.wav");	// shotgun reload
	PRECACHE_SOUND ("weapons/reload3.wav");	// shotgun reload
}


int CXM1014::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "buckshot";
	p->iMaxAmmo1 = XM1014_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = XM1014_MAX_CLIP;
	p->iSlot = 0;
	p->iPosition = 12;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_XM1014;
	p->iWeight = XM1014_WEIGHT;

	return 1;
}



BOOL CXM1014::Deploy( )
{
	return DefaultDeploy( "models/v_xm1014.mdl", "models/p_xm1014.mdl", XM1014_DRAW, "mp5" );
}


void CXM1014::PrimaryAttack()
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
		SendWeaponAnim( XM1014_FIRE1 );
	else
		SendWeaponAnim( XM1014_FIRE2 );

	// player "shoot" animation
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

	UTIL_MakeVectors( m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle );

	Vector	vecShellVelocity = m_pPlayer->pev->velocity
							 + gpGlobals->v_right * RANDOM_FLOAT(100,170)
							 + gpGlobals->v_up * RANDOM_FLOAT(100,150)
							 + gpGlobals->v_forward * 25;

	// the shell comes out with the pump: EjectBrassLate, from ItemPostFrame
	m_pPlayer->m_flEjectBrass = gpGlobals->time + 0.1;

	EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/xm1014-1.wav", RANDOM_FLOAT(0.95, 1.0), 0.4);


	Vector vecSrc	 = m_pPlayer->GetGunPosition( );
	Vector vecAiming = m_pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );

	if ( g_pGameRules->IsDeathmatch() )
	{
		// altered deathmatch spread
		m_pPlayer->FireBullets( 6, vecSrc, vecAiming, VECTOR_CONE_DM_SHOTGUN, 2048, BULLET_PLAYER_BUCKSHOT, 0 );
	}
	else
	{
		// regular old, untouched spread.
		m_pPlayer->FireBullets( 6, vecSrc, vecAiming, VECTOR_CONE_10DEGREES, 2048, BULLET_PLAYER_BUCKSHOT, 0 );
	}

	if (!m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);

	if (m_iClip != 0)
		m_flPumpTime = gpGlobals->time + 0.125;

	m_flNextPrimaryAttack = gpGlobals->time + 0.25;
	m_flNextSecondaryAttack = gpGlobals->time + 0.25;
	if (m_iClip != 0)
		m_flTimeWeaponIdle = gpGlobals->time + 5.0;
	else
		m_flTimeWeaponIdle = 0.75;	// sic: an absolute time, as in CShotgun's
	m_fInReload = 0;

	m_pPlayer->pev->punchangle.x -= RANDOM_LONG( 3, 5 );	// Beta 5.0 (the leak: 4 to 6)
}


void CXM1014::Reload( void )
{
	if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 || m_iClip == XM1014_MAX_CLIP)
		return;

	if (m_flNextReload > gpGlobals->time)
		return;

	// don't reload until recoil is done
	if (m_flNextPrimaryAttack > gpGlobals->time)
		return;

	// check to see if we're ready to reload
	if (m_fInReload == 0)
	{
		SendWeaponAnim( XM1014_START_RELOAD );
		m_fInReload = 1;
		m_pPlayer->m_flNextAttack = gpGlobals->time + 0.4;
		m_flTimeWeaponIdle = gpGlobals->time + 0.4;
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

		SendWeaponAnim( XM1014_RELOAD );

		m_flNextReload = gpGlobals->time + 0.3;
		m_flTimeWeaponIdle = gpGlobals->time + 0.3;
	}
	else
	{
		// Add them to the clip
		m_iClip += 1;
		m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] -= 1;
		m_fInReload = 1;
	}
}


void CXM1014::WeaponIdle( void )
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
			if (m_iClip != 7 && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType])
			{
				Reload( );
			}
			else
			{
				// reload debounce has timed out
				SendWeaponAnim( XM1014_PUMP );

				m_fInReload = 0;
				m_flTimeWeaponIdle = gpGlobals->time + 1.5;
			}
		}
		else
		{
			SendWeaponAnim( XM1014_IDLE );
		}
	}
}

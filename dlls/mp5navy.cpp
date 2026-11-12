/***
*
*	Counter-Strike Beta 1.1, mp5navy.cpp: the MP5 Navy submachine gun
*	(weapon_mp5navy).
*
*	The M4A1's shape without the zoom: both attacks fire. weapon_sg552,
*	weapon_knife, weapon_c4 and weapon_claymore are this class too (sic):
*	names without weapons of their own yet, each an MP5 Navy when it
*	spawns. File, class and method names are Beta 3.1's (Linux symbols);
*	the animation names and m_fEjectShell are ours.
*
****/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"

enum mp5n_e {
	MP5N_IDLE1 = 0,
	MP5N_RELOAD,
	MP5N_DRAW,
	MP5N_SHOOT1,
	MP5N_SHOOT2,
	MP5N_SHOOT3,
};

class CMP5N : public CBasePlayerWeapon
{
public:
	void Spawn( void );
	void Precache( void );
	int iItemSlot( void ) { return 4; }
	int GetItemInfo(ItemInfo *p);

	void PrimaryAttack( void );
	void SecondaryAttack( void );
	void MP5NFire( float flSpread, float flCycleTime, BOOL fUseAutoAim );
	BOOL Deploy( void );
	void Reload( void );
	void WeaponIdle( void );
	int m_iShell;
	BOOL m_fEjectShell;		// the shot throws a shell; flips with every shot
};
LINK_ENTITY_TO_CLASS( weapon_sg552, CMP5N );
LINK_ENTITY_TO_CLASS( weapon_knife, CMP5N );
LINK_ENTITY_TO_CLASS( weapon_c4, CMP5N );
LINK_ENTITY_TO_CLASS( weapon_claymore, CMP5N );
LINK_ENTITY_TO_CLASS( weapon_mp5navy, CMP5N );


void CMP5N::Spawn( )
{
	pev->classname = MAKE_STRING("weapon_mp5navy");
	Precache( );
	m_iId = WEAPON_MP5N;
	SET_MODEL(ENT(pev), "models/w_mp5.mdl");

	m_iDefaultAmmo = MP5N_DEFAULT_GIVE;

	FallInit();// get ready to fall down.
}


void CMP5N::Precache( void )
{
	PRECACHE_MODEL("models/v_mp5.mdl");
	PRECACHE_MODEL("models/w_mp5.mdl");
	PRECACHE_MODEL("models/p_mp5.mdl");

	PRECACHE_SOUND("weapons/mp5-1.wav");
	PRECACHE_SOUND("weapons/mp5-2.wav");
	PRECACHE_SOUND("weapons/mp5_clipout.wav");
	PRECACHE_SOUND("weapons/mp5_clipin.wav");
	PRECACHE_SOUND("weapons/mp5_slideback.wav");

	m_iShell = PRECACHE_MODEL ("models/shell.mdl");// brass shell
}

int CMP5N::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "9mm";
	p->iMaxAmmo1 = MP5N_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = MP5N_MAX_CLIP;
	p->iSlot = 3;
	p->iPosition = 2;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_MP5N;
	p->iWeight = MP5N_WEIGHT;

	return 1;
}

BOOL CMP5N::Deploy( )
{
	return DefaultDeploy( "models/v_mp5.mdl", "models/p_mp5.mdl", MP5N_DRAW, "mp5" );
}

void CMP5N::SecondaryAttack( void )
{
	float flSpread;

	if (m_pPlayer->pev->velocity.Length2D() > 0)
	{
		flSpread = m_pPlayer->m_flAccuracy * 0.07;
		MP5NFire( flSpread, 0.085, FALSE );
	}
	else if (m_pPlayer->pev->flags & FL_DUCKING)
	{
		flSpread = m_pPlayer->m_flAccuracy * 0.04;
		MP5NFire( flSpread, 0.085, FALSE );
	}
	else
	{
		flSpread = m_pPlayer->m_flAccuracy * 0.05;
		MP5NFire( flSpread, 0.085, FALSE );
	}
}

void CMP5N::PrimaryAttack( void )
{
	float flSpread;

	if (m_pPlayer->pev->velocity.Length2D() > 0)
	{
		flSpread = m_pPlayer->m_flAccuracy * 0.07;
		MP5NFire( flSpread, 0.085, FALSE );
	}
	else if (m_pPlayer->pev->flags & FL_DUCKING)
	{
		flSpread = m_pPlayer->m_flAccuracy * 0.07;
		MP5NFire( flSpread, 0.085, FALSE );
	}
	else
	{
		flSpread = m_pPlayer->m_flAccuracy * 0.09;
		MP5NFire( flSpread, 0.085, FALSE );
	}
}

void CMP5N::MP5NFire( float flSpread , float flCycleTime, BOOL fUseAutoAim )
{
	// the spread grows with the square of the shots fired
	m_pPlayer->m_iShotsFired++;
	m_pPlayer->m_flAccuracy = (m_pPlayer->m_iShotsFired * m_pPlayer->m_iShotsFired / 200) + 0.7;

	if (m_iClip <= 0)
	{
		if (m_fFireOnEmpty)
		{
			PlayEmptySound();
			m_flNextPrimaryAttack = gpGlobals->time + 0.2;
		}

		return;
	}

	m_iClip--;

	m_pPlayer->pev->effects = (int)(m_pPlayer->pev->effects) | EF_MUZZLEFLASH;

	if (m_iClip != 0)
	{
		switch (RANDOM_LONG(0,2))
		{
		case 0:
			SendWeaponAnim( MP5N_SHOOT1 );
			// no breaks (sic): case 0 sends all three animations, case 1 the last two
		case 1:
			SendWeaponAnim( MP5N_SHOOT2 );
		case 2:
			SendWeaponAnim( MP5N_SHOOT3 );
		}
	}

	// player "shoot" animation
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

	UTIL_MakeVectors( m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle );

	Vector	vecShellVelocity = m_pPlayer->pev->velocity
							 + gpGlobals->v_right * RANDOM_FLOAT(150,200)
							 + gpGlobals->v_up * RANDOM_FLOAT(25,55)
							 + gpGlobals->v_forward * 25;
	if (m_fEjectShell)
		EjectBrass2 ( pev->origin + m_pPlayer->pev->view_ofs + gpGlobals->v_up * -11 + gpGlobals->v_forward * 32 - gpGlobals->v_right * 7, vecShellVelocity, pev->angles.y, m_iShell, TE_BOUNCE_SHELL, m_pPlayer->pev );
	m_fEjectShell = 1 - m_fEjectShell;

	m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = DIM_GUN_FLASH;

	if (RANDOM_LONG(0,1))
		EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/mp5-1.wav", RANDOM_FLOAT(0.92, 1.0), ATTN_NORM, 0, 98 + RANDOM_LONG(0,3));
	else
		EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/mp5-2.wav", RANDOM_FLOAT(0.92, 1.0), ATTN_NORM, 0, 98 + RANDOM_LONG(0,3));

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

	m_pPlayer->FireBullets3( vecSrc, vecAiming, Vector( flSpread, flSpread, flSpread ), 8192, 1, BULLET_PLAYER_9MM, 19, 0.92 );
	m_flNextPrimaryAttack = m_flNextSecondaryAttack = gpGlobals->time + flCycleTime;

	if (!m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);

	m_flTimeWeaponIdle = gpGlobals->time + RANDOM_FLOAT ( 10, 15 );

	m_pPlayer->pev->punchangle.x = RANDOM_FLOAT( -1.5, 1.5 );
}


void CMP5N::Reload( void )
{
	if (DefaultReload( MP5N_MAX_CLIP, MP5N_RELOAD, 2.3 ))
	{
		m_flTimeWeaponIdle = gpGlobals->time + RANDOM_FLOAT ( 10, 15 );
	}

	m_pPlayer->m_flAccuracy = 0;
}



void CMP5N::WeaponIdle( void )
{
	ResetEmptySound( );

	m_pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );

	if (m_flTimeWeaponIdle > gpGlobals->time)
		return;

	// sic: m_flTimeWeaponIdle is not renewed, so once it has passed every call sends the idle animation again
	SendWeaponAnim( MP5N_IDLE1 );
}

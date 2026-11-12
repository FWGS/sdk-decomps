/***
*
*	Counter-Strike Beta 6.0, mac10.cpp: the MAC-10 submachine gun
*	(weapon_mac10).
*
*	The TMP's text (tmp.cpp) as Beta 6.0 has it, loud: a muzzle flash, a dim
*	gun flash, one sound, its own .45 ACP ammunition (named "45acp", sic: the
*	USP's is "45ACP", so the two do not share). File, class and method names
*	are Beta 6.0's (Linux symbols); the animation names and m_fEjectShell are
*	ours.
*
****/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"

enum mac10_e {
	MAC10_IDLE1 = 0,
	MAC10_RELOAD,
	MAC10_DRAW,
	MAC10_SHOOT1,
	MAC10_SHOOT2,
	MAC10_SHOOT3,
};

class CMAC10 : public CBasePlayerWeapon
{
public:
	void Spawn( void );
	void Precache( void );
	int iItemSlot( void ) { return 1; }
	int GetItemInfo(ItemInfo *p);
	float GetMaxSpeed( void ) { return 250; }

	void PrimaryAttack( void );
	void MAC10Fire( float flSpread, float flCycleTime, BOOL fUseAutoAim );
	BOOL Deploy( void );
	void Reload( void );
	void WeaponIdle( void );
	int m_iShell;
	BOOL m_fEjectShell;		// the shot throws a shell; flips with every shot
};
LINK_ENTITY_TO_CLASS( weapon_mac10, CMAC10 );


void CMAC10::Spawn( )
{
	pev->classname = MAKE_STRING("weapon_mac10");
	Precache( );
	m_iId = WEAPON_MAC10;
	SET_MODEL(ENT(pev), "models/w_mac10.mdl");

	m_iDefaultAmmo = MAC10_DEFAULT_GIVE;

	FallInit();// get ready to fall down.
}


void CMAC10::Precache( void )
{
	PRECACHE_MODEL("models/v_mac10.mdl");
	PRECACHE_MODEL("models/v_mac10_r.mdl");
	PRECACHE_MODEL("models/w_mac10.mdl");
	PRECACHE_MODEL("models/p_mac10.mdl");

	PRECACHE_SOUND("weapons/mac10-1.wav");
	PRECACHE_SOUND("weapons/mac10_clipout.wav");
	PRECACHE_SOUND("weapons/mac10_clipin.wav");
	PRECACHE_SOUND("weapons/mac10_boltpull.wav");

	m_iShell = PRECACHE_MODEL ("models/pshell.mdl");// brass shell
}

int CMAC10::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "45acp";	// sic: not the USP's "45ACP"
	p->iMaxAmmo1 = MAC10_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = MAC10_MAX_CLIP;
	p->iSlot = 0;
	p->iPosition = 13;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_MAC10;
	p->iWeight = MAC10_WEIGHT;

	return 1;
}

BOOL CMAC10::Deploy( )
{
	m_pPlayer->m_flAccuracy = 0.15;
	m_fDelayFire = false;
	m_fEjectShell = 1;

	// v_mac10.mdl holds the gun in the left hand, v_mac10_r.mdl in the right
	if ( m_pPlayer->m_bLeftHanded == true )
		return DefaultDeploy( "models/v_mac10.mdl", "models/p_mac10.mdl", MAC10_DRAW, "onehanded" );

	return DefaultDeploy( "models/v_mac10_r.mdl", "models/p_mac10.mdl", MAC10_DRAW, "onehanded" );
}

void CMAC10::PrimaryAttack( void )
{
	// off the ground the spread is 0.15 times the accuracy
	if ( !FBitSet( m_pPlayer->pev->flags, FL_ONGROUND ) )
		MAC10Fire( m_pPlayer->m_flAccuracy * 0.15, 0.07, FALSE );
	else
		MAC10Fire( m_pPlayer->m_flAccuracy * 0.03, 0.07, FALSE );
}

void CMAC10::MAC10Fire( float flSpread , float flCycleTime, BOOL fUseAutoAim )
{
	// the spread grows with the cube of the shots fired
	m_pPlayer->m_iShotsFired++;
	m_pPlayer->m_flAccuracy = (m_pPlayer->m_iShotsFired * m_pPlayer->m_iShotsFired * m_pPlayer->m_iShotsFired / 200) + 0.6;

	if (m_pPlayer->m_flAccuracy > 1.65)
		m_pPlayer->m_flAccuracy = 1.65;

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
			SendWeaponAnim( MAC10_SHOOT1 );
			break;
		case 1:
			SendWeaponAnim( MAC10_SHOOT2 );
			break;
		case 2:
			SendWeaponAnim( MAC10_SHOOT3 );
			break;
		}
	}

	// player "shoot" animation
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

	UTIL_MakeVectors( m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle );

	m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = DIM_GUN_FLASH;

	EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/mac10-1.wav", 0.95, 0.8);

	Vector vecAiming;

	if ( fUseAutoAim )
	{
		vecAiming = m_pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );
	}
	else
	{
		vecAiming = gpGlobals->v_forward;
	}

	m_flNextPrimaryAttack = m_flNextSecondaryAttack = gpGlobals->time + flCycleTime;

	if (!m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);

	m_flTimeWeaponIdle = gpGlobals->time + RANDOM_FLOAT ( 10, 15 );

	// the recoil by how the player stands
	if (m_pPlayer->pev->velocity.Length2D() > 0)
		KickBack( 0.9, 0.45, 0.25, 0.035, 3.5, 2.75, 7 );
	else if ( !FBitSet(m_pPlayer->pev->flags, FL_ONGROUND) )
		KickBack( 1.2, 0.45, 0.3, 0.04, 3.75, 2.75, 7 );
	else if ( FBitSet(m_pPlayer->pev->flags, FL_DUCKING) )
		KickBack( 0.75, 0.4, 0.175, 0.03, 2.75, 2.5, 10 );
	else
		KickBack( 0.775, 0.425, 0.2, 0.03, 3, 2.75, 9 );

	// the bullets leave 5 units to the side of the hand that holds the gun,
	// the shell flies out of the other side
	Vector vecSrc	 = m_pPlayer->GetGunPosition( );

	if ( m_pPlayer->m_bLeftHanded == true )
	{
		vecSrc = vecSrc - gpGlobals->v_right * 5;

		Vector	vecShellVelocity = m_pPlayer->pev->velocity
								 + gpGlobals->v_right * RANDOM_FLOAT(100,120)
								 + gpGlobals->v_up * RANDOM_FLOAT(25,75)
								 + gpGlobals->v_forward * 25;
		if (m_fEjectShell)
			EjectBrass2 ( pev->origin + m_pPlayer->pev->view_ofs + gpGlobals->v_up * -9 + gpGlobals->v_forward * 32 - gpGlobals->v_right * 11, vecShellVelocity, pev->angles.y, m_iShell, TE_BOUNCE_SHELL, m_pPlayer->pev );
		m_fEjectShell = 1 - m_fEjectShell;
	}
	else
	{
		vecSrc = vecSrc + gpGlobals->v_right * 5;

		Vector	vecShellVelocity = m_pPlayer->pev->velocity
								 - gpGlobals->v_right * RANDOM_FLOAT(100,120)
								 + gpGlobals->v_up * RANDOM_FLOAT(25,75)
								 + gpGlobals->v_forward * 25;
		if (m_fEjectShell)
			EjectBrass2 ( pev->origin + m_pPlayer->pev->view_ofs + gpGlobals->v_up * -9 + gpGlobals->v_forward * 32 + gpGlobals->v_right * 11, vecShellVelocity, pev->angles.y, m_iShell, TE_BOUNCE_SHELL, m_pPlayer->pev );
		m_fEjectShell = 1 - m_fEjectShell;
	}

	m_pPlayer->FireBullets3( vecSrc, vecAiming, Vector( flSpread, flSpread, flSpread ), 8192, 1, BULLET_PLAYER_45ACP, 29, 0.82 );
}


void CMAC10::Reload( void )
{
	if (DefaultReload( MAC10_MAX_CLIP, MAC10_RELOAD, 3.3 ))
	{
		m_flTimeWeaponIdle = gpGlobals->time + RANDOM_FLOAT ( 10, 15 );
	}

	m_pPlayer->m_flAccuracy = 0;
}



void CMAC10::WeaponIdle( void )
{
	ResetEmptySound( );

	m_pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );

	if (m_flTimeWeaponIdle > gpGlobals->time)
		return;

	m_flTimeWeaponIdle = gpGlobals->time + 20;
	SendWeaponAnim( MAC10_IDLE1 );
}

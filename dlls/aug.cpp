/***
*
*	Counter-Strike Beta 6.0, aug.cpp: the Steyr AUG rifle (weapon_aug).
*
*	The SG-552's text (sg552.cpp) as Beta 6.0 has it: the spread grows with
*	the shots fired, every other shot throws a shell, the secondary attack is
*	a single zoom (to 65 degrees) that slows the cycle; one sound. File,
*	class and method names are Beta 6.0's (Linux symbols); the animation
*	names and m_fEjectShell are ours.
*
****/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"

enum aug_e {
	AUG_IDLE1 = 0,
	AUG_RELOAD,
	AUG_DRAW,
	AUG_SHOOT1,
	AUG_SHOOT2,
	AUG_SHOOT3,
};

class CAUG : public CBasePlayerWeapon
{
public:
	void Spawn( void );
	void Precache( void );
	int iItemSlot( void ) { return 1; }
	int GetItemInfo(ItemInfo *p);

	void PrimaryAttack( void );
	void SecondaryAttack( void );
	void AUGFire( float flSpread, float flCycleTime, BOOL fUseAutoAim );
	BOOL Deploy( void );
	void Reload( void );
	void WeaponIdle( void );
	float GetMaxSpeed( void ) { return 240; }
	int m_iShell;
	BOOL m_fEjectShell;		// the shot throws a shell; flips with every shot
};
LINK_ENTITY_TO_CLASS( weapon_aug, CAUG );


void CAUG::Spawn( )
{
	pev->classname = MAKE_STRING("weapon_aug");
	Precache( );
	m_iId = WEAPON_AUG;
	SET_MODEL(ENT(pev), "models/w_aug.mdl");

	m_iDefaultAmmo = AUG_DEFAULT_GIVE;

	FallInit();// get ready to fall down.
}


void CAUG::Precache( void )
{
	PRECACHE_MODEL("models/v_aug.mdl");
	PRECACHE_MODEL("models/v_aug_r.mdl");
	PRECACHE_MODEL("models/w_aug.mdl");
	PRECACHE_MODEL("models/p_aug.mdl");

	PRECACHE_SOUND("weapons/aug-1.wav");
	PRECACHE_SOUND("weapons/aug_clipout.wav");
	PRECACHE_SOUND("weapons/aug_clipin.wav");
	PRECACHE_SOUND("weapons/aug_boltpull.wav");
	PRECACHE_SOUND("weapons/aug_boltslap.wav");
	PRECACHE_SOUND("weapons/aug_forearm.wav");

	m_iShell = PRECACHE_MODEL ("models/rshell.mdl");// brass shell
}

int CAUG::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "556Nato";
	p->iMaxAmmo1 = AUG_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = AUG_MAX_CLIP;
	p->iSlot = 0;
	p->iPosition = 14;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_AUG;
	p->iWeight = AUG_WEIGHT;

	return 1;
}

BOOL CAUG::Deploy( )
{
	m_pPlayer->m_flAccuracy = 0;
	m_fEjectShell = 1;

	// v_aug.mdl holds the gun in the left hand, v_aug_r.mdl in the right
	if ( m_pPlayer->m_bLeftHanded == true )
		return DefaultDeploy( "models/v_aug.mdl", "models/p_aug.mdl", AUG_DRAW, "aug" );

	return DefaultDeploy( "models/v_aug_r.mdl", "models/p_aug.mdl", AUG_DRAW, "aug" );
}

void CAUG::SecondaryAttack( void )
{
	if (m_pPlayer->m_iFOV == 90)
		m_pPlayer->m_iFOV = 65;
	else if (m_pPlayer->m_iFOV == 65)
		m_pPlayer->m_iFOV = 90;
	else
		m_pPlayer->m_iFOV = 90;

	m_flNextSecondaryAttack = gpGlobals->time + 0.3;
}

void CAUG::PrimaryAttack( void )
{
	// off the ground the spread is 0.25 times the accuracy
	if ( !FBitSet( m_pPlayer->pev->flags, FL_ONGROUND ) )
		AUGFire( m_pPlayer->m_flAccuracy * 0.25, 0.0825, FALSE );
	else if (m_pPlayer->m_iFOV == 90)
		AUGFire( m_pPlayer->m_flAccuracy * 0.02, 0.0825, FALSE );
	else
		AUGFire( m_pPlayer->m_flAccuracy * 0.02, 0.135, FALSE );	// zoomed in: a slower cycle
}

void CAUG::AUGFire( float flSpread , float flCycleTime, BOOL fUseAutoAim )
{
	// the spread grows with the cube of the shots fired
	m_fDelayFire = true;
	m_pPlayer->m_iShotsFired++;
	m_pPlayer->m_flAccuracy = (m_pPlayer->m_iShotsFired * m_pPlayer->m_iShotsFired * m_pPlayer->m_iShotsFired / 275) + 0.3;

	if (m_pPlayer->m_flAccuracy > 0.5)
		m_pPlayer->m_flAccuracy = 0.5;

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
			SendWeaponAnim( AUG_SHOOT1 );
			break;
		case 1:
			SendWeaponAnim( AUG_SHOOT2 );
			break;
		case 2:
			SendWeaponAnim( AUG_SHOOT3 );
			break;
		}
	}

	// player "shoot" animation
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

	UTIL_MakeVectors( m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle );

	m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = BRIGHT_GUN_FLASH;

	EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/aug-1.wav", RANDOM_FLOAT(0.92, 1.0), 0.56);

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
		KickBack( 1.15, 0.45, 0.22, 0.18, 5, 3.5, 6 );
	else if ( !FBitSet(m_pPlayer->pev->flags, FL_ONGROUND) )
		KickBack( 1.25, 0.45, 0.22, 0.18, 5.5, 4, 5 );
	else if ( FBitSet(m_pPlayer->pev->flags, FL_DUCKING) )
		KickBack( 0.525, 0.275, 0.2, 0.011, 4.25, 1.25, 12 );
	else
		KickBack( 0.575, 0.325, 0.25, 0.0125, 4.25, 1.25, 12 );

	// the bullets leave 5 units to the side of the hand that holds the gun,
	// the shell flies out of the other side
	Vector vecSrc	 = m_pPlayer->GetGunPosition( );

	if ( m_pPlayer->m_bLeftHanded == true )
	{
		vecSrc = vecSrc - gpGlobals->v_right * 5;

		Vector	vecShellVelocity = m_pPlayer->pev->velocity
								 - gpGlobals->v_right * RANDOM_FLOAT(100,120)
								 + gpGlobals->v_up * RANDOM_FLOAT(35,55)
								 + gpGlobals->v_forward * RANDOM_FLOAT(40,55);
		if (m_fEjectShell)
			EjectBrass2 ( pev->origin + m_pPlayer->pev->view_ofs - gpGlobals->v_up * 8 + gpGlobals->v_forward * 15 - gpGlobals->v_right * 15, vecShellVelocity, pev->angles.y, m_iShell, TE_BOUNCE_SHELL, m_pPlayer->pev );
	}
	else
	{
		vecSrc = vecSrc + gpGlobals->v_right * 5;

		Vector	vecShellVelocity = m_pPlayer->pev->velocity
								 + gpGlobals->v_right * RANDOM_FLOAT(100,120)
								 + gpGlobals->v_up * RANDOM_FLOAT(35,55)
								 + gpGlobals->v_forward * RANDOM_FLOAT(40,55);
		if (m_fEjectShell)
			EjectBrass2 ( pev->origin + m_pPlayer->pev->view_ofs - gpGlobals->v_up * 8 + gpGlobals->v_forward * 15 + gpGlobals->v_right * 15, vecShellVelocity, pev->angles.y, m_iShell, TE_BOUNCE_SHELL, m_pPlayer->pev );
	}
	m_fEjectShell = 1 - m_fEjectShell;

	m_pPlayer->FireBullets3( vecSrc, vecAiming, Vector( flSpread, flSpread, flSpread ), 8192, 2, BULLET_PLAYER_556MM, 33, 0.96 );
}


void CAUG::Reload( void )
{
	int iResult;

	iResult = DefaultReload( AUG_MAX_CLIP, AUG_RELOAD, 3.9 );
	// a reload takes the zoom off, tested on its own as the M4A1's
	if (iResult && m_pPlayer->m_iFOV != 90)
		SecondaryAttack();

	if (iResult)
	{
		m_flTimeWeaponIdle = gpGlobals->time + RANDOM_FLOAT ( 10, 15 );
	}

	m_pPlayer->m_flAccuracy = 0;
	m_pPlayer->m_iShotsFired = 0;
	m_fDelayFire = false;
}



void CAUG::WeaponIdle( void )
{
	ResetEmptySound( );

	m_pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );

	if (m_flTimeWeaponIdle > gpGlobals->time)
		return;

	m_flTimeWeaponIdle = gpGlobals->time + 20;
	SendWeaponAnim( AUG_IDLE1 );
}

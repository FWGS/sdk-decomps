/***
*
*	Counter-Strike Beta 2.0, sg552.cpp: the SG-552 Commando rifle
*	(weapon_sg552).
*
*	A copy of the M4A1 (m4a1.cpp) as Beta 2.0 has it, unsilenced: the spread
*	grows with the shots fired, every other shot throws a shell, and the
*	secondary attack is a single zoom (to 25 degrees) that slows the cycle.
*	File, class and method names are Beta 3.1's (Linux symbols); the
*	animation names and m_fEjectShell are ours.
*
****/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"

enum sg552_e {
	SG552_IDLE1 = 0,
	SG552_RELOAD,
	SG552_DRAW,
	SG552_SHOOT1,
	SG552_SHOOT2,
	SG552_SHOOT3,
};

class CSG552 : public CBasePlayerWeapon
{
public:
	void Spawn( void );
	void Precache( void );
	int iItemSlot( void ) { return 1; }
	int GetItemInfo(ItemInfo *p);

	void PrimaryAttack( void );
	void SecondaryAttack( void );
	void SG552Fire( float flSpread, float flCycleTime, BOOL fUseAutoAim );
	BOOL Deploy( void );
	void Reload( void );
	void WeaponIdle( void );
	float GetMaxSpeed( void ) { return 240; }	// Beta 5.0 (leak): Linux symbol
	int m_iShell;
	BOOL m_fEjectShell;		// the shot throws a shell; flips with every shot
};
LINK_ENTITY_TO_CLASS( weapon_sg552, CSG552 );


void CSG552::Spawn( )
{
	pev->classname = MAKE_STRING("weapon_sg552");
	Precache( );
	m_iId = WEAPON_SG552;
	SET_MODEL(ENT(pev), "models/w_sg552.mdl");

	m_iDefaultAmmo = SG552_DEFAULT_GIVE;

	FallInit();// get ready to fall down.
}


void CSG552::Precache( void )
{
	PRECACHE_MODEL("models/v_sg552.mdl");
	PRECACHE_MODEL("models/w_sg552.mdl");
	PRECACHE_MODEL("models/p_sg552.mdl");

	PRECACHE_SOUND("weapons/sg552-1.wav");
	PRECACHE_SOUND("weapons/sg552-2.wav");
	PRECACHE_SOUND("weapons/sg552_clipout.wav");
	PRECACHE_SOUND("weapons/sg552_clipin.wav");
	PRECACHE_SOUND("weapons/sg552_boltpull.wav");

	m_iShell = PRECACHE_MODEL ("models/shell.mdl");// brass shell
}

int CSG552::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "556Nato";
	p->iMaxAmmo1 = SG552_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = SG552_MAX_CLIP;
	p->iSlot = 0;
	p->iPosition = 10;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_SG552;
	p->iWeight = SG552_WEIGHT;

	return 1;
}

BOOL CSG552::Deploy( )
{
	m_pPlayer->m_flAccuracy = 0;
	m_fEjectShell = 1;
	return DefaultDeploy( "models/v_sg552.mdl", "models/p_sg552.mdl", SG552_DRAW, "mp5" );
}

void CSG552::SecondaryAttack( void )
{
	if (m_pPlayer->m_iFOV == 90)
		m_pPlayer->m_iFOV = 25;
	else if (m_pPlayer->m_iFOV == 25)
		m_pPlayer->m_iFOV = 90;
	else
		m_pPlayer->m_iFOV = 90;

	m_flNextSecondaryAttack = gpGlobals->time + 0.3;
}

void CSG552::PrimaryAttack( void )
{
	float flSpread;

	// Beta 4.0: the same spread however the player moves (the product
	// written in the call: retail's code loads the accuracy, compares the
	// FOV and multiplies, in that order); Beta 4.1: 0.02 (was 0.0225)
	if (m_pPlayer->m_iFOV == 90)
	{
		flSpread = m_pPlayer->m_flAccuracy;
		SG552Fire( flSpread * 0.02, 0.0825, FALSE );
	}
	else
	{
		// zoomed in: a slower cycle
		flSpread = m_pPlayer->m_flAccuracy;
		SG552Fire( flSpread * 0.02, 0.135, FALSE );
	}
}

void CSG552::SG552Fire( float flSpread , float flCycleTime, BOOL fUseAutoAim )
{
	// the spread grows with the cube of the shots fired
	m_fDelayFire = true;
	m_pPlayer->m_iShotsFired++;
	m_pPlayer->m_flAccuracy = (m_pPlayer->m_iShotsFired * m_pPlayer->m_iShotsFired * m_pPlayer->m_iShotsFired / 225) + 0.3;

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
			SendWeaponAnim( SG552_SHOOT1 );
			// no breaks (sic): case 0 sends all three animations, case 1 the last two
		case 1:
			SendWeaponAnim( SG552_SHOOT2 );
		case 2:
			SendWeaponAnim( SG552_SHOOT3 );
		}
	}

	// player "shoot" animation
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

	UTIL_MakeVectors( m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle );

	Vector	vecShellVelocity = m_pPlayer->pev->velocity
							 + gpGlobals->v_right * RANDOM_FLOAT(100,120)
							 + gpGlobals->v_up * RANDOM_FLOAT(35,55)
							 + gpGlobals->v_forward * RANDOM_FLOAT(40,55);
	if (m_fEjectShell)
		EjectBrass2 ( pev->origin + m_pPlayer->pev->view_ofs - gpGlobals->v_up * 8 + gpGlobals->v_forward * 20 - gpGlobals->v_right * 10, vecShellVelocity, pev->angles.y, m_iShell, TE_BOUNCE_SHELL, m_pPlayer->pev );
	m_fEjectShell = 1 - m_fEjectShell;

	m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = BRIGHT_GUN_FLASH;

	if (RANDOM_LONG(0,1))
		EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/sg552-1.wav", RANDOM_FLOAT(0.92, 1.0), 0.56);
	else
		EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/sg552-2.wav", RANDOM_FLOAT(0.92, 1.0), 0.56);

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

	m_pPlayer->FireBullets3( vecSrc, vecAiming, Vector( flSpread, flSpread, flSpread ), 8192, 2, BULLET_PLAYER_556MM, 33, 0.955 );
	m_flNextPrimaryAttack = m_flNextSecondaryAttack = gpGlobals->time + flCycleTime;

	if (!m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);

	m_flTimeWeaponIdle = gpGlobals->time + RANDOM_FLOAT ( 10, 15 );

	// Beta 4.0: the recoil by how the player stands
	if (m_pPlayer->pev->velocity.Length2D() > 0)
		KickBack( 1.15, 0.45, 0.22, 0.18, 5, 3.5, 6 );
	else if ( !FBitSet(m_pPlayer->pev->flags, FL_ONGROUND) )
		KickBack( 1.25, 0.45, 0.22, 0.18, 5.5, 4, 5 );
	else if ( FBitSet(m_pPlayer->pev->flags, FL_DUCKING) )
		KickBack( 0.6, 0.35, 0.125, 0.0125, 4.75, 2, 10 );
	else
		KickBack( 0.625, 0.375, 0.135, 0.0125, 5, 2.25, 9 );
}


void CSG552::Reload( void )
{
	int iResult;

	iResult = DefaultReload( SG552_MAX_CLIP, SG552_RELOAD, 2.5 );
	// Beta 5.0 (leak): a reload takes the zoom off. Tested on its own, as
	// written here: with the test inside the idle block below VC6 keeps a
	// zero in eax for the three resets, retail stores them as immediates
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



void CSG552::WeaponIdle( void )
{
	ResetEmptySound( );

	m_pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );

	if (m_flTimeWeaponIdle > gpGlobals->time)
		return;

	m_flTimeWeaponIdle = gpGlobals->time + 20;
	SendWeaponAnim( SG552_IDLE1 );
}

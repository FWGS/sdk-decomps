/***
*
*	Counter-Strike Beta 2.0, m4a1.cpp: the M4A1 rifle (weapon_m4a1).
*
*	The M249's shape (SDK 1.0's glock.cpp edited, mp5.cpp's automatic fire):
*	the spread grows with the shots fired, every other shot throws a shell,
*	and the secondary attack is a single zoom. Beta 2.0 silences it (no
*	muzzle flash, no weapon flash, the "silenced_rifle" player animations).
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

enum m4a1_e {
	M4A1_IDLE1 = 0,
	M4A1_RELOAD,
	M4A1_DRAW,
	M4A1_SHOOT1,
	M4A1_SHOOT2,
	M4A1_SHOOT3,
};

class CM4A1 : public CBasePlayerWeapon
{
public:
	void Spawn( void );
	void Precache( void );
	int iItemSlot( void ) { return 1; }
	int GetItemInfo(ItemInfo *p);

	void PrimaryAttack( void );
	void SecondaryAttack( void );
	void M4A1Fire( float flSpread, float flCycleTime, BOOL fUseAutoAim );
	BOOL Deploy( void );
	void Reload( void );
	void WeaponIdle( void );
	float GetMaxSpeed( void ) { if ( m_pPlayer->m_iFOV != 90 ) return 200; return 230; }	// Beta 5.0 (leak): Linux symbol; Beta 6.0: 230 (was 240)
	int m_iShell;
	BOOL m_fEjectShell;		// the shot throws a shell; flips with every shot
};
LINK_ENTITY_TO_CLASS( weapon_m4a1, CM4A1 );


void CM4A1::Spawn( )
{
	pev->classname = MAKE_STRING("weapon_m4a1");
	Precache( );
	m_iId = WEAPON_M4A1;
	SET_MODEL(ENT(pev), "models/w_m4a1.mdl");

	m_iDefaultAmmo = M4A1_DEFAULT_GIVE;

	FallInit();// get ready to fall down.
}


void CM4A1::Precache( void )
{
	PRECACHE_MODEL("models/v_m4a1.mdl");
	PRECACHE_MODEL("models/v_m4a1_r.mdl");	// Beta 6.0: the right-handed view model
	PRECACHE_MODEL("models/w_m4a1.mdl");
	PRECACHE_MODEL("models/p_m4a1.mdl");

	PRECACHE_SOUND("weapons/m4a1-1.wav");
	PRECACHE_SOUND("weapons/m4a1-2.wav");

	m_iShell = PRECACHE_MODEL ("models/rshell.mdl");// brass shell (Beta 6.0: the rifle shell)
}

int CM4A1::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "556Nato";
	p->iMaxAmmo1 = M4A1_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = M4A1_MAX_CLIP;
	p->iSlot = 0;
	p->iPosition = 6;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_M4A1;
	p->iWeight = M4A1_WEIGHT;

	return 1;
}

BOOL CM4A1::Deploy( )
{
	m_pPlayer->m_flAccuracy = 0;
	m_fDelayFire = true;
	m_fEjectShell = 1;

	// Beta 6.0: v_m4a1.mdl holds the gun in the left hand, v_m4a1_r.mdl in the right
	if ( m_pPlayer->m_bLeftHanded == true )
		return DefaultDeploy( "models/v_m4a1.mdl", "models/p_m4a1.mdl", M4A1_DRAW, "silenced_rifle" );

	return DefaultDeploy( "models/v_m4a1_r.mdl", "models/p_m4a1.mdl", M4A1_DRAW, "silenced_rifle" );
}

void CM4A1::SecondaryAttack( void )
{
	if (m_pPlayer->m_iFOV == 90)
		m_pPlayer->m_iFOV = 45;
	else if (m_pPlayer->m_iFOV == 45)
		m_pPlayer->m_iFOV = 90;
	else
		m_pPlayer->m_iFOV = 90;

	m_flNextSecondaryAttack = gpGlobals->time + 0.3;
}

void CM4A1::PrimaryAttack( void )
{
	// Beta 6.0: off the ground the spread is 0.25 times the accuracy; no else
	// (sic): the gun then fires a second shot as below
	if ( !FBitSet( m_pPlayer->pev->flags, FL_ONGROUND ) )
		M4A1Fire( m_pPlayer->m_flAccuracy * 0.25, 0.0875, FALSE );

	if (m_pPlayer->m_iFOV == 90)
		M4A1Fire( m_pPlayer->m_flAccuracy * 0.02, 0.0875, FALSE );
	else
		M4A1Fire( m_pPlayer->m_flAccuracy * 0.02, 0.165, FALSE );	// zoomed in: a slower cycle
}

void CM4A1::M4A1Fire( float flSpread , float flCycleTime, BOOL fUseAutoAim )
{
	// the spread grows with the cube of the shots fired
	m_fDelayFire = true;
	m_pPlayer->m_iShotsFired++;
	m_pPlayer->m_flAccuracy = (m_pPlayer->m_iShotsFired * m_pPlayer->m_iShotsFired * m_pPlayer->m_iShotsFired / 250) + 0.3;

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

	// no muzzle flash: the M4A1 is silenced
	if (m_iClip != 0)
	{
		switch (RANDOM_LONG(0,2))
		{
		case 0:
			SendWeaponAnim( M4A1_SHOOT1 );
			break;	// Beta 5.2: the breaks (Beta 5.0 sent all three for case 0)
		case 1:
			SendWeaponAnim( M4A1_SHOOT2 );
			break;
		case 2:
			SendWeaponAnim( M4A1_SHOOT3 );
			break;
		}
	}

	// player "shoot" animation
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

	UTIL_MakeVectors( m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle );

	m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;

	if (RANDOM_LONG(0,1))
		EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/m4a1-1.wav", RANDOM_FLOAT(0.92, 1.0), 1.28);
	else
		EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/m4a1-2.wav", RANDOM_FLOAT(0.92, 1.0), 1.28);

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

	// Beta 4.0: the recoil by how the player stands
	if (m_pPlayer->pev->velocity.Length2D() > 0)
		KickBack( 1, 0.4, 0.23, 0.15, 5, 3, 7 );
	else if ( !FBitSet(m_pPlayer->pev->flags, FL_ONGROUND) )
		KickBack( 1.2, 0.45, 0.23, 0.15, 5.5, 3.5, 6 );
	else if ( FBitSet(m_pPlayer->pev->flags, FL_DUCKING) )
		KickBack( 0.55, 0.3, 0.2, 0.0125, 4.5, 1.5, 10 );
	else
		KickBack( 0.6, 0.35, 0.25, 0.015, 4.5, 1.5, 10 );

	// Beta 6.1: the shot leaves from the eye again ("off center shooting
	// fixed"; Beta 6.0: 5 units to the gun hand's side); the shell still
	// flies out of the other side
	Vector vecSrc	 = m_pPlayer->GetGunPosition( );

	if ( m_pPlayer->m_bLeftHanded == true )
	{
		Vector	vecShellVelocity = m_pPlayer->pev->velocity
								 + gpGlobals->v_right * RANDOM_FLOAT(100,120)
								 + gpGlobals->v_up * RANDOM_FLOAT(35,55)
								 + gpGlobals->v_forward * RANDOM_FLOAT(40,55);
		if (m_fEjectShell)
			EjectBrass2 ( pev->origin + m_pPlayer->pev->view_ofs - gpGlobals->v_up * 8 + gpGlobals->v_forward * 20 - gpGlobals->v_right * 10, vecShellVelocity, pev->angles.y, m_iShell, TE_BOUNCE_SHELL, m_pPlayer->pev );
	}
	else
	{
		Vector	vecShellVelocity = m_pPlayer->pev->velocity
								 - gpGlobals->v_right * RANDOM_FLOAT(100,120)
								 + gpGlobals->v_up * RANDOM_FLOAT(35,55)
								 + gpGlobals->v_forward * RANDOM_FLOAT(40,55);
		if (m_fEjectShell)
			EjectBrass2 ( pev->origin + m_pPlayer->pev->view_ofs - gpGlobals->v_up * 8 + gpGlobals->v_forward * 20 + gpGlobals->v_right * 10, vecShellVelocity, pev->angles.y, m_iShell, TE_BOUNCE_SHELL, m_pPlayer->pev );
	}
	m_fEjectShell = 1 - m_fEjectShell;

	m_pPlayer->FireBullets3( vecSrc, vecAiming, Vector( flSpread, flSpread, flSpread ), 8192, 2, BULLET_PLAYER_556MM, 32, 0.97 );
}


void CM4A1::Reload( void )
{
	int iResult;

	iResult = DefaultReload( M4A1_MAX_CLIP, M4A1_RELOAD, 2.2 );
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



void CM4A1::WeaponIdle( void )
{
	ResetEmptySound( );

	m_pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );

	if (m_flTimeWeaponIdle > gpGlobals->time)
		return;

	m_flTimeWeaponIdle = gpGlobals->time + 20;
	SendWeaponAnim( M4A1_IDLE1 );
}

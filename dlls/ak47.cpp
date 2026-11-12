/***
*
*	Counter-Strike Beta 2.0, ak47.cpp: the AK-47 rifle (weapon_ak47).
*
*	A copy of Beta 1.1's M4A1 (m4a1.cpp) with the M4A1's new spread table:
*	the spread grows with the shots fired, every other shot throws a shell.
*	The secondary attack fires as the primary does (there is no zoom).
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

enum ak47_e {
	AK47_IDLE1 = 0,
	AK47_RELOAD,
	AK47_DRAW,
	AK47_SHOOT1,
	AK47_SHOOT2,
	AK47_SHOOT3,
};

class CAK47 : public CBasePlayerWeapon
{
public:
	void Spawn( void );
	void Precache( void );
	int iItemSlot( void ) { return 1; }
	int GetItemInfo(ItemInfo *p);

	void PrimaryAttack( void );
	void SecondaryAttack( void );
	void AK47Fire( float flSpread, float flCycleTime, BOOL fUseAutoAim );
	BOOL Deploy( void );
	void Reload( void );
	void WeaponIdle( void );
	float GetMaxSpeed( void ) { return 221; }	// Beta 5.0 (leak): Linux symbol; Beta 6.0: 221 (was 230)
	int m_iShell;
	BOOL m_fEjectShell;		// the shot throws a shell; flips with every shot
};
LINK_ENTITY_TO_CLASS( weapon_ak47, CAK47 );


void CAK47::Spawn( )
{
	pev->classname = MAKE_STRING("weapon_ak47");
	Precache( );
	m_iId = WEAPON_AK47;
	SET_MODEL(ENT(pev), "models/w_ak47.mdl");

	m_iDefaultAmmo = AK47_DEFAULT_GIVE;

	FallInit();// get ready to fall down.
}


void CAK47::Precache( void )
{
	PRECACHE_MODEL("models/v_ak47.mdl");
	PRECACHE_MODEL("models/v_ak47_r.mdl");	// Beta 6.0: the right-handed view model
	PRECACHE_MODEL("models/w_ak47.mdl");
	PRECACHE_MODEL("models/p_ak47.mdl");

	PRECACHE_SOUND("weapons/ak47-1.wav");
	PRECACHE_SOUND("weapons/ak47-2.wav");
	PRECACHE_SOUND("weapons/ak47_clipout.wav");
	PRECACHE_SOUND("weapons/ak47_clipin.wav");
	PRECACHE_SOUND("weapons/ak47_boltpull.wav");

	m_iShell = PRECACHE_MODEL ("models/rshell.mdl");// brass shell (Beta 6.0: the rifle shell)
}

int CAK47::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "762Nato";
	p->iMaxAmmo1 = AK47_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = AK47_MAX_CLIP;
	p->iSlot = 0;
	p->iPosition = 1;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_AK47;
	p->iWeight = AK47_WEIGHT;

	return 1;
}

BOOL CAK47::Deploy( )
{
	m_pPlayer->m_flAccuracy = 0;
	m_fEjectShell = 1;

	// Beta 6.0: v_ak47.mdl holds the gun in the left hand, v_ak47_r.mdl in the right
	if ( m_pPlayer->m_bLeftHanded == true )
		return DefaultDeploy( "models/v_ak47.mdl", "models/p_ak47.mdl", AK47_DRAW, "mp5" );

	return DefaultDeploy( "models/v_ak47_r.mdl", "models/p_ak47.mdl", AK47_DRAW, "mp5" );
}

void CAK47::SecondaryAttack( void )
{
	// Beta 5.0 (leak): nothing (Beta 4.1 fired as PrimaryAttack)
}

void CAK47::PrimaryAttack( void )
{
	// Beta 6.0: off the ground the spread is 0.35 times the accuracy
	if ( !FBitSet( m_pPlayer->pev->flags, FL_ONGROUND ) )
		AK47Fire( m_pPlayer->m_flAccuracy * 0.35, 0.0955, FALSE );
	else
		AK47Fire( m_pPlayer->m_flAccuracy * 0.0275, 0.0955, FALSE );
}

void CAK47::AK47Fire( float flSpread , float flCycleTime, BOOL fUseAutoAim )
{
	m_fDelayFire = true;

	// the spread grows with the cube of the shots fired
	m_pPlayer->m_iShotsFired++;
	m_pPlayer->m_flAccuracy = (m_pPlayer->m_iShotsFired * m_pPlayer->m_iShotsFired * m_pPlayer->m_iShotsFired / 200) + 0.35;

	if (m_pPlayer->m_flAccuracy > 0.65)
		m_pPlayer->m_flAccuracy = 0.65;

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
			SendWeaponAnim( AK47_SHOOT1 );
			break;	// Beta 5.2: the breaks (Beta 5.0 sent all three for case 0)
		case 1:
			SendWeaponAnim( AK47_SHOOT2 );
			break;
		case 2:
			SendWeaponAnim( AK47_SHOOT3 );
			break;
		}
	}

	// player "shoot" animation
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

	UTIL_MakeVectors( m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle );

	m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = BRIGHT_GUN_FLASH;

	if (RANDOM_LONG(0,1))
		EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/ak47-1.wav", 1, 0.4);
	else
		EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/ak47-2.wav", 1, 0.4);

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
		KickBack( 1.5, 0.45, 0.25, 0.185, 7, 4, 6 );
	else if ( !FBitSet(m_pPlayer->pev->flags, FL_ONGROUND) )
		KickBack( 2, 1, 0.5, 0.35, 9, 6, 5 );
	else if ( FBitSet(m_pPlayer->pev->flags, FL_DUCKING) )
		KickBack( 0.9, 0.35, 0.15, 0.025, 5.5, 1.5, 9 );
	else
		KickBack( 1, 0.375, 0.175, 0.0375, 5.75, 1.75, 8 );

	// Beta 6.0: the bullets leave 5 units to the side of the hand that holds
	// the gun, the shell flies out of the other side
	Vector vecSrc	 = m_pPlayer->GetGunPosition( );

	if ( m_pPlayer->m_bLeftHanded == true )
	{
		vecSrc = vecSrc - gpGlobals->v_right * 5;

		Vector	vecShellVelocity = m_pPlayer->pev->velocity
								 + gpGlobals->v_right * RANDOM_FLOAT(100,120)
								 + gpGlobals->v_up * RANDOM_FLOAT(35,55)
								 + gpGlobals->v_forward * RANDOM_FLOAT(40,55);
		if (m_fEjectShell)
			EjectBrass2 ( pev->origin + m_pPlayer->pev->view_ofs - gpGlobals->v_up * 8 + gpGlobals->v_forward * 20 - gpGlobals->v_right * 10, vecShellVelocity, pev->angles.y, m_iShell, TE_BOUNCE_SHELL, m_pPlayer->pev );
	}
	else
	{
		vecSrc = vecSrc + gpGlobals->v_right * 5;

		Vector	vecShellVelocity = m_pPlayer->pev->velocity
								 - gpGlobals->v_right * RANDOM_FLOAT(100,120)
								 + gpGlobals->v_up * RANDOM_FLOAT(35,55)
								 + gpGlobals->v_forward * RANDOM_FLOAT(40,55);
		if (m_fEjectShell)
			EjectBrass2 ( pev->origin + m_pPlayer->pev->view_ofs - gpGlobals->v_up * 8 + gpGlobals->v_forward * 20 + gpGlobals->v_right * 10, vecShellVelocity, pev->angles.y, m_iShell, TE_BOUNCE_SHELL, m_pPlayer->pev );
	}
	m_fEjectShell = 1 - m_fEjectShell;

	m_pPlayer->FireBullets3( vecSrc, vecAiming, Vector( flSpread, flSpread, flSpread ), 8192, 2, BULLET_PLAYER_762MM, 37, 0.98 );
}


void CAK47::Reload( void )
{
	int iResult;

	iResult = DefaultReload( AK47_MAX_CLIP, AK47_RELOAD, 2.75 );
	if (iResult)
	{
		m_flTimeWeaponIdle = gpGlobals->time + RANDOM_FLOAT ( 10, 15 );
	}

	m_pPlayer->m_flAccuracy = 0;
	m_pPlayer->m_iShotsFired = 0;
	m_fDelayFire = false;
}



void CAK47::WeaponIdle( void )
{
	ResetEmptySound( );

	m_pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );

	if (m_flTimeWeaponIdle > gpGlobals->time)
		return;

	m_flTimeWeaponIdle = gpGlobals->time + 20;
	SendWeaponAnim( AK47_IDLE1 );
}

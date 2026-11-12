/***
*
*	Counter-Strike Beta 3.0, p90.cpp: the FN P90 submachine gun (weapon_p90).
*
*	m249.cpp's text changed: 5.7mm ammunition, a clip of 50, the spread
*	growing with the square of the shots fired, one sound, a dim flash.
*	File, class and method names are Beta 3.1's (Linux symbols); the
*	animation names, m_fEjectShell and the P90_ macros are ours.
*
****/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"

enum p90_e {
	P90_IDLE1 = 0,
	P90_RELOAD,
	P90_DRAW,
	P90_SHOOT1,
	P90_SHOOT2,
	P90_SHOOT3,
};

class CP90 : public CBasePlayerWeapon
{
public:
	void Spawn( void );
	void Precache( void );
	int iItemSlot( void ) { return 1; }
	int GetItemInfo(ItemInfo *p);
	float GetMaxSpeed( void ) { return 245; }	// Beta 5.0 (leak): Linux symbol; Beta 6.0: 245 (was 260)

	void PrimaryAttack( void );
	void P90Fire( float flSpread, float flCycleTime, BOOL fUseAutoAim );
	BOOL Deploy( void );
	void Reload( void );
	void WeaponIdle( void );
	int m_iShell;
	BOOL m_fEjectShell;		// the shot throws a shell; flips with every shot
};
LINK_ENTITY_TO_CLASS( weapon_p90, CP90 );


void CP90::Spawn( )
{
	pev->classname = MAKE_STRING("weapon_p90");
	Precache( );
	m_iId = WEAPON_P90;
	SET_MODEL(ENT(pev), "models/w_p90.mdl");

	m_iDefaultAmmo = P90_DEFAULT_GIVE;

	FallInit();// get ready to fall down.
}


void CP90::Precache( void )
{
	PRECACHE_MODEL("models/v_p90.mdl");
	PRECACHE_MODEL("models/v_p90_r.mdl");	// Beta 6.0: the right-handed view model
	PRECACHE_MODEL("models/w_p90.mdl");
	PRECACHE_MODEL("models/p_p90.mdl");

	PRECACHE_SOUND("weapons/p90-1.wav");
	PRECACHE_SOUND("weapons/p90_clipout.wav");
	PRECACHE_SOUND("weapons/p90_clipin.wav");
	PRECACHE_SOUND("weapons/p90_boltpull.wav");
	PRECACHE_SOUND("weapons/p90_cliprelease.wav");

	m_iShell = PRECACHE_MODEL ("models/rshell.mdl");// brass shell (Beta 6.0: the rifle shell)
}

int CP90::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "57mm";
	p->iMaxAmmo1 = P90_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = P90_MAX_CLIP;
	p->iSlot = 0;
	p->iPosition = 8;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_P90;
	p->iWeight = P90_WEIGHT;

	return 1;
}

BOOL CP90::Deploy( )
{
	m_pPlayer->m_flAccuracy = 0;
	m_fEjectShell = 1;
	m_fDelayFire = false;

	// Beta 6.0: the left-handed model is the old one, the right-handed one is new
	if ( m_pPlayer->m_bLeftHanded == true )
		return DefaultDeploy( "models/v_p90.mdl", "models/p_p90.mdl", P90_DRAW, "p90" );

	return DefaultDeploy( "models/v_p90_r.mdl", "models/p_p90.mdl", P90_DRAW, "p90" );
}

void CP90::PrimaryAttack( void )
{
	// Beta 6.0: more spread in the air (0.4) and when running (0.2).
	// Beta 6.1: less when running (0.15) and standing (0.045, 6.0: 0.06)
	if ( !FBitSet( m_pPlayer->pev->flags, FL_ONGROUND ) )
		P90Fire( m_pPlayer->m_flAccuracy * 0.4, 0.066, FALSE );
	else if ( m_pPlayer->pev->velocity.Length2D() > 170 )
		P90Fire( m_pPlayer->m_flAccuracy * 0.15, 0.066, FALSE );
	else
		P90Fire( m_pPlayer->m_flAccuracy * 0.045, 0.066, FALSE );
}

void CP90::P90Fire( float flSpread , float flCycleTime, BOOL fUseAutoAim )
{
	// the spread grows with the square of the shots fired
	m_pPlayer->m_iShotsFired++;
	m_pPlayer->m_flAccuracy = (m_pPlayer->m_iShotsFired * m_pPlayer->m_iShotsFired * m_pPlayer->m_iShotsFired / 175) + 0.45;

	if (m_pPlayer->m_flAccuracy > 1)
		m_pPlayer->m_flAccuracy = 1;

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
		switch (RANDOM_LONG(0,1))
		{
		case 0:
			SendWeaponAnim( P90_SHOOT1 );
			break;	// Beta 5.2: the breaks (Beta 5.0 sent both for case 0)
		case 1:
			SendWeaponAnim( P90_SHOOT2 );
			break;
		}
	}

	// player "shoot" animation
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

	UTIL_MakeVectors( m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle );

	m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = DIM_GUN_FLASH;

	EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/p90-1.wav", RANDOM_FLOAT(0.92, 1.0), ATTN_NORM);

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
		KickBack( 0.65, 0.3, 0.2, 0.0275, 4, 2.25, 4 );
	else if ( !FBitSet(m_pPlayer->pev->flags, FL_ONGROUND) )
		KickBack( 0.65, 0.35, 0.25, 0.03, 4.25, 2.5, 3 );
	else if ( FBitSet(m_pPlayer->pev->flags, FL_DUCKING) )
		KickBack( 0.575, 0.25, 0.125, 0.02, 3.5, 1, 8 );	// Beta 6.1 (Beta 6.0: 0.6)
	else
		KickBack( 0.6, 0.275, 0.125, 0.02, 3.75, 1.25, 6 );	// Beta 6.1 (Beta 6.0: 0.625)

	// Beta 6.1: the shot leaves from the eye again ("off center shooting
	// fixed"; Beta 6.0: 5 units to the gun hand's side); the shell still
	// flies out of the other side
	Vector vecSrc	 = m_pPlayer->GetGunPosition( );

	if ( m_pPlayer->m_bLeftHanded == true )
	{
		Vector	vecShellVelocity = m_pPlayer->pev->velocity
								 - gpGlobals->v_right * RANDOM_FLOAT(100,120)
								 + gpGlobals->v_up * RANDOM_FLOAT(50,90)
								 + gpGlobals->v_forward * 25;
		if (m_fEjectShell)
			EjectBrass2 ( pev->origin + m_pPlayer->pev->view_ofs + gpGlobals->v_up * -14 + gpGlobals->v_forward * 35 - gpGlobals->v_right * 22, vecShellVelocity, pev->angles.y, m_iShell, TE_BOUNCE_SHELL, m_pPlayer->pev );

		m_fEjectShell = 1 - m_fEjectShell;
	}
	else
	{
		Vector	vecShellVelocity = m_pPlayer->pev->velocity
								 + gpGlobals->v_right * RANDOM_FLOAT(100,120)
								 + gpGlobals->v_up * RANDOM_FLOAT(50,90)
								 + gpGlobals->v_forward * 25;
		if (m_fEjectShell)
			EjectBrass2 ( pev->origin + m_pPlayer->pev->view_ofs + gpGlobals->v_up * -14 + gpGlobals->v_forward * 35 + gpGlobals->v_right * 22, vecShellVelocity, pev->angles.y, m_iShell, TE_BOUNCE_SHELL, m_pPlayer->pev );

		m_fEjectShell = 1 - m_fEjectShell;
	}

	// Beta 6.0: 20 damage (was 23), range modifier 0.9 (0.925)
	m_pPlayer->FireBullets3( vecSrc, vecAiming, Vector( flSpread, flSpread, flSpread ), 8192, 1, BULLET_PLAYER_57MM, 22, 0.9 );	// Beta 6.1: 22 damage (Beta 6.0: 20)
}


void CP90::Reload( void )
{
	if (DefaultReload( P90_MAX_CLIP, P90_RELOAD, 3.5 ))
	{
		m_flTimeWeaponIdle = gpGlobals->time + RANDOM_FLOAT ( 10, 15 );
	}

	m_pPlayer->m_flAccuracy = 0;
}



void CP90::WeaponIdle( void )
{
	ResetEmptySound( );

	m_pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );

	if (m_flTimeWeaponIdle > gpGlobals->time)
		return;

	m_flTimeWeaponIdle = gpGlobals->time + 20;
	SendWeaponAnim( P90_IDLE1 );
}

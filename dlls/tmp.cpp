/***
*
*	Counter-Strike Beta 2.0, tmp.cpp: the TMP submachine gun (weapon_tmp).
*
*	The MP5 Navy's shape, silenced: no muzzle flash, no gun flash, the shot
*	at volume 0.6. File, class and method names are Beta 3.1's (Linux
*	symbols); the animation names and m_fEjectShell are ours.
*
****/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"

enum tmp_e {
	TMP_IDLE1 = 0,
	TMP_RELOAD,
	TMP_DRAW,
	TMP_SHOOT1,
	TMP_SHOOT2,
	TMP_SHOOT3,
};

class CTMP : public CBasePlayerWeapon
{
public:
	void Spawn( void );
	void Precache( void );
	int iItemSlot( void ) { return 1; }
	int GetItemInfo(ItemInfo *p);
	float GetMaxSpeed( void ) { return 250; }	// Beta 5.0 (leak): Linux symbol; Beta 6.0: 250 (was 260)

	void PrimaryAttack( void );
	void TMPFire( float flSpread, float flCycleTime, BOOL fUseAutoAim );
	BOOL Deploy( void );
	void Reload( void );
	void WeaponIdle( void );
	int m_iShell;
	BOOL m_fEjectShell;		// the shot throws a shell; flips with every shot
};
LINK_ENTITY_TO_CLASS( weapon_tmp, CTMP );


void CTMP::Spawn( )
{
	pev->classname = MAKE_STRING("weapon_tmp");
	Precache( );
	m_iId = WEAPON_TMP;
	SET_MODEL(ENT(pev), "models/w_tmp.mdl");

	m_iDefaultAmmo = TMP_DEFAULT_GIVE;

	FallInit();// get ready to fall down.
}


void CTMP::Precache( void )
{
	PRECACHE_MODEL("models/v_tmp.mdl");
	PRECACHE_MODEL("models/v_tmp_r.mdl");	// Beta 6.0: the right-handed view model
	PRECACHE_MODEL("models/w_tmp.mdl");
	PRECACHE_MODEL("models/p_tmp.mdl");

	PRECACHE_SOUND("weapons/tmp-1.wav");
	PRECACHE_SOUND("weapons/tmp-2.wav");

	m_iShell = PRECACHE_MODEL ("models/pshell.mdl");// brass shell (Beta 6.0: the pistol shell)
}

int CTMP::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "9mm";
	p->iMaxAmmo1 = TMP_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = TMP_MAX_CLIP;
	p->iSlot = 0;
	p->iPosition = 11;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_TMP;
	p->iWeight = TMP_WEIGHT;

	return 1;
}

BOOL CTMP::Deploy( )
{
	m_pPlayer->m_flAccuracy = 0.95;
	m_fEjectShell = 1;
	m_fDelayFire = false;

	// Beta 6.0: the left-handed model is the old one, the right-handed one is new
	if ( m_pPlayer->m_bLeftHanded == true )
		return DefaultDeploy( "models/v_tmp.mdl", "models/p_tmp.mdl", TMP_DRAW, "tmp" );

	return DefaultDeploy( "models/v_tmp_r.mdl", "models/p_tmp.mdl", TMP_DRAW, "tmp" );
}

void CTMP::PrimaryAttack( void )
{
	// Beta 6.0: ten times the spread in the air
	if ( !FBitSet( m_pPlayer->pev->flags, FL_ONGROUND ) )
		TMPFire( m_pPlayer->m_flAccuracy * 0.3, 0.07, FALSE );
	else
		TMPFire( m_pPlayer->m_flAccuracy * 0.03, 0.07, FALSE );
}

void CTMP::TMPFire( float flSpread , float flCycleTime, BOOL fUseAutoAim )
{
	// the spread grows with the cube of the shots fired
	m_pPlayer->m_iShotsFired++;
	m_pPlayer->m_flAccuracy = (m_pPlayer->m_iShotsFired * m_pPlayer->m_iShotsFired * m_pPlayer->m_iShotsFired / 200) + 0.55;

	if (m_pPlayer->m_flAccuracy > 1.4)
		m_pPlayer->m_flAccuracy = 1.4;

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

	if (m_iClip != 0)
	{
		switch (RANDOM_LONG(0,2))
		{
		case 0:
			SendWeaponAnim( TMP_SHOOT1 );
			break;	// Beta 5.2: the breaks (Beta 5.0 sent all three for case 0)
		case 1:
			SendWeaponAnim( TMP_SHOOT2 );
			break;
		case 2:
			SendWeaponAnim( TMP_SHOOT3 );
			break;
		}
	}

	// player "shoot" animation
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

	UTIL_MakeVectors( m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle );

	m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;

	if (RANDOM_LONG(0,1))
		EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/tmp-1.wav", 0.6, 1.6);
	else
		EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/tmp-2.wav", 0.6, 1.6);

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
		KickBack( 0.8, 0.4, 0.2, 0.03, 3, 2.5, 7 );
	else if ( !FBitSet(m_pPlayer->pev->flags, FL_ONGROUND) )
		KickBack( 1, 0.4, 0.25, 0.035, 3.5, 2.5, 7 );
	else if ( FBitSet(m_pPlayer->pev->flags, FL_DUCKING) )
		KickBack( 0.7, 0.35, 0.125, 0.025, 2.5, 2, 10 );
	else
		KickBack( 0.725, 0.375, 0.15, 0.025, 2.75, 2.25, 9 );

	// Beta 6.1: the shot leaves from the eye again ("off center shooting
	// fixed"; Beta 6.0: 5 units to the gun hand's side); the shell still
	// flies out of the other side
	Vector vecSrc	 = m_pPlayer->GetGunPosition( );

	if ( m_pPlayer->m_bLeftHanded == true )
	{
		Vector	vecShellVelocity = m_pPlayer->pev->velocity
								 + gpGlobals->v_right * RANDOM_FLOAT(100,120)
								 - gpGlobals->v_up * RANDOM_FLOAT(5,55)
								 + gpGlobals->v_forward * 25;
		if (m_fEjectShell)
			EjectBrass2 ( pev->origin + m_pPlayer->pev->view_ofs + gpGlobals->v_up * -6 + gpGlobals->v_forward * 32 - gpGlobals->v_right * 11, vecShellVelocity, pev->angles.y, m_iShell, TE_BOUNCE_SHELL, m_pPlayer->pev );

		m_fEjectShell = 1 - m_fEjectShell;
	}
	else
	{
		Vector	vecShellVelocity = m_pPlayer->pev->velocity
								 - gpGlobals->v_right * RANDOM_FLOAT(100,120)
								 - gpGlobals->v_up * RANDOM_FLOAT(5,55)
								 + gpGlobals->v_forward * 25;
		if (m_fEjectShell)
			EjectBrass2 ( pev->origin + m_pPlayer->pev->view_ofs + gpGlobals->v_up * -6 + gpGlobals->v_forward * 32 + gpGlobals->v_right * 11, vecShellVelocity, pev->angles.y, m_iShell, TE_BOUNCE_SHELL, m_pPlayer->pev );

		m_fEjectShell = 1 - m_fEjectShell;
	}

	m_pPlayer->FireBullets3( vecSrc, vecAiming, Vector( flSpread, flSpread, flSpread ), 8192, 1, BULLET_PLAYER_9MM, 20, 0.85 );
}


void CTMP::Reload( void )
{
	if (DefaultReload( TMP_MAX_CLIP, TMP_RELOAD, 2.0 ))
	{
		m_flTimeWeaponIdle = gpGlobals->time + RANDOM_FLOAT ( 10, 15 );
	}

	m_pPlayer->m_flAccuracy = 0;
}



void CTMP::WeaponIdle( void )
{
	ResetEmptySound( );

	m_pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );

	if (m_flTimeWeaponIdle > gpGlobals->time)
		return;

	m_flTimeWeaponIdle = gpGlobals->time + 20;
	SendWeaponAnim( TMP_IDLE1 );
}

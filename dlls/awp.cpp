/***
*
*	Counter-Strike Beta 2.0, awp.cpp: the AWP sniper rifle (weapon_awp).
*
*	SDK 1.0's glock.cpp edited: the same class shape and fire function,
*	FireBullets3 instead of FireBullets, two zoom levels on the secondary
*	attack. File, class and method names are Beta 3.1's (Linux symbols);
*	the animation names are ours.
*
****/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"

enum awp_e {
	AWP_IDLE = 0,
	AWP_SHOOT1,
	AWP_SHOOT2,
	AWP_RELOAD,
	AWP_DRAW,
};

class CAWP : public CBasePlayerWeapon
{
public:
	void Spawn( void );
	void Precache( void );
	int iItemSlot( void ) { return 1; }
	int GetItemInfo(ItemInfo *p);

	void PrimaryAttack( void );
	void SecondaryAttack( void );
	void AWPFire( float flSpread, float flCycleTime, BOOL fUseAutoAim );
	BOOL Deploy( void );
	void Reload( void );
	void WeaponIdle( void );
	float GetMaxSpeed( void ) { if ( m_pPlayer->m_iFOV != 90 ) return 150; return 210; }	// Beta 5.0 (leak): Linux symbol; slower zoomed in (Beta 6.0: 150 and 210, were 160 and 200)
	int m_iShell;
};
LINK_ENTITY_TO_CLASS( weapon_awp, CAWP );


void CAWP::Spawn( )
{
	pev->classname = MAKE_STRING("weapon_awp");
	Precache( );
	m_iId = WEAPON_AWP;
	SET_MODEL(ENT(pev), "models/w_awp.mdl");

	m_iDefaultAmmo = AWP_DEFAULT_GIVE;

	FallInit();// get ready to fall down.
}


void CAWP::Precache( void )
{
	PRECACHE_MODEL("models/v_awp.mdl");
	PRECACHE_MODEL("models/v_awp_r.mdl");	// Beta 6.0: the right-handed view model
	PRECACHE_MODEL("models/w_awp.mdl");
	PRECACHE_MODEL("models/p_awp.mdl");

	PRECACHE_SOUND("weapons/awp1.wav");
	PRECACHE_SOUND("weapons/boltpull1.wav");
	PRECACHE_SOUND("weapons/boltup.wav");
	PRECACHE_SOUND("weapons/boltdown.wav");
	PRECACHE_SOUND("weapons/zoom.wav");
	PRECACHE_SOUND("weapons/awp_deploy.wav");
	PRECACHE_SOUND("weapons/awp_clipin.wav");
	PRECACHE_SOUND("weapons/awp_clipout.wav");

	m_iShellLate = m_iShell = PRECACHE_MODEL ("models/rshell.mdl");// brass shell (Beta 6.0: the rifle shell)
}

int CAWP::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "338Magnum";
	p->iMaxAmmo1 = AWP_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = AWP_MAX_CLIP;
	p->iSlot = 0;
	p->iPosition = 2;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_AWP;
	p->iWeight = AWP_WEIGHT;

	return 1;
}

BOOL CAWP::Deploy( )
{
	// Beta 6.0: v_awp.mdl holds the gun in the left hand, v_awp_r.mdl in the right
	if ( m_pPlayer->m_bLeftHanded == true )
		return DefaultDeploy( "models/v_awp.mdl", "models/p_awp.mdl", AWP_DRAW, "sniper" );

	return DefaultDeploy( "models/v_awp_r.mdl", "models/p_awp.mdl", AWP_DRAW, "sniper" );
}

void CAWP::SecondaryAttack( void )
{
	m_pPlayer->pev->viewmodel = 0;

	// Beta 4.0: slower while zoomed in
	// Beta 6.0: the first zoom step is 40 (was 45)
	if (m_pPlayer->m_iFOV == 90)
	{
		m_pPlayer->m_iFOV = 40;
	}
	else if (m_pPlayer->m_iFOV == 40)
	{
		m_pPlayer->m_iFOV = 10;
	}
	else if (m_pPlayer->m_iFOV == 10)
	{
		// Beta 6.0: the view model of the player's hand
		if ( m_pPlayer->m_bLeftHanded == true )
			m_pPlayer->pev->viewmodel = MAKE_STRING("models/v_awp.mdl");
		else
			m_pPlayer->pev->viewmodel = MAKE_STRING("models/v_awp_r.mdl");

		m_pPlayer->m_iFOV = 90;
	}

	m_pPlayer->ResetMaxSpeed();	// Beta 5.0 (leak): GetMaxSpeed decides
	EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/zoom.wav", 1, ATTN_NORM);
	m_flNextSecondaryAttack = gpGlobals->time + 0.3;
}

void CAWP::PrimaryAttack( void )
{
	// Beta 6.0: off the ground 0.85, running 0.65 (were 0.2 and 0.15; the
	// ground is tested first now)
	if ( !FBitSet(m_pPlayer->pev->flags, FL_ONGROUND) )
		AWPFire( 0.85, 1.9, FALSE );
	else if (m_pPlayer->pev->velocity.Length2D() > 170)
		AWPFire( 0.65, 1.9, FALSE );
	else if (m_pPlayer->pev->flags & FL_DUCKING)
		AWPFire( 0, 1.9, FALSE );
	else
		AWPFire( 0.007, 1.9, FALSE );
}

void CAWP::AWPFire( float flSpread , float flCycleTime, BOOL fUseAutoAim )
{
	// the shot takes the scope off; Reload and ItemPostFrame put it back
	if (m_pPlayer->m_iFOV != 90)
	{
		// Beta 6.0: the view model of the player's hand
		if ( m_pPlayer->m_bLeftHanded == true )
			m_pPlayer->pev->viewmodel = MAKE_STRING("models/v_awp.mdl");
		else
			m_pPlayer->pev->viewmodel = MAKE_STRING("models/v_awp_r.mdl");

		m_pPlayer->m_fResumeZoom = TRUE;
		m_pPlayer->m_iLastZoom = m_pPlayer->m_iFOV;
		m_pPlayer->m_iFOV = 90;
	}
	else
		flSpread += 0.025;	// Beta 3.1: unscoped, the spread grows

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

	if (RANDOM_LONG(0,1))
		SendWeaponAnim( AWP_SHOOT1 );
	else
		SendWeaponAnim( AWP_SHOOT2 );

	// player "shoot" animation
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

	UTIL_MakeVectors( m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle );

	// the bolt throws the shell after the shot
	m_pPlayer->m_flEjectBrass = gpGlobals->time + 0.55;

	m_pPlayer->m_iWeaponVolume = BIG_EXPLOSION_VOLUME;
	m_pPlayer->m_iWeaponFlash = NORMAL_GUN_FLASH;
	EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/awp1.wav", RANDOM_FLOAT(0.92, 1.0), ATTN_NORM, 0, 98 + RANDOM_LONG(0,3));

	Vector vecAiming;

	// sic: fUseAutoAim is not tested, the auto-aim vector is always used
	vecAiming = m_pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );

	m_flNextPrimaryAttack = m_flNextSecondaryAttack = gpGlobals->time + flCycleTime;

	if (!m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);

	m_flTimeWeaponIdle = gpGlobals->time + RANDOM_FLOAT ( 10, 15 );

	m_pPlayer->pev->punchangle.x -= 2;

	// Beta 6.1: the shot leaves from the eye again ("off center shooting
	// fixed"; Beta 6.0: 5 units to the gun hand's side)
	Vector vecSrc	 = m_pPlayer->GetGunPosition( );

	m_pPlayer->FireBullets3( vecSrc, vecAiming, Vector( flSpread, flSpread, flSpread ), 8192, 3, BULLET_PLAYER_338MAG, 200, 0.99, NULL, true );
}


void CAWP::Reload( void )
{
	int iResult;

	iResult = DefaultReload( AWP_MAX_CLIP, AWP_RELOAD, 2.5 );

	if (iResult)
	{
		// Beta 5.0 (leak): zoomed in, the last zoom step takes the scope off
		if (m_pPlayer->m_iFOV != 90)
		{
			m_pPlayer->m_iFOV = 10;
			SecondaryAttack();
		}

		m_flTimeWeaponIdle = gpGlobals->time + RANDOM_FLOAT ( 10, 15 );
	}
}



void CAWP::WeaponIdle( void )
{
	ResetEmptySound( );

	m_pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );

	if (m_flTimeWeaponIdle > gpGlobals->time)
		return;

	if (m_iClip != 0)
	{
		m_flTimeWeaponIdle = gpGlobals->time + 60;
		SendWeaponAnim( AWP_IDLE );
	}
}

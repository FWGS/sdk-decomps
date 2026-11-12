/***
*
*	Counter-Strike Beta 2.0, deagle.cpp: the Desert Eagle .50AE
*	(weapon_deagle).
*
*	A copy of the USP (usp.cpp) as Beta 1.1 had it: the same class shape,
*	the semi-automatic test, the silencer test on pev->body (with a bright
*	flash), the muzzle flash; both attacks fire alike and WeaponIdle has a
*	single idle animation. File, class and method names are Beta 3.1's
*	(Linux symbols); the animation names are ours.
*
****/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"

enum deagle_e {
	DEAGLE_IDLE1 = 0,
	DEAGLE_SHOOT1,
	DEAGLE_SHOOT2,
	DEAGLE_SHOOT_EMPTY,
	DEAGLE_RELOAD,
	DEAGLE_DRAW,
};

class CDEAGLE : public CBasePlayerWeapon
{
public:
	void Spawn( void );
	void Precache( void );
	int iItemSlot( void ) { return 2; }
	int GetItemInfo(ItemInfo *p);

	void PrimaryAttack( void );
	void SecondaryAttack( void );
	void DEAGLEFire( float flSpread, float flCycleTime, BOOL fUseSemi );
	BOOL Deploy( void );
	void Reload( void );
	void WeaponIdle( void );
	int m_iShell;
};
LINK_ENTITY_TO_CLASS( weapon_deagle, CDEAGLE );


void CDEAGLE::Spawn( )
{
	pev->classname = MAKE_STRING("weapon_deagle");
	Precache( );
	m_iId = WEAPON_DEAGLE;
	SET_MODEL(ENT(pev), "models/w_deagle.mdl");

	m_iDefaultAmmo = DEAGLE_DEFAULT_GIVE;

	FallInit();// get ready to fall down.
}


void CDEAGLE::Precache( void )
{
	PRECACHE_MODEL("models/v_deagle.mdl");
	PRECACHE_MODEL("models/w_deagle.mdl");
	PRECACHE_MODEL("models/p_deagle.mdl");

	PRECACHE_SOUND ("weapons/deagle-1.wav");
	PRECACHE_SOUND ("weapons/deagle-2.wav");
	PRECACHE_SOUND ("weapons/de_clipout.wav");
	PRECACHE_SOUND ("weapons/de_clipin.wav");
	PRECACHE_SOUND ("weapons/de_deploy.wav");

	m_iShell = PRECACHE_MODEL ("models/shell.mdl");// brass shell
}

int CDEAGLE::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "50AE";
	p->iMaxAmmo1 = DEAGLE_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = DEAGLE_MAX_CLIP;
	p->iSlot = 1;
	p->iPosition = 4;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_DEAGLE;
	p->iWeight = DEAGLE_WEIGHT;

	return 1;
}

BOOL CDEAGLE::Deploy( )
{
	m_pPlayer->m_flAccuracy = 0.875;
	g_engfuncs.pfnSetClientMaxspeed( ENT(m_pPlayer->pev), 260 );

	return DefaultDeploy( "models/v_deagle.mdl", "models/p_deagle.mdl", DEAGLE_DRAW, "onehanded" );
}

void CDEAGLE::SecondaryAttack( void )
{
	if (m_pPlayer->pev->velocity.Length2D() > 0)
		DEAGLEFire( (1 - m_pPlayer->m_flAccuracy) * 0.25, 0.325, FALSE );
	else if ( !FBitSet(m_pPlayer->pev->flags, FL_ONGROUND) )
		DEAGLEFire( (1 - m_pPlayer->m_flAccuracy) * 0.35, 0.325, FALSE );
	else if (FBitSet(m_pPlayer->pev->flags, FL_DUCKING))
		DEAGLEFire( (1 - m_pPlayer->m_flAccuracy) * 0.115, 0.325, FALSE );
	else
		DEAGLEFire( (1 - m_pPlayer->m_flAccuracy) * 0.13, 0.325, FALSE );
}

void CDEAGLE::PrimaryAttack( void )
{
	if (m_pPlayer->pev->velocity.Length2D() > 0)
		DEAGLEFire( (1 - m_pPlayer->m_flAccuracy) * 0.25, 0.325, FALSE );
	else if ( !FBitSet(m_pPlayer->pev->flags, FL_ONGROUND) )
		DEAGLEFire( (1 - m_pPlayer->m_flAccuracy) * 0.35, 0.325, FALSE );
	else if (FBitSet(m_pPlayer->pev->flags, FL_DUCKING))
		DEAGLEFire( (1 - m_pPlayer->m_flAccuracy) * 0.115, 0.325, FALSE );
	else
		DEAGLEFire( (1 - m_pPlayer->m_flAccuracy) * 0.13, 0.325, FALSE );
}

void CDEAGLE::DEAGLEFire( float flSpread , float flCycleTime, BOOL fUseSemi )
{
	if (fUseSemi == TRUE)
	{
		// one shot per press: ItemPostFrame clears m_iShotsFired when the button is let go
		m_pPlayer->m_iShotsFired++;
		if (m_pPlayer->m_iShotsFired > 1)
			return;
	}

	if (m_pPlayer->m_flLastFired == 0)
		m_pPlayer->m_flLastFired = gpGlobals->time;
	else
	{
		// accuracy grows with the time since the last shot
		m_pPlayer->m_flAccuracy = 0.6 + (gpGlobals->time - m_pPlayer->m_flLastFired) * 0.2;
		if (m_pPlayer->m_flAccuracy > 0.875)
			m_pPlayer->m_flAccuracy = 0.875;
		m_pPlayer->m_flLastFired = gpGlobals->time;
	}

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
		if (RANDOM_LONG(0,1))
			SendWeaponAnim( DEAGLE_SHOOT1 );
		else
			SendWeaponAnim( DEAGLE_SHOOT2 );
	}
	else
		SendWeaponAnim( DEAGLE_SHOOT_EMPTY );

	// player "shoot" animation
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

	UTIL_MakeVectors( m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle );

	Vector	vecShellVelocity = m_pPlayer->pev->velocity
							 + gpGlobals->v_right * RANDOM_FLOAT(50,70)
							 + gpGlobals->v_up * RANDOM_FLOAT(100,150)
							 + gpGlobals->v_forward * 25;
	EjectBrass2 ( pev->origin + m_pPlayer->pev->view_ofs + gpGlobals->v_up * -12 + gpGlobals->v_forward * 32 - gpGlobals->v_right * 12 , vecShellVelocity, pev->angles.y, m_iShell, TE_BOUNCE_SHELL, m_pPlayer->pev );

	// silenced
	if (pev->body == 1)
	{
		m_pPlayer->m_iWeaponVolume = BIG_EXPLOSION_VOLUME;
		m_pPlayer->m_iWeaponFlash = BRIGHT_GUN_FLASH;

		switch(RANDOM_LONG(0,1))
		{
		case 0:
			EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/deagle-1.wav", RANDOM_FLOAT(0.9, 1.0), 0.6);
			break;
		case 1:
			EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/deagle-2.wav", RANDOM_FLOAT(0.9, 1.0), 0.6);
			break;
		}
	}
	else
	{
		// non-silenced
		m_pPlayer->m_iWeaponVolume = BIG_EXPLOSION_VOLUME;
		m_pPlayer->m_iWeaponFlash = NORMAL_GUN_FLASH;
		if (RANDOM_LONG(0,1))
			EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/deagle-1.wav", RANDOM_FLOAT(0.92, 1.0), ATTN_NORM, 0, 98 + RANDOM_LONG(0,3));
		else
			EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/deagle-2.wav", RANDOM_FLOAT(0.92, 1.0), ATTN_NORM, 0, 98 + RANDOM_LONG(0,3));
	}

	Vector vecSrc	 = m_pPlayer->GetGunPosition( );
	Vector vecAiming;

	vecAiming = gpGlobals->v_forward;

	m_pPlayer->FireBullets3( vecSrc, vecAiming, Vector( flSpread, flSpread, flSpread ), 4096, 2, BULLET_PLAYER_50AE, 54, 0.8, NULL, true );
	m_flNextPrimaryAttack = m_flNextSecondaryAttack = gpGlobals->time + flCycleTime;

	if (!m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);

	m_flTimeWeaponIdle = gpGlobals->time + RANDOM_FLOAT ( 10, 15 );

	m_pPlayer->pev->punchangle.x -= 2;
}


void CDEAGLE::Reload( void )
{
	if (DefaultReload( DEAGLE_MAX_CLIP, DEAGLE_RELOAD, 2.1 ))
	{
		m_flTimeWeaponIdle = gpGlobals->time + RANDOM_FLOAT ( 10, 15 );
	}

	m_pPlayer->m_flAccuracy = 0.98;
}



void CDEAGLE::WeaponIdle( void )
{
	ResetEmptySound( );

	m_pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );

	if (m_flTimeWeaponIdle > gpGlobals->time)
		return;

	// only idle if the slid isn't back
	if (m_iClip != 0)
	{
		m_flTimeWeaponIdle = gpGlobals->time + 60.0 / 16.0;
		SendWeaponAnim( DEAGLE_IDLE1 );
	}
}

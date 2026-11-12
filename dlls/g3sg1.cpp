/***
*
*	Counter-Strike Beta 2.0, g3sg1.cpp: the G3/SG-1 sniper rifle (weapon_g3sg1).
*
*	The AWP's shape (SDK 1.0's glock.cpp edited): two zoom levels with a
*	green fade over the scope (Beta 1.1 renewed it as night vision), an
*	accuracy that recovers with the time between shots, and the shell
*	thrown at once with EjectBrass2.
*	File, class and method names are Beta 3.1's (Linux symbols); the
*	animation names are ours.
*
****/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"

enum g3sg1_e {
	G3SG1_IDLE = 0,
	G3SG1_SHOOT1,
	G3SG1_SHOOT2,
	G3SG1_RELOAD,
	G3SG1_DRAW,
};

class CG3SG1 : public CBasePlayerWeapon
{
public:
	void Spawn( void );
	void Precache( void );
	int iItemSlot( void ) { return 5; }
	int GetItemInfo(ItemInfo *p);

	void PrimaryAttack( void );
	void SecondaryAttack( void );
	void G3SG1Fire( float flSpread, float flCycleTime, BOOL fUseAutoAim );
	BOOL Deploy( void );
	void Reload( void );
	void WeaponIdle( void );
	int m_iShell;
};
LINK_ENTITY_TO_CLASS( weapon_g3sg1, CG3SG1 );


void CG3SG1::Spawn( )
{
	pev->classname = MAKE_STRING("weapon_g3sg1");
	Precache( );
	m_iId = WEAPON_G3SG1;
	SET_MODEL(ENT(pev), "models/w_g3sg1.mdl");

	m_iDefaultAmmo = G3SG1_DEFAULT_GIVE;

	FallInit();// get ready to fall down.
}


void CG3SG1::Precache( void )
{
	PRECACHE_MODEL("models/v_g3sg1.mdl");
	PRECACHE_MODEL("models/w_g3sg1.mdl");
	PRECACHE_MODEL("models/p_g3sg1.mdl");

	PRECACHE_SOUND("weapons/g3sg1-1.wav");
	PRECACHE_SOUND("weapons/g3sg1_slide.wav");
	PRECACHE_SOUND("weapons/g3sg1_clipin.wav");
	PRECACHE_SOUND("weapons/g3sg1_clipout.wav");
	PRECACHE_SOUND("weapons/zoom.wav");

	m_iShellLate = m_iShell = PRECACHE_MODEL ("models/shell.mdl");// brass shell
}

int CG3SG1::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "762Nato";
	p->iMaxAmmo1 = G3SG1_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = G3SG1_MAX_CLIP;
	p->iSlot = 4;
	p->iPosition = 3;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_G3SG1;
	p->iWeight = G3SG1_WEIGHT;

	return 1;
}

BOOL CG3SG1::Deploy( )
{
	g_engfuncs.pfnSetClientMaxspeed( ENT(m_pPlayer->pev), 220 );
	return DefaultDeploy( "models/v_g3sg1.mdl", "models/p_g3sg1.mdl", G3SG1_DRAW, "mp5" );
}

void CG3SG1::SecondaryAttack( void )
{
	m_pPlayer->pev->viewmodel = 0;

	// Beta 3.0: no green tint any more (Beta 2.0 and 2.1 faded the screen
	// green going in and back coming out)
	if (m_pPlayer->m_iFOV == 90)
	{
		m_pPlayer->m_iFOV = 40;
	}
	else if (m_pPlayer->m_iFOV == 40)
		m_pPlayer->m_iFOV = 15;
	else if (m_pPlayer->m_iFOV == 15)
	{
		m_pPlayer->pev->viewmodel = MAKE_STRING("models/v_g3sg1.mdl");
		m_pPlayer->m_iFOV = 90;
	}

	EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/zoom.wav", 1, ATTN_NORM);
	m_flNextSecondaryAttack = gpGlobals->time + 0.3;
}

void CG3SG1::PrimaryAttack( void )
{
	if (m_pPlayer->pev->velocity.Length2D() > 0)
		G3SG1Fire( 0.07, 0.25, FALSE );	// Beta 3.1 (3.0: 0.09, 0.2, 0.03, 0.065)
	else if ( !FBitSet(m_pPlayer->pev->flags, FL_ONGROUND) )
		G3SG1Fire( (1 - m_pPlayer->m_flAccuracy) * 0.15, 0.25, FALSE );
	else if (m_pPlayer->pev->flags & FL_DUCKING)
		G3SG1Fire( (1 - m_pPlayer->m_flAccuracy) * 0.035, 0.25, FALSE );
	else
		G3SG1Fire( (1 - m_pPlayer->m_flAccuracy) * 0.055, 0.25, FALSE );
}

void CG3SG1::G3SG1Fire( float flSpread , float flCycleTime, BOOL fUseAutoAim )
{
	// semi-automatic: one shot per press (ItemPostFrame clears m_iShotsFired)
	m_pPlayer->m_iShotsFired++;
	if (m_pPlayer->m_iShotsFired > 1)
		return;

	// Beta 3.1: unscoped, the spread grows
	if (m_pPlayer->m_iFOV == 90)
		flSpread += 0.025;

	// the accuracy recovers with the time since the last shot
	if (m_pPlayer->m_flLastFired == 0)
		m_pPlayer->m_flLastFired = gpGlobals->time;
	else
	{
		m_pPlayer->m_flAccuracy = (gpGlobals->time - m_pPlayer->m_flLastFired) * 0.225 + 0.65;	// Beta 3.1 (3.0: 0.3, 0.75)

		if (m_pPlayer->m_flAccuracy > 0.98)
			m_pPlayer->m_flAccuracy = 0.98;

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

	if (RANDOM_LONG(0,1))
		SendWeaponAnim( G3SG1_SHOOT1 );
	else
		SendWeaponAnim( G3SG1_SHOOT2 );

	// player "shoot" animation
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

	UTIL_MakeVectors( m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle );

	Vector	vecShellVelocity = m_pPlayer->pev->velocity
							 + gpGlobals->v_right * RANDOM_FLOAT(100,120)
							 + gpGlobals->v_up * RANDOM_FLOAT(35,55)
							 + gpGlobals->v_forward * RANDOM_FLOAT(40,55);
	EjectBrass2 ( pev->origin + m_pPlayer->pev->view_ofs - gpGlobals->v_up * 8 + gpGlobals->v_forward * 20 - gpGlobals->v_right * 10, vecShellVelocity, pev->angles.y, m_iShell, TE_BOUNCE_SHELL, m_pPlayer->pev );

	m_pPlayer->m_iWeaponVolume = BIG_EXPLOSION_VOLUME;
	m_pPlayer->m_iWeaponFlash = NORMAL_GUN_FLASH;
	EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/g3sg1-1.wav", RANDOM_FLOAT(0.92, 1.0), ATTN_NORM);

	Vector vecSrc	 = m_pPlayer->GetGunPosition( );
	Vector vecAiming;

	// sic: fUseAutoAim is not tested, the auto-aim vector is always used
	vecAiming = m_pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );

	m_pPlayer->FireBullets3( vecSrc, vecAiming, Vector( flSpread, flSpread, flSpread ), 8192, 3, BULLET_PLAYER_762MM, 60, 0.98, NULL, true );
	m_flNextPrimaryAttack = m_flNextSecondaryAttack = gpGlobals->time + flCycleTime;

	if (!m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);

	m_flTimeWeaponIdle = gpGlobals->time + RANDOM_FLOAT ( 10, 15 );

	m_pPlayer->pev->punchangle.x -= 2;
}


void CG3SG1::Reload( void )
{
	int iResult;

	iResult = DefaultReload( G3SG1_MAX_CLIP, G3SG1_RELOAD, 4.6 );

	if (m_pPlayer->m_iFOV != 90)
	{
		m_pPlayer->pev->viewmodel = MAKE_STRING("models/v_g3sg1.mdl");
		m_pPlayer->m_fResumeZoom = TRUE;
		m_pPlayer->m_iLastZoom = m_pPlayer->m_iFOV;
		m_pPlayer->m_iFOV = 90;
	}

	if (iResult)
	{
		m_flTimeWeaponIdle = gpGlobals->time + RANDOM_FLOAT ( 10, 15 );
	}
}



void CG3SG1::WeaponIdle( void )
{
	ResetEmptySound( );

	m_pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );

	if (m_flTimeWeaponIdle > gpGlobals->time)
		return;

	if (m_iClip != 0)
	{
		m_flTimeWeaponIdle = gpGlobals->time + 60;
		SendWeaponAnim( G3SG1_IDLE );
	}
}

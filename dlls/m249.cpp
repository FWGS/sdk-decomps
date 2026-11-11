/***
*
*	Counter-Strike Beta 1.0, m249.cpp: the M249 machine gun (weapon_m249).
*
*	SDK 1.0's glock.cpp edited (the fire function is GlockFire's) with the
*	mp5.cpp's automatic fire: the spread grows with the shots fired since
*	the trigger was pulled, and every other shot throws a shell.
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

enum m249_e {
	M249_IDLE1 = 0,
	M249_SHOOT1,
	M249_SHOOT2,
	M249_RELOAD,
	M249_DRAW,
};

class CM249 : public CBasePlayerWeapon
{
public:
	void Spawn( void );
	void Precache( void );
	int iItemSlot( void ) { return 5; }
	int GetItemInfo(ItemInfo *p);

	void PrimaryAttack( void );
	void SecondaryAttack( void );
	void M249Fire( float flSpread, float flCycleTime, BOOL fUseAutoAim );
	BOOL Deploy( void );
	void Reload( void );
	void WeaponIdle( void );
	int m_iShell;
	BOOL m_fEjectShell;		// the shot throws a shell; flips with every shot
};
LINK_ENTITY_TO_CLASS( weapon_m249, CM249 );


void CM249::Spawn( )
{
	pev->classname = MAKE_STRING("weapon_m249");
	Precache( );
	m_iId = WEAPON_M249;
	SET_MODEL(ENT(pev), "models/w_m249.mdl");

	m_iDefaultAmmo = M249_DEFAULT_GIVE;

	FallInit();// get ready to fall down.
}


void CM249::Precache( void )
{
	PRECACHE_MODEL("models/v_m249.mdl");
	PRECACHE_MODEL("models/w_m249.mdl");
	PRECACHE_MODEL("models/p_m249.mdl");

	PRECACHE_SOUND("weapons/m249-1.wav");
	PRECACHE_SOUND("weapons/m249-2.wav");
	PRECACHE_SOUND("weapons/m249_boxout.wav");
	PRECACHE_SOUND("weapons/m249_boxin.wav");
	PRECACHE_SOUND("weapons/m249_chain.wav");
	PRECACHE_SOUND("weapons/m249_coverup.wav");
	PRECACHE_SOUND("weapons/m249_coverdown.wav");

	m_iShell = PRECACHE_MODEL ("models/shell.mdl");// brass shell
}

int CM249::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "556Nato";
	p->iMaxAmmo1 = M249_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = M249_MAX_CLIP;
	p->iSlot = 4;
	p->iPosition = 2;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_M249;
	p->iWeight = M249_WEIGHT;

	return 1;
}

BOOL CM249::Deploy( )
{
	return DefaultDeploy( "models/v_m249.mdl", "models/p_m249.mdl", M249_DRAW, "m249" );
}

void CM249::SecondaryAttack( void )
{
	float flSpread;

	if (m_pPlayer->pev->velocity.Length2D() > 0)
	{
		flSpread = m_pPlayer->m_flAccuracy * 0.1;
		M249Fire( flSpread, 0.1, FALSE );
	}
	else if (m_pPlayer->pev->flags & FL_DUCKING)
	{
		flSpread = m_pPlayer->m_flAccuracy * 0.03;
		M249Fire( flSpread, 0.1, FALSE );
	}
	else
	{
		flSpread = m_pPlayer->m_flAccuracy * 0.055;
		M249Fire( flSpread, 0.1, FALSE );
	}
}

void CM249::PrimaryAttack( void )
{
	float flSpread;

	if (m_pPlayer->pev->velocity.Length2D() > 0)
	{
		flSpread = m_pPlayer->m_flAccuracy * 0.1;
		M249Fire( flSpread, 0.1, FALSE );
	}
	else if (m_pPlayer->pev->flags & FL_DUCKING)
	{
		flSpread = m_pPlayer->m_flAccuracy * 0.03;
		M249Fire( flSpread, 0.1, FALSE );
	}
	else
	{
		flSpread = m_pPlayer->m_flAccuracy * 0.055;
		M249Fire( flSpread, 0.1, FALSE );
	}
}

void CM249::M249Fire( float flSpread , float flCycleTime, BOOL fUseAutoAim )
{
	// the spread grows with the cube of the shots fired
	m_pPlayer->m_iShotsFired++;
	m_pPlayer->m_flAccuracy = (m_pPlayer->m_iShotsFired * m_pPlayer->m_iShotsFired * m_pPlayer->m_iShotsFired / 200) + 0.8;

	if (m_pPlayer->m_flAccuracy > 3.0)
		m_pPlayer->m_flAccuracy = 3.0;

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
			SendWeaponAnim( M249_SHOOT1 );
			// no break (sic): both animations are sent
		case 1:
			SendWeaponAnim( M249_SHOOT2 );
		}
	}

	// player "shoot" animation
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

	UTIL_MakeVectors( m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle );

	Vector	vecShellVelocity = m_pPlayer->pev->velocity
							 - gpGlobals->v_right * RANDOM_FLOAT(100,120)
							 + gpGlobals->v_up * RANDOM_FLOAT(50,90)
							 + gpGlobals->v_forward * 25;
	if (m_fEjectShell)
		EjectBrass2 ( pev->origin + m_pPlayer->pev->view_ofs + gpGlobals->v_up * -14 + gpGlobals->v_forward * 35 - gpGlobals->v_right * 22, vecShellVelocity, pev->angles.y, m_iShell, TE_BOUNCE_SHELL, m_pPlayer->pev );
	m_fEjectShell = 1 - m_fEjectShell;

	m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = BRIGHT_GUN_FLASH;

	if (RANDOM_LONG(0,1))
		EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/m249-1.wav", RANDOM_FLOAT(0.92, 1.0), ATTN_NORM, 0, 98 + RANDOM_LONG(0,3));
	else
		EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/m249-2.wav", RANDOM_FLOAT(0.92, 1.0), ATTN_NORM, 0, 98 + RANDOM_LONG(0,3));

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

	m_pPlayer->FireBullets3( vecSrc, vecAiming, Vector( flSpread, flSpread, flSpread ), 8192, 2, BULLET_PLAYER_556MM, 31, 0.95 );
	m_flNextPrimaryAttack = m_flNextSecondaryAttack = gpGlobals->time + flCycleTime;

	if (!m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);

	m_flTimeWeaponIdle = gpGlobals->time + RANDOM_FLOAT ( 10, 15 );

	m_pPlayer->pev->punchangle.x = RANDOM_FLOAT( -1.5, 1.5 );
}


void CM249::Reload( void )
{
	if (DefaultReload( M249_MAX_CLIP, M249_RELOAD, 4.6 ))
	{
		m_flTimeWeaponIdle = gpGlobals->time + RANDOM_FLOAT ( 10, 15 );
	}

	m_pPlayer->m_flAccuracy = 0;
}



void CM249::WeaponIdle( void )
{
	ResetEmptySound( );

	m_pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );

	if (m_flTimeWeaponIdle > gpGlobals->time)
		return;

	// sic: m_flTimeWeaponIdle is not renewed, so once it has passed every call sends the idle animation again
	SendWeaponAnim( M249_IDLE1 );
}

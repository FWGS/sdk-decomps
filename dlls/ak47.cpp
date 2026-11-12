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
	int iItemSlot( void ) { return 3; }
	int GetItemInfo(ItemInfo *p);

	void PrimaryAttack( void );
	void SecondaryAttack( void );
	void AK47Fire( float flSpread, float flCycleTime, BOOL fUseAutoAim );
	BOOL Deploy( void );
	void Reload( void );
	void WeaponIdle( void );
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
	PRECACHE_MODEL("models/w_ak47.mdl");
	PRECACHE_MODEL("models/p_ak47.mdl");

	PRECACHE_SOUND("weapons/ak47-1.wav");
	PRECACHE_SOUND("weapons/ak47-2.wav");
	PRECACHE_SOUND("weapons/ak47_clipout.wav");
	PRECACHE_SOUND("weapons/ak47_clipin.wav");
	PRECACHE_SOUND("weapons/ak47_boltpull.wav");

	m_iShell = PRECACHE_MODEL ("models/shell.mdl");// brass shell
}

int CAK47::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "762Nato";
	p->iMaxAmmo1 = AK47_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = AK47_MAX_CLIP;
	p->iSlot = 2;
	p->iPosition = 3;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_AK47;
	p->iWeight = AK47_WEIGHT;

	return 1;
}

BOOL CAK47::Deploy( )
{
	return DefaultDeploy( "models/v_ak47.mdl", "models/p_ak47.mdl", AK47_DRAW, "mp5" );
}

void CAK47::SecondaryAttack( void )
{
	float flSpread;

	if (m_pPlayer->pev->velocity.Length2D() > 0)
	{
		flSpread = m_pPlayer->m_flAccuracy * 0.14;
		AK47Fire( flSpread, 0.0955, FALSE );
	}
	else if ( !FBitSet(m_pPlayer->pev->flags, FL_ONGROUND) )
	{
		flSpread = m_pPlayer->m_flAccuracy * 0.18;
		AK47Fire( flSpread, 0.0955, FALSE );
	}
	else if (m_pPlayer->pev->flags & FL_DUCKING)
	{
		flSpread = m_pPlayer->m_flAccuracy * 0.0375;
		AK47Fire( flSpread, 0.0955, FALSE );
	}
	else
	{
		flSpread = m_pPlayer->m_flAccuracy * 0.065;
		AK47Fire( flSpread, 0.0955, FALSE );
	}
}

void CAK47::PrimaryAttack( void )
{
	float flSpread;

	if (m_pPlayer->pev->velocity.Length2D() > 0)
	{
		flSpread = m_pPlayer->m_flAccuracy * 0.14;
		AK47Fire( flSpread, 0.0955, FALSE );
	}
	else if ( !FBitSet(m_pPlayer->pev->flags, FL_ONGROUND) )
	{
		flSpread = m_pPlayer->m_flAccuracy * 0.18;
		AK47Fire( flSpread, 0.0955, FALSE );
	}
	else if (m_pPlayer->pev->flags & FL_DUCKING)
	{
		flSpread = m_pPlayer->m_flAccuracy * 0.0375;
		AK47Fire( flSpread, 0.0955, FALSE );
	}
	else
	{
		flSpread = m_pPlayer->m_flAccuracy * 0.065;
		AK47Fire( flSpread, 0.0955, FALSE );
	}
}

void CAK47::AK47Fire( float flSpread , float flCycleTime, BOOL fUseAutoAim )
{
	// the spread grows with the cube of the shots fired
	m_pPlayer->m_iShotsFired++;
	m_pPlayer->m_flAccuracy = (m_pPlayer->m_iShotsFired * m_pPlayer->m_iShotsFired * m_pPlayer->m_iShotsFired / 200) + 0.625;

	if (m_pPlayer->m_flAccuracy > 3)
		m_pPlayer->m_flAccuracy = 3;

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
			// no breaks (sic): case 0 sends all three animations, case 1 the last two
		case 1:
			SendWeaponAnim( AK47_SHOOT2 );
		case 2:
			SendWeaponAnim( AK47_SHOOT3 );
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
		EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/ak47-1.wav", RANDOM_FLOAT(0.92, 1.0), ATTN_NORM, 0, 98 + RANDOM_LONG(0,3));
	else
		EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/ak47-2.wav", RANDOM_FLOAT(0.92, 1.0), ATTN_NORM, 0, 98 + RANDOM_LONG(0,3));

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

	m_pPlayer->FireBullets3( vecSrc, vecAiming, Vector( flSpread, flSpread, flSpread ), 8192, 2, BULLET_PLAYER_762MM, 40, 0.925 );
	m_flNextPrimaryAttack = m_flNextSecondaryAttack = gpGlobals->time + flCycleTime;

	if (!m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);

	m_flTimeWeaponIdle = gpGlobals->time + RANDOM_FLOAT ( 10, 15 );

	m_pPlayer->pev->punchangle.y += RANDOM_FLOAT( -1, 1 );
	m_pPlayer->pev->punchangle.x += RANDOM_FLOAT( 0.5, 1.5 );
}


void CAK47::Reload( void )
{
	if (DefaultReload( AK47_MAX_CLIP, AK47_RELOAD, 2.75 ))
	{
		m_flTimeWeaponIdle = gpGlobals->time + RANDOM_FLOAT ( 10, 15 );
	}

	m_pPlayer->m_flAccuracy = 0;
}



void CAK47::WeaponIdle( void )
{
	ResetEmptySound( );

	m_pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );

	if (m_flTimeWeaponIdle > gpGlobals->time)
		return;

	// sic: m_flTimeWeaponIdle is not renewed, so once it has passed every call sends the idle animation again
	SendWeaponAnim( AK47_IDLE1 );
}

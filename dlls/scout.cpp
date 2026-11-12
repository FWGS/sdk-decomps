/***
*
*	Counter-Strike Beta 4.0, scout.cpp: the Steyr Scout sniper rifle
*	(weapon_scout).
*
*	awp.cpp's text changed: the 7.62 mm cartridge, two zoom levels (45 and
*	15) that slow the carrier to 220, a lighter bolt-action rifle. File,
*	class and method names are Beta 4.0's (Linux symbols: CSCOUT,
*	SCOUTFire); the animation names and the SCOUT_ macros are ours.
*
****/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"

enum scout_e {
	SCOUT_IDLE = 0,
	SCOUT_SHOOT1,
	SCOUT_SHOOT2,
	SCOUT_RELOAD,
	SCOUT_DRAW,
};

class CSCOUT : public CBasePlayerWeapon
{
public:
	void Spawn( void );
	void Precache( void );
	int iItemSlot( void ) { return 1; }
	int GetItemInfo(ItemInfo *p);

	void PrimaryAttack( void );
	void SecondaryAttack( void );
	void SCOUTFire( float flSpread, float flCycleTime, BOOL fUseAutoAim );
	BOOL Deploy( void );
	void Reload( void );
	void WeaponIdle( void );
	float GetMaxSpeed( void ) { if ( m_pPlayer->m_iFOV != 90 ) return 220; return 260; }	// Beta 5.0 (leak): Linux symbol; slower zoomed in
	int m_iShell;
};
LINK_ENTITY_TO_CLASS( weapon_scout, CSCOUT );


void CSCOUT::Spawn( )
{
	pev->classname = MAKE_STRING("weapon_scout");
	Precache( );
	m_iId = WEAPON_SCOUT;
	SET_MODEL(ENT(pev), "models/w_scout.mdl");

	m_iDefaultAmmo = SCOUT_DEFAULT_GIVE;

	FallInit();// get ready to fall down.
}


void CSCOUT::Precache( void )
{
	PRECACHE_MODEL("models/v_scout.mdl");
	PRECACHE_MODEL("models/w_scout.mdl");
	PRECACHE_MODEL("models/p_scout.mdl");

	PRECACHE_SOUND("weapons/scout_fire-1.wav");
	PRECACHE_SOUND("weapons/scout_bolt.wav");
	PRECACHE_SOUND("weapons/scout_clipin.wav");
	PRECACHE_SOUND("weapons/scout_clipout.wav");
	PRECACHE_SOUND("weapons/zoom.wav");

	m_iShellLate = m_iShell = PRECACHE_MODEL ("models/shell.mdl");// brass shell
}

int CSCOUT::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "762Nato";
	p->iMaxAmmo1 = SCOUT_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = SCOUT_MAX_CLIP;
	p->iSlot = 0;
	p->iPosition = 9;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_SCOUT;
	p->iWeight = SCOUT_WEIGHT;

	return 1;
}

BOOL CSCOUT::Deploy( )
{
	return DefaultDeploy( "models/v_scout.mdl", "models/p_scout.mdl", SCOUT_DRAW, "bow" );
}

void CSCOUT::SecondaryAttack( void )
{
	m_pPlayer->pev->viewmodel = 0;

	// slower while zoomed in
	if (m_pPlayer->m_iFOV == 90)
	{
		m_pPlayer->m_iFOV = 45;
	}
	else if (m_pPlayer->m_iFOV == 45)
	{
		m_pPlayer->m_iFOV = 15;
	}
	else if (m_pPlayer->m_iFOV == 15)
	{
		m_pPlayer->pev->viewmodel = MAKE_STRING("models/v_scout.mdl");
		m_pPlayer->m_iFOV = 90;
	}

	m_pPlayer->ResetMaxSpeed();	// Beta 5.0 (leak): GetMaxSpeed decides
	EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/zoom.wav", 1, ATTN_NORM);
	m_flNextSecondaryAttack = gpGlobals->time + 0.3;
}

void CSCOUT::PrimaryAttack( void )
{
	if (m_pPlayer->pev->velocity.Length2D() > 170)
		SCOUTFire( 0.075, 1.45, FALSE );
	else if ( !FBitSet(m_pPlayer->pev->flags, FL_ONGROUND) )
		SCOUTFire( 0.1, 1.45, FALSE );
	else if (m_pPlayer->pev->flags & FL_DUCKING)
		SCOUTFire( 0, 1.45, FALSE );
	else
		SCOUTFire( 0.007, 1.45, FALSE );
}

void CSCOUT::SCOUTFire( float flSpread , float flCycleTime, BOOL fUseAutoAim )
{
	// the shot takes the scope off; Reload and ItemPostFrame put it back
	if (m_pPlayer->m_iFOV != 90)
	{
		m_pPlayer->pev->viewmodel = MAKE_STRING("models/v_scout.mdl");
		m_pPlayer->m_fResumeZoom = TRUE;
		m_pPlayer->m_iLastZoom = m_pPlayer->m_iFOV;
		m_pPlayer->m_iFOV = 90;
	}
	else
		flSpread += 0.025;	// unscoped, the spread grows (the AWP's since Beta 3.1)

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
		SendWeaponAnim( SCOUT_SHOOT1 );
	else
		SendWeaponAnim( SCOUT_SHOOT2 );

	// player "shoot" animation
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

	UTIL_MakeVectors( m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle );

	// the bolt throws the shell after the shot
	m_pPlayer->m_flEjectBrass = gpGlobals->time + 0.56;

	m_pPlayer->m_iWeaponVolume = BIG_EXPLOSION_VOLUME;
	m_pPlayer->m_iWeaponFlash = NORMAL_GUN_FLASH;
	EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/scout_fire-1.wav", RANDOM_FLOAT(0.92, 1.0), ATTN_NORM, 0, 98 + RANDOM_LONG(0,3));

	Vector vecSrc	 = m_pPlayer->GetGunPosition( );
	Vector vecAiming;

	// sic: fUseAutoAim is not tested, the auto-aim vector is always used
	vecAiming = m_pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );

	m_pPlayer->FireBullets3( vecSrc, vecAiming, Vector( flSpread, flSpread, flSpread ), 8192, 3, BULLET_PLAYER_762MM, 65, 0.98, NULL, true );
	m_flNextPrimaryAttack = m_flNextSecondaryAttack = gpGlobals->time + flCycleTime;

	if (!m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);

	m_flTimeWeaponIdle = gpGlobals->time + RANDOM_FLOAT ( 10, 15 );

	m_pPlayer->pev->punchangle.x -= 2;
}


void CSCOUT::Reload( void )
{
	int iResult;

	iResult = DefaultReload( SCOUT_MAX_CLIP, SCOUT_RELOAD, 2 );

	if (iResult)
	{
		// Beta 5.0 (leak): zoomed in, the last zoom step takes the scope off
		if (m_pPlayer->m_iFOV != 90)
		{
			m_pPlayer->m_iFOV = 15;
			SecondaryAttack();
		}

		m_flTimeWeaponIdle = gpGlobals->time + RANDOM_FLOAT ( 10, 15 );
	}
}



void CSCOUT::WeaponIdle( void )
{
	ResetEmptySound( );

	m_pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );

	if (m_flTimeWeaponIdle > gpGlobals->time)
		return;

	if (m_iClip != 0)
	{
		m_flTimeWeaponIdle = gpGlobals->time + 60;
		SendWeaponAnim( SCOUT_IDLE );
	}
}

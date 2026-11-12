/***
*
*	Counter-Strike Beta 4.0, p228.cpp: the SIG P228 pistol (weapon_p228).
*
*	usp.cpp's text changed: no silencer, one shot sound, the .357 SIG
*	cartridge, the Desert Eagle's single idle animation. File, class and
*	method names are Beta 4.0's (Linux symbols); the animation names and
*	the P228_ macros are ours.
*
****/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"

enum p228_e {
	P228_IDLE = 0,
	P228_SHOOT1,
	P228_SHOOT2,
	P228_SHOOT3,
	P228_SHOOT_EMPTY,
	P228_RELOAD,
	P228_DRAW
};

class CP228 : public CBasePlayerWeapon
{
public:
	void Spawn( void );
	void Precache( void );
	int iItemSlot( void ) { return 2; }
	int GetItemInfo(ItemInfo *p);

	void PrimaryAttack( void );
	void SecondaryAttack( void );
	void P228Fire( float flSpread, float flCycleTime, BOOL fUseSemi );
	BOOL Deploy( void );
	void Reload( void );
	void WeaponIdle( void );
	int m_iShell;
};
LINK_ENTITY_TO_CLASS( weapon_p228, CP228 );


void CP228::Spawn( )
{
	pev->classname = MAKE_STRING("weapon_p228"); // hack to allow for old names
	Precache( );
	m_iId = WEAPON_P228;
	SET_MODEL(ENT(pev), "models/w_p228.mdl");

	m_iDefaultAmmo = P228_DEFAULT_GIVE;

	FallInit();// get ready to fall down.
}


void CP228::Precache( void )
{
	PRECACHE_MODEL("models/v_p228.mdl");
	PRECACHE_MODEL("models/w_p228.mdl");
	PRECACHE_MODEL("models/p_p228.mdl");

	PRECACHE_SOUND ("weapons/p228-1.wav");

	PRECACHE_SOUND ("weapons/p228_clipout.wav");
	PRECACHE_SOUND ("weapons/p228_clipin.wav");
	PRECACHE_SOUND ("weapons/p228_sliderelease.wav");
	PRECACHE_SOUND ("weapons/p228_slidepull.wav");

	m_iShell = PRECACHE_MODEL ("models/shell.mdl");// brass shell
}

int CP228::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "357SIG";
	p->iMaxAmmo1 = P228_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = P228_MAX_CLIP;
	p->iSlot = 1;
	p->iPosition = 5;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_P228;
	p->iWeight = P228_WEIGHT;

	return 1;
}

BOOL CP228::Deploy( )
{
	m_pPlayer->m_flAccuracy = 0.98;
	g_engfuncs.pfnSetClientMaxspeed( ENT(m_pPlayer->pev), 260 );
	return DefaultDeploy( "models/v_p228.mdl", "models/p_p228.mdl", P228_DRAW, "onehanded" );
}

void CP228::SecondaryAttack( void )
{
	if (m_pPlayer->pev->velocity.Length2D() > 0)
		P228Fire( (1 - m_pPlayer->m_flAccuracy) * 0.255, 0.225, FALSE );
	else if ( !FBitSet(m_pPlayer->pev->flags, FL_ONGROUND) )
		P228Fire( (1 - m_pPlayer->m_flAccuracy) * 0.35, 0.225, FALSE );
	else if (FBitSet(m_pPlayer->pev->flags, FL_DUCKING))
		P228Fire( (1 - m_pPlayer->m_flAccuracy) * 0.075, 0.225, FALSE );
	else
		P228Fire( (1 - m_pPlayer->m_flAccuracy) * 0.15, 0.225, FALSE );
}

void CP228::PrimaryAttack( void )
{
	if (m_pPlayer->pev->velocity.Length2D() > 0)
		P228Fire( (1 - m_pPlayer->m_flAccuracy) * 0.255, 0.225, FALSE );
	else if ( !FBitSet(m_pPlayer->pev->flags, FL_ONGROUND) )
		P228Fire( (1 - m_pPlayer->m_flAccuracy) * 0.35, 0.225, FALSE );
	else if (FBitSet(m_pPlayer->pev->flags, FL_DUCKING))
		P228Fire( (1 - m_pPlayer->m_flAccuracy) * 0.075, 0.225, FALSE );
	else
		P228Fire( (1 - m_pPlayer->m_flAccuracy) * 0.15, 0.225, FALSE );
}

void CP228::P228Fire( float flSpread , float flCycleTime, BOOL fUseSemi )
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
		m_pPlayer->m_flAccuracy = 0.65 + (gpGlobals->time - m_pPlayer->m_flLastFired) * 0.2;
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

	if (m_iClip != 0)
	{
		switch (RANDOM_LONG(0,2))
		{
		case 0:
			SendWeaponAnim( P228_SHOOT1 );
			break;
		case 1:
			SendWeaponAnim( P228_SHOOT2 );
			break;
		case 2:
			SendWeaponAnim( P228_SHOOT3 );
			break;
		}
	}
	else
		SendWeaponAnim( P228_SHOOT_EMPTY );

	// player "shoot" animation
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

	UTIL_MakeVectors( m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle );

	Vector	vecShellVelocity = m_pPlayer->pev->velocity
							 + gpGlobals->v_right * RANDOM_FLOAT(40,150)
							 + gpGlobals->v_up * RANDOM_FLOAT(100,260)
							 + gpGlobals->v_forward * RANDOM_FLOAT(-100,100);
	EjectBrass2 ( pev->origin + m_pPlayer->pev->view_ofs + gpGlobals->v_up * -12 + gpGlobals->v_forward * 32 - gpGlobals->v_right * 12 , vecShellVelocity, pev->angles.y, m_iShell, TE_BOUNCE_SHELL, m_pPlayer->pev );

	m_pPlayer->m_iWeaponVolume = BIG_EXPLOSION_VOLUME;
	m_pPlayer->m_iWeaponFlash = DIM_GUN_FLASH;

	EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/p228-1.wav", RANDOM_FLOAT(0.5, 0.6), ATTN_NORM);

	Vector vecSrc	 = m_pPlayer->GetGunPosition( );
	Vector vecAiming;

	vecAiming = gpGlobals->v_forward;

	m_pPlayer->FireBullets3( vecSrc, vecAiming, Vector( flSpread, flSpread, flSpread ), 4096, 1, BULLET_PLAYER_357SIG, 30, 0.9, NULL, true );
	m_flNextPrimaryAttack = m_flNextSecondaryAttack = gpGlobals->time + flCycleTime;

	if (!m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);

	m_flTimeWeaponIdle = gpGlobals->time + RANDOM_FLOAT ( 10, 15 );

	m_pPlayer->pev->punchangle.x -= 2;
}


void CP228::Reload( void )
{
	if (DefaultReload( P228_MAX_CLIP, P228_RELOAD, 2.7 ))
	{
		m_flTimeWeaponIdle = gpGlobals->time + RANDOM_FLOAT ( 10, 15 );
	}

	m_pPlayer->m_flAccuracy = 0.98;
}



void CP228::WeaponIdle( void )
{
	ResetEmptySound( );

	m_pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );

	if (m_flTimeWeaponIdle > gpGlobals->time)
		return;

	// only idle if the slid isn't back
	if (m_iClip != 0)
	{
		m_flTimeWeaponIdle = gpGlobals->time + 3.0625;
		SendWeaponAnim( P228_IDLE );
	}
}

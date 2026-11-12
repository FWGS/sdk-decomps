/***
*
*	Counter-Strike Beta 2.0, usp.cpp: the Heckler & Koch USP .45 Tactical
*	(weapon_usp), the counter-terrorists' pistol.
*
*	SDK 1.0's glock.cpp as the Counter-Strike team changed it: the same
*	class shape and WeaponIdle, the silencer test on pev->body kept, the
*	bullets fired with FireBullets3 and the spread taken from the player's
*	accuracy. Beta 1.1 made the secondary attack semi-automatic; Beta 2.0
*	passes FALSE from both attacks, and has no muzzle flash. File, class and
*	method names are Beta 3.1's (Linux symbols); animations: models/v_usp.mdl.
*
****/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"

enum usp_e {
	USP_IDLE1 = 0,
	USP_IDLE2,
	USP_IDLE3,
	USP_SHOOT,
	USP_SHOOT_EMPTY,
	USP_RELOAD,
	USP_DRAW,
	USP_HOLSTER,
	USP_ADD_SILENCER,
	USP_DRAW2,
	USP_SHOOT2,
	USP_RELOAD2
};

class CUSP : public CBasePlayerWeapon
{
public:
	void Spawn( void );
	void Precache( void );
	int iItemSlot( void ) { return 2; }
	int GetItemInfo(ItemInfo *p);
	float GetMaxSpeed( void ) { return 260; }	// Beta 5.0 (leak): Linux symbol

	void PrimaryAttack( void );
	void USPFire( float flSpread, float flCycleTime, BOOL fUseSemi );
	BOOL Deploy( void );
	void Reload( void );
	void WeaponIdle( void );
	int m_iShell;
};
LINK_ENTITY_TO_CLASS( weapon_usp, CUSP );


void CUSP::Spawn( )
{
	pev->classname = MAKE_STRING("weapon_usp"); // hack to allow for old names
	Precache( );
	m_iId = WEAPON_USP;
	SET_MODEL(ENT(pev), "models/w_usp.mdl");

	m_iDefaultAmmo = USP_DEFAULT_GIVE;

	FallInit();// get ready to fall down.
}


void CUSP::Precache( void )
{
	PRECACHE_MODEL("models/v_usp.mdl");
	PRECACHE_MODEL("models/w_usp.mdl");
	PRECACHE_MODEL("models/p_usp.mdl");

	PRECACHE_SOUND ("weapons/usp1.wav");
	PRECACHE_SOUND ("weapons/usp2.wav");

	PRECACHE_SOUND ("weapons/clipout1.wav");
	PRECACHE_SOUND ("weapons/clipin1.wav");
	PRECACHE_SOUND ("weapons/sliderelease1.wav");
	PRECACHE_SOUND ("weapons/slideback1.wav");

	m_iShell = PRECACHE_MODEL ("models/shell.mdl");// brass shell
}

int CUSP::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "45ACP";
	p->iMaxAmmo1 = USP_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = USP_MAX_CLIP;
	p->iSlot = 1;
	p->iPosition = 4;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_USP;
	p->iWeight = USP_WEIGHT;

	return 1;
}

BOOL CUSP::Deploy( )
{
	m_pPlayer->m_flAccuracy = 0.98;

	if (RANDOM_LONG(0,1))
		return DefaultDeploy( "models/v_usp.mdl", "models/p_usp.mdl", USP_DRAW, "tmp" );
	else
		return DefaultDeploy( "models/v_usp.mdl", "models/p_usp.mdl", USP_DRAW2, "tmp" );
}

void CUSP::PrimaryAttack( void )
{
	if (m_pPlayer->pev->velocity.Length2D() > 0)
		USPFire( (1 - m_pPlayer->m_flAccuracy) * 0.225, 0.285, FALSE );
	else if ( !FBitSet(m_pPlayer->pev->flags, FL_ONGROUND) )
		USPFire( (1 - m_pPlayer->m_flAccuracy) * 0.3, 0.285, FALSE );
	else if (FBitSet(m_pPlayer->pev->flags, FL_DUCKING))
		USPFire( (1 - m_pPlayer->m_flAccuracy) * 0.08, 0.285, FALSE );
	else
		USPFire( (1 - m_pPlayer->m_flAccuracy) * 0.1, 0.285, FALSE );
}

void CUSP::USPFire( float flSpread , float flCycleTime, BOOL fUseSemi )
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
		m_pPlayer->m_flAccuracy = 0.7 + (gpGlobals->time - m_pPlayer->m_flLastFired) * 0.3;
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

	// no muzzle flash any more, silencer on or off

	if (m_iClip != 0)
	{
		if (RANDOM_LONG(0,1))
			SendWeaponAnim( USP_SHOOT );
		else
			SendWeaponAnim( USP_SHOOT2 );
	}
	else
		SendWeaponAnim( USP_SHOOT_EMPTY );

	// player "shoot" animation
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

	UTIL_MakeVectors( m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle );

	Vector	vecShellVelocity = m_pPlayer->pev->velocity
							 + gpGlobals->v_right * RANDOM_FLOAT(40,150)
							 + gpGlobals->v_up * RANDOM_FLOAT(100,260)
							 + gpGlobals->v_forward * RANDOM_FLOAT(-100,100);
	EjectBrass2 ( pev->origin + m_pPlayer->pev->view_ofs + gpGlobals->v_up * -12 + gpGlobals->v_forward * 32 - gpGlobals->v_right * 12 , vecShellVelocity, pev->angles.y, m_iShell, TE_BOUNCE_SHELL, m_pPlayer->pev );

	// silenced
	if (pev->body == 1)
	{
		m_pPlayer->m_iWeaponVolume = BIG_EXPLOSION_VOLUME;
		m_pPlayer->m_iWeaponFlash = DIM_GUN_FLASH;

		switch(RANDOM_LONG(0,1))
		{
		case 0:
			EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/usp1.wav", RANDOM_FLOAT(0.5, 0.6), ATTN_NORM);
			break;
		case 1:
			EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/usp2.wav", RANDOM_FLOAT(0.5, 0.6), ATTN_NORM);
			break;
		}
	}
	else
	{
		// non-silenced
		m_pPlayer->m_iWeaponVolume = BIG_EXPLOSION_VOLUME;
		m_pPlayer->m_iWeaponFlash = NORMAL_GUN_FLASH;
		if (RANDOM_LONG(0,1))
			EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/usp1.wav", RANDOM_FLOAT(0.92, 1.0), 1.6);
		else
			EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/usp2.wav", RANDOM_FLOAT(0.92, 1.0), 1.6);
	}

	Vector vecSrc	 = m_pPlayer->GetGunPosition( );
	Vector vecAiming;

	vecAiming = gpGlobals->v_forward;

	m_pPlayer->FireBullets3( vecSrc, vecAiming, Vector( flSpread, flSpread, flSpread ), 4096, 1, BULLET_PLAYER_45ACP, 34, 0.83, NULL, true );
	m_flNextPrimaryAttack = m_flNextSecondaryAttack = gpGlobals->time + flCycleTime;

	if (!m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);

	m_flTimeWeaponIdle = gpGlobals->time + RANDOM_FLOAT ( 10, 15 );

	m_pPlayer->pev->punchangle.x -= 2;
}


void CUSP::Reload( void )
{
	int iResult;

	if (RANDOM_LONG(0,1))
		iResult = DefaultReload( USP_MAX_CLIP, USP_RELOAD, 2.2 );
	else
		iResult = DefaultReload( USP_MAX_CLIP, USP_RELOAD2, 2.2 );

	if (iResult)
	{
		m_flTimeWeaponIdle = gpGlobals->time + RANDOM_FLOAT ( 10, 15 );
	}

	m_pPlayer->m_flAccuracy = 0.98;
}



void CUSP::WeaponIdle( void )
{
	ResetEmptySound( );

	m_pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );

	if (m_flTimeWeaponIdle > gpGlobals->time)
		return;

	// only idle if the slid isn't back
	if (m_iClip != 0)
	{
		int iAnim;
		float flRand = RANDOM_FLOAT(0, 1);
		if (flRand <= 0.3 + 0 * 0.75)
		{
			iAnim = USP_IDLE3;
			m_flTimeWeaponIdle = gpGlobals->time + 49.0 / 16;
		}
		else if (flRand <= 0.6 + 0 * 0.875)
		{
			iAnim = USP_IDLE1;
			m_flTimeWeaponIdle = gpGlobals->time + 60.0 / 16.0;
		}
		else
		{
			iAnim = USP_IDLE2;
			m_flTimeWeaponIdle = gpGlobals->time + 40.0 / 16.0;
		}
		SendWeaponAnim( iAnim );
	}
}

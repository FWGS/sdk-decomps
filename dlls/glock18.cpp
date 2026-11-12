/***
*
*	Counter-Strike Beta 2.0, glock18.cpp: the Glock 18 Select Fire
*	(weapon_glock18), the terrorists' pistol.
*
*	SDK 1.0's glock.cpp as the Counter-Strike team changed it: the same
*	class shape and WeaponIdle, the bullets fired with FireBullets3. Beta
*	2.0: the secondary attack switches the primary between single shots and
*	a three-round burst (Beta 1.1 fired the burst from the secondary attack;
*	GLOCK18Fire fires the first round and FireRemaining, weapons.cpp, the
*	other two). File, class and method names are Beta 3.1's (Linux
*	symbols); the animation names are the sequence labels of Beta 1.0's
*	models/v_glock18.mdl (Beta 1.1's model adds one, GLOCK18_SHOOT3).
*
****/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"

enum glock18_e {
	GLOCK18_IDLE1 = 0,
	GLOCK18_IDLE2,
	GLOCK18_IDLE3,
	GLOCK18_SHOOT,
	GLOCK18_SHOOT2,
	GLOCK18_SHOOT3,		// new in Beta 1.1: the single shot (name ours; the model's label is not known)
	GLOCK18_SHOOT_EMPTY,
	GLOCK18_RELOAD,
	GLOCK18_DRAW,
	GLOCK18_HOLSTER,
	GLOCK18_ADD_SILENCER,
	GLOCK18_DRAW2,
	GLOCK18_RELOAD2
};

class CGLOCK18 : public CBasePlayerWeapon
{
public:
	void Spawn( void );
	void Precache( void );
	int iItemSlot( void ) { return 2; }
	int GetItemInfo(ItemInfo *p);
	float GetMaxSpeed( void ) { return 250; }	// Beta 5.0 (leak): Linux symbol; Beta 6.0: 250 (was 260)

	void PrimaryAttack( void );
	void SecondaryAttack( void );
	void GLOCK18Fire( float flSpread, float flCycleTime, BOOL fUseBurstMode );
	BOOL Deploy( void );
	void Reload( void );
	void WeaponIdle( void );
	int m_iShell;
	bool m_fBurstMode;		// the secondary attack switches the primary between single shots and three-round bursts (name ours)
};
LINK_ENTITY_TO_CLASS( weapon_glock18, CGLOCK18 );


void CGLOCK18::Spawn( )
{
	pev->classname = MAKE_STRING("weapon_glock18"); // hack to allow for old names
	Precache( );
	m_iId = WEAPON_GLOCK18;
	SET_MODEL(ENT(pev), "models/w_glock18.mdl");

	m_iDefaultAmmo = GLOCK18_DEFAULT_GIVE;

	FallInit();// get ready to fall down.
}


void CGLOCK18::Precache( void )
{
	PRECACHE_MODEL("models/v_glock18.mdl");
	PRECACHE_MODEL("models/v_glock18_r.mdl");	// Beta 6.0: the right-handed view model
	PRECACHE_MODEL("models/w_glock18.mdl");
	PRECACHE_MODEL("models/p_glock18.mdl");

	PRECACHE_SOUND ("weapons/glock18-1.wav");
	PRECACHE_SOUND ("weapons/glock18-2.wav");

	PRECACHE_SOUND ("weapons/clipout1.wav");
	PRECACHE_SOUND ("weapons/clipin1.wav");
	PRECACHE_SOUND ("weapons/sliderelease1.wav");
	PRECACHE_SOUND ("weapons/slideback1.wav");
	PRECACHE_SOUND ("weapons/357_cock1.wav");
	PRECACHE_SOUND ("weapons/de_clipin.wav");
	PRECACHE_SOUND ("weapons/de_clipout.wav");

	m_iShellLate = m_iShell = PRECACHE_MODEL ("models/pshell.mdl");// brass shell (Beta 6.0: the pistol shell)
}

int CGLOCK18::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "9mm";
	p->iMaxAmmo1 = GLOCK18_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = GLOCK18_MAX_CLIP;
	p->iSlot = 1;
	p->iPosition = 2;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_GLOCK18;
	p->iWeight = GLOCK18_WEIGHT;

	return 1;
}

BOOL CGLOCK18::Deploy( )
{
	m_fBurstMode = false;

	// Beta 6.0: v_glock18.mdl holds the gun in the left hand, v_glock18_r.mdl in the right
	if ( m_pPlayer->m_bLeftHanded == true )
	{
		if (RANDOM_LONG(0,1))
			return DefaultDeploy( "models/v_glock18.mdl", "models/p_glock18.mdl", GLOCK18_DRAW, "onehanded" );
		else
			return DefaultDeploy( "models/v_glock18.mdl", "models/p_glock18.mdl", GLOCK18_DRAW2, "onehanded" );
	}
	else
	{
		if (RANDOM_LONG(0,1))
			return DefaultDeploy( "models/v_glock18_r.mdl", "models/p_glock18.mdl", GLOCK18_DRAW, "onehanded" );
		else
			return DefaultDeploy( "models/v_glock18_r.mdl", "models/p_glock18.mdl", GLOCK18_DRAW2, "onehanded" );
	}
}

void CGLOCK18::SecondaryAttack( void )
{
	// switch the primary attack between the single shot and the burst
	if (!m_fBurstMode)
	{
		ClientPrint( m_pPlayer->pev, HUD_PRINTCENTER, "Switched to burst-fire" );
		m_fBurstMode = true;
	}
	else
	{
		ClientPrint( m_pPlayer->pev, HUD_PRINTCENTER, "Switched to semi-automatic" );
		m_fBurstMode = false;
	}

	m_flNextSecondaryAttack = gpGlobals->time + 0.3;
}

void CGLOCK18::PrimaryAttack( void )
{
	if (m_fBurstMode == TRUE)
	{
		// the three-round burst
		GLOCK18Fire( 0.015, 0.5, TRUE );
	}
	else if (m_pPlayer->pev->velocity.Length2D() > 0)
		GLOCK18Fire( (1 - m_pPlayer->m_flAccuracy) * 0.165, 0.225, FALSE );
	else if ( !FBitSet(m_pPlayer->pev->flags, FL_ONGROUND) )
		GLOCK18Fire( (1 - m_pPlayer->m_flAccuracy) * 0.2, 0.225, FALSE );
	else if (FBitSet(m_pPlayer->pev->flags, FL_DUCKING))
		GLOCK18Fire( (1 - m_pPlayer->m_flAccuracy) * 0.075, 0.225, FALSE );
	else
		GLOCK18Fire( (1 - m_pPlayer->m_flAccuracy) * 0.1, 0.225, FALSE );
}

void CGLOCK18::GLOCK18Fire( float flSpread , float flCycleTime, BOOL fUseBurstMode )
{
	if (fUseBurstMode == TRUE)
		m_pPlayer->m_iBurstShotsFired = 0;

	if (m_pPlayer->m_flLastFired == 0)
		m_pPlayer->m_flLastFired = gpGlobals->time;
	else
	{
		// accuracy grows with the time since the last shot
		m_pPlayer->m_flAccuracy = (gpGlobals->time - m_pPlayer->m_flLastFired) * 0.2 + 0.6;
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
		if (fUseBurstMode == TRUE)
		{
			if (RANDOM_LONG(0,1))
				SendWeaponAnim( GLOCK18_SHOOT );
			else
				SendWeaponAnim( GLOCK18_SHOOT2 );
		}
		else
			SendWeaponAnim( GLOCK18_SHOOT3 );
	}
	else
		SendWeaponAnim( GLOCK18_SHOOT_EMPTY );

	// player "shoot" animation
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

	UTIL_MakeVectors( m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle );

	m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = NORMAL_GUN_FLASH;

	// one sound for the burst, the other for the single shot (Beta 5.0 leak: and
	// for the burst's last round)
	if (fUseBurstMode == TRUE && m_iClip > 0)
		EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/glock18-1.wav", RANDOM_FLOAT(0.92, 1.0), ATTN_NORM);
	else
		EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/glock18-2.wav", RANDOM_FLOAT(0.92, 1.0), ATTN_NORM);

	Vector vecAiming;

	vecAiming = gpGlobals->v_forward;

	m_flNextPrimaryAttack = m_flNextSecondaryAttack = gpGlobals->time + flCycleTime;

	if (!m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);

	m_flTimeWeaponIdle = gpGlobals->time + RANDOM_FLOAT ( 10, 15 );

	m_pPlayer->pev->punchangle.x -= 2;

	if (fUseBurstMode == TRUE)
	{
		// the burst's other two rounds: FireRemaining, from ItemPostFrame
		m_pPlayer->m_flNextBurstShot = gpGlobals->time + 0.1;
		m_pPlayer->m_iBurstShotsFired++;
	}

	// Beta 6.1: the shot leaves from the eye again ("off center shooting
	// fixed"; Beta 6.0: 5 units to the gun hand's side); the shell still
	// flies out of the other side
	Vector vecSrc	 = m_pPlayer->GetGunPosition( );

	if ( m_pPlayer->m_bLeftHanded == true )
	{

		// the shell comes out closer to the gun (Beta 5.2: 20 down, 32 forward, 20 to the side)
		Vector	vecShellVelocity = m_pPlayer->pev->velocity
								 + gpGlobals->v_right * RANDOM_FLOAT(40,150)
								 + gpGlobals->v_up * RANDOM_FLOAT(100,360)
								 + gpGlobals->v_forward * RANDOM_FLOAT(25,200);
		EjectBrass2 ( pev->origin + m_pPlayer->pev->view_ofs + gpGlobals->v_up * -12 + gpGlobals->v_forward * 16 - gpGlobals->v_right * 6 , vecShellVelocity, pev->angles.y, m_iShell, TE_BOUNCE_SHELL, m_pPlayer->pev );
	}
	else
	{
		Vector	vecShellVelocity = m_pPlayer->pev->velocity
								 - gpGlobals->v_right * RANDOM_FLOAT(40,150)
								 + gpGlobals->v_up * RANDOM_FLOAT(100,360)
								 + gpGlobals->v_forward * RANDOM_FLOAT(25,200);
		EjectBrass2 ( pev->origin + m_pPlayer->pev->view_ofs + gpGlobals->v_up * -12 + gpGlobals->v_forward * 16 + gpGlobals->v_right * 6 , vecShellVelocity, pev->angles.y, m_iShell, TE_BOUNCE_SHELL, m_pPlayer->pev );
	}

	m_pPlayer->FireBullets3( vecSrc, vecAiming, Vector( flSpread, flSpread, flSpread ), 8192, 1, BULLET_PLAYER_9MM, 24, 0.87, NULL, true );
}


void CGLOCK18::Reload( void )
{
	int iResult;

	if (RANDOM_LONG(0,1))
		iResult = DefaultReload( GLOCK18_MAX_CLIP, GLOCK18_RELOAD, 2.2 );
	else
		iResult = DefaultReload( GLOCK18_MAX_CLIP, GLOCK18_RELOAD2, 2.2 );

	if (iResult)
	{
		m_flTimeWeaponIdle = gpGlobals->time + RANDOM_FLOAT ( 10, 15 );
	}
}



void CGLOCK18::WeaponIdle( void )
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
			iAnim = GLOCK18_IDLE3;
			m_flTimeWeaponIdle = gpGlobals->time + 49.0 / 16;
		}
		else if (flRand <= 0.6 + 0 * 0.875)
		{
			iAnim = GLOCK18_IDLE1;
			m_flTimeWeaponIdle = gpGlobals->time + 60.0 / 16.0;
		}
		else
		{
			iAnim = GLOCK18_IDLE2;
			m_flTimeWeaponIdle = gpGlobals->time + 40.0 / 16.0;
		}
		SendWeaponAnim( iAnim );
	}
}

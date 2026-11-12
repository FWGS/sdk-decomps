/***
*
*	Counter-Strike Beta 2.0, flashbang.cpp: the flashbang grenade
*	(weapon_flashbang).
*
*	SDK 1.0's handgrenade.cpp as the Counter-Strike team changed it: the
*	same throw, one throw animation, no holster animation, no fidget. The
*	grenade itself is the stock CGrenade::ShootTimed, which in Beta 1.0
*	sets models/w_flashbang.mdl and blinds through RadiusFlash when it
*	goes off (ggrenade.cpp). File, class and method names are Beta 3.1's
*	(Linux symbols); the animation names are the sequence labels of
*	models/v_flashbang.mdl.
*
****/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"


#define	HANDGRENADE_PRIMARY_VOLUME		450

enum flashbang_e {
	FLASHBANG_IDLE = 0,
	FLASHBANG_PULLPIN,
	FLASHBANG_THROW,
	FLASHBANG_DEPLOY
};


class CFlashbang : public CBasePlayerWeapon
{
public:
	void Spawn( void );
	void Precache( void );
	int iItemSlot( void ) { return 1; }
	int GetItemInfo(ItemInfo *p);

	void PrimaryAttack( void );
	BOOL Deploy( void );
	BOOL CanHolster( void );
	void Holster( void );
	void WeaponIdle( void );
	float m_flStartThrow;
	float m_flReleaseThrow;
};
LINK_ENTITY_TO_CLASS( weapon_flashbang, CFlashbang );


void CFlashbang::Spawn( )
{
	pev->classname = MAKE_STRING("weapon_flashbang");
	Precache( );
	m_iId = WEAPON_FLASHBANG;
	SET_MODEL(ENT(pev), "models/w_flashbang.mdl");

	pev->dmg = 4;

	m_iDefaultAmmo = FLASHBANG_DEFAULT_GIVE;

	FallInit();// get ready to fall down.
}


void CFlashbang::Precache( void )
{
	PRECACHE_MODEL("models/w_flashbang.mdl");
	PRECACHE_MODEL("models/v_flashbang.mdl");
	PRECACHE_MODEL("models/p_flashbang.mdl");

	PRECACHE_SOUND("weapons/flashbang-1.wav");
	PRECACHE_SOUND("weapons/flashbang-2.wav");
	PRECACHE_SOUND("weapons/pinpull.wav");
}

int CFlashbang::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "Flashbang";
	p->iMaxAmmo1 = FLASHBANG_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = 0;
	p->iPosition = 2;
	p->iId = m_iId = WEAPON_FLASHBANG;
	p->iWeight = FLASHBANG_WEIGHT;
	p->iFlags = ITEM_FLAG_LIMITINWORLD | ITEM_FLAG_EXHAUSTIBLE;

	return 1;
}


BOOL CFlashbang::Deploy( )
{
	m_flReleaseThrow = -1;
	g_engfuncs.pfnSetClientMaxspeed( ENT(m_pPlayer->pev), 260 );
	return DefaultDeploy( "models/v_flashbang.mdl", "models/p_flashbang.mdl", FLASHBANG_DEPLOY, "crowbar" );
}

BOOL CFlashbang::CanHolster( void )
{
	// can only holster hand grenades when not primed!
	return ( m_flStartThrow == 0 );
}

void CFlashbang::Holster( )
{
	m_pPlayer->m_flNextAttack = gpGlobals->time + 0.5;
	if (!m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType])
	{
		// no more grenades!
		m_pPlayer->pev->weapons &= ~(1<<WEAPON_FLASHBANG);
		SetThink( DestroyItem );
		pev->nextthink = gpGlobals->time + 0.1;
		m_pPlayer->m_iFlashBangs = 0;
	}

	EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "common/null.wav", 1.0, ATTN_NORM);
}

void CFlashbang::PrimaryAttack()
{
	if (!m_flStartThrow && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] > 0)
	{
		m_flStartThrow = gpGlobals->time;
		m_flReleaseThrow = 0;

		SendWeaponAnim( FLASHBANG_PULLPIN );
		m_flTimeWeaponIdle = gpGlobals->time + 0.5;
	}
}


void CFlashbang::WeaponIdle( void )
{
	if (m_flReleaseThrow == 0)
		m_flReleaseThrow = gpGlobals->time;

	if (m_flTimeWeaponIdle > gpGlobals->time)
		return;

	if (m_flStartThrow)
	{
		m_pPlayer->Radio( "FIREINHOLE" );	// Beta 4.0: back ("automatic flashbang .wav added back")

		Vector angThrow = m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle;

		if (angThrow.x < 0)
			angThrow.x = -10 + angThrow.x * ((90 - 10) / 90.0);
		else
			angThrow.x = -10 + angThrow.x * ((90 + 10) / 90.0);

		float flVel = (90 - angThrow.x) * 6;
		if (flVel > 750)
			flVel = 750;

		UTIL_MakeVectors( angThrow );

		Vector vecSrc = m_pPlayer->pev->origin + m_pPlayer->pev->view_ofs + gpGlobals->v_forward * 16 - gpGlobals->v_right * 12;

		Vector vecThrow = gpGlobals->v_forward * flVel + m_pPlayer->pev->velocity;

		// alway explode 3 seconds after the pin was pulled
		float time = m_flStartThrow - gpGlobals->time + 3.0;
		if (time < 0)
			time = 0;

		// (Beta 3.0 radioed "fireinhole" here)
		CGrenade::ShootTimed( m_pPlayer->pev, vecSrc, vecThrow, time );

		SendWeaponAnim( FLASHBANG_THROW );

		// player "shoot" animation
		m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

		m_flStartThrow = 0;
		m_flNextPrimaryAttack = gpGlobals->time + 0.5;
		m_flTimeWeaponIdle = gpGlobals->time + 0.75;

		m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType]--;
		m_pPlayer->m_iFlashBangs--;

		if ( !m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] )
		{
			// just threw last grenade
			// set attack times in the future, and weapon idle in the future so we can see the whole throw
			// animation, weapon idle will automatically retire the weapon for us.
			m_flTimeWeaponIdle = m_flNextSecondaryAttack = m_flNextPrimaryAttack = gpGlobals->time + 0.5;// ensure that the animation can finish playing
		}
		return;
	}
	else if (m_flReleaseThrow > 0)
	{
		// we've finished the throw, restart.
		m_flStartThrow = 0;

		if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType])
		{
			SendWeaponAnim( FLASHBANG_DEPLOY );
		}
		else
		{
			RetireWeapon();
			return;
		}

		m_flTimeWeaponIdle = gpGlobals->time + RANDOM_FLOAT ( 10, 15 );
		m_flReleaseThrow = -1;
		return;
	}

	if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType])
	{
		int iAnim;
		float flRand = RANDOM_FLOAT(0, 1);
		if (flRand <= 0.75)
		{
			iAnim = FLASHBANG_IDLE;
			m_flTimeWeaponIdle = gpGlobals->time + RANDOM_FLOAT ( 10, 15 );// how long till we do this again.
		}
		else
		{
			iAnim = FLASHBANG_IDLE;	// Beta 4.0 (Beta 3.x left it unset here)
			m_flTimeWeaponIdle = gpGlobals->time + 75.0 / 30.0;
		}

		SendWeaponAnim( iAnim );
	}
}

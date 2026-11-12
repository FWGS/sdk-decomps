/***
*
*	Counter-Strike Beta 4.0, hegrenade.cpp: the high-explosive grenade
*	(weapon_hegrenade).
*
*	flashbang.cpp's text changed: its own models and sounds, HUD slot 0
*	position 1, and the grenade is CGrenade::ShootTimed2 (ggrenade.cpp), two
*	seconds and the thrower's team. File, class and method names are Beta
*	4.0's (Linux symbols); the animation names and the HEGRENADE_ macros
*	are ours.
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

enum hegrenade_e {
	HEGRENADE_IDLE = 0,
	HEGRENADE_PULLPIN,
	HEGRENADE_THROW,
	HEGRENADE_DEPLOY
};


class CHEGrenade : public CBasePlayerWeapon
{
public:
	void Spawn( void );
	void Precache( void );
	int iItemSlot( void ) { return 4; }
	int GetItemInfo(ItemInfo *p);

	void PrimaryAttack( void );
	BOOL Deploy( void );
	BOOL CanHolster( void );
	void Holster( void );
	void WeaponIdle( void );
	float m_flStartThrow;
	float m_flReleaseThrow;
	float GetMaxSpeed( void ) { return 260; }	// Beta 5.0 (leak): Linux symbol
	BOOL CanDrop( void ) { return FALSE; }		// Beta 5.0 (leak): Linux symbol
	BOOL CanDeploy( void ) { return m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType]; }	// Beta 5.0 (Linux symbol): only with a grenade left
};
LINK_ENTITY_TO_CLASS( weapon_hegrenade, CHEGrenade );


void CHEGrenade::Spawn( )
{
	// Beta 5.0: the classname is left as the map or the factory gave it
	Precache( );
	m_iId = WEAPON_HEGRENADE;
	SET_MODEL(ENT(pev), "models/w_hegrenade.mdl");

	pev->dmg = 4;

	m_iDefaultAmmo = HEGRENADE_DEFAULT_GIVE;

	FallInit();// get ready to fall down.
}


void CHEGrenade::Precache( void )
{
	PRECACHE_MODEL("models/w_hegrenade.mdl");
	PRECACHE_MODEL("models/v_hegrenade.mdl");
	PRECACHE_MODEL("models/p_hegrenade.mdl");

	PRECACHE_SOUND("weapons/hegrenade-1.wav");
	PRECACHE_SOUND("weapons/hegrenade-2.wav");
	PRECACHE_SOUND("weapons/he_bounce-1.wav");
	PRECACHE_SOUND("weapons/pinpull.wav");
}

int CHEGrenade::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "HEGrenade";
	p->iMaxAmmo1 = HEGRENADE_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = 3;
	p->iPosition = 1;
	p->iId = m_iId = WEAPON_HEGRENADE;
	p->iWeight = HEGRENADE_WEIGHT;
	p->iFlags = ITEM_FLAG_LIMITINWORLD | ITEM_FLAG_EXHAUSTIBLE;

	return 1;
}


BOOL CHEGrenade::Deploy( )
{
	m_flReleaseThrow = -1;
	return DefaultDeploy( "models/v_hegrenade.mdl", "models/p_hegrenade.mdl", HEGRENADE_DEPLOY, "crowbar" );
}

BOOL CHEGrenade::CanHolster( void )
{
	// can only holster hand grenades when not primed!
	return ( m_flStartThrow == 0 );
}

void CHEGrenade::Holster( )
{
	m_pPlayer->m_flNextAttack = gpGlobals->time + 0.5;
	if (!m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType])
	{
		// no more grenades!
		m_pPlayer->pev->weapons &= ~(1<<WEAPON_HEGRENADE);
		DestroyItem();	// Beta 5.0: at once (the leak: a tenth of a second later)
	}

	// Beta 5.0: the pin goes back in
	m_flReleaseThrow = 0;
	m_flStartThrow = 0;

	EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "common/null.wav", 1.0, ATTN_NORM);
}

void CHEGrenade::PrimaryAttack()
{
	if (!m_flStartThrow && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] > 0)
	{
		m_flStartThrow = gpGlobals->time;
		m_flReleaseThrow = 0;

		SendWeaponAnim( HEGRENADE_PULLPIN );
		m_flTimeWeaponIdle = gpGlobals->time + 0.5;
	}
}


void CHEGrenade::WeaponIdle( void )
{
	if (m_flReleaseThrow == 0)
		m_flReleaseThrow = gpGlobals->time;

	if (m_flTimeWeaponIdle > gpGlobals->time)
		return;

	if (m_flStartThrow)
	{
		m_pPlayer->Radio( "%!MRAD_FIREINHOLE", "Fire in the hole!\n" );	// Beta 5.0 (leak): the sentence and the chat text

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

		// two seconds after the throw, whenever the pin was pulled; the
		// thrower's team mates are spared (CBasePlayer::TakeDamage)
		CGrenade::ShootTimed2( m_pPlayer->pev, vecSrc, vecThrow, 2, m_pPlayer->m_iTeam );

		SendWeaponAnim( HEGRENADE_THROW );

		// player "shoot" animation
		m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

		m_flStartThrow = 0;
		m_flNextPrimaryAttack = gpGlobals->time + 0.5;
		m_flTimeWeaponIdle = gpGlobals->time + 0.75;

		m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType]--;

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
			SendWeaponAnim( HEGRENADE_DEPLOY );
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
		// Beta 5.0: always the idle animation (the leak: a quarter of the
		// time an unset one, sic, for 2.5 seconds)
		SendWeaponAnim( HEGRENADE_IDLE );
		m_flTimeWeaponIdle = gpGlobals->time + RANDOM_FLOAT ( 10, 15 );// how long till we do this again.
	}
}

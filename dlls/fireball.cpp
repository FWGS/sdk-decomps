/***
*
*	Scientist Slaughterhouse: the fireball launcher.
*	The file name and the names marked "ours" are ours; class, entity and
*	exported member names come from the binary.
*
****/
/*

===== fireball.cpp ========================================================

  weapon_fireball (CFBLauncher): the fireball launcher, on the gluon gun's
  models. Each shot costs 5 "ffuel" and throws a fireball (CFireBall), a
  grenade that explodes on contact; the special attack switches between
  direct fire (straight flight) and ballistic fire (it falls). While it
  flies the fireball sheds ten fire trails (fire_trail, CFireTrail), small
  burning sprites that follow its path and die with it.
  ammo_fireballclip gives 20 "ffuel".

*/
#if !defined( OEM_BUILD ) && !defined( HLDEMO_BUILD )

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"

#define FBLAUNCHER_PRIMARY_VOLUME	450	// name ours (the gluon gun's)

enum egon_e {
	EGON_IDLE1 = 0,
	EGON_FIDGET1,
	EGON_ALTFIREON,
	EGON_ALTFIRECYCLE,
	EGON_ALTFIREOFF,
	EGON_FIRE1,
	EGON_FIRE2,
	EGON_FIRE3,
	EGON_FIRE4,
	EGON_DRAW,
	EGON_HOLSTER
};

//=========================================================
// fire_trail - a burning sprite a fireball leaves behind;
// it lives as long as the fireball does
//=========================================================
class CFireTrail : public CSprite
{
public:
	void Spawn( void );

	void EXPORT TrailThink( void );

	EHANDLE m_hOwner;	// name ours
};

LINK_ENTITY_TO_CLASS( fire_trail, CFireTrail );

void CFireTrail::Spawn( void )
{
	SET_MODEL( ENT( pev ), "sprites/fireball.spr" );
	pev->solid = SOLID_BBOX;
	UTIL_SetSize( pev, Vector( -4, -4, -4 ), Vector( 4, 4, 4 ) );
	m_maxFrame = (float) MODEL_FRAMES( pev->modelindex ) - 1;
	pev->framerate = 10;
	TurnOn( );
	SetThink( TrailThink );
	SetTransparency( kRenderTransAdd, 255, 255, 255, 255, kRenderFxNone );
	pev->spawnflags |= SF_SPRITE_TEMPORARY;
}

void CFireTrail::TrailThink( void )
{
	AnimateThink( );

	if ( m_hOwner == NULL )
		UTIL_Remove( this );
}

//=========================================================
// fireball
//=========================================================
class CFireBall : public CGrenade
{
public:
	void Spawn( void );
	void Precache( void );
	const char *GetKillVerb( BOOL fGibbed, int &iHitGroup ) { return "blasted"; }
	const char *GetKillDesc( void ) { return "a fireball"; }
	int HeatSignature( void ) { return 5; }

	void EXPORT BurnThink( void );
	void EXPORT FireBallTouch( CBaseEntity *pOther );

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	float	m_lastTime;
	float	m_maxFrame;
};

LINK_ENTITY_TO_CLASS( fireball, CFireBall );

TYPEDESCRIPTION	CFireBall::m_SaveData[] =
{
	DEFINE_FIELD( CFireBall, m_lastTime, FIELD_TIME ),
	DEFINE_FIELD( CFireBall, m_maxFrame, FIELD_FLOAT ),
};

IMPLEMENT_SAVERESTORE( CFireBall, CGrenade );

void CFireBall::Spawn( void )
{
	Precache( );
	SET_MODEL( ENT( pev ), "sprites/fireball.spr" );
	pev->dmg = 100;
	pev->spawnflags |= 1;
	pev->solid = SOLID_BBOX;
	pev->movetype = MOVETYPE_FLY;
	pev->renderamt = 255;
	pev->rendermode = kRenderTransAdd;

	SetThink( BurnThink );
	SetTouch( ExplodeTouch );
	pev->nextthink = gpGlobals->time + 0.05;

	UTIL_SetSize( pev, Vector( -4, -4, -4 ), Vector( 4, 4, 4 ) );

	pev->iuser1 = 10;			// fire trails left to drop
	pev->vuser1 = pev->origin;	// where they start
	m_maxFrame = (float) MODEL_FRAMES( pev->modelindex ) - 1;
	m_lastTime = gpGlobals->time;
}

void CFireBall::Precache( void )
{
	PRECACHE_MODEL( "sprites/fireball.spr" );
}

void CFireBall::BurnThink( void )
{
	pev->nextthink = gpGlobals->time + 0.1;

	if ( UTIL_PointContents( pev->origin ) == CONTENTS_WATER )
	{
		Smoke( );
		return;
	}

	if ( pev->iuser1 > 0 )
	{
		CFireTrail *pTrail = (CFireTrail *)Create( "fire_trail", pev->vuser1, g_vecZero, pev->owner );
		pTrail->m_hOwner = this;
		pTrail->pev->scale = pev->iuser1 * 0.1;
		pTrail->pev->velocity = pev->vuser2;	// the fireball's launch velocity
		pTrail->pev->movetype = pev->movetype;
		pTrail->pev->gravity = pev->gravity;
		pTrail->pev->fuser1 = pev->iuser1 * 0.05;
		pev->iuser1--;
		pev->nextthink = gpGlobals->time + 0.05;
	}

	// animate
	pev->frame += ( gpGlobals->time - m_lastTime ) * 10;
	m_lastTime = gpGlobals->time;
	if ( pev->frame >= m_maxFrame )
		pev->frame -= m_maxFrame;

	if ( !( pev->iuser1 % 2 ) )
	{
		MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, pev->origin );
			WRITE_BYTE( TE_DLIGHT );
			WRITE_COORD( pev->origin.x );	// X
			WRITE_COORD( pev->origin.y );	// Y
			WRITE_COORD( pev->origin.z );	// Z
			WRITE_BYTE( 32 );		// radius * 0.1
			WRITE_BYTE( 255 );		// r
			WRITE_BYTE( 192 );		// g
			WRITE_BYTE( 0 );		// b
			WRITE_BYTE( 1 );		// time * 10
			WRITE_BYTE( 0 );		// decay * 0.1
		MESSAGE_END( );
	}
}

// Nothing sets this touch: the fireball explodes with CGrenade's ExplodeTouch.
void CFireBall::FireBallTouch( CBaseEntity *pOther )
{
	if ( pev->dmg == 100 )
	{
		ExplodeTouch( pOther );
		return;
	}

	TraceResult tr;
	Vector vecSpot = pev->origin;
	vecSpot.z = pev->absmin.z - 32;

	UTIL_TraceLine( pev->origin, vecSpot, ignore_monsters, ENT( pev ), &tr );

	if ( tr.flFraction >= 1.0 )
	{
		// nothing below: drop
		if ( pev->iuser1 == 1 )
			ExplodeTouch( pOther );

		pev->iuser1 = 1;
		pev->movetype = MOVETYPE_TOSS;
		pev->velocity.x = pev->velocity.y = 0;
		pev->velocity.z = -fabs( pev->velocity.z );
		return;
	}

	ExplodeTouch( pOther );
	CBaseEntity::Create( "fire_spin", pev->origin, pev->angles, pev->owner );	// sic: there is no fire_spin
}

//=========================================================
// weapon_fireball
//=========================================================
class CFBLauncher : public CBasePlayerWeapon
{
public:
	void Spawn( void );
	void Precache( void );
	int iItemSlot( void ) { return 4; }
	int GetItemInfo(ItemInfo *p);
	int AddToPlayer( CBasePlayer *pPlayer );

	BOOL Deploy( void );
	void Holster( int skiplocal = 0 );

	void PrimaryAttack( void );
	void WeaponSpecial( void );
	void WeaponIdle( void );

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	int		m_ballistic;	// the fireballs fall
};

LINK_ENTITY_TO_CLASS( weapon_fireball, CFBLauncher );

TYPEDESCRIPTION	CFBLauncher::m_SaveData[] =
{
	DEFINE_FIELD( CFBLauncher, m_ballistic, FIELD_INTEGER ),
};

IMPLEMENT_SAVERESTORE( CFBLauncher, CBasePlayerWeapon );

void CFBLauncher::Spawn( )
{
	Precache( );
	m_iId = WEAPON_FIREBALL;
	SET_MODEL(ENT(pev), "models/w_egon.mdl");
	pev->renderfx = kRenderFxGlowShell;
	pev->rendercolor.x = 255;

	m_iDefaultAmmo = 20;

	FallInit();// get ready to fall down.
}

void CFBLauncher::Precache( void )
{
	PRECACHE_MODEL("models/w_egon.mdl");
	PRECACHE_MODEL("models/v_egon.mdl");
	PRECACHE_MODEL("models/p_egon.mdl");

	PRECACHE_MODEL("models/w_9mmclip.mdl");
	PRECACHE_SOUND("items/9mmclip1.wav");

	PRECACHE_SOUND ("weapons/357_cock1.wav");
	PRECACHE_SOUND ("x/x_shoot1.wav");

	UTIL_PrecacheOther( "fireball" );
}

BOOL CFBLauncher::Deploy( void )
{
	return DefaultDeploy( "models/v_egon.mdl", "models/p_egon.mdl", EGON_DRAW, "egon" );
}

int CFBLauncher::AddToPlayer( CBasePlayer *pPlayer )
{
	if ( CBasePlayerWeapon::AddToPlayer( pPlayer ) )
	{
		MESSAGE_BEGIN( MSG_ONE, gmsgWeapPickup, NULL, pPlayer->pev );
			WRITE_BYTE( m_iId );
		MESSAGE_END();
		return TRUE;
	}
	return FALSE;
}

void CFBLauncher::Holster( int skiplocal /* = 0 */ )
{
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;
	SendWeaponAnim( EGON_HOLSTER );
}

int CFBLauncher::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "ffuel";
	p->iMaxAmmo1 = FUEL_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = 3;
	p->iPosition = 2;
	p->iId = m_iId = WEAPON_FIREBALL;
	p->iFlags = 0;
	p->iWeight = FIREBALL_WEIGHT;

	return 1;
}

void CFBLauncher::PrimaryAttack( void )
{
	// don't fire underwater
	if ( m_pPlayer->pev->waterlevel == 3 )
	{
		PlayEmptySound( );
		m_flNextSecondaryAttack = m_flNextPrimaryAttack = gpGlobals->time + 0.15;
		return;
	}

	if ( m_pPlayer->m_rgAmmo[ m_iPrimaryAmmoType ] < 5 )
	{
		PlayEmptySound( );
		m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;
		return;
	}

	m_pPlayer->m_iWeaponVolume = FBLAUNCHER_PRIMARY_VOLUME;

	// player "shoot" animation
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );
	SendWeaponAnim( EGON_ALTFIRECYCLE );

	Vector anglesAim = m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle;
	Vector vecSrc = m_pPlayer->GetGunPosition( );
	UTIL_MakeVectors( anglesAim );

	EMIT_SOUND( ENT(pev), CHAN_WEAPON, "x/x_shoot1.wav", 1, ATTN_NORM );

	CBaseEntity *pFireBall = CBaseEntity::Create( "fireball", vecSrc, g_vecZero, m_pPlayer->edict() );
	pFireBall->pev->velocity = gpGlobals->v_forward * 1000;
	pFireBall->pev->vuser2 = pFireBall->pev->velocity;
	if ( m_ballistic )
		pFireBall->pev->movetype = MOVETYPE_TOSS;

	m_pPlayer->m_rgAmmo[ m_iPrimaryAmmoType ] -= 5;

	m_flTimeWeaponIdle = gpGlobals->time + 1.0;
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 1.5;
}

void CFBLauncher::WeaponSpecial( void )
{
	m_ballistic = !m_ballistic;
	SendWeaponAnim( EGON_ALTFIREON + m_ballistic * 2 );
	ClientPrint( m_pPlayer->pev, HUD_PRINTCENTER, "%s mode selected", m_ballistic ? "Ballistic" : "Direct" );
}

void CFBLauncher::WeaponIdle( void )
{
	ResetEmptySound( );

	if ( m_flTimeWeaponIdle > gpGlobals->time )
		return;

	int iAnim;

	float flRand = RANDOM_FLOAT(0,1);

	if ( flRand <= 0.5 )
	{
		iAnim = EGON_IDLE1;
		m_flTimeWeaponIdle = gpGlobals->time + RANDOM_FLOAT(10,15);
	}
	else
	{
		iAnim = EGON_FIDGET1;
		m_flTimeWeaponIdle = gpGlobals->time + 3;
	}

	SendWeaponAnim( iAnim );
}

class CFireBallAmmo : public CBasePlayerAmmo	// name ours
{
	void Spawn( void )
	{
		Precache( );
		SET_MODEL(ENT(pev), "models/w_weaponbox.mdl");
		pev->renderfx = kRenderFxGlowShell;
		pev->rendercolor.x = 255;
		CBasePlayerAmmo::Spawn( );
	}
	void Precache( void )
	{
		PRECACHE_MODEL ("models/w_weaponbox.mdl");
		PRECACHE_SOUND("items/9mmclip1.wav");
	}
	BOOL AddAmmo( CBaseEntity *pOther )
	{
		if (pOther->GiveAmmo( 20, "ffuel", FUEL_MAX_CARRY ) != -1)
		{
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM);
			return TRUE;
		}
		return FALSE;
	}
};
LINK_ENTITY_TO_CLASS( ammo_fireballclip, CFireBallAmmo );

#endif

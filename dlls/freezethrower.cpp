/***
*
*	Scientist Slaughterhouse: the freeze thrower.
*	The file name and the names marked "ours" are ours; class, entity and
*	exported member names come from the binary.
*
****/
/*

===== freezethrower.cpp ========================================================

  weapon_freezethrower (server only; the egon's models): fires a freezespark
  at 1000 units/s for one uranium every 0.3 seconds.
  freezespark (CFreezeBlast): a flickering sprite that bounces off walls; it
  breaks after 3 to 6 bounces, or at once when it hits something that takes
  damage (10 damage, DMG_SLOWFREEZE) or is shielded.

*/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"
#include "effects.h"

#define FREEZETHROWER_VOLUME	450	// name ours: the egon's primary volume

// the egon's animations (names ours)
enum freezethrower_e {
	FREEZETHROWER_IDLE1 = 0,
	FREEZETHROWER_FIDGET1,
	FREEZETHROWER_ALTFIREON,
	FREEZETHROWER_ALTFIRECYCLE,
	FREEZETHROWER_ALTFIREOFF,
	FREEZETHROWER_FIRE1,
	FREEZETHROWER_FIRE2,
	FREEZETHROWER_FIRE3,
	FREEZETHROWER_FIRE4,
	FREEZETHROWER_DRAW,
	FREEZETHROWER_HOLSTER
};

class CFreezeBlast : public CLaserPulse
{
public:
	void Spawn( void );
	void Precache( void );
	const char *GetKillDesc( void ) { return "a freezethrower"; }
	const char *GetKillVerb( BOOL fGibbed, int &iHitGroup ) { return DamageVerb( DMG_SLOWFREEZE, fGibbed ); }

	void EXPORT AnimateThink( void );
	void EXPORT FreezeTouch( CBaseEntity *pOther );
};

LINK_ENTITY_TO_CLASS( freezespark, CFreezeBlast );

void CFreezeBlast::AnimateThink( void )
{
	pev->frame = RANDOM_LONG( 0, 4 );
	pev->nextthink = gpGlobals->time + 0.1;
}

void CFreezeBlast::Spawn( void )
{
	Precache( );

	pev->solid = SOLID_BBOX;
	pev->movetype = MOVETYPE_BOUNCEMISSILE;
	pev->rendermode = kRenderTransAdd;
	pev->renderamt = 255;
	pev->health = RANDOM_LONG( 3, 6 );	// bounces left
	pev->scale = 0.8;

	SET_MODEL( ENT( pev ), "sprites/qtorp.spr" );
	UTIL_SetSize( pev, Vector( -1, -1, -1 ), Vector( 1, 1, 1 ) );

	SetThink( AnimateThink );
	SetTouch( FreezeTouch );
	pev->nextthink = gpGlobals->time + 0.1;
}

void CFreezeBlast::Precache( void )
{
	PRECACHE_SOUND( "x/x_shoot1.wav" );
	PRECACHE_MODEL( "sprites/qtorp.spr" );
}

void CFreezeBlast::FreezeTouch( CBaseEntity *pOther )
{
	if ( pOther->Shielded() )
		pev->health = 0;

	bool fBreak = pev->health <= 0;

	if ( pOther->pev->takedamage )
	{
		fBreak = true;
		pOther->TakeDamage( pev, VARS( pev->owner ), 10, DMG_SLOWFREEZE | DMG_NEVERGIB );
	}

	pev->health -= 1;

	if ( fBreak )
	{
		EMIT_SOUND_DYN( ENT( pev ), CHAN_WEAPON, "x/x_shoot1.wav", 1, 0.8, 0, PITCH_NORM );

		CSprite *pSprite = CSprite::SpriteCreate( "sprites/qtorp.spr", pev->origin, FALSE );
		pSprite->SetScale( pev->scale );
		pSprite->SetTransparency( kRenderTransAdd, 255, 255, 255, 255, kRenderFxNone );
		pSprite->pev->frame = pev->frame;
		pSprite->Expand( 4, 500 );

		UTIL_Remove( this );
	}
}


class CFreezeThrower : public CBasePlayerWeapon	// name ours
{
public:
	void Spawn( void );
	void Precache( void );
	int iItemSlot( void ) { return 4; }
	int GetItemInfo( ItemInfo *p );
	int AddToPlayer( CBasePlayer *pPlayer );

	BOOL Deploy( void );
	void Holster( int skiplocal = 0 );

	void PrimaryAttack( void );
	void WeaponIdle( void );
};

LINK_ENTITY_TO_CLASS( weapon_freezethrower, CFreezeThrower );

void CFreezeThrower::Spawn( )
{
	Precache( );
	m_iId = WEAPON_FREEZETHROWER;
	SET_MODEL( ENT( pev ), "models/w_egon.mdl" );

	pev->rendercolor = Vector( 128, 128, 255 );
	pev->renderfx = kRenderFxGlowShell;

	m_iDefaultAmmo = 100;

	FallInit( );// get ready to fall down.
}

void CFreezeThrower::Precache( void )
{
	PRECACHE_MODEL( "models/w_egon.mdl" );
	PRECACHE_MODEL( "models/v_egon.mdl" );
	PRECACHE_MODEL( "models/p_egon.mdl" );

	PRECACHE_MODEL( "models/w_9mmclip.mdl" );
	PRECACHE_SOUND( "items/9mmclip1.wav" );

	PRECACHE_SOUND( "weapons/357_cock1.wav" );
	PRECACHE_SOUND( "weapons/displacer_start.wav" );

	UTIL_PrecacheOther( "freezespark" );
}

BOOL CFreezeThrower::Deploy( void )
{
	return DefaultDeploy( "models/v_egon.mdl", "models/p_egon.mdl", FREEZETHROWER_DRAW, "egon" );
}

int CFreezeThrower::AddToPlayer( CBasePlayer *pPlayer )
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

void CFreezeThrower::Holster( int skiplocal /* = 0 */ )
{
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;
	SendWeaponAnim( FREEZETHROWER_HOLSTER );
}

int CFreezeThrower::GetItemInfo( ItemInfo *p )
{
	p->pszName = STRING( pev->classname );
	p->pszAmmo1 = "uranium";
	p->iMaxAmmo1 = URANIUM_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = 3;
	p->iPosition = 5;
	p->iId = m_iId = WEAPON_FREEZETHROWER;
	p->iFlags = 0;
	p->iWeight = FREEZETHROWER_WEIGHT;

	return 1;
}

int g_iFreezeThrowerFireAnims[] = { FREEZETHROWER_FIRE1, FREEZETHROWER_FIRE2, FREEZETHROWER_FIRE3, FREEZETHROWER_FIRE4 };	// name ours

void CFreezeThrower::PrimaryAttack( void )
{
	// don't fire underwater
	if ( m_pPlayer->pev->waterlevel == 3 )
	{
		PlayEmptySound( );
		m_flNextSecondaryAttack = m_flNextPrimaryAttack = gpGlobals->time + 0.15;
		return;
	}

	if ( !m_pPlayer->m_rgAmmo[ m_iPrimaryAmmoType ] )
	{
		PlayEmptySound( );
		m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;
		return;
	}

	m_pPlayer->m_iWeaponVolume = FREEZETHROWER_VOLUME;
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );
	SendWeaponAnim( g_iFreezeThrowerFireAnims[ RANDOM_LONG( 0, 3 ) ] );

	// sic: the source comes from the vectors of the last UTIL_MakeVectors, not of this aim
	Vector anglesAim = m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle;
	Vector vecSrc = m_pPlayer->GetGunPosition( ) + gpGlobals->v_up * -12 + gpGlobals->v_right * 3;
	UTIL_MakeVectors( anglesAim );
	Vector vecDir = gpGlobals->v_forward;

	EMIT_SOUND_DYN( ENT( pev ), CHAN_WEAPON, "weapons/displacer_start.wav", 1, 0.8, 0, PITCH_NORM );

	CBaseEntity *pSpark = CBaseEntity::Create( "freezespark", vecSrc, m_pPlayer->pev->v_angle, m_pPlayer->edict() );
	pSpark->pev->velocity = vecDir * 1000;
	pSpark->pev->speed = 1000;

	m_pPlayer->m_rgAmmo[ m_iPrimaryAmmoType ]--;
	if ( m_pPlayer->m_rgAmmo[ m_iPrimaryAmmoType ] <= 0 )
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate( "!HEV_AMO0", FALSE, 0 );

	m_flTimeWeaponIdle = gpGlobals->time + 1.0;
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.3;
}

void CFreezeThrower::WeaponIdle( void )
{
	ResetEmptySound( );

	if ( m_flTimeWeaponIdle > gpGlobals->time )
		return;

	int iAnim;

	float flRand = RANDOM_FLOAT( 0, 1 );

	if ( flRand <= 0.5 )
	{
		iAnim = FREEZETHROWER_IDLE1;
		m_flTimeWeaponIdle = gpGlobals->time + RANDOM_FLOAT( 10, 15 );
	}
	else
	{
		iAnim = FREEZETHROWER_FIDGET1;
		m_flTimeWeaponIdle = gpGlobals->time + 3;
	}

	SendWeaponAnim( iAnim );
}

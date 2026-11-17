/***
*
*	Scientist Slaughterhouse: the flamethrower.
*	The file name and the names marked "ours" are ours; class, entity and
*	exported member names come from the binary.
*
****/
/*

===== flamethrower.cpp ========================================================

  weapon_flamethrower (CFlameThrower, declared in weapons.h; the client
  predicts it): on the gluon gun's models. The primary attack sprays
  flames, the secondary molecular acid that falls; both throw a fly_flame
  (CFlame) every 0.05 seconds and burn one unit of their ammo ("ffuel",
  "acid") every 0.2 seconds. flamefire.sc starts the spray's effects
  (iparam1 0 flames, 1 acid), flamestop.sc stops them; pev->dmgtime is the
  time the next unit of ammo goes, 0 while not firing.
  fly_flame: an invisible ball that slows down, grows and burns or eats
  through what is around it for 3 seconds.
  ammo_acidtank gives 30 "acid".

*/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "effects.h"
#include "customentity.h"
#include "gamerules.h"
#include "decals.h"

#define	FLAMETHROWER_PRIMARY_VOLUME		450	// name ours (the gluon gun's)

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

#ifndef CLIENT_DLL

//=========================================================
// fly_flame - the flamethrower's flame (pev->weapons 0) or
// acid (1). pev->dmg is its age.
//=========================================================
class CFlame : public CSprite
{
public:
	void Spawn( void );
	BOOL CanBeRepulsed( bool fForce ) { return FALSE; }
	BOOL CanBeTractored( void ) { return FALSE; }
	const char *GetKillVerb( BOOL fGibbed, int &iHitGroup ) { return DamageVerb( pev->weapons ? DMG_ACID : DMG_SLOWBURN, fGibbed ); }
	const char *GetKillDesc( void ) { return pev->weapons ? "molecular acid" : "a flamethrower"; }
	int HeatSignature( void ) { return !pev->weapons * 5; }

	void EXPORT BurnThink( void );
	void EXPORT HurtTouch( CBaseEntity *pOther );
};

LINK_ENTITY_TO_CLASS( fly_flame, CFlame );

void CFlame::Spawn( void )
{
	SET_MODEL( ENT( pev ), g_pModelNameLaser );
	pev->solid = SOLID_BBOX;
	pev->movetype = MOVETYPE_FLY;
	pev->effects = EF_NODRAW;
	pev->spawnflags |= SF_SPRITE_TEMPORARY;
	pev->dmg = 0;
	pev->nextthink = gpGlobals->time + 1;
	m_lastTime = gpGlobals->time;

	SetTouch( HurtTouch );
	SetThink( BurnThink );

	UTIL_SetOrigin( pev, pev->origin );
	UTIL_SetSize( pev, Vector( 0, 0, 0 ), Vector( 0, 0, 0 ) );
}

void CFlame::BurnThink( void )
{
	pev->nextthink = gpGlobals->time + 0.1;

	float flInterval = gpGlobals->time - m_lastTime;
	m_lastTime = gpGlobals->time;

	pev->velocity = pev->velocity * 0.8;
	pev->dmg += flInterval;

	if ( pev->dmg >= 3 )
	{
		UTIL_Remove( this );
		return;
	}

	if ( pev->dmg > 1 )
		pev->scale = ( pev->dmg - 1 ) * 20 + 10;

	entvars_t *pevOwner = pev;
	if ( pev->owner )
		pevOwner = VARS( pev->owner );

	// pev->scale is the radius
	if ( pev->weapons )
		RadiusDamage( pev->origin, pev, pevOwner, 5, pev->scale, CLASS_NONE, DMG_ACID | DMG_NEVERGIB );
	else
		RadiusDamage( pev->origin, pev, pevOwner, 5, pev->scale, CLASS_NONE, DMG_SLOWBURN | DMG_NEVERGIB );
}

void CFlame::HurtTouch( CBaseEntity *pOther )
{
	SetThink( NULL );
	SetTouch( NULL );

	TraceResult tr = UTIL_GetGlobalTrace( );

	if ( pOther->pev->takedamage )
	{
		entvars_t *pevOwner = pev;
		if ( pev->owner )
			pevOwner = VARS( pev->owner );

		ClearMultiDamage( );
		if ( pev->weapons )
			pOther->TraceAttack( pevOwner, 5, pev->velocity.Normalize( ), &tr, DMG_ACID | DMG_NEVERGIB );
		else
			pOther->TraceAttack( pevOwner, 5, pev->velocity.Normalize( ), &tr, DMG_SLOWBURN | DMG_NEVERGIB );
		ApplyMultiDamage( pev, pevOwner );
	}

	if ( pOther->IsBSPModel( ) )
	{
		if ( pev->weapons )
			UTIL_DecalTrace( &tr, DECAL_SPIT1 + RANDOM_LONG( 0, 1 ) );
		else
			UTIL_DecalTrace( &tr, DECAL_SMALLSCORCH1 + RANDOM_LONG( 0, 2 ) );
	}

	UTIL_Remove( this );
}

#endif	// CLIENT_DLL

LINK_ENTITY_TO_CLASS( weapon_flamethrower, CFlameThrower );

void CFlameThrower::Spawn( )
{
	Precache( );
	m_iId = WEAPON_FLAMETHROWER;
	SET_MODEL(ENT(pev), "models/w_egon.mdl");
	pev->renderfx = kRenderFxGlowShell;
	pev->rendercolor.x = 255;
	pev->rendercolor.y = 64;

	m_iDefaultAmmo = 20;

	FallInit();// get ready to fall down.
}

void CFlameThrower::Precache( void )
{
	PRECACHE_MODEL("models/w_egon.mdl");
	PRECACHE_MODEL("models/v_egon.mdl");
	PRECACHE_MODEL("models/p_egon.mdl");

	PRECACHE_MODEL("models/w_9mmclip.mdl");
	PRECACHE_SOUND("items/9mmclip1.wav");

	PRECACHE_SOUND( "garg/gar_flameoff1.wav" );
	PRECACHE_SOUND( "garg/gar_flamerun1.wav" );
	PRECACHE_SOUND( "garg/gar_flameon1.wav" );

	PRECACHE_SOUND ("weapons/357_cock1.wav");

	m_usFlameFire = PRECACHE_EVENT ( 1, "events/flamefire.sc" );
	m_usFlameStop = PRECACHE_EVENT ( 1, "events/flamestop.sc" );
}

BOOL CFlameThrower::Deploy( void )
{
	pev->dmgtime = 0;
	return DefaultDeploy( "models/v_egon.mdl", "models/p_egon.mdl", EGON_DRAW, "egon" );
}

int CFlameThrower::AddToPlayer( CBasePlayer *pPlayer )
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

void CFlameThrower::Holster( int skiplocal /* = 0 */ )
{
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;
	SendWeaponAnim( EGON_HOLSTER );

	EndAttack();
}

int CFlameThrower::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "ffuel";
	p->iMaxAmmo1 = FUEL_MAX_CARRY;
	p->pszAmmo2 = "acid";
	p->iMaxAmmo2 = ACID_MAX_CARRY;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = 3;
	p->iPosition = 1;
	p->iId = m_iId = WEAPON_FLAMETHROWER;
	p->iFlags = 0;
	p->iWeight = FLAMETHROWER_WEIGHT;

	return 1;
}

void CFlameThrower::PrimaryAttack( void )
{
	// don't fire underwater
	if ( m_pPlayer->pev->waterlevel == 3 )
	{
		PlayEmptySound( );
		m_flNextSecondaryAttack = m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.15;
		EndAttack();
		return;
	}

	if ( !m_pPlayer->m_rgAmmo[ m_iPrimaryAmmoType ] )
	{
		m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;
		EndAttack();
		return;
	}

	if ( pev->dmgtime == 0 )
		PLAYBACK_EVENT_FULL( FEV_NOTHOST, m_pPlayer->edict(), m_usFlameFire, 0.0, (float *)&m_pPlayer->pev->origin, (float *)&m_pPlayer->pev->angles, 0.0, 0.0, 0, 0, 0, 0 );

#ifndef CLIENT_DLL
	m_pPlayer->m_iWeaponVolume = FLAMETHROWER_PRIMARY_VOLUME;

	Vector vecSrc;
	vecSrc = m_pPlayer->GetGunPosition( );
	UTIL_MakeVectors( m_pPlayer->pev->v_angle );
	vecSrc = vecSrc + gpGlobals->v_forward * 16 + gpGlobals->v_up * -8 + gpGlobals->v_right * 3;

	CBaseEntity *pFlame = CBaseEntity::Create( "fly_flame", vecSrc, g_vecZero, m_pPlayer->edict() );
	pFlame->pev->velocity = gpGlobals->v_forward * 240;
	pFlame->pev->weapons = 0;	// fire
#endif

	if ( pev->dmgtime < gpGlobals->time )
	{
		m_pPlayer->m_rgAmmo[ m_iPrimaryAmmoType ]--;
		pev->dmgtime = gpGlobals->time + 0.2;
	}

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.0;
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.05;
	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 1;
}

void CFlameThrower::SecondaryAttack( void )
{
	// don't fire underwater
	if ( m_pPlayer->pev->waterlevel == 3 )
	{
		PlayEmptySound( );
		m_flNextSecondaryAttack = m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.15;
		EndAttack();
		return;
	}

	if ( !m_pPlayer->m_rgAmmo[ m_iSecondaryAmmoType ] )
	{
		m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;
		EndAttack();
		return;
	}

	if ( pev->dmgtime == 0 )
		PLAYBACK_EVENT_FULL( FEV_NOTHOST, m_pPlayer->edict(), m_usFlameFire, 0.0, (float *)&m_pPlayer->pev->origin, (float *)&m_pPlayer->pev->angles, 0.0, 0.0, 1, 0, 0, 0 );

#ifndef CLIENT_DLL
	m_pPlayer->m_iWeaponVolume = FLAMETHROWER_PRIMARY_VOLUME;

	Vector vecSrc;
	vecSrc = m_pPlayer->GetGunPosition( );
	UTIL_MakeVectors( m_pPlayer->pev->v_angle );
	vecSrc = vecSrc + gpGlobals->v_forward * 16 + gpGlobals->v_up * -8 + gpGlobals->v_right * 3;

	CBaseEntity *pFlame = CBaseEntity::Create( "fly_flame", vecSrc, g_vecZero, m_pPlayer->edict() );
	pFlame->pev->velocity = gpGlobals->v_forward * 240;
	pFlame->pev->weapons = 1;	// acid
	pFlame->pev->movetype = MOVETYPE_TOSS;
	pFlame->pev->gravity = 0.1;
#endif

	if ( pev->dmgtime < gpGlobals->time )
	{
		m_pPlayer->m_rgAmmo[ m_iSecondaryAmmoType ]--;
		pev->dmgtime = gpGlobals->time + 0.2;
	}

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.0;
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.05;
	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 1;
}

void CFlameThrower::WeaponIdle( void )
{
	EndAttack();
	ResetEmptySound( );

	if ( m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
		return;

	int iAnim;

	float flRand = RANDOM_FLOAT(0,1);

	if ( flRand <= 0.5 )
	{
		iAnim = EGON_IDLE1;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + RANDOM_FLOAT(10,15);
	}
	else
	{
		iAnim = EGON_FIDGET1;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 3;
	}

	SendWeaponAnim( iAnim );
}

void CFlameThrower::EndAttack( void )
{
	if ( pev->dmgtime != 0 )
	{
		PLAYBACK_EVENT_FULL( FEV_GLOBAL | FEV_RELIABLE | FEV_NOTHOST, m_pPlayer->edict(), m_usFlameStop, 0, (float *)&g_vecZero, (float *)&g_vecZero, 0.0, 0.0, 0, 0, 0, 0 );
		pev->dmgtime = 0;
	}
}

#ifndef CLIENT_DLL

class CAcidAmmo : public CBasePlayerAmmo	// name ours
{
	void Spawn( void )
	{
		Precache( );
		SET_MODEL(ENT(pev), "models/w_weaponbox.mdl");
		pev->renderfx = kRenderFxGlowShell;
		pev->rendercolor.y = 255;
		CBasePlayerAmmo::Spawn( );
	}
	void Precache( void )
	{
		PRECACHE_MODEL ("models/w_weaponbox.mdl");
		PRECACHE_SOUND("items/9mmclip1.wav");
	}
	BOOL AddAmmo( CBaseEntity *pOther )
	{
		if (pOther->GiveAmmo( 30, "acid", ACID_MAX_CARRY ) != -1)
		{
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM);
			return TRUE;
		}
		return FALSE;
	}
};
LINK_ENTITY_TO_CLASS( ammo_acidtank, CAcidAmmo );

#endif	// CLIENT_DLL

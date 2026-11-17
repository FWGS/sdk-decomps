/***
*
*	Copyright (c) 1996-2001, Valve LLC. All rights reserved.
*
*	This product contains software technology licensed from Id
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc.
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/
//=========================================================
// repulsor.cpp - Scientist Slaughterhouse's repulsor: repulsor_beam
// and weapon_repulsor (file name ours)
//=========================================================

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"
#include "effects.h"
#include "gamerules.h"


#define	REPULSOR_FIRE_VOLUME	450	// name ours: how loud the repulsor is (the gauss's primary fire volume)

// the repulsor uses the gauss's models; names ours
enum repulsor_e {
	REPULSOR_IDLE = 0,
	REPULSOR_IDLE2,
	REPULSOR_FIDGET,
	REPULSOR_SPINUP,
	REPULSOR_SPIN,
	REPULSOR_FIRE,
	REPULSOR_FIRE2,
	REPULSOR_HOLSTER,
	REPULSOR_DRAW
};

#ifndef CLIENT_DLL

//=========================================================
// repulsor_beam: the green spark both attacks fire. It flies
// at 1000 units/s; what it hits that can be repulsed takes its
// velocity and the beam dies, off the rest it bounces (4 to 8
// times). The secondary attack's beam also does 35 damage and
// dies on the first thing it hurts.
//=========================================================
class CRepulsorBeam : public CBaseEntity
{
public:
	void Spawn( void );
	void Precache( void );
	int  Classify( void );
	BOOL CanBeRepulsed( bool fForce ) { return TRUE; }
	const char *GetKillDesc( void ) { return "a magneto-gravitic repulsor"; }

	static CRepulsorBeam *BeamCreate( void );	// name ours

private:
	void EXPORT BeamTouch( CBaseEntity *pOther );

	int m_iTrail;	// name ours
};
LINK_ENTITY_TO_CLASS( repulsor_beam, CRepulsorBeam );

CRepulsorBeam *CRepulsorBeam::BeamCreate( void )
{
	// Create a new entity with CRepulsorBeam private data
	CRepulsorBeam *pBeam = GetClassPtr( (CRepulsorBeam *)NULL );
	pBeam->pev->classname = MAKE_STRING("repulsor_beam");
	pBeam->Spawn();

	return pBeam;
}

void CRepulsorBeam::Spawn( void )
{
	Precache( );
	pev->movetype = MOVETYPE_BOUNCEMISSILE;
	pev->solid = SOLID_BBOX;

	SET_MODEL( ENT(pev), "sprites/xspark4.spr" );
	pev->rendermode = kRenderTransAdd;
	pev->rendercolor = Vector( 0, 255, 0 );
	pev->renderamt = 255;

	// the number of bounces
	pev->health = RANDOM_LONG( 4, 8 );

	UTIL_SetOrigin( pev, pev->origin );
	UTIL_SetSize( pev, Vector( 0, 0, 0 ), Vector( 0, 0, 0 ) );

	SetTouch( BeamTouch );

	MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
		WRITE_BYTE( TE_BEAMFOLLOW );
		WRITE_SHORT( entindex() );	// entity
		WRITE_SHORT( m_iTrail );	// model
		WRITE_BYTE( 5 ); // life
		WRITE_BYTE( 5 );  // width
		WRITE_BYTE( 0 );   // r, g, b
		WRITE_BYTE( 255 );   // r, g, b
		WRITE_BYTE( 0 );   // r, g, b
		WRITE_BYTE( 255 );	// brightness
	MESSAGE_END();
}

void CRepulsorBeam::Precache( void )
{
	PRECACHE_MODEL( "sprites/xspark4.spr" );
	m_iTrail = PRECACHE_MODEL( "sprites/xenobeam.spr" );
}

int CRepulsorBeam::Classify( void )
{
	return CLASS_NONE;
}

void CRepulsorBeam::BeamTouch( CBaseEntity *pOther )
{
	BOOL fHit = FALSE;

	pev->health -= 1;

	if ( pev->dmg != 0 && pOther->pev->takedamage != DAMAGE_NO )
	{
		pOther->TakeDamage( pev, VARS( pev->owner ), pev->dmg, DMG_NEVERGIB | DMG_FALL );
		fHit = TRUE;
	}

	if ( pOther->CanBeRepulsed( false ) )
	{
		// push it along
		pOther->pev->velocity = pev->velocity;
		fHit = TRUE;
	}

	if ( fHit || pev->health <= 0 )
		Killed( pev, GIB_NEVER );
}

#endif

LINK_ENTITY_TO_CLASS( weapon_repulsor, CRepulsor );

void CRepulsor::Spawn( void )
{
	Precache( );
	m_iId = WEAPON_REPULSOR;
	SET_MODEL( ENT(pev), "models/w_gauss.mdl" );

	pev->renderfx = kRenderFxGlowShell;
	pev->rendercolor = Vector( 0, 255, 0 );

	m_iDefaultAmmo = REPULSOR_MAX_CARRY;

#ifndef CLIENT_DLL
	if ( !g_pGameRules->IsMultiplayer() )
		pev->spawnflags |= SF_NORESPAWN;
#endif

	FallInit();// get ready to fall down.
}

void CRepulsor::Precache( void )
{
	PRECACHE_MODEL( "models/w_gauss.mdl" );
	PRECACHE_MODEL( "models/v_gauss.mdl" );
	PRECACHE_MODEL( "models/p_gauss.mdl" );

	PRECACHE_SOUND( "weapons/ric1.wav" );
	PRECACHE_SOUND( "items/9mmclip1.wav" );

	UTIL_PrecacheOther( "repulsor_beam" );
	UTIL_PrecacheOther( "env_tractorbeam" );

	m_usRepulsor = PRECACHE_EVENT( 1, "events/repulsor.sc" );
}

int CRepulsor::AddToPlayer( CBasePlayer *pPlayer )
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

int CRepulsor::GetItemInfo( ItemInfo *p )
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "rep";
	p->iMaxAmmo1 = REPULSOR_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = 1;
	p->iPosition = 4;
	p->iId = m_iId = WEAPON_REPULSOR;
	p->iFlags = ITEM_FLAG_NOAUTOSWITCHEMPTY | ITEM_FLAG_NOAUTORELOAD;
	p->iWeight = REPULSOR_WEIGHT;

	return 1;
}

BOOL CRepulsor::Deploy( void )
{
	return DefaultDeploy( "models/v_gauss.mdl", "models/p_gauss.mdl", REPULSOR_DRAW, "gauss" );
}

void CRepulsor::Holster( int skiplocal /* = 0 */ )
{
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;
	SendWeaponAnim( REPULSOR_HOLSTER );

	//!!!HACKHACK - can't select the repulsor if it's empty! (as the hornet gun)
	if ( !m_pPlayer->m_rgAmmo[ PrimaryAmmoIndex() ] )
	{
		m_pPlayer->m_rgAmmo[ PrimaryAmmoIndex() ] = 1;
	}
}

void CRepulsor::PrimaryAttack( void )
{
#ifndef CLIENT_DLL
	// with the tractor beam on, the primary attack pulls the held entity in
	// further: 4 units more, from 1440 back to 128
	if ( m_pTractorBeam )
	{
		char szText[ 48 ];

		pev->frags += 4;
		if ( pev->frags > 1440 )
			pev->frags = 128;

		sprintf( szText, "Tractor beam length set to %.2f meters", pev->frags * 0.0254 );
		ClientPrint( m_pPlayer->pev, HUD_PRINTCENTER, szText );
		return;
	}
#else
	// the server sends m_chargeReady while the tractor beam is on
	if ( m_chargeReady )
		return;
#endif

	// don't fire underwater
	if ( m_pPlayer->pev->waterlevel == 3 )
	{
		PlayEmptySound( );
		m_flNextSecondaryAttack = m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.15;
		return;
	}

	if ( !m_pPlayer->m_rgAmmo[ m_iPrimaryAmmoType ] )
	{
		m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;
		return;
	}

	int flags;
#if defined( CLIENT_WEAPONS )
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif

	PLAYBACK_EVENT( flags, m_pPlayer->edict(), m_usRepulsor );

#ifndef CLIENT_DLL
	m_pPlayer->m_iWeaponVolume = REPULSOR_FIRE_VOLUME;

	// player "shoot" animation
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

	Vector vecAngles;
	vecAngles = m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle;
	UTIL_MakeVectors( vecAngles );
	Vector vecSrc = m_pPlayer->GetGunPosition( ) + gpGlobals->v_up * -10 + gpGlobals->v_right * 7;

	CRepulsorBeam *pBeam = CRepulsorBeam::BeamCreate();
	pBeam->pev->origin = vecSrc;
	pBeam->pev->owner = m_pPlayer->edict();
	pBeam->pev->velocity = gpGlobals->v_forward * 1000;
	pBeam->pev->speed = 1000;
#endif

	m_pPlayer->m_rgAmmo[ m_iPrimaryAmmoType ]--;

	m_flRechargeTime = UTIL_WeaponTimeBase() + 0.5;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.0;
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.3;
}

void CRepulsor::SecondaryAttack( void )
{
#ifndef CLIENT_DLL
	// with the tractor beam on, the secondary attack lets the held entity
	// out: 4 units less, down to 128
	if ( m_pTractorBeam )
	{
		char szText[ 48 ];

		pev->frags -= 4;
		if ( pev->frags < 128 )
			pev->frags = 128;

		sprintf( szText, "Tractor beam length set to %.2f meters", pev->frags * 0.0254 );
		ClientPrint( m_pPlayer->pev, HUD_PRINTCENTER, szText );
		return;
	}
#else
	if ( m_chargeReady )
		return;
#endif

	// don't fire underwater
	if ( m_pPlayer->pev->waterlevel == 3 )
	{
		PlayEmptySound( );
		m_flNextSecondaryAttack = m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.15;
		return;
	}

	if ( m_pPlayer->m_rgAmmo[ m_iPrimaryAmmoType ] < 2 )
	{
		m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;
		return;
	}

	int flags;
#if defined( CLIENT_WEAPONS )
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif

	PLAYBACK_EVENT( flags, m_pPlayer->edict(), m_usRepulsor );

#ifndef CLIENT_DLL
	m_pPlayer->m_iWeaponVolume = REPULSOR_FIRE_VOLUME;

	// player "shoot" animation
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );
	SendWeaponAnim( REPULSOR_FIRE );

	// sic: the beam's start uses the vectors of the previous UTIL_MakeVectors
	Vector vecAngles;
	Vector vecSrc = m_pPlayer->GetGunPosition( ) + gpGlobals->v_up * -10 + gpGlobals->v_right * 7;
	vecAngles = m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle;
	UTIL_MakeVectors( vecAngles );

	CRepulsorBeam *pBeam = CRepulsorBeam::BeamCreate();
	pBeam->pev->origin = vecSrc;
	pBeam->pev->owner = m_pPlayer->edict();
	pBeam->pev->velocity = gpGlobals->v_forward * 1000;
	pBeam->pev->speed = 1000;
	pBeam->pev->dmg = 35;
#endif

	m_pPlayer->m_rgAmmo[ m_iPrimaryAmmoType ] -= 2;

	m_flRechargeTime = UTIL_WeaponTimeBase() + 0.5;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.0;
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.3;
}

//=========================================================
// WeaponSpecial - the tractor beam: locks on the entity
// within 1440 units that can be tractored and lies closest
// to the aim (within 25 degrees, in sight), or lets go
//=========================================================
void CRepulsor::WeaponSpecial( void )
{
	m_flNextSecondaryAttack = m_flNextPrimaryAttack = m_flNextWeaponSpecial = UTIL_WeaponTimeBase() + 1;

#ifndef CLIENT_DLL
	if ( !m_pTractorBeam )
	{
		float flBestDot = VIEW_FIELD_FULL;
		Vector vecSrc = m_pPlayer->GetGunPosition( );

		UTIL_MakeVectors( m_pPlayer->pev->v_angle );

		CBaseEntity *pEntity = NULL;
		while ( ( pEntity = UTIL_FindEntityInSphere( pEntity, vecSrc, 1440 ) ) != NULL )
		{
			if ( pEntity->CanBeTractored() && pEntity != m_pPlayer )
			{
				Vector vecDir = ( pEntity->Center() - vecSrc ).Normalize();
				float flDot = DotProduct( vecDir, gpGlobals->v_forward );

				if ( flDot > VIEW_FIELD_ULTRA_NARROW && flDot > flBestDot )
				{
					TraceResult tr;

					UTIL_TraceLine( vecSrc, pEntity->pev->origin, ignore_monsters, m_pPlayer->edict(), &tr );
					if ( tr.flFraction >= 1 || tr.pHit == pEntity->edict() )
					{
						flBestDot = flDot;
						m_hTarget = pEntity;
					}
				}
			}
		}

		if ( m_hTarget )
		{
			m_pTractorBeam = CBaseEntity::Create( "env_tractorbeam", m_hTarget->Center(), pev->angles, m_pPlayer->edict() );
			m_pTractorBeam->pev->playerclass = 1;
			m_pTractorBeam->pev->spawnflags |= SF_BEAM_TEMPORARY;
			((CTractorBeam *)m_pTractorBeam)->m_hTarget = m_hTarget;

			// the beam's length: from 128 to 1440
			pev->frags = ( m_pTractorBeam->pev->origin - vecSrc ).Length();
			if ( pev->frags < 128 )
				pev->frags = 128;
			else if ( pev->frags > 1440 )
				pev->frags = 1440;

			m_pTractorBeam->pev->fuser1 = m_hTarget->GetHoldDistance();
			if ( m_pTractorBeam->pev->fuser1 < 4 )
				m_pTractorBeam->pev->fuser1 = 4;

			ClientPrint( m_pPlayer->pev, HUD_PRINTCENTER, "Locked tractor beam on %s", STRING( m_hTarget->pev->classname ) );
		}
		else
		{
			ClientPrint( m_pPlayer->pev, HUD_PRINTCENTER, "Unable to lock tractor beam" );
		}
	}
	else
	{
		m_pTractorBeam->Killed( pev, GIB_NEVER );
		m_pTractorBeam = NULL;
		m_hTarget = NULL;
	}
#endif
}

void CRepulsor::WeaponIdle( void )
{
#ifndef CLIENT_DLL
	m_chargeReady = TractorThink();
	if ( m_chargeReady )
		return;
#else
	if ( TractorThink() )
		return;
#endif

	Reload( );

	if ( m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
		return;

	int iAnim;
	float flRand = RANDOM_FLOAT( 0, 1 );
	if ( flRand <= 0.5 )
	{
		iAnim = REPULSOR_IDLE;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + RANDOM_FLOAT( 10, 15 );
	}
	else if ( flRand <= 0.75 )
	{
		iAnim = REPULSOR_IDLE2;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + RANDOM_FLOAT( 10, 15 );
	}
	else
	{
		iAnim = REPULSOR_FIDGET;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 3;
	}

	SendWeaponAnim( iAnim );
}

//=========================================================
// Reload - the repulsor recharges: one shot every half
// second, up to 16
//=========================================================
void CRepulsor::Reload( void )
{
	while ( m_pPlayer->m_rgAmmo[ m_iPrimaryAmmoType ] < REPULSOR_MAX_CARRY && m_flRechargeTime < gpGlobals->time )
	{
		m_pPlayer->m_rgAmmo[ m_iPrimaryAmmoType ]++;
		m_flRechargeTime += 0.5;
	}
}

//=========================================================
// TractorThink - pulls the tractor beam's target toward the
// point pev->frags units in front of the gun; lets go when
// the target is gone. TRUE while the beam holds something.
//=========================================================
bool CRepulsor::TractorThink( void )
{
#ifndef CLIENT_DLL
	if ( !m_pTractorBeam )
		return FALSE;

	if ( m_hTarget == NULL )
	{
		m_pTractorBeam->Killed( pev, GIB_NEVER );
		m_pTractorBeam = NULL;
		m_hTarget = NULL;
		return FALSE;
	}

	Vector vecAngles;
	vecAngles = m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle;
	UTIL_MakeVectors( vecAngles );

	Vector vecDest = m_pPlayer->GetGunPosition( ) + gpGlobals->v_forward * pev->frags;
	Vector vecDir = vecDest - m_hTarget->Center();

	m_hTarget->pev->avelocity = g_vecZero;
	m_hTarget->pev->flags &= ~FL_ONGROUND;

	float flDist = vecDir.Length();

	// close enough: hold it there
	if ( flDist < m_pTractorBeam->pev->fuser1 )
	{
		m_hTarget->pev->velocity = g_vecZero;
		return TRUE;
	}

	if ( flDist < m_pTractorBeam->pev->fuser1 * 2 )
		vecDir = vecDir.Normalize() * ( m_pTractorBeam->pev->fuser1 * 2 );

	m_hTarget->pev->velocity = m_hTarget->pev->velocity * 0.2 + vecDir;
	return TRUE;
#else
	return m_chargeReady != 0;
#endif
}

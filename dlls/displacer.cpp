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
/*

  The displacer (weapon_displacer) spins up for a second and then fires,
  for 20 uranium a shot: the primary fire a displacer_ball that throws
  lightning around and makes a displacement wave where it lands; the
  secondary fire an energy_disk that seeks the monster nearest to the aim.

*/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"
#include "effects.h"

enum displacer_e {
	DISPLACER_IDLE1 = 0,
	DISPLACER_IDLE2,
	DISPLACER_SPINUP,
	DISPLACER_SPIN,
	DISPLACER_FIRE,
	DISPLACER_DRAW,
	DISPLACER_HOLSTER1
};

#define DISPLACER_DEFAULT_GIVE		100	// names ours
#define DISPLACER_AMMO_PER_SHOT		20

#ifndef CLIENT_DLL

// displacer_ball: flies at 1000 units/s flashing lightning at random spots around it
// (two sets of four beams, alternating); what it touches takes 250, and if that
// kills a monster that can be teleported, it vanishes. 0.6 seconds later a
// displacement wave: 250 damage in a radius of 300.
class CDisplacerBall : public CSprite
{
public:
	void Spawn( void );
	void Precache( void );

	// the ball in flight is solid, the wave after it lands is not
	const char *GetKillDesc( void ) { return pev->solid ? "a displacer ball" : "a displacement wave"; }
	const char *GetKillVerb( BOOL fGibbed, int &iHitGroup ) { return pev->solid ? "displaced" : DamageVerb( DMG_GENERIC, fGibbed ); }
	const char *GetHitGroupDesc( int iHitGroup ) { return pev->solid ? " to an undefined location" : ""; }

	void EXPORT FlyThink( void );
	void EXPORT BlowThink( void );
	void EXPORT DisplaceTouch( CBaseEntity *pOther );

	int		m_iRingSprite;	// names ours
	CBeam	*m_pBeam[8];	// pev->weapons picks the set of four in use
};

LINK_ENTITY_TO_CLASS( displacer_ball, CDisplacerBall );

void CDisplacerBall::Spawn( void )
{
	Precache( );

	pev->solid = SOLID_BBOX;
	pev->movetype = MOVETYPE_FLY;
	pev->renderamt = 255;
	pev->rendermode = kRenderTransAdd;

	SET_MODEL( ENT( pev ), "sprites/exit1.spr" );
	UTIL_SetSize( pev, Vector( -4, -4, -4 ), Vector( 4, 4, 4 ) );
	m_maxFrame = (float) MODEL_FRAMES( pev->modelindex ) - 1;

	EMIT_SOUND( ENT( pev ), CHAN_WEAPON, "weapons/displacer_start.wav", 1, 0.8 );

	pev->framerate = 10;
	TurnOn( );

	SetThink( FlyThink );
	SetTouch( DisplaceTouch );
}

void CDisplacerBall::Precache( void )
{
	PRECACHE_MODEL( "sprites/exit1.spr" );
	m_iRingSprite = PRECACHE_MODEL( "sprites/disp_ring.spr" );
	PRECACHE_MODEL( "sprites/b-tele1.spr" );
	PRECACHE_SOUND( "weapons/displacer_start.wav" );
	PRECACHE_SOUND( "weapons/displacer_impact.wav" );
	PRECACHE_SOUND( "weapons/displacer_teleport.wav" );
}

void CDisplacerBall::FlyThink( void )
{
	AnimateThink( );

	pev->weapons = !pev->weapons;

	for ( int i = 0; i < 4; i++ )
	{
		TraceResult tr;
		Vector vecDir = Vector( RANDOM_FLOAT( -1, 1 ), RANDOM_FLOAT( -1, 1 ), RANDOM_FLOAT( -1, 1 ) );
		UTIL_TraceLine( pev->origin, pev->origin + vecDir * 1024, ignore_monsters, ENT( pev ), &tr );

		if ( tr.fAllSolid )
			continue;

		CBeam *pBeam = m_pBeam[i + pev->weapons * 4];

		if ( !pBeam )
		{
			m_pBeam[i + pev->weapons * 4] = CBeam::BeamCreate( "sprites/lgtning.spr", 48 );
			pBeam = m_pBeam[i + pev->weapons * 4];
			pBeam->PointEntInit( tr.vecEndPos, entindex( ) );
			pBeam->SetBrightness( 100 );
			pBeam->SetColor( 192, 255, 64 );
			pBeam->SetNoise( 48 );
			pBeam->SetScrollRate( 35 );
			pBeam->pev->spawnflags |= SF_BEAM_TEMPORARY;
		}
		else
		{
			pBeam->SetStartPos( tr.vecEndPos );
		}

		pBeam->SetThink( SUB_Remove );
		pBeam->pev->nextthink = gpGlobals->time + 4;
	}
}

void CDisplacerBall::BlowThink( void )
{
	entvars_t *pevOwner = pev;
	if ( pev->owner )
		pevOwner = VARS( pev->owner );
	pev->owner = NULL;	// so the owner takes damage too

	::RadiusDamage( pev->origin, pev, pevOwner, 250, 300, CLASS_NONE, DMG_ENERGYBEAM | DMG_ALWAYSGIB );
	EMIT_SOUND( ENT( pev ), CHAN_WEAPON, "weapons/displacer_teleport.wav", 1, 0.8 );

	SetThink( SUB_Remove );
	pev->nextthink = gpGlobals->time + 0.1;

	for ( int i = 0; i < 8; i++ )
	{
		UTIL_Remove( m_pBeam[i] );
		m_pBeam[i] = NULL;
	}
}

void CDisplacerBall::DisplaceTouch( CBaseEntity *pOther )
{
	pev->nextthink = gpGlobals->time + 0.6;

	if ( pOther->pev->takedamage )
	{
		pOther->TakeDamage( pev, VARS( pev->owner ), 250, DMG_ENERGYBEAM | DMG_FADE | DMG_NOPUSH );

		if ( pOther->CanBeTeleported( ) && pOther->pev->health <= 0 )
		{
			// gone just before the wave
			pOther->SetThink( SUB_Remove );
			pOther->pev->nextthink = pev->nextthink - 0.05;
		}
	}

	SetTouch( NULL );
	pev->velocity = g_vecZero;
	pev->solid = SOLID_NOT;

	EMIT_SOUND( ENT( pev ), CHAN_WEAPON, "weapons/displacer_impact.wav", 1, 0.8 );

	MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
		WRITE_BYTE( TE_BEAMCYLINDER );
		WRITE_COORD( pev->origin.x );
		WRITE_COORD( pev->origin.y );
		WRITE_COORD( pev->origin.z );
		WRITE_COORD( pev->origin.x );
		WRITE_COORD( pev->origin.y );
		WRITE_COORD( pev->origin.z + 750 );
		WRITE_SHORT( m_iRingSprite );
		WRITE_BYTE( 0 ); // startframe
		WRITE_BYTE( 0 ); // framerate
		WRITE_BYTE( 4 ); // life
		WRITE_BYTE( 25 );  // width
		WRITE_BYTE( 0 );   // noise
		WRITE_BYTE( 255 );   // r, g, b
		WRITE_BYTE( 255 );   // r, g, b
		WRITE_BYTE( 255 );   // r, g, b
		WRITE_BYTE( 255 );	// brightness
		WRITE_BYTE( 0 );		// speed
	MESSAGE_END();

	MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, pev->origin );
		WRITE_BYTE( TE_DLIGHT );
		WRITE_COORD( pev->origin.x );	// X
		WRITE_COORD( pev->origin.y );	// Y
		WRITE_COORD( pev->origin.z );	// Z
		WRITE_BYTE( 14 );		// radius * 0.1
		WRITE_BYTE( 192 );		// r
		WRITE_BYTE( 255 );		// g
		WRITE_BYTE( 64 );		// b
		WRITE_BYTE( 8 );		// time * 10
		WRITE_BYTE( 10 );		// decay * 0.1
	MESSAGE_END( );

	SetThink( BlowThink );
}

#endif

LINK_ENTITY_TO_CLASS( weapon_displacer, CDisplacer );

void CDisplacer::Spawn( )
{
	Precache( );
	m_iId = WEAPON_DISPLACER;
	SET_MODEL( ENT( pev ), "models/w_displacer.mdl" );

	m_iDefaultAmmo = DISPLACER_DEFAULT_GIVE;

	FallInit();// get ready to fall down.
}

int CDisplacer::AddToPlayer( CBasePlayer *pPlayer )
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

void CDisplacer::Precache( void )
{
	PRECACHE_MODEL( "models/w_displacer.mdl" );
	PRECACHE_MODEL( "models/v_displacer.mdl" );
	PRECACHE_MODEL( "models/p_displacer.mdl" );

	PRECACHE_SOUND( "weapons/displacer_fire.wav" );
	PRECACHE_SOUND( "weapons/displacer_spin.wav" );

	UTIL_PrecacheOther( "displacer_ball" );
	UTIL_PrecacheOther( "energy_disk" );

	m_usDispSpin = PRECACHE_EVENT( 1, "events/disp_spin.sc" );
	m_usDispFire = PRECACHE_EVENT( 1, "events/disp_fire.sc" );
}

int CDisplacer::GetItemInfo( ItemInfo *p )
{
	p->pszName = STRING( pev->classname );
	p->pszAmmo1 = "uranium";
	p->iMaxAmmo1 = URANIUM_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = 5;
	p->iPosition = 1;
	p->iId = WEAPON_DISPLACER;
	p->iFlags = 0;
	p->iWeight = DISPLACER_WEIGHT;

	return 1;
}

BOOL CDisplacer::Deploy( )
{
	return DefaultDeploy( "models/v_displacer.mdl", "models/p_displacer.mdl", DISPLACER_DRAW, "gauss" );
}

void CDisplacer::Holster( int skiplocal /* = 0 */ )
{
	m_fInReload = FALSE;// cancel any reload in progress.

	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase( ) + 0.5;
	SendWeaponAnim( DISPLACER_HOLSTER1 );
	pev->frags = 0;
}

// the attacks only spin up (disp_spin event); WeaponIdle fires a second later
void CDisplacer::PrimaryAttack( void )
{
	// don't fire underwater
	if ( m_pPlayer->pev->waterlevel == 3 )
	{
		PlayEmptySound( );
		m_flNextSecondaryAttack = m_flNextPrimaryAttack = UTIL_WeaponTimeBase( ) + 0.15;
		return;
	}

	if ( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] < DISPLACER_AMMO_PER_SHOT )
	{
		PlayEmptySound( );
		m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase( ) + 0.5;
		return;
	}

	m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;

	PLAYBACK_EVENT_FULL( FEV_NOTHOST, m_pPlayer->edict( ), m_usDispSpin, 0.0, (float *)&m_pPlayer->pev->origin, (float *)&g_vecZero, 0.0, 0.0, 0, 0, 0, 0 );

	m_flNextPrimaryAttack = m_flNextSecondaryAttack = UTIL_WeaponTimeBase( ) + 2;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase( ) + 1;
	pev->frags = 1;
}

void CDisplacer::SecondaryAttack( void )
{
	// don't fire underwater
	if ( m_pPlayer->pev->waterlevel == 3 )
	{
		PlayEmptySound( );
		m_flNextSecondaryAttack = m_flNextPrimaryAttack = UTIL_WeaponTimeBase( ) + 0.15;
		return;
	}

	if ( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] < DISPLACER_AMMO_PER_SHOT )
	{
		PlayEmptySound( );
		m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase( ) + 0.5;
		return;
	}

	m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;

	PLAYBACK_EVENT_FULL( FEV_NOTHOST, m_pPlayer->edict( ), m_usDispSpin, 0.0, (float *)&m_pPlayer->pev->origin, (float *)&g_vecZero, 0.0, 0.0, 0, 0, 1, 0 );

	pev->frags = 2;
	m_flNextPrimaryAttack = m_flNextSecondaryAttack = UTIL_WeaponTimeBase( ) + 2;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase( ) + 1;
}

void CDisplacer::WeaponIdle( void )
{
	Vector vecAiming = m_pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );

	ResetEmptySound( );

	if ( m_flTimeWeaponIdle < UTIL_WeaponTimeBase( ) )
	{
		if ( pev->frags == 1 )
		{
			m_pPlayer->SetAnimation( PLAYER_ATTACK1 );
			m_pPlayer->m_iWeaponVolume = LOUD_GUN_VOLUME;

			PLAYBACK_EVENT_FULL( FEV_NOTHOST, m_pPlayer->edict( ), m_usDispFire, 0.0, (float *)&m_pPlayer->pev->origin, (float *)&g_vecZero, 0.0, 0.0, 0, 0, 0, 0 );

#ifndef CLIENT_DLL
			Vector anglesAim = m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle;
			UTIL_MakeVectors( anglesAim );

			Vector vecSrc = m_pPlayer->GetGunPosition( ) + gpGlobals->v_up * -8 + gpGlobals->v_forward * 8;

			CBaseEntity *pBall = CBaseEntity::Create( "displacer_ball", vecSrc, g_vecZero, m_pPlayer->edict( ) );
			pBall->pev->velocity = vecAiming * 1000;
#endif

			m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] -= DISPLACER_AMMO_PER_SHOT;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase( ) + 1.0;
			pev->frags = 0;
		}
		else if ( pev->frags == 2 )
		{
			m_pPlayer->SetAnimation( PLAYER_ATTACK1 );
			m_pPlayer->m_iWeaponVolume = LOUD_GUN_VOLUME;

			PLAYBACK_EVENT_FULL( FEV_NOTHOST, m_pPlayer->edict( ), m_usDispFire, 0.0, (float *)&m_pPlayer->pev->origin, (float *)&g_vecZero, 0.0, 0.0, 0, 0, 0, 0 );

#ifndef CLIENT_DLL
			Vector vecSrc = m_pPlayer->GetGunPosition( ) + gpGlobals->v_up * -12 + gpGlobals->v_forward * 32;

			CBaseMonster *pDisk = (CBaseMonster *)CBaseEntity::Create( "energy_disk", vecSrc, m_pPlayer->pev->v_angle, m_pPlayer->edict( ) );
			pDisk->pev->angles.x = -m_pPlayer->pev->v_angle.x;
			pDisk->pev->dmg = 100;
			pDisk->pev->speed = 512;
			pDisk->m_hActivator = m_pPlayer;

			// the disk homes in on the visible enemy closest to the aim, within 25 degrees and 1024 units
			float flBest = -1;
			UTIL_MakeVectors( m_pPlayer->pev->v_angle );

			CBaseEntity *pEntity = NULL;
			while ( ( pEntity = UTIL_FindEntityInSphere( pEntity, vecSrc, 1024 ) ) != NULL )
			{
				if ( pEntity != m_pPlayer && pEntity->pev->size.z >= 32 && pEntity->pev->takedamage && m_pPlayer->IRelationship( pEntity ) >= R_DL )
				{
					float flDot = DotProduct( ( pEntity->Center( ) - vecSrc ).Normalize( ), gpGlobals->v_forward );

					if ( flDot > VIEW_FIELD_ULTRA_NARROW && flDot > flBest )
					{
						TraceResult tr;
						UTIL_TraceLine( vecSrc, pEntity->pev->origin, ignore_monsters, m_pPlayer->edict( ), &tr );

						if ( tr.flFraction >= 1 || tr.pHit == pEntity->edict( ) )
						{
							flBest = flDot;
							pDisk->m_hEnemy = pEntity;
						}
					}
				}
			}

			pDisk->pev->velocity = gpGlobals->v_forward;
#endif

			m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] -= DISPLACER_AMMO_PER_SHOT;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase( ) + 1.0;
			pev->frags = 0;
		}
	}
}

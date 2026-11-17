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
// reflector.cpp - Scientist Slaughterhouse's reflector,
// weapon_reflector (server only; file name ours)
//=========================================================

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"
#include "effects.h"
#include "decals.h"

#define REFLECTOR_PRIMARY_VOLUME	450		// names ours from here
#define REFLECTOR_DEFAULT_GIVE		100
#define REFLECTOR_BEAM_SPRITE		"sprites/cutbeam.spr"
#define REFLECTOR_LASER_SPRITE		"sprites/laserbeam.spr"
#define REFLECTOR_SOUND_RUN			"ambience/screammachine.wav"

// the reflector uses the gauss gun's models
enum reflector_e {
	REFLECTOR_IDLE = 0,
	REFLECTOR_IDLE2,
	REFLECTOR_FIDGET,
	REFLECTOR_SPINUP,
	REFLECTOR_SPIN,
	REFLECTOR_FIRE,
	REFLECTOR_FIRE2,
	REFLECTOR_HOLSTER,
	REFLECTOR_DRAW
};

class CReflector : public CBasePlayerWeapon
{
public:
	int		Save( CSave &save );
	int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	void Spawn( void );
	void Precache( void );
	int iItemSlot( void ) { return 1; }
	const char *GetKillDesc( void ) { return "a laser beam"; }
	const char *GetKillVerb( BOOL fGibbed, int &iHitGroup ) { return "shot"; }
	const char *GetHitGroupDesc( int iHitGroup ) { return HitGroupIn( iHitGroup ); }
	int GetItemInfo(ItemInfo *p);
	int AddToPlayer( CBasePlayer *pPlayer );

	BOOL Deploy( void );
	void Holster( int skiplocal = 0 );

	void PrimaryAttack( void );
	void SecondaryAttack( void );
	void WeaponIdle( void );

	void Fire( const Vector &vecOrigSrc, const Vector &vecAngles );		// names ours from here
	void FireWide( const Vector &vecOrigSrc, const Vector &vecAngles );
	void EndAttack( void );
	void UpdateBeam( int iBeam, const Vector &vecSrc, const Vector &vecEnd, bool fFirst, bool fWide );
	void RemoveBeam( int iBeam );

	void UseAmmo( int count )
	{
		if ( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] >= count )
			m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] -= count;
		else
			m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] = 0;
	}

	float	m_flAmmoUseTime;	// since we use < 1 point of ammo per update, we subtract ammo on a timer.
	CBeam	*m_pBeams[15];		// five beams of up to three segments each
	int		m_shootTime;		// the run sound still has to start
	int		m_fireState;
};

LINK_ENTITY_TO_CLASS( weapon_reflector, CReflector );

TYPEDESCRIPTION	CReflector::m_SaveData[] =
{
	DEFINE_ARRAY( CReflector, m_pBeams, FIELD_CLASSPTR, 15 ),
	DEFINE_FIELD( CReflector, m_shootTime, FIELD_INTEGER ),
	DEFINE_FIELD( CReflector, m_fireState, FIELD_INTEGER ),
	DEFINE_FIELD( CReflector, m_flAmmoUseTime, FIELD_TIME ),
};
IMPLEMENT_SAVERESTORE( CReflector, CBasePlayerWeapon );

void CReflector::Spawn( )
{
	Precache( );
	m_iId = WEAPON_REFLECTOR;
	SET_MODEL(ENT(pev), "models/w_gauss.mdl");

	m_iDefaultAmmo = REFLECTOR_DEFAULT_GIVE;
	pev->rendercolor = Vector( 64, 192, 255 );
	pev->renderfx = kRenderFxGlowShell;

	FallInit();// get ready to fall down.
}


void CReflector::Precache( void )
{
	PRECACHE_MODEL("models/w_gauss.mdl");
	PRECACHE_MODEL("models/v_gauss.mdl");
	PRECACHE_MODEL("models/p_gauss.mdl");

	PRECACHE_SOUND("items/9mmclip1.wav");

	PRECACHE_SOUND( REFLECTOR_SOUND_RUN );

	PRECACHE_SOUND ("weapons/357_cock1.wav");

	PRECACHE_MODEL( REFLECTOR_BEAM_SPRITE );
}


int CReflector::AddToPlayer( CBasePlayer *pPlayer )
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


int CReflector::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "uranium";
	p->iMaxAmmo1 = URANIUM_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = 5;
	p->iPosition = 4;
	p->iId = m_iId = WEAPON_REFLECTOR;
	p->iFlags = 0;
	p->iWeight = REFLECTOR_WEIGHT;

	return 1;
}


BOOL CReflector::Deploy( void )
{
	return DefaultDeploy( "models/v_gauss.mdl", "models/p_gauss.mdl", REFLECTOR_DRAW, "gauss" );
}


void CReflector::Holster( int skiplocal /* = 0 */ )
{
	if ( m_fireState == FIRE_CHARGE )
		EndAttack();

	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;
	SendWeaponAnim( REFLECTOR_HOLSTER );

	EMIT_SOUND( ENT(m_pPlayer->pev), CHAN_WEAPON, "common/null.wav", 1.0, ATTN_NORM );
}


// five reflecting laser beams
void CReflector::PrimaryAttack( void )
{
	// don't fire underwater
	if ( m_pPlayer->pev->waterlevel == 3 )
	{
		if ( m_fireState == FIRE_CHARGE )
			EndAttack();

		PlayEmptySound( );
		return;
	}

	switch( m_fireState )
	{
		case FIRE_OFF:
		{
			if ( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 )
			{
				m_flNextPrimaryAttack = m_flNextSecondaryAttack = gpGlobals->time + 0.25;
				PlayEmptySound( );
				return;
			}

			m_flAmmoUseTime = gpGlobals->time;// start using ammo ASAP.

			SendWeaponAnim( REFLECTOR_SPIN );
			m_shootTime = 1;

			m_pPlayer->m_iWeaponVolume = REFLECTOR_PRIMARY_VOLUME;
			m_flTimeWeaponIdle = gpGlobals->time + 0.1;
			pev->dmgtime = gpGlobals->time + 0.1;
			m_fireState = FIRE_CHARGE;
		}
		break;

		case FIRE_CHARGE:
		{
			Fire( m_pPlayer->GetGunPosition( ), m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle );
			m_pPlayer->m_iWeaponVolume = REFLECTOR_PRIMARY_VOLUME;

			if ( m_shootTime )
			{
				EMIT_SOUND_DYN( ENT(m_pPlayer->pev), CHAN_STATIC, REFLECTOR_SOUND_RUN, 0.98, ATTN_NORM, 0, 125 );
				m_shootTime = 0;
			}

			if ( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 )
			{
				EndAttack();
				m_fireState = FIRE_OFF;
				m_flNextPrimaryAttack = m_flNextSecondaryAttack = gpGlobals->time + 1.0;
			}
		}
		break;
	}
}


// one beam that goes through walls
void CReflector::SecondaryAttack( void )
{
	// don't fire underwater
	if ( m_pPlayer->pev->waterlevel == 3 )
	{
		if ( m_fireState == FIRE_CHARGE )
			EndAttack();

		PlayEmptySound( );
		return;
	}

	switch( m_fireState )
	{
		case FIRE_OFF:
		{
			if ( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 )
			{
				m_flNextPrimaryAttack = m_flNextSecondaryAttack = gpGlobals->time + 0.25;
				PlayEmptySound( );
				return;
			}

			m_flAmmoUseTime = gpGlobals->time;// start using ammo ASAP.

			SendWeaponAnim( REFLECTOR_SPIN );
			m_shootTime = 1;

			m_pPlayer->m_iWeaponVolume = REFLECTOR_PRIMARY_VOLUME;
			m_flTimeWeaponIdle = gpGlobals->time + 0.1;
			pev->dmgtime = gpGlobals->time + 0.1;
			m_fireState = FIRE_CHARGE;
		}
		break;

		case FIRE_CHARGE:
		{
			FireWide( m_pPlayer->GetGunPosition( ), m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle );
			m_pPlayer->m_iWeaponVolume = REFLECTOR_PRIMARY_VOLUME;

			if ( m_shootTime )
			{
				EMIT_SOUND_DYN( ENT(m_pPlayer->pev), CHAN_STATIC, REFLECTOR_SOUND_RUN, 0.98, ATTN_NORM, 0, 125 );
				m_shootTime = 0;
			}

			if ( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 )
			{
				EndAttack();
				m_fireState = FIRE_OFF;
				m_flNextPrimaryAttack = m_flNextSecondaryAttack = gpGlobals->time + 1.0;
			}
		}
		break;
	}
}


// the primary beams' directions, added to the view angles (name ours)
static Vector s_vecReflectorAngles[5] =
{
	Vector( 0, 0, 0 ),
	Vector( 30, 0, 0 ),
	Vector( -30, 0, 0 ),
	Vector( 0, 30, 0 ),
	Vector( 0, -30, 0 ),
};

// 2 damage (DMG_BULLET) per beam and frame; a beam bounces off whatever reflects gauss
// shots unless it is shielded (type 2 and up)
void CReflector::Fire( const Vector &vecOrigSrc, const Vector &vecAngles )
{
	int iBeams = 0;
	edict_t *pentIgnore = ENT( m_pPlayer->pev );
	TraceResult tr;

	// use 6 ammo/second
	if ( gpGlobals->time >= m_flAmmoUseTime )
	{
		UseAmmo( 1 );
		m_flAmmoUseTime = gpGlobals->time + 0.166;
	}

	for ( int i = 0; i < 5; i++ )
	{
		UTIL_MakeVectors( vecAngles + s_vecReflectorAngles[i] );

		Vector vecSrc = vecOrigSrc;
		Vector vecDir = gpGlobals->v_forward;
		Vector vecDest = vecOrigSrc + vecDir * 4096;

		int nMaxHits = 3;

		while ( nMaxHits > 0 )
		{
			nMaxHits--;
			int iBeam = i * 3 + 2 - nMaxHits;

			UTIL_TraceLine( vecSrc, vecDest, dont_ignore_monsters, pentIgnore, &tr );

			if ( tr.fAllSolid )
				break;

			iBeams |= 1 << iBeam;
			UpdateBeam( iBeam, vecSrc, tr.vecEndPos, nMaxHits == 2, false );

			CBaseEntity *pEntity = CBaseEntity::Instance( tr.pHit );

			UTIL_Sparks( tr.vecEndPos );

			if ( pEntity == NULL )
				break;

			if ( pEntity->IsBSPModel() )
				UTIL_DecalTrace( &tr, DECAL_GUNSHOT1 + RANDOM_LONG( 0, 4 ) );

			if ( pEntity->pev->takedamage )
			{
				ClearMultiDamage();
				pEntity->TraceAttack( m_pPlayer->pev, 2, vecDir, &tr, DMG_BULLET );
				ApplyMultiDamage( m_pPlayer->pev, m_pPlayer->pev );
			}

			if ( !pEntity->ReflectGauss() )
				break;

			float n = -DotProduct( tr.vecPlaneNormal, vecDir );

			if ( pEntity->Shielded() < 2 )
			{
				// reflect
				vecDir = 2.0 * tr.vecPlaneNormal * n + vecDir;
				vecSrc = tr.vecEndPos;
				vecDest = vecSrc + vecDir * 4096;
			}
		}
	}

	for ( i = 0; i < 15; i++ )
	{
		if ( !( iBeams & ( 1 << i ) ) )
			RemoveBeam( i );
	}
}


// one beam, 10 damage (DMG_ENERGYBEAM) per hit and frame, through up to five things;
// it punches through brushes but stops at shields
void CReflector::FireWide( const Vector &vecOrigSrc, const Vector &vecAngles )
{
	edict_t *pentIgnore = ENT( m_pPlayer->pev );
	TraceResult tr, beam_tr;

	// use 6 ammo/second
	if ( gpGlobals->time >= m_flAmmoUseTime )
	{
		UseAmmo( 1 );
		m_flAmmoUseTime = gpGlobals->time + 0.166;
	}

	UTIL_MakeVectors( vecAngles );

	Vector vecSrc = vecOrigSrc;
	Vector vecDir = gpGlobals->v_forward;
	Vector vecEnd = vecSrc + vecDir * 4096;

	for ( int i = 0; i < 5; i++ )
	{
		Vector vecDest = vecSrc + vecDir * 4096;

		UTIL_TraceLine( vecSrc, vecDest, dont_ignore_monsters, pentIgnore, &tr );

		if ( tr.fAllSolid )
			break;

		vecEnd = tr.vecEndPos;
		UTIL_Sparks( vecEnd );

		CBaseEntity *pEntity = CBaseEntity::Instance( tr.pHit );

		if ( pEntity == NULL )
			break;

		if ( pEntity->IsBSPModel() )
			UTIL_DecalTrace( &tr, DECAL_BIGSHOT1 + RANDOM_LONG( 0, 4 ) );

		if ( pEntity->pev->takedamage )
		{
			ClearMultiDamage();
			pEntity->TraceAttack( m_pPlayer->pev, 10, vecDir, &tr, DMG_ENERGYBEAM );
			ApplyMultiDamage( pev, m_pPlayer->pev );
		}

		if ( pEntity->Shielded() )
			break;

		if ( pEntity->IsBSPModel() )
		{
			// punch through the wall
			UTIL_TraceLine( tr.vecEndPos + vecDir * 8, vecDest, ignore_monsters, pentIgnore, &beam_tr );

			if ( !beam_tr.fAllSolid )
			{
				// trace backwards to find the exit point
				UTIL_TraceLine( beam_tr.vecEndPos, tr.vecEndPos, ignore_monsters, pentIgnore, &beam_tr );

				UTIL_DecalTrace( &tr, DECAL_BIGSHOT1 + RANDOM_LONG( 0, 4 ) );
				UTIL_Sparks( tr.vecEndPos );

				vecSrc = beam_tr.vecEndPos;
				pentIgnore = NULL;
			}
			else
				break;
		}
		else
		{
			vecSrc = tr.vecEndPos;
			pentIgnore = tr.pHit;
		}
	}

	UpdateBeam( 0, vecOrigSrc, vecEnd, true, true );
}


void CReflector::WeaponIdle( void )
{
	ResetEmptySound( );

	if ( m_fireState == FIRE_CHARGE )
		EndAttack();

	if ( m_flTimeWeaponIdle > gpGlobals->time )
		return;

	int iAnim;

	float flRand = RANDOM_FLOAT(0,1);

	if ( flRand <= 0.5 )
	{
		iAnim = REFLECTOR_IDLE;
		m_flTimeWeaponIdle = gpGlobals->time + RANDOM_FLOAT( 10, 15 );
	}
	else if ( flRand <= 0.75 )
	{
		iAnim = REFLECTOR_IDLE2;
		m_flTimeWeaponIdle = gpGlobals->time + RANDOM_FLOAT( 10, 15 );
	}
	else
	{
		iAnim = REFLECTOR_FIDGET;
		m_flTimeWeaponIdle = gpGlobals->time + 3;
	}

	SendWeaponAnim( iAnim );
}


void CReflector::EndAttack( void )
{
	STOP_SOUND( ENT(m_pPlayer->pev), CHAN_STATIC, REFLECTOR_SOUND_RUN );

	m_fireState = FIRE_OFF;
	m_flTimeWeaponIdle = gpGlobals->time + 2.0;
	m_flNextPrimaryAttack = gpGlobals->time + 0.5;

	for ( int i = 0; i < 15; i++ )
		RemoveBeam( i );
}


// a beam runs from vecEnd back to vecSrc; the first of a series ends at the gun's attachment
void CReflector::UpdateBeam( int iBeam, const Vector &vecSrc, const Vector &vecEnd, bool fFirst, bool fWide )
{
	if ( m_pBeams[iBeam] )
	{
		m_pBeams[iBeam]->SetStartPos( vecEnd );
		if ( !fFirst )
			m_pBeams[iBeam]->SetEndPos( vecSrc );
		return;
	}

	if ( fWide )
		m_pBeams[iBeam] = CBeam::BeamCreate( REFLECTOR_BEAM_SPRITE, 40 );
	else
		m_pBeams[iBeam] = CBeam::BeamCreate( REFLECTOR_LASER_SPRITE, 20 );

	if ( fFirst )
	{
		m_pBeams[iBeam]->PointEntInit( vecEnd, m_pPlayer->entindex() );
		m_pBeams[iBeam]->SetEndAttachment( 1 );
	}
	else
		m_pBeams[iBeam]->PointsInit( vecEnd, vecSrc );

	if ( fWide )
	{
		m_pBeams[iBeam]->SetScrollRate( 35 );
		m_pBeams[iBeam]->SetColor( 255, 255, 255 );
	}
	else
	{
		m_pBeams[iBeam]->SetScrollRate( 10 );
		m_pBeams[iBeam]->SetColor( 64, 192, 255 );
	}

	// sic: start and end the other way round from the updates above, until the next frame
	m_pBeams[iBeam]->SetStartPos( vecSrc );
	m_pBeams[iBeam]->SetEndPos( vecEnd );
}


void CReflector::RemoveBeam( int iBeam )
{
	if ( m_pBeams[iBeam] )
		UTIL_Remove( m_pBeams[iBeam] );
	m_pBeams[iBeam] = NULL;
}

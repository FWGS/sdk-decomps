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
// rocket.cpp - Scientist Slaughterhouse's rocket, the base of
// the RPG's rocket and of the mod's missiles (file name ours)
//=========================================================

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "soundent.h"
#include "decals.h"


LINK_ENTITY_TO_CLASS( rocket, CRocket );

TYPEDESCRIPTION	CRocket::m_SaveData[] =
{
	DEFINE_FIELD( CRocket, m_flSeekTime, FIELD_TIME ),
	DEFINE_FIELD( CRocket, m_lastTime, FIELD_TIME ),
	DEFINE_FIELD( CRocket, m_maxFrame, FIELD_FLOAT ),
	DEFINE_FIELD( CRocket, m_vecBSphereColor, FIELD_VECTOR ),
};
IMPLEMENT_SAVERESTORE( CRocket, CGrenade );

//=========================================================
//=========================================================
CRocket *CRocket::CreateRocket( const Vector &vecOrigin, const Vector &vecAngles, edict_t *pOwner, const Vector &vecColor, int iszModel )
{
	CRocket *pRocket = GetClassPtr( (CRocket *)NULL );

	pRocket->pev->model = iszModel;
	UTIL_SetOrigin( pRocket->pev, vecOrigin );
	pRocket->pev->angles = vecAngles;
	pRocket->pev->rendercolor = vecColor;
	pRocket->Spawn();
	pRocket->pev->owner = pOwner;

	return pRocket;
}

//=========================================================
//=========================================================
void CRocket :: Spawn( void )
{
	Precache( );
	// motor
	pev->movetype = MOVETYPE_FLYMISSILE;
	pev->solid = SOLID_BBOX;

	if ( pev->model )
	{
		const char *pszModel = STRING( pev->model );

		if ( pszModel && *pszModel )
		{
			// a sprite animates (FlyThink, Steer)
			if ( !strcmp( pszModel + strlen( pszModel ) - 3, "spr" ) )
				pev->spawnflags |= SF_ROCKET_SPRITE;

			SET_MODEL( ENT(pev), pszModel );
		}
		else
			SET_MODEL( ENT(pev), "models/rpgrocket.mdl" );
	}
	else
		SET_MODEL( ENT(pev), "models/rpgrocket.mdl" );

	UTIL_SetSize(pev, Vector( 0, 0, 0), Vector(0, 0, 0));
	UTIL_SetOrigin( pev, pev->origin );

	pev->classname = MAKE_STRING("rocket");

	if ( pev->rendercolor == g_vecZero )
		IgniteTrail( Vector( 224, 224, 255 ) );
	else
		IgniteTrail( pev->rendercolor );

	if ( HasSpawnFlag( SF_ROCKET_SPRITE ) )
	{
		m_maxFrame = (float) MODEL_FRAMES( pev->modelindex ) - 1;
		m_lastTime = gpGlobals->time;
		m_vecBSphereColor = pev->rendercolor;
		pev->rendercolor = g_vecZero;
		pev->frame = RANDOM_LONG( 0, m_maxFrame );
		pev->framerate = 10;
	}

	SetThink( FlyThink );
	SetTouch( RocketTouch );

	UTIL_MakeAimVectors( pev->angles );
	pev->velocity = gpGlobals->v_forward * 400;
	m_vecEnemyLKP = gpGlobals->v_forward * 8192 + pev->origin;
	pev->speed = 2000;
	m_flFieldOfView = 0;

	pev->nextthink = gpGlobals->time + 0.1;
	m_flSeekTime = gpGlobals->time;

	pev->dmg = gSkillData.plrDmgRPG;
}

//=========================================================
//=========================================================
void CRocket :: RocketTouch ( CBaseEntity *pOther )
{
	STOP_SOUND( edict(), CHAN_VOICE, "weapons/rocket1.wav" );
	pev->effects &= ~EF_LIGHT;

	// a sprite rocket's plasma or laser blast detonates a moment later (ExDetonate)
	if ( ( pev->spawnflags & ( SF_ROCKET_SPRITE | 8 ) ) == ( SF_ROCKET_SPRITE | 8 ) || ( pev->spawnflags & ( SF_ROCKET_SPRITE | 16 ) ) == ( SF_ROCKET_SPRITE | 16 ) )
	{
		pev->rendercolor = m_vecBSphereColor;
		pev->movedir = pev->velocity.Normalize();
		SetThink( ExDetonate );
		pev->nextthink = gpGlobals->time + gpGlobals->frametime;
	}
	else
		ExplodeTouch( pOther );
}

//=========================================================
//=========================================================
void CRocket :: Precache( void )
{
	PRECACHE_MODEL("models/rpgrocket.mdl");
	PRECACHE_SOUND ("weapons/rocket1.wav");
}

void CRocket :: IgniteTrail( const Vector &vecColor )
{
	if ( !HasSpawnFlag( SF_ROCKET_SPRITE ) )
		pev->effects |= EF_LIGHT;

	// make rocket sound
	EMIT_SOUND( ENT(pev), CHAN_VOICE, "weapons/rocket1.wav", 1, 0.5 );

	// rocket trail
	BeamFollow( g_sModelIndexSmokePuff, 40, 5, vecColor, 255 );
}

// steers at the laser spot closest to the line of flight
void CRocket :: FollowThink( void  )
{
	CBaseEntity *pOther = NULL;
	Vector vecDir;
	float flDist, flMax, flDot;
	TraceResult tr;

	UTIL_MakeAimVectors( pev->angles );

	m_vecEnemyLKP = pev->origin + pev->velocity * pev->speed;
	flMax = 4096;

	// Examine all entities within a reasonable radius
	while ((pOther = UTIL_FindEntityByClassname( pOther, "laser_spot" )) != NULL)
	{
		UTIL_TraceLine ( pev->origin, pOther->pev->origin, dont_ignore_monsters, ENT(pev), &tr );
		// ALERT( at_console, "%f\n", tr.flFraction );
		if (tr.flFraction >= 1)
		{
			vecDir = pOther->pev->origin - pev->origin;
			flDist = vecDir.Length( );
			vecDir = vecDir.Normalize( );
			flDot = DotProduct( gpGlobals->v_forward, vecDir );
			if ((flDot > 0) && (flDist * (1 - flDot) < flMax))
			{
				flMax = flDist * (1 - flDot);
				m_vecEnemyLKP = pOther->pev->origin;
			}
		}
	}

	Steer( );

	pev->nextthink = gpGlobals->time + 0.1;
}

// heat seeking: steers at the target with the best heat signature in the field of view
void CRocket :: SeekThink( void  )
{
	CBaseEntity *pOther = NULL;
	Vector vecDir;
	float flDist, flMax, flDot;
	int iHeat;
	TraceResult tr;

	flMax = 4096;

	UTIL_MakeAimVectors( pev->angles );

	m_vecEnemyLKP = pev->origin + pev->velocity * pev->speed;

	if ( m_flFieldOfView == 0 )
		m_flFieldOfView = 0.7;

	if ( gpGlobals->time >= m_flSeekTime )
	{
		while ((pOther = UTIL_FindEntityInSphere( pOther, pev->origin, 4096 )) != NULL)
		{
			iHeat = pOther->HeatSignature( );
			if ( !iHeat )
				continue;

			// sic: skips what has the same owner, not the owner
			if ( pev->owner && pOther->pev->owner == pev->owner )
				continue;

			vecDir = pOther->BodyTarget( pev->origin ) - pev->origin;
			flDist = vecDir.Length( );
			vecDir = vecDir.Normalize( );
			flDot = DotProduct( gpGlobals->v_forward, vecDir );
			if ( flDot > m_flFieldOfView )
			{
				flDist = (1 - flDot) * flDist / iHeat;
				if ( flDist < flMax )
				{
					UTIL_TraceLine ( pev->origin, pOther->pev->origin, ignore_monsters, ENT(pev), &tr );
					if ( tr.flFraction >= 1 || tr.pHit == pOther->edict() )
					{
						flMax = flDist;
						m_vecEnemyLKP = pOther->BodyTarget( pev->origin );
					}
				}
			}
		}
	}

	Steer( );

	pev->nextthink = gpGlobals->time + 0.1;
}

// flies straight on until m_flSeekTime, then at m_hEnemy
void CRocket :: TargetThink( void  )
{
	if ( gpGlobals->time >= m_flSeekTime )
	{
		if ( m_hEnemy == NULL )
			SetThink( FlyThink );
		else
			m_vecEnemyLKP = m_hEnemy->BodyTarget( pev->origin );
	}
	else
	{
		UTIL_MakeAimVectors( pev->angles );
		m_vecEnemyLKP = pev->origin + pev->velocity * pev->speed;
	}

	Steer( );

	pev->nextthink = gpGlobals->time + 0.1;
}

// straight on, speeding up to pev->speed
void CRocket :: FlyThink( void  )
{
	if ( HasSpawnFlag( SF_ROCKET_SPRITE ) )
	{
		pev->frame += ( gpGlobals->time - m_lastTime ) * pev->framerate;
		m_lastTime = gpGlobals->time;
		if ( pev->frame >= m_maxFrame )
			pev->frame -= m_maxFrame;
	}

	float flSpeed = pev->velocity.Length();

	UTIL_MakeAimVectors( pev->angles );

	flSpeed += 256;
	if (pev->waterlevel == 3)
	{
		// go slow underwater
		pev->velocity = gpGlobals->v_forward * ( flSpeed >= 300 ? ( flSpeed = 300 ) : flSpeed );	// sic: keeps the cap
		UTIL_BubbleTrail( pev->origin - pev->velocity * 0.1, pev->origin, 4 );
	}
	else
	{
		pev->velocity = gpGlobals->v_forward * min( flSpeed, pev->speed );
	}

	pev->nextthink = gpGlobals->time + 0.1;

	if (flSpeed > 200)
	{
		CSoundEnt::InsertSound ( bits_SOUND_DANGER, pev->origin + pev->velocity * 0.5, max( 1000, flSpeed ), 0.1 );
	}
}

//=========================================================
//=========================================================
CRocket *CRocket::ShootArc( edict_t *pOwner, Vector vecOrigin, Vector vecVelocity, const Vector &vecColor )
{
	CRocket *pRocket = GetClassPtr( (CRocket *)NULL );

	pRocket->pev->rendercolor = vecColor;
	pRocket->Spawn();
	pRocket->pev->movetype = MOVETYPE_TOSS;
	pRocket->pev->gravity = 0.5;
	UTIL_SetOrigin( pRocket->pev, vecOrigin );
	pRocket->pev->velocity = vecVelocity;
	pRocket->pev->angles = UTIL_VecToAngles( vecVelocity );
	pRocket->pev->owner = pOwner;
	pRocket->SetThink( ArcThink );

	return pRocket;
}

void CRocket :: ArcThink( void )
{
	pev->angles = UTIL_VecToAngles( pev->velocity );
	pev->nextthink = gpGlobals->time + 0.1;
}

// this acceleration and turning math is totally wrong, but it seems to respond well so don't change it.
void CRocket :: Steer( void )
{
	if ( HasSpawnFlag( SF_ROCKET_SPRITE ) && m_maxFrame != 0 )
	{
		pev->frame += ( gpGlobals->time - m_lastTime ) * pev->framerate;
		m_lastTime = gpGlobals->time;
		if ( pev->frame >= m_maxFrame )
			pev->frame -= m_maxFrame;
	}

	float flSpeed = pev->velocity.Length() * 0.4;
	if ( flSpeed < 256 )
		flSpeed = 256;

	Vector vecTarget = ( m_vecEnemyLKP - pev->origin ).Normalize();
	pev->velocity = pev->velocity * 0.8 + vecTarget * flSpeed;

	flSpeed = pev->velocity.Length();
	if (pev->waterlevel == 3)
	{
		// go slow underwater
		if (flSpeed > 300)
		{
			pev->velocity = pev->velocity.Normalize() * 300;
		}
		UTIL_BubbleTrail( pev->origin - pev->velocity * 0.1, pev->origin, 4 );
	}
	else if (flSpeed > pev->speed)
	{
		pev->velocity = pev->velocity.Normalize() * pev->speed;
	}

	pev->angles = UTIL_VecToAngles( pev->velocity );

	if (flSpeed > 200)
	{
		flSpeed = max( 1000, flSpeed );
		CSoundEnt::InsertSound ( bits_SOUND_DANGER, pev->origin + pev->velocity * 0.5, flSpeed, 0.1 );
	}
}

void CRocket :: Killed( entvars_t *pevAttacker, int iGib )
{
	STOP_SOUND( edict(), CHAN_VOICE, "weapons/rocket1.wav" );

	// a sprite rocket's plasma blast
	if ( ( pev->spawnflags & ( SF_ROCKET_SPRITE | 8 ) ) == ( SF_ROCKET_SPRITE | 8 ) )
	{
		pev->rendercolor = m_vecBSphereColor;
		SetThink( Detonate );
		pev->nextthink = gpGlobals->time + gpGlobals->frametime;
		return;
	}

	CGrenade::Killed( pevAttacker, iGib );
}

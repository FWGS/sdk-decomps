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
//
// beams.cpp: Scientist Slaughterhouse's beam entities. beam_cannon is env_laser grown up (sprites at both
// ends, dynamic lights, end effects, damage types); beam_charge a sprite that charges up and fires its
// targets; beam_turret an invisible monster that aims a beam_cannon; beam_multi a set of beam_cannons.
//
#include "extdll.h"
#include "util.h"
#include "customentity.h"
#include "cbase.h"
#include "monsters.h"
#include "effects.h"
#include "beams.h"
#include "weapons.h"
#include "decals.h"

extern float gTankAccuracy[];	// defined in func_turret.cpp, name ours: firing cones by "accuracy" choice


//=========================================================
// beam_cannon
//=========================================================

LINK_ENTITY_TO_CLASS( beam_cannon, CBeamCannon );

TYPEDESCRIPTION	CBeamCannon::m_SaveData[] =
{
	DEFINE_FIELD( CBeamCannon, m_pStartSprite, FIELD_CLASSPTR ),
	DEFINE_FIELD( CBeamCannon, m_iszStartSpriteName, FIELD_STRING ),
	DEFINE_FIELD( CBeamCannon, m_flStartSpriteScale, FIELD_FLOAT ),
	DEFINE_FIELD( CBeamCannon, m_pEndSprite, FIELD_CLASSPTR ),
	DEFINE_FIELD( CBeamCannon, m_iszEndSpriteName, FIELD_STRING ),
	DEFINE_FIELD( CBeamCannon, m_flEndSpriteScale, FIELD_FLOAT ),
	DEFINE_FIELD( CBeamCannon, m_firePosition, FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( CBeamCannon, m_startLight, FIELD_VECTOR ),
	DEFINE_FIELD( CBeamCannon, m_endLight, FIELD_VECTOR ),
	DEFINE_FIELD( CBeamCannon, m_damageType, FIELD_INTEGER ),
	DEFINE_FIELD( CBeamCannon, m_endType, FIELD_INTEGER ),
	DEFINE_FIELD( CBeamCannon, m_firstShot, FIELD_INTEGER ),
	DEFINE_FIELD( CBeamCannon, m_endValue, FIELD_FLOAT ),
	DEFINE_FIELD( CBeamCannon, m_radius, FIELD_FLOAT ),
	DEFINE_FIELD( CBeamCannon, m_scale, FIELD_FLOAT ),
	DEFINE_FIELD( CBeamCannon, m_flicker, FIELD_FLOAT ),
	DEFINE_FIELD( CBeamCannon, m_expireTime, FIELD_TIME ),
	DEFINE_FIELD( CBeamCannon, m_maxFrame, FIELD_INTEGER ),
	DEFINE_FIELD( CBeamCannon, m_lastTime, FIELD_TIME ),
};

IMPLEMENT_SAVERESTORE( CBeamCannon, CBeam );

void CBeamCannon::Spawn( void )
{
	if ( FStringNull( pev->model ) )
	{
		SetThink( SUB_Remove );
		return;
	}
	pev->solid = SOLID_NOT;							// Remove model & collisions
	Precache( );

	m_expireTime = 0;
	pev->flags |= FL_CUSTOMENTITY;

	PointsInit( pev->origin, pev->origin );

	if ( !m_pStartSprite && m_iszStartSpriteName )
	{
		m_pStartSprite = CSprite::SpriteCreate( STRING(m_iszStartSpriteName), pev->origin, TRUE );
		m_pStartSprite->pev->framerate = 10;
		m_pStartSprite->Spawn();
	}
	else
		m_pStartSprite = NULL;

	// the trail and beam end effects don't use the end sprite
	if ( !m_pEndSprite && m_iszEndSpriteName && ( m_endType < CANNON_END_SPRITES || m_endType > CANNON_END_CYLINDER ) && m_endType != CANNON_END_RESERVED )
	{
		m_pEndSprite = CSprite::SpriteCreate( STRING(m_iszEndSpriteName), pev->origin, TRUE );
		m_pEndSprite->pev->framerate = 10;
		m_pEndSprite->Spawn();
	}
	else
		m_pEndSprite = NULL;

	if ( pev->spawnflags & SF_CANNON_GLOWSPRITES )
	{
		if ( m_pStartSprite )
			m_pStartSprite->SetTransparency( kRenderGlow, pev->rendercolor.x, pev->rendercolor.y, pev->rendercolor.z, pev->renderamt, kRenderFxNoDissipation );
		if ( m_pEndSprite )
			m_pEndSprite->SetTransparency( kRenderGlow, pev->rendercolor.x, pev->rendercolor.y, pev->rendercolor.z, pev->renderamt, kRenderFxNoDissipation );
	}
	else if ( pev->spawnflags & SF_CANNON_ADDSPRITES )
	{
		if ( m_pStartSprite )
			m_pStartSprite->SetTransparency( kRenderTransAdd, pev->rendercolor.x, pev->rendercolor.y, pev->rendercolor.z, pev->renderamt, pev->renderfx );
		if ( m_pEndSprite )
			m_pEndSprite->SetTransparency( kRenderTransAdd, pev->rendercolor.x, pev->rendercolor.y, pev->rendercolor.z, pev->renderamt, pev->renderfx );
	}
	else if ( pev->spawnflags & SF_CANNON_SOLID )
	{
		if ( m_pStartSprite )
			m_pStartSprite->SetTransparency( kRenderTransAlpha, pev->rendercolor.x, pev->rendercolor.y, pev->rendercolor.z, pev->renderamt, pev->renderfx );
		if ( m_pEndSprite )
			m_pEndSprite->SetTransparency( kRenderTransAlpha, pev->rendercolor.x, pev->rendercolor.y, pev->rendercolor.z, pev->renderamt, pev->renderfx );
	}
	else
	{
		if ( m_pStartSprite )
			m_pStartSprite->SetTransparency( kRenderGlow, pev->rendercolor.x, pev->rendercolor.y, pev->rendercolor.z, pev->renderamt, kRenderFxNoDissipation );
		if ( m_pEndSprite )
			m_pEndSprite->SetTransparency( kRenderTransAdd, pev->rendercolor.x, pev->rendercolor.y, pev->rendercolor.z, pev->renderamt, pev->renderfx );
	}

	if ( m_pStartSprite )
		m_pStartSprite->pev->scale = m_flStartSpriteScale;
	if ( m_pEndSprite )
		m_pEndSprite->pev->scale = m_flEndSpriteScale;

	if ( pev->spawnflags & SF_CANNON_SPIRAL )
		SetFlags( GetFlags() | BEAM_FSINE );
	if ( pev->spawnflags & SF_CANNON_SOLID )
		SetFlags( GetFlags() | BEAM_FSOLID );

	if ( pev->spawnflags & SF_CANNON_MIMIC )
	{
		SetThink( MatchThink );
		TurnOff();
		pev->nextthink = gpGlobals->time + 0.1;
	}
	else
	{
		SetThink( StrikeThink );
		if ( pev->targetname && !(pev->spawnflags & SF_CANNON_STARTON) )
			TurnOff();
		else
			TurnOn();
	}
}

void CBeamCannon::Precache( void )
{
	pev->modelindex = PRECACHE_MODEL( (char *)STRING(pev->model) );
	if ( m_iszStartSpriteName )
		PRECACHE_MODEL( (char *)STRING(m_iszStartSpriteName) );
	if ( m_iszEndSpriteName )
		m_iEndSpriteIndex = PRECACHE_MODEL( (char *)STRING(m_iszEndSpriteName) );
}


void CBeamCannon::KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "LaserTarget"))
	{
		pev->message = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "width"))
	{
		m_scale = atof(pkvd->szValue);
		SetWidth( m_scale );
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "flicker"))
	{
		m_flicker = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "NoiseAmplitude"))
	{
		SetNoise( atoi(pkvd->szValue) );
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "TextureScroll"))
	{
		SetScrollRate( atoi(pkvd->szValue) );
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "texture"))
	{
		pev->model = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "StartSprite"))
	{
		m_iszStartSpriteName = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "StartSpriteScale"))
	{
		m_flStartSpriteScale = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "EndSprite"))
	{
		m_iszEndSpriteName = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "EndSpriteScale"))
	{
		m_flEndSpriteScale = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "framestart"))
	{
		pev->frame = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "elight"))
	{
		UTIL_StringToVector( m_endLight, pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "slight"))
	{
		UTIL_StringToVector( m_startLight, pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "damage"))
	{
		pev->dmg = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "damagetype"))
	{
		m_damageType = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "radius"))
	{
		m_radius = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "m_endValue"))
	{
		m_endValue = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "endtype"))
	{
		m_endType = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "killdesc"))
	{
		m_iszKillDesc = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CBeam::KeyValue( pkvd );
}


int CBeamCannon::IsOn( void )
{
	if (pev->effects & EF_NODRAW)
		return 0;
	return 1;
}


void CBeamCannon::TurnOffBeam( void )
{
	pev->effects |= EF_NODRAW;
	SetThink( StartFxThink );
	if ( m_pEndSprite )
		m_pEndSprite->TurnOff();
}


void CBeamCannon::TurnOnBeam( void )
{
	pev->effects &= ~EF_NODRAW;
	if ( m_pEndSprite )
		m_pEndSprite->TurnOn();
	SetThink( StrikeThink );
	pev->nextthink = gpGlobals->time;
	pev->dmgtime = pev->nextthink;
	m_firstShot = TRUE;
}


void CBeamCannon::TurnOff( void )
{
	pev->effects |= EF_NODRAW;
	pev->nextthink = 0;
	if ( m_pStartSprite )
		m_pStartSprite->TurnOff();
	if ( m_pEndSprite )
		m_pEndSprite->TurnOff();
}


void CBeamCannon::TurnOn( void )
{
	pev->effects &= ~EF_NODRAW;
	if ( m_pStartSprite )
		m_pStartSprite->TurnOn();
	if ( m_pEndSprite )
		m_pEndSprite->TurnOn();
	pev->nextthink = gpGlobals->time;
	pev->dmgtime = pev->nextthink;
	m_firstShot = TRUE;
}


void CBeamCannon::Explode( const Vector &vecPos )
{
	int iScale = max( 10.0, ( m_radius * 0.6 - 50 ) * 0.6 );

	MESSAGE_BEGIN( MSG_PAS, SVC_TEMPENTITY, pev->origin );
		WRITE_BYTE( TE_EXPLOSION );
		WRITE_COORD( vecPos.x );
		WRITE_COORD( vecPos.y );
		WRITE_COORD( vecPos.z );
		WRITE_SHORT( g_sModelIndexFireball );
		WRITE_BYTE( (BYTE)min( 255, iScale ) ); // scale * 10
		WRITE_BYTE( 15 ); // framerate
		WRITE_BYTE( TE_EXPLFLAG_NONE );
	MESSAGE_END();

	entvars_t *pevOwner = pev;
	if ( pev->owner )
		pevOwner = VARS( pev->owner );
	RadiusDamage( vecPos, pev, pevOwner, pev->dmg, m_radius, CLASS_NONE, DMG_BLAST );
}


void CBeamCannon::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	int active = IsOn();

	if ( !ShouldToggle( useType, active ) )
		return;
	if ( active )
	{
		TurnOff();
	}
	else
	{
		TurnOn();
	}
}


// while the beam itself is off (beam_turret's shots), the start sprite and light stay
void CBeamCannon::StartFxThink( void )
{
	if ( m_pStartSprite )
		UTIL_SetOrigin( m_pStartSprite->pev, pev->origin );

	if ( m_startLight != g_vecZero )
	{
		MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, pev->origin );
			WRITE_BYTE( TE_DLIGHT );
			WRITE_COORD( pev->origin.x );	// X
			WRITE_COORD( pev->origin.y );	// Y
			WRITE_COORD( pev->origin.z );	// Z
			WRITE_BYTE( 64 );		// radius * 0.1
			WRITE_BYTE( m_startLight.x );		// r
			WRITE_BYTE( m_startLight.y );		// g
			WRITE_BYTE( m_startLight.z );		// b
			WRITE_BYTE( 1 );		// time * 10
			WRITE_BYTE( 0 );		// decay * 0.1
		MESSAGE_END( );
	}

	pev->nextthink = gpGlobals->time + 0.1;
}


void CBeamCannon::FireAtPoint( TraceResult &tr )
{
	if ( (pev->spawnflags & SF_CANNON_POINT2POINT) && !(pev->spawnflags & SF_CANNON_DIRECTIONAL) )
	{
		SetEndPos( m_firePosition );
		m_firePosition = pev->angles;
	}
	else
	{
		SetEndPos( tr.vecEndPos );
		m_firePosition = pev->angles;
	}

	if ( m_pEndSprite )
		UTIL_SetOrigin( m_pEndSprite->pev, GetEndPos() );
	if ( m_pStartSprite )
		UTIL_SetOrigin( m_pStartSprite->pev, GetStartPos() );

	BeamDamage( &tr );

	if ( m_startLight != g_vecZero )
	{
		MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, pev->origin );
			WRITE_BYTE( TE_DLIGHT );
			WRITE_COORD( pev->origin.x );	// X
			WRITE_COORD( pev->origin.y );	// Y
			WRITE_COORD( pev->origin.z );	// Z
			WRITE_BYTE( 64 );		// radius * 0.1
			WRITE_BYTE( m_startLight.x );		// r
			WRITE_BYTE( m_startLight.y );		// g
			WRITE_BYTE( m_startLight.z );		// b
			WRITE_BYTE( 1 );		// time * 10
			WRITE_BYTE( 0 );		// decay * 0.1
		MESSAGE_END( );
	}

	if ( m_endLight != g_vecZero )
	{
		Vector vecEnd = GetEndPos();
		MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, vecEnd );
			WRITE_BYTE( TE_DLIGHT );
			WRITE_COORD( vecEnd.x );	// X
			WRITE_COORD( vecEnd.y );	// Y
			WRITE_COORD( vecEnd.z );	// Z
			WRITE_BYTE( 48 );		// radius * 0.1
			WRITE_BYTE( m_endLight.x );		// r
			WRITE_BYTE( m_endLight.y );		// g
			WRITE_BYTE( m_endLight.z );		// b
			WRITE_BYTE( 1 );		// time * 10
			WRITE_BYTE( 0 );		// decay * 0.1
		MESSAGE_END( );
	}

	DoSparks( GetStartPos(), tr.vecEndPos );

	switch ( m_endType )
	{
	case CANNON_END_EXPLOSION:
		if ( m_firstShot )
			Explode( tr.vecEndPos );
		break;

	case CANNON_END_EXPLOSIONS:
		if ( gpGlobals->time - pev->fuser2 > m_endValue )
		{
			Explode( tr.vecEndPos );
			pev->fuser2 = gpGlobals->time;
		}
		break;

	case CANNON_END_SPRITES:
		if ( m_iEndSpriteIndex )
		{
			MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, pev->origin );
				if ( pev->spawnflags & SF_CANNON_GLOWSPRITES )
					WRITE_BYTE( TE_GLOWSPRITE );
				else
					WRITE_BYTE( TE_SPRITE );
				WRITE_COORD( tr.vecEndPos.x );
				WRITE_COORD( tr.vecEndPos.y );
				WRITE_COORD( tr.vecEndPos.z );
				WRITE_SHORT( m_iEndSpriteIndex );
				WRITE_BYTE( m_flEndSpriteScale * 0.1 );	// sic: the scale byte is in tenths
				if ( !(pev->spawnflags & SF_CANNON_GLOWSPRITES) )
					WRITE_BYTE( pev->renderamt );	// sic: a glow sprite gets one byte of its three
			MESSAGE_END();
		}
		break;

	case CANNON_END_TORUS:
	case CANNON_END_DISK:
	case CANNON_END_CYLINDER:
		if ( m_iEndSpriteIndex && gpGlobals->time - pev->fuser2 > m_endValue * 0.5 )
		{
			pev->fuser2 = gpGlobals->time;
			MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
				switch ( m_endType )
				{
				case CANNON_END_DISK:
					WRITE_BYTE( TE_BEAMDISK );
					break;
				case CANNON_END_CYLINDER:
					WRITE_BYTE( TE_BEAMCYLINDER );
					break;
				default:
					WRITE_BYTE( TE_BEAMTORUS );
					break;
				}
				WRITE_COORD( tr.vecEndPos.x );
				WRITE_COORD( tr.vecEndPos.y );
				WRITE_COORD( tr.vecEndPos.z );
				WRITE_COORD( tr.vecEndPos.x );
				WRITE_COORD( tr.vecEndPos.y );
				WRITE_COORD( tr.vecEndPos.z + m_radius );
				WRITE_SHORT( m_iEndSpriteIndex );
				WRITE_BYTE( 0 ); // startframe
				WRITE_BYTE( 10 ); // framerate
				WRITE_BYTE( m_endValue * 10 ); // life
				WRITE_BYTE( GetWidth() * 10 );  // width
				WRITE_BYTE( 0 );   // noise
				WRITE_BYTE( pev->rendercolor.x );
				WRITE_BYTE( pev->rendercolor.y );
				WRITE_BYTE( pev->rendercolor.z );
				WRITE_BYTE( pev->renderamt );	// brightness
				WRITE_BYTE( pev->animtime );		// speed
			MESSAGE_END();
		}
		break;

	case CANNON_END_PARTICLES:
		if ( gpGlobals->time - pev->fuser2 > m_endValue * 0.5 )
			pev->fuser2 = gpGlobals->time;	// sic: no particles
		break;

	case CANNON_END_BLOOD:
		UTIL_BloodDrips( tr.vecEndPos, (tr.vecEndPos - pev->origin).Normalize(), BLOOD_COLOR_RED, pev->dmg );
		break;

	case CANNON_END_ALIENBLOOD:
		UTIL_BloodDrips( tr.vecEndPos, (tr.vecEndPos - pev->origin).Normalize(), BLOOD_COLOR_YELLOW, pev->dmg );
		break;
	}

	m_firstShot = FALSE;
}


// CBeam::BeamDamage with an owner, a damage type, radius damage and decals by beam width
void CBeamCannon::BeamDamage( TraceResult *ptr )
{
	RelinkBeam();
	if ( ptr->flFraction != 1.0 && ptr->pHit != NULL )
	{
		CBaseEntity *pHit = CBaseEntity::Instance(ptr->pHit);
		if ( pHit )
		{
			entvars_t *pevOwner = pev;
			if ( pev->owner )
				pevOwner = VARS( pev->owner );

			if ( pev->dmg > 0 )
			{
				ClearMultiDamage();
				pHit->TraceAttack( pevOwner, pev->dmg * (gpGlobals->time - pev->dmgtime), (ptr->vecEndPos - pev->origin).Normalize(), ptr, m_damageType );
				ApplyMultiDamage( pev, pevOwner );
				if ( m_radius > 0 )
					RadiusDamage( ptr->vecEndPos, pev, pevOwner, pev->dmg * (gpGlobals->time - pev->dmgtime), m_radius, CLASS_NONE, m_damageType );
			}

			if ( pev->spawnflags & SF_CANNON_DECALS )
			{
				if ( pHit->IsBSPModel() )
				{
					if ( m_endType == CANNON_END_BLOOD )
						UTIL_DecalTrace( ptr, DECAL_BLOOD1 + RANDOM_LONG(0,5) );
					else if ( m_endType == CANNON_END_ALIENBLOOD )
						UTIL_DecalTrace( ptr, DECAL_YBLOOD1 + RANDOM_LONG(0,5) );
					else if ( GetWidth() > 240 )
						UTIL_DecalTrace( ptr, DECAL_SCORCH1 + RANDOM_LONG(0,2) );	// sic: 2 gives DECAL_BLOOD1
					else if ( GetWidth() > 100 )
						UTIL_DecalTrace( ptr, DECAL_SMALLSCORCH1 + RANDOM_LONG(0,2) );
					else
						UTIL_DecalTrace( ptr, DECAL_BIGSHOT1 + RANDOM_LONG(0,4) );
				}
			}
		}
	}
	pev->dmgtime = gpGlobals->time;
}


void CBeamCannon::StrikeThink( void )
{
	CBaseEntity *pEnd = RandomTargetname( STRING(pev->message) );

	if ( pEnd )
		m_firePosition = pEnd->pev->origin;

	if ( pev->spawnflags & SF_CANNON_DIRECTIONAL )
	{
		Vector vecDir = ( m_firePosition - pev->origin ).Normalize();
		m_firePosition = pev->origin + vecDir * 4096;
	}

	if ( m_maxFrame && pev->framerate > 0 )
	{
		pev->frame += gpGlobals->time - m_lastTime;
		if ( m_maxFrame < pev->frame )
			pev->frame = fmod( pev->frame, m_maxFrame );
		m_lastTime = gpGlobals->time;
	}

	if ( m_flicker > 0 )
		SetWidth( RANDOM_FLOAT( 1 - m_flicker, 1 + m_flicker ) * m_scale );

	TraceResult tr;
	IGNORE_GLASS glass = dont_ignore_glass;
	if ( pev->spawnflags & SF_CANNON_IGNOREGLASS )
		glass = ignore_glass;

	UTIL_TraceLine( pev->origin, m_firePosition, dont_ignore_monsters, glass, pev->owner, &tr );
	FireAtPoint( tr );
	pev->nextthink = gpGlobals->time + 0.1;

	if ( m_expireTime > 0 && m_expireTime <= gpGlobals->time )
		SetThink( SUB_Remove );
}


// "Mimic target": follow another beam_cannon's beam and on/off state
void CBeamCannon::MatchThink( void )
{
	if ( FStringNull( pev->message ) )
	{
		UTIL_Remove( this );
		return;
	}

	CBaseEntity *pTarget = UTIL_FindEntityByTargetname( NULL, STRING(pev->message) );
	while ( pTarget && !FClassnameIs( pTarget->pev, "beam_cannon" ) )
		pTarget = UTIL_FindEntityByTargetname( pTarget, STRING(pev->message) );

	if ( pTarget )
	{
		CBeamCannon *pCannon = (CBeamCannon *)pTarget;
		if ( pCannon->IsOn() )
		{
			if ( !IsOn() )
				TurnOn();

			SetStartPos( pCannon->GetStartPos() );
			m_firePosition = pCannon->GetEndPos();

			TraceResult tr;
			IGNORE_GLASS glass = dont_ignore_glass;
			if ( pev->spawnflags & SF_CANNON_IGNOREGLASS )
				glass = ignore_glass;
			UTIL_TraceLine( pev->origin, m_firePosition, dont_ignore_monsters, glass, pCannon->pev->owner, &tr );
			FireAtPoint( tr );
		}
		else if ( IsOn() )
			TurnOff();
	}

	pev->nextthink = gpGlobals->time + 0.1;
}


CBeamCannon *CBeamCannon::CannonCreate( int iszModel, int iszStartSprite, int iszEndSprite )
{
	CBeamCannon *pCannon = GetClassPtr( (CBeamCannon *)NULL );

	pCannon->pev->classname = MAKE_STRING("beam_cannon");
	pCannon->pev->model = iszModel;
	pCannon->m_iszStartSpriteName = iszStartSprite;
	pCannon->m_iszEndSpriteName = iszEndSprite;
	pCannon->Spawn();

	return pCannon;
}


//=========================================================
// beam_charge
//=========================================================

LINK_ENTITY_TO_CLASS( beam_charge, CBeamCharge );

TYPEDESCRIPTION	CBeamCharge::m_SaveData[] =
{
	DEFINE_FIELD( CBeamCharge, m_on, FIELD_INTEGER ),
	DEFINE_FIELD( CBeamCharge, m_beamSlack, FIELD_FLOAT ),
	DEFINE_FIELD( CBeamCharge, m_scale, FIELD_FLOAT ),
	DEFINE_FIELD( CBeamCharge, m_fxScale, FIELD_FLOAT ),
	DEFINE_FIELD( CBeamCharge, m_alpha, FIELD_FLOAT ),
	DEFINE_FIELD( CBeamCharge, m_flWarmTime, FIELD_FLOAT ),
	DEFINE_FIELD( CBeamCharge, m_flCoolTime, FIELD_FLOAT ),
	DEFINE_FIELD( CBeamCharge, m_volume, FIELD_FLOAT ),
	DEFINE_FIELD( CBeamCharge, m_iszBeamSpriteName, FIELD_STRING ),
};

IMPLEMENT_SAVERESTORE( CBeamCharge, CBaseEntity );	// sic: CSprite's fields aren't saved

void CBeamCharge::Spawn( void )
{
	pev->solid			= SOLID_NOT;
	pev->movetype		= MOVETYPE_NONE;
	pev->effects		= 0;
	pev->frame			= 0;

	Precache();
	SET_MODEL( ENT(pev), STRING(pev->model) );

	if ( m_flCoolTime <= 0 )
		m_flCoolTime = 0.05;
	if ( m_flWarmTime <= 0 )
		m_flWarmTime = 0.05;

	m_maxFrame = (float) MODEL_FRAMES( pev->modelindex ) - 1;
	if ( pev->spawnflags & SF_BEAMCHARGE_STARTON )
		m_on = TRUE;
	else
		m_on = FALSE;
	TurnOff();
}


void CBeamCharge::Precache( void )
{
	PRECACHE_MODEL( (char *)STRING(pev->model) );
	if ( m_iszBeamSpriteName )
		m_iBeamSprite = PRECACHE_MODEL( (char *)STRING(m_iszBeamSpriteName) );
	if ( pev->noise1 )
		PRECACHE_SOUND( (char *)STRING(pev->noise1) );
	if ( pev->noise2 )
		PRECACHE_SOUND( (char *)STRING(pev->noise2) );
	if ( pev->noise3 )
		PRECACHE_SOUND( (char *)STRING(pev->noise3) );
}


void CBeamCharge::Expand( float scaleSpeed, float fadeSpeed )
{
	pev->speed = scaleSpeed;
	pev->fuser1 = fadeSpeed;
	SetThink( ExpandThink );

	pev->nextthink	= gpGlobals->time;
	m_lastTime		= gpGlobals->time;
}


// the reverse of Expand: grow and fade in over the warm-up time
void CBeamCharge::Charge( void )
{
	pev->speed = m_scale / m_flWarmTime;
	pev->fuser1 = m_alpha / m_flWarmTime;
	SetThink( ChargeThink );

	pev->nextthink	= gpGlobals->time;
	m_lastTime		= gpGlobals->time;
}


void CBeamCharge::ExpandThink( void )
{
	float frametime = gpGlobals->time - m_lastTime;
	pev->scale += pev->speed * frametime;
	pev->renderamt -= pev->fuser1 * frametime;
	if ( pev->renderamt <= 0 )
	{
		pev->renderamt = 0;
		pev->effects = EF_NODRAW;
		pev->nextthink = 0;
	}
	else
	{
		pev->nextthink		= gpGlobals->time + 0.1;
		m_lastTime			= gpGlobals->time;

		if ( m_light != g_vecZero )
		{
			MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, pev->origin );
				WRITE_BYTE( TE_DLIGHT );
				WRITE_COORD( pev->origin.x );	// X
				WRITE_COORD( pev->origin.y );	// Y
				WRITE_COORD( pev->origin.z );	// Z
				WRITE_BYTE( 64 );		// radius * 0.1
				WRITE_BYTE( m_light.x );		// r
				WRITE_BYTE( m_light.y );		// g
				WRITE_BYTE( m_light.z );		// b
				WRITE_BYTE( 1 );		// time * 10
				WRITE_BYTE( 0 );		// decay * 0.1
			MESSAGE_END( );
		}

		if ( m_maxFrame > 0 )
		{
			pev->frame += frametime * pev->framerate;
			if ( pev->frame > m_maxFrame )
				pev->frame -= m_maxFrame;
		}
	}
}


void CBeamCharge::ChargeThink( void )
{
	float frametime = gpGlobals->time - m_lastTime;
	pev->scale += pev->speed * frametime;
	pev->renderamt += pev->fuser1 * frametime;
	if ( pev->renderamt >= m_alpha )
	{
		// charged: hide and fire the targets (the beam)
		pev->renderamt = m_alpha;
		pev->scale = m_scale;
		pev->effects = EF_NODRAW;
		pev->nextthink = 0;
		SUB_UseTargets( this, USE_ON, 0 );
		if ( pev->noise2 )
			EMIT_SOUND( ENT(pev), CHAN_STATIC, STRING(pev->noise2), m_volume, ATTN_NORM );
		return;
	}

	pev->nextthink		= gpGlobals->time + 0.1;
	m_lastTime			= gpGlobals->time;

	if ( m_iszBeamSpriteName )
		RandomBeam();

	if ( m_light != g_vecZero )
	{
		MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, pev->origin );
			WRITE_BYTE( TE_DLIGHT );
			WRITE_COORD( pev->origin.x );	// X
			WRITE_COORD( pev->origin.y );	// Y
			WRITE_COORD( pev->origin.z );	// Z
			WRITE_BYTE( 64 );		// radius * 0.1
			WRITE_BYTE( m_light.x );		// r
			WRITE_BYTE( m_light.y );		// g
			WRITE_BYTE( m_light.z );		// b
			WRITE_BYTE( 1 );		// time * 10
			WRITE_BYTE( 0 );		// decay * 0.1
		MESSAGE_END( );
	}

	if ( m_maxFrame > 0 )
	{
		pev->frame += frametime * pev->framerate;
		if ( pev->frame > m_maxFrame )
			pev->frame -= m_maxFrame;
	}
}


// a short random beam from the sprite, in a cone of m_beamSlack around its angles
void CBeamCharge::RandomBeam( void )
{
	if ( m_beamSlack < 1 && m_fxScale > 0 )
	{
		UTIL_MakeVectors( pev->angles );

		// get circular gaussian spread
		float x, y, z;
		do {
			x = RANDOM_FLOAT(-0.5,0.5) + RANDOM_FLOAT(-0.5,0.5);
			y = RANDOM_FLOAT(-0.5,0.5) + RANDOM_FLOAT(-0.5,0.5);
			z = x*x+y*y;
		} while (z > 1);

		Vector vecDir = gpGlobals->v_forward +
						x * m_beamSlack * gpGlobals->v_right +
						y * m_beamSlack * gpGlobals->v_up;
		Vector vecEnd = pev->origin + vecDir * RANDOM_FLOAT( m_fxScale * 0.5, m_fxScale );

		TraceResult tr;
		UTIL_TraceLine( pev->origin, vecEnd, dont_ignore_monsters, ENT(pev), &tr );

		MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, pev->origin );
			WRITE_BYTE( TE_BEAMENTPOINT );
			WRITE_SHORT( entindex() );
			WRITE_COORD( tr.vecEndPos.x );
			WRITE_COORD( tr.vecEndPos.y );
			WRITE_COORD( tr.vecEndPos.z );
			WRITE_SHORT( m_iBeamSprite );
			WRITE_BYTE( 10 ); // startframe
			WRITE_BYTE( RANDOM_LONG( 1, 10 ) ); // framerate
			WRITE_BYTE( RANDOM_LONG( 10, 40 ) ); // life
			WRITE_BYTE( RANDOM_LONG( 16, 64 ) ); // width
			WRITE_BYTE( pev->rendercolor.x );	// sic: no noise byte, everything after shifts by one
			WRITE_BYTE( pev->rendercolor.y );
			WRITE_BYTE( pev->rendercolor.z );
			WRITE_BYTE( min( 255, (int)pev->renderamt + 128 ) );
			WRITE_BYTE( 35 );
		MESSAGE_END();
	}
}


void CBeamCharge::CoolDown( void )
{
	if ( !ShouldToggle( USE_OFF, m_on ) )
		return;

	m_on = FALSE;
	SUB_UseTargets( this, USE_OFF, 0 );

	pev->scale = m_scale;
	pev->renderamt = m_alpha;
	if ( pev->spawnflags & SF_BEAMCHARGE_EXPANDFADE )
		Expand( m_scale / m_flCoolTime, m_alpha / m_flCoolTime );
	else
		Expand( -m_scale / m_flCoolTime, m_alpha / m_flCoolTime );

	if ( pev->noise2 )
		STOP_SOUND( ENT(pev), CHAN_STATIC, STRING(pev->noise2) );
	if ( pev->noise3 )
		EMIT_SOUND( ENT(pev), CHAN_WEAPON, STRING(pev->noise3), m_volume, ATTN_NORM );

	pev->effects = 0;
}


void CBeamCharge::WarmUp( void )
{
	if ( !ShouldToggle( USE_ON, m_on ) )
		return;

	m_on = TRUE;
	pev->scale = 0;
	pev->renderamt = 0;
	Charge();

	if ( pev->noise1 )
		EMIT_SOUND( ENT(pev), CHAN_WEAPON, STRING(pev->noise1), m_volume, ATTN_NORM );

	pev->effects = 0;

	if ( pev->spawnflags & SF_BEAMCHARGE_PARTICLES )
	{
		MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, pev->origin );
			WRITE_BYTE( TE_IMPLOSION );
			WRITE_COORD( pev->origin.x );
			WRITE_COORD( pev->origin.y );
			WRITE_COORD( pev->origin.z );
			WRITE_BYTE( min( 255, (int)m_fxScale ) );	// radius
			WRITE_BYTE( RANDOM_LONG( 160, 255 ) );	// count
			WRITE_BYTE( min( 255, (int)(m_flWarmTime * 10) ) );	// life
		MESSAGE_END();
	}
}


void CBeamCharge::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if ( !ShouldToggle( useType, m_on ) )
		return;

	if ( m_on )
		CoolDown();
	else
		WarmUp();
}


// sic: an unused inline function, a stand-in for text retail's beams.cpp had
// and ours lacks here (11 to 13 statements fit; with the one before
// CBeamTurret::KeyValue it puts CBeam's inline virtuals where retail emits
// them in beams.obj)
inline void sic_il_beams( int *p )
{
	p[0] = 0;
	p[1] = 0;
	p[2] = 0;
	p[3] = 0;
	p[4] = 0;
	p[5] = 0;
	p[6] = 0;
	p[7] = 0;
	p[8] = 0;
	p[9] = 0;
	p[10] = 0;
	p[11] = 0;
}

void CBeamCharge::KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "beam_texture"))
	{
		m_iszBeamSpriteName = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "spritescale"))
	{
		m_scale = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "light"))
	{
		UTIL_StringToVector( m_light, pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "FXScale"))
	{
		m_fxScale = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "Cooldown"))
	{
		m_flCoolTime = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "Warmup"))
	{
		m_flWarmTime = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "alpha"))
	{
		m_alpha = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "slack"))
	{
		m_beamSlack = atof(pkvd->szValue) * 0.00873;	// degrees, as VECTOR_CONE_1DEGREES
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "volume"))
	{
		m_volume = atof(pkvd->szValue);
		if ( m_volume > 10 )
			m_volume = 10;
		if ( m_volume < 0 )
			m_volume = 0;
		m_volume /= 10;
		pkvd->fHandled = TRUE;
	}
	else
		CSprite::KeyValue( pkvd );
}


//=========================================================
// beam_turret: no model, it aims a beam_cannon (target) as if the beam came from here, after an
// optional beam_charge (netname) has charged up
//=========================================================

// m_type
#define BEAMTURRET_NORMAL	0
#define BEAMTURRET_SLASH	1	// a random slash through the target, pev->speed long
#define BEAMTURRET_SHOTS	2	// m_shots shots in m_duration seconds
#define BEAMTURRET_SWEEP	3	// the beam sweeps after the enemy

#define SF_BEAMTURRET_STARTON	0x0001

class CBeamTurret : public CBaseMonster
{
public:
	void	Spawn( void );
	void	KeyValue( KeyValueData *pkvd );
	BOOL	CanBeRepulsed( bool fForce ) { return FALSE; }
	int		GetEntityTriggerState( void ) { return m_on; }
	int		Classify( void ) { return CLASS_NONE; }
	void	Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	int		IRelationship( CBaseEntity *pTarget );

	CBeamCannon *GetBeam( void );
	CBeamCharge *GetCharge( void );
	void	EXPORT SearchThink( void );
	void	EXPORT ActiveThink( void );
	void	Shoot( const Vector &vecTarget );
	void	EXPORT WaitThink( void );
	void	EXPORT BeamThink( void );

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	int		m_allies;
	int		m_on;
	int		m_type;
	int		m_shots;
	float	m_duration;
	float	m_shotDur;
	float	m_startTime;
	float	m_shotStartTime;
	float	m_spread;
	float	m_flFiringArc;
	CBeamCannon *m_pBeam;
	CBeamCharge *m_pCharge;
	Vector	m_point1;	// where the beam is aimed
	Vector	m_point2;	// its movement per think (slash, sweep)
};

LINK_ENTITY_TO_CLASS( beam_turret, CBeamTurret );

TYPEDESCRIPTION	CBeamTurret::m_SaveData[] =
{
	DEFINE_FIELD( CBeamTurret, m_on, FIELD_INTEGER ),
	DEFINE_FIELD( CBeamTurret, m_allies, FIELD_INTEGER ),
	DEFINE_FIELD( CBeamTurret, m_type, FIELD_INTEGER ),
	DEFINE_FIELD( CBeamTurret, m_shots, FIELD_INTEGER ),
	DEFINE_FIELD( CBeamTurret, m_duration, FIELD_FLOAT ),
	DEFINE_FIELD( CBeamTurret, m_shotDur, FIELD_FLOAT ),
	DEFINE_FIELD( CBeamTurret, m_startTime, FIELD_TIME ),
	DEFINE_FIELD( CBeamTurret, m_shotStartTime, FIELD_TIME ),
	DEFINE_FIELD( CBeamTurret, m_spread, FIELD_FLOAT ),
	DEFINE_FIELD( CBeamTurret, m_flFiringArc, FIELD_FLOAT ),
	DEFINE_FIELD( CBeamTurret, m_point1, FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( CBeamTurret, m_point2, FIELD_VECTOR ),
	DEFINE_FIELD( CBeamTurret, m_pBeam, FIELD_CLASSPTR ),
	DEFINE_FIELD( CBeamTurret, m_pCharge, FIELD_CLASSPTR ),
};

IMPLEMENT_SAVERESTORE( CBeamTurret, CBaseMonster );

void CBeamTurret::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if ( !ShouldToggle( useType, m_on ) )
		return;

	if ( m_on )
	{
		m_on = FALSE;
		SetThink( NULL );
		pev->nextthink = 0;
		if ( m_pCharge )
			m_pCharge->CoolDown();
		else if ( m_pBeam )
			m_pBeam->TurnOff();
	}
	else
	{
		m_on = TRUE;
		SetThink( SearchThink );
		pev->nextthink = m_startTime + m_flWait;
	}
}


// sic: an unused inline function, a stand-in for text retail's beams.cpp had
// and ours lacks here (4 to 16 statements fit, with the one before
// CBeamCharge::KeyValue)
inline void sic_il_beams2( int *p )
{
	p[0] = 0;
	p[1] = 0;
	p[2] = 0;
	p[3] = 0;
	p[4] = 0;
	p[5] = 0;
	p[6] = 0;
	p[7] = 0;
	p[8] = 0;
	p[9] = 0;
}

void CBeamTurret::KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "Allies"))
	{
		m_allies = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "type"))
	{
		m_type = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "shots"))
	{
		m_shots = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "Duration"))
	{
		m_duration = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "accuracy"))
	{
		m_spread = gTankAccuracy[atoi(pkvd->szValue)];
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "fov"))
	{
		m_flFiringArc = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CBaseMonster::KeyValue( pkvd );
}


// "Allies": who the beam leaves alone
int CBeamTurret::IRelationship( CBaseEntity *pTarget )
{
	int iClass = pTarget->Classify();

	if ( iClass == CLASS_NONE || iClass == CLASS_BARNACLE || iClass == CLASS_INSECT )
		return R_NO;

	switch ( m_allies )
	{
	case 1:	// the player
		if ( pTarget->IsPlayer() || iClass == 14 )
			return R_NO;
		return R_DL;
	case 2:	// good guys
		if ( pTarget->IsPlayer() || iClass == CLASS_HUMAN_PASSIVE || iClass == CLASS_PLAYER_ALLY || iClass == 14 )
			return R_NO;
		return R_DL;
	case 3:	// human military
		if ( iClass == CLASS_HUMAN_MILITARY || iClass == CLASS_MACHINE )
			return R_NO;
		return R_DL;
	case 4:	// aliens
		if ( iClass >= CLASS_ALIEN_MILITARY && iClass <= CLASS_ALIEN_PREDATOR )
			return R_NO;
		return R_DL;
	case 5:	// everyone but the player
		return pTarget->IsPlayer() ? R_DL : R_NO;
	case 6:	// everyone but scientists
		return ( iClass == CLASS_HUMAN_PASSIVE ) ? R_DL : R_NO;
	}

	return R_DL;
}


void CBeamTurret::Spawn( void )
{
	pev->movetype = MOVETYPE_NONE;
	pev->solid = SOLID_NOT;
	pev->view_ofs = g_vecZero;
	m_flFieldOfView = m_flFiringArc;

	if ( m_type == BEAMTURRET_SHOTS )
	{
		if ( m_shots <= 1 )
			m_shots = 3;
		m_shotDur = m_duration / ( m_shots * 2 - 1 );
	}

	if ( m_duration <= 0 )
		m_duration = 1;

	if ( !(pev->spawnflags & SF_BEAMTURRET_STARTON) && pev->targetname )
	{
		m_on = FALSE;
		SetThink( NULL );
		pev->nextthink = 0;
	}
	else
	{
		m_on = TRUE;
		SetThink( SearchThink );
		pev->nextthink = gpGlobals->time + 0.2;
	}
}


CBeamCannon *CBeamTurret::GetBeam( void )
{
	if ( m_pBeam )
		return m_pBeam;

	edict_t *pent = FIND_ENTITY_BY_TARGETNAME( NULL, STRING(pev->target) );
	while ( !FNullEnt( pent ) )
	{
		if ( FClassnameIs( pent, "beam_cannon" ) )
		{
			m_pBeam = (CBeamCannon *)CBaseEntity::Instance( pent );
			break;
		}
		pent = FIND_ENTITY_BY_TARGETNAME( pent, STRING(pev->target) );
	}
	m_pBeam->pev->spawnflags |= SF_CANNON_DIRECTIONAL;	// sic: no beam_cannon, no check
	return m_pBeam;
}


CBeamCharge *CBeamTurret::GetCharge( void )
{
	if ( m_pCharge )
		return m_pCharge;

	edict_t *pent = FIND_ENTITY_BY_TARGETNAME( NULL, STRING(pev->netname) );
	while ( !FNullEnt( pent ) )
	{
		if ( FClassnameIs( pent, "beam_charge" ) )
		{
			m_pCharge = (CBeamCharge *)CBaseEntity::Instance( pent );
			break;
		}
		pent = FIND_ENTITY_BY_TARGETNAME( pent, STRING(pev->netname) );
	}
	return m_pCharge;
}


void CBeamTurret::SearchThink( void )
{
	pev->nextthink = gpGlobals->time + RANDOM_FLOAT( 0.2, 0.5 );

	if ( m_hEnemy != NULL && !m_hEnemy->IsAlive() )
		m_hEnemy = NULL;

	if ( m_hEnemy == NULL )
	{
		Look( 4096 );
		m_hEnemy = BestVisibleEnemy();
	}

	if ( m_hEnemy != NULL )
		SetThink( ActiveThink );
}


void CBeamTurret::ActiveThink( void )
{
	int fAttack = 0;
	Vector vecDirToEnemy;

	pev->nextthink = gpGlobals->time + 0.1;

	if ( !m_on || m_hEnemy == NULL || !m_hEnemy->IsAlive() )
	{
		m_hEnemy = NULL;
		SetThink( SearchThink );
		return;
	}

	GetBeam();

	Vector vecMid = pev->origin;
	Vector vecMidEnemy;
	int fEnemyVisible = FBoxVisible( pev, m_hEnemy->pev, vecMidEnemy );
	vecMidEnemy = m_hEnemy->BodyTarget( vecMid );
	vecDirToEnemy = vecMidEnemy - vecMid;	// calculate dir and dist to enemy
	float flDistToEnemy = vecDirToEnemy.Length();

	if ( !fEnemyVisible )
	{
		m_hEnemy = NULL;
		SetThink( SearchThink );
		return;
	}

	UTIL_MakeAimVectors( pev->angles );
	Vector vecLOS = vecDirToEnemy.Normalize();

	// Is the Gun looking at the target
	if ( DotProduct( vecLOS, gpGlobals->v_forward ) < m_flFiringArc )
		fAttack = FALSE;
	else
		fAttack = TRUE;

	if ( fAttack )
		Shoot( vecMidEnemy );
	else
	{
		m_hEnemy = NULL;
		SetThink( SearchThink );
	}
}


void CBeamTurret::Shoot( const Vector &vecTarget )
{
	Vector vecDir( vecTarget - pev->origin );
	float flDist = vecDir.Length();
	Vector vecAim( vecDir );
	vecAim = vecAim.Normalize();
	UTIL_MakeAimVectors( UTIL_VecToAngles( vecAim ) );

	// get circular gaussian spread
	float x, y, z;
	do {
		x = RANDOM_FLOAT(-0.5,0.5) + RANDOM_FLOAT(-0.5,0.5);
		y = RANDOM_FLOAT(-0.5,0.5) + RANDOM_FLOAT(-0.5,0.5);
		z = x*x+y*y;
	} while (z > 1);

	Vector vecShoot( vecAim +
					x * m_spread * gpGlobals->v_right +
					y * m_spread * gpGlobals->v_up );
	m_point1 = pev->origin + vecShoot * flDist;
	m_vecLastPosition = vecTarget;

	if ( m_type == BEAMTURRET_SLASH )
	{
		Vector vecCenter( m_point1 );
		Vector vecAngles = Vector( RANDOM_FLOAT( -180, 180 ), RANDOM_FLOAT( -180, 180 ), RANDOM_FLOAT( -180, 180 ) );
		UTIL_MakeVectors( vecAngles );
		m_point1 = vecCenter + gpGlobals->v_forward * RANDOM_FLOAT( 0, pev->speed );
		Vector vecEnd( vecCenter + gpGlobals->v_forward * -RANDOM_FLOAT( 0, pev->speed ) );
		m_point2 = vecEnd - m_point1;
		float flLength = m_point2.Length();
		vecEnd = m_point2.Normalize();
		m_point2 = vecEnd * ( flLength / m_duration ) * 0.05;
	}

	if ( pev->netname )
		GetCharge();

	m_pBeam->m_firePosition = m_point1;
	if ( m_pCharge )
	{
		m_pCharge->WarmUp();
		SetThink( WaitThink );
	}
	else
	{
		m_pBeam->TurnOn();
		SetThink( BeamThink );
		m_shotStartTime = m_startTime = gpGlobals->time;
	}

	pev->nextthink = gpGlobals->time;
}


// wait for the charge to turn the beam on
void CBeamTurret::WaitThink( void )
{
	if ( m_pBeam->IsOn() )
	{
		SetThink( BeamThink );
		pev->nextthink = m_shotStartTime = m_startTime = gpGlobals->time;
	}
	else
		pev->nextthink = gpGlobals->time + 0.05;
}


void CBeamTurret::BeamThink( void )
{
	pev->nextthink = gpGlobals->time + 0.05;
	m_pBeam->m_firePosition = m_point1;

	if ( m_type == BEAMTURRET_SLASH )
		m_point1 = m_point1 + m_point2;

	if ( m_type == BEAMTURRET_SWEEP )
	{
		if ( m_hEnemy == NULL || !m_hEnemy->IsAlive() )
		{
			Look( 4096 );
			m_hEnemy = BestVisibleEnemy();
		}
		if ( m_hEnemy != NULL )
		{
			m_point2 = ( m_hEnemy->BodyTarget( pev->origin ) - m_point1 ).Normalize() * 5;
		}
		m_point1 = m_point1 + m_point2;
	}

	float flEndTime = m_startTime + m_duration, flShotEndTime = m_shotStartTime + m_shotDur;	// sic: unused, but their size gives VC6 retail's inlining budget for the aim below
	if ( m_startTime + m_duration < gpGlobals->time )
	{
		if ( m_pBeam->m_pfnThink == &CBeamCannon::StartFxThink )
			m_pBeam->TurnOnBeam();
		if ( m_pCharge )
			m_pCharge->CoolDown();
		else
			m_pBeam->TurnOff();
		SetThink( SearchThink );
		pev->nextthink = gpGlobals->time + m_flWait + RANDOM_FLOAT( 0, m_flWait * 0.3 );
		return;
	}

	if ( m_type == BEAMTURRET_SHOTS && m_shotStartTime + m_shotDur < gpGlobals->time )
	{
		m_shotStartTime = gpGlobals->time;
		if ( m_pBeam->IsOn() )
		{
			// between shots: aim again
			if ( m_hEnemy != NULL )
				m_vecLastPosition = m_hEnemy->BodyTarget( pev->origin );

			Vector vecDir( m_vecLastPosition - pev->origin );
			float flDist = vecDir.Length();
			Vector vecAim = vecDir;
			vecAim = vecAim.Normalize();
			UTIL_MakeAimVectors( UTIL_VecToAngles( vecAim ) );

			// get circular gaussian spread
			float x, y, z;
			do {
				x = RANDOM_FLOAT(-0.5,0.5) + RANDOM_FLOAT(-0.5,0.5);
				y = RANDOM_FLOAT(-0.5,0.5) + RANDOM_FLOAT(-0.5,0.5);
				z = x*x+y*y;
			} while (z > 1);

			Vector vecShoot = vecAim +
							x * m_spread * gpGlobals->v_right +
							y * m_spread * gpGlobals->v_up;
			m_point1 = pev->origin + vecShoot * flDist;
			m_pBeam->TurnOffBeam();
		}
		else
			m_pBeam->TurnOnBeam();
	}
}


//=========================================================
// beam_multi: up to 16 beam_cannons with this one's settings
//=========================================================

#define MAX_MULTIBEAMS		16

// m_type
#define MULTIBEAM_POINT_MULTIPLE	0	// one start, the end entities
#define MULTIBEAM_MULTIPLE_POINT	1	// the start entities, one end
#define MULTIBEAM_GRID				2	// random start and end entities
#define MULTIBEAM_TARGET_PAIRS		3	// each start entity to its target
#define MULTIBEAM_TARGET_STRING		4	// a chain of targets from one start
#define MULTIBEAM_RANDOM_POINT		5	// from the start entities in random directions
#define MULTIBEAM_RANDOM_AREA		6	// random beams on the surfaces around the start entities

class CMultiBeam : public CBeamCannon
{
public:
	void	Spawn( void );
	void	Precache( void );
	void	KeyValue( KeyValueData *pkvd );
	void	Killed( entvars_t *pevAttacker, int iGib ) { ClearBeams(); CBeamCannon::Killed( pevAttacker, iGib ); }
	int		GetEntityTriggerState( void ) { return pev->nextthink != 0; }
	void	Think( void );
	void	RandomArea( void );
	void	RandomPoint( void );
	void	CreateBeams( void );
	void	ClearBeams( void );
	void	Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	CBeamCannon *m_pBeams[MAX_MULTIBEAMS];
	int		m_iBeamCount;
	int		m_type;
	float	m_flThinkRate;
	int		m_startName;
	int		m_endName;
};

LINK_ENTITY_TO_CLASS( beam_multi, CMultiBeam );

TYPEDESCRIPTION	CMultiBeam::m_SaveData[] =
{
	DEFINE_FIELD( CMultiBeam, m_iBeamCount, FIELD_INTEGER ),
	DEFINE_ARRAY( CMultiBeam, m_pBeams, FIELD_CLASSPTR, MAX_MULTIBEAMS ),
	DEFINE_FIELD( CMultiBeam, m_type, FIELD_INTEGER ),
	DEFINE_FIELD( CMultiBeam, m_startName, FIELD_STRING ),
	DEFINE_FIELD( CMultiBeam, m_endName, FIELD_STRING ),
	DEFINE_FIELD( CMultiBeam, m_flThinkRate, FIELD_FLOAT ),
};

IMPLEMENT_SAVERESTORE( CMultiBeam, CBeamCannon );

void CMultiBeam::Spawn( void )
{
	if ( FStringNull( pev->model ) )
	{
		SetThink( SUB_Remove );
		return;
	}
	Precache( );

	m_iBeamCount = min( MAX_MULTIBEAMS, m_iBeamCount );
	if ( m_flThinkRate < 0 )
		m_flThinkRate = 0.05;

	pev->solid = SOLID_NOT;
	pev->effects = EF_NODRAW;

	if ( FStringNull( pev->targetname ) || (pev->spawnflags & SF_CANNON_STARTON) )
		pev->nextthink = gpGlobals->time + 0.1;
}


void CMultiBeam::Precache( void )
{
	PRECACHE_MODEL( (char *)STRING(pev->model) );
	if ( m_iszStartSpriteName )
		PRECACHE_MODEL( (char *)STRING(m_iszStartSpriteName) );
	if ( m_iszEndSpriteName )
		m_iEndSpriteIndex = PRECACHE_MODEL( (char *)STRING(m_iszEndSpriteName) );
}


void CMultiBeam::KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "beamcount"))
	{
		m_iBeamCount = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "type"))
	{
		m_type = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "start"))
	{
		m_startName = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "end"))
	{
		m_endName = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "thinkrate"))
	{
		m_flThinkRate = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CBeamCannon::KeyValue( pkvd );
}


void CMultiBeam::Think( void )
{
	pev->nextthink = gpGlobals->time + m_flThinkRate;

	if ( !m_pBeams[0] )
		CreateBeams();

	CBaseEntity *pStart = NULL;
	CBaseEntity *pEnd = NULL;
	int i;

	switch ( m_type )
	{
	case MULTIBEAM_MULTIPLE_POINT:
		pEnd = RandomTargetname( STRING(m_endName) );
		for ( i = 0; i < m_iBeamCount && m_pBeams[i]; i++ )
		{
			pStart = UTIL_FindEntityByTargetname( pStart, STRING(m_startName) );
			if ( pStart )
				m_pBeams[i]->SetStartPos( pStart->pev->origin );
			if ( pEnd )
			{
				CBeamCannon *pBeam = m_pBeams[i];
				pBeam->SetEndPos( pEnd->pev->origin );
				pBeam->m_firePosition = pBeam->pev->angles;
			}
		}
		break;

	case MULTIBEAM_GRID:
		for ( i = 0; i < m_iBeamCount && m_pBeams[i]; i++ )
		{
			pStart = RandomTargetname( STRING(m_startName) );
			pEnd = RandomTargetname( STRING(m_endName) );
			if ( pStart )
				m_pBeams[i]->SetStartPos( pStart->pev->origin );
			if ( pEnd )
			{
				CBeamCannon *pBeam = m_pBeams[i];
				pBeam->SetEndPos( pEnd->pev->origin );
				pBeam->m_firePosition = pBeam->pev->angles;
			}
		}
		break;

	case MULTIBEAM_TARGET_PAIRS:
		for ( i = 0; i < m_iBeamCount && m_pBeams[i]; i++ )
		{
			pStart = UTIL_FindEntityByTargetname( pStart, STRING(m_startName) );
			if ( pStart )
			{
				m_pBeams[i]->SetStartPos( pStart->pev->origin );
				pEnd = UTIL_FindEntityByTargetname( NULL, STRING(pStart->pev->target) );
				if ( pEnd )
				{
					CBeamCannon *pBeam = m_pBeams[i];
					pBeam->SetEndPos( pEnd->pev->origin );
					pBeam->m_firePosition = pBeam->pev->angles;
				}
			}
		}
		break;

	case MULTIBEAM_TARGET_STRING:
		pStart = RandomTargetname( STRING(m_startName) );
		for ( i = 0; i < m_iBeamCount && m_pBeams[i]; i++ )
		{
			if ( pStart )
			{
				m_pBeams[i]->SetStartPos( pStart->pev->origin );
				pStart = UTIL_FindEntityByTargetname( NULL, STRING(pStart->pev->target) );
				if ( pStart )
				{
					CBeamCannon *pBeam = m_pBeams[i];
					pBeam->SetEndPos( pStart->pev->origin );
					pBeam->m_firePosition = pBeam->pev->angles;
				}
			}
		}
		break;

	case MULTIBEAM_RANDOM_POINT:
		RandomPoint();
		break;

	case MULTIBEAM_RANDOM_AREA:
		RandomArea();
		break;

	default:	// point -> multiple
		pStart = RandomTargetname( STRING(m_startName) );
		for ( i = 0; i < m_iBeamCount && m_pBeams[i]; i++ )
		{
			pEnd = UTIL_FindEntityByTargetname( pEnd, STRING(m_endName) );
			if ( pStart )
				m_pBeams[i]->SetStartPos( pStart->pev->origin );
			if ( pEnd )
			{
				CBeamCannon *pBeam = m_pBeams[i];
				pBeam->SetEndPos( pEnd->pev->origin );
				pBeam->m_firePosition = pBeam->pev->angles;
			}
		}
		break;
	}
}


// beams between random points on the surfaces around the start entities
void CMultiBeam::RandomArea( void )
{
	Vector vecSrc = pev->origin;

	for ( int i = 0; i < m_iBeamCount && m_pBeams[i]; i++ )
	{
		CBaseEntity *pStart = RandomTargetname( STRING(m_startName) );
		if ( pStart )
			vecSrc = pStart->pev->origin;

		Vector vecDir1 = Vector( RANDOM_FLOAT( -1.0, 1.0 ), RANDOM_FLOAT( -1.0, 1.0 ),RANDOM_FLOAT( -1.0, 1.0 ) );
		TraceResult		tr;
		UTIL_TraceLine( vecSrc, vecSrc + vecDir1 * 4096, ignore_monsters, ENT(pev), &tr );
		if ( tr.fAllSolid )
			continue;

		vecSrc = tr.vecEndPos;
		UTIL_MakeVectors( UTIL_VecToAngles( tr.vecPlaneNormal ) );
		Vector vecDir2;
		vecDir2 = UTIL_SpreadVector( Vector( 0.39285, 0.39285, 0.39285 ) );
		UTIL_TraceLine( vecSrc, vecSrc + vecDir2 * 4096, ignore_monsters, ENT(pev), &tr );
		if ( tr.fAllSolid )
			continue;

		m_pBeams[i]->SetStartPos( vecSrc );
		CBeamCannon *pBeam = m_pBeams[i];
		pBeam->SetEndPos( tr.vecEndPos );
		pBeam->m_firePosition = pBeam->pev->angles;
	}
}


// beams from the start entities in random directions
void CMultiBeam::RandomPoint( void )
{
	Vector vecSrc = pev->origin;

	for ( int i = 0; i < m_iBeamCount && m_pBeams[i]; i++ )
	{
		CBaseEntity *pStart = RandomTargetname( STRING(m_startName) );
		if ( pStart )
			vecSrc = pStart->pev->origin;

		Vector vecDir1 = Vector( RANDOM_FLOAT( -1.0, 1.0 ), RANDOM_FLOAT( -1.0, 1.0 ),RANDOM_FLOAT( -1.0, 1.0 ) );
		TraceResult		tr;
		UTIL_TraceLine( vecSrc, vecSrc + vecDir1 * 4096, ignore_monsters, ENT(pev), &tr );
		if ( !tr.fAllSolid )
		{
			m_pBeams[i]->SetStartPos( vecSrc );
			CBeamCannon *pBeam = m_pBeams[i];
			pBeam->SetEndPos( tr.vecEndPos );
			pBeam->m_firePosition = pBeam->pev->angles;
		}
	}
}


void CMultiBeam::CreateBeams( void )
{
	if ( m_pBeams[0] )
		ClearBeams();

	for ( int i = 0; i < m_iBeamCount; i++ )
	{
		m_pBeams[i] = GetClassPtr( (CBeamCannon *)NULL );
		m_pBeams[i]->pev->classname = MAKE_STRING("beam_cannon");
		m_pBeams[i]->pev->model = pev->model;
		m_pBeams[i]->m_iszStartSpriteName = m_iszStartSpriteName;
		m_pBeams[i]->m_iszEndSpriteName = m_iszEndSpriteName;
		m_pBeams[i]->m_flStartSpriteScale = m_flStartSpriteScale;
		m_pBeams[i]->m_flEndSpriteScale = m_flEndSpriteScale;
		m_pBeams[i]->pev->spawnflags = (pev->spawnflags & ~SF_CANNON_MIMIC) | SF_CANNON_STARTON;
		m_pBeams[i]->m_endType = m_endType;
		m_pBeams[i]->pev->rendermode = pev->rendermode;
		m_pBeams[i]->pev->rendercolor = pev->rendercolor;
		m_pBeams[i]->pev->renderamt = pev->renderamt;
		m_pBeams[i]->pev->renderfx = pev->renderfx;
		m_pBeams[i]->Spawn();
		m_pBeams[i]->m_damageType = m_damageType;
		m_pBeams[i]->m_startLight = m_startLight;
		m_pBeams[i]->m_endLight = m_endLight;
		m_pBeams[i]->m_endValue = m_endValue;
		m_pBeams[i]->m_flicker = m_flicker;
		m_pBeams[i]->m_iszKillDesc = m_iszKillDesc;
		m_pBeams[i]->m_radius = m_radius;
		m_pBeams[i]->m_scale = m_scale;
		m_pBeams[i]->pev->scale = pev->scale;
		m_pBeams[i]->pev->body = pev->body;
		m_pBeams[i]->pev->frame = pev->frame;
		m_pBeams[i]->pev->framerate = pev->framerate;
		m_pBeams[i]->pev->animtime = pev->animtime;
		m_pBeams[i]->pev->owner = pev->owner;
	}
}


void CMultiBeam::ClearBeams( void )
{
	for ( int i = 0; i < m_iBeamCount; i++ )
	{
		if ( m_pBeams[i] )
		{
			m_pBeams[i]->Killed( pev, GIB_NORMAL );
			m_pBeams[i] = NULL;
		}
	}
}


void CMultiBeam::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	bool active = pev->nextthink != 0;

	if ( !ShouldToggle( useType, active ) )
		return;

	if ( active )
	{
		pev->nextthink = 0;
		ClearBeams();
	}
	else
		pev->nextthink = gpGlobals->time + 0.1;
}

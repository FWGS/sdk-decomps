/***
*
*	Copyright (c) 1999, Valve LLC. All rights reserved.
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
// func_wind: an invisible brush volume whose wind blows in
// gusts. Each gust is half a sine wave of random strength
// and length; leaf_god's leaves are carried by it.
//=========================================================

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "funcwind.h"

LINK_ENTITY_TO_CLASS( func_wind, CFuncWind );

TYPEDESCRIPTION	CFuncWind::m_SaveData[] =
{
	DEFINE_FIELD( CFuncWind, m_Wind, FIELD_VECTOR ),
	DEFINE_FIELD( CFuncWind, m_Direction, FIELD_VECTOR ),
	DEFINE_FIELD( CFuncWind, m_fMaxAmp, FIELD_FLOAT ),
	DEFINE_FIELD( CFuncWind, m_fMinAmp, FIELD_FLOAT ),
	DEFINE_FIELD( CFuncWind, m_fMaxFreq, FIELD_FLOAT ),
	DEFINE_FIELD( CFuncWind, m_fMinFreq, FIELD_FLOAT ),
	DEFINE_FIELD( CFuncWind, m_fAmp, FIELD_FLOAT ),
	DEFINE_FIELD( CFuncWind, m_fFreq, FIELD_FLOAT ),
	DEFINE_FIELD( CFuncWind, m_fSqrTime, FIELD_FLOAT ),
	DEFINE_FIELD( CFuncWind, m_fSqrScaler, FIELD_FLOAT ),
	DEFINE_FIELD( CFuncWind, theta, FIELD_FLOAT ),
};

IMPLEMENT_SAVERESTORE( CFuncWind, CBaseEntity );

void CFuncWind::KeyValue( KeyValueData *pkvd )
{
	// sic: separate ifs, so the else at the end marks every key
	// but dir_z unhandled again
	if ( FStrEq( pkvd->szKeyName, "minamp" ) )
	{
		m_fMinAmp = atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	if ( FStrEq( pkvd->szKeyName, "maxamp" ) )
	{
		m_fMaxAmp = atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	if ( FStrEq( pkvd->szKeyName, "minfreq" ) )
	{
		m_fMinFreq = atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	if ( FStrEq( pkvd->szKeyName, "maxfreq" ) )
	{
		m_fMaxFreq = atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	if ( FStrEq( pkvd->szKeyName, "dir_x" ) )
	{
		m_Direction.x = atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	if ( FStrEq( pkvd->szKeyName, "dir_y" ) )
	{
		m_Direction.y = atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	if ( FStrEq( pkvd->szKeyName, "dir_z" ) )
	{
		m_Direction.z = atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		CBaseEntity::KeyValue( pkvd );
}

void CFuncWind::Spawn( void )
{
	pev->angles = g_vecZero;
	pev->effects |= EF_NODRAW;
	pev->movetype = MOVETYPE_NONE;
	pev->solid = SOLID_NOT;

	UTIL_SetOrigin( pev, pev->origin );
	SET_MODEL( ENT( pev ), STRING( pev->model ) );

	UTIL_MakeVectors( m_Direction );
	m_Direction = gpGlobals->v_forward;

	// past pi, so the first think starts a gust
	theta = M_PI + 1;
	pev->nextthink = gpGlobals->time + 0.1;
}

void CFuncWind::Think( void )
{
	pev->nextthink = gpGlobals->time + 0.1;

	if ( theta >= M_PI )
	{
		// a new gust
		m_fAmp = RANDOM_FLOAT( m_fMinAmp, m_fMaxAmp );
		m_fFreq = RANDOM_FLOAT( m_fMinFreq, m_fMaxFreq );
		theta = 0;
		m_fAmp = ( m_fAmp - m_fMinAmp ) * ( m_fFreq / ( ( m_fMaxFreq - m_fMinFreq ) / 2 ) ) + m_fMinAmp;
	}
	else
	{
		m_Wind = m_Direction * ( sin( theta ) * m_fAmp );
		theta += M_PI / ( m_fFreq * 10 );
	}
}

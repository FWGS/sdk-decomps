//
// rain.cpp
//
// Purpose: rain and water streams (events/rain.sc, env_rain and the water
//          monster), moved clientside in 0.8a. Names ours but EV_Rain,
//          rain_t::Think, CRainVolume's, RainEffectThink and RainEffectVidInit.
//
// $NoKeywords: $
//=============================================================================

#include "hud.h"
#include "cl_util.h"
#include "const.h"
#include "entity_state.h"
#include "cl_entity.h"
#include "r_efx.h"
#include "event_args.h"

//=========================================================
// iCount tracer particles from random points in the entity's
// box, along its forward vector (or straight down from 200
// units lower, sic: the direction is up), at flSpeed
//=========================================================
void EmitFallingDrops( cl_entity_t *pEnt, float flSpeed, int iType, int iCount, float flLife, bool bDown )
{
	Vector vecForward, vecRight, vecUp;

	if ( bDown )
		vecForward = Vector( 0, 0, 1 );
	else
		gEngfuncs.pfnAngleVectors( pEnt->angles, vecForward, vecRight, vecUp );

	Vector vecVelocity = vecForward * flSpeed;

	for ( int i = 0; i < iCount; i++ )
	{
		Vector vecOrigin;
		vecOrigin.x = gEngfuncs.pfnRandomFloat( pEnt->curstate.mins.x, pEnt->curstate.maxs.x );
		vecOrigin.y = gEngfuncs.pfnRandomFloat( pEnt->curstate.mins.y, pEnt->curstate.maxs.y );
		vecOrigin.z = gEngfuncs.pfnRandomFloat( pEnt->curstate.mins.z, pEnt->curstate.maxs.z );
		vecOrigin = vecOrigin + pEnt->origin;
		int iColor;

		// the tracer colours (the engine's tracer palette)
		switch ( iType )
		{
		case 1:
			iColor = gEngfuncs.pfnRandomLong( 1, 2 ) * 2;
			break;
		case 2:
			iColor = gEngfuncs.pfnRandomLong( 0, 1 ) * 4 + 1;
			break;
		case 3:
		case 4:
		case 5:
		case 6:
		case 7:
			iColor = iType - 3;
			break;
		case 8:
			iColor = gEngfuncs.pfnRandomLong( 0, 12 );
			break;
		case 10:
			if ( gEngfuncs.pfnRandomFloat( 0, 1 ) < 0.4 )
			{
				iColor = 0;
				break;
			}
			// fall through
		case 9:
			iColor = gEngfuncs.pfnRandomLong( 0, 3 ) + 8;
			break;
		default:
			{
				float flRand = gEngfuncs.pfnRandomFloat( 0, 1 );
				if ( flRand < 0.4 )
					iColor = 7;
				else if ( flRand < 0.7 )
					iColor = 3;
				else
					iColor = 12;
			}
			break;
		}

		if ( bDown )
			vecOrigin.z -= 200;

		Vector vecVel = vecVelocity * gEngfuncs.pfnRandomFloat( 0.9, 1.1 );
		gEngfuncs.pEfxAPI->R_UserTracerParticle( vecOrigin, vecVel, flLife, iColor, 1.0, 0, NULL );
	}
}

#define MAX_RAIN	32

struct rain_t
{
	void Think( float flTime );

	int		m_iEntity;
	int		m_iType;		// colours
	float	m_flInterval;	// between bursts
	int		m_iCount;		// particles a burst
	float	m_flSpeed;
	float	m_flLife;
	float	m_flNextTime;
	bool	m_bDown;
};

class CRainVolume
{
public:
	CRainVolume();

	void Initialize( void );
	void Think( float flTime );
	void Start( int iEntity, int iType, float flSpeed, float flLife, int iCount, float flInterval, bool bDown );
	void End( int iEntity );

	rain_t	m_Rain[ MAX_RAIN ];
};

CRainVolume g_Rain;

CRainVolume::CRainVolume()
{
	Initialize();
}

void CRainVolume::Initialize( void )
{
	memset( this, 0, sizeof( *this ) );
}

void rain_t::Think( float flTime )
{
	cl_entity_t *pEnt = gEngfuncs.GetEntityByIndex( m_iEntity );

	if ( !pEnt )
	{
		m_iEntity = 0;
		return;
	}

	m_flNextTime = flTime + m_flInterval;
	EmitFallingDrops( pEnt, m_flSpeed, m_iType, m_iCount, m_flLife, m_bDown );
}

void CRainVolume::Think( float flTime )
{
	for ( int i = 0; i < MAX_RAIN; i++ )
	{
		if ( m_Rain[i].m_iEntity && m_Rain[i].m_flNextTime <= flTime )
			m_Rain[i].Think( flTime );
	}
}

void CRainVolume::Start( int iEntity, int iType, float flSpeed, float flLife, int iCount, float flInterval, bool bDown )
{
	rain_t *pRain = &m_Rain[0];

	// the entity's own slot, or the first free one (slot 0 when all are taken)
	for ( int i = 0; i < MAX_RAIN; i++ )
	{
		if ( m_Rain[i].m_iEntity == iEntity )
		{
			pRain = &m_Rain[i];
			break;
		}

		if ( pRain->m_iEntity && !m_Rain[i].m_iEntity )
			pRain = &m_Rain[i];
	}

	pRain->m_iEntity = iEntity;
	pRain->m_iCount = iCount;
	pRain->m_iType = iType;
	pRain->m_flSpeed = flSpeed;
	pRain->m_flLife = flLife;
	pRain->m_flInterval = flInterval;
	pRain->m_bDown = bDown;
	pRain->m_flNextTime = gEngfuncs.GetClientTime();
}

void CRainVolume::End( int iEntity )
{
	for ( int i = 0; i < MAX_RAIN; i++ )
	{
		if ( m_Rain[i].m_iEntity == iEntity )
		{
			m_Rain[i].m_iEntity = 0;
			return;
		}
	}
}

void RainEffectThink( float flTime )
{
	g_Rain.Think( flTime );
}

void RainEffectVidInit( void )
{
	g_Rain.Initialize();
}

//=========================================================
// events/rain.sc: bparam1 stops the entity's rain; otherwise
// iparam1 is the colour type (low 4 bits) and the particles a
// burst (the rest), iparam2 the speed, fparam1 the particles'
// life, fparam2 the time between bursts, bparam2 straight down
//=========================================================
extern "C" void EV_Rain( event_args_t *args )
{
	if ( args->bparam1 )
	{
		g_Rain.End( args->entindex );
		return;
	}

	g_Rain.Start( args->entindex, args->iparam1 & 15, args->iparam2, args->fparam1, args->iparam1 >> 4, args->fparam2, args->bparam2 != 0 );
}

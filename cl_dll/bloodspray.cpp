//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: blood sprays: xblood sprites, and the blood trails of gibs and
//          wounded monsters (events/bloodspray.sc), moved clientside in 0.8a
//
// $NoKeywords: $
//=============================================================================

#include "hud.h"
#include "cl_util.h"
#include "const.h"
#include "entity_state.h"
#include "cl_entity.h"
#include "r_efx.h"
#include "event_api.h"
#include "event_args.h"
#include "pm_defs.h"
#include "pmtrace.h"

#include <stdio.h>

bool EV_IsInPVS( cl_entity_t *pEntity );

static void EV_BloodDecalHit( TEMPENTITY *pEnt, pmtrace_t *pTrace );

//=========================================================
// Blood sprites: one with no spread, or iCount of them spread
// around vecDir; bDecal ones leave a decal where they land
//=========================================================
bool XBlood( Vector &origin, const Vector &vecDir, int iCount, float flScale, float flSpeed, bool bDecal )
{
	int iModel = gEngfuncs.pEventAPI->EV_FindModelIndex( "sprites/xblood.spr" );

	if ( !iCount )
	{
		TEMPENTITY *pTemp = gEngfuncs.pEfxAPI->R_TempSprite( origin, vecDir * flSpeed, flScale, iModel, kRenderTransTexture, kRenderFxNone, 255, 5, FTENT_SLOWGRAVITY | FTENT_COLLIDEWORLD | FTENT_SPRCYCLE | FTENT_COLLIDEKILL );

		if ( pTemp )
		{
			pTemp->hitcallback = EV_BloodDecalHit;
			return true;
		}

		return false;
	}

	for ( int i = 0; i < iCount; i++ )
	{
		Vector vecVelocity( gEngfuncs.pfnRandomFloat( -0.3, 0.3 ), gEngfuncs.pfnRandomFloat( -0.3, 0.3 ), gEngfuncs.pfnRandomFloat( -0.3, 0.3 ) );
		float flVel = gEngfuncs.pfnRandomFloat( 0.75, 1.25 ) * flSpeed;
		Vector vecSpray = ( vecDir + vecVelocity ).Normalize();
		vecVelocity = vecSpray * flVel;

		if ( bDecal )
		{
			TEMPENTITY *pTemp = gEngfuncs.pEfxAPI->R_TempSprite( origin, vecVelocity, flScale, iModel, kRenderTransTexture, kRenderFxNone, 255, 5, FTENT_SLOWGRAVITY | FTENT_COLLIDEWORLD | FTENT_SPRCYCLE | FTENT_COLLIDEKILL );

			if ( pTemp )
				pTemp->hitcallback = EV_BloodDecalHit;
		}
		else
		{
			gEngfuncs.pEfxAPI->R_TempSprite( origin, vecVelocity, flScale, iModel, kRenderTransTexture, kRenderFxNone, 255, 5, FTENT_GRAVITY | FTENT_SPRCYCLE );
		}
	}

	return true;
}

//=========================================================
// A blood sprite hit something: a red decal there
//=========================================================
static void EV_BloodDecalHit( TEMPENTITY *pEnt, pmtrace_t *pTrace )
{
	if ( !pTrace )
		return;

	if ( !gEngfuncs.pEventAPI->EV_GetPhysent( pTrace->ent ) )
		return;

	float flDecals = gEngfuncs.pfnGetCvarFloat( "r_decals" );

	if ( gEngfuncs.pfnRandomLong( 0, 10000 ) < flDecals )
	{
		char szDecal[12];

		sprintf( szDecal, "{blood%i", gEngfuncs.pfnRandomLong( 0, 2 ) + 7 );

		gEngfuncs.pEfxAPI->R_DecalShoot( 
			gEngfuncs.pEfxAPI->Draw_DecalIndex( gEngfuncs.pEfxAPI->Draw_DecalIndexFromName( szDecal ) ), 
			gEngfuncs.pEventAPI->EV_IndexFromTrace( pTrace ), 0, pTrace->endpos, 0 );
	}
}

#define MAX_BLOODSPRAYS	64

// types (iparam1); names ours
#define BLOODSPRAY_NECK		1	// a beheaded body: spurts up from attachment 0 for 4 s, weaker after 1 s
#define BLOODSPRAY_DRIP		2	// a flying head: a drop every 1-4 s
#define BLOODSPRAY_GIB		3	// a flying gib (any other type too): a trail for 10 s while it is in the PVS

struct bloodspray_t
{
	void Think( float flTime, float flRate );

	cl_entity_t	*m_pEntity;
	int			m_iEntity;
	int			m_iType;
	float		m_flStartTime;
	float		m_flNextTime;
	float		m_flScale;
};

class CBloodSpray
{
public:
	CBloodSpray();

	void Initialize( void );
	void Think( float flTime, float flFrametime );
	void Start( int iEntity, int iType );
	void End( int iEntity );

	bloodspray_t	m_Sprays[ MAX_BLOODSPRAYS ];
};

CBloodSpray g_BloodSpray;

CBloodSpray::CBloodSpray()
{
	Initialize();
}

void CBloodSpray::Initialize( void )
{
	memset( this, 0, sizeof( *this ) );
}

void bloodspray_t::Think( float flTime, float flRate )
{
	cl_entity_t *pEnt;

	if ( m_iType == BLOODSPRAY_NECK )
	{
		if ( m_flStartTime + 4 < flTime )
		{
			m_iEntity = 0;
			return;
		}

		m_flNextTime = flTime + 0.5;

		pEnt = gEngfuncs.GetEntityByIndex( m_iEntity );
		if ( pEnt != m_pEntity || !pEnt->curstate.modelindex )
		{
			m_iEntity = 0;
			return;
		}

		if ( flTime - m_flStartTime >= 1 )
			m_flScale *= 0.66;
		else
			m_flNextTime = flTime + 0.1;

		Vector vecOrigin = pEnt->attachment[0];
		Vector vecDir( 0, 0, 1 );

		XBlood( vecOrigin, vecDir, (int)( m_flScale * 20 + 1 ), 0.15, m_flScale * 200, false );
	}
	else if ( m_iType == BLOODSPRAY_DRIP )
	{
		pEnt = gEngfuncs.GetEntityByIndex( m_iEntity );
		if ( pEnt != m_pEntity || !pEnt->curstate.modelindex )
		{
			m_iEntity = 0;
			return;
		}

		m_flNextTime = flTime + gEngfuncs.pfnRandomFloat( 1, 4 );

		Vector vecDir( 0, 0, 0 );
		XBlood( pEnt->origin, vecDir, 0, 0.2, 0, false );
	}
	else
	{
		if ( m_flStartTime + 10 < flTime )
		{
			m_iEntity = 0;
			return;
		}

		pEnt = gEngfuncs.GetEntityByIndex( m_iEntity );
		if ( pEnt != m_pEntity || !pEnt->curstate.modelindex || !EV_IsInPVS( pEnt ) )
		{
			m_iEntity = 0;
			return;
		}

		m_flNextTime = flTime + 0.05;

		if ( gEngfuncs.pfnRandomFloat( 1, 14 ) < flRate )
		{
			Vector vecDir( 0, 0, 0 );
			if ( !XBlood( pEnt->origin, vecDir, 0, 0.3, 0, false ) )
				m_iEntity = 0;
		}
	}
}

void CBloodSpray::Think( float flTime, float flFrametime )
{
	float flRate = 1 / flFrametime;

	for ( int i = 0; i < MAX_BLOODSPRAYS; i++ )
	{
		if ( m_Sprays[i].m_iEntity && m_Sprays[i].m_flNextTime <= flTime )
			m_Sprays[i].Think( flTime, flRate );
	}
}

void CBloodSpray::Start( int iEntity, int iType )
{
	bloodspray_t *pSpray = &m_Sprays[0];

	for ( int i = 0; i < MAX_BLOODSPRAYS; i++ )
	{
		if ( !m_Sprays[i].m_iEntity || m_Sprays[i].m_flStartTime < pSpray->m_flStartTime )
		{
			pSpray = &m_Sprays[i];
			if ( !pSpray->m_iEntity )
				break;
		}
	}

	pSpray->m_iEntity = iEntity;
	pSpray->m_pEntity = gEngfuncs.GetEntityByIndex( iEntity );
	pSpray->m_iType = iType;
	pSpray->m_flScale = 1.0;
	pSpray->m_flStartTime = pSpray->m_flNextTime = gEngfuncs.GetClientTime();
}

void CBloodSpray::End( int iEntity )
{
	for ( int i = 0; i < MAX_BLOODSPRAYS; i++ )
	{
		if ( m_Sprays[i].m_iEntity == iEntity )
		{
			m_Sprays[i].m_iEntity = 0;
			return;
		}
	}
}

void BloodSprayThink( float flTime, float flFrametime )
{
	g_BloodSpray.Think( flTime, flFrametime );
}

void BloodSprayVidInit( void )
{
	g_BloodSpray.Initialize();
}

//=========================================================
// events/bloodspray.sc: iparam1 the trail type, bparam1 set
// to stop the entity's trail
//=========================================================
extern "C" void EV_BloodSpray( event_args_t *args )
{
	if ( args->bparam1 )
		g_BloodSpray.End( args->entindex );
	else
		g_BloodSpray.Start( args->entindex, args->iparam1 );
}

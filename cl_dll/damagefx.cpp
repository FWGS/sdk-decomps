//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: burning and electrocuted players and monsters: flames and
//          lightning arcs on the bodies the server names in DamageFX
//
// $NoKeywords: $
//=============================================================================

#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"
#include "const.h"
#include "entity_state.h"
#include "cl_entity.h"
#include "r_efx.h"
#include "event_api.h"

bool EV_IsEntityVisible( cl_entity_t *pEntity );

int g_iFlameModel;		// name ours
int g_iLightningModel;	// name ours

//=========================================================
// One flame sprite rising from vecOrigin
//=========================================================
bool FX_BodyFlame( float *origin )	// name ours
{
	Vector vecVelocity( 0.0f, 0.0f, gEngfuncs.pfnRandomFloat( 40, 80 ) );

	TEMPENTITY *pTemp = gEngfuncs.pEfxAPI->R_TempSprite( origin, vecVelocity, 0.8, g_iFlameModel, kRenderTransAdd, kRenderFxNone, 255, 5, FTENT_SPRANIMATE );

	if ( pTemp )
	{
		pTemp->entity.curstate.framerate = 10;
		return true;
	}

	return false;
}

//=========================================================
// iCount lightning arcs between random points of a body,
// each with a spark shower
//=========================================================
void FX_Sparks( Vector &vecOrigin, int iCount, float flRadius )	// name ours
{
	for ( int i = 0; i < iCount; i++ )
	{
		Vector vecAngles, vecForward, vecRight;
		Vector vecStart, vecEnd;	// also take the unused up vectors (sic)

		vecAngles[0] = gEngfuncs.pfnRandomFloat( -180, 180 );
		vecAngles[1] = gEngfuncs.pfnRandomFloat( -180, 180 );
		vecAngles[2] = gEngfuncs.pfnRandomFloat( -180, 180 );
		gEngfuncs.pfnAngleVectors( vecAngles, vecForward, vecRight, vecEnd );
		vecStart = vecOrigin + vecForward * gEngfuncs.pfnRandomFloat( 0, flRadius );

		vecAngles[0] = gEngfuncs.pfnRandomFloat( -180, 180 );
		vecAngles[1] = gEngfuncs.pfnRandomFloat( -180, 180 );
		vecAngles[2] = gEngfuncs.pfnRandomFloat( -180, 180 );
		gEngfuncs.pfnAngleVectors( vecAngles, vecForward, vecRight, vecEnd );
		vecEnd = vecOrigin + vecForward * gEngfuncs.pfnRandomFloat( 0, flRadius );

		Vector vecColor( 0.0, 0.5, 1.0 );

		int iColor = gEngfuncs.pfnRandomLong( 0, 4 );
		if ( iColor == 3 )
			vecColor.y = 0;
		else if ( iColor == 4 )
			vecColor = Vector( 1, 1, 1 );

		gEngfuncs.pEfxAPI->R_BeamPoints( vecStart, vecEnd, g_iLightningModel, gEngfuncs.pfnRandomFloat( 0.2, 0.5 ), 2, 3, 0.5, 0, 0, 10, vecColor.x, vecColor.y, vecColor.z );

		vecAngles[0] = gEngfuncs.pfnRandomFloat( -180, 180 );
		vecAngles[1] = gEngfuncs.pfnRandomFloat( -180, 180 );
		vecAngles[2] = gEngfuncs.pfnRandomFloat( -180, 180 );
		gEngfuncs.pfnAngleVectors( vecAngles, vecForward, vecRight, vecStart );
		vecAngles = vecOrigin + vecForward * gEngfuncs.pfnRandomFloat( 0, flRadius );

		gEngfuncs.pEfxAPI->R_SparkShower( vecAngles );
	}
}

#define MAX_DAMAGEFX	64

struct damagefx_t
{
	cl_entity_t	*m_pEntity;
	int			m_iEntity;
	int			m_iModel;
	float		m_flStartTime;
	float		m_flRadius;
	int			m_iBurn;
	int			m_iShock;
};

class CDamageFX
{
public:
	CDamageFX();

	void Initialize( void );
	void Think( float flTime, float flFrametime );
	void Start( int iEntity, int iBurn, int iShock );

	damagefx_t	m_Ents[ MAX_DAMAGEFX ];
	float		m_flAccum;
};

CDamageFX g_DamageFX;

int __MsgFunc_DamageFX( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );

	int iEntity = READ_SHORT();
	int iBurn = READ_BYTE();
	int iShock = READ_BYTE();

	g_DamageFX.Start( iEntity, iBurn, iShock );
	return 1;
}

CDamageFX::CDamageFX()
{
	Initialize();
}

void CDamageFX::Initialize( void )
{
	memset( this, 0, sizeof( *this ) );
}

//=========================================================
// Ten times a second: a flame for each burning body, arcs
// for each electrocuted one, while it stays in the PVS
//=========================================================
void CDamageFX::Think( float flTime, float flFrametime )
{
	m_flAccum += flFrametime;
	if ( m_flAccum < 0.1 )
		return;

	m_flAccum -= 0.1;

	g_iFlameModel = gEngfuncs.pEventAPI->EV_FindModelIndex( "sprites/playerflame.spr" );
	g_iLightningModel = gEngfuncs.pEventAPI->EV_FindModelIndex( "sprites/lgtning.spr" );

	for ( int i = 0; i < MAX_DAMAGEFX; i++ )
	{
		damagefx_t *pFX = &m_Ents[i];

		if ( !pFX->m_iEntity )
			continue;

		cl_entity_t *pEnt = gEngfuncs.GetEntityByIndex( pFX->m_iEntity );
		if ( pEnt != pFX->m_pEntity || pEnt->curstate.modelindex != pFX->m_iModel || !EV_IsEntityVisible( pEnt ) )
		{
			pFX->m_iEntity = 0;
			continue;
		}

		if ( pFX->m_iBurn > 0 )
		{
			Vector vecFlame;
			vecFlame.x = gEngfuncs.pfnRandomFloat( pEnt->curstate.mins.x, pEnt->curstate.maxs.x );
			vecFlame.y = gEngfuncs.pfnRandomFloat( pEnt->curstate.mins.y, pEnt->curstate.maxs.y );
			vecFlame.z = gEngfuncs.pfnRandomFloat( pEnt->curstate.mins.z, pEnt->curstate.maxs.z );
			FX_BodyFlame( pEnt->origin + vecFlame );
			pFX->m_iBurn--;
		}

		if ( pFX->m_iShock > 0 )
		{
			float flHeight = ( pEnt->curstate.maxs.z - pEnt->curstate.mins.z ) * 0.5;
			int iArcs = pFX->m_iShock * 0.2 + 1;

			FX_Sparks( pEnt->origin + Vector( 0.0f, 0.0f, flHeight ), min( iArcs, 10 ), pFX->m_flRadius );
			pFX->m_iShock -= min( 1, iArcs );
		}

		if ( pFX->m_iShock <= 0 && pFX->m_iBurn <= 0 )
			pFX->m_iEntity = 0;
	}
}

void CDamageFX::Start( int iEntity, int iBurn, int iShock )
{
	damagefx_t *pFX = &m_Ents[0];

	for ( int i = 0; i < MAX_DAMAGEFX; i++ )
	{
		if ( m_Ents[i].m_iEntity == iEntity )
		{
			pFX = &m_Ents[i];
			pFX->m_flStartTime = gEngfuncs.GetClientTime();
			pFX->m_iBurn += iBurn;
			pFX->m_iShock += iShock;
			return;
		}

		if ( !m_Ents[i].m_iEntity || m_Ents[i].m_flStartTime < pFX->m_flStartTime )
			pFX = &m_Ents[i];
	}

	pFX->m_iEntity = iEntity;
	pFX->m_pEntity = gEngfuncs.GetEntityByIndex( iEntity );
	pFX->m_iModel = pFX->m_pEntity->curstate.modelindex;
	pFX->m_flStartTime = gEngfuncs.GetClientTime();
	pFX->m_flRadius = ( pFX->m_pEntity->curstate.maxs - pFX->m_pEntity->curstate.mins ).Length() * 0.5;
	pFX->m_iBurn = iBurn;
	pFX->m_iShock = iShock;
}

void CL_DamageTick( float flTime, float flFrametime )	// name ours
{
	g_DamageFX.Think( flTime, flFrametime );
}

void CL_DamageFXReset( void )	// name ours
{
	g_DamageFX.Initialize();
}

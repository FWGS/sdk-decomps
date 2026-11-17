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
#include "../hud.h"
#include "../cl_util.h"
#include "../demo.h"

#include "demo_api.h"
#include "const.h"
#include "entity_state.h"
#include "cl_entity.h"

#include "pm_defs.h"
#include "event_api.h"
#include "entity_types.h"
#include "r_efx.h"

extern BEAM *pBeam;
extern BEAM *pBeam2;
extern int g_iBeamStyle;
extern BEAM *g_pTeslaBeams[5];
extern TEMPENTITY *g_pEgonFlare;
void HUD_GetLastOrg( float *org );

void UpdateBeams ( void )
{
	vec3_t forward, vecSrc, vecEnd, origin, angles, right, up;
	vec3_t view_ofs;
	pmtrace_t tr;
	cl_entity_t *pthisplayer = gEngfuncs.GetLocalPlayer();
	int idx = pthisplayer->index;
	float flTime = gEngfuncs.GetClientTime();
	int i;
		
	// Get our exact viewangles from engine
	gEngfuncs.GetViewAngles( (float *)angles );

	// Determine our last predicted origin
	HUD_GetLastOrg( (float *)&origin );

	AngleVectors( angles, forward, right, up );

	VectorCopy( origin, vecSrc );
	
	VectorMA( vecSrc, 2048, forward, vecEnd );

	gEngfuncs.pEventAPI->EV_SetUpPlayerPrediction( false, true );	
						
	// Store off the old count
	gEngfuncs.pEventAPI->EV_PushPMStates();
					
	// Now add in all of the players.
	gEngfuncs.pEventAPI->EV_SetSolidPlayers ( idx - 1 );	

	gEngfuncs.pEventAPI->EV_SetTraceHull( 2 );
	gEngfuncs.pEventAPI->EV_PlayerTrace( vecSrc, vecEnd, PM_STUDIO_BOX, -1, &tr );

	gEngfuncs.pEventAPI->EV_PopPMStates();

	if ( pBeam )
	{
		pBeam->target = tr.endpos;
		pBeam->die	  = flTime + 0.1; // We keep it alive just a little bit forward in the future, just in case.

		// the gluon gun's beam pulses in blue
		if ( g_iBeamStyle == 1 )
			pBeam->b = fabsf( sin( flTime * 10 ) ) * ( 80 / 255.0f ) + 144 / 255.0f;
	}
		
	if ( pBeam2 )
	{
		pBeam2->target = tr.endpos;
		pBeam2->die	   = flTime + 0.1; // We keep it alive just a little bit forward in the future, just in case.
	}

	for ( i = 0; i < 5; i++ )
	{
		if ( g_pTeslaBeams[i] )
		{
			g_pTeslaBeams[i]->target = tr.endpos;
			g_pTeslaBeams[i]->die = flTime + 0.1;
		}
	}

	if ( g_pEgonFlare )
	{
		g_pEgonFlare->entity.origin = tr.endpos;
		g_pEgonFlare->die = flTime + 0.1;
	}
}

/*
=====================
Game_AddObjects

Add game specific, client-side objects here
=====================
*/
void Game_AddObjects( void )
{
	if ( pBeam || pBeam2 || g_pTeslaBeams[0] )
		UpdateBeams();
}

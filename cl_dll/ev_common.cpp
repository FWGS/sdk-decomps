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
// shared event functions
#include "hud.h"
#include "cl_util.h"
#include "const.h"
#include "entity_state.h"
#include "cl_entity.h"

#include "r_efx.h"

#include "eventscripts.h"
#include "event_api.h"

/*
=================
GetEntity

Return's the requested cl_entity_t
=================
*/
struct cl_entity_s *GetEntity( int idx )
{
	return gEngfuncs.GetEntityByIndex( idx );
}

/*
=================
GetViewEntity

Return's the current weapon/view model
=================
*/
struct cl_entity_s *GetViewEntity( void )
{
	return gEngfuncs.GetViewModel();
}

/*
=================
EV_CreateTracer

Creates a tracer effect
=================
*/
void EV_CreateTracer( float *start, float *end )
{
	gEngfuncs.pEfxAPI->R_TracerEffect( start, end );
}

/*
=================
EV_IsPlayer

Is the entity's index in the player range?
=================
*/
qboolean EV_IsPlayer( int idx )
{
	if ( idx >= 1 && idx <= gEngfuncs.GetMaxClients() )
		return true;

	return false;
}

/*
=================
EV_IsLocal

Is the entity == the local player
=================
*/
qboolean EV_IsLocal( int idx )
{
	return gEngfuncs.pEventAPI->EV_IsLocal( idx - 1 ) ? true : false;
}

/*
=================
EV_GetGunPosition

Figure out the height of the gun
=================
*/
void EV_GetGunPosition( event_args_t *args, float *pos, float *origin )
{
	int idx;
	vec3_t view_ofs;

	idx = args->entindex;

	VectorClear( view_ofs );
	view_ofs[2] = DEFAULT_VIEWHEIGHT;

	if ( EV_IsPlayer( idx ) )
	{
		if ( EV_IsLocal( idx ) )
		{
			// Grab predicted result for local player
			gEngfuncs.pEventAPI->EV_LocalPlayerViewheight( view_ofs );
		}
		else if ( args->ducking == 1 )
		{
			view_ofs[2] = VEC_DUCK_VIEW;
		}
	}

	VectorAdd( origin, view_ofs, pos );
}

/*
=================
EV_EjectBrass

Bullet shell casings
=================
*/
void EV_EjectBrass( float *origin, float *velocity, float rotation, int model, int soundtype, int idx, int angularScale )
{
	vec3_t endpos;
	TEMPENTITY *pTemp;	// name ours

	if ( EV_IsLocal( idx ) )
	{
		VectorClear( endpos );
		endpos[1] = rotation;
		g_ShellList.Add( gHUD.m_flTime );
		pTemp = gEngfuncs.pEfxAPI->R_TempModel( origin, velocity, endpos, 2.0, model, soundtype );

		if ( pTemp )
		{
			pTemp->entity.baseline.angles[0] *= angularScale;
			pTemp->entity.baseline.angles[1] *= angularScale;
			pTemp->entity.baseline.angles[2] *= angularScale;
		}
	}
	else if ( g_ShellList.m_iCount < g_ShellList.m_iMax )
	{
		VectorClear( endpos );
		endpos[1] = rotation;
		g_ShellList.Add( gHUD.m_flTime );
		pTemp = gEngfuncs.pEfxAPI->R_TempModel( origin, velocity, endpos, 2.0, model, soundtype );

		if ( pTemp )
		{
			pTemp->entity.baseline.angles[0] *= angularScale;
			pTemp->entity.baseline.angles[1] *= angularScale;
			pTemp->entity.baseline.angles[2] *= angularScale;
		}
	}
}

/*
=================
EV_GetDefaultShellInfo

Determine where to eject shells from
=================
*/
void EV_GetDefaultShellInfo( event_args_t *args, float *origin, float *velocity, float *ShellVelocity, float *ShellOrigin, float *forward, float *right, float *up, float forwardScale, float upScale, float rightScale, bool bReverseDirection )
{
	int i;
	vec3_t view_ofs;
	float fR, fU, fF;

	int idx;

	idx = args->entindex;

	VectorClear( view_ofs );
	view_ofs[2] = DEFAULT_VIEWHEIGHT;

	if ( EV_IsPlayer( idx ) )
	{
		if ( EV_IsLocal( idx ) )
		{
			gEngfuncs.pEventAPI->EV_LocalPlayerViewheight( view_ofs );
		}
		else if ( args->ducking == 1 )
		{
			view_ofs[2] = VEC_DUCK_VIEW;
		}
	}

	if ( rightScale > 0 )
		fR = -1.0;
	else
		fR = 1.0;

	fR *= gEngfuncs.pfnRandomFloat( 50, 90 );
	fU = gEngfuncs.pfnRandomFloat( 75, 175 );
	fF = gEngfuncs.pfnRandomFloat( -50, 50 );

	velocity[0] = velocity[0] / 2;
	velocity[1] = velocity[1] / 2;
	velocity[2] = velocity[2] / 2;

	for ( i = 0; i < 3; i++ )
	{
		if ( bReverseDirection == true )
			ShellVelocity[i] = velocity[i] - right[i] * fR + up[i] * fU + forward[i] * fF;
		else
			ShellVelocity[i] = velocity[i] + right[i] * fR + up[i] * fU + forward[i] * fF;

		ShellOrigin[i]   = origin[i] + view_ofs[i] + up[i] * upScale + forward[i] * forwardScale + right[i] * rightScale + ( velocity[i] / 5 );
	}
}

/*
=================
EV_MuzzleFlash

Flag weapon/view model for muzzle flash
=================
*/
void EV_MuzzleFlash( void )
{
	// Add muzzle flash to current weapon model
	cl_entity_t *ent = GetViewEntity();
	if ( !ent )
	{
		return;
	}

	// Or in the muzzle flash
	ent->curstate.effects |= EF_MUZZLEFLASH;
}
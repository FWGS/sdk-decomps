/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
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
#include "hud.h"
#include "cl_util.h"
#include "const.h"
#include "entity_state.h"
#include "cl_entity.h"
#include "entity_types.h"
#include "usercmd.h"
#include "pm_defs.h"
#include "pm_materials.h"

#include "eventscripts.h"
#include "ev_hldm.h"

#include "r_efx.h"
#include "event_api.h"
#include "event_args.h"
#include "in_defs.h"

#include <string.h>

#include "r_studioint.h"
#include "com_model.h"

extern engine_studio_api_t IEngineStudio;

static int tracerCount[ 32 ];

extern cvar_t *cl_righthand;

extern "C" char PM_FindTextureType( char *name );

void V_PunchAxis( int axis, float punch );

extern "C"
{

// HLDM
void EV_FireGlock1( struct event_args_s *args  );
void EV_FireGlock2( struct event_args_s *args  );
void EV_FireShotGunSingle( struct event_args_s *args  );
void EV_FireShotGunDouble( struct event_args_s *args  );
void EV_FireMP5( struct event_args_s *args  );
void EV_FirePython( struct event_args_s *args  );
void EV_FireGauss( struct event_args_s *args  );
void EV_SpinGauss( struct event_args_s *args  );
void EV_VehiclePitchAdjust( struct event_args_s *args );
void EV_TrainPitchAdjust( struct event_args_s *args );

// Counter-Strike
void EV_FireUSP( struct event_args_s *args );
void EV_FireMP5N( struct event_args_s *args );
void EV_FireAK47( struct event_args_s *args );
void EV_FireAug( struct event_args_s *args );
void EV_FireDeagle( struct event_args_s *args );
void EV_FireG3SG1( struct event_args_s *args );
void EV_FireGlock18( struct event_args_s *args );
void EV_FireM249( struct event_args_s *args );
void EV_FireM3( struct event_args_s *args );
void EV_FireM4A1( struct event_args_s *args );
void EV_FireMac10( struct event_args_s *args );
void EV_FireP228( struct event_args_s *args );
void EV_FireP90( struct event_args_s *args );
void EV_FireXM1014( struct event_args_s *args );
void EV_FireScout( struct event_args_s *args );
void EV_FireAWP( struct event_args_s *args );
void EV_FireSG552( struct event_args_s *args );
void EV_FireTMP( struct event_args_s *args );
void EV_CreateSmoke( struct event_args_s *args );
void EV_RemoveAllDecals( struct event_args_s *args );
void EV_FireEliteLeft( struct event_args_s *args );
void EV_FireEliteRight( struct event_args_s *args );
void EV_Knife( struct event_args_s *args );
void EV_FireFiveSeven( struct event_args_s *args );
void EV_FireUMP45( struct event_args_s *args );
void EV_FireSG550( struct event_args_s *args );
}

#define VECTOR_CONE_1DEGREES Vector( 0.00873, 0.00873, 0.00873 )
#define VECTOR_CONE_2DEGREES Vector( 0.01745, 0.01745, 0.01745 )
#define VECTOR_CONE_3DEGREES Vector( 0.02618, 0.02618, 0.02618 )
#define VECTOR_CONE_4DEGREES Vector( 0.03490, 0.03490, 0.03490 )
#define VECTOR_CONE_5DEGREES Vector( 0.04362, 0.04362, 0.04362 )
#define VECTOR_CONE_6DEGREES Vector( 0.05234, 0.05234, 0.05234 )
#define VECTOR_CONE_7DEGREES Vector( 0.06105, 0.06105, 0.06105 )	
#define VECTOR_CONE_8DEGREES Vector( 0.06976, 0.06976, 0.06976 )
#define VECTOR_CONE_9DEGREES Vector( 0.07846, 0.07846, 0.07846 )
#define VECTOR_CONE_10DEGREES Vector( 0.08716, 0.08716, 0.08716 )
#define VECTOR_CONE_15DEGREES Vector( 0.13053, 0.13053, 0.13053 )
#define VECTOR_CONE_20DEGREES Vector( 0.17365, 0.17365, 0.17365 )

// play a strike sound based on the texture that was hit by the attack traceline.  VecSrc/VecEnd are the
// original traceline endpoints used by the attacker, iBulletType is the type of bullet that hit the texture.
// returns the texture type that was hit
char EV_HLDM_PlayTextureSound( int idx, pmtrace_t *ptr, float *vecSrc, float *vecEnd, int iBulletType )
{
	// hit the world, try to play sound based on texture material type
	char chTextureType = CHAR_TEX_CONCRETE;
	float fvol;
	char *rgsz[4];
	int cnt;
	float fattn = ATTN_NORM;
	int entity;
	char *pTextureName;
	char texname[ 64 ];
	char szbuffer[ 64 ];

	entity = gEngfuncs.pEventAPI->EV_IndexFromTrace( ptr );

	// FIXME check if playtexture sounds movevar is set
	//

	chTextureType = 0;

	// Player
	if ( entity >= 1 && entity <= gEngfuncs.GetMaxClients() )
	{
		// hit body
		chTextureType = CHAR_TEX_FLESH;
	}
	else if ( entity == 0 )
	{
		// get texture from entity or world (world is ent(0))
		pTextureName = (char *)gEngfuncs.pEventAPI->EV_TraceTexture( ptr->ent, vecSrc, vecEnd );
		
		if ( pTextureName )
		{
			strcpy( texname, pTextureName );
			pTextureName = texname;

			// strip leading '-0' or '+0~' or '{' or '!'
			if (*pTextureName == '-' || *pTextureName == '+')
			{
				pTextureName += 2;
			}

			if (*pTextureName == '{' || *pTextureName == '!' || *pTextureName == '~' || *pTextureName == ' ')
			{
				pTextureName++;
			}
			
			// '}}'
			strcpy( szbuffer, pTextureName );
			szbuffer[ CBTEXTURENAMEMAX - 1 ] = 0;
				
			// get texture type
			chTextureType = PM_FindTextureType( szbuffer );	
		}
	}
	
	switch (chTextureType)
	{
	default:
	case CHAR_TEX_CONCRETE: fvol = 0.9;
		rgsz[0] = "player/pl_step1.wav";
		rgsz[1] = "player/pl_step2.wav";
		cnt = 2;
		break;
	case CHAR_TEX_METAL: fvol = 0.9;
		rgsz[0] = "player/pl_metal1.wav";
		rgsz[1] = "player/pl_metal2.wav";
		cnt = 2;
		break;
	case CHAR_TEX_DIRT:	fvol = 0.9;
		rgsz[0] = "player/pl_dirt1.wav";
		rgsz[1] = "player/pl_dirt2.wav";
		rgsz[2] = "player/pl_dirt3.wav";
		cnt = 3;
		break;
	case CHAR_TEX_VENT:	fvol = 0.5;
		rgsz[0] = "player/pl_duct1.wav";
		rgsz[1] = "player/pl_duct1.wav";
		cnt = 2;
		break;
	case CHAR_TEX_GRATE: fvol = 0.9;
		rgsz[0] = "player/pl_grate1.wav";
		rgsz[1] = "player/pl_grate4.wav";
		cnt = 2;
		break;
	case CHAR_TEX_TILE:	fvol = 0.8;
		rgsz[0] = "player/pl_tile1.wav";
		rgsz[1] = "player/pl_tile3.wav";
		rgsz[2] = "player/pl_tile2.wav";
		rgsz[3] = "player/pl_tile4.wav";
		cnt = 4;
		break;
	case CHAR_TEX_SLOSH: fvol = 0.9;
		rgsz[0] = "player/pl_slosh1.wav";
		rgsz[1] = "player/pl_slosh3.wav";
		rgsz[2] = "player/pl_slosh2.wav";
		rgsz[3] = "player/pl_slosh4.wav";
		cnt = 4;
		break;
	case CHAR_TEX_SNOW: fvol = 0.7;
		rgsz[0] = "debris/pl_snow1.wav";
		rgsz[1] = "debris/pl_snow2.wav";
		rgsz[2] = "debris/pl_snow3.wav";
		rgsz[3] = "debris/pl_snow4.wav";
		cnt = 4;
		break;
	case CHAR_TEX_WOOD: fvol = 0.9;
		rgsz[0] = "debris/wood1.wav";
		rgsz[1] = "debris/wood2.wav";
		rgsz[2] = "debris/wood3.wav";
		cnt = 3;
		break;
	case CHAR_TEX_GLASS:
	case CHAR_TEX_COMPUTER:
		fvol = 0.8;
		rgsz[0] = "debris/glass1.wav";
		rgsz[1] = "debris/glass2.wav";
		rgsz[2] = "debris/glass3.wav";
		cnt = 3;
		break;
	case CHAR_TEX_FLESH:
		if (iBulletType == BULLET_PLAYER_CROWBAR)
			return chTextureType; // crowbar already makes this sound
		fvol = 1.0;
		rgsz[0] = "weapons/bullet_hit1.wav";
		rgsz[1] = "weapons/bullet_hit2.wav";
		fattn = 1.0;
		cnt = 2;
		break;
	}

	// play material hit sound
	gEngfuncs.pEventAPI->EV_PlaySound( 0, ptr->endpos, CHAN_STATIC, rgsz[gEngfuncs.pfnRandomLong(0,cnt-1)], fvol, fattn, 0, 96 + gEngfuncs.pfnRandomLong(0,0xf) );
	return chTextureType;
}

// name ours (EV_HLDM_CreateLight in Valve's 1.6 debug information; nvg.cpp calls it): a dynamic light at the view origin (night vision)
void EV_HLDM_CreateDLight( float *origin, float radius, int r, int g, int b, float time )
{
	dlight_t *dl = gEngfuncs.pEfxAPI->CL_AllocDlight( 0 );

	VectorCopy( gHUD.m_vecOrigin, dl->origin );
	dl->radius = radius;
	dl->color.r = r;
	dl->color.g = g;
	dl->color.b = b;
	dl->die = gHUD.m_flTime + time;
}

// names ours: the wind that drifts the smoke puffs
static int g_iWindDirX = 1;
static int g_iWindDirY = 1;
static float g_flWindX;
static float g_flWindY;

// a smoke puff slows down and rises, and drifts with the wind
void SmokeRise_Wind( struct tempent_s *te, float frametime, float current_time )
{
	if ( te->entity.curstate.frame > 7 )
	{
		te->entity.baseline.origin[0] *= 0.97;
		te->entity.baseline.origin[1] *= 0.97;
		te->entity.baseline.origin[2] = te->entity.baseline.origin[2] * 0.97 + 0.7;
		if ( te->entity.baseline.origin[2] > 70 )
			te->entity.baseline.origin[2] = 70;
	}

	if ( te->entity.curstate.frame > 6 )
	{
		if ( g_iWindDirX == 1 )
		{
			g_flWindX += 0.075;
			if ( g_flWindX > 5 )
				g_flWindX = 5;
			te->entity.baseline.origin[0] += g_flWindX;
		}
		else if ( g_iWindDirX == 0 )
		{
			g_flWindX += 0.075;
			if ( g_flWindX > 5 )
				g_flWindX = 5;
			te->entity.baseline.origin[0] -= g_flWindX;
		}

		if ( g_iWindDirY == 1 )
		{
			g_flWindY += 0.075;
			if ( g_flWindY > 5 )
				g_flWindY = 5;
			te->entity.baseline.origin[1] += g_flWindY;
		}
		else if ( g_iWindDirY == 0 )
		{
			g_flWindY += 0.075;
			if ( g_flWindY > 5 )
				g_flWindY = 5;
			te->entity.baseline.origin[1] -= g_flWindY;
		}

		if ( !gEngfuncs.pfnRandomLong( 0, 10 ) && g_flWindY > 3.0 )
		{
			g_flWindY = 0;
			g_iWindDirY = 1 - g_iWindDirY;
		}

		if ( !gEngfuncs.pfnRandomLong( 0, 10 ) && g_flWindX > 3.0 )
		{
			g_flWindX = 0;
			g_iWindDirX = 1 - g_iWindDirX;
		}
	}
}

// black smoke fades and grows
void SmokeRise_Wind_Expand( struct tempent_s *te, float frametime, float current_time )
{
	if ( te->entity.curstate.renderamt >= 75 )
		te->entity.curstate.renderamt -= gEngfuncs.pfnRandomLong( 0, 9 ) == 0;

	if ( te->entity.curstate.scale < te->entity.baseline.fuser1 )
		te->entity.curstate.scale *= 1.025f;
}

// a smoke puff slows down and rises
void SmokeRise( struct tempent_s *te, float frametime, float current_time )
{
	if ( te->entity.curstate.frame > 7 )
	{
		te->entity.baseline.origin[0] *= 0.97;
		te->entity.baseline.origin[1] *= 0.97;
		te->entity.baseline.origin[2] = te->entity.baseline.origin[2] * 0.97 + 0.7;
		if ( te->entity.baseline.origin[2] > 70 )
			te->entity.baseline.origin[2] = 70;
	}
}

// a smoke puff that hits something slides along it
void HugWalls( struct tempent_s *te, struct pmtrace_s *ptr )
{
	Vector wall_normal;
	Vector smoke_velocity;
	Vector new_velocity;
	float flLength;
	float flDot;	// sic: never used (see below)

	new_velocity = Vector( 0, 0, 0 );	// sic: dead store (as in 1.3; without it four x87 operands of the cross products stand the other way round)
	smoke_velocity = te->entity.baseline.origin;
	wall_normal = ptr->plane.normal;

	flLength = smoke_velocity.Length();
	flDot = smoke_velocity.x * wall_normal.x + smoke_velocity.y * wall_normal.y;	// sic: unused (as in 1.3; x87 operand order of the cross products)

	if ( flLength == 0.0 )
	{
		smoke_velocity.x = 0;
		smoke_velocity.y = 0;
		smoke_velocity.z = 0;
	}
	else
	{
		smoke_velocity.x /= flLength;
		smoke_velocity.y /= flLength;
		smoke_velocity.z /= flLength;
	}

	smoke_velocity = CrossProduct( smoke_velocity, wall_normal );
	new_velocity = CrossProduct( smoke_velocity, wall_normal );

	if ( flLength > 2000.0 )
		flLength = 2000;

	new_velocity.x *= flLength * 1.5;
	new_velocity.y *= flLength * 1.5;
	new_velocity.z *= flLength * 1.5;
	te->entity.baseline.origin = new_velocity;
}

static char szBlackSmoke[25];	// name ours: the black smoke sprite's name, built a character at a time (25 to 28 bytes fit before tracerCount)

// a smoke puff tempent
void EV_HLDM_CreateSmoke( float *origin, float *dir, int speed, float scale, int r, int g, int b, int iSmokeType, float *base_velocity, bool bWind, int framerate )
{
	TEMPENTITY *te = NULL;
	int model;

	if ( g_SmokeList.m_iCount >= g_SmokeList.m_iMax && iSmokeType )
		return;

	g_SmokeList.Add( gHUD.m_flTime );

	if ( iSmokeType == 2 )
	{
		switch ( gEngfuncs.pfnRandomLong( 0, 3 ) )
		{
		case 0:	model = gEngfuncs.pEventAPI->EV_FindModelIndex( "sprites/wall_puff1.spr" ); break;
		case 1:	model = gEngfuncs.pEventAPI->EV_FindModelIndex( "sprites/wall_puff2.spr" ); break;
		case 2:	model = gEngfuncs.pEventAPI->EV_FindModelIndex( "sprites/wall_puff3.spr" ); break;
		case 3:	model = gEngfuncs.pEventAPI->EV_FindModelIndex( "sprites/wall_puff4.spr" ); break;
		default: model = gEngfuncs.pEventAPI->EV_FindModelIndex( "sprites/wall_puff1.spr" ); break;
		}
	}
	else if ( iSmokeType == 0 || iSmokeType == 1 )
	{
		if ( iSmokeType > 70 )
		{
			gEngfuncs.Con_Printf( "sprites/black_smoke1.spr" );
			gEngfuncs.Con_Printf( "sprites/black_smoke2.spr" );
			gEngfuncs.Con_Printf( "sprites/black_smoke3.spr" );
			gEngfuncs.Con_Printf( "sprites/black_smoke4.spr" );
			gEngfuncs.Con_Printf( "sprites/fast_wallpuff1.spr" );
		}

		szBlackSmoke[0] = 's';
		szBlackSmoke[1] = 'p';
		szBlackSmoke[2] = 'r';
		szBlackSmoke[3] = 'i';
		szBlackSmoke[4] = 't';
		szBlackSmoke[5] = 'e';
		szBlackSmoke[6] = 's';
		szBlackSmoke[7] = '/';
		szBlackSmoke[8] = 'b';
		szBlackSmoke[9] = 'l';
		szBlackSmoke[10] = 'a';
		szBlackSmoke[11] = 'c';
		szBlackSmoke[12] = 'k';
		szBlackSmoke[13] = '_';
		szBlackSmoke[14] = 's';
		szBlackSmoke[15] = 'm';
		szBlackSmoke[16] = 'o';
		szBlackSmoke[17] = 'k';
		szBlackSmoke[18] = 'e';
		szBlackSmoke[19] = '1';
		szBlackSmoke[20] = '.';
		szBlackSmoke[21] = 's';
		szBlackSmoke[22] = 'p';
		szBlackSmoke[23] = 'r';
		szBlackSmoke[24] = 0;

		switch ( gEngfuncs.pfnRandomLong( 0, 3 ) )
		{
		case 1:	szBlackSmoke[19] = '2'; break;
		case 2:	szBlackSmoke[19] = '3'; break;
		case 3:	szBlackSmoke[19] = '4'; break;
		case 0:	szBlackSmoke[19] = '1'; break;
		default:	szBlackSmoke[19] = '1'; break;
		}

		model = gEngfuncs.pEventAPI->EV_FindModelIndex( szBlackSmoke );

		if ( CVAR_GET_FLOAT( "fastsprites" ) > 1 )
		{
			char szFastSmoke[27];	// name ours (27 or 28 bytes give the image's frame)

			szFastSmoke[0] = 's';
			szFastSmoke[1] = 'p';
			szFastSmoke[2] = 'r';
			szFastSmoke[3] = 'i';
			szFastSmoke[4] = 't';
			szFastSmoke[5] = 'e';
			szFastSmoke[6] = 's';
			szFastSmoke[7] = '/';
			szFastSmoke[8] = 'f';
			szFastSmoke[9] = 'a';
			szFastSmoke[10] = 's';
			szFastSmoke[11] = 't';
			szFastSmoke[12] = '_';
			szFastSmoke[13] = 'w';
			szFastSmoke[14] = 'a';
			szFastSmoke[15] = 'l';
			szFastSmoke[16] = 'l';
			szFastSmoke[17] = 'p';
			szFastSmoke[18] = 'u';
			szFastSmoke[19] = 'f';
			szFastSmoke[20] = 'f';
			szFastSmoke[21] = '1';
			szFastSmoke[22] = '.';
			szFastSmoke[23] = 's';
			szFastSmoke[24] = 'p';
			szFastSmoke[25] = 'r';
			szFastSmoke[26] = 0;

			model = gEngfuncs.pEventAPI->EV_FindModelIndex( szFastSmoke );
		}

		if ( !model )
			model = 293;
	}
	else if ( iSmokeType == 3 )
	{
		if ( gEngfuncs.pfnRandomLong( 0, 1 ) )
			model = gEngfuncs.pEventAPI->EV_FindModelIndex( "sprites/pistol_smoke1.spr" );
		else
			model = gEngfuncs.pEventAPI->EV_FindModelIndex( "sprites/pistol_smoke2.spr" );
	}
	else if ( iSmokeType == 4 )
	{
		switch ( gEngfuncs.pfnRandomLong( 0, 2 ) )
		{
		case 0:	model = gEngfuncs.pEventAPI->EV_FindModelIndex( "sprites/rifle_smoke1.spr" ); break;
		case 1:	model = gEngfuncs.pEventAPI->EV_FindModelIndex( "sprites/rifle_smoke2.spr" ); break;
		case 2:	model = gEngfuncs.pEventAPI->EV_FindModelIndex( "sprites/rifle_smoke3.spr" ); break;
		}
	}

	te = gEngfuncs.pEfxAPI->R_DefaultSprite( origin, model, framerate );

	if ( te )
	{
		te->flags |= ( FTENT_COLLIDEWORLD | FTENT_CLIENTCUSTOM );
		te->entity.curstate.scale = gEngfuncs.pfnRandomFloat( 0, scale / 2 ) + scale;

		if ( iSmokeType != 0 && iSmokeType != 1 )
		{
			if ( !bWind )
				te->callback = SmokeRise;
			else
				te->callback = SmokeRise_Wind;

			te->entity.curstate.rendermode = kRenderTransAdd;
			te->entity.curstate.renderamt = 150;
			te->entity.curstate.renderfx = kRenderFxFadeSlow;
		}
		else
		{
			if ( CVAR_GET_FLOAT( "fastsprites" ) == 1 )
			{
				te->entity.curstate.rendermode = kRenderTransAdd;
				te->entity.curstate.renderamt = 200;
			}
			else if ( CVAR_GET_FLOAT( "fastsprites" ) > 1 )
			{
				te->entity.curstate.rendermode = kRenderNormal;
			}
			else
			{
				te->entity.curstate.rendermode = kRenderTransAlpha;
				te->entity.curstate.renderamt = 200;
			}

			te->callback = SmokeRise_Wind_Expand;

			if ( iSmokeType == 1 )
				te->entity.baseline.fuser1 = 6;
			else
				te->entity.baseline.fuser1 = 8.5;

			if ( CVAR_GET_FLOAT( "fastsprites" ) > 1 )
				te->entity.baseline.fuser1 -= 3;
		}

		te->hitcallback = HugWalls;
		te->entity.curstate.rendercolor.r = r;
		te->entity.curstate.rendercolor.g = g;
		te->entity.curstate.rendercolor.b = b;

		if ( speed )
			speed += gEngfuncs.pfnRandomLong( 0, 5 );

		if ( origin[0] != dir[0] && origin[1] != dir[1] && origin[2] != dir[2] )
		{
			te->entity.origin = origin;
			te->entity.baseline.origin = dir;
			te->entity.baseline.origin.x *= speed;
			te->entity.baseline.origin.y *= speed;
			te->entity.baseline.origin.z *= speed;

			if ( base_velocity )
			{
				te->entity.baseline.origin.x += base_velocity[0] * 0.9;
				te->entity.baseline.origin.y += base_velocity[1] * 0.9;
				te->entity.baseline.origin.z += base_velocity[2] * 0.5;
			}
		}
	}
}

// sic: a stand-in, name ours.  ev_hldm.obj writes the precompiled header and emits the headers' inline functions by
// the intermediate code of its own text before each of its functions.  Retail 1.4 has VGUI's Dar<ChangeSignal*>
// assignment and Scheme's copy constructor ahead of EV_HLDM_GunshotDecalTrace and Scheme's assignment behind it
// (0x1901de0, 0x1901e00, 0x1902080; 1.3 had all three behind it): its own text before EV_HLDM_GunshotDecalTrace
// held about 400 bytes of intermediate code more than ours.  This unused inline gives retail's order with 57 to
// 110 empty statements (full builds; 54 and 120 do not).  Nothing calls it.
inline void r14_evhldm_unknown( void )
{
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
	;
}

char *EV_HLDM_DamageDecal( physent_t *pe )
{
	static char decalname[ 32 ];
	int idx;

	if ( pe->classnumber == 1 )
	{
		idx = gEngfuncs.pfnRandomLong( 0, 2 );
		sprintf( decalname, "{break%i", idx + 1 );
	}
	else if ( pe->rendermode != kRenderNormal )
	{
		sprintf( decalname, "{bproof1" );
	}
	else
	{
		idx = gEngfuncs.pfnRandomLong( 0, 4 );
		sprintf( decalname, "{shot%i", idx + 1 );
	}
	return decalname;
}

void EV_HLDM_GunshotDecalTrace( pmtrace_t *pTrace, char *decalName, char chTextureType )
{
	int iRand;
	physent_t *pe;

	gEngfuncs.pEfxAPI->R_BulletImpactParticles( pTrace->endpos );

	iRand = gEngfuncs.pfnRandomLong(0,0x7FFF);
	if ( iRand < (0x7fff/2) )// not every bullet makes a sound.
	{
		if ( chTextureType == CHAR_TEX_METAL || chTextureType == CHAR_TEX_VENT )
		{
			if ( iRand % 2 )
				gEngfuncs.pEventAPI->EV_PlaySound( -1, pTrace->endpos, 0, "weapons/ric_metal-1.wav", 1.0, ATTN_NORM, 0, PITCH_NORM );
			else
				gEngfuncs.pEventAPI->EV_PlaySound( -1, pTrace->endpos, 0, "weapons/ric_metal-2.wav", 1.0, ATTN_NORM, 0, PITCH_NORM );
		}
		else
		{
			switch( iRand % 6 )
			{
			case 0:	gEngfuncs.pEventAPI->EV_PlaySound( -1, pTrace->endpos, 0, "weapons/ric1.wav", 1.0, ATTN_NORM, 0, PITCH_NORM ); break;
			case 1:	gEngfuncs.pEventAPI->EV_PlaySound( -1, pTrace->endpos, 0, "weapons/ric2.wav", 1.0, ATTN_NORM, 0, PITCH_NORM ); break;
			case 2:	gEngfuncs.pEventAPI->EV_PlaySound( -1, pTrace->endpos, 0, "weapons/ric3.wav", 1.0, ATTN_NORM, 0, PITCH_NORM ); break;
			case 3:	gEngfuncs.pEventAPI->EV_PlaySound( -1, pTrace->endpos, 0, "weapons/ric4.wav", 1.0, ATTN_NORM, 0, PITCH_NORM ); break;
			case 4:	gEngfuncs.pEventAPI->EV_PlaySound( -1, pTrace->endpos, 0, "weapons/ric5.wav", 1.0, ATTN_NORM, 0, PITCH_NORM ); break;
			case 5:	gEngfuncs.pEventAPI->EV_PlaySound( -1, pTrace->endpos, 0, "weapons/ric_conc-1.wav", 1.0, ATTN_NORM, 0, PITCH_NORM ); break;
			case 6:	gEngfuncs.pEventAPI->EV_PlaySound( -1, pTrace->endpos, 0, "weapons/ric_conc-2.wav", 1.0, ATTN_NORM, 0, PITCH_NORM ); break;
			}
		}
	}

	pe = gEngfuncs.pEventAPI->EV_GetPhysent( pTrace->ent );

	// Only decal brush models such as the world etc.
	if (  decalName && decalName[0] && pe && ( pe->solid == SOLID_BSP || pe->movetype == MOVETYPE_PUSHSTEP ) )
	{
		if ( CVAR_GET_FLOAT( "r_decals" ) )
		{
			gEngfuncs.pEfxAPI->R_DecalShoot( 
				gEngfuncs.pEfxAPI->Draw_DecalIndex( gEngfuncs.pEfxAPI->Draw_DecalIndexFromName( decalName ) ), 
				gEngfuncs.pEventAPI->EV_IndexFromTrace( pTrace ), 0, pTrace->endpos, 0 );
		}
	}
}

void EV_HLDM_DecalGunshot( pmtrace_t *pTrace, int iBulletType, float scale, int r, int g, int b, bool bCreateSparks, char cTextureType )
{
	physent_t *pe;

	pe = gEngfuncs.pEventAPI->EV_GetPhysent( pTrace->ent );

	if ( pe && pe->solid == SOLID_BSP )
	{
		EV_HLDM_GunshotDecalTrace( pTrace, EV_HLDM_DamageDecal( pe ), cTextureType );

		if ( bCreateSparks == true )
		{
			vec3_t dir;

			dir[0] = pTrace->plane.normal[0] * gEngfuncs.pfnRandomFloat( 4, 10 );
			dir[1] = pTrace->plane.normal[1] * gEngfuncs.pfnRandomFloat( 4, 10 );
			dir[2] = pTrace->plane.normal[2] * gEngfuncs.pfnRandomFloat( 4, 10 );

			float speed = gEngfuncs.pfnRandomFloat( 4, 10 );
			int amount = gEngfuncs.pfnRandomLong( 15, 30 );
			gEngfuncs.pEfxAPI->R_StreakSplash( pTrace->endpos, dir, 30, amount, speed, -75, 75 );
		}

		vec3_t position;

		position[0] = pTrace->endpos[0] + pTrace->plane.normal[0] * 5;
		position[1] = pTrace->endpos[1] + pTrace->plane.normal[1] * 5;
		position[2] = pTrace->endpos[2] + pTrace->plane.normal[2] * 5;

		EV_HLDM_CreateSmoke( position, pTrace->plane.normal, 25, scale, r, g, b, 2, NULL, true, 35 );
	}
}

int EV_HLDM_CheckTracer( int idx, float *vecSrc, float *end, float *forward, float *right, int iBulletType, int iTracerFreq, int *tracerCount )
{
	int tracer = 0;
	int i;
	qboolean player = idx >= 1 && idx <= gEngfuncs.GetMaxClients() ? true : false;

	if ( iTracerFreq != 0 && ( (*tracerCount)++ % iTracerFreq) == 0 )
	{
		vec3_t vecTracerSrc;

		if ( player )
		{
			vec3_t offset( 0, 0, -4 );

			// adjust tracer position for player
			for ( i = 0; i < 3; i++ )
			{
				vecTracerSrc[ i ] = vecSrc[ i ] + offset[ i ] + right[ i ] * 2 + forward[ i ] * 16;
			}
		}
		else
		{
			VectorCopy( vecSrc, vecTracerSrc );
		}
		
		if ( iTracerFreq != 1 )		// guns that always trace also always decal
			tracer = 1;

		switch( iBulletType )
		{
		case BULLET_PLAYER_MP5:
		case BULLET_MONSTER_MP5:
		case BULLET_MONSTER_9MM:
		case BULLET_MONSTER_12MM:
		default:
			EV_CreateTracer( vecTracerSrc, end );
			break;
		}
	}

	return tracer;
}

// Counter-Strike: the texture type a trace hit, EV_HLDM_PlayTextureSound's lookup without the sounds.
// Nothing calls it in this version, so none of its code is in the image; the linker still counted it
// (which copy of identical functions it keeps, and the fixups .reloc reserves room for).
// Name, parameters and place are those of Valve's 1.6 build, which has
// debug information; the body is the SDK's text above.
char UTIL_TextureHit( pmtrace_t *ptr, Vector vecSrc, Vector vecEnd )
{
	char chTextureType;
	char *pTextureName;
	char texname[ 64 ];
	char szbuffer[ 64 ];
	int entity;

	entity = gEngfuncs.pEventAPI->EV_IndexFromTrace( ptr );

	chTextureType = 0;

	// Player
	if ( entity >= 1 && entity <= gEngfuncs.GetMaxClients() )
	{
		// hit body
		chTextureType = CHAR_TEX_FLESH;
	}
	else if ( entity == 0 )
	{
		// get texture from entity or world (world is ent(0))
		pTextureName = (char *)gEngfuncs.pEventAPI->EV_TraceTexture( ptr->ent, vecSrc, vecEnd );
		
		if ( pTextureName )
		{
			strcpy( texname, pTextureName );
			pTextureName = texname;

			// strip leading '-0' or '+0~' or '{' or '!'
			if (*pTextureName == '-' || *pTextureName == '+')
			{
				pTextureName += 2;
			}

			if (*pTextureName == '{' || *pTextureName == '!' || *pTextureName == '~' || *pTextureName == ' ')
			{
				pTextureName++;
			}
			
			// '}}'
			strcpy( szbuffer, pTextureName );
			szbuffer[ CBTEXTURENAMEMAX - 1 ] = 0;
				
			// get texture type
			chTextureType = PM_FindTextureType( szbuffer );	
		}
	}

	return chTextureType;
}

// Counter-Strike 1.3: nothing calls it; its statics are in retail's .bss (16,384 bytes ahead of
// hl_baseentity.obj's data).  Name, parameter, statics and place are Counter-Strike 1.6's (line 829 there,
// between UTIL_TextureHit and EV_HLDM_FireBullets' comment), the format string is its binary's.  The place
// shows in the image: which copies of identical functions the linker keeps.
char *vstr( float *v )
{
	static int idx = 0;
	static char string[ 16 ][ 1024 ];

	idx++;
	idx &= 15;

	sprintf( string[ idx ], "%.4f %.4f %.4f", v[0], v[1], v[2] );
	return string[ idx ];
}

/*
================
FireBullets

Go to the trouble of combining multiple pellets into a single damage call.
================
*/
void EV_HLDM_FireBullets( int idx, float *forward, float *right, float *up, int cShots, float *vecSrc, float *vecDirShooting, float *vecSpread, float flDistance, int iBulletType, int iTracerFreq, int *tracerCount, int iPenetration )
{
	int i;
	pmtrace_t tr;
	int iShot;
	vec3_t vecRight, vecUp;
	int tracer;
	int r13_firebullets_unknown = 0;	// sic: a stand-in, name ours: with a second spread loop that the optimiser removes (below) the
										// loop before it addresses its arrays as the image does

	float scale_smoke;
	int r_smoke, g_smoke, b_smoke;

	r_smoke = g_smoke = b_smoke = 40;
	scale_smoke = 0.4;
	int iInitialPenetration = iPenetration;
	float flPenetrationDistance;
	float flCurrentDistance;
	int iPenetrationPower;

	switch ( iBulletType )
	{
	case BULLET_PLAYER_9MM:		iPenetrationPower = 21; flPenetrationDistance = 800; break;
	case BULLET_PLAYER_45ACP:	iPenetrationPower = 15; flPenetrationDistance = 500; break;
	case BULLET_PLAYER_50AE:	iPenetrationPower = 30; flPenetrationDistance = 1000; break;
	case BULLET_PLAYER_762MM:	iPenetrationPower = 39; flPenetrationDistance = 5000; break;
	case BULLET_PLAYER_556MM:	iPenetrationPower = 35; flPenetrationDistance = 4000; break;
	case BULLET_PLAYER_338MAG:	iPenetrationPower = 45; flPenetrationDistance = 8000; break;
	case BULLET_PLAYER_57MM:	iPenetrationPower = 30; flPenetrationDistance = 2000; break;
	case BULLET_PLAYER_357SIG:	iPenetrationPower = 25; flPenetrationDistance = 800; break;
	case BULLET_PLAYER_BUCKSHOT:	iPenetrationPower = 8; flPenetrationDistance = 500; break;
	default:	iPenetrationPower = 0; flPenetrationDistance = 0; break;
	}

	iPenetrationPower *= 2;

	VectorCopy( right, vecRight );
	VectorCopy( up, vecUp );	// never read (1.6 declares vecUp too); without this copy the spread terms below come out in another operand order

	for ( iShot = 1; iShot <= cShots; iShot++ )
	{
		vec3_t vecDir, vecEnd;

		iPenetration = iInitialPenetration;

		vec3_t spread;
		//We randomize for the Shotgun.
		if ( iBulletType == BULLET_PLAYER_BUCKSHOT )
		{
			do {
				spread[0] = gEngfuncs.pfnRandomFloat(-0.5,0.5) + gEngfuncs.pfnRandomFloat(-0.5,0.5);
				spread[1] = gEngfuncs.pfnRandomFloat(-0.5,0.5) + gEngfuncs.pfnRandomFloat(-0.5,0.5);
				spread[2] = spread[0] * spread[0] + spread[1] *spread[1];
			} while (spread[2] > 1);

			for ( i = 0 ; i < 3; i++ )
			{
				vecDir[i] = vecDirShooting[i] + spread[ 0 ] * vecSpread[ 0 ] * vecRight[ i ] + spread[ 1 ] * vecSpread[ 1 ] * up [ i ];
				vecEnd[i] = vecSrc[ i ] + flDistance * vecDir[ i ];
			}
		}//But other guns already have their spread randomized in the synched spread.
		else
		{

			for ( i = 0 ; i < 3; i++ )
			{
				vecDir[i] = vecDirShooting[i] + vecSpread[ 0 ] * right[ i ] + vecSpread[ 1 ] * up [ i ];
				vecEnd[i] = vecSrc[ i ] + flDistance * vecDir[ i ];
			}
			if ( r13_firebullets_unknown )	// sic: never true (see the declaration)
			{
				for ( i = 0 ; i < 3; i++ )
				{
					vecDir[i] = vecDirShooting[i] + spread[ 0 ] * vecSpread[ 0 ] * vecRight[ i ] + spread[ 1 ] * vecSpread[ 1 ] * up [ i ];
					vecEnd[i] = vecSrc[ i ] + flDistance * vecDir[ i ];
				}
			}
		}

		gEngfuncs.pEventAPI->EV_SetUpPlayerPrediction( false, true );
	
		// Store off the old count
		gEngfuncs.pEventAPI->EV_PushPMStates();
	
		// Now add in all of the players.
		gEngfuncs.pEventAPI->EV_SetSolidPlayers ( idx - 1 );	

		gEngfuncs.pEventAPI->EV_SetTraceHull( 2 );

		char cTextureType;
		bool bCreateSparks = false;

		while ( iPenetration != 0 )
		{
			gEngfuncs.pEventAPI->EV_PlayerTrace( vecSrc, vecEnd, PM_NORMAL, -1, &tr );

			tracer = EV_HLDM_CheckTracer( idx, vecSrc, tr.endpos, forward, right, iBulletType, iTracerFreq, tracerCount );

			// sic: this spelling of the range test (the product tested before it is stored, the store
			// inside the second test) is the only one found that keeps the product on the FPU stack
			// across the zero test as the image does
			if ( tr.fraction * flDistance == 0.0 )
				break;

			iPenetration--;

			if ( ( flCurrentDistance = tr.fraction * flDistance ) > flPenetrationDistance )
				iPenetration = 0;

			switch ( iBulletType )
			{
			case BULLET_PLAYER_9MM:
				if ( !tracer )
					cTextureType = EV_HLDM_PlayTextureSound( idx, &tr, vecSrc, vecEnd, iBulletType );
				break;
			case BULLET_PLAYER_MP5:
				if ( !tracer )
					cTextureType = EV_HLDM_PlayTextureSound( idx, &tr, vecSrc, vecEnd, iBulletType );
				break;
			case BULLET_PLAYER_BUCKSHOT:
				break;
			case BULLET_PLAYER_357:
				if ( !tracer )
					cTextureType = EV_HLDM_PlayTextureSound( idx, &tr, vecSrc, vecEnd, iBulletType );
				break;
			default:
				cTextureType = EV_HLDM_PlayTextureSound( idx, &tr, vecSrc, vecEnd, iBulletType );
				break;
			}

			switch ( cTextureType )
			{
			case CHAR_TEX_METAL:
				iPenetrationPower *= 0.15;
				bCreateSparks = true;
				break;
			case CHAR_TEX_CONCRETE:
				r_smoke = 65;
				g_smoke = 65;
				b_smoke = 65;
				iPenetrationPower *= 0.25;
				bCreateSparks = true;
				break;
			case CHAR_TEX_GRATE:
			case CHAR_TEX_VENT:
				iPenetrationPower *= 0.5;
				bCreateSparks = true;
				break;
			case CHAR_TEX_TILE:
				iPenetrationPower *= 0.65;
				bCreateSparks = true;
				break;
			case CHAR_TEX_COMPUTER:
				iPenetrationPower *= 0.4;
				bCreateSparks = true;
				break;
			case CHAR_TEX_WOOD:
				r_smoke = 75;
				g_smoke = 42;
				b_smoke = 15;
				scale_smoke = 0.5;
				bCreateSparks = false;
				break;
			}

			EV_HLDM_DecalGunshot( &tr, iBulletType, scale_smoke, r_smoke, g_smoke, b_smoke, bCreateSparks, cTextureType );

			if ( !iPenetration )
				break;

			flDistance = ( flDistance - flCurrentDistance ) * 0.5;

			for ( i = 0; i <= 2; i++ )
			{
				vecSrc[i] = tr.endpos[i] + iPenetrationPower * vecDir[i];
				vecEnd[i] = vecSrc[i] + flDistance * vecDir[i];
			}

			// Counter-Strike 1.4: the hole on the far side of the wall (locals as Counter-Strike 1.6 names them)
			for ( int iPen = 1; iPen < iPenetrationPower; iPen++ )
			{
				pmtrace_t trOriginal;
				Vector vStartPos = tr.endpos + vecDir * iPen;
				Vector vEndPos = vStartPos + vecDir;

				gEngfuncs.pEventAPI->EV_SetTraceHull( 2 );
				gEngfuncs.pEventAPI->EV_PlayerTrace( vStartPos, vEndPos, PM_NORMAL, -1, &trOriginal );

				if ( trOriginal.startsolid && trOriginal.inopen )
				{
					gEngfuncs.pEventAPI->EV_PlayerTrace( vEndPos, vStartPos, PM_NORMAL, -1, &trOriginal );

					EV_HLDM_DecalGunshot( &trOriginal, iBulletType, scale_smoke, r_smoke, g_smoke, b_smoke, bCreateSparks, cTextureType );
					break;
				}
			}
		}

		gEngfuncs.pEventAPI->EV_PopPMStates();
	}
}

void EV_FireGlock1( event_args_t *args )
{
	int idx;
	vec3_t origin;
	vec3_t angles;
	vec3_t velocity;
	int empty;

	int i;
	vec3_t ShellVelocity;
	vec3_t ShellOrigin;
	int shell;
	vec3_t vecSrc, vecAiming;
	vec3_t vecSpread;
	vec3_t up, right, forward;
	float flSpread = 0.01;

	idx = args->entindex;
	VectorCopy( args->origin, origin );
	VectorCopy( args->angles, angles );
	VectorCopy( args->velocity, velocity );

	empty = args->bparam1;
	AngleVectors( angles, forward, right, up );

	shell = gEngfuncs.pEventAPI->EV_FindModelIndex ("models/shell.mdl");// brass shell

	if ( EV_IsLocal( idx ) )
	{
		EV_MuzzleFlash();
		gEngfuncs.pEventAPI->EV_WeaponAnimation( empty ? GLOCK_SHOOT_EMPTY : GLOCK_SHOOT, 2 );
	}

	EV_GetDefaultShellInfo( args, origin, velocity, ShellVelocity, ShellOrigin, forward, right, up, 20, -12, 4, false );

	EV_EjectBrass ( ShellOrigin, ShellVelocity, angles[ YAW ], shell, TE_BOUNCE_SHELL, idx, 1 ); 

	gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "weapons/pl_gun3.wav", gEngfuncs.pfnRandomFloat(0.92, 1.0), ATTN_NORM, 0, 98 + gEngfuncs.pfnRandomLong( 0, 3 ) );

	EV_GetGunPosition( args, vecSrc, origin );
	
	VectorCopy( forward, vecAiming );

	for ( i = 0; i < 3; i++ )
	{
		vecSpread[ i ] = flSpread;
	}

	EV_HLDM_FireBullets( idx, forward, right, up, 1, vecSrc, vecAiming, vecSpread, 8192, BULLET_PLAYER_9MM, 0, &tracerCount[idx-1], 1 );

	if ( EV_IsLocal( idx ) )
	{
		V_PunchAxis( 0, -2.0 );
	}
}

void EV_FireGlock2( event_args_t *args )
{
	int idx;
	vec3_t origin;
	vec3_t angles;
	vec3_t velocity;

	int i;
	vec3_t ShellVelocity;
	vec3_t ShellOrigin;
	int shell;
	vec3_t vecSrc, vecAiming;
	vec3_t vecSpread;
	vec3_t up, right, forward;
	float flSpread = 0.1;

	idx = args->entindex;
	VectorCopy( args->origin, origin );
	VectorCopy( args->angles, angles );
	VectorCopy( args->velocity, velocity );

	AngleVectors( angles, forward, right, up );

	shell = gEngfuncs.pEventAPI->EV_FindModelIndex ("models/shell.mdl");// brass shell

	if ( EV_IsLocal( idx ) )
	{
		// Add muzzle flash to current weapon model
		EV_MuzzleFlash();
		gEngfuncs.pEventAPI->EV_WeaponAnimation( GLOCK_SHOOT, 2 );
	}

	EV_GetDefaultShellInfo( args, origin, velocity, ShellVelocity, ShellOrigin, forward, right, up, 20, -12, 4, false );

	EV_EjectBrass ( ShellOrigin, ShellVelocity, angles[ YAW ], shell, TE_BOUNCE_SHELL, idx, 1 ); 

	gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "weapons/pl_gun3.wav", gEngfuncs.pfnRandomFloat(0.92, 1.0), ATTN_NORM, 0, 98 + gEngfuncs.pfnRandomLong( 0, 3 ) );

	EV_GetGunPosition( args, vecSrc, origin );
	
	VectorCopy( forward, vecAiming );

	for ( i = 0; i < 3; i++ )
	{
		vecSpread[ i ] = flSpread;
	}

	EV_HLDM_FireBullets( idx, forward, right, up, 1, vecSrc, vecAiming, vecSpread, 8192, BULLET_PLAYER_9MM, 0, &tracerCount[idx-1], 1 );
	
	if ( EV_IsLocal( idx ) )
	{
		V_PunchAxis( 0, -2.0 );
	}
}

void EV_FireShotGunDouble( event_args_t *args )
{
	int idx;
	vec3_t origin;
	vec3_t angles;
	vec3_t velocity;

	int i, j;
	vec3_t ShellVelocity;
	vec3_t ShellOrigin;
	int shell;
	vec3_t vecSrc, vecAiming;
	vec3_t vecSpread;
	vec3_t up, right, forward;
	float flSpread = 0.01;

	idx = args->entindex;
	VectorCopy( args->origin, origin );
	VectorCopy( args->angles, angles );
	VectorCopy( args->velocity, velocity );

	AngleVectors( angles, forward, right, up );

	shell = gEngfuncs.pEventAPI->EV_FindModelIndex ("models/shotgunshell.mdl");// brass shell

	if ( EV_IsLocal( idx ) )
	{
		// Add muzzle flash to current weapon model
		EV_MuzzleFlash();
		gEngfuncs.pEventAPI->EV_WeaponAnimation( SHOTGUN_FIRE2, 2 );
	}

	for ( j = 0; j < 2; j++ )
	{
		EV_GetDefaultShellInfo( args, origin, velocity, ShellVelocity, ShellOrigin, forward, right, up, 32, -12, 6, false );

		EV_EjectBrass ( ShellOrigin, ShellVelocity, angles[ YAW ], shell, TE_BOUNCE_SHOTSHELL, idx, 1 ); 
	}

	gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "weapons/dbarrel1.wav", gEngfuncs.pfnRandomFloat(0.98, 1.0), ATTN_NORM, 0, 85 + gEngfuncs.pfnRandomLong( 0, 0x1f ) );

	EV_GetGunPosition( args, vecSrc, origin );
	VectorCopy( forward, vecAiming );

	if ( gEngfuncs.GetMaxClients() > 1 )
	{
		for ( i = 0; i < 3; i++ )
		{
			vecSpread[0] = 0.17365;
			vecSpread[1] = 0.04362;
			vecSpread[2] = 0.00;
		}

		EV_HLDM_FireBullets( idx, forward, right, up, 8, vecSrc, vecAiming, vecSpread, 2048, BULLET_PLAYER_BUCKSHOT, 0, &tracerCount[idx-1], 1 );
	}
	else
	{
		EV_HLDM_FireBullets( idx, forward, right, up, 12, vecSrc, vecAiming, VECTOR_CONE_10DEGREES, 2048, BULLET_PLAYER_BUCKSHOT, 0, &tracerCount[idx-1], 1 );
	}

	if ( EV_IsLocal( idx ) )
	{
		V_PunchAxis( 0, -10.0 );
	}
}

void EV_FireShotGunSingle( event_args_t *args )
{
	int idx;
	vec3_t origin;
	vec3_t angles;
	vec3_t velocity;
	
	int i;
	vec3_t ShellVelocity;
	vec3_t ShellOrigin;
	int shell;
	vec3_t vecSrc, vecAiming;
	vec3_t vecSpread;
	vec3_t up, right, forward;
	float flSpread = 0.01;

	idx = args->entindex;
	VectorCopy( args->origin, origin );
	VectorCopy( args->angles, angles );
	VectorCopy( args->velocity, velocity );

	AngleVectors( angles, forward, right, up );

	shell = gEngfuncs.pEventAPI->EV_FindModelIndex ("models/shotgunshell.mdl");// brass shell

	if ( EV_IsLocal( idx ) )
	{
		// Add muzzle flash to current weapon model
		EV_MuzzleFlash();
		gEngfuncs.pEventAPI->EV_WeaponAnimation( SHOTGUN_FIRE, 2 );
	}

	EV_GetDefaultShellInfo( args, origin, velocity, ShellVelocity, ShellOrigin, forward, right, up, 32, -12, 6, false );

	EV_EjectBrass ( ShellOrigin, ShellVelocity, angles[ YAW ], shell, TE_BOUNCE_SHOTSHELL, idx, 1 ); 

	gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "weapons/sbarrel1.wav", gEngfuncs.pfnRandomFloat(0.95, 1.0), ATTN_NORM, 0, 93 + gEngfuncs.pfnRandomLong( 0, 0x1f ) );

	EV_GetGunPosition( args, vecSrc, origin );
	VectorCopy( forward, vecAiming );

	if ( gEngfuncs.GetMaxClients() > 1 )
	{
		for ( i = 0; i < 3; i++ )
		{
			vecSpread[0] = 0.08716;
			vecSpread[1] = 0.04362;
			vecSpread[2] = 0.00;
		}

		EV_HLDM_FireBullets( idx, forward, right, up, 4, vecSrc, vecAiming, vecSpread, 2048, BULLET_PLAYER_BUCKSHOT, 0, &tracerCount[idx-1], 1 );
	}
	else
	{
		EV_HLDM_FireBullets( idx, forward, right, up, 6, vecSrc, vecAiming, VECTOR_CONE_10DEGREES, 2048, BULLET_PLAYER_BUCKSHOT, 0, &tracerCount[idx-1], 1 );
	}

	if ( EV_IsLocal( idx ) )
	{
		V_PunchAxis( 0, -5.0 );
	}
}

void EV_FireMP5( event_args_t *args )
{
	int idx;
	vec3_t origin;
	vec3_t angles;
	vec3_t velocity;

	vec3_t ShellVelocity;
	vec3_t ShellOrigin;
	int shell;
	vec3_t vecSrc, vecAiming;
	vec3_t up, right, forward;
	float flSpread = 0.01;

	idx = args->entindex;
	VectorCopy( args->origin, origin );
	VectorCopy( args->angles, angles );
	VectorCopy( args->velocity, velocity );

	AngleVectors( angles, forward, right, up );

	shell = gEngfuncs.pEventAPI->EV_FindModelIndex ("models/shell.mdl");// brass shell
	
	if ( EV_IsLocal( idx ) )
	{
		// Add muzzle flash to current weapon model
		EV_MuzzleFlash();
		gEngfuncs.pEventAPI->EV_WeaponAnimation( MP5_FIRE1 + gEngfuncs.pfnRandomLong(0,2), 2 );
	}

	EV_GetDefaultShellInfo( args, origin, velocity, ShellVelocity, ShellOrigin, forward, right, up, 20, -12, 4, false );

	EV_EjectBrass ( ShellOrigin, ShellVelocity, angles[ YAW ], shell, TE_BOUNCE_SHOTSHELL, idx, 1 ); 

	switch( gEngfuncs.pfnRandomLong( 0, 1 ) )
	{
	case 0:
		gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "weapons/hks1.wav", 1, ATTN_NORM, 0, 94 + gEngfuncs.pfnRandomLong( 0, 0xf ) );
		break;
	case 1:
		gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "weapons/hks2.wav", 1, ATTN_NORM, 0, 94 + gEngfuncs.pfnRandomLong( 0, 0xf ) );
		break;
	}

	EV_GetGunPosition( args, vecSrc, origin );
	VectorCopy( forward, vecAiming );

	if ( gEngfuncs.GetMaxClients() > 1 )
	{
		EV_HLDM_FireBullets( idx, forward, right, up, 1, vecSrc, vecAiming, VECTOR_CONE_6DEGREES, 8192, BULLET_PLAYER_MP5, 2, &tracerCount[idx-1], 1 );
	}
	else
	{
		EV_HLDM_FireBullets( idx, forward, right, up, 1, vecSrc, vecAiming, VECTOR_CONE_3DEGREES, 8192, BULLET_PLAYER_MP5, 2, &tracerCount[idx-1], 1 );
	}

	if ( EV_IsLocal( idx ) )
	{
		V_PunchAxis( 0, gEngfuncs.pfnRandomFloat( -2, 2 ) );
	}
}

void EV_FirePython( event_args_t *args )
{
	int idx;
	vec3_t origin;
	vec3_t angles;
	vec3_t velocity;

	vec3_t vecSrc, vecAiming;
	vec3_t up, right, forward;
	float flSpread = 0.01;

	idx = args->entindex;
	VectorCopy( args->origin, origin );
	VectorCopy( args->angles, angles );
	VectorCopy( args->velocity, velocity );

	AngleVectors( angles, forward, right, up );

	if ( EV_IsLocal( idx ) )
	{
		// Python uses different body in multiplayer versus single player
		int multiplayer = gEngfuncs.GetMaxClients() == 1 ? 0 : 1;

		// Add muzzle flash to current weapon model
		EV_MuzzleFlash();
		gEngfuncs.pEventAPI->EV_WeaponAnimation( PYTHON_FIRE1, multiplayer ? 1 : 0 );
	}

	switch( gEngfuncs.pfnRandomLong( 0, 1 ) )
	{
	case 0:
		gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "weapons/357_shot1.wav", gEngfuncs.pfnRandomFloat(0.8, 0.9), ATTN_NORM, 0, PITCH_NORM );
		break;
	case 1:
		gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "weapons/357_shot2.wav", gEngfuncs.pfnRandomFloat(0.8, 0.9), ATTN_NORM, 0, PITCH_NORM );
		break;
	}

	EV_GetGunPosition( args, vecSrc, origin );
	
	VectorCopy( forward, vecAiming );

	EV_HLDM_FireBullets( idx, forward, right, up, 1, vecSrc, vecAiming, VECTOR_CONE_1DEGREES, 8192, BULLET_PLAYER_357, 0, &tracerCount[idx-1], 1 );

	if ( EV_IsLocal( idx ) )
	{
		V_PunchAxis( 0, -10.0 );
	}
}

#define SND_CHANGE_PITCH	(1<<7)		// duplicated in protocol.h change sound pitch

void EV_SpinGauss( event_args_t *args )
{
	int idx;
	vec3_t origin;
	vec3_t angles;
	vec3_t velocity;
	int iSoundState = 0;

	int pitch;

	idx = args->entindex;
	VectorCopy( args->origin, origin );
	VectorCopy( args->angles, angles );
	VectorCopy( args->velocity, velocity );

	pitch = args->iparam1;

	iSoundState = args->bparam1 ? SND_CHANGE_PITCH : 0;

	gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "ambience/pulsemachine.wav", 1.0, ATTN_NORM, iSoundState, pitch );
}

// FIXME, pass m_fPrimaryFire in as parameter!!!
void EV_FireGauss( event_args_t *args )
{
	int idx;
	vec3_t origin;
	vec3_t angles;
	vec3_t velocity;
	float flDamage = args->fparam1;
	int primaryfire = args->bparam1;

	int m_fPrimaryFire = args->bparam1;
	int m_iWeaponVolume = GAUSS_PRIMARY_FIRE_VOLUME;
	vec3_t vecSrc;
	vec3_t vecDest;
	edict_t		*pentIgnore;
	pmtrace_t tr, beam_tr;
	float flMaxFrac = 1.0;
	int	nTotal = 0;
	int fHasPunched = 0;
	int fFirstBeam = 1;
	int	nMaxHits = 10;
	physent_t *pEntity;
	int m_iBeam, m_iGlow, m_iBalls;
	vec3_t up, right, forward;

	idx = args->entindex;
	VectorCopy( args->origin, origin );
	VectorCopy( args->angles, angles );
	VectorCopy( args->velocity, velocity );

	// Make sure we don't have a gauss spin event in the queue for this guy
	gEngfuncs.pEventAPI->EV_KillEvents( idx, "events/gaussspin.sc" );

//	Con_Printf( "Firing gauss with %f\n", flDamage );
	EV_GetGunPosition( args, vecSrc, origin );

	m_iBeam = gEngfuncs.pEventAPI->EV_FindModelIndex( "sprites/smoke.spr" );
	m_iBalls = m_iGlow = gEngfuncs.pEventAPI->EV_FindModelIndex( "sprites/hotglow.spr" );
	
	AngleVectors( angles, forward, right, up );

	VectorMA( vecSrc, 8192, forward, vecDest );

	if ( EV_IsLocal( idx ) )
	{
		V_PunchAxis( 0, -2.0 );
		gEngfuncs.pEventAPI->EV_WeaponAnimation( GAUSS_FIRE2, 2 );

	}

	gEngfuncs.pEventAPI->EV_StopSound( idx, CHAN_WEAPON, "ambience/pulsemachine.wav" );

	gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "weapons/gauss2.wav", 0.5 + flDamage * (1.0 / 400.0), ATTN_NORM, 0, 85 + gEngfuncs.pfnRandomLong( 0, 0x1f ) );

	while (flDamage > 10 && nMaxHits > 0)
	{
		nMaxHits--;

		gEngfuncs.pEventAPI->EV_SetUpPlayerPrediction( false, true );
		
		// Store off the old count
		gEngfuncs.pEventAPI->EV_PushPMStates();
	
		// Now add in all of the players.
		gEngfuncs.pEventAPI->EV_SetSolidPlayers ( idx - 1 );	

		gEngfuncs.pEventAPI->EV_SetTraceHull( 2 );
		gEngfuncs.pEventAPI->EV_PlayerTrace( vecSrc, vecDest, PM_STUDIO_BOX, -1, &tr );

		gEngfuncs.pEventAPI->EV_PopPMStates();

		if ( tr.allsolid )
			break;

		pEntity = gEngfuncs.pEventAPI->EV_GetPhysent( tr.ent );
		if ( pEntity == NULL )
			break;

		if (fFirstBeam)
		{
			if ( EV_IsLocal( idx ) )
			{
				// Add muzzle flash to current weapon model
				EV_MuzzleFlash();
			}
			fFirstBeam = 0;

			gEngfuncs.pEfxAPI->R_BeamEntPoint( 
				idx | 0x1000,
				tr.endpos,
				m_iBeam,
				0.1,
				m_fPrimaryFire ? 1.0 : 2.5,
				0.0,
				m_fPrimaryFire ? 128.0 : flDamage,
				0,
				0,
				0,
				m_fPrimaryFire ? 255 : 255,
				m_fPrimaryFire ? 128 : 255,
				m_fPrimaryFire ? 0 : 255
			);
		}
		else
		{
			gEngfuncs.pEfxAPI->R_BeamPoints( vecSrc,
				tr.endpos,
				m_iBeam,
				0.1,
				m_fPrimaryFire ? 1.0 : 2.5,
				0.0,
				m_fPrimaryFire ? 128.0 : flDamage,
				0,
				0,
				0,
				m_fPrimaryFire ? 255 : 255,
				m_fPrimaryFire ? 128 : 255,
				m_fPrimaryFire ? 0 : 255
			);
		}

		if ( pEntity->solid == SOLID_BSP )
		{
			float n;

			pentIgnore = NULL;

			n = -DotProduct( tr.plane.normal, forward );

			if (n < 0.5) // 60 degrees
			{
				// ALERT( at_console, "reflect %f\n", n );
				// reflect
				vec3_t r;
			
				VectorMA( forward, 2.0 * n, tr.plane.normal, r );

				flMaxFrac = flMaxFrac - tr.fraction;
				
				VectorCopy( r, forward );

				VectorMA( tr.endpos, 8.0, forward, vecSrc );
				VectorMA( vecSrc, 8192.0, forward, vecDest );

				gEngfuncs.pEfxAPI->R_TempSprite( tr.endpos, vec3_origin, 0.2, m_iGlow, kRenderGlow, kRenderFxNoDissipation, flDamage * n / 255.0, flDamage * n * 0.5 * 0.1, FTENT_FADEOUT );
				
				{
					vec3_t fwd;
					VectorAdd( tr.endpos, tr.plane.normal, fwd );
					gEngfuncs.pEfxAPI->R_Sprite_Trail( TE_SPRITETRAIL, tr.endpos, fwd, m_iBalls, (int)(n * flDamage * 0.3), 0.1, gEngfuncs.pfnRandomFloat( 10, 20 ) / 100.0, 100,
						255, 200 );
				}
	
				// lose energy
				if ( n == 0 )
				{
					n = 0.1;
				}
				
				flDamage = flDamage * (1 - n);

			}
			else
			{
				// tunnel
				EV_HLDM_DecalGunshot( &tr, BULLET_MONSTER_12MM, 0.5, 75, 75, 75, false, CHAR_TEX_CONCRETE );

				gEngfuncs.pEfxAPI->R_TempSprite( tr.endpos, vec3_origin, 1.0, m_iGlow, kRenderGlow, kRenderFxNoDissipation, flDamage / 255.0, 6.0, FTENT_FADEOUT );

				// limit it to one hole punch
				if (fHasPunched)
				{
					break;
				}
				fHasPunched = 1;
				
				// try punching through wall if secondary attack (primary is incapable of breaking through)
				if ( !m_fPrimaryFire )
				{
					vec3_t start;

					VectorMA( tr.endpos, 8.0, forward, start );

					// Store off the old count
					gEngfuncs.pEventAPI->EV_PushPMStates();
						
					// Now add in all of the players.
					gEngfuncs.pEventAPI->EV_SetSolidPlayers ( idx - 1 );

					gEngfuncs.pEventAPI->EV_SetTraceHull( 2 );
					gEngfuncs.pEventAPI->EV_PlayerTrace( start, vecDest, PM_STUDIO_BOX, -1, &beam_tr );

					if ( !beam_tr.allsolid )
					{
						vec3_t delta;
						float n;

						// trace backwards to find exit point

						gEngfuncs.pEventAPI->EV_PlayerTrace( beam_tr.endpos, tr.endpos, PM_STUDIO_BOX, -1, &beam_tr );

						VectorSubtract( beam_tr.endpos, tr.endpos, delta );
						
						n = Length( delta );

						if (n < flDamage)
						{
							if (n == 0)
								n = 1;
							flDamage -= n;

							// absorption balls
							{
								vec3_t fwd;
								VectorSubtract( tr.endpos, forward, fwd );
								gEngfuncs.pEfxAPI->R_Sprite_Trail( TE_SPRITETRAIL, tr.endpos, fwd, m_iBalls, 3, 0.1, gEngfuncs.pfnRandomFloat( 10, 20 ) / 100.0, 100,
									255, 100 );
							}

	//////////////////////////////////// WHAT TO DO HERE
							// CSoundEnt::InsertSound ( bits_SOUND_COMBAT, pev->origin, NORMAL_EXPLOSION_VOLUME, 3.0 );

							EV_HLDM_DecalGunshot( &beam_tr, BULLET_MONSTER_12MM, 0.65, 65, 65, 65, false, CHAR_TEX_CONCRETE );
							
							gEngfuncs.pEfxAPI->R_TempSprite( beam_tr.endpos, vec3_origin, 0.1, m_iGlow, kRenderGlow, kRenderFxNoDissipation, flDamage / 255.0, 6.0, FTENT_FADEOUT );
			
							// balls
							{
								vec3_t fwd;
								VectorSubtract( beam_tr.endpos, forward, fwd );
								gEngfuncs.pEfxAPI->R_Sprite_Trail( TE_SPRITETRAIL, beam_tr.endpos, fwd, m_iBalls, (int)(flDamage * 0.3), 0.1, gEngfuncs.pfnRandomFloat( 10, 20 ) / 100.0, 200,
									255, 40 );
							}
							
							VectorAdd( beam_tr.endpos, forward, vecSrc );
						}
					}
					else
					{
						flDamage = 0;
					}

					gEngfuncs.pEventAPI->EV_PopPMStates();
				}
				else
				{
					if ( m_fPrimaryFire )
					{
						// slug doesn't punch through ever with primary 
						// fire, so leave a little glowy bit and make some balls
						gEngfuncs.pEfxAPI->R_TempSprite( tr.endpos, vec3_origin, 0.2, m_iGlow, kRenderGlow, kRenderFxNoDissipation, 200.0 / 255.0, 0.3, FTENT_FADEOUT );
			
						{
							vec3_t fwd;
							VectorAdd( tr.endpos, tr.plane.normal, fwd );
							gEngfuncs.pEfxAPI->R_Sprite_Trail( TE_SPRITETRAIL, tr.endpos, fwd, m_iBalls, 8, 0.6, gEngfuncs.pfnRandomFloat( 10, 20 ) / 100.0, 100,
								255, 200 );
						}
					}

					flDamage = 0;
				}
			}
		}
		else
		{
			VectorAdd( tr.endpos, forward, vecSrc );
		}
	}
}

void EV_VehiclePitchAdjust( event_args_t *args )
{
	int idx;
	vec3_t origin;

	unsigned short us_params;
	int noise;
	float m_flVolume;
	int pitch;
	int stop;
	
	char sz[ 256 ];

	idx = args->entindex;
	
	VectorCopy( args->origin, origin );

	us_params = (unsigned short)args->iparam1;
	stop	  = args->bparam1;

	m_flVolume	= (float)(us_params & 0x003f)/40.0;
	noise		= (int)(((us_params) >> 12 ) & 0x0007);
	pitch		= (int)( 10.0 * (float)( ( us_params >> 6 ) & 0x003f ) );

	switch ( noise )
	{
	case 1: strcpy( sz, "plats/vehicle1.wav"); break;
	case 2: strcpy( sz, "plats/vehicle2.wav"); break;
	case 3: strcpy( sz, "plats/vehicle3.wav"); break; 
	case 4: strcpy( sz, "plats/vehicle4.wav"); break;
	case 5: strcpy( sz, "plats/vehicle6.wav"); break;
	case 6: strcpy( sz, "plats/vehicle7.wav"); break;
	default:
		// no sound
		strcpy( sz, "" );
		return;
	}

	if ( stop )
	{
		gEngfuncs.pEventAPI->EV_StopSound( idx, CHAN_STATIC, sz );
	}
	else
	{
		gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_STATIC, sz, m_flVolume, ATTN_NORM, SND_CHANGE_PITCH, pitch );
	}
}

void EV_TrainPitchAdjust( event_args_t *args )
{
	int idx;
	vec3_t origin;

	unsigned short us_params;
	int noise;
	float m_flVolume;
	int pitch;
	int stop;
	
	char sz[ 256 ];

	idx = args->entindex;
	
	VectorCopy( args->origin, origin );

	us_params = (unsigned short)args->iparam1;
	stop	  = args->bparam1;

	m_flVolume	= (float)(us_params & 0x003f)/40.0;
	noise		= (int)(((us_params) >> 12 ) & 0x0007);
	pitch		= (int)( 10.0 * (float)( ( us_params >> 6 ) & 0x003f ) );

	switch ( noise )
	{
	case 1: strcpy( sz, "plats/ttrain1.wav"); break;
	case 2: strcpy( sz, "plats/ttrain2.wav"); break;
	case 3: strcpy( sz, "plats/ttrain3.wav"); break; 
	case 4: strcpy( sz, "plats/ttrain4.wav"); break;
	case 5: strcpy( sz, "plats/ttrain6.wav"); break;
	case 6: strcpy( sz, "plats/ttrain7.wav"); break;
	default:
		// no sound
		strcpy( sz, "" );
		return;
	}

	if ( stop )
	{
		gEngfuncs.pEventAPI->EV_StopSound( idx, CHAN_STATIC, sz );
	}
	else
	{
		gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_STATIC, sz, m_flVolume, ATTN_NORM, SND_CHANGE_PITCH, pitch );
	}
}

int EV_TFC_IsAllyTeam( int iTeam1, int iTeam2 )
{
	return 0;
}
//======================
//	   COUNTER-STRIKE
//======================

void EV_FireUSP( event_args_t *args )
{
	int idx;
	vec3_t origin;
	vec3_t angles;
	vec3_t velocity;
	int empty;
	int silencer_on;
	vec3_t ShellVelocity;
	vec3_t ShellOrigin;
	int shell;
	vec3_t vecSrc, vecAiming;
	vec3_t up, right, forward;
	cl_entity_t *ent = GetViewEntity();

	int lefthand = cl_righthand->value == 0;

	silencer_on = args->bparam2;
	empty = args->bparam1;
	idx = args->entindex;
	VectorCopy( args->origin, origin );
	VectorCopy( args->velocity, velocity );
	VectorCopy( args->angles, angles );

	angles[0] += args->iparam1 / 100.0;

	AngleVectors( angles, forward, right, up );

	shell = gEngfuncs.pEventAPI->EV_FindModelIndex ("models/pshell.mdl");// brass shell
	
	if ( EV_IsLocal( idx ) )
	{
		g_iShotsFired++;

		if ( !silencer_on )
		{
			// Add muzzle flash to current weapon model
			EV_MuzzleFlash();

			if ( empty )
				gEngfuncs.pEventAPI->EV_WeaponAnimation( USP_UNSIL_SHOOT_EMPTY, 2 );
			else
				gEngfuncs.pEventAPI->EV_WeaponAnimation( USP_UNSIL_SHOOT1 + gEngfuncs.pfnRandomLong(0,2), 2 );
		}
		else
		{
			if ( empty )
				gEngfuncs.pEventAPI->EV_WeaponAnimation( USP_SHOOT_EMPTY, 2 );
			else
				gEngfuncs.pEventAPI->EV_WeaponAnimation( USP_SHOOT1 + gEngfuncs.pfnRandomLong(0,2), 2 );
		}

		if ( gHUD.m_bGunSmoke == true )
		{
			vec3_t smoke_origin;
			smoke_origin = ent->attachment[0];
			smoke_origin[0] -= forward[0] * 3;
			smoke_origin[1] -= forward[1] * 3;
			smoke_origin[2] -= forward[2] * 3;
			float base_scale = gEngfuncs.pfnRandomFloat( 0.1, 0.25 );

			EV_HLDM_CreateSmoke( smoke_origin, forward, 0, base_scale, 7, 7, 7, 3, velocity, false, 35 );
			EV_HLDM_CreateSmoke( smoke_origin, forward, 20, base_scale + 0.1, 10, 10, 10, 2, velocity, false, 35 );
			EV_HLDM_CreateSmoke( smoke_origin, forward, 40, base_scale, 13, 13, 13, 2, velocity, false, 35 );
		}
	}

	if ( EV_IsLocal( idx ) )
	{
		if ( lefthand )
			EV_GetDefaultShellInfo( args, origin, velocity, ShellVelocity, ShellOrigin, forward, right, up, 36, -14, -14, false );
		else
			EV_GetDefaultShellInfo( args, origin, velocity, ShellVelocity, ShellOrigin, forward, right, up, 36, -14, 14, false );

		ShellVelocity = ShellVelocity * 0.5;
		ShellVelocity[2] += 45;
		ShellOrigin = ent->attachment[1];
	}
	else
	{
		EV_GetDefaultShellInfo( args, origin, velocity, ShellVelocity, ShellOrigin, forward, right, up, 20, -12, 4, false );
	}

	EV_EjectBrass ( ShellOrigin, ShellVelocity, angles[ YAW ], shell, TE_BOUNCE_SHELL, idx, 5 );

	if ( silencer_on )
		gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "weapons/usp1.wav", 1, 2.0, 0, 94 + gEngfuncs.pfnRandomLong( 0, 0xf ) );
	else
		gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "weapons/usp_unsil-1.wav", 1, 0.8, 0, 87 + gEngfuncs.pfnRandomLong( 0, 0x12 ) );

	EV_GetGunPosition( args, vecSrc, origin );
	VectorCopy( forward, vecAiming );

	Vector vSpread;
	vSpread = Vector( args->fparam1, args->fparam2, args->fparam1 );
	vSpread.z = 0;
	EV_HLDM_FireBullets( idx, forward, right, up, 1, vecSrc, vecAiming, vSpread, 4096, BULLET_PLAYER_45ACP, 0, &tracerCount[idx-1], 2 );
}

void EV_FireMP5N( event_args_t *args )
{
	int idx;
	vec3_t origin;
	vec3_t angles;
	vec3_t velocity;
	vec3_t ShellVelocity;
	vec3_t ShellOrigin;
	int shell;
	vec3_t vecSrc, vecAiming;
	vec3_t up, right, forward;
	cl_entity_t *ent = GetViewEntity();

	int lefthand = cl_righthand->value == 0;

	idx = args->entindex;
	VectorCopy( args->origin, origin );
	VectorCopy( args->velocity, velocity );
	VectorCopy( args->angles, angles );

	angles[0] += args->iparam1 / 100.0;
	angles[1] += args->iparam2 / 100.0;

	AngleVectors( angles, forward, right, up );

	shell = gEngfuncs.pEventAPI->EV_FindModelIndex ("models/pshell.mdl");// brass shell
	
	if ( EV_IsLocal( idx ) )
	{
		g_iShotsFired++;

		// Add muzzle flash to current weapon model
		EV_MuzzleFlash();
		gEngfuncs.pEventAPI->EV_WeaponAnimation( MP5N_SHOOT1 + gEngfuncs.pfnRandomLong(0,2), 2 );

		if ( gHUD.m_bGunSmoke == true )
			EV_HLDM_CreateSmoke( ent->attachment[0], forward, 3, 0.25, 10, 10, 10, 3, velocity, false, 35 );
	}

	if ( EV_IsLocal( idx ) )
	{
		if ( lefthand )
			EV_GetDefaultShellInfo( args, origin, velocity, ShellVelocity, ShellOrigin, forward, right, up, 34, -10, -11, false );
		else
			EV_GetDefaultShellInfo( args, origin, velocity, ShellVelocity, ShellOrigin, forward, right, up, 34, -10, 11, false );

		ShellVelocity[2] -= 50;
		ShellOrigin = ent->attachment[1];
	}
	else
	{
		EV_GetDefaultShellInfo( args, origin, velocity, ShellVelocity, ShellOrigin, forward, right, up, 20, -12, 4, false );
	}

	EV_EjectBrass ( ShellOrigin, ShellVelocity, angles[ YAW ], shell, TE_BOUNCE_SHELL, idx, 13 );

	gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "weapons/mp5-1.wav", 1, 0.64, 0, 94 + gEngfuncs.pfnRandomLong( 0, 0xf ) );

	EV_GetGunPosition( args, vecSrc, origin );
	VectorCopy( forward, vecAiming );

	Vector vSpread;
	vSpread = Vector( args->fparam1, args->fparam2, args->fparam1 );
	vSpread.z = 0;
	EV_HLDM_FireBullets( idx, forward, right, up, 1, vecSrc, vecAiming, vSpread, 8192, BULLET_PLAYER_9MM, 0, &tracerCount[idx-1], 2 );
}

void EV_FireAK47( event_args_t *args )
{
	int idx;
	vec3_t origin;
	vec3_t angles;
	vec3_t velocity;
	vec3_t ShellVelocity;
	vec3_t ShellOrigin;
	int shell;
	vec3_t vecSrc, vecAiming;
	vec3_t up, right, forward;
	cl_entity_t *ent = GetViewEntity();

	int lefthand = cl_righthand->value == 0;

	idx = args->entindex;
	VectorCopy( args->origin, origin );
	VectorCopy( args->velocity, velocity );
	VectorCopy( args->angles, angles );

	angles[0] += args->iparam1 / 100.0;
	angles[1] += args->iparam2 / 100.0;

	AngleVectors( angles, forward, right, up );

	shell = gEngfuncs.pEventAPI->EV_FindModelIndex ("models/rshell.mdl");// brass shell
	
	if ( EV_IsLocal( idx ) )
	{
		g_iShotsFired++;

		// Add muzzle flash to current weapon model
		EV_MuzzleFlash();
		gEngfuncs.pEventAPI->EV_WeaponAnimation( AK47_SHOOT1 + gEngfuncs.pfnRandomLong(0,2), 2 );

		if ( gHUD.m_bGunSmoke == true )
			EV_HLDM_CreateSmoke( ent->attachment[0], forward, 3, 0.35, 20, 20, 20, 4, velocity, false, 35 );
	}

	if ( EV_IsLocal( idx ) )
	{
		if ( lefthand )
			EV_GetDefaultShellInfo( args, origin, velocity, ShellVelocity, ShellOrigin, forward, right, up, 20, -8, -10, false );
		else
			EV_GetDefaultShellInfo( args, origin, velocity, ShellVelocity, ShellOrigin, forward, right, up, 20, -8, 10, false );

		ShellVelocity[2] -= 75;
		ShellOrigin = ent->attachment[1];
	}
	else
	{
		EV_GetDefaultShellInfo( args, origin, velocity, ShellVelocity, ShellOrigin, forward, right, up, 20, -12, 4, false );
	}

	EV_EjectBrass ( ShellOrigin, ShellVelocity, angles[ YAW ], shell, TE_BOUNCE_SHELL, idx, 9 );

	gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "weapons/ak47-1.wav", 1, 0.4, 0, 94 + gEngfuncs.pfnRandomLong( 0, 0xf ) );

	EV_GetGunPosition( args, vecSrc, origin );

	VectorCopy( forward, vecAiming );
	Vector vSpread;
	vSpread = Vector( args->fparam1, args->fparam2, args->fparam1 );
	vSpread.z = 0;
	EV_HLDM_FireBullets( idx, forward, right, up, 1, vecSrc, vecAiming, vSpread, 8192, BULLET_PLAYER_762MM, 0, &tracerCount[idx-1], 2 );
}

void EV_FireAug( event_args_t *args )
{
	int idx;
	vec3_t origin;
	vec3_t angles;
	vec3_t velocity;
	vec3_t ShellVelocity;
	vec3_t ShellOrigin;
	int shell;
	vec3_t vecSrc, vecAiming;
	vec3_t up, right, forward;
	cl_entity_t *ent = GetViewEntity();

	int lefthand = cl_righthand->value == 0;

	idx = args->entindex;
	VectorCopy( args->origin, origin );
	VectorCopy( args->velocity, velocity );
	VectorCopy( args->angles, angles );

	angles[0] += args->iparam1 / 100.0;
	angles[1] += args->iparam2 / 100.0;

	AngleVectors( angles, forward, right, up );

	shell = gEngfuncs.pEventAPI->EV_FindModelIndex ("models/rshell.mdl");// brass shell
	
	if ( EV_IsLocal( idx ) )
	{
		g_iShotsFired++;

		// Add muzzle flash to current weapon model
		EV_MuzzleFlash();
		gEngfuncs.pEventAPI->EV_WeaponAnimation( AUG_SHOOT1 + gEngfuncs.pfnRandomLong(0,2), 2 );

		if ( gHUD.m_bGunSmoke == true )
			EV_HLDM_CreateSmoke( ent->attachment[0], forward, 3, 0.26, 15, 15, 15, 4, velocity, false, 35 );
	}

	if ( EV_IsLocal( idx ) )
	{
		if ( lefthand )
			EV_GetDefaultShellInfo( args, origin, velocity, ShellVelocity, ShellOrigin, forward, right, up, 17, -8, -14, true );
		else
			EV_GetDefaultShellInfo( args, origin, velocity, ShellVelocity, ShellOrigin, forward, right, up, 17, -8, 14, true );

		ShellVelocity = ShellVelocity * 1.25;
		ShellVelocity[2] -= 122;
		ShellOrigin = ent->attachment[1];
	}
	else
	{
		EV_GetDefaultShellInfo( args, origin, velocity, ShellVelocity, ShellOrigin, forward, right, up, 20, -12, 4, false );
	}

	EV_EjectBrass ( ShellOrigin, ShellVelocity, angles[ YAW ], shell, TE_BOUNCE_SHELL, idx, 8 );

	gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "weapons/aug-1.wav", 1, 0.48, 0, 94 + gEngfuncs.pfnRandomLong( 0, 0xf ) );

	EV_GetGunPosition( args, vecSrc, origin );

	VectorCopy( forward, vecAiming );
	Vector vSpread;
	vSpread = Vector( args->fparam1, args->fparam2, args->fparam1 );
	vSpread.z = 0;
	EV_HLDM_FireBullets( idx, forward, right, up, 1, vecSrc, vecAiming, vSpread, 8192, BULLET_PLAYER_556MM, 0, &tracerCount[idx-1], 2 );
}

void EV_FireDeagle( event_args_t *args )
{
	int idx;
	vec3_t origin;
	vec3_t angles;
	vec3_t velocity;
	int empty;
	vec3_t ShellVelocity;
	vec3_t ShellOrigin;
	int shell;
	vec3_t vecSrc, vecAiming;
	vec3_t up, right, forward;
	cl_entity_t *ent = GetViewEntity();

	int lefthand = cl_righthand->value == 0;

	empty = args->bparam1;
	idx = args->entindex;
	VectorCopy( args->origin, origin );
	VectorCopy( args->velocity, velocity );
	VectorCopy( args->angles, angles );

	angles[0] += args->iparam1 / 100.0;
	angles[1] += args->iparam2 / 100.0;

	AngleVectors( angles, forward, right, up );

	shell = gEngfuncs.pEventAPI->EV_FindModelIndex ("models/pshell.mdl");// brass shell
	
	if ( EV_IsLocal( idx ) )
	{
		g_iShotsFired++;

		// Add muzzle flash to current weapon model
		EV_MuzzleFlash();
		if ( empty )
			gEngfuncs.pEventAPI->EV_WeaponAnimation( DEAGLE_SHOOT_EMPTY, 2 );
		else
			gEngfuncs.pEventAPI->EV_WeaponAnimation( DEAGLE_SHOOT1 + gEngfuncs.pfnRandomLong(0,1), 2 );

		if ( gHUD.m_bGunSmoke == true )
		{
			EV_HLDM_CreateSmoke( ent->attachment[0], forward, 0, 0.25, 10, 10, 10, 3, velocity, false, 35 );
			EV_HLDM_CreateSmoke( ent->attachment[0], forward, 25, 0.3, 15, 15, 15, 2, velocity, false, 35 );
			EV_HLDM_CreateSmoke( ent->attachment[0], forward, 50, 0.2, 25, 25, 25, 2, velocity, false, 35 );
		}
	}

	if ( EV_IsLocal( idx ) )
	{
		if ( lefthand )
			EV_GetDefaultShellInfo( args, origin, velocity, ShellVelocity, ShellOrigin, forward, right, up, 35, -11, -16, false );
		else
			EV_GetDefaultShellInfo( args, origin, velocity, ShellVelocity, ShellOrigin, forward, right, up, 35, -11, 16, false );

		ShellVelocity = ShellVelocity * 0.75;
		ShellVelocity[2] += 25;
		ShellOrigin = ent->attachment[1];
	}
	else
	{
		EV_GetDefaultShellInfo( args, origin, velocity, ShellVelocity, ShellOrigin, forward, right, up, 20, -12, 4, false );
	}

	EV_EjectBrass ( ShellOrigin, ShellVelocity, angles[ YAW ], shell, TE_BOUNCE_SHELL, idx, 5 );

	gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "weapons/deagle-1.wav", 1, 0.6, 0, 94 + gEngfuncs.pfnRandomLong( 0, 0xf ) );

	EV_GetGunPosition( args, vecSrc, origin );
	VectorCopy( forward, vecAiming );

	Vector vSpread;
	vSpread = Vector( args->fparam1, args->fparam2, args->fparam1 );
	vSpread.z = 0;
	EV_HLDM_FireBullets( idx, forward, right, up, 1, vecSrc, vecAiming, vSpread, 8192, BULLET_PLAYER_50AE, 0, &tracerCount[idx-1], 2 );
}

void EV_FireG3SG1( event_args_t *args )
{
	int idx;
	vec3_t origin;
	vec3_t angles;
	vec3_t velocity;
	vec3_t ShellVelocity;
	vec3_t ShellOrigin;
	int shell;
	vec3_t vecSrc, vecAiming;
	vec3_t up, right, forward;
	cl_entity_t *ent = GetViewEntity();

	int lefthand = cl_righthand->value == 0;

	idx = args->entindex;
	VectorCopy( args->origin, origin );
	VectorCopy( args->velocity, velocity );
	VectorCopy( args->angles, angles );

	angles[0] += args->iparam1 / 100.0;
	angles[1] += args->iparam2 / 100.0;

	AngleVectors( angles, forward, right, up );

	shell = gEngfuncs.pEventAPI->EV_FindModelIndex ("models/rshell.mdl");// brass shell
	
	if ( EV_IsLocal( idx ) )
	{
		g_iShotsFired++;

		// Add muzzle flash to current weapon model
		EV_MuzzleFlash();
		gEngfuncs.pEventAPI->EV_WeaponAnimation( G3SG1_SHOOT1 + gEngfuncs.pfnRandomLong(0,1), 2 );

		if ( gHUD.m_bGunSmoke == true )
		{
			EV_HLDM_CreateSmoke( ent->attachment[0], forward, 3, 0.3, 35, 35, 35, 4, velocity, false, 35 );
			EV_HLDM_CreateSmoke( ent->attachment[0], forward, 35, 0.35, 30, 30, 30, 2, velocity, false, 35 );
			EV_HLDM_CreateSmoke( ent->attachment[0], forward, 70, 0.3, 25, 25, 25, 2, velocity, false, 35 );
		}
	}

	if ( EV_IsLocal( idx ) )
	{
		if ( lefthand )
			EV_GetDefaultShellInfo( args, origin, velocity, ShellVelocity, ShellOrigin, forward, right, up, 20, -8, -10, false );
		else
			EV_GetDefaultShellInfo( args, origin, velocity, ShellVelocity, ShellOrigin, forward, right, up, 17, -8, 10, false );

		ShellVelocity = ShellVelocity * 1.5;
		ShellVelocity[2] -= 50;
		ShellOrigin = ent->attachment[1];
	}
	else
	{
		EV_GetDefaultShellInfo( args, origin, velocity, ShellVelocity, ShellOrigin, forward, right, up, 20, -12, 4, false );
	}

	EV_EjectBrass ( ShellOrigin, ShellVelocity, angles[ YAW ], shell, TE_BOUNCE_SHELL, idx, 17 );

	gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "weapons/g3sg1-1.wav", 1, 0.4, 0, 94 + gEngfuncs.pfnRandomLong( 0, 0xf ) );

	EV_GetGunPosition( args, vecSrc, origin );
	VectorCopy( forward, vecAiming );

	Vector vSpread;
	vSpread = Vector( args->fparam1, args->fparam2, args->fparam1 );
	EV_HLDM_FireBullets( idx, forward, right, up, 1, vecSrc, vecAiming, vSpread, 8192, BULLET_PLAYER_762MM, 0, &tracerCount[idx-1], 2 );
}

void EV_FireGlock18( event_args_t *args )
{
	int idx;
	vec3_t origin;
	vec3_t angles;
	vec3_t velocity;
	int empty;
	vec3_t ShellVelocity;
	vec3_t ShellOrigin;
	int shell;
	vec3_t vecSrc, vecAiming;
	vec3_t up, right, forward;
	cl_entity_t *ent = GetViewEntity();

	int lefthand = cl_righthand->value == 0;

	empty = args->bparam1;
	idx = args->entindex;
	VectorCopy( args->origin, origin );
	VectorCopy( args->velocity, velocity );
	VectorCopy( args->angles, angles );

	angles[0] += args->iparam1 / 100.0;
	angles[1] += args->iparam2 / 100.0;

	AngleVectors( angles, forward, right, up );

	shell = gEngfuncs.pEventAPI->EV_FindModelIndex ("models/pshell.mdl");// brass shell
	
	if ( EV_IsLocal( idx ) )
	{
		// Add muzzle flash to current weapon model
		EV_MuzzleFlash();
		if ( empty )
			gEngfuncs.pEventAPI->EV_WeaponAnimation( GLOCK18_SHOOT_EMPTY, 2 );
		else
			gEngfuncs.pEventAPI->EV_WeaponAnimation( GLOCK18_SHOOT3, 2 );

		if ( gHUD.m_bGunSmoke == true )
		{
			EV_HLDM_CreateSmoke( ent->attachment[0], forward, 0, 0.2, 10, 10, 10, 3, velocity, false, 35 );
			EV_HLDM_CreateSmoke( ent->attachment[0], forward, 40, 0.4, 20, 20, 20, 2, velocity, false, 35 );
		}
	}

	if ( EV_IsLocal( idx ) )
	{
		g_iShotsFired++;

		if ( lefthand )
			EV_GetDefaultShellInfo( args, origin, velocity, ShellVelocity, ShellOrigin, forward, right, up, 36, -14, -14, false );
		else
			EV_GetDefaultShellInfo( args, origin, velocity, ShellVelocity, ShellOrigin, forward, right, up, 36, -14, 14, false );

		ShellVelocity = ShellVelocity * 0.65;
		ShellVelocity[2] += 25;
		ShellOrigin = ent->attachment[1];
	}
	else
	{
		EV_GetDefaultShellInfo( args, origin, velocity, ShellVelocity, ShellOrigin, forward, right, up, 20, -12, 4, false );
	}

	EV_EjectBrass ( ShellOrigin, ShellVelocity, angles[ YAW ], shell, TE_BOUNCE_SHELL, idx, 4 );

	gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "weapons/glock18-2.wav", 1, 0.8, 0, 94 + gEngfuncs.pfnRandomLong( 0, 0xf ) );

	EV_GetGunPosition( args, vecSrc, origin );
	VectorCopy( forward, vecAiming );

	Vector vSpread;
	vSpread = Vector( args->fparam1, args->fparam2, args->fparam1 );
	EV_HLDM_FireBullets( idx, forward, right, up, 1, vecSrc, vecAiming, vSpread, 4096, BULLET_PLAYER_9MM, 0, &tracerCount[idx-1], 2 );
}

void EV_FireM249( event_args_t *args )
{
	int idx;
	vec3_t origin;
	vec3_t angles;
	vec3_t velocity;
	vec3_t ShellVelocity;
	vec3_t ShellOrigin;
	int shell;
	vec3_t vecSrc, vecAiming;
	vec3_t up, right, forward;
	cl_entity_t *ent = GetViewEntity();

	int lefthand = cl_righthand->value == 0;

	idx = args->entindex;
	VectorCopy( args->origin, origin );
	VectorCopy( args->velocity, velocity );
	VectorCopy( args->angles, angles );

	angles[0] += args->iparam1 / 100.0;
	angles[1] += args->iparam2 / 100.0;

	AngleVectors( angles, forward, right, up );

	shell = gEngfuncs.pEventAPI->EV_FindModelIndex ("models/rshell.mdl");// brass shell
	
	if ( EV_IsLocal( idx ) )
	{
		g_iShotsFired++;

		// Add muzzle flash to current weapon model
		EV_MuzzleFlash();
		gEngfuncs.pEventAPI->EV_WeaponAnimation( M249_SHOOT1 + gEngfuncs.pfnRandomLong(0,1), 2 );

		if ( gHUD.m_bGunSmoke == true )
			EV_HLDM_CreateSmoke( ent->attachment[0], forward, 3, 0.26, 15, 15, 15, 4, velocity, false, 35 );
	}

	if ( EV_IsLocal( idx ) )
	{
		if ( lefthand )
			EV_GetDefaultShellInfo( args, origin, velocity, ShellVelocity, ShellOrigin, forward, right, up, 20, -10, -13, true );
		else
			EV_GetDefaultShellInfo( args, origin, velocity, ShellVelocity, ShellOrigin, forward, right, up, 20, -10, 13, true );

		ShellVelocity = ShellVelocity * 1.2;
		ShellVelocity[2] -= 75;
		ShellOrigin = ent->attachment[1];
	}
	else
	{
		EV_GetDefaultShellInfo( args, origin, velocity, ShellVelocity, ShellOrigin, forward, right, up, 20, -12, 4, false );
	}

	EV_EjectBrass ( ShellOrigin, ShellVelocity, angles[ YAW ], shell, TE_BOUNCE_SHELL, idx, 10 );

	gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "weapons/m249-1.wav", 1, 0.52, 0, 94 + gEngfuncs.pfnRandomLong( 0, 0xf ) );

	EV_GetGunPosition( args, vecSrc, origin );

	VectorCopy( forward, vecAiming );
	Vector vSpread;
	vSpread = Vector( args->fparam1, args->fparam2, args->fparam1 );
	vSpread.z = 0;
	EV_HLDM_FireBullets( idx, forward, right, up, 1, vecSrc, vecAiming, vSpread, 8192, BULLET_PLAYER_556MM, 0, &tracerCount[idx-1], 2 );
}

void EV_FireM3( event_args_t *args )
{
	int idx;
	vec3_t origin;
	vec3_t angles;
	vec3_t velocity;

	cl_entity_t *ent;
	vec3_t vecSrc, vecAiming;
	vec3_t vecSpread;
	vec3_t up, right, forward;

	ent = GetViewEntity();
	idx = args->entindex;
	VectorCopy( args->origin, origin );
	VectorCopy( args->angles, angles );
	VectorCopy( args->velocity, velocity );

	AngleVectors( angles, forward, right, up );

	if ( EV_IsLocal( idx ) )
	{
		g_iShotsFired++;

		// Add muzzle flash to current weapon model
		EV_MuzzleFlash();
		gEngfuncs.pEventAPI->EV_WeaponAnimation( M3_FIRE1 + gEngfuncs.pfnRandomLong(0,1), 2 );

		if ( gHUD.m_bGunSmoke == true )
		{
			EV_HLDM_CreateSmoke( ent->attachment[0], forward, 3, 0.5, 25, 25, 25, 3, velocity, false, 35 );
			EV_HLDM_CreateSmoke( ent->attachment[0], forward, 25, 0.5, 20, 20, 20, 2, velocity, false, 35 );
			EV_HLDM_CreateSmoke( ent->attachment[0], forward, 50, 0.45, 15, 15, 15, 2, velocity, false, 35 );
			EV_HLDM_CreateSmoke( ent->attachment[0], forward, 75, 0.35, 7, 7, 7, 2, velocity, false, 35 );
		}
	}

	gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "weapons/m3-1.wav", 1, 0.48, 0, 94 + gEngfuncs.pfnRandomLong( 0, 0xf ) );

	EV_GetGunPosition( args, vecSrc, origin );
	VectorCopy( forward, vecAiming );

	vecSpread = Vector( 0.0675, 0.0675, 0.0 );
	EV_HLDM_FireBullets( idx, forward, right, up, 9, vecSrc, vecAiming, vecSpread, 8192, BULLET_PLAYER_BUCKSHOT, 0, &tracerCount[idx-1], 1 );
}

void EV_FireM4A1( event_args_t *args )
{
	int idx;
	vec3_t origin;
	vec3_t angles;
	vec3_t velocity;
	int silencer_on;

	vec3_t ShellVelocity;
	vec3_t ShellOrigin;
	int shell;
	vec3_t vecSrc, vecAiming;
	vec3_t up, right, forward;
	cl_entity_t *ent = GetViewEntity();

	int lefthand = cl_righthand->value == 0;

	idx = args->entindex;
	silencer_on = args->bparam1;
	VectorCopy( args->origin, origin );
	VectorCopy( args->velocity, velocity );
	VectorCopy( args->angles, angles );

	angles[0] += args->iparam1 / 100.0;
	angles[1] += args->iparam2 / 100.0;

	AngleVectors( angles, forward, right, up );

	shell = gEngfuncs.pEventAPI->EV_FindModelIndex ("models/rshell.mdl");// brass shell
	
	if ( EV_IsLocal( idx ) )
	{
		// Add muzzle flash to current weapon model
		EV_MuzzleFlash();
		if ( silencer_on )
			gEngfuncs.pEventAPI->EV_WeaponAnimation( M4A1_SHOOT1 + gEngfuncs.pfnRandomLong(0,2), 2 );
		else
			gEngfuncs.pEventAPI->EV_WeaponAnimation( M4A1_UNSIL_SHOOT1 + gEngfuncs.pfnRandomLong(0,2), 2 );

		if ( gHUD.m_bGunSmoke == true )
		{
			if ( silencer_on )
				EV_HLDM_CreateSmoke( ent->attachment[0], forward, 3, 0.3, 16, 16, 16, 3, velocity, false, 35 );
			else
				EV_HLDM_CreateSmoke( ent->attachment[2], forward, 3, 0.2, 16, 16, 16, 4, velocity, false, 35 );
		}
	}

	if ( EV_IsLocal( idx ) )
	{
		g_iShotsFired++;

		if ( lefthand )
			EV_GetDefaultShellInfo( args, origin, velocity, ShellVelocity, ShellOrigin, forward, right, up, 20, -8, -10, false );
		else
			EV_GetDefaultShellInfo( args, origin, velocity, ShellVelocity, ShellOrigin, forward, right, up, 20, -8, 10, false );

		ShellVelocity[2] -= 45;
		ShellOrigin = ent->attachment[1];
	}
	else
	{
		EV_GetDefaultShellInfo( args, origin, velocity, ShellVelocity, ShellOrigin, forward, right, up, 20, -12, 4, false );
	}

	EV_EjectBrass ( ShellOrigin, ShellVelocity, angles[ YAW ], shell, TE_BOUNCE_SHELL, idx, 10 );

	if ( silencer_on )
		gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "weapons/m4a1-1.wav", 1, 1.4, 0, 94 + gEngfuncs.pfnRandomLong( 0, 0xf ) );
	else if ( gEngfuncs.pfnRandomLong( 0, 1 ) )
		gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "weapons/m4a1_unsil-1.wav", 1, 0.52, 0, 94 + gEngfuncs.pfnRandomLong( 0, 0xf ) );
	else
		gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "weapons/m4a1_unsil-2.wav", 1, 0.52, 0, 94 + gEngfuncs.pfnRandomLong( 0, 0xf ) );

	EV_GetGunPosition( args, vecSrc, origin );

	VectorCopy( forward, vecAiming );
	Vector vSpread;
	vSpread = Vector( args->fparam1, args->fparam2, args->fparam1 );
	vSpread.z = 0;
	EV_HLDM_FireBullets( idx, forward, right, up, 1, vecSrc, vecAiming, vSpread, 8192, BULLET_PLAYER_556MM, 0, &tracerCount[idx-1], 2 );
}

void EV_FireMac10( event_args_t *args )
{
	int idx;
	vec3_t origin;
	vec3_t angles;
	vec3_t velocity;
	vec3_t ShellVelocity;
	vec3_t ShellOrigin;
	int shell;
	vec3_t vecSrc, vecAiming;
	vec3_t up, right, forward;
	cl_entity_t *ent = GetViewEntity();

	int lefthand = cl_righthand->value == 0;

	idx = args->entindex;
	VectorCopy( args->origin, origin );
	VectorCopy( args->velocity, velocity );
	VectorCopy( args->angles, angles );

	angles[0] += args->iparam1 / 100.0;
	angles[1] += args->iparam2 / 100.0;

	AngleVectors( angles, forward, right, up );

	shell = gEngfuncs.pEventAPI->EV_FindModelIndex ("models/pshell.mdl");// brass shell
	
	if ( EV_IsLocal( idx ) )
	{
		// Add muzzle flash to current weapon model
		EV_MuzzleFlash();
		gEngfuncs.pEventAPI->EV_WeaponAnimation( MAC10_SHOOT1 + gEngfuncs.pfnRandomLong(0,2), 2 );

		if ( gHUD.m_bGunSmoke == true )
			EV_HLDM_CreateSmoke( ent->attachment[0], forward, 2, 0.25, 8, 8, 8, 3, velocity, false, 35 );
	}

	if ( EV_IsLocal( idx ) )
	{
		g_iShotsFired++;

		if ( lefthand )
			EV_GetDefaultShellInfo( args, origin, velocity, ShellVelocity, ShellOrigin, forward, right, up, 32, -9, -11, false );
		else
			EV_GetDefaultShellInfo( args, origin, velocity, ShellVelocity, ShellOrigin, forward, right, up, 32, -9, 11, false );

		ShellVelocity = ShellVelocity * 0.75;
		ShellVelocity[2] -= 65;
		ShellOrigin = ent->attachment[1];
	}
	else
	{
		EV_GetDefaultShellInfo( args, origin, velocity, ShellVelocity, ShellOrigin, forward, right, up, 20, -12, 4, false );
	}

	EV_EjectBrass ( ShellOrigin, ShellVelocity, angles[ YAW ], shell, TE_BOUNCE_SHELL, idx, 7 );

	gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "weapons/mac10-1.wav", 1, 0.72, 0, 94 + gEngfuncs.pfnRandomLong( 0, 0xf ) );

	EV_GetGunPosition( args, vecSrc, origin );
	VectorCopy( forward, vecAiming );

	Vector vSpread;
	vSpread = Vector( args->fparam1, args->fparam2, args->fparam1 );
	vSpread.z = 0;
	EV_HLDM_FireBullets( idx, forward, right, up, 1, vecSrc, vecAiming, vSpread, 8192, BULLET_PLAYER_9MM, 0, &tracerCount[idx-1], 2 );
}

void EV_FireP228( event_args_t *args )
{
	int idx;
	vec3_t origin;
	vec3_t angles;
	vec3_t velocity;
	int empty;
	vec3_t ShellVelocity;
	vec3_t ShellOrigin;
	int shell;
	vec3_t vecSrc, vecAiming;
	vec3_t up, right, forward;
	cl_entity_t *ent = GetViewEntity();

	int lefthand = cl_righthand->value == 0;

	empty = args->bparam1;
	idx = args->entindex;
	VectorCopy( args->origin, origin );
	VectorCopy( args->velocity, velocity );
	VectorCopy( args->angles, angles );

	angles[0] += args->iparam1 / 100.0;
	angles[1] += args->iparam2 / 100.0;

	AngleVectors( angles, forward, right, up );

	shell = gEngfuncs.pEventAPI->EV_FindModelIndex ("models/pshell.mdl");// brass shell
	
	if ( EV_IsLocal( idx ) )
	{
		g_iShotsFired++;

		// Add muzzle flash to current weapon model
		EV_MuzzleFlash();
		if ( empty )
			gEngfuncs.pEventAPI->EV_WeaponAnimation( P228_SHOOT_EMPTY, 2 );
		else
			gEngfuncs.pEventAPI->EV_WeaponAnimation( P228_SHOOT1 + gEngfuncs.pfnRandomLong(0,2), 2 );

		if ( gHUD.m_bGunSmoke == true )
		{
			EV_HLDM_CreateSmoke( ent->attachment[0], forward, 0, 0.25, 10, 10, 10, 3, velocity, false, 35 );
			EV_HLDM_CreateSmoke( ent->attachment[0], forward, 25, 0.3, 15, 15, 15, 2, velocity, false, 35 );
			EV_HLDM_CreateSmoke( ent->attachment[0], forward, 50, 0.2, 20, 20, 20, 2, velocity, false, 35 );
		}
	}

	if ( EV_IsLocal( idx ) )
	{
		if ( lefthand )
			EV_GetDefaultShellInfo( args, origin, velocity, ShellVelocity, ShellOrigin, forward, right, up, 36, -14, -14, false );
		else
			EV_GetDefaultShellInfo( args, origin, velocity, ShellVelocity, ShellOrigin, forward, right, up, 36, -14, 14, false );

		ShellVelocity = ShellVelocity * 0.75;
		ShellVelocity[2] += 35;
		ShellOrigin = ent->attachment[1];
	}
	else
	{
		EV_GetDefaultShellInfo( args, origin, velocity, ShellVelocity, ShellOrigin, forward, right, up, 20, -12, 4, false );
	}

	EV_EjectBrass ( ShellOrigin, ShellVelocity, angles[ YAW ], shell, TE_BOUNCE_SHELL, idx, 4 );

	gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "weapons/p228-1.wav", 1, 0.8, 0, 94 + gEngfuncs.pfnRandomLong( 0, 0xf ) );

	EV_GetGunPosition( args, vecSrc, origin );
	VectorCopy( forward, vecAiming );

	Vector vSpread;
	vSpread = Vector( args->fparam1, args->fparam2, args->fparam1 );
	vSpread.z = 0;
	EV_HLDM_FireBullets( idx, forward, right, up, 1, vecSrc, vecAiming, vSpread, 8192, BULLET_PLAYER_357SIG, 0, &tracerCount[idx-1], 2 );
}

void EV_FireP90( event_args_t *args )
{
	int idx;
	vec3_t origin;
	vec3_t angles;
	vec3_t velocity;
	vec3_t ShellVelocity;
	vec3_t ShellOrigin;
	int shell;
	vec3_t vecSrc, vecAiming;
	vec3_t up, right, forward;
	cl_entity_t *ent = GetViewEntity();

	int lefthand = cl_righthand->value == 0;

	idx = args->entindex;
	VectorCopy( args->origin, origin );
	VectorCopy( args->velocity, velocity );
	VectorCopy( args->angles, angles );

	angles[0] += args->iparam1 / 100.0;
	angles[1] += args->iparam2 / 100.0;

	AngleVectors( angles, forward, right, up );

	shell = gEngfuncs.pEventAPI->EV_FindModelIndex ("models/rshell.mdl");// brass shell
	
	if ( EV_IsLocal( idx ) )
	{
		// Add muzzle flash to current weapon model
		EV_MuzzleFlash();
		gEngfuncs.pEventAPI->EV_WeaponAnimation( P90_SHOOT1 + gEngfuncs.pfnRandomLong(0,2), 2 );

		if ( gHUD.m_bGunSmoke == true )
			EV_HLDM_CreateSmoke( ent->attachment[0], forward, 3, 0.16, 7, 7, 7, 4, velocity, false, 35 );
	}

	if ( EV_IsLocal( idx ) )
	{
		g_iShotsFired++;

		if ( lefthand )
			EV_GetDefaultShellInfo( args, origin, velocity, ShellVelocity, ShellOrigin, forward, right, up, 35, -16, -22, true );
		else
			EV_GetDefaultShellInfo( args, origin, velocity, ShellVelocity, ShellOrigin, forward, right, up, 35, -16, 22, true );

		ShellVelocity = ShellVelocity * 1.25;
		ShellVelocity[2] -= 50;
		ShellOrigin = ent->attachment[1];
	}
	else
	{
		EV_GetDefaultShellInfo( args, origin, velocity, ShellVelocity, ShellOrigin, forward, right, up, 20, -12, 4, false );
	}

	EV_EjectBrass ( ShellOrigin, ShellVelocity, angles[ YAW ], shell, TE_BOUNCE_SHELL, idx, 10 );

	gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "weapons/p90-1.wav", 1, 0.64, 0, 94 + gEngfuncs.pfnRandomLong( 0, 0xf ) );

	EV_GetGunPosition( args, vecSrc, origin );

	VectorCopy( forward, vecAiming );
	Vector vSpread;
	vSpread = Vector( args->fparam1, args->fparam2, args->fparam1 );
	vSpread.z = 0;
	EV_HLDM_FireBullets( idx, forward, right, up, 1, vecSrc, vecAiming, vSpread, 8192, BULLET_PLAYER_57MM, 0, &tracerCount[idx-1], 2 );
}

void EV_FireXM1014( event_args_t *args )
{
	int idx;
	vec3_t origin;
	vec3_t angles;
	vec3_t velocity;
	vec3_t ShellVelocity;
	vec3_t ShellOrigin;
	int shell;
	vec3_t vecSrc, vecAiming;
	vec3_t vecSpread;
	vec3_t up, right, forward;
	cl_entity_t *ent = GetViewEntity();

	int lefthand = cl_righthand->value == 0;

	idx = args->entindex;
	VectorCopy( args->origin, origin );
	VectorCopy( args->velocity, velocity );
	VectorCopy( args->angles, angles );

	angles[0] += args->iparam1 / 100.0;
	angles[1] += args->iparam2 / 100.0;

	AngleVectors( angles, forward, right, up );

	shell = gEngfuncs.pEventAPI->EV_FindModelIndex ("models/shotgunshell.mdl");// brass shell
	
	if ( EV_IsLocal( idx ) )
	{
		// Add muzzle flash to current weapon model
		EV_MuzzleFlash();
		gEngfuncs.pEventAPI->EV_WeaponAnimation( XM1014_FIRE1 + gEngfuncs.pfnRandomLong(0,1), 2 );

		if ( gHUD.m_bGunSmoke == true )
		{
			EV_HLDM_CreateSmoke( ent->attachment[0], forward, 3, 0.45, 15, 15, 15, 3, velocity, false, 35 );
			EV_HLDM_CreateSmoke( ent->attachment[0], forward, 40, 0.35, 9, 9, 9, 2, velocity, false, 35 );
		}
	}

	if ( EV_IsLocal( idx ) )
	{
		g_iShotsFired++;

		if ( lefthand )
			EV_GetDefaultShellInfo( args, origin, velocity, ShellVelocity, ShellOrigin, forward, right, up, 22, -9, -11, false );
		else
			EV_GetDefaultShellInfo( args, origin, velocity, ShellVelocity, ShellOrigin, forward, right, up, 22, -9, 11, false );

		ShellVelocity = ShellVelocity * 1.25;
		ShellVelocity[2] -= 50;
		ShellOrigin = ent->attachment[1];
	}
	else
	{
		EV_GetDefaultShellInfo( args, origin, velocity, ShellVelocity, ShellOrigin, forward, right, up, 20, -12, 4, false );
	}

	EV_EjectBrass ( ShellOrigin, ShellVelocity, angles[ YAW ], shell, TE_BOUNCE_SHOTSHELL, idx, 3 );

	gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "weapons/xm1014-1.wav", 1, 0.52, 0, 94 + gEngfuncs.pfnRandomLong( 0, 0xf ) );

	EV_GetGunPosition( args, vecSrc, origin );
	VectorCopy( forward, vecAiming );

	vecSpread = Vector( 0.0725, 0.0725, 0.0 );
	EV_HLDM_FireBullets( idx, forward, right, up, 6, vecSrc, vecAiming, vecSpread, 8192, BULLET_PLAYER_BUCKSHOT, 0, &tracerCount[idx-1], 1 );
}

void EV_FireScout( event_args_t *args )
{
	int idx;
	vec3_t origin;
	vec3_t angles;
	vec3_t velocity;

	cl_entity_t *ent;
	vec3_t vecSrc, vecAiming;
	vec3_t up, right, forward;

	ent = GetViewEntity();
	idx = args->entindex;
	VectorCopy( args->origin, origin );
	VectorCopy( args->velocity, velocity );
	VectorCopy( args->angles, angles );

	angles[0] += args->iparam1 / 100.0;
	angles[1] += args->iparam2 / 100.0;

	AngleVectors( angles, forward, right, up );

	if ( EV_IsLocal( idx ) )
	{
		g_iShotsFired++;

		// Add muzzle flash to current weapon model
		EV_MuzzleFlash();
		gEngfuncs.pEventAPI->EV_WeaponAnimation( SCOUT_SHOOT + gEngfuncs.pfnRandomLong(0,1), 2 );

		if ( gHUD.m_bGunSmoke == true )
		{
			EV_HLDM_CreateSmoke( ent->attachment[0], forward, 3, 0.45, 25, 25, 25, 3, velocity, false, 35 );
			EV_HLDM_CreateSmoke( ent->attachment[0], forward, 35, 0.45, 20, 20, 20, 2, velocity, false, 35 );
			EV_HLDM_CreateSmoke( ent->attachment[0], forward, 70, 0.35, 15, 15, 15, 2, velocity, false, 35 );
		}
	}

	gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "weapons/scout_fire-1.wav", 1, 1.6, 0, 94 + gEngfuncs.pfnRandomLong( 0, 0xf ) );

	EV_GetGunPosition( args, vecSrc, origin );
	VectorCopy( forward, vecAiming );

	Vector vSpread;
	vSpread.x = args->fparam1 / 1000.0;
	vSpread.y = args->fparam2 / 1000.0;
	vSpread.z = 0;
	EV_HLDM_FireBullets( idx, forward, right, up, 1, vecSrc, vecAiming, vSpread, 8192, BULLET_PLAYER_762MM, 0, &tracerCount[idx-1], 2 );
}

void EV_FireAWP( event_args_t *args )
{
	int idx;
	vec3_t origin;
	vec3_t angles;
	vec3_t velocity;

	cl_entity_t *ent;
	vec3_t vecSrc, vecAiming;
	vec3_t up, right, forward;

	ent = GetViewEntity();
	idx = args->entindex;
	VectorCopy( args->origin, origin );
	VectorCopy( args->velocity, velocity );
	VectorCopy( args->angles, angles );

	angles[0] += args->iparam1 / 100.0;
	angles[1] += args->iparam2 / 100.0;

	AngleVectors( angles, forward, right, up );

	if ( EV_IsLocal( idx ) )
	{
		g_iShotsFired++;

		// Add muzzle flash to current weapon model
		EV_MuzzleFlash();
		gEngfuncs.pEventAPI->EV_WeaponAnimation( AWP_SHOOT1 + gEngfuncs.pfnRandomLong(0,2), 2 );

		if ( gHUD.m_bGunSmoke == true )
		{
			EV_HLDM_CreateSmoke( ent->attachment[0], forward, 3, 0.5, 20, 20, 20, 3, velocity, false, 35 );
			EV_HLDM_CreateSmoke( ent->attachment[0], forward, 40, 0.5, 15, 15, 15, 2, velocity, false, 35 );
			EV_HLDM_CreateSmoke( ent->attachment[0], forward, 80, 0.45, 10, 10, 10, 2, velocity, false, 35 );
		}
	}

	gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "weapons/awp1.wav", 1, 0.28, 0, 94 + gEngfuncs.pfnRandomLong( 0, 0xf ) );

	EV_GetGunPosition( args, vecSrc, origin );
	VectorCopy( forward, vecAiming );

	Vector vSpread;
	vSpread = Vector( args->fparam1, args->fparam2, args->fparam1 );
	vSpread.z = 0;
	EV_HLDM_FireBullets( idx, forward, right, up, 1, vecSrc, vecAiming, vSpread, 8192, BULLET_PLAYER_338MAG, 0, &tracerCount[idx-1], 3 );
}

void EV_FireSG552( event_args_t *args )
{
	int idx;
	vec3_t origin;
	vec3_t angles;
	vec3_t velocity;
	vec3_t ShellVelocity;
	vec3_t ShellOrigin;
	int shell;
	vec3_t vecSrc, vecAiming;
	vec3_t up, right, forward;
	cl_entity_t *ent = GetViewEntity();

	int lefthand = cl_righthand->value == 0;

	idx = args->entindex;
	VectorCopy( args->origin, origin );
	VectorCopy( args->velocity, velocity );
	VectorCopy( args->angles, angles );

	angles[0] += args->iparam1 / 100.0;
	angles[1] += args->iparam2 / 100.0;

	AngleVectors( angles, forward, right, up );

	shell = gEngfuncs.pEventAPI->EV_FindModelIndex ("models/rshell.mdl");// brass shell
	
	if ( EV_IsLocal( idx ) )
	{
		g_iShotsFired++;

		// Add muzzle flash to current weapon model
		EV_MuzzleFlash();
		gEngfuncs.pEventAPI->EV_WeaponAnimation( SG552_SHOOT1 + gEngfuncs.pfnRandomLong(0,2), 2 );

		if ( gHUD.m_bGunSmoke == true )
			EV_HLDM_CreateSmoke( ent->attachment[0], forward, 3, 0.2, 18, 18, 18, 4, velocity, false, 35 );
	}

	if ( EV_IsLocal( idx ) )
	{
		if ( lefthand )
			EV_GetDefaultShellInfo( args, origin, velocity, ShellVelocity, ShellOrigin, forward, right, up, 20, -8, -10, false );
		else
			EV_GetDefaultShellInfo( args, origin, velocity, ShellVelocity, ShellOrigin, forward, right, up, 20, -8, 10, false );

		ShellVelocity = ShellVelocity * 1.65;
		ShellVelocity[2] -= 120;
		ShellOrigin = ent->attachment[1];
	}
	else
	{
		EV_GetDefaultShellInfo( args, origin, velocity, ShellVelocity, ShellOrigin, forward, right, up, 20, -12, 4, false );
	}

	EV_EjectBrass ( ShellOrigin, ShellVelocity, angles[ YAW ], shell, TE_BOUNCE_SHELL, idx, 15 );

	gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "weapons/sg552-1.wav", 1, 0.4, 0, 94 + gEngfuncs.pfnRandomLong( 0, 0xf ) );

	EV_GetGunPosition( args, vecSrc, origin );

	VectorCopy( forward, vecAiming );
	Vector vSpread;
	vSpread = Vector( args->fparam1, args->fparam2, args->fparam1 );
	vSpread.z = 0;
	EV_HLDM_FireBullets( idx, forward, right, up, 1, vecSrc, vecAiming, vSpread, 8192, BULLET_PLAYER_556MM, 0, &tracerCount[idx-1], 2 );
}

void EV_FireTMP( event_args_t *args )
{
	int idx;
	vec3_t origin;
	vec3_t angles;
	vec3_t velocity;
	vec3_t ShellVelocity;
	vec3_t ShellOrigin;
	int shell;
	vec3_t vecSrc, vecAiming;
	vec3_t up, right, forward;
	cl_entity_t *ent = GetViewEntity();

	int lefthand = cl_righthand->value == 0;

	idx = args->entindex;
	VectorCopy( args->origin, origin );
	VectorCopy( args->velocity, velocity );
	VectorCopy( args->angles, angles );

	angles[0] += args->iparam1 / 100.0;
	angles[1] += args->iparam2 / 100.0;

	AngleVectors( angles, forward, right, up );

	shell = gEngfuncs.pEventAPI->EV_FindModelIndex ("models/pshell.mdl");// brass shell
	
	if ( EV_IsLocal( idx ) )
	{
		g_iShotsFired++;

		// Add muzzle flash to current weapon model
		EV_MuzzleFlash();
		gEngfuncs.pEventAPI->EV_WeaponAnimation( TMP_SHOOT1 + gEngfuncs.pfnRandomLong(0,2), 2 );

		if ( gHUD.m_bGunSmoke == true )
			EV_HLDM_CreateSmoke( ent->attachment[0], forward, 3, 0.2, 10, 10, 10, 3, velocity, false, 35 );
	}

	if ( EV_IsLocal( idx ) )
	{
		if ( lefthand )
			EV_GetDefaultShellInfo( args, origin, velocity, ShellVelocity, ShellOrigin, forward, right, up, 32, -6, -11, false );
		else
			EV_GetDefaultShellInfo( args, origin, velocity, ShellVelocity, ShellOrigin, forward, right, up, 32, -6, 11, false );

		ShellVelocity = ShellVelocity * 1.2;
		ShellVelocity[2] -= 50;
		ShellOrigin = ent->attachment[1];
	}
	else
	{
		EV_GetDefaultShellInfo( args, origin, velocity, ShellVelocity, ShellOrigin, forward, right, up, 20, -12, 4, false );
	}

	EV_EjectBrass ( ShellOrigin, ShellVelocity, angles[ YAW ], shell, TE_BOUNCE_SHELL, idx, 8 );

	gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "weapons/tmp-1.wav", 1, 1.6, 0, 94 + gEngfuncs.pfnRandomLong( 0, 0xf ) );

	EV_GetGunPosition( args, vecSrc, origin );
	VectorCopy( forward, vecAiming );

	Vector vSpread;
	vSpread = Vector( args->fparam1, args->fparam2, args->fparam1 );
	vSpread.z = 0;
	EV_HLDM_FireBullets( idx, forward, right, up, 1, vecSrc, vecAiming, vSpread, 8192, BULLET_PLAYER_9MM, 0, &tracerCount[idx-1], 2 );
}

void EV_FireEliteLeft( event_args_t *args )
{
	int idx;
	vec3_t origin;
	vec3_t angles;
	vec3_t velocity;
	int bullets_left;
	float time_diff;	// the time since the last shot

	cl_entity_t *ent;
	vec3_t ShellVelocity;
	vec3_t ShellOrigin;
	int shell;
	vec3_t vecSrc, vecAiming;
	vec3_t up, right, forward;

	ent = GetViewEntity();
	time_diff = args->fparam1;
	bullets_left = args->iparam2;
	idx = args->entindex;
	VectorCopy( args->origin, origin );
	VectorCopy( args->velocity, velocity );
	VectorCopy( args->angles, angles );

	AngleVectors( angles, forward, right, up );

	shell = gEngfuncs.pEventAPI->EV_FindModelIndex ("models/pshell.mdl");// brass shell
	
	if ( EV_IsLocal( idx ) )
	{
		g_iShotsFired++;

		// Add muzzle flash to current weapon model
		EV_MuzzleFlash();
		if ( bullets_left <= 1 )
			gEngfuncs.pEventAPI->EV_WeaponAnimation( ELITE_SHOOTLEFTLAST, 2 );
		else if ( time_diff >= 0.5 )
			gEngfuncs.pEventAPI->EV_WeaponAnimation( ELITE_SHOOTLEFT5, 2 );
		else if ( time_diff >= 0.4 )
			gEngfuncs.pEventAPI->EV_WeaponAnimation( ELITE_SHOOTLEFT4, 2 );
		else if ( time_diff >= 0.3 )
			gEngfuncs.pEventAPI->EV_WeaponAnimation( ELITE_SHOOTLEFT3, 2 );
		else if ( time_diff >= 0.2 )
			gEngfuncs.pEventAPI->EV_WeaponAnimation( ELITE_SHOOTLEFT2, 2 );
		else if ( time_diff >= 0.0 )
			gEngfuncs.pEventAPI->EV_WeaponAnimation( ELITE_SHOOTLEFT1, 2 );

		if ( gHUD.m_bGunSmoke == true )
		{
			EV_HLDM_CreateSmoke( ent->attachment[0], forward, 0, 0.25, 10, 10, 10, 3, velocity, false, 35 );
			EV_HLDM_CreateSmoke( ent->attachment[0], forward, 25, 0.3, 15, 15, 15, 2, velocity, false, 35 );
			EV_HLDM_CreateSmoke( ent->attachment[0], forward, 50, 0.2, 25, 25, 25, 2, velocity, false, 35 );
		}
	}

	if ( EV_IsLocal( idx ) )
	{
		EV_GetDefaultShellInfo( args, origin, velocity, ShellVelocity, ShellOrigin, forward, right, up, 35, -11, -16, false );

		ShellVelocity = ShellVelocity * 0.75;
		ShellVelocity[2] += 25;
		ShellOrigin = ent->attachment[2];
	}
	else
	{
		EV_GetDefaultShellInfo( args, origin, velocity, ShellVelocity, ShellOrigin, forward, right, up, 20, -12, 4, false );
	}

	EV_EjectBrass ( ShellOrigin, ShellVelocity, angles[ YAW ], shell, TE_BOUNCE_SHELL, idx, 5 );

	gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "weapons/elite_fire.wav", 1, ATTN_NORM, 0, 94 + gEngfuncs.pfnRandomLong( 0, 0xf ) );

	EV_GetGunPosition( args, vecSrc, origin );
	vecSrc = vecSrc - right * 5;
	VectorCopy( forward, vecAiming );

	Vector vSpread;
	vSpread = Vector( args->fparam2, (float)( args->iparam1 / 100.0 ), args->fparam2 );
	vSpread.z = 0;
	EV_HLDM_FireBullets( idx, forward, right, up, 1, vecSrc, vecAiming, vSpread, 8192, BULLET_PLAYER_50AE, 0, &tracerCount[idx-1], 2 );
}

void EV_FireEliteRight( event_args_t *args )
{
	int idx;
	vec3_t origin;
	vec3_t angles;
	vec3_t velocity;
	int bullets_left;
	float time_diff;	// the time since the last shot

	cl_entity_t *ent;
	vec3_t ShellVelocity;
	vec3_t ShellOrigin;
	int shell;
	vec3_t vecSrc, vecAiming;
	vec3_t up, right, forward;

	ent = GetViewEntity();
	time_diff = args->fparam1;
	bullets_left = args->iparam2;
	idx = args->entindex;
	VectorCopy( args->origin, origin );
	VectorCopy( args->velocity, velocity );
	VectorCopy( args->angles, angles );

	AngleVectors( angles, forward, right, up );

	shell = gEngfuncs.pEventAPI->EV_FindModelIndex ("models/pshell.mdl");// brass shell
	
	if ( EV_IsLocal( idx ) )
	{
		g_iShotsFired++;

		// Add muzzle flash to current weapon model
		EV_MuzzleFlash();
		if ( bullets_left <= 1 )
			gEngfuncs.pEventAPI->EV_WeaponAnimation( ELITE_SHOOTRIGHTLAST, 2 );
		else if ( time_diff >= 0.5 )
			gEngfuncs.pEventAPI->EV_WeaponAnimation( ELITE_SHOOTRIGHT5, 2 );
		else if ( time_diff >= 0.4 )
			gEngfuncs.pEventAPI->EV_WeaponAnimation( ELITE_SHOOTRIGHT4, 2 );
		else if ( time_diff >= 0.3 )
			gEngfuncs.pEventAPI->EV_WeaponAnimation( ELITE_SHOOTRIGHT3, 2 );
		else if ( time_diff >= 0.2 )
			gEngfuncs.pEventAPI->EV_WeaponAnimation( ELITE_SHOOTRIGHT2, 2 );
		else if ( time_diff >= 0.0 )
			gEngfuncs.pEventAPI->EV_WeaponAnimation( ELITE_SHOOTRIGHT1, 2 );

		if ( gHUD.m_bGunSmoke == true )
		{
			EV_HLDM_CreateSmoke( ent->attachment[1], forward, 0, 0.25, 10, 10, 10, 3, velocity, false, 35 );
			EV_HLDM_CreateSmoke( ent->attachment[1], forward, 25, 0.3, 15, 15, 15, 2, velocity, false, 35 );
			EV_HLDM_CreateSmoke( ent->attachment[1], forward, 50, 0.2, 25, 25, 25, 2, velocity, false, 35 );
		}
	}

	if ( EV_IsLocal( idx ) )
	{
		EV_GetDefaultShellInfo( args, origin, velocity, ShellVelocity, ShellOrigin, forward, right, up, 35, -11, 16, false );

		ShellVelocity = ShellVelocity * 0.75;
		ShellVelocity[2] += 25;
		ShellOrigin = ent->attachment[3];
	}
	else
	{
		EV_GetDefaultShellInfo( args, origin, velocity, ShellVelocity, ShellOrigin, forward, right, up, 20, -12, 4, false );
	}

	EV_EjectBrass ( ShellOrigin, ShellVelocity, angles[ YAW ], shell, TE_BOUNCE_SHELL, idx, 5 );

	gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "weapons/elite_fire.wav", 1, ATTN_NORM, 0, 94 + gEngfuncs.pfnRandomLong( 0, 0xf ) );

	EV_GetGunPosition( args, vecSrc, origin );
	vecSrc = vecSrc + right * 5;
	VectorCopy( forward, vecAiming );

	Vector vSpread;
	vSpread = Vector( args->fparam2, (float)( args->iparam1 / 100.0 ), args->fparam2 );
	EV_HLDM_FireBullets( idx, forward, right, up, 1, vecSrc, vecAiming, vSpread, 8192, BULLET_PLAYER_50AE, 0, &tracerCount[idx-1], 2 );
}

void EV_Knife( event_args_t *args )
{
	int idx;

	idx = args->entindex;

	if ( EV_IsLocal( idx ) )
		gEngfuncs.pEventAPI->EV_WeaponAnimation( args->iparam1, 2 );
}

void EV_FireFiveSeven( event_args_t *args )
{
	int idx;
	vec3_t origin;
	vec3_t angles;
	vec3_t velocity;
	int empty;
	vec3_t ShellVelocity;
	vec3_t ShellOrigin;
	int shell;
	vec3_t vecSrc, vecAiming;
	vec3_t up, right, forward;
	cl_entity_t *ent = GetViewEntity();

	int lefthand = cl_righthand->value == 0;

	empty = args->bparam1;
	idx = args->entindex;
	VectorCopy( args->origin, origin );
	VectorCopy( args->velocity, velocity );
	VectorCopy( args->angles, angles );

	AngleVectors( angles, forward, right, up );

	angles[0] += args->iparam1 / 100.0;
	angles[1] += args->iparam2 / 100.0;

	shell = gEngfuncs.pEventAPI->EV_FindModelIndex ("models/pshell.mdl");// brass shell
	
	if ( EV_IsLocal( idx ) )
	{
		g_iShotsFired++;

		// Add muzzle flash to current weapon model
		EV_MuzzleFlash();
		if ( empty )
			gEngfuncs.pEventAPI->EV_WeaponAnimation( FIVESEVEN_SHOOT_EMPTY, 2 );
		else
			gEngfuncs.pEventAPI->EV_WeaponAnimation( FIVESEVEN_SHOOT1 + gEngfuncs.pfnRandomLong(0,1), 2 );

		if ( gHUD.m_bGunSmoke == true )
		{
			EV_HLDM_CreateSmoke( ent->attachment[0], forward, 0, 0.25, 10, 10, 10, 3, velocity, false, 35 );
			EV_HLDM_CreateSmoke( ent->attachment[0], forward, 25, 0.3, 15, 15, 15, 2, velocity, false, 35 );
			EV_HLDM_CreateSmoke( ent->attachment[0], forward, 50, 0.2, 20, 20, 20, 2, velocity, false, 35 );
		}
	}

	if ( EV_IsLocal( idx ) )
	{
		if ( lefthand )
			EV_GetDefaultShellInfo( args, origin, velocity, ShellVelocity, ShellOrigin, forward, right, up, 36, -14, -14, false );
		else
			EV_GetDefaultShellInfo( args, origin, velocity, ShellVelocity, ShellOrigin, forward, right, up, 36, -14, 14, false );

		ShellVelocity = ShellVelocity * 0.75;
		ShellVelocity[2] += 35;
		ShellOrigin = ent->attachment[1];
	}
	else
	{
		EV_GetDefaultShellInfo( args, origin, velocity, ShellVelocity, ShellOrigin, forward, right, up, 20, -12, 4, false );
	}

	EV_EjectBrass ( ShellOrigin, ShellVelocity, angles[ YAW ], shell, TE_BOUNCE_SHELL, idx, 4 );

	gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "weapons/fiveseven-1.wav", 1, 0.8, 0, 94 + gEngfuncs.pfnRandomLong( 0, 0xf ) );

	EV_GetGunPosition( args, vecSrc, origin );
	VectorCopy( forward, vecAiming );

	Vector vSpread;
	vSpread = Vector( args->fparam1, args->fparam2, args->fparam1 );
	vSpread.z = 0;
	EV_HLDM_FireBullets( idx, forward, right, up, 1, vecSrc, vecAiming, vSpread, 8192, BULLET_PLAYER_57MM, 0, &tracerCount[idx-1], 2 );
}

void EV_FireUMP45( event_args_t *args )
{
	int idx;
	vec3_t origin;
	vec3_t angles;
	vec3_t velocity;
	vec3_t ShellVelocity;
	vec3_t ShellOrigin;
	int shell;
	vec3_t vecSrc, vecAiming;
	vec3_t up, right, forward;
	cl_entity_t *ent = GetViewEntity();

	int lefthand = cl_righthand->value == 0;

	idx = args->entindex;
	VectorCopy( args->origin, origin );
	VectorCopy( args->velocity, velocity );
	VectorCopy( args->angles, angles );

	angles[0] += args->iparam1 / 100.0;
	angles[1] += args->iparam2 / 100.0;

	AngleVectors( angles, forward, right, up );

	shell = gEngfuncs.pEventAPI->EV_FindModelIndex ("models/pshell.mdl");// brass shell
	
	if ( EV_IsLocal( idx ) )
	{
		g_iShotsFired++;

		// Add muzzle flash to current weapon model
		EV_MuzzleFlash();
		gEngfuncs.pEventAPI->EV_WeaponAnimation( UMP45_SHOOT1 + gEngfuncs.pfnRandomLong(0,2), 2 );

		if ( gHUD.m_bGunSmoke == true )
			EV_HLDM_CreateSmoke( ent->attachment[0], forward, 3, 0.25, 10, 10, 10, 3, velocity, false, 35 );
	}

	if ( EV_IsLocal( idx ) )
	{
		if ( lefthand )
			EV_GetDefaultShellInfo( args, origin, velocity, ShellVelocity, ShellOrigin, forward, right, up, 34, -10, -11, false );
		else
			EV_GetDefaultShellInfo( args, origin, velocity, ShellVelocity, ShellOrigin, forward, right, up, 34, -10, 11, false );

		ShellVelocity[2] -= 50;
		ShellOrigin = ent->attachment[1];
	}
	else
	{
		EV_GetDefaultShellInfo( args, origin, velocity, ShellVelocity, ShellOrigin, forward, right, up, 20, -12, 4, false );
	}

	EV_EjectBrass ( ShellOrigin, ShellVelocity, angles[ YAW ], shell, TE_BOUNCE_SHELL, idx, 13 );

	gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "weapons/ump45-1.wav", 1, 0.64, 0, 94 + gEngfuncs.pfnRandomLong( 0, 0xf ) );

	EV_GetGunPosition( args, vecSrc, origin );
	VectorCopy( forward, vecAiming );

	Vector vSpread;
	vSpread = Vector( args->fparam1, args->fparam2, args->fparam1 );
	vSpread.z = 0;
	EV_HLDM_FireBullets( idx, forward, right, up, 1, vecSrc, vecAiming, vSpread, 8192, BULLET_PLAYER_45ACP, 0, &tracerCount[idx-1], 2 );
}

void EV_FireSG550( event_args_t *args )
{
	int idx;
	vec3_t origin;
	vec3_t angles;
	vec3_t velocity;
	vec3_t ShellVelocity;
	vec3_t ShellOrigin;
	int shell;
	vec3_t vecSrc, vecAiming;
	vec3_t up, right, forward;
	cl_entity_t *ent = GetViewEntity();

	int lefthand = cl_righthand->value == 0;

	idx = args->entindex;
	VectorCopy( args->origin, origin );
	VectorCopy( args->velocity, velocity );
	VectorCopy( args->angles, angles );

	AngleVectors( angles, forward, right, up );

	angles[0] += args->iparam1 / 100.0;
	angles[1] += args->iparam2 / 100.0;

	shell = gEngfuncs.pEventAPI->EV_FindModelIndex ("models/rshell.mdl");// brass shell
	
	if ( EV_IsLocal( idx ) )
	{
		g_iShotsFired++;

		// Add muzzle flash to current weapon model
		EV_MuzzleFlash();
		gEngfuncs.pEventAPI->EV_WeaponAnimation( SG550_SHOOT1 + gEngfuncs.pfnRandomLong(0,1), 2 );

		if ( gHUD.m_bGunSmoke == true )
		{
			EV_HLDM_CreateSmoke( ent->attachment[0], forward, 3, 0.3, 35, 35, 35, 4, velocity, false, 35 );
			EV_HLDM_CreateSmoke( ent->attachment[0], forward, 35, 0.35, 30, 30, 30, 2, velocity, false, 35 );
			EV_HLDM_CreateSmoke( ent->attachment[0], forward, 70, 0.3, 25, 25, 25, 2, velocity, false, 35 );
		}
	}

	if ( EV_IsLocal( idx ) )
	{
		if ( lefthand )
			EV_GetDefaultShellInfo( args, origin, velocity, ShellVelocity, ShellOrigin, forward, right, up, 20, -8, -10, false );
		else
			EV_GetDefaultShellInfo( args, origin, velocity, ShellVelocity, ShellOrigin, forward, right, up, 17, -8, 10, false );

		ShellVelocity = ShellVelocity * 1.5;
		ShellVelocity[2] -= 50;
		ShellOrigin = ent->attachment[1];
	}
	else
	{
		EV_GetDefaultShellInfo( args, origin, velocity, ShellVelocity, ShellOrigin, forward, right, up, 20, -12, 4, false );
	}

	EV_EjectBrass ( ShellOrigin, ShellVelocity, angles[ YAW ], shell, TE_BOUNCE_SHELL, idx, 17 );

	gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "weapons/sg550-1.wav", 1, 0.4, 0, 94 + gEngfuncs.pfnRandomLong( 0, 0xf ) );

	EV_GetGunPosition( args, vecSrc, origin );
	VectorCopy( forward, vecAiming );

	Vector vSpread;
	vSpread = Vector( args->fparam1, args->fparam2, args->fparam1 );
	EV_HLDM_FireBullets( idx, forward, right, up, 1, vecSrc, vecAiming, vSpread, 8192, BULLET_PLAYER_556MM, 0, &tracerCount[idx-1], 2 );
}

void EV_CreateSmoke( event_args_t *args )
{
	vec3_t origin;
	vec3_t velocity;
	int colour;
	double flAngle = 15 * ( 3.141517 / 180 );	// name ours: 15 degrees, pi as 3.141517; this grouping gives the image's constant (15 * 3.141517 / 180 differs in the last bit)

	VectorCopy( args->origin, origin );
	velocity[0] = args->fparam1;
	velocity[1] = args->fparam2;
	velocity[2] = args->iparam1 / 100.0;

	colour = gEngfuncs.pfnRandomLong( 85, 100 );

	if ( args->bparam1 )
		EV_HLDM_CreateSmoke( origin, velocity, gEngfuncs.pfnRandomFloat( 5, 10 ), 2.0, colour, colour, colour, 1, NULL, true, 4 );
	else
		EV_HLDM_CreateSmoke( origin, velocity, gEngfuncs.pfnRandomFloat( 5, 10 ), 2.0, colour, colour, colour, 0, NULL, true, 4 );

	float old_z = velocity[2];
	Vector vecTemp( velocity[0] * cos( flAngle ) - velocity[1] * sin( flAngle ), velocity[0] * sin( flAngle ) + velocity[1] * cos( flAngle ), 0.0 );	// name ours
	Vector vecRotated;		// name ours
	float *pflRotated = vecRotated;	// sic: name ours; the one store through this pointer stops VC6 reusing the rotated x it holds in a register, which gives the image's frame
	vecRotated.x = vecTemp.x;
	vecRotated.y = vecTemp.y;
	pflRotated[2] = 0;		// sic
	velocity = vecRotated;
	velocity[0] = velocity[0] * 0.75;
	velocity[1] = velocity[1] * 0.75;
	velocity[2] = old_z * 0.75;
	EV_HLDM_CreateSmoke( origin, velocity, gEngfuncs.pfnRandomFloat( 2, 5 ), 1.2, colour, colour, colour, 1, NULL, true, 4 );
}

// Counter-Strike 1.4: the bodies of dead players (temp entities made by CreateCorpse)
TEMPENTITY *g_DeadPlayerModels[ 64 ];

void EV_RemoveAllDecals( event_args_t *args )
{
	for ( int har = 0; har < (int)CVAR_GET_FLOAT( "r_decals" ); har++ )
		gEngfuncs.pEfxAPI->R_DecalRemoveAll( har );

	if ( g_DeadPlayerModels[ 0 ] )
	{
		for ( int i = 0; i < 64; i++ )
		{
			if ( g_DeadPlayerModels[ i ] )
			{
				g_DeadPlayerModels[ i ]->die = 0;
				g_DeadPlayerModels[ i ] = NULL;
			}
		}
	}
}

// a body sinks into the floor once it has lain for cl_corpsestay seconds
void RemoveBody( TEMPENTITY *te, float frametime, float current_time )
{
	if ( current_time >= CVAR_GET_FLOAT( "cl_corpsestay" ) + te->entity.curstate.fuser2 + 5 )
		te->entity.origin.z -= frametime * 5;
}

void HitBody( TEMPENTITY *ent, pmtrace_s *ptr )
{
	if ( ptr->plane.normal.z > 0 )
		ent->flags |= FTENT_CORPSELANDED;
}

void CreateCorpse( Vector vOrigin, Vector vAngles, const char *pModel, float flAnimTime, int iSequence, int iBody )
{
	int iModelIndex = gEngfuncs.pEventAPI->EV_FindModelIndex( pModel );
	TEMPENTITY *pBody = gEngfuncs.pEfxAPI->R_TempModel( vOrigin, Vector( 0, 0, 0 ), vAngles, 100, gEngfuncs.pEventAPI->EV_FindModelIndex( pModel ), 0 );

	if ( pBody == NULL )
		return;

	pBody->flags |= FTENT_COLLIDEWORLD | FTENT_SPRANIMATE | FTENT_PERSIST | FTENT_CLIENTCUSTOM | FTENT_CORPSE;
	pBody->frameMax = 255;

	pBody->entity.curstate.animtime = flAnimTime;
	pBody->entity.curstate.framerate = 1;
	pBody->entity.curstate.sequence = iSequence;
	pBody->entity.curstate.frame = 0;
	pBody->entity.curstate.body = iBody;
	pBody->entity.curstate.renderamt = 255;
	pBody->entity.curstate.fuser1 = gHUD.m_flTime + 1;
	pBody->entity.curstate.fuser2 = CVAR_GET_FLOAT( "cl_corpsestay" ) + gHUD.m_flTime;

	pBody->die = gEngfuncs.GetClientTime() + pBody->entity.curstate.fuser2 + 5;
	pBody->callback = RemoveBody;
	pBody->hitcallback = HitBody;
	pBody->bounceFactor = 0;

	for ( int i = 0; i < 64; i++ )
	{
		if ( g_DeadPlayerModels[ i ] == NULL )
		{
			g_DeadPlayerModels[ i ] = pBody;
			break;
		}
	}
}

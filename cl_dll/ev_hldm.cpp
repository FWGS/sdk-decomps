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
#include "effectlist.h"

#include <string.h>

static int tracerCount[ 32 ];

extern "C" char PM_FindTextureType( char *name );

void V_PunchAxis( int axis, float punch );
void VectorAngles( const float *forward, float *angles );

extern "C"
{

// HLDM
void EV_FireGlock1( struct event_args_s *args  );
void EV_FireGlock2( struct event_args_s *args  );
void EV_FireShotGunSingle( struct event_args_s *args  );
void EV_FireMP5( struct event_args_s *args  );
void EV_FirePython( struct event_args_s *args  );
void EV_FireGauss( struct event_args_s *args  );
void EV_SpinGauss( struct event_args_s *args  );
void EV_TrainPitchAdjust( struct event_args_s *args );

// Gunman
void EV_FireGWShotgun( struct event_args_s *args );
void EV_TankMGuns( struct event_args_s *args );
void EV_TurretBullets( struct event_args_s *args );
void EV_MonsterSentry( struct event_args_s *args );
void EV_MonsterShotgun( struct event_args_s *args );
void EV_MonsterMechagun( struct event_args_s *args );
void EV_ChopperFire( struct event_args_s *args );
void EV_FireGWMinigun( struct event_args_s *args );
void EV_MinigunHeat( struct event_args_s *args );
void EV_Cleanup( struct event_args_s *args );
void EV_BatbotBeam( struct event_args_s *args );
void EV_RustbitImpact( struct event_args_s *args );
void EV_BallLightning( struct event_args_s *args );
void EV_KataImpact( struct event_args_s *args );
void EV_SniperShot( struct event_args_s *args );
void EV_ChargedImpact( struct event_args_s *args );
void EV_SpurtBlood( struct event_args_s *args );
void EV_RapidImpact( struct event_args_s *args );
void EV_ChemicalSpray( struct event_args_s *args );
void EV_ClusterExplosion( struct event_args_s *args );
void EV_SphereExplode( struct event_args_s *args );
void EV_Lightning( struct event_args_s *args );
void EV_RocketSmoke( struct event_args_s *args );
void EV_Cylinder( struct event_args_s *args );
void EV_VolumeExplosion( struct event_args_s *args );
void EV_WaterRipple( struct event_args_s *args );
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
// returns volume of strike instrument (crowbar) to play
float EV_HLDM_PlayTextureSound( int idx, pmtrace_t *ptr, float *vecSrc, float *vecEnd, int iBulletType )
{
	// hit the world, try to play sound based on texture material type
	char chTextureType = CHAR_TEX_CONCRETE;
	float fvol;
	float fvolbar;
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
	case CHAR_TEX_CONCRETE: fvol = 0.9;	fvolbar = 0.6;
		rgsz[0] = "player/pl_step1.wav";
		rgsz[1] = "player/pl_step2.wav";
		cnt = 2;
		break;
	case CHAR_TEX_METAL: fvol = 0.9; fvolbar = 0.3;
		rgsz[0] = "player/pl_metal1.wav";
		rgsz[1] = "player/pl_metal2.wav";
		cnt = 2;
		break;
	case CHAR_TEX_DIRT:	fvol = 0.9; fvolbar = 0.1;
		rgsz[0] = "player/pl_dirt1.wav";
		rgsz[1] = "player/pl_dirt2.wav";
		rgsz[2] = "player/pl_dirt3.wav";
		cnt = 3;
		break;
	case CHAR_TEX_VENT:	fvol = 0.5; fvolbar = 0.3;
		rgsz[0] = "player/pl_duct1.wav";
		rgsz[1] = "player/pl_duct1.wav";
		cnt = 2;
		break;
	case CHAR_TEX_GRATE: fvol = 0.9; fvolbar = 0.5;
		rgsz[0] = "player/pl_grate1.wav";
		rgsz[1] = "player/pl_grate4.wav";
		cnt = 2;
		break;
	case CHAR_TEX_TILE:	fvol = 0.8; fvolbar = 0.2;
		rgsz[0] = "player/pl_tile1.wav";
		rgsz[1] = "player/pl_tile3.wav";
		rgsz[2] = "player/pl_tile2.wav";
		rgsz[3] = "player/pl_tile4.wav";
		cnt = 4;
		break;
	case CHAR_TEX_SLOSH: fvol = 0.9; fvolbar = 0.0;
		rgsz[0] = "player/pl_slosh1.wav";
		rgsz[1] = "player/pl_slosh3.wav";
		rgsz[2] = "player/pl_slosh2.wav";
		rgsz[3] = "player/pl_slosh4.wav";
		cnt = 4;
		break;
	case CHAR_TEX_WOOD: fvol = 0.9; fvolbar = 0.2;
		rgsz[0] = "debris/wood1.wav";
		rgsz[1] = "debris/wood2.wav";
		rgsz[2] = "debris/wood3.wav";
		cnt = 3;
		break;
	case CHAR_TEX_GLASS:
	case CHAR_TEX_COMPUTER:
		fvol = 0.8; fvolbar = 0.2;
		rgsz[0] = "debris/glass1.wav";
		rgsz[1] = "debris/glass2.wav";
		rgsz[2] = "debris/glass3.wav";
		cnt = 3;
		break;
	case CHAR_TEX_FLESH:
		if (iBulletType == BULLET_PLAYER_CROWBAR)
			return 0.0; // crowbar already makes this sound
		fvol = 1.0;	fvolbar = 0.2;
		rgsz[0] = "weapons/bullet_hit1.wav";
		rgsz[1] = "weapons/bullet_hit2.wav";
		fattn = 1.0;
		cnt = 2;
		break;
	}

	// play material hit sound
	gEngfuncs.pEventAPI->EV_PlaySound( 0, ptr->endpos, CHAN_STATIC, rgsz[gEngfuncs.pfnRandomLong(0,cnt-1)], fvol, fattn, 0, 96 + gEngfuncs.pfnRandomLong(0,0xf) );
	return fvolbar;
}

// show an effect for the texture the shot hit. Returns true if the shot
// hit a force field, which stops it
bool EV_HLDM_TextureEffect( int idx, pmtrace_t *ptr, float *vecSrc, float *vecEnd, int iBulletType )
{
	char chTextureType;
	int entity;
	char *pTextureName;
	char texname[ 64 ];
	char szbuffer[ 64 ];

	entity = gEngfuncs.pEventAPI->EV_IndexFromTrace( ptr );

	// no effect on players
	if ( entity >= 1 && entity <= gEngfuncs.GetMaxClients() )
	{
	}
	else
	{
		pTextureName = (char *)gEngfuncs.pEventAPI->EV_TraceTexture( ptr->ent, vecSrc, vecEnd );
		if ( !pTextureName )
			return true;

		strcpy( texname, pTextureName );
		pTextureName = texname;

		if ( *pTextureName != '-' && *pTextureName != '+' )
		{
			if ( *pTextureName == '{' || *pTextureName == '!' || *pTextureName == '~' || *pTextureName == ' ' )
			{
				pTextureName++;
			}

			strcpy( szbuffer, pTextureName );
			szbuffer[ CBTEXTURENAMEMAX - 1 ] = 0;

			chTextureType = PM_FindTextureType( szbuffer );

			if ( chTextureType == CHAR_TEX_METAL )
			{
				for ( int i = 0; i < 3; i++ )
					gEngfuncs.pEfxAPI->R_SparkShower( ptr->endpos );

				g_Effects.Add( CEffect( 6, ptr->endpos, ptr->plane.normal, 0.4, EFFECT_FLAME, 200, 20, gEngfuncs.GetClientTime(), (struct model_s *)gEngfuncs.GetSpritePointer( SPR_Load( "sprites/flambeam.spr" ) ) ) );
				return false;
			}

			if ( chTextureType == CHAR_TEX_CONCRETE )
			{
				g_Effects.Add( CEffect( 3, ptr->endpos, ptr->plane.normal, 0.4, EFFECT_FLAME, 200, 20, gEngfuncs.GetClientTime(), (struct model_s *)gEngfuncs.GetSpritePointer( SPR_Load( "sprites/flambeam.spr" ) ) ) );
				gEngfuncs.pEfxAPI->R_BulletImpactParticles( ptr->endpos );
				return false;
			}

			if ( chTextureType == CHAR_TEX_GRATE )
			{
				vec3_t size;
				size[2] = 1;
				size[1] = 1;
				size[0] = 1;
				gEngfuncs.pEfxAPI->R_BreakModel( ptr->endpos, size, ptr->plane.normal, 100, 1, 1, gEngfuncs.pEventAPI->EV_FindModelIndex( "models/smallwoodgib.mdl" ), 8 );
				return false;
			}

			if ( chTextureType == CHAR_TEX_VENT || chTextureType == CHAR_TEX_TILE )
			{
				vec3_t angles, forward, right, up;
				vec3_t vecVelocity, vecOrigin;
				int iModel;
				TEMPENTITY *pTent;

				VectorAngles( ptr->plane.normal, angles );
				AngleVectors( angles, forward, right, up );
				forward.z = forward.z * -1;

				iModel = gEngfuncs.pEventAPI->EV_FindModelIndex( "sprites/wallpuff.spr" );
				vecVelocity = forward * gEngfuncs.pfnRandomFloat( 10, 35 ) + up * gEngfuncs.pfnRandomFloat( -15, 15 ) + right * gEngfuncs.pfnRandomFloat( -15, 15 );
				pTent = gEngfuncs.pEfxAPI->R_TempSprite( ptr->endpos + ptr->plane.normal * 3, vecVelocity, 0.5, iModel, kRenderTransAdd, kRenderFxNone, 50, 10, FTENT_SPRANIMATE );
				if ( pTent )
				{
					pTent->entity.curstate.framerate = 20;
					pTent->entity.curstate.renderamt = 70;
				}

				// a puff of dust off the floor
				if ( ptr->plane.normal.z > 0.5 )
				{
					vecOrigin = Vector( ptr->endpos.x, ptr->endpos.y, ptr->endpos.z + 10 );

					pTent = gEngfuncs.pEfxAPI->R_TempSprite( vecOrigin, Vector( 0, 0, 0 ), 0.75, gEngfuncs.pEventAPI->EV_FindModelIndex( "sprites/bulletpuff.spr" ), kRenderTransAlpha, kRenderFxNoDissipation, 255, 10, FTENT_SPRANIMATE );
					if ( pTent )
						pTent->entity.curstate.framerate = 40;
				}
			}

		}
		else
		{
			// a force field
			if ( !strcmp( pTextureName + 2, "ff" ) )
			{
				g_Effects.Add( CEffect( 20, ptr->endpos, ptr->plane.normal, 1, EFFECT_SPRITE, 0, 10, 20, 10, 0, gEngfuncs.GetClientTime(), (struct model_s *)gEngfuncs.GetSpritePointer( SPR_Load( "sprites/ripple.spr" ) ) ) );
			}

			gEngfuncs.pEventAPI->EV_PlaySound( -1, ptr->endpos, CHAN_AUTO, "weapons/forcefield.wav", 1.0, ATTN_NORM, 0, PITCH_NORM );
			return true;
		}
	}

	return false;
}

// blood for a shot that hit a player (does nothing)
void EV_HLDM_PlayerBlood( float flDamage, Vector vecDir, Vector vecOrigin )
{
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
	else if ( pe->rendermode == kRenderTransAlpha )
	{
		return NULL;
	}
	else if ( pe->rendermode != kRenderNormal )
	{
		idx = gEngfuncs.pfnRandomLong( 0, 2 );
		sprintf( decalname, "{break%i", idx + 1 );
	}
	else
	{
		idx = gEngfuncs.pfnRandomLong( 0, 4 );
		sprintf( decalname, "{shot%i", idx + 1 );
	}
	return decalname;
}

void EV_HLDM_GunshotDecalTrace( pmtrace_t *pTrace, char *decalName )
{
	int iRand;
	physent_t *pe;

	iRand = gEngfuncs.pfnRandomLong(0,0x7FFF);
	if ( iRand < (0x7fff/2) )// not every bullet makes a sound.
	{
		switch( iRand % 5)
		{
		case 0:	gEngfuncs.pEventAPI->EV_PlaySound( -1, pTrace->endpos, 0, "weapons/ric1.wav", 1.0, ATTN_NORM, 0, PITCH_NORM ); break;
		case 1:	gEngfuncs.pEventAPI->EV_PlaySound( -1, pTrace->endpos, 0, "weapons/ric2.wav", 1.0, ATTN_NORM, 0, PITCH_NORM ); break;
		case 2:	gEngfuncs.pEventAPI->EV_PlaySound( -1, pTrace->endpos, 0, "weapons/ric3.wav", 1.0, ATTN_NORM, 0, PITCH_NORM ); break;
		case 3:	gEngfuncs.pEventAPI->EV_PlaySound( -1, pTrace->endpos, 0, "weapons/ric4.wav", 1.0, ATTN_NORM, 0, PITCH_NORM ); break;
		case 4:	gEngfuncs.pEventAPI->EV_PlaySound( -1, pTrace->endpos, 0, "weapons/ric5.wav", 1.0, ATTN_NORM, 0, PITCH_NORM ); break;
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

void EV_HLDM_DecalGunshot( pmtrace_t *pTrace, int iBulletType )
{
	physent_t *pe;

	pe = gEngfuncs.pEventAPI->EV_GetPhysent( pTrace->ent );

	if ( pe && pe->solid == SOLID_BSP )
	{
		switch( iBulletType )
		{
		case BULLET_PLAYER_9MM:
		case BULLET_MONSTER_9MM:
		case BULLET_PLAYER_MP5:
		case BULLET_MONSTER_MP5:
		case BULLET_PLAYER_BUCKSHOT:
		case BULLET_PLAYER_357:
		default:
			// smoke and decal
			EV_HLDM_GunshotDecalTrace( pTrace, EV_HLDM_DamageDecal( pe ) );
			break;
		}
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

/*
================
FireBullets

Go to the trouble of combining multiple pellets into a single damage call.
================
*/
void EV_HLDM_FireBullets( int idx, float *forward, float *right, float *up, int cShots, float *vecSrc, float *vecDirShooting, float *vecSpread, float flDistance, int iBulletType, int iTracerFreq, int *tracerCount )
{
	int i;
	pmtrace_t tr;
	int iShot;
	vec3_t vecRight, vecUp;
	int tracer;
	physent_t *pe;
	bool bHitPlayer = false;
	bool bForceField;

	VectorCopy( right, vecRight );
	VectorCopy( up, vecUp );

	for ( iShot = 1; iShot <= cShots; iShot++ )	
	{
		vec3_t vecDir, vecEnd;
			
		// get circular gaussian spread
		vec3_t spread;
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

		gEngfuncs.pEventAPI->EV_SetUpPlayerPrediction( false, true );
	
		// Store off the old count
		gEngfuncs.pEventAPI->EV_PushPMStates();
	
		// Now add in all of the players.
		gEngfuncs.pEventAPI->EV_SetSolidPlayers ( idx - 1 );	

		gEngfuncs.pEventAPI->EV_SetTraceHull( 2 );
		gEngfuncs.pEventAPI->EV_PlayerTrace( vecSrc, vecEnd, PM_STUDIO_BOX, -1, &tr );

		tracer = EV_HLDM_CheckTracer( idx, vecSrc, tr.endpos, forward, right, iBulletType, iTracerFreq, tracerCount );

		pe = gEngfuncs.pEventAPI->EV_GetPhysent( tr.ent );
		if ( pe && pe->takedamage && pe->player )
			bHitPlayer = true;

		// do damage, paint decals
		if ( tr.fraction != 1.0 )
		{
			switch(iBulletType)
			{
			case BULLET_PLAYER_MP5:		
				if ( !tracer )
				{
					EV_HLDM_PlayTextureSound( idx, &tr, vecSrc, vecEnd, iBulletType );
					if ( bHitPlayer )
						EV_HLDM_PlayerBlood( 15, vecDirShooting, tr.endpos );
					if ( !EV_HLDM_TextureEffect( idx, &tr, vecSrc, vecEnd, iBulletType ) )
						EV_HLDM_DecalGunshot( &tr, iBulletType );
				}
				break;
			case BULLET_PLAYER_357:
				if ( !tracer )
				{
					if ( bHitPlayer )
						EV_HLDM_PlayerBlood( 12, vecDirShooting, tr.endpos );
					EV_HLDM_PlayTextureSound( idx, &tr, vecSrc, vecEnd, iBulletType );
					if ( !EV_HLDM_TextureEffect( idx, &tr, vecSrc, vecEnd, iBulletType ) )
						EV_HLDM_DecalGunshot( &tr, iBulletType );
				}
				break;
			case BULLET_PLAYER_BUCKSHOT:
				if ( !tracer )
				{
					if ( bHitPlayer )
						EV_HLDM_PlayerBlood( 8, vecDirShooting, tr.endpos );
					if ( !EV_HLDM_TextureEffect( idx, &tr, vecSrc, vecEnd, iBulletType ) )
						EV_HLDM_DecalGunshot( &tr, iBulletType );
				}
				break;
			default:
			case BULLET_PLAYER_9MM:		
				if ( !tracer )
				{
					EV_HLDM_PlayTextureSound( idx, &tr, vecSrc, vecEnd, iBulletType );
					bForceField = EV_HLDM_TextureEffect( idx, &tr, vecSrc, vecEnd, iBulletType );
					if ( bHitPlayer )
						EV_HLDM_PlayerBlood( 9, vecDirShooting, tr.endpos );
					if ( !bForceField )
						EV_HLDM_DecalGunshot( &tr, iBulletType );
				}
				break;
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

	EV_GetDefaultShellInfo( args, origin, velocity, ShellVelocity, ShellOrigin, forward, right, up, 20, -12, 4 );

	EV_EjectBrass ( ShellOrigin, ShellVelocity, angles[ YAW ], shell, TE_BOUNCE_SHELL ); 

	gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "weapons/pl_gun3.wav", gEngfuncs.pfnRandomFloat(0.92, 1.0), ATTN_NORM, 0, 98 + gEngfuncs.pfnRandomLong( 0, 3 ) );

	EV_GetGunPosition( args, vecSrc, origin );
	
	VectorCopy( forward, vecAiming );

	for ( i = 0; i < 3; i++ )
	{
		vecSpread[ i ] = flSpread;
	}

	EV_HLDM_FireBullets( idx, forward, right, up, 1, vecSrc, vecAiming, vecSpread, 8192, BULLET_PLAYER_9MM, 0, &tracerCount[idx-1] );

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

	EV_GetDefaultShellInfo( args, origin, velocity, ShellVelocity, ShellOrigin, forward, right, up, 20, -12, 4 );

	EV_EjectBrass ( ShellOrigin, ShellVelocity, angles[ YAW ], shell, TE_BOUNCE_SHELL ); 

	gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "weapons/pl_gun3.wav", gEngfuncs.pfnRandomFloat(0.92, 1.0), ATTN_NORM, 0, 98 + gEngfuncs.pfnRandomLong( 0, 3 ) );

	EV_GetGunPosition( args, vecSrc, origin );
	
	VectorCopy( forward, vecAiming );

	for ( i = 0; i < 3; i++ )
	{
		vecSpread[ i ] = flSpread;
	}

	EV_HLDM_FireBullets( idx, forward, right, up, 1, vecSrc, vecAiming, vecSpread, 8192, BULLET_PLAYER_9MM, 0, &tracerCount[idx-1] );
	
	if ( EV_IsLocal( idx ) )
	{
		V_PunchAxis( 0, -2.0 );
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
	}

	EV_GetDefaultShellInfo( args, origin, velocity, ShellVelocity, ShellOrigin, forward, right, up, 32, -12, 6 );

	EV_EjectBrass ( ShellOrigin, ShellVelocity, angles[ YAW ], shell, TE_BOUNCE_SHOTSHELL ); 

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

		EV_HLDM_FireBullets( idx, forward, right, up, 4, vecSrc, vecAiming, vecSpread, 2048, BULLET_PLAYER_BUCKSHOT, 0, &tracerCount[idx-1] );
	}
	else
	{
		EV_HLDM_FireBullets( idx, forward, right, up, 6, vecSrc, vecAiming, VECTOR_CONE_10DEGREES, 2048, BULLET_PLAYER_BUCKSHOT, 0, &tracerCount[idx-1] );
	}

	if ( EV_IsLocal( idx ) )
	{
		V_PunchAxis( 0, -5.0 );
	}
}

void EV_FireGWShotgun( event_args_t *args )
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
	int iShells;

	idx = args->entindex;
	VectorCopy( args->origin, origin );
	iShells = args->iparam2;
	VectorCopy( args->angles, angles );
	VectorCopy( args->velocity, velocity );

	AngleVectors( angles, forward, right, up );

	shell = gEngfuncs.pEventAPI->EV_FindModelIndex ("models/shotgunshell.mdl");// brass shell

	// iparam1 is the spread setting
	switch ( args->iparam1 )
	{
	case 1:
		vecSpread = VECTOR_CONE_6DEGREES;
		break;
	case 2:
		vecSpread = Vector( 0.10906, 0.10906, 0.10906 );
		break;
	case 3:
		vecSpread = VECTOR_CONE_20DEGREES;
		break;
	case 4:
		vecSpread = VECTOR_CONE_20DEGREES;
		break;
	default:
		vecSpread = VECTOR_CONE_20DEGREES;
		break;
	}

	if ( EV_IsLocal( idx ) )
	{
		// Add muzzle flash to current weapon model
		EV_MuzzleFlash();

		switch ( args->iparam2 )
		{
		case 1:
			gEngfuncs.pEventAPI->EV_WeaponAnimation( 3, 2 );
			break;
		case 2:
			gEngfuncs.pEventAPI->EV_WeaponAnimation( 4, 2 );
			break;
		case 3:
			gEngfuncs.pEventAPI->EV_WeaponAnimation( 5, 2 );
			break;
		case 4:
			gEngfuncs.pEventAPI->EV_WeaponAnimation( 6, 2 );
			break;
		}
	}

	for ( i = 0; i < args->iparam2; i++ )
	{
		EV_GetDefaultShellInfo( args, origin, velocity, ShellVelocity, ShellOrigin, forward, right, up, 32, -12, 6 );

		EV_EjectBrass ( ShellOrigin, ShellVelocity, angles[ YAW ], shell, TE_BOUNCE_SHOTSHELL ); 
	}

	gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "weapons/sbarrel1.wav", gEngfuncs.pfnRandomFloat(0.95, 1.0), ATTN_NORM, 0, 93 + gEngfuncs.pfnRandomLong( 0, 0x1f ) );

	EV_GetGunPosition( args, vecSrc, origin );
	VectorCopy( forward, vecAiming );

	EV_HLDM_FireBullets( idx, forward, right, up, iShells * 4, vecSrc, vecAiming, vecSpread, 2048, BULLET_PLAYER_BUCKSHOT, 0, &tracerCount[idx-1] );

	if ( EV_IsLocal( idx ) )
	{
		V_PunchAxis( 0, -( ( iShells + 1 ) * iShells ) );
	}
}

void EV_TankMGuns( event_args_t *args )
{
	int idx;
	vec3_t origin;
	vec3_t angles;
	vec3_t vecSrc, vecAiming;
	vec3_t vecSpread;
	vec3_t up, right, forward;
	float flVolume;
	cl_entity_t *ent;

	idx = args->entindex;
	VectorCopy( args->origin, origin );
	VectorCopy( args->angles, angles );
	flVolume = args->fparam1;

	AngleVectors( angles, forward, right, up );

	// fire from where the tank is now
	ent = GetEntity( idx );
	origin = origin + ent->ph[ ent->current_position ].origin;

	vecSpread = VECTOR_CONE_4DEGREES;

	switch( gEngfuncs.pfnRandomLong( 0, 2 ) )
	{
	case 0:
		gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "weapons/hks1.wav", flVolume, ATTN_NORM, 0, PITCH_NORM );
		break;
	case 1:
		gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "weapons/hks2.wav", flVolume, ATTN_NORM, 0, PITCH_NORM );
		break;
	case 2:
		gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "weapons/hks3.wav", flVolume, ATTN_NORM, 0, PITCH_NORM );
		break;
	}

	VectorCopy( origin, vecSrc );
	VectorCopy( forward, vecAiming );

	EV_HLDM_FireBullets( idx, forward, right, up, 2, vecSrc, vecAiming, vecSpread, 8192, BULLET_PLAYER_MP5, 3, &tracerCount[idx-1] );
}

void EV_TurretBullets( event_args_t *args )
{
	static vec3_t vecSpreads[] = { Vector( 0, 0, 0 ), Vector( 0.025, 0.025, 0.025 ), Vector( 0.05, 0.05, 0.05 ), Vector( 0.1, 0.1, 0.1 ), Vector( 0.25, 0.25, 0.25 ) };

	int idx;
	vec3_t origin;
	vec3_t angles;
	vec3_t vecSrc, vecAiming;
	vec3_t up, right, forward;

	idx = args->entindex;
	VectorCopy( args->origin, origin );
	VectorCopy( args->angles, angles );

	AngleVectors( angles, forward, right, up );

	VectorCopy( origin, vecSrc );
	VectorCopy( forward, vecAiming );

	// the turret is upside down
	forward[2] = forward[2] * -1;
	vecAiming[2] = vecAiming[2] * -1;

	EV_HLDM_FireBullets( idx, forward, right, up, (int)args->fparam1, vecSrc, vecAiming, vecSpreads[ args->iparam1 ], 4096, args->iparam2, 1, &tracerCount[idx-1] );
}

void EV_MonsterSentry( event_args_t *args )
{
	int idx;
	vec3_t origin;
	vec3_t angles;
	int shell;
	vec3_t vecSpread;
	vec3_t up, right, forward;

	idx = args->entindex;
	VectorCopy( args->origin, origin );
	VectorCopy( args->angles, angles );

	AngleVectors( angles, forward, right, up );

	if ( args->bparam1 )
		vecSpread = Vector( 0, 0, 0 );
	else
		vecSpread = VECTOR_CONE_10DEGREES;

	shell = gEngfuncs.pEventAPI->EV_FindModelIndex ("models/shell.mdl");// brass shell

	// sic: shoots along the angles, not forward
	EV_HLDM_FireBullets( idx, forward, right, up, 1, origin, angles, vecSpread, 2048, BULLET_PLAYER_MP5, 1, &tracerCount[idx-1] );
}

void EV_MonsterShotgun( event_args_t *args )
{
	int idx;
	vec3_t origin;
	vec3_t angles;
	vec3_t velocity;
	
	int i;
	vec3_t ShellVelocity;
	vec3_t ShellOrigin;
	int shell;
	vec3_t vecSpread;
	vec3_t vecSrc, vecAiming;
	vec3_t up, right, forward;

	idx = args->entindex;
	VectorCopy( args->origin, origin );
	VectorCopy( args->angles, angles );
	VectorCopy( args->velocity, velocity );

	AngleVectors( angles, forward, right, up );

	vecSpread = VECTOR_CONE_15DEGREES;

	shell = gEngfuncs.pEventAPI->EV_FindModelIndex ("models/shotgunshell.mdl");// brass shell

	for ( i = 0; i < 3; i++ )
	{
		EV_GetDefaultShellInfo( args, origin, velocity, ShellVelocity, ShellOrigin, forward, right, up, 32, -12, 6 );

		ShellOrigin = ShellOrigin + forward * -8 + up * -3 + right * 5;
		EV_EjectBrass ( ShellOrigin, ShellVelocity, angles[ YAW ], shell, TE_BOUNCE_SHOTSHELL ); 
	}

	// sic: vecSrc and vecAiming go unused; the bullets start at the origin
	// and fly along the angles, not forward
	VectorCopy( origin, vecSrc );
	VectorCopy( angles, vecAiming );

	EV_HLDM_FireBullets( idx, forward, right, up, 8, origin, angles, vecSpread, 2048, BULLET_PLAYER_BUCKSHOT, 0, &tracerCount[idx-1] );
}

void EV_MonsterMechagun( event_args_t *args )
{
	int idx;
	vec3_t origin;
	vec3_t angles;
	vec3_t velocity;
	
	int i;
	vec3_t ShellVelocity;
	vec3_t ShellOrigin;
	int shell;
	vec3_t vecSpread;
	vec3_t vecSrc, vecAiming;
	vec3_t up, right, forward;

	idx = args->entindex;
	VectorCopy( args->origin, origin );
	VectorCopy( args->angles, angles );
	VectorCopy( args->velocity, velocity );

	AngleVectors( angles, forward, right, up );

	vecSpread = VECTOR_CONE_3DEGREES;

	shell = gEngfuncs.pEventAPI->EV_FindModelIndex ("models/shell.mdl");// brass shell

	for ( i = 0; i < 2; i++ )
	{
		EV_GetDefaultShellInfo( args, origin, velocity, ShellVelocity, ShellOrigin, forward, right, up, 32, -12, 6 );

		ShellOrigin = ShellOrigin + forward * -8 + up * -3 + right * 5;
		EV_EjectBrass ( ShellOrigin, ShellVelocity, angles[ YAW ], shell, TE_BOUNCE_SHELL ); 
	}

	// sic: vecSrc and vecAiming go unused; the bullets start at the origin
	// and fly along the angles, not forward
	VectorCopy( origin, vecSrc );
	VectorCopy( angles, vecAiming );

	EV_HLDM_FireBullets( idx, forward, right, up, 1, origin, angles, vecSpread, 2048, BULLET_PLAYER_MP5, 1, &tracerCount[idx-1] );
}

void EV_ChopperFire( event_args_t *args )
{
	int idx;
	vec3_t origin;
	vec3_t angles;
	vec3_t velocity;
	
	int i;
	vec3_t ShellVelocity;
	vec3_t ShellOrigin;
	int shell;
	vec3_t vecSpread;
	vec3_t vecSrc, vecAiming;
	vec3_t up, right, forward;

	idx = args->entindex;
	VectorCopy( args->origin, origin );
	VectorCopy( args->angles, angles );
	VectorCopy( args->velocity, velocity );

	AngleVectors( angles, forward, right, up );

	vecSpread = VECTOR_CONE_8DEGREES;

	shell = gEngfuncs.pEventAPI->EV_FindModelIndex ("models/shell.mdl");// brass shell

	for ( i = 0; i < 2; i++ )
	{
		EV_GetDefaultShellInfo( args, origin, velocity, ShellVelocity, ShellOrigin, forward, right, up, 32, -12, 6 );

		ShellOrigin = ShellOrigin + forward * -8 + up * -3 + right * 5;
		EV_EjectBrass ( ShellOrigin, ShellVelocity, angles[ YAW ], shell, TE_BOUNCE_SHELL ); 
	}

	// sic: vecSrc and vecAiming go unused; the bullets start at the origin
	// and fly along the angles, not forward
	VectorCopy( origin, vecSrc );
	VectorCopy( angles, vecAiming );

	EV_HLDM_FireBullets( idx, forward, right, up, 1, origin, angles, vecSpread, 2048, BULLET_PLAYER_MP5, 2, &tracerCount[idx-1] );
}

void EV_FireGWMinigun( event_args_t *args )
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

	idx = args->entindex;
	VectorCopy( args->origin, origin );
	VectorCopy( args->angles, angles );
	VectorCopy( args->velocity, velocity );

	AngleVectors( angles, forward, right, up );

	shell = gEngfuncs.pEventAPI->EV_FindModelIndex ("models/shell.mdl");// brass shell

	// iparam1 is the spread setting
	switch ( args->iparam1 )
	{
	case 1:
		vecSpread = VECTOR_CONE_10DEGREES;
		break;
	case 2:
		vecSpread = VECTOR_CONE_3DEGREES;
		break;
	case 4:
		vecSpread = VECTOR_CONE_6DEGREES;
		break;
	default:
		vecSpread = VECTOR_CONE_20DEGREES;
		break;
	}
	
	if ( EV_IsLocal( idx ) )
	{
		// Add muzzle flash to current weapon model
		EV_MuzzleFlash();

		if ( args->iparam2 )
		{
			if ( args->fparam1 < args->fparam2 )
				gEngfuncs.pEventAPI->EV_WeaponAnimation( 4, args->bparam1 );
		}
		else
		{
			gEngfuncs.pEventAPI->EV_WeaponAnimation( 2, args->bparam1 );
		}
	}

	EV_GetDefaultShellInfo( args, origin, velocity, ShellVelocity, ShellOrigin, forward, right, up, 20, -12, 4 );

	EV_EjectBrass ( ShellOrigin + up * 8 + forward * 2 + right * 8, ShellVelocity, angles[ YAW ], shell, TE_BOUNCE_SHELL ); 

	switch( gEngfuncs.pfnRandomLong( 0, 2 ) )
	{
	case 0:
		gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "weapons/hks1.wav", 0.65, ATTN_NORM, 0, 94 + gEngfuncs.pfnRandomLong( 0, 0xf ) );
		break;
	case 1:
		gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "weapons/hks2.wav", 0.65, ATTN_NORM, 0, 94 + gEngfuncs.pfnRandomLong( 0, 0xf ) );
		break;
	case 2:
		gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "weapons/hks3.wav", 0.65, ATTN_NORM, 0, 94 + gEngfuncs.pfnRandomLong( 0, 0xf ) );
		break;
	}

	EV_GetGunPosition( args, vecSrc, origin );
	VectorCopy( forward, vecAiming );

	EV_HLDM_FireBullets( idx, forward, right, up, 1, vecSrc, vecAiming, vecSpread, 8192, BULLET_PLAYER_MP5, 1, &tracerCount[idx-1] );

	if ( EV_IsLocal( idx ) )
	{
		V_PunchAxis( 0, gEngfuncs.pfnRandomFloat( -2, 2 ) );
	}
}

void EV_MinigunHeat( event_args_t *args )
{
	int bOverheating = args->bparam1;
	int bCooled = args->bparam2;

	if ( EV_IsLocal( args->entindex ) )
	{
		cl_entity_t *view = gEngfuncs.GetViewModel();

		if ( view )
		{
			// the barrels glow while it is too hot to fire
			if ( bOverheating )
			{
				view->curstate.renderfx = kRenderFxGlowShell;
				view->curstate.renderamt = 60;

				if ( !bCooled )
				{
					view->curstate.rendercolor.r = 255;
					view->curstate.rendercolor.g = 100;
					view->curstate.rendercolor.b = 0;
				}
				else
				{
					view->curstate.rendercolor.r = 100;
					view->curstate.rendercolor.g = 100;
					view->curstate.rendercolor.b = 255;
				}
			}
			else
			{
				view->curstate.renderfx = kRenderFxNone;
			}
		}
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

	EV_GetDefaultShellInfo( args, origin, velocity, ShellVelocity, ShellOrigin, forward, right, up, 20, -12, 4 );

	EV_EjectBrass ( ShellOrigin, ShellVelocity, angles[ YAW ], shell, TE_BOUNCE_SHELL ); 

	switch( gEngfuncs.pfnRandomLong( 0, 1 ) )
	{
	case 0:
		gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "weapons/hks1.wav", 0.5, ATTN_NORM, 0, 94 + gEngfuncs.pfnRandomLong( 0, 0xf ) );
		break;
	case 1:
		gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "weapons/hks2.wav", 0.5, ATTN_NORM, 0, 94 + gEngfuncs.pfnRandomLong( 0, 0xf ) );
		break;
	}

	EV_GetGunPosition( args, vecSrc, origin );
	VectorCopy( forward, vecAiming );

	if ( gEngfuncs.GetMaxClients() > 1 )
	{
		EV_HLDM_FireBullets( idx, forward, right, up, 1, vecSrc, vecAiming, VECTOR_CONE_6DEGREES, 8192, BULLET_PLAYER_MP5, 2, &tracerCount[idx-1] );
	}
	else
	{
		EV_HLDM_FireBullets( idx, forward, right, up, 1, vecSrc, vecAiming, VECTOR_CONE_3DEGREES, 8192, BULLET_PLAYER_MP5, 2, &tracerCount[idx-1] );
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

	EV_HLDM_FireBullets( idx, forward, right, up, 1, vecSrc, vecAiming, VECTOR_CONE_1DEGREES, 8192, BULLET_PLAYER_357, 0, &tracerCount[idx-1] );

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
// the gauss pistol fired a beam this frame, and who fired it
int g_iGaussBeamFired;
int g_iGaussBeamOwner;

void EV_FireGauss( event_args_t *args )
{
	int idx;
	vec3_t origin;
	vec3_t angles;
	int fNoTrace = args->bparam2;

	vec3_t vecSrc;
	vec3_t vecDest;
	vec3_t vecBeam, vecNormal;
	pmtrace_t tr;
	physent_t *pEntity;
	int m_iBeam, m_iGlow, m_iBalls;
	int iStartEnt;
	vec3_t up, right, forward;
	char *decalName = "{pulse2";

	idx = args->entindex;
	VectorCopy( args->origin, origin );
	VectorCopy( args->angles, angles );

	EV_GetGunPosition( args, vecSrc, origin );

	m_iBeam = gEngfuncs.pEventAPI->EV_FindModelIndex( "sprites/gaussbeam2.spr" );
	m_iGlow = gEngfuncs.pEventAPI->EV_FindModelIndex( "sprites/gausspark.spr" );
	m_iBalls = gEngfuncs.pEventAPI->EV_FindModelIndex( "models/w_gauss.mdl" );

	AngleVectors( angles, forward, right, up );
	VectorMA( vecSrc, 8192, forward, vecDest );

	if ( EV_IsLocal( idx ) )
	{
		V_PunchAxis( 0, -1.0 );
		gEngfuncs.pEventAPI->EV_WeaponAnimation( 2, 2 );
	}

	if ( args->iparam1 == 1 )
		gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "weapons/gauss_fire4.wav", 1.0, ATTN_NORM, 0, 85 + gEngfuncs.pfnRandomLong( 0, 0x1f ) );
	else
		gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "weapons/gauss_fire4.wav", 1.0, ATTN_NORM, 0, 85 + gEngfuncs.pfnRandomLong( 0, 0x1f ) );

	gEngfuncs.pEventAPI->EV_SetUpPlayerPrediction( false, true );
	
	// Store off the old count
	gEngfuncs.pEventAPI->EV_PushPMStates();

	// Now add in all of the players.
	gEngfuncs.pEventAPI->EV_SetSolidPlayers ( idx - 1 );	

	gEngfuncs.pEventAPI->EV_SetTraceHull( 2 );

	if ( !fNoTrace )
		gEngfuncs.pEventAPI->EV_PlayerTrace( vecSrc, vecDest, PM_STUDIO_BOX, -1, &tr );

	pEntity = gEngfuncs.pEventAPI->EV_GetPhysent( tr.ent );

	gEngfuncs.pEventAPI->EV_PopPMStates();

	if ( EV_IsLocal( idx ) )
	{
		V_PunchAxis( 0, -2.2 );
	}

	if ( !fNoTrace )
	{
		VectorCopy( tr.endpos, vecBeam );
		VectorCopy( tr.plane.normal, vecNormal );
		iStartEnt = idx | 0x1000;
	}
	else
	{
		if ( args->iparam2 )
			iStartEnt = idx | 0x1000;
		else
			iStartEnt = idx | 0x2000;
		VectorCopy( angles, vecBeam );
		VectorCopy( origin, vecNormal );
	}

	g_iGaussBeamFired = 1;
	g_iGaussBeamOwner = idx;
	gEngfuncs.pEfxAPI->R_BeamEntPoint( iStartEnt, vecBeam, m_iBeam, 0.1, 0.8, 0, 128, 0, 0, 0, 255, 255, 255 );

	if ( pEntity )
	{
		if ( pEntity->solid == SOLID_BSP )
		{
			vec3_t vecEnd = tr.endpos + tr.plane.normal;

			if ( gEngfuncs.GetMaxClients() < 2 )
				gEngfuncs.pEfxAPI->R_Sprite_Trail( TE_SPRITETRAIL, tr.endpos, vecEnd, m_iGlow, 2, 0, gEngfuncs.pfnRandomFloat( 0, 20 ) / 100, 100, 255, 200 );

			g_Effects.Add( CEffect( 8, vecBeam + vecNormal * 5, vecNormal, 0.5, EFFECT_FLAME2, 255, 20, gEngfuncs.GetClientTime(), (struct model_s *)gEngfuncs.GetSpritePointer( SPR_Load( "sprites/gaussimpact.spr" ) ) ) );
		}
		else if ( pEntity->takedamage && EV_IsPlayer( pEntity->info ) )
		{
			EV_HLDM_PlayerBlood( 9, forward, vecBeam );
		}
	}

	g_Effects.Add( CEffect( 30, vecBeam, vecNormal, 3, EFFECT_SHOCK, 255, 32, gEngfuncs.GetClientTime(), NULL ) );

	pEntity = gEngfuncs.pEventAPI->EV_GetPhysent( tr.ent );

	// Only decal brush models such as the world etc.
	if ( pEntity && decalName && decalName[0] && pEntity->rendermode == kRenderNormal && ( pEntity->solid == SOLID_BSP || pEntity->movetype == MOVETYPE_PUSHSTEP ) )
	{
		if ( CVAR_GET_FLOAT( "r_decals" ) )
		{
			gEngfuncs.pEfxAPI->R_DecalShoot( 
				gEngfuncs.pEfxAPI->Draw_DecalIndex( gEngfuncs.pEfxAPI->Draw_DecalIndexFromName( decalName ) ), 
				gEngfuncs.pEventAPI->EV_IndexFromTrace( &tr ), 0, tr.endpos, 0 );
		}
	}
}

void EV_Cleanup( event_args_t *args )
{
	if ( EV_IsLocal( args->entindex ) )
	{
		cl_entity_t *view = gEngfuncs.GetViewModel();

		if ( view )
			view->curstate.renderfx = kRenderFxNone;
	}
}

void EV_BatbotBeam( event_args_t *args )
{
	int idx;
	vec3_t origin;

	idx = args->entindex;
	VectorCopy( args->origin, origin );

	g_Effects.Add( CEffect( 4, origin, Vector( 0, 0, -1 ), 0.1, EFFECT_BEAM, 240, 6, idx, 0, 0, gEngfuncs.GetClientTime(), (struct model_s *)gEngfuncs.GetSpritePointer( SPR_Load( "sprites/hugeball.spr" ) ) ) );
}

void EV_RustbitImpact( event_args_t *args )
{
	int idx;
	vec3_t origin;
	vec3_t angles;
	vec3_t up, right, forward;
	vec3_t vecDest;
	pmtrace_t tr;
	physent_t *pe;
	char *decalName = "{pulse";

	idx = args->entindex;
	VectorCopy( args->origin, origin );
	VectorCopy( args->angles, angles );

	AngleVectors( angles, forward, right, up );
	VectorMA( origin, 2048, forward, vecDest );

	gEngfuncs.pEventAPI->EV_SetUpPlayerPrediction( false, true );
	gEngfuncs.pEventAPI->EV_PushPMStates();
	gEngfuncs.pEventAPI->EV_SetSolidPlayers ( idx - 1 );	
	gEngfuncs.pEventAPI->EV_SetTraceHull( 2 );

	origin = origin + forward * -10;
	vecDest = origin + forward * 2048;
	gEngfuncs.pEventAPI->EV_PlayerTrace( origin, vecDest, PM_STUDIO_BOX, -1, &tr );

	gEngfuncs.pEventAPI->EV_PopPMStates();

	pe = gEngfuncs.pEventAPI->EV_GetPhysent( tr.ent );

	// Only decal brush models such as the world etc.
	if ( decalName && decalName[0] && pe && pe->rendermode == kRenderNormal && ( pe->solid == SOLID_BSP || pe->movetype == MOVETYPE_PUSHSTEP ) )
	{
		if ( CVAR_GET_FLOAT( "r_decals" ) )
		{
			gEngfuncs.pEfxAPI->R_DecalShoot( 
				gEngfuncs.pEfxAPI->Draw_DecalIndex( gEngfuncs.pEfxAPI->Draw_DecalIndexFromName( decalName ) ), 
				gEngfuncs.pEventAPI->EV_IndexFromTrace( &tr ), 0, tr.endpos, 0 );
		}
	}
}

void EV_BallLightning( event_args_t *args )
{
	int idx;
	vec3_t origin;
	vec3_t angles;
	vec3_t up, right, forward;
	vec3_t vecDest, vecSpot;
	pmtrace_t tr;
	physent_t *pe;
	dlight_t *dl;
	char *decalName;
	int iSize;
	int fBurn = args->bparam2;

	idx = args->entindex;
	VectorCopy( args->origin, origin );
	VectorCopy( args->angles, angles );

	AngleVectors( angles, forward, right, up );

	if ( !fBurn )
		forward[2] = forward[2] * -1;

	VectorMA( origin, 2048, forward, vecDest );

	gEngfuncs.pEventAPI->EV_SetUpPlayerPrediction( false, true );
	gEngfuncs.pEventAPI->EV_PushPMStates();
	gEngfuncs.pEventAPI->EV_SetSolidPlayers ( idx - 1 );	
	gEngfuncs.pEventAPI->EV_SetTraceHull( 2 );

	origin = origin + forward * -10;
	vecDest = origin + forward * 2048;
	gEngfuncs.pEventAPI->EV_PlayerTrace( origin, vecDest, PM_STUDIO_BOX, -1, &tr );

	gEngfuncs.pEventAPI->EV_PopPMStates();

	vecSpot = tr.endpos + tr.plane.normal * 16;
	vecSpot[2] = vecSpot[2] - 8;

	if ( fBurn )
	{
		dl = gEngfuncs.pEfxAPI->CL_AllocDlight( 0 );
		VectorCopy( vecSpot, dl->origin );
		dl->radius = 130;
		dl->color.r = 180;
		dl->color.g = 0;
		dl->color.b = 255;
		dl->decay = 80;
		dl->die = gEngfuncs.GetClientTime() + 1.0;
	}

	if ( fBurn )
		decalName = "{balburn";
	else
		decalName = "{pulse";

	pe = gEngfuncs.pEventAPI->EV_GetPhysent( tr.ent );

	// Only decal brush models such as the world etc.
	if ( decalName && decalName[0] && pe && pe->rendermode == kRenderNormal && ( pe->solid == SOLID_BSP || pe->movetype == MOVETYPE_PUSHSTEP ) )
	{
		if ( CVAR_GET_FLOAT( "r_decals" ) )
		{
			gEngfuncs.pEfxAPI->R_DecalShoot( 
				gEngfuncs.pEfxAPI->Draw_DecalIndex( gEngfuncs.pEfxAPI->Draw_DecalIndexFromName( decalName ) ), 
				gEngfuncs.pEventAPI->EV_IndexFromTrace( &tr ), 0, tr.endpos, 0 );
		}
	}

	if ( fBurn )
		iSize = 130;
	else
		iSize = 65;

	g_Effects.Add( CEffect( iSize, vecSpot, tr.plane.normal, 0, EFFECT_SPRITE, 0, 3, 10, 11, 11, gEngfuncs.GetClientTime(), (struct model_s *)gEngfuncs.GetSpritePointer( SPR_Load( "sprites/ballpop.spr" ) ) ) );
}

void EV_KataImpact( event_args_t *args )
{
	int idx;
	vec3_t origin;
	vec3_t angles;
	vec3_t velocity;
	vec3_t up, right, forward;
	vec3_t vecDest, vecSpot;
	pmtrace_t tr;
	physent_t *pe;
	dlight_t *dl;
	char *decalName = "{scorch1";

	idx = args->entindex;
	VectorCopy( args->origin, origin );
	VectorCopy( args->angles, angles );
	VectorCopy( args->velocity, velocity );

	VectorAngles( velocity, angles );
	AngleVectors( angles, forward, right, up );
	forward[2] = forward[2] * -1;

	VectorMA( origin, 2048, forward, vecDest );

	gEngfuncs.pEventAPI->EV_SetUpPlayerPrediction( false, true );
	gEngfuncs.pEventAPI->EV_PushPMStates();
	gEngfuncs.pEventAPI->EV_SetSolidPlayers ( idx - 1 );	
	gEngfuncs.pEventAPI->EV_SetTraceHull( 2 );

	origin = origin + forward * -10;
	vecDest = origin + forward * 2048;
	gEngfuncs.pEventAPI->EV_PlayerTrace( origin, vecDest, PM_STUDIO_BOX, -1, &tr );

	gEngfuncs.pEventAPI->EV_PopPMStates();

	vecSpot = tr.endpos + tr.plane.normal * 16;
	vecSpot[2] = vecSpot[2] - 8;

	dl = gEngfuncs.pEfxAPI->CL_AllocDlight( 0 );
	VectorCopy( vecSpot, dl->origin );
	dl->radius = 130;
	dl->color.r = 0;
	dl->color.g = 255;
	dl->color.b = 255;
	dl->decay = 80;
	dl->die = gEngfuncs.GetClientTime() + 1.0;

	pe = gEngfuncs.pEventAPI->EV_GetPhysent( tr.ent );

	// Only decal brush models such as the world etc.
	if ( decalName && decalName[0] && pe && pe->rendermode == kRenderNormal && ( pe->solid == SOLID_BSP || pe->movetype == MOVETYPE_PUSHSTEP ) )
	{
		if ( CVAR_GET_FLOAT( "r_decals" ) )
		{
			gEngfuncs.pEfxAPI->R_DecalShoot( 
				gEngfuncs.pEfxAPI->Draw_DecalIndex( gEngfuncs.pEfxAPI->Draw_DecalIndexFromName( decalName ) ), 
				gEngfuncs.pEventAPI->EV_IndexFromTrace( &tr ), 0, tr.endpos, 0 );
		}
	}

	g_Effects.Add( CEffect( 130, vecSpot, tr.plane.normal, 0, EFFECT_SPRITE, 0, 3, 12, 18, 18, gEngfuncs.GetClientTime(), (struct model_s *)gEngfuncs.GetSpritePointer( SPR_Load( "sprites/kataimpact.spr" ) ) ) );
}

void EV_SniperShot( event_args_t *args )
{
	int idx;
	vec3_t origin;
	vec3_t angles;
	vec3_t vecSrc, vecDest, vecEnd;
	vec3_t vecStart, vecDir;
	vec3_t up, right, forward;
	pmtrace_t tr;
	dlight_t *dl;
	int m_iBeam;
	int fRings = args->bparam1;
	int iSize;
	float flLength;
	char *decalName = "{pulse2";

	idx = args->entindex;
	VectorCopy( args->origin, origin );
	VectorCopy( args->angles, angles );

	EV_GetGunPosition( args, vecSrc, origin );

	m_iBeam = gEngfuncs.pEventAPI->EV_FindModelIndex( "sprites/gaussbeam2.spr" );

	AngleVectors( angles, forward, right, up );
	VectorMA( vecSrc, 8192, forward, vecDest );

	gEngfuncs.pEventAPI->EV_SetUpPlayerPrediction( false, true );
	gEngfuncs.pEventAPI->EV_PushPMStates();
	gEngfuncs.pEventAPI->EV_SetSolidPlayers ( idx - 1 );	
	gEngfuncs.pEventAPI->EV_SetTraceHull( 2 );

	gEngfuncs.pEventAPI->EV_PlayerTrace( vecSrc, vecDest, PM_STUDIO_BOX, -1, &tr );

	gEngfuncs.pEventAPI->EV_PopPMStates();

	gEngfuncs.pEventAPI->EV_GetPhysent( tr.ent );

	if ( EV_IsLocal( idx ) )
	{
		V_PunchAxis( 0, -2.2 );
	}

	VectorCopy( tr.endpos, vecEnd );

	// the rings start at the barrel
	vecStart = vecSrc + right * 1 + forward * 11 + up * -1;
	vecDir = vecStart - vecEnd;
	flLength = vecDir.Length();

	iSize = 60;
	if ( fRings )
	{
		iSize = 100;
		for ( int i = 80; i < flLength; i += 80 )
		{
			g_Effects.Add( CEffect( 13, vecStart + forward * i, forward, 0.1, EFFECT_RING, 240, 5, gEngfuncs.GetClientTime(), (struct model_s *)gEngfuncs.GetSpritePointer( SPR_Load( "sprites/gaussring.spr" ) ) ) );
		}
	}

	g_Effects.Add( CEffect( iSize, vecEnd, tr.plane.normal, 0, EFFECT_SPRITE, 0, 3, 13, 9, 9, gEngfuncs.GetClientTime(), (struct model_s *)gEngfuncs.GetSpritePointer( SPR_Load( "sprites/gausspoof.spr" ) ) ) );

	dl = gEngfuncs.pEfxAPI->CL_AllocDlight( 0 );
	VectorCopy( vecEnd, dl->origin );
	dl->radius = iSize + 40;
	dl->color.r = 0;
	dl->color.g = 255;
	dl->color.b = 0;
	dl->decay = 80;
	dl->die = gEngfuncs.GetClientTime() + 0.7;

	physent_t *pe = gEngfuncs.pEventAPI->EV_GetPhysent( tr.ent );

	// Only decal brush models such as the world etc.
	if ( pe && decalName && decalName[0] && pe->rendermode == kRenderNormal && ( pe->solid == SOLID_BSP || pe->movetype == MOVETYPE_PUSHSTEP ) )
	{
		if ( CVAR_GET_FLOAT( "r_decals" ) )
		{
			gEngfuncs.pEfxAPI->R_DecalShoot( 
				gEngfuncs.pEfxAPI->Draw_DecalIndex( gEngfuncs.pEfxAPI->Draw_DecalIndexFromName( decalName ) ), 
				gEngfuncs.pEventAPI->EV_IndexFromTrace( &tr ), 0, tr.endpos, 0 );
		}
	}
}

void EV_ChargedImpact( event_args_t *args )
{
	int idx;
	vec3_t origin;
	vec3_t angles;
	vec3_t up, right, forward;
	vec3_t vecDest, vecSpot;
	pmtrace_t tr;
	physent_t *pe;
	dlight_t *dl;
	char *decalName = "{charge";

	idx = args->entindex;
	VectorCopy( args->origin, origin );
	VectorCopy( args->angles, angles );

	AngleVectors( angles, forward, right, up );
	VectorMA( origin, 2048, forward, vecDest );

	gEngfuncs.pEventAPI->EV_SetUpPlayerPrediction( false, true );
	gEngfuncs.pEventAPI->EV_PushPMStates();
	gEngfuncs.pEventAPI->EV_SetSolidPlayers ( idx - 1 );	
	gEngfuncs.pEventAPI->EV_SetTraceHull( 2 );

	origin = origin + forward * -10;
	vecDest = origin + forward * 2048;
	gEngfuncs.pEventAPI->EV_PlayerTrace( origin, vecDest, PM_STUDIO_BOX, -1, &tr );

	pe = gEngfuncs.pEventAPI->EV_GetPhysent( tr.ent );

	gEngfuncs.pEventAPI->EV_PopPMStates();

	vecSpot = tr.endpos + tr.plane.normal * 16;
	vecSpot[2] = vecSpot[2] - 8;

	gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "weapons/gauss_spritebig.wav", 1.0, ATTN_NORM, 0, 85 + gEngfuncs.pfnRandomLong( 0, 0x1f ) );

	if ( pe && pe->solid == SOLID_BSP )
		gEngfuncs.pEfxAPI->R_Explosion( vecSpot, gEngfuncs.pEventAPI->EV_FindModelIndex( "sprites/gausspuff.spr" ), 1, 10, TE_EXPLFLAG_NODLIGHTS | TE_EXPLFLAG_NOSOUND | TE_EXPLFLAG_NOPARTICLES );

	dl = gEngfuncs.pEfxAPI->CL_AllocDlight( 0 );
	VectorCopy( vecSpot, dl->origin );
	dl->radius = 130;
	dl->color.r = 0;
	dl->color.g = 150;
	dl->color.b = 255;
	dl->decay = 80;
	dl->die = gEngfuncs.GetClientTime() + 0.5;

	g_Effects.Add( CEffect( 150, vecSpot, tr.plane.normal, 4, EFFECT_SHOCK, 255, 8, gEngfuncs.GetClientTime(), NULL ) );

	if ( pe && pe->takedamage && EV_IsPlayer( pe->info ) )
		EV_HLDM_PlayerBlood( 50, forward, origin );

	// Only decal brush models such as the world etc.
	if ( decalName && decalName[0] && pe && pe->rendermode == kRenderNormal && ( pe->solid == SOLID_BSP || pe->movetype == MOVETYPE_PUSHSTEP ) )
	{
		if ( CVAR_GET_FLOAT( "r_decals" ) )
		{
			gEngfuncs.pEfxAPI->R_DecalShoot( 
				gEngfuncs.pEfxAPI->Draw_DecalIndex( gEngfuncs.pEfxAPI->Draw_DecalIndexFromName( decalName ) ), 
				gEngfuncs.pEventAPI->EV_IndexFromTrace( &tr ), 0, tr.endpos, 0 );
		}
	}
}

void EV_SpurtBlood( event_args_t *args )
{
	vec3_t origin;
	vec3_t angles;
	float flDamage = args->fparam1;

	VectorCopy( args->origin, origin );
	VectorCopy( args->angles, angles );

	EV_HLDM_PlayerBlood( flDamage, angles, origin );
}

void EV_RapidImpact( event_args_t *args )
{
	int idx;
	vec3_t origin;
	vec3_t angles;
	vec3_t up, right, forward;
	vec3_t vecDest, vecSpot;
	pmtrace_t tr;
	physent_t *pe;
	char *decalName = "{shot5";

	idx = args->entindex;
	VectorCopy( args->origin, origin );
	VectorCopy( args->angles, angles );

	AngleVectors( angles, forward, right, up );
	VectorMA( origin, 2048, forward, vecDest );

	gEngfuncs.pEventAPI->EV_SetUpPlayerPrediction( false, true );
	gEngfuncs.pEventAPI->EV_PushPMStates();
	gEngfuncs.pEventAPI->EV_SetSolidPlayers ( idx - 1 );	
	gEngfuncs.pEventAPI->EV_SetTraceHull( 2 );

	origin = origin + forward * -10;
	vecDest = origin + forward * 2048;
	gEngfuncs.pEventAPI->EV_PlayerTrace( origin, vecDest, PM_STUDIO_BOX, -1, &tr );

	pe = gEngfuncs.pEventAPI->EV_GetPhysent( tr.ent );

	gEngfuncs.pEventAPI->EV_PopPMStates();

	vecSpot = tr.endpos + tr.plane.normal * 5;
	vecSpot[2] = vecSpot[2] - 12;

	gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "weapons/gauss_spritesmall.wav", 1.0, ATTN_NORM, 0, 85 + gEngfuncs.pfnRandomLong( 0, 0x1f ) );

	if ( pe && pe->solid == SOLID_BSP )
		gEngfuncs.pEfxAPI->R_Explosion( tr.endpos, gEngfuncs.pEventAPI->EV_FindModelIndex( "sprites/gausspuff.spr" ), 0.25, 15, TE_EXPLFLAG_NODLIGHTS | TE_EXPLFLAG_NOSOUND | TE_EXPLFLAG_NOPARTICLES );

	g_Effects.Add( CEffect( 30, vecSpot, tr.plane.normal, 4, EFFECT_SHOCK, 255, 32, gEngfuncs.GetClientTime(), NULL ) );

	if ( pe && pe->takedamage && EV_IsPlayer( pe->info ) )
		EV_HLDM_PlayerBlood( 12, forward, origin );

	// Only decal brush models such as the world etc.
	if ( decalName && decalName[0] && pe && pe->rendermode == kRenderNormal && ( pe->solid == SOLID_BSP || pe->movetype == MOVETYPE_PUSHSTEP ) )
	{
		if ( CVAR_GET_FLOAT( "r_decals" ) )
		{
			gEngfuncs.pEfxAPI->R_DecalShoot( 
				gEngfuncs.pEfxAPI->Draw_DecalIndex( gEngfuncs.pEfxAPI->Draw_DecalIndexFromName( decalName ) ), 
				gEngfuncs.pEventAPI->EV_IndexFromTrace( &tr ), 0, tr.endpos, 0 );
		}
	}
}

void EV_ChemicalSpray( event_args_t *args )
{
	int idx;
	vec3_t origin;
	vec3_t velocity;
	pmtrace_t tr;
	int iSprite;

	// iparam1 is the mixture's colour
	if ( args->iparam1 == 0 )
		iSprite = gEngfuncs.pEventAPI->EV_FindModelIndex( "sprites/greensplash.spr" );
	else if ( args->iparam1 == 3 )
		iSprite = gEngfuncs.pEventAPI->EV_FindModelIndex( "sprites/redsplash.spr" );
	else if ( args->iparam1 == 2 )
		iSprite = gEngfuncs.pEventAPI->EV_FindModelIndex( "sprites/orangesplash.spr" );
	else
		iSprite = gEngfuncs.pEventAPI->EV_FindModelIndex( "sprites/yellowsplash.spr" );

	idx = args->entindex;
	VectorCopy( args->origin, origin );
	VectorCopy( args->velocity, velocity );

	gEngfuncs.pEventAPI->EV_SetUpPlayerPrediction( false, true );
	gEngfuncs.pEventAPI->EV_PushPMStates();
	gEngfuncs.pEventAPI->EV_SetSolidPlayers ( idx - 1 );	
	gEngfuncs.pEventAPI->EV_SetTraceHull( 2 );

	gEngfuncs.pEventAPI->EV_PlayerTrace( origin, velocity * 10, PM_STUDIO_BOX, -1, &tr );

	gEngfuncs.pEventAPI->EV_PopPMStates();

	if ( !tr.allsolid )
		gEngfuncs.pEfxAPI->R_Sprite_Spray( origin, tr.plane.normal, iSprite, 10, 50, 100 );
}

void EV_ClusterExplosion( event_args_t *args )
{
	int idx;
	vec3_t origin;
	vec3_t angles;
	vec3_t velocity;
	vec3_t up, right, forward;
	pmtrace_t tr;
	int iExplode;

	iExplode = gEngfuncs.pEventAPI->EV_FindModelIndex( "sprites/kaboom.spr" );

	idx = args->entindex;
	VectorCopy( args->origin, origin );
	VectorCopy( args->angles, angles );
	VectorCopy( args->velocity, velocity );

	gEngfuncs.pEventAPI->EV_SetUpPlayerPrediction( false, true );
	gEngfuncs.pEventAPI->EV_PushPMStates();
	gEngfuncs.pEventAPI->EV_SetSolidPlayers ( idx - 1 );	
	gEngfuncs.pEventAPI->EV_SetTraceHull( 2 );

	gEngfuncs.pEventAPI->EV_PlayerTrace( origin, velocity * 10, PM_STUDIO_BOX, -1, &tr );

	gEngfuncs.pEventAPI->EV_PopPMStates();

	gEngfuncs.pEfxAPI->R_Explosion( origin, iExplode, 3, 15, TE_EXPLFLAG_NOSOUND | TE_EXPLFLAG_NOPARTICLES );

	// bparam1 is the manta's carpet bombing
	if ( !args->bparam1 )
	{
		switch ( gEngfuncs.pfnRandomLong( 0, 2 ) )
		{
		case 0:
			gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "weapons/kaboom1.wav", 1.0, ATTN_NORM, 0, 85 + gEngfuncs.pfnRandomLong( 0, 0x1f ) );
			break;
		case 1:
			gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "weapons/kaboom2.wav", 1.0, ATTN_NORM, 0, 85 + gEngfuncs.pfnRandomLong( 0, 0x1f ) );
			break;
		case 2:
			gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "weapons/kaboom3.wav", 1.0, ATTN_NORM, 0, 85 + gEngfuncs.pfnRandomLong( 0, 0x1f ) );
			break;
		}
	}
	else
	{
		switch ( gEngfuncs.pfnRandomLong( 0, 2 ) )
		{
		case 0:
			gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "manta/carpetbomb1.wav", 1.0, ATTN_NORM, 0, PITCH_NORM );
			break;
		case 1:
			gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "manta/carpetbomb2.wav", 1.0, ATTN_NORM, 0, PITCH_NORM );
			break;
		case 2:
			gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "manta/carpetbomb3.wav", 1.0, ATTN_NORM, 0, PITCH_NORM );
			break;
		}
	}

	AngleVectors( angles, forward, right, up );

	g_Effects.Add( CEffect( 90, origin, Vector( 0, 0, 0 ), 3, EFFECT_SPHERE, 240, 10, gEngfuncs.GetClientTime(), NULL ) );
}

void EV_SphereExplode( event_args_t *args )
{
	vec3_t origin;
	vec3_t angles;
	vec3_t up, right, forward;
	vec3_t vecVelocity;
	float flRadius = args->fparam1;
	float flSpeed = args->fparam2;
	float flBrightness = args->iparam1;
	float flFade = args->iparam2;
	int fTracers = args->bparam1;
	int fImplode = args->bparam2;
	int i, iLength;

	VectorCopy( args->origin, origin );

	g_Effects.Add( CEffect( flRadius, origin, Vector( 0, 0, 0 ), flSpeed, EFFECT_SPHERE, flBrightness, flFade, gEngfuncs.GetClientTime(), NULL ) );

	AngleVectors( angles, forward, right, up );

	if ( fTracers )
	{
		for ( i = 0; i < 30; i++ )
		{
			vecVelocity = forward * gEngfuncs.pfnRandomFloat( -600, 600 ) + right * gEngfuncs.pfnRandomFloat( -600, 600 ) + up * gEngfuncs.pfnRandomFloat( -600, 600 );

			iLength = abs( (int)vecVelocity.Length() );
			if ( iLength >= 100 )
				gEngfuncs.pEfxAPI->R_UserTracerParticle( origin, vecVelocity, flRadius * 2.5 / iLength, 5, 3, 0, NULL );
		}
	}

	if ( fImplode )
		gEngfuncs.pEfxAPI->R_Implosion( origin, flRadius * 1.4, 70, 1 );
}

void EV_Lightning( event_args_t *args )
{
	int idx;
	vec3_t origin;
	vec3_t angles;
	vec3_t vecSrc, vecDest, vecEnd;
	vec3_t up, right, forward;
	pmtrace_t tr;
	int iBeam;

	idx = args->entindex;
	VectorCopy( args->origin, origin );
	VectorCopy( args->angles, angles );

	EV_GetGunPosition( args, vecSrc, origin );

	iBeam = gEngfuncs.pEventAPI->EV_FindModelIndex( "sprites/zbeam5.spr" );

	AngleVectors( angles, forward, right, up );
	VectorMA( vecSrc, 8192, forward, vecDest );

	gEngfuncs.pEventAPI->EV_SetUpPlayerPrediction( false, true );
	gEngfuncs.pEventAPI->EV_PushPMStates();
	gEngfuncs.pEventAPI->EV_SetSolidPlayers ( idx - 1 );	
	gEngfuncs.pEventAPI->EV_SetTraceHull( 2 );

	gEngfuncs.pEventAPI->EV_PlayerTrace( vecSrc, vecDest, PM_STUDIO_BOX, -1, &tr );

	gEngfuncs.pEventAPI->EV_PopPMStates();

	gEngfuncs.pEventAPI->EV_GetPhysent( tr.ent );

	VectorCopy( tr.endpos, vecEnd );
	gEngfuncs.pEfxAPI->R_BeamEntPoint( idx | 0x1000, vecEnd, iBeam, 0.1, 2.5, 0.25, 200, 0, 0, 10, 1, 1, 1 );
}

void EV_RocketSmoke( event_args_t *args )
{
	vec3_t origin;
	vec3_t angles;
	vec3_t up, right, forward;

	VectorCopy( args->origin, origin );
	VectorCopy( args->angles, angles );

	AngleVectors( angles, forward, right, up );

	g_Effects.Add( CEffect( 16, origin, forward, 2.4, EFFECT_RING, 255, 15, gEngfuncs.GetClientTime(), (struct model_s *)gEngfuncs.GetSpritePointer( SPR_Load( "sprites/firebeam.spr" ) ) ) );
}

void EV_Cylinder( event_args_t *args )
{
	vec3_t origin;
	vec3_t angles;
	vec3_t up, right, forward;

	VectorCopy( args->origin, origin );
	VectorCopy( args->angles, angles );

	AngleVectors( angles, forward, right, up );

	g_Effects.Add( CEffect( 10, origin, forward, 0.2, EFFECT_RING, 220, 4, gEngfuncs.GetClientTime(), (struct model_s *)gEngfuncs.GetSpritePointer( SPR_Load( "sprites/smoke.spr" ) ) ) );
}

void EV_VolumeExplosion( event_args_t *args )
{
}

void EV_WaterRipple( event_args_t *args )
{
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
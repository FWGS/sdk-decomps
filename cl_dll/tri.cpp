// Triangle rendering, if any

#include "hud.h"
#include "cl_util.h"

// Triangle rendering apis are in gEngfuncs.pTriAPI

#include "const.h"
#include "entity_state.h"
#include "cl_entity.h"
#include "triangleapi.h"
#include "effectlist.h"

#define DLLEXPORT __declspec( dllexport )

extern "C"
{
	void DLLEXPORT HUD_DrawNormalTriangles( void );
	void DLLEXPORT HUD_DrawTransparentTriangles( void );
};

#include <math.h>

#ifndef M_PI
#define M_PI		3.14159265358979323846	// matches value in gcc v2 math.h
#endif

void VectorAngles( const float *forward, float *angles );

// the sphere mesh the sphere effects are drawn with: two poles and
// SPHERE_STACKS rings of SPHERE_SLICES points
#define SPHERE_SLICES		20
#define SPHERE_STACKS		10
#define SPHERE_POINTS		( 2 + SPHERE_SLICES * SPHERE_STACKS )
#define SPHERE_TRIS			( SPHERE_SLICES * 2 * SPHERE_STACKS )

// names ours, picked so that VC6's .bss hash puts the points before the
// triangle indexes and the texture coordinates after them, as retail has them
vec3_t g_vecSphereVertex[ SPHERE_POINTS ];
int g_iSphereTris[ SPHERE_TRIS * 3 ];
vec3_t g_vecSphereTexCoord[ SPHERE_POINTS ];

CEffectList g_ClientEffects;	// name ours

// retail defines two more vectors here that nothing reads (names ours)
vec3_t g_vecUnknownA;
vec3_t g_vecUnknown0;

// and an initialised float nothing reads, the first of tri.obj's data
// (1.0, before the sprite names; name ours)
float g_flUnused = 1.0;

// sic: retail's tri.obj also has a 12-byte and a 16-byte slot that nothing
// reads or writes and no constructor runs for (names and types ours);
// g_flLastTime and InitSphere's bInit are zero-initialised, after them, as
// in retail
static int s_iUnknown_1[3];
static int s_iSpare[4];

static float g_flLastTime = 0;

void InitSphere( void );

//#define TEST_IT
#if defined( TEST_IT )

/*
=================
Draw_Triangles

Example routine.  Draws a sprite offset from the player origin.
=================
*/
void Draw_Triangles( void )
{
	cl_entity_t *player;
	vec3_t org;

	// Load it up with some bogus data
	player = gEngfuncs.GetLocalPlayer();
	if ( !player )
		return;

	org = player->origin;

	org.x += 50;
	org.y += 50;

	if (gHUD.m_hsprCursor == 0)
	{
		char sz[256];
		sprintf( sz, "sprites/cursor.spr" );
		gHUD.m_hsprCursor = SPR_Load( sz );
	}

	if ( !gEngfuncs.pTriAPI->SpriteTexture( (struct model_s *)gEngfuncs.GetSpritePointer( gHUD.m_hsprCursor ), 0 ))
	{
		return;
	}
	
	// Create a triangle, sigh
	gEngfuncs.pTriAPI->RenderMode( kRenderNormal );
	gEngfuncs.pTriAPI->CullFace( TRI_NONE );
	gEngfuncs.pTriAPI->Begin( TRI_QUADS );
	// Overload p->color with index into tracer palette, p->packedColor with brightness
	gEngfuncs.pTriAPI->Color4f( 1.0, 1.0, 1.0, 1.0 );
	// UNDONE: This gouraud shading causes tracers to disappear on some cards (permedia2)
	gEngfuncs.pTriAPI->Brightness( 1 );
	gEngfuncs.pTriAPI->TexCoord2f( 0, 0 );
	gEngfuncs.pTriAPI->Vertex3f( org.x, org.y, org.z );

	gEngfuncs.pTriAPI->Brightness( 1 );
	gEngfuncs.pTriAPI->TexCoord2f( 0, 1 );
	gEngfuncs.pTriAPI->Vertex3f( org.x, org.y + 50, org.z );

	gEngfuncs.pTriAPI->Brightness( 1 );
	gEngfuncs.pTriAPI->TexCoord2f( 1, 1 );
	gEngfuncs.pTriAPI->Vertex3f( org.x + 50, org.y + 50, org.z );

	gEngfuncs.pTriAPI->Brightness( 1 );
	gEngfuncs.pTriAPI->TexCoord2f( 1, 0 );
	gEngfuncs.pTriAPI->Vertex3f( org.x + 50, org.y, org.z );

	gEngfuncs.pTriAPI->End();
	gEngfuncs.pTriAPI->RenderMode( kRenderNormal );
}

#endif

/*
=================
DrawSphere

Draws the sphere mesh around origin. The callers pass 0 and 400 for the
two unused parameters
=================
*/
void DrawSphere( int iUnused, Vector &origin, float flRadius, float flUnused, float *color, float flAlpha )
{
	if ( flRadius <= 0 )
		return;

	gEngfuncs.pTriAPI->RenderMode( kRenderTransAdd );
	gEngfuncs.pTriAPI->CullFace( TRI_NONE );
	gEngfuncs.pTriAPI->SpriteTexture( (struct model_s *)gEngfuncs.GetSpritePointer( SPR_Load( "sprites/fthrower.spr" ) ), 4 );
	gEngfuncs.pTriAPI->Begin( TRI_TRIANGLES );
	gEngfuncs.pTriAPI->Color4f( color[0] / 255.0, color[1] / 255.0, color[2] / 255.0, flAlpha / 255.0 );

	int *pTris = g_iSphereTris;

	for ( int i = 0; i < SPHERE_TRIS; i++ )
	{
		int *pTri = pTris;
		pTris += 3;

		for ( int j = 0; j < 3; j++ )
		{
			int iPoint = *pTri;
			Vector vecPoint;
			vecPoint = g_vecSphereVertex[ iPoint ];

			gEngfuncs.pTriAPI->TexCoord2f( g_vecSphereTexCoord[ iPoint ].x, g_vecSphereTexCoord[ iPoint ].z );
			gEngfuncs.pTriAPI->Vertex3fv( vecPoint * flRadius + origin );
			pTri++;
		}
	}

	gEngfuncs.pTriAPI->End();
	gEngfuncs.pTriAPI->RenderMode( kRenderNormal );
}

/*
=================
DrawStrip

A twisting strip along dir, flLength long and flWidth wide
=================
*/
void DrawStrip( Vector &origin, Vector &dir, float flWidth, float flLength, int iSegments, float flBrightness )
{
	vec3_t angles, forward, right, up;
	vec3_t vecPoint[2];
	float flStep;
	float v;

	if ( iSegments < 3 )
		return;

	gEngfuncs.pTriAPI->RenderMode( kRenderTransAdd );
	gEngfuncs.pTriAPI->CullFace( TRI_NONE );
	gEngfuncs.pTriAPI->Begin( TRI_QUAD_STRIP );
	gEngfuncs.pTriAPI->Color4f( 1, 1, 1, flBrightness / 255 );

	flStep = 1.0 / iSegments;

	for ( int i = 0; i <= iSegments; i++ )
	{
		v = i * flStep;

		VectorAngles( dir, angles );
		angles[2] = fmod( v * 360 + angles[2], 360 );
		AngleVectors( angles, forward, right, up );

		vecPoint[0][0] = forward[0] * flLength + origin[0] + ( -( up[0] * flWidth ) );
		vecPoint[0][1] = forward[1] * flLength + ( -( up[1] * flWidth ) ) + origin[1];
		vecPoint[0][2] = ( -( forward[2] * flLength ) ) + origin[2] + up[2] * flWidth;

		vecPoint[1][0] = ( -( up[0] * flWidth ) ) + origin[0];
		vecPoint[1][1] = ( -( up[1] * flWidth ) ) + origin[1];
		vecPoint[1][2] = up[2] * flWidth + origin[2];

		gEngfuncs.pTriAPI->TexCoord2f( 0, v );
		gEngfuncs.pTriAPI->Vertex3fv( vecPoint[0] );
		gEngfuncs.pTriAPI->TexCoord2f( 1, v );
		gEngfuncs.pTriAPI->Vertex3fv( vecPoint[1] );
	}

	gEngfuncs.pTriAPI->End();
	gEngfuncs.pTriAPI->RenderMode( kRenderNormal );
}

/*
=================
DrawFlameFan

A cone of flame, flScale long, pointing back along dir
=================
*/
void DrawFlameFan( Vector &origin, Vector &dir, float flScale, int iSegments, float flBrightness, int iType )
{
	vec3_t angles, forward, right, up;
	vec3_t vecPoint, vecTip, vecLast;
	float flStep, flLast;
	float v;
	int iWidth;

	if ( iSegments < 2 )
		return;

	gEngfuncs.pTriAPI->RenderMode( kRenderTransAdd );
	gEngfuncs.pTriAPI->CullFace( TRI_NONE );
	gEngfuncs.pTriAPI->Begin( TRI_TRIANGLE_FAN );
	gEngfuncs.pTriAPI->Color4f( 1, 1, 1, flBrightness / 255 );

	flStep = 1.0 / iSegments;

	for ( int i = 0; i <= iSegments; i++ )
	{
		v = i * flStep;

		VectorAngles( dir, angles );
		angles[2] = fmod( v * 360.0 + angles[2], 360 );
		AngleVectors( angles, forward, right, up );

		if ( iType == EFFECT_FLAME )
			iWidth = 2;
		else
			iWidth = 1;

		vecPoint[0] = ( -( iWidth * up[0] * flScale ) ) + origin[0];
		vecPoint[1] = ( -( iWidth * up[1] * flScale ) ) + origin[1];
		vecPoint[2] = iWidth * up[2] * flScale + origin[2];

		vecTip[0] = origin[0] - forward[0] * flScale;
		vecTip[1] = origin[1] - forward[1] * flScale;
		vecTip[2] = origin[2] + forward[2] * flScale;

		if ( i )
		{
			gEngfuncs.pTriAPI->TexCoord2f( 0, v );
			gEngfuncs.pTriAPI->Vertex3fv( vecTip );
			gEngfuncs.pTriAPI->TexCoord2f( 1, flLast );
			gEngfuncs.pTriAPI->Vertex3fv( vecLast );
			gEngfuncs.pTriAPI->TexCoord2f( 1, v );
			gEngfuncs.pTriAPI->Vertex3fv( vecPoint );
		}

		VectorCopy( vecPoint, vecLast );
		flLast = v;
	}

	gEngfuncs.pTriAPI->End();
	gEngfuncs.pTriAPI->RenderMode( kRenderNormal );
}

/*
=================
DrawShocks

EFFECT_SHOCK: a growing gauss shock sprite flat on the surface
=================
*/
void DrawShocks( void )
{
	float flTime = gEngfuncs.GetClientTime();

	for ( int i = 0; i < g_ClientEffects.m_iCount; i++ )
	{
		if ( g_ClientEffects[i].m_iType != EFFECT_SHOCK )
			continue;

		float flBrightness = g_ClientEffects[i].m_flBrightness - g_ClientEffects[i].m_flFadeSpeed * ( flTime - g_ClientEffects[i].m_flStartTime ) * 30;
		float flScale = g_ClientEffects[i].m_flScale + g_ClientEffects[i].m_flScaleSpeed * ( flTime - g_ClientEffects[i].m_flStartTime ) * 30;

		if ( flBrightness < 1 )
			flBrightness = 1;

		if ( g_ClientEffects[i].m_flMaxScale < flScale )
		{
			g_ClientEffects.Remove( g_ClientEffects[i].m_iId );
			continue;
		}

		vec3_t vecOrigin, vecAngles, angles, forward, right, up;
		vec3_t vecPoint2, vecPoint3, vecPoint4;

		gEngfuncs.pTriAPI->SpriteTexture( (struct model_s *)gEngfuncs.GetSpritePointer( SPR_Load( "sprites/gaussshock.spr" ) ), 0 );

		vecOrigin = g_ClientEffects[i].m_vecOrigin;
		vec3_t vecNormal = g_ClientEffects[i].m_vecAngles;
		VectorAngles( vecNormal, vecAngles );
		angles = vecAngles;
		AngleVectors( angles, forward, right, up );

		// center the quad on the spot, a little off the surface
		vecOrigin = vecOrigin + vecNormal * 5;
		vecOrigin = vecOrigin + right * -flScale * 0.5;
		vecOrigin.x = up.x * flScale / 2 + vecOrigin.x;
		vecOrigin.y = up.y * flScale / 2 + vecOrigin.y;
		vecOrigin[2] = vecOrigin.z - up.z * flScale / 2;

		vecPoint2 = vecOrigin + flScale * right;

		vecPoint3.x = ( right.x - up.x ) * flScale + vecOrigin.x;
		vecPoint3.y = ( right.y - up.y ) * flScale + vecOrigin.y;
		vecPoint3.z = ( right.z + up.z ) * flScale + vecOrigin.z;

		vecPoint4.x = vecOrigin.x - up.x * flScale;
		vecPoint4.y = vecOrigin.y - up.y * flScale;
		vecPoint4.z = vecOrigin.z + up.z * flScale;

		gEngfuncs.pTriAPI->RenderMode( kRenderTransAdd );
		gEngfuncs.pTriAPI->CullFace( TRI_FRONT );
		gEngfuncs.pTriAPI->Begin( TRI_QUADS );
		gEngfuncs.pTriAPI->Color4f( 1, 1, 1, flBrightness / 255 );

		gEngfuncs.pTriAPI->TexCoord2f( 0, 0 );
		gEngfuncs.pTriAPI->Vertex3f( vecOrigin.x, vecOrigin.y, vecOrigin.z );
		gEngfuncs.pTriAPI->TexCoord2f( 0, 1 );
		gEngfuncs.pTriAPI->Vertex3f( vecPoint2.x, vecPoint2.y, vecPoint2.z );
		gEngfuncs.pTriAPI->TexCoord2f( 1, 1 );
		gEngfuncs.pTriAPI->Vertex3f( vecPoint3.x, vecPoint3.y, vecPoint3.z );
		gEngfuncs.pTriAPI->TexCoord2f( 1, 0 );
		gEngfuncs.pTriAPI->Vertex3f( vecPoint4.x, vecPoint4.y, vecPoint4.z );

		gEngfuncs.pTriAPI->End();
		gEngfuncs.pTriAPI->RenderMode( kRenderNormal );
	}
}

/*
=================
DrawFlameStrip

Like DrawFlameFan, for EFFECT_FLAME3
=================
*/
void DrawFlameStrip( float *origin, Vector &dir, float flScale, int iSegments, float flBrightness )
{
	vec3_t angles, forward, right, up;
	vec3_t vecPoint, vecTip, vecLast;
	float flStep, flLast;
	float v;

	if ( iSegments < 2 )
		return;

	gEngfuncs.pTriAPI->RenderMode( kRenderTransAdd );
	gEngfuncs.pTriAPI->CullFace( TRI_NONE );
	gEngfuncs.pTriAPI->Begin( TRI_QUAD_STRIP );
	gEngfuncs.pTriAPI->Color4f( 1, 1, 1, flBrightness / 255 );

	flStep = 1.0 / iSegments;

	for ( int i = 0; i <= iSegments; i++ )
	{
		v = i * flStep;

		VectorAngles( dir, angles );
		angles[2] = fmod( v * 360.0 + angles[2], 360 );
		AngleVectors( angles, forward, right, up );

		vecPoint[0] = ( -( up[0] * flScale ) ) + origin[0];
		vecPoint[1] = ( -( up[1] * flScale ) ) + origin[1];
		vecPoint[2] = up[2] * flScale + origin[2];

		vecTip[0] = origin[0] - forward[0] * flScale * -6;
		vecTip[1] = origin[1] - forward[1] * flScale * -6;
		vecTip[2] = origin[2] - forward[2] * flScale * 6;

		if ( i )
		{
			gEngfuncs.pTriAPI->TexCoord2f( 0, v );
			gEngfuncs.pTriAPI->Vertex3fv( vecTip );
			gEngfuncs.pTriAPI->TexCoord2f( 1, flLast );
			gEngfuncs.pTriAPI->Vertex3fv( vecLast );
			gEngfuncs.pTriAPI->TexCoord2f( 1, v );
			gEngfuncs.pTriAPI->Vertex3fv( vecPoint );
		}

		VectorCopy( vecPoint, vecLast );
		flLast = v;
	}

	gEngfuncs.pTriAPI->End();
	gEngfuncs.pTriAPI->RenderMode( kRenderNormal );
}

/*
=================
DrawEffect8

EFFECT_8: a strip that turns a little each frame
=================
*/
void DrawEffect8( void )
{
	float flTime = gEngfuncs.GetClientTime();

	for ( int i = 0; i < g_ClientEffects.m_iCount; i++ )
	{
		CEffect effect = g_ClientEffects[i];

		if ( effect.m_iType != EFFECT_8 )
			continue;

		vec3_t angles, forward, right, up;
		float flBrightness = effect.m_flBrightness;
		float flWidth = effect.m_flMaxScale;
		vec3_t vecDir;
		vecDir = effect.m_vecAngles;

		VectorAngles( vecDir, angles );
		AngleVectors( angles, forward, right, up );

		// sic: turns the copy, not the effect in the list
		vecDir = Vector( vecDir + right * 17 ).Normalize();
		effect.m_vecAngles = vecDir;

		if ( flBrightness < 1 )
			flBrightness = 1;

		if ( effect.m_pSprite )
		{
			gEngfuncs.pTriAPI->SpriteTexture( effect.m_pSprite, 0 );
			// sic: the direction goes through a float * and back, which
			// generates no code; the one inline operator float *() it costs
			// is what leaves Normalize's non-zero Vector( float, float, float )
			// out of line, as in retail
			DrawStrip( effect.m_vecOrigin, *(Vector *)(float *)vecDir, flWidth, 8, 32, flBrightness );
		}
	}
}

/*
=================
DrawBeams

EFFECT_BEAM: a strip that flies out of entity m_iRed
=================
*/
void DrawBeams( void )
{
	float flTime = gEngfuncs.GetClientTime();

	for ( int i = 0; i < g_ClientEffects.m_iCount; i++ )
	{
		CEffect effect = g_ClientEffects[i];

		if ( effect.m_iType != EFFECT_BEAM )
			continue;

		float flBrightness = effect.m_flBrightness - ( flTime - effect.m_flStartTime ) * effect.m_flFadeSpeed * 30;
		float flWidth = effect.m_flScale + 8;
		cl_entity_t *pEntity = gEngfuncs.GetEntityByIndex( effect.m_iRed );

		if ( !pEntity )
		{
			g_ClientEffects.Remove( effect.m_iId );
			continue;
		}

		vec3_t vecOrigin = effect.m_vecAngles * ( ( flTime - effect.m_flStartTime ) * effect.m_flMaxScale * 30 ) + pEntity->curstate.origin;

		if ( flBrightness < 1 )
			flBrightness = 1;

		if ( flBrightness < 5 )
		{
			g_ClientEffects.Remove( effect.m_iId );
		}
		else if ( effect.m_pSprite )
		{
			gEngfuncs.pTriAPI->SpriteTexture( effect.m_pSprite, 0 );
			DrawStrip( vecOrigin, effect.m_vecAngles, flWidth, 4.5, 32, flBrightness );
		}
	}
}

/*
=================
DrawRings

EFFECT_RING
=================
*/
void DrawRings( void )
{
	float flTime = gEngfuncs.GetClientTime();

	for ( int i = 0; i < g_ClientEffects.m_iCount; i++ )
	{
		CEffect effect = g_ClientEffects[i];

		if ( effect.m_iType == EFFECT_RING )
		{
			float flBrightness = effect.m_flBrightness - ( flTime - effect.m_flStartTime ) * effect.m_flFadeSpeed * 30;
			float flScale = ( flTime - effect.m_flStartTime ) * effect.m_flScaleSpeed * 30 + effect.m_flScale + 7;

			if ( flBrightness < 1 )
				flBrightness = 1;

			if ( flBrightness < 5 )
			{
				g_ClientEffects.Remove( effect.m_iId );
			}
			else if ( effect.m_pSprite )
			{
				gEngfuncs.pTriAPI->SpriteTexture( effect.m_pSprite, 0 );
				DrawStrip( effect.m_vecOrigin, effect.m_vecAngles, flScale, 8, 32, flBrightness );
			}
		}
	}
}

/*
=================
DrawSprites

EFFECT_SPRITE: an animated sprite flat on the surface. m_iRed is the
frame rate, m_iGreen the frame count and m_iBlue, if set, how many
frames to play
=================
*/
void DrawSprites( void )
{
	float flTime = gEngfuncs.GetClientTime();

	for ( int i = 0; i < g_ClientEffects.m_iCount; i++ )
	{
		CEffect effect = g_ClientEffects[i];

		if ( effect.m_iType != EFFECT_SPRITE )
			continue;

		float flBrightness = 255 - ( flTime - effect.m_flStartTime ) * effect.m_flFadeSpeed * 30;
		float flFrames = effect.m_iRed * ( flTime - effect.m_flStartTime );
		float flScale;

		if ( effect.m_flScaleSpeed )
			flScale = ( flTime - effect.m_flStartTime ) * effect.m_flScaleSpeed * 30 + effect.m_flBrightness + 1;
		else
			flScale = effect.m_flMaxScale;

		float flFrame = flFrames;
		float flNumFrames = effect.m_iGreen;
		while ( flFrame >= flNumFrames )
			flFrame -= flNumFrames;

		if ( flBrightness < 1 )
			flBrightness = 1;

		if ( effect.m_iBlue )
		{
			if ( flFrames > effect.m_iBlue )
			{
				g_ClientEffects.Remove( effect.m_iId );
				continue;
			}
		}
		else if ( flBrightness < 4 )
		{
			g_ClientEffects.Remove( effect.m_iId );
			continue;
		}

		if ( !effect.m_pSprite )
			continue;

		vec3_t vecOrigin, vecAngles, angles, forward, right, up;
		vec3_t vecPoint2, vecPoint3, vecPoint4;

		gEngfuncs.pTriAPI->SpriteTexture( effect.m_pSprite, (int)flFrame );

		vecOrigin = g_ClientEffects[i].m_vecOrigin;
		vec3_t vecNormal = g_ClientEffects[i].m_vecAngles;
		VectorAngles( vecNormal, vecAngles );
		angles = vecAngles;
		AngleVectors( angles, forward, right, up );

		vecOrigin = vecOrigin + vecNormal * 1;
		vecOrigin = vecOrigin + right * -flScale * 0.5;
		vecOrigin.x = up.x * flScale / 2 + vecOrigin.x;
		vecOrigin.y = up.y * flScale / 2 + vecOrigin.y;
		vecOrigin.z = vecOrigin.z - up.z * flScale / 2;

		vecPoint2 = vecOrigin + right * flScale;

		vecPoint3.x = ( right.x - up.x ) * flScale + vecOrigin.x;
		vecPoint3.y = ( right.y - up.y ) * flScale + vecOrigin.y;
		vecPoint3.z = ( right.z + up.z ) * flScale + vecOrigin.z;

		vecPoint4.x = vecOrigin.x - up.x * flScale;
		vecPoint4.y = vecOrigin.y - up.y * flScale;
		vecPoint4[2] = vecOrigin.z + up.z * flScale;

		gEngfuncs.pTriAPI->RenderMode( kRenderTransAdd );
		gEngfuncs.pTriAPI->CullFace( TRI_FRONT );
		gEngfuncs.pTriAPI->Begin( TRI_QUADS );
		gEngfuncs.pTriAPI->Color4f( 1, 1, 1, flBrightness / 255 );

		gEngfuncs.pTriAPI->TexCoord2f( 0, 0 );
		gEngfuncs.pTriAPI->Vertex3f( vecOrigin.x, vecOrigin.y, vecOrigin.z );
		gEngfuncs.pTriAPI->TexCoord2f( 0, 1 );
		gEngfuncs.pTriAPI->Vertex3f( vecPoint2.x, vecPoint2.y, vecPoint2.z );
		gEngfuncs.pTriAPI->TexCoord2f( 1, 1 );
		gEngfuncs.pTriAPI->Vertex3f( vecPoint3.x, vecPoint3.y, vecPoint3.z );
		gEngfuncs.pTriAPI->TexCoord2f( 1, 0 );
		gEngfuncs.pTriAPI->Vertex3f( vecPoint4.x, vecPoint4.y, vecPoint4.z );

		gEngfuncs.pTriAPI->End();
		gEngfuncs.pTriAPI->RenderMode( kRenderNormal );
	}
}

/*
=================
CEffectNode::Previous

The node before the one with this id
=================
*/
inline CEffectNode *CEffectNode::Previous( int iId )
{
	if ( !m_pNext )
		return NULL;
	if ( m_pNext->m_Effect.m_iId == iId )
		return this;
	return m_pNext->Previous( iId );
}

/*
=================
DrawFlames

EFFECT_FLAME, EFFECT_FLAME2 and EFFECT_FLAME3
=================
*/
void DrawFlames( void )
{
	float flTime = gEngfuncs.GetClientTime();

	for ( int i = 0; i < g_ClientEffects.m_iCount; i++ )
	{
		CEffect effect = g_ClientEffects[i];

		if ( effect.m_iType != EFFECT_FLAME && effect.m_iType != EFFECT_FLAME2 && effect.m_iType != EFFECT_FLAME3 )
			continue;

		float flBrightness = effect.m_flBrightness - ( flTime - effect.m_flStartTime ) * effect.m_flFadeSpeed * 30;
		float flScale = ( flTime - effect.m_flStartTime ) * effect.m_flScaleSpeed * 30 + effect.m_flScale;

		// EFFECT_FLAME3 flies off along its direction
		vec3_t vecOrigin = effect.m_vecOrigin + effect.m_vecAngles * 50 * 30 * ( flTime - effect.m_flStartTime );

		if ( flBrightness < 1 )
			flBrightness = 1;

		if ( flScale > effect.m_flMaxScale )
		{
			// sic: called through a pointer, which VC6 turns into a plain
			// call of the out-of-line CEffectList::Remove; retail calls it
			// here and inlines the Vector operators after it as if the call
			// were no inline candidate at all, which a direct call is (it
			// is inlined, or takes the operators' inline budget)
			bool ( CEffectList::*pfnRemove )( int ) = &CEffectList::Remove;
			( g_ClientEffects.*pfnRemove )( effect.m_iId );
		}
		else if ( effect.m_pSprite )
		{
			gEngfuncs.pTriAPI->SpriteTexture( effect.m_pSprite, 0 );

			// sic: the first test is redundant. flScale * 1 folds to
			// flScale, but as an expression VC6 loads it once and multiplies
			// each component by it on the x87 stack, as retail does; the
			// plain variable would be used from memory
			if ( effect.m_iType == EFFECT_FLAME || effect.m_iType != EFFECT_FLAME3 )
				DrawFlameFan( effect.m_vecOrigin + effect.m_vecAngles * ( flScale * 1 ), effect.m_vecAngles, flScale, 16, flBrightness, effect.m_iType );
			else
				DrawFlameStrip( vecOrigin, effect.m_vecAngles, flScale, 16, flBrightness );
		}
	}
}

/*
=================
CEffectList::Remove
=================
*/
inline bool CEffectList::Remove( int iId )
{
	CEffectNode *pNode;

	if ( m_pHead->m_Effect.m_iId == iId )
	{
		pNode = m_pHead;
		m_pHead = m_pHead->m_pNext;
		m_iCount--;
		delete pNode;
		return true;
	}

	CEffectNode *pPrev = m_pHead->Previous( iId );
	if ( !pPrev )
		return false;

	pNode = pPrev->m_pNext;
	pPrev->m_pNext = pNode->m_pNext;
	m_iCount--;
	delete pNode;
	return true;
}

/*
=================
DrawSpheres

EFFECT_SPHERE
=================
*/
void DrawSpheres( void )
{
	float flTime = gEngfuncs.GetClientTime();

	for ( int i = 0; i < g_ClientEffects.m_iCount; i++ )
	{
		CEffect effect = g_ClientEffects[i];

		if ( effect.m_iType == EFFECT_SPHERE )
		{
			float flBrightness = g_ClientEffects[i].m_flBrightness - g_ClientEffects[i].m_flFadeSpeed * ( flTime - g_ClientEffects[i].m_flStartTime ) * 30;
			float flScale = g_ClientEffects[i].m_flScale + g_ClientEffects[i].m_flScaleSpeed * ( flTime - g_ClientEffects[i].m_flStartTime ) * 30;

			if ( flScale > effect.m_flMaxScale )
			{
				g_ClientEffects.Remove( effect.m_iId );
			}
			else
			{
				if ( flBrightness < 1 )
					flBrightness = 1;

				Vector color( 255, 255, 255 );
				DrawSphere( 0, effect.m_vecOrigin, flScale, 400, color, flBrightness );
			}
		}
	}
}

/*
=================
HUD_DrawNormalTriangles

Non-transparent triangles-- add them here
=================
*/
void DLLEXPORT HUD_DrawNormalTriangles( void )
{

#if defined( TEST_IT )
//	Draw_Triangles();
#endif
}

/*
=================
HUD_DrawTransparentTriangles

Render any triangles with transparent rendermode needs here
=================
*/
void DLLEXPORT HUD_DrawTransparentTriangles( void )
{

#if defined( TEST_IT )
//	Draw_Triangles();
#endif

	InitSphere();

	DrawSpheres();
	DrawShocks();
	DrawFlames();
	DrawRings();
	DrawBeams();
	DrawEffect8();
	DrawSprites();

	g_flLastTime = gEngfuncs.GetClientTime();
}

/*
=================
InitSphere

Builds the sphere mesh the first time it is needed
=================
*/
void InitSphere( void )
{
	static bool bInit = false;
	int i, j;

	if ( bInit )
		return;

	g_vecSphereVertex[0] = Vector( 0, 1, 0 );
	g_vecSphereVertex[1] = Vector( 0, -1, 0 );

	for ( i = 0; i < SPHERE_STACKS; i++ )
	{
		float flPhi = ( i + 1 ) * ( (float)M_PI / ( SPHERE_STACKS + 1 ) );
		float flY = cos( flPhi );

		for ( j = 0; j < SPHERE_SLICES; j++ )
		{
			int iPoint = 2 + i * SPHERE_SLICES + j;
			float flTheta = j * ( (float)M_PI / ( SPHERE_SLICES / 2 ) );
			float flX, flZ;

			g_vecSphereVertex[ iPoint ].y = flY;
			g_vecSphereVertex[ iPoint ].x = flX = sin( flTheta ) * sin( flPhi );
			g_vecSphereVertex[ iPoint ].z = flZ = cos( flTheta ) * sin( flPhi );

			g_vecSphereTexCoord[ iPoint ].x = 1.0 - ( flX + 1.0 ) * 0.5;
			g_vecSphereTexCoord[ iPoint ].y = 1.0 - ( (float)cos( flPhi ) + 1.0 ) * 0.5;
			g_vecSphereTexCoord[ iPoint ].z = 1.0 - ( flZ + 1.0 ) * 0.5;
		}
	}

	// the top cap
	for ( i = 0; i < SPHERE_SLICES; i++ )
	{
		g_iSphereTris[ i * 3 ] = 0;
		g_iSphereTris[ i * 3 + 1 ] = i + 2;
		g_iSphereTris[ i * 3 + 2 ] = i + 3;
		if ( i == SPHERE_SLICES - 1 )
			g_iSphereTris[ i * 3 + 2 ] = 2;
	}

	// the bands between the rings
	for ( i = 0; i < SPHERE_STACKS - 1; i++ )
	{
		int iRing = 2 + i * SPHERE_SLICES;
		int iIndex = ( SPHERE_SLICES + i * SPHERE_SLICES * 2 ) * 3;

		for ( j = 0; j < SPHERE_SLICES; j++ )
		{
			g_iSphereTris[ iIndex + j * 6 ] = iRing + j;
			g_iSphereTris[ iIndex + j * 6 + 2 ] = iRing + j + 1;
			g_iSphereTris[ iIndex + j * 6 + 1 ] = iRing + j + SPHERE_SLICES;
			g_iSphereTris[ iIndex + j * 6 + 3 ] = iRing + j + SPHERE_SLICES;
			g_iSphereTris[ iIndex + j * 6 + 5 ] = iRing + j + 1;
			g_iSphereTris[ iIndex + j * 6 + 4 ] = iRing + j + SPHERE_SLICES + 1;
			if ( j == SPHERE_SLICES - 1 )
			{
				g_iSphereTris[ iIndex + j * 6 + 2 ] = iRing;
				g_iSphereTris[ iIndex + j * 6 + 5 ] = iRing;
				g_iSphereTris[ iIndex + j * 6 + 4 ] = iRing + SPHERE_SLICES;
			}
		}
	}

	// the bottom cap
	int iIndex = SPHERE_TRIS - SPHERE_SLICES;
	for ( i = 0; i < SPHERE_SLICES; i++ )
	{
		g_iSphereTris[ ( iIndex + i ) * 3 ] = 1;
		g_iSphereTris[ ( iIndex + i ) * 3 + 1 ] = i + SPHERE_POINTS - SPHERE_SLICES + 1;
		g_iSphereTris[ ( iIndex + i ) * 3 + 2 ] = i + SPHERE_POINTS - SPHERE_SLICES;
		if ( i == SPHERE_SLICES - 1 )
			g_iSphereTris[ ( iIndex + i ) * 3 + 1 ] = SPHERE_POINTS - SPHERE_SLICES;
	}

	bInit = true;
}

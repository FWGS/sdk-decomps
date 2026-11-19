//
// tractorbeam.cpp
//
// Purpose: the tractor beam's cone (events/tractorbeam.sc): a sprite-textured
//          cone of 16 triangles from the beam's source to the entity. The
//          names CTractorBeam (Create, Delete, Render), TractorBeamCreate,
//          TractorBeamDelete and Matrix with its operator* are retail's
//          (their ILT buckets fit); the rest are ours.
//

#include "hud.h"
#include "cl_util.h"
#include "const.h"
#include "entity_state.h"
#include "cl_entity.h"
#include "triangleapi.h"

#include <string.h>

bool EV_IsInPVS( cl_entity_t *pEntity );

extern void VectorAngles( const float *forward, float *angles );

// the cone's base: its centre and 16 points around it (sic: the circle is
// slightly uneven, 0.92488 against 0.922879 and so on)
Vector g_vecTractorCone[17] =
{
	Vector( 0.0, 0.0, 0.0 ),
	Vector( 0.924879, -1.0, 0.383684 ),
	Vector( 1.0, -1.0, 0.0 ),
	Vector( 0.708107, -1.0, 0.708107 ),
	Vector( 0.383683, -1.0, 0.92488 ),
	Vector( 0.0, -1.0, 1.0 ),
	Vector( -0.381683, -1.0, 0.924879 ),
	Vector( -0.706107, -1.0, 0.708107 ),
	Vector( -0.92288, -1.0, 0.383683 ),
	Vector( -1.0, -1.0, 0.0 ),
	Vector( -0.92288, -1.0, -0.381683 ),
	Vector( -0.706107, -1.0, -0.706107 ),
	Vector( -0.381683, -1.0, -0.922879 ),
	Vector( 0.0, -1.0, -1.0 ),
	Vector( 0.383684, -1.0, -0.922879 ),
	Vector( 0.708107, -1.0, -0.706107 ),
	Vector( 0.92488, -1.0, -0.381683 ),
};

// texture coordinates of the triangles' corners
Vector2D g_vecTractorTex[50] =
{
	Vector2D( -5.0, 0.0 ), Vector2D( -7.8125, 0.0 ), Vector2D( -8.124999, 0.647584 ), Vector2D( -7.5, 0.647584 ),
	Vector2D( -6.5625, 0.0 ), Vector2D( -7.34375, 0.0 ), Vector2D( -8.437499, 0.0 ), Vector2D( -8.749999, 0.647584 ),
	Vector2D( -6.875, 0.0 ), Vector2D( -9.062499, 0.0 ), Vector2D( -9.374999, 0.647584 ), Vector2D( -7.1875, 0.0 ),
	Vector2D( -8.28125, 0.0 ), Vector2D( -9.687499, 0.0 ), Vector2D( -9.999999, 0.647584 ), Vector2D( -7.5, 0.0 ),
	Vector2D( -8.75, 0.0 ), Vector2D( -0.3125, 0.0 ), Vector2D( -0.625, 0.647584 ), Vector2D( -0.000001, 0.647584 ),
	Vector2D( -2.8125, 0.0 ), Vector2D( -1.71875, 0.0 ), Vector2D( -0.9375, 0.0 ), Vector2D( -1.25, 0.647584 ),
	Vector2D( -3.125, 0.0 ), Vector2D( -2.1875, 0.0 ), Vector2D( -1.5625, 0.0 ), Vector2D( -1.875, 0.647584 ),
	Vector2D( -3.4375, 0.0 ), Vector2D( -2.65625, 0.0 ), Vector2D( -2.5, 0.647584 ), Vector2D( -3.75, 0.0 ),
	Vector2D( -3.125, 0.647584 ), Vector2D( -4.0625, 0.0 ), Vector2D( -3.59375, 0.0 ), Vector2D( -3.75, 0.647584 ),
	Vector2D( -4.375, 0.0 ), Vector2D( -4.375, 0.647584 ), Vector2D( -4.6875, 0.0 ), Vector2D( -4.53125, 0.0 ),
	Vector2D( -5.0, 0.647584 ), Vector2D( -5.3125, 0.0 ), Vector2D( -5.625, 0.647584 ), Vector2D( -5.46875, 0.0 ),
	Vector2D( -5.9375, 0.0 ), Vector2D( -6.25, 0.647584 ), Vector2D( -5.625, 0.0 ), Vector2D( -6.875, 0.647584 ),
	Vector2D( -6.40625, 0.0 ), Vector2D( -6.25, 0.0 ),
};

// the 16 triangles: the apex (0) and two points of the base
static const int s_iTractorVerts[48] =
{
	0, 1, 2,	0, 3, 1,	0, 4, 3,	0, 5, 4,	0, 6, 5,	0, 7, 6,	0, 8, 7,	0, 9, 8,
	0, 10, 9,	0, 11, 10,	0, 12, 11,	0, 13, 12,	0, 14, 13,	0, 15, 14,	0, 16, 15,	0, 2, 16,
};

// their texture coordinates (g_vecTractorTex)
static const int s_iTractorTexs[48] =
{
	1, 2, 3,	6, 7, 2,	9, 10, 7,	13, 14, 10,	17, 18, 19,	22, 23, 18,	26, 27, 23,	25, 30, 27,
	20, 32, 30,	28, 35, 32,	33, 37, 35,	38, 40, 37,	41, 42, 40,	44, 45, 42,	4, 47, 45,	11, 3, 47,
};

class Matrix
{
public:
	Matrix();
	Vector operator*( const Vector &v );

	float m[4][4];
};

Matrix::Matrix()
{
	m[0][0] = m[1][1] = m[2][2] = m[3][3] = 1;
	m[0][1] = m[0][2] = m[0][3] = m[1][0] = m[1][2] = m[1][3] = m[2][0] = m[2][1] = m[2][3] = m[3][0] = m[3][1] = m[3][2] = 0;
}

Vector Matrix::operator*( const Vector &v )
{
	return Vector( v.x * m[0][0] + v.y * m[1][0] + v.z * m[2][0] + m[3][0],
				   v.x * m[0][1] + v.y * m[1][1] + v.z * m[2][1] + m[3][1],
				   v.x * m[0][2] + v.y * m[1][2] + v.z * m[2][2] + m[3][2] );
}

//=========================================================
// A cone flLength long and flRadius wide at the base, from
// origin along angles; random flicker in brightness and alpha
//=========================================================
void DrawTractorCone( float *origin, float *angles, float flLength, float flRadius, unsigned char *color, float flAlpha )	// name ours
{
	Vector forward, right, up;
	gEngfuncs.pfnAngleVectors( angles, forward, right, up );

	Matrix mat;
	mat.m[0][0] = right.x;
	mat.m[1][0] = up.x;
	mat.m[2][0] = forward.x;
	mat.m[0][1] = right.y;
	mat.m[1][1] = up.y;
	mat.m[2][1] = forward.y;
	mat.m[0][2] = right.z;
	mat.m[1][2] = up.z;
	mat.m[2][2] = forward.z;
	mat.m[3][0] = origin[0];
	mat.m[3][1] = origin[1];
	mat.m[3][2] = origin[2];

	Vector vecVerts[17];

	for ( int i = 1; i < 17; i++ )
	{
		vecVerts[i].x = flRadius * g_vecTractorCone[i].x;
		vecVerts[i].y = flRadius * g_vecTractorCone[i].z;
		vecVerts[i].z = flLength;
		vecVerts[i] = mat * vecVerts[i];
	}

	VectorCopy( origin, vecVerts[0] );

	gEngfuncs.pTriAPI->Begin( TRI_TRIANGLES );
	gEngfuncs.pTriAPI->Color4ub( color[0], color[1], color[2], flAlpha );

	const int *pVert = s_iTractorVerts;
	const int *pTex = s_iTractorTexs;

	while ( pTex < &s_iTractorTexs[48] )
	{
		gEngfuncs.pTriAPI->Brightness( 1 );
		gEngfuncs.pTriAPI->TexCoord2f( g_vecTractorTex[*pTex].x, g_vecTractorTex[*pTex].y );
		gEngfuncs.pTriAPI->Vertex3fv( vecVerts[*pVert] );
		pTex++;
		pVert++;

		gEngfuncs.pTriAPI->Brightness( gEngfuncs.pfnRandomFloat( 0, 1 ) < 0.8 );
		gEngfuncs.pTriAPI->Color4ub( color[0], color[1], color[2], gEngfuncs.pfnRandomFloat( 0, flAlpha ) );
		gEngfuncs.pTriAPI->TexCoord2f( g_vecTractorTex[*pTex].x, g_vecTractorTex[*pTex].y );
		gEngfuncs.pTriAPI->Vertex3fv( vecVerts[*pVert] );
		pTex++;
		pVert++;

		gEngfuncs.pTriAPI->Brightness( gEngfuncs.pfnRandomFloat( 0, 1 ) < 0.8 );
		gEngfuncs.pTriAPI->Color4ub( color[0], color[1], color[2], gEngfuncs.pfnRandomFloat( 0, flAlpha ) );
		gEngfuncs.pTriAPI->TexCoord2f( g_vecTractorTex[*pTex].x, g_vecTractorTex[*pTex].y );
		gEngfuncs.pTriAPI->Vertex3fv( vecVerts[*pVert] );
		pTex++;
		pVert++;
	}

	gEngfuncs.pTriAPI->End();
}

#define MAX_TRACTORBEAMS	32

class CTractorBeam
{
public:
	CTractorBeam();
	~CTractorBeam() {}

	void Render( void );
	void Create( int iEntity );
	void Delete( int iEntity );

	int		m_iEntities[ MAX_TRACTORBEAMS ];	// -1: free
};

CTractorBeam::CTractorBeam()
{
	memset( m_iEntities, -1, sizeof( m_iEntities ) );
}

//=========================================================
// Each beam entity: its sprite as the cone's texture, the
// cone from the owner's attachment 0 to the entity when it has
// one (playerclass set), else along the entity's angles with
// its scale as the length; fuser1 is the cone's radius, fuser2
// its alpha, rendercolor its colour
//=========================================================
void CTractorBeam::Render( void )
{
	gEngfuncs.pTriAPI->RenderMode( kRenderTransAdd );

	for ( int i = 0; i < MAX_TRACTORBEAMS; i++ )
	{
		if ( m_iEntities[i] <= 0 )
			continue;

		cl_entity_t *pEnt = gEngfuncs.GetEntityByIndex( m_iEntities[i] );

		if ( !pEnt || !pEnt->curstate.modelindex )
		{
			m_iEntities[i] = -1;
			continue;
		}

		if ( !EV_IsInPVS( pEnt ) )
			continue;

		float origin[3], angles[3];
		VectorCopy( pEnt->origin, origin );
		float flLength = pEnt->curstate.scale;
		VectorCopy( pEnt->angles, angles );

		if ( pEnt->curstate.playerclass && pEnt->curstate.owner )
		{
			cl_entity_t *pOwner = gEngfuncs.GetEntityByIndex( pEnt->curstate.owner );
			Vector vecEnd( origin );
			VectorCopy( pOwner->attachment[0], origin );

			Vector vecDir = vecEnd - Vector( origin );
			flLength = vecDir.Length();

			VectorAngles( vecDir.Normalize(), angles );
			angles[2] = pEnt->angles.z;
		}

		angles[0] = -angles[0];

		gEngfuncs.pTriAPI->SpriteTexture( pEnt->model, pEnt->curstate.frame );
		DrawTractorCone( origin, angles, flLength, pEnt->curstate.fuser1, &pEnt->curstate.rendercolor.r, pEnt->curstate.fuser2 );
	}
}

void CTractorBeam::Create( int iEntity )
{
	for ( int i = 0; i < MAX_TRACTORBEAMS; i++ )
	{
		if ( m_iEntities[i] < 0 )
		{
			m_iEntities[i] = iEntity;
			return;
		}
	}
}

void CTractorBeam::Delete( int iEntity )
{
	for ( int i = 0; i < MAX_TRACTORBEAMS; i++ )
	{
		if ( m_iEntities[i] == iEntity )
			m_iEntities[i] = -1;
	}
}

CTractorBeam *g_pTractorBeam = NULL;	// name ours

// the first call only sets the list up
void RenderAllTractorbeams( void )
{
	static CTractorBeam s_TractorBeam;

	if ( g_pTractorBeam )
		g_pTractorBeam->Render();
	else
		g_pTractorBeam = &s_TractorBeam;
}

void TractorbeamVidInit( void )
{
	if ( g_pTractorBeam )
	{
		for ( int i = 0; i < MAX_TRACTORBEAMS; i++ )
			g_pTractorBeam->m_iEntities[i] = -1;
	}
}

void TractorBeamCreate( int iEntity )
{
	if ( g_pTractorBeam )
		g_pTractorBeam->Create( iEntity );
}

void TractorBeamDelete( int iEntity )
{
	if ( g_pTractorBeam )
		g_pTractorBeam->Delete( iEntity );
}

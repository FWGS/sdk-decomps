//========= Copyright (c) 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: the blast spheres (events/blastsphere.sc): a geodesic sphere mesh
//          (92 vertices, 180 triangles) drawn with TriAPI, a growing sprite
//          sphere and a white shell that fades towards its edge
//
// $NoKeywords: $
//=============================================================================

#include "hud.h"
#include "const.h"
#include "entity_state.h"
#include "cl_entity.h"
#include "triangleapi.h"
#include "event_args.h"

#include <math.h>

#define SPHERE_VERTS		92
#define SPHERE_TEXCOORDS	108
#define SPHERE_TRIS			180

static Vector g_vecSphereVerts[ SPHERE_VERTS ] =
{
	Vector( 0.000000, -1.000000, 0.000000 ),
	Vector( 0.360729, -0.932670, 0.000000 ),
	Vector( 0.111471, -0.932670, 0.343074 ),
	Vector( 0.672883, -0.739749, 0.000000 ),
	Vector( 0.479506, -0.805422, 0.348381 ),
	Vector( 0.207932, -0.739749, 0.639950 ),
	Vector( 0.894427, -0.447214, 0.000000 ),
	Vector( 0.784354, -0.516806, 0.343074 ),
	Vector( 0.568661, -0.516806, 0.639950 ),
	Vector( 0.276393, -0.447213, 0.850651 ),
	Vector( -0.291836, -0.932670, 0.212031 ),
	Vector( -0.183155, -0.805422, 0.563693 ),
	Vector( -0.544374, -0.739749, 0.395511 ),
	Vector( -0.083904, -0.516806, 0.851981 ),
	Vector( -0.432902, -0.516806, 0.738584 ),
	Vector( -0.723607, -0.447213, 0.525731 ),
	Vector( -0.291836, -0.932670, -0.212031 ),
	Vector( -0.592702, -0.805422, 0.000000 ),
	Vector( -0.544374, -0.739749, -0.395511 ),
	Vector( -0.836210, -0.516806, 0.183479 ),
	Vector( -0.836210, -0.516806, -0.183479 ),
	Vector( -0.723607, -0.447214, -0.525731 ),
	Vector( 0.111471, -0.932670, -0.343074 ),
	Vector( -0.183155, -0.805422, -0.563693 ),
	Vector( 0.207932, -0.739749, -0.639950 ),
	Vector( -0.432902, -0.516806, -0.738584 ),
	Vector( -0.083904, -0.516806, -0.851981 ),
	Vector( 0.276393, -0.447214, -0.850651 ),
	Vector( 0.479506, -0.805422, -0.348381 ),
	Vector( 0.568661, -0.516806, -0.639950 ),
	Vector( 0.784354, -0.516806, -0.343074 ),
	Vector( 0.964719, -0.156077, -0.212031 ),
	Vector( 0.964719, -0.156077, 0.212031 ),
	Vector( 0.905103, 0.156077, -0.395510 ),
	Vector( 0.985456, 0.169933, 0.000000 ),
	Vector( 0.905103, 0.156077, 0.395511 ),
	Vector( 0.723607, 0.447214, -0.525731 ),
	Vector( 0.836210, 0.516806, -0.183479 ),
	Vector( 0.836210, 0.516806, 0.183479 ),
	Vector( 0.723607, 0.447214, 0.525731 ),
	Vector( 0.499768, -0.156077, 0.851981 ),
	Vector( 0.096461, -0.156077, 0.983023 ),
	Vector( 0.655845, 0.156077, 0.738584 ),
	Vector( 0.304522, 0.169933, 0.937224 ),
	Vector( -0.096461, 0.156077, 0.983023 ),
	Vector( 0.432902, 0.516806, 0.738584 ),
	Vector( 0.083904, 0.516806, 0.851981 ),
	Vector( -0.276393, 0.447214, 0.850651 ),
	Vector( -0.655845, -0.156077, 0.738584 ),
	Vector( -0.905103, -0.156077, 0.395511 ),
	Vector( -0.499768, 0.156077, 0.851981 ),
	Vector( -0.797250, 0.169933, 0.579236 ),
	Vector( -0.964719, 0.156077, 0.212031 ),
	Vector( -0.568661, 0.516806, 0.639950 ),
	Vector( -0.784354, 0.516806, 0.343074 ),
	Vector( -0.894427, 0.447214, 0.000000 ),
	Vector( -0.905103, -0.156077, -0.395511 ),
	Vector( -0.655845, -0.156077, -0.738584 ),
	Vector( -0.964719, 0.156077, -0.212031 ),
	Vector( -0.797250, 0.169933, -0.579236 ),
	Vector( -0.499768, 0.156077, -0.851981 ),
	Vector( -0.784354, 0.516806, -0.343074 ),
	Vector( -0.568661, 0.516806, -0.639950 ),
	Vector( -0.276393, 0.447214, -0.850651 ),
	Vector( 0.096461, -0.156077, -0.983023 ),
	Vector( 0.499768, -0.156077, -0.851981 ),
	Vector( -0.096461, 0.156077, -0.983023 ),
	Vector( 0.304523, 0.169933, -0.937224 ),
	Vector( 0.655845, 0.156077, -0.738584 ),
	Vector( 0.083904, 0.516806, -0.851981 ),
	Vector( 0.432902, 0.516806, -0.738584 ),
	Vector( 0.797250, -0.169933, 0.579236 ),
	Vector( -0.304523, -0.169933, 0.937224 ),
	Vector( -0.985456, -0.169933, 0.000000 ),
	Vector( -0.304522, -0.169933, -0.937224 ),
	Vector( 0.797250, -0.169933, -0.579236 ),
	Vector( 0.000000, 1.000000, 0.000000 ),
	Vector( -0.111471, 0.932671, 0.343074 ),
	Vector( 0.291836, 0.932671, 0.212031 ),
	Vector( -0.207932, 0.739749, 0.639950 ),
	Vector( 0.183155, 0.805422, 0.563693 ),
	Vector( 0.544374, 0.739749, 0.395511 ),
	Vector( -0.360729, 0.932671, 0.000000 ),
	Vector( -0.672883, 0.739749, 0.000000 ),
	Vector( -0.479506, 0.805422, 0.348381 ),
	Vector( -0.111471, 0.932671, -0.343074 ),
	Vector( -0.207932, 0.739749, -0.639950 ),
	Vector( -0.479506, 0.805422, -0.348381 ),
	Vector( 0.291836, 0.932671, -0.212031 ),
	Vector( 0.544374, 0.739749, -0.395510 ),
	Vector( 0.183155, 0.805422, -0.563693 ),
	Vector( 0.592702, 0.805422, 0.000000 )
};

static Vector2D g_vecSphereTexCoords[ SPHERE_TEXCOORDS ] =
{
	Vector2D( 0.600000, 1.000000 ),
	Vector2D( 0.500000, 0.882528 ),
	Vector2D( 0.700000, 0.882528 ),
	Vector2D( 0.500000, 0.765056 ),
	Vector2D( 0.600000, 0.798061 ),
	Vector2D( 0.700000, 0.765056 ),
	Vector2D( 0.500000, 0.647584 ),
	Vector2D( 0.565623, 0.672879 ),
	Vector2D( 0.634377, 0.672879 ),
	Vector2D( 0.700000, 0.647584 ),
	Vector2D( 0.800000, 1.000000 ),
	Vector2D( 0.900000, 0.882528 ),
	Vector2D( 0.800000, 0.798061 ),
	Vector2D( 0.900000, 0.765056 ),
	Vector2D( 0.765623, 0.672879 ),
	Vector2D( 0.834377, 0.672879 ),
	Vector2D( 0.900000, 0.647584 ),
	Vector2D( 0.000000, 1.000000 ),
	Vector2D( -0.100000, 0.882528 ),
	Vector2D( 0.100000, 0.882528 ),
	Vector2D( -0.100000, 0.765056 ),
	Vector2D( 0.000000, 0.798061 ),
	Vector2D( 0.100000, 0.765056 ),
	Vector2D( 0.965623, 0.672879 ),
	Vector2D( -0.034377, 0.672879 ),
	Vector2D( 0.034377, 0.672879 ),
	Vector2D( 0.100000, 0.647584 ),
	Vector2D( 0.200000, 1.000000 ),
	Vector2D( 0.300000, 0.882528 ),
	Vector2D( 0.200000, 0.798061 ),
	Vector2D( 0.300000, 0.765056 ),
	Vector2D( 0.165623, 0.672879 ),
	Vector2D( 0.234377, 0.672879 ),
	Vector2D( 0.300000, 0.647584 ),
	Vector2D( 0.400000, 1.000000 ),
	Vector2D( 0.400000, 0.798061 ),
	Vector2D( 0.365623, 0.672879 ),
	Vector2D( 0.434377, 0.672879 ),
	Vector2D( 0.465568, 0.549885 ),
	Vector2D( 0.534432, 0.549885 ),
	Vector2D( 0.434433, 0.450115 ),
	Vector2D( 0.500000, 0.445645 ),
	Vector2D( 0.565567, 0.450115 ),
	Vector2D( 0.400000, 0.352416 ),
	Vector2D( 0.465623, 0.327121 ),
	Vector2D( 0.534377, 0.327121 ),
	Vector2D( 0.600000, 0.352416 ),
	Vector2D( 0.665568, 0.549885 ),
	Vector2D( 0.734433, 0.549885 ),
	Vector2D( 0.634432, 0.450115 ),
	Vector2D( 0.700000, 0.445645 ),
	Vector2D( 0.765567, 0.450115 ),
	Vector2D( 0.665623, 0.327121 ),
	Vector2D( 0.734377, 0.327121 ),
	Vector2D( 0.800000, 0.352416 ),
	Vector2D( 0.865568, 0.549885 ),
	Vector2D( 0.934433, 0.549885 ),
	Vector2D( 0.834432, 0.450115 ),
	Vector2D( 0.900000, 0.445645 ),
	Vector2D( 0.965567, 0.450115 ),
	Vector2D( 0.865623, 0.327121 ),
	Vector2D( 0.934377, 0.327121 ),
	Vector2D( -0.034433, 0.450115 ),
	Vector2D( -0.065623, 0.327121 ),
	Vector2D( 0.000000, 0.352416 ),
	Vector2D( 0.065568, 0.549885 ),
	Vector2D( 0.134432, 0.549885 ),
	Vector2D( 0.034433, 0.450115 ),
	Vector2D( 0.100000, 0.445645 ),
	Vector2D( 0.165568, 0.450115 ),
	Vector2D( 0.065623, 0.327121 ),
	Vector2D( 0.134377, 0.327121 ),
	Vector2D( 0.200000, 0.352416 ),
	Vector2D( 0.265568, 0.549885 ),
	Vector2D( 0.334433, 0.549885 ),
	Vector2D( 0.234433, 0.450115 ),
	Vector2D( 0.300000, 0.445645 ),
	Vector2D( 0.365568, 0.450115 ),
	Vector2D( 0.265623, 0.327121 ),
	Vector2D( 0.334377, 0.327121 ),
	Vector2D( 0.600000, 0.554355 ),
	Vector2D( 0.800000, 0.554355 ),
	Vector2D( 0.000000, 0.554355 ),
	Vector2D( -0.065567, 0.549885 ),
	Vector2D( 0.200000, 0.554355 ),
	Vector2D( 0.400000, 0.554355 ),
	Vector2D( 0.700000, 0.000000 ),
	Vector2D( 0.800000, 0.117472 ),
	Vector2D( 0.600000, 0.117472 ),
	Vector2D( 0.800000, 0.234944 ),
	Vector2D( 0.700000, 0.201939 ),
	Vector2D( 0.600000, 0.234944 ),
	Vector2D( -0.100000, 0.000000 ),
	Vector2D( 0.000000, 0.117472 ),
	Vector2D( -0.200000, 0.117472 ),
	Vector2D( 0.000000, 0.234944 ),
	Vector2D( -0.100000, 0.201939 ),
	Vector2D( 0.900000, 0.201939 ),
	Vector2D( 0.100000, 0.000000 ),
	Vector2D( 0.200000, 0.117472 ),
	Vector2D( 0.200000, 0.234944 ),
	Vector2D( 0.100000, 0.201939 ),
	Vector2D( 0.300000, 0.000000 ),
	Vector2D( 0.400000, 0.117472 ),
	Vector2D( 0.400000, 0.234944 ),
	Vector2D( 0.300000, 0.201939 ),
	Vector2D( 0.500000, 0.000000 ),
	Vector2D( 0.500000, 0.201939 )
};

static const int g_iSphereVertIndex[ SPHERE_TRIS * 3 ] =
{
	0, 1, 2, 1, 3, 4, 1, 4, 2, 2, 4, 5,
	3, 6, 7, 3, 7, 4, 4, 7, 8, 4, 8, 5,
	5, 8, 9, 0, 2, 10, 2, 5, 11, 2, 11, 10,
	10, 11, 12, 5, 9, 13, 5, 13, 11, 11, 13, 14,
	11, 14, 12, 12, 14, 15, 0, 10, 16, 10, 12, 17,
	10, 17, 16, 16, 17, 18, 12, 15, 19, 12, 19, 17,
	17, 19, 20, 17, 20, 18, 18, 20, 21, 0, 16, 22,
	16, 18, 23, 16, 23, 22, 22, 23, 24, 18, 21, 25,
	18, 25, 23, 23, 25, 26, 23, 26, 24, 24, 26, 27,
	0, 22, 1, 22, 24, 28, 22, 28, 1, 1, 28, 3,
	24, 27, 29, 24, 29, 28, 28, 29, 30, 28, 30, 3,
	3, 30, 6, 6, 31, 32, 31, 33, 34, 31, 34, 32,
	32, 34, 35, 33, 36, 37, 33, 37, 34, 34, 37, 38,
	34, 38, 35, 35, 38, 39, 9, 40, 41, 40, 42, 43,
	40, 43, 41, 41, 43, 44, 42, 39, 45, 42, 45, 43,
	43, 45, 46, 43, 46, 44, 44, 46, 47, 15, 48, 49,
	48, 50, 51, 48, 51, 49, 49, 51, 52, 50, 47, 53,
	50, 53, 51, 51, 53, 54, 51, 54, 52, 52, 54, 55,
	21, 56, 57, 56, 58, 59, 56, 59, 57, 57, 59, 60,
	58, 55, 61, 58, 61, 59, 59, 61, 62, 59, 62, 60,
	60, 62, 63, 27, 64, 65, 64, 66, 67, 64, 67, 65,
	65, 67, 68, 66, 63, 69, 66, 69, 67, 67, 69, 70,
	67, 70, 68, 68, 70, 36, 39, 42, 35, 42, 40, 71,
	42, 71, 35, 35, 71, 32, 40, 9, 8, 40, 8, 71,
	71, 8, 7, 71, 7, 32, 32, 7, 6, 47, 50, 44,
	50, 48, 72, 50, 72, 44, 44, 72, 41, 48, 15, 14,
	48, 14, 72, 72, 14, 13, 72, 13, 41, 41, 13, 9,
	55, 58, 52, 58, 56, 73, 58, 73, 52, 52, 73, 49,
	56, 21, 20, 56, 20, 73, 73, 20, 19, 73, 19, 49,
	49, 19, 15, 63, 66, 60, 66, 64, 74, 66, 74, 60,
	60, 74, 57, 64, 27, 26, 64, 26, 74, 74, 26, 25,
	74, 25, 57, 57, 25, 21, 36, 33, 68, 33, 31, 75,
	33, 75, 68, 68, 75, 65, 31, 6, 30, 31, 30, 75,
	75, 30, 29, 75, 29, 65, 65, 29, 27, 76, 77, 78,
	77, 79, 80, 77, 80, 78, 78, 80, 81, 79, 47, 46,
	79, 46, 80, 80, 46, 45, 80, 45, 81, 81, 45, 39,
	76, 82, 77, 82, 83, 84, 82, 84, 77, 77, 84, 79,
	83, 55, 54, 83, 54, 84, 84, 54, 53, 84, 53, 79,
	79, 53, 47, 76, 85, 82, 85, 86, 87, 85, 87, 82,
	82, 87, 83, 86, 63, 62, 86, 62, 87, 87, 62, 61,
	87, 61, 83, 83, 61, 55, 76, 88, 85, 88, 89, 90,
	88, 90, 85, 85, 90, 86, 89, 36, 70, 89, 70, 90,
	90, 70, 69, 90, 69, 86, 86, 69, 63, 76, 78, 88,
	78, 81, 91, 78, 91, 88, 88, 91, 89, 81, 39, 38,
	81, 38, 91, 91, 38, 37, 91, 37, 89, 89, 37, 36
};

static const int g_iSphereTexIndex[ SPHERE_TRIS * 3 ] =
{
	0, 1, 2, 1, 3, 4, 1, 4, 2, 2, 4, 5,
	3, 6, 7, 3, 7, 4, 4, 7, 8, 4, 8, 5,
	5, 8, 9, 10, 2, 11, 2, 5, 12, 2, 12, 11,
	11, 12, 13, 5, 9, 14, 5, 14, 12, 12, 14, 15,
	12, 15, 13, 13, 15, 16, 17, 18, 19, 18, 20, 21,
	18, 21, 19, 19, 21, 22, 13, 16, 23, 20, 24, 21,
	21, 24, 25, 21, 25, 22, 22, 25, 26, 27, 19, 28,
	19, 22, 29, 19, 29, 28, 28, 29, 30, 22, 26, 31,
	22, 31, 29, 29, 31, 32, 29, 32, 30, 30, 32, 33,
	34, 28, 1, 28, 30, 35, 28, 35, 1, 1, 35, 3,
	30, 33, 36, 30, 36, 35, 35, 36, 37, 35, 37, 3,
	3, 37, 6, 6, 38, 39, 38, 40, 41, 38, 41, 39,
	39, 41, 42, 40, 43, 44, 40, 44, 41, 41, 44, 45,
	41, 45, 42, 42, 45, 46, 9, 47, 48, 47, 49, 50,
	47, 50, 48, 48, 50, 51, 49, 46, 52, 49, 52, 50,
	50, 52, 53, 50, 53, 51, 51, 53, 54, 16, 55, 56,
	55, 57, 58, 55, 58, 56, 56, 58, 59, 57, 54, 60,
	57, 60, 58, 58, 60, 61, 58, 61, 59, 62, 63, 64,
	26, 65, 66, 65, 67, 68, 65, 68, 66, 66, 68, 69,
	67, 64, 70, 67, 70, 68, 68, 70, 71, 68, 71, 69,
	69, 71, 72, 33, 73, 74, 73, 75, 76, 73, 76, 74,
	74, 76, 77, 75, 72, 78, 75, 78, 76, 76, 78, 79,
	76, 79, 77, 77, 79, 43, 46, 49, 42, 49, 47, 80,
	49, 80, 42, 42, 80, 39, 47, 9, 8, 47, 8, 80,
	80, 8, 7, 80, 7, 39, 39, 7, 6, 54, 57, 51,
	57, 55, 81, 57, 81, 51, 51, 81, 48, 55, 16, 15,
	55, 15, 81, 81, 15, 14, 81, 14, 48, 48, 14, 9,
	64, 67, 62, 67, 65, 82, 67, 82, 62, 62, 82, 83,
	65, 26, 25, 65, 25, 82, 82, 25, 24, 82, 24, 83,
	56, 23, 16, 72, 75, 69, 75, 73, 84, 75, 84, 69,
	69, 84, 66, 73, 33, 32, 73, 32, 84, 84, 32, 31,
	84, 31, 66, 66, 31, 26, 43, 40, 77, 40, 38, 85,
	40, 85, 77, 77, 85, 74, 38, 6, 37, 38, 37, 85,
	85, 37, 36, 85, 36, 74, 74, 36, 33, 86, 87, 88,
	87, 89, 90, 87, 90, 88, 88, 90, 91, 89, 54, 53,
	89, 53, 90, 90, 53, 52, 90, 52, 91, 91, 52, 46,
	92, 93, 94, 93, 95, 96, 93, 96, 94, 87, 97, 89,
	95, 64, 63, 95, 63, 96, 97, 61, 60, 97, 60, 89,
	89, 60, 54, 98, 99, 93, 99, 100, 101, 99, 101, 93,
	93, 101, 95, 100, 72, 71, 100, 71, 101, 101, 71, 70,
	101, 70, 95, 95, 70, 64, 102, 103, 99, 103, 104, 105,
	103, 105, 99, 99, 105, 100, 104, 43, 79, 104, 79, 105,
	105, 79, 78, 105, 78, 100, 100, 78, 72, 106, 88, 103,
	88, 91, 107, 88, 107, 103, 103, 107, 104, 91, 46, 45,
	91, 45, 107, 107, 45, 44, 107, 44, 104, 104, 44, 43
};

//=========================================================
// A sphere of the current texture and colour
//=========================================================
void RenderSphere( const Vector &vecOrigin, float flRadius, const Vector &vecColor, float flAlpha )
{
	Vector vecVerts[ SPHERE_VERTS ];
	int i;

	for ( i = 0; i < SPHERE_VERTS; i++ )
		vecVerts[i] = vecOrigin + g_vecSphereVerts[i] * flRadius;

	gEngfuncs.pTriAPI->Begin( TRI_TRIANGLES );
	gEngfuncs.pTriAPI->Color4f( vecColor.x, vecColor.y, vecColor.z, flAlpha );
	gEngfuncs.pTriAPI->Brightness( 1 );

	for ( i = 0; i < SPHERE_TRIS * 3; )
	{
		gEngfuncs.pTriAPI->TexCoord2f( g_vecSphereTexCoords[ g_iSphereTexIndex[i] ].x, g_vecSphereTexCoords[ g_iSphereTexIndex[i] ].y );
		gEngfuncs.pTriAPI->Vertex3fv( vecVerts[ g_iSphereVertIndex[i] ] );
		i++;
		gEngfuncs.pTriAPI->TexCoord2f( g_vecSphereTexCoords[ g_iSphereTexIndex[i] ].x, g_vecSphereTexCoords[ g_iSphereTexIndex[i] ].y );
		gEngfuncs.pTriAPI->Vertex3fv( vecVerts[ g_iSphereVertIndex[i] ] );
		i++;
		gEngfuncs.pTriAPI->TexCoord2f( g_vecSphereTexCoords[ g_iSphereTexIndex[i] ].x, g_vecSphereTexCoords[ g_iSphereTexIndex[i] ].y );
		gEngfuncs.pTriAPI->Vertex3fv( vecVerts[ g_iSphereVertIndex[i] ] );
		i++;
	}

	gEngfuncs.pTriAPI->End();
}

const float SPHERE_SOLID = 0.6;	// facing the view at least this much: vecColor1
const float SPHERE_EDGE = 0.4;	// below this: vecColor2 fading out

static float g_flSphereBlend = 1 / ( SPHERE_SOLID - SPHERE_EDGE );
static Vector g_vecSphereUnused( 0, 0, 0 );	// sic: never read
static float g_flSphereFade = 1 / SPHERE_EDGE;

//=========================================================
// A shell, inside out: vecColor1 where it faces vecDir,
// blending to vecColor2 and fading to nothing at its edge
//=========================================================
void RenderSphere2( const Vector &vecOrigin, const Vector &vecDir, float flRadius, const Vector &vecColor1, const Vector &vecColor2, float flAlpha )
{
	Vector vecVerts[ SPHERE_VERTS ];
	Vector vecColors[ SPHERE_VERTS ];
	int i;

	for ( i = 0; i < SPHERE_VERTS; i++ )
	{
		vecVerts[i] = vecOrigin + g_vecSphereVerts[i] * flRadius;

		// how much this vertex faces vecDir, summed y and z first
		float flDot = g_vecSphereVerts[i].y * vecDir.y + g_vecSphereVerts[i].z * vecDir.z;
		flDot += g_vecSphereVerts[i].x * vecDir.x;
		flDot = fabs( flDot );

		if ( flDot > SPHERE_SOLID )
			vecColors[i] = vecColor1;
		else if ( flDot >= SPHERE_EDGE )
		{
			float flBlend = ( flDot - SPHERE_EDGE ) * g_flSphereBlend;
			vecColors[i] = vecColor2 + flBlend * ( vecColor1 - vecColor2 );
		}
		else
			vecColors[i] = flDot * g_flSphereFade * vecColor2;
	}

	gEngfuncs.pTriAPI->Begin( TRI_TRIANGLES );
	gEngfuncs.pTriAPI->Brightness( 1 );

	for ( i = 0; i < SPHERE_TRIS * 3; )
	{
		i += 2;
		gEngfuncs.pTriAPI->Color4f( vecColors[ g_iSphereVertIndex[i] ].x, vecColors[ g_iSphereVertIndex[i] ].y, vecColors[ g_iSphereVertIndex[i] ].z, flAlpha );
		gEngfuncs.pTriAPI->TexCoord2f( g_vecSphereTexCoords[ g_iSphereTexIndex[i] ].x, g_vecSphereTexCoords[ g_iSphereTexIndex[i] ].y );
		gEngfuncs.pTriAPI->Vertex3fv( vecVerts[ g_iSphereVertIndex[i] ] );
		i--;
		gEngfuncs.pTriAPI->Color4f( vecColors[ g_iSphereVertIndex[i] ].x, vecColors[ g_iSphereVertIndex[i] ].y, vecColors[ g_iSphereVertIndex[i] ].z, flAlpha );
		gEngfuncs.pTriAPI->TexCoord2f( g_vecSphereTexCoords[ g_iSphereTexIndex[i] ].x, g_vecSphereTexCoords[ g_iSphereTexIndex[i] ].y );
		gEngfuncs.pTriAPI->Vertex3fv( vecVerts[ g_iSphereVertIndex[i] ] );
		i--;
		gEngfuncs.pTriAPI->Color4f( vecColors[ g_iSphereVertIndex[i] ].x, vecColors[ g_iSphereVertIndex[i] ].y, vecColors[ g_iSphereVertIndex[i] ].z, flAlpha );
		gEngfuncs.pTriAPI->TexCoord2f( g_vecSphereTexCoords[ g_iSphereTexIndex[i] ].x, g_vecSphereTexCoords[ g_iSphereTexIndex[i] ].y );
		gEngfuncs.pTriAPI->Vertex3fv( vecVerts[ g_iSphereVertIndex[i] ] );
		i += 3;
	}

	gEngfuncs.pTriAPI->End();
}

#define MAX_SPHERES		64

struct blastsphere_t
{
	Vector	origin;
	Vector	color;
	float	frame;
	char	framedir;	// the sprite's frames run back and forth
	char	shell;		// a white RenderSphere2 shell, not the sprite
	float	radius;
	float	maxradius;
	float	growrate;	// units a second
	float	alpha;		// alive while above 0
	float	fadetime;	// starts fading
	float	faderate;	// alpha a second
};

class CSphere
{
public:
	CSphere();
	~CSphere() {}

	void Initialize( void );
	void Render( float flTime, float flFrametime );
	void Create( const Vector &vecOrigin, const Vector &vecColor, float flMaxRadius, float flGrowRate, float flFadeDelay, float flFadeRate, char cShell );

	blastsphere_t	m_Spheres[ MAX_SPHERES ];
	struct model_s	*m_pSprite;
	struct model_s	*m_pWhite;
	int				m_iFrames;
};

CSphere::CSphere()
{
	for ( int i = 0; i < MAX_SPHERES; i++ )
		m_Spheres[i].alpha = 0;

	m_pSprite = NULL;
	m_pWhite = NULL;
}

void CSphere::Initialize( void )
{
	HSPRITE hSprite = gEngfuncs.pfnSPR_Load( "sprites/blastsphere.spr" );
	m_pSprite = (struct model_s *)gEngfuncs.GetSpritePointer( hSprite );
	m_iFrames = gEngfuncs.pfnSPR_Frames( hSprite ) - 1;
	m_pWhite = (struct model_s *)gEngfuncs.GetSpritePointer( gEngfuncs.pfnSPR_Load( "sprites/white.spr" ) );
}

void CSphere::Render( float flTime, float flFrametime )
{
	int i;

	for ( i = 0; i < MAX_SPHERES; i++ )
	{
		if ( m_Spheres[i].alpha > 0 )
		{
			if ( flTime > m_Spheres[i].fadetime )
				m_Spheres[i].alpha -= flFrametime * m_Spheres[i].faderate;

			if ( m_Spheres[i].radius < m_Spheres[i].maxradius )
			{
				m_Spheres[i].radius += flFrametime * m_Spheres[i].growrate;
				if ( m_Spheres[i].radius > m_Spheres[i].maxradius )
					m_Spheres[i].radius = m_Spheres[i].maxradius;
			}
		}
	}

	if ( !m_pSprite || !m_pWhite )
		Initialize();

	Vector vecAngles, vecForward, vecRight, vecUp;
	gEngfuncs.GetViewAngles( vecAngles );
	gEngfuncs.pfnAngleVectors( vecAngles, vecForward, vecRight, vecUp );

	gEngfuncs.pTriAPI->CullFace( TRI_NONE );
	gEngfuncs.pTriAPI->RenderMode( kRenderTransAdd );

	for ( i = 0; i < MAX_SPHERES; i++ )
	{
		if ( m_Spheres[i].alpha <= 0 )
			continue;

		if ( m_Spheres[i].shell )
		{
			gEngfuncs.pTriAPI->SpriteTexture( m_pWhite, 0 );
			RenderSphere2( m_Spheres[i].origin, -vecForward, m_Spheres[i].radius + gEngfuncs.pfnRandomFloat( -32, 32 ), Vector( 1, 1, 1 ), m_Spheres[i].color, m_Spheres[i].alpha );
		}
		else
		{
			m_Spheres[i].frame += m_Spheres[i].framedir * flFrametime * 10;

			if ( m_Spheres[i].frame >= m_iFrames )
			{
				m_Spheres[i].frame = m_iFrames;
				m_Spheres[i].framedir = -1;
			}
			else if ( m_Spheres[i].frame <= 0 )
			{
				m_Spheres[i].frame = 0;
				m_Spheres[i].framedir = 1;
			}

			gEngfuncs.pTriAPI->SpriteTexture( m_pSprite, (int)m_Spheres[i].frame );
			RenderSphere( m_Spheres[i].origin, m_Spheres[i].radius, m_Spheres[i].color, m_Spheres[i].alpha );
		}
	}
}

void CSphere::Create( const Vector &vecOrigin, const Vector &vecColor, float flMaxRadius, float flGrowRate, float flFadeDelay, float flFadeRate, char cShell )
{
	blastsphere_t *pSphere = &m_Spheres[0];

	for ( int i = 0; i < MAX_SPHERES; i++ )
	{
		if ( m_Spheres[i].alpha < pSphere->alpha )
		{
			pSphere = &m_Spheres[i];
			if ( pSphere->alpha <= 0 )
				break;
		}
	}

	pSphere->shell = cShell;
	pSphere->radius = pSphere->alpha = 1;
	pSphere->color = vecColor / 255;
	pSphere->growrate = flGrowRate;
	pSphere->faderate = flFadeRate;
	pSphere->fadetime = gEngfuncs.GetClientTime() + flFadeDelay;
	pSphere->maxradius = flMaxRadius;
	pSphere->origin = vecOrigin;
	pSphere->frame = gEngfuncs.pfnRandomFloat( 0, m_iFrames );
	pSphere->framedir = ( pSphere->frame > m_iFrames * 0.5 ) ? -1 : 1;
}

static CSphere *g_pSpheres;

void RenderAllSpheres( float flTime, float flFrametime )
{
	static CSphere s_Spheres;

	if ( g_pSpheres )
		g_pSpheres->Render( flTime, flFrametime );
	else
		g_pSpheres = &s_Spheres;
}

void SphereVidInit( void )
{
	if ( g_pSpheres )
	{
		g_pSpheres->m_pSprite = NULL;
		g_pSpheres->m_pWhite = NULL;

		for ( int i = 0; i < MAX_SPHERES; i++ )
			g_pSpheres->m_Spheres[i].alpha = 0;
	}
}

//=========================================================
// events/blastsphere.sc: a sphere of the entity's render
// colour growing to iparam1 at iparam2 units a second,
// fading after fparam1 s at fparam2 a second; bparam1 adds
// a yellowish shell 40 units smaller and 100 u/s faster
//=========================================================
extern "C" void EV_BlastSphere( event_args_t *args )
{
	cl_entity_t *pEnt = gEngfuncs.GetEntityByIndex( args->entindex );
	Vector vecColor;

	vecColor.x = pEnt->curstate.rendercolor.r;
	vecColor.y = pEnt->curstate.rendercolor.g;
	vecColor.z = pEnt->curstate.rendercolor.b;

	if ( args->bparam1 )
		g_pSpheres->Create( args->origin, Vector( 255, 255, 64 ), args->iparam1 - 40, args->iparam2 + 100, args->fparam1, args->fparam2, 1 );

	g_pSpheres->Create( args->origin, vecColor, args->iparam1, args->iparam2, args->fparam1, args->fparam2, 0 );
}

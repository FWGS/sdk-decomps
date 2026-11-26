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
//
// util.cpp
//
// implementation of class-less helper functions
//

#include "STDIO.H"
#include "STDLIB.H"
#include "MATH.H"

#include "hud.h"
#include "cl_util.h"
#include <string.h>

#ifndef M_PI
#define M_PI		3.14159265358979323846	// matches value in gcc v2 math.h
#endif

vec3_t vec3_origin( 0, 0, 0 );

double sqrt(double x);

float Length(const float *v)
{
	int		i;
	float	length;
	
	length = 0;
	for (i=0 ; i< 3 ; i++)
		length += v[i]*v[i];
	length = sqrt (length);		// FIXME

	return length;
}

void VectorAngles( const float *forward, float *angles )
{
	float	tmp, yaw, pitch;
	
	if (forward[1] == 0 && forward[0] == 0)
	{
		yaw = 0;
		if (forward[2] > 0)
			pitch = 90;
		else
			pitch = 270;
	}
	else
	{
		yaw = (atan2(forward[1], forward[0]) * 180 / M_PI);
		if (yaw < 0)
			yaw += 360;

		tmp = sqrt (forward[0]*forward[0] + forward[1]*forward[1]);
		pitch = (atan2(forward[2], tmp) * 180 / M_PI);
		if (pitch < 0)
			pitch += 360;
	}
	
	angles[0] = pitch;
	angles[1] = yaw;
	angles[2] = 0;
}

float VectorNormalize (float *v)
{
	float	length, ilength;

	length = v[0]*v[0] + v[1]*v[1] + v[2]*v[2];
	length = sqrt (length);		// FIXME

	if (length)
	{
		ilength = 1/length;
		v[0] *= ilength;
		v[1] *= ilength;
		v[2] *= ilength;
	}
		
	return length;

}

void VectorInverse ( float *v )
{
	v[0] = -v[0];
	v[1] = -v[1];
	v[2] = -v[2];
}

void VectorScale (const float *in, float scale, float *out)
{
	out[0] = in[0]*scale;
	out[1] = in[1]*scale;
	out[2] = in[2]*scale;
}

void VectorMA (const float *veca, float scale, const float *vecb, float *vecc)
{
	vecc[0] = veca[0] + scale*vecb[0];
	vecc[1] = veca[1] + scale*vecb[1];
	vecc[2] = veca[2] + scale*vecb[2];
}

HSPRITE LoadSprite(const char *pszName)
{
	int i;
	char sz[256]; 

	// Gunman: 640 sprites only above 640 wide
	if (ScreenWidth <= 640)
		i = 320;
	else
		i = 640;

	sprintf(sz, pszName, i);

	return SPR_Load(sz);
}

// sic: placeholders, ours: functions retail was linked with and dropped
// (nothing calls them); their text is unknown. They give LINK's
// identical-copy sort the (size, relocations) keys with which it keeps
// retail's copies (see Unknown_W1_1 in ev_hldm.cpp): a fit, not evidence.
struct cl_enginefuncs_s;
extern struct cl_enginefuncs_s gEngfuncs;
void InitInput( void );

// key (48, 0)
int Unknown_end_util_1( int *p )
{
	p[0] = 1;
	p[1] = 2;
	p[2] = 3;
	p[3] = 4;
	return 0;
}

// key (1264, 59), 59 absolute
int Unknown_end_util_2( int *p )
{
	p[0] = 1;
	p[1] = 2;
	p[2] = 3;
	p[3] = 4;
	p[4] = 5;
	p[5] = 6;
	p[6] = 7;
	p[7] = 8;
	p[8] = 9;
	p[9] = 10;
	p[10] = 11;
	p[11] = 12;
	p[12] = 13;
	p[13] = 14;
	p[14] = 15;
	p[15] = 16;
	p[16] = 17;
	p[17] = 18;
	p[18] = 19;
	p[19] = 20;
	p[20] = 21;
	p[21] = 22;
	p[22] = 23;
	p[23] = 24;
	p[24] = 25;
	p[25] = 26;
	p[26] = 27;
	p[27] = 28;
	p[28] = 29;
	p[29] = 30;
	p[30] = 31;
	p[31] = 32;
	p[32] = 33;
	p[33] = 34;
	p[34] = 35;
	p[35] = 36;
	p[36] = 37;
	p[37] = 38;
	p[38] = 39;
	p[39] = 40;
	p[40] = 41;
	p[41] = 42;
	p[42] = 43;
	p[43] = 44;
	p[44] = 45;
	p[45] = 46;
	p[46] = 47;
	p[47] = 48;
	p[48] = 49;
	p[49] = 50;
	p[50] = 51;
	p[51] = 52;
	p[52] = 53;
	p[53] = 54;
	p[54] = 55;
	p[55] = 56;
	p[56] = 57;
	p[57] = 58;
	p[58] = 59;
	p[59] = 60;
	p[60] = 61;
	p[61] = 62;
	p[62] = 63;
	p[63] = 64;
	p[64] = 65;
	p[65] = 66;
	p[66] = 67;
	p[67] = 68;
	p[68] = 69;
	p[69] = 70;
	p[70] = 71;
	p[71] = 72;
	p[72] = 73;
	p[73] = 74;
	p[74] = 75;
	p[75] = 76;
	p[76] = 77;
	p[77] = 78;
	p[78] = 79;
	p[79] = 80;
	p[80] = 81;
	p[81] = 82;
	p[82] = 83;
	p[83] = 84;
	p[84] = 85;
	p[85] = 86;
	p[86] = 87;
	p[87] = 88;
	return ((int *)&gEngfuncs)[1] + ((int *)&gEngfuncs)[2] + ((int *)&gEngfuncs)[3] + ((int *)&gEngfuncs)[4] + ((int *)&gEngfuncs)[5] + ((int *)&gEngfuncs)[6] + ((int *)&gEngfuncs)[7] + ((int *)&gEngfuncs)[8] + ((int *)&gEngfuncs)[9] + ((int *)&gEngfuncs)[10] + ((int *)&gEngfuncs)[11] + ((int *)&gEngfuncs)[12] + ((int *)&gEngfuncs)[13] + ((int *)&gEngfuncs)[14] + ((int *)&gEngfuncs)[15] + ((int *)&gEngfuncs)[16] + ((int *)&gEngfuncs)[17] + ((int *)&gEngfuncs)[18] + ((int *)&gEngfuncs)[19] + ((int *)&gEngfuncs)[20] + ((int *)&gEngfuncs)[21] + ((int *)&gEngfuncs)[22] + ((int *)&gEngfuncs)[23] + ((int *)&gEngfuncs)[24] + ((int *)&gEngfuncs)[25] + ((int *)&gEngfuncs)[26] + ((int *)&gEngfuncs)[27] + ((int *)&gEngfuncs)[28] + ((int *)&gEngfuncs)[29] + ((int *)&gEngfuncs)[30] + ((int *)&gEngfuncs)[31] + ((int *)&gEngfuncs)[32] + ((int *)&gEngfuncs)[33] + ((int *)&gEngfuncs)[34] + ((int *)&gEngfuncs)[35] + ((int *)&gEngfuncs)[36] + ((int *)&gEngfuncs)[37] + ((int *)&gEngfuncs)[38] + ((int *)&gEngfuncs)[39] + ((int *)&gEngfuncs)[40] + ((int *)&gEngfuncs)[41] + ((int *)&gEngfuncs)[42] + ((int *)&gEngfuncs)[43] + ((int *)&gEngfuncs)[44] + ((int *)&gEngfuncs)[45] + ((int *)&gEngfuncs)[46] + ((int *)&gEngfuncs)[47] + ((int *)&gEngfuncs)[48] + ((int *)&gEngfuncs)[49] + ((int *)&gEngfuncs)[50] + ((int *)&gEngfuncs)[51] + ((int *)&gEngfuncs)[52] + ((int *)&gEngfuncs)[53] + ((int *)&gEngfuncs)[54] + ((int *)&gEngfuncs)[55] + ((int *)&gEngfuncs)[56] + ((int *)&gEngfuncs)[57] + ((int *)&gEngfuncs)[58] + ((int *)&gEngfuncs)[59];
}

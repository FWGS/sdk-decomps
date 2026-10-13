/***
*
*	Copyright (c) 1999, 2000 Valve LLC. All rights reserved.
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
#ifndef DECALS_H
#define DECALS_H

//
// Dynamic Decals
//
enum decal_e 
{	
	DECAL_GUNSHOT1 = 0,
	DECAL_GUNSHOT2,
	DECAL_GUNSHOT3,
	DECAL_GUNSHOT4,
	DECAL_GUNSHOT5,
	DECAL_LAMBDA1,
	DECAL_LAMBDA2,
	DECAL_LAMBDA3,
	DECAL_LAMBDA4,
	DECAL_LAMBDA5,
	DECAL_LAMBDA6,
	DECAL_SCORCH1,
	DECAL_SCORCH2,
	DECAL_YBLOOD1,
	DECAL_YBLOOD2,
	DECAL_YBLOOD3,
	DECAL_YBLOOD4,
	DECAL_YBLOOD5,
	DECAL_YBLOOD6,
	DECAL_GLASSBREAK1,
	DECAL_GLASSBREAK2,
	DECAL_GLASSBREAK3,
	DECAL_BIGSHOT1,
	DECAL_BIGSHOT2,
	DECAL_BIGSHOT3,
	DECAL_BIGSHOT4,
	DECAL_BIGSHOT5,
	DECAL_SPIT1,
	DECAL_SPIT2,
	DECAL_BPROOF1,
	DECAL_SMALLSCORCH1,
	DECAL_SMALLSCORCH2,
	DECAL_SMALLSCORCH3,
	DECAL_OIL1,
	DECAL_OIL2,
	DECAL_OIL3,
	DECAL_OIL4,
	DECAL_OIL5,
	DECAL_OIL6,
	DECAL_XENOME1,
	DECAL_XENOME2,
	DECAL_XENOME3,
	DECAL_XENOME4,
	DECAL_XENOME5,
	DECAL_XENOME6,
	DECAL_TUBE1,
	DECAL_TUBE2,
	DECAL_TUBE3,
	DECAL_TUBE4,
	DECAL_TUBE5,
	DECAL_TUBE6,
	DECAL_BEAK1,
	DECAL_BEAK2,
	DECAL_BEAK3,
	DECAL_BEAK4,
	DECAL_BEAK5,
	DECAL_BEAK6,
	DECAL_TUBEBIG,
	DECAL_XENOBIG,
	DECAL_OILBIG,
	DECAL_BEAKBIG,
	DECAL_HUMAN1,
	DECAL_HUMAN2,
	DECAL_HUMAN3,
	DECAL_HUMAN4,
	DECAL_HUMAN5,
	DECAL_HUMAN6,
	DECAL_CG_GREEN,
	DECAL_CG_YELLOW,
	DECAL_CG_ORANGE,
	DECAL_CG_RED,
	DECAL_BALBURN,
	DECAL_EGG1,
	DECAL_EGG2,
	DECAL_MAGGOT,
	DECAL_CRATER3,
	DECAL_HUMANBIG,
	DECAL_CHARGE,
	DECAL_PULSE,
};

typedef struct 
{
	char	*name;
	int		index;
} DLL_DECALLIST;

extern DLL_DECALLIST gDecals[];

#endif	// DECALS_H

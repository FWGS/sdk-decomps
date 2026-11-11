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
#ifndef ANIMATION_H
#define ANIMATION_H

#define ACTIVITY_NOT_AVAILABLE		-1

#ifndef MONSTEREVENT_H
#include "monsterevent.h"
#endif

extern int IsSoundEvent( int eventNumber );

// sic: a stand-in for one declaration retail had where animating.cpp sees it and
// util.cpp, plats.cpp and maprules.cpp do not. VC5 orders some x87 operands by the
// number of declarations a file has seen;
// CBaseAnimating::SetSequenceBox needs 1 more here (or 3, 4,
// 5 or 7, mod 8). What it was the image cannot say.
typedef int sic_declcount_animation_h;

int LookupActivity( void *pmodel, entvars_t *pev, int activity );
int LookupActivityHeaviest( void *pmodel, entvars_t *pev, int activity );
int LookupSequence( void *pmodel, const char *label );
void GetSequenceInfo( void *pmodel, entvars_t *pev, float *pflFrameRate, float *pflGroundSpeed );
int GetSequenceFlags( void *pmodel, entvars_t *pev );
int LookupAnimationEvents( void *pmodel, entvars_t *pev, float flStart, float flEnd );
float SetController( void *pmodel, entvars_t *pev, int iController, float flValue );
float SetBlending( void *pmodel, entvars_t *pev, int iBlender, float flValue );
void GetEyePosition( void *pmodel, float *vecEyePosition );
void SequencePrecache( void *pmodel, const char *pSequenceName );
int FindTransition( void *pmodel, int iEndingAnim, int iGoalAnim, int *piDir );
void SetBodygroup( void *pmodel, entvars_t *pev, int iGroup, int iValue );
int GetBodygroup( void *pmodel, entvars_t *pev, int iGroup );

int GetAnimationEvent( void *pmodel, entvars_t *pev, MonsterEvent_t *pMonsterEvent, float flStart, float flEnd, int index );
int ExtractBbox( void *pmodel, int sequence, float *mins, float *maxs );

// From /engine/studio.h
#define STUDIO_LOOPING		0x0001


#endif	//ANIMATION_H

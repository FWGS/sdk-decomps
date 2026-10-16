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

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"
#include "gamerules.h"
#include "items.h"


// POV: the crowbar is a prop that can't be taken
class CCrowbar : public CItem
{
public:
	void Spawn( void );
	void Precache( void );
	BOOL MyTouch( CBasePlayer *pPlayer );
};
LINK_ENTITY_TO_CLASS( weapon_crowbar, CCrowbar );


void CCrowbar::Spawn( )
{
	Precache( );
	SET_MODEL(ENT(pev), "models/w_crowbar.mdl");
	CItem::Spawn( );
	UTIL_MakeProp( this );
}


void CCrowbar::Precache( void )
{
	PRECACHE_MODEL("models/w_crowbar.mdl");
	PRECACHE_SOUND("weapons/cbar_hit1.wav");
	PRECACHE_SOUND("weapons/cbar_hit2.wav");
	PRECACHE_SOUND("weapons/cbar_hitbod1.wav");
	PRECACHE_SOUND("weapons/cbar_hitbod2.wav");
	PRECACHE_SOUND("weapons/cbar_hitbod3.wav");
	PRECACHE_SOUND("weapons/cbar_miss1.wav");
}

BOOL CCrowbar::MyTouch( CBasePlayer *pPlayer )
{
	return FALSE;
}


// Retail has this in POV's alien hands file (weapon_xhand, 0x10094410);
// it stays here until that file is written.
void FindHullIntersection( const Vector &vecSrc, TraceResult &tr, float *mins, float *maxs, edict_t *pEntity )
{
	int			i, j, k;
	float		distance;
	float		*minmaxs[2] = {mins, maxs};
	TraceResult tmpTrace;
	Vector		vecHullEnd = tr.vecEndPos;
	Vector		vecEnd;

	distance = 1e6f;

	vecHullEnd = vecSrc + ((vecHullEnd - vecSrc)*2);
	UTIL_TraceLine( vecSrc, vecHullEnd, dont_ignore_monsters, pEntity, &tmpTrace );
	if ( tmpTrace.flFraction < 1.0 )
	{
		tr = tmpTrace;
		return;
	}

	for ( i = 0; i < 2; i++ )
	{
		for ( j = 0; j < 2; j++ )
		{
			for ( k = 0; k < 2; k++ )
			{
				vecEnd.x = vecHullEnd.x + minmaxs[i][0];
				vecEnd.y = vecHullEnd.y + minmaxs[j][1];
				vecEnd.z = vecHullEnd.z + minmaxs[k][2];

				UTIL_TraceLine( vecSrc, vecEnd, dont_ignore_monsters, pEntity, &tmpTrace );
				if ( tmpTrace.flFraction < 1.0 )
				{
					float thisDistance = (tmpTrace.vecEndPos - vecSrc).Length();
					if ( thisDistance < distance )
					{
						tr = tmpTrace;
						distance = thisDistance;
					}
				}
			}
		}
	}
}

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
#if !defined( OEM_BUILD ) && !defined( HLDEMO_BUILD )

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "effects.h"
#include "customentity.h"
#include "gamerules.h"
#include "items.h"

#define	EGON_PRIMARY_VOLUME		450
#define EGON_BEAM_SPRITE		"sprites/xbeam1.spr"
#define EGON_FLARE_SPRITE		"sprites/XSpark1.spr"
#define EGON_SOUND_OFF			"weapons/egon_off1.wav"
#define EGON_SOUND_RUN			"weapons/egon_run3.wav"
#define EGON_SOUND_STARTUP		"weapons/egon_windup2.wav"

// POV: the egon is a prop that can't be taken
class CEgon : public CItem
{
public:
	void Spawn( void )
	{
		Precache( );
		SET_MODEL(ENT(pev), "models/w_egon.mdl");
		CItem::Spawn( );
		UTIL_MakeProp( this );
	}
	void Precache( void )
	{
		PRECACHE_MODEL("models/w_egon.mdl");

		PRECACHE_MODEL("models/w_9mmclip.mdl");
		PRECACHE_SOUND("items/9mmclip1.wav");

		PRECACHE_SOUND( EGON_SOUND_OFF );
		PRECACHE_SOUND( EGON_SOUND_RUN );
		PRECACHE_SOUND( EGON_SOUND_STARTUP );

		PRECACHE_MODEL( EGON_BEAM_SPRITE );
		PRECACHE_MODEL( EGON_FLARE_SPRITE );

		PRECACHE_SOUND ("weapons/357_cock1.wav");
	}
	BOOL MyTouch( CBasePlayer *pPlayer )
	{
		return FALSE;
	}
};

LINK_ENTITY_TO_CLASS( weapon_egon, CEgon );

#endif

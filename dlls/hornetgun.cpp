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
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"
#include "hornet.h"
#include "gamerules.h"
#include "items.h"


#define SF_HGUN_INVLIGHT	1	// POV, name ours

// POV: the hivehand is a prop; touching it gives the player POV's single
// and then dual hornet guns
class CHgun : public CItem
{
public:
	void Spawn( void )
	{
		Precache( );
		SET_MODEL(ENT(pev), "models/w_hgun.mdl");

		if ( pev->spawnflags & SF_HGUN_INVLIGHT )
			pev->effects = EF_INVLIGHT;

		CItem::Spawn( );
	}
	void Precache( void )
	{
		PRECACHE_MODEL("models/v_hgun.mdl");
		PRECACHE_MODEL("models/w_hgun.mdl");
		PRECACHE_MODEL("models/p_hgun.mdl");

		UTIL_PrecacheOther("hornet");
	}
	BOOL MyTouch( CBasePlayer *pPlayer )
	{
		if ( pPlayer->m_iHornetStatus != 2 )
		{
			if ( pPlayer->m_iHornetStatus == 0 )
			{
				pPlayer->GiveNamedItem( "weapon_hornetgunsingle" );
				pPlayer->m_iHornetStatus = 1;
				return TRUE;
			}
			if ( pPlayer->m_iHornetStatus == 1 )
			{
				pPlayer->GiveNamedItem( "weapon_hornetgundual" );
				pPlayer->m_iHornetStatus = 2;
				return TRUE;
			}
		}
		return FALSE;
	}
};
LINK_ENTITY_TO_CLASS( weapon_hornetgun, CHgun );

#endif

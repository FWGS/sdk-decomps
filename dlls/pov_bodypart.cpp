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
//=========================================================
// POV: detail_bodypart, a gib laid about the map as a prop.
// pev->body picks the part: 0 any, 1 any human gib, 2 the
// grunt's, 3 and up one of hgibs.mdl's bodies. File name,
// class name and field names are ours.
//=========================================================

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "weapons.h"
#include "player.h"
#include "skill.h"
#include "items.h"
#include "gamerules.h"

#define SF_DETAIL_RANDOMYAW		8	// name ours

class CDetailBodypart : public CItem
{
public:
	void	Spawn( void );
	void	Precache( void );
	BOOL	MyTouch( CBasePlayer *pPlayer ) { return FALSE; }

	char	m_szModel[64];
};

LINK_ENTITY_TO_CLASS( detail_bodypart, CDetailBodypart );

void CDetailBodypart::Spawn( void )
{
	Precache( );
	SET_MODEL( ENT(pev), m_szModel );

	if ( pev->spawnflags & SF_DETAIL_RANDOMYAW )
		pev->angles = Vector( 0, RANDOM_LONG( 0, 360 ), 0 );

	CItem::Spawn( );
}

void CDetailBodypart::Precache( void )
{
	int iPart;

	switch ( pev->body )
	{
	case 0:	// any part
		iPart = RANDOM_LONG( 0, 8 );
		if ( iPart == 8 )
		{
			PRECACHE_MODEL( "models/gib_hgrunt.mdl" );
			strcpy( m_szModel, "models/gib_hgrunt.mdl" );
			pev->body = 0;
		}
		else
		{
			PRECACHE_MODEL( "models/hgibs.mdl" );
			strcpy( m_szModel, "models/hgibs.mdl" );
			if ( iPart == 7 )
				pev->body = RANDOM_LONG( 7, 10 );
			else
				pev->body = iPart;
		}
		break;
	case 1:	// any of hgibs.mdl
		PRECACHE_MODEL( "models/hgibs.mdl" );
		strcpy( m_szModel, "models/hgibs.mdl" );
		iPart = RANDOM_LONG( 0, 7 );
		if ( iPart == 7 )
			pev->body = RANDOM_LONG( 7, 10 );
		else
			pev->body = iPart;
		break;
	case 2:
		PRECACHE_MODEL( "models/gib_hgrunt.mdl" );
		strcpy( m_szModel, "models/gib_hgrunt.mdl" );
		pev->body = 0;
		break;
	case 3:
		PRECACHE_MODEL( "models/hgibs.mdl" );
		strcpy( m_szModel, "models/hgibs.mdl" );
		pev->body = 0;
		break;
	case 4:
		PRECACHE_MODEL( "models/hgibs.mdl" );
		strcpy( m_szModel, "models/hgibs.mdl" );
		pev->body = 1;
		break;
	case 5:
		PRECACHE_MODEL( "models/hgibs.mdl" );
		strcpy( m_szModel, "models/hgibs.mdl" );
		pev->body = 2;
		break;
	case 6:
		PRECACHE_MODEL( "models/hgibs.mdl" );
		strcpy( m_szModel, "models/hgibs.mdl" );
		pev->body = 3;
		break;
	case 7:
		PRECACHE_MODEL( "models/hgibs.mdl" );
		strcpy( m_szModel, "models/hgibs.mdl" );
		pev->body = 4;
		break;
	case 8:
		PRECACHE_MODEL( "models/hgibs.mdl" );
		strcpy( m_szModel, "models/hgibs.mdl" );
		pev->body = 5;
		break;
	case 9:
		PRECACHE_MODEL( "models/hgibs.mdl" );
		strcpy( m_szModel, "models/hgibs.mdl" );
		pev->body = 6;
		break;
	case 10:
		PRECACHE_MODEL( "models/hgibs.mdl" );
		strcpy( m_szModel, "models/hgibs.mdl" );
		pev->body = 7;
		break;
	case 11:
		PRECACHE_MODEL( "models/hgibs.mdl" );
		strcpy( m_szModel, "models/hgibs.mdl" );
		pev->body = 8;
		break;
	case 12:
		PRECACHE_MODEL( "models/hgibs.mdl" );
		strcpy( m_szModel, "models/hgibs.mdl" );
		pev->body = 9;
		break;
	case 13:
		PRECACHE_MODEL( "models/hgibs.mdl" );
		strcpy( m_szModel, "models/hgibs.mdl" );
		pev->body = 10;
		break;
	}
}

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
// POV: detail_object, a piece of garbage, a can or a gib
// of an office or of a military, medical or tech crate laid
// about the map. pev->body picks it: N00 a random one of
// group N, N01 and up a given one. File name, class name and
// field names are ours.
//=========================================================

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "weapons.h"
#include "player.h"
#include "skill.h"
#include "items.h"
#include "gamerules.h"

#define SF_DETAIL_NODROP		2	// name ours: stays where it is placed
#define SF_DETAIL_RANDOMYAW		8	// name ours

class CDetailObject : public CItem
{
public:
	void	Spawn( void );
	void	Precache( void );
	BOOL	MyTouch( CBasePlayer *pPlayer ) { return FALSE; }

	char	m_szModel[64];
};

LINK_ENTITY_TO_CLASS( detail_object, CDetailObject );

void CDetailObject::Spawn( void )
{
	Precache( );
	SET_MODEL( ENT(pev), m_szModel );

	if ( pev->spawnflags & SF_DETAIL_RANDOMYAW )
		pev->angles = Vector( 0, RANDOM_LONG( 0, 360 ), 0 );

	pev->movetype = MOVETYPE_NONE;
	pev->solid = SOLID_NOT;
	UTIL_SetOrigin( pev, pev->origin );
	UTIL_SetSize( pev, g_vecZero, g_vecZero );
	SetTouch(ItemTouch);

	if ( !( pev->spawnflags & SF_DETAIL_NODROP ) && DROP_TO_FLOOR(ENT(pev)) == 0 )
	{
		ALERT(at_error, "Item %s fell out of level at %f,%f,%f", STRING( pev->classname ), pev->origin.x, pev->origin.y, pev->origin.z);
		UTIL_Remove( this );
		return;
	}
}

void CDetailObject::Precache( void )
{
	int iRand;

	switch ( pev->body )
	{
	case 200:	// any garbage
		iRand = RANDOM_LONG( 0, 6 );
		if ( iRand == 6 )
		{
			pev->body = 300;	// a can
			Precache( );
			return;
		}

		PRECACHE_MODEL( "models/garbagegibs.mdl" );
		strcpy( m_szModel, "models/garbagegibs.mdl" );
		switch ( iRand )
		{
		case 0:	pev->body = 0;	break;
		case 1:	pev->body = 1;	break;
		case 2:	pev->body = 4;	break;
		case 3:	pev->body = 5;	break;
		case 4:	pev->body = 6;	break;
		default:	pev->body = 7;	break;
		}
		break;
	case 300:	// any can
		iRand = RANDOM_LONG( 0, 6 );
		if ( iRand == 6 )
		{
			PRECACHE_MODEL( "models/garbagegibs.mdl" );
			strcpy( m_szModel, "models/garbagegibs.mdl" );
			pev->body = 11;
		}
		else
		{
			PRECACHE_MODEL( "models/can.mdl" );
			strcpy( m_szModel, "models/can.mdl" );
			pev->body = 0;
			pev->skin = iRand;
		}
		break;
	case 301:
		PRECACHE_MODEL( "models/can.mdl" );
		strcpy( m_szModel, "models/can.mdl" );
		pev->body = 0;
		pev->skin = 0;	// sic: every can has the first skin
		break;
	case 302:
		PRECACHE_MODEL( "models/can.mdl" );
		strcpy( m_szModel, "models/can.mdl" );
		pev->body = 0;
		pev->skin = 0;	// sic: every can has the first skin
		break;
	case 303:
		PRECACHE_MODEL( "models/can.mdl" );
		strcpy( m_szModel, "models/can.mdl" );
		pev->body = 0;
		pev->skin = 0;	// sic: every can has the first skin
		break;
	case 304:
		PRECACHE_MODEL( "models/can.mdl" );
		strcpy( m_szModel, "models/can.mdl" );
		pev->body = 0;
		pev->skin = 0;	// sic: every can has the first skin
		break;
	case 305:
		PRECACHE_MODEL( "models/can.mdl" );
		strcpy( m_szModel, "models/can.mdl" );
		pev->body = 0;
		pev->skin = 0;	// sic: every can has the first skin
		break;
	case 306:
		PRECACHE_MODEL( "models/can.mdl" );
		strcpy( m_szModel, "models/can.mdl" );
		pev->body = 0;
		pev->skin = 0;	// sic: every can has the first skin
		break;
	case 307:
		PRECACHE_MODEL( "models/garbagegibs.mdl" );
		strcpy( m_szModel, "models/garbagegibs.mdl" );
		pev->body = 11;
		break;
	case 400:
		PRECACHE_MODEL( "models/garbagegibs.mdl" );
		strcpy( m_szModel, "models/garbagegibs.mdl" );
		pev->body = RANDOM_LONG( 0, 1 );
		break;
	case 401:
		PRECACHE_MODEL( "models/garbagegibs.mdl" );
		strcpy( m_szModel, "models/garbagegibs.mdl" );
		pev->body = 0;
		break;
	case 402:
		PRECACHE_MODEL( "models/garbagegibs.mdl" );
		strcpy( m_szModel, "models/garbagegibs.mdl" );
		pev->body = 1;
		break;
	case 500:
		iRand = RANDOM_LONG( 4, 8 );
		if ( iRand == 8 )
		{
			pev->body = 300;	// a can
			Precache( );
			return;
		}

		PRECACHE_MODEL( "models/garbagegibs.mdl" );
		strcpy( m_szModel, "models/garbagegibs.mdl" );
		pev->body = iRand;
		break;
	case 501:
		PRECACHE_MODEL( "models/garbagegibs.mdl" );
		strcpy( m_szModel, "models/garbagegibs.mdl" );
		pev->body = 4;
		break;
	case 502:
		PRECACHE_MODEL( "models/garbagegibs.mdl" );
		strcpy( m_szModel, "models/garbagegibs.mdl" );
		pev->body = 5;
		break;
	case 503:
		PRECACHE_MODEL( "models/garbagegibs.mdl" );
		strcpy( m_szModel, "models/garbagegibs.mdl" );
		pev->body = 6;
		break;
	case 504:
		PRECACHE_MODEL( "models/garbagegibs.mdl" );
		strcpy( m_szModel, "models/garbagegibs.mdl" );
		pev->body = 7;
		break;
	case 505:
		pev->body = 300;	// a can
		Precache( );
		break;
	case 600:
		PRECACHE_MODEL( "models/garbagegibs.mdl" );
		strcpy( m_szModel, "models/garbagegibs.mdl" );
		pev->body = RANDOM_LONG( 8, 8 );
		break;
	case 601:
		PRECACHE_MODEL( "models/garbagegibs.mdl" );
		strcpy( m_szModel, "models/garbagegibs.mdl" );
		pev->body = 8;
		break;
	case 700:
		PRECACHE_MODEL( "models/garbagegibs.mdl" );
		strcpy( m_szModel, "models/garbagegibs.mdl" );
		pev->body = RANDOM_LONG( 9, 9 );
		break;
	case 701:
		PRECACHE_MODEL( "models/garbagegibs.mdl" );
		strcpy( m_szModel, "models/garbagegibs.mdl" );
		pev->body = 9;
		break;
	case 800:	// any office gib
		iRand = RANDOM_LONG( 0, 12 );
		if ( iRand == 11 )
		{
			pev->body = 600;
			Precache( );
			return;
		}
		if ( iRand == 12 )
		{
			pev->body = 700;
			Precache( );
			return;
		}

		PRECACHE_MODEL( "models/office_gibs.mdl" );
		strcpy( m_szModel, "models/office_gibs.mdl" );
		pev->body = iRand;
		break;
	case 900:
		PRECACHE_MODEL( "models/office_gibs.mdl" );
		strcpy( m_szModel, "models/office_gibs.mdl" );
		pev->body = RANDOM_LONG( 0, 6 );
		break;
	case 901:
		PRECACHE_MODEL( "models/office_gibs.mdl" );
		strcpy( m_szModel, "models/office_gibs.mdl" );
		pev->body = 0;
		break;
	case 902:
		PRECACHE_MODEL( "models/office_gibs.mdl" );
		strcpy( m_szModel, "models/office_gibs.mdl" );
		pev->body = 1;
		break;
	case 903:
		PRECACHE_MODEL( "models/office_gibs.mdl" );
		strcpy( m_szModel, "models/office_gibs.mdl" );
		pev->body = 2;
		break;
	case 904:
		PRECACHE_MODEL( "models/office_gibs.mdl" );
		strcpy( m_szModel, "models/office_gibs.mdl" );
		pev->body = 3;
		break;
	case 905:
		PRECACHE_MODEL( "models/office_gibs.mdl" );
		strcpy( m_szModel, "models/office_gibs.mdl" );
		pev->body = 4;
		break;
	case 906:
		PRECACHE_MODEL( "models/office_gibs.mdl" );
		strcpy( m_szModel, "models/office_gibs.mdl" );
		pev->body = 5;
		break;
	case 907:
		PRECACHE_MODEL( "models/office_gibs.mdl" );
		strcpy( m_szModel, "models/office_gibs.mdl" );
		pev->body = 6;
		break;
	case 1000:
		PRECACHE_MODEL( "models/office_gibs.mdl" );
		strcpy( m_szModel, "models/office_gibs.mdl" );
		pev->body = RANDOM_LONG( 7, 8 );
		break;
	case 1001:
		PRECACHE_MODEL( "models/office_gibs.mdl" );
		strcpy( m_szModel, "models/office_gibs.mdl" );
		pev->body = 7;
		break;
	case 1002:
		PRECACHE_MODEL( "models/office_gibs.mdl" );
		strcpy( m_szModel, "models/office_gibs.mdl" );
		pev->body = 8;
		break;
	case 1100:
		PRECACHE_MODEL( "models/office_gibs.mdl" );
		strcpy( m_szModel, "models/office_gibs.mdl" );
		pev->body = RANDOM_LONG( 9, 10 );
		break;
	case 1101:
		PRECACHE_MODEL( "models/office_gibs.mdl" );
		strcpy( m_szModel, "models/office_gibs.mdl" );
		pev->body = 9;
		break;
	case 1102:
		PRECACHE_MODEL( "models/office_gibs.mdl" );
		strcpy( m_szModel, "models/office_gibs.mdl" );
		pev->body = 10;
		break;
	case 1200:	// any military crate gib
		PRECACHE_MODEL( "models/mil_crategibs.mdl" );
		strcpy( m_szModel, "models/mil_crategibs.mdl" );
		switch ( RANDOM_LONG( 0, 7 ) )
		{
		case 0:	pev->body = 5;	break;
		case 1:	pev->body = 7;	break;
		case 2:	pev->body = 8;	break;
		case 3:	pev->body = 9;	break;
		case 4:	pev->body = 13;	break;
		case 5:	pev->body = 14;	break;
		case 6:	pev->body = 15;	break;
		default:	pev->body = 16;	break;
		}
		break;
	case 1300:
		PRECACHE_MODEL( "models/mil_crategibs.mdl" );
		strcpy( m_szModel, "models/mil_crategibs.mdl" );
		pev->body = RANDOM_LONG( 5, 5 );
		break;
	case 1301:
		PRECACHE_MODEL( "models/mil_crategibs.mdl" );
		strcpy( m_szModel, "models/mil_crategibs.mdl" );
		pev->body = 5;
		break;
	case 1400:
		PRECACHE_MODEL( "models/mil_crategibs.mdl" );
		strcpy( m_szModel, "models/mil_crategibs.mdl" );
		pev->body = RANDOM_LONG( 9, 9 );
		break;
	case 1401:
		PRECACHE_MODEL( "models/mil_crategibs.mdl" );
		strcpy( m_szModel, "models/mil_crategibs.mdl" );
		pev->body = 9;
		break;
	case 1500:
		PRECACHE_MODEL( "models/mil_crategibs.mdl" );
		strcpy( m_szModel, "models/mil_crategibs.mdl" );
		if ( RANDOM_LONG( 0, 1 ) == 1 )
			pev->body = 16;
		else
			pev->body = 7;
		break;
	case 1501:
		PRECACHE_MODEL( "models/mil_crategibs.mdl" );
		strcpy( m_szModel, "models/mil_crategibs.mdl" );
		pev->body = 7;
		break;
	case 1502:
		PRECACHE_MODEL( "models/mil_crategibs.mdl" );
		strcpy( m_szModel, "models/mil_crategibs.mdl" );
		pev->body = 16;
		break;
	case 1600:
		PRECACHE_MODEL( "models/mil_crategibs.mdl" );
		strcpy( m_szModel, "models/mil_crategibs.mdl" );
		if ( RANDOM_LONG( 0, 1 ) == 1 )
			pev->body = 13;
		else
			pev->body = 8;
		break;
	case 1601:
		PRECACHE_MODEL( "models/mil_crategibs.mdl" );
		strcpy( m_szModel, "models/mil_crategibs.mdl" );
		pev->body = 8;
		break;
	case 1602:
		PRECACHE_MODEL( "models/mil_crategibs.mdl" );
		strcpy( m_szModel, "models/mil_crategibs.mdl" );
		pev->body = 13;
		break;
	case 1700:
		PRECACHE_MODEL( "models/mil_crategibs.mdl" );
		strcpy( m_szModel, "models/mil_crategibs.mdl" );
		pev->body = RANDOM_LONG( 14, 14 );
		break;
	case 1701:
		PRECACHE_MODEL( "models/mil_crategibs.mdl" );
		strcpy( m_szModel, "models/mil_crategibs.mdl" );
		pev->body = 14;
		break;
	case 1800:
		PRECACHE_MODEL( "models/mil_crategibs.mdl" );
		strcpy( m_szModel, "models/mil_crategibs.mdl" );
		pev->body = RANDOM_LONG( 15, 15 );
		break;
	case 1801:
		PRECACHE_MODEL( "models/mil_crategibs.mdl" );
		strcpy( m_szModel, "models/mil_crategibs.mdl" );
		pev->body = 15;
		break;
	case 1900:	// any medical crate gib
		PRECACHE_MODEL( "models/med_crategibs.mdl" );
		strcpy( m_szModel, "models/med_crategibs.mdl" );
		switch ( RANDOM_LONG( 0, 8 ) )
		{
		case 0:	pev->body = 6;	break;
		case 1:	pev->body = 7;	break;
		case 2:	pev->body = 8;	break;
		case 3:	pev->body = 9;	break;
		case 4:	pev->body = 10;	break;
		case 5:	pev->body = 11;	break;
		case 6:	pev->body = 14;	break;
		case 7:	pev->body = 15;	break;
		default:	pev->body = 16;	break;
		}
		break;
	case 2000:
		PRECACHE_MODEL( "models/med_crategibs.mdl" );
		strcpy( m_szModel, "models/med_crategibs.mdl" );
		pev->body = RANDOM_LONG( 6, 6 );
		break;
	case 2001:
		PRECACHE_MODEL( "models/med_crategibs.mdl" );
		strcpy( m_szModel, "models/med_crategibs.mdl" );
		pev->body = 6;
		break;
	case 2100:
		PRECACHE_MODEL( "models/med_crategibs.mdl" );
		strcpy( m_szModel, "models/med_crategibs.mdl" );
		switch ( RANDOM_LONG( 0, 2 ) )
		{
		case 0:	pev->body = 7;	break;
		case 1:	pev->body = 14;	break;
		default:	pev->body = 15;	break;
		}
		break;
	case 2101:
		PRECACHE_MODEL( "models/med_crategibs.mdl" );
		strcpy( m_szModel, "models/med_crategibs.mdl" );
		pev->body = 7;
		break;
	case 2102:
		PRECACHE_MODEL( "models/med_crategibs.mdl" );
		strcpy( m_szModel, "models/med_crategibs.mdl" );
		pev->body = 14;
		break;
	case 2103:
		PRECACHE_MODEL( "models/med_crategibs.mdl" );
		strcpy( m_szModel, "models/med_crategibs.mdl" );
		pev->body = 15;
		break;
	case 2200:
		PRECACHE_MODEL( "models/med_crategibs.mdl" );
		strcpy( m_szModel, "models/med_crategibs.mdl" );
		pev->body = RANDOM_LONG( 8, 10 );
		break;
	case 2201:
		PRECACHE_MODEL( "models/med_crategibs.mdl" );
		strcpy( m_szModel, "models/med_crategibs.mdl" );
		pev->body = 8;
		break;
	case 2202:
		PRECACHE_MODEL( "models/med_crategibs.mdl" );
		strcpy( m_szModel, "models/med_crategibs.mdl" );
		pev->body = 9;
		break;
	case 2203:
		PRECACHE_MODEL( "models/med_crategibs.mdl" );
		strcpy( m_szModel, "models/med_crategibs.mdl" );
		pev->body = 10;
		break;
	case 2300:
		PRECACHE_MODEL( "models/med_crategibs.mdl" );
		strcpy( m_szModel, "models/med_crategibs.mdl" );
		pev->body = RANDOM_LONG( 11, 11 );
		break;
	case 2301:
		PRECACHE_MODEL( "models/med_crategibs.mdl" );
		strcpy( m_szModel, "models/med_crategibs.mdl" );
		pev->body = 11;
		break;
	case 2400:
		PRECACHE_MODEL( "models/med_crategibs.mdl" );
		strcpy( m_szModel, "models/med_crategibs.mdl" );
		pev->body = RANDOM_LONG( 16, 16 );
		break;
	case 2401:
		PRECACHE_MODEL( "models/med_crategibs.mdl" );
		strcpy( m_szModel, "models/med_crategibs.mdl" );
		pev->body = 16;
		break;
	case 2500:
		PRECACHE_MODEL( "models/tech_crategibs.mdl" );
		strcpy( m_szModel, "models/tech_crategibs.mdl" );
		pev->body = RANDOM_LONG( 6, 6 );
		break;
	case 2501:
		PRECACHE_MODEL( "models/tech_crategibs.mdl" );
		strcpy( m_szModel, "models/tech_crategibs.mdl" );
		pev->body = 6;
		break;
	case 2600:
		PRECACHE_MODEL( "models/tech_crategibs.mdl" );
		strcpy( m_szModel, "models/tech_crategibs.mdl" );
		if ( RANDOM_LONG( 0, 1 ) == 1 )
			pev->body = 13;
		else
			pev->body = 7;
		break;
	case 2601:
		PRECACHE_MODEL( "models/tech_crategibs.mdl" );
		strcpy( m_szModel, "models/tech_crategibs.mdl" );
		pev->body = 7;
		break;
	case 2602:
		PRECACHE_MODEL( "models/tech_crategibs.mdl" );
		strcpy( m_szModel, "models/tech_crategibs.mdl" );
		pev->body = 13;
		break;
	case 2700:
		PRECACHE_MODEL( "models/tech_crategibs.mdl" );
		strcpy( m_szModel, "models/tech_crategibs.mdl" );
		pev->body = RANDOM_LONG( 8, 12 );
		break;
	case 2900:
		PRECACHE_MODEL( "models/tech_crategibs.mdl" );
		strcpy( m_szModel, "models/tech_crategibs.mdl" );
		pev->body = RANDOM_LONG( 9, 9 );
		break;
	case 2901:
		PRECACHE_MODEL( "models/tech_crategibs.mdl" );
		strcpy( m_szModel, "models/tech_crategibs.mdl" );
		pev->body = 9;
		break;
	case 3000:
		PRECACHE_MODEL( "models/tech_crategibs.mdl" );
		strcpy( m_szModel, "models/tech_crategibs.mdl" );
		pev->body = RANDOM_LONG( 10, 11 );
		break;
	case 3001:
		PRECACHE_MODEL( "models/tech_crategibs.mdl" );
		strcpy( m_szModel, "models/tech_crategibs.mdl" );
		pev->body = 10;
		break;
	case 3002:
		PRECACHE_MODEL( "models/tech_crategibs.mdl" );
		strcpy( m_szModel, "models/tech_crategibs.mdl" );
		pev->body = 11;
		break;
	case 3100:
		PRECACHE_MODEL( "models/tech_crategibs.mdl" );
		strcpy( m_szModel, "models/tech_crategibs.mdl" );
		pev->body = RANDOM_LONG( 12, 12 );
		break;
	case 3101:
		PRECACHE_MODEL( "models/tech_crategibs.mdl" );
		strcpy( m_szModel, "models/tech_crategibs.mdl" );
		pev->body = 12;
		break;
	case 3200:
		PRECACHE_MODEL( "models/tech_crategibs.mdl" );
		strcpy( m_szModel, "models/tech_crategibs.mdl" );
		pev->body = RANDOM_LONG( 14, 14 );
		break;
	case 3201:
		PRECACHE_MODEL( "models/tech_crategibs.mdl" );
		strcpy( m_szModel, "models/tech_crategibs.mdl" );
		pev->body = 14;
		break;
	}
}

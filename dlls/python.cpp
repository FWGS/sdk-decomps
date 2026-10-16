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
#include "weapons.h"
#include "monsters.h"
#include "player.h"
#include "gamerules.h"
#include "items.h"


// POV: the 357 and its ammo are props that can't be taken
class CPython : public CItem
{
public:
	void Spawn( void )
	{
		Precache( );
		SET_MODEL(ENT(pev), "models/w_357.mdl");
		CItem::Spawn( );
		UTIL_MakeProp( this );
	}
	void Precache( void )
	{
		PRECACHE_MODEL("models/w_357.mdl");

		PRECACHE_MODEL("models/w_357ammobox.mdl");
		PRECACHE_SOUND("items/9mmclip1.wav");              

		PRECACHE_SOUND ("weapons/357_reload1.wav");
		PRECACHE_SOUND ("weapons/357_cock1.wav");
		PRECACHE_SOUND ("weapons/357_shot1.wav");
		PRECACHE_SOUND ("weapons/357_shot2.wav");
	}
	BOOL MyTouch( CBasePlayer *pPlayer )
	{
		return FALSE;
	}
};
LINK_ENTITY_TO_CLASS( weapon_python, CPython );
LINK_ENTITY_TO_CLASS( weapon_357, CPython );



class CPythonAmmo : public CItem
{
	void Spawn( void )
	{ 
		Precache( );
		SET_MODEL(ENT(pev), "models/w_357ammobox.mdl");
		CItem::Spawn( );
	}
	void Precache( void )
	{
		PRECACHE_MODEL ("models/w_357ammobox.mdl");
	}
	BOOL MyTouch( CBasePlayer *pPlayer )
	{
		return FALSE;
	}
};
LINK_ENTITY_TO_CLASS( ammo_357, CPythonAmmo );


#endif

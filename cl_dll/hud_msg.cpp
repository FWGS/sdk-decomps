/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
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
//  hud_msg.cpp
//

#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"
#include "r_efx.h"

#define MAX_CLIENTS 32


/// USER-DEFINED SERVER MESSAGE HANDLERS

int CHud :: MsgFunc_ResetHUD(const char *pszName, int iSize, void *pbuf )
{
	ASSERT( iSize == 0 );

	// clear all hud data
	HUDLIST *pList = m_pHudList;

	while ( pList )
	{
		if ( pList->p )
			pList->p->Reset();
		pList = pList->pNext;
	}

	// reset sensitivity
	m_flMouseSensitivity = 0;

	// reset concussion effect
	m_iConcussionEffect = 0;

	return 1;
}

void CAM_ToFirstPerson(void);

void CHud :: MsgFunc_ViewMode( const char *pszName, int iSize, void *pbuf )
{
	CAM_ToFirstPerson();
}

void CheckPlayerModelFiles( void );

void CHud :: MsgFunc_CheckModels( const char *pszName, int iSize, void *pbuf )
{
	CheckPlayerModelFiles();
}

void CHud :: MsgFunc_BombDrop( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );

	g_PlayerExtraInfo[MAX_CLIENTS].radar_pos.x = READ_COORD();
	g_PlayerExtraInfo[MAX_CLIENTS].radar_pos.y = READ_COORD();
	g_PlayerExtraInfo[MAX_CLIENTS].radar_pos.z = READ_COORD();

	g_PlayerExtraInfo[MAX_CLIENTS].flash_next = gHUD.m_flTime;
	g_PlayerExtraInfo[MAX_CLIENTS].flash_on = 1;
	g_PlayerExtraInfo[MAX_CLIENTS].flash_left = 99999;

	strcpy( g_PlayerExtraInfo[MAX_CLIENTS].teamname, "TERRORIST" );
	g_PlayerExtraInfo[MAX_CLIENTS].dead = false;
}

void CHud :: MsgFunc_BombPickup( const char *pszName, int iSize, void *pbuf )
{
	g_PlayerExtraInfo[MAX_CLIENTS].flash_next = -1;
	g_PlayerExtraInfo[MAX_CLIENTS].flash_on = 0;
	g_PlayerExtraInfo[MAX_CLIENTS].flash_left = 0;
}

void CHud :: MsgFunc_HostagePos( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );

	int flag = READ_BYTE();
	int idx = READ_BYTE();

	if ( idx <= MAX_HOSTAGES )
	{
		g_HostageInfo[idx].origin.x = READ_COORD();
		g_HostageInfo[idx].origin.y = READ_COORD();
		g_HostageInfo[idx].origin.z = READ_COORD();

		if ( flag == 1 )
		{
			g_HostageInfo[idx].radarflash = gHUD.m_flTime;
			g_HostageInfo[idx].radarflashon = 1;
			g_HostageInfo[idx].radarflashes = 99999;
		}

		strcpy( g_HostageInfo[idx].teamname, "CT" );
		g_HostageInfo[idx].dead = false;
	}
}

void CHud :: MsgFunc_HostageK( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );

	int idx = READ_BYTE();

	if ( idx <= MAX_HOSTAGES )
	{
		g_HostageInfo[idx].radarflash = gHUD.m_flTime;
		g_HostageInfo[idx].radarflashes = 15;
		g_HostageInfo[idx].dead = true;
	}
}

void CHud :: MsgFunc_InitHUD( const char *pszName, int iSize, void *pbuf )
{
	// prepare all hud data
	HUDLIST *pList = m_pHudList;

	while (pList)
	{
		if ( pList->p )
			pList->p->InitHUDData();
		pList = pList->pNext;
	}

}


int CHud :: MsgFunc_GameMode(const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );
	m_Teamplay = READ_BYTE();

	return 1;
}


int CHud :: MsgFunc_Damage(const char *pszName, int iSize, void *pbuf )
{
	int		armor, blood;
	Vector	from;
	int		i;
	float	count;
	
	BEGIN_READ( pbuf, iSize );
	armor = READ_BYTE();
	blood = READ_BYTE();

	for (i=0 ; i<3 ; i++)
		from[i] = READ_COORD();

	count = (blood * 0.5) + (armor * 0.5);

	if (count < 10)
		count = 10;

	// TODO: kick viewangles,  show damage visually

	return 1;
}

int CHud :: MsgFunc_Concuss( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );
	m_iConcussionEffect = READ_BYTE();
	if (m_iConcussionEffect)
		this->m_StatusIcons.EnableIcon("dmg_concuss",255,160,0,false);
	else
		this->m_StatusIcons.DisableIcon("dmg_concuss");
	return 1;
}

int CHud :: MsgFunc_ReloadSound( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );

	short vol = READ_BYTE();	// name ours
	short type = READ_BYTE();	// name ours

	switch ( type )
	{
	case 0:
		PlaySound( "weapons/generic_shot_reload.wav", vol / 255.0f );
		break;

	case 1:
		PlaySound( "weapons/generic_reload.wav", vol / 255.0f );
		break;

	default:
		PlaySound( "weapons/generic_reload.wav", vol / 255.0f );
		break;
	}

	return 1;
}


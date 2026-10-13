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
// battery.cpp
//
// implementation of CHudBattery class
//

#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"

#include <string.h>
#include <stdio.h>

int g_HUD_number_10;	// provisional: ammo.cpp defines and sets it (CHudAmmo::VidInit)

static int m_HUD_armor_graph;

DECLARE_MESSAGE(m_Battery, Battery)

int CHudBattery::Init(void)
{
	m_iBat = 0;
	m_fFade = 0;
	m_iFlags = 0;

	HOOK_MESSAGE(Battery);

	gHUD.AddHudElem(this);

	return 1;
};


int CHudBattery::VidInit(void)
{
	int HUD_suit_empty = gHUD.GetSpriteIndex( "suit_empty" );
	int HUD_suit_full = gHUD.GetSpriteIndex( "suit_full" );
	m_HUD_armor_graph = gHUD.GetSpriteIndex( "armor_graph" );

	m_hSprite1 = m_hSprite2 = 0;  // delaying get sprite handles until we know the sprites are loaded
	m_prc1 = &gHUD.GetSpriteRect( HUD_suit_empty );
	m_prc2 = &gHUD.GetSpriteRect( HUD_suit_full );
	m_iHeight = m_prc2->bottom - m_prc1->top;
	m_fFade = 0;
	return 1;
};

int CHudBattery:: MsgFunc_Battery(const char *pszName,  int iSize, void *pbuf )
{
	m_iFlags |= HUD_ACTIVE;

	
	BEGIN_READ( pbuf, iSize );
	int x = READ_SHORT();

	if (x != m_iBat)
	{
		m_fFade = FADE_TIME;
		m_iBat = x;
	}

	return 1;
}


int CHudBattery::Draw(float flTime)
{
	// Gunman: the armor number sits low on the left, with a bar above it
	if ( !( gHUD.m_iHideHUDDisplay & HIDEHUD_HEALTH ) )
	{
		int x = 164;
		int y = ScreenHeight - 63;

		if ( CVAR_GET_FLOAT( "hud_smallhud" ) != 0 || ScreenWidth <= 640 )
			x = 35;

		if ( m_iBat > 99 )
		{
			// a hundred has its own sprite
			SPR_Set( gHUD.GetSprite( g_HUD_number_10 ), 0, 255, 0 );
			SPR_DrawAdditive( 0, x - 3, y, &gHUD.GetSpriteRect( g_HUD_number_10 ) );
		}
		else
		{
			gHUD.DrawHudNumber( x, y, DHN_DRAWZERO | DHN_2DIGITS, m_iBat, 0, 255, 0 );
		}

		if ( CVAR_GET_FLOAT( "hud_smallhud" ) == 0 && ScreenWidth > 640 )
		{
			wrect_t rc = gHUD.GetSpriteRect( m_HUD_armor_graph );

			if ( m_iBat > 96 )
			{
				rc.right = 96;
			}
			else
			{
				rc.right = 1;
				if ( m_iBat )
					rc.right = m_iBat;
			}

			SPR_Set( gHUD.GetSprite( m_HUD_armor_graph ), 255, 255, 255 );
			SPR_DrawAdditive( 0, 27, ScreenHeight - 112, &rc );
		}
	}

	return 1;
}
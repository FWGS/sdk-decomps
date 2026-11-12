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
#include "util.h"
#include "parsemsg.h"

#include <string.h>
#include <stdio.h>

DECLARE_MESSAGE(m_Battery, Battery)
DECLARE_MESSAGE(m_Battery, ArmorType)

int CHudBattery::Init(void)
{
	m_iBat = 0;
	m_fFade = 0;
	m_iFlags = 0;
	m_enArmorType = 0;

	HOOK_MESSAGE(Battery);
	HOOK_MESSAGE(ArmorType);

	gHUD.AddHudElem(this);

	return 1;
};


int CHudBattery::VidInit(void)
{
	m_HUD_suit_empty = gHUD.GetSpriteIndex( "suit_empty" );
	m_HUD_suit_full = gHUD.GetSpriteIndex( "suit_full" );
	m_HUD_suithelmet_empty = gHUD.GetSpriteIndex( "suithelmet_empty" );
	m_HUD_suithelmet_full = gHUD.GetSpriteIndex( "suithelmet_full" );

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

		if ( x <= 0 )
			m_enArmorType = 0;
	}

	return 1;
}

int CHudBattery:: MsgFunc_ArmorType(const char *pszName,  int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );
	m_enArmorType = READ_BYTE();

	return 1;
}


int CHudBattery::Draw(float flTime)
{
	if ( gHUD.m_iHideHUDDisplay & HIDEHUD_HEALTH )
		return 1;

	int r, g, b, x, y, a;
	wrect_t rc;
	int iEmpty, iFull;	// names ours

	if ( m_enArmorType == 1 )
	{
		iFull = m_HUD_suithelmet_full;
		iEmpty = m_HUD_suithelmet_empty;
	}
	else
	{
		iFull = m_HUD_suit_full;
		iEmpty = m_HUD_suit_empty;
	}

	rc = gHUD.GetSpriteRect( iFull );
	rc.top  += gHUD.GetSpriteHeight( iFull ) * ((float)(100-(min(100,m_iBat))) * 0.01);	// battery can go from 0 to 100 so * 0.01 goes from 0 to 1

	UnpackRGB(r,g,b, RGB_YELLOWISH);

	if (!(gHUD.m_iWeaponBits & (1<<(WEAPON_SUIT)) ))
		return 1;

	// Has health changed? Flash the health #
	if (m_fFade)
	{
		if (m_fFade > FADE_TIME)
			m_fFade = FADE_TIME;

		m_fFade -= (gHUD.m_flTimeDelta * 20);
		if (m_fFade <= 0)
		{
			a = 128;
			m_fFade = 0;
		}

		// Fade the health number back to dim

		a = MIN_ALPHA +  (m_fFade/FADE_TIME) * 128;

	}
	else
		a = MIN_ALPHA;

	ScaleColors(r, g, b, a );
	
	int iOffset = gHUD.GetSpriteHeight( iEmpty ) / 6;

	y = ScreenHeight - gHUD.m_iFontHeight - gHUD.m_iFontHeight / 2 + ( ScreenWidth < 640 ? 1 : 4 );
	x = ScreenWidth/5;

	SPR_Set( gHUD.GetSprite( iEmpty ), r, g, b );
	SPR_DrawAdditive( 0,  x, y - iOffset, &gHUD.GetSpriteRect( iEmpty ) );

	if (rc.bottom > rc.top)
	{
		SPR_Set( gHUD.GetSprite( iFull ), r, g, b );
		SPR_DrawAdditive( 0, x, y - iOffset + (rc.top - gHUD.GetSpriteRect( iFull ).top), &rc );
	}

	if ( ScreenWidth < 640 )
		y -= 1;
	else
		y -= 4;

	x += gHUD.GetSpriteWidth( iEmpty );
	x = gHUD.DrawHudNumber(x, y, DHN_3DIGITS | DHN_DRAWZERO, m_iBat, r, g, b);

	return 1;
}

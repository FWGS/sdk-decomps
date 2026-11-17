/***
*
*	Copyright (c) 1996-2001, Valve LLC. All rights reserved.
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
	int HUD_suit_shield = gHUD.GetSpriteIndex( "suit_shield" );
	int HUD_suit_meter = gHUD.GetSpriteIndex( "suit_meter" );

	m_hSprite1 = m_hSprite2 = 0;  // delaying get sprite handles until we know the sprites are loaded
	m_prc1 = &gHUD.GetSpriteRect( HUD_suit_empty );
	m_prc2 = &gHUD.GetSpriteRect( HUD_suit_full );
	m_prc3 = &gHUD.GetSpriteRect( HUD_suit_shield );
	m_prc4 = &gHUD.GetSpriteRect( HUD_suit_meter );
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
	if ( gHUD.m_iHideHUDDisplay & HIDEHUD_HEALTH )
		return 1;

	int r, g, b, x, y, a;
	wrect_t rc, rc2;

	// m_iBat 0..100 is the suit's armor, 101..300 the energy shield on top of it (suit_shield, suit_meter)
	float flArmor = min( 100, m_iBat ) * 0.01;
	float flShield = 0;
	if ( m_iBat > 100 )
		flShield = ( m_iBat - 100 ) * 0.005;

	y = ScreenHeight - gHUD.m_iFontHeight - m_iHeight * 0.6;
	x = ScreenWidth/5;

	int iOffset = (m_prc1->bottom - m_prc1->top)/6;

	// make sure we have the right sprite handles
	if ( !m_hSprite1 )
		m_hSprite1 = gHUD.GetSprite( gHUD.GetSpriteIndex( "suit_empty" ) );
	if ( !m_hSprite2 )
		m_hSprite2 = gHUD.GetSprite( gHUD.GetSpriteIndex( "suit_full" ) );
	if ( !m_hSprite3 )
		m_hSprite3 = gHUD.GetSprite( gHUD.GetSpriteIndex( "suit_shield" ) );
	if ( !m_hSprite4 )
		m_hSprite4 = gHUD.GetSprite( gHUD.GetSpriteIndex( "suit_meter" ) );

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

	UnpackRGB(r,g,b, RGB_WHITE);

	if ( m_iBat <= 0 )
	{
		ScaleColors(r, g, b, a );
		SPR_Set(m_hSprite1, r, g, b );
		SPR_DrawAdditive( 0,  x, y - iOffset, m_prc1);
	}
	else
	{
		if ( m_iBat <= 100 )
		{
			ScaleColors(r, g, b, a );

			if ( m_iBat == 100 )
			{
				SPR_Set(m_hSprite2, r, g, b );
				SPR_DrawAdditive( 0, x, y - iOffset, m_prc2);
			}
			else
			{
				// the empty suit above the armor's level, the full one below it
				rc = *m_prc1;
				rc2 = *m_prc2;
				float flHeight = m_iHeight * flArmor;
				rc.bottom -= flHeight;
				rc2.top = rc2.bottom - flHeight;

				SPR_Set(m_hSprite1, r, g, b );
				SPR_DrawAdditive( 0,  x, y - iOffset, &rc);
				SPR_Set(m_hSprite2, r, g, b );
				SPR_DrawAdditive( 0, x, y - iOffset + (rc2.top - m_prc2->top), &rc2);
			}
		}
		else if ( m_iBat == 300 )
		{
			ScaleColors(r, g, b, a );
			SPR_Set(m_hSprite3, r, g, b );
			SPR_DrawAdditive( 0, x, y - iOffset, m_prc3);
		}
		else
		{
			// the full suit fades into the shield
			float flAlpha = a;
			ScaleColors(r, g, b, (1 - flShield) * flAlpha );
			SPR_Set(m_hSprite2, r, g, b );
			SPR_DrawAdditive( 0, x, y - iOffset, m_prc2);

			UnpackRGB(r,g,b, RGB_WHITE);
			ScaleColors(r, g, b, flAlpha * flShield );
			SPR_Set(m_hSprite3, r, g, b );
			SPR_DrawAdditive( 0, x, y - iOffset, m_prc3);
		}

		// the shield meter, left of the suit
		UnpackRGB(r,g,b, RGB_WHITE);
		ScaleColors(r, g, b, a );

		if ( m_iBat == 300 )
		{
			SPR_Set(m_hSprite4, r, g, b );
			SPR_DrawAdditive( 0, x - m_prc4->right, y - iOffset, m_prc4);
		}
		else
		{
			int iShield = min( 300, m_iBat );
			rc2 = *m_prc4;
			rc2.top = rc2.bottom - ( rc2.bottom - rc2.top ) * ( iShield / 300.0f );
			SPR_Set(m_hSprite4, r, g, b );
			SPR_DrawAdditive( 0, x - m_prc4->right, y - iOffset + (rc2.top - m_prc4->top), &rc2);
		}
	}

	x += (m_prc1->right - m_prc1->left);
	y = ScreenHeight - gHUD.m_iFontHeight - gHUD.m_iFontHeight / 2;
	x = gHUD.DrawHudNumber(x, y, DHN_3DIGITS | DHN_DRAWZERO, m_iBat, r, g, b);

	return 1;
}
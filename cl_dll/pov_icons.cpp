//
// pov_icons.cpp
//
// Point of View's HUD icons: the break icon (shown while the player
// stands in an area_break) and the journal's story icon (an entry was
// found and not read yet). File and class names ours.
//

#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"

#include <string.h>
#include <stdio.h>



DECLARE_MESSAGE(m_BreakIcon, BreakIcon)

int CHudBreakIcon::Init(void)
{
	HOOK_MESSAGE(BreakIcon);

	m_iFlags &= ~HUD_ACTIVE;

	gHUD.AddHudElem(this);

	return 1;
};

int CHudBreakIcon::VidInit(void)
{
	m_hSprite = LoadSprite("sprites/breakicon.spr");
	m_iWidth = SPR_Width(m_hSprite, 0);
	m_iHeight = SPR_Height(m_hSprite, 0);

	return 1;
};

int CHudBreakIcon:: MsgFunc_BreakIcon(const char *pszName,  int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );
	m_iState = READ_BYTE();

	if ( !m_iState )
		m_iFlags &= ~HUD_ACTIVE;
	else
		m_iFlags |= HUD_ACTIVE;

	return 1;
}

int CHudBreakIcon::Draw(float flTime)
{
	if ( gHUD.m_iHideHUDDisplay & HIDEHUD_ALL )
		return 1;

	if (!(gHUD.m_iWeaponBits & (1<<(WEAPON_SUIT)) ))
		return 1;

	int r, g, b, x, y;

	r = 0;
	g = 255;
	b = 0;
	ScaleColors(r, g, b, 225);

	y = ScreenHeight - m_iHeight/2 - gHUD.m_iFontHeight;
	x = ScreenWidth/2 - m_iWidth/2;

	SPR_Set(m_hSprite, r, g, b );
	SPR_DrawAdditive( 0, x, y, NULL );

	return 1;
}



DECLARE_MESSAGE(m_StoryIcon, StoryIcon)

int CHudStoryIcon::Init(void)
{
	HOOK_MESSAGE(StoryIcon);

	m_iFlags &= ~HUD_ACTIVE;

	gHUD.AddHudElem(this);

	return 1;
};

int CHudStoryIcon::VidInit(void)
{
	m_hSprite = LoadSprite("sprites/storyicon.spr");
	m_iWidth = SPR_Width(m_hSprite, 0);
	m_iHeight = SPR_Height(m_hSprite, 0);

	m_iMaxAlpha = 220;
	m_iAlpha = 220;
	m_iMinAlpha = 100;
	m_iFadeOut = 1;
	m_flNextFade = 0;

	return 1;
};

int CHudStoryIcon:: MsgFunc_StoryIcon(const char *pszName,  int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );
	m_iState = READ_BYTE();

	if ( !m_iState )
		m_iFlags &= ~HUD_ACTIVE;
	else
		m_iFlags |= HUD_ACTIVE;

	return 1;
}

int CHudStoryIcon::Draw(float flTime)
{
	if ( gHUD.m_iHideHUDDisplay & HIDEHUD_ALL )
		return 1;

	if (!(gHUD.m_iWeaponBits & (1<<(WEAPON_SUIT)) ))
		return 1;

	// pulse between m_iMinAlpha and m_iMaxAlpha
	if ( flTime > m_flNextFade )
	{
		if ( m_iFadeOut == 1 )
		{
			m_iAlpha -= 10;
			if ( m_iAlpha < m_iMinAlpha )
			{
				m_iAlpha = m_iMinAlpha;
				m_iFadeOut = 0;
			}
		}
		else
		{
			m_iAlpha += 10;
			if ( m_iAlpha > m_iMaxAlpha )
			{
				m_iAlpha = m_iMaxAlpha;
				m_iFadeOut = 1;
			}
		}

		m_flNextFade = flTime + 0.1;
	}

	int r, g, b, x, y;

	r = 0;
	g = 255;
	b = 0;
	ScaleColors(r, g, b, m_iAlpha);

	x = ScreenWidth - m_iWidth/2 - m_iWidth;
	y = m_iHeight * 2;

	SPR_Set(m_hSprite, r, g, b );
	SPR_DrawAdditive( 0, x, y, NULL );

	return 1;
}

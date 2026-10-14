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
// zoom.cpp
//
// implementation of CHudZoom class: the gauss pistol's sniper scope.
// The scope slides the FOV between steps and spins its target sprite.
//

#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"

#include <string.h>
#include <stdio.h>

extern cvar_t *sensitivity;
extern float g_lastFOV;

static int m_HUD_zoom_target;

DECLARE_MESSAGE(m_Zoom, Zoom)

int CHudZoom::Init(void)
{
	HOOK_MESSAGE(Zoom);

	gHUD.AddHudElem(this);

	m_fChanging = true;
	m_iFrame = 0;
	m_flNextFrameTime = 0;
	m_iFOV = 0;
	m_flCrosshairTime = 0;
	m_flOffTime = 0;

	return 1;
}

int CHudZoom::VidInit(void)
{
	m_HUD_zoom_target = gHUD.GetSpriteIndex( "zoom_target" );

	return 1;
}

int CHudZoom::MsgFunc_Zoom(const char *pszName,  int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );

	// 0 drops the scope at once, 1 raises it, 2 stops the FOV where it
	// is, anything else lowers the scope after a moment
	int iMode = READ_SHORT();

	if ( iMode == 0 )
	{
		Reset();
		return 1;
	}

	if ( iMode == 1 )
	{
		m_iFOV = 0;
		m_flNextFOVTime = 0;
		m_flNextFrameTime = 0;
		m_iFrame = 0;
		float flNow = gEngfuncs.GetClientTime();
		m_iTargetFOV = 15;
		m_fZoomed = true;
		m_iFlags |= HUD_ACTIVE;
		m_fZoomingOut = false;
		m_flNextFOVTime = flNow + 0.4;
		m_flNextFrameTime = flNow + 0.4;
		m_flOffTime = 0;
		m_flCrosshairTime = 0;
		m_flZoomTime = 0;
		return 1;
	}

	if ( iMode == 2 )
	{
		m_fChanging = false;
		return 1;
	}

	if ( m_fChanging )
		m_flOffTime = gEngfuncs.GetClientTime();
	else
		m_flOffTime = gEngfuncs.GetClientTime() + 0.5;

	return 1;
}

void CHudZoom::Reset(void)
{
	m_iFOV = 0;
	m_flNextFOVTime = 0;
	m_iFlags &= ~HUD_ACTIVE;
	m_iFrame = 0;
	m_flNextFrameTime = 0;
	m_flCrosshairTime = 0;

	int def_fov = CVAR_GET_FLOAT( "default_fov" );
	gHUD.m_iFOV = def_fov;
	g_lastFOV = def_fov;
	gHUD.m_flMouseSensitivity = 0;

	m_flOffTime = 0;
	m_iTargetFOV = 15;
	m_fChanging = true;
	m_fZoomed = false;
	m_fZoomingOut = false;

	if ( gHUD.m_Ammo.m_pWeapon )
		SetCrosshair( gHUD.m_Ammo.m_pWeapon->hCrosshair, gHUD.m_Ammo.m_pWeapon->rcCrosshair, 255, 255, 255 );
}

// one step in, by the next-weapon key
void CHudZoom::ZoomIn(void)
{
	if ( m_iFOV == 0 )
		m_iFOV = 90;

	m_iTargetFOV = m_iFOV - 7;
	if ( m_iTargetFOV < 9 )
		m_iTargetFOV = 9;

	m_fChanging = true;
	m_fZoomingOut = false;
	m_flZoomTime = gEngfuncs.GetClientTime();
	m_flNextFrameTime = m_flNextFOVTime = gEngfuncs.GetClientTime();
}

// one step out, by the previous-weapon key
void CHudZoom::ZoomOut(void)
{
	if ( m_iFOV <= 65 )
	{
		if ( m_iFOV == 0 )
			m_iFOV = 90;

		m_iTargetFOV = m_iFOV + 7;
		if ( m_iTargetFOV > 65 )
			m_iTargetFOV = 65;

		m_fChanging = true;
		m_flZoomTime = gEngfuncs.GetClientTime();
		m_fZoomingOut = true;
		m_flNextFrameTime = m_flNextFOVTime = gEngfuncs.GetClientTime();
	}
}

int CHudZoom::Draw(float flTime)
{
	float flNow = gEngfuncs.GetClientTime();

	if ( !gHUD.m_Ammo.m_pWeapon )
		return 0;

	if ( m_flOffTime != 0 && m_flOffTime < flNow )
	{
		Reset();
		return 1;
	}

	m_fZoomed = true;

	if ( m_iFOV == 0 || m_iFOV > 65 )
		m_iFOV = 65;

	// slide the FOV towards its target
	if ( m_flNextFOVTime < flNow && m_fChanging )
	{
		if ( m_iFOV > m_iTargetFOV && !m_fZoomingOut )
		{
			m_iFOV = m_iFOV - ( flNow - m_flNextFOVTime ) * 200;
		}
		else if ( m_iFOV < m_iTargetFOV && m_fZoomingOut == true )
		{
			m_iFOV += 2;
		}
		else
		{
			m_fChanging = false;
			if ( m_flZoomTime != 0 )
				m_flZoomTime = 0;
		}

		m_flNextFOVTime = flNow + 0.01;
	}

	// and spin the target while it slides
	if ( m_flNextFrameTime < flNow && m_fChanging )
	{
		if ( !m_fZoomingOut )
		{
			if ( ++m_iFrame > 2 )
				m_iFrame = 0;
		}
		else
		{
			if ( --m_iFrame < 0 )
				m_iFrame = 2;
		}

		m_flNextFrameTime = flNow + 0.1;
	}

	if ( m_flCrosshairTime < flNow )
	{
		SetCrosshair( gHUD.m_Ammo.m_pWeapon->hZoomedCrosshair, gHUD.m_Ammo.m_pWeapon->rcZoomedCrosshair, 255, 255, 255 );
		SPR_Set( gHUD.GetSprite( m_HUD_zoom_target ), 0, 255, 0 );
		SPR_DrawAdditive( m_iFrame, ScreenWidth / 2 - 64, ScreenHeight / 2 - 64, &gHUD.GetSpriteRect( m_HUD_zoom_target ) );
	}

	int def_fov = CVAR_GET_FLOAT( "default_fov" );

	if ( m_iFOV == 0 )
	{
		gHUD.m_iFOV = def_fov;
		g_lastFOV = def_fov;
	}
	else
	{
		g_lastFOV = m_iFOV;
		gHUD.m_iFOV = m_iFOV;
	}

	if ( m_iFOV == 0 )
	{
		m_iFlags &= ~HUD_ACTIVE;
	}
	else
	{
		// mouse sensitivity in proportion to the zoom, but never under 1
		gHUD.m_flMouseSensitivity = CVAR_GET_FLOAT( "zoom_sensitivity_ratio" ) * ( (float)m_iFOV / def_fov * sensitivity->value );
		if ( gHUD.m_flMouseSensitivity >= 1 )
			return 1;
	}

	// sic: unzoomed, the sensitivity is 1 rather than 0 (the player's own)
	gHUD.m_flMouseSensitivity = 1;

	return 1;
}

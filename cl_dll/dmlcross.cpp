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
// dmlcross.cpp
//
// implementation of CHudDmlCross class: the lock sprite the DML
// shows over its crosshair while it guides a rocket
//

#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"

#include <string.h>
#include <stdio.h>
#include <math.h>

static int m_HUD_dmllock;

DECLARE_MESSAGE(m_DmlCross, DmlCross)

int CHudDmlCross::Init(void)
{
	HOOK_MESSAGE(DmlCross);

	gHUD.AddHudElem(this);

	m_flFrame = 6;
	m_flLastTime = 0;
	m_iMode = 0;

	return 1;
}

int CHudDmlCross::VidInit(void)
{
	m_HUD_dmllock = gHUD.GetSpriteIndex( "dmllock" );

	return 1;
}

int CHudDmlCross::MsgFunc_DmlCross(const char *pszName,  int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );

	// 0 turns the lock off, 1 shows the crosshair lock, 2 the
	// animated lock, 3 the plain one
	switch ( READ_SHORT() )
	{
	case 0:
		if ( gHUD.m_Ammo.m_pWeapon )
			SetCrosshair( gHUD.m_Ammo.m_pWeapon->hCrosshair, gHUD.m_Ammo.m_pWeapon->rcCrosshair, 255, 255, 255 );
		Reset();
		return 1;

	case 1:
		m_iMode = 0;
		m_iFlags |= HUD_ACTIVE;
		m_flRed = 255;
		m_flGreen = 255;
		m_flBlue = 255;
		return 1;

	case 2:
		m_iMode = 2;
		m_iFlags |= HUD_ACTIVE;
		m_flRed = 255;
		m_flGreen = 255;
		m_flBlue = 255;
		return 1;

	case 3:
		m_iMode = 1;
		m_iFlags |= HUD_ACTIVE;
		m_flRed = 255;
		m_flGreen = 255;
		m_flBlue = 255;
		return 1;
	}

	return 1;
}

void CHudDmlCross::Reset(void)
{
	m_flFrame = 6;
	m_iFlags &= ~HUD_ACTIVE;
	m_flRed = 255;
	m_flGreen = 255;
	m_flBlue = 255;
	m_iMode = 0;
	m_flLastTime = 0;
}

int CHudDmlCross::Draw(float flTime)
{
	float flMin, flMax, flNow;

	if ( !gHUD.m_Ammo.m_pWeapon )
		return 0;

	switch ( m_iMode )
	{
	case 0:
		SetCrosshair( gHUD.m_Ammo.m_pWeapon->hZoomedCrosshair, gHUD.m_Ammo.m_pWeapon->rcZoomedCrosshair, 255, 255, 255 );
		break;
	case 1:
		SetCrosshair( gHUD.m_Ammo.m_pWeapon->hCrosshair, gHUD.m_Ammo.m_pWeapon->rcCrosshair, 255, 255, 255 );
		break;
	case 2:
		SetCrosshair( gHUD.m_Ammo.m_pWeapon->hAutoaim, gHUD.m_Ammo.m_pWeapon->rcAutoaim, 255, 255, 255 );
		break;
	}

	// mode 2 cycles frames 0 to 5, the others hold frame 6
	flMin = 0;
	flNow = gEngfuncs.GetClientTime();
	if ( m_iMode != 2 )
	{
		flMax = 11;
		flMin = 6;
	}
	else
	{
		flMax = 5;
	}

	m_flFrame = ( flNow - m_flLastTime ) * 10 + m_flFrame;
	if ( m_flFrame > flMax )
		m_flFrame = fmod( m_flFrame - flMin, 6 ) + flMin;

	if ( m_iMode != 2 )
		m_flFrame = 6;

	SPR_Set( gHUD.GetSprite( m_HUD_dmllock ), m_flRed, m_flGreen, m_flBlue );
	SPR_DrawAdditive( m_flFrame, ScreenWidth / 2 - 16, ScreenHeight / 2 - 16, &gHUD.GetSpriteRect( m_HUD_dmllock ) );

	m_flLastTime = flNow;

	return 1;
}

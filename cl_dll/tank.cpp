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
// tank.cpp
//
// implementation of CHudTank class: the tank's body and turret in a
// corner of the screen, the hit quarters flashing, and the horn
//

#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"
#include "vgui_TeamFortressViewport.h"
#include "vgui_CustomizationMenu.h"
#include "vgui_HudWeaponStatus.h"

#include <string.h>
#include <stdio.h>

static int m_HUD_tank_bodytr;
static int m_HUD_tank_bodytl;
static int m_HUD_tank_bodybl;
static int m_HUD_tank_bodybr;
static int m_HUD_tank_bodyhittr;
static int m_HUD_tank_bodyhittl;
static int m_HUD_tank_bodyhitbl;
static int m_HUD_tank_bodyhitbr;
static int m_HUD_tank_turret;

DECLARE_MESSAGE(m_Tank, TankBody)
DECLARE_MESSAGE(m_Tank, TurretPos)
DECLARE_COMMAND(m_Tank, Honk)

int CHudTank::Init(void)
{
	HOOK_MESSAGE(TankBody);
	HOOK_MESSAGE(TurretPos);
	HOOK_COMMAND("honk", Honk);

	gHUD.AddHudElem(this);

	m_iBodyHit = 0;
	m_iTurretPos = 0;
	m_fInTank = false;
	m_flHitTime = 0;

	CVAR_CREATE( "hud_tankhud", "1", FCVAR_ARCHIVE );

	return 1;
}

int CHudTank::VidInit(void)
{
	m_HUD_tank_bodytr = gHUD.GetSpriteIndex( "tank_bodytr" );
	m_HUD_tank_bodytl = gHUD.GetSpriteIndex( "tank_bodytl" );
	m_HUD_tank_bodybl = gHUD.GetSpriteIndex( "tank_bodybl" );
	m_HUD_tank_bodybr = gHUD.GetSpriteIndex( "tank_bodybr" );
	m_HUD_tank_bodyhittr = gHUD.GetSpriteIndex( "tank_bodyhittr" );
	m_HUD_tank_bodyhittl = gHUD.GetSpriteIndex( "tank_bodyhittl" );
	m_HUD_tank_bodyhitbl = gHUD.GetSpriteIndex( "tank_bodyhitbl" );
	m_HUD_tank_bodyhitbr = gHUD.GetSpriteIndex( "tank_bodyhitbr" );
	m_HUD_tank_turret = gHUD.GetSpriteIndex( "tank_turret" );

	return 1;
}

void CHudTank::UserCmd_Honk(void)
{
	if ( m_fInTank )
		PlaySound( "ambience/horn1.wav", 1 );
}

// 1 the player got in, 2 shows the empty gas can, 3 the full one, 0 got out
void CHudTank::SetTank( int iState )
{
	if ( iState == 1 )
	{
		m_fInTank = true;
		m_iFlags |= HUD_ACTIVE;

		if ( gViewPort )
		{
			if ( gViewPort->m_pWeaponStatus )
				gViewPort->m_pWeaponStatus->HideGasCans();
			if ( gViewPort && gViewPort->m_pCustomizationMenu )
				gViewPort->m_pCustomizationMenu->HideAll();
		}
	}
	else if ( iState == 2 )
	{
		m_fInTank = true;
		m_iFlags |= HUD_ACTIVE;

		if ( gViewPort && gViewPort->m_pWeaponStatus )
			gViewPort->m_pWeaponStatus->ShowEmptyGasCan();
	}
	else if ( iState == 3 )
	{
		if ( gViewPort && gViewPort->m_pWeaponStatus )
			gViewPort->m_pWeaponStatus->ShowGasCan();
	}
	else if ( iState == 0 )
	{
		m_fInTank = false;
		m_iFlags &= ~HUD_ACTIVE;

		if ( gViewPort && gViewPort->m_pWeaponStatus )
			gViewPort->m_pWeaponStatus->HideGasCans();
	}
}

void CHudTank::Reset(void)
{
	m_iFlags &= ~HUD_ACTIVE;
	m_fInTank = false;
	m_flHitTime = 0;

	if ( gViewPort && gViewPort->m_pWeaponStatus )
		gViewPort->m_pWeaponStatus->HideGasCans();
}

int CHudTank::MsgFunc_TankBody(const char *pszName,  int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );
	m_iBodyHit = READ_SHORT();
	m_flHitTime = gEngfuncs.GetClientTime() + 1.5;

	return 1;
}

int CHudTank::MsgFunc_TurretPos(const char *pszName,  int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );
	m_iTurretPos = READ_SHORT();

	return 1;
}

int CHudTank::Draw(float flTime)
{
	int x, y, iScale;

	// hud_tankhud 0 hides it, 1 puts it in the bottom left corner
	if ( CVAR_GET_FLOAT( "hud_tankhud" ) == 0 )
		return 0;

	x = 100;
	y = 100;

	if ( m_flHitTime < gEngfuncs.GetClientTime() )
		m_iBodyHit = 0;

	iScale = ( ScreenWidth >= 640 ) + 1;

	if ( CVAR_GET_FLOAT( "hud_tankhud" ) == 1 )
	{
		x = iScale * 32;
		y = ScreenHeight - iScale * 64;
	}

	if ( ScreenWidth < 640 )
	{
		SPR_Set( gHUD.GetSprite( m_HUD_tank_turret ), 255, 255, 255 );
		SPR_DrawAdditive( m_iTurretPos, x + 11, y + 16, &gHUD.GetSpriteRect( m_HUD_tank_turret ) );
	}
	else
	{
		SPR_Set( gHUD.GetSprite( m_HUD_tank_turret ), 255, 255, 255 );
		SPR_DrawAdditive( m_iTurretPos, x + 22, y + 33, &gHUD.GetSpriteRect( m_HUD_tank_turret ) );
	}

	// the front quarters, flashing when the front was hit
	if ( m_iBodyHit == 1 )
	{
		SPR_Set( gHUD.GetSprite( m_HUD_tank_bodyhittl ), 255, 255, 255 );
		SPR_DrawAdditive( 0, x, y, &gHUD.GetSpriteRect( m_HUD_tank_bodyhittl ) );
		SPR_Set( gHUD.GetSprite( m_HUD_tank_bodyhittr ), 255, 255, 255 );
		SPR_DrawAdditive( 0, x + iScale * 32, y, &gHUD.GetSpriteRect( m_HUD_tank_bodyhittr ) );
	}
	else
	{
		SPR_Set( gHUD.GetSprite( m_HUD_tank_bodytl ), 255, 255, 255 );
		SPR_DrawAdditive( 0, x, y, &gHUD.GetSpriteRect( m_HUD_tank_bodytl ) );
		SPR_Set( gHUD.GetSprite( m_HUD_tank_bodytr ), 255, 255, 255 );
		SPR_DrawAdditive( 0, x + iScale * 32, y, &gHUD.GetSpriteRect( m_HUD_tank_bodytr ) );
	}

	// and the rear ones
	if ( m_iBodyHit == 2 )
	{
		SPR_Set( gHUD.GetSprite( m_HUD_tank_bodyhitbl ), 255, 255, 255 );
		SPR_DrawAdditive( 0, x, y + iScale * 32, &gHUD.GetSpriteRect( m_HUD_tank_bodyhitbl ) );
		SPR_Set( gHUD.GetSprite( m_HUD_tank_bodyhitbr ), 255, 255, 255 );
		SPR_DrawAdditive( 0, x + iScale * 32, y + iScale * 32, &gHUD.GetSpriteRect( m_HUD_tank_bodyhitbr ) );
	}
	else
	{
		SPR_Set( gHUD.GetSprite( m_HUD_tank_bodybl ), 255, 255, 255 );
		SPR_DrawAdditive( 0, x, y + iScale * 32, &gHUD.GetSpriteRect( m_HUD_tank_bodybl ) );
		SPR_Set( gHUD.GetSprite( m_HUD_tank_bodybr ), 255, 255, 255 );
		SPR_DrawAdditive( 0, x + iScale * 32, y + iScale * 32, &gHUD.GetSpriteRect( m_HUD_tank_bodybr ) );
	}

	return 1;
}

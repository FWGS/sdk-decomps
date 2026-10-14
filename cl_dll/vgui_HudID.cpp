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
// vgui_HudID.cpp
//
// the name of the player in the crosshair, friend or enemy: each frame the
// panel traces from the local player's eyes, and the text stays up for
// ID_SHOW_TIME seconds after the player looks away
//

#include "VGUI_Font.h"
#include <VGUI_TextImage.h>

#include "hud.h"
#include "cl_util.h"
#include "camera.h"
#include "kbutton.h"
#include "cvardef.h"
#include "usercmd.h"
#include "const.h"
#include "camera.h"
#include "in_defs.h"
#include "parsemsg.h"

#include "vgui_int.h"
#include "vgui_TeamFortressViewport.h"
#include "vgui_ServerBrowser.h"
#include "entity_state.h"
#include "cl_entity.h"
#include "pm_defs.h"
#include "event_api.h"
#include "vgui_HudID.h"

CIDPanel::CIDPanel( int iTrans, int iRemoveMe, int x, int y, int wide, int tall ) : CMenuPanel( iTrans, iRemoveMe, x, y, wide, tall )
{
	char sz[80];
	int r, g, b, a;
	int iWide, iTall;

	r = g = b = a = 255;

	CSchemeManager *pSchemes = gViewPort->GetSchemeManager();
	SchemeHandle_t hTextScheme = pSchemes->getSchemeHandle( "Primary Button Text" );

	sprintf( sz, "text/warning.txt" );
	gEngfuncs.COM_LoadFile( sz, 5, NULL );		// sic: loaded and dropped

	m_pText = new TextPanel( "ID_test_tag_init_value", 0, 0, wide, tall );
	m_pText->setParent( this );
	m_pText->setFont( pSchemes->getFont( hTextScheme ) );
	pSchemes->getFgColor( hTextScheme, r, g, b, a );
	m_pText->setFgColor( r, g, b, a );
	pSchemes->getBgColor( hTextScheme, r, g, b, a );
	m_pText->setBgColor( r, g, b, 255 );
	m_pText->getTextImage()->getTextSizeWrapped( iWide, iTall );
	m_pText->setSize( iWide, iTall );
	m_pText->setVisible( false );
}

// Put the text in and center the panel; a size of 0 takes the text's
void CIDPanel::SetID( char *pszText, int iWide, int iTall )
{
	m_pText->setText( pszText );

	if ( !iWide || !iTall )
	{
		int iTextWide, iTextTall;

		m_pText->getTextImage()->getTextSizeWrapped( iTextWide, iTextTall );

		if ( !iTall )
			iTall = iTextTall;
		if ( !iWide )
			iWide = iTextWide;
	}

	m_pText->setSize( iWide, iTall );

	gEngfuncs.pfnDrawConsoleStringLen( pszText, &iWide, &iTall );

	int x = max( 0, max( 2, ScreenWidth - iWide ) / 2 );
	int y = ScreenHeight * 4 / 5;
	if ( CVAR_GET_FLOAT( "hud_centerid" ) == 1 )
		y = ScreenHeight * 2 / 3;

	gViewPort->m_pIDPanel->setPos( x, y );
}

bool CIDPanel::CanShowID( void )
{
	if ( gEngfuncs.GetMaxClients() > 0 && gViewPort && gEngfuncs.pEventAPI && m_pText )
		return gEngfuncs.GetLocalPlayer() ? true : false;

	return false;
}

// "Friend: " or "Enemy:  " and the player's name, colored to match
void CIDPanel::GetIDString( char *pszBuf, int iLen, int iPlayer, int iLocalPlayer )
{
	// sic: the team info is indexed by player
	team_info_t *pPlayerTeam = &g_TeamInfo[iPlayer];
	team_info_t *pLocalTeam = &g_TeamInfo[iLocalPlayer];

	bool bFriend = pPlayerTeam && pLocalTeam && pPlayerTeam->teamnumber == pLocalTeam->teamnumber && gHUD.m_Teamplay;

	if ( !bFriend )
	{
		sprintf( pszBuf, "Enemy:  " );
		gViewPort->m_pIDPanel->m_pText->setFgColor( 255, 0, 0, 1 );
	}
	else
	{
		sprintf( pszBuf, "Friend: " );
		gViewPort->m_pIDPanel->m_pText->setFgColor( 0, 255, 0, 1 );
	}

	hud_player_info_t *pInfo = &g_PlayerInfoList[iPlayer];
	if ( pInfo )
	{
		int iLeft = iLen - strlen( pszBuf );

		gEngfuncs.pfnGetPlayerInfo( iPlayer, pInfo );

		if ( pInfo->name )
			strncat( pszBuf, pInfo->name, iLeft );
		else
			strncat( pszBuf, "<null>", iLeft );
	}
}

// Trace from the eyes, and show the player hit
void CIDPanel::paint( void )
{
	if ( CanShowID() )
	{
		cl_entity_t *pLocal = gEngfuncs.GetLocalPlayer();

		gEngfuncs.pEventAPI->EV_SetUpPlayerPrediction( false, true );
		gEngfuncs.pEventAPI->EV_PushPMStates();
		gEngfuncs.pEventAPI->EV_SetSolidPlayers( pLocal->index - 1 );
		gEngfuncs.pEventAPI->EV_SetTraceHull( 2 );

		vec3_t view_ofs, angles, forward;
		gEngfuncs.pEventAPI->EV_LocalPlayerViewheight( view_ofs );
		gEngfuncs.GetViewAngles( (float *)angles );
		angles[YAW] = pLocal->angles[YAW];
		gEngfuncs.pfnAngleVectors( angles, forward, NULL, NULL );

		Vector vecDir = forward * 2048;
		Vector vecSrc = pLocal->origin + view_ofs;
		Vector vecEnd = vecSrc + vecDir;

		pmtrace_t tr;
		gEngfuncs.pEventAPI->EV_PlayerTrace( pLocal->origin + view_ofs, vecEnd, PM_STUDIO_BOX, -1, &tr );

		physent_t *pe = gEngfuncs.pEventAPI->EV_GetPhysent( tr.ent );
		if ( pe && pe->info >= 1 && pe->info <= gEngfuncs.GetMaxClients() )
		{
			char szID[256];

			szID[sizeof( szID ) - 1] = '\0';
			GetIDString( szID, 255, pe->info, pLocal->index );
			gViewPort->m_pIDPanel->SetID( szID, 0, 0 );
			gViewPort->m_pIDPanel->m_pText->setVisible( true );

			int iWide, iTall;
			m_pText->getTextImage()->getTextSizeWrapped( iWide, iTall );
			m_pText->setSize( iWide, iTall );

			m_flHideTime = gEngfuncs.GetClientTime() + ID_SHOW_TIME;
		}
		else if ( gEngfuncs.GetClientTime() > m_flHideTime )
		{
			gViewPort->m_pIDPanel->m_pText->setVisible( false );
		}

		gEngfuncs.pEventAPI->EV_PopPMStates();
	}
}

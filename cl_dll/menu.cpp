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
// menu.cpp
//
// generic menu handler
//
#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"

#include <string.h>
#include <stdio.h>

#include "vgui_TeamFortressViewport.h"

#define MAX_MENU_STRING	512
char g_szMenuString[MAX_MENU_STRING];
char g_szPrelocalisedMenuString[MAX_MENU_STRING];

int KB_ConvertString( char *in, char **ppout );

DECLARE_MESSAGE( m_Menu, ShowMenu );

int CHudMenu :: Init( void )
{
	gHUD.AddHudElem( this );

	HOOK_MESSAGE( ShowMenu );

	InitHUDData();

	return 1;
}

void CHudMenu :: InitHUDData( void )
{
	for ( int i = 0; i < MAX_PLAYERS; i++ )
	{
		g_PlayerExtraInfo[i].flash_next = -1;
		g_PlayerExtraInfo[i].dead = true;
	}

	m_fMenuDisplayed = 0;
	m_bitsValidSlots = 0;
	Reset();
}

void CHudMenu :: Reset( void )
{
	g_szPrelocalisedMenuString[0] = 0;
	m_fWaitingForMore = FALSE;
}

int CHudMenu :: VidInit( void )
{
	return 1;
}

// the colour and position of the menu text, set by the escape tokens in the menu string (names ours)
int menu_r, menu_g, menu_b, menu_x, menu_ralign;

// reads an escape token (a backslash and one letter) and returns the text after it (name ours)
inline char *ParseEscapeToken( char *token )
{
	if ( *token != '\\' )
		return token;

	token++;

	switch ( *token )
	{
	case '\0':
		return token;

	case 'w':
		menu_r = 255;
		menu_g = 255;
		menu_b = 255;
		break;

	case 'd':
		menu_r = 100;
		menu_g = 100;
		menu_b = 100;
		break;

	case 'y':
		menu_r = 255;
		menu_g = 210;
		menu_b = 64;
		break;

	case 'r':
		menu_r = 210;
		menu_g = 24;
		menu_b = 0;
		break;

	case 'R':
		menu_x = 299;
		menu_ralign = TRUE;
		break;
	}

	return ++token;
}

int CHudMenu :: Draw( float flTime )
{
	// check for if menu is set to disappear
	if ( m_flShutoffTime > 0 )
	{
		if ( m_flShutoffTime <= gHUD.m_flTime )
		{  // times up, shutoff
			m_fMenuDisplayed = 0;
			m_iFlags &= ~HUD_ACTIVE;
			return 1;
		}
	}

	// don't draw the menu if the scoreboard is being shown
	if ( gViewPort && gViewPort->IsScoreBoardVisible() )
		return 1;

	// draw the menu, along the left-hand side of the screen

	// count the number of newlines
	int nlc = 0;
	for ( int i = 0; i < MAX_MENU_STRING && g_szMenuString[i] != '\0'; i++ )
	{
		if ( g_szMenuString[i] == '\n' )
			nlc++;
	}

	// center it
	int y = (ScreenHeight/2) - ((nlc/2)*12) - 40; // make sure it is above the say text

	menu_r = 255;
	menu_g = 255;
	menu_b = 255;
	menu_x = 20;
	menu_ralign = FALSE;

	char *sptr = g_szMenuString;	// name ours

	while ( *sptr != '\0' )
	{
		if ( *sptr == '\\' )
		{
			sptr = ParseEscapeToken( sptr );
		}
		else if ( *sptr == '\n' )
		{
			menu_ralign = FALSE;
			menu_x = 20;
			y += 12;

			sptr++;
		}
		else
		{
			if ( menu_ralign )
			{
				// right-aligned text is drawn up to the end of the line; it does not parse escape tokens
				menu_x = gHUD.DrawHudStringReverse( menu_x, y, 0, sptr, menu_r, menu_g, menu_b );

				while ( *sptr != '\0' && *sptr != '\n' )
					sptr++;
			}
			else
			{
				menu_x = gHUD.DrawHudString( menu_x, y, 320, sptr, menu_r, menu_g, menu_b );

				while ( *sptr != '\0' && *sptr != '\n' && *sptr != '\\' )
					sptr++;
			}
		}
	}
	
	return 1;
}

// selects an item from the menu
void CHudMenu :: SelectMenuItem( int menu_item )
{
	// Counter-Strike 1.3: no menu keys while the MOTD window is up
	if ( gViewPort->m_pCurrentMenu && gViewPort->m_pCurrentMenu->GetMenuID() == MENU_INTRO )
		return;

	// if menu_item is in a valid slot,  send a menuselect command to the server
	if ( (menu_item > 0) && (m_bitsValidSlots & (1 << (menu_item-1))) )
	{
		char szbuf[32];
		sprintf( szbuf, "menuselect %d\n", menu_item );
		ClientCmd( szbuf );

		// remove the menu
		m_fMenuDisplayed = 0;
		m_iFlags &= ~HUD_ACTIVE;
	}
}


// Message handler for ShowMenu message
// takes four values:
//		short: a bitfield of keys that are valid input
//		char : the duration, in seconds, the menu should stay up. -1 means is stays until something is chosen.
//		byte : a boolean, TRUE if there is more string yet to be received before displaying the menu, FALSE if it's the last string
//		string: menu string to display
// if this message is never received, then scores will simply be the combined totals of the players.
int CHudMenu :: MsgFunc_ShowMenu( const char *pszName, int iSize, void *pbuf )
{
	char *temp = NULL;

	BEGIN_READ( pbuf, iSize );

	m_bitsValidSlots = READ_SHORT();
	int DisplayTime = READ_CHAR();
	int NeedMore = READ_BYTE();

	if ( DisplayTime > 0 )
		m_flShutoffTime = DisplayTime + gHUD.m_flTime;
	else
		m_flShutoffTime = -1;

	if ( m_bitsValidSlots )
	{
		if ( !m_fWaitingForMore ) // this is the start of a new menu
		{
			strncpy( g_szPrelocalisedMenuString, READ_STRING(), MAX_MENU_STRING );
		}
		else
		{  // append to the current menu string
			strncat( g_szPrelocalisedMenuString, READ_STRING(), MAX_MENU_STRING - strlen(g_szPrelocalisedMenuString) );
		}
		g_szPrelocalisedMenuString[MAX_MENU_STRING-1] = 0;  // ensure null termination (strncat/strncpy does not)

		if ( !NeedMore )
		{  // we have the whole string, so we can localise it now
			strcpy( g_szMenuString, gHUD.m_TextMessage.BufferedLocaliseTextString( g_szPrelocalisedMenuString ) );

			// Swap in characters
			if ( KB_ConvertString( g_szMenuString, &temp ) )
			{
				strcpy( g_szMenuString, temp );
				free( temp );
			}
		}

		m_fMenuDisplayed = 1;
		m_iFlags |= HUD_ACTIVE;
	}
	else
	{
		m_fMenuDisplayed = 0; // no valid slots means that the menu should be turned off
		m_iFlags &= ~HUD_ACTIVE;
	}

	m_fWaitingForMore = NeedMore;

	return 1;
}

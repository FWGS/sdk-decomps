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
// hud_redraw.cpp
//
#include <windows.h>
#include <math.h>
#include "hud.h"
#include "cl_util.h"

#include "vgui_TeamFortressViewport.h"

CTempList g_ShellList;	// name ours
CTempList g_SmokeList;	// name ours

#define MAX_LOGO_FRAMES 56

int grgLogoFrame[MAX_LOGO_FRAMES] = 
{
	1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 13, 13, 13, 13, 13, 12, 11, 10, 9, 8, 14, 15,
	16, 17, 18, 19, 20, 20, 20, 20, 20, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 
	29, 29, 29, 29, 29, 28, 27, 26, 25, 24, 30, 31 
};


extern int g_iVisibleMouse;

float HUD_GetFOV( void );

extern cvar_t *sensitivity;

// Think
void CHud::Think(void)
{
	int newfov;
	HUDLIST *pList = m_pHudList;

	while (pList)
	{
		if (pList->p->m_iFlags & HUD_ACTIVE)
			pList->p->Think();
		pList = pList->pNext;
	}

	newfov = HUD_GetFOV();
	if ( newfov == 0 )
	{
		m_iFOV = default_fov->value;
	}
	else
	{
		m_iFOV = newfov;
	}

	// the clients fov is actually set in the client data update section of the hud

	// Set a new sensitivity
	if ( m_iFOV == default_fov->value )
	{  
		// reset to saved sensitivity
		m_flMouseSensitivity = 0;
	}
	else
	{  
		// set a new sensitivity that is proportional to the change from the FOV default
		m_flMouseSensitivity = sensitivity->value * ((float)newfov / (float)default_fov->value) * CVAR_GET_FLOAT("zoom_sensitivity_ratio");
	}

	// think about default fov
	if ( m_iFOV == 0 )
	{  // only let players adjust up in fov,  and only if they are not overriden by something else
		m_iFOV = max( default_fov->value, 90 );  
	}

	g_ShellList.Expire( m_flTime );
	g_SmokeList.Expire( m_flTime );
}

// Redraw
// step through the local data,  placing the appropriate graphics & text as appropriate
// returns 1 if they've changed, 0 otherwise
int CHud :: Redraw( float flTime, int intermission )
{
	m_fOldTime = m_flTime;	// save time of previous redraw
	m_flTime = flTime;
	m_flTimeDelta = (double)m_flTime - m_fOldTime;
	static m_flShotTime = 0;
	
	// Clock was reset, reset delta
	if ( m_flTimeDelta < 0 )
		m_flTimeDelta = 0;

	// Bring up the scoreboard during intermission
	if (gViewPort)
	{
		if ( m_iIntermission && !intermission )
		{
			// Have to do this here so the scoreboard goes away
			m_iIntermission = intermission;
			gViewPort->HideCommandMenu();
			gViewPort->HideScoreBoard();
		}
		else if ( !m_iIntermission && intermission )
		{
			gViewPort->HideCommandMenu();
			gViewPort->ShowScoreBoard();

			// Take a screenshot if the client's got the cvar set
			if ( CVAR_GET_FLOAT( "hud_takesshots" ) != 0 )
				m_flShotTime = flTime + 1.0;	// Take a screenshot in a second
		}
	}

	if (m_flShotTime && m_flShotTime < flTime)
	{
		gEngfuncs.pfnClientCmd("snapshot\n");
		m_flShotTime = 0;
	}

	m_iIntermission = intermission;

	// if no redrawing is necessary
	// return 0;
	
	HUDLIST *pList = m_pHudList;

	while (pList)
	{
		if ( !intermission )
		{
			if ( (pList->p->m_iFlags & HUD_ACTIVE) && !(m_iHideHUDDisplay & HIDEHUD_ALL) )
				pList->p->Draw(flTime);
		}
		else
		{  // it's an intermission,  so only draw hud elements that are set to draw during intermissions
			if ( pList->p->m_iFlags & HUD_INTERMISSION )
				pList->p->Draw( flTime );
		}

		pList = pList->pNext;
	}

	// are we in demo mode? do we need to draw the logo in the top corner?
	if (m_iLogo)
	{
		int x, y, i;

		if (m_hsprLogo == 0)
			m_hsprLogo = LoadSprite("sprites/%d_logo.spr");

		SPR_Set(m_hsprLogo, 250, 250, 250 );
		
		x = SPR_Width(m_hsprLogo, 0);
		x = ScreenWidth - x;
		y = SPR_Height(m_hsprLogo, 0)/2;

		// Draw the logo at 20 fps
		int iFrame = (int)(flTime * 20) % MAX_LOGO_FRAMES;
		i = grgLogoFrame[iFrame] - 1;

		SPR_DrawAdditive(i, x, y, NULL);
	}

	/*
	if ( g_iVisibleMouse )
	{
		void IN_GetMousePos( int *mx, int *my );
		int mx, my;

		IN_GetMousePos( &mx, &my );
		
		if (m_hsprCursor == 0)
		{
			char sz[256];
			sprintf( sz, "sprites/cursor.spr" );
			m_hsprCursor = SPR_Load( sz );
		}

		SPR_Set(m_hsprCursor, 250, 250, 250 );
		
		// Draw the logo at 20 fps
		SPR_DrawAdditive( 0, mx, my, NULL );
	}
	*/
	// twice a second, hold the cvars that matter for fair play at their values
	if ( flTime > m_flCvarCheckTime || m_flCvarCheckTime == -1 )
	{
		// the commands are spelled out byte by byte, not kept as strings; the
		// image stores the bytes down these columns
		char *pszKill = new char[6];		// name ours
		pszKill[0] = 'k';
		pszKill[1] = 'i';
		pszKill[2] = 'l';
		pszKill[3] = 'l';
		pszKill[4] = '\n';
		pszKill[5] = 0;

		char *pszDisconnect = new char[12];	// name ours
		pszDisconnect[0] = 'd';	pszDisconnect[6] = 'n';
		pszDisconnect[1] = 'i';	pszDisconnect[7] = 'e';
		pszDisconnect[2] = 's';	pszDisconnect[8] = 'c';
		pszDisconnect[3] = 'c';	pszDisconnect[9] = 't';
		pszDisconnect[4] = 'o';	pszDisconnect[10] = '\n';
		pszDisconnect[5] = 'n';	pszDisconnect[11] = 0;

		char *pszRestart = new char[9];		// name ours
		pszRestart[0] = 'r';	pszRestart[6] = 't';
		pszRestart[1] = 'e';	pszRestart[7] = '\n';
		pszRestart[2] = 's';	pszRestart[8] = 0;
		pszRestart[3] = 't';
		pszRestart[4] = 'a';
		pszRestart[5] = 'r';

		char *pszQuit = new char[6];		// name ours
		pszQuit[0] = 'q';	pszQuit[5] = 0;
		pszQuit[1] = 'u';
		pszQuit[2] = 'i';
		pszQuit[3] = 't';
		pszQuit[4] = '\n';

		char szCommand[100];	// name ours

		if ( CVAR_GET_FLOAT( "cl_pitchup" ) != 89 )
		{
			ClientCmd( "cl_pitchup 89\n" );
			ClientCmd( pszKill );
			ClientCmd( pszDisconnect );
		}

		if ( CVAR_GET_FLOAT( "cl_pitchdown" ) != 89 )
		{
			ClientCmd( "cl_pitchdown 89\n" );
			ClientCmd( pszKill );
			ClientCmd( pszDisconnect );
		}

		if ( CVAR_GET_FLOAT( "r_drawentities" ) >= 2 )
		{
			ClientCmd( "r_drawentites 1\n" );	// the image spells it so
			ClientCmd( pszKill );
			ClientCmd( pszDisconnect );
		}

		if ( CVAR_GET_FLOAT( "gl_monolights" ) != 0 )
		{
			ClientCmd( "gl_monolights 0\n" );
			ClientCmd( pszRestart );
		}

		if ( CVAR_GET_FLOAT( "sensitivity" ) < 1 )
			ClientCmd( "sensitivity 1\n" );

		if ( CVAR_GET_FLOAT( "gl_clear" ) != 0 )
		{
			ClientCmd( "gl_clear 0\n" );
			ClientCmd( pszKill );
		}

		if ( CVAR_GET_FLOAT( "gl_texsort" ) != m_flTexSort )
		{
			sprintf( szCommand, "gl_texsort %f\n", m_flTexSort );
			ClientCmd( szCommand );
			ClientCmd( pszRestart );
		}

		if ( CVAR_GET_FLOAT( "lambert" ) != 1.5 )
		{
			ClientCmd( "lambert 1.5\n" );
			ClientCmd( pszRestart );
		}

		if ( CVAR_GET_FLOAT( "gl_alphamin" ) != 0.25 && !m_bNoAlphaMin )
		{
			ClientCmd( "gl_alphamin 0.25\n" );
			ClientCmd( pszRestart );
		}

		if ( CVAR_GET_FLOAT( "gl_zmax" ) < m_flZMax && !m_bNoAlphaMin )
		{
			sprintf( szCommand, "gl_zmax %f\n", m_flZMax );
			ClientCmd( szCommand );
			ClientCmd( pszRestart );
		}

		// night vision may change the lighting
		if ( !m_NVG.m_iOn )
		{
			if ( CVAR_GET_FLOAT( "lightgamma" ) != 2.5 )
			{
				ClientCmd( "lightgamma 2.5\n" );
				ClientCmd( pszKill );
			}

			if ( CVAR_GET_FLOAT( "direct" ) < 0.8 )
			{
				ClientCmd( "direct 0.9\n" );
				ClientCmd( pszKill );
			}
		}

		// quit when a debugger is attached
		char *pszKernel = new char[13];		// name ours
		char *pszIsDebuggerPresent = new char[18];	// name ours
		pszKernel[0] = 'k';	pszKernel[5] = 'l';	pszKernel[10] = 'l';
		pszKernel[1] = 'e';	pszKernel[6] = '3';	pszKernel[11] = 'l';
		pszKernel[2] = 'r';	pszKernel[7] = '2';	pszKernel[12] = 0;
		pszKernel[3] = 'n';	pszKernel[8] = '.';
		pszKernel[4] = 'e';	pszKernel[9] = 'd';
		pszIsDebuggerPresent[0] = 'I';	pszIsDebuggerPresent[5] = 'u';	pszIsDebuggerPresent[10] = 'P';	pszIsDebuggerPresent[15] = 'n';
		pszIsDebuggerPresent[1] = 's';	pszIsDebuggerPresent[6] = 'g';	pszIsDebuggerPresent[11] = 'r';	pszIsDebuggerPresent[16] = 't';
		pszIsDebuggerPresent[2] = 'D';	pszIsDebuggerPresent[7] = 'g';	pszIsDebuggerPresent[12] = 'e';	pszIsDebuggerPresent[17] = 0;
		pszIsDebuggerPresent[3] = 'e';	pszIsDebuggerPresent[8] = 'e';	pszIsDebuggerPresent[13] = 's';
		pszIsDebuggerPresent[4] = 'b';	pszIsDebuggerPresent[9] = 'r';	pszIsDebuggerPresent[14] = 'e';

		HMODULE hKernel = LoadLibrary( pszKernel );	// name ours
		BOOL (WINAPI *pfnIsDebuggerPresent)( void ) = (BOOL (WINAPI *)( void ))GetProcAddress( hKernel, pszIsDebuggerPresent );	// name ours
		if ( pfnIsDebuggerPresent && pfnIsDebuggerPresent() )
			ClientCmd( pszQuit );

		float flPitch = CVAR_GET_FLOAT( "m_pitch" );	// name ours
		if ( flPitch < 0.022 && flPitch >= 0.0 )
			ClientCmd( "m_pitch 0.022\n" );
		else if ( flPitch > -0.022 && flPitch < 0.0 )
			ClientCmd( "m_pitch -0.022\n" );

		g_ShellList.m_iMax = CVAR_GET_FLOAT( "max_shells" );
		g_SmokeList.m_iMax = CVAR_GET_FLOAT( "max_smokepuffs" );

		m_flCvarCheckTime = flTime + 0.5;
	}

	return 1;
}

void ScaleColors( int &r, int &g, int &b, int a )
{
	float x = (float)a / 255;
	r = (int)(r * x);
	g = (int)(g * x);
	b = (int)(b * x);
}

int CHud :: DrawHudString(int xpos, int ypos, int iMaxX, char *szIt, int r, int g, int b )
{
	// draw the string until we hit the null character or a newline character
	for ( ; *szIt != 0 && *szIt != '\n' && *szIt != '\\'; szIt++ )
	{
		int next = xpos + gHUD.m_scrinfo.charWidths[ *szIt ]; // variable-width fonts look cool
		if ( next > iMaxX )
			return xpos;

		TextMessageDrawChar( xpos, ypos, *szIt, r, g, b );
		xpos = next;		
	}

	return xpos;
}

int CHud :: DrawHudNumberString( int xpos, int ypos, int iMinX, int iNumber, int r, int g, int b )
{
	char szString[32];
	sprintf( szString, "%d", iNumber );
	return DrawHudStringReverse( xpos, ypos, iMinX, szString, r, g, b );

}

// draws a string from right to left (right-aligned)
int CHud :: DrawHudStringReverse( int xpos, int ypos, int iMinX, char *szString, int r, int g, int b )
{
	// find the end of the string
	for ( char *szIt = szString; *szIt != 0 && *szIt != '\n'; szIt++ )
	{ // we should count the length?		
	}

	// iterate throug the string in reverse
	for ( szIt--;  szIt != (szString-1);  szIt-- )	
	{
		int next = xpos - gHUD.m_scrinfo.charWidths[ *szIt ]; // variable-width fonts look cool
		if ( next < iMinX )
			return xpos;
		xpos = next;

		TextMessageDrawChar( xpos, ypos, *szIt, r, g, b );
	}

	return xpos;
}

// draws a number of five places, blanks for leading zeros; a minus sign is not drawn
int CHud :: DrawHudNumber( int x, int y, int iNumber, int r, int g, int b )
{
	static char szBuffer[16];	// name ours
	char *pszPosint;			// name ours
	int iWidth = GetSpriteRect(m_HUD_number_0).right - GetSpriteRect(m_HUD_number_0).left;
	int k;

	sprintf( szBuffer, "%5i", iNumber );
	if ( iNumber < 0 )
		pszPosint = szBuffer + 1;
	else
		pszPosint = szBuffer;

	while ( *pszPosint )
	{
		k = *pszPosint - '0';
		if ( k >= 0 && k <= 9 )
		{
			SPR_Set(GetSprite(m_HUD_number_0 + k), r, g, b );
			SPR_DrawAdditive( 0, x, y, &GetSpriteRect(m_HUD_number_0 + k));
		}
		x += iWidth;
		pszPosint++;
	}

	return x;
}

int CHud :: DrawHudNumber( int x, int y, int iFlags, int iNumber, int r, int g, int b)
{
	int iWidth = GetSpriteRect(m_HUD_number_0).right - GetSpriteRect(m_HUD_number_0).left;
	int k;
	int iPadZero = iFlags & DHN_PADZERO;	// name ours
	
	if (iNumber > 0)
	{
		// SPR_Draw 100's
		if (iNumber >= 100)
		{
			 k = iNumber/100;
			SPR_Set(GetSprite(m_HUD_number_0 + k), r, g, b );
			SPR_DrawAdditive( 0, x, y, &GetSpriteRect(m_HUD_number_0 + k));
			x += iWidth;
		}
		else if (iFlags & (DHN_3DIGITS))
		{
			if (iPadZero)
				SPR_DrawAdditive( 0, x, y, &GetSpriteRect(m_HUD_number_0) );
			x += iWidth;
		}

		// SPR_Draw 10's
		if (iNumber >= 10)
		{
			k = (iNumber % 100)/10;
			SPR_Set(GetSprite(m_HUD_number_0 + k), r, g, b );
			SPR_DrawAdditive( 0, x, y, &GetSpriteRect(m_HUD_number_0 + k));
			x += iWidth;
		}
		else if (iFlags & (DHN_3DIGITS | DHN_2DIGITS))
		{
			if (iPadZero)
				SPR_DrawAdditive( 0, x, y, &GetSpriteRect(m_HUD_number_0) );
			x += iWidth;
		}

		// SPR_Draw ones
		k = iNumber % 10;
		SPR_Set(GetSprite(m_HUD_number_0 + k), r, g, b );
		SPR_DrawAdditive(0,  x, y, &GetSpriteRect(m_HUD_number_0 + k));
		x += iWidth;
	} 
	else if (iFlags & DHN_DRAWZERO) 
	{
		SPR_Set(GetSprite(m_HUD_number_0), r, g, b );

		// SPR_Draw 100's
		if (iFlags & (DHN_3DIGITS))
		{
			if (iPadZero)
				SPR_DrawAdditive( 0, x, y, &GetSpriteRect(m_HUD_number_0) );
			x += iWidth;
		}

		if (iFlags & (DHN_3DIGITS | DHN_2DIGITS))
		{
			if (iPadZero)
				SPR_DrawAdditive( 0, x, y, &GetSpriteRect(m_HUD_number_0) );
			x += iWidth;
		}

		// SPR_Draw ones
		
		SPR_DrawAdditive( 0,  x, y, &GetSpriteRect(m_HUD_number_0));
		x += iWidth;
	}

	return x;
}


int CHud::GetNumWidth( int iNumber, int iFlags )
{
	if (iFlags & (DHN_3DIGITS))
		return 3;

	if (iFlags & (DHN_2DIGITS))
		return 2;

	if (iNumber <= 0)
	{
		if (iFlags & (DHN_DRAWZERO))
			return 1;
		else
			return 0;
	}

	if (iNumber < 10)
		return 1;

	if (iNumber < 100)
		return 2;

	return 3;

}	



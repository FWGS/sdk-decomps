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
// Train.cpp
//
// implementation of CHudAmmo class
//

#include "hud.h"
#include "cl_util.h"
#include <string.h>
#include <stdio.h>
#include "parsemsg.h"

DECLARE_MESSAGE(m_Train, Train )


int CHudTrain::Init(void)
{
	HOOK_MESSAGE( Train );

	m_iPos = 0;
	m_iFlags = 0;
	gHUD.AddHudElem(this);

	return 1;
};

int CHudTrain::VidInit(void)
{
	m_hSprite = 0;

	return 1;
};

int CHudTrain::Draw(float fTime)
{
	if ( !m_hSprite )
		m_hSprite = LoadSprite("sprites/%d_train.spr");

	if (m_iPos)
	{
		int r, g, b, x, y;

		UnpackRGB(r,g,b, RGB_CYANISH);
		SPR_Set(m_hSprite, r, g, b );

		// This should show up to the right and part way up the armor number
		y = ScreenHeight - SPR_Height(m_hSprite,0) - gHUD.m_iFontHeight;
		x = ScreenWidth/3 + SPR_Width(m_hSprite,0)/4;

		SPR_DrawAdditive( m_iPos - 1,  x, y, NULL);

	}

	return 1;
}


int CHudTrain::MsgFunc_Train(const char *pszName,  int iSize, void *pbuf)
{
	BEGIN_READ( pbuf, iSize );

	// update Train data
	m_iPos = READ_BYTE();

	if (m_iPos)
		m_iFlags |= HUD_ACTIVE;
	else
		m_iFlags &= ~HUD_ACTIVE;

	return 1;
}

// sic: placeholders, ours: functions retail was linked with and dropped
// (nothing calls them); their text is unknown. They give LINK's
// identical-copy sort the (size, relocations) keys with which it keeps
// retail's copies (see Unknown_W1_1 in ev_hldm.cpp): a fit, not evidence.
struct cl_enginefuncs_s;
extern struct cl_enginefuncs_s gEngfuncs;
void InitInput( void );

// key (16, 1)
void Unknown_end_train_1( void )
{
	InitInput();
}

// key (16, 0)
void Unknown_end_train_2( void )
{
}

// key (32, 1)
int Unknown_end_train_3( int *p )
{
	InitInput();
	p[0] = 1;
	return 0;
}

// key (128, 2)
int Unknown_end_train_4( int *p )
{
	InitInput();
	InitInput();
	p[0] = 1;
	p[1] = 2;
	p[2] = 3;
	p[3] = 4;
	p[4] = 5;
	p[5] = 6;
	p[6] = 7;
	p[7] = 8;
	p[8] = 9;
	p[9] = 10;
	p[10] = 11;
	p[11] = 12;
	p[12] = 13;
	p[13] = 14;
	return 0;
}

// key (16, 1)
void Unknown_end_train_5( void )
{
	InitInput();
}

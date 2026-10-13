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
// vgui_AmmoPanel.cpp
//
// the ammo panel: the current weapon's ammo as rows of bullet images in the
// bottom right corner
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
#include "ammohistory.h"

typedef struct
{
	char	*pszIcon;		// TGA of one bullet
	int		iPerClip;		// bullets in one row
	int		iSpacing;		// height of a bullet
} ammopanel_t;

// indexed by ammo type - 1
static ammopanel_t sAmmoPanelTypes[MAX_AMMOPANEL_TYPES] =
{
	{ "ammo_buckshot",	12, 12 },
	{ "ammo_chem",		9,	16 },
	{ "ammo_battery",	12, 13 },
	{ "ammo_minigun",	20, 7 },
	{ "ammo_rocket",	10, 10 },
	{ "ammo_gauss",		10, 14 },
};

CAmmoPanel::CAmmoPanel(int iTrans, int iRemoveMe, int x,int y,int wide,int tall) : CMenuPanel(iTrans, iRemoveMe, x,y,wide,tall)
{
	char sz[256];

	for ( int i = 0; i < MAX_AMMOPANEL_TYPES; i++ )
	{
		sprintf( sz, "%s", sAmmoPanelTypes[i].pszIcon );
		BitmapTGA *pTGA = LoadTGA( sz );

		for ( int j = 0; j < MAX_AMMOPANEL_ICONS; j++ )
		{
			m_pAmmo[i][j] = new CImageLabel( pTGA, 0, sAmmoPanelTypes[i].iSpacing * j, 64, 32 );
			m_pAmmo[i][j]->setParent( this );
			m_pAmmo[i][j]->setVisible( false );
		}
	}

	m_iAmmoType = 0;
	m_iAmmoCount = 0;
	m_iShown = true;
}

// The HUD calls it each frame with the current weapon's ammo
void CAmmoPanel::UpdateAmmo( int iAmmoType, int iMaxClip )
{
	int iCount = gWR.CountAmmo( iAmmoType );
	m_iAmmoCount = iCount;

	// hide the old type's bullets
	if ( m_iAmmoType > 0 && m_iAmmoType <= MAX_AMMOPANEL_TYPES && iAmmoType != m_iAmmoType )
	{
		for ( int i = 0; i < 3; i++ )
			m_pAmmo[m_iAmmoType - 1][i]->setVisible( false );
	}

	m_iAmmoType = iAmmoType;

	if ( iAmmoType < 1 || iAmmoType > MAX_AMMOPANEL_TYPES )
		return;

	int iPerClip = sAmmoPanelTypes[iAmmoType - 1].iPerClip;
	int iClips = iMaxClip / iPerClip;
	if ( iClips < 1 )
		return;

	int iLeft = iCount % iPerClip;
	if ( iLeft < 1 )
	{
		iLeft = iCount ? iPerClip : 0;
	}
	else if ( iLeft > iPerClip * iClips )
	{
		m_iClips = iClips;
		iLeft = iCount % ( iPerClip * iClips );
	}

	m_iClips = ( iCount - iLeft ) / iPerClip;

	// sic: only ever shows the first three
	for ( int i = 0; i < 3; i++ )
		m_pAmmo[iAmmoType - 1][i]->setVisible( true );
}

bool CAmmoPanel::SlotInput( int iSlot )
{
	return false;
}

void CAmmoPanel::Open( void )
{
	Update();
	CMenuPanel::Open();
}

void CAmmoPanel::Update( void )
{
}

void CAmmoPanel::SetActiveInfo( int iInput )
{
}

void CAmmoPanel::Initialize( void )
{
}

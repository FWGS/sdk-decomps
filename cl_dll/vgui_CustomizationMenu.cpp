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
// vgui_CustomizationMenu.cpp
//
// the weapon customization menu (STUB: being reconstructed)
//

#include "VGUI_Font.h"

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
#include "vgui_CustomizationMenu.h"

char *GetConfigText( const char *pszName, char *pszText )
{
	return NULL;
}

CCustomizationMenu::CCustomizationMenu( int iTrans, int iRemoveMe, int x, int y, int wide, int tall ) : CMenuPanel( iTrans, iRemoveMe, x, y, wide, tall )
{
}

void CCustomizationMenu::HideAll( void )
{
	setVisible( false );
}

void CCustomizationMenu::paint( void )
{
}

void CCustomizationMenu::Reset( void )
{
	CMenuPanel::Reset();
}

void CCustomizationMenu::Open( void )
{
	Update();
	CMenuPanel::Open();
}

void CCustomizationMenu::Initialize( void )
{
	setVisible( false );
}

void CCustomizationMenu::Update( void )
{
}

/***
*
*	Copyright (c) 1999, 2000 Valve LLC. All rights reserved.
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
// Wanted: the telegram panel. Menu 3 (the class menu) shows a full-screen
// picture instead, the TGA the VGUIMenu message names; a click anywhere
// sends "closetelegram". Cut down from vgui_ClassMenu.cpp, which stays in
// the project but is no longer used. The file name, CTelegramPanel,
// g_szTelegram and the member names are ours; InvisibleButton is retail's.
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

// Class Menu Dimensions
#define CLASSMENU_WINDOW_SIZE_Y			YRES(312)

// The telegram's name, from the VGUIMenu message
char g_szTelegram[128];

// A button that draws nothing: it covers the panel and takes the click
class VGUIAPI InvisibleButton : public Button
{
public:
	InvisibleButton(const char* text,int x,int y,int wide,int tall) : Button(text,x,y,wide,tall)
	{
	}

	virtual void paintBackground()
	{
	}
};

// Creation
CTelegramPanel::CTelegramPanel(int iTrans, int iRemoveMe, int x,int y,int wide,int tall) : CMenuPanel(iTrans, iRemoveMe, x,y,wide,tall)
{
	// Create the Scroll panel
	m_pScrollPanel = new ScrollPanel( 0, 0, wide, tall );
	m_pScrollPanel->setParent(this);
	//force the scrollbars on, so after the validate clientClip will be smaller
	m_pScrollPanel->setScrollBarAutoVisible(false, false);
	m_pScrollPanel->setScrollBarVisible(true, true);
	m_pScrollPanel->validate();

	int clientWide=m_pScrollPanel->getClient()->getWide();

	//turn scrollpanel back into auto show scrollbar mode and validate
	m_pScrollPanel->setScrollBarAutoVisible(false,false);
	m_pScrollPanel->setScrollBarVisible(false,false);
	m_pScrollPanel->validate();

	// Create the Info Window
	m_pClassInfoPanel = new CTransparentPanel( 255, 0, 0, clientWide, CLASSMENU_WINDOW_SIZE_Y );
	m_pClassInfoPanel->setParent( m_pScrollPanel->getClient() );

	// The telegram, centred
	m_pClassImage = new CImageLabel( "telegram", 0, 0 );
	m_pClassImage->setParent( m_pClassInfoPanel );
	m_pClassImage->setPos( wide * 0.5 - m_pClassImage->getImageWide() * 0.5, tall * 0.5 - m_pClassImage->getImageTall() * 0.5 );

	// Clicking anywhere closes it
	InvisibleButton *pButton = new InvisibleButton( "", 0, 0, wide, tall );
	pButton->setParent( this );
	pButton->addActionSignal( new CMenuHandler_StringCommand( "closetelegram", true ) );

	m_pClassInfoPanel->setSize( wide, tall );
}

//======================================
// Load the telegram the server named before opening
void CTelegramPanel::Open( void )
{
	m_pClassInfoPanel->setVisible( true );

	// LoadTGA wants the name without the extension
	char sz[128];
	strcpy( sz, g_szTelegram );
	if ( strstr( g_szTelegram, ".tga" ) )
	{
		sz[ strlen( sz ) - 4 ] = '\0';
		strncpy( sz, g_szTelegram, strstr( g_szTelegram, ".tga" ) - g_szTelegram );
		sz[ strstr( g_szTelegram, ".tga" ) - g_szTelegram ] = '\0';
	}

	m_pClassImage->m_pTGA = LoadTGA( sz );
	m_pClassImage->setImage( m_pClassImage->m_pTGA );
	m_pClassImage->setPos( ScreenWidth * 0.5 - m_pClassImage->getImageWide() * 0.5, ScreenHeight * 0.5 - m_pClassImage->getImageTall() * 0.5 );

	m_pScrollPanel->setScrollValue(0,0);
	m_pScrollPanel->validate();

	CMenuPanel::Open();
}

//-----------------------------------------------------------------------------
// Purpose: Called each time a new level is started.
//-----------------------------------------------------------------------------
void CTelegramPanel::Initialize( void )
{
	setVisible( false );
	m_pScrollPanel->setScrollValue( 0, 0 );
}

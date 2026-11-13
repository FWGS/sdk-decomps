//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: Counter-Strike's rifle buy menu (file and class names ours):
// the class menu (vgui_ClassMenu.cpp) with the weapon's picture above its text
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================

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
#define CLASSMENU_TITLE_X				XRES(40)
#define CLASSMENU_TITLE_Y				YRES(32)
#define CLASSMENU_TOPLEFT_BUTTON_X		XRES(40)
#define CLASSMENU_TOPLEFT_BUTTON_Y		YRES(80)
#define CLASSMENU_BUTTON_SIZE_X			XRES(124)
#define CLASSMENU_BUTTON_SIZE_Y			YRES(24)
#define CLASSMENU_BUTTON_SPACER_Y		YRES(8)
#define CLASSMENU_WINDOW_X				XRES(176)
#define CLASSMENU_WINDOW_Y				YRES(80)
#define CLASSMENU_WINDOW_SIZE_X			XRES(424)
#define CLASSMENU_WINDOW_SIZE_Y			YRES(312)
#define CLASSMENU_WINDOW_TEXT_X			XRES(150)
#define CLASSMENU_WINDOW_TEXT_Y			YRES(110)
#define CLASSMENU_WINDOW_NAME_X			8
#define CLASSMENU_WINDOW_NAME_Y			YRES(8)
#define CLASSMENU_WINDOW_PLAYERS_Y		YRES(42)

// Creation
CRifleMenuPanel::CRifleMenuPanel(int iTrans, int iRemoveMe, int x,int y,int wide,int tall) : CMenuPanel(iTrans, iRemoveMe, x,y,wide,tall)
{
	// don't show class graphics at below 640x480 resolution
	bool bShowClassGraphic = true;
	if ( ScreenWidth < 640 )
	{
		bShowClassGraphic = false;
	}

	memset( m_pClassImages, 0, sizeof(m_pClassImages) );

	// Get the scheme used for the Titles
	CSchemeManager *pSchemes = gViewPort->GetSchemeManager();

	// schemes
	SchemeHandle_t hTitleScheme = pSchemes->getSchemeHandle( "Title Font" );
	SchemeHandle_t hClassWindowText = pSchemes->getSchemeHandle( "Briefing Text" );

	// color schemes
	int r, g, b, a;

	// Create the title
	Label *pLabel = new Label( "", CLASSMENU_TITLE_X, CLASSMENU_TITLE_Y );
	pLabel->setParent( this );
	pLabel->setFont( pSchemes->getFont(hTitleScheme) );
	pSchemes->getFgColor( hTitleScheme, r, g, b, a );
	pLabel->setFgColor( r, g, b, a );
	pSchemes->getBgColor( hTitleScheme, r, g, b, a );
	pLabel->setBgColor( r, g, b, a );
	pLabel->setContentAlignment( vgui::Label::a_west );
	pLabel->setText(gHUD.m_TextMessage.BufferedLocaliseTextString("Rifle Selection"));

	// Create the Scroll panel
	m_pScrollPanel = new CTFScrollPanel( CLASSMENU_WINDOW_X, CLASSMENU_WINDOW_Y, CLASSMENU_WINDOW_SIZE_X, CLASSMENU_WINDOW_SIZE_Y );
	m_pScrollPanel->setParent(this);
	//force the scrollbars on, so after the validate clientClip will be smaller
	m_pScrollPanel->setScrollBarAutoVisible(false, false);
	m_pScrollPanel->setScrollBarVisible(true, true);
	m_pScrollPanel->setBorder( new LineBorder( Color(255 * 0.7,170 * 0.7,0,0) ) );
	m_pScrollPanel->validate();

	int clientWide=m_pScrollPanel->getClient()->getWide();

	//turn scrollpanel back into auto show scrollbar mode and validate
	m_pScrollPanel->setScrollBarAutoVisible(false,true);
	m_pScrollPanel->setScrollBarVisible(false,false);
	m_pScrollPanel->validate();

	// Create the Class buttons
	for (int i = 0; i <= 7; i++)
	{
		char sz[256]; 
		int iYPos = CLASSMENU_TOPLEFT_BUTTON_Y + ( (CLASSMENU_BUTTON_SIZE_Y + CLASSMENU_BUTTON_SPACER_Y) * i );

		// Class button
		switch ( i )
		{
		case 0:
			m_pButtons[i] = new CommandButton( CHudTextMessage::BufferedLocaliseTextString( "#AK47" ), CLASSMENU_TOPLEFT_BUTTON_X, iYPos, CLASSMENU_BUTTON_SIZE_X, CLASSMENU_BUTTON_SIZE_Y, true );
			strcpy( sz, "slot1" );
			break;
		case 1:
			m_pButtons[i] = new CommandButton( CHudTextMessage::BufferedLocaliseTextString( "#Krieg552" ), CLASSMENU_TOPLEFT_BUTTON_X, iYPos, CLASSMENU_BUTTON_SIZE_X, CLASSMENU_BUTTON_SIZE_Y, true );
			strcpy( sz, "slot2" );
			break;
		case 2:
			m_pButtons[i] = new CommandButton( CHudTextMessage::BufferedLocaliseTextString( "#M4A1_Short" ), CLASSMENU_TOPLEFT_BUTTON_X, iYPos, CLASSMENU_BUTTON_SIZE_X, CLASSMENU_BUTTON_SIZE_Y, true );
			strcpy( sz, "slot3" );
			break;
		case 3:
			m_pButtons[i] = new CommandButton( CHudTextMessage::BufferedLocaliseTextString( "#Aug" ), CLASSMENU_TOPLEFT_BUTTON_X, iYPos, CLASSMENU_BUTTON_SIZE_X, CLASSMENU_BUTTON_SIZE_Y, true );
			strcpy( sz, "slot4" );
			break;
		case 4:
			m_pButtons[i] = new CommandButton( CHudTextMessage::BufferedLocaliseTextString( "#Schmidt" ), CLASSMENU_TOPLEFT_BUTTON_X, iYPos, CLASSMENU_BUTTON_SIZE_X, CLASSMENU_BUTTON_SIZE_Y, true );
			strcpy( sz, "slot5" );
			break;
		case 5:
			m_pButtons[i] = new CommandButton( CHudTextMessage::BufferedLocaliseTextString( "#Magnum" ), CLASSMENU_TOPLEFT_BUTTON_X, iYPos, CLASSMENU_BUTTON_SIZE_X, CLASSMENU_BUTTON_SIZE_Y, true );
			strcpy( sz, "slot6" );
			break;
		case 6:
			m_pButtons[i] = new CommandButton( CHudTextMessage::BufferedLocaliseTextString( "#D3AU1" ), CLASSMENU_TOPLEFT_BUTTON_X, iYPos, CLASSMENU_BUTTON_SIZE_X, CLASSMENU_BUTTON_SIZE_Y, true );
			strcpy( sz, "slot7" );
			break;
		case 7:
			m_pButtons[i] = new CommandButton( CHudTextMessage::BufferedLocaliseTextString( "#Krieg550" ), CLASSMENU_TOPLEFT_BUTTON_X, iYPos, CLASSMENU_BUTTON_SIZE_X, CLASSMENU_BUTTON_SIZE_Y, true );
			strcpy( sz, "slot8" );
			break;
		default:
			m_pButtons[i] = new CommandButton( CHudTextMessage::BufferedLocaliseTextString( "#AK47" ), CLASSMENU_TOPLEFT_BUTTON_X, iYPos, CLASSMENU_BUTTON_SIZE_X, CLASSMENU_BUTTON_SIZE_Y, true );
			strcpy( sz, "slot1" );
			break;
		}

		m_pButtons[i]->setBoundKey( sz[4] );
		m_pButtons[i]->setContentAlignment( vgui::Label::a_west );
		m_pButtons[i]->addActionSignal( new CMenuHandler_StringCommandWatch( sz, true ) );
		m_pButtons[i]->addInputSignal( new CHandler_MenuButtonOver(this, i) );
		m_pButtons[i]->setParent( this );

		// Create the Class Info Window
		m_pClassInfoPanel[i] = new CTransparentPanel( 255, 0, 0, clientWide, CLASSMENU_WINDOW_SIZE_Y );
		m_pClassInfoPanel[i]->setParent( m_pScrollPanel->getClient() );
		m_pClassInfoPanel[i]->setVisible( false );

		// don't show class pic in lower resolutions
		int textOffs = XRES(8);

		if ( bShowClassGraphic )
		{
			textOffs = CLASSMENU_WINDOW_NAME_X;
		}

		// Create the Class Name Label
		switch ( i )
		{
		case 0:
			sprintf( sz, "#AK47" );
			break;
		case 1:
			sprintf( sz, "#SG552" );
			break;
		case 2:
			sprintf( sz, "#M4A1" );
			break;
		case 3:
			sprintf( sz, "#Aug" );
			break;
		case 4:
			sprintf( sz, "#Scout" );
			break;
		case 5:
			sprintf( sz, "#ArcticWarfareMagnum" );
			break;
		case 6:
			sprintf( sz, "#G3SG1" );
			break;
		case 7:
			sprintf( sz, "#SG550" );
			break;
		default:
			sprintf( sz, "#AK47" );
			break;
		}

		char* localName=CHudTextMessage::BufferedLocaliseTextString( sz );
		Label *pNameLabel = new Label( "", textOffs, CLASSMENU_WINDOW_NAME_Y );
		pNameLabel->setFont( pSchemes->getFont(hTitleScheme) ); 
		pNameLabel->setParent( m_pClassInfoPanel[i] );
		pSchemes->getFgColor( hTitleScheme, r, g, b, a );
		pNameLabel->setFgColor( r, g, b, a );
		pSchemes->getBgColor( hTitleScheme, r, g, b, a );
		pNameLabel->setBgColor( r, g, b, a );
		pNameLabel->setContentAlignment( vgui::Label::a_west );
		pNameLabel->setText(localName);

		// Create the Class Image
		if ( bShowClassGraphic )
		{
			switch ( i )
			{
			case 0:
				strcpy( sz, "ak47" );
				break;
			case 1:
				strcpy( sz, "sg552" );
				break;
			case 2:
				strcpy( sz, "m4a1" );
				break;
			case 3:
				strcpy( sz, "aug" );
				break;
			case 4:
				strcpy( sz, "scout" );
				break;
			case 5:
				strcpy( sz, "awp" );
				break;
			case 6:
				strcpy( sz, "g3sg1" );
				break;
			case 7:
				strcpy( sz, "sg550" );
				break;
			default:
				strcpy( sz, "ak47" );
				break;
			}

			m_pClassImages[i] = new CImageLabel( sz, 0, 0, CLASSMENU_WINDOW_TEXT_X, CLASSMENU_WINDOW_TEXT_Y );

			CImageLabel *pLabel = m_pClassImages[i];
			pLabel->setParent( m_pClassInfoPanel[i] );

			// Reposition it based upon it's size
			int xOut, yOut;
			pNameLabel->getTextSize( xOut, yOut );
			int xPos = ( CLASSMENU_WINDOW_SIZE_X / 2 ) - ( pLabel->getWide() / 2 );
			if ( xPos < 0 )
				xPos = 0;
			int yPos = yOut * 2;
			pLabel->setPos( xPos, yPos );
		}

		// Create the Player count string
		gHUD.m_TextMessage.LocaliseTextString( "#Title_CurrentlyOnYourTeam", m_sPlayersOnTeamString, STRLENMAX_PLAYERSONTEAM );
		m_pPlayers[i] = new Label( "", textOffs, CLASSMENU_WINDOW_PLAYERS_Y );
		m_pPlayers[i]->setParent( m_pClassInfoPanel[i] );
		m_pPlayers[i]->setBgColor( 0, 0, 0, 255 );
		m_pPlayers[i]->setContentAlignment( vgui::Label::a_west );
		m_pPlayers[i]->setFont( pSchemes->getFont(hClassWindowText) );

		// Open up the Class Briefing File
		switch ( i )
		{
		case 0:
			sprintf( sz, "classes/%s.txt", "ak47" );
			break;
		case 1:
			sprintf( sz, "classes/%s.txt", "sg552" );
			break;
		case 2:
			sprintf( sz, "classes/%s.txt", "m4a1" );
			break;
		case 3:
			sprintf( sz, "classes/%s.txt", "aug" );
			break;
		case 4:
			sprintf( sz, "classes/%s.txt", "scout" );
			break;
		case 5:
			sprintf( sz, "classes/%s.txt", "awp" );
			break;
		case 6:
			sprintf( sz, "classes/%s.txt", "g3sg1" );
			break;
		case 7:
			sprintf( sz, "classes/%s.txt", "sg550" );
			break;
		default:
			sprintf( sz, "classes/%s.txt", "ak47" );
			break;
		}

		char *cText = "Class Description not available.";
		char *pfile = (char*)gEngfuncs.COM_LoadFile( sz, 5, NULL );
		if (pfile)
			cText = pfile;
		
		// Create the Text info window
		TextPanel *pTextWindow = new TextPanel(cText, textOffs, CLASSMENU_WINDOW_TEXT_Y, (CLASSMENU_WINDOW_SIZE_X - textOffs)-5, CLASSMENU_WINDOW_SIZE_Y - CLASSMENU_WINDOW_TEXT_Y);
		pTextWindow->setParent( m_pClassInfoPanel[i] );
		pTextWindow->setFont( pSchemes->getFont(hClassWindowText) );
		pSchemes->getFgColor( hClassWindowText, r, g, b, a );
		pTextWindow->setFgColor( r, g, b, a );
		pSchemes->getBgColor( hClassWindowText, r, g, b, a );
		pTextWindow->setBgColor( r, g, b, a );

		// Resize the Info panel to fit it all
		int wide,tall;
		pTextWindow->getTextImage()->getTextSizeWrapped( wide,tall);
		pTextWindow->setSize(wide,tall);
		int xx,yy;
		pTextWindow->getPos(xx,yy);
		int maxX=xx+wide;
		int maxY=yy+tall;

		//check to see if the image goes lower than the text
		if(m_pClassImages[i]!=null)
		{
			m_pClassImages[i]->getPos(xx,yy);
			if((yy+m_pClassImages[i]->getTall())>maxY)
			{
				maxY=yy+m_pClassImages[i]->getTall();
			}
		}

		m_pClassInfoPanel[i]->setSize( maxX , maxY );
	}

	// Create the Cancel button
	m_pCancelButton = new CommandButton( CHudTextMessage::BufferedLocaliseTextString( "CANCEL" ), CLASSMENU_TOPLEFT_BUTTON_X, YRES(392) - CLASSMENU_BUTTON_SIZE_Y, CLASSMENU_BUTTON_SIZE_X, CLASSMENU_BUTTON_SIZE_Y, false );
	m_pCancelButton->setParent( this );
	m_pCancelButton->setBoundKey( '0' );
	m_pCancelButton->addActionSignal( new CMenuHandler_TextWindow(HIDE_TEXTWINDOW) );
	m_pCancelButton->addActionSignal( new CMenuHandler_StringCommandWatch( "slot10", true ) );

	m_iCurrentInfo = 0;
}


// Update (first after the constructor since Beta 7.1: the image keeps the rifle
// menu's own SlotInput after it, and with Update ahead of SlotInput VC6 reloads
// bTerrorist before iMapType after positioning a button, as 7.1 does; 7.0 had
// SlotInput and Open first and the other reload order)
void CRifleMenuPanel::Update()
{
	int	 iYPos = CLASSMENU_TOPLEFT_BUTTON_Y;
	int iMapType;
	bool bTerrorist;

	const char *pszMapName = gEngfuncs.pfnGetLevelName();

	if ( !strncmp( pszMapName, "maps/as_", 8 ) )
		iMapType = 1;
	else if ( !strncmp( pszMapName, "maps/cs_", 8 ) )
		iMapType = 2;
	else if ( !strncmp( pszMapName, "maps/de_", 8 ) )
		iMapType = 3;
	else if ( !strncmp( pszMapName, "maps/es_", 8 ) )
		iMapType = 4;

	for ( int j = 1; j < MAX_PLAYERS; j++ )
	{
		if ( g_PlayerInfoList[j].thisplayer )
			bTerrorist = ( g_PlayerExtraInfo[j].teamnumber == 1 );
	}

	// Cycle through the rest of the buttons
	for (int i = 0; i <= 7; i++)
	{
		if ( m_pButtons[i]->IsNotValid() )
		{
			m_pButtons[i]->setVisible( false );
		}
		else
		{
			m_pButtons[i]->setVisible( true );

			switch ( i )
			{
			case 0:
				if ( bTerrorist == false )
					m_pButtons[i]->setVisible( false );
				break;
			case 1:
				if ( bTerrorist == false || iMapType == 1 )
					m_pButtons[i]->setVisible( false );
				break;
			case 2:
				if ( bTerrorist == true )
					m_pButtons[i]->setVisible( false );
				break;
			case 3:
				if ( bTerrorist == true )
					m_pButtons[i]->setVisible( false );
				break;
			case 4:
				if ( iMapType == 1 )
					m_pButtons[i]->setVisible( false );
				break;
			case 5:
				if ( bTerrorist == false && iMapType == 1 )
					m_pButtons[i]->setVisible( false );
				break;
			case 6:
				if ( ( bTerrorist == true && iMapType == 1 ) || bTerrorist == false )
					m_pButtons[i]->setVisible( false );
				break;
			case 7:
				if ( bTerrorist == true )
					m_pButtons[i]->setVisible( false );
				break;
			}

			m_pButtons[i]->setPos( CLASSMENU_TOPLEFT_BUTTON_X, iYPos );
			iYPos += CLASSMENU_BUTTON_SIZE_Y + CLASSMENU_BUTTON_SPACER_Y;
		}

		m_pPlayers[i]->setFgColor( 50, 200, 50, 0 );

		// set the graphic
		if ( m_pClassImages[i] )
		{
			m_pClassImages[i]->setVisible( false );
		}

		if ( m_pClassImages[i] != NULL )
		{
			m_pClassImages[i]->setVisible( true );
		}
	}

	m_pCancelButton->setVisible( true );
}

//======================================
// Key inputs for the Class Menu
bool CRifleMenuPanel::SlotInput( int iSlot )
{
	if ( (iSlot < 0) || (iSlot > 9) )
		return false;

	if ( iSlot == 9 )
	{
		if ( !(m_pCancelButton->IsNotValid()) )
		{
			m_pCancelButton->fireActionSignal();
			return true;
		}
	}

	if ( !m_pButtons[ iSlot ] )
		return false;

	if ( !(m_pButtons[ iSlot ]->IsNotValid()) )
	{
		m_pButtons[ iSlot ]->fireActionSignal();
		return true;
	}

	return false;
}

//======================================
// Update the Class menu before opening it
void CRifleMenuPanel::Open( void )
{
	Update();
	CMenuPanel::Open();
}

//-----------------------------------------------------------------------------
// Purpose: Called each time a new level is started.
//-----------------------------------------------------------------------------
void CRifleMenuPanel::Initialize( void )
{
	setVisible( false );
	m_pScrollPanel->setScrollValue( 0, 0 );
}

//======================================
// Mouse is over a class button, bring up the class info
void CRifleMenuPanel::SetActiveInfo( int iInput )
{
	// Remove all the Info panels and bring up the specified one
	for (int i = 0; i <= 7; i++)
	{
		m_pButtons[i]->setArmed( false );
		m_pClassInfoPanel[i]->setVisible( false );
	}

	m_pCancelButton->setArmed( false );

	if ( iInput > 9 || iInput < 0 )
		iInput = 0;

	m_pButtons[iInput]->setArmed( true );
	m_pClassInfoPanel[iInput]->setVisible( true );
	m_iCurrentInfo = iInput;

	m_pScrollPanel->setScrollValue(0,0);
	m_pScrollPanel->validate();
}

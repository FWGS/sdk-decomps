//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: Counter-Strike's terrorist model menu (file and class names ours):
// the CT model menu's code with the terrorists' models
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
#define CLASSMENU_WINDOW_TEXT_Y			YRES(80)
#define CLASSMENU_WINDOW_NAME_X			XRES(150)
#define CLASSMENU_WINDOW_NAME_Y			YRES(8)
#define CLASSMENU_WINDOW_PLAYERS_Y		YRES(42)

// Creation
CTerroristMenuPanel::CTerroristMenuPanel(int iTrans, int iRemoveMe, int x,int y,int wide,int tall) : CMenuPanel(iTrans, iRemoveMe, x,y,wide,tall)
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
	pLabel->setText(gHUD.m_TextMessage.BufferedLocaliseTextString("#Title_terrorist_model_selection"));

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
	for (int i = 0; i <= 4; i++)
	{
		char sz[256]; 
		int iYPos = CLASSMENU_TOPLEFT_BUTTON_Y + ( (CLASSMENU_BUTTON_SIZE_Y + CLASSMENU_BUTTON_SPACER_Y) * i );

		// Class button
		switch ( i )
		{
		case 0:
			m_pButtons[i] = new CommandButton( CHudTextMessage::BufferedLocaliseTextString( "#Phoenix_Connexion" ), CLASSMENU_TOPLEFT_BUTTON_X, iYPos, CLASSMENU_BUTTON_SIZE_X, CLASSMENU_BUTTON_SIZE_Y, true );
			strcpy( sz, "slot1" );
			break;
		case 1:
			m_pButtons[i] = new CommandButton( CHudTextMessage::BufferedLocaliseTextString( "#L337_Krew" ), CLASSMENU_TOPLEFT_BUTTON_X, iYPos, CLASSMENU_BUTTON_SIZE_X, CLASSMENU_BUTTON_SIZE_Y, true );
			strcpy( sz, "slot2" );
			break;
		case 2:
			m_pButtons[i] = new CommandButton( CHudTextMessage::BufferedLocaliseTextString( "#Arctic_Avengers" ), CLASSMENU_TOPLEFT_BUTTON_X, iYPos, CLASSMENU_BUTTON_SIZE_X, CLASSMENU_BUTTON_SIZE_Y, true );
			strcpy( sz, "slot3" );
			break;
		case 3:
			m_pButtons[i] = new CommandButton( CHudTextMessage::BufferedLocaliseTextString( "#Guerilla_Warfare" ), CLASSMENU_TOPLEFT_BUTTON_X, iYPos, CLASSMENU_BUTTON_SIZE_X, CLASSMENU_BUTTON_SIZE_Y, true );
			strcpy( sz, "slot4" );
			break;
		case 4:
			iYPos += CLASSMENU_BUTTON_SIZE_Y + CLASSMENU_BUTTON_SPACER_Y;
			m_pButtons[i] = new CommandButton( CHudTextMessage::BufferedLocaliseTextString( "#Auto_Select" ), CLASSMENU_TOPLEFT_BUTTON_X, iYPos, CLASSMENU_BUTTON_SIZE_X, CLASSMENU_BUTTON_SIZE_Y, true );
			strcpy( sz, "slot5" );
			break;
		default:
			m_pButtons[i] = new CommandButton( CHudTextMessage::BufferedLocaliseTextString( "#Phoenix_Connexion" ), CLASSMENU_TOPLEFT_BUTTON_X, iYPos, CLASSMENU_BUTTON_SIZE_X, CLASSMENU_BUTTON_SIZE_Y, true );
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
			sprintf( sz, CHudTextMessage::BufferedLocaliseTextString( "#Phoenix_Connexion" ) );
			break;
		case 1:
			sprintf( sz, CHudTextMessage::BufferedLocaliseTextString( "#L337_Krew" ) );
			break;
		case 2:
			sprintf( sz, CHudTextMessage::BufferedLocaliseTextString( "#Arctic_Avengers" ) );
			break;
		case 3:
			sprintf( sz, CHudTextMessage::BufferedLocaliseTextString( "#Guerilla_Warfare" ) );
			break;
		case 4:
			sprintf( sz, CHudTextMessage::BufferedLocaliseTextString( "#Auto_Select" ) );
			break;
		default:
			sprintf( sz, CHudTextMessage::BufferedLocaliseTextString( "#Phoenix_Connexion" ) );
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
				strcpy( sz, "terror" );
				break;
			case 1:
				strcpy( sz, "leet" );
				break;
			case 2:
				strcpy( sz, "arctic" );
				break;
			case 3:
				strcpy( sz, "guerilla" );
				break;
			case 4:
				strcpy( sz, "t_random" );
				break;
			default:
				strcpy( sz, "terror" );
				break;
			}

			m_pClassImages[i] = new CImageLabel( sz, 0, 0, CLASSMENU_WINDOW_TEXT_X, CLASSMENU_WINDOW_TEXT_Y );

			CImageLabel *pLabel = m_pClassImages[i];
			pLabel->setParent( m_pClassInfoPanel[i] );

			// Reposition it based upon it's size
			int xOut, yOut;
			pNameLabel->getTextSize( xOut, yOut );
			pLabel->setPos( (CLASSMENU_WINDOW_TEXT_X - pLabel->getWide()) / 2, yOut /2 );
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
			sprintf( sz, "classes/%s.txt", "terror" );
			break;
		case 1:
			sprintf( sz, "classes/%s.txt", "leet" );
			break;
		case 2:
			sprintf( sz, "classes/%s.txt", "arctic" );
			break;
		case 3:
			sprintf( sz, "classes/%s.txt", "guerilla" );
			break;
		case 4:
			sprintf( sz, "classes/%s.txt", "autoselect" );
			break;
		default:
			sprintf( sz, "classes/%s.txt", "terror" );
			break;
		}

		char *cText = CHudTextMessage::BufferedLocaliseTextString( "#Class_descr_not_avail" );
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

	m_iCurrentInfo = 0;
}


// Update
void CTerroristMenuPanel::Update()
{
	// Don't allow the player to join a team if they're not in a team
	if (!g_iTeamNumber)
		return;

	int	 iYPos = CLASSMENU_TOPLEFT_BUTTON_Y;

	// Cycle through the rest of the buttons
	for (int i = 0; i <= 4; i++)
	{
		bool bCivilian = (gViewPort->GetValidClasses(g_iTeamNumber) == -1);

		if ( bCivilian )
		{
			// If this team can only be civilians, only the civilian button's visible
			if (i == 0)
			{
				m_pButtons[0]->setVisible( true );
				SetActiveInfo( 0 );
				iYPos += CLASSMENU_BUTTON_SIZE_Y + CLASSMENU_BUTTON_SPACER_Y;
			}
			else
			{
				m_pButtons[i]->setVisible( false );
			}
		}
		else 
		{
			if ( m_pButtons[i]->IsNotValid() || i == 0 )
			{
				m_pButtons[i]->setVisible( false );
			}
			else
			{
				m_pButtons[i]->setVisible( true );
				m_pButtons[i]->setPos( CLASSMENU_TOPLEFT_BUTTON_X, iYPos );
				iYPos += CLASSMENU_BUTTON_SIZE_Y + CLASSMENU_BUTTON_SPACER_Y;

				// Start with the first option up
				if (!m_iCurrentInfo)
					SetActiveInfo( i );
			}
		}

		// Now count the number of teammembers of this class
		int iTotal = 0;
		for ( int j = 1; j < MAX_PLAYERS; j++ )
		{
			if ( g_PlayerInfoList[j].name == NULL )
				continue; // empty player slot, skip
			if ( g_PlayerExtraInfo[j].teamname[0] == 0 )
				continue; // skip over players who are not in a team
			if ( g_PlayerInfoList[j].thisplayer )
				continue; // skip this player
			if ( g_PlayerExtraInfo[j].teamnumber != g_iTeamNumber )
				continue; // skip over players in other teams

			// If this team is forced to be civilians, just count the number of teammates
			if ( g_PlayerExtraInfo[j].playerclass != i && !bCivilian )
				continue;

			iTotal++;
		}

		char sz[256]; 
		sprintf(sz, m_sPlayersOnTeamString, iTotal);
		m_pPlayers[i]->setText( sz );

		// Set the text color to the teamcolor
		m_pPlayers[i]->setFgColor( iTeamColors[g_iTeamNumber][0], iTeamColors[g_iTeamNumber][1], iTeamColors[g_iTeamNumber][2], 0 );

		// set the graphic to be the team pick
		if ( m_pClassImages[i] )
		{
			m_pClassImages[i]->setVisible( false );
		}

		if ( m_pClassImages[i] != NULL )
		{
			m_pClassImages[i]->setVisible( true );
		}
	}
}

//======================================
// Key inputs for the Class Menu
bool CTerroristMenuPanel::SlotInput( int iSlot )
{
	if ( (iSlot < 0) || (iSlot > 4) )
		return false;
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
void CTerroristMenuPanel::Open( void )
{
	Update();
	CMenuPanel::Open();
}

//-----------------------------------------------------------------------------
// Purpose: Called each time a new level is started.
//-----------------------------------------------------------------------------
void CTerroristMenuPanel::Initialize( void )
{
	setVisible( false );
	m_pScrollPanel->setScrollValue( 0, 0 );
}

//======================================
// Mouse is over a class button, bring up the class info
void CTerroristMenuPanel::SetActiveInfo( int iInput )
{
	// Remove all the Info panels and bring up the specified one
	for (int i = 0; i <= 4; i++)
	{
		m_pButtons[i]->setArmed( false );
		m_pClassInfoPanel[i]->setVisible( false );
	}

	if ( iInput > 5 || iInput < 0 )
		iInput = 0;

	m_pButtons[iInput]->setArmed( true );
	m_pClassInfoPanel[iInput]->setVisible( true );
	m_iCurrentInfo = iInput;

	m_pScrollPanel->setScrollValue(0,0);
	m_pScrollPanel->validate();
}

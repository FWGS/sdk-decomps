//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: Counter-Strike's buy menu (file and class names ours): the
// team menu's shape, a button for each kind of purchase
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================

#include "vgui_int.h"
#include "VGUI_Font.h"
#include "VGUI_ScrollPanel.h"
#include "VGUI_TextImage.h"

#include "hud.h"
#include "cl_util.h"
#include "vgui_TeamFortressViewport.h"

// Buy Menu Dimensions
#define TEAMMENU_TITLE_X				XRES(40)
#define TEAMMENU_TITLE_Y				YRES(32)
#define TEAMMENU_TOPLEFT_BUTTON_X		XRES(40)
#define TEAMMENU_TOPLEFT_BUTTON_Y		YRES(80)
#define TEAMMENU_BUTTON_SIZE_X			XRES(124)
#define TEAMMENU_BUTTON_SIZE_Y			YRES(24)
#define TEAMMENU_BUTTON_SPACER_Y		YRES(8)
#define TEAMMENU_WINDOW_X				XRES(176)
#define TEAMMENU_WINDOW_Y				YRES(80)
#define TEAMMENU_WINDOW_SIZE_X			XRES(200)
#define TEAMMENU_WINDOW_SIZE_Y			YRES(100)
#define TEAMMENU_WINDOW_INFO_X			XRES(8)
#define TEAMMENU_WINDOW_INFO_Y			YRES(8)
      
// Creation
CBuyMenuPanel::CBuyMenuPanel(int iTrans, int iRemoveMe, int x,int y,int wide,int tall) : CMenuPanel(iTrans, iRemoveMe, x,y,wide,tall)
{
	// Get the scheme used for the Titles
	CSchemeManager *pSchemes = gViewPort->GetSchemeManager();

	// schemes
	SchemeHandle_t hTitleScheme = pSchemes->getSchemeHandle( "Title Font" );
	SchemeHandle_t hTeamWindowText = pSchemes->getSchemeHandle( "Briefing Text" );
	SchemeHandle_t hTeamInfoText = pSchemes->getSchemeHandle( "Team Info Text" );

	// get the Font used for the Titles
	Font *pTitleFont = pSchemes->getFont( hTitleScheme );
	int r, g, b, a;

	// Create the title
	Label *pLabel = new Label( "", TEAMMENU_TITLE_X, TEAMMENU_TITLE_Y );
	pLabel->setParent( this );
	pLabel->setFont( pTitleFont );
	pSchemes->getFgColor( hTitleScheme, r, g, b, a );
	pLabel->setFgColor( r, g, b, a );
	pSchemes->getBgColor( hTitleScheme, r, g, b, a );
	pLabel->setBgColor( r, g, b, a );
	pLabel->setContentAlignment( vgui::Label::a_west );
	pLabel->setText(gHUD.m_TextMessage.BufferedLocaliseTextString("#Title_select_category_of_purchase"));

	// Create the Info Window
	m_pTeamWindow  = new CTransparentPanel( 255, TEAMMENU_WINDOW_X, TEAMMENU_WINDOW_Y, TEAMMENU_WINDOW_SIZE_X, TEAMMENU_WINDOW_SIZE_Y );
	m_pTeamWindow->setParent( this );
	m_pTeamWindow->setBorder( new LineBorder( Color(255*0.7,170*0.7,0,0 )) );

	// Buy Menu buttons
	for (int i = 0; i <= 7; i++)
	{
		char sz[256]; 

		int iYPos = TEAMMENU_TOPLEFT_BUTTON_Y + ( (TEAMMENU_BUTTON_SIZE_Y + TEAMMENU_BUTTON_SPACER_Y) * i );

		switch ( i )
		{
		case 0:
			m_pButtons[i] = new CommandButton( CHudTextMessage::BufferedLocaliseTextString( "#Pistols" ), TEAMMENU_TOPLEFT_BUTTON_X, iYPos, TEAMMENU_BUTTON_SIZE_X, TEAMMENU_BUTTON_SIZE_Y, true );
			strcpy( sz, "slot1" );
			break;
		case 1:
			m_pButtons[i] = new CommandButton( CHudTextMessage::BufferedLocaliseTextString( "#Shotguns" ), TEAMMENU_TOPLEFT_BUTTON_X, iYPos, TEAMMENU_BUTTON_SIZE_X, TEAMMENU_BUTTON_SIZE_Y, true );
			strcpy( sz, "slot2" );
			break;
		case 2:
			m_pButtons[i] = new CommandButton( CHudTextMessage::BufferedLocaliseTextString( "#SMGs" ), TEAMMENU_TOPLEFT_BUTTON_X, iYPos, TEAMMENU_BUTTON_SIZE_X, TEAMMENU_BUTTON_SIZE_Y, true );
			strcpy( sz, "slot3" );
			break;
		case 3:
			m_pButtons[i] = new CommandButton( CHudTextMessage::BufferedLocaliseTextString( "#Rifles" ), TEAMMENU_TOPLEFT_BUTTON_X, iYPos, TEAMMENU_BUTTON_SIZE_X, TEAMMENU_BUTTON_SIZE_Y, true );
			strcpy( sz, "slot4" );
			break;
		case 4:
			m_pButtons[i] = new CommandButton( CHudTextMessage::BufferedLocaliseTextString( "#MachineGuns" ), TEAMMENU_TOPLEFT_BUTTON_X, iYPos, TEAMMENU_BUTTON_SIZE_X, TEAMMENU_BUTTON_SIZE_Y, true );
			strcpy( sz, "slot5" );
			break;
		case 5:
			m_pButtons[i] = new CommandButton( CHudTextMessage::BufferedLocaliseTextString( "#Prim_Ammo" ), TEAMMENU_TOPLEFT_BUTTON_X, iYPos, TEAMMENU_BUTTON_SIZE_X, TEAMMENU_BUTTON_SIZE_Y, true );
			strcpy( sz, "slot6" );
			break;
		case 6:
			m_pButtons[i] = new CommandButton( CHudTextMessage::BufferedLocaliseTextString( "#Sec_Ammo" ), TEAMMENU_TOPLEFT_BUTTON_X, iYPos, TEAMMENU_BUTTON_SIZE_X, TEAMMENU_BUTTON_SIZE_Y, true );
			strcpy( sz, "slot7" );
			break;
		case 7:
			m_pButtons[i] = new CommandButton( CHudTextMessage::BufferedLocaliseTextString( "#Equipment" ), TEAMMENU_TOPLEFT_BUTTON_X, iYPos, TEAMMENU_BUTTON_SIZE_X, TEAMMENU_BUTTON_SIZE_Y, true );
			strcpy( sz, "slot8" );
			break;
		default:
			m_pButtons[i] = new CommandButton( CHudTextMessage::BufferedLocaliseTextString( "#Pistols" ), TEAMMENU_TOPLEFT_BUTTON_X, iYPos, TEAMMENU_BUTTON_SIZE_X, TEAMMENU_BUTTON_SIZE_Y, true );
			strcpy( sz, "slot1" );
			break;
		}

		m_pButtons[i]->setParent( this );
		m_pButtons[i]->setContentAlignment( vgui::Label::a_west );
		m_pButtons[i]->setVisible( false );
		m_pButtons[i]->setBoundKey( sz[4] );

		// Create the Signals
		m_pButtons[i]->addActionSignal( new CMenuHandler_StringCommandWatch( sz, true ) );
		m_pButtons[i]->addInputSignal( new CHandler_MenuButtonOver(this, i) );

		// Create the Info panel
		m_pTeamInfoPanel[i] = new TextPanel("", TEAMMENU_WINDOW_INFO_X, TEAMMENU_WINDOW_INFO_Y, TEAMMENU_WINDOW_SIZE_X - TEAMMENU_WINDOW_INFO_X, TEAMMENU_WINDOW_SIZE_X - TEAMMENU_WINDOW_INFO_Y );
		m_pTeamInfoPanel[i]->setParent( m_pTeamWindow );
		m_pTeamInfoPanel[i]->setFont( pSchemes->getFont(hTeamInfoText) );
		pSchemes->getFgColor( hTitleScheme, r, g, b, a );
		m_pTeamInfoPanel[i]->setFgColor( r, g, b, a );
		m_pTeamInfoPanel[i]->setBgColor( 0,0,0, 255 );
		m_pTeamInfoPanel[i]->setVisible( false );
	}

	// Create the Cancel button
	m_pCancelButton = new CommandButton( CHudTextMessage::BufferedLocaliseTextString( "#Menu_Cancel" ), TEAMMENU_TOPLEFT_BUTTON_X, YRES(392) - TEAMMENU_BUTTON_SIZE_Y, TEAMMENU_BUTTON_SIZE_X, TEAMMENU_BUTTON_SIZE_Y, false );
	m_pCancelButton->setParent( this );
	m_pCancelButton->setBoundKey( '0' );
	m_pCancelButton->addActionSignal( new CMenuHandler_TextWindow(HIDE_TEXTWINDOW) );
	m_pCancelButton->addActionSignal( new CMenuHandler_StringCommandWatch( "slot10", true ) );

	Initialize();
}

//-----------------------------------------------------------------------------
// Purpose: Called each time a new level is started.
//-----------------------------------------------------------------------------
void CBuyMenuPanel::Initialize( void )
{
	m_iCurrentInfo = 0;
}

//-----------------------------------------------------------------------------
// Purpose: Called everytime the Buy Menu is displayed
//-----------------------------------------------------------------------------
void CBuyMenuPanel::Update( void )
{
	int	 iYPos = TEAMMENU_TOPLEFT_BUTTON_Y;

	// Set the buttons
	for (int i = 0; i <= 7; i++)
	{
		if (m_pButtons[i])
		{
			m_pButtons[i]->setVisible( true );
			m_pButtons[i]->setPos( TEAMMENU_TOPLEFT_BUTTON_X, iYPos );
			iYPos += TEAMMENU_BUTTON_SIZE_Y + TEAMMENU_BUTTON_SPACER_Y;

			switch ( i )
			{
			case 0:
				m_pTeamInfoPanel[i]->setText( CHudTextMessage::BufferedLocaliseTextString( "#Buy_pistols" ) );
				break;
			case 1:
				m_pTeamInfoPanel[i]->setText( CHudTextMessage::BufferedLocaliseTextString( "#Buy_shotguns" ) );
				break;
			case 2:
				m_pTeamInfoPanel[i]->setText( CHudTextMessage::BufferedLocaliseTextString( "#Buy_smgs" ) );
				break;
			case 3:
				m_pTeamInfoPanel[i]->setText( CHudTextMessage::BufferedLocaliseTextString( "#Buy_rifles" ) );
				break;
			case 4:
				m_pTeamInfoPanel[i]->setText( CHudTextMessage::BufferedLocaliseTextString( "#Buy_machineguns" ) );
				break;
			case 5:
				m_pTeamInfoPanel[i]->setText( CHudTextMessage::BufferedLocaliseTextString( "#Buy_prim_ammo" ) );
				break;
			case 6:
				m_pTeamInfoPanel[i]->setText( CHudTextMessage::BufferedLocaliseTextString( "#Buy_sec_ammo" ) );
				break;
			case 7:
				m_pTeamInfoPanel[i]->setText( CHudTextMessage::BufferedLocaliseTextString( "#Buy_equipment" ) );
				break;
			default:
				m_pTeamInfoPanel[i]->setText( CHudTextMessage::BufferedLocaliseTextString( "#Buy_pistols" ) );
				break;
			}
		}
	}

	m_pCancelButton->setVisible( true );
}

//=====================================
// Key inputs
bool CBuyMenuPanel::SlotInput( int iSlot )
{
	if ( (iSlot < 0) || ( (iSlot > 7) && (iSlot != 9) ) )
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

	// Is the button pushable?
	if ( m_pButtons[ iSlot ]->isVisible() )
	{
		m_pButtons[ iSlot ]->fireActionSignal();
		return true;
	}

	return false;
}

//======================================
// Update the Buy menu before opening it
void CBuyMenuPanel::Open( void )
{
	Update();
	CMenuPanel::Open();
}

void CBuyMenuPanel::paintBackground()
{
	CMenuPanel::paintBackground();
}

//======================================
// Mouse is over a button, bring up the info
void CBuyMenuPanel::SetActiveInfo( int iInput )
{
	// Remove all the Info panels and bring up the specified one
	for (int i = 0; i <= 7; i++)
	{
		m_pButtons[i]->setArmed( false );
		m_pTeamInfoPanel[i]->setVisible( false );
	}

	m_pButtons[iInput]->setArmed( true );
	m_pTeamInfoPanel[iInput]->setVisible( true );

	m_iCurrentInfo = iInput;
}

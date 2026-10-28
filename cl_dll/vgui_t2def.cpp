//=========== T2DEF (Todesangst 2) ===========
//
// Purpose: T2DEF's VGUI panels: the Config menu with its Settings panel
// (music and caption language, sent to the server as t2def* commands),
// the letterbox panel for cut-scene captions and the security camera panel.
//
// File, class, member and local names are ours (locals chosen for their
// stack slots); strings and layouts are retail's.
//=============================================================================

#include "vgui_int.h"
#include "VGUI_Font.h"
#include "VGUI_ScrollPanel.h"
#include "VGUI_TextImage.h"

#include "hud.h"
#include "cl_util.h"
#include "vgui_TeamFortressViewport.h"

int CMenuHandler_SecurityCam::m_iCamera = -1;

char *sConfigMenuNames[] =
{
	" Config",
	"Weapons",
	"Characters",
};

char *sSettingsCommands[] =
{
	"t2defmusicon",
	"t2defmusicoff",
	"t2defenglish",
	"t2defgerman",
	"t2defnotext",
};

char *sSecurityCamCommands[] =
{
	"t2defexitcam",
	"t2defprevcam",
	"t2defnextcam",
};

char *sMusicStatus[] =
{
	"Controls playback of mp3 soundtrack during the game. Status: Enabled",
	"Controls playback of mp3 soundtrack during the game. Status: Disabled",
};

char *sLanguageStatus[] =
{
	"Language for subtitles during cinematic cut scene playback. Status: English",
	"Language for subtitles during cinematic cut scene playback. Status: German",
	"Language for subtitles during cinematic cut scene playback. Status: Disabled",
};

//======================================================================================
// CONFIG MENU
//======================================================================================
CConfigMenuPanel::CConfigMenuPanel(int iTrans, int iRemoveMe, int x,int y,int wide,int tall) : CMenuPanel(iTrans, iRemoveMe, x,y,wide,tall)
{
	// Get the scheme used for the Titles
	CSchemeManager *pSchemes = gViewPort->GetSchemeManager();

	// schemes
	SchemeHandle_t hTitleText = pSchemes->getSchemeHandle( "Title Font" );
	SchemeHandle_t hBriefingText = pSchemes->getSchemeHandle( "Briefing Text" );
	SchemeHandle_t hButtonText = pSchemes->getSchemeHandle( "Primary Button Text" );

	// color schemes
	int r, g, b, a;

	// Create the window
	m_pWindow = new CTransparentPanel( 200, XRES(80), YRES(80), XRES(480), YRES(320) );
	m_pWindow->setParent( this );
	m_pWindow->setBorder( new LineBorder( Color(0,0,190,0) ) );

	// Create the Return button
	m_pCancelButton = new CommandButton( CHudTextMessage::BufferedLocaliseTextString( "  Return to game" ), XRES(315), YRES(282), XRES(150), YRES(26) );
	m_pCancelButton->setContentAlignment( vgui::Label::a_west );
	m_pCancelButton->setParent( m_pWindow );
	m_pCancelButton->setFont( pSchemes->getFont(hButtonText) );
	m_pCancelButton->addActionSignal( new CMenuHandler_TextWindow(HIDE_TEXTWINDOW) );

	// Create the title
	m_pTitle = new Label( "", XRES(1), YRES(1), XRES(478), YRES(58) );
	m_pTitle->setParent( m_pWindow );
	m_pTitle->setFont( pSchemes->getFont(hTitleText) );
	pSchemes->getFgColor( hTitleText, r, g, b, a );
	m_pTitle->setFgColor( r, g, b, a );
	pSchemes->getBgColor( hTitleText, r, g, b, a );
	m_pTitle->setBgColor( r, g, b, a );
	m_pTitle->setTextAlignment( vgui::Label::a_center );
	m_pTitle->setText( "Todesangst 2: der echte Feind" );

	// Create the section buttons
	for ( int i = 0; i < 1; i++ )
	{
		int iYPos = 60 + ( 50 * i );
		char sz[256];

		sprintf( sz, "%s", CHudTextMessage::BufferedLocaliseTextString( sConfigMenuNames[i] ) );
		m_pButtons[i] = new CommandButton( sz, XRES(10), YRES(iYPos), XRES(80), YRES(25) );
		m_pButtons[i]->setContentAlignment( vgui::Label::a_west );
		m_pButtons[i]->setParent( m_pWindow );
		m_pButtons[i]->setFont( pSchemes->getFont(hButtonText) );
		m_pButtons[i]->addActionSignal( new CMenuHandler_ConfigSection( i ) );
	}

	// Create the Settings panel
	m_pSettings = new CSettingsPanel( 0, 0, XRES(100), YRES(50), XRES(375), YRES(220) );
	m_pSettings->setParent( m_pWindow );
}

void CConfigMenuPanel::ShowSection( int iSection )
{
}

//======================================================================================
// SETTINGS PANEL
//======================================================================================
CSettingsPanel::CSettingsPanel(int iTrans, int iRemoveMe, int x,int y,int wide,int tall) : CMenuPanel(iTrans, iRemoveMe, x,y,wide,tall)
{
	// Get the scheme used for the Titles
	CSchemeManager *pSchemes = gViewPort->GetSchemeManager();

	// schemes
	SchemeHandle_t hTitleText = pSchemes->getSchemeHandle( "Scoreboard Title Text" );
	SchemeHandle_t hBriefingText = pSchemes->getSchemeHandle( "Briefing Text" );
	SchemeHandle_t hButtonText = pSchemes->getSchemeHandle( "Primary Button Text" );

	// color schemes
	int r, g, b, a;
	char sz[256];

	// Create the window
	m_pWindow = new CTransparentPanel( 200, XRES(0), YRES(0), XRES(375), YRES(220) );
	m_pWindow->setParent( this );
	m_pWindow->setBorder( new LineBorder( Color(0,0,190,0) ) );

	// Music
	m_pMusicTitle = new Label( "", XRES(10), YRES(10), XRES(355), YRES(25) );
	m_pMusicTitle->setParent( m_pWindow );
	m_pMusicTitle->setFont( pSchemes->getFont(hTitleText) );
	pSchemes->getFgColor( hTitleText, r, g, b, a );
	m_pMusicTitle->setFgColor( r, g, b, a );
	pSchemes->getBgColor( hTitleText, r, g, b, a );
	m_pMusicTitle->setBgColor( r, g, b, a );
	m_pMusicTitle->setTextAlignment( vgui::Label::a_center );
	m_pMusicTitle->setText( "Music Settings" );

	m_pButtons[0] = new CommandButton( CHudTextMessage::BufferedLocaliseTextString( "  Enable" ), XRES(97), YRES(75), XRES(80), YRES(25) );
	m_pButtons[1] = new CommandButton( CHudTextMessage::BufferedLocaliseTextString( "  Disable" ), XRES(198), YRES(75), XRES(80), YRES(25) );

	// Language
	m_pButtons[2] = new CommandButton( CHudTextMessage::BufferedLocaliseTextString( "  English" ), XRES(47), YRES(185), XRES(80), YRES(25) );
	m_pButtons[3] = new CommandButton( CHudTextMessage::BufferedLocaliseTextString( "  German" ), XRES(147), YRES(185), XRES(80), YRES(25) );
	m_pButtons[4] = new CommandButton( CHudTextMessage::BufferedLocaliseTextString( "  Disable" ), XRES(247), YRES(185), XRES(80), YRES(25) );

	for ( int i = 0; i < 5; i++ )
	{
		char szCommand[256];

		m_pButtons[i]->setContentAlignment( vgui::Label::a_west );
		m_pButtons[i]->setParent( m_pWindow );
		m_pButtons[i]->setFgColor( 0, 64, 0, 0 );
		m_pButtons[i]->setFont( pSchemes->getFont(hButtonText) );

		sprintf( szCommand, "%s", CHudTextMessage::BufferedLocaliseTextString( sSettingsCommands[i] ) );
		m_pButtons[i]->addActionSignal( new CMenuHandler_StringCommand( szCommand ) );
	}

	m_pMusicStatus = new Label( "", XRES(10), YRES(40), XRES(355), YRES(25) );
	m_pMusicStatus->setParent( m_pWindow );
	m_pMusicStatus->setFont( pSchemes->getFont(hBriefingText) );
	pSchemes->getFgColor( hBriefingText, r, g, b, a );
	m_pMusicStatus->setFgColor( r, g, b, a );
	pSchemes->getBgColor( hBriefingText, r, g, b, a );
	m_pMusicStatus->setBgColor( r, g, b, a );
	m_pMusicStatus->setTextAlignment( vgui::Label::a_center );
	sprintf( sz, "%s", CHudTextMessage::BufferedLocaliseTextString( sMusicStatus[0] ) );
	m_pMusicStatus->setText( sz );

	m_pLanguageTitle = new Label( "", XRES(10), YRES(120), XRES(355), YRES(25) );
	m_pLanguageTitle->setParent( m_pWindow );
	m_pLanguageTitle->setFont( pSchemes->getFont(hTitleText) );
	pSchemes->getFgColor( hTitleText, r, g, b, a );
	m_pLanguageTitle->setFgColor( r, g, b, a );
	pSchemes->getBgColor( hTitleText, r, g, b, a );
	m_pLanguageTitle->setBgColor( r, g, b, a );
	m_pLanguageTitle->setTextAlignment( vgui::Label::a_center );
	m_pLanguageTitle->setText( "Language Settings" );

	m_pLanguageStatus = new Label( "", XRES(10), YRES(150), XRES(355), YRES(25) );
	m_pLanguageStatus->setParent( m_pWindow );
	m_pLanguageStatus->setFont( pSchemes->getFont(hBriefingText) );
	pSchemes->getFgColor( hBriefingText, r, g, b, a );
	m_pLanguageStatus->setFgColor( r, g, b, a );
	pSchemes->getBgColor( hBriefingText, r, g, b, a );
	m_pLanguageStatus->setBgColor( r, g, b, a );
	m_pLanguageStatus->setTextAlignment( vgui::Label::a_center );
	sprintf( sz, "%s", CHudTextMessage::BufferedLocaliseTextString( sLanguageStatus[0] ) );
	m_pLanguageStatus->setText( sz );
}

// Show the server's settings (GetSettings)
void CSettingsPanel::Update( int iMusicOn, int iTextLanguage )
{
	char sz[256];

	// sic: indexed by the "music on" flag, so on reads "Disabled"
	sprintf( sz, "%s", CHudTextMessage::BufferedLocaliseTextString( sMusicStatus[iMusicOn] ) );
	m_pMusicStatus->setText( sz );

	sprintf( sz, "%s", CHudTextMessage::BufferedLocaliseTextString( sLanguageStatus[iTextLanguage] ) );
	m_pLanguageStatus->setText( sz );
}

//======================================================================================
// LETTERBOX CAPTIONS
//======================================================================================
CLetterboxPanel::CLetterboxPanel(int iRemoveMe, int x,int y,int wide,int tall) : CMenuPanel(0, iRemoveMe, x,y,wide,tall)
{
	// Get the scheme used for the Titles
	CSchemeManager *pSchemes = gViewPort->GetSchemeManager();

	// schemes
	SchemeHandle_t hBriefingText = pSchemes->getSchemeHandle( "Briefing Text" );
	SchemeHandle_t hSpeakerText = pSchemes->getSchemeHandle( "Speaker Text" );

	// color schemes
	int r, g, b, a;

	// the bars
	m_pBottomBar = new CTransparentPanel( 5, XRES(0), YRES(380), XRES(640), YRES(100) );
	m_pBottomBar->setParent( this );

	m_pTopBar = new CTransparentPanel( 5, XRES(0), YRES(0), XRES(640), YRES(50) );
	m_pTopBar->setParent( this );

	// the speaker
	m_pSpeaker = new TextPanel( "", XRES(15), YRES(15), XRES(80), YRES(70) );
	m_pSpeaker->setParent( m_pBottomBar );
	m_pSpeaker->setFont( pSchemes->getFont(hSpeakerText) );
	pSchemes->getFgColor( hSpeakerText, r, g, b, a );
	m_pSpeaker->setFgColor( r, g, b, a );
	pSchemes->getBgColor( hSpeakerText, r, g, b, a );
	m_pSpeaker->setBgColor( r, g, b, a );
	m_pSpeaker->setText( " " );

	// the caption
	m_pText = new TextPanel( "", XRES(105), YRES(15), XRES(520), YRES(70) );
	m_pText->setParent( m_pBottomBar );
	m_pText->setFont( pSchemes->getFont(hBriefingText) );
	pSchemes->getFgColor( hBriefingText, r, g, b, a );
	m_pText->setFgColor( r, g, b, a );
	pSchemes->getBgColor( hBriefingText, r, g, b, a );
	m_pText->setBgColor( r, g, b, a );
	m_pText->setText( " " );
}

// pszSpeaker starts with '1' (new caption) or '2' (continued caption)
void CLetterboxPanel::SetCaption( const char *pszSpeaker, const char *pszText )
{
	if ( !pszSpeaker || !pszText )
		return;

	if ( !strncmp( pszSpeaker, "1", 1 ) )
		strcpy( m_szText, pszText );
	else if ( !strncmp( pszSpeaker, "2", 1 ) )
		strcat( m_szText, pszText );
	else
		return;

	m_pSpeaker->setText( pszSpeaker + 1 );
	m_pText->setText( m_szText );

	// Get the total size of the text and resize the text panel
	int iScrollSizeX, iScrollSizeY;

	m_pText->getTextImage()->setSize( XRES(520), YRES(70) );
	m_pText->getTextImage()->getTextSizeWrapped( iScrollSizeX, iScrollSizeY );
	m_pText->setSize( iScrollSizeX, iScrollSizeY );
}

//======================================================================================
// SECURITY CAMERA
//======================================================================================
CSecurityCamPanel::CSecurityCamPanel(int iRemoveMe, int x,int y,int wide,int tall) : CMenuPanel(0, iRemoveMe, x,y,wide,tall)
{
	// Get the scheme used for the buttons
	CSchemeManager *pSchemes = gViewPort->GetSchemeManager();
	SchemeHandle_t hButtonText = pSchemes->getSchemeHandle( "Primary Button Text" );
	char sz[256];

	// the frame
	m_pLeftBar = new CTransparentPanel( 1, XRES(0), YRES(0), XRES(50), YRES(480) );
	m_pLeftBar->setParent( this );

	if ( XRES(640) == 800 )
		m_pRightBar = new CTransparentPanel( 1, XRES(590), YRES(0), 63, YRES(480) );
	else
		m_pRightBar = new CTransparentPanel( 1, XRES(590), YRES(0), XRES(50), YRES(480) );
	m_pRightBar->setParent( this );

	m_pTopBar = new CTransparentPanel( 1, XRES(50), YRES(0), XRES(540), YRES(50) );
	m_pTopBar->setParent( this );

	m_pBottomBar = new CTransparentPanel( 1, XRES(50), YRES(380), XRES(540), YRES(100) );
	m_pBottomBar->setParent( this );

	// the buttons
	m_pExitButton = new CommandButton( CHudTextMessage::BufferedLocaliseTextString( " Exit" ), XRES(0), YRES(15), XRES(80), YRES(25) );
	m_pExitButton->setContentAlignment( vgui::Label::a_west );
	m_pExitButton->setParent( m_pBottomBar );
	m_pExitButton->setFont( pSchemes->getFont(hButtonText) );
	sprintf( sz, "%s", CHudTextMessage::BufferedLocaliseTextString( sSecurityCamCommands[0] ) );
	m_pExitButton->addActionSignal( new CMenuHandler_SecurityCam( sz, true ) );

	m_pPrevButton = new CommandButton( CHudTextMessage::BufferedLocaliseTextString( " Previous" ), XRES(360), YRES(15), XRES(80), YRES(25) );
	m_pPrevButton->setContentAlignment( vgui::Label::a_west );
	m_pPrevButton->setParent( m_pBottomBar );
	m_pPrevButton->setFont( pSchemes->getFont(hButtonText) );
	sprintf( sz, "%s", CHudTextMessage::BufferedLocaliseTextString( sSecurityCamCommands[1] ) );
	m_pPrevButton->addActionSignal( new CMenuHandler_SecurityCam( sz ) );

	m_pNextButton = new CommandButton( CHudTextMessage::BufferedLocaliseTextString( " Next" ), XRES(460), YRES(15), XRES(80), YRES(25) );
	m_pNextButton->setContentAlignment( vgui::Label::a_west );
	m_pNextButton->setParent( m_pBottomBar );
	m_pNextButton->setFont( pSchemes->getFont(hButtonText) );
	sprintf( sz, "%s", CHudTextMessage::BufferedLocaliseTextString( sSecurityCamCommands[2] ) );
	m_pNextButton->addActionSignal( new CMenuHandler_SecurityCam( sz ) );
}

// The camera whose entity index the buttons send (SCShow)
void CSecurityCamPanel::SetCamera( int iCamera )
{
	CMenuHandler_SecurityCam::SetCamera( iCamera );
}

// Leave the camera as the Exit button does
void CSecurityCamPanel::Exit( void )
{
	m_pExitButton->fireActionSignal();
}

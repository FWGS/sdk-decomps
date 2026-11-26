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
// the weapon customization menu: while a customization item is selected the
// HUD shows its weapon's choices here, read from text/configtext.txt, with
// the current ones highlighted
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
#include <VGUI_TextImage.h>
#include <ctype.h>

// sic: a stand-in, names ours, nothing uses it. Retail's file declared more
// before its functions than ours does (declarations whose text is unknown).
// VC6 numbers every symbol it declares, string literals included, and its
// back end recycles the register-candidate slots of "Dml Title Text" and
// "Dml Text" in the order of those numbers modulo 64. That decides which of
// this and iRow ShowBeamgun reloads first after its inner loop: 24 to 34
// more symbols give retail's order. This enum declares 29 (28 constants and
// the enum). The count is relative to the placeholders below, which declare
// symbols too.
enum { SIC_CUSTMENU_0, SIC_CUSTMENU_1, SIC_CUSTMENU_2, SIC_CUSTMENU_3, SIC_CUSTMENU_4, SIC_CUSTMENU_5, SIC_CUSTMENU_6, SIC_CUSTMENU_7, SIC_CUSTMENU_8, SIC_CUSTMENU_9, SIC_CUSTMENU_10, SIC_CUSTMENU_11, SIC_CUSTMENU_12, SIC_CUSTMENU_13, SIC_CUSTMENU_14, SIC_CUSTMENU_15, SIC_CUSTMENU_16, SIC_CUSTMENU_17, SIC_CUSTMENU_18, SIC_CUSTMENU_19, SIC_CUSTMENU_20, SIC_CUSTMENU_21, SIC_CUSTMENU_22, SIC_CUSTMENU_23, SIC_CUSTMENU_24, SIC_CUSTMENU_25, SIC_CUSTMENU_26, SIC_CUSTMENU_27 };

// sic: placeholders, ours: functions retail was linked with and dropped
// (nothing calls them); their text is unknown. They give LINK's
// identical-copy sort the (size, relocations) keys with which it keeps
// retail's copies (see Unknown_W1_1 in ev_hldm.cpp): a fit, not evidence.
struct cl_enginefuncs_s;
extern struct cl_enginefuncs_s gEngfuncs;
void InitInput( void );

// key (16, 0)
void Unknown_top_vgui_CustomizationMenu_1( void )
{
}

// key (1184, 40), 10 absolute
int Unknown_top_vgui_CustomizationMenu_2( int *p )
{
	InitInput();
	InitInput();
	InitInput();
	InitInput();
	InitInput();
	InitInput();
	InitInput();
	InitInput();
	InitInput();
	InitInput();
	InitInput();
	InitInput();
	InitInput();
	InitInput();
	InitInput();
	InitInput();
	InitInput();
	InitInput();
	InitInput();
	InitInput();
	InitInput();
	InitInput();
	InitInput();
	InitInput();
	InitInput();
	InitInput();
	InitInput();
	InitInput();
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
	p[14] = 15;
	p[15] = 16;
	p[16] = 17;
	p[17] = 18;
	p[18] = 19;
	p[19] = 20;
	p[20] = 21;
	p[21] = 22;
	p[22] = 23;
	p[23] = 24;
	p[24] = 25;
	p[25] = 26;
	p[26] = 27;
	p[27] = 28;
	p[28] = 29;
	p[29] = 30;
	p[30] = 31;
	p[31] = 32;
	p[32] = 33;
	p[33] = 34;
	p[34] = 35;
	p[35] = 36;
	p[36] = 37;
	p[37] = 38;
	p[38] = 39;
	p[39] = 40;
	p[40] = 41;
	p[41] = 42;
	p[42] = 43;
	p[43] = 44;
	p[44] = 45;
	p[45] = 46;
	p[46] = 47;
	p[47] = 48;
	p[48] = 49;
	p[49] = 50;
	p[50] = 51;
	p[51] = 52;
	p[52] = 53;
	p[53] = 54;
	p[54] = 55;
	p[55] = 56;
	p[56] = 57;
	p[57] = 58;
	p[58] = 59;
	p[59] = 60;
	p[60] = 61;
	p[61] = 62;
	p[62] = 63;
	p[63] = 64;
	p[64] = 65;
	p[65] = 66;
	p[66] = 67;
	p[67] = 68;
	p[68] = 69;
	p[69] = 70;
	p[70] = 71;
	p[71] = 72;
	p[72] = 73;
	p[73] = 74;
	p[74] = 75;
	p[75] = 76;
	p[76] = 77;
	p[77] = 78;
	p[78] = 79;
	p[79] = 80;
	p[80] = 81;
	p[81] = 82;
	p[82] = 83;
	p[83] = 84;
	p[84] = 85;
	p[85] = 86;
	p[86] = 87;
	p[87] = 88;
	p[88] = 89;
	p[89] = 90;
	p[90] = 91;
	p[91] = 92;
	p[92] = 93;
	p[93] = 94;
	p[94] = 95;
	p[95] = 96;
	p[96] = 97;
	p[97] = 98;
	p[98] = 99;
	p[99] = 100;
	p[100] = 101;
	p[101] = 102;
	p[102] = 103;
	p[103] = 104;
	return ((int *)&gEngfuncs)[1] + ((int *)&gEngfuncs)[2] + ((int *)&gEngfuncs)[3] + ((int *)&gEngfuncs)[4] + ((int *)&gEngfuncs)[5] + ((int *)&gEngfuncs)[6] + ((int *)&gEngfuncs)[7] + ((int *)&gEngfuncs)[8] + ((int *)&gEngfuncs)[9] + ((int *)&gEngfuncs)[10];
}

// key (16, 2)
void Unknown_top_vgui_CustomizationMenu_3( void )
{
	InitInput();
	InitInput();
}

// key (16, 1)
void Unknown_top_vgui_CustomizationMenu_4( void )
{
	InitInput();
}

// sic: never used
char *sCustomizationStats[] = { "accuracy", "speed", "damage", "ammo", "heat" };
char *sCustomizationWeapons[] = { "shotgun", "beamgun", "pistol", "plasma", "tazer", "chainlight", "balllight", "effects" };
char *sCustomizationGauss[] = { "pulse", "charge", "rapid", "sniper" };

// indexed like m_pShotgun
char *sCustomizationShotgun[6] = { "shells", "rifle", "shotgun", "riotgun", "shells", "spread" };

// each row: the title, then the choices
char *sCustomizationDml[4][5] =
{
	{ "launch", "fired", "targetted", "", "" },
	{ "flightpath", "guided", "homing", "spiral", "" },
	{ "trigger", "impact", "proximity", "timed", "tripped" },
	{ "payload", "explosive", "cluster", "", "" },
};

char *sCustomizationBeam[4][5] =
{
	{ "range", "touch_tazer", "short_tazer", "medium_beam", "long_beam" },
	{ "power", "low", "medium", "high", "highest" },
	{ "accuracy", "highest", "high", "medium", "low" },
	{ "lightning", "beam", "chain", "ball", "none" },
};

char *sCustomizationGrenade[2][4] =
{
	{ "trigger", "impact", "timed", "tripped" },
	{ "payload", "explosive", "cluster", "" },
};

char *sCustomizationChem[4] = { "acid", "neut", "base", "pressure" };

static char *memfgets( byte *pMemFile, int fileSize, int &filePos, char *pBuffer, int bufferSize );

CCustomizationMenu::CCustomizationMenu( int iTrans, int iRemoveMe, int x, int y, int wide, int tall ) : CMenuPanel( iTrans, iRemoveMe, x, y, wide, tall )
{
	int r, g, b, a;
	int iXPos = 50;
	int iWide, iTall;
	int iTextWide, iTextTall;
	int iLaunchWide, iLaunchTall;
	int iChemWide, iChemTall;
	// sic: the zero initialisations and pImage below are dead; VC6 numbers
	// locals by first reference, and six more of them numbered before iWide
	// give retail's frame layout and centring order
	int j = 0, k = 0, iChoices;
	int iYPos = 0;
	int iThird = 0;
	char *pszConfig = NULL;
	char szName[256];
	char szText[256];

	r = g = b = a = 255;

	memset( m_pDml, 0, sizeof( m_pDml ) );
	memset( m_pGrenade, 0, sizeof( m_pGrenade ) );
	memset( m_pShotgun, 0, sizeof( m_pShotgun ) );
	memset( m_pChem, 0, sizeof( m_pChem ) );
	memset( m_pBeam, 0, sizeof( m_pBeam ) );

	CSchemeManager *pSchemes = gViewPort->GetSchemeManager();
	SchemeHandle_t hTitleScheme = pSchemes->getSchemeHandle( "Title Font" );
	SchemeHandle_t hBriefingScheme = pSchemes->getSchemeHandle( "Briefing Text" );
	SchemeHandle_t hShotgunSelected = pSchemes->getSchemeHandle( "Shotgun Selected Text" );
	SchemeHandle_t hDmlChoice = pSchemes->getSchemeHandle( "Dml Sel" );
	SchemeHandle_t hDmlTitle = pSchemes->getSchemeHandle( "Dml Sel" );	// sic: not "Dml Title Text"
	SchemeHandle_t hShotgunText = pSchemes->getSchemeHandle( "Shotgun Text" );
	SchemeHandle_t hDmlText = pSchemes->getSchemeHandle( "Dml Text" );

	//
	// shotgun: shells and spread
	//
	sprintf( szName, "shells_config" );
	GetConfigText( szName, szText );
	m_pShotgun[4] = new TextPanel( szText, iXPos, 50, 200, 200 );
	m_pShotgun[4]->setParent( this );
	m_pShotgun[4]->setFont( pSchemes->getFont( hShotgunSelected ) );
	pSchemes->getFgColor( hShotgunSelected, r, g, b, a );
	m_pShotgun[4]->setFgColor( r, g, b, a );
	pSchemes->getBgColor( hShotgunSelected, r, g, b, a );
	m_pShotgun[4]->setBgColor( r, g, b, a );
	TextImage *pImage = m_pShotgun[4]->getTextImage();	// sic
	pImage->getTextSizeWrapped( iWide, iTall );
	m_pShotgun[4]->setSize( iWide, iTall );
	m_pShotgun[4]->setVisible( true );
	iXPos = ScreenWidth / 2 - ( iWide / 2 + iWide / 12 );
	m_pShotgun[4]->setPos( iXPos, 63 );
	m_pShotgun[4]->setFont( pSchemes->getFont( hDmlText ) );
	pSchemes->getFgColor( hDmlText, r, g, b, a );
	m_pShotgun[4]->setFgColor( r, g, b, a );
	pSchemes->getBgColor( hDmlText, r, g, b, a );
	m_pShotgun[4]->setBgColor( r, g, b, a );

	// sic: the number of shells shows its title until ShowShotgun sets it
	sprintf( szName, "shells_config" );
	GetConfigText( szName, szText );
	m_pShotgun[0] = new TextPanel( szText, iWide + iXPos + 20, 63, 200, 200 );
	m_pShotgun[0]->setParent( this );
	m_pShotgun[0]->setFont( pSchemes->getFont( hShotgunText ) );
	pSchemes->getFgColor( hShotgunText, r, g, b, a );
	m_pShotgun[0]->setFgColor( r, g, b, a );
	pSchemes->getBgColor( hShotgunText, r, g, b, a );
	m_pShotgun[0]->setBgColor( r, g, b, a );
	m_pShotgun[0]->getTextImage()->getTextSizeWrapped( iTextWide, iTextTall );
	m_pShotgun[0]->setSize( iTextWide, iTextTall );
	m_pShotgun[0]->setVisible( true );

	sprintf( szName, "spread_config" );
	GetConfigText( szName, szText );
	m_pShotgun[5] = new TextPanel( szText, iXPos, iTall + iTall / 3 + 63, 200, 200 );
	m_pShotgun[5]->setParent( this );
	m_pShotgun[5]->setFont( pSchemes->getFont( hShotgunSelected ) );
	pSchemes->getFgColor( hShotgunSelected, r, g, b, a );
	m_pShotgun[5]->setFgColor( r, g, b, a );
	pSchemes->getBgColor( hShotgunSelected, r, g, b, a );
	m_pShotgun[5]->setBgColor( r, g, b, a );
	m_pShotgun[5]->getTextImage()->getTextSizeWrapped( iTextWide, iTextTall );
	m_pShotgun[5]->setSize( iTextWide, iTextTall );
	m_pShotgun[5]->setVisible( true );
	m_pShotgun[5]->setFont( pSchemes->getFont( hDmlChoice ) );
	pSchemes->getFgColor( hDmlChoice, r, g, b, a );
	m_pShotgun[5]->setFgColor( r, g, b, a );
	pSchemes->getBgColor( hDmlChoice, r, g, b, a );
	m_pShotgun[5]->setBgColor( r, g, b, a );

	for ( j = 1; j <= 3; j++ )
	{
		sprintf( szName, "%s_config", sCustomizationShotgun[j] );
		GetConfigText( szName, szText );
		m_pShotgun[j] = new TextPanel( szText, iWide + iXPos + 20, iTall + iTall / 3 + 63, 200, 200 );
		m_pShotgun[j]->setParent( this );
		m_pShotgun[j]->setFont( pSchemes->getFont( hShotgunSelected ) );
		pSchemes->getFgColor( hShotgunSelected, r, g, b, a );
		m_pShotgun[j]->setFgColor( r, g, b, a );
		pSchemes->getBgColor( hShotgunSelected, r, g, b, a );
		m_pShotgun[j]->setBgColor( r, g, b, a );
		m_pShotgun[j]->getTextImage()->getTextSizeWrapped( iTextWide, iTextTall );
		m_pShotgun[j]->setSize( iTextWide, iTextTall );
		m_pShotgun[j]->setVisible( true );
	}

	//
	// DML: launch, flightpath, detonate and payload
	//
	sprintf( szName, "launched_config" );
	pszConfig = GetConfigText( szName, szText );
	if ( !pszConfig )
		pszConfig = "No Configuration Available";
	m_pDml[0][0] = new TextPanel( pszConfig, 0, 0, 100, 50 );
	m_pDml[0][0]->setParent( this );
	m_pDml[0][0]->setFont( pSchemes->getFont( hDmlChoice ) );
	pSchemes->getFgColor( hDmlChoice, r, g, b, a );
	m_pDml[0][0]->setFgColor( r, g, b, a );
	pSchemes->getBgColor( hDmlChoice, r, g, b, a );
	m_pDml[0][0]->setBgColor( r, g, b, a );
	m_pDml[0][0]->getTextImage()->getTextSizeWrapped( iLaunchWide, iLaunchTall );
	m_pDml[0][0]->setSize( iLaunchWide, iLaunchTall );
	m_pDml[0][0]->setVisible( false );

	iXPos = ScreenWidth / 2 - iLaunchWide;
	for ( j = 0; j < 4; j++ )
	{
		switch ( j )
		{
		case 0:
			iChoices = 2;
			break;
		case 1:
			iChoices = 3;
			break;
		case 2:
			iChoices = 4;
			break;
		case 3:
			iChoices = 2;
			break;
		}

		// sic: the first title replaces the panel made above
		sprintf( szName, "%s_config", sCustomizationDml[j][0] );
		pszConfig = GetConfigText( szName, szText );
		if ( !pszConfig )
			pszConfig = "No Configuration Available";
		m_pDml[j][0] = new TextPanel( pszConfig, iXPos, iLaunchTall * j / 3 + 47, 200, 200 );
		m_pDml[j][0]->setParent( this );
		m_pDml[j][0]->setFont( pSchemes->getFont( hDmlTitle ) );
		pSchemes->getFgColor( hDmlTitle, r, g, b, a );
		m_pDml[j][0]->setFgColor( r, g, b, a );
		pSchemes->getBgColor( hDmlTitle, r, g, b, a );
		m_pDml[j][0]->setBgColor( r, g, b, a );
		m_pDml[j][0]->getTextImage()->getTextSizeWrapped( iTextWide, iTextTall );
		m_pDml[j][0]->setSize( iTextWide, iTextTall );
		m_pDml[j][0]->setVisible( false );

		for ( k = 1; k <= iChoices; k++ )
		{
			sprintf( szName, "%s_config", sCustomizationDml[j][k] );
			pszConfig = GetConfigText( szName, szText );
			if ( !pszConfig )
				pszConfig = "No Configuration Available";
			m_pDml[j][k] = new TextPanel( pszConfig, iXPos + iLaunchWide + iLaunchWide / 3, iLaunchTall * j / 3 + 47, 300, 200 );
			m_pDml[j][k]->setParent( this );
			m_pDml[j][k]->setFont( pSchemes->getFont( hDmlChoice ) );
			pSchemes->getFgColor( hDmlChoice, r, g, b, a );
			m_pDml[j][k]->setFgColor( r, g, b, a );
			pSchemes->getBgColor( hDmlChoice, r, g, b, a );
			m_pDml[j][k]->setBgColor( r, g, b, a );
			m_pDml[j][k]->getTextImage()->getTextSizeWrapped( iTextWide, iTextTall );
			m_pDml[j][k]->setSize( iTextWide, iTextTall );
			m_pDml[j][k]->setVisible( false );
		}
	}

	//
	// DML grenade: detonate and payload
	//
	sprintf( szName, "launched_config" );
	pszConfig = GetConfigText( szName, szText );
	if ( !pszConfig )
		pszConfig = "No Configuration Available";
	m_pGrenade[0][0] = new TextPanel( pszConfig, 0, 0, 100, 50 );
	m_pGrenade[0][0]->setParent( this );
	m_pGrenade[0][0]->setFont( pSchemes->getFont( hDmlChoice ) );
	pSchemes->getFgColor( hDmlChoice, r, g, b, a );
	m_pGrenade[0][0]->setFgColor( r, g, b, a );
	pSchemes->getBgColor( hDmlChoice, r, g, b, a );
	m_pGrenade[0][0]->setBgColor( r, g, b, a );
	m_pGrenade[0][0]->getTextImage()->getTextSizeWrapped( iLaunchWide, iLaunchTall );
	m_pGrenade[0][0]->setSize( iLaunchWide, iLaunchTall );
	m_pGrenade[0][0]->setVisible( false );

	iXPos = ScreenWidth / 2 - iLaunchWide;
	for ( j = 0; j < 2; j++ )
	{
		// sic: the first title replaces the panel made above
		sprintf( szName, "%s_config", sCustomizationGrenade[j][0] );
		pszConfig = GetConfigText( szName, szText );
		if ( !pszConfig )
			pszConfig = "No Configuration Available";
		m_pGrenade[j][0] = new TextPanel( pszConfig, iXPos, iLaunchTall * j / 3 + 47, 200, 200 );
		m_pGrenade[j][0]->setParent( this );
		m_pGrenade[j][0]->setFont( pSchemes->getFont( hDmlTitle ) );
		pSchemes->getFgColor( hDmlTitle, r, g, b, a );
		m_pGrenade[j][0]->setFgColor( r, g, b, a );
		pSchemes->getBgColor( hDmlTitle, r, g, b, a );
		m_pGrenade[j][0]->setBgColor( r, g, b, a );
		m_pGrenade[j][0]->getTextImage()->getTextSizeWrapped( iTextWide, iTextTall );
		m_pGrenade[j][0]->setSize( iTextWide, iTextTall );
		m_pGrenade[j][0]->setVisible( false );

		for ( k = 1; k < 4; k++ )
		{
			sprintf( szName, "%s_config", sCustomizationGrenade[j][k] );
			pszConfig = GetConfigText( szName, szText );
			if ( !pszConfig )
				pszConfig = "No Configuration Available";
			m_pGrenade[j][k] = new TextPanel( pszConfig, iXPos + iLaunchWide + iLaunchWide / 3, iLaunchTall * j / 3 + 47, 300, 200 );
			m_pGrenade[j][k]->setParent( this );
			m_pGrenade[j][k]->setFont( pSchemes->getFont( hDmlChoice ) );
			pSchemes->getFgColor( hDmlChoice, r, g, b, a );
			m_pGrenade[j][k]->setFgColor( r, g, b, a );
			pSchemes->getBgColor( hDmlChoice, r, g, b, a );
			m_pGrenade[j][k]->setBgColor( r, g, b, a );
			m_pGrenade[j][k]->getTextImage()->getTextSizeWrapped( iTextWide, iTextTall );
			m_pGrenade[j][k]->setSize( iTextWide, iTextTall );
			m_pGrenade[j][k]->setVisible( false );
		}
	}

	//
	// chemical gun: acid, neutralizer, base and pressure
	//
	sprintf( szName, "%s_config", sCustomizationChem[3] );
	pszConfig = GetConfigText( szName, szText );
	if ( !pszConfig )
		pszConfig = "No Configuration Available";
	m_pChem[0] = new TextPanel( pszConfig, iXPos, 47, 300, 200 );
	m_pChem[0]->setParent( this );
	m_pChem[0]->setFont( pSchemes->getFont( hShotgunSelected ) );
	pSchemes->getFgColor( hShotgunSelected, r, g, b, a );
	m_pChem[0]->setFgColor( r, g, b, a );
	pSchemes->getBgColor( hShotgunSelected, r, g, b, a );
	m_pChem[0]->setBgColor( r, g, b, a );
	m_pChem[0]->getTextImage()->getTextSizeWrapped( iChemWide, iChemTall );
	m_pChem[0]->setSize( iChemWide, iChemTall );
	m_pChem[0]->setVisible( false );

	iXPos = ScreenWidth / 2 - 200;
	if ( ScreenWidth == 640 )
		iXPos += 75;
	else if ( ScreenWidth < 640 )
		iXPos += 100;
	if ( ScreenWidth < 512 )
		iXPos += 25;

	// sic: the first replaces the panel made above
	for ( j = 0; j < 4; j++ )
	{
		sprintf( szName, "%s_config", sCustomizationChem[j] );
		pszConfig = GetConfigText( szName, szText );
		if ( !pszConfig )
			pszConfig = "No Configuration Available";
		m_pChem[j] = new TextPanel( pszConfig, iXPos, ( iChemTall + 7 ) * j + 54, 300, 200 );
		m_pChem[j]->setParent( this );
		m_pChem[j]->setFont( pSchemes->getFont( hShotgunSelected ) );
		pSchemes->getFgColor( hShotgunSelected, r, g, b, a );
		m_pChem[j]->setFgColor( r, g, b, a );
		pSchemes->getBgColor( hShotgunSelected, r, g, b, a );
		m_pChem[j]->setBgColor( r, g, b, a );
		m_pChem[j]->getTextImage()->getTextSizeWrapped( iTextWide, iTextTall );
		m_pChem[j]->setSize( iTextWide, iTextTall );
		m_pChem[j]->setVisible( false );
	}

	//
	// beam gun: range, power, accuracy and effect
	//
	sprintf( szName, "launched_config" );
	pszConfig = GetConfigText( szName, szText );
	if ( !pszConfig )
		pszConfig = "No Configuration Available";
	m_pBeam[0][0] = new TextPanel( pszConfig, 0, 0, 100, 50 );
	m_pBeam[0][0]->setParent( this );
	m_pBeam[0][0]->setFont( pSchemes->getFont( hDmlChoice ) );
	pSchemes->getFgColor( hDmlChoice, r, g, b, a );
	m_pBeam[0][0]->setFgColor( r, g, b, a );
	pSchemes->getBgColor( hDmlChoice, r, g, b, a );
	m_pBeam[0][0]->setBgColor( r, g, b, a );
	m_pBeam[0][0]->getTextImage()->getTextSizeWrapped( iLaunchWide, iLaunchTall );
	m_pBeam[0][0]->setSize( iLaunchWide, iLaunchTall );
	m_pBeam[0][0]->setVisible( false );

	iXPos = ScreenWidth / 2 - iLaunchWide;
	for ( j = 0; j < 4; j++ )
	{
		// sic: the first title replaces the panel made above
		sprintf( szName, "%s_config", sCustomizationBeam[j][0] );
		pszConfig = GetConfigText( szName, szText );
		if ( !pszConfig )
			pszConfig = "No Configuration Available";

		iYPos = iLaunchTall * j / 3 + 47;
		if ( j == 2 )
			iYPos = iLaunchTall / 3 + iLaunchTall / 4 + 47;
		else if ( j == 3 )
		{
			// sic: the third in its own local gives retail's registers
			iThird = iLaunchTall / 3;
			iYPos = iThird * 2 + iLaunchTall / 4 + 47;
		}

		m_pBeam[j][0] = new TextPanel( pszConfig, iXPos, iYPos, 200, 200 );
		m_pBeam[j][0]->setParent( this );
		m_pBeam[j][0]->setFont( pSchemes->getFont( hDmlTitle ) );
		pSchemes->getFgColor( hDmlTitle, r, g, b, a );
		m_pBeam[j][0]->setFgColor( r, g, b, a );
		pSchemes->getBgColor( hDmlTitle, r, g, b, a );
		m_pBeam[j][0]->setBgColor( r, g, b, a );
		m_pBeam[j][0]->getTextImage()->getTextSizeWrapped( iTextWide, iTextTall );
		m_pBeam[j][0]->setSize( iTextWide, iTextTall );
		m_pBeam[j][0]->setVisible( false );

		for ( k = 1; k < 5; k++ )
		{
			sprintf( szName, "%s_config", sCustomizationBeam[j][k] );
			pszConfig = GetConfigText( szName, szText );
			if ( !pszConfig )
				pszConfig = "No Configuration Available";
			m_pBeam[j][k] = new TextPanel( pszConfig, iXPos + iLaunchWide + iLaunchWide / 3, iYPos, 300, 200 );
			m_pBeam[j][k]->setParent( this );
			m_pBeam[j][k]->setFont( pSchemes->getFont( hDmlChoice ) );
			pSchemes->getFgColor( hDmlChoice, r, g, b, a );
			m_pBeam[j][k]->setFgColor( r, g, b, a );
			pSchemes->getBgColor( hDmlChoice, r, g, b, a );
			m_pBeam[j][k]->setBgColor( r, g, b, a );
			m_pBeam[j][k]->getTextImage()->getTextSizeWrapped( iTextWide, iTextTall );
			m_pBeam[j][k]->setSize( iTextWide, iTextTall );
			m_pBeam[j][k]->setVisible( false );
		}
	}

	setVisible( true );
	m_iStartFrames = 1;
}

// Hide with the weapons HUD, and on the second frame after creation
void CCustomizationMenu::paint( void )
{
	if ( gHUD.m_iHideHUDDisplay & ( HIDEHUD_WEAPONS | HIDEHUD_ALL ) )
	{
		setVisible( false );
		return;
	}

	if ( m_iStartFrames == 2 )
	{
		m_iStartFrames = 0;
		setVisible( false );
		HideAll();
	}
	else if ( m_iStartFrames == 1 )
	{
		m_iStartFrames = 2;
	}
}

char *GetConfigText( const char *pszName, char *pszText )
{
	char szKey[512];
	char szLine[512];
	int fileSize, filePos;
	int i;

	sprintf( szKey, "#%s", pszName );
	memset( szLine, 0, sizeof( szLine ) );
	filePos = 0;

	// sic: never freed
	byte *pMemFile = gEngfuncs.COM_LoadFile( "text/configtext.txt", 5, &fileSize );
	if ( !pMemFile )
		return NULL;

	int bFound = false;
	while ( memfgets( pMemFile, fileSize, filePos, szLine, 511 ) != NULL )
	{
		// the line after the key is the text
		if ( bFound )
		{
			strcpy( pszText, szLine );
			return pszText;
		}

		// cut the line after the key
		for ( i = 1; i < 512; i++ )
		{
			if ( !isalpha( szLine[i] ) && szLine[i] != '_' )
			{
				szLine[i] = 0;
				break;
			}
		}

		if ( !strcmp( szKey, szLine ) )
			bFound = true;
	}

	return NULL;
}

// As sound.cpp's
static char *memfgets( byte *pMemFile, int fileSize, int &filePos, char *pBuffer, int bufferSize )
{
	// Bullet-proofing
	if ( !pMemFile || !pBuffer )
		return NULL;

	if ( filePos >= fileSize )
		return NULL;

	int i = filePos;
	int last = fileSize;

	// fgets always NULL terminates, so only read bufferSize-1 characters
	if ( last - filePos > (bufferSize-1) )
		last = filePos + (bufferSize-1);

	int stop = 0;

	// Stop at the next newline (inclusive) or end of buffer
	while ( i < last && !stop )
	{
		if ( pMemFile[i] == '\n' )
			stop = 1;
		i++;
	}

	// If we actually advanced the pointer, copy it over
	if ( i != filePos )
	{
		// We read in size bytes
		int size = i - filePos;
		// copy it out
		memcpy( pBuffer, pMemFile + filePos, sizeof(byte)*size );

		// If the buffer isn't full, terminate (this is always true)
		if ( size < bufferSize )
			pBuffer[size] = 0;

		// Update file pointer
		filePos = i;
		return pBuffer;
	}

	// No data read, bail
	return NULL;
}

// iWeapon is the customization item in hand: 4 the shells, else the spread
void CCustomizationMenu::ShowShotgun( int iShells, int iSpread, int iWeapon )
{
	char sz[256];
	int r, g, b, a;

	HideAll();

	CSchemeManager *pSchemes = gViewPort->GetSchemeManager();
	SchemeHandle_t hText = pSchemes->getSchemeHandle( "Shotgun Text" );
	SchemeHandle_t hSelected = pSchemes->getSchemeHandle( "Shotgun Selected Text" );

	if ( iWeapon == 4 )
	{
		pSchemes->getFgColor( hSelected, r, g, b, a );
		m_pShotgun[4]->setFont( pSchemes->getFont( hSelected ) );
		m_pShotgun[4]->setFgColor( r, g, b, a );
		m_pShotgun[0]->setFont( pSchemes->getFont( hSelected ) );
		m_pShotgun[0]->setFgColor( r, g, b, a );
		pSchemes->getFgColor( hText, r, g, b, a );
		m_pShotgun[5]->setFont( pSchemes->getFont( hText ) );
		m_pShotgun[5]->setFgColor( r, g, b, a );
	}
	else
	{
		pSchemes->getFgColor( hSelected, r, g, b, a );
		m_pShotgun[5]->setFont( pSchemes->getFont( hSelected ) );
		m_pShotgun[5]->setFgColor( r, g, b, a );
		pSchemes->getFgColor( hText, r, g, b, a );
		m_pShotgun[4]->setFont( pSchemes->getFont( hText ) );
		m_pShotgun[4]->setFgColor( r, g, b, a );
		m_pShotgun[0]->setFont( pSchemes->getFont( hText ) );
		m_pShotgun[0]->setFgColor( r, g, b, a );
	}

	sprintf( sz, "%d", iShells );
	m_pShotgun[0]->setText( sz );
	m_pShotgun[4]->setVisible( true );
	m_pShotgun[0]->setVisible( true );
	m_pShotgun[5]->setVisible( true );

	// only the current spread
	for ( int i = 1; i <= 3; i++ )
	{
		if ( i != iSpread )
		{
			m_pShotgun[i]->setVisible( false );
		}
		else
		{
			SchemeHandle_t hScheme = hText;
			if ( iWeapon != 4 )
				hScheme = hSelected;

			pSchemes->getFgColor( hScheme, r, g, b, a );
			m_pShotgun[i]->setFont( pSchemes->getFont( hScheme ) );
			m_pShotgun[i]->setFgColor( r, g, b, a );
			m_pShotgun[i]->setVisible( true );
		}
	}

	setVisible( true );
}

void CCustomizationMenu::HideAll( void )
{
	int i, j;

	for ( i = 0; i < 4; i++ )
	{
		for ( j = 0; j < 5; j++ )
		{
			if ( m_pDml[i][j] )
				m_pDml[i][j]->setVisible( false );
		}
	}

	for ( i = 0; i < 4; i++ )
	{
		for ( j = 0; j < 5; j++ )
		{
			if ( m_pBeam[i][j] )
				m_pBeam[i][j]->setVisible( false );
		}
	}

	for ( i = 0; i < 2; i++ )
	{
		for ( j = 0; j < 4; j++ )
		{
			if ( m_pGrenade[i][j] )
				m_pGrenade[i][j]->setVisible( false );
		}
	}

	for ( i = 0; i < 6; i++ )
	{
		if ( m_pShotgun[i] )
			m_pShotgun[i]->setVisible( false );
	}

	for ( i = 0; i < 4; i++ )
	{
		if ( m_pChem[i] )
			m_pChem[i]->setVisible( false );
	}

	setVisible( false );
}

// iWeapon is the customization item in hand, 9 to 12 (cust_1DMLLaunch to
// cust_4DMLPayload); its row is highlighted
void CCustomizationMenu::ShowDml( int iLaunch, int iFlightpath, int iDetonate, int iPayload, int iWeapon )
{
	int r, g, b, a;

	CSchemeManager *pSchemes = gViewPort->GetSchemeManager();
	SchemeHandle_t hText = pSchemes->getSchemeHandle( "Dml Text" );
	SchemeHandle_t hTitle = pSchemes->getSchemeHandle( "Dml Title Text" );
	SchemeHandle_t hChoice = pSchemes->getSchemeHandle( "Dml Sel" );
	SchemeHandle_t hSelected = pSchemes->getSchemeHandle( "Dml Sel" );

	int iRow = iWeapon - 9;

	HideAll();

	for ( int i = 0; i < 4; i++ )
	{
		int iCurrent;

		switch ( i )
		{
		case 0:
			iCurrent = iLaunch;
			break;
		case 1:
			iCurrent = iFlightpath;
			break;
		case 2:
			iCurrent = iDetonate;
			break;
		case 3:
			iCurrent = iPayload;
			break;
		}

		if ( !m_pDml[i][0] )
			continue;

		SchemeHandle_t hScheme = ( iRow == i ) ? hSelected : hTitle;
		m_pDml[i][0]->setFont( pSchemes->getFont( hScheme ) );
		pSchemes->getFgColor( hScheme, r, g, b, a );
		m_pDml[i][0]->setFgColor( r, g, b, a );
		m_pDml[i][0]->setVisible( true );

		// only the current choice
		for ( int k = 1; k < 5; k++ )
		{
			if ( !m_pDml[i][k] )
				continue;

			if ( k == iCurrent )
			{
				hScheme = ( iRow == i ) ? hChoice : hText;
				m_pDml[i][k]->setFont( pSchemes->getFont( hScheme ) );
				pSchemes->getFgColor( hScheme, r, g, b, a );
				m_pDml[i][k]->setFgColor( r, g, b, a );
				m_pDml[i][k]->setVisible( true );
			}
			else
			{
				m_pDml[i][k]->setVisible( false );
			}
		}
	}

	setVisible( true );
}

// iRow is the customization item in hand: 0 frequency (the range), 1 current
// (the power and the accuracy), 2 voltage (the effect)
void CCustomizationMenu::ShowBeamgun( int iFrequency, int iCurrent, int iVoltage, int iRow )
{
	int r, g, b, a;

	CSchemeManager *pSchemes = gViewPort->GetSchemeManager();
	SchemeHandle_t hText = pSchemes->getSchemeHandle( "Dml Text" );	// sic: never used
	SchemeHandle_t hTitle = pSchemes->getSchemeHandle( "Dml Title Text" );
	SchemeHandle_t hChoice = pSchemes->getSchemeHandle( "Dml Sel" );
	SchemeHandle_t hSelected = pSchemes->getSchemeHandle( "Dml Sel" );

	int iChoice;

	HideAll();

	for ( int i = 0; i < 4; i++ )
	{
		switch ( i )
		{
		case 0:
			iChoice = iFrequency;
			break;
		case 1:		// power
			iChoice = iCurrent;
			break;
		case 2:		// accuracy
			iChoice = iCurrent;
			break;
		case 3:
			iChoice = iVoltage;
			break;
		}

		if ( !m_pBeam[i][0] )
			continue;

		SchemeHandle_t hScheme;
		if ( iRow == 2 )
		{
			// the voltage shows on the effect row
			if ( i == 3 )
				hScheme = hSelected;
			else
				hScheme = hTitle;
		}
		else
		{
			if ( iRow == i )
				hScheme = hSelected;
			else
				hScheme = hTitle;

			// the current shows on the power and the accuracy rows
			if ( iRow == 1 && i == 2 )
				hScheme = hChoice;
		}

		m_pBeam[i][0]->setFont( pSchemes->getFont( hScheme ) );
		pSchemes->getFgColor( hScheme, r, g, b, a );
		m_pBeam[i][0]->setFgColor( r, g, b, a );
		m_pBeam[i][0]->setVisible( true );

		// only the current choice, like its title
		for ( int k = 1; k < 5; k++ )
		{
			if ( !m_pBeam[i][k] )
				continue;

			if ( k == iChoice )
			{
				m_pBeam[i][k]->setFont( pSchemes->getFont( hScheme ) );
				pSchemes->getFgColor( hScheme, r, g, b, a );
				m_pBeam[i][k]->setFgColor( r, g, b, a );
				m_pBeam[i][k]->setVisible( true );
			}
			else
			{
				m_pBeam[i][k]->setVisible( false );
			}
		}
	}

	setVisible( true );
}

// iWeapon is the customization item in hand, 5 or 6 (cust_1DMLGrenDetonate,
// cust_2DMLGrenPayload); its row is highlighted
void CCustomizationMenu::ShowGrenade( int iDetonate, int iPayload, int iWeapon )
{
	int r, g, b, a;

	CSchemeManager *pSchemes = gViewPort->GetSchemeManager();
	SchemeHandle_t hText = pSchemes->getSchemeHandle( "Dml Text" );
	SchemeHandle_t hTitle = pSchemes->getSchemeHandle( "Dml Title Text" );
	SchemeHandle_t hChoice = pSchemes->getSchemeHandle( "Dml Sel" );
	SchemeHandle_t hSelected = pSchemes->getSchemeHandle( "Dml Sel" );

	int iRow = iWeapon - 5;

	HideAll();

	for ( int i = 0; i < 2; i++ )
	{
		int iCurrent;

		if ( i == 0 )
			iCurrent = iDetonate;
		else
			iCurrent = iPayload;

		if ( !m_pGrenade[i][0] )
			continue;

		SchemeHandle_t hScheme = ( iRow == i ) ? hSelected : hTitle;
		m_pGrenade[i][0]->setFont( pSchemes->getFont( hScheme ) );
		pSchemes->getFgColor( hScheme, r, g, b, a );
		m_pGrenade[i][0]->setFgColor( r, g, b, a );
		m_pGrenade[i][0]->setVisible( true );

		int iChoices = 2;
		if ( i == 0 )
			iChoices = 3;

		// only the current choice
		for ( int k = 1; k <= iChoices; k++ )
		{
			if ( !m_pGrenade[i][k] )
				continue;

			if ( k == iCurrent )
			{
				hScheme = ( iRow == i ) ? hChoice : hText;
				m_pGrenade[i][k]->setFont( pSchemes->getFont( hScheme ) );
				pSchemes->getFgColor( hScheme, r, g, b, a );
				m_pGrenade[i][k]->setFgColor( r, g, b, a );
				m_pGrenade[i][k]->setVisible( true );
			}
			else
			{
				m_pGrenade[i][k]->setVisible( false );
			}
		}
	}

	setVisible( true );
}

// All four, the selected one highlighted
void CCustomizationMenu::ShowChemgun( int iSelected )
{
	int r, g, b, a;

	CSchemeManager *pSchemes = gViewPort->GetSchemeManager();
	SchemeHandle_t hText = pSchemes->getSchemeHandle( "Shotgun Text" );
	SchemeHandle_t hSelected = pSchemes->getSchemeHandle( "Shotgun Selected Text" );

	HideAll();

	for ( int i = 0; i < 4; i++ )
	{
		SchemeHandle_t hScheme = ( iSelected == i ) ? hSelected : hText;

		if ( m_pChem[i] )
		{
			m_pChem[i]->setFont( pSchemes->getFont( hScheme ) );
			pSchemes->getFgColor( hScheme, r, g, b, a );
			m_pChem[i]->setFgColor( r, g, b, a );
			m_pChem[i]->setVisible( true );
		}
	}

	setVisible( true );
}

void CCustomizationMenu::Initialize( void )
{
	setVisible( false );
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

void CCustomizationMenu::Update( void )
{
}

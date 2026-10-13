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
// vgui_Temperature.cpp
//
// the temperature gauge in hot places, and its warning texts
//

#include "VGUI_Font.h"
#include "VGUI_TextImage.h"

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
#include "vgui_Temperature.h"

CTemperaturePanel::CTemperaturePanel( int x, int y, int wide, int tall ) : CMenuPanel( false, x, y, wide, tall )
{
	char sz[256];
	char *cText;
	char *pfile;

	// the gauge
	BitmapTGA *pTGA = LoadTGA( "temperature" );
	m_pGauge = new CImageLabel( pTGA, 0, 25, 256, 128 );
	m_pGauge->setParent( this );
	m_pGauge->setVisible( true );

	// Get the scheme used for the texts
	CSchemeManager *pSchemes = gViewPort->GetSchemeManager();
	int r, g, b, a;
	r = g = b = a = 255;
	SchemeHandle_t hTempText = pSchemes->getSchemeHandle( "Temp Text" );
	SchemeHandle_t hTempDanger = pSchemes->getSchemeHandle( "Temp Danger" );

	// the warning
	sprintf( sz, "text/warning.txt" );
	cText = "Class Description not available.";
	pfile = (char*)gEngfuncs.COM_LoadFile( sz, 5, NULL );
	if ( pfile )
		cText = pfile;

	m_pWarning = new TextPanel( cText, 20, 0, 200, 200 );
	m_pWarning->setParent( this );
	m_pWarning->setFont( pSchemes->getFont( hTempText ) );
	pSchemes->getFgColor( hTempText, r, g, b, a );
	m_pWarning->setFgColor( r, g, b, a );
	pSchemes->getBgColor( hTempText, r, g, b, a );
	m_pWarning->setBgColor( r, g, b, a );

	int iWide, iTall;
	m_pWarning->getTextImage()->getTextSizeWrapped( iWide, iTall );
	m_pWarning->setSize( iWide, iTall );
	m_pWarning->setVisible( false );

	// the danger
	sprintf( sz, "text/danger.txt" );
	cText = "Class Description not available.";
	pfile = (char*)gEngfuncs.COM_LoadFile( sz, 5, NULL );
	if ( pfile )
		cText = pfile;

	m_pDanger = new TextPanel( cText, 25, 0, 200, 200 );
	m_pDanger->setParent( this );
	m_pDanger->setFont( pSchemes->getFont( hTempText ) );
	pSchemes->getFgColor( hTempDanger, r, g, b, a );
	m_pDanger->setFgColor( r, g, b, a );
	pSchemes->getBgColor( hTempDanger, r, g, b, a );
	m_pDanger->setBgColor( r, g, b, a );

	m_pDanger->getTextImage()->getTextSizeWrapped( iWide, iTall );
	m_pDanger->setSize( iWide, iTall );
	m_pDanger->setVisible( false );

	// and the malfunction
	sprintf( sz, "text/malfunction.txt" );
	cText = "Class Description not available.";
	pfile = (char*)gEngfuncs.COM_LoadFile( sz, 5, NULL );
	if ( pfile )
		cText = pfile;

	m_pMalfunction = new TextPanel( cText, 0, 0, 200, 200 );
	m_pMalfunction->setParent( this );
	m_pMalfunction->setFont( pSchemes->getFont( hTempText ) );
	pSchemes->getFgColor( hTempDanger, r, g, b, a );
	m_pMalfunction->setFgColor( r, g, b, a );
	pSchemes->getBgColor( hTempDanger, r, g, b, a );
	m_pMalfunction->setBgColor( r, g, b, a );

	m_pMalfunction->getTextImage()->getTextSizeWrapped( iWide, iTall );
	m_pMalfunction->setSize( iWide, iTall );
	m_pMalfunction->setVisible( false );
}

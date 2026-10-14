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
// vgui_Temperature.h
//
// the temperature gauge in hot places, and its warning texts
//

#ifndef VGUI_TEMPERATURE_H
#define VGUI_TEMPERATURE_H

class CTemperaturePanel : public CMenuPanel
{
public:
	CImageLabel	*m_pGauge;
	TextPanel	*m_pWarning;
	TextPanel	*m_pDanger;
	TextPanel	*m_pMalfunction;

public:
	CTemperaturePanel( int x, int y, int wide, int tall );

	// the health HUD shows one text by the heat, or none
	void ShowTexts( bool bWarning, bool bDanger, bool bMalfunction )
	{
		m_pWarning->setVisible( bWarning );
		m_pDanger->setVisible( bDanger );
		m_pMalfunction->setVisible( bMalfunction );
	}

	virtual void paintBackground( void )
	{
	}

	virtual void paint( void )
	{
	}
};

#endif // VGUI_TEMPERATURE_H

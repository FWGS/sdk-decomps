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
// vgui_HudID.h
//
// the name of the player in the crosshair (STUB: being reconstructed)
//

#ifndef VGUI_HUDID_H
#define VGUI_HUDID_H

class CIDPanel : public CMenuPanel
{
public:
	TextPanel	*m_pText;
	int			m_iUnknown;

public:
	CIDPanel( int iTrans, int iRemoveMe, int x, int y, int wide, int tall );
};

#endif // VGUI_HUDID_H

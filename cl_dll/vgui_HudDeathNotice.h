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
// vgui_HudDeathNotice.h
//
// the death notices as a VGUI panel (STUB: being reconstructed)
//

#ifndef VGUI_HUDDEATHNOTICE_H
#define VGUI_HUDDEATHNOTICE_H

class CDeathNoticePanel : public CMenuPanel
{
public:
	int		m_iData[34];

public:
	CDeathNoticePanel( int iTrans, int iRemoveMe, int x, int y, int wide, int tall );

	virtual void Reset( void );
	virtual void Open( void );
	virtual void Initialize( void );
	virtual void Update( void );
};

#endif // VGUI_HUDDEATHNOTICE_H

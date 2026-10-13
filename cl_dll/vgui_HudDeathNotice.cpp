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
// vgui_HudDeathNotice.cpp
//
// the death notices as a VGUI panel (STUB: being reconstructed)
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
#include "vgui_HudDeathNotice.h"

CDeathNoticePanel::CDeathNoticePanel( int iTrans, int iRemoveMe, int x, int y, int wide, int tall ) : CMenuPanel( iTrans, iRemoveMe, x, y, wide, tall )
{
}

void CDeathNoticePanel::Reset( void )
{
	CMenuPanel::Reset();
}

void CDeathNoticePanel::Open( void )
{
	Update();
	CMenuPanel::Open();
}

void CDeathNoticePanel::Initialize( void )
{
}

void CDeathNoticePanel::Update( void )
{
}

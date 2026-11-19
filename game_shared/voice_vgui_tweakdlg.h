//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef VOICE_VGUI_TWEAKDLG_H
#define VOICE_VGUI_TWEAKDLG_H
#ifdef _WIN32
#pragma once
#endif


class CMenuPanel;


// Returns true if the tweak dialog is currently up.
bool IsTweakDlgOpen();

// Returns a global instance of the tweak dialog.
CMenuPanel* GetVoiceTweakDlg();


// sic: a stand-in for a declaration that is not known (see hud.h's): with it
// CVoiceVGUITweakDlg::Open gets retail's stack slots
enum { TWEAK_SIC_1 };


#endif // VOICE_VGUI_TWEAKDLG_H

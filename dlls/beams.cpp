/***
*
*	Copyright (c) 1996-2001, Valve LLC. All rights reserved.
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
// beam_cannon, beam_charge, beam_turret, beam_multi.
// A stand-in until the beam entities' code is written: only the two functions other
// files call (the crossbow's laser net creates beam cannons), so the server links.
// CannonCreate makes a plain beam that removes itself after a second.

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "effects.h"
#include "beams.h"

CBeamCannon *CBeamCannon::CannonCreate( int iszModel, int iszStartSprite, int iszEndSprite )
{
	// stand-in
	CBeamCannon *pBeam = GetClassPtr( (CBeamCannon *)NULL );
	pBeam->BeamInit( STRING( iszModel ), 10 );
	pBeam->SetThink( &CBeamCannon::SUB_Remove );
	pBeam->pev->nextthink = gpGlobals->time + 1;
	return pBeam;
}

void CBeamCannon::TurnOn( void )
{
	// stand-in: the beam ends where it fires
	PointsInit( pev->origin, m_firePosition );
}

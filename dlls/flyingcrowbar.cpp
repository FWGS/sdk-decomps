/***
*
*	Copyright (c) 1999, 2000 Valve LLC. All rights reserved.
*
*	This product contains software technology licensed from Id
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc.
*	All Rights Reserved.
*
*   This source code contains proprietary and confidential information of
*   Valve LLC and its suppliers.  Access to this code is restricted to
*   persons who have executed a written SDK license with Valve.  Any access,
*   use or distribution of this code by or to any unlicensed person is illegal.
*
****/
//=========================================================
// flyingcrowbar.cpp - the thrown hatchet (flying_crowbar),
// after Lambda Bubble's throwing crowbar tutorial (file
// name ours)
//=========================================================

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"
#include "gamerules.h"


LINK_ENTITY_TO_CLASS( flying_crowbar, CFlyingCrowbar );

void CFlyingCrowbar::Spawn( )
{
	Precache( );

	// The flying crowbar is MOVETYPE_TOSS, and SOLID_BBOX.
	// We want it to be affected by gravity, and hit objects
	// within the game.
	pev->movetype = MOVETYPE_TOSS;
	pev->solid = SOLID_BBOX;

	// Use the world crowbar model.
	SET_MODEL(ENT(pev), "models/w_crowbar.mdl");

	// Set the origin and size for the HL engine collision
	// tables.
	UTIL_SetOrigin( this, pev->origin );
	UTIL_SetSize(pev, Vector(-4, -4, -4), Vector(4, 4, 4));

	// Store the owner for later use. We want the owner to be able
	// to hit themselves with the crowbar. The pev->owner gets cleared
	// later to avoid hitting the player as they throw the crowbar.
	if ( pev->owner )
		m_hOwner = Instance( pev->owner );

	// Set the think funtion.
	SetThink( BubbleThink );
	SetNextThink( 0.25 );

	// Set the touch function.
	SetTouch( SpinTouch );
}

void CFlyingCrowbar::Precache( )
{
	PRECACHE_MODEL ("models/w_crowbar.mdl");
	PRECACHE_SOUND ("weapons/cbar_hitbod1.wav");
	PRECACHE_SOUND ("weapons/cbar_hit1.wav");
	PRECACHE_SOUND ("weapons/cbar_miss1.wav");
}

void CFlyingCrowbar::SpinTouch( CBaseEntity *pOther )
{
	// We touched something in the game. Look to see if the object
	// is allowed to take damage.
	if (pOther->pev->takedamage)
	{
		// Get the traceline info to the target.
		TraceResult tr = UTIL_GetGlobalTrace( );

		// Apply damage to the target. If we have an owner stored, use that one,
		// otherwise count it as self-inflicted.
		ClearMultiDamage( );
		pOther->TraceAttack(pev, 90, pev->velocity.Normalize(), &tr,
							DMG_NEVERGIB | DMG_SLASH );
		if (m_hOwner != NULL)
			ApplyMultiDamage( pev, m_hOwner->pev );
		else
			ApplyMultiDamage( pev, pev );
	}

	int fHitFlesh = FALSE; // name ours

	// If we hit a monster, make a nice squishy thunk sound. Otherwise
	// make a clang noise and throw a bunch of sparks.
	if ( pOther->Classify() != CLASS_NONE && pOther->Classify() != CLASS_MACHINE )
	{
		EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "weapons/cbar_hitbod1.wav",
						1.0, ATTN_NORM, 0, 100);
		fHitFlesh = TRUE;
	}
	else
	{
		EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "weapons/cbar_hit1.wav",
						1.0, ATTN_NORM, 0, 100);
		if ( UTIL_PointContents(pev->origin) != CONTENTS_WATER)
		{
			UTIL_Sparks( pev->origin );
			UTIL_Sparks( pev->origin );
			UTIL_Sparks( pev->origin );
		}
	}

	// Don't draw the flying crowbar anymore.
	pev->effects |= EF_NODRAW;
	pev->solid = SOLID_NOT;

	// Spawn a crowbar weapon
	CBaseEntity *pItem = CBaseEntity::Create( "weapon_crowbar",
							pev->origin , pev->angles, edict() );

	// Get the unit vector in the direction of motion.
	Vector vecDir = pev->velocity.Normalize( );

	// Trace a line along the velocity vector to get the normal at impact.
	TraceResult tr;
	UTIL_TraceLine(pev->origin, pev->origin + vecDir * 100,
					dont_ignore_monsters, ENT(pev), &tr);

	// Throw the crowbar along the normal so it looks kinda
	// like a ricochet.
	if ( fHitFlesh )
	{
		pItem->pev->velocity = tr.vecPlaneNormal * 30;
		pItem->pev->avelocity.x = 0;
	}
	else
	{
		pItem->pev->velocity = tr.vecPlaneNormal * 300;
		pItem->pev->avelocity.x = -500;
	}

	// Remove this flying_crowbar from the world.
	SetThink( SUB_Remove );
	SetNextThink( 0.1 );
}

void CFlyingCrowbar::BubbleThink( void )
{
	// We have no owner. We do this .25 seconds AFTER the crowbar
	// is thrown so that we don't hit the owner immediately when throwing
	// it. If is comes back later, we want to be able to hit the owner.
	pev->owner = NULL;

	// Only think every .25 seconds.
	SetNextThink( 0.25 );

	// Make a whooshy sound.
	EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "weapons/cbar_miss1.wav",
					1, ATTN_NORM, 0, 120);

	// If the crowbar enters water, make some bubbles.
	if (pev->waterlevel)
		UTIL_BubbleTrail( pev->origin - pev->velocity * 0.1, pev->origin, 1 );
}

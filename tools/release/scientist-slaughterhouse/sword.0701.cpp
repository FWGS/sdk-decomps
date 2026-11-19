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
//=========================================================
// sword.cpp - Scientist Slaughterhouse's sword, weapon_sword
// (file name ours)
//=========================================================

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"
#include "gamerules.h"


#define	CROWBAR_BODYHIT_VOLUME 128
#define	CROWBAR_WALLHIT_VOLUME 512

// defined in crowbar.cpp
extern void FindHullIntersection( const Vector &vecSrc, TraceResult &tr, float *mins, float *maxs, edict_t *pEntity );

LINK_ENTITY_TO_CLASS( weapon_sword, CSword );



enum sword_e {	// v_sword.mdl's sequences
	SWORD_IDLE1 = 0,
	SWORD_DRAW,
	SWORD_HOLSTER,
	SWORD_ATTACK1,
	SWORD_ATTACK2,
	SWORD_ATTACK3,
	SWORD_IDLE2,
	SWORD_IDLE3
};


void CSword::Spawn( )
{
	Precache( );
	m_iId = WEAPON_SWORD;
	SET_MODEL(ENT(pev), "models/w_crowbar.mdl");
	m_iClip = -1;

	FallInit();// get ready to fall down.
}


void CSword::Precache( void )
{
	PRECACHE_MODEL("models/v_crowbar.mdl");
	PRECACHE_MODEL("models/w_sword.mdl");
	PRECACHE_MODEL("models/p_crowbar.mdl");
	PRECACHE_SOUND("weapons/cbar_hit1.wav");
	PRECACHE_SOUND("weapons/cbar_hit2.wav");
	PRECACHE_SOUND("weapons/cbar_hitbod1.wav");
	PRECACHE_SOUND("weapons/cbar_hitbod2.wav");
	PRECACHE_SOUND("weapons/cbar_hitbod3.wav");
	PRECACHE_SOUND("weapons/cbar_miss1.wav");
	PRECACHE_MODEL("sprites/yelflare1.spr");

	m_usSword = PRECACHE_EVENT ( 1, "events/sword.sc" );
}

int CSword::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = NULL;
	p->iMaxAmmo1 = -1;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = 0;
	p->iPosition = 1;
	p->iId = WEAPON_SWORD;
	p->iWeight = SWORD_WEIGHT;
	return 1;
}

#ifndef CLIENT_DLL
const char *CSword::GetKillVerb( BOOL fGibbed, int &iHitGroup )
{
	return DamageVerb( DMG_SLASH, fGibbed );
}

const char *CSword::GetHitGroupDesc( int iHitGroup )
{
	return HitGroupOf( iHitGroup );
}
#endif



BOOL CSword::Deploy( )
{
#ifndef CLIENT_DLL
	// the blade's glow on (events/sword.sc)
	PLAYBACK_EVENT_FULL( FEV_HOSTONLY, m_pPlayer->edict(), m_usSword, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, 0, 0, 1, 0, 0, 0 );
#endif
	return DefaultDeploy( "models/v_crowbar.mdl", "models/p_crowbar.mdl", SWORD_DRAW, "crowbar" );
}

void CSword::Holster( int skiplocal /* = 0 */ )
{
#ifndef CLIENT_DLL
	// and off
	PLAYBACK_EVENT_FULL( FEV_HOSTONLY, m_pPlayer->edict(), m_usSword, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, 0, 0, -1, 0, 0, 0 );
#endif
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;
	SendWeaponAnim( SWORD_HOLSTER );
}


void CSword::Smack( )
{
	DecalGunshot( &m_trHit, BULLET_PLAYER_CROWBAR );
}


void CSword::PrimaryAttack()
{
#ifndef CLIENT_DLL
	TraceResult tr;

	UTIL_MakeVectors (m_pPlayer->pev->v_angle);
	Vector vecSrc	= m_pPlayer->GetGunPosition( );
	Vector vecEnd	= vecSrc + gpGlobals->v_forward * 64;

	UTIL_TraceLine( vecSrc, vecEnd, dont_ignore_monsters, ENT( m_pPlayer->pev ), &tr );

	if ( tr.flFraction >= 1.0 )
	{
		UTIL_TraceHull( vecSrc, vecEnd, dont_ignore_monsters, head_hull, ENT( m_pPlayer->pev ), &tr );
		if ( tr.flFraction < 1.0 )
		{
			// Calculate the point of intersection of the line (or hull) and the object we hit
			// This is and approximation of the "best" intersection
			CBaseEntity *pHit = CBaseEntity::Instance( tr.pHit );
			if ( !pHit || pHit->IsBSPModel() )
				FindHullIntersection( vecSrc, tr, VEC_DUCK_HULL_MIN, VEC_DUCK_HULL_MAX, m_pPlayer->edict() );
			vecEnd = tr.vecEndPos;	// This is the point on the actual surface (the hull could have hit space)
		}
	}
#endif

	PLAYBACK_EVENT_FULL( FEV_NOTHOST, m_pPlayer->edict(), m_usSword, 
	0.0, (float *)&g_vecZero, (float *)&g_vecZero, 0, 0, 0,
	0.0, 0, 0.0 );

	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.5;

	// player "shoot" animation
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

	int iSwing = UTIL_SharedRandomLong( m_pPlayer->random_seed, 0, 2 );	// name ours
	switch( iSwing )
	{
	case 1:
		SendWeaponAnim( SWORD_ATTACK2 ); break;
	case 2:
		SendWeaponAnim( SWORD_ATTACK3 ); break;
	default:
		SendWeaponAnim( SWORD_ATTACK1 ); break;
	}

#ifndef CLIENT_DLL
	if ( tr.flFraction < 1 )
	{
		// hit
		CBaseEntity *pEntity = CBaseEntity::Instance(tr.pHit);

		ClearMultiDamage( );

		// the third swing, a thrust, does more damage and cuts the victim in half unless it hits the head
		int iDamage = iSwing == 2 ? gSkillData.plrDmgCrowbar * 5 : gSkillData.plrDmgCrowbar * 3;
		pEntity->TraceAttack(m_pPlayer->pev, iDamage, gpGlobals->v_forward, &tr,
			( iSwing == 2 && tr.iHitgroup != HITGROUP_HEAD ) ? ( DMG_SLASH | DMG_ENERGYBEAM | DMG_NEVERGIB | DMG_HALVE ) : ( DMG_SLASH | DMG_ENERGYBEAM | DMG_NOGIB ) );
		ApplyMultiDamage( m_pPlayer->pev, m_pPlayer->pev );

		if (pEntity)
		{
			if ( pEntity->Classify() != CLASS_NONE && pEntity->Classify() != CLASS_MACHINE )
			{
				// play thwack or smack sound
				switch( RANDOM_LONG(0,2) )
				{
				case 0:
					EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/cbar_hitbod1.wav", 1, ATTN_NORM); break;
				case 1:
					EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/cbar_hitbod2.wav", 1, ATTN_NORM); break;
				case 2:
					EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/cbar_hitbod3.wav", 1, ATTN_NORM); break;
				}
				m_pPlayer->m_iWeaponVolume = CROWBAR_BODYHIT_VOLUME;
				return;
			}
		}

		// play texture hit sound
		float fvolbar = TEXTURETYPE_PlaySound(&tr, vecSrc, vecSrc + (vecEnd-vecSrc)*2, BULLET_PLAYER_CROWBAR);

		if ( g_pGameRules->IsMultiplayer() )
		{
			// override the volume here, cause we don't play texture sounds in multiplayer, 
			// and fvolbar is going to be 0 from the above call.

			fvolbar = 1;
		}

		// also play crowbar strike
		switch( RANDOM_LONG(0,1) )
		{
		case 0:
			EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/cbar_hit1.wav", fvolbar, ATTN_NORM, 0, 98 + RANDOM_LONG(0,3)); 
			break;
		case 1:
			EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/cbar_hit2.wav", fvolbar, ATTN_NORM, 0, 98 + RANDOM_LONG(0,3)); 
			break;
		}

		// delay the decal a bit
		m_trHit = tr;

		m_pPlayer->m_iWeaponVolume = CROWBAR_WALLHIT_VOLUME;

		SetThink( Smack );
		pev->nextthink = gpGlobals->time + 0.2;
	}
#endif
}
// Build-side stand-in, not source: sword.cpp as it may have been at the last full link
// (2002-07-01). Retail's .data keeps two contributions of sword.obj from that link (21-24
// and 18-20 bytes) and places p_sword.mdl and v_sword.mdl first-fit, as a later relink does.
// Here the view and player models are still the crowbar's (crowbar.obj owns those strings),
// so sword.obj's own strings are yelflare1.spr and w_sword.mdl. The text is a guess.

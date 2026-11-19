/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
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

#ifdef SIC1
LINK_ENTITY_TO_CLASS( weapon_crowbar, CCrowbar );
#else
LINK_ENTITY_TO_CLASS( weapon_fork, CCrowbar );
#endif



enum gauss_e {
	CROWBAR_IDLE = 0,
#ifdef SIC1
	CROWBAR_DRAW,
	CROWBAR_HOLSTER,
#else
	CROWBAR_HOLSTER,	// the fork model has these two the other way round
	CROWBAR_DRAW,
#endif
	CROWBAR_ATTACK1HIT,
	CROWBAR_ATTACK1MISS,
	CROWBAR_ATTACK2MISS,
	CROWBAR_ATTACK2HIT,
	CROWBAR_ATTACK3MISS,
	CROWBAR_ATTACK3HIT
};


void CCrowbar::Spawn( )
{
	Precache( );
	m_iId = WEAPON_CROWBAR;
#ifdef SIC1
	SET_MODEL(ENT(pev), "models/w_say.mdl");
#else
	SET_MODEL(ENT(pev), "models/w_fork.mdl");
#endif
	m_iClip = -1;

	FallInit();// get ready to fall down.
}


void CCrowbar::Precache( void )
{
#ifdef SIC1
	PRECACHE_MODEL("models/v_say.mdl");
	PRECACHE_MODEL("models/w_say.mdl");
	PRECACHE_MODEL("models/p_crowbar.mdl");
	PRECACHE_SOUND("weapons/cbar_hit1.wav");
	PRECACHE_SOUND("weapons/cbar_hit2.wav");
#else
	PRECACHE_MODEL("models/v_fork.mdl");
	PRECACHE_MODEL("models/w_fork.mdl");
	PRECACHE_MODEL("models/p_crowbar.mdl");
	PRECACHE_SOUND("weapons/fork_hit1.wav");
	PRECACHE_SOUND("weapons/fork_hit2.wav");
#endif
	PRECACHE_SOUND("weapons/cbar_hitbod1.wav");
	PRECACHE_SOUND("weapons/cbar_hitbod2.wav");
	PRECACHE_SOUND("weapons/cbar_hitbod3.wav");
#ifdef SIC1
	PRECACHE_SOUND("weapons/cbar_miss1.wav");
#else
	PRECACHE_SOUND("weapons/fork_miss1.wav");
#endif

	m_usCrowbar = PRECACHE_EVENT ( 1, "events/crowbar.sc" );
}

int CCrowbar::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = NULL;
	p->iMaxAmmo1 = -1;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = 0;
	p->iPosition = 0;
	p->iId = WEAPON_CROWBAR;
	p->iWeight = CROWBAR_WEIGHT;
	return 1;
}



BOOL CCrowbar::Deploy( )
{
#ifdef SIC1
	return DefaultDeploy( "models/v_say.mdl", "models/p_crowbar.mdl", CROWBAR_DRAW, "crowbar" );
#else
	return DefaultDeploy( "models/v_fork.mdl", "models/p_crowbar.mdl", CROWBAR_DRAW, "crowbar" );
#endif
}

void CCrowbar::Holster( int skiplocal /* = 0 */ )
{
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;
	SendWeaponAnim( CROWBAR_HOLSTER );
}


void FindHullIntersection( const Vector &vecSrc, TraceResult &tr, float *mins, float *maxs, edict_t *pEntity )
{
	int			i, j, k;
	float		distance;
	float		*minmaxs[2] = {mins, maxs};
	TraceResult tmpTrace;
	Vector		vecHullEnd = tr.vecEndPos;
	Vector		vecEnd;

	distance = 1e6f;

	vecHullEnd = vecSrc + ((vecHullEnd - vecSrc)*2);
	UTIL_TraceLine( vecSrc, vecHullEnd, dont_ignore_monsters, pEntity, &tmpTrace );
	if ( tmpTrace.flFraction < 1.0 )
	{
		tr = tmpTrace;
		return;
	}

	for ( i = 0; i < 2; i++ )
	{
		for ( j = 0; j < 2; j++ )
		{
			for ( k = 0; k < 2; k++ )
			{
				vecEnd.x = vecHullEnd.x + minmaxs[i][0];
				vecEnd.y = vecHullEnd.y + minmaxs[j][1];
				vecEnd.z = vecHullEnd.z + minmaxs[k][2];

				UTIL_TraceLine( vecSrc, vecEnd, dont_ignore_monsters, pEntity, &tmpTrace );
				if ( tmpTrace.flFraction < 1.0 )
				{
					float thisDistance = (tmpTrace.vecEndPos - vecSrc).Length();
					if ( thisDistance < distance )
					{
						tr = tmpTrace;
						distance = thisDistance;
					}
				}
			}
		}
	}
}


void CCrowbar::PrimaryAttack()
{
	if (! Swing( 1 ))
	{
		SetThink(&CCrowbar:: SwingAgain );
		SetNextThink( 0.1 );
	}
}


void CCrowbar::Smack( )
{
	DecalGunshot( &m_trHit, BULLET_PLAYER_CROWBAR );
}


void CCrowbar::SwingAgain( void )
{
	Swing( 0 );
}


int CCrowbar::Swing( int fFirst )
{
	int fDidHit = FALSE;

	TraceResult tr;

	UTIL_MakeVectors (m_pPlayer->pev->v_angle);
	Vector vecSrc	= m_pPlayer->GetGunPosition( );
	Vector vecEnd	= vecSrc + gpGlobals->v_forward * 32;

	UTIL_TraceLine( vecSrc, vecEnd, dont_ignore_monsters, ENT( m_pPlayer->pev ), &tr );

#ifndef CLIENT_DLL
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

	PLAYBACK_EVENT_FULL( FEV_NOTHOST, m_pPlayer->edict(), m_usCrowbar, 
	0.0, (float *)&g_vecZero, (float *)&g_vecZero, 0, 0, 0,
	0.0, 0, 0.0 );


	if ( tr.flFraction >= 1.0 )
	{
		if (fFirst)
		{
			// miss
#ifdef SIC1
			m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.5;
#else
			m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.9;
#endif
			
			// player "shoot" animation
			m_pPlayer->SetAnimation( PLAYER_ATTACK1 );
		}
	}
	else
	{
		switch( ((m_iSwing++) % 2) + 1 )
		{
		case 0:
			SendWeaponAnim( CROWBAR_ATTACK1HIT ); break;
		case 1:
			SendWeaponAnim( CROWBAR_ATTACK2HIT ); break;
		case 2:
			SendWeaponAnim( CROWBAR_ATTACK3HIT ); break;
		}

		// player "shoot" animation
		m_pPlayer->SetAnimation( PLAYER_ATTACK1 );
		
#ifndef CLIENT_DLL

		// hit
		fDidHit = TRUE;
		CBaseEntity *pEntity = CBaseEntity::Instance(tr.pHit);

		ClearMultiDamage( );

		if ( (m_flNextPrimaryAttack + 1 < UTIL_WeaponTimeBase() ) || g_pGameRules->IsMultiplayer() )
		{
			// first swing does full damage
#ifdef SIC1
			pEntity->TraceAttack(m_pPlayer->pev, gSkillData.plrDmgCrowbar, gpGlobals->v_forward, &tr, DMG_CLUB ); 
#else
			pEntity->TraceAttack(m_pPlayer->pev, gSkillData.plrDmgFork, gpGlobals->v_forward, &tr, DMG_CLUB ); 
#endif
		}
		else
		{
			// subsequent swings do half
#ifdef SIC1
			pEntity->TraceAttack(m_pPlayer->pev, gSkillData.plrDmgCrowbar / 2, gpGlobals->v_forward, &tr, DMG_CLUB ); 
#else
			pEntity->TraceAttack(m_pPlayer->pev, gSkillData.plrDmgFork / 2, gpGlobals->v_forward, &tr, DMG_CLUB ); 
#endif
		}	
		ApplyMultiDamage( m_pPlayer->pev, m_pPlayer->pev );

		// play thwack, smack, or dong sound
		float flVol = 1.0;
		int fHitWorld = TRUE;

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
				if ( !pEntity->IsAlive() )
				{
#ifdef SIC1
					m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.2; //LRC: corrected half-life bug
#else
					m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.9; //LRC: corrected half-life bug
#endif
					return TRUE;
				}
				else
					  flVol = 0.1;

				fHitWorld = FALSE;
			}
		}

		// play texture hit sound
		// UNDONE: Calculate the correct point of intersection when we hit with the hull instead of the line

		if (fHitWorld)
		{
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
#ifdef SIC1
				EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/cbar_hit1.wav", fvolbar, ATTN_NORM, 0, 98 + RANDOM_LONG(0,3)); 
#else
				EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/fork_hit1.wav", fvolbar, ATTN_NORM, 0, 98 + RANDOM_LONG(0,3)); 
#endif
				break;
			case 1:
#ifdef SIC1
				EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/cbar_hit2.wav", fvolbar, ATTN_NORM, 0, 98 + RANDOM_LONG(0,3)); 
#else
				EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/fork_hit2.wav", fvolbar, ATTN_NORM, 0, 98 + RANDOM_LONG(0,3)); 
#endif
				break;
			}

#ifdef SIC1
			// delay the decal a bit
			m_trHit = tr;
#else
			// two puffs of dust; m_trHit is no longer set, so Smack has
			// nothing to decal (sic)
			MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, tr.vecEndPos );
				WRITE_BYTE( TE_SPRITE );
				WRITE_COORD( tr.vecEndPos.x + 2 );
				WRITE_COORD( tr.vecEndPos.y + 2 );
				WRITE_COORD( tr.vecEndPos.z + 2 );
				WRITE_SHORT( g_sModelIndexSmallPuff );
				WRITE_BYTE( 2 ); // scale * 10
				WRITE_BYTE( 55 ); // brightness
			MESSAGE_END();

			MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, tr.vecEndPos );
				WRITE_BYTE( TE_SPRITE );
				WRITE_COORD( tr.vecEndPos.x - 2 );
				WRITE_COORD( tr.vecEndPos.y - 2 );
				WRITE_COORD( tr.vecEndPos.z - 2 );
				WRITE_SHORT( g_sModelIndexSmallPuff );
				WRITE_BYTE( 2 ); // scale * 10
				WRITE_BYTE( 55 ); // brightness
			MESSAGE_END();
#endif
		}

		m_pPlayer->m_iWeaponVolume = flVol * CROWBAR_WALLHIT_VOLUME;
#endif
#ifdef SIC1
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.2;
#else
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.9;
#endif
		
		SetThink(&CCrowbar:: Smack );
#ifdef SIC1
		SetNextThink( 0.2 );
#else
		SetNextThink( 0.9 );
#endif

		
	}
	return fDidHit;
}




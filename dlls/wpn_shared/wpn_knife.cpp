/***
*
*	Copyright (c) 1999, 2000 Valve LLC. All rights reserved.
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

#define	KNIFE_BODYHIT_VOLUME 128
#define	KNIFE_WALLHIT_VOLUME 512

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"
#include "gamerules.h"


LINK_ENTITY_TO_CLASS( weapon_knife, CKnife );



enum knife_e {		// names ours
	KNIFE_IDLE = 0,
	KNIFE_ATTACK1HIT,
	KNIFE_ATTACK2HIT,
	KNIFE_DRAW,
	KNIFE_STABHIT,
	KNIFE_STABMISS,
	KNIFE_MIDATTACK1HIT,
	KNIFE_MIDATTACK2HIT,
};


void CKnife::Spawn( )
{
	Precache( );
	m_iId = WEAPON_KNIFE;
	SET_MODEL(ENT(pev), "models/w_knife.mdl");
	m_iClip = -1;

	FallInit();// get ready to fall down.
}


void CKnife::Precache( void )
{
	PRECACHE_MODEL("models/v_knife.mdl");
	PRECACHE_MODEL("models/v_knife_r.mdl");
	PRECACHE_MODEL("models/w_knife.mdl");
	PRECACHE_SOUND("weapons/knife_deploy1.wav");
	PRECACHE_SOUND("weapons/knife_hit1.wav");
	PRECACHE_SOUND("weapons/knife_hit2.wav");
	PRECACHE_SOUND("weapons/knife_hit3.wav");
	PRECACHE_SOUND("weapons/knife_hit4.wav");
	PRECACHE_SOUND("weapons/knife_slash1.wav");
	PRECACHE_SOUND("weapons/knife_slash2.wav");
	PRECACHE_SOUND("weapons/knife_stab.wav");
	PRECACHE_SOUND("weapons/knife_hitwall1.wav");

	m_usKnife = PRECACHE_EVENT( 1, "events/knife.sc" );
}

int CKnife::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = NULL;
	p->iMaxAmmo1 = -1;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = 2;
	p->iPosition = 1;
	p->iId = WEAPON_KNIFE;
	p->iWeight = KNIFE_WEIGHT;
	return 1;
}



BOOL CKnife::Deploy( )
{
	EMIT_SOUND( ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/knife_deploy1.wav", 0.3, 2.4 );

	if ( m_pPlayer->m_bLeftHanded == true )
		return DefaultDeploy( "models/v_knife.mdl", "models/p_knife.mdl", KNIFE_DRAW, "knife", UseDecrement() ? 1 : 0 );
	else
		return DefaultDeploy( "models/v_knife_r.mdl", "models/p_knife.mdl", KNIFE_DRAW, "knife", UseDecrement() ? 1 : 0 );
}

void CKnife::Holster( int skiplocal /* = 0 */ )
{
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;
}

void CKnife::WeaponAnimation( int iAnimation )
{
	PLAYBACK_EVENT_FULL( FEV_NOTHOST, m_pPlayer->edict(), m_usKnife, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, 0.0, 0.0, iAnimation, 2, 3, 4 );
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


void CKnife::PrimaryAttack()
{
	if (! Swing( 1 ))
	{
		SetThink( SwingAgain );
		pev->nextthink = UTIL_WeaponTimeBase() + 0.1;
	}
}


void CKnife::SecondaryAttack()
{
	Stab( 1 );
	pev->nextthink = UTIL_WeaponTimeBase() + 0.35;
}


void CKnife::Smack( )
{
	DecalGunshot( &m_trHit, BULLET_PLAYER_CROWBAR, false, m_pPlayer->pev, false );
}


void CKnife::SwingAgain( void )
{
	Swing( 0 );
}


int CKnife::Swing( int fFirst )
{
	;	// sic: a stand-in of two lines (two or more give retail's client).  Counter-Strike 1.3's client has
	;	// Vector::operator* between Swing and Stab: 48 bytes more intermediate code before Stab than the SDK's layout; the eight breaks below are 40
	int fDidHit = FALSE;

	TraceResult tr;

	UTIL_MakeVectors (m_pPlayer->pev->v_angle);
	Vector vecSrc	= m_pPlayer->GetGunPosition( );
	Vector vecEnd	= vecSrc + gpGlobals->v_forward * 65;

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

	if ( tr.flFraction >= 1.0 )
	{
		if (fFirst)
		{
			// miss
			if ( m_flNextPrimaryAttack + 0.4 < UTIL_WeaponTimeBase() )
			{
				switch( (m_iSwing++) % 2 )
				{
				case 0:
					SendWeaponAnim( KNIFE_ATTACK1HIT );
					break;
				case 1:
					SendWeaponAnim( KNIFE_ATTACK2HIT );
					break;
				}
			}
			else
			{
				switch( (m_iSwing++) % 2 )
				{
				case 0:
					SendWeaponAnim( KNIFE_MIDATTACK1HIT );
					break;
				case 1:
					SendWeaponAnim( KNIFE_MIDATTACK2HIT );
					break;
				}
			}

			m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.35;
			m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.5;

			// play wiff or swish sound
			if ( RANDOM_LONG(0,1) )
				EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/knife_slash1.wav", 1, ATTN_NORM, 0, 94);
			else
				EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/knife_slash2.wav", 1, ATTN_NORM, 0, 94);

			// player "shoot" animation
			m_pPlayer->SetAnimation( PLAYER_ATTACK1 );
		}
	}
	else
	{
		// hit
		fDidHit = TRUE;

		CBaseEntity *pEntity = CBaseEntity::Instance(tr.pHit);

		if ( m_flNextPrimaryAttack + 0.4 < UTIL_WeaponTimeBase() )
		{
			switch( (m_iSwing++) % 2 )
			{
			case 0:
				SendWeaponAnim( KNIFE_ATTACK1HIT );
				break;
			case 1:
				SendWeaponAnim( KNIFE_ATTACK2HIT );
				break;
			}
		}
		else
		{
			switch( (m_iSwing++) % 2 )
			{
			case 0:
				SendWeaponAnim( KNIFE_MIDATTACK1HIT );
				break;
			case 1:
				SendWeaponAnim( KNIFE_MIDATTACK2HIT );
				break;
			}
		}

		// player "shoot" animation
		m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

		ClearMultiDamage( );
		if ( m_flNextPrimaryAttack + 0.4 < UTIL_WeaponTimeBase() )
		{
			// first swing does full damage
			pEntity->TraceAttack(m_pPlayer->pev, 20, gpGlobals->v_forward, &tr, DMG_NEVERGIB | DMG_BULLET ); 
		}
		else
		{
			// subsequent swings do less
			pEntity->TraceAttack(m_pPlayer->pev, 15, gpGlobals->v_forward, &tr, DMG_NEVERGIB | DMG_BULLET ); 
		}	
		ApplyMultiDamage( m_pPlayer->pev, m_pPlayer->pev );

		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.4;
		m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.5;

		// play thwack, smack, or dong sound
		float flVol = 1.0;
		int fHitWorld = TRUE;

		if (pEntity)
		{
			if (pEntity->Classify() != CLASS_NONE && pEntity->Classify() != CLASS_MACHINE)
			{
				// play thwack or smack sound
				switch( RANDOM_LONG(0,3) )
				{
				case 0:
					EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/knife_hit1.wav", 1, ATTN_NORM); break;
				case 1:
					EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/knife_hit2.wav", 1, ATTN_NORM); break;
				case 2:
					EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/knife_hit3.wav", 1, ATTN_NORM); break;
				case 3:
					EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/knife_hit4.wav", 1, ATTN_NORM); break;
				}
				m_pPlayer->m_iWeaponVolume = KNIFE_BODYHIT_VOLUME;
				if (!pEntity->IsAlive() )
					return TRUE;
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

			fvolbar = 1;

			// also play knife strike
			switch( RANDOM_LONG(0,1) )
			{
			case 0:
				EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/knife_hitwall1.wav", fvolbar, ATTN_NORM, 0, 98 + RANDOM_LONG(0,3)); 
				break;
			case 1:
				EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/knife_hitwall1.wav", fvolbar, ATTN_NORM, 0, 98 + RANDOM_LONG(0,3)); 
				break;
			}
		}

		// delay the decal a bit
		m_trHit = tr;
		SetThink( Smack );
		pev->nextthink = UTIL_WeaponTimeBase() + 0.2;

		m_pPlayer->m_iWeaponVolume = flVol * KNIFE_WALLHIT_VOLUME;
	}
	return fDidHit;
}


int CKnife::Stab( int fFirst )
{
	int fDidHit = FALSE;

	TraceResult tr;

	UTIL_MakeVectors (m_pPlayer->pev->v_angle);
	Vector vecSrc	= m_pPlayer->GetGunPosition( );
	Vector vecEnd	= vecSrc + gpGlobals->v_forward * 32;

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

	if ( tr.flFraction >= 1.0 )
	{
		if (fFirst)
		{
			// miss
			SendWeaponAnim( KNIFE_STABMISS );

			m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 1;
			m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 1;

			// play wiff or swish sound
			if ( RANDOM_LONG(0,1) )
				EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/knife_slash1.wav", 1, ATTN_NORM, 0, 94);
			else
				EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/knife_slash2.wav", 1, ATTN_NORM, 0, 94);

			// player "shoot" animation
			m_pPlayer->SetAnimation( PLAYER_ATTACK1 );
		}
	}
	else
	{
		// hit
		fDidHit = TRUE;

		CBaseEntity *pEntity = CBaseEntity::Instance(tr.pHit);

		SendWeaponAnim( KNIFE_STABHIT );

		// player "shoot" animation
		m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

		ClearMultiDamage( );
		pEntity->TraceAttack(m_pPlayer->pev, 65, gpGlobals->v_forward, &tr, DMG_NEVERGIB | DMG_BULLET ); 
		ApplyMultiDamage( m_pPlayer->pev, m_pPlayer->pev );

		m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 1.1;
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 1.1;

		// play thwack, smack, or dong sound
		float flVol = 1.0;
		int fHitWorld = TRUE;

		if (pEntity)
		{
			if (pEntity->Classify() != CLASS_NONE && pEntity->Classify() != CLASS_MACHINE)
			{
				EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/knife_stab.wav", 1, ATTN_NORM);
				m_pPlayer->m_iWeaponVolume = KNIFE_BODYHIT_VOLUME;
				if (!pEntity->IsAlive() )
					return TRUE;
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

			fvolbar = 1;

			// also play knife strike
			switch( RANDOM_LONG(0,1) )
			{
			case 0:
				EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/knife_hitwall1.wav", fvolbar, ATTN_NORM, 0, 98 + RANDOM_LONG(0,3)); 
				break;
			case 1:
				EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/knife_hitwall1.wav", fvolbar, ATTN_NORM, 0, 98 + RANDOM_LONG(0,3)); 
				break;
			}
		}

		// delay the decal a bit
		m_trHit = tr;
		SetThink( Smack );
		pev->nextthink = UTIL_WeaponTimeBase() + 0.2;

		m_pPlayer->m_iWeaponVolume = flVol * KNIFE_WALLHIT_VOLUME;
	}
	return fDidHit;
}

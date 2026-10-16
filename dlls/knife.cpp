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

// Wanted: the knife (weapon_knife) and the knife it throws (knife_throw).
// The file name is ours; retail's is unknown. The weapon is the SDK 1.0
// crowbar (its Holster( void ) included) with Wanted's sounds and horse
// traces; the secondary attack throws it. A thrown knife is a
// crossbow-bolt-like projectile that drops a weapon_knife where it lands.

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"
#include "gamerules.h"

extern void ExpandHorseHulls( int fExpand );	// name ours; horses and pumas are traced with big boxes


#define	KNIFE_BODYHIT_VOLUME 128
#define	KNIFE_WALLHIT_VOLUME 512

// the thrown knife
class CKnifethrow : public CBaseEntity
{
	void Spawn( void );
	void Precache( void );
	int  Classify ( void );
	void EXPORT KnifeTouch( CBaseEntity *pOther );
	void AirThink( void );	// name ours

public:
	static CKnifethrow *KnifeCreate( void );	// name ours
};
LINK_ENTITY_TO_CLASS( knife_throw, CKnifethrow );

CKnifethrow *CKnifethrow::KnifeCreate( void )
{
	// Create a new entity with CKnifethrow private data
	CKnifethrow *pKnife = GetClassPtr( (CKnifethrow *)NULL );
	pKnife->pev->classname = MAKE_STRING("knifethrow");
	pKnife->Spawn();

	return pKnife;
}

void CKnifethrow::Spawn( )
{
	Precache( );
	pev->movetype = MOVETYPE_BOUNCE;
	pev->solid = SOLID_BBOX;

	pev->gravity = 0.5;

	UTIL_SetSize(pev, Vector(-5, -5, -5), Vector(5, 5, 5));

	SET_MODEL(ENT(pev), "models/w_knife.mdl");

	UTIL_SetOrigin( pev, pev->origin );

	SetTouch( KnifeTouch );
	SetThink( AirThink );
	AirThink( );
}


void CKnifethrow::Precache( )
{
	PRECACHE_MODEL ("models/w_knife.mdl");
}


int	CKnifethrow :: Classify ( void )
{
	return	CLASS_NONE;
}

void CKnifethrow::KnifeTouch( CBaseEntity *pOther )
{
	SetTouch( NULL );
	SetThink( NULL );

	if (pOther->pev->takedamage)
	{
		TraceResult tr = UTIL_GetGlobalTrace( );
		entvars_t	*pevOwner;

		pevOwner = VARS( pev->owner );

		ClearMultiDamage( );

		pOther->TraceAttack(pevOwner, gSkillData.plrDmgKnife, pev->velocity.Normalize(), &tr, DMG_NEVERGIB );

		ApplyMultiDamage( pev, pevOwner );

		pev->velocity = Vector( 0, 0, 0 );
		EMIT_SOUND(ENT(pev), CHAN_WEAPON, "weapons/knife_hitbod1.wav", 1, ATTN_NORM);

		Killed( pev, GIB_NEVER );
	}
	else
	{
		EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "weapons/knife_hit1.wav", RANDOM_FLOAT(0.95, 1.0), ATTN_NORM, 0, 98 + RANDOM_LONG(0,7));

		SetThink( SUB_Remove );
		pev->nextthink = gpGlobals->time;

		// stick in whatever it hit
		Vector vecDir = pev->velocity.Normalize( );
		UTIL_SetOrigin( pev, pev->origin - vecDir * 12 );
		pev->angles = UTIL_VecToAngles( vecDir );
		pev->solid = SOLID_NOT;
		pev->movetype = MOVETYPE_FLY;
		pev->velocity = Vector( 0, 0, 0 );
		pev->avelocity.z = 0;
		pev->angles.z = RANDOM_LONG(0,360);

		if (UTIL_PointContents(pev->origin) != CONTENTS_WATER)
		{
			UTIL_Sparks( pev->origin );
		}
	}

	// leave a knife to pick up; speed 1 keeps it from respawning
	CBaseEntity *pKnife = CBaseEntity::Create( "weapon_knife", pev->origin, pev->angles, NULL );
	if ( pKnife )
		pKnife->pev->speed = 1;
}

// every frame: look 36 units ahead with the horses' big boxes in and
// touch what is there (an arrow-like AirThink; name ours)
void CKnifethrow::AirThink( void )
{
	Vector vecEnd;
	TraceResult tr;
	Vector vecOrigin = pev->origin;

	// sic: aims along ( p - 2p*p, 2*yaw, 2*roll ), not along the flight
	UTIL_MakeVectors( pev->angles + Vector( -( ( pev->angles.x + pev->angles.x ) * pev->angles.x ), pev->angles.y, pev->angles.z ) );

	vecEnd = pev->origin + gpGlobals->v_forward * 36;

	ExpandHorseHulls( TRUE );
	UTIL_TraceLine( pev->origin, vecEnd, dont_ignore_monsters, ENT( pev ), &tr );
	ExpandHorseHulls( FALSE );

	if ( tr.flFraction < 1.0 )
	{
		pev->origin = tr.vecEndPos;

		CBaseEntity *pEntity = CBaseEntity::Instance( tr.pHit );
		if ( pEntity )
			Touch( pEntity );
	}

	pev->origin = vecOrigin;
	pev->nextthink = gpGlobals->time;
}

























// the knife

class CKnife : public CBasePlayerWeapon
{
public:
	void Spawn( void );
	void Precache( void );
	int iItemSlot( void ) { return 1; }
	void EXPORT SwingAgain( void );
	void EXPORT Smack( void );
	int GetItemInfo(ItemInfo *p);

	void PrimaryAttack( void );
	void SecondaryAttack( void );
	int Swing( int fFirst );
	BOOL Deploy( void );
	void Holster( void );	// sic: SDK 1.0's; doesn't override Holster( int ), so it's never called
	void WeaponIdle( void );
	void CheckRespawn( void );
	int m_iSwing;
	TraceResult m_trHit;
};
LINK_ENTITY_TO_CLASS( weapon_knife, CKnife );



// sequences of models/v_knife.mdl (names ours, after the crowbar's)
enum knife_e {
	KNIFE_IDLE = 0,
	KNIFE_DRAW,
	KNIFE_HOLSTER,		// also the throw
	KNIFE_ATTACK1HIT,
	KNIFE_ATTACK1MISS,
	KNIFE_ATTACK2MISS,
	KNIFE_ATTACK2HIT,
	KNIFE_ATTACK3MISS,
	KNIFE_ATTACK3HIT,
	KNIFE_IDLE2,
	KNIFE_IDLE3
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
	PRECACHE_MODEL("models/w_knife.mdl");
	PRECACHE_MODEL("models/p_knife.mdl");
	PRECACHE_SOUND("weapons/knife_hit1.wav");
	PRECACHE_SOUND("weapons/knife_hit2.wav");
	PRECACHE_SOUND("weapons/knife_hitbod1.wav");
	PRECACHE_SOUND("weapons/knife_hitbod2.wav");
	PRECACHE_SOUND("weapons/knife_hitbod3.wav");
	PRECACHE_SOUND("weapons/knife_miss1.wav");
}

int CKnife::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = NULL;
	p->iMaxAmmo1 = -1;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = 0;
	p->iPosition = 2;
	p->iId = WEAPON_KNIFE;
	p->iWeight = CROWBAR_WEIGHT;
	return 1;
}



BOOL CKnife::Deploy( )
{
	return DefaultDeploy( "models/v_knife.mdl", "models/p_knife.mdl", KNIFE_DRAW, "knife" );
}

void CKnife::Holster( )
{
	m_pPlayer->m_flNextAttack = gpGlobals->time + 0.5;
	SendWeaponAnim( KNIFE_HOLSTER );
}


// the crowbar's FindHullIntersection; pick.cpp has the other copy under
// the SDK's name. Retail places both in source order, so neither is
// static: this one must have had a name of its own (ours).
void FindKnifeHullIntersection( const Vector &vecSrc, TraceResult &tr, float *mins, float *maxs, edict_t *pEntity )
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
		pev->nextthink = gpGlobals->time + 0.1;
	}
}


// throw the knife: it leaves the inventory and flies at 750 units/s
void CKnife::SecondaryAttack( void )
{
	m_pPlayer->m_iFOV = 0;
	ResetEmptySound( );
	m_pPlayer->GetAutoaimVector( AUTOAIM_2DEGREES );  // get the autoaim vector but ignore it

	m_pPlayer->m_iWeaponVolume = QUIET_GUN_VOLUME;

	SendWeaponAnim( KNIFE_HOLSTER );

	// player "shoot" animation
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

	Vector anglesAim = m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle;
	UTIL_MakeVectors( anglesAim );

	anglesAim.x		= -anglesAim.x;
	Vector vecSrc	 = m_pPlayer->GetGunPosition( ) - gpGlobals->v_up * 2;
	Vector vecDir	 = gpGlobals->v_forward;

	CKnifethrow *pKnife = CKnifethrow::KnifeCreate();
	pKnife->pev->origin = vecSrc;
	pKnife->pev->angles = anglesAim;
	pKnife->pev->owner = m_pPlayer->edict();
	pKnife->pev->velocity = vecDir * 750;
	pKnife->pev->speed = 750;
	pKnife->pev->avelocity.z = 50;

	m_flNextPrimaryAttack = gpGlobals->time + 2;
	m_flNextSecondaryAttack = gpGlobals->time + 2;
	m_flTimeWeaponIdle = 2;	// sic: not a time

	m_pPlayer->pev->punchangle.x -= 2;

	m_pPlayer->RemoveNamedPlayerItem( "weapon_knife" );
	m_pPlayer->pev->weapons &= ~(1<<WEAPON_KNIFE);
}


void CKnife::Smack( )
{
	DecalGunshot( &m_trHit, BULLET_PLAYER_KNIFE );
}


void CKnife::SwingAgain( void )
{
	Swing( 0 );
}


int CKnife::Swing( int fFirst )
{
	int fDidHit = FALSE;

	TraceResult tr;

	UTIL_MakeVectors (m_pPlayer->pev->v_angle);
	Vector vecSrc	= m_pPlayer->GetGunPosition( );
	Vector vecEnd	= vecSrc + gpGlobals->v_forward * 32;

	ExpandHorseHulls( TRUE );
	UTIL_TraceLine( vecSrc, vecEnd, dont_ignore_monsters, ENT( m_pPlayer->pev ), &tr );
	ExpandHorseHulls( FALSE );

	if ( tr.flFraction >= 1.0 )
	{
		UTIL_TraceHull( vecSrc, vecEnd, dont_ignore_monsters, head_hull, ENT( m_pPlayer->pev ), &tr );
		if ( tr.flFraction < 1.0 )
		{
			// Calculate the point of intersection of the line (or hull) and the object we hit
			// This is and approximation of the "best" intersection
			CBaseEntity *pHit = CBaseEntity::Instance( tr.pHit );
			if ( !pHit || pHit->IsBSPModel() )
				FindKnifeHullIntersection( vecSrc, tr, VEC_DUCK_HULL_MIN, VEC_DUCK_HULL_MAX, m_pPlayer->edict() );
			vecEnd = tr.vecEndPos;	// This is the point on the actual surface (the hull could have hit space)
		}
	}

	if ( tr.flFraction >= 1.0 )
	{
		if (fFirst)
		{
			// miss
			switch( (m_iSwing++) % 3 )
			{
			case 0:
				SendWeaponAnim( KNIFE_ATTACK1MISS ); break;
			case 1:
				SendWeaponAnim( KNIFE_ATTACK2MISS ); break;
			case 2:
				SendWeaponAnim( KNIFE_ATTACK3MISS ); break;
			}
			m_flNextPrimaryAttack = gpGlobals->time + 0.5;
			// play wiff or swish sound
			EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/knife_miss1.wav", 1, ATTN_NORM, 0, 94 + RANDOM_LONG(0,0xF));

			// player "shoot" animation
			m_pPlayer->SetAnimation( PLAYER_ATTACK1 );
		}
	}
	else
	{
		// hit
		fDidHit = TRUE;

		CBaseEntity *pEntity = CBaseEntity::Instance(tr.pHit);

		switch( ((m_iSwing++) % 2) + 1 )
		{
		case 0:
			SendWeaponAnim( KNIFE_ATTACK1HIT ); break;
		case 1:
			SendWeaponAnim( KNIFE_ATTACK2HIT ); break;
		case 2:
			SendWeaponAnim( KNIFE_ATTACK3HIT ); break;
		}

		// player "shoot" animation
		m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

		ClearMultiDamage( );
		if ( (m_flNextPrimaryAttack + 1 < gpGlobals->time) || g_pGameRules->IsMultiplayer() )
		{
			// first swing does full damage
			pEntity->TraceAttack(m_pPlayer->pev, gSkillData.plrDmgKnife, gpGlobals->v_forward, &tr, DMG_CLUB );
		}
		else
		{
			// subsequent swings do half
			pEntity->TraceAttack(m_pPlayer->pev, gSkillData.plrDmgKnife / 2, gpGlobals->v_forward, &tr, DMG_CLUB );
		}
		ApplyMultiDamage( m_pPlayer->pev, m_pPlayer->pev );

		m_flNextPrimaryAttack = gpGlobals->time + 0.25;

		// play thwack, smack, or dong sound
		float flVol = 1.0;
		int fHitWorld = TRUE;

		if (pEntity)
		{
			if (pEntity->Classify() != CLASS_NONE && pEntity->Classify() != CLASS_MACHINE)
			{
				// play thwack or smack sound
				switch( RANDOM_LONG(0,2) )
				{
				case 0:
					EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/knife_hitbod1.wav", 1, ATTN_NORM); break;
				case 1:
					EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/knife_hitbod2.wav", 1, ATTN_NORM); break;
				case 2:
					EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/knife_hitbod3.wav", 1, ATTN_NORM); break;
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
			float fvolbar = TEXTURETYPE_PlaySound(&tr, vecSrc, vecSrc + (vecEnd-vecSrc)*2, BULLET_PLAYER_KNIFE);

			if ( g_pGameRules->IsMultiplayer() )
			{
				// override the volume here, cause we don't play texture sounds in multiplayer,
				// and fvolbar is going to be 0 from the above call.

				fvolbar = 1;
			}

			// also play knife strike
			switch( RANDOM_LONG(0,1) )
			{
			case 0:
				EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/knife_hit1.wav", fvolbar, ATTN_NORM, 0, 98 + RANDOM_LONG(0,3));
				break;
			case 1:
				EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/knife_hit2.wav", fvolbar, ATTN_NORM, 0, 98 + RANDOM_LONG(0,3));
				break;
			}
		}

		// delay the decal a bit
		m_trHit = tr;
		SetThink( Smack );
		pev->nextthink = gpGlobals->time + 0.2;

		m_pPlayer->m_iWeaponVolume = flVol * KNIFE_WALLHIT_VOLUME;
	}
	return fDidHit;
}


void CKnife::WeaponIdle( void )
{
	ResetEmptySound( );
	m_pPlayer->m_iFOV = 0;
	m_pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );

	if ( m_flTimeWeaponIdle > gpGlobals->time )
		return;

	switch ( RANDOM_LONG( 0, 2 ) )
	{
	case 0:
		SendWeaponAnim( KNIFE_IDLE );
		m_flTimeWeaponIdle = gpGlobals->time + 2.23;
		break;
	case 1:
		SendWeaponAnim( KNIFE_IDLE2 );
		m_flTimeWeaponIdle = gpGlobals->time + 1.933;
		break;
	case 2:
		SendWeaponAnim( KNIFE_IDLE3 );
		m_flTimeWeaponIdle = gpGlobals->time + 0.64;
		break;
	}
}


// knives dropped by a throw (speed 1) don't respawn
void CKnife::CheckRespawn( void )
{
	if ( pev->speed == 0 )
		CBasePlayerItem::CheckRespawn( );
}

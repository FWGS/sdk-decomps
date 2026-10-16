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

// Wanted: the bear trap (weapon_beartrap) and the trap it throws
// (monster_beartrap), written from the satchel charge. The trap snaps shut
// on the first monster or player that steps into it, holds him in place
// and bites until he dies. The file name is ours: retail's code sits
// between barnacle.obj and bloater.obj, after the bear's file. weapons.h
// comes right after cbase.h (as in buffalo.cpp): this file then builds
// its own .pch with weapons.h in it, which puts the CBasePlayerItem and
// CBasePlayerWeapon inline defaults at the end of the object, as retail
// has them.

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "weapons.h"
#include "monsters.h"
#include "player.h"
#include "gamerules.h"

class CBearTrapWorld : public CBaseAnimating
{
	void Spawn( void );
	void Precache( void );
	void BounceSound( void );
	void Spring( void );		// name ours
	void HoldVictim( void );	// name ours

	void EXPORT TrapSlide( CBaseEntity *pOther );
	void EXPORT TrapThink( void );

public:
	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	void Deactivate( void );

	float m_NextDmgTime;		// next bite; 0 until the trap has closed
	int Victim_OldMovetype;		// the victim's movetype, given back when he dies
};
LINK_ENTITY_TO_CLASS( monster_beartrap, CBearTrapWorld );

TYPEDESCRIPTION	CBearTrapWorld::m_SaveData[] =
{
	DEFINE_FIELD( CBearTrapWorld, m_NextDmgTime, FIELD_TIME ),
	DEFINE_FIELD( CBearTrapWorld, Victim_OldMovetype, FIELD_INTEGER ),
};
IMPLEMENT_SAVERESTORE( CBearTrapWorld, CBaseAnimating );

//=========================================================
// Deactivate - do whatever it is we do to an orphaned
// satchel when we don't want it in the world anymore.
// (nothing calls it)
//=========================================================
void CBearTrapWorld::Deactivate( void )
{
	pev->solid = SOLID_NOT;
	UTIL_Remove( this );
}


void CBearTrapWorld :: Spawn( void )
{
	Precache( );
	// motor
	pev->movetype = MOVETYPE_BOUNCE;
	pev->solid = SOLID_TRIGGER;

	SET_MODEL(ENT(pev), "models/w_beartrap.mdl");
	UTIL_SetSize(pev, Vector( -4, -4, 0), Vector(4, 4, 4));
	UTIL_SetOrigin( pev, pev->origin );

	SetTouch( TrapSlide );
	SetThink( TrapThink );
	pev->nextthink = gpGlobals->time + 0.3;

	pev->gravity = 0.5;
	pev->friction = 0.8;

	pev->dmg = gSkillData.plrDmgBearTrap;
	pev->sequence = 0;	// open
}

// Wanted: the first call snaps the trap shut (sequence 1); the next one
// makes it a solid box that holds the victim and starts the bites
void CBearTrapWorld::Spring( void )
{
	if ( pev->sequence == 0 )
	{
		pev->sequence = 1;
		pev->frame = 0;
		ResetSequenceInfo( );
		EMIT_SOUND(ENT(pev), CHAN_WEAPON, "weapons/beartrap_sprung.wav", 1, ATTN_NORM);
		return;
	}

	UTIL_SetSize(pev, Vector( -16, -16, 0), Vector(16, 16, 48));
	pev->solid = SOLID_SLIDEBOX;
	UTIL_SetOrigin( pev, pev->origin );
	m_NextDmgTime = gpGlobals->time + 0.4;
	SetTouch( NULL );
}


void CBearTrapWorld::TrapSlide( CBaseEntity *pOther )
{
	if ( !pOther )
		return;

	if ( pOther->MyMonsterPointer() )
	{
		float flRadius;

		// don't catch the guy who threw it before it has landed
		if ( pOther->edict() == pev->owner && !pev->team )
			return;

		if ( pOther->IsPlayer() )
		{
			flRadius = 10;

			// nor his team mates
			if ( g_pGameRules->IsMultiplayer() && pev->owner && pOther->edict() != pev->owner )
			{
				if ( FStrEq( pOther->TeamID(), ((CBaseEntity *)GET_PRIVATE( pev->owner ))->TeamID() ) )
					return;
			}
		}
		else
		{
			flRadius = 32;
		}

		if ( !pOther->pev->takedamage )
			return;

		if ( ( pOther->Center() - pev->origin ).Length2D() < flRadius )
		{
			// monsters in a script walk over it
			if ( pOther->IsAlive() && ((CBaseMonster *)pOther)->m_IdealMonsterState == MONSTERSTATE_SCRIPT )
				return;

			pev->enemy = pOther->edict();
			Victim_OldMovetype = pOther->pev->movetype;
			Spring( );
		}
	}
	else
	{
		pev->team = 1;	// landed: now it can catch its owner too
		pev->gravity = 1;// normal gravity now

		// HACKHACK - On ground isn't always set, so look for ground underneath
		TraceResult tr;
		UTIL_TraceLine( pev->origin, pev->origin - Vector(0,0,10), ignore_monsters, edict(), &tr );

		if ( tr.flFraction < 1.0 )
		{
			// add a bit of static friction
			pev->velocity = pev->velocity * 0.7;
			pev->avelocity = pev->avelocity * 0.8;
			// play sliding sound, volume based on velocity
		}
		if ( !(pev->flags & FL_ONGROUND) && pev->velocity.Length2D() > 10 )
		{
			BounceSound();
		}
	}
}


void CBearTrapWorld :: TrapThink( void )
{
	StudioFrameAdvance( );
	pev->nextthink = gpGlobals->time + 0.1;

	if (!IsInWorld())
	{
		UTIL_Remove( this );
		return;
	}

	if (pev->waterlevel == 3)
	{
		pev->movetype = MOVETYPE_FLY;
		pev->velocity = pev->velocity * 0.8;
		pev->avelocity = pev->avelocity * 0.9;
		pev->velocity.z += 8;
	}
	else if (pev->waterlevel == 0)
	{
		pev->movetype = MOVETYPE_BOUNCE;
	}
	else
	{
		pev->velocity.z -= 8;
	}

	CBaseEntity *pVictim = CBaseEntity::Instance( pev->enemy );
	if ( !pVictim )
		return;

	HoldVictim( );

	if ( m_NextDmgTime && gpGlobals->time > m_NextDmgTime )
	{
		TraceResult tr;

		ClearMultiDamage( );
		UTIL_TraceModel( pVictim->Center(), pVictim->Center() + Vector( 0, 0, 1 ), 2, pVictim->edict(), &tr );
		pVictim->TraceAttack( pev, pev->dmg, gpGlobals->v_forward, &tr, DMG_NEVERGIB );
		if ( pev->owner )
			ApplyMultiDamage( pev, &pev->owner->v );
		else
			ApplyMultiDamage( pev, pev );

		// the first bite does plrDmgBearTrap, the next ones 30, one a second
		pev->dmg = 30;
		m_NextDmgTime = gpGlobals->time + 1.0;

		if ( pVictim->pev->health <= 0 || pVictim->pev->deadflag != DEAD_NO )
		{
			// let go of the body and fade away
			pVictim->pev->maxspeed = 0;
			pVictim->pev->movetype = Victim_OldMovetype;
			pev->enemy = NULL;
			SUB_StartFadeOut( );
		}
	}
}

// Wanted: keep the victim where he is; a player can't move at all
void CBearTrapWorld::HoldVictim( void )
{
	CBaseEntity *pVictim = CBaseEntity::Instance( pev->enemy );

	if ( pVictim && pVictim->pev->takedamage )
	{
		pVictim->pev->velocity = g_vecZero;

		if ( pVictim->IsPlayer() )
		{
			pVictim->pev->maxspeed = 0.001;
			pVictim->pev->movetype = MOVETYPE_NOCLIP;
		}
		else if ( pVictim->pev->deadflag == DEAD_NO )
		{
			pVictim->pev->movetype = MOVETYPE_NONE;
		}
	}
}

void CBearTrapWorld :: Precache( void )
{
	PRECACHE_MODEL("models/w_beartrapammo.mdl");
	PRECACHE_SOUND("weapons/g_bounce1.wav");
	PRECACHE_SOUND("weapons/g_bounce2.wav");
	PRECACHE_SOUND("weapons/g_bounce3.wav");
	PRECACHE_SOUND("weapons/beartrap_sprung.wav");
}

void CBearTrapWorld :: BounceSound( void )
{
	switch ( RANDOM_LONG( 0, 2 ) )
	{
	case 0:	EMIT_SOUND(ENT(pev), CHAN_VOICE, "weapons/g_bounce1.wav", 1, ATTN_NORM);	break;
	case 1:	EMIT_SOUND(ENT(pev), CHAN_VOICE, "weapons/g_bounce2.wav", 1, ATTN_NORM);	break;
	case 2:	EMIT_SOUND(ENT(pev), CHAN_VOICE, "weapons/g_bounce3.wav", 1, ATTN_NORM);	break;
	}
}


// sequences of models/v_beartrap.mdl (names ours)
enum beartrap_e {
	BEARTRAP_DRAW = 0,
	BEARTRAP_HOLSTER,
	BEARTRAP_IDLE,
	BEARTRAP_FIDGET,
	BEARTRAP_THROW,
};

class CBearTrap : public CBasePlayerWeapon	// name ours
{
public:
	void Spawn( void );
	void Precache( void );
	int iItemSlot( void ) { return 5; }
	int GetItemInfo(ItemInfo *p);
	void PrimaryAttack( void );
	BOOL Deploy( void );
	void Holster( int skiplocal = 0 );
	void WeaponIdle( void );

	float m_flNextFidget;	// name ours; never set, so the fidget never plays
};
LINK_ENTITY_TO_CLASS( weapon_beartrap, CBearTrap );

void CBearTrap::Spawn( )
{
	Precache( );
	m_iId = WEAPON_BEARTRAP;
	SET_MODEL(ENT(pev), "models/w_beartrapammo.mdl");

	m_iDefaultAmmo = 2;

	FallInit();// get ready to fall down.
}


void CBearTrap::Precache( void )
{
	PRECACHE_MODEL("models/v_beartrap.mdl");
	PRECACHE_MODEL("models/p_beartrap.mdl");
	PRECACHE_MODEL("models/w_beartrap.mdl");

	UTIL_PrecacheOther( "monster_beartrap" );
}


int CBearTrap::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "trap";
	p->iMaxAmmo1 = 10;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = 4;
	p->iPosition = 1;
	p->iFlags = ITEM_FLAG_SELECTONEMPTY | ITEM_FLAG_LIMITINWORLD | ITEM_FLAG_EXHAUSTIBLE;
	p->iId = m_iId = WEAPON_BEARTRAP;
	p->iWeight = -10;

	return 1;
}

BOOL CBearTrap::Deploy( )
{
	m_pPlayer->pev->viewmodel = MAKE_STRING("models/v_beartrap.mdl");
	m_pPlayer->pev->weaponmodel = MAKE_STRING("models/p_beartrap.mdl");
	SendWeaponAnim( BEARTRAP_DRAW );
	// use tripmine animations
	strcpy( m_pPlayer->m_szAnimExtention, "trip" );

	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 1.0;
	m_flTimeWeaponIdle = gpGlobals->time + RANDOM_FLOAT ( 10, 15 );
	return TRUE;
}


void CBearTrap::Holster( int skiplocal /* = 0 */ )
{
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;

	SendWeaponAnim( BEARTRAP_HOLSTER );
	EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "common/null.wav", 1.0, ATTN_NORM);

	if ( !m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] )
	{
		m_pPlayer->pev->weapons &= ~(1<<WEAPON_BEARTRAP);
		SetThink( DestroyItem );
		pev->nextthink = gpGlobals->time + 0.1;
	}
}


void CBearTrap::PrimaryAttack( void )
{
	if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] > 0)
	{
		Vector vecSrc = m_pPlayer->pev->origin;

		Vector vecThrow = gpGlobals->v_forward * 274 + m_pPlayer->pev->velocity;

		CBaseEntity *pTrap = Create( "monster_beartrap", vecSrc, Vector( 0, 0, 0), m_pPlayer->edict() );
		pTrap->pev->velocity = vecThrow;
		pTrap->pev->avelocity.y = RANDOM_FLOAT( -400, 400 );

		SendWeaponAnim( BEARTRAP_THROW );

		// player "shoot" animation
		m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

		m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType]--;

		m_flNextPrimaryAttack = gpGlobals->time + 1.0;
		m_flNextSecondaryAttack = gpGlobals->time + 0.5;
		m_flTimeWeaponIdle = gpGlobals->time + 0.5;

		pev->team = 1;	// just thrown: WeaponIdle draws the next one
	}
}


void CBearTrap::WeaponIdle( void )
{
	m_pPlayer->m_iFOV = 0;

	if (m_flTimeWeaponIdle > gpGlobals->time)
		return;

	if ( !m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] )
	{
		Holster( );
		return;
	}

	if ( gpGlobals->time < m_flNextFidget )
	{
		SendWeaponAnim( BEARTRAP_FIDGET );
		m_flTimeWeaponIdle = gpGlobals->time + 1.0;
		m_flNextFidget = gpGlobals->time + RANDOM_FLOAT ( 10, 15 );
	}
	else
	{
		m_flTimeWeaponIdle = gpGlobals->time + 1.0;

		if ( !pev->team )
			SendWeaponAnim( BEARTRAP_IDLE );
		else
			SendWeaponAnim( BEARTRAP_DRAW );

		pev->team = 0;
	}

	// use tripmine animations
	strcpy( m_pPlayer->m_szAnimExtention, "trip" );

	m_flNextPrimaryAttack = gpGlobals->time + 0.5;
	m_flNextSecondaryAttack = gpGlobals->time + 0.5;
}

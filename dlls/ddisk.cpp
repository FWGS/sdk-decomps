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
// ddisk.cpp - the energy disk, a screaming saw blade that
// steers towards the nearest enemy and bounces from one
// victim to the next (file name ours)
//=========================================================

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "soundent.h"

class CClipDisk : public CRocket
{
public:
	void Spawn( void );
	void Precache( void );
	BOOL CanBeTractored( void ) { return FALSE; }
	const char *GetKillDesc( void ) { return !m_iszKillDesc ? "an energy disk" : STRING( m_iszKillDesc ); }
	const char *GetKillVerb( BOOL fGibbed, int &iHitGroup ) { return "clipped"; }
	int  IRelationship( CBaseEntity *pTarget );

	int		Save( CSave &save );
	int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	void EXPORT DiskTouch( CBaseEntity *pOther );
	void EXPORT DiskThink( void );

	float	m_flTurnRate;
};

LINK_ENTITY_TO_CLASS( energy_disk, CClipDisk );

TYPEDESCRIPTION	CClipDisk::m_SaveData[] =
{
	DEFINE_FIELD( CClipDisk, m_flTurnRate, FIELD_FLOAT ),
};

IMPLEMENT_SAVERESTORE( CClipDisk, CRocket );

void CClipDisk::Spawn( void )
{
	Precache();

	SET_MODEL( ENT(pev), "models/disk.mdl" );
	UTIL_SetSize( pev, Vector( -16, -16, 0 ), Vector( 16, 16, 1 ) );

	pev->body = 0;
	pev->rendermode = kRenderTransAdd;
	pev->renderamt = 255;
	pev->movetype = MOVETYPE_FLYMISSILE;
	pev->solid = SOLID_BBOX;
	m_flFieldOfView = -0.7;
	pev->iuser1 = 0;	// the next slot of m_hOldEnemy
	pev->dmg = 140;
	m_flTurnRate = 0;

	SetTouch( DiskTouch );
	SetThink( DiskThink );
	pev->nextthink = gpGlobals->time + 0.1;
	pev->dmgtime = gpGlobals->time + 1;

	EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "ambience/screammachine.wav", 1, 0.3, 0, PITCH_NORM );
}

void CClipDisk::Precache( void )
{
	PRECACHE_MODEL( "models/disk.mdl" );
	PRECACHE_SOUND( "ambience/screammachine.wav" );
	PRECACHE_SOUND( "weapons/shock_discharge.wav" );
}

//=========================================================
// DiskTouch - cuts what it touches; a wall or a shield
// stops it, anything else is remembered (so it won't
// come back to it) and the disk flies on
//=========================================================
void CClipDisk::DiskTouch( CBaseEntity *pOther )
{
	if ( pOther->pev->takedamage )
	{
		TraceResult tr = UTIL_GetGlobalTrace();
		entvars_t *pevAttacker = pev;

		if ( m_hActivator != NULL )
			pevAttacker = m_hActivator->pev;

		ClearMultiDamage();
		pOther->TraceAttack( pevAttacker, pev->dmg, pev->velocity.Normalize(), &tr, DMG_ENERGYBEAM | DMG_NOGIB | 0x2000000 | 0x4000000 );
		ApplyMultiDamage( pev, pevAttacker );
	}

	if ( pOther->IsBSPModel() || pOther->Shielded() )
	{
		STOP_SOUND( ENT(pev), CHAN_VOICE, "ambience/screammachine.wav" );
		EMIT_SOUND( ENT(pev), CHAN_WEAPON, "weapons/shock_discharge.wav", 1, ATTN_NORM );

		pev->avelocity = pev->velocity = g_vecZero;
		SUB_StartFadeOut();
		return;
	}

	if ( m_hEnemy == pOther )
		m_hEnemy = NULL;

	if ( pev->owner )
	{
		m_hOldEnemy[ pev->iuser1++ ] = CBaseEntity::Instance( pev->owner );
		if ( pev->iuser1 >= MAX_OLD_ENEMIES )
			pev->iuser1 = 0;
	}

	pev->owner = pOther->edict();
}

//=========================================================
// DiskThink - homes in on the best visible enemy, turning
// faster the longer it turns one way, and banks into the
// turn
//=========================================================
void CClipDisk::DiskThink( void )
{
	StudioFrameAdvance();
	pev->nextthink = gpGlobals->time + 0.1;
	pev->skin = !pev->skin;

	if ( m_hEnemy != NULL && m_hEnemy->IsAlive() )
	{
		m_vecEnemyLKP = m_hEnemy->Center();
	}
	else
	{
		Look( 1024 );
		m_hEnemy = BestVisibleEnemy();
		m_vecEnemyLKP = pev->origin + pev->velocity;
	}

	if ( pev->dmgtime < gpGlobals->time )
		CSoundEnt::InsertSound( bits_SOUND_DANGER, pev->origin + pev->velocity * 0.25, pev->velocity.Length() * 0.5, 0.1 );

	float flOldYaw = pev->angles.y;
	Vector vecAngles = UTIL_VecToAngles( m_vecEnemyLKP - pev->origin );

	float flDiff = UTIL_AngleDiff( vecAngles.y, pev->angles.y );
	if ( fabs( flDiff - m_flTurnRate ) <= 10 )
	{
		pev->angles.y = vecAngles.y;
	}
	else
	{
		float flDir = ( flDiff + m_flTurnRate < 0 ) ? -1 : 1;
		if ( flDir != ( ( m_flTurnRate < 0 ) ? -1 : 1 ) )
			m_flTurnRate = 0;
		pev->angles.y = pev->angles.y + flDir * 10 + m_flTurnRate;
		m_flTurnRate += flDir * 0.2;
	}

	flDiff = UTIL_AngleDiff( vecAngles.x, pev->angles.x );
	if ( fabs( flDiff ) <= 10 )
		pev->angles.x = vecAngles.x;
	else
		pev->angles.x += ( ( flDiff < 0 ) ? -1 : 1 ) * 10;

	float flRoll = UTIL_AngleDiff( pev->angles.y, flOldYaw ) * -2;
	flDiff = UTIL_AngleDiff( flRoll, pev->angles.z );
	if ( fabs( flDiff ) <= 4 )
		pev->angles.z = flRoll;
	else
		pev->angles.z += ( ( flDiff < 0 ) ? -1 : 1 ) * 4;

	UTIL_MakeAimVectors( pev->angles );
	pev->velocity = gpGlobals->v_forward * pev->speed;
}

//=========================================================
// IRelationship - hates what its owner hates, but never
// machines, its last victims or the one it is cutting
//=========================================================
int CClipDisk::IRelationship( CBaseEntity *pTarget )
{
	int iClass = pTarget->Classify();

	if ( iClass == 20 || iClass == CLASS_MACHINE )	// 20: a class the mod added
		return R_NO;

	if ( pTarget->edict() == pev->owner )
		return R_NO;

	for ( int i = 0; i < MAX_OLD_ENEMIES; i++ )
	{
		if ( m_hOldEnemy[i] == pTarget )
			return R_NO;
	}

	if ( m_hActivator != NULL )
	{
		CBaseMonster *pMonster = m_hActivator->MyMonsterPointer();
		if ( pMonster )
			return pMonster->IRelationship( pTarget );
	}

	return R_NO;
}

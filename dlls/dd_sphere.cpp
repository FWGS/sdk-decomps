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
// dd_sphere.cpp - the Biocide sphere and cube, flying
// guardians that fire particle beams, pulse lasers and
// quantum torpedoes (file name ours)
//=========================================================

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"schedule.h"
#include	"flyingobject.h"
#include	"effects.h"
#include	"weapons.h"
#include	"decals.h"

//=========================================================
// CBeamWeapon - a particle beam that hangs off a sphere or
// cube for a second, keeping its place relative to the
// owner, and sweeps from one random point near the target
// to another, hurting whatever it hits
//=========================================================
class CBeamWeapon : public CBeam
{
public:
	void Spawn( void );
	void Precache( void );
	const char *GetKillDesc( void ) { return "a particle beam"; }

	int		Save( CSave &save );
	int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	void EXPORT WeaponThink( void );

	EHANDLE	m_hLinked;			// the monster that fired it
	Vector	m_vecRelativePos;	// where it sits, in the owner's forward, right and up
	Vector	m_point1;			// the point the beam aims at
	Vector	m_point2;			// how far that point moves every tenth of a second
};

LINK_ENTITY_TO_CLASS( beamweapon, CBeamWeapon );

TYPEDESCRIPTION	CBeamWeapon::m_SaveData[] =
{
	DEFINE_FIELD( CBeamWeapon, m_hLinked, FIELD_EHANDLE ),
	DEFINE_FIELD( CBeamWeapon, m_vecRelativePos, FIELD_VECTOR ),
	DEFINE_FIELD( CBeamWeapon, m_point1, FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( CBeamWeapon, m_point2, FIELD_POSITION_VECTOR ),
};

IMPLEMENT_SAVERESTORE( CBeamWeapon, CBeam );

void CBeamWeapon::Spawn( void )
{
	Precache();

	BeamInit( "sprites/pbeam.spr", 20 );
	PointsInit( pev->origin, pev->angles );	// the creator passes the offset as the origin and the target as the angles
	pev->solid = SOLID_NOT;

	m_vecRelativePos = pev->origin;

	pev->renderamt = 200;
	SetScrollRate( 35 );
	pev->dmg = 4;
	pev->pain_finished = gpGlobals->time + 1;	// lifetime

	Vector vecTarget = pev->angles;

	UTIL_MakeVectors( Vector( RANDOM_FLOAT( -180, 180 ), RANDOM_FLOAT( -180, 180 ), RANDOM_FLOAT( -180, 180 ) ) );
	m_point1 = vecTarget + gpGlobals->v_forward * RANDOM_FLOAT( 0, 128 );
	Vector vecEnd = vecTarget + gpGlobals->v_forward * -RANDOM_FLOAT( 0, 128 );
	m_point2 = vecEnd - m_point1;
	m_point2 = m_point2.Normalize() * m_point2.Length() * 0.1;

	m_hLinked = CBaseEntity::Instance( pev->owner );

	SetThink( WeaponThink );
	pev->nextthink = gpGlobals->time;
}

void CBeamWeapon::Precache( void )
{
	PRECACHE_MODEL( "sprites/pbeam.spr" );
}

void CBeamWeapon::WeaponThink( void )
{
	if ( m_hLinked != NULL && m_hLinked->IsAlive() && pev->pain_finished >= gpGlobals->time )
	{
		pev->nextthink = gpGlobals->time + 0.05;

		UTIL_MakeVectors( m_hLinked->pev->angles );
		pev->origin = m_hLinked->pev->origin + gpGlobals->v_forward * m_vecRelativePos.x + gpGlobals->v_right * m_vecRelativePos.y + gpGlobals->v_up * -m_vecRelativePos.z;

		if ( pev->dmgtime > gpGlobals->time )
			return;

		pev->dmgtime = gpGlobals->time + 0.1;

		Vector vecDir;
		TraceResult tr;

		m_point1 = m_point2 + m_point1;
		vecDir = ( m_point1 - pev->origin ).Normalize();
		Vector vecEnd = pev->origin + vecDir * 4096;

		UTIL_TraceLine( pev->origin, vecEnd, dont_ignore_monsters, pev->owner, &tr );
		pev->angles = tr.vecEndPos;

		if ( tr.pHit )
		{
			CBaseEntity *pEntity = CBaseEntity::Instance( tr.pHit );

			if ( pEntity->pev->takedamage )
			{
				ClearMultiDamage();
				pEntity->TraceAttack( m_hLinked->pev, pev->dmg, vecDir, &tr, DMG_ENERGYBEAM );
				ApplyMultiDamage( pev, m_hLinked->pev );
			}

			if ( pEntity->IsBSPModel() )
				UTIL_DecalTrace( &tr, DECAL_BIGSHOT1 + RANDOM_LONG( 0, 4 ) );

			UTIL_Sparks( tr.vecEndPos );
		}
	}
	else
	{
		UTIL_Remove( this );
	}
}


//=========================================================
// monster_ddisphere - the Biocide sphere: one particle beam
// and two pulse lasers, each with a target of its own
//=========================================================
class CDSphere : public CBaseFlyingObject
{
public:
	void Spawn( void );
	void Precache( void );
	int  Classify( void ) { return CLASS_HUMAN_MILITARY; }
	const char *GetKillDesc( void ) { return "a Biocide sphere"; }
	const char *GetKillVerb( BOOL fGibbed, int &iHitGroup ) { return DamageVerb( DMG_BLAST, fGibbed ); }
	float ChangeYaw( int speed ) { return 0; }
	void SetYawSpeed( void ) { pev->yaw_speed = 0; }
	BOOL FacingIdeal( void ) { return TRUE; }
	int  GetBattlePower( void );
	void RunAttack( void );
	void Killed( entvars_t *pevAttacker, int iGib );
	int  IRelationship( CBaseEntity *pTarget );

	int		Save( CSave &save );
	int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	EHANDLE	m_hTargets[7];		// one target per weapon (pev->weapons of them)
	float	m_flNextFire[7];
};

LINK_ENTITY_TO_CLASS( monster_ddisphere, CDSphere );

TYPEDESCRIPTION	CDSphere::m_SaveData[] =
{
	DEFINE_ARRAY( CDSphere, m_hTargets, FIELD_EHANDLE, 7 ),
	DEFINE_ARRAY( CDSphere, m_flNextFire, FIELD_TIME, 7 ),
};

IMPLEMENT_SAVERESTORE( CDSphere, CBaseFlyingObject );

void CDSphere::Spawn( void )
{
	Precache();
	CBaseFlyingObject::Spawn();

	SET_MODEL( ENT(pev), "models/cube.mdl" );
	pev->body = 1;
	UTIL_SetSize( pev, Vector( -24, -24, -24 ), Vector( 24, 24, 24 ) );

	pev->dmg = 50;
	pev->max_health = 100;
	pev->health = 100;
	pev->sequence = 0;
	m_bloodColor = DONT_BLEED;

	for ( int i = 0; i < 7; i++ )
		m_hTargets[i] = NULL;
}

void CDSphere::Precache( void )
{
	PRECACHE_MODEL( "models/cube.mdl" );
	m_iBodyGibs = PRECACHE_MODEL( "models/metalplategibs_dark.mdl" );
	UTIL_PrecacheOther( "beamweapon" );

	pev->weapons = 3;
}

int CDSphere::GetBattlePower( void )
{
	return ( pev->max_health * 0.1 ) * 94;
}

//=========================================================
// RunAttack - spins slowly while hovering, faster the
// slower it moves; fires the particle beam (weapon 0) and
// the two pulse lasers at their own targets
//=========================================================
void CDSphere::RunAttack( void )
{
	int i;
	Vector vecDir, vecAngles;

	if ( !IsMoving() )
	{
		pev->avelocity.y = 10;
		m_flGroundSpeed = 100;
	}
	else if ( m_flGroundSpeed < 320 )
	{
		pev->avelocity.y = 144 - m_flGroundSpeed;
		if ( pev->avelocity.y < 0 )
			pev->avelocity.y = 0;
		m_flGroundSpeed = UTIL_Approach( 320, m_flGroundSpeed, 10 );
	}

	if ( ( m_MonsterState != MONSTERSTATE_NONE && m_MonsterState != MONSTERSTATE_PRONE && m_MonsterState != MONSTERSTATE_DEAD ) &&
		 ( !FNullEnt( FIND_CLIENT_IN_PVS( edict() ) ) || ( m_MonsterState == MONSTERSTATE_COMBAT ) ) )
	{
		pev->iuser1 = 1;	// IRelationship spreads the weapons over the enemies while this is set

		for ( i = 0; i < 3; i++ )
		{
			if ( m_flNextFire[i] <= gpGlobals->time )
			{
				if ( m_hTargets[i] == NULL || !m_hTargets[i]->IsAlive() || !FVisible( m_hTargets[i] ) )
					m_hTargets[i] = BestVisibleEnemy();

				if ( m_hTargets[i] != NULL )
				{
					vecDir = ( m_hTargets[i]->BodyTarget( pev->origin ) - Center() ).Normalize();
					vecAngles = UTIL_VecToAngles( vecDir );

					if ( i != 0 )
					{
						Vector vecSrc, vecEnd;	// sic: unused; they keep Normalize's Vector( 0, 0, 1 ) out of line, as retail
						UTIL_MakeVectors( vecAngles + Vector( RANDOM_FLOAT( -80, 80 ), RANDOM_FLOAT( -80, 80 ), 0 ) );

						CLaserPulse *pPulse = CLaserPulse::Shoot( pev->origin + gpGlobals->v_forward * 20, edict(), Vector( 70, 140, 255 ), 0, 0.25, 0 );
						pPulse->pev->velocity = vecDir * RANDOM_FLOAT( 800, 1200 );
						pPulse->pev->dmg = 9;
						pPulse->m_iszKillDesc = MAKE_STRING( "a particle projector cannon" );

						m_flNextFire[i] = gpGlobals->time + RANDOM_FLOAT( 0.2, 0.4 );
					}
					else
					{
						UTIL_MakeVectors( vecAngles - pev->angles );
						CBaseEntity::Create( "beamweapon", gpGlobals->v_forward * 16, m_hTargets[0]->BodyTarget( pev->origin ), edict() );
						m_flNextFire[0] = gpGlobals->time + RANDOM_FLOAT( 1.2, 1.4 );
					}
				}
			}
		}

		pev->iuser1 = 0;
	}
}

void CDSphere::Killed( entvars_t *pevAttacker, int iGib )
{
	if ( !( m_afMemory & bits_MEMORY_KILLED ) )
	{
		// smoke trails from attachments 3 and 4
		MESSAGE_BEGIN( MSG_ALL, SVC_TEMPENTITY );
			WRITE_BYTE( TE_BEAMFOLLOW );
			WRITE_SHORT( entindex() | 0x3000 );
			WRITE_SHORT( g_sModelIndexSmokePuff );
			WRITE_BYTE( 20 ); // life
			WRITE_BYTE( 5 );  // width
			WRITE_BYTE( 255 );   // r
			WRITE_BYTE( 128 );   // g
			WRITE_BYTE( 16 );   // b
			WRITE_BYTE( 255 );	// brightness
		MESSAGE_END();

		MESSAGE_BEGIN( MSG_ALL, SVC_TEMPENTITY );
			WRITE_BYTE( TE_BEAMFOLLOW );
			WRITE_SHORT( entindex() | 0x4000 );
			WRITE_SHORT( g_sModelIndexSmokePuff );
			WRITE_BYTE( 20 ); // life
			WRITE_BYTE( 5 );  // width
			WRITE_BYTE( 255 );   // r
			WRITE_BYTE( 128 );   // g
			WRITE_BYTE( 16 );   // b
			WRITE_BYTE( 255 );	// brightness
		MESSAGE_END();
	}

	CBaseFlyingObject::Killed( pevAttacker, iGib );
}

//=========================================================
// IRelationship - while it picks targets, an enemy that
// is already some weapon's target counts for less, so
// the weapons spread over the enemies in sight
//=========================================================
int CDSphere::IRelationship( CBaseEntity *pTarget )
{
	int iRelationship = CBaseMonster::IRelationship( pTarget );

	if ( pev->iuser1 && iRelationship >= R_DL )
	{
		int iRating = R_NM;

		for ( int i = 0; i < pev->weapons; i++ )
		{
			if ( m_hTargets[i] == pTarget )
			{
				if ( --iRating < R_DL )
					return R_NO;
			}
		}

		return iRating;
	}

	return iRelationship;
}


//=========================================================
// monster_ddicube - the Biocide cube: four pulse lasers,
// two particle beams and a quantum torpedo (class name ours)
//=========================================================
class CDCube : public CDSphere
{
public:
	void Spawn( void );
	void Precache( void );
	const char *GetKillDesc( void ) { return "a Biocide cube"; }
	BOOL CanBeRepulsed( bool fForce ) { return fForce; }
	int  GetBattlePower( void );
	void RunAttack( void );
	void Killed( entvars_t *pevAttacker, int iGib );
};

LINK_ENTITY_TO_CLASS( monster_ddicube, CDCube );

void CDCube::Spawn( void )
{
	Precache();
	CBaseFlyingObject::Spawn();

	SET_MODEL( ENT(pev), "models/cube.mdl" );
	pev->body = 0;
	UTIL_SetSize( pev, Vector( -64, -64, -64 ), Vector( 64, 64, 64 ) );

	pev->max_health = 400;
	pev->health = 400;
	pev->sequence = 0;
	m_flHeight = 256;
	pev->dmg = 200;
	m_bloodColor = DONT_BLEED;

	for ( int i = 0; i < 7; i++ )
		m_hTargets[i] = NULL;
}

void CDCube::Precache( void )
{
	PRECACHE_MODEL( "models/cube.mdl" );
	m_iBodyGibs = PRECACHE_MODEL( "models/metalplategibs_dark.mdl" );
	m_iSpriteTexture = PRECACHE_MODEL( "sprites/white.spr" );
	PRECACHE_MODEL( "sprites/qtorp.spr" );
	UTIL_PrecacheOther( "beamweapon" );
	UTIL_PrecacheOther( "rocket" );
	PRECACHE_SOUND( "weapons/mortarhit.wav" );

	pev->weapons = 7;
}

int CDCube::GetBattlePower( void )
{
	return ( pev->max_health * 0.1 ) * 460;
}

//=========================================================
// RunAttack - weapons 0-3 are pulse lasers fired from a
// random point of the cube, 4 and 5 particle beams from
// the face that looks at the target, 6 a quantum torpedo
//=========================================================
void CDCube::RunAttack( void )
{
	int i;
	Vector vecSrc, vecDir, vecAngles, vecOffset;

	if ( !IsMoving() || pev->velocity == Vector( 0, 0, 0 ) )
	{
		pev->avelocity.y = 10;
		m_flGroundSpeed = 100;
	}
	else if ( m_flGroundSpeed < 240 )
	{
		pev->avelocity.y = 144 - m_flGroundSpeed;
		if ( pev->avelocity.y < 0 )
			pev->avelocity.y = 0;
		m_flGroundSpeed = UTIL_Approach( 240, m_flGroundSpeed, 10 );
	}

	pev->iuser1 = 1;	// IRelationship spreads the weapons over the enemies while this is set

	for ( i = 0; i < 7; i++ )
	{
		if ( m_flNextFire[i] <= gpGlobals->time )
		{
			if ( m_hTargets[i] == NULL || !m_hTargets[i]->IsAlive() || !FVisible( m_hTargets[i] ) )
			{
				m_hTargets[i] = NULL;	// sic: cleared, then set
				m_hTargets[i] = BestVisibleEnemy();
			}

			if ( m_hTargets[i] != NULL )
			{
				if ( i < 4 )
				{
					UTIL_MakeVectors( pev->angles );
					vecSrc = pev->origin + gpGlobals->v_forward * RANDOM_FLOAT( -56, 56 ) + gpGlobals->v_up * RANDOM_FLOAT( -56, 56 ) + gpGlobals->v_right * RANDOM_FLOAT( -56, 56 );
					Vector vecShootDir = ( m_hTargets[i]->BodyTarget( pev->origin ) - vecSrc ).Normalize();

					CLaserPulse *pPulse = CLaserPulse::Shoot( vecSrc, edict(), Vector( 70, 140, 255 ), 0, 0.3, 0 );
					pPulse->pev->velocity = vecShootDir * RANDOM_FLOAT( 800, 1200 );
					pPulse->pev->dmg = 20;
					pPulse->m_iszKillDesc = MAKE_STRING( "a particle projector cannon" );

					m_flNextFire[i] = gpGlobals->time + RANDOM_FLOAT( 0.3, 0.6 );
				}
				else if ( i < 6 )
				{
					Vector vecDir, vecOffset;
					vecDir = ( m_hTargets[i]->BodyTarget( pev->origin ) - Center() ).Normalize();
					vecAngles = UTIL_VecToAngles( vecDir );
					// which face of the cube looks at the target
					int iFace = 0;
					float flYaw = UTIL_AngleDiff( vecAngles.y, pev->angles.y );
					float flPitch = UTIL_AngleDiff( vecAngles.x, pev->angles.x );

					if ( flPitch > 75 )
						iFace = 4;
					else if ( flPitch < -75 )
						iFace = 5;
					else
					{
						flYaw = UTIL_AngleMod( flYaw + 45 );
						if ( flYaw > 270 )
							iFace = 3;
						else if ( flYaw > 180 )
							iFace = 2;
						else if ( flYaw > 90 )
							iFace = 1;
					}

					switch ( iFace )
					{
					case 1:
						UTIL_MakeVectors( pev->angles + Vector( 0, 90, 0 ) );
						break;
					case 2:
						UTIL_MakeVectors( pev->angles + Vector( 0, 180, 0 ) );
						break;
					case 3:
						UTIL_MakeVectors( pev->angles + Vector( 0, 270, 0 ) );
						break;
					case 4:
						UTIL_MakeVectors( pev->angles + Vector( 90, 0, 0 ) );
						break;
					case 5:
						UTIL_MakeVectors( pev->angles - Vector( 90, 0, 0 ) );
						break;
					default:
						UTIL_MakeVectors( pev->angles );
						break;
					}

					vecOffset = gpGlobals->v_forward * 56 + gpGlobals->v_right * RANDOM_FLOAT( -48, 48 ) + gpGlobals->v_up * RANDOM_FLOAT( -48, 48 );

					CBaseEntity *pBeam = CBaseEntity::Create( "beamweapon", vecOffset, m_hTargets[i]->BodyTarget( pev->origin ), edict() );
					pBeam->pev->scale = 40;
					pBeam->pev->dmg = 10;

					m_flNextFire[i] = gpGlobals->time + RANDOM_FLOAT( 1.2, 2.0 );
				}
				else
				{
					vecDir = ( m_hTargets[i]->BodyTarget( pev->origin ) - Center() ).Normalize();
					vecAngles = UTIL_VecToAngles( vecDir );
					vecAngles = vecAngles + Vector( RANDOM_FLOAT( -90, 90 ), RANDOM_FLOAT( -90, 90 ), 0 );
					UTIL_MakeVectors( vecAngles );

					CRocket *pRocket = CRocket::CreateRocket( pev->origin, vecAngles, edict(), Vector( 70, 140, 255 ), MAKE_STRING( "sprites/qtorp.spr" ) );
					if ( pRocket )
					{
						pRocket->SetThink( &CRocket::TargetThink );
						pRocket->pev->speed = 1500;
						pRocket->pev->renderamt = 255;
						pRocket->pev->rendermode = kRenderTransAdd;
						pRocket->m_flSeekTime = gpGlobals->time + 0.2;
						pRocket->pev->velocity = gpGlobals->v_forward * 256;
						pRocket->m_hEnemy = m_hTargets[i];
						pRocket->pev->dmg = 100;
						pRocket->pev->scale = 0.5;
						pRocket->pev->spawnflags |= 8;
						pRocket->m_iszKillDesc = MAKE_STRING( "a quantum torpedo" );
					}

					m_flNextFire[i] = gpGlobals->time + RANDOM_FLOAT( 5, 7 );
				}
			}
		}
	}

	pev->iuser1 = 0;
}

void CDCube::Killed( entvars_t *pevAttacker, int iGib )
{
	if ( !( m_afMemory & bits_MEMORY_KILLED ) )
	{
		// smoke trails from attachments 1 and 2
		MESSAGE_BEGIN( MSG_ALL, SVC_TEMPENTITY );
			WRITE_BYTE( TE_BEAMFOLLOW );
			WRITE_SHORT( entindex() | 0x1000 );
			WRITE_SHORT( g_sModelIndexSmokePuff );
			WRITE_BYTE( 20 ); // life
			WRITE_BYTE( 5 );  // width
			WRITE_BYTE( 255 );   // r
			WRITE_BYTE( 128 );   // g
			WRITE_BYTE( 16 );   // b
			WRITE_BYTE( 255 );	// brightness
		MESSAGE_END();

		MESSAGE_BEGIN( MSG_ALL, SVC_TEMPENTITY );
			WRITE_BYTE( TE_BEAMFOLLOW );
			WRITE_SHORT( entindex() | 0x2000 );
			WRITE_SHORT( g_sModelIndexSmokePuff );
			WRITE_BYTE( 20 ); // life
			WRITE_BYTE( 5 );  // width
			WRITE_BYTE( 255 );   // r
			WRITE_BYTE( 128 );   // g
			WRITE_BYTE( 16 );   // b
			WRITE_BYTE( 255 );	// brightness
		MESSAGE_END();
	}

	CBaseFlyingObject::Killed( pevAttacker, iGib );
}

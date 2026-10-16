/***
*
*	Copyright (c) 1999, Valve LLC. All rights reserved.
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
// POV: monster_relationship, an invisible monster that
// changes how monsters get along: it classifies itself as
// the enemy of "monstarget"'s class (or of its own enemy's)
// and gives the monsters "monsrelaplyto" picks the
// relationship "monsrelation". No map uses it. Class and
// field names are POV's (save table); file name, flag names
// and the comments' names are ours.
//=========================================================
#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"schedule.h"

#define	SF_RELATION_INVULNERABLE		1	// name ours
#define	SF_RELATION_NOTSOLID			2	// name ours

class CMonstRelation : public CBaseMonster
{
public:
	void Spawn( void );
	void Precache( void );
	void SetYawSpeed( void );
	int  Classify ( void );
	void HandleAnimEvent( MonsterEvent_t *pEvent );
	int ISoundMask ( void );
	void KeyValue( KeyValueData *pkvd );
	void TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType);
	int TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );
	int BloodColor( void );
	void GibMonster( void );
	int IRelationship( CBaseEntity *pTarget );

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	int			m_iType;			// 0 the enemy of the target's class, 2 alien military, 3 human military
	int			m_iRelantionship;	// R_* given to the monsters below
	int			m_iRelantionAplyTo;	// 0 living things, 1 humans, 2 aliens
	string_t	m_iszEnemyOf;
};
LINK_ENTITY_TO_CLASS( monster_relationship, CMonstRelation );

TYPEDESCRIPTION	CMonstRelation::m_SaveData[] =
{
	DEFINE_FIELD( CMonstRelation, m_iType, FIELD_INTEGER ),
	DEFINE_FIELD( CMonstRelation, m_iRelantionship, FIELD_INTEGER ),
	DEFINE_FIELD( CMonstRelation, m_iRelantionAplyTo, FIELD_INTEGER ),
	DEFINE_FIELD( CMonstRelation, m_iszEnemyOf, FIELD_STRING ),
};

IMPLEMENT_SAVERESTORE( CMonstRelation, CBaseMonster );

void CMonstRelation :: KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "monstype"))
	{
		m_iType = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "monsrelation"))
	{
		m_iRelantionship = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "monsrelaplyto"))
	{
		m_iRelantionAplyTo = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "monstarget"))
	{
		m_iszEnemyOf = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CBaseMonster::KeyValue( pkvd );
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CMonstRelation :: SetYawSpeed ( void )
{
	int ys;

	switch ( m_Activity )
	{
	case ACT_IDLE:
	default:
		ys = 90;
	}

	pev->yaw_speed = ys;
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CMonstRelation :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	switch( pEvent->event )
	{
	case 0:
	default:
		CBaseMonster::HandleAnimEvent( pEvent );
		break;
	}
}

//=========================================================
// ISoundMask - can't hear.
//=========================================================
int CMonstRelation :: ISoundMask ( void )
{
	return	NULL;
}

//=========================================================
// Spawn
//=========================================================
void CMonstRelation :: Spawn()
{
	Precache();

	SET_MODEL( ENT(pev), "models/player.mdl" );
	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	pev->movetype		= MOVETYPE_NONE;
	m_bloodColor		= DONT_BLEED;
	m_flFieldOfView		= 0.5;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState		= MONSTERSTATE_NONE;

	MonsterInit();

	if ( pev->spawnflags & SF_RELATION_INVULNERABLE )
		pev->health = 80000;

	if ( pev->spawnflags & SF_RELATION_NOTSOLID )
	{
		pev->solid = SOLID_NOT;
		pev->takedamage = DAMAGE_NO;
	}
	else
	{
		pev->solid = SOLID_SLIDEBOX;
		pev->takedamage = DAMAGE_YES;
	}
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CMonstRelation :: Precache()
{
	PRECACHE_MODEL( "models/player.mdl" );
}

void CMonstRelation :: TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType)
{
	CBaseMonster::TraceAttack( pevAttacker, flDamage, vecDir, ptr, bitsDamageType );
}

int CMonstRelation :: TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	if ( pev->spawnflags & SF_RELATION_INVULNERABLE )
	{
		pev->health = 80000;

		if ( flDamage > 0 )
			SetConditions( bits_COND_LIGHT_DAMAGE );

		if ( flDamage >= 20 )
			SetConditions( bits_COND_HEAVY_DAMAGE );
	}

	return CBaseMonster::TakeDamage( pevInflictor, pevAttacker, flDamage, bitsDamageType );
}

int CMonstRelation :: BloodColor( void )
{
	return DONT_BLEED;
}

void CMonstRelation :: GibMonster( void )
{
}

//=========================================================
// Classify - the enemy of the target's class
//=========================================================
int	CMonstRelation :: Classify ( void )
{
	if ( m_iType == 0 )
	{
		int iClass = CLASS_NONE;

		if ( !FStringNull( m_iszEnemyOf ) )
		{
			edict_t *pentTarget = FIND_ENTITY_BY_TARGETNAME( NULL, STRING( m_iszEnemyOf ) );

			// sic: the message is printed when the target was found
			if ( !FNullEnt( pentTarget ) )
				ALERT( at_console, "monster_relationship: didn't found %s!\n", STRING( m_iszEnemyOf ) );
			else
			{
				CBaseEntity *pTarget = CBaseEntity::Instance( pentTarget );

				if ( pTarget )
					iClass = pTarget->Classify();
				else
					ALERT( at_console, "monster_relationship: error in instance of %s!\n", STRING( m_iszEnemyOf ) );
			}
		}
		else if ( m_hEnemy != NULL )
			iClass = m_hEnemy->Classify();

		if ( UTIL_IsAlienClass( iClass ) )
			return CLASS_HUMAN_MILITARY;

		if ( UTIL_IsHumanClass( iClass ) )
			return CLASS_ALIEN_MILITARY;
	}
	else if ( m_iType == 2 )
		return CLASS_ALIEN_MILITARY;
	else if ( m_iType == 3 )
		return CLASS_HUMAN_MILITARY;

	return CLASS_NONE;
}

int CMonstRelation :: IRelationship( CBaseEntity *pTarget )
{
	BOOL fApply;

	if ( m_iRelantionAplyTo == 0 )
		fApply = UTIL_IsLivingClass( pTarget->Classify() );
	else if ( m_iRelantionAplyTo == 1 )
		fApply = UTIL_IsHumanClass( pTarget->Classify() );
	else if ( m_iRelantionAplyTo == 2 )
		fApply = UTIL_IsAlienClass( pTarget->Classify() );
	else
		return R_NO;

	if ( fApply )
		return m_iRelantionship;

	return R_NO;
}

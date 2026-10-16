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
// Generic Monster - purely for scripted sequence work.
//=========================================================
#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"schedule.h"
#include	"weapons.h"	// POV: AddMultiDamage

// For holograms, make them not solid so the player can walk through them
#define	SF_GENERICMONSTER_NOTSOLID					4 
// POV (names ours)
#define	SF_GENERICMONSTER_INVULNERABLE				8	// bullets ricochet, health never drops
#define	SF_GENERICMONSTER_NOCLASS					32	// CLASS_NONE: nobody minds it
#define	SF_GENERICMONSTER_FOLLOWTRAIN				64	// rides along with the func_train "traintofollow"

//=========================================================
// Monster's Anim Events Go Here
//=========================================================

class CGenericMonster : public CBaseMonster
{
public:
	void Spawn( void );
	void Precache( void );
	void SetYawSpeed( void );
	int  Classify ( void );
	void HandleAnimEvent( MonsterEvent_t *pEvent );
	int ISoundMask ( void );

	// POV
	void TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType);
	int TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );
	void MonsterThink( void );
	void KeyValue( KeyValueData *pkvd );

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	string_t		m_iszTrainToFollow;
	BOOL			m_flFollowTrainGotPos;
	Vector			m_vecFollowTrainDist;
	CBaseEntity		*m_pFollowTankEnt;
};
LINK_ENTITY_TO_CLASS( monster_generic, CGenericMonster );

TYPEDESCRIPTION	CGenericMonster::m_SaveData[] = 
{
	DEFINE_FIELD( CGenericMonster, m_iszTrainToFollow, FIELD_STRING ),
	DEFINE_FIELD( CGenericMonster, m_pFollowTankEnt, FIELD_CLASSPTR ),
	DEFINE_FIELD( CGenericMonster, m_flFollowTrainGotPos, FIELD_BOOLEAN ),
	DEFINE_FIELD( CGenericMonster, m_vecFollowTrainDist, FIELD_VECTOR ),
};

IMPLEMENT_SAVERESTORE( CGenericMonster, CBaseMonster );

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CGenericMonster :: Classify ( void )
{
	if ( pev->spawnflags & SF_GENERICMONSTER_NOCLASS )
		return CLASS_NONE;

	return	CLASS_PLAYER_ALLY;
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CGenericMonster :: SetYawSpeed ( void )
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
void CGenericMonster :: HandleAnimEvent( MonsterEvent_t *pEvent )
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
// ISoundMask - generic monster can't hear.
//=========================================================
int CGenericMonster :: ISoundMask ( void )
{
	return	NULL;
}

//=========================================================
// Spawn
//=========================================================
void CGenericMonster :: Spawn()
{
	Precache();

	SET_MODEL( ENT(pev), STRING(pev->model) );

/*
	if ( FStrEq( STRING(pev->model), "models/player.mdl" ) )
		UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);
	else
		UTIL_SetSize(pev, VEC_HULL_MIN, VEC_HULL_MAX);
*/

	if ( FStrEq( STRING(pev->model), "models/player.mdl" ) || FStrEq( STRING(pev->model), "models/holo.mdl" ) )
		UTIL_SetSize(pev, VEC_HULL_MIN, VEC_HULL_MAX);
	else
		UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	if ( !FStringNull( m_iszTrainToFollow ) )
	{
		pev->movetype		= MOVETYPE_FLY;
		pev->flags			|= FL_FLY;
	}
	else
	{
		pev->solid			= SOLID_SLIDEBOX;
		pev->movetype		= MOVETYPE_STEP;
	}

	if ( pev->spawnflags & SF_GENERICMONSTER_INVULNERABLE )
	{
		pev->health			= 100;
		m_bloodColor		= DONT_BLEED;
	}
	else
	{
		pev->health			= 8;
		m_bloodColor		= BLOOD_COLOR_RED;
	}
	m_flFieldOfView		= 0.5;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState		= MONSTERSTATE_NONE;

	MonsterInit();

	if ( pev->spawnflags & SF_GENERICMONSTER_NOTSOLID )
	{
		pev->solid = SOLID_NOT;
		pev->takedamage = DAMAGE_NO;
	}

	m_pFollowTankEnt = NULL;
	m_vecFollowTrainDist = g_vecZero;
	m_flFollowTrainGotPos = FALSE;
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CGenericMonster :: Precache()
{
	PRECACHE_MODEL( (char *)STRING(pev->model) );
}	

//=========================================================
// POV: invulnerable generic monsters
//=========================================================
void CGenericMonster :: TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType)
{
	if ( pev->spawnflags & SF_GENERICMONSTER_INVULNERABLE )
	{
		UTIL_Ricochet( ptr->vecEndPos, 1.0 );
		AddMultiDamage( pevAttacker, this, flDamage, bitsDamageType );
		return;
	}

	CBaseMonster::TraceAttack( pevAttacker, flDamage, vecDir, ptr, bitsDamageType );
}

int CGenericMonster :: TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	if ( pev->spawnflags & SF_GENERICMONSTER_INVULNERABLE )
	{
		pev->health = pev->max_health / 2;

		if ( flDamage > 0 )
			SetConditions( bits_COND_LIGHT_DAMAGE );

		if ( flDamage >= 20 )
			SetConditions( bits_COND_HEAVY_DAMAGE );
	}
	else
		CBaseMonster::TakeDamage( pevInflictor, pevAttacker, flDamage, bitsDamageType );

	return 1;
}

//=========================================================
// POV: ride along with a func_train
//=========================================================
void CGenericMonster :: MonsterThink( void )
{
	if ( pev->spawnflags & SF_GENERICMONSTER_FOLLOWTRAIN )
	{
		if ( m_flFollowTrainGotPos )
		{
			pev->origin = m_pFollowTankEnt->pev->origin + m_vecFollowTrainDist;
			pev->avelocity = m_pFollowTankEnt->pev->avelocity;
		}
		else
		{
			if ( !m_pFollowTankEnt )
			{
				edict_t *pentTrain = FIND_ENTITY_BY_TARGETNAME( NULL, STRING( m_iszTrainToFollow ) );
				if ( FNullEnt( pentTrain ) )
				{
					ALERT( at_console, "CHEVOrangeHelmet: ERROR - couldn't find func_train %s!!\n", STRING( m_iszTrainToFollow ) );
				}
				else if ( !FClassnameIs( pentTrain, "func_train" ) )
				{
					ALERT( at_console, "CHEVOrangeHelmet: ERROR - %s is not a func_train!!\n", STRING( m_iszTrainToFollow ) );
				}
				else
				{
					m_pFollowTankEnt = (CBaseEntity *)GET_PRIVATE( pentTrain );
					if ( !m_pFollowTankEnt )
					{
						ALERT( at_console, "CHEVOrangeHelmet: ERROR - Couldnt instance %s !!\n", STRING( m_iszTrainToFollow ) );
						m_pFollowTankEnt = NULL;
					}
				}
			}

			if ( m_pFollowTankEnt->pev->velocity.x != 0 || m_pFollowTankEnt->pev->velocity.y != 0 || m_pFollowTankEnt->pev->velocity.z != 0 )
			{
				m_vecFollowTrainDist = pev->origin - m_pFollowTankEnt->pev->origin;
				m_flFollowTrainGotPos = TRUE;
				MonsterThink();
				return;
			}
		}
	}

	CBaseMonster::MonsterThink();
}

void CGenericMonster :: KeyValue( KeyValueData *pkvd )
{
	if ( FStrEq( pkvd->szKeyName, "traintofollow" ) )
	{
		m_iszTrainToFollow = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		CBaseMonster::KeyValue( pkvd );
}

//=========================================================
// AI Schedules Specific to this monster
//=========================================================

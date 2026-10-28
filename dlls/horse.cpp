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
//=========================================================
// Wanted: the horse (monster_horse). A tethered horse that stands
// about and neighs, warns anyone who stays close in front of it and
// kicks them four seconds later. The horse itself is not solid: while
// it stands on the ground it carries two invisible boxes, one at its
// chest and one at its rump, that block players. Bullets, arrows and
// knives find it through ExpandHorseHulls, which gives horses and
// pumas a big box for the length of a trace. The file name, CHorse,
// the fields and the helpers are ours.
//=========================================================

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"schedule.h"

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define	HORSE_AE_IDLE		( 1 )	// names ours
#define	HORSE_AE_KICK		( 2 )

class CHorse : public CBaseMonster
{
public:
	void Spawn( void );
	void Precache( void );
	int  Classify ( void );
	float HearingSensitivity( void );
	BOOL CheckMeleeAttack1 ( float flDot, float flDist );
	int IRelationship ( CBaseEntity *pTarget );
	void SetYawSpeed( void );
	void HandleAnimEvent( MonsterEvent_t *pEvent );

	void IdleSound( void );
	void WarnSound( void );
	void DeathSound( void );
	void PainSound( void );

	Schedule_t *GetScheduleOfType ( int Type );
	void RunAI( void );
	int TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType );

	CUSTOM_SCHEDULES;

	static const char *pAttackHitSounds[];

	float	m_flKickTime;	// > 0: the kick is due then; < 0: minus (the last time the enemy was away + 1)
	BOOL	m_fKick;		// CheckMeleeAttack1 allowed the kick
	BOOL	m_fHasBoxes;	// m_pBox[] are spawned and solid
	CBaseEntity *m_pBox[2];	// invisible solid boxes at the chest and the rump
};

LINK_ENTITY_TO_CLASS( monster_horse, CHorse );

// sic: never used (the bear's hit sounds; retail keeps the table at the
// start of this file's data, and no code reads it)
const char *CHorse::pAttackHitSounds[] =
{
	"player/pl_pain4.wav",
	"player/pl_pain5.wav",
	"player/pl_pain6.wav",
};

void SaveHorseHull( edict_t *pent );

//=========================================================
// Classify - indicates this monster's place in the
// relationship table.
//=========================================================
int	CHorse :: Classify ( void )
{
	return	CLASS_NONE;
}

//=========================================================
// HearingSensitivity - the horse hears nothing.
//=========================================================
float CHorse :: HearingSensitivity( void )
{
	return 0;
}

//=========================================================
// CheckMeleeAttack1 - an enemy that has stayed within 110
// units in front of the horse for a second is warned
// (WarnSound) and kicked four seconds later if it is still
// there. The kick must be checked for within half a second of
// that; if the check misses the window, the horse doesn't
// kick again until the enemy has left (retail behaviour).
//=========================================================
BOOL CHorse :: CheckMeleeAttack1 ( float flDot, float flDist )
{
	if ( m_hEnemy != NULL && flDist <= 110 && flDot >= 0.7 )
	{
		if ( m_flKickTime != 0 && gpGlobals->time > m_flKickTime && gpGlobals->time < m_flKickTime + 0.5 )
		{
			m_flKickTime = 0;
			m_fKick = TRUE;
			return TRUE;
		}

		if ( m_flKickTime == 0 )
		{
			WarnSound();
			m_flKickTime = gpGlobals->time + 4.0;
			return FALSE;
		}

		if ( m_flKickTime < 0 && -gpGlobals->time < m_flKickTime )
		{
			m_flKickTime = 0;
			return FALSE;
		}
	}
	else
	{
		m_flKickTime = -gpGlobals->time - 1.0;
	}

	return FALSE;
}

//=========================================================
// IRelationship - the horse dislikes players and ignores
// everything else.
//=========================================================
int CHorse :: IRelationship ( CBaseEntity *pTarget )
{
	if ( pTarget && pTarget->IsPlayer() )
		return R_DL;

	return R_NO;
}

//=========================================================
// SetYawSpeed - the horse only turns in scripts.
//=========================================================
void CHorse :: SetYawSpeed ( void )
{
	int ys = 0;

	if ( m_IdealMonsterState == MONSTERSTATE_SCRIPT )
		ys = 270;

	pev->yaw_speed = ys;
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CHorse :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	switch( pEvent->event )
	{
	case HORSE_AE_IDLE:
		IdleSound();
		return;

	case HORSE_AE_KICK:
		if ( !m_fKick )
			break;	// CheckMeleeAttack1 didn't allow it
		{
			// kick low at a crouching enemy
			int iHeight = 32;

			if ( m_hEnemy != NULL && FBitSet( m_hEnemy->pev->flags, FL_DUCKING ) )
				iHeight = 16;

			pev->origin.z += iHeight;

			for ( int i = 0; i < 2; i++ )
				m_pBox[i]->pev->solid = SOLID_NOT;

			CBaseEntity *pHurt = CheckTraceHullAttack( 140, gSkillData.horseDmgKick, DMG_GENERIC );

			for ( i = 0; i < 2; i++ )
				m_pBox[i]->pev->solid = SOLID_SLIDEBOX;

			pev->origin.z -= iHeight;

			if ( pHurt && pHurt != this && FBitSet( pHurt->pev->flags, FL_MONSTER | FL_CLIENT ) )
			{
				pHurt->pev->punchangle.z = RANDOM_FLOAT( 0, 3 );
				pHurt->pev->punchangle.y = RANDOM_FLOAT( -2, 2 );
				pHurt->pev->punchangle.x = -5;
				MAKE_VECTORS( pev->angles );
				pHurt->pev->basevelocity = gpGlobals->v_forward * 300;
			}

			m_fKick = FALSE;
		}
		return;
	}

	CBaseMonster::HandleAnimEvent( pEvent );
}

//=========================================================
// Spawn
//=========================================================
void CHorse :: Spawn()
{
	Precache( );

	SET_MODEL(ENT(pev), "models/horse.mdl");
	UTIL_SetSize( pev, Vector( -2, -2, 0 ), Vector( 2, 2, 32 ) );
	pev->spawnflags |= SF_MONSTER_FALL_TO_GROUND;
	SaveHorseHull( ENT(pev) );

	pev->solid			= SOLID_NOT;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= BLOOD_COLOR_RED;
	pev->effects		= 0;
	pev->health			= gSkillData.horseHealth;
	pev->yaw_speed		= 5;
	m_flFieldOfView		= 0.7;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState		= MONSTERSTATE_NONE;
	m_fHasBoxes			= FALSE;

	MonsterInit();
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CHorse :: Precache()
{
	PRECACHE_MODEL("models/horse.mdl");
	PRECACHE_MODEL("models/dynamite.mdl");	// the boxes' model; they are never drawn

	PRECACHE_SOUND("horse/horse1.wav");
	PRECACHE_SOUND("horse/horse2.wav");
	PRECACHE_SOUND("horse/horse3.wav");
	PRECACHE_SOUND("horse/horse4.wav");
	PRECACHE_SOUND("horse/horse5.wav");
	PRECACHE_SOUND("horse/horse6.wav");
	PRECACHE_SOUND("horse/horse7.wav");
	PRECACHE_SOUND("horse/horse8.wav");
	PRECACHE_SOUND("horse/horse9.wav");
	PRECACHE_SOUND("horse/pain.wav");
}

//=========================================================
// IdleSound
//=========================================================
void CHorse :: IdleSound( void )
{
	switch ( RANDOM_LONG( 0, 5 ) )
	{
	case 0:
		EMIT_SOUND( ENT(pev), CHAN_VOICE, "horse/horse1.wav", 1, ATTN_NORM );
		break;
	case 1:
		EMIT_SOUND( ENT(pev), CHAN_VOICE, "horse/horse3.wav", 1, ATTN_NORM );
		break;
	case 2:
		EMIT_SOUND( ENT(pev), CHAN_VOICE, "horse/horse6.wav", 1, ATTN_NORM );
		break;
	case 3:
		EMIT_SOUND( ENT(pev), CHAN_VOICE, "horse/horse7.wav", 1, ATTN_NORM );
		break;
	case 4:
		EMIT_SOUND( ENT(pev), CHAN_VOICE, "horse/horse8.wav", 1, ATTN_NORM );
		break;
	case 5:
		EMIT_SOUND( ENT(pev), CHAN_VOICE, "horse/horse9.wav", 1, ATTN_NORM );
		break;
	}
}

//=========================================================
// WarnSound - the kick is coming (name ours)
//=========================================================
void CHorse :: WarnSound( void )
{
	switch ( RANDOM_LONG( 0, 2 ) )
	{
	case 0:
		EMIT_SOUND( ENT(pev), CHAN_WEAPON, "horse/horse2.wav", 1, ATTN_NORM );
		break;
	case 1:
		EMIT_SOUND( ENT(pev), CHAN_WEAPON, "horse/horse4.wav", 1, ATTN_NORM );
		break;
	case 2:
		EMIT_SOUND( ENT(pev), CHAN_WEAPON, "horse/horse5.wav", 1, ATTN_NORM );
		break;
	}
}

//=========================================================
// DeathSound
//=========================================================
void CHorse :: DeathSound( void )
{
	EMIT_SOUND( ENT(pev), CHAN_VOICE, "horse/horse5.wav", 1, ATTN_NORM );
}

//=========================================================
// PainSound - half the time
//=========================================================
void CHorse :: PainSound( void )
{
	if ( RANDOM_LONG( 0, 1 ) == 0 )
		EMIT_SOUND( ENT(pev), CHAN_VOICE, "horse/pain.wav", 1, ATTN_NORM );
}

//=========================================================
// AI Schedules Specific to this monster
//=========================================================

// kick the enemy
Task_t	tlHorseMeleeAttack1[] =
{
	{ TASK_STOP_MOVING,			0				},
	{ TASK_MELEE_ATTACK1,		(float)0		},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE	},
};

Schedule_t	slHorseMeleeAttack1[] =
{
	{
		tlHorseMeleeAttack1,
		ARRAYSIZE ( tlHorseMeleeAttack1 ),
		0,
		0,
		"HorseMeleeAttack1"
	},
};

// hurt: rear up and kick
Task_t	tlHorseFlinch[] =
{
	{ TASK_STOP_MOVING,			0							},
	{ TASK_PLAY_SEQUENCE,		(float)ACT_MELEE_ATTACK1	},
};

Schedule_t	slHorseFlinch[] =
{
	{
		tlHorseFlinch,
		ARRAYSIZE ( tlHorseFlinch ),
		0,
		0,
		"HorseFlinch"
	},
};

DEFINE_CUSTOM_SCHEDULES( CHorse )
{
	slHorseMeleeAttack1,
	slHorseFlinch,
};

IMPLEMENT_CUSTOM_SCHEDULES( CHorse, CBaseMonster );

//=========================================================
// GetScheduleOfType - a hurt horse rears up; one that would
// face, chase or wake angry stands still and calms down.
//=========================================================
Schedule_t* CHorse :: GetScheduleOfType ( int Type )
{
	if ( m_IdealMonsterState == MONSTERSTATE_DEAD )
		return CBaseMonster :: GetScheduleOfType( Type );

	if ( !HasConditions( bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE ) )
	{
		switch	( Type )
		{
		case SCHED_MELEE_ATTACK1:
			return &slHorseMeleeAttack1[ 0 ];

		case SCHED_ALERT_FACE:
		case SCHED_ALERT_STAND:
			pev->yaw_speed = 0;
			SetState( MONSTERSTATE_IDLE );
			return CBaseMonster :: GetScheduleOfType( SCHED_IDLE_STAND );

		case SCHED_WAKE_ANGRY:
		case SCHED_COMBAT_FACE:
		case SCHED_CHASE_ENEMY:
			pev->yaw_speed = 0;
			m_MonsterState = MONSTERSTATE_IDLE;
			return CBaseMonster :: GetScheduleOfType( SCHED_IDLE_STAND );
		}

		return CBaseMonster :: GetScheduleOfType( Type );
	}

	// hurt
	m_Activity = ACT_RESET;
	return &slHorseFlinch[ 0 ];
}

//=========================================================
// RunAI - a live horse on the ground gets its two boxes,
// 32x32x60 units, 10 units behind its origin and 50 in front;
// a scripted one makes them non-solid and gets new ones when
// the script is over (they don't follow the horse).
//=========================================================
void CHorse :: RunAI( void )
{
	if ( !m_fHasBoxes && FBitSet( pev->flags, FL_ONGROUND ) && m_MonsterState != MONSTERSTATE_SCRIPT && m_IdealMonsterState != MONSTERSTATE_SCRIPT && pev->health > 0 )
	{
		MAKE_VECTORS( pev->angles );

		for ( int i = 0; i < 2; i++ )
		{
			m_pBox[i] = GetClassPtr( (CBaseEntity *)NULL );
			m_pBox[i]->pev->modelindex = MODEL_INDEX( "models/dynamite.mdl" );
			m_pBox[i]->pev->effects = EF_NODRAW;
			m_pBox[i]->pev->mins = Vector( -16, -16, 20 );
			m_pBox[i]->pev->maxs = Vector( 16, 16, 80 );
			UTIL_SetSize( m_pBox[i]->pev, m_pBox[i]->pev->mins, m_pBox[i]->pev->maxs );
			m_pBox[i]->pev->solid = SOLID_SLIDEBOX;
			m_pBox[i]->pev->movetype = MOVETYPE_FLY;
			m_pBox[i]->pev->origin = pev->origin - gpGlobals->v_forward * 10 + gpGlobals->v_forward * 60 * i;
			UTIL_SetOrigin( m_pBox[i]->pev, m_pBox[i]->pev->origin );
		}

		m_fHasBoxes = TRUE;
	}

	if ( m_MonsterState == MONSTERSTATE_SCRIPT && m_fHasBoxes )
	{
		for ( int i = 0; i < 2; i++ )
		{
			m_pBox[i]->pev->solid = SOLID_NOT;
			UTIL_SetOrigin( m_pBox[i]->pev, m_pBox[i]->pev->origin );
		}

		m_fHasBoxes = FALSE;
	}

	CBaseMonster :: RunAI();
}

//=========================================================
// TakeDamage - the boxes go when the horse dies, or when a
// scripted horse is brought below zero health.
//=========================================================
int CHorse :: TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType )
{
	float flOldHealth = pev->health;
	MONSTERSTATE oldState = m_IdealMonsterState;

	int ret = CBaseMonster :: TakeDamage( pevInflictor, pevAttacker, flDamage, bitsDamageType );

	if ( ( m_IdealMonsterState == MONSTERSTATE_DEAD && oldState != MONSTERSTATE_DEAD ) ||
		 ( flOldHealth > 0 && pev->health < 0 && m_MonsterState == MONSTERSTATE_SCRIPT ) )
	{
		for ( int i = 0; i < 2; i++ )
			m_pBox[i]->SUB_Remove();
	}

	return ret;
}

//=========================================================
// SaveHorseHull - keeps an entity's own size in startpos and
// endpos for ExpandHorseHulls (the puma's Spawn calls it too).
// Name ours.
//=========================================================
void SaveHorseHull( edict_t *pent )
{
	pent->v.startpos = pent->v.mins;
	pent->v.endpos = pent->v.maxs;
}

//=========================================================
// ExpandHorseHulls - horses and pumas get big boxes while a
// bullet is traced, so that the trace hits them, and their own
// sizes back afterwards (horses are not solid otherwise; dead
// pumas aren't either). FireBullets, the bow, the knife and the
// pick call it around their traces. Name ours.
//=========================================================
void ExpandHorseHulls( int fExpand )
{
	char *pszClassname;

	for ( int i = 0; i < 2; i++ )
	{
		if ( i == 0 )
			pszClassname = "monster_horse";
		else if ( i == 1 )
			pszClassname = "monster_puma";

		CBaseEntity *pEntity = NULL;
		while ( ( pEntity = UTIL_FindEntityByClassname( pEntity, pszClassname ) ) != NULL )
		{
			if ( fExpand )
			{
				UTIL_SetSize( pEntity->pev, Vector( -76, -76, 0 ), Vector( 76, 76, 128 ) );
				pEntity->pev->solid = SOLID_SLIDEBOX;
			}
			else
			{
				UTIL_SetSize( pEntity->pev, pEntity->pev->startpos, pEntity->pev->endpos );
				if ( i == 0 )
					pEntity->pev->solid = SOLID_NOT;
				else if ( i == 1 )
				{
					if ( pEntity->IsAlive() )
						pEntity->pev->solid = SOLID_SLIDEBOX;
					else
						pEntity->pev->solid = SOLID_NOT;
				}
			}
			UTIL_SetOrigin( pEntity->pev, pEntity->pev->origin );
		}
	}
}

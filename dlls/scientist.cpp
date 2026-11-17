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
// human scientist (passive lab worker)
//=========================================================

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"talkmonster.h"
#include	"schedule.h"
#include	"defaultai.h"
#include	"scripted.h"
#include	"animation.h"
#include	"soundent.h"
#include	"weapons.h"
#include	"player.h"
#include	"gamerules.h"
#include	"game.h"
#include	"nodes.h"


#define		NUM_SCIENTIST_HEADS		4 // four heads available for scientist model
enum { HEAD_GLASSES = 0, HEAD_EINSTEIN = 1, HEAD_LUTHER = 2, HEAD_SLICK = 3 };

enum
{
	SCHED_HIDE = LAST_TALKMONSTER_SCHEDULE + 1,
	SCHED_FEAR,
	SCHED_PANIC,
	SCHED_STARTLE,
	SCHED_TARGET_CHASE_SCARED,
	SCHED_TARGET_FACE_SCARED,
	SCHED_IDLE_RUN,		// name ours: walk a path at a run
};

enum
{
	TASK_SAY_HEAL = LAST_TALKMONSTER_TASK + 1,
	TASK_HEAL,
	TASK_SAY_FEAR,
	TASK_RUN_PATH_SCARED,
	TASK_SCREAM,
	TASK_RANDOM_SCREAM,
	TASK_MOVE_TO_TARGET_RANGE_SCARED,
};

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define		SCIENTIST_AE_HEAL		( 1 )
#define		SCIENTIST_AE_NEEDLEON	( 2 )
#define		SCIENTIST_AE_NEEDLEOFF	( 3 )

// spawnflags
#define		SF_SCIENTIST_FEARPLAYER		64	// name ours: starts provoked (the fgd's "Fear Player" says 32)
#define		SF_BARNEY_HATE_SCIENTISTS	32	// as in barney.cpp

// damage bits only the scientist tests (names ours)
#define		DMG_NOPUSH		(1 << 25)	// no knockback (energy disk, displacer ball)
#define		DMG_HALVE		(1 << 26)	// cuts the victim in half (energy disk, sword)

// bodygroup 0 (head): 0-3 the heads, +4 headless, 8 skull, 9 none
// bodygroup 2 (body): 0 whole, 1 legs only, 2 upper body only, 3 none
// bodygroup 3 (skeleton): 0 none, 1 whole, 2 legs, 3 upper body

extern int gmsgSciInfo;
extern entvars_t *g_pevLastInflictor;

//=======================================================
// Scientist
//=======================================================

class CScientist : public CTalkMonster
{
public:
	void Spawn( void );
	void Precache( void );

	void SetYawSpeed( void );
	int  Classify ( void );
	void HandleAnimEvent( MonsterEvent_t *pEvent );
	void RunTask( Task_t *pTask );
	void StartTask( Task_t *pTask );
	int	ObjectCaps( void ) { return CTalkMonster :: ObjectCaps() | FCAP_IMPULSE_USE; }
	const char *GetKillVerb( BOOL fGibbed, int &iHitGroup )
	{
		if ( pev->takedamage == DAMAGE_NO )
			return "telefragged";
		if ( m_bitsDamageType & DMG_RADIATION )
			return "died of radiation poisoning";
		if ( m_bitsDamageType & (DMG_POISON | DMG_NERVEGAS) )
			return "was poisoned";
		if ( m_bitsDamageType & DMG_SLOWBURN )
			return "burned to death";
		if ( m_bitsDamageType & DMG_SLOWFREEZE )
			return "was frozen";
		return "drowned";
	}
	const char *GetKillDesc( void )
	{
		if ( pev->takedamage != DAMAGE_NO )
			return "";
		return GetHudName();
	}
	const char *GetHudName( void )
	{
		switch ( GetBodygroup( 0 ) )
		{
		case HEAD_EINSTEIN:
		case HEAD_EINSTEIN + NUM_SCIENTIST_HEADS:
			return "Einstein";
		case HEAD_LUTHER:
		case HEAD_LUTHER + NUM_SCIENTIST_HEADS:
			return "Luther";
		case HEAD_SLICK:
		case HEAD_SLICK + NUM_SCIENTIST_HEADS:
			return "Slick";
		default:
			return "Walter";
		}
	}
	int GetBattlePower( void ) { return pev->max_health * 0.1; }
	BOOL IsGrenadeTarget( void ) { return TRUE; }
	int TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType);
	void TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType);
	virtual int FriendNumber( int arrayNumber );
	void SetActivity ( Activity newActivity );
	Activity GetStoppedActivity( void );
	int ISoundMask( void );
	void DeclineFollowing( void );
	BOOL FValidateHintType( short sHint );
	BOOL FCanActiveIdle( void );
	int IRelationship( CBaseEntity *pTarget );
	void MonsterThink( void );
	void StartMonster( void );
	int FOkToSpeak( void );

	float	CoverRadius( void ) { return 1200; }		// Need more room for cover because scientists want to get far away!
	BOOL	DisregardEnemy( CBaseEntity *pEnemy ) { return !pEnemy->IsAlive() || (gpGlobals->time - m_fearTime) > 15; }

	virtual void	AcidMelt( void );	// name ours: dissolve to a skeleton where the acid hit
	virtual BOOL	CanHeal( void );
	void	Heal( void );
	void	Scream( void );
	virtual bool	HasHead( void ) { return GetBodygroup( 0 ) < NUM_SCIENTIST_HEADS; }	// name ours
	virtual bool	CanBeHalved( void ) { return !GetBodygroup( 2 ); }	// name ours
	virtual bool	CanMelt( void ) { return !GetBodygroup( 3 ); }	// name ours

	void	Shatter( void );	// name ours: frozen solid and hit
	void	CutInHalf( void );	// name ours
	void	DeathMessage( entvars_t *pevAttacker, entvars_t *pevInflictor, BOOL fGibbed );	// name ours

	// Override these to set behavior
	Schedule_t *GetScheduleOfType ( int Type );
	Schedule_t *GetSchedule ( void );
	MONSTERSTATE GetIdealState ( void );

	void DeathSound( void );
	void PainSound( void );
	
	void TalkInit( void );

	void			Killed( entvars_t *pevAttacker, int iGib );
	void			GibMonster( void );
	void			FadeMonster( void );
	
	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	CUSTOM_SCHEDULES;

private:	
	float m_painTime;
	float m_healTime;
	float m_fearTime;
public:
	float m_glowTime;			// the glow shell of radiation, freeze or paralysis goes at this time
	int m_frozen;
	int m_iGlassGibs;			// name ours
	float m_flHeadDamage;		// name ours: head hits since the last TakeDamage
	float m_flAcidDamageUpper;
	float m_flAcidDamageLower;
};

LINK_ENTITY_TO_CLASS( monster_scientist, CScientist );

TYPEDESCRIPTION	CScientist::m_SaveData[] = 
{
	DEFINE_FIELD( CScientist, m_painTime, FIELD_TIME ),
	DEFINE_FIELD( CScientist, m_healTime, FIELD_TIME ),
	DEFINE_FIELD( CScientist, m_fearTime, FIELD_TIME ),
	DEFINE_FIELD( CScientist, m_glowTime, FIELD_TIME ),
	DEFINE_FIELD( CScientist, m_frozen, FIELD_INTEGER ),
	DEFINE_FIELD( CScientist, m_flAcidDamageUpper, FIELD_FLOAT ),
	DEFINE_FIELD( CScientist, m_flAcidDamageLower, FIELD_FLOAT ),
};

IMPLEMENT_SAVERESTORE( CScientist, CTalkMonster );

//=========================================================
// AI Schedules Specific to this monster
//=========================================================
Task_t	tlFollow[] =
{
	{ TASK_SET_FAIL_SCHEDULE,	(float)SCHED_CANT_FOLLOW },	// If you fail, bail out of follow
	{ TASK_MOVE_TO_TARGET_RANGE,(float)128		},	// Move within 128 of target ent (client)
//	{ TASK_SET_SCHEDULE,		(float)SCHED_TARGET_FACE },
};

Schedule_t	slFollow[] =
{
	{
		tlFollow,
		ARRAYSIZE ( tlFollow ),
		bits_COND_NEW_ENEMY |
		bits_COND_LIGHT_DAMAGE |
		bits_COND_HEAVY_DAMAGE |
		bits_COND_HEAR_SOUND,
		bits_SOUND_COMBAT |
		bits_SOUND_DANGER,
		"Follow"
	},
};

Task_t	tlFollowScared[] =
{
	{ TASK_SET_FAIL_SCHEDULE,	(float)SCHED_TARGET_CHASE },// If you fail, follow normally
	{ TASK_MOVE_TO_TARGET_RANGE_SCARED,(float)128		},	// Move within 128 of target ent (client)
//	{ TASK_SET_SCHEDULE,		(float)SCHED_TARGET_FACE_SCARED },
};

Schedule_t	slFollowScared[] =
{
	{
		tlFollowScared,
		ARRAYSIZE ( tlFollowScared ),
		bits_COND_NEW_ENEMY |
		bits_COND_HEAR_SOUND |
		bits_COND_LIGHT_DAMAGE |
		bits_COND_HEAVY_DAMAGE,
		bits_SOUND_DANGER,
		"FollowScared"
	},
};

Task_t	tlFaceTargetScared[] =
{
	{ TASK_FACE_TARGET,			(float)0		},
	{ TASK_SET_ACTIVITY,		(float)ACT_CROUCHIDLE },
	{ TASK_SET_SCHEDULE,		(float)SCHED_TARGET_CHASE_SCARED },
};

Schedule_t	slFaceTargetScared[] =
{
	{
		tlFaceTargetScared,
		ARRAYSIZE ( tlFaceTargetScared ),
		bits_COND_HEAR_SOUND |
		bits_COND_NEW_ENEMY,
		bits_SOUND_DANGER,
		"FaceTargetScared"
	},
};

Task_t	tlStopFollowing[] =
{
	{ TASK_CANT_FOLLOW,		(float)0 },
};

Schedule_t	slStopFollowing[] =
{
	{
		tlStopFollowing,
		ARRAYSIZE ( tlStopFollowing ),
		0,
		0,
		"StopFollowing"
	},
};


Task_t	tlHeal[] =
{
	{ TASK_MOVE_TO_TARGET_RANGE,(float)50		},	// Move within 60 of target ent (client)
	{ TASK_SET_FAIL_SCHEDULE,	(float)SCHED_TARGET_CHASE },	// If you fail, catch up with that guy! (change this to put syringe away and then chase)
	{ TASK_FACE_IDEAL,			(float)0		},
	{ TASK_SAY_HEAL,			(float)0		},
	{ TASK_PLAY_SEQUENCE_FACE_TARGET,		(float)ACT_ARM	},			// Whip out the needle
	{ TASK_HEAL,				(float)0	},	// Put it in the player
	{ TASK_PLAY_SEQUENCE_FACE_TARGET,		(float)ACT_DISARM	},			// Put away the needle
};

Schedule_t	slHeal[] =
{
	{
		tlHeal,
		ARRAYSIZE ( tlHeal ),
		0,	// Don't interrupt or he'll end up running around with a needle all the time
		0,
		"Heal"
	},
};


Task_t	tlFaceTarget[] =
{
	{ TASK_STOP_MOVING,			(float)0		},
	{ TASK_FACE_TARGET,			(float)0		},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE },
	{ TASK_SET_SCHEDULE,		(float)SCHED_TARGET_CHASE },
};

Schedule_t	slFaceTarget[] =
{
	{
		tlFaceTarget,
		ARRAYSIZE ( tlFaceTarget ),
		bits_COND_CLIENT_PUSH |
		bits_COND_NEW_ENEMY |
		bits_COND_HEAR_SOUND,
		bits_SOUND_COMBAT |
		bits_SOUND_DANGER,
		"FaceTarget"
	},
};


Task_t	tlSciPanic[] =
{
	{ TASK_STOP_MOVING,			(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_SCREAM,				(float)0		},
	{ TASK_PLAY_SEQUENCE_FACE_ENEMY,		(float)ACT_EXCITED	},	// This is really fear-stricken excitement
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE	},
};

Schedule_t	slSciPanic[] =
{
	{
		tlSciPanic,
		ARRAYSIZE ( tlSciPanic ),
		0,
		0,
		"SciPanic"
	},
};


Task_t	tlIdleSciStand[] =
{
	{ TASK_STOP_MOVING,			0				},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE },
	{ TASK_WAIT,				(float)2		}, // repick IDLESTAND every two seconds.
	{ TASK_TLK_HEADRESET,		(float)0		}, // reset head position
};

Schedule_t	slIdleSciStand[] =
{
	{ 
		tlIdleSciStand,
		ARRAYSIZE ( tlIdleSciStand ), 
		bits_COND_NEW_ENEMY		|
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE	|
		bits_COND_HEAR_SOUND	|
		bits_COND_SMELL			|
		bits_COND_CLIENT_PUSH	|
		bits_COND_PROVOKED,

		bits_SOUND_COMBAT		|// sound flags
		//bits_SOUND_PLAYER		|
		//bits_SOUND_WORLD		|
		bits_SOUND_DANGER		|
		bits_SOUND_MEAT			|// scents
		bits_SOUND_CARCASS		|
		bits_SOUND_GARBAGE,
		"IdleSciStand"

	},
};


Task_t	tlScientistCover[] =
{
	{ TASK_SET_FAIL_SCHEDULE,		(float)SCHED_PANIC },		// If you fail, just panic!
	{ TASK_STOP_MOVING,				(float)0					},
	{ TASK_FIND_COVER_FROM_ENEMY,	(float)0					},
	{ TASK_RUN_PATH_SCARED,			(float)0					},
	{ TASK_TURN_LEFT,				(float)179					},
	{ TASK_SET_SCHEDULE,			(float)SCHED_HIDE			},
};

Schedule_t	slScientistCover[] =
{
	{ 
		tlScientistCover,
		ARRAYSIZE ( tlScientistCover ), 
		bits_COND_NEW_ENEMY,
		0,
		"ScientistCover"
	},
};



Task_t	tlScientistHide[] =
{
	{ TASK_SET_FAIL_SCHEDULE,		(float)SCHED_PANIC },		// If you fail, just panic!
	{ TASK_STOP_MOVING,				(float)0					},
	{ TASK_PLAY_SEQUENCE,			(float)ACT_CROUCH			},
	{ TASK_SET_ACTIVITY,			(float)ACT_CROUCHIDLE		},	// FIXME: This looks lame
	{ TASK_WAIT_RANDOM,				(float)10.0					},
};

Schedule_t	slScientistHide[] =
{
	{ 
		tlScientistHide,
		ARRAYSIZE ( tlScientistHide ), 
		bits_COND_NEW_ENEMY |
		bits_COND_HEAR_SOUND |
		bits_COND_SEE_ENEMY |
		bits_COND_SEE_HATE |
		bits_COND_SEE_FEAR |
		bits_COND_SEE_DISLIKE,
		bits_SOUND_DANGER,
		"ScientistHide"
	},
};


Task_t	tlScientistStartle[] =
{
	{ TASK_SET_FAIL_SCHEDULE,		(float)SCHED_PANIC },		// If you fail, just panic!
	{ TASK_RANDOM_SCREAM,			(float)0.3 },				// Scream 30% of the time
	{ TASK_STOP_MOVING,				(float)0					},
	{ TASK_PLAY_SEQUENCE_FACE_ENEMY,			(float)ACT_CROUCH			},
	{ TASK_RANDOM_SCREAM,			(float)0.1 },				// Scream again 10% of the time
	{ TASK_PLAY_SEQUENCE_FACE_ENEMY,			(float)ACT_CROUCHIDLE		},
	{ TASK_WAIT_RANDOM,				(float)1.0					},
};

Schedule_t	slScientistStartle[] =
{
	{ 
		tlScientistStartle,
		ARRAYSIZE ( tlScientistStartle ), 
		bits_COND_NEW_ENEMY |
		bits_COND_SEE_ENEMY |
		bits_COND_SEE_HATE |
		bits_COND_SEE_FEAR |
		bits_COND_SEE_DISLIKE,
		0,
		"ScientistStartle"
	},
};



Task_t	tlFear[] =
{
	{ TASK_STOP_MOVING,				(float)0					},
	{ TASK_FACE_ENEMY,				(float)0					},
	{ TASK_SAY_FEAR,				(float)0					},
//	{ TASK_PLAY_SEQUENCE,			(float)ACT_FEAR_DISPLAY		},
};

Schedule_t	slFear[] =
{
	{ 
		tlFear,
		ARRAYSIZE ( tlFear ), 
		bits_COND_NEW_ENEMY,
		0,
		"Fear"
	},
};


// walk a path at a run (name ours)
Task_t	tlIdleRun[] =
{
	{ TASK_SET_FAIL_SCHEDULE,	(float)SCHED_IDLE_WALK	},
	{ TASK_RUN_PATH,			(float)0		},
	{ TASK_WAIT_FOR_MOVEMENT,	(float)0		},
};

Schedule_t	slIdleRun[] =
{
	{ 
		tlIdleRun,
		ARRAYSIZE ( tlIdleRun ), 
		bits_COND_NEW_ENEMY		|
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE	|
		bits_COND_HEAR_SOUND	|
		bits_COND_SMELL_FOOD	|
		bits_COND_SMELL			|
		bits_COND_PROVOKED,

		bits_SOUND_COMBAT		|// sound flags
		bits_SOUND_MEAT			|// scents
		bits_SOUND_CARCASS		|
		bits_SOUND_GARBAGE,
		"Idle Run"
	},
};


DEFINE_CUSTOM_SCHEDULES( CScientist )
{
	slFollow,
	slFaceTarget,
	slIdleSciStand,
	slFear,
	slScientistCover,
	slScientistHide,
	slScientistStartle,
	slHeal,
	slStopFollowing,
	slSciPanic,
	slFollowScared,
	slFaceTargetScared,
	slIdleRun,
};


IMPLEMENT_CUSTOM_SCHEDULES( CScientist, CTalkMonster );


void CScientist::DeclineFollowing( void )
{
	Talk( 10 );
	m_hTalkTarget = m_hEnemy;
	PlaySentence( "SC_POK", 2, VOL_NORM, ATTN_NORM );
}


void CScientist :: Scream( void )
{
	if ( FOkToSpeak() )
	{
		Talk( 10 );
		m_hTalkTarget = m_hEnemy;
		PlaySentence( "SC_SCREAM", RANDOM_FLOAT(3, 6), VOL_NORM, ATTN_NORM );
	}
}


Activity CScientist::GetStoppedActivity( void )
{ 
	if ( m_hEnemy != NULL ) 
		return ACT_EXCITED;
	return CTalkMonster::GetStoppedActivity();
}


//=========================================================
// FValidateHintType - the scientist's own work hints, then
// the talkmonster's
//=========================================================
BOOL CScientist :: FValidateHintType ( short sHint )
{
	int i;

	static short sScientistHints[] =
	{
		HINT_SCIENTIST_WORK,
		HINT_SCIENTIST_USE,
		HINT_SCIENTIST_TRANSIT,
	};

	for ( i = 0 ; i < ARRAYSIZE ( sScientistHints ) ; i++ )
	{
		if ( sScientistHints[ i ] == sHint )
		{
			return TRUE;
		}
	}

	return CTalkMonster::FValidateHintType( sHint );
}


void CScientist :: StartTask( Task_t *pTask )
{
	switch( pTask->iTask )
	{
	case TASK_SAY_HEAL:
//		if ( FOkToSpeak() )
		Talk( 2 );
		m_hTalkTarget = m_hTargetEnt;
		PlaySentence( "SC_HEAL", 2, VOL_NORM, ATTN_IDLE );

		TaskComplete();
		break;

	case TASK_SCREAM:
		Scream();
		TaskComplete();
		break;

	case TASK_RANDOM_SCREAM:
		if ( RANDOM_FLOAT( 0, 1 ) < pTask->flData )
			Scream();
		TaskComplete();
		break;

	case TASK_SAY_FEAR:
		if ( FOkToSpeak() )
		{
			Talk( 2 );
			m_hTalkTarget = m_hEnemy;
			if ( m_hEnemy->IsPlayer() )
				PlaySentence( "SC_PLFEAR", 5, VOL_NORM, ATTN_NORM );
			else
				PlaySentence( "SC_FEAR", 5, VOL_NORM, ATTN_NORM );
		}
		TaskComplete();
		break;

	case TASK_HEAL:
		m_IdealActivity = ACT_MELEE_ATTACK1;
		break;

	case TASK_RUN_PATH_SCARED:
		m_movementActivity = ACT_RUN_SCARED;
		break;

	case TASK_MOVE_TO_TARGET_RANGE_SCARED:
		{
			if ( (m_hTargetEnt->pev->origin - pev->origin).Length() < 1 )
				TaskComplete();
			else
			{
				m_vecMoveGoal = m_hTargetEnt->pev->origin;
				if ( !MoveToTarget( ACT_WALK_SCARED, 0.5 ) )
					TaskFail();
			}
		}
		break;

	default:
		CTalkMonster::StartTask( pTask );
		break;
	}
}


//=========================================================
// IRelationship - a barney who hates scientists is hated back
//=========================================================
int CScientist::IRelationship( CBaseEntity *pTarget )
{
	if ( (pTarget->pev->spawnflags & SF_BARNEY_HATE_SCIENTISTS) && pTarget->Classify() == CLASS_PLAYER_ALLY )
		return R_HT;

	return CTalkMonster::IRelationship( pTarget );
}

void CScientist :: RunTask( Task_t *pTask )
{
	switch ( pTask->iTask )
	{
	case TASK_RUN_PATH_SCARED:
		if ( MovementIsComplete() )
			TaskComplete();
		if ( RANDOM_LONG(0,31) < 8 )
			Scream();
		break;

	case TASK_MOVE_TO_TARGET_RANGE_SCARED:
		{
			if ( RANDOM_LONG(0,63)< 8 )
				Scream();

			if ( m_hEnemy == NULL )
			{
				TaskFail();
			}
			else
			{
				float distance;

				distance = ( m_vecMoveGoal - pev->origin ).Length2D();
				// Re-evaluate when you think your finished, or the target has moved too far
				if ( (distance < pTask->flData) || (m_vecMoveGoal - m_hTargetEnt->pev->origin).Length() > pTask->flData * 0.5 )
				{
					m_vecMoveGoal = m_hTargetEnt->pev->origin;
					distance = ( m_vecMoveGoal - pev->origin ).Length2D();
					FRefreshRoute();
				}

				// Set the appropriate activity based on an overlapping range
				// overlap the range to prevent oscillation
				if ( distance < pTask->flData )
				{
					TaskComplete();
					RouteClear();		// Stop moving
				}
				else if ( distance < 190 && m_movementActivity != ACT_WALK_SCARED )
					m_movementActivity = ACT_WALK_SCARED;
				else if ( distance >= 270 && m_movementActivity != ACT_RUN_SCARED )
					m_movementActivity = ACT_RUN_SCARED;
			}
		}
		break;

	case TASK_HEAL:
		if ( m_fSequenceFinished )
		{
			TaskComplete();
		}
		else
		{
			if ( TargetDistance() > 90 )
				TaskComplete();
			pev->ideal_yaw = UTIL_VecToYaw( m_hTargetEnt->pev->origin - pev->origin );
			ChangeYaw( pev->yaw_speed );
		}
		break;
	default:
		CTalkMonster::RunTask( pTask );
		break;
	}
}

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CScientist :: Classify ( void )
{
	return	CLASS_HUMAN_PASSIVE;
}


//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CScientist :: SetYawSpeed ( void )
{
	int ys;

	ys = 90;

	switch ( m_Activity )
	{
	case ACT_IDLE:
		ys = 120;
		break;
	case ACT_WALK:
		ys = 180;
		break;
	case ACT_RUN:
		ys = 150;
		break;
	case ACT_TURN_LEFT:
	case ACT_TURN_RIGHT:
		ys = 120;
		break;
	}

	pev->yaw_speed = ys;
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CScientist :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	switch( pEvent->event )
	{		
	case SCIENTIST_AE_HEAL:		// Heal my target (if within range)
		Heal();
		break;
	case SCIENTIST_AE_NEEDLEON:
		SetBodygroup( 1, 1 );
		break;
	case SCIENTIST_AE_NEEDLEOFF:
		SetBodygroup( 1, 0 );
		break;

	default:
		CTalkMonster::HandleAnimEvent( pEvent );
	}
}

//=========================================================
// Spawn
//=========================================================
void CScientist :: Spawn( void )
{
	Precache( );

	SET_MODEL(ENT(pev), "models/scientist.mdl");
	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= BLOOD_COLOR_RED;
	pev->health			= gSkillData.scientistHealth;
	pev->view_ofs		= Vector ( 0, 0, 50 );// position of the eyes relative to monster's origin.
	m_flFieldOfView		= VIEW_FIELD_WIDE; // NOTE: we need a wide field of view so scientists will notice player and say hello
	m_MonsterState		= MONSTERSTATE_NONE;

//	m_flDistTooFar		= 256.0;

	m_afCapability		= bits_CAP_HEAR | bits_CAP_TURN_HEAD | bits_CAP_OPEN_DOORS | bits_CAP_AUTO_DOORS | bits_CAP_USE;

	// White hands
	pev->skin = 0;

	if ( pev->body == -1 )
	{// -1 chooses a random head
		pev->body = RANDOM_LONG(0, NUM_SCIENTIST_HEADS-1);// pick a head, any head
	}

	// Luther is black, make his hands black
	if ( GetBodygroup( 0 ) == HEAD_LUTHER || GetBodygroup( 0 ) == HEAD_LUTHER + NUM_SCIENTIST_HEADS )
		pev->skin = 1;
	
	MonsterInit();
	SetUse( FollowerUse );

	if ( pev->spawnflags & SF_SCIENTIST_FEARPLAYER )
		Remember( bits_MEMORY_PROVOKED );
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CScientist :: Precache( void )
{
	PRECACHE_MODEL("models/scientist.mdl");
	PRECACHE_SOUND("scientist/sci_pain1.wav");
	PRECACHE_SOUND("scientist/sci_pain2.wav");
	PRECACHE_SOUND("scientist/sci_pain3.wav");
	PRECACHE_SOUND("scientist/sci_pain4.wav");
	PRECACHE_SOUND("scientist/sci_pain5.wav");

	m_iGlassGibs = PRECACHE_MODEL( "models/glassgibs.mdl" );
	PRECACHE_SOUND( "debris/bustglass1.wav" );
	PRECACHE_SOUND( "debris/bustglass2.wav" );
	PRECACHE_SOUND( "debris/bustglass3.wav" );

	// every new scientist must call this, otherwise
	// when a level is loaded, nobody will talk (time is reset to 0)
	TalkInit();

	CTalkMonster::Precache();
}	

// Init talk data
void CScientist :: TalkInit()
{
	
	CTalkMonster::TalkInit();

	// scientists speach group names (group names are in sentences.txt)

	m_szGrp[TLK_ANSWER]  =	"SC_ANSWER";
	m_szGrp[TLK_QUESTION] =	"SC_QUESTION";
	m_szGrp[TLK_IDLE] =		"SC_IDLE";
	m_szGrp[TLK_STARE] =	"SC_STARE";
	m_szGrp[TLK_USE] =		"SC_OK";
	m_szGrp[TLK_UNUSE] =	"SC_WAIT";
	m_szGrp[TLK_STOP] =		"SC_STOP";
	m_szGrp[TLK_NOSHOOT] =	"SC_SCARED";
	m_szGrp[TLK_HELLO] =	"SC_HELLO";

	m_szGrp[TLK_PLHURT1] =	"!SC_CUREA";
	m_szGrp[TLK_PLHURT2] =	"!SC_CUREB"; 
	m_szGrp[TLK_PLHURT3] =	"!SC_CUREC";

	m_szGrp[TLK_PHELLO] =	"SC_PHELLO";
	m_szGrp[TLK_PIDLE] =	"SC_PIDLE";
	m_szGrp[TLK_PQUESTION] = "SC_PQUEST";
	m_szGrp[TLK_SMELL] =	"SC_SMELL";
	
	m_szGrp[TLK_WOUND] =	"SC_WOUND";
	m_szGrp[TLK_MORTAL] =	"SC_MORTAL";

	// get voice for head
	switch (pev->body % 3)
	{
	default:
	case HEAD_GLASSES:	m_voicePitch = 105; break;	//glasses
	case HEAD_EINSTEIN: m_voicePitch = 100; break;	//einstein
	case HEAD_LUTHER:	m_voicePitch = 95;  break;	//luther
	case HEAD_SLICK:	m_voicePitch = 100;  break;//slick
	}
}

int CScientist :: TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType)
{
	if ( m_frozen )
	{
		// cold can't hurt a frozen scientist any more
		if ( bitsDamageType & (DMG_FREEZE | DMG_SLOWFREEZE) )
			return 0;

		m_frozen = FALSE;
		if ( bitsDamageType & (DMG_BURN | DMG_SLOWBURN) )
		{
			// thawed out: back to life
			pev->animtime = gpGlobals->time;
			ResetSequenceInfo( );
			SetThink( CallMonsterThink );
			pev->nextthink = gpGlobals->time + 0.1;
			pev->renderfx = kRenderFxNone;
		}
		else if ( bitsDamageType & DMG_FADE )
		{
			SUB_StartFadeOut();
			return 1;
		}
		else
		{
			Shatter();
			return 1;
		}
	}

	if ( bitsDamageType & DMG_ACID )
	{
		ALERT( at_console, "hitgroup %d\n", m_LastHitGroup );

		if ( m_LastHitGroup >= HITGROUP_LEFTLEG )
			m_flAcidDamageLower += flDamage;
		else if ( m_LastHitGroup == HITGROUP_STOMACH )
		{
			m_flAcidDamageUpper += flDamage;
			m_flAcidDamageLower += flDamage;
		}
		else if ( m_LastHitGroup > HITGROUP_GENERIC )
			m_flAcidDamageUpper += flDamage;
	}
	else if ( bitsDamageType & DMG_RADIATION )
	{
		pev->rendercolor = Vector( 0, 255, 0 );
		pev->renderamt = 10;
		pev->renderfx = kRenderFxGlowShell;
		m_glowTime = gpGlobals->time + 8;
	}
	else if ( bitsDamageType & (DMG_FREEZE | DMG_SLOWFREEZE) )
	{
		pev->rendercolor = Vector( 128, 192, 255 );
		pev->renderamt = 0;
		pev->renderfx = kRenderFxGlowShell;
		m_glowTime = gpGlobals->time + 4;
	}
	else if ( bitsDamageType & DMG_PARALYZE )
	{
		pev->rendercolor = Vector( 220, 140, 140 );
		pev->renderamt = 10;
		pev->renderfx = kRenderFxGlowShell;
		m_glowTime = gpGlobals->time + 4;
	}

	if ( pevAttacker && pevAttacker->flags & FL_CLIENT )
	{
		Remember( bits_MEMORY_PROVOKED );
		StopFollowing( TRUE );
	}

	// < 0: push the body back, > 0: the head flies off this fast
	float flForce = -IsAlive();

	if ( HasHead() && m_flHeadDamage > 0 && pev->health - flDamage <= -30 &&
		( pev->health - m_flHeadDamage > -30 || (bitsDamageType & DMG_NOGIB) ) &&
		!(bitsDamageType & (DMG_NEVERGIB | DMG_ALWAYSGIB)) )
	{
		// the head comes off
		bitsDamageType |= DMG_NEVERGIB;
		pev->body += NUM_SCIENTIST_HEADS;
		pev->health = 0;

		Vector vecOrigin, vecAngles;
		GetAttachment( 0, vecOrigin, vecAngles );
		SpawnBlood( vecOrigin, BLOOD_COLOR_RED, 250 );

		if ( pev->deadflag != DEAD_DEAD )
			PLAYBACK_EVENT_FULL( 0, edict(), g_iBloodSprayEvent, 0.0, pev->origin, pev->angles, 0.0, 0.0, 1, 0, 0, 0 );

		if ( bitsDamageType & (DMG_CRUSH | DMG_BLAST | DMG_ENERGYBEAM | DMG_ACID | DMG_MORTAR) )
			flForce = 0;
		else
			flForce = flDamage;
	}
	m_flHeadDamage = 0;

	if ( bitsDamageType & DMG_HALVE )
		bitsDamageType |= DMG_NEVERGIB;

	// make sure friends talk about it if player hurts scientist...
	int iRet = CTalkMonster::TakeDamage(pevInflictor, pevAttacker, flDamage, bitsDamageType);

	if ( (bitsDamageType & DMG_HALVE) && pev->health <= 0 && CanBeHalved() )
	{
		CutInHalf();
		return 1;
	}

	if ( (bitsDamageType & DMG_ACID) && pev->health <= 0 && CanMelt() )
	{
		AcidMelt();
		return 1;
	}

	if ( (bitsDamageType & (DMG_FREEZE | DMG_SLOWFREEZE)) && pev->health < 0 )
	{
		// frozen solid
		m_frozen = TRUE;
		StopAnimation();
		pev->velocity = g_vecZero;
		pev->avelocity = g_vecZero;
		pev->effects |= EF_NOINTERP;
		SetThink( SUB_StartFadeOut );
		pev->nextthink = gpGlobals->time + 10;
		return 1;
	}

	if ( flForce > 0 )
	{
		CGib::SpawnPoppedHead( pev, DamageForce( flForce ) );
		return iRet;
	}

	if ( flForce < 0 && !(bitsDamageType & DMG_NOPUSH) && pevInflictor && pevInflictor->solid != SOLID_TRIGGER && pevInflictor != pev && (!pevAttacker || pevAttacker->solid != SOLID_TRIGGER) )
	{
		pev->velocity = pev->velocity + g_vecAttackDir * -DamageForce( flDamage );
	}

	return iRet;
}


//=========================================================
// MonsterThink - the glow shell wears off, and scientists
// drown
//=========================================================
void CScientist :: MonsterThink ( void )
{
	if ( m_glowTime && m_glowTime < gpGlobals->time )
	{
		pev->renderfx = kRenderFxNone;
		m_glowTime = 0;
	}

	if ( pev->waterlevel != 3 )
	{
		pev->air_finished = gpGlobals->time + 12;
		pev->dmg = 0;
	}
	else if ( pev->air_finished < gpGlobals->time && pev->dmgtime < gpGlobals->time )
	{
		// drown
		pev->dmg += 1;
		if ( pev->dmg > 5 )
			pev->dmg = 5;
		TakeDamage( pev, pev, pev->dmg, DMG_DROWN );
		pev->dmgtime = gpGlobals->time + 1;
	}

	CBaseMonster::MonsterThink();
}


//=========================================================
// ISoundMask - returns a bit mask indicating which types
// of sounds this monster regards. In the base class implementation,
// monsters care about all sounds, but no scents.
//=========================================================
int CScientist :: ISoundMask ( void )
{
	return	bits_SOUND_WORLD	|
			bits_SOUND_COMBAT	|
			bits_SOUND_DANGER	|
			bits_SOUND_PLAYER;
}
	
//=========================================================
// PainSound
//=========================================================
void CScientist :: PainSound ( void )
{
	if (gpGlobals->time < m_painTime )
		return;
	
	m_painTime = gpGlobals->time + RANDOM_FLOAT(0.5, 0.75);

	switch (RANDOM_LONG(0,4))
	{
	case 0: EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "scientist/sci_pain1.wav", 1, ATTN_NORM, 0, GetVoicePitch()); break;
	case 1: EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "scientist/sci_pain2.wav", 1, ATTN_NORM, 0, GetVoicePitch()); break;
	case 2: EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "scientist/sci_pain3.wav", 1, ATTN_NORM, 0, GetVoicePitch()); break;
	case 3: EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "scientist/sci_pain4.wav", 1, ATTN_NORM, 0, GetVoicePitch()); break;
	case 4: EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "scientist/sci_pain5.wav", 1, ATTN_NORM, 0, GetVoicePitch()); break;
	}
}

//=========================================================
// DeathSound 
//=========================================================
void CScientist :: DeathSound ( void )
{
	PainSound();
}


void CScientist::Killed( entvars_t *pevAttacker, int iGib )
{
	if ( !HasMemory( bits_MEMORY_KILLED ) )
		DeathMessage( pevAttacker, g_pevLastInflictor, ShouldGibMonster( iGib ) );

	SetUse( NULL );	
	CTalkMonster::Killed( pevAttacker, iGib );
}


void CScientist :: SetActivity ( Activity newActivity )
{
	int	iSequence;

	iSequence = LookupActivity ( newActivity );

	// Set to the desired anim, or default anim if the desired is not present
	if ( iSequence == ACTIVITY_NOT_AVAILABLE )
		newActivity = ACT_IDLE;
	CTalkMonster::SetActivity( newActivity );
}


Schedule_t* CScientist :: GetScheduleOfType ( int Type )
{
	Schedule_t *psched;

	switch( Type )
	{
	// Hook these to make a looping schedule
	case SCHED_TARGET_FACE:
		// call base class default so that scientist will talk
		// when 'used' 
		psched = CTalkMonster::GetScheduleOfType(Type);

		if (psched == slIdleStand)
			return slFaceTarget;	// override this for different target face behavior
		else
			return psched;

	case SCHED_TARGET_CHASE:
		return slFollow;
	
	case SCHED_CANT_FOLLOW:
		return slStopFollowing;

	case SCHED_PANIC:
		return slSciPanic;

	case SCHED_TARGET_CHASE_SCARED:
		return slFollowScared;

	case SCHED_TARGET_FACE_SCARED:
		return slFaceTargetScared;

	case SCHED_IDLE_STAND:
		// call base class default so that scientist will talk
		// when standing during idle
		psched = CTalkMonster::GetScheduleOfType(Type);

		if (psched == slIdleStand)
			return slIdleSciStand;
		else
			return psched;

	case SCHED_HIDE:
		return slScientistHide;

	case SCHED_STARTLE:
		return slScientistStartle;

	case SCHED_FEAR:
		return slFear;

	case SCHED_IDLE_RUN:
		return slIdleRun;
	}

	return CTalkMonster::GetScheduleOfType( Type );
}

Schedule_t *CScientist :: GetSchedule ( void )
{
	// so we don't keep calling through the EHANDLE stuff
	CBaseEntity *pEnemy = m_hEnemy;

	if ( HasConditions( bits_COND_HEAR_SOUND ) )
	{
		CSound *pSound;
		pSound = PBestSound();

		ASSERT( pSound != NULL );
		if ( pSound && (pSound->m_iType & bits_SOUND_DANGER) )
			return GetScheduleOfType( SCHED_TAKE_COVER_FROM_BEST_SOUND );
	}

	switch( m_MonsterState )
	{
	case MONSTERSTATE_ALERT:	
	case MONSTERSTATE_IDLE:
		if ( pEnemy )
		{
			if ( HasConditions( bits_COND_SEE_ENEMY ) )
				m_fearTime = gpGlobals->time;
			else if ( DisregardEnemy( pEnemy ) )		// After 15 seconds of being hidden, return to alert
			{
				m_hEnemy = NULL;
				pEnemy = NULL;
			}
		}

		if ( HasConditions(bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE))
		{
			// flinch if hurt
			return GetScheduleOfType( SCHED_SMALL_FLINCH );
		}

		// Cower when you hear something scary
		if ( HasConditions( bits_COND_HEAR_SOUND ) )
		{
			CSound *pSound;
			pSound = PBestSound();

			ASSERT( pSound != NULL );
			if ( pSound )
			{
				if ( pSound->m_iType & (bits_SOUND_DANGER | bits_SOUND_COMBAT) )
				{
					if ( gpGlobals->time - m_fearTime > 3 )	// Only cower every 3 seconds or so
					{
						m_fearTime = gpGlobals->time;		// Update last fear
						return GetScheduleOfType( SCHED_STARTLE );	// This will just duck for a second
					}
				}
			}
		}

		// Behavior for following the player
		if ( IsFollowing() )
		{
			if ( !m_hTargetEnt->IsAlive() )
			{
				// UNDONE: Comment about the recently dead player here?
				StopFollowing( FALSE );
				break;
			}

			int relationship = R_NO;

			// Nothing scary, just me and the player
			if ( pEnemy != NULL )
				relationship = IRelationship( pEnemy );

			// UNDONE: Model fear properly, fix R_FR and add multiple levels of fear
			if ( relationship != R_DL && relationship != R_HT )
			{
				// If I'm already close enough to my target
				if ( TargetDistance() <= 128 )
				{
					if ( CanHeal() )	// Heal opportunistically
						return slHeal;
					if ( HasConditions( bits_COND_CLIENT_PUSH ) )	// Player wants me to move
						return GetScheduleOfType( SCHED_MOVE_AWAY_FOLLOW );
				}
				return GetScheduleOfType( SCHED_TARGET_FACE );	// Just face and follow.
			}
			else	// UNDONE: When afraid, scientist won't move out of your way.  Keep This?  If not, write move away scared
			{
				if ( HasConditions( bits_COND_NEW_ENEMY ) ) // I just saw something new and scary, react
					return GetScheduleOfType( SCHED_FEAR );					// React to something scary
				return GetScheduleOfType( SCHED_TARGET_FACE_SCARED );	// face and follow, but I'm scared!
			}
		}

		if ( HasConditions( bits_COND_CLIENT_PUSH ) )	// Player wants me to move
			return GetScheduleOfType( SCHED_MOVE_AWAY );

		// try to say something about smells
		TrySmellTalk();
		break;
	case MONSTERSTATE_COMBAT:
		if ( HasConditions( bits_COND_NEW_ENEMY ) )
			return slFear;					// Point and scream!
		if ( HasConditions( bits_COND_SEE_ENEMY ) )
			return slScientistCover;		// Take Cover
		
		if ( HasConditions( bits_COND_HEAR_SOUND ) )
			return slTakeCoverFromBestSound;	// Cower and panic from the scary sound!

		return slScientistCover;			// Run & Cower
		break;
	}
	
	return CTalkMonster::GetSchedule();
}

MONSTERSTATE CScientist :: GetIdealState ( void )
{
	if ( pev->health <= 0 )
	{
		m_IdealMonsterState = MONSTERSTATE_DEAD;
		return m_IdealMonsterState;
	}

	switch ( m_MonsterState )
	{
	case MONSTERSTATE_ALERT:
	case MONSTERSTATE_IDLE:
		if ( HasConditions( bits_COND_NEW_ENEMY ) )
		{
			if ( IsFollowing() )
			{
				int relationship = IRelationship( m_hEnemy );
				if ( relationship != R_FR || relationship != R_HT && !HasConditions( bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE ) )
				{
					// Don't go to combat if you're following the player
					m_IdealMonsterState = MONSTERSTATE_ALERT;
					return m_IdealMonsterState;
				}
				StopFollowing( TRUE );
			}
		}
		else if ( HasConditions( bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE ) )
		{
			// Stop following if you take damage
			if ( IsFollowing() )
				StopFollowing( TRUE );
		}
		break;

	case MONSTERSTATE_COMBAT:
		{
			CBaseEntity *pEnemy = m_hEnemy;
			if ( pEnemy != NULL )
			{
				if ( DisregardEnemy( pEnemy ) )		// After 15 seconds of being hidden, return to alert
				{
					// Strip enemy when going to alert
					m_IdealMonsterState = MONSTERSTATE_ALERT;
					m_hEnemy = NULL;
					return m_IdealMonsterState;
				}
				// Follow if only scared a little
				if ( m_hTargetEnt != NULL )
				{
					m_IdealMonsterState = MONSTERSTATE_ALERT;
					return m_IdealMonsterState;
				}

				if ( HasConditions ( bits_COND_SEE_ENEMY ) )
				{
					m_fearTime = gpGlobals->time;
					m_IdealMonsterState = MONSTERSTATE_COMBAT;
					return m_IdealMonsterState;
				}

			}
		}
		break;
	}

	return CTalkMonster::GetIdealState();
}


BOOL CScientist::CanHeal( void )
{ 
	if ( (m_healTime > gpGlobals->time) || (m_hTargetEnt == NULL) || (m_hTargetEnt->pev->health > (m_hTargetEnt->pev->max_health * 0.5)) )
		return FALSE;

	return TRUE;
}

void CScientist::Heal( void )
{
	if ( !CanHeal() )
		return;

	Vector target = m_hTargetEnt->pev->origin - pev->origin;
	if ( target.Length() > 100 )
		return;

	m_hTargetEnt->TakeHealth( gSkillData.scientistHeal, DMG_GENERIC );
	// Don't heal again for 1 minute
	m_healTime = gpGlobals->time + 60;
}

int CScientist::FriendNumber( int arrayNumber )
{
	static int array[3] = { 1, 2, 0 };
	if ( arrayNumber < 3 )
		return array[ arrayNumber ];
	return arrayNumber;
}


//=========================================================
// Dead Scientist PROP
//=========================================================
class CDeadScientist : public CBaseMonster
{
public:
	void Spawn( void );
	int	Classify ( void ) { return	CLASS_HUMAN_PASSIVE; }

	void KeyValue( KeyValueData *pkvd );
	int	m_iPose;// which sequence to display
	static char *m_szPoses[7];
};
char *CDeadScientist::m_szPoses[] = { "lying_on_back", "lying_on_stomach", "dead_sitting", "dead_hang", "dead_table1", "dead_table2", "dead_table3" };

void CDeadScientist::KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "pose"))
	{
		m_iPose = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "animation"))
	{
		// a custom pose: the first frame of this sequence
		pev->message = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CBaseMonster::KeyValue( pkvd );
}
LINK_ENTITY_TO_CLASS( monster_scientist_dead, CDeadScientist );

//
// ********** DeadScientist SPAWN **********
//
void CDeadScientist :: Spawn( )
{
	PRECACHE_MODEL("models/scientist.mdl");
	SET_MODEL(ENT(pev), "models/scientist.mdl");
	
	pev->effects		= 0;
	pev->sequence		= 0;
	// Corpses have less health
	pev->health			= 8;//gSkillData.scientistHealth;
	
	m_bloodColor = BLOOD_COLOR_RED;

	if ( pev->body == -1 )
	{// -1 chooses a random head
		pev->body = RANDOM_LONG(0, NUM_SCIENTIST_HEADS-1);// pick a head, any head
	}
	else if ( pev->body == -2 )
	{// -2 chooses a random headless body
		pev->body = RANDOM_LONG(0, NUM_SCIENTIST_HEADS-1) + NUM_SCIENTIST_HEADS;
	}
	// Luther is black, make his hands black
	if ( GetBodygroup( 0 ) == HEAD_LUTHER || GetBodygroup( 0 ) == HEAD_LUTHER + NUM_SCIENTIST_HEADS )
		pev->skin = 1;
	else
		pev->skin = 0;

	pev->sequence = LookupSequence( pev->message ? STRING( pev->message ) : m_szPoses[m_iPose] );
	if (pev->sequence == -1)
	{
		ALERT ( at_console, "Dead scientist with bad pose\n" );
	}

	//	pev->skin += 2; // use bloody skin -- UNDONE: Turn this back on when we have a bloody skin again!
	MonsterInitDead();
}


//=========================================================
// Sitting Scientist PROP
//=========================================================

class CSittingScientist : public CScientist // kdb: changed from public CBaseMonster so he can speak
{
public:
	void Spawn( void );
	void  Precache( void );

	void EXPORT SittingThink( void );
	int	Classify ( void );
	int GetBattlePower( void ) { return 0; }
	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	virtual void SetAnswerQuestion( CTalkMonster *pSpeaker );
	int FriendNumber( int arrayNumber );

	int FIdleSpeak ( void );
	int		m_baseSequence;	
	int		m_headTurn;
	float	m_flResponseDelay;
};

LINK_ENTITY_TO_CLASS( monster_sitting_scientist, CSittingScientist );
TYPEDESCRIPTION	CSittingScientist::m_SaveData[] = 
{
	// Don't need to save/restore m_baseSequence (recalced)
	DEFINE_FIELD( CSittingScientist, m_headTurn, FIELD_INTEGER ),
	DEFINE_FIELD( CSittingScientist, m_flResponseDelay, FIELD_FLOAT ),
};

IMPLEMENT_SAVERESTORE( CSittingScientist, CScientist );

// animation sequence aliases 
typedef enum
{
SITTING_ANIM_sitlookleft,
SITTING_ANIM_sitlookright,
SITTING_ANIM_sitscared,
SITTING_ANIM_sitting2,
SITTING_ANIM_sitting3
} SITTING_ANIM;


//
// ********** Scientist SPAWN **********
//
void CSittingScientist :: Spawn( )
{
	PRECACHE_MODEL("models/scientist.mdl");
	SET_MODEL(ENT(pev), "models/scientist.mdl");
	Precache();
	InitBoneControllers();

	UTIL_SetSize(pev, Vector(-14, -14, 0), Vector(14, 14, 36));

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	pev->effects		= 0;
	pev->health			= 50;
	
	m_bloodColor = BLOOD_COLOR_RED;
	m_flFieldOfView		= VIEW_FIELD_WIDE; // indicates the width of this monster's forward view cone ( as a dotproduct result )

	m_afCapability		= bits_CAP_HEAR | bits_CAP_TURN_HEAD;

	SetBits(pev->spawnflags, SF_MONSTER_PREDISASTER); // predisaster only!

	if ( pev->body == -1 )
	{// -1 chooses a random head
		pev->body = RANDOM_LONG(0, NUM_SCIENTIST_HEADS-1);// pick a head, any head
	}
	// Luther is black, make his hands black
	if ( GetBodygroup( 0 ) == HEAD_LUTHER || GetBodygroup( 0 ) == HEAD_LUTHER + NUM_SCIENTIST_HEADS )
		pev->skin = 1;
	
	m_baseSequence = LookupSequence( "sitlookleft" );
	pev->sequence = m_baseSequence + RANDOM_LONG(0,4);
	ResetSequenceInfo( );
	
	SetThink (SittingThink);
	pev->nextthink = gpGlobals->time + 0.1;

	DROP_TO_FLOOR ( ENT(pev) );
}

void CSittingScientist :: Precache( void )
{
	m_baseSequence = LookupSequence( "sitlookleft" );
	TalkInit();
}

//=========================================================
// ID as a passive human
//=========================================================
int	CSittingScientist :: Classify ( void )
{
	return	CLASS_HUMAN_PASSIVE;
}


int CSittingScientist::FriendNumber( int arrayNumber )
{
	static int array[3] = { 2, 1, 0 };
	if ( arrayNumber < 3 )
		return array[ arrayNumber ];
	return arrayNumber;
}



//=========================================================
// sit, do stuff
//=========================================================
void CSittingScientist :: SittingThink( void )
{
	CBaseEntity *pent;	

	StudioFrameAdvance( );

	// try to greet player
	if (FIdleHello())
	{
		pent = FindNearestFriend(TRUE);
		if (pent)
		{
			float yaw = VecToYaw(pent->pev->origin - pev->origin) - pev->angles.y;

			if (yaw > 180) yaw -= 360;
			if (yaw < -180) yaw += 360;
				
			if (yaw > 0)
				pev->sequence = m_baseSequence + SITTING_ANIM_sitlookleft;
			else
				pev->sequence = m_baseSequence + SITTING_ANIM_sitlookright;
		
		ResetSequenceInfo( );
		pev->frame = 0;
		SetBoneController( 0, 0 );
		}
	}
	else if (m_fSequenceFinished)
	{
		int i = RANDOM_LONG(0,99);
		m_headTurn = 0;
		
		if (m_flResponseDelay && gpGlobals->time > m_flResponseDelay)
		{
			// respond to question
			IdleRespond();
			pev->sequence = m_baseSequence + SITTING_ANIM_sitscared;
			m_flResponseDelay = 0;
		}
		else if (i < 30)
		{
			pev->sequence = m_baseSequence + SITTING_ANIM_sitting3;	

			// turn towards player or nearest friend and speak

			if (!FBitSet(m_bitsSaid, bit_saidHelloPlayer))
				pent = FindNearestFriend(TRUE);
			else
				pent = FindNearestFriend(FALSE);

			if (!FIdleSpeak() || !pent)
			{	
				m_headTurn = RANDOM_LONG(0,8) * 10 - 40;
				pev->sequence = m_baseSequence + SITTING_ANIM_sitting3;
			}
			else
			{
				// only turn head if we spoke
				float yaw = VecToYaw(pent->pev->origin - pev->origin) - pev->angles.y;

				if (yaw > 180) yaw -= 360;
				if (yaw < -180) yaw += 360;
				
				if (yaw > 0)
					pev->sequence = m_baseSequence + SITTING_ANIM_sitlookleft;
				else
					pev->sequence = m_baseSequence + SITTING_ANIM_sitlookright;

				//ALERT(at_console, "sitting speak\n");
			}
		}
		else if (i < 60)
		{
			pev->sequence = m_baseSequence + SITTING_ANIM_sitting3;	
			m_headTurn = RANDOM_LONG(0,8) * 10 - 40;
			if (RANDOM_LONG(0,99) < 5)
			{
				//ALERT(at_console, "sitting speak2\n");
				FIdleSpeak();
			}
		}
		else if (i < 80)
		{
			pev->sequence = m_baseSequence + SITTING_ANIM_sitting2;
		}
		else if (i < 100)
		{
			pev->sequence = m_baseSequence + SITTING_ANIM_sitscared;
		}

		ResetSequenceInfo( );
		pev->frame = 0;
		SetBoneController( 0, m_headTurn );
	}
	pev->nextthink = gpGlobals->time + 0.1;
}

// prepare sitting scientist to answer a question
void CSittingScientist :: SetAnswerQuestion( CTalkMonster *pSpeaker )
{
	m_flResponseDelay = gpGlobals->time + RANDOM_FLOAT(3, 4);
	m_hTalkTarget = (CBaseMonster *)pSpeaker;
}


//=========================================================
// FIdleSpeak
// ask question of nearby friend, or make statement
//=========================================================
int CSittingScientist :: FIdleSpeak ( void )
{ 
	// try to start a conversation, or make statement
	int pitch;
	
	if (!FOkToSpeak())
		return FALSE;

	// set global min delay for next conversation
	CTalkMonster::g_talkWaitTime = gpGlobals->time + RANDOM_FLOAT(4.8, 5.2);

	pitch = GetVoicePitch();
		
	// if there is a friend nearby to speak to, play sentence, set friend's response time, return

	// try to talk to any standing or sitting scientists nearby
	CBaseEntity *pentFriend = FindNearestFriend(FALSE);

	if (pentFriend && RANDOM_LONG(0,1))
	{
		CTalkMonster *pTalkMonster = GetClassPtr((CTalkMonster *)pentFriend->pev);
		pTalkMonster->SetAnswerQuestion( this );
		
		IdleHeadTurn(pentFriend->pev->origin);
		SENTENCEG_PlayRndSz( ENT(pev), m_szGrp[TLK_PQUESTION], 1.0, ATTN_IDLE, 0, pitch );
		// set global min delay for next conversation
		CTalkMonster::g_talkWaitTime = gpGlobals->time + RANDOM_FLOAT(4.8, 5.2);
		return TRUE;
	}

	// otherwise, play an idle statement
	if (RANDOM_LONG(0,1))
	{
		SENTENCEG_PlayRndSz( ENT(pev), m_szGrp[TLK_PIDLE], 1.0, ATTN_IDLE, 0, pitch );
		// set global min delay for next conversation
		CTalkMonster::g_talkWaitTime = gpGlobals->time + RANDOM_FLOAT(4.8, 5.2);
		return TRUE;
	}

	// never spoke
	CTalkMonster::g_talkWaitTime = 0;
	return FALSE;
}


//=========================================================
// GibMonster - ssh_scigibs pieces, the head unless it's
// already gone
//=========================================================
void CScientist :: GibMonster( void )
{
	SpawnBlood( Center(), BloodColor(), 250 );
	EMIT_SOUND(ENT(pev), CHAN_WEAPON, "common/bodysplat.wav", 1, ATTN_NORM);

	int cGibs = ssh_scigibs.value;
	int cSticky = RANDOM_LONG( 0, cGibs / 2 - 1 );

	if ( GetBodygroup( 0 ) < NUM_SCIENTIST_HEADS )
		CGib::SpawnHeadGib( pev );
	CGib::SpawnRandomGibs( pev, cGibs - cSticky, 1 );	// throw some human gibs.
	if ( cSticky > 0 )
		CGib::SpawnStickyGibs( pev, Center(), cSticky );

	SetThink ( SUB_Remove );
	pev->nextthink = gpGlobals->time;
}


//=========================================================
// Shatter - a frozen scientist breaks like glass
//=========================================================
void CScientist :: Shatter( void )
{
	switch ( RANDOM_LONG( 0, 2 ) )
	{
	case 0:	EMIT_SOUND(ENT(pev), CHAN_WEAPON, "debris/bustglass1.wav", 1, ATTN_NORM); break;
	case 1:	EMIT_SOUND(ENT(pev), CHAN_WEAPON, "debris/bustglass2.wav", 1, ATTN_NORM); break;
	case 2:	EMIT_SOUND(ENT(pev), CHAN_WEAPON, "debris/bustglass3.wav", 1, ATTN_NORM); break;
	}

	Vector vecSpot = Center();

	MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, vecSpot );
		WRITE_BYTE( TE_BREAKMODEL );

		// position
		WRITE_COORD( vecSpot.x );
		WRITE_COORD( vecSpot.y );
		WRITE_COORD( vecSpot.z );

		// size
		WRITE_COORD( pev->size.x );
		WRITE_COORD( pev->size.y );
		WRITE_COORD( pev->size.z );

		// velocity
		WRITE_COORD( 0 );
		WRITE_COORD( 0 );
		WRITE_COORD( 0 );

		// randomization
		WRITE_BYTE( 10 );

		// Model
		WRITE_SHORT( m_iGlassGibs );	//model id#

		// # of shards
		WRITE_BYTE( 32 );

		// duration
		WRITE_BYTE( 40 );// 4.0 seconds

		// flags
		WRITE_BYTE( BREAK_GLASS );
	MESSAGE_END();

	SetThink ( SUB_Remove );
	pev->nextthink = gpGlobals->time;
}


//=========================================================
// FadeMonster - the glow shell goes, and the corpse stays
// solid while it fades
//=========================================================
void CScientist :: FadeMonster( void )
{
	if ( m_glowTime )
	{
		pev->renderfx = kRenderFxNone;
		m_glowTime = 0;
	}

	StopAnimation();
	pev->movetype = MOVETYPE_TOSS;
	pev->animtime = gpGlobals->time;
	pev->effects |= EF_NOINTERP;
	SUB_StartFadeOut();
	pev->solid = SOLID_BBOX;
}


//=========================================================
// DeathMessage - the killer scores, the scoreboard counts
// the head, and everybody hears how it happened
//=========================================================
void CScientist :: DeathMessage( entvars_t *pevAttacker, entvars_t *pevInflictor, BOOL fGibbed )
{
	// Work out what killed the scientist, and tell everybody about it
	EHANDLE hKiller, hInflictor;
	char szText[ 512 ];

	if ( !pevAttacker )
		pevAttacker = pev;
	if ( !pevInflictor )
		pevInflictor = pevAttacker;

	hKiller = CBaseEntity::Instance( pevAttacker );
	hInflictor = CBaseEntity::Instance( pevInflictor );

	int iHitGroup = m_LastHitGroup;
	const char *pszVerb = DamageVerb( 0, FALSE );
	const char *pszWeapon = "";
	const char *pszVictim = GetHudName();
	const char *pszKiller = "";
	const char *pszHitGroup = "";

	if ( pevInflictor == pevAttacker && (pevAttacker->flags & FL_CLIENT) )
	{
		// If the inflictor is the killer,  then it must be their current weapon doing the damage
		CBasePlayer *pPlayer = (CBasePlayer*)CBaseEntity::Instance( pevAttacker );

		if ( pPlayer && pPlayer->m_pActiveItem )
			hInflictor = pPlayer->m_pActiveItem;
	}

	// a point for the player who did it
	if ( hKiller != NULL && hKiller->IsPlayer() )
		hKiller->AddPoints( 1, FALSE );
	else if ( m_hActivator != NULL && m_hActivator->IsPlayer() )
		m_hActivator->AddPoints( 1, FALSE );

	// the scoreboard counts the dead (and gibbed) per head
	int iHead = GetBodygroup( 0 ) % NUM_SCIENTIST_HEADS;
	CBasePlayer *pPlayer = (CBasePlayer *)CBaseEntity::Instance( INDEXENT( 1 ) );

	pPlayer->m_iSciDeaths[ iHead ]++;
	if ( fGibbed )
		pPlayer->m_iSciGibs[ iHead ]++;

	MESSAGE_BEGIN( MSG_ALL, gmsgSciInfo );
		WRITE_BYTE( iHead );
		WRITE_SHORT( pPlayer->m_iSciDeaths[ iHead ] );
		WRITE_SHORT( pPlayer->m_iSciGibs[ iHead ] );
	MESSAGE_END();

	if ( hInflictor )
	{
		pszWeapon = hInflictor->GetKillDesc();
		pszVerb = hInflictor->GetKillVerb( fGibbed, iHitGroup );
	}

	if ( hKiller )
		pszKiller = hKiller->GetKillDesc();

	if ( (CBaseEntity *)hKiller != this && pszKiller[0] )
	{
		if ( hInflictor )
			pszHitGroup = hInflictor->GetHitGroupDesc( iHitGroup );

		if ( (CBaseEntity *)hKiller != (CBaseEntity *)hInflictor && pszWeapon[0] )
			sprintf( szText, "%s %s %s%s with %s\n", pszKiller, pszVerb, pszVictim, pszHitGroup, pszWeapon );
		else if ( pszHitGroup[0] == ' ' )
			sprintf( szText, "%s was %s%s by %s\n", pszVictim, pszVerb, pszHitGroup, pszWeapon );
		else
			sprintf( szText, "%s%s was %s by %s\n", pszVictim, pszHitGroup, pszVerb, pszWeapon );
	}
	else
	{
		if ( VerbTakesName( pszVerb ) > 0 )
		{
			sprintf( szText, pszVerb, pszVictim );
			strcat( szText, "\n" );
		}
		else
			sprintf( szText, "%s %s\n", pszVictim, pszVerb );
	}

	UTIL_ClientPrintAll( HUD_PRINTNOTIFY, szText );
}


//=========================================================
// StartMonster - a path corner of speed 100 or more is run
//=========================================================
void CScientist :: StartMonster( void )
{
	CTalkMonster::StartMonster();

	if ( pev->target && m_pGoalEnt && m_pGoalEnt->pev->speed >= 100 )
	{
		m_movementActivity = ACT_RUN;
		ChangeSchedule( GetScheduleOfType( SCHED_IDLE_RUN ) );
	}
}


//=========================================================
// TraceAttack - no hits on parts that are gone; head hits
// add up for TakeDamage
//=========================================================
void CScientist :: TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType)
{
	if ( pev->takedamage == DAMAGE_NO )
		return;

	switch ( ptr->iHitgroup )
	{
	case HITGROUP_HEAD:
		if ( GetBodygroup( 0 ) >= 9 )
			return;
		m_flHeadDamage += flDamage;
		flDamage *= gSkillData.monHead;
		break;
	case HITGROUP_CHEST:
		if ( GetBodygroup( 2 ) == 1 )
			return;
		flDamage *= gSkillData.monChest;
		break;
	case HITGROUP_STOMACH:
		flDamage *= gSkillData.monStomach;
		break;
	case HITGROUP_LEFTARM:
	case HITGROUP_RIGHTARM:
		if ( GetBodygroup( 2 ) == 1 )
			return;
		flDamage *= gSkillData.monArm;
		break;
	case HITGROUP_LEFTLEG:
	case HITGROUP_RIGHTLEG:
		if ( GetBodygroup( 2 ) == 2 )
			return;
		flDamage *= gSkillData.monLeg;
		break;
	}

	m_LastHitGroup = ptr->iHitgroup;

	SpawnBlood(ptr->vecEndPos, BloodColor(), flDamage);// a little surface blood.
	TraceBleed( flDamage, vecDir, ptr, bitsDamageType );
	AddMultiDamage( pevAttacker, this, flDamage, bitsDamageType );
}


//=========================================================
// FOkToSpeak - not under water; and unlike a talkmonster's,
// even with an enemy in sight
//=========================================================
int CScientist :: FOkToSpeak( void )
{
	if ( pev->waterlevel == 3 )
		return FALSE;

	// if in the grip of a barnacle, don't speak
	if ( m_MonsterState == MONSTERSTATE_PRONE || m_IdealMonsterState == MONSTERSTATE_PRONE )
	{
		return FALSE;
	}

	// if not alive, certainly don't speak
	if ( pev->deadflag != DEAD_NO )
	{
		return FALSE;
	}

	// if someone else is talking, don't speak
	if (gpGlobals->time <= CTalkMonster::g_talkWaitTime)
		return FALSE;

	if ( pev->spawnflags & SF_MONSTER_GAG )
		return FALSE;

	// if player is not in pvs, don't speak
	if (FNullEnt(FIND_CLIENT_IN_PVS(edict())))
		return FALSE;

	return TRUE;
}


//=========================================================
// FCanActiveIdle - only when the node graph has any hint
// node of ours
//=========================================================
BOOL CScientist :: FCanActiveIdle ( void )
{
	static int iHintNodes = 0;	// name ours: 1 there are, -1 there aren't, 0 not looked yet

	if ( !iHintNodes )
	{
		ALERT( at_console, "%s queryhintnodeexistence\n", STRING( pev->classname ) );
		iHintNodes = HasHintNodes() ? 1 : -1;
	}

	if ( iHintNodes == -1 )
		return FALSE;

	return CTalkMonster::FCanActiveIdle();
}


//=========================================================
// Scientist in a hazard suit (class name ours)
//=========================================================
class CCleansuitScientist : public CScientist
{
public:
	void Spawn( void );
	void Precache( void );
	BOOL FValidateHintType( short sHint );
	BOOL FCanActiveIdle( void );

	void AcidMelt( void );
	BOOL CanHeal( void ) { return FALSE; }
	bool HasHead( void ) { return false; }
	bool CanBeHalved( void ) { return false; }
	bool CanMelt( void ) { return m_bloodColor != DONT_BLEED; }
};

LINK_ENTITY_TO_CLASS( monster_cleansuit_sci, CCleansuitScientist );

//=========================================================
// Spawn
//=========================================================
void CCleansuitScientist :: Spawn( void )
{
	Precache( );

	SET_MODEL(ENT(pev), "models/cleansuit_scientist.mdl");
	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= BLOOD_COLOR_RED;
	pev->health			= gSkillData.scientistHealth * 1.25;
	pev->view_ofs		= Vector ( 0, 0, 50 );// position of the eyes relative to monster's origin.
	m_flFieldOfView		= VIEW_FIELD_WIDE; // NOTE: we need a wide field of view so scientists will notice player and say hello
	m_MonsterState		= MONSTERSTATE_NONE;

	m_afCapability		= bits_CAP_HEAR | bits_CAP_TURN_HEAD | bits_CAP_OPEN_DOORS | bits_CAP_AUTO_DOORS | bits_CAP_USE;

	pev->skin = 0;

	if ( pev->body == -1 )
	{// -1 chooses a random head
		pev->body = RANDOM_LONG(0, NUM_SCIENTIST_HEADS-1);// pick a head, any head
	}

	MonsterInit();
	SetUse( FollowerUse );

	if ( pev->spawnflags & SF_SCIENTIST_FEARPLAYER )
		Remember( bits_MEMORY_PROVOKED );
}

//=========================================================
// Precache - the plain model is the acid skeleton's
//=========================================================
void CCleansuitScientist :: Precache( void )
{
	PRECACHE_MODEL("models/scientist.mdl");
	PRECACHE_MODEL("models/cleansuit_scientist.mdl");
	PRECACHE_SOUND("scientist/sci_pain1.wav");
	PRECACHE_SOUND("scientist/sci_pain2.wav");
	PRECACHE_SOUND("scientist/sci_pain3.wav");
	PRECACHE_SOUND("scientist/sci_pain4.wav");
	PRECACHE_SOUND("scientist/sci_pain5.wav");

	TalkInit();

	CTalkMonster::Precache();
}

//=========================================================
// FValidateHintType - the cleansuit's own work hints
//=========================================================
BOOL CCleansuitScientist :: FValidateHintType ( short sHint )
{
	int i;

	static short sCleansuitHints[] =
	{
		HINT_CLEANSUIT_WORK,
		HINT_CLEANSUIT_USE,
		HINT_CLEANSUIT_TRANSIT,
	};

	for ( i = 0 ; i < ARRAYSIZE ( sCleansuitHints ) ; i++ )
	{
		if ( sCleansuitHints[ i ] == sHint )
		{
			return TRUE;
		}
	}

	return CTalkMonster::FValidateHintType( sHint );
}

//=========================================================
// FCanActiveIdle
//=========================================================
BOOL CCleansuitScientist :: FCanActiveIdle ( void )
{
	static int iHintNodes = 0;	// name ours

	if ( !iHintNodes )
	{
		ALERT( at_console, "%s queryhintnodeexistence\n", STRING( pev->classname ) );
		iHintNodes = HasHintNodes() ? 1 : -1;
	}

	if ( iHintNodes == -1 )
		return FALSE;

	return CTalkMonster::FCanActiveIdle();
}


//=========================================================
// CutInHalf - the upper half becomes a scientist corpse of
// its own, this one keeps the legs
//=========================================================
void CScientist :: CutInHalf( void )
{
	int iBody = GetBodygroup( 2 );

	CScientist *pHalf = (CScientist *)CBaseEntity::Create( "monster_scientist", pev->origin, pev->angles, edict() );

	pHalf->pev->velocity = g_vecAttackDir * RANDOM_FLOAT( -40, 0 ) + Vector( pev->velocity.x + RANDOM_FLOAT( -10, 10 ), pev->velocity.y + RANDOM_FLOAT( -10, 10 ), pev->velocity.z + 200 );
	pHalf->pev->avelocity = Vector( pev->avelocity.x, pev->avelocity.y + RANDOM_FLOAT( 0, 20 ), pev->avelocity.z );
	pHalf->pev->movetype = MOVETYPE_TOSS;
	pHalf->pev->solid = SOLID_NOT;
	pHalf->pev->spawnflags |= SF_MONSTER_FADECORPSE | SF_MONSTER_FALL_TO_GROUND;
	pHalf->StartMonster();

	SpawnBlood( Center(), BLOOD_COLOR_RED, 1000 );

	pHalf->m_IdealActivity = GetDeathActivity();
	pHalf->SetActivity( pHalf->m_IdealActivity );

	pev->health = 0;
	pHalf->pev->health = 0;
	pHalf->Remember( bits_MEMORY_KILLED );
	pHalf->pev->deadflag = DEAD_DYING;
	m_IdealMonsterState = MONSTERSTATE_DEAD;
	pHalf->m_IdealMonsterState = MONSTERSTATE_DEAD;
	pHalf->pev->skin = pev->skin;
	pHalf->pev->body = pev->body;

	if ( iBody == 0 || iBody == 2 )
		pHalf->SetBodygroup( 2, 2 );
	else
	{
		pHalf->SetBodygroup( 2, 3 );
		pHalf->SetBodygroup( 2, 3 );	// sic: the skeleton was meant
	}

	pev->body = 0;
	SetBodygroup( 0, 9 );
	if ( iBody == 0 || iBody == 1 )
		SetBodygroup( 2, 1 );
	else
	{
		SetBodygroup( 2, 3 );
		SetBodygroup( 3, 2 );
	}
}


//=========================================================
// AcidMelt - the part the acid hit dissolves to bones; all
// of him when the other part had half his health in acid
//=========================================================
void CScientist :: AcidMelt( void )
{
	SetBodygroup( 1, 0 );	// no needle

	if ( m_LastHitGroup > HITGROUP_GENERIC && m_LastHitGroup != HITGROUP_STOMACH && m_LastHitGroup <= HITGROUP_RIGHTLEG )
	{
		if ( m_LastHitGroup >= HITGROUP_LEFTLEG )
		{
			if ( m_flAcidDamageUpper > pev->max_health * 0.5 )
			{
				SetBodygroup( 3, GetBodygroup( 2 ) + 1 );
				SetBodygroup( 2, 3 );
				SetBodygroup( 0, GetBodygroup( 0 ) < NUM_SCIENTIST_HEADS ? 8 : 9 );
				m_bloodColor = DONT_BLEED;
				return;
			}

			// the legs
			switch ( GetBodygroup( 2 ) )
			{
			case 0:
				SetBodygroup( 2, 2 );
				SetBodygroup( 3, 2 );
				break;
			case 1:
				SetBodygroup( 2, 3 );
				SetBodygroup( 3, 2 );
				break;
			}
			return;
		}

		if ( m_flAcidDamageLower <= pev->max_health * 0.5 )
		{
			// the upper body
			SetBodygroup( 0, GetBodygroup( 0 ) < NUM_SCIENTIST_HEADS ? 8 : 9 );
			switch ( GetBodygroup( 2 ) )
			{
			case 0:
				SetBodygroup( 2, 1 );
				SetBodygroup( 3, 3 );
				break;
			case 2:
				SetBodygroup( 2, 3 );
				SetBodygroup( 3, 3 );
				break;
			}
			return;
		}
	}

	// all of him
	SetBodygroup( 3, GetBodygroup( 2 ) + 1 );
	SetBodygroup( 2, 3 );
	SetBodygroup( 0, GetBodygroup( 0 ) < NUM_SCIENTIST_HEADS ? 8 : 9 );
	m_bloodColor = DONT_BLEED;
}


//=========================================================
// AcidMelt - the suit goes with him: a whole skeleton of the
// plain model
//=========================================================
void CCleansuitScientist :: AcidMelt( void )
{
	SET_MODEL( ENT(pev), "models/scientist.mdl" );
	UTIL_SetSize( pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX );
	pev->body = 8;
	SetBodygroup( 2, 3 );
	SetBodygroup( 3, 1 );
	m_bloodColor = DONT_BLEED;
}

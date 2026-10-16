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
// POV: monster_scientist_brave, a scientist that fights
// back with his fists (the zombie's claw sounds, the human
// grunt's kick). Built from the scientist, barney and the
// talking monster; file and class name and the names
// retail doesn't give are ours.
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
#include	"scientist.h"

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define		BRVSCI_AE_PUNCH		( 3 )

LINK_ENTITY_TO_CLASS( monster_scientist_brave, CBrvScientist );

//=========================================================
// AI Schedules Specific to this monster
//=========================================================
Task_t	tlBrvSciFollow[] =
{
	{ TASK_SET_FAIL_SCHEDULE,	(float)SCHED_CANT_FOLLOW },	// If you fail, bail out of follow
	{ TASK_MOVE_TO_TARGET_RANGE,(float)128		},	// Move within 128 of target ent (client)
//	{ TASK_SET_SCHEDULE,		(float)SCHED_TARGET_FACE },
};

Schedule_t	slBrvSciFollow[] =
{
	{
		tlBrvSciFollow,
		ARRAYSIZE ( tlBrvSciFollow ),
		bits_COND_NEW_ENEMY |
		bits_COND_LIGHT_DAMAGE |
		bits_COND_HEAVY_DAMAGE |
		bits_COND_HEAR_SOUND,
		bits_SOUND_COMBAT |
		bits_SOUND_DANGER,
		"BrvSciFollow"
	},
};


Task_t	tlBrvSciPanic[] =
{
	{ TASK_STOP_MOVING,			(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_SCREAM,				(float)0		},
	{ TASK_PLAY_SEQUENCE_FACE_ENEMY,		(float)ACT_RESET	},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE	},
};

Schedule_t	slBrvSciPanic[] =
{
	{
		tlBrvSciPanic,
		ARRAYSIZE ( tlBrvSciPanic ),
		0,
		0,
		"BrvSciPanic"
	},
};


Task_t	tlBrvSciCover[] =
{
	{ TASK_SET_FAIL_SCHEDULE,		(float)SCHED_PANIC },		// If you fail, just panic!
	{ TASK_STOP_MOVING,				(float)0					},
	{ TASK_FIND_COVER_FROM_ENEMY,	(float)0					},
	{ TASK_RUN_PATH,				(float)0					},
	{ TASK_TURN_LEFT,				(float)179					},
	{ TASK_SET_SCHEDULE,			(float)SCHED_HIDE			},
};

Schedule_t	slBrvSciCover[] =
{
	{
		tlBrvSciCover,
		ARRAYSIZE ( tlBrvSciCover ),
		bits_COND_NEW_ENEMY,
		0,
		"BrvSciCover"
	},
};


Task_t	tlBrvSciHide[] =
{
	{ TASK_SET_FAIL_SCHEDULE,		(float)SCHED_PANIC },		// If you fail, just panic!
	{ TASK_STOP_MOVING,				(float)0					},
	{ TASK_SET_ACTIVITY,			(float)ACT_IDLE				},
	{ TASK_WAIT_RANDOM,				(float)10.0					},
};

Schedule_t	slBrvSciHide[] =
{
	{
		tlBrvSciHide,
		ARRAYSIZE ( tlBrvSciHide ),
		bits_COND_NEW_ENEMY |
		bits_COND_HEAR_SOUND |
		bits_COND_SEE_ENEMY |
		bits_COND_SEE_HATE |
		bits_COND_SEE_FEAR |
		bits_COND_SEE_DISLIKE,
		bits_SOUND_DANGER,
		"BrvSciHide"
	},
};


Task_t	tlBrvSciStartle[] =
{
	{ TASK_SET_FAIL_SCHEDULE,		(float)SCHED_PANIC },		// If you fail, just panic!
	{ TASK_RANDOM_SCREAM,			(float)0.3 },				// Scream 30% of the time
	{ TASK_STOP_MOVING,				(float)0					},
	{ TASK_PLAY_SEQUENCE_FACE_ENEMY,			(float)ACT_RESET			},
	{ TASK_RANDOM_SCREAM,			(float)0.1 },				// Scream again 10% of the time
	{ TASK_PLAY_SEQUENCE_FACE_ENEMY,			(float)ACT_IDLE		},
	{ TASK_WAIT_RANDOM,				(float)1.0					},
};

Schedule_t	slBrvSciStartle[] =
{
	{
		tlBrvSciStartle,
		ARRAYSIZE ( tlBrvSciStartle ),
		bits_COND_NEW_ENEMY |
		bits_COND_SEE_ENEMY |
		bits_COND_SEE_HATE |
		bits_COND_SEE_FEAR |
		bits_COND_SEE_DISLIKE,
		0,
		"BrvSciStartle"
	},
};


Task_t	tlBrvSciFear[] =
{
	{ TASK_STOP_MOVING,				(float)0					},
	{ TASK_FACE_ENEMY,				(float)0					},
	{ TASK_SAY_FEAR,				(float)0					},
};

Schedule_t	slBrvSciFear[] =
{
	{
		tlBrvSciFear,
		ARRAYSIZE ( tlBrvSciFear ),
		bits_COND_NEW_ENEMY,
		0,
		"BrvSciFear"
	},
};


Task_t	tlBrvSciFaceTarget[] =
{
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE },
	{ TASK_FACE_TARGET,			(float)0		},
	{ TASK_SET_SCHEDULE,		(float)SCHED_TARGET_CHASE },
};

Schedule_t	slBrvSciFaceTarget[] =
{
	{
		tlBrvSciFaceTarget,
		ARRAYSIZE ( tlBrvSciFaceTarget ),
		bits_COND_CLIENT_PUSH	|
		bits_COND_NEW_ENEMY		|
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE	|
		bits_COND_HEAR_SOUND	|
		bits_COND_PROVOKED,
		bits_SOUND_DANGER,
		"FaceTarget"
	},
};


Task_t	tlBrvSciIdleStand[] =
{
	{ TASK_STOP_MOVING,			0				},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE },
	{ TASK_WAIT,				(float)2		}, // repick IDLESTAND every two seconds.
	{ TASK_TLK_HEADRESET,		(float)0		}, // reset head position
};

Schedule_t	slBrvSciIdleStand[] =
{
	{
		tlBrvSciIdleStand,
		ARRAYSIZE ( tlBrvSciIdleStand ),
		bits_COND_NEW_ENEMY		|
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE	|
		bits_COND_HEAR_SOUND	|
		bits_COND_SMELL			|
		bits_COND_PROVOKED,

		bits_SOUND_COMBAT		|// sound flags
		bits_SOUND_DANGER		|
		bits_SOUND_MEAT			|// scents
		bits_SOUND_CARCASS		|
		bits_SOUND_GARBAGE,
		"IdleStand"
	},
};


Task_t	tlBrvSciIdleSpeak[] =
{
	{ TASK_TLK_SPEAK,		(float)0		},// question or remark
	{ TASK_TLK_IDEALYAW,	(float)0		},// look at who I'm talking to
	{ TASK_FACE_IDEAL,		(float)0		},
	{ TASK_SET_ACTIVITY,	(float)ACT_IDLE	},
	{ TASK_TLK_EYECONTACT,	(float)0		},
	{ TASK_WAIT_RANDOM,		(float)0.5		},
};

Schedule_t	slBrvSciIdleSpeak[] =
{
	{
		tlBrvSciIdleSpeak,
		ARRAYSIZE ( tlBrvSciIdleSpeak ),
		bits_COND_NEW_ENEMY		|
		bits_COND_CLIENT_PUSH	|
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE,
		0,
		"BrvSciIdleSpeak"
	},
};


Task_t	tlBrvSciTlkIdleEyecontact[] =
{
	{ TASK_TLK_IDEALYAW,	(float)0		},// look at who I'm talking to
	{ TASK_FACE_IDEAL,		(float)0		},
	{ TASK_SET_ACTIVITY,	(float)ACT_IDLE	},
	{ TASK_TLK_EYECONTACT,	(float)0		},// Wait until speaker is done
};

Schedule_t	slBrvSciTlkIdleEyecontact[] =
{
	{
		tlBrvSciTlkIdleEyecontact,
		ARRAYSIZE ( tlBrvSciTlkIdleEyecontact ),
		bits_COND_NEW_ENEMY		|
		bits_COND_CLIENT_PUSH	|
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE,
		0,
		"BrvSciTlkIdleEyecontact"
	},
};


Task_t	tlBrvSciIdleSpeakWait[] =
{
	{ TASK_SET_ACTIVITY,	(float)ACT_IDLE	},// Stop and talk
	{ TASK_TLK_SPEAK,		(float)0		},// question or remark
	{ TASK_TLK_EYECONTACT,	(float)0		},//
	{ TASK_WAIT,			(float)2		},// wait - used when sci is in 'use' mode to keep head turned
};

Schedule_t	slBrvSciIdleSpeakWait[] =
{
	{
		tlBrvSciIdleSpeakWait,
		ARRAYSIZE ( tlBrvSciIdleSpeakWait ),
		bits_COND_NEW_ENEMY		|
		bits_COND_CLIENT_PUSH	|
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE,
		0,
		"Idle Speak Wait"
	},
};


Task_t	tlBrvSciIdleResponse[] =
{
	{ TASK_SET_ACTIVITY,	(float)ACT_IDLE	},// Stop and listen
	{ TASK_WAIT,			(float)0.5		},// Wait until sure it's me they are talking to
	{ TASK_TLK_EYECONTACT,	(float)0		},// Wait until speaker is done
	{ TASK_TLK_RESPOND,		(float)0		},// Wait and then say my response
	{ TASK_TLK_IDEALYAW,	(float)0		},// look at who I'm talking to
	{ TASK_FACE_IDEAL,		(float)0		},
	{ TASK_SET_ACTIVITY,	(float)ACT_IDLE	},
	{ TASK_TLK_EYECONTACT,	(float)0		},// Wait until speaker is done
};

Schedule_t	slBrvSciIdleResponse[] =
{
	{
		tlBrvSciIdleResponse,
		ARRAYSIZE ( tlBrvSciIdleResponse ),
		bits_COND_NEW_ENEMY		|
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE,
		0,
		"Idle Response"

	},
};


DEFINE_CUSTOM_SCHEDULES( CBrvScientist )
{
	slBrvSciFollow,
	slBrvSciPanic,
	slBrvSciCover,
	slBrvSciHide,
	slBrvSciStartle,
	slBrvSciFear,
	slBrvSciFaceTarget,
	slBrvSciIdleStand,
	slBrvSciIdleSpeak,
	slBrvSciTlkIdleEyecontact,
	slBrvSciIdleSpeakWait,
	slBrvSciIdleResponse,
};


IMPLEMENT_CUSTOM_SCHEDULES( CBrvScientist, CScientist );


//=========================================================
// Spawn
//=========================================================
void CBrvScientist :: Spawn( void )
{
	CScientist::Spawn();

	SET_MODEL(ENT(pev), "models/povscientist.mdl");
	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	pev->body = (pev->body % NUM_SCIENTIST_HEADS) + NUM_SCIENTIST_HEADS * 1;

	m_afCapability |= bits_CAP_MELEE_ATTACK1;
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CBrvScientist :: Precache( void )
{
	PRECACHE_MODEL("models/povscientist.mdl");

	PRECACHE_SOUND("zombie/claw_miss1.wav");
	PRECACHE_SOUND("zombie/claw_miss2.wav");
	PRECACHE_SOUND("zombie/claw_strike1.wav");
	PRECACHE_SOUND("zombie/claw_strike2.wav");
	PRECACHE_SOUND("zombie/claw_strike3.wav");

	PRECACHE_SOUND("scientist/sci_pain1.wav");
	PRECACHE_SOUND("scientist/sci_pain2.wav");
	PRECACHE_SOUND("scientist/sci_pain3.wav");
	PRECACHE_SOUND("scientist/sci_pain4.wav");
	PRECACHE_SOUND("scientist/sci_pain5.wav");

	CScientist::Precache();

	TalkInit();
}

// Init talk data
void CBrvScientist :: TalkInit()
{
	CScientist::TalkInit();

	m_szGrp[TLK_ATTACK] =	"SC_ATTACK";
	m_szGrp[TLK_KILL] =		"SC_KILL";
}

//=========================================================
// AlertSound
//=========================================================
void CBrvScientist :: AlertSound( void )
{
	if ( m_hEnemy != NULL )
	{
		if ( m_MonsterState != MONSTERSTATE_PRONE && m_IdealMonsterState != MONSTERSTATE_PRONE &&
			 pev->deadflag == DEAD_NO && !(pev->spawnflags & SF_MONSTER_GAG) && IsAlive() )
		{
			PlaySentence( m_szGrp[TLK_ATTACK], RANDOM_FLOAT(3, 4.5), VOL_NORM, ATTN_IDLE );
		}
	}
}

//=========================================================
// AttackMissSound, AttackHitSound - the zombie's claws
//=========================================================
void CBrvScientist :: AttackMissSound( void )
{
	if ( RANDOM_LONG(0,1) )
		EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, "zombie/claw_miss2.wav", 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
	else
		EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, "zombie/claw_miss1.wav", 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
}

void CBrvScientist :: AttackHitSound( void )
{
	switch ( RANDOM_LONG(0,2) )
	{
	case 0:
		EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, "zombie/claw_strike1.wav", 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
		break;
	case 1:
		EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, "zombie/claw_strike2.wav", 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
		break;
	default:
		EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, "zombie/claw_strike3.wav", 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
		break;
	}
}

//=========================================================
// Kick and return if you hit anything (the human grunt's)
//=========================================================
CBaseEntity *CBrvScientist :: Kick( void )
{
	TraceResult tr;

	UTIL_MakeVectors( pev->angles );
	Vector vecStart = pev->origin;
	vecStart.z += pev->size.z * 0.5;
	Vector vecEnd = vecStart + (gpGlobals->v_forward * 70);

	UTIL_TraceHull( vecStart, vecEnd, dont_ignore_monsters, head_hull, ENT(pev), &tr );

	if ( tr.pHit )
	{
		CBaseEntity *pEntity = CBaseEntity::Instance( tr.pHit );
		return pEntity;
	}

	return NULL;
}

//=========================================================
// CheckMeleeAttack1
//=========================================================
BOOL CBrvScientist :: CheckMeleeAttack1 ( float flDot, float flDist )
{
	if ( m_hEnemy != NULL && flDist <= 64 && flDot >= 0.7 &&
		 m_hEnemy->Classify() != CLASS_ALIEN_BIOWEAPON &&
		 m_hEnemy->Classify() != CLASS_PLAYER_BIOWEAPON )
	{
		return TRUE;
	}
	return FALSE;
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CBrvScientist :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	switch( pEvent->event )
	{
	case BRVSCI_AE_PUNCH:
		{
			CBaseEntity *pHurt = Kick();

			if ( pHurt )
			{
				AttackHitSound();

				UTIL_MakeVectors( pev->angles );
				pHurt->pev->punchangle.x = 15;
				pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_forward * 100 + gpGlobals->v_up * 50;
				pHurt->TakeDamage( pev, pev, gSkillData.scientistHeal, DMG_POISON );
			}
			else
				AttackMissSound();
		}
		break;

	default:
		CScientist::HandleAnimEvent( pEvent );
		break;
	}
}

// the talking monster's, with this file's schedule
void CBrvScientist :: SetAnswerQuestion( CTalkMonster *pSpeaker )
{
	if ( !m_pCine )
		ChangeSchedule( slBrvSciIdleResponse );
	m_hTalkTarget = (CBaseMonster *)pSpeaker;
}

//=========================================================
// Classify - indicates this monster's place in the
// relationship table.
//=========================================================
int	CBrvScientist :: Classify ( void )
{
	return	CLASS_PLAYER_ALLY;
}

//=========================================================
// SetActivity (the human grunt's)
//=========================================================
void CBrvScientist :: SetActivity ( Activity NewActivity )
{
	int	iSequence = ACTIVITY_NOT_AVAILABLE;
	void *pmodel = GET_MODEL_PTR( ENT(pev) );

	switch ( NewActivity )
	{
	case ACT_MELEE_ATTACK1:
		iSequence = LookupSequence( "venom" );
		break;
	default:
		iSequence = LookupActivity ( NewActivity );
		break;
	}

	m_Activity = NewActivity; // Go ahead and set this so it doesn't keep trying when the anim is not present

	// Set to the desired anim, or default anim if the desired is not present
	if ( iSequence > ACTIVITY_NOT_AVAILABLE )
	{
		if ( pev->sequence != iSequence || !m_fSequenceLoops )
		{
			pev->frame = 0;
		}

		pev->sequence		= iSequence;	// Set to the reset anim (if it's there)
		ResetSequenceInfo( );
		SetYawSpeed();
	}
	else
	{
		pev->sequence		= 0;	// Set to the reset anim (if it's there)
	}
}

//=========================================================
// GetSchedule (barney's)
//=========================================================
Schedule_t *CBrvScientist :: GetSchedule ( void )
{
	if ( HasConditions( bits_COND_HEAR_SOUND ) )
	{
		CSound *pSound;
		pSound = PBestSound();

		ASSERT( pSound != NULL );
		if ( pSound && (pSound->m_iType & bits_SOUND_DANGER) )
			return GetScheduleOfType( SCHED_TAKE_COVER_FROM_BEST_SOUND );
	}
	if ( HasConditions( bits_COND_ENEMY_DEAD ) && FOkToSpeak() )
	{
		PlaySentence( m_szGrp[TLK_KILL], 4, VOL_NORM, ATTN_NORM );
	}

	switch( m_MonsterState )
	{
	case MONSTERSTATE_COMBAT:
		{
// dead enemy
			if ( HasConditions( bits_COND_ENEMY_DEAD ) )
			{
				// call base class, all code to handle dead enemies is centralized there.
				return CBaseMonster :: GetSchedule();
			}

			if ( HasConditions( bits_COND_NEW_ENEMY ) )
				AlertSound();

			// always act surprized with a new enemy
			if ( HasConditions( bits_COND_NEW_ENEMY ) && HasConditions( bits_COND_LIGHT_DAMAGE) )
				return GetScheduleOfType( SCHED_SMALL_FLINCH );

			if ( HasConditions( bits_COND_HEAVY_DAMAGE ) )
				return GetScheduleOfType( SCHED_TAKE_COVER_FROM_ENEMY );

			if ( HasConditions( bits_COND_CAN_MELEE_ATTACK1 ) )
				return GetScheduleOfType( SCHED_MELEE_ATTACK1 );
		}
		break;

	case MONSTERSTATE_ALERT:
	case MONSTERSTATE_IDLE:
		if ( HasConditions(bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE))
		{
			// flinch if hurt
			return GetScheduleOfType( SCHED_SMALL_FLINCH );
		}

		// try to say something about smells
		TrySmellTalk();
		break;
	}

	return CTalkMonster::GetSchedule();
}

Schedule_t* CBrvScientist :: GetScheduleOfType ( int Type )
{
	switch( Type )
	{
	case SCHED_TARGET_FACE:
		// speak during 'use'
		if (RANDOM_LONG(0,99) < 2)
			return slBrvSciIdleSpeakWait;
		else
			return slBrvSciFaceTarget;

	case SCHED_TARGET_CHASE:
		return slBrvSciFollow;

	case SCHED_PANIC:
		return slBrvSciPanic;

	case SCHED_TARGET_CHASE_SCARED:
		return slBrvSciFollow;

	case SCHED_TARGET_FACE_SCARED:
		return slBrvSciFaceTarget;

	case SCHED_IDLE_STAND:
		{
			// sustained light wounds?
			if (!FBitSet(m_bitsSaid, bit_saidWoundLight) && (pev->health <= (pev->max_health * 0.75)))
			{
				PlaySentence( m_szGrp[TLK_WOUND], RANDOM_FLOAT(2.8, 3.2), VOL_NORM, ATTN_IDLE );
				SetBits(m_bitsSaid, bit_saidWoundLight);
				return slBrvSciIdleStand;
			}
			// sustained heavy wounds?
			else if (!FBitSet(m_bitsSaid, bit_saidWoundHeavy) && (pev->health <= (pev->max_health * 0.5)))
			{
				PlaySentence( m_szGrp[TLK_MORTAL], RANDOM_FLOAT(2.8, 3.2), VOL_NORM, ATTN_IDLE );
				SetBits(m_bitsSaid, bit_saidWoundHeavy);
				return slBrvSciIdleStand;
			}

			// talk about world
			if (FOkToSpeak() && RANDOM_LONG(0,m_nSpeak * 2) == 0)
			{
				return slBrvSciIdleSpeak;
			}

			if (IsTalking())
				// look at who we're talking to
				return slBrvSciTlkIdleEyecontact;
			else
				// regular standing idle
				return slBrvSciIdleStand;
		}
		break;

	case SCHED_HIDE:
		return slBrvSciHide;

	case SCHED_STARTLE:
		return slBrvSciStartle;

	case SCHED_FEAR:
		return slBrvSciFear;
	}

	return CTalkMonster::GetScheduleOfType( Type );
}

MONSTERSTATE CBrvScientist :: GetIdealState ( void )
{
	return CTalkMonster::GetIdealState();
}

void CBrvScientist :: StartTask( Task_t *pTask )
{
	switch( pTask->iTask )
	{
	case TASK_RUN_PATH_SCARED:
		m_movementActivity = ACT_RUN;
		break;

	case TASK_MOVE_TO_TARGET_RANGE_SCARED:
		{
			if ( (m_hTargetEnt->pev->origin - pev->origin).Length() < 1 )
				TaskComplete();
			else
			{
				m_vecMoveGoal = m_hTargetEnt->pev->origin;
				if ( !MoveToTarget( ACT_WALK, 0.5 ) )
					TaskFail();
			}
		}
		break;

	default:
		CTalkMonster::StartTask( pTask );
		break;
	}
}

void CBrvScientist :: RunTask( Task_t *pTask )
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
				else if ( distance < 190 && m_movementActivity != ACT_WALK_SCARED && m_movementActivity != ACT_WALK )
					m_movementActivity = ACT_WALK;
				else if ( distance >= 270 && m_movementActivity != ACT_RUN_SCARED && m_movementActivity != ACT_RUN )
					m_movementActivity = ACT_RUN;
			}
		}
		break;

	default:
		CTalkMonster::RunTask( pTask );
		break;
	}
}

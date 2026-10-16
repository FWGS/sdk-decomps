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
// Wanted: the small miner (monster_smallminer) and his corpse
// (monster_smallminer_dead). smallminer.cpp with the small miner's model,
// health and pick, and the grunt's own grenade timings. The file name,
// CDeadSmallminer, g_fSmallminerQuestion, pSmallminerSentences and the
// schedule array names are ours.
//=========================================================

//=========================================================
// Hit groups!
//=========================================================
/*

  1 - Head
  2 - Stomach
  3 - Gun

*/


#include	"extdll.h"
#include	"plane.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"schedule.h"
#include	"animation.h"
#include	"squadmonster.h"
#include	"weapons.h"
#include	"talkmonster.h"
#include	"soundent.h"
#include	"effects.h"
#include	"customentity.h"

int g_fSmallminerQuestion;				// true if an idle miner asked a question. Cleared when someone answers.

extern DLL_GLOBAL int		g_iSkillLevel;

//=========================================================
// monster-specific DEFINE's
//=========================================================
#define SMALLMINER_VOL					0.35		// volume of miner sounds
#define SMALLMINER_ATTN					ATTN_NORM	// attenutation of miner sentences
#define SMALLMINER_LIMP_HEALTH			20
#define	SMALLMINER_SENTENCE_VOLUME		(float)0.35 // volume of miner sentences

#define HEAD_GROUP					1

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define		SMALLMINER_AE_KICK			( 3 )
#define		SMALLMINER_AE_GREN_TOSS		( 7 )
#define		SMALLMINER_AE_GREN_DROP		( 9 )
#define		SMALLMINER_AE_CAUGHT_ENEMY	( 10) // miner established sight with an enemy (player only) that had previously eluded the squad.

//=========================================================
// monster-specific schedule types
//=========================================================
enum
{
	// sic: the first value lost hgrunt's "= LAST_COMMON_SCHEDULE + 1", so
	// these overlap SCHED_NONE ... SCHED_ALERT_FACE
	SCHED_SMALLMINER_ESTABLISH_LINE_OF_FIRE,// move to a location to set up an attack against the enemy. (usually when a friendly is in the way).
	SCHED_SMALLMINER_SWEEP,
	SCHED_SMALLMINER_FOUND_ENEMY,
	SCHED_SMALLMINER_WAIT_FACE_ENEMY,
	SCHED_SMALLMINER_TAKECOVER_FAILED,// special schedule type that forces analysis of conditions and picks the best possible schedule to recover from this type of failure.
	SCHED_SMALLMINER_ELOF_FAIL,
};

//=========================================================
// monster-specific tasks
//=========================================================
enum
{
	TASK_SMALLMINER_FACE_TOSS_DIR = LAST_COMMON_TASK + 1,
	TASK_SMALLMINER_SPEAK_SENTENCE,
	TASK_SMALLMINER_CHECK_FIRE,
};

//=========================================================
// monster-specific conditions
//=========================================================
#define bits_COND_SMALLMINER_NOFIRE	( bits_COND_SPECIAL1 )

class CSmallminer : public CSquadMonster
{
public:
	void Spawn( void );
	void Precache( void );
	void SetYawSpeed ( void );
	int  Classify ( void );
	int ISoundMask ( void );
	void HandleAnimEvent( MonsterEvent_t *pEvent );
	BOOL FCanCheckAttacks ( void );
	BOOL CheckMeleeAttack1 ( float flDot, float flDist );
	BOOL CheckRangeAttack1 ( float flDot, float flDist ) { return FALSE; }
	BOOL CheckRangeAttack2 ( float flDot, float flDist );
	void SetActivity ( Activity NewActivity );
	void StartTask ( Task_t *pTask );
	void RunTask ( Task_t *pTask );
	void DeathSound( void );
	void PainSound( void );
	void IdleSound ( void );
	Vector GetGunPosition( void );
	void PrescheduleThink ( void );
	void SpeakSentence( void );

	int	Save( CSave &save );
	int Restore( CRestore &restore );

	CBaseEntity	*Kick( void );
	Schedule_t	*GetSchedule( void );
	Schedule_t  *GetScheduleOfType ( int Type );
	void TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType);
	int TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );

	BOOL FOkToSpeak( void );
	void JustSpoke( void );

	CUSTOM_SCHEDULES;
	static TYPEDESCRIPTION m_SaveData[];

	// checking the feasibility of a grenade toss is kind of costly, so we do it every couple of seconds,
	// not every server frame.
	float m_flNextGrenadeCheck;
	float m_flNextPainTime;
	float m_flLastEnemySightTime;

	Vector	m_vecTossVelocity;

	BOOL	m_fThrowGrenade;
	BOOL	m_fFirstEncounter;// only put on the handsign show in the squad's first encounter.

	int m_voicePitch;

	int		m_iSentence;

	static const char *pSmallminerSentences[];
};

LINK_ENTITY_TO_CLASS( monster_smallminer, CSmallminer );

TYPEDESCRIPTION	CSmallminer::m_SaveData[] =
{
	DEFINE_FIELD( CSmallminer, m_flNextGrenadeCheck, FIELD_TIME ),
	DEFINE_FIELD( CSmallminer, m_flNextPainTime, FIELD_TIME ),
	DEFINE_FIELD( CSmallminer, m_flLastEnemySightTime, FIELD_TIME ),
	DEFINE_FIELD( CSmallminer, m_vecTossVelocity, FIELD_VECTOR ),
	DEFINE_FIELD( CSmallminer, m_fThrowGrenade, FIELD_BOOLEAN ),
	DEFINE_FIELD( CSmallminer, m_fFirstEncounter, FIELD_BOOLEAN ),
	DEFINE_FIELD( CSmallminer, m_voicePitch, FIELD_INTEGER ),
	DEFINE_FIELD( CSmallminer, m_iSentence, FIELD_INTEGER ),
};

IMPLEMENT_SAVERESTORE( CSmallminer, CSquadMonster );

const char *CSmallminer::pSmallminerSentences[] =
{
	"MI_GREN", // grenade scared miner
	"MI_ALERT", // sees player
	"MI_MONSTER", // sees monster (sic: the group is MI_MONST)
	"MI_COVER", // running to cover
	"MI_THROW", // about to throw grenade
	"MI_CHARGE",  // running out to get the enemy
	"MI_TAUNT", // say rude things
	"MI_PUNCH", // never cued: the punch says it directly
};

enum
{
	SMALLMINER_SENT_NONE = -1,
	SMALLMINER_SENT_GREN = 0,
	SMALLMINER_SENT_ALERT,
	SMALLMINER_SENT_MONSTER,
	SMALLMINER_SENT_COVER,
	SMALLMINER_SENT_THROW,
	SMALLMINER_SENT_CHARGE,
	SMALLMINER_SENT_TAUNT,
	SMALLMINER_SENT_PUNCH,
} SMALLMINER_SENTENCE_TYPES;

//=========================================================
// Speak Sentence - say your cued up sentence.
//
// Some grunt sentences (take cover and charge) rely on actually
// being able to execute the intended action. It's really lame
// when a grunt says 'COVER ME' and then doesn't move. The problem
// is that the sentences were played when the decision to TRY
// to move to cover was made. Now the sentence is played after
// we know for sure that there is a valid path. The schedule
// may still fail but in most cases, well after the grunt has
// started moving.
//=========================================================
void CSmallminer :: SpeakSentence( void )
{
	if ( m_iSentence == SMALLMINER_SENT_NONE )
	{
		// no sentence cued up.
		return;
	}

	if (FOkToSpeak())
	{
		SENTENCEG_PlayRndSz( ENT(pev), pSmallminerSentences[ m_iSentence ], SMALLMINER_SENTENCE_VOLUME, SMALLMINER_ATTN, 0, m_voicePitch);
		JustSpoke();
	}
}

//=========================================================
// ISoundMask - Overidden for human grunts because they
// hear the DANGER sound that is made by hand grenades and
// other dangerous items.
//=========================================================
int CSmallminer :: ISoundMask ( void )
{
	return	bits_SOUND_WORLD	|
			bits_SOUND_COMBAT	|
			bits_SOUND_PLAYER	|
			bits_SOUND_DANGER;
}

//=========================================================
// someone else is talking - don't speak
//=========================================================
BOOL CSmallminer :: FOkToSpeak( void )
{
// if someone else is talking, don't speak
	if (gpGlobals->time <= CTalkMonster::g_talkWaitTime)
		return FALSE;

	if ( pev->spawnflags & SF_MONSTER_GAG )
	{
		if ( m_MonsterState != MONSTERSTATE_COMBAT )
		{
			// no talking outside of combat if gagged.
			return FALSE;
		}
	}

	// if player is not in pvs, don't speak
//	if (FNullEnt(FIND_CLIENT_IN_PVS(edict())))
//		return FALSE;

	return TRUE;
}

//=========================================================
//=========================================================
void CSmallminer :: JustSpoke( void )
{
	CTalkMonster::g_talkWaitTime = gpGlobals->time + RANDOM_FLOAT(1.5, 2.0);
	m_iSentence = SMALLMINER_SENT_NONE;
}

//=========================================================
// PrescheduleThink - this function runs after conditions
// are collected and before scheduling code is run.
//=========================================================
void CSmallminer :: PrescheduleThink ( void )
{
	if ( InSquad() && m_hEnemy != NULL )
	{
		if ( HasConditions ( bits_COND_SEE_ENEMY ) )
		{
			// update the squad's last enemy sighting time.
			MySquadLeader()->m_flLastEnemySightTime = gpGlobals->time;
		}
		else
		{
			if ( gpGlobals->time - MySquadLeader()->m_flLastEnemySightTime > 5 )
			{
				// been a while since we've seen the enemy
				MySquadLeader()->m_fEnemyEluded = TRUE;
			}
		}
	}
}

//=========================================================
// FCanCheckAttacks - this is overridden for human grunts
// because they can throw/shoot grenades when they can't see their
// target and the base class doesn't check attacks if the monster
// cannot see its enemy.
//=========================================================
BOOL CSmallminer :: FCanCheckAttacks ( void )
{
	if ( !HasConditions( bits_COND_ENEMY_TOOFAR ) )
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}


//=========================================================
// CheckMeleeAttack1
//=========================================================
BOOL CSmallminer :: CheckMeleeAttack1 ( float flDot, float flDist )
{
	CBaseMonster *pEnemy;

	if ( m_hEnemy != NULL )
	{
		pEnemy = m_hEnemy->MyMonsterPointer();

		if ( !pEnemy )
		{
			return FALSE;
		}
	}

	if ( flDist <= 64 && flDot >= 0.7	&&
		 pEnemy->Classify() != CLASS_ALIEN_BIOWEAPON &&
		 pEnemy->Classify() != CLASS_PLAYER_BIOWEAPON )
	{
		return TRUE;
	}
	return FALSE;
}

//=========================================================
// CheckRangeAttack2 - this checks the miner's dynamite
// attack.
//=========================================================
BOOL CSmallminer :: CheckRangeAttack2 ( float flDot, float flDist )
{
	// if the miner isn't moving, it's ok to check.
	if ( m_flGroundSpeed != 0 )
	{
		m_fThrowGrenade = FALSE;
		return m_fThrowGrenade;
	}

	// assume things haven't changed too much since last time
	if (gpGlobals->time < m_flNextGrenadeCheck )
	{
		return m_fThrowGrenade;
	}

	if ( !FBitSet ( m_hEnemy->pev->flags, FL_ONGROUND ) && m_hEnemy->pev->waterlevel == 0 && m_vecEnemyLKP.z > pev->absmax.z  )
	{
		//!!!BUGBUG - we should make this check movetype and make sure it isn't FLY? Players who jump a lot are unlikely to
		// be grenaded.
		// don't throw grenades at anything that isn't on the ground!
		m_fThrowGrenade = FALSE;
		return m_fThrowGrenade;
	}

	Vector vecTarget;

	// find feet
	if (RANDOM_LONG(0,1))
	{
		// magically know where they are
		vecTarget = Vector( m_hEnemy->pev->origin.x, m_hEnemy->pev->origin.y, m_hEnemy->pev->absmin.z );
	}
	else
	{
		// toss it to where you last saw them
		vecTarget = m_vecEnemyLKP;
	}

	// are any of my squad members near the intended grenade impact area?
	if ( InSquad() )
	{
		if (SquadMemberInRange( vecTarget, 256 ))
		{
			// crap, I might blow my own guy up. Don't throw a grenade and don't check again for a while.
			m_flNextGrenadeCheck = gpGlobals->time + 1; // one full second.
			m_fThrowGrenade = FALSE;
		}
	}

	if ( ( vecTarget - pev->origin ).Length2D() <= 256 )
	{
		// crap, I don't want to blow myself up
		m_flNextGrenadeCheck = gpGlobals->time + 1; // one full second.
		Forget( bits_MEMORY_INCOVER );
		m_fThrowGrenade = FALSE;
		return m_fThrowGrenade;
	}

	Vector vecToss = VecCheckToss( pev, GetGunPosition(), vecTarget, 0.5 );

	if ( vecToss != g_vecZero )
	{
		m_vecTossVelocity = vecToss;

		// throw a hand grenade
		m_fThrowGrenade = TRUE;
		// don't check again for a while.
		m_flNextGrenadeCheck = gpGlobals->time; // 1/3 second.
	}
	else
	{
		// don't throw
		m_fThrowGrenade = FALSE;
		// don't check again for a while.
		m_flNextGrenadeCheck = gpGlobals->time + 1; // one full second.
	}

	return m_fThrowGrenade;
}


//=========================================================
// TraceAttack - make sure we're not taking it in the helmet
//=========================================================
void CSmallminer :: TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType)
{
	// check for helmet shot
	if (ptr->iHitgroup == 11)
	{
		// make sure we're wearing one
		if (GetBodygroup( 1 ) == 0 && (bitsDamageType & (DMG_BULLET | DMG_SLASH | DMG_BLAST | DMG_CLUB)))
		{
			// absorb damage
			flDamage -= 20;
			if (flDamage <= 0)
			{
				UTIL_Ricochet( ptr->vecEndPos, 1.0 );
				flDamage = 0.01;
			}
		}
		// it's head shot anyways
		ptr->iHitgroup = HITGROUP_HEAD;
	}
	CSquadMonster::TraceAttack( pevAttacker, flDamage, vecDir, ptr, bitsDamageType );
}


//=========================================================
// TakeDamage - overridden for the miner because the miner
// needs to forget that he is in cover if he's hurt. (Obviously
// not in a safe place anymore).
//=========================================================
int CSmallminer :: TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	Forget( bits_MEMORY_INCOVER );

	return CSquadMonster :: TakeDamage ( pevInflictor, pevAttacker, flDamage, bitsDamageType );
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CSmallminer :: SetYawSpeed ( void )
{
	int ys;

	switch ( m_Activity )
	{
	case ACT_IDLE:
		ys = 150;
		break;
	case ACT_RUN:
		ys = 150;
		break;
	case ACT_WALK:
		ys = 180;
		break;
	case ACT_RANGE_ATTACK2:
		ys = 120;
		break;
	case ACT_MELEE_ATTACK1:
		ys = 120;
		break;
	case ACT_TURN_LEFT:
	case ACT_TURN_RIGHT:
		ys = 180;
		break;
	default:
		ys = 90;
		break;
	}

	pev->yaw_speed = ys;
}

void CSmallminer :: IdleSound( void )
{
	if (FOkToSpeak() && (g_fSmallminerQuestion || RANDOM_LONG(0,1)))
	{
		if (!g_fSmallminerQuestion)
		{
			// ask question or make statement
			switch (RANDOM_LONG(0,2))
			{
			case 0: // check in
				SENTENCEG_PlayRndSz(ENT(pev), "MI_CHECK", SMALLMINER_SENTENCE_VOLUME, ATTN_NORM, 0, m_voicePitch);
				g_fSmallminerQuestion = 1;
				break;
			case 1: // question
				SENTENCEG_PlayRndSz(ENT(pev), "MI_QUEST", SMALLMINER_SENTENCE_VOLUME, ATTN_NORM, 0, m_voicePitch);
				g_fSmallminerQuestion = 2;
				break;
			case 2: // statement
				SENTENCEG_PlayRndSz(ENT(pev), "MI_IDLE", SMALLMINER_SENTENCE_VOLUME, ATTN_NORM, 0, m_voicePitch);
				break;
			}
		}
		else
		{
			switch (g_fSmallminerQuestion)
			{
			case 1: // check in
				SENTENCEG_PlayRndSz(ENT(pev), "MI_CLEAR", SMALLMINER_SENTENCE_VOLUME, ATTN_NORM, 0, m_voicePitch);
				break;
			case 2: // question
				SENTENCEG_PlayRndSz(ENT(pev), "MI_ANSWER", SMALLMINER_SENTENCE_VOLUME, ATTN_NORM, 0, m_voicePitch);
				break;
			}
			g_fSmallminerQuestion = 0;
		}
		JustSpoke();
	}
}

//=========================================================
// Classify - indicates this monster's place in the
// relationship table.
//=========================================================
int	CSmallminer :: Classify ( void )
{
	return	CLASS_HUMAN_MILITARY;
}

//=========================================================
//=========================================================
CBaseEntity *CSmallminer :: Kick( void )
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
// GetGunPosition	return the end of the barrel
//=========================================================

Vector CSmallminer :: GetGunPosition( )
{
	return pev->origin + Vector( 0, 0, 60 );
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CSmallminer :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	Vector	vecShootDir;
	Vector	vecShootOrigin;

	switch( pEvent->event )
	{
		case SMALLMINER_AE_GREN_TOSS:
		{
			UTIL_MakeVectors( pev->angles );
			// CGrenade::ShootTimed( pev, pev->origin + gpGlobals->v_forward * 34 + Vector (0, 0, 32), m_vecTossVelocity, 3.5 );
			CGrenade::ShootTimed( pev, GetGunPosition(), m_vecTossVelocity, 3.5 );

			m_fThrowGrenade = FALSE;
			m_flNextGrenadeCheck = gpGlobals->time + 3;// wait three seconds before even looking again to see if a grenade can be thrown.
			// !!!LATER - when in a group, only try to throw grenade if ordered.
		}
		break;

		case SMALLMINER_AE_GREN_DROP:
		{
			UTIL_MakeVectors( pev->angles );
			CGrenade::ShootTimed( pev, pev->origin + gpGlobals->v_forward * 17 - gpGlobals->v_right * 27 + gpGlobals->v_up * 6, g_vecZero, 3 );
		}
		break;

		case SMALLMINER_AE_KICK:
		{
			CBaseEntity *pHurt = Kick();

			if ( pHurt )
			{
				if ( FOkToSpeak() )
				{
					SENTENCEG_PlayRndSz(ENT(pev), "MI_PUNCH", SMALLMINER_SENTENCE_VOLUME, SMALLMINER_ATTN, 0, m_voicePitch);
					JustSpoke();
				}

				UTIL_MakeVectors( pev->angles );
				pHurt->pev->punchangle.x = 15;
				pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_forward * 100 + gpGlobals->v_up * 50;
				pHurt->TakeDamage( pev, pev, gSkillData.smallminerDmgPick, DMG_CLUB );
			}
		}
		break;

		case SMALLMINER_AE_CAUGHT_ENEMY:
		{
			if ( FOkToSpeak() )
			{
				SENTENCEG_PlayRndSz(ENT(pev), "MI_ALERT", SMALLMINER_SENTENCE_VOLUME, SMALLMINER_ATTN, 0, m_voicePitch);
				 JustSpoke();
			}

		}

		default:
			CSquadMonster::HandleAnimEvent( pEvent );
			break;
	}
}

//=========================================================
// Spawn
//=========================================================
void CSmallminer :: Spawn()
{
	Precache( );

	SET_MODEL(ENT(pev), "models/smallminer.mdl");
	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= BLOOD_COLOR_RED;
	pev->effects		= 0;
	pev->health			= gSkillData.smallminerHealth;
	m_flFieldOfView		= 0.2;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState		= MONSTERSTATE_NONE;
	m_flNextGrenadeCheck = gpGlobals->time + 1;
	m_flNextPainTime	= gpGlobals->time;
	m_iSentence			= SMALLMINER_SENT_NONE;

	m_afCapability		= bits_CAP_SQUAD | bits_CAP_TURN_HEAD | bits_CAP_DOORS_GROUP;

	m_fEnemyEluded		= FALSE;
	m_fFirstEncounter	= TRUE;// this is true when the miner spawns, because he hasn't encountered an enemy yet.

	m_HackedGunPos = Vector ( 0, 0, 55 );

	// a random head unless the map picked one
	SetBodygroup( HEAD_GROUP, pev->body == -1 ? RANDOM_LONG( 0, 3 ) : pev->body );

	CTalkMonster::g_talkWaitTime = 0;

	MonsterInit();
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CSmallminer :: Precache()
{
	PRECACHE_MODEL("models/smallminer.mdl");

	PRECACHE_SOUND( "miner/death1.wav" );
	PRECACHE_SOUND( "miner/death2.wav" );
	PRECACHE_SOUND( "miner/death6.wav" );

	PRECACHE_SOUND( "miner/pain1.wav" );
	PRECACHE_SOUND( "miner/pain2.wav" );
	PRECACHE_SOUND( "miner/pain3.wav" );

	PRECACHE_SOUND("zombie/claw_miss2.wav");// because we use the basemonster SWIPE animation event

	m_voicePitch = 100;
}

//=========================================================
// start task
//=========================================================
void CSmallminer :: StartTask ( Task_t *pTask )
{
	m_iTaskStatus = TASKSTATUS_RUNNING;

	switch ( pTask->iTask )
	{
	case TASK_SMALLMINER_CHECK_FIRE:
		if ( !NoFriendlyFire() )
		{
			SetConditions( bits_COND_SMALLMINER_NOFIRE );
		}
		TaskComplete();
		break;

	case TASK_SMALLMINER_SPEAK_SENTENCE:
		SpeakSentence();
		TaskComplete();
		break;

	case TASK_WALK_PATH:
	case TASK_RUN_PATH:
		// miner no longer assumes he is covered if he moves
		Forget( bits_MEMORY_INCOVER );
		CSquadMonster ::StartTask( pTask );
		break;

	case TASK_SMALLMINER_FACE_TOSS_DIR:
		break;

	case TASK_FACE_IDEAL:
	case TASK_FACE_ENEMY:
		CSquadMonster :: StartTask( pTask );
		if (pev->movetype == MOVETYPE_FLY)
		{
			m_IdealActivity = ACT_GLIDE;
		}
		break;

	default:
		CSquadMonster :: StartTask( pTask );
		break;
	}
}

//=========================================================
// RunTask
//=========================================================
void CSmallminer :: RunTask ( Task_t *pTask )
{
	switch ( pTask->iTask )
	{
	case TASK_SMALLMINER_FACE_TOSS_DIR:
		{
			// project a point along the toss vector and turn to face that point.
			MakeIdealYaw( pev->origin + m_vecTossVelocity * 64 );
			ChangeYaw( pev->yaw_speed );

			if ( FacingIdeal() )
			{
				m_iTaskStatus = TASKSTATUS_COMPLETE;
			}
			break;
		}
	default:
		{
			CSquadMonster :: RunTask( pTask );
			break;
		}
	}
}

//=========================================================
// PainSound
//=========================================================
void CSmallminer :: PainSound ( void )
{
	if ( gpGlobals->time > m_flNextPainTime )
	{
		switch ( RANDOM_LONG(0,3) )
		{
		case 0:
			EMIT_SOUND( ENT(pev), CHAN_VOICE, "miner/pain3.wav", 1, ATTN_NORM );
			break;
		case 1:
			EMIT_SOUND( ENT(pev), CHAN_VOICE, "miner/pain1.wav", 1, ATTN_NORM );
			break;
		case 2:
			EMIT_SOUND( ENT(pev), CHAN_VOICE, "miner/pain2.wav", 1, ATTN_NORM );
			break;
		}

		m_flNextPainTime = gpGlobals->time + 1;
	}
}

//=========================================================
// DeathSound
//=========================================================
void CSmallminer :: DeathSound ( void )
{
	switch ( RANDOM_LONG(0,2) )
	{
	case 0:
		EMIT_SOUND( ENT(pev), CHAN_VOICE, "miner/death1.wav", 1, ATTN_IDLE );
		break;
	case 1:
		EMIT_SOUND( ENT(pev), CHAN_VOICE, "miner/death2.wav", 1, ATTN_IDLE );
		break;
	case 2:
		EMIT_SOUND( ENT(pev), CHAN_VOICE, "miner/death6.wav", 1, ATTN_IDLE );
		break;
	}
}

//=========================================================
// AI Schedules Specific to this monster
//=========================================================

//=========================================================
// SmallminerFail
//=========================================================
Task_t	tlSmallminerFail[] =
{
	{ TASK_STOP_MOVING,			0				},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE },
	{ TASK_WAIT,				(float)0.2		},
	{ TASK_WAIT_PVS,			(float)0		},
};

Schedule_t	slSmallminerFail[] =
{
	{
		tlSmallminerFail,
		ARRAYSIZE ( tlSmallminerFail ),
		bits_COND_CAN_RANGE_ATTACK2 |
		bits_COND_CAN_MELEE_ATTACK1,
		0,
		"Small Miner Fail"
	},
};

//=========================================================
// Smallminer Combat Fail
//=========================================================
Task_t	tlSmallminerCombatFail[] =
{
	{ TASK_STOP_MOVING,			0				},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE },
	{ TASK_WAIT_FACE_ENEMY,		(float)0		},
	{ TASK_WAIT_PVS,			(float)0		},
};

Schedule_t	slSmallminerCombatFail[] =
{
	{
		tlSmallminerCombatFail,
		ARRAYSIZE ( tlSmallminerCombatFail ),
		bits_COND_CAN_RANGE_ATTACK2	|
		bits_COND_CAN_MELEE_ATTACK1,
		0,
		"Small Miner Combat Fail"
	},
};

//=========================================================
// Victory dance!
//=========================================================
Task_t	tlSmallminerVictoryDance[] =
{
	{ TASK_STOP_MOVING,						(float)0					},
	{ TASK_FACE_ENEMY,						(float)0					},
	{ TASK_WAIT,							(float)1.5					},
	{ TASK_GET_PATH_TO_ENEMY_CORPSE,		(float)0					},
	{ TASK_WALK_PATH,						(float)0					},
	{ TASK_WAIT_FOR_MOVEMENT,				(float)0					},
	{ TASK_FACE_ENEMY,						(float)0					},
	{ TASK_PLAY_SEQUENCE,					(float)ACT_VICTORY_DANCE	},
};

Schedule_t	slSmallminerVictoryDance[] =
{
	{
		tlSmallminerVictoryDance,
		ARRAYSIZE ( tlSmallminerVictoryDance ),
		bits_COND_NEW_ENEMY		|
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE,
		0,
		"SmallminerVictoryDance"
	},
};

//=========================================================
// Establish line of fire - move to a position that allows
// the miner to attack.
//=========================================================
Task_t tlSmallminerEstablishLineOfFire[] =
{
	{ TASK_SET_FAIL_SCHEDULE,	(float)SCHED_SMALLMINER_ELOF_FAIL	},
	{ TASK_GET_PATH_TO_ENEMY,	(float)0						},
	{ TASK_SMALLMINER_SPEAK_SENTENCE,(float)0						},
	{ TASK_RUN_PATH,			(float)0						},
	{ TASK_WAIT_FOR_MOVEMENT,	(float)0						},
};

Schedule_t slSmallminerEstablishLineOfFire[] =
{
	{
		tlSmallminerEstablishLineOfFire,
		ARRAYSIZE ( tlSmallminerEstablishLineOfFire ),
		bits_COND_NEW_ENEMY			|
		bits_COND_ENEMY_DEAD		|
		bits_COND_CAN_MELEE_ATTACK1	|
		bits_COND_CAN_RANGE_ATTACK2	|
		bits_COND_HEAR_SOUND,

		bits_SOUND_DANGER,
		"SmallminerEstablishLineOfFire"
	},
};

//=========================================================
// SmallminerFoundEnemy - miner established sight with an enemy
// that was hiding from the squad.
//=========================================================
Task_t	tlSmallminerFoundEnemy[] =
{
	{ TASK_STOP_MOVING,				0							},
	{ TASK_FACE_ENEMY,				(float)0					},
	{ TASK_PLAY_SEQUENCE_FACE_ENEMY,(float)ACT_SIGNAL1			},
};

Schedule_t	slSmallminerFoundEnemy[] =
{
	{
		tlSmallminerFoundEnemy,
		ARRAYSIZE ( tlSmallminerFoundEnemy ),
		bits_COND_HEAR_SOUND,

		bits_SOUND_DANGER,
		"SmallminerFoundEnemy"
	},
};

//=========================================================
// SmallminerCombatFace Schedule
//=========================================================
Task_t	tlSmallminerCombatFace1[] =
{
	{ TASK_STOP_MOVING,				0							},
	{ TASK_SET_ACTIVITY,			(float)ACT_IDLE				},
	{ TASK_FACE_ENEMY,				(float)0					},
	{ TASK_WAIT,					(float)0.2					},
	{ TASK_SET_SCHEDULE,			(float)SCHED_SMALLMINER_SWEEP	},
};

Schedule_t	slSmallminerCombatFace[] =
{
	{
		tlSmallminerCombatFace1,
		ARRAYSIZE ( tlSmallminerCombatFace1 ),
		bits_COND_NEW_ENEMY				|
		bits_COND_ENEMY_DEAD			|
		bits_COND_CAN_MELEE_ATTACK1		|
		bits_COND_CAN_RANGE_ATTACK2,
		0,
		"Combat Face"
	},
};

//=========================================================
// miner wait in cover - we don't allow danger or the ability
// to attack to break a miner's run to cover schedule, but
// when a miner is in cover, we do want them to attack if they can.
//=========================================================
Task_t	tlSmallminerWaitInCover[] =
{
	{ TASK_STOP_MOVING,				(float)0					},
	{ TASK_SET_ACTIVITY,			(float)ACT_IDLE				},
	{ TASK_WAIT_FACE_ENEMY,			(float)0					},
};

Schedule_t	slSmallminerWaitInCover[] =
{
	{
		tlSmallminerWaitInCover,
		ARRAYSIZE ( tlSmallminerWaitInCover ),
		bits_COND_NEW_ENEMY			|
		bits_COND_HEAR_SOUND		|
		bits_COND_CAN_RANGE_ATTACK2	|
		bits_COND_CAN_MELEE_ATTACK1,

		bits_SOUND_DANGER,
		"SmallminerWaitInCover"
	},
};

//=========================================================
// run to cover.
// !!!BUGBUG - set a decent fail schedule here.
//=========================================================
Task_t	tlSmallminerTakeCover1[] =
{
	{ TASK_STOP_MOVING,				(float)0							},
	{ TASK_SET_FAIL_SCHEDULE,		(float)SCHED_SMALLMINER_TAKECOVER_FAILED	},
	{ TASK_WAIT,					(float)0.1							},
	{ TASK_FIND_COVER_FROM_ENEMY,	(float)0							},
	{ TASK_SMALLMINER_SPEAK_SENTENCE,	(float)0							},
	{ TASK_RUN_PATH,				(float)0							},
	{ TASK_WAIT_FOR_MOVEMENT,		(float)0							},
	{ TASK_REMEMBER,				(float)bits_MEMORY_INCOVER			},
	{ TASK_SET_SCHEDULE,			(float)SCHED_SMALLMINER_WAIT_FACE_ENEMY	},
};

Schedule_t	slSmallminerTakeCover[] =
{
	{
		tlSmallminerTakeCover1,
		ARRAYSIZE ( tlSmallminerTakeCover1 ),
		0,
		0,
		"TakeCover"
	},
};

//=========================================================
// drop grenade then run to cover.
//=========================================================
Task_t	tlSmallminerGrenadeCover1[] =
{
	{ TASK_STOP_MOVING,						(float)0							},
	{ TASK_FIND_COVER_FROM_ENEMY,			(float)99							},
	{ TASK_FIND_FAR_NODE_COVER_FROM_ENEMY,	(float)384							},
	{ TASK_PLAY_SEQUENCE,					(float)ACT_SPECIAL_ATTACK1			},
	{ TASK_CLEAR_MOVE_WAIT,					(float)0							},
	{ TASK_RUN_PATH,						(float)0							},
	{ TASK_WAIT_FOR_MOVEMENT,				(float)0							},
	{ TASK_SET_SCHEDULE,					(float)SCHED_SMALLMINER_WAIT_FACE_ENEMY	},
};

Schedule_t	slSmallminerGrenadeCover[] =
{
	{
		tlSmallminerGrenadeCover1,
		ARRAYSIZE ( tlSmallminerGrenadeCover1 ),
		0,
		0,
		"GrenadeCover"
	},
};


//=========================================================
// drop grenade then run to cover.
//=========================================================
Task_t	tlSmallminerTossGrenadeCover1[] =
{
	{ TASK_FACE_ENEMY,						(float)0							},
	{ TASK_RANGE_ATTACK2, 					(float)0							},
	{ TASK_SET_SCHEDULE,					(float)SCHED_TAKE_COVER_FROM_ENEMY	},
};

Schedule_t	slSmallminerTossGrenadeCover[] =
{
	{
		tlSmallminerTossGrenadeCover1,
		ARRAYSIZE ( tlSmallminerTossGrenadeCover1 ),
		0,
		0,
		"TossGrenadeCover"
	},
};

//=========================================================
// hide from the loudest sound source (to run from grenade)
//=========================================================
Task_t	tlSmallminerTakeCoverFromBestSound[] =
{
	{ TASK_SET_FAIL_SCHEDULE,			(float)SCHED_COWER			},// duck and cover if cannot move from explosion
	{ TASK_STOP_MOVING,					(float)0					},
	{ TASK_FIND_COVER_FROM_BEST_SOUND,	(float)0					},
	{ TASK_RUN_PATH,					(float)0					},
	{ TASK_WAIT_FOR_MOVEMENT,			(float)0					},
	{ TASK_REMEMBER,					(float)bits_MEMORY_INCOVER	},
	{ TASK_TURN_LEFT,					(float)179					},
};

Schedule_t	slSmallminerTakeCoverFromBestSound[] =
{
	{
		tlSmallminerTakeCoverFromBestSound,
		ARRAYSIZE ( tlSmallminerTakeCoverFromBestSound ),
		0,
		0,
		"SmallminerTakeCoverFromBestSound"
	},
};

//=========================================================
// Do a turning sweep of the area
//=========================================================
Task_t	tlSmallminerSweep[] =
{
	{ TASK_TURN_LEFT,			(float)179	},
	{ TASK_WAIT,				(float)0.2	},
	{ TASK_TURN_LEFT,			(float)179	},
	{ TASK_WAIT,				(float)0.2	},
};

Schedule_t	slSmallminerSweep[] =
{
	{
		tlSmallminerSweep,
		ARRAYSIZE ( tlSmallminerSweep ),

		bits_COND_NEW_ENEMY		|
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE	|
		bits_COND_CAN_MELEE_ATTACK1	|
		bits_COND_CAN_RANGE_ATTACK2	|
		bits_COND_HEAR_SOUND,

		bits_SOUND_WORLD		|// sound flags
		bits_SOUND_DANGER		|
		bits_SOUND_PLAYER,

		"Smallminer Sweep"
	},
};

//=========================================================
// secondary range attack. Overriden because base class stops attacking when the enemy is occluded.
// miner's grenade toss requires the enemy be occluded.
//=========================================================
Task_t	tlSmallminerRangeAttack2[] =
{
	{ TASK_STOP_MOVING,				(float)0					},
	{ TASK_SMALLMINER_FACE_TOSS_DIR,	(float)0					},
	{ TASK_PLAY_SEQUENCE,			(float)ACT_RANGE_ATTACK2	},
	{ TASK_SET_SCHEDULE,			(float)SCHED_SMALLMINER_WAIT_FACE_ENEMY	},// don't run immediately after throwing grenade.
};

Schedule_t	slSmallminerRangeAttack2[] =
{
	{
		tlSmallminerRangeAttack2,
		ARRAYSIZE ( tlSmallminerRangeAttack2 ),
		0,
		0,
		"RangeAttack2"
	},
};


DEFINE_CUSTOM_SCHEDULES( CSmallminer )
{
	slSmallminerFail,
	slSmallminerCombatFail,
	slSmallminerVictoryDance,
	slSmallminerEstablishLineOfFire,
	slSmallminerFoundEnemy,
	slSmallminerCombatFace,
	slSmallminerWaitInCover,
	slSmallminerTakeCover,
	slSmallminerGrenadeCover,
	slSmallminerTossGrenadeCover,
	slSmallminerTakeCoverFromBestSound,
	slSmallminerSweep,
	slSmallminerRangeAttack2,
};

IMPLEMENT_CUSTOM_SCHEDULES( CSmallminer, CSquadMonster );

//=========================================================
// SetActivity
//=========================================================
void CSmallminer :: SetActivity ( Activity NewActivity )
{
	int	iSequence = ACTIVITY_NOT_AVAILABLE;
	void *pmodel = GET_MODEL_PTR( ENT(pev) );

	switch ( NewActivity)
	{
	case ACT_RANGE_ATTACK2:
		// get toss anim
		iSequence = LookupSequence( "throwgrenade" );
		break;
	case ACT_RUN:
		if ( pev->health <= SMALLMINER_LIMP_HEALTH )
		{
			// limp!
			iSequence = LookupActivity ( ACT_RUN_HURT );
		}
		else
		{
			iSequence = LookupActivity ( NewActivity );
		}
		break;
	case ACT_WALK:
		if ( pev->health <= SMALLMINER_LIMP_HEALTH )
		{
			// limp!
			iSequence = LookupActivity ( ACT_WALK_HURT );
		}
		else
		{
			iSequence = LookupActivity ( NewActivity );
		}
		break;
	case ACT_IDLE:
		if ( m_MonsterState == MONSTERSTATE_COMBAT )
		{
			NewActivity = ACT_IDLE_ANGRY;
		}
		iSequence = LookupActivity ( NewActivity );
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
		// Not available try to get default anim
//		ALERT ( at_console, "%s has no sequence for act:%d\n", STRING(pev->classname), NewActivity );
		pev->sequence		= 0;	// Set to the reset anim (if it's there)
	}
}

//=========================================================
// Get Schedule!
//=========================================================
Schedule_t *CSmallminer :: GetSchedule( void )
{

	// clear old sentence
	m_iSentence = SMALLMINER_SENT_NONE;

	// miners place HIGH priority on running away from danger sounds.
	if ( HasConditions(bits_COND_HEAR_SOUND) )
	{
		CSound *pSound;
		pSound = PBestSound();

		ASSERT( pSound != NULL );
		if ( pSound)
		{
			if (pSound->m_iType & bits_SOUND_DANGER)
			{
				// dangerous sound nearby!
				if (FOkToSpeak())
				{
					SENTENCEG_PlayRndSz( ENT(pev), "MI_GREN", SMALLMINER_SENTENCE_VOLUME, SMALLMINER_ATTN, 0, m_voicePitch);
					JustSpoke();
				}
				return GetScheduleOfType( SCHED_TAKE_COVER_FROM_BEST_SOUND );
			}
		}
	}
	switch	( m_MonsterState )
	{
	case MONSTERSTATE_COMBAT:
		{
// dead enemy
			if ( HasConditions( bits_COND_ENEMY_DEAD ) )
			{
				// call base class, all code to handle dead enemies is centralized there.
				return CBaseMonster :: GetSchedule();
			}

// new enemy
			if ( HasConditions(bits_COND_NEW_ENEMY) )
			{
				if ( InSquad() )
				{
					MySquadLeader()->m_fEnemyEluded = FALSE;

					if ( !IsLeader() )
					{
						return GetScheduleOfType ( SCHED_TAKE_COVER_FROM_ENEMY );
					}
					else
					{
						if (FOkToSpeak())// && RANDOM_LONG(0,1))
						{
							if ((m_hEnemy != NULL) && m_hEnemy->IsPlayer())
								// player
								SENTENCEG_PlayRndSz( ENT(pev), "MI_ALERT", SMALLMINER_SENTENCE_VOLUME, SMALLMINER_ATTN, 0, m_voicePitch);
							else if ((m_hEnemy != NULL) &&
									(m_hEnemy->Classify() != CLASS_PLAYER_ALLY) &&
									(m_hEnemy->Classify() != CLASS_HUMAN_PASSIVE) &&
									(m_hEnemy->Classify() != CLASS_MACHINE))
								// monster
								SENTENCEG_PlayRndSz( ENT(pev), "MI_MONST", SMALLMINER_SENTENCE_VOLUME, SMALLMINER_ATTN, 0, m_voicePitch);

							JustSpoke();
						}

						return GetScheduleOfType ( SCHED_SMALLMINER_ESTABLISH_LINE_OF_FIRE );
					}
				}
			}
// damaged just a little
			else if ( HasConditions( bits_COND_LIGHT_DAMAGE ) )
			{
				// if hurt:
				// 90% chance of taking cover
				// 10% chance of flinch.
				int iPercent = RANDOM_LONG(0,99);

				if ( iPercent <= 90 && m_hEnemy != NULL )
				{
					// only try to take cover if we actually have an enemy!
					if (FOkToSpeak()) // && RANDOM_LONG(0,1))
					{
						//SENTENCEG_PlayRndSz( ENT(pev), "MI_COVER", SMALLMINER_SENTENCE_VOLUME, SMALLMINER_ATTN, 0, m_voicePitch);
						m_iSentence = SMALLMINER_SENT_COVER;
						//JustSpoke();
					}
					return GetScheduleOfType( SCHED_TAKE_COVER_FROM_ENEMY );
				}
				else
				{
					return GetScheduleOfType( SCHED_SMALL_FLINCH );
				}
			}
// can kick
			else if ( HasConditions ( bits_COND_CAN_MELEE_ATTACK1 ) )
			{
				return GetScheduleOfType ( SCHED_MELEE_ATTACK1 );
			}
// can throw
			else if ( HasConditions( bits_COND_CAN_RANGE_ATTACK2 ) )
			{
				if ( HasConditions( bits_COND_ENEMY_OCCLUDED ) )
				{
					if (FOkToSpeak())
					{
						SENTENCEG_PlayRndSz( ENT(pev), "MI_THROW", SMALLMINER_SENTENCE_VOLUME, SMALLMINER_ATTN, 0, m_voicePitch);
						JustSpoke();
					}
					return GetScheduleOfType( SCHED_RANGE_ATTACK2 );
				}
				else if ( OccupySlot( bits_SLOTS_HGRUNT_ENGAGE ) )
				{
					if (FOkToSpeak())// && RANDOM_LONG(0,1))
					{
						//SENTENCEG_PlayRndSz( ENT(pev), "MI_CHARGE", SMALLMINER_SENTENCE_VOLUME, SMALLMINER_ATTN, 0, m_voicePitch);
						m_iSentence = SMALLMINER_SENT_CHARGE;
						//JustSpoke();
					}

					return GetScheduleOfType( SCHED_SMALLMINER_ESTABLISH_LINE_OF_FIRE );
				}
				else
				{
					if (FOkToSpeak() && RANDOM_LONG(0,1))
					{
						SENTENCEG_PlayRndSz( ENT(pev), "MI_TAUNT", SMALLMINER_SENTENCE_VOLUME, SMALLMINER_ATTN, 0, m_voicePitch);
						JustSpoke();
					}
					return GetScheduleOfType( SCHED_STANDOFF );
				}
			}

			if ( HasConditions( bits_COND_SEE_ENEMY ) )
			{
				return GetScheduleOfType ( SCHED_SMALLMINER_ESTABLISH_LINE_OF_FIRE );
			}
		}
	}

	// no special cases here, call the base class
	return CSquadMonster :: GetSchedule();
}

//=========================================================
//=========================================================
Schedule_t* CSmallminer :: GetScheduleOfType ( int Type )
{
	switch	( Type )
	{
	case SCHED_TAKE_COVER_FROM_ENEMY:
		{
			if ( InSquad() )
			{
				if ( g_iSkillLevel == SKILL_HARD && HasConditions( bits_COND_CAN_RANGE_ATTACK2 ) )
				{
					if (FOkToSpeak())
					{
						SENTENCEG_PlayRndSz( ENT(pev), "MI_THROW", SMALLMINER_SENTENCE_VOLUME, SMALLMINER_ATTN, 0, m_voicePitch);
						JustSpoke();
					}
					return slSmallminerTossGrenadeCover;
				}
				else
				{
					return &slSmallminerTakeCover[ 0 ];
				}
			}
			else
			{
				if ( RANDOM_LONG(0,1) )
				{
					return &slSmallminerTakeCover[ 0 ];
				}
				else
				{
					return &slSmallminerGrenadeCover[ 0 ];
				}
			}
		}
	case SCHED_TAKE_COVER_FROM_BEST_SOUND:
		{
			return &slSmallminerTakeCoverFromBestSound[ 0 ];
		}
	case SCHED_SMALLMINER_TAKECOVER_FAILED:
		{
			return GetScheduleOfType ( SCHED_FAIL );
		}
		break;
	case SCHED_SMALLMINER_ELOF_FAIL:
		{
			// miner is unable to move to a position that allows him to attack the enemy.
			return GetScheduleOfType ( SCHED_TAKE_COVER_FROM_ENEMY );
		}
		break;
	case SCHED_SMALLMINER_ESTABLISH_LINE_OF_FIRE:
		{
			return &slSmallminerEstablishLineOfFire[ 0 ];
		}
		break;
	case SCHED_RANGE_ATTACK2:
		{
			return &slSmallminerRangeAttack2[ 0 ];
		}
	case SCHED_COMBAT_FACE:
		{
			return &slSmallminerCombatFace[ 0 ];
		}
	case SCHED_SMALLMINER_WAIT_FACE_ENEMY:
		{
			return &slSmallminerWaitInCover[ 0 ];
		}
	case SCHED_SMALLMINER_SWEEP:
		{
			return &slSmallminerSweep[ 0 ];
		}
	case SCHED_SMALLMINER_FOUND_ENEMY:
		{
			return &slSmallminerFoundEnemy[ 0 ];
		}
	case SCHED_VICTORY_DANCE:
		{
			if ( InSquad() )
			{
				if ( !IsLeader() )
				{
					return &slSmallminerFail[ 0 ];
				}
			}

			return &slSmallminerVictoryDance[ 0 ];
		}
	case SCHED_FAIL:
		{
			if ( m_hEnemy != NULL )
			{
				// miner has an enemy, so pick a different default fail schedule most likely to help recover.
				return &slSmallminerCombatFail[ 0 ];
			}

			return &slSmallminerFail[ 0 ];
		}
	default:
		{
			return CSquadMonster :: GetScheduleOfType ( Type );
		}
	}
}


//=========================================================
// DEAD SMALLMINER PROP
//=========================================================
class CDeadSmallminer : public CBaseMonster
{
public:
	void Spawn( void );
	int	Classify ( void ) { return	CLASS_HUMAN_MILITARY; }

	void KeyValue( KeyValueData *pkvd );

	int	m_iPose;// which sequence to display	-- temporary, don't need to save
	static char *m_szPoses[3];
};

char *CDeadSmallminer::m_szPoses[] = { "deadstomach", "deadside", "deadsitting" };

void CDeadSmallminer::KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "pose"))
	{
		m_iPose = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CBaseMonster::KeyValue( pkvd );
}

LINK_ENTITY_TO_CLASS( monster_smallminer_dead, CDeadSmallminer );

//=========================================================
// ********** DeadSmallminer SPAWN **********
//=========================================================
void CDeadSmallminer :: Spawn( void )
{
	PRECACHE_MODEL("models/smallminer.mdl");
	SET_MODEL(ENT(pev), "models/smallminer.mdl");

	pev->effects		= 0;
	pev->yaw_speed		= 8;
	pev->sequence		= 0;
	m_bloodColor		= BLOOD_COLOR_RED;

	pev->sequence = LookupSequence( m_szPoses[m_iPose] );

	if (pev->sequence == -1)
	{
		ALERT ( at_console, "Dead smallminer with bad pose\n" );
	}

	// Corpses have less health
	pev->health			= 8;

	// map old bodies onto new bodies
	switch( pev->body )
	{
	case 0:
		pev->body = 0;
		pev->skin = 0;
		SetBodygroup( HEAD_GROUP, 0 );
		break;
	case 1:
		pev->body = 0;
		pev->skin = 0;
		SetBodygroup( HEAD_GROUP, 1 );
		break;
	case 2:
		pev->body = 0;
		pev->skin = 0;
		SetBodygroup( HEAD_GROUP, 2 );
		break;
	case 3:
		pev->body = 0;
		pev->skin = 0;
		SetBodygroup( HEAD_GROUP, 3 );
		break;
	}

	MonsterInitDead();
}

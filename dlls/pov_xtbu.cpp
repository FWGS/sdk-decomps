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
// POV: monster_human_gr_xtbu, the human grunt carrying an
// XTBS. A copy of POV's hgrunt.cpp without guns, grenades
// and repel. File name and the names retail doesn't give
// are ours.
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

int g_fXTBUGruntQuestion;				// true if an idle grunt asked a question. Cleared when someone answers.

extern DLL_GLOBAL int		g_iSkillLevel;

//=========================================================
// monster-specific DEFINE's
//=========================================================
#define XTBUGRUNT_VOL						0.35		// volume of grunt sounds
#define XTBUGRUNT_ATTN						ATTN_NORM	// attenutation of grunt sentences
#define XTBUGRUNT_SENTENCE_VOLUME			(float)0.35 // volume of grunt sentences

#define SF_XTBUGRUNT_INVULNERABLE		8	// POV, name ours: 80000 health
#define SF_XTBUGRUNT_SHOWXTBU			32	// POV, name ours: starts with the XTBS shown

#define HEAD_GROUP					1
#define HEAD_GRUNT					0
#define GUN_GROUP					2
#define GUN_MP5						0
#define XTBU_GROUP					3	// POV, names ours
#define XTBU_NONE					0
#define XTBU_SHOWN					1
#define XTBU_TAKEN					2

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define		XTBUGRUNT_AE_KICK			( 3 )
#define		XTBUGRUNT_AE_CAUGHT_ENEMY	( 10) // grunt established sight with an enemy (player only) that had previously eluded the squad.
#define		XTBUGRUNT_AE_DROP_GUN		( 11) // POV: drops the XTBS
#define		XTBUGRUNT_AE_TAKE_PROP		( 14) // POV, name ours: removes the XTBS prop entity and takes it
#define		XTBUGRUNT_AE_SHOW_XTBU		( 15) // POV, name ours

//=========================================================
// monster-specific schedule types
//=========================================================
enum
{
	SCHED_XTBUGRUNT_SUPPRESS = LAST_COMMON_SCHEDULE + 1,
	SCHED_XTBUGRUNT_ESTABLISH_LINE_OF_FIRE,// move to a location to set up an attack against the enemy. (usually when a friendly is in the way).
	SCHED_XTBUGRUNT_SWEEP,
	SCHED_XTBUGRUNT_FOUND_ENEMY,
	SCHED_XTBUGRUNT_WAIT_FACE_ENEMY,
	SCHED_XTBUGRUNT_TAKECOVER_FAILED,// special schedule type that forces analysis of conditions and picks the best possible schedule to recover from this type of failure.
	SCHED_XTBUGRUNT_ELOF_FAIL,
};

//=========================================================
// monster-specific tasks
//=========================================================
enum
{
	TASK_XTBUGRUNT_FACE_TOSS_DIR = LAST_COMMON_TASK + 1,
	TASK_XTBUGRUNT_SPEAK_SENTENCE,
	TASK_XTBUGRUNT_CHECK_FIRE,
};

//=========================================================
// monster-specific conditions
//=========================================================
#define bits_COND_XTBUGRUNT_NOFIRE	( bits_COND_SPECIAL1 )

class CXTBUGrunt : public CSquadMonster
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
	BOOL CheckRangeAttack1 ( float flDot, float flDist );
	BOOL CheckRangeAttack2 ( float flDot, float flDist );
	void CheckAmmo ( void );
	void SetActivity ( Activity NewActivity );
	void StartTask ( Task_t *pTask );
	void RunTask ( Task_t *pTask );
	void DeathSound( void );
	void PainSound( void );
	void IdleSound ( void );
	Vector GetGunPosition( void );
	void PrescheduleThink ( void );
	void GibMonster( void );
	void SpeakSentence( void );
	void KeyValue( KeyValueData *pkvd );

	int	Save( CSave &save );
	int Restore( CRestore &restore );

	CBaseEntity	*Kick( void );
	Schedule_t	*GetSchedule( void );
	Schedule_t  *GetScheduleOfType ( int Type );
	void TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType);
	int TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );

	int IRelationship ( CBaseEntity *pTarget );

	BOOL FOkToSpeak( void );
	void JustSpoke( void );

	CUSTOM_SCHEDULES;
	static TYPEDESCRIPTION m_SaveData[];

	float m_flNextPainTime;
	float m_flLastEnemySightTime;

	BOOL	m_fFirstEncounter;// only put on the handsign show in the squad's first encounter.

	int m_voicePitch;

	int		m_iSentence;

	string_t m_iszXTBUPropEnt;	// "xtbupropent": the XTBS prop entity to take

	static const char *pGruntSentences[];
};

LINK_ENTITY_TO_CLASS( monster_human_gr_xtbu, CXTBUGrunt );

TYPEDESCRIPTION	CXTBUGrunt::m_SaveData[] =
{
	DEFINE_FIELD( CXTBUGrunt, m_flNextPainTime, FIELD_TIME ),
//	DEFINE_FIELD( CXTBUGrunt, m_flLastEnemySightTime, FIELD_TIME ), // don't save, go to zero
	DEFINE_FIELD( CXTBUGrunt, m_fFirstEncounter, FIELD_BOOLEAN ),
	DEFINE_FIELD( CXTBUGrunt, m_voicePitch, FIELD_INTEGER ),
	DEFINE_FIELD( CXTBUGrunt, m_iSentence, FIELD_INTEGER ),
	DEFINE_FIELD( CXTBUGrunt, m_iszXTBUPropEnt, FIELD_STRING ),
};

IMPLEMENT_SAVERESTORE( CXTBUGrunt, CSquadMonster );

const char *CXTBUGrunt::pGruntSentences[] =
{
	"HG_GREN", // grenade scared grunt
	"HG_ALERT", // sees player
	"HG_MONSTER", // sees monster
	"HG_COVER", // running to cover
	"HG_THROW", // about to throw grenade
	"HG_CHARGE",  // running out to get the enemy
	"HG_TAUNT", // say rude things
};

enum
{
	XTBUGRUNT_SENT_NONE = -1,
	XTBUGRUNT_SENT_GREN = 0,
	XTBUGRUNT_SENT_ALERT,
	XTBUGRUNT_SENT_MONSTER,
	XTBUGRUNT_SENT_COVER,
	XTBUGRUNT_SENT_THROW,
	XTBUGRUNT_SENT_CHARGE,
	XTBUGRUNT_SENT_TAUNT,
} XTBUGRUNT_SENTENCE_TYPES;

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
void CXTBUGrunt :: SpeakSentence( void )
{
	if ( m_iSentence == XTBUGRUNT_SENT_NONE )
	{
		// no sentence cued up.
		return;
	}

	if (FOkToSpeak())
	{
		SENTENCEG_PlayRndSz( ENT(pev), pGruntSentences[ m_iSentence ], XTBUGRUNT_SENTENCE_VOLUME, XTBUGRUNT_ATTN, 0, m_voicePitch);
		JustSpoke();
	}
}

//=========================================================
// IRelationship - overridden because Alien Grunts are
// Human Grunt's nemesis.
//=========================================================
int CXTBUGrunt::IRelationship ( CBaseEntity *pTarget )
{
	if ( FClassnameIs( pTarget->pev, "monster_alien_grunt" ) || ( FClassnameIs( pTarget->pev,  "monster_gargantua" ) ) )
	{
		return R_NM;
	}

	// POV: the special forces are no friends of the marines
	if ( FClassnameIs( pTarget->pev, "monster_human_spforce" ) || FClassnameIs( pTarget->pev, "monster_hev_spforce" ) || FClassnameIs( pTarget->pev, "monster_hev_spforce_xtbu" ) )
	{
		return R_HT;
	}

	return CSquadMonster::IRelationship( pTarget );
}

//=========================================================
// GibMonster - POV: lose the XTBS
//=========================================================
void CXTBUGrunt :: GibMonster ( void )
{
	SetBodygroup( XTBU_GROUP, XTBU_NONE );

	CBaseMonster :: GibMonster();
}

//=========================================================
// ISoundMask - Overidden for human grunts because they
// hear the DANGER sound that is made by hand grenades and
// other dangerous items.
//=========================================================
int CXTBUGrunt :: ISoundMask ( void )
{
	return	bits_SOUND_WORLD	|
			bits_SOUND_COMBAT	|
			bits_SOUND_PLAYER	|
			bits_SOUND_DANGER;
}

//=========================================================
// someone else is talking - don't speak
//=========================================================
BOOL CXTBUGrunt :: FOkToSpeak( void )
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
void CXTBUGrunt :: JustSpoke( void )
{
	CTalkMonster::g_talkWaitTime = gpGlobals->time + RANDOM_FLOAT(1.5, 2.0);
	m_iSentence = XTBUGRUNT_SENT_NONE;
}

//=========================================================
// PrescheduleThink - this function runs after conditions
// are collected and before scheduling code is run.
//=========================================================
void CXTBUGrunt :: PrescheduleThink ( void )
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
//
// !!!BUGBUG - this gets called before a 3-round burst is fired
// which means that a friendly can still be hit with up to 2 rounds.
// ALSO, grenades will not be tossed if there is a friendly in front,
// this is a bad bug. Friendly machine gun fire avoidance
// will unecessarily prevent the throwing of a grenade as well.
//=========================================================
BOOL CXTBUGrunt :: FCanCheckAttacks ( void )
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
BOOL CXTBUGrunt :: CheckMeleeAttack1 ( float flDot, float flDist )
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
// CheckRangeAttack1 - POV: no gun
//=========================================================
BOOL CXTBUGrunt :: CheckRangeAttack1 ( float flDot, float flDist )
{
	return FALSE;
}

//=========================================================
// CheckRangeAttack2 - POV: no grenades
//=========================================================
BOOL CXTBUGrunt :: CheckRangeAttack2 ( float flDot, float flDist )
{
	return FALSE;
}


//=========================================================
// TraceAttack - POV: no helmet
//=========================================================
void CXTBUGrunt :: TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType)
{
	CSquadMonster::TraceAttack( pevAttacker, flDamage, vecDir, ptr, bitsDamageType );
}


//=========================================================
// TakeDamage - overridden for the grunt because the grunt
// needs to forget that he is in cover if he's hurt. (Obviously
// not in a safe place anymore).
//=========================================================
int CXTBUGrunt :: TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	Forget( bits_MEMORY_INCOVER );

	return CSquadMonster :: TakeDamage ( pevInflictor, pevAttacker, flDamage, bitsDamageType );
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CXTBUGrunt :: SetYawSpeed ( void )
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
	case ACT_RANGE_ATTACK1:
		ys = 120;
		break;
	case ACT_RANGE_ATTACK2:
		ys = 120;
		break;
	case ACT_MELEE_ATTACK1:
		ys = 120;
		break;
	case ACT_MELEE_ATTACK2:
		ys = 120;
		break;
	case ACT_TURN_LEFT:
	case ACT_TURN_RIGHT:
		ys = 180;
		break;
	case ACT_GLIDE:
	case ACT_FLY:
		ys = 30;
		break;
	default:
		ys = 90;
		break;
	}

	pev->yaw_speed = ys;
}

void CXTBUGrunt :: IdleSound( void )
{
	if (FOkToSpeak() && (g_fXTBUGruntQuestion || RANDOM_LONG(0,1)))
	{
		if (!g_fXTBUGruntQuestion)
		{
			// ask question or make statement
			switch (RANDOM_LONG(0,2))
			{
			case 0: // check in
				SENTENCEG_PlayRndSz(ENT(pev), "HG_CHECK", XTBUGRUNT_SENTENCE_VOLUME, ATTN_NORM, 0, m_voicePitch);
				g_fXTBUGruntQuestion = 1;
				break;
			case 1: // question
				SENTENCEG_PlayRndSz(ENT(pev), "HG_QUEST", XTBUGRUNT_SENTENCE_VOLUME, ATTN_NORM, 0, m_voicePitch);
				g_fXTBUGruntQuestion = 2;
				break;
			case 2: // statement
				SENTENCEG_PlayRndSz(ENT(pev), "HG_IDLE", XTBUGRUNT_SENTENCE_VOLUME, ATTN_NORM, 0, m_voicePitch);
				break;
			}
		}
		else
		{
			switch (g_fXTBUGruntQuestion)
			{
			case 1: // check in
				SENTENCEG_PlayRndSz(ENT(pev), "HG_CLEAR", XTBUGRUNT_SENTENCE_VOLUME, ATTN_NORM, 0, m_voicePitch);
				break;
			case 2: // question
				SENTENCEG_PlayRndSz(ENT(pev), "HG_ANSWER", XTBUGRUNT_SENTENCE_VOLUME, ATTN_NORM, 0, m_voicePitch);
				break;
			}
			g_fXTBUGruntQuestion = 0;
		}
		JustSpoke();
	}
}

//=========================================================
// CheckAmmo - overridden for the grunt because he actually
// uses ammo! (base class doesn't)
//=========================================================
void CXTBUGrunt :: CheckAmmo ( void )
{
	if ( m_cAmmoLoaded <= 0 )
	{
		SetConditions(bits_COND_NO_AMMO_LOADED);
	}
}

//=========================================================
// Classify - indicates this monster's place in the
// relationship table.
//=========================================================
int	CXTBUGrunt :: Classify ( void )
{
	return	CLASS_HUMAN_MILITARY;
}

//=========================================================
//=========================================================
CBaseEntity *CXTBUGrunt :: Kick( void )
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

Vector CXTBUGrunt :: GetGunPosition( )
{
	return pev->origin + Vector( 0, 0, 60 );
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CXTBUGrunt :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	Vector	vecShootDir;
	Vector	vecShootOrigin;

	switch( pEvent->event )
	{
		case XTBUGRUNT_AE_DROP_GUN:
			SetBodygroup( XTBU_GROUP, XTBU_NONE );
			break;

		case XTBUGRUNT_AE_KICK:
		{
			CBaseEntity *pHurt = Kick();

			if ( pHurt )
			{
				// SOUND HERE!
				EMIT_SOUND( ENT(pev), CHAN_WEAPON, "common/kick.wav", 1, ATTN_NORM );
				UTIL_MakeVectors( pev->angles );
				pHurt->pev->punchangle.x = 15;
				pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_forward * 100 + gpGlobals->v_up * 50;
				pHurt->TakeDamage( pev, pev, gSkillData.hgruntDmgKick, DMG_CLUB );
			}
		}
		break;

		case XTBUGRUNT_AE_CAUGHT_ENEMY:
		{
			if ( FOkToSpeak() )
			{
				SENTENCEG_PlayRndSz(ENT(pev), "HG_ALERT", XTBUGRUNT_SENTENCE_VOLUME, XTBUGRUNT_ATTN, 0, m_voicePitch);
				 JustSpoke();
			}

		}
		break;

		case XTBUGRUNT_AE_TAKE_PROP:
		{
			if ( !FStringNull( m_iszXTBUPropEnt ) )
			{
				char *pszName = "";	// sic: POV copies the name into a string literal

				strcpy( pszName, STRING( m_iszXTBUPropEnt ) );

				edict_t *pentProp = FIND_ENTITY_BY_TARGETNAME( NULL, pszName );
				while ( !FNullEnt( pentProp ) )
				{
					UTIL_Remove( CBaseEntity::Instance( pentProp ) );
					pentProp = FIND_ENTITY_BY_TARGETNAME( pentProp, pszName );
				}
			}
			else
			{
				ALERT( at_console, "HEVORHXTBU: Warning - no XTBU prop ent name given\n" );
			}
			SetBodygroup( XTBU_GROUP, XTBU_TAKEN );
		}
		break;

		case XTBUGRUNT_AE_SHOW_XTBU:
			SetBodygroup( XTBU_GROUP, XTBU_SHOWN );
			break;

		default:
			CSquadMonster::HandleAnimEvent( pEvent );
			break;
	}
}

//=========================================================
// Spawn
//=========================================================
void CXTBUGrunt :: Spawn()
{
	Precache( );

	SET_MODEL(ENT(pev), "models/gruntegged.mdl");
	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= BLOOD_COLOR_RED;
	pev->effects		= 0;
	if ( pev->spawnflags & SF_XTBUGRUNT_INVULNERABLE )
		pev->health		= 80000;
	else
		pev->health		= gSkillData.hgruntHealth;
	m_flFieldOfView		= 0.2;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState		= MONSTERSTATE_NONE;
	m_flNextPainTime	= gpGlobals->time;
	m_iSentence			= XTBUGRUNT_SENT_NONE;

	m_afCapability		= bits_CAP_SQUAD | bits_CAP_TURN_HEAD | bits_CAP_DOORS_GROUP;

	m_fEnemyEluded		= FALSE;
	m_fFirstEncounter	= TRUE;// this is true when the grunt spawns, because he hasn't encountered an enemy yet.

	m_HackedGunPos = Vector ( 0, 0, 55 );

	pev->skin = 0;
	pev->weapons = 0;

	SetBodygroup( HEAD_GROUP, HEAD_GRUNT );
	SetBodygroup( GUN_GROUP, GUN_MP5 );
	if ( pev->spawnflags & SF_XTBUGRUNT_SHOWXTBU )
		SetBodygroup( XTBU_GROUP, XTBU_SHOWN );
	else
		SetBodygroup( XTBU_GROUP, XTBU_NONE );

	CTalkMonster::g_talkWaitTime = 0;

	MonsterInit();
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CXTBUGrunt :: Precache()
{
	PRECACHE_MODEL("models/gruntegged.mdl");

	PRECACHE_SOUND("zombie/claw_miss2.wav");// because we use the basemonster SWIPE animation event
	PRECACHE_SOUND("common/kick.wav");

	m_voicePitch = 108;
}

//=========================================================
// start task
//=========================================================
void CXTBUGrunt :: StartTask ( Task_t *pTask )
{
	m_iTaskStatus = TASKSTATUS_RUNNING;

	switch ( pTask->iTask )
	{
	case TASK_XTBUGRUNT_CHECK_FIRE:
		if ( !NoFriendlyFire() )
		{
			SetConditions( bits_COND_XTBUGRUNT_NOFIRE );
		}
		TaskComplete();
		break;

	case TASK_XTBUGRUNT_SPEAK_SENTENCE:
		SpeakSentence();
		TaskComplete();
		break;

	case TASK_WALK_PATH:
	case TASK_RUN_PATH:
		// grunt no longer assumes he is covered if he moves
		Forget( bits_MEMORY_INCOVER );
		CSquadMonster ::StartTask( pTask );
		break;

	case TASK_RELOAD:
		break;

	case TASK_XTBUGRUNT_FACE_TOSS_DIR:
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
void CXTBUGrunt :: RunTask ( Task_t *pTask )
{
	switch ( pTask->iTask )
	{
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
void CXTBUGrunt :: PainSound ( void )
{
	if ( gpGlobals->time > m_flNextPainTime )
	{
		if (FOkToSpeak())
		{
			SENTENCEG_PlayRndSz(ENT(pev), "HG_PAIN", XTBUGRUNT_SENTENCE_VOLUME, ATTN_NORM, 0, m_voicePitch);
			JustSpoke();
		}

		m_flNextPainTime = gpGlobals->time + 1;
	}
}

//=========================================================
// DeathSound
//=========================================================
void CXTBUGrunt :: DeathSound ( void )
{
	SENTENCEG_PlayRndSz(ENT(pev), "HG_DIE", XTBUGRUNT_SENTENCE_VOLUME, ATTN_NORM, 0, m_voicePitch);
}

//=========================================================
// AI Schedules Specific to this monster
//=========================================================

//=========================================================
// GruntFail
//=========================================================
Task_t	tlXTBUGruntFail[] =
{
	{ TASK_STOP_MOVING,			0				},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE },
	{ TASK_WAIT,				(float)2		},
	{ TASK_WAIT_PVS,			(float)0		},
};

Schedule_t	slXTBUGruntFail[] =
{
	{
		tlXTBUGruntFail,
		ARRAYSIZE ( tlXTBUGruntFail ),
		bits_COND_CAN_RANGE_ATTACK1 |
		bits_COND_CAN_RANGE_ATTACK2 |
		bits_COND_CAN_MELEE_ATTACK1 |
		bits_COND_CAN_MELEE_ATTACK2,
		0,
		"XTBU Grunt Fail"
	},
};

//=========================================================
// Grunt Combat Fail
//=========================================================
Task_t	tlXTBUGruntCombatFail[] =
{
	{ TASK_STOP_MOVING,			0				},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE },
	{ TASK_WAIT_FACE_ENEMY,		(float)2		},
	{ TASK_WAIT_PVS,			(float)0		},
};

Schedule_t	slXTBUGruntCombatFail[] =
{
	{
		tlXTBUGruntCombatFail,
		ARRAYSIZE ( tlXTBUGruntCombatFail ),
		bits_COND_CAN_RANGE_ATTACK1	|
		bits_COND_CAN_RANGE_ATTACK2,
		0,
		"XTBU Grunt Combat Fail"
	},
};

//=========================================================
// Victory dance!
//=========================================================
Task_t	tlXTBUGruntVictoryDance[] =
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

Schedule_t	slXTBUGruntVictoryDance[] =
{
	{
		tlXTBUGruntVictoryDance,
		ARRAYSIZE ( tlXTBUGruntVictoryDance ),
		bits_COND_NEW_ENEMY		|
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE,
		0,
		"XTBUGruntVictoryDance"
	},
};

//=========================================================
// Establish line of fire - move to a position that allows
// the grunt to attack.
//=========================================================
Task_t tlXTBUGruntEstablishLineOfFire[] =
{
	{ TASK_SET_FAIL_SCHEDULE,	(float)SCHED_XTBUGRUNT_ELOF_FAIL	},
	{ TASK_GET_PATH_TO_ENEMY,	(float)0						},
	{ TASK_XTBUGRUNT_SPEAK_SENTENCE,(float)0						},
	{ TASK_RUN_PATH,			(float)0						},
	{ TASK_WAIT_FOR_MOVEMENT,	(float)0						},
};

Schedule_t slXTBUGruntEstablishLineOfFire[] =
{
	{
		tlXTBUGruntEstablishLineOfFire,
		ARRAYSIZE ( tlXTBUGruntEstablishLineOfFire ),
		bits_COND_NEW_ENEMY			|
		bits_COND_ENEMY_DEAD		|
		bits_COND_CAN_RANGE_ATTACK1	|
		bits_COND_CAN_MELEE_ATTACK1	|
		bits_COND_CAN_RANGE_ATTACK2	|
		bits_COND_CAN_MELEE_ATTACK2	|
		bits_COND_HEAR_SOUND,

		bits_SOUND_DANGER,
		"XTBUGruntEstablishLineOfFire"
	},
};

//=========================================================
// GruntFoundEnemy - grunt established sight with an enemy
// that was hiding from the squad.
//=========================================================
Task_t	tlXTBUGruntFoundEnemy[] =
{
	{ TASK_STOP_MOVING,				0							},
	{ TASK_FACE_ENEMY,				(float)0					},
	{ TASK_PLAY_SEQUENCE_FACE_ENEMY,(float)ACT_SIGNAL1			},
};

Schedule_t	slXTBUGruntFoundEnemy[] =
{
	{
		tlXTBUGruntFoundEnemy,
		ARRAYSIZE ( tlXTBUGruntFoundEnemy ),
		bits_COND_HEAR_SOUND,

		bits_SOUND_DANGER,
		"XTBUGruntFoundEnemy"
	},
};

//=========================================================
// GruntCombatFace Schedule
//=========================================================
Task_t	tlXTBUGruntCombatFace1[] =
{
	{ TASK_STOP_MOVING,				0							},
	{ TASK_SET_ACTIVITY,			(float)ACT_IDLE				},
	{ TASK_FACE_ENEMY,				(float)0					},
	{ TASK_WAIT,					(float)1.5					},
	{ TASK_SET_SCHEDULE,			(float)SCHED_XTBUGRUNT_SWEEP	},
};

Schedule_t	slXTBUGruntCombatFace[] =
{
	{
		tlXTBUGruntCombatFace1,
		ARRAYSIZE ( tlXTBUGruntCombatFace1 ),
		bits_COND_NEW_ENEMY				|
		bits_COND_ENEMY_DEAD			|
		bits_COND_CAN_RANGE_ATTACK1		|
		bits_COND_CAN_RANGE_ATTACK2,
		0,
		"XTBU Combat Face"
	},
};

//=========================================================
// Suppressing fire - don't stop shooting until the clip is
// empty or grunt gets hurt.
//=========================================================
Task_t	tlXTBUGruntSignalSuppress[] =
{
	{ TASK_STOP_MOVING,					0						},
	{ TASK_FACE_IDEAL,					(float)0				},
	{ TASK_PLAY_SEQUENCE_FACE_ENEMY,	(float)ACT_SIGNAL2		},
	{ TASK_FACE_ENEMY,					(float)0				},
	{ TASK_XTBUGRUNT_CHECK_FIRE,			(float)0				},
	{ TASK_RANGE_ATTACK1,				(float)0				},
	{ TASK_FACE_ENEMY,					(float)0				},
	{ TASK_XTBUGRUNT_CHECK_FIRE,			(float)0				},
	{ TASK_RANGE_ATTACK1,				(float)0				},
	{ TASK_FACE_ENEMY,					(float)0				},
	{ TASK_XTBUGRUNT_CHECK_FIRE,			(float)0				},
	{ TASK_RANGE_ATTACK1,				(float)0				},
	{ TASK_FACE_ENEMY,					(float)0				},
	{ TASK_XTBUGRUNT_CHECK_FIRE,			(float)0				},
	{ TASK_RANGE_ATTACK1,				(float)0				},
	{ TASK_FACE_ENEMY,					(float)0				},
	{ TASK_XTBUGRUNT_CHECK_FIRE,			(float)0				},
	{ TASK_RANGE_ATTACK1,				(float)0				},
};

Schedule_t	slXTBUGruntSignalSuppress[] =
{
	{
		tlXTBUGruntSignalSuppress,
		ARRAYSIZE ( tlXTBUGruntSignalSuppress ),
		bits_COND_ENEMY_DEAD		|
		bits_COND_LIGHT_DAMAGE		|
		bits_COND_HEAVY_DAMAGE		|
		bits_COND_HEAR_SOUND		|
		bits_COND_XTBUGRUNT_NOFIRE		|
		bits_COND_NO_AMMO_LOADED,

		bits_SOUND_DANGER,
		"XTBUSignalSuppress"
	},
};

Task_t	tlXTBUGruntSuppress[] =
{
	{ TASK_STOP_MOVING,			0							},
	{ TASK_FACE_ENEMY,			(float)0					},
	{ TASK_XTBUGRUNT_CHECK_FIRE,	(float)0					},
	{ TASK_RANGE_ATTACK1,		(float)0					},
	{ TASK_FACE_ENEMY,			(float)0					},
	{ TASK_XTBUGRUNT_CHECK_FIRE,	(float)0					},
	{ TASK_RANGE_ATTACK1,		(float)0					},
	{ TASK_FACE_ENEMY,			(float)0					},
	{ TASK_XTBUGRUNT_CHECK_FIRE,	(float)0					},
	{ TASK_RANGE_ATTACK1,		(float)0					},
	{ TASK_FACE_ENEMY,			(float)0					},
	{ TASK_XTBUGRUNT_CHECK_FIRE,	(float)0					},
	{ TASK_RANGE_ATTACK1,		(float)0					},
	{ TASK_FACE_ENEMY,			(float)0					},
	{ TASK_XTBUGRUNT_CHECK_FIRE,	(float)0					},
	{ TASK_RANGE_ATTACK1,		(float)0					},
};

Schedule_t	slXTBUGruntSuppress[] =
{
	{
		tlXTBUGruntSuppress,
		ARRAYSIZE ( tlXTBUGruntSuppress ),
		bits_COND_ENEMY_DEAD		|
		bits_COND_LIGHT_DAMAGE		|
		bits_COND_HEAVY_DAMAGE		|
		bits_COND_HEAR_SOUND		|
		bits_COND_XTBUGRUNT_NOFIRE		|
		bits_COND_NO_AMMO_LOADED,

		bits_SOUND_DANGER,
		"XTBUSuppress"
	},
};


//=========================================================
// grunt wait in cover - we don't allow danger or the ability
// to attack to break a grunt's run to cover schedule, but
// when a grunt is in cover, we do want them to attack if they can.
//=========================================================
Task_t	tlXTBUGruntWaitInCover[] =
{
	{ TASK_STOP_MOVING,				(float)0					},
	{ TASK_SET_ACTIVITY,			(float)ACT_IDLE				},
	{ TASK_WAIT_FACE_ENEMY,			(float)1					},
};

Schedule_t	slXTBUGruntWaitInCover[] =
{
	{
		tlXTBUGruntWaitInCover,
		ARRAYSIZE ( tlXTBUGruntWaitInCover ),
		bits_COND_NEW_ENEMY			|
		bits_COND_HEAR_SOUND		|
		bits_COND_CAN_RANGE_ATTACK1	|
		bits_COND_CAN_RANGE_ATTACK2	|
		bits_COND_CAN_MELEE_ATTACK1	|
		bits_COND_CAN_MELEE_ATTACK2,

		bits_SOUND_DANGER,
		"XTBUGruntWaitInCover"
	},
};

//=========================================================
// run to cover.
// !!!BUGBUG - set a decent fail schedule here.
//=========================================================
Task_t	tlXTBUGruntTakeCover1[] =
{
	{ TASK_STOP_MOVING,				(float)0							},
	{ TASK_SET_FAIL_SCHEDULE,		(float)SCHED_XTBUGRUNT_TAKECOVER_FAILED	},
	{ TASK_WAIT,					(float)0.2							},
	{ TASK_FIND_COVER_FROM_ENEMY,	(float)0							},
	{ TASK_XTBUGRUNT_SPEAK_SENTENCE,	(float)0							},
	{ TASK_RUN_PATH,				(float)0							},
	{ TASK_WAIT_FOR_MOVEMENT,		(float)0							},
	{ TASK_REMEMBER,				(float)bits_MEMORY_INCOVER			},
	{ TASK_SET_SCHEDULE,			(float)SCHED_XTBUGRUNT_WAIT_FACE_ENEMY	},
};

Schedule_t	slXTBUGruntTakeCover[] =
{
	{
		tlXTBUGruntTakeCover1,
		ARRAYSIZE ( tlXTBUGruntTakeCover1 ),
		0,
		0,
		"XTBUTakeCover"
	},
};

//=========================================================
// drop grenade then run to cover.
//=========================================================
Task_t	tlXTBUGruntGrenadeCover1[] =
{
	{ TASK_STOP_MOVING,						(float)0							},
	{ TASK_FIND_COVER_FROM_ENEMY,			(float)99							},
	{ TASK_FIND_FAR_NODE_COVER_FROM_ENEMY,	(float)384							},
	{ TASK_PLAY_SEQUENCE,					(float)ACT_SPECIAL_ATTACK1			},
	{ TASK_CLEAR_MOVE_WAIT,					(float)0							},
	{ TASK_RUN_PATH,						(float)0							},
	{ TASK_WAIT_FOR_MOVEMENT,				(float)0							},
	{ TASK_SET_SCHEDULE,					(float)SCHED_XTBUGRUNT_WAIT_FACE_ENEMY	},
};

Schedule_t	slXTBUGruntGrenadeCover[] =
{
	{
		tlXTBUGruntGrenadeCover1,
		ARRAYSIZE ( tlXTBUGruntGrenadeCover1 ),
		0,
		0,
		"XTBUGrenadeCover"
	},
};


//=========================================================
// drop grenade then run to cover.
//=========================================================
Task_t	tlXTBUGruntTossGrenadeCover1[] =
{
	{ TASK_FACE_ENEMY,						(float)0							},
	{ TASK_RANGE_ATTACK2, 					(float)0							},
	{ TASK_SET_SCHEDULE,					(float)SCHED_TAKE_COVER_FROM_ENEMY	},
};

Schedule_t	slXTBUGruntTossGrenadeCover[] =
{
	{
		tlXTBUGruntTossGrenadeCover1,
		ARRAYSIZE ( tlXTBUGruntTossGrenadeCover1 ),
		0,
		0,
		"XTBUTossGrenadeCover"
	},
};

//=========================================================
// hide from the loudest sound source (to run from grenade)
//=========================================================
Task_t	tlXTBUGruntTakeCoverFromBestSound[] =
{
	{ TASK_SET_FAIL_SCHEDULE,			(float)SCHED_COWER			},// duck and cover if cannot move from explosion
	{ TASK_STOP_MOVING,					(float)0					},
	{ TASK_FIND_COVER_FROM_BEST_SOUND,	(float)0					},
	{ TASK_RUN_PATH,					(float)0					},
	{ TASK_WAIT_FOR_MOVEMENT,			(float)0					},
	{ TASK_REMEMBER,					(float)bits_MEMORY_INCOVER	},
	{ TASK_TURN_LEFT,					(float)179					},
};

Schedule_t	slXTBUGruntTakeCoverFromBestSound[] =
{
	{
		tlXTBUGruntTakeCoverFromBestSound,
		ARRAYSIZE ( tlXTBUGruntTakeCoverFromBestSound ),
		0,
		0,
		"XTBUGruntTakeCoverFromBestSound"
	},
};

//=========================================================
// Do a turning sweep of the area
//=========================================================
Task_t	tlXTBUGruntSweep[] =
{
	{ TASK_TURN_LEFT,			(float)179	},
	{ TASK_WAIT,				(float)1	},
	{ TASK_TURN_LEFT,			(float)179	},
	{ TASK_WAIT,				(float)1	},
};

Schedule_t	slXTBUGruntSweep[] =
{
	{
		tlXTBUGruntSweep,
		ARRAYSIZE ( tlXTBUGruntSweep ),

		bits_COND_NEW_ENEMY		|
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE	|
		bits_COND_CAN_RANGE_ATTACK1	|
		bits_COND_CAN_RANGE_ATTACK2	|
		bits_COND_HEAR_SOUND,

		bits_SOUND_WORLD		|// sound flags
		bits_SOUND_DANGER		|
		bits_SOUND_PLAYER,

		"XTBU Grunt Sweep"
	},
};


DEFINE_CUSTOM_SCHEDULES( CXTBUGrunt )
{
	slXTBUGruntFail,
	slXTBUGruntCombatFail,
	slXTBUGruntVictoryDance,
	slXTBUGruntEstablishLineOfFire,
	slXTBUGruntFoundEnemy,
	slXTBUGruntCombatFace,
	slXTBUGruntSignalSuppress,
	slXTBUGruntSuppress,
	slXTBUGruntWaitInCover,
	slXTBUGruntTakeCover,
	slXTBUGruntGrenadeCover,
	slXTBUGruntTossGrenadeCover,
	slXTBUGruntTakeCoverFromBestSound,
	slXTBUGruntSweep,
};

IMPLEMENT_CUSTOM_SCHEDULES( CXTBUGrunt, CSquadMonster );

//=========================================================
// SetActivity
//=========================================================
void CXTBUGrunt :: SetActivity ( Activity NewActivity )
{
	int	iSequence = ACTIVITY_NOT_AVAILABLE;
	void *pmodel = GET_MODEL_PTR( ENT(pev) );

	switch ( NewActivity)
	{
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
		ALERT ( at_console, "%s has no sequence for act:%d\n", STRING(pev->classname), NewActivity );
		pev->sequence		= 0;	// Set to the reset anim (if it's there)
	}
}

//=========================================================
// Get Schedule!
//=========================================================
Schedule_t *CXTBUGrunt :: GetSchedule( void )
{

	// clear old sentence
	m_iSentence = XTBUGRUNT_SENT_NONE;

	// grunts place HIGH priority on running away from danger sounds.
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

				//!!!KELLY - currently, this is the grunt's signal that a grenade has landed nearby,
				// and the grunt should find cover from the blast
				// good place for "SHIT!" or some other colorful verbal indicator of dismay.
				// It's not safe to play a verbal order here "Scatter", etc cause
				// this may only affect a single individual in a squad.

				if (FOkToSpeak())
				{
					SENTENCEG_PlayRndSz( ENT(pev), "HG_GREN", XTBUGRUNT_SENTENCE_VOLUME, XTBUGRUNT_ATTN, 0, m_voicePitch);
					JustSpoke();
				}
				return GetScheduleOfType( SCHED_TAKE_COVER_FROM_BEST_SOUND );
			}
			/*
			if (!HasConditions( bits_COND_SEE_ENEMY ) && ( pSound->m_iType & (bits_SOUND_PLAYER | bits_SOUND_COMBAT) ))
			{
				MakeIdealYaw( pSound->m_vecOrigin );
			}
			*/
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
						//!!!KELLY - the leader of a squad of grunts has just seen the player or a
						// monster and has made it the squad's enemy. You
						// can check pev->flags for FL_CLIENT to determine whether this is the player
						// or a monster. He's going to immediately start
						// firing, though. If you'd like, we can make an alternate "first sight"
						// schedule where the leader plays a handsign anim
						// that gives us enough time to hear a short sentence or spoken command
						// before he starts pluggin away.
						if (FOkToSpeak())// && RANDOM_LONG(0,1))
						{
							if ((m_hEnemy != NULL) && m_hEnemy->IsPlayer())
								// player
								SENTENCEG_PlayRndSz( ENT(pev), "HG_ALERT", XTBUGRUNT_SENTENCE_VOLUME, XTBUGRUNT_ATTN, 0, m_voicePitch);
							else if ((m_hEnemy != NULL) &&
									(m_hEnemy->Classify() != CLASS_PLAYER_ALLY) &&
									(m_hEnemy->Classify() != CLASS_HUMAN_PASSIVE) &&
									(m_hEnemy->Classify() != CLASS_MACHINE))
								// monster
								SENTENCEG_PlayRndSz( ENT(pev), "HG_MONST", XTBUGRUNT_SENTENCE_VOLUME, XTBUGRUNT_ATTN, 0, m_voicePitch);

							JustSpoke();
						}

						if ( HasConditions ( bits_COND_CAN_RANGE_ATTACK1 ) )
						{
							return GetScheduleOfType ( SCHED_XTBUGRUNT_SUPPRESS );
						}
						else
						{
							return GetScheduleOfType ( SCHED_XTBUGRUNT_ESTABLISH_LINE_OF_FIRE );
						}
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

					//!!!KELLY - this grunt was hit and is going to run to cover.
					if (FOkToSpeak()) // && RANDOM_LONG(0,1))
					{
						//SENTENCEG_PlayRndSz( ENT(pev), "HG_COVER", XTBUGRUNT_SENTENCE_VOLUME, XTBUGRUNT_ATTN, 0, m_voicePitch);
						m_iSentence = XTBUGRUNT_SENT_COVER;
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
// can't see enemy
			else if ( HasConditions( bits_COND_ENEMY_OCCLUDED ) )
			{
				//!!!KELLY - grunt is going to stay put for a couple seconds to see if
				// the enemy wanders back out into the open, or approaches the
				// grunt's covered position. Good place for a taunt, I guess?
				if (FOkToSpeak() && RANDOM_LONG(0,1))
				{
					SENTENCEG_PlayRndSz( ENT(pev), "HG_TAUNT", XTBUGRUNT_SENTENCE_VOLUME, XTBUGRUNT_ATTN, 0, m_voicePitch);
					JustSpoke();
				}
				return GetScheduleOfType( SCHED_STANDOFF );
			}

			if ( HasConditions( bits_COND_SEE_ENEMY ) && !HasConditions ( bits_COND_CAN_RANGE_ATTACK1 ) )
			{
				return GetScheduleOfType ( SCHED_XTBUGRUNT_ESTABLISH_LINE_OF_FIRE );
			}
		}
	}

	// no special cases here, call the base class
	return CSquadMonster :: GetSchedule();
}

//=========================================================
//=========================================================
Schedule_t* CXTBUGrunt :: GetScheduleOfType ( int Type )
{
	switch	( Type )
	{
	case SCHED_TAKE_COVER_FROM_ENEMY:
		{
			if ( InSquad() )
			{
				return &slXTBUGruntTakeCover[ 0 ];
			}
			else
			{
				return &slXTBUGruntTakeCover[ 0 ];
			}
		}
	case SCHED_TAKE_COVER_FROM_BEST_SOUND:
		{
			return &slXTBUGruntTakeCoverFromBestSound[ 0 ];
		}
	case SCHED_XTBUGRUNT_TAKECOVER_FAILED:
		{
			return GetScheduleOfType ( SCHED_FAIL );
		}
		break;
	case SCHED_XTBUGRUNT_ELOF_FAIL:
		{
			// human grunt is unable to move to a position that allows him to attack the enemy.
			return GetScheduleOfType ( SCHED_TAKE_COVER_FROM_ENEMY );
		}
		break;
	case SCHED_XTBUGRUNT_ESTABLISH_LINE_OF_FIRE:
		{
			return &slXTBUGruntEstablishLineOfFire[ 0 ];
		}
		break;
	case SCHED_COMBAT_FACE:
		{
			return &slXTBUGruntCombatFace[ 0 ];
		}
	case SCHED_XTBUGRUNT_WAIT_FACE_ENEMY:
		{
			return &slXTBUGruntWaitInCover[ 0 ];
		}
	case SCHED_XTBUGRUNT_SWEEP:
		{
			return &slXTBUGruntSweep[ 0 ];
		}
	case SCHED_XTBUGRUNT_FOUND_ENEMY:
		{
			return &slXTBUGruntFoundEnemy[ 0 ];
		}
	case SCHED_VICTORY_DANCE:
		{
			return &slXTBUGruntVictoryDance[ 0 ];
		}
	case SCHED_XTBUGRUNT_SUPPRESS:
		{
			if ( m_hEnemy->IsPlayer() && m_fFirstEncounter )
			{
				m_fFirstEncounter = FALSE;// after first encounter, leader won't issue handsigns anymore when he has a new enemy
				return &slXTBUGruntSignalSuppress[ 0 ];
			}
			else
			{
				return &slXTBUGruntSuppress[ 0 ];
			}
		}
	case SCHED_FAIL:
		{
			if ( m_hEnemy != NULL )
			{
				// grunt has an enemy, so pick a different default fail schedule most likely to help recover.
				return &slXTBUGruntCombatFail[ 0 ];
			}

			return &slXTBUGruntFail[ 0 ];
		}
	default:
		{
			return CSquadMonster :: GetScheduleOfType ( Type );
		}
	}
}

//=========================================================
// KeyValue - POV: "xtbupropent"
//=========================================================
void CXTBUGrunt :: KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "xtbupropent"))
	{
		m_iszXTBUPropEnt = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		CSquadMonster::KeyValue( pkvd );
}

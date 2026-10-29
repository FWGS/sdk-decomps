/***
*
*	Copyright (c) 1999, Valve LLC. All rights reserved.
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
// the rust bot: a big robot of the rust chapter that punches
// and sweeps a beam from its head, talking like a human grunt;
// monster_rustbot fights the player, monster_rustbot_friendly
// is on his side
//=========================================================

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"schedule.h"
#include	"animation.h"
#include	"weapons.h"
#include	"talkmonster.h"
#include	"effects.h"
#include	"customentity.h"
#include	"soundent.h"
#include	"rust.h"

extern DLL_GLOBAL int g_iSkillLevel;

int g_fRustbotQuestion;				// true if an idle rust bot asked a question. Cleared when someone answers.

// sic: retail has 4 bytes of .bss after g_fRustbotQuestion, before beak.obj's,
// that nothing reads or writes (name ours)
static int s_iUnknown;

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define		RUSTBOT_AE_PUNCH			( 1 )
#define		RUSTBOT_AE_BEAM_OFF			( 2 )
#define		RUSTBOT_AE_BEAM_ON			( 3 )

#define		SF_RUSTBOT_BORG				64	// stands still until hurt

//=========================================================
// the sentence groups, by m_iSentence
//=========================================================
enum
{
	RUSTBOT_SENT_GREN = 0,
	RUSTBOT_SENT_ALERT,
	RUSTBOT_SENT_COVER,
	RUSTBOT_SENT_THROW,
	RUSTBOT_SENT_CHARGE,
	RUSTBOT_SENT_TAUNT,
	RUSTBOT_SENT_CHECK,
	RUSTBOT_SENT_QUEST,
	RUSTBOT_SENT_IDLE,
	RUSTBOT_SENT_CLEAR,
	RUSTBOT_SENT_ANSWER,
	RUSTBOT_SENT_DANGER,
};

//=========================================================
// monster-specific schedule types
//=========================================================
enum
{
	SCHED_RUSTBOT_ESTABLISH_LINE_OF_FIRE = LAST_COMMON_SCHEDULE + 1,// move to a location to set up an attack against the enemy.
	SCHED_RUSTBOT_CHARGE,			// the same, but face the enemy if it fails
	SCHED_RUSTBOT_SWEEP,
	SCHED_RUSTBOT_WAIT_IN_COVER,
	SCHED_RUSTBOT_TAKECOVER_FAILED,
	SCHED_RUSTBOT_ELOF_FAIL,
	SCHED_RUSTBOT_BORG,
};

//=========================================================
// monster-specific tasks
//=========================================================
enum
{
	TASK_RUSTBOT_FINISH_ANIM = LAST_COMMON_TASK + 1,	// no schedule uses it
	TASK_RUSTBOT_UNUSED,
	TASK_RUSTBOT_FIRE,
	TASK_RUSTBOT_CHARGE,
	TASK_RUSTBOT_SPEAK_SENTENCE,
};

class CRustbot : public CBaseRust
{
public:
	void Spawn( void );
	void Precache( void );
	void SetYawSpeed( void );
	int  Classify( void );
	int  MonsterType( void );
	void SetObjectCollisionBox( void );
	void PrecacheSounds( void );
	const char *SentenceGroup( int index );
	void HandleAnimEvent( MonsterEvent_t *pEvent );
	BOOL CheckMeleeAttack1( float flDot, float flDist );
	BOOL CheckRangeAttack1( float flDot, float flDist );
	void StartTask( Task_t *pTask );
	void RunTask( Task_t *pTask );
	void RunAI( void );
	void SetActivity( Activity NewActivity );
	void PrescheduleThink( void );
	int  TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );
	void Killed( entvars_t *pevAttacker, int iGib );
	void PreRemove( void );

	void PlayBeamSound( int iSound );
	void UpdateBeam( void );
	void CreateBeam( float flUnused );
	void DestroyBeam( void );
	void BeamOn( void );
	void CenterHead( void );
	void AimHead( const Vector &vecTarget );
	void Sparks( int iCount, int iFrustration );

	void PainSound( void );
	void DeathSound( void );
	void IdleSound( void );
	void AlertSound( void );
	void MeleeAttackSound( void );
	void AttackHitSound( CBaseMonster *pVictim );

	Schedule_t *GetSchedule( void );
	Schedule_t *GetScheduleOfType( int Type );

	int	Save( CSave &save );
	int Restore( CRestore &restore );

	CUSTOM_SCHEDULES;
	static TYPEDESCRIPTION m_SaveData[];

	static const char *pRustbotSentences[];
	static const char *pSounds[];

	CBeam	*m_pBeam;
	CBeam	*m_pNoise;
	CSprite	*m_pSprite;			// where the beam hits
	int		m_iUnused2c8;
	int		m_iHeadTurn;
	float	m_fFrustration;		// 0 to 10; hits and misses raise it, quiet time lowers it
	float	m_tChangeFrustration;
	float	m_fAgility;			// random, speeds up the punch
	float	m_fIntel;			// 1 - m_fAgility, never read
	int		m_iUnused2e0;
	BOOL	m_fOnTarget;		// the head points at the enemy
	BOOL	m_bBorg;			// stand still until hurt
};

//=========================================================
// AI Schedules Specific to this monster
//=========================================================

//=========================================================
// Fail
//=========================================================
Task_t	tlRustbotFail[] =
{
	{ TASK_STOP_MOVING,			(float)0		},
	{ TASK_SET_ACTIVITY_IDLE,	(float)0		},
	{ TASK_WAIT,				(float)2		},
	{ TASK_WAIT_PVS,			(float)0		},
};

Schedule_t	slRustbotFail[] =
{
	{
		tlRustbotFail,
		ARRAYSIZE ( tlRustbotFail ),
		bits_COND_CAN_RANGE_ATTACK1	|
		bits_COND_CAN_MELEE_ATTACK1,
		0,
		"Rustbot Fail"
	},
};

//=========================================================
// Combat Fail
//=========================================================
Task_t	tlRustbotCombatFail[] =
{
	{ TASK_STOP_MOVING,			(float)0		},
	{ TASK_SET_ACTIVITY_IDLE,	(float)0		},
	{ TASK_WAIT_FACE_ENEMY,		(float)0		},
	{ TASK_WAIT_PVS,			(float)0		},
};

Schedule_t	slRustbotCombatFail[] =
{
	{
		tlRustbotCombatFail,
		ARRAYSIZE ( tlRustbotCombatFail ),
		bits_COND_SEE_ENEMY			|
		bits_COND_CAN_RANGE_ATTACK1	|
		bits_COND_CAN_MELEE_ATTACK1	|
		bits_COND_NEW_ENEMY,
		0,
		"Rustbot Combat Fail"
	},
};

//=========================================================
// Borg - stand still until hurt
//=========================================================
Task_t	tlRustbotBorg[] =
{
	{ TASK_STOP_MOVING,			(float)0		},
	{ TASK_SET_ACTIVITY_IDLE,	(float)0		},
	{ TASK_SET_SCHEDULE,		(float)SCHED_RUSTBOT_BORG },
};

Schedule_t	slRustbotBorg[] =
{
	{
		tlRustbotBorg,
		ARRAYSIZE ( tlRustbotBorg ),
		bits_COND_LIGHT_DAMAGE		|
		bits_COND_HEAVY_DAMAGE,
		0,
		"Rustbot Borg"
	},
};

//=========================================================
// Establish line of fire - move to a position that allows
// the rust bot to attack.
//=========================================================
Task_t tlRustbotEstablishLineOfFire[] =
{
	{ TASK_SET_FAIL_SCHEDULE,		(float)SCHED_RUSTBOT_ELOF_FAIL	},
	{ TASK_GET_PATH_TO_ENEMY,		(float)0						},
	{ TASK_RUSTBOT_SPEAK_SENTENCE,	(float)0						},
	{ TASK_RUN_PATH,				(float)0						},
	{ TASK_WAIT_FOR_MOVEMENT,		(float)0						},
};

Schedule_t slRustbotEstablishLineOfFire[] =
{
	{
		tlRustbotEstablishLineOfFire,
		ARRAYSIZE ( tlRustbotEstablishLineOfFire ),
		bits_COND_CAN_RANGE_ATTACK1	|
		bits_COND_CAN_MELEE_ATTACK1	|
		bits_COND_NEW_ENEMY			|
		bits_COND_HEAR_SOUND		|
		bits_COND_ENEMY_DEAD,

		bits_SOUND_DANGER,
		"Rusbot Establish Line Of Fire"
	},
};

//=========================================================
// Charge - the same, facing the enemy if it fails
//=========================================================
Task_t tlRustbotCharge[] =
{
	{ TASK_SET_FAIL_SCHEDULE,		(float)SCHED_COMBAT_FACE	},
	{ TASK_GET_PATH_TO_ENEMY,		(float)0					},
	{ TASK_RUSTBOT_SPEAK_SENTENCE,	(float)0					},
	{ TASK_RUN_PATH,				(float)0					},
	{ TASK_WAIT_FOR_MOVEMENT,		(float)0					},
};

Schedule_t slRustbotCharge[] =
{
	{
		tlRustbotCharge,
		ARRAYSIZE ( tlRustbotCharge ),
		bits_COND_CAN_RANGE_ATTACK1	|
		bits_COND_CAN_MELEE_ATTACK1	|
		bits_COND_NEW_ENEMY			|
		bits_COND_HEAR_SOUND		|
		bits_COND_ENEMY_DEAD,

		bits_SOUND_DANGER,
		"Rusbot Establish Line Of Fire"
	},
};

//=========================================================
// Combat Face - face the enemy, then sweep for him
//=========================================================
Task_t	tlRustbotCombatFace[] =
{
	{ TASK_STOP_MOVING,			(float)0		},
	{ TASK_SET_ACTIVITY_IDLE,	(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_WAIT,				(float)1.5		},
	{ TASK_SET_SCHEDULE,		(float)SCHED_RUSTBOT_SWEEP },
};

Schedule_t	slRustbotCombatFace[] =
{
	{
		tlRustbotCombatFace,
		ARRAYSIZE ( tlRustbotCombatFace ),
		bits_COND_CAN_RANGE_ATTACK1	|
		bits_COND_CAN_MELEE_ATTACK1	|
		bits_COND_NEW_ENEMY			|
		bits_COND_ENEMY_DEAD,
		0,
		"Rustbot Combat Face"
	},
};

//=========================================================
// Wait In Cover
//=========================================================
Task_t	tlRustbotWaitInCover[] =
{
	{ TASK_STOP_MOVING,			(float)0		},
	{ TASK_SET_ACTIVITY_IDLE,	(float)0		},
	{ TASK_WAIT_FACE_ENEMY,		(float)0.5		},
};

Schedule_t	slRustbotWaitInCover[] =
{
	{
		tlRustbotWaitInCover,
		ARRAYSIZE ( tlRustbotWaitInCover ),
		bits_COND_CAN_RANGE_ATTACK1	|
		bits_COND_CAN_MELEE_ATTACK1	|
		bits_COND_NEW_ENEMY			|
		bits_COND_HEAR_SOUND,

		bits_SOUND_DANGER,
		"Rustbot Wait In Cover"
	},
};

//=========================================================
// Take Cover
//=========================================================
Task_t	tlRustbotTakeCover[] =
{
	{ TASK_STOP_MOVING,				(float)0							},
	{ TASK_SET_FAIL_SCHEDULE,		(float)SCHED_RUSTBOT_TAKECOVER_FAILED	},
	{ TASK_WAIT,					(float)0.2							},
	{ TASK_FIND_COVER_FROM_ENEMY,	(float)0							},
	{ TASK_RUSTBOT_SPEAK_SENTENCE,	(float)0							},
	{ TASK_RUN_PATH,				(float)0							},
	{ TASK_WAIT_FOR_MOVEMENT,		(float)0							},
	{ TASK_REMEMBER,				(float)bits_MEMORY_INCOVER			},
	{ TASK_SET_SCHEDULE,			(float)SCHED_RUSTBOT_WAIT_IN_COVER	},
};

Schedule_t	slRustbotTakeCover[] =
{
	{
		tlRustbotTakeCover,
		ARRAYSIZE ( tlRustbotTakeCover ),
		0,
		0,
		"TakeCover"
	},
};

//=========================================================
// Sweep - look around
//=========================================================
Task_t	tlRustbotSweep[] =
{
	{ TASK_TURN_LEFT,			(float)179		},
	{ TASK_WAIT,				(float)1		},
	{ TASK_TURN_LEFT,			(float)179		},
	{ TASK_WAIT,				(float)1		},
};

Schedule_t	slRustbotSweep[] =
{
	{
		tlRustbotSweep,
		ARRAYSIZE ( tlRustbotSweep ),
		bits_COND_SEE_ENEMY			|
		bits_COND_LIGHT_DAMAGE		|
		bits_COND_HEAVY_DAMAGE		|
		bits_COND_CAN_RANGE_ATTACK1	|
		bits_COND_CAN_MELEE_ATTACK1	|
		bits_COND_NEW_ENEMY			|
		bits_COND_HEAR_SOUND,

		bits_SOUND_WORLD			|
		bits_SOUND_PLAYER			|
		bits_SOUND_DANGER,
		"Rustbot Sweep"
	},
};

//=========================================================
// Range Attack - charge up, then fire the beam
//=========================================================
Task_t	tlRustbotRangeAttack[] =
{
	{ TASK_STOP_MOVING,			(float)0		},
	{ TASK_FACE_IDEAL,			(float)0		},
	{ TASK_RUSTBOT_CHARGE,		(float)0		},
	{ TASK_RUSTBOT_FIRE,		(float)0		},
	{ TASK_PLAY_SEQUENCE,		(float)ACT_DISARM },
};

Schedule_t	slRustbotRangeAttack[] =
{
	{
		tlRustbotRangeAttack,
		ARRAYSIZE ( tlRustbotRangeAttack ),
		0,
		0,
		"Rustbot Range Attack"
	},
};

LINK_ENTITY_TO_CLASS( monster_rustbot, CRustbot );

//=========================================================
// Classify - indicates this monster's place in the
// relationship table.
//=========================================================
int CRustbot::Classify( void )
{
	return CLASS_ROBOT;
}

int CRustbot::MonsterType( void )
{
	return 13;
}

void CRustbot::SetObjectCollisionBox( void )
{
	pev->absmin = pev->origin - Vector( 40, 40, 0 );
	pev->absmax = pev->origin + Vector( 40, 40, 64 );
}

TYPEDESCRIPTION	CRustbot::m_SaveData[] =
{
	DEFINE_FIELD( CRustbot, m_pBeam, FIELD_CLASSPTR ),
	DEFINE_FIELD( CRustbot, m_pNoise, FIELD_CLASSPTR ),
	DEFINE_FIELD( CRustbot, m_pSprite, FIELD_CLASSPTR ),
	DEFINE_FIELD( CRustbot, m_fFrustration, FIELD_FLOAT ),
	DEFINE_FIELD( CRustbot, m_tChangeFrustration, FIELD_TIME ),
	DEFINE_FIELD( CRustbot, m_iHeadTurn, FIELD_INTEGER ),
	DEFINE_FIELD( CRustbot, m_bBorg, FIELD_BOOLEAN ),
	DEFINE_FIELD( CRustbot, m_fIntel, FIELD_TIME ),		// sic: not a time
};

IMPLEMENT_SAVERESTORE( CRustbot, CBaseRust );

DEFINE_CUSTOM_SCHEDULES( CRustbot )
{
	slRustbotBorg,
	slRustbotFail,
	slRustbotCombatFail,
	slRustbotEstablishLineOfFire,
	slRustbotCharge,
	slRustbotCombatFace,
	slRustbotWaitInCover,
	slRustbotTakeCover,
	slRustbotSweep,
	slRustbotRangeAttack,
};

IMPLEMENT_CUSTOM_SCHEDULES( CRustbot, CBaseRust );

const char *CRustbot::pRustbotSentences[] =
{
	"RB_GREN",
	"RB_ALERT",
	"RB_COVER",
	"RB_THROW",
	"RB_CHARGE",
	"RB_TAUNT",
	"RB_CHECK",
	"RB_QUEST",
	"RB_IDLE",
	"RB_CLEAR",
	"RB_ANSWER",
	"RB_DANGER",
};

const char *CRustbot::pSounds[] =
{
	"rustbot/alert1.wav",
	"rustbot/alert2.wav",
	"rustbot/attack1.wav",
	"rustbot/attack2.wav",
	"rustbot/CAMP-BEAM-1.wav",
	"rustbot/die1.wav",
	"rustbot/die2.wav",
	"rustbot/idle1.wav",
	"rustbot/idle2.wav",
	"rustbot/pain1.wav",
	"rustbot/pain2.wav",
	"rustbot/plasma1.wav",
	"rustbot/p-up.wav",
	"rustbot/r-chat1.wav",
	"rustbot/r-chat2.wav",
	"rustbot/r-chat3.wav",
	"rustbot/r-chat4.wav",
	"rustbot/r-chat5.wav",
	"rustbot/r-chat6.wav",
	"rustbot/r-rant1.wav",
	"rustbot/r-rant2.wav",
	"rustbot/r-rant3.wav",
	"rustbot/r-rant4.wav",
	"debris/bustflesh1.wav",
	"buttons/spark6.wav",
};

void CRustbot::PrecacheSounds( void )
{
	for ( int i = 0; i < ARRAYSIZE( pSounds ); i++ )
		PRECACHE_SOUND( (char *)pSounds[i] );
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CRustbot::Precache( void )
{
	PRECACHE_MODEL( "sprites/xbeam1.spr" );
	PRECACHE_MODEL( "sprites/rustbeam_end.spr" );
	PRECACHE_MODEL( "models/rustbot.mdl" );

	CBaseRust::Precache( );
}

const char *CRustbot::SentenceGroup( int index )
{
	return pRustbotSentences[ index ];
}

//=========================================================
// PainSound
//=========================================================
void CRustbot::PainSound( void )
{
	if ( IsAlive() )
	{
		switch ( RANDOM_LONG( 0, 1 ) )
		{
		case 0:
			EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "rustbot/pain1.wav", 1, ATTN_NORM, 0, m_iVoicePitch );
			break;
		case 1:
			EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "rustbot/pain2.wav", 1, ATTN_NORM, 0, m_iVoicePitch );
			break;
		}
	}
}

//=========================================================
// DeathSound
//=========================================================
void CRustbot::DeathSound( void )
{
	switch ( RANDOM_LONG( 0, 1 ) )
	{
	case 0:
		EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, "rustbot/die1.wav", 1, ATTN_NORM, 0, m_iVoicePitch );
		break;
	case 1:
		EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, "rustbot/die2.wav", 1, ATTN_NORM, 0, m_iVoicePitch );
		break;
	}
}

//=========================================================
// IdleSound - rust bots ask and answer each other like human
// grunts
//=========================================================
void CRustbot::IdleSound( void )
{
	if ( g_fRustbotQuestion || RANDOM_LONG( 0, 1 ) )
	{
		if ( !g_fRustbotQuestion )
		{
			// ask question or make statement
			switch ( RANDOM_LONG( 0, 2 ) )
			{
			case 0: // check in
				m_iSentence = RUSTBOT_SENT_CHECK;
				g_fRustbotQuestion = 1;
				break;
			case 1: // question
				m_iSentence = RUSTBOT_SENT_QUEST;
				g_fRustbotQuestion = 2;
				break;
			case 2: // statement
				m_iSentence = RUSTBOT_SENT_IDLE;
				break;
			}
		}
		else
		{
			switch ( g_fRustbotQuestion )
			{
			case 1: // check in
				m_iSentence = RUSTBOT_SENT_CLEAR;
				break;
			case 2: // question
				m_iSentence = RUSTBOT_SENT_ANSWER;
				break;
			}
			g_fRustbotQuestion = 0;
		}
		SpeakSentence();
	}
	else
	{
		switch ( RANDOM_LONG( 0, 1 ) )
		{
		case 0:
			EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "rustbot/idle1.wav", 1, ATTN_NORM, 0, m_iVoicePitch );
			break;
		case 1:
			EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "rustbot/idle2.wav", 1, ATTN_NORM, 0, m_iVoicePitch );
			break;
		}
	}
}

//=========================================================
// AlertSound
//=========================================================
void CRustbot::AlertSound( void )
{
	switch ( RANDOM_LONG( 0, 2 ) )
	{
	case 0:
		EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "rustbot/alert1.wav", 1, ATTN_NORM, 0, m_iVoicePitch );
		break;
	case 1:
		EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "rustbot/alert2.wav", 1, ATTN_NORM, 0, m_iVoicePitch );
		break;
	case 2:
		EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "rustbot/attack2.wav", 1, ATTN_NORM, 0, m_iVoicePitch );
		break;
	}
}

//=========================================================
// PlayBeamSound - 1: the beam fires, 2: it charges up
//=========================================================
void CRustbot::PlayBeamSound( int iSound )
{
	switch ( iSound )
	{
	case 1:
		EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, "rustbot/Plasma1.wav", 0.6, ATTN_NORM, 0, m_iVoicePitch );
		break;
	case 2:
		CSoundEnt::InsertSound( bits_SOUND_DANGER, pev->origin, 1024, 0.3 );
		EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "rustbot/p-up.wav", 0.6, ATTN_NORM, 0, m_iVoicePitch );
		break;
	}
}

//=========================================================
// MeleeAttackSound
//=========================================================
void CRustbot::MeleeAttackSound( void )
{
	CSoundEnt::InsertSound( bits_SOUND_DANGER, pev->origin, 512, 0.2 );
	EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, "rustbot/attack1.wav", 0.6, ATTN_NORM, 0, m_iVoicePitch );
}

//=========================================================
// AttackHitSound - the punch hit something
//=========================================================
void CRustbot::AttackHitSound( CBaseMonster *pVictim )
{
	EMIT_SOUND_DYN( ENT(pVictim->pev), CHAN_VOICE, "lg_scorp/lg_scorp_attackb.wav", 0.7, ATTN_IDLE, 0, 40 );
}

//=========================================================
// UpdateBeam - aims the beam where the head points, and
// burns what it hits
//=========================================================
void CRustbot::UpdateBeam( void )
{
	if ( m_pBeam == NULL || m_pNoise == NULL )
		CreateBeam( 1 );

	if ( m_pBeam )
		m_pBeam->RelinkBeam();
	if ( m_pNoise )
		m_pNoise->RelinkBeam();

	int iDamage = gSkillData.rustbotDmgLong;

	Vector vecSrc, vecAngles, vecAim;
	GetAttachment( 0, vecSrc, vecAngles );

	MakeIdealYaw( m_vecEnemyLKP );

	vecAim = pev->angles;
	float flHeadTurn = m_iHeadTurn;
	vecAim.y += flHeadTurn;
	g_engfuncs.pfnAngleVectors( vecAim, vecAim, NULL, NULL );
	vecAim.Normalize();		// sic: the result is thrown away
	Vector vecDir = ShootAtEnemy( vecSrc );
	vecAim.z = vecDir.z;

	TraceResult tr;
	Vector vecEnd = vecSrc + vecAim * 2048;
	UTIL_TraceLine( vecSrc, vecEnd, dont_ignore_monsters, ENT(pev), &tr );

	m_pBeam->SetStartPos( tr.vecEndPos );
	m_pNoise->SetStartPos( tr.vecEndPos );
	m_pBeam->SetEndPos( vecSrc );
	m_pNoise->SetEndPos( vecSrc );

	UTIL_SetOrigin( m_pSprite->pev, tr.vecEndPos );
	m_pSprite->pev->frame += gpGlobals->frametime * 8;
	if ( m_pSprite->Frames() < m_pSprite->pev->frame )
		m_pSprite->pev->frame = 0;

	if ( !tr.fAllSolid )
	{
		CBaseEntity *pEntity = CBaseEntity::Instance( tr.pHit );
		if ( pEntity != NULL )
		{
			ClearMultiDamage( );
			if ( pEntity->pev->takedamage )
			{
				m_fFrustration -= 0.1;
				pEntity->TraceAttack( pev, iDamage, vecAim, &tr, DMG_ENERGYBEAM );
				if ( pEntity->IsPlayer() )
					EMIT_SOUND_DYN( ENT(pEntity->pev), CHAN_VOICE, "buttons/spark6.wav", 0.8, ATTN_NORM, 0, 120 );
			}
			ApplyMultiDamage( pev, pev );
		}
	}
}

//=========================================================
// CreateBeam - two beams from the head to a glowing end
//=========================================================
void CRustbot::CreateBeam( float flUnused )
{
	DestroyBeam( );

	m_pSprite = CSprite::SpriteCreate( "sprites/rustbeam_end.spr", pev->origin, FALSE );
	m_pSprite->SetScale( 0.75 );
	m_pSprite->SetTransparency( kRenderGlow, 255, 255, 255, 200, kRenderFxNoDissipation );
	m_pSprite->pev->spawnflags |= SF_SPRITE_TEMPORARY;

	m_pBeam = CBeam::BeamCreate( "sprites/xbeam1.spr", 40 );
	m_pBeam->PointsInit( pev->origin, pev->origin );
	m_pBeam->SetFlags( BEAM_FSINE );
	m_pBeam->SetScrollRate( 5 );
	m_pBeam->SetNoise( 10 );
	m_pBeam->SetBrightness( 255 );
	m_pBeam->SetWidth( 10 );
	m_pBeam->SetColor( 80, 120, 255 );
	m_pBeam->SetEndAttachment( 0 );
	m_pBeam->pev->spawnflags |= SF_BEAM_TEMPORARY;

	m_pNoise = CBeam::BeamCreate( "sprites/xbeam1.spr", 55 );
	m_pNoise->PointsInit( pev->origin, pev->origin );
	m_pNoise->SetColor( 80, 120, 255 );
	m_pNoise->SetScrollRate( 5 );
	m_pNoise->SetNoise( 10 );
	m_pNoise->SetBrightness( 255 );
	m_pNoise->SetWidth( 10 );
	m_pNoise->SetEndAttachment( 0 );
	m_pNoise->pev->spawnflags |= SF_BEAM_TEMPORARY;

	UpdateBeam( );
}

//=========================================================
// DestroyBeam
//=========================================================
void CRustbot::DestroyBeam( void )
{
	if ( m_pBeam )
	{
		UTIL_Remove( m_pBeam );
		m_pBeam = NULL;
	}
	if ( m_pNoise )
	{
		UTIL_Remove( m_pNoise );
		m_pNoise = NULL;
	}
	if ( m_pSprite )
	{
		m_pSprite->Expand( 10, 500 );
		m_pSprite = NULL;
	}

	EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, "common/null.wav", 0, ATTN_NORM, 0, PITCH_NORM );
}

//=========================================================
// PrescheduleThink - keeps the squad's enemy sighting time;
// a frustrated rust bot walks faster and throws sparks
//=========================================================
void CRustbot::PrescheduleThink( void )
{
	if ( InSquad() && m_hEnemy != NULL )
	{
		if ( HasConditions( bits_COND_SEE_ENEMY ) )
		{
			// update the squad's last enemy sighting time.
			MySquadLeader()->m_flLastEnemySightTime = gpGlobals->time;
			m_flLastEnemySightTime = gpGlobals->time;
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
	else if ( m_hEnemy != NULL && HasConditions( bits_COND_SEE_ENEMY ) )
	{
		m_flLastEnemySightTime = gpGlobals->time;
	}

	if ( m_fFrustration < 0 )
		m_fFrustration = 0;
	else if ( m_fFrustration > 10 )
		m_fFrustration = 10;

	if ( m_fFrustration >= 5 )
	{
		float flRate = m_fFrustration / 10;
		if ( flRate > 0.7 )
			flRate = 0.7;
		pev->framerate = flRate + 1.2;

		int iChance = rand() % 40;
		if ( iChance < 2 )
		{
			Sparks( 1, 0 );
		}
		else if ( iChance < 1 )	// sic: never true
		{
			int iSparks = rand() % 5;
			for ( int i = 0; i < iSparks; i++ )
				Sparks( iSparks, 0 );
		}
	}
}

//=========================================================
// CheckMeleeAttack1 - close, facing the enemy, and room to
// swing
//=========================================================
BOOL CRustbot::CheckMeleeAttack1( float flDot, float flDist )
{
	if ( flDist < 85 && flDot > 0.7 )
	{
		TraceResult tr;

		UTIL_TraceLine( pev->origin, pev->origin + Vector( 0, 0, 70 ), ignore_monsters, ENT(pev), &tr );

		if ( tr.flFraction >= 1.0 )
			return TRUE;
	}

	return FALSE;
}

//=========================================================
// CheckRangeAttack1 - the head points at the enemy, or the
// rust bot faces him, and nothing is in the way
//=========================================================
BOOL CRustbot::CheckRangeAttack1( float flDot, float flDist )
{
	if ( ( m_fOnTarget || flDot > 0.95 ) && flDist > 480 )
	{
		TraceResult tr;

		UTIL_TraceLine( pev->origin, m_hEnemy->BodyTarget( pev->origin ), ignore_monsters, ignore_glass, ENT(pev), &tr );

		if ( tr.flFraction == 1.0 )
			return TRUE;
	}

	return FALSE;
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CRustbot::SetYawSpeed( void )
{
	int ys;

	switch ( m_Activity )
	{
	case ACT_IDLE:
	case ACT_RUN:
		ys = 120;
		break;
	case ACT_WALK:
	case ACT_TURN_LEFT:
	case ACT_TURN_RIGHT:
		ys = 150;
		break;
	case ACT_MELEE_ATTACK1:
		ys = 100;
		break;
	default:
		ys = 100;
		break;
	}

	pev->yaw_speed = ys;
}

//=========================================================
// BeamOn - firing frustrates
//=========================================================
void CRustbot::BeamOn( void )
{
	m_fFrustration += 2;
	float flWidth = RANDOM_LONG( 5, 6 );
	CreateBeam( flWidth );
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CRustbot::HandleAnimEvent( MonsterEvent_t *pEvent )
{
	Vector vecShootDir;
	Vector vecShootOrigin;

	switch ( pEvent->event )
	{
	case RUSTBOT_AE_PUNCH:
		{
			int iSparks = rand() % 3 + 1;
			Sparks( iSparks, 6 );
			MeleeAttackSound();

			CBaseEntity *pHurt = CheckTraceHullAttack( 128, gSkillData.rustbotDmgMelee, DMG_SLASH );

			if ( pHurt )
			{
				pHurt->pev->punchangle.z = rand() % 30 - 15;
				pHurt->pev->punchangle.x = rand() % 30 - 15;
				pHurt->pev->velocity = pHurt->pev->velocity - gpGlobals->v_forward * 30;
				pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_up * 30;

				CBaseMonster *pMonster = pHurt->MyMonsterPointer();
				if ( pMonster )
				{
					TraceResult tr;
					UTIL_TraceLine( BodyTarget( g_vecZero ), pMonster->BodyTarget( g_vecZero ), dont_ignore_monsters, ENT(pev), &tr );
					pMonster->TraceBleed( gSkillData.rustbotDmgMelee, ( pMonster->BodyTarget( g_vecZero ) - BodyTarget( g_vecZero ) ).Normalize(), &tr, DMG_SLASH, tr.vecEndPos );

					if ( pMonster->IsPlayer() )
						AttackHitSound( pMonster );
				}

				m_fFrustration -= -2 - rand() % 2;
			}
			else
			{
				m_fFrustration += rand() % 3;
			}
		}
		break;

	case RUSTBOT_AE_BEAM_OFF:
		DestroyBeam();
		break;

	case RUSTBOT_AE_BEAM_ON:
		{
			int iSparks = rand() % 4;
			Sparks( iSparks, 5 );
		}
		DestroyBeam();
		BeamOn();
		CSoundEnt::InsertSound( bits_SOUND_COMBAT, pev->origin, 384, 0.3 );
		break;

	default:
		CBaseRust::HandleAnimEvent( pEvent );
		break;
	}
}

//=========================================================
// Spawn
//=========================================================
void CRustbot::Spawn( void )
{
	Precache( );
	DestroyBeam( );

	SET_MODEL( ENT(pev), "models/rustbot.mdl" );
	UTIL_SetSize( pev, Vector( -32, -32, 0 ), Vector( 32, 32, 64 ) );

	pev->movetype = MOVETYPE_STEP;
	pev->solid = SOLID_SLIDEBOX;
	pev->health = UTIL_RandomizeHealth( gSkillData.rustbotHealth );
	m_iSentence = RUST_SENT_NONE;
	m_tChangeFrustration = gpGlobals->time;
	CTalkMonster::g_talkWaitTime = 0;

	m_pBeam = NULL;
	m_pNoise = NULL;

	m_fAgility = RANDOM_FLOAT( 0, 1 );
	m_fIntel = 1 - m_fAgility;

	if ( RANDOM_LONG( 0, 1 ) )
		m_iVoicePitch = RANDOM_LONG( 0, 7 ) + 109;
	else
		m_iVoicePitch = 100;

	m_fFrustration = 0;
	m_iHeadTurn = 0;
	SetBoneController( 2, 0 );

	if ( FBitSet( pev->spawnflags, SF_RUSTBOT_BORG ) )
	{
		m_bBorg = TRUE;
		ALERT( at_console, "borgRustbot\n" );
	}

	RustInit( );
	MonsterInit( );

	pev->view_ofs = Vector( 0, 0, 40 );
}

//=========================================================
// StartTask
//=========================================================
void CRustbot::StartTask( Task_t *pTask )
{
	m_iTaskStatus = TASKSTATUS_RUNNING;

	switch ( pTask->iTask )
	{
	case TASK_RUSTBOT_FIRE:
		PlayBeamSound( 1 );
		m_IdealActivity = ACT_RANGE_ATTACK1;
		break;

	case TASK_RUSTBOT_CHARGE:
		m_IdealActivity = ACT_ARM;
		PlayBeamSound( 2 );
		break;

	case TASK_RUSTBOT_SPEAK_SENTENCE:
		SpeakSentence();
		TaskComplete();
		break;

	default:
		CBaseRust::StartTask( pTask );
		break;
	}
}

//=========================================================
// RunTask
//=========================================================
void CRustbot::RunTask( Task_t *pTask )
{
	switch ( pTask->iTask )
	{
	case TASK_RUSTBOT_FINISH_ANIM:
		if ( m_fSequenceFinished )
			TaskComplete();
		break;

	case TASK_RUSTBOT_FIRE:
		if ( m_fSequenceFinished )
			TaskComplete();
		break;

	case TASK_RUSTBOT_CHARGE:
		if ( m_fSequenceFinished )
			TaskComplete();
		break;

	default:
		CBaseRust::RunTask( pTask );
		break;
	}
}

//=========================================================
// RunAI - animation speeds, the beam, calming down, and
// turning the head toward the enemy
//=========================================================
void CRustbot::RunAI( void )
{
	CBaseRust::RunAI();

	if ( IsAlive() )
	{
		m_fOnTarget = FALSE;

		switch ( m_Activity )
		{
		case ACT_RANGE_ATTACK1:
			pev->framerate = 2.5;
			UpdateBeam();
			break;
		case ACT_MELEE_ATTACK1:
			pev->framerate = m_fAgility / 2 + 1.3;
			break;
		case ACT_ARM:
			pev->framerate = 1.6;
			break;
		default:
			pev->framerate = 1.0;
			break;
		}

		if ( m_hEnemy == NULL && m_tChangeFrustration < gpGlobals->time )
		{
			m_fFrustration -= 1;
			m_tChangeFrustration = gpGlobals->time + 2;
		}

		if ( m_MonsterState == MONSTERSTATE_COMBAT )
			AimHead( m_vecEnemyLKP );
	}
}

//=========================================================
// SetActivity - limps when hurt
//=========================================================
void CRustbot::SetActivity( Activity NewActivity )
{
	int iSequence;

	if ( NewActivity >= ACT_WALK && NewActivity <= ACT_RUN && pev->health <= 50 )
		iSequence = LookupActivity( ACT_WALK_HURT );
	else
		iSequence = LookupActivity( NewActivity );

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
// GetSchedule - Decides which type of schedule best suits
// the monster's current state and conditions. Then calls
// monster's member function to get a pointer to a schedule
// of the proper type.
//=========================================================
Schedule_t *CRustbot::GetSchedule( void )
{
	// clear old sentence
	m_iSentence = RUST_SENT_NONE;

	if ( m_bBorg == TRUE )
	{
		if ( !HasConditions( bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE ) )
			return GetScheduleOfType( SCHED_RUSTBOT_BORG );

		m_bBorg = FALSE;
	}

	// rust bots place HIGH priority on running away from danger sounds.
	if ( HasConditions( bits_COND_HEAR_SOUND ) )
	{
		CSound *pSound;
		pSound = PBestSound();

		ASSERT( pSound != NULL );
		if ( pSound )
		{
			if ( !HasConditions( bits_COND_SEE_ENEMY ) && ( pSound->m_iType & ( bits_SOUND_PLAYER | bits_SOUND_COMBAT ) ) )
				MakeIdealYaw( pSound->m_vecOrigin );

			if ( pSound->m_iType & bits_SOUND_DANGER )
			{
				m_iSentence = RUSTBOT_SENT_DANGER;
				SpeakSentence();
				return GetScheduleOfType( SCHED_TAKE_COVER_FROM_BEST_SOUND );
			}
		}
	}

	switch ( m_MonsterState )
	{
	case MONSTERSTATE_COMBAT:
		{
// dead enemy
			if ( HasConditions( bits_COND_ENEMY_DEAD ) )
			{
				// call base class, all code to handle dead enemies is centralized there.
				return CBaseRust::GetSchedule();
			}

// new enemy
			if ( HasConditions( bits_COND_NEW_ENEMY ) )
			{
				if ( HasConditions( bits_COND_CAN_MELEE_ATTACK1 ) )
					return GetScheduleOfType( SCHED_MELEE_ATTACK1 );

				if ( RANDOM_LONG( 0, 1 ) && m_hEnemy != NULL &&
					m_hEnemy->Classify() != CLASS_PLAYER_ALLY &&
					m_hEnemy->Classify() != CLASS_HUMAN_PASSIVE &&
					m_hEnemy->Classify() != CLASS_MACHINE )
				{
					m_iSentence = RUSTBOT_SENT_DANGER;
					SpeakSentence();
				}

				if ( HasConditions( bits_COND_CAN_MELEE_ATTACK1 ) )
					return GetScheduleOfType( SCHED_MELEE_ATTACK1 );

				return GetScheduleOfType( SCHED_RUSTBOT_ESTABLISH_LINE_OF_FIRE );
			}
// can punch
			else if ( HasConditions( bits_COND_CAN_MELEE_ATTACK1 ) )
			{
				return GetScheduleOfType( SCHED_MELEE_ATTACK1 );
			}
// damaged a lot
			else if ( HasConditions( bits_COND_HEAVY_DAMAGE ) )
			{
				m_iSentence = RUSTBOT_SENT_COVER;
				SpeakSentence();
				return GetScheduleOfType( SCHED_TAKE_COVER_FROM_ENEMY );
			}
// can fire the beam
			else if ( HasConditions( bits_COND_CAN_RANGE_ATTACK1 ) )
			{
				return GetScheduleOfType( SCHED_RANGE_ATTACK1 );
			}
// can't see enemy
			else if ( HasConditions( bits_COND_ENEMY_OCCLUDED ) )
			{
				if ( RANDOM_LONG( 0, 1 ) )
				{
					m_iSentence = RUSTBOT_SENT_CHARGE;
					SpeakSentence();
				}
				return GetScheduleOfType( SCHED_RUSTBOT_CHARGE );
			}
// sees the enemy, can't attack
			else if ( HasConditions( bits_COND_SEE_ENEMY ) )
			{
				return GetScheduleOfType( SCHED_RUSTBOT_ESTABLISH_LINE_OF_FIRE );
			}
		}
	}

	// no special cases here, call the base class
	return CBaseRust::GetSchedule();
}

//=========================================================
// GetScheduleOfType
//=========================================================
Schedule_t *CRustbot::GetScheduleOfType( int Type )
{
	switch ( Type )
	{
	case SCHED_RUSTBOT_BORG:
		return &slRustbotBorg[ 0 ];

	case SCHED_RUSTBOT_ELOF_FAIL:
		if ( gpGlobals->time - m_flLastEnemySightTime > 10 )
			return GetScheduleOfType( SCHED_FAIL );
		return GetScheduleOfType( SCHED_RANGE_ATTACK1 );

	case SCHED_RUSTBOT_ESTABLISH_LINE_OF_FIRE:
		return &slRustbotEstablishLineOfFire[ 0 ];

	case SCHED_RUSTBOT_CHARGE:
		return &slRustbotCharge[ 0 ];

	case SCHED_RANGE_ATTACK1:
		return &slRustbotRangeAttack[ 0 ];

	case SCHED_COMBAT_FACE:
		return &slRustbotCombatFace[ 0 ];

	case SCHED_RUSTBOT_WAIT_IN_COVER:
		return &slRustbotWaitInCover[ 0 ];

	case SCHED_RUSTBOT_SWEEP:
		return &slRustbotSweep[ 0 ];

	case SCHED_TAKE_COVER_FROM_ENEMY:
		m_iSentence = RUSTBOT_SENT_COVER;
		SpeakSentence();
		return &slRustbotTakeCover[ 0 ];

	case SCHED_RUSTBOT_TAKECOVER_FAILED:
		if ( HasConditions( bits_COND_CAN_RANGE_ATTACK1 ) )
			return GetScheduleOfType( SCHED_RANGE_ATTACK1 );
		return GetScheduleOfType( SCHED_FAIL );

	case SCHED_FAIL:
		if ( m_hEnemy != NULL )
			return &slRustbotCombatFail[ 0 ];
		return &slRustbotFail[ 0 ];
	}

	return CBaseRust::GetScheduleOfType( Type );
}

//=========================================================
// TakeDamage - beams do half damage
//=========================================================
int CRustbot::TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	if ( bitsDamageType == DMG_ENERGYBEAM )
		flDamage /= 2;

	return CBaseRust::TakeDamage( pevInflictor, pevAttacker, flDamage, bitsDamageType );
}

//=========================================================
// Killed - the beam goes with the rust bot
//=========================================================
void CRustbot::Killed( entvars_t *pevAttacker, int iGib )
{
	EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, "common/null.wav", 0, ATTN_NORM, 0, PITCH_NORM );
	DestroyBeam( );

	CBaseRust::Killed( pevAttacker, iGib );
}

void CRustbot::PreRemove( void )
{
	EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, "common/null.wav", 0, ATTN_NORM, 0, PITCH_NORM );
	DestroyBeam( );

	CBaseRust::PreRemove();
}

//=========================================================
// CenterHead - turns a head turned right back by 5 degrees;
// nothing calls it
//=========================================================
void CRustbot::CenterHead( void )
{
	if ( IsAlive() )
	{
		int iHeadTurn = m_iHeadTurn;

		if ( iHeadTurn > 0 && iHeadTurn - 5 > 0 )
			iHeadTurn -= 5;

		m_iHeadTurn = iHeadTurn;
		SetBoneController( 2, iHeadTurn );
	}
}

//=========================================================
// AimHead - turns the head toward a point, slower while
// attacking on the lower skill levels
//=========================================================
void CRustbot::AimHead( const Vector &vecTarget )
{
	if ( !IsAlive() )
		return;

	float flMaxTurn;

	switch ( m_Activity )
	{
	case ACT_RANGE_ATTACK1:
	case ACT_ARM:
	case ACT_DISARM:
		switch ( g_iSkillLevel )
		{
		case SKILL_MEDIUM:
			flMaxTurn = 7;
			break;
		case SKILL_HARD:
			flMaxTurn = 10;
			break;
		default:
			flMaxTurn = 5;
			break;
		}
		break;
	default:
		flMaxTurn = 10;
		break;
	}

	// sic: the head turn is read through piHeadTurn and the yaw sum from
	// the member, so VC6 loads the member twice (mov into a register,
	// fild from it) as retail does; read the same way twice it reuses one load
	int *piHeadTurn = &m_iHeadTurn;
	int iHeadTurn = *piHeadTurn;
	Vector vecOrigin;
	vecOrigin = pev->origin;

	Vector vecDir = vecTarget - vecOrigin;
	float flYaw = m_iHeadTurn + pev->angles.y;
	float flTarget = UTIL_VecToYaw( vecDir );
	float flDelta = UTIL_AngleDiff( flTarget, flYaw );

	if ( flDelta > 180 )
		flDelta -= 360;
	if ( flDelta < -180 )
		flDelta += 360;

	if ( flDelta >= flMaxTurn )
		flDelta = flMaxTurn;
	else if ( flDelta <= -flMaxTurn )
		flDelta = -flMaxTurn;
	else
		m_fOnTarget = TRUE;

	iHeadTurn += (int)flDelta;

	if ( iHeadTurn > 75 )
	{
		m_fOnTarget = FALSE;
		iHeadTurn = 75;
	}
	else if ( iHeadTurn < -75 )
	{
		m_fOnTarget = FALSE;
		iHeadTurn = -75;
	}

	m_iHeadTurn = iHeadTurn;
	SetBoneController( 2, iHeadTurn );
}

//=========================================================
// Sparks - throws spark showers around the rust bot, if it
// is frustrated enough; no count means up to three
//=========================================================
void CRustbot::Sparks( int iCount, int iFrustration )
{
	if ( !iFrustration || iFrustration <= m_fFrustration )
	{
		if ( !iCount )
			iCount = rand() % 4;

		for ( int i = 0; i < iCount; i++ )
		{
			Vector vecSrc;
			vecSrc.x = pev->origin.x - 45 + rand() % 90;
			vecSrc.y = pev->origin.y - 45 + rand() % 90;
			vecSrc.z = rand() % 30 + pev->origin.z + 24;
			CBaseEntity::Create( "spark_shower", vecSrc, pev->v_angle, NULL );
		}
	}
}

//=========================================================
// monster_rustbot_friendly - on the player's side
//=========================================================
class CRustbotFriendly : public CRustbot
{
public:
	int  Classify( void );
};

LINK_ENTITY_TO_CLASS( monster_rustbot_friendly, CRustbotFriendly );

int CRustbotFriendly::Classify( void )
{
	return CLASS_PLAYER_ALLY;
}

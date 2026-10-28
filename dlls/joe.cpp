/***
*
*	Copyright (c) 1999, 2000 Valve LLC. All rights reserved.
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
// monster_joe: T2DEF's security guard, made from barney.cpp
// (file name ours)
//=========================================================

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"talkmonster.h"
#include	"schedule.h"
#include	"defaultai.h"
#include	"scripted.h"
#include	"weapons.h"
#include	"soundent.h"

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
// first flag is barney dying for scripted sequences?
#define		JOE_AE_DRAW		( 2 )
#define		JOE_AE_SHOOT		( 3 )
#define		JOE_AE_HOLSTER	( 4 )
#define		JOE_AE_RELOAD		( 5 )	// name ours
#define		JOE_AE_ZAPPER_DRAW	( 6 )	// name ours ("firestun")
#define		JOE_AE_ZAPPER_HOLSTER	( 7 )	// name ours

#define	JOE_BODY_GUNHOLSTERED	0
#define	JOE_BODY_GUNDRAWN		1
#define JOE_BODY_GUNGONE			2
#define JOE_BODY_ZAPPER			3	// name ours ("joeholdzapper")

#define SF_JOE_HOSTILE			64	// name ours

enum
{
	SCHED_JOE_HIDE_RELOAD = LAST_COMMON_SCHEDULE + 1,	// name ours; sic: the same number as SCHED_CANT_FOLLOW
};

class CJoe : public CTalkMonster
{
public:
	void Spawn( void );
	void Precache( void );
	void SetYawSpeed( void );
	int  ISoundMask( void );
	void JoeFirePistol( void );
	void AlertSound( void );
	int  Classify ( void );
	void HandleAnimEvent( MonsterEvent_t *pEvent );
	void CheckAmmo ( void );

	void RunTask( Task_t *pTask );
	void StartTask( Task_t *pTask );
	virtual int	ObjectCaps( void ) { return CTalkMonster :: ObjectCaps() | FCAP_IMPULSE_USE; }
	int TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType);
	BOOL CheckRangeAttack1 ( float flDot, float flDist );

	void DeclineFollowing( void );

	// Override these to set behavior
	Schedule_t *GetScheduleOfType ( int Type );
	Schedule_t *GetSchedule ( void );
	MONSTERSTATE GetIdealState ( void );

	void DeathSound( void );
	void PainSound( void );

	void TalkInit( void );

	void TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType);
	void Killed( entvars_t *pevAttacker, int iGib );

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	BOOL	m_fGunDrawn;
	float	m_painTime;
	float	m_checkAttackTime;
	BOOL	m_lastAttackCheck;

	// UNDONE: What is this for?  It isn't used?
	float	m_flPlayerDamage;// how much pain has the player inflicted on me?

	CUSTOM_SCHEDULES;
};

LINK_ENTITY_TO_CLASS( monster_joe, CJoe );

TYPEDESCRIPTION	CJoe::m_SaveData[] =
{
	DEFINE_FIELD( CJoe, m_fGunDrawn, FIELD_BOOLEAN ),
	DEFINE_FIELD( CJoe, m_painTime, FIELD_TIME ),
	DEFINE_FIELD( CJoe, m_checkAttackTime, FIELD_TIME ),
	DEFINE_FIELD( CJoe, m_lastAttackCheck, FIELD_BOOLEAN ),
	DEFINE_FIELD( CJoe, m_flPlayerDamage, FIELD_FLOAT ),
};

IMPLEMENT_SAVERESTORE( CJoe, CTalkMonster );

//=========================================================
// AI Schedules Specific to this monster
// (task list and schedule names ours)
//=========================================================
Task_t	tlJoeRangeAttack1[] =
{
	{ TASK_STOP_MOVING,			0				},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
};

Schedule_t	slJoeRangeAttack1[] =
{
	{
		tlJoeRangeAttack1,
		ARRAYSIZE ( tlJoeRangeAttack1 ),
		bits_COND_NEW_ENEMY			|
		bits_COND_ENEMY_DEAD		|
		bits_COND_LIGHT_DAMAGE		|
		bits_COND_HEAVY_DAMAGE		|
		bits_COND_ENEMY_OCCLUDED	|
		bits_COND_NO_AMMO_LOADED	|
		bits_COND_HEAR_SOUND,

		bits_SOUND_DANGER,
		"JoeRange Attack1"
	},
};

Task_t	tlJoFollow[] =
{
	{ TASK_MOVE_TO_TARGET_RANGE,(float)128		},	// Move within 128 of target ent (client)
	{ TASK_SET_SCHEDULE,		(float)SCHED_TARGET_FACE },
};

Schedule_t	slJoFollow[] =
{
	{
		tlJoFollow,
		ARRAYSIZE ( tlJoFollow ),
		bits_COND_NEW_ENEMY		|
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE	|
		bits_COND_HEAR_SOUND |
		bits_COND_PROVOKED,
		bits_SOUND_DANGER,
		"Follow"
	},
};

//=========================================================
// JoeDraw- much better looking draw schedule for when
// joe knows who he's gonna attack.
//=========================================================
Task_t	tlJoeEnemyDraw[] =
{
	{ TASK_STOP_MOVING,					0				},
	{ TASK_FACE_ENEMY,					0				},
	{ TASK_PLAY_SEQUENCE_FACE_ENEMY,	(float) ACT_ARM },
};

Schedule_t slJoeEnemyDraw[] =
{
	{
		tlJoeEnemyDraw,
		ARRAYSIZE ( tlJoeEnemyDraw ),
		0,
		0,
		"Joe Enemy Draw"
	}
};

Task_t	tlJoFaceTarget[] =
{
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE },
	{ TASK_FACE_TARGET,			(float)0		},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE },
	{ TASK_SET_SCHEDULE,		(float)SCHED_TARGET_CHASE },
};

Schedule_t	slJoFaceTarget[] =
{
	{
		tlJoFaceTarget,
		ARRAYSIZE ( tlJoFaceTarget ),
		bits_COND_CLIENT_PUSH	|
		bits_COND_NEW_ENEMY		|
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE	|
		bits_COND_HEAR_SOUND |
		bits_COND_PROVOKED,
		bits_SOUND_DANGER,
		"FaceTarget"
	},
};


Task_t	tlIdleJoStand[] =
{
	{ TASK_STOP_MOVING,			0				},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE },
	{ TASK_WAIT,				(float)2		}, // repick IDLESTAND every two seconds.
	{ TASK_TLK_HEADRESET,		(float)0		}, // reset head position
};

Schedule_t	slIdleJoStand[] =
{
	{
		tlIdleJoStand,
		ARRAYSIZE ( tlIdleJoStand ),
		bits_COND_NEW_ENEMY		|
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE	|
		bits_COND_HEAR_SOUND	|
		bits_COND_SMELL			|
		bits_COND_PROVOKED,

		bits_SOUND_COMBAT		|// sound flags - change these, and you'll break the talking code.
		//bits_SOUND_PLAYER		|
		//bits_SOUND_WORLD		|

		bits_SOUND_DANGER		|
		bits_SOUND_MEAT			|// scents
		bits_SOUND_CARCASS		|
		bits_SOUND_GARBAGE,
		"IdleStand"
	},
};

//=========================================================
// hide from the loudest sound source (to run from grenade)
//=========================================================
Task_t	tlJoeHideReload[] =
{
	{ TASK_STOP_MOVING,				(float)0					},
	{ TASK_SET_FAIL_SCHEDULE,		(float)SCHED_RELOAD			},
	{ TASK_FIND_COVER_FROM_ENEMY,	(float)0					},
	{ TASK_RUN_PATH,				(float)0					},
	{ TASK_WAIT_FOR_MOVEMENT,		(float)0					},
	{ TASK_REMEMBER,				(float)bits_MEMORY_INCOVER	},
	{ TASK_FACE_ENEMY,				(float)0					},
	{ TASK_PLAY_SEQUENCE,			(float)ACT_RELOAD			},
};

Schedule_t slJoeHideReload[] =
{
	{
		tlJoeHideReload,
		ARRAYSIZE ( tlJoeHideReload ),
		bits_COND_HEAVY_DAMAGE	|
		bits_COND_HEAR_SOUND,

		bits_SOUND_DANGER,
		"JoeHideReload"
	}
};

DEFINE_CUSTOM_SCHEDULES( CJoe )
{
	slJoeRangeAttack1,
	slJoFollow,
	slJoeEnemyDraw,
	slJoFaceTarget,
	slIdleJoStand,
	slJoeHideReload,
};


IMPLEMENT_CUSTOM_SCHEDULES( CJoe, CTalkMonster );

void CJoe :: StartTask( Task_t *pTask )
{
	CTalkMonster::StartTask( pTask );
}

void CJoe :: RunTask( Task_t *pTask )
{
	switch ( pTask->iTask )
	{
	case TASK_RANGE_ATTACK1:
		if (m_hEnemy != NULL && (m_hEnemy->IsPlayer()))
		{
			pev->framerate = 1.5;
		}
		CTalkMonster::RunTask( pTask );
		break;
	default:
		CTalkMonster::RunTask( pTask );
		break;
	}
}




//=========================================================
// ISoundMask - returns a bit mask indicating which types
// of sounds this monster regards.
//=========================================================
int CJoe :: ISoundMask ( void)
{
	return	bits_SOUND_WORLD	|
			bits_SOUND_COMBAT	|
			bits_SOUND_CARCASS	|
			bits_SOUND_MEAT		|
			bits_SOUND_GARBAGE	|
			bits_SOUND_DANGER	|
			bits_SOUND_PLAYER;
}

//=========================================================
// Classify - indicates this monster's place in the
// relationship table.
//=========================================================
int	CJoe :: Classify ( void )
{
	if (pev->spawnflags & SF_JOE_HOSTILE)
		return m_iClass?m_iClass:CLASS_ALIEN_MONSTER;
	else
		return m_iClass?m_iClass:CLASS_PLAYER_ALLY;
}

//=========================================================
// ALertSound - joe says "Freeze!"
//=========================================================
void CJoe :: AlertSound( void )
{
	if ( m_hEnemy != NULL )
	{
		if ( FOkToSpeak() )
		{
			if (m_iszSpeakAs)
			{
				char szBuf[32];
				strcpy(szBuf,STRING(m_iszSpeakAs));
				strcat(szBuf,"_ATTACK");
				PlaySentence( szBuf, RANDOM_FLOAT(2.8, 3.2), VOL_NORM, ATTN_IDLE );
			}
			else
			{
				PlaySentence( "JO_ATTACK", RANDOM_FLOAT(2.8, 3.2), VOL_NORM, ATTN_IDLE );
			}
		}
	}

}
//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CJoe :: SetYawSpeed ( void )
{
	int ys;

	ys = 0;

	switch ( m_Activity )
	{
	case ACT_IDLE:
		ys = 70;
		break;
	case ACT_WALK:
		ys = 70;
		break;
	case ACT_RUN:
		ys = 90;
		break;
	default:
		ys = 70;
		break;
	}

	pev->yaw_speed = ys;
}


//=========================================================
// CheckRangeAttack1
//=========================================================
BOOL CJoe :: CheckRangeAttack1 ( float flDot, float flDist )
{
	if ( flDist <= 1024 && flDot >= 0.5 )
	{
		if ( gpGlobals->time > m_checkAttackTime )
		{
			TraceResult tr;

			Vector shootOrigin = pev->origin + Vector( 0, 0, 55 );
			CBaseEntity *pEnemy = m_hEnemy;
			Vector shootTarget = ( (pEnemy->BodyTarget( shootOrigin ) - pEnemy->pev->origin) + m_vecEnemyLKP );
			UTIL_TraceLine( shootOrigin, shootTarget, dont_ignore_monsters, ENT(pev), &tr );
			m_checkAttackTime = gpGlobals->time + 1;
			if ( tr.flFraction == 1.0 || (tr.pHit != NULL && CBaseEntity::Instance(tr.pHit) == pEnemy) )
				m_lastAttackCheck = TRUE;
			else
				m_lastAttackCheck = FALSE;
			m_checkAttackTime = gpGlobals->time + 1.5;
		}
		return m_lastAttackCheck;
	}
	return FALSE;
}


//=========================================================
// JoeFirePistol - shoots one round from the pistol at
// the enemy joe is facing.
//=========================================================
void CJoe :: JoeFirePistol ( void )
{
	Vector vecShootOrigin;

	UTIL_MakeVectors(pev->angles);
	vecShootOrigin = pev->origin + Vector( 0, 0, 55 );
	Vector vecShootDir = ShootAtEnemy( vecShootOrigin );

	Vector angDir = UTIL_VecToAngles( vecShootDir );
	SetBlending( 0, angDir.x );
	pev->effects = EF_MUZZLEFLASH;

	FireBullets(1, vecShootOrigin, vecShootDir, VECTOR_CONE_2DEGREES, 1024, BULLET_MONSTER_9MM );

	int pitchShift = RANDOM_LONG( 0, 20 );

	// Only shift about half the time
	if ( pitchShift > 10 )
		pitchShift = 0;
	else
		pitchShift -= 5;
	if ( RANDOM_LONG( 0, 1 ) == 0 )
		EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, "weapons/357_shot1.wav", 1, ATTN_NORM, 0, 100 + pitchShift );
	else
		EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, "weapons/357_shot2.wav", 1, ATTN_NORM, 0, 100 + pitchShift );

	CSoundEnt::InsertSound ( bits_SOUND_COMBAT, pev->origin, 384, 0.3 );

	m_cAmmoLoaded--;// take away a bullet!
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//
// Returns number of events handled, 0 if none.
//=========================================================
void CJoe :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	switch( pEvent->event )
	{
	case JOE_AE_SHOOT:
		JoeFirePistol();
		break;

	case JOE_AE_DRAW:
		// joe's bodygroup switches here so he can pull gun from holster
		pev->body = JOE_BODY_GUNDRAWN;
		m_fGunDrawn = TRUE;
		break;

	case JOE_AE_HOLSTER:
		// change bodygroup to replace gun in holster
		pev->body = JOE_BODY_GUNHOLSTERED;
		m_fGunDrawn = FALSE;
		break;

	case JOE_AE_RELOAD:
		EMIT_SOUND( ENT(pev), CHAN_WEAPON, "weapons/357_reload1.wav", 1, ATTN_NORM );
		m_cAmmoLoaded = 6;
		ClearConditions(bits_COND_NO_AMMO_LOADED);
		break;

	case JOE_AE_ZAPPER_DRAW:
		pev->body = JOE_BODY_ZAPPER;
		m_fGunDrawn = FALSE;
		break;

	case JOE_AE_ZAPPER_HOLSTER:
		pev->body = JOE_BODY_GUNHOLSTERED;
		m_fGunDrawn = FALSE;
		break;

	default:
		CTalkMonster::HandleAnimEvent( pEvent );
	}
}

//=========================================================
// Spawn
//=========================================================
void CJoe :: Spawn()
{
	Precache( );

	SET_MODEL(ENT(pev), "models/joe.mdl");
	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= BLOOD_COLOR_RED;
	if (pev->spawnflags & SF_JOE_HOSTILE)
		pev->health		= 1;
	else
		pev->health		= 200;
	pev->view_ofs		= Vector ( 0, 0, 50 );// position of the eyes relative to monster's origin.
	m_flFieldOfView		= VIEW_FIELD_WIDE; // NOTE: we need a wide field of view so npc will notice player and say hello
	m_MonsterState		= MONSTERSTATE_NONE;

	pev->body			= 0; // gun in holster
	m_fGunDrawn			= FALSE;

	m_afCapability		= bits_CAP_HEAR | bits_CAP_TURN_HEAD | bits_CAP_DOORS_GROUP;

	MonsterInit();
	SetUse( FollowerUse );
	m_cAmmoLoaded		= 6;
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CJoe :: Precache()
{
	PRECACHE_MODEL("models/joe.mdl");

	PRECACHE_SOUND("weapons/357_shot1.wav" );
	PRECACHE_SOUND("weapons/357_shot2.wav" );
	PRECACHE_SOUND("weapons/357_reload1.wav" );

	PRECACHE_SOUND("joe/joepain_1.wav");
	PRECACHE_SOUND("joe/joepain_2.wav");
	PRECACHE_SOUND("joe/joepain_3.wav");
	PRECACHE_SOUND("joe/joepain_4.wav");

	PRECACHE_SOUND("joe/joedie_1.wav");
	PRECACHE_SOUND("joe/joedie_2.wav");
	PRECACHE_SOUND("joe/joedie_3.wav");
	PRECACHE_SOUND("joe/joedie_4.wav");

	// every new joe must call this, otherwise
	// when a level is loaded, nobody will talk (time is reset to 0)
	TalkInit();
	CTalkMonster::Precache();
}

// Init talk data
void CJoe :: TalkInit()
{

	CTalkMonster::TalkInit();

	// joe speech group names (group names are in sentences.txt)

	if (!m_iszSpeakAs)
	{
		m_szGrp[TLK_ANSWER]		=	"JO_ANSWER";
		m_szGrp[TLK_QUESTION]	=	"JO_QUESTION";
		m_szGrp[TLK_IDLE]		=	"JO_IDLE";
		m_szGrp[TLK_STARE]		=	"JO_STARE";
		if (pev->spawnflags & SF_MONSTER_PREDISASTER) //LRC
			m_szGrp[TLK_USE]	=	"JO_HELLO";
		else
			m_szGrp[TLK_USE] =	"JO_OK";
		if (pev->spawnflags & SF_MONSTER_PREDISASTER)
			m_szGrp[TLK_UNUSE] = "JO_HELLO";
		else
			m_szGrp[TLK_UNUSE] = "JO_WAIT";
		if (pev->spawnflags & SF_MONSTER_PREDISASTER)
			m_szGrp[TLK_DECLINE] =	"JO_HELLO";
		else
			m_szGrp[TLK_DECLINE] =	"JO_NOTOK";
		m_szGrp[TLK_STOP] =		"JO_STOP";

		m_szGrp[TLK_NOSHOOT] =	"JO_SCARED";
		m_szGrp[TLK_HELLO] =	"JO_HELLO";

		m_szGrp[TLK_PLHURT1] =	"!JO_CUREA";
		m_szGrp[TLK_PLHURT2] =	"!JO_CUREB";
		m_szGrp[TLK_PLHURT3] =	"!JO_CUREC";

		m_szGrp[TLK_PHELLO] =	NULL;	//"JO_PHELLO";		// UNDONE
		m_szGrp[TLK_PIDLE] =	NULL;	//"JO_PIDLE";			// UNDONE
		m_szGrp[TLK_PQUESTION] = "JO_QUESTION";		// UNDONE

		m_szGrp[TLK_SMELL] =	"JO_SMELL";

		m_szGrp[TLK_WOUND] =	"JO_WOUND";
		m_szGrp[TLK_MORTAL] =	"JO_MORTAL";
	}

	// get voice for head - just one joe voice for now
	m_voicePitch = 100;
}

int CJoe :: TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType)
{
	if ( FClassnameIs( pevAttacker, "monster_joe" ) && !(pev->spawnflags & SF_JOE_HOSTILE) )
		return 0;

	// make sure friends talk about it if player hurts talkmonsters...
	int ret = CTalkMonster::TakeDamage(pevInflictor, pevAttacker, flDamage, bitsDamageType);
	if ( !IsAlive() || pev->deadflag == DEAD_DYING )
		return ret;

	if ( pev->spawnflags & SF_JOE_HOSTILE )
		return ret;

	if ( m_MonsterState != MONSTERSTATE_PRONE && (pevAttacker->flags & FL_CLIENT) )
	{
		m_flPlayerDamage += flDamage;

		// This is a heurstic to determine if the player intended to harm me
		// If I have an enemy, we can't establish intent (may just be crossfire)
		if ( m_hEnemy == NULL )
		{
			// If the player was facing directly at me, or I'm already suspicious, get mad
			if ( (m_afMemory & bits_MEMORY_SUSPICIOUS) || UTIL_IsFacing( pevAttacker, pev->origin ) )
			{
				// Alright, now I'm pissed!
				if (m_iszSpeakAs)
				{
					char szBuf[32];
					strcpy(szBuf,STRING(m_iszSpeakAs));
					strcat(szBuf,"_MAD");
					PlaySentence( szBuf, 4, VOL_NORM, ATTN_NORM );
				}
				else
				{
					PlaySentence( "JO_MAD", 4, VOL_NORM, ATTN_NORM );
				}

				Remember( bits_MEMORY_PROVOKED );
				StopFollowing( TRUE );
			}
			else
			{
				// Hey, be careful with that
				if (m_iszSpeakAs)
				{
					char szBuf[32];
					strcpy(szBuf,STRING(m_iszSpeakAs));
					strcat(szBuf,"_SHOT");
					PlaySentence( szBuf, 4, VOL_NORM, ATTN_NORM );
				}
				else
				{
					PlaySentence( "JO_SHOT", 4, VOL_NORM, ATTN_NORM );
				}
				Remember( bits_MEMORY_SUSPICIOUS );
			}
		}
		else if ( !(m_hEnemy->IsPlayer()) && pev->deadflag == DEAD_NO )
		{
			if (m_iszSpeakAs)
			{
				char szBuf[32];
				strcpy(szBuf,STRING(m_iszSpeakAs));
				strcat(szBuf,"_SHOT");
				PlaySentence( szBuf, 4, VOL_NORM, ATTN_NORM );
			}
			else
			{
				PlaySentence( "JO_SHOT", 4, VOL_NORM, ATTN_NORM );
			}
		}
	}

	return ret;
}


//=========================================================
// PainSound
//=========================================================
void CJoe :: PainSound ( void )
{
	if (gpGlobals->time < m_painTime)
		return;

	m_painTime = gpGlobals->time + RANDOM_FLOAT(0.5, 0.75);

	switch (RANDOM_LONG(0,3))
	{
	case 0: EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "joe/joepain_1.wav", 1, ATTN_NORM, 0, GetVoicePitch()); break;
	case 1: EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "joe/joepain_2.wav", 1, ATTN_NORM, 0, GetVoicePitch()); break;
	case 2: EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "joe/joepain_3.wav", 1, ATTN_NORM, 0, GetVoicePitch()); break;
	case 3: EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "joe/joepain_4.wav", 1, ATTN_NORM, 0, GetVoicePitch()); break;
	}
}

//=========================================================
// DeathSound
//=========================================================
void CJoe :: DeathSound ( void )
{
	switch (RANDOM_LONG(0,3))
	{
	case 0: EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "joe/joedie_1.wav", 1, ATTN_NORM, 0, GetVoicePitch()); break;
	case 1: EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "joe/joedie_2.wav", 1, ATTN_NORM, 0, GetVoicePitch()); break;
	case 2: EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "joe/joedie_3.wav", 1, ATTN_NORM, 0, GetVoicePitch()); break;
	case 3: EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "joe/joedie_4.wav", 1, ATTN_NORM, 0, GetVoicePitch()); break;
	}
}


void CJoe::TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType)
{
	switch( ptr->iHitgroup)
	{
	case HITGROUP_CHEST:
	case HITGROUP_STOMACH:
		if (bitsDamageType & (DMG_BULLET | DMG_SLASH | DMG_BLAST))
		{
			flDamage = flDamage / 2;
		}
		break;
	case 10:
		if (bitsDamageType & (DMG_BULLET | DMG_SLASH | DMG_CLUB))
		{
			flDamage -= 20;
			if (flDamage <= 0)
			{
				UTIL_Ricochet( ptr->vecEndPos, 1.0 );
				flDamage = 0.01;
			}
		}
		// always a head shot
		ptr->iHitgroup = HITGROUP_HEAD;
		break;
	}

	CTalkMonster::TraceAttack( pevAttacker, flDamage, vecDir, ptr, bitsDamageType );
}


void CJoe::Killed( entvars_t *pevAttacker, int iGib )
{
	if ( pev->body < JOE_BODY_GUNGONE && !(pev->spawnflags & SF_MONSTER_NO_WPN_DROP))
	{// drop the gun!
		Vector vecGunPos;
		Vector vecGunAngles;

		pev->body = JOE_BODY_GUNGONE;

		GetAttachment( 0, vecGunPos, vecGunAngles );

		CBaseEntity *pGun = DropItem( "weapon_357", vecGunPos, vecGunAngles );
	}

	SetUse( NULL );
	CTalkMonster::Killed( pevAttacker, iGib );
}

//=========================================================
// AI Schedules Specific to this monster
//=========================================================

Schedule_t* CJoe :: GetScheduleOfType ( int Type )
{
	Schedule_t *psched;

	switch( Type )
	{
	case SCHED_ARM_WEAPON:
		if ( m_hEnemy != NULL )
		{
			// face enemy, then draw.
			return slJoeEnemyDraw;
		}
		break;

	// Hook these to make a looping schedule
	case SCHED_TARGET_FACE:
		// call base class default so that joe will talk
		// when 'used'
		psched = CTalkMonster::GetScheduleOfType(Type);

		if (psched == slIdleStand)
			return slJoFaceTarget;	// override this for different target face behavior
		else
			return psched;

	case SCHED_TARGET_CHASE:
		return slJoFollow;

	case SCHED_IDLE_STAND:
		// call base class default so that scientist will talk
		// when standing during idle
		psched = CTalkMonster::GetScheduleOfType(Type);

		if (psched == slIdleStand)
		{
			// just look straight ahead.
			return slIdleJoStand;
		}
		else
			return psched;

	case SCHED_RANGE_ATTACK1:
		return slJoeRangeAttack1;

	case SCHED_JOE_HIDE_RELOAD:
		return slJoeHideReload;
	}

	return CTalkMonster::GetScheduleOfType( Type );
}

//=========================================================
// GetSchedule - Decides which type of schedule best suits
// the monster's current state and conditions. Then calls
// monster's member function to get a pointer to a schedule
// of the proper type.
//=========================================================
Schedule_t *CJoe :: GetSchedule ( void )
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
		// Hey, be careful with that
		if (m_iszSpeakAs)
		{
			char szBuf[32];
			strcpy(szBuf,STRING(m_iszSpeakAs));
			strcat(szBuf,"_KILL");
			PlaySentence( szBuf, 4, VOL_NORM, ATTN_NORM );
		}
		else
		{
			PlaySentence( "JO_KILL", 4, VOL_NORM, ATTN_NORM );
		}
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

			// out of ammo: reload, behind cover if hurt
			if ( HasConditions( bits_COND_NO_AMMO_LOADED ) )
			{
				if ( pev->spawnflags & SF_JOE_HOSTILE )
					return GetScheduleOfType( SCHED_RELOAD );

				if ( pev->health > 80.0 )
					return GetScheduleOfType( SCHED_RELOAD );

				return GetScheduleOfType( SCHED_JOE_HIDE_RELOAD );
			}

			// always act surprized with a new enemy
			if ( HasConditions( bits_COND_NEW_ENEMY ) && HasConditions( bits_COND_LIGHT_DAMAGE) )
				return GetScheduleOfType( SCHED_SMALL_FLINCH );

			// wait for one schedule to draw gun
			if (!m_fGunDrawn )
				return GetScheduleOfType( SCHED_ARM_WEAPON );

			if ( HasConditions( bits_COND_HEAVY_DAMAGE ) )
				return GetScheduleOfType( SCHED_TAKE_COVER_FROM_ENEMY );
		}
		break;

	case MONSTERSTATE_ALERT:
	case MONSTERSTATE_IDLE:
		if ( HasConditions(bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE))
		{
			// flinch if hurt
			return GetScheduleOfType( SCHED_SMALL_FLINCH );
		}

		if ( m_hEnemy == NULL && IsFollowing() )
		{
			if ( !m_hTargetEnt->IsAlive() )
			{
				// UNDONE: Comment about the recently dead player here?
				StopFollowing( FALSE );
				break;
			}
			else
			{
				if ( HasConditions( bits_COND_CLIENT_PUSH ) )
				{
					return GetScheduleOfType( SCHED_MOVE_AWAY_FOLLOW );
				}
				return GetScheduleOfType( SCHED_TARGET_FACE );
			}
		}

		if ( HasConditions( bits_COND_CLIENT_PUSH ) )
		{
			return GetScheduleOfType( SCHED_MOVE_AWAY );
		}

		// try to say something about smells
		TrySmellTalk();
		break;
	}

	return CTalkMonster::GetSchedule();
}

MONSTERSTATE CJoe :: GetIdealState ( void )
{
	return CTalkMonster::GetIdealState();
}



void CJoe::DeclineFollowing( void )
{
	PlaySentence( m_szGrp[TLK_DECLINE], 2, VOL_NORM, ATTN_NORM ); //LRC
	StopForCaption( TRUE );
}

//=========================================================
// CheckAmmo - overridden for joe because he actually
// uses ammo! (base class doesn't)
//=========================================================
void CJoe :: CheckAmmo ( void )
{
	if ( m_cAmmoLoaded <= 0 )
	{
		SetConditions(bits_COND_NO_AMMO_LOADED);
	}
}

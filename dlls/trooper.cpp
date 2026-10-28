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
// T2DEF: monster_trooper, the Schwarzgeist trooper with a
// shotgun or a crossbow, and the crossbow bolt it fires.
// Built from hgrunt.cpp and crossbow.cpp's CCrossbowBolt.
// (file name and the names marked "name ours" are ours)
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
#include	"scripted.h" //LRC

int g_fTrooperQuestion;				// true if an idle trooper asked a question. Cleared when someone answers. (name ours)
int g_iLastTrooperHead;				// the head the last trooper spawned with (name ours)

extern DLL_GLOBAL int		g_iSkillLevel;

#define BOLT_AIR_VELOCITY	2000
#define BOLT_WATER_VELOCITY	1000

// UNDONE: Save/restore this?  Don't forget to set classname and LINK_ENTITY_TO_CLASS()
//
// OVERLOADS SOME ENTVARS:
//
// speed - the ideal magnitude of my velocity
class CTrooperCrossbowBolt : public CBaseEntity
{
	void Spawn( void );
	void Precache( void );
	int  Classify ( void );
	void EXPORT BubbleThink( void );
	void EXPORT BoltTouch( CBaseEntity *pOther );

	int m_iTrail;

public:
	static CTrooperCrossbowBolt *BoltCreate( void );
};
LINK_ENTITY_TO_CLASS( trooper_crossbow_bolt, CTrooperCrossbowBolt );

CTrooperCrossbowBolt *CTrooperCrossbowBolt::BoltCreate( void )
{
	// Create a new entity with CTrooperCrossbowBolt private data
	CTrooperCrossbowBolt *pBolt = GetClassPtr( (CTrooperCrossbowBolt *)NULL );
	pBolt->pev->classname = MAKE_STRING("trooper_bolt");
	pBolt->Spawn();

	return pBolt;
}

void CTrooperCrossbowBolt::Spawn( )
{
	Precache( );
	pev->movetype = MOVETYPE_FLY;
	pev->solid = SOLID_BBOX;

	pev->gravity = 0.5;

	SET_MODEL(ENT(pev), "models/crossbow_bolt.mdl");

	UTIL_SetOrigin( this, pev->origin );
	UTIL_SetSize(pev, Vector(0, 0, 0), Vector(0, 0, 0));

	SetTouch( BoltTouch );
	SetThink( BubbleThink );
	SetNextThink( 0.2 );
}


void CTrooperCrossbowBolt::Precache( )
{
	PRECACHE_MODEL ("models/crossbow_bolt.mdl");
	PRECACHE_SOUND("weapons/xbow_hitbod1.wav");
	PRECACHE_SOUND("weapons/xbow_hitbod2.wav");
	PRECACHE_SOUND("weapons/xbow_fly1.wav");
	PRECACHE_SOUND("weapons/xbow_hit1.wav");
	PRECACHE_SOUND("fvox/beep.wav");
	m_iTrail = PRECACHE_MODEL("sprites/streak.spr");
}


int	CTrooperCrossbowBolt :: Classify ( void )
{
	return	CLASS_NONE;
}

void CTrooperCrossbowBolt::BoltTouch( CBaseEntity *pOther )
{
	SetTouch( NULL );
	SetThink( NULL );

	if (pOther->pev->takedamage)
	{
		TraceResult tr = UTIL_GetGlobalTrace( );
		entvars_t	*pevOwner;

		pevOwner = VARS( pev->owner );

		// UNDONE: this needs to call TraceAttack instead
		ClearMultiDamage( );

		if ( pOther->IsPlayer() )
		{
			pOther->TraceAttack(pevOwner, gSkillData.plrDmgCrossbowClient, pev->velocity.Normalize(), &tr, DMG_NEVERGIB );
		}
		else
		{
			pOther->TraceAttack(pevOwner, gSkillData.plrDmgCrossbowMonster, pev->velocity.Normalize(), &tr, DMG_BULLET | DMG_NEVERGIB );
		}

		ApplyMultiDamage( pev, pevOwner );

		pev->velocity = Vector( 0, 0, 0 );
		// play body "thwack" sound
		switch( RANDOM_LONG(0,1) )
		{
		case 0:
			EMIT_SOUND(ENT(pev), CHAN_BODY, "weapons/xbow_hitbod1.wav", 1, ATTN_NORM); break;
		case 1:
			EMIT_SOUND(ENT(pev), CHAN_BODY, "weapons/xbow_hitbod2.wav", 1, ATTN_NORM); break;
		}

		SetThink( SUB_Remove );
		SetNextThink( 0 );
	}
	else
	{
		EMIT_SOUND_DYN(ENT(pev), CHAN_BODY, "weapons/xbow_hit1.wav", RANDOM_FLOAT(0.95, 1.0), ATTN_NORM, 0, 98 + RANDOM_LONG(0,7));

		SetThink( SUB_Remove );
		SetNextThink( 0 );// this will get changed below if the bolt is allowed to stick in what it hit.

		if ( FClassnameIs( pOther->pev, "worldspawn" ) )
		{
			// if what we hit is static architecture, can stay around for a while.
			Vector vecDir = pev->velocity.Normalize( );
			UTIL_SetOrigin( this, pev->origin - vecDir * 12 );
			pev->angles = UTIL_VecToAngles( vecDir );
			pev->solid = SOLID_NOT;
			pev->movetype = MOVETYPE_FLY;
			pev->velocity = Vector( 0, 0, 0 );
			pev->avelocity.z = 0;
			pev->angles.z = RANDOM_LONG(0,360);
			SetNextThink( 10.0 );
		}

		if (UTIL_PointContents(pev->origin) != CONTENTS_WATER)
		{
			UTIL_Sparks( pev->origin );
		}
	}
}

void CTrooperCrossbowBolt::BubbleThink( void )
{
	SetNextThink( 0.1 );

	if (pev->waterlevel == 0 || pev->watertype <= CONTENT_FLYFIELD)
		return;

	UTIL_BubbleTrail( pev->origin - pev->velocity * 0.1, pev->origin, 1 );
}

//=========================================================
// monster-specific DEFINE's
//=========================================================
#define TROOPER_VOL						0.35		// volume of trooper sounds
#define TROOPER_ATTN					ATTN_NORM	// attenutation of trooper sentences
#define TROOPER_LIMP_HEALTH				20
#define	TROOPER_SENTENCE_VOLUME			(float)0.85 // volume of trooper sentences

#define TROOPER_SHOTGUN				( 1 << 0)
#define TROOPER_CROSSBOW			( 1 << 1)

#define HEAD_GROUP					1

#define GUN_GROUP					2
#define GUN_SHOTGUN					0
#define GUN_CROSSBOW				1
#define GUN_NONE					2

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define		TROOPER_AE_RELOAD		( 2 )
#define		TROOPER_AE_KICK			( 3 )
#define		TROOPER_AE_BURST1		( 4 )
#define		TROOPER_AE_BURST2		( 5 )
#define		TROOPER_AE_BURST3		( 6 )
#define		TROOPER_AE_CAUGHT_ENEMY	( 10) // trooper established sight with an enemy (player only) that had previously eluded the squad.
#define		TROOPER_AE_DROP_GUN		( 11) // trooper (probably dead) is dropping his gun.

//=========================================================
// monster-specific schedule types
//=========================================================
enum
{
	SCHED_TROOPER_SUPPRESS = LAST_COMMON_SCHEDULE + 1,
	SCHED_TROOPER_ESTABLISH_LINE_OF_FIRE,// move to a location to set up an attack against the enemy. (usually when a friendly is in the way).
	SCHED_TROOPER_COVER_AND_RELOAD,
	SCHED_TROOPER_SWEEP,
	SCHED_TROOPER_FOUND_ENEMY,
	SCHED_TROOPER_WAIT_FACE_ENEMY,
	SCHED_TROOPER_TAKECOVER_FAILED,// special schedule type that forces analysis of conditions and picks the best possible schedule to recover from this type of failure.
	SCHED_TROOPER_ELOF_FAIL,
};

//=========================================================
// monster-specific tasks
//=========================================================
enum
{
	TASK_TROOPER_SPEAK_SENTENCE = LAST_COMMON_TASK + 1,
	TASK_TROOPER_CHECK_FIRE,
};

//=========================================================
// monster-specific conditions
//=========================================================
#define bits_COND_TROOPER_NOFIRE	( bits_COND_SPECIAL1 )

class CTrooper : public CSquadMonster
{
public:
	void Spawn( void );
	void Precache( void );
	void SetYawSpeed ( void );
	int  Classify ( void );
	void HandleAnimEvent( MonsterEvent_t *pEvent );
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
	void ShootBolt ( void );
	void Shotgun ( void );
	void PrescheduleThink ( void );
	void GibMonster( void );
	void SpeakSentence( void );

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

	BOOL	m_fStanding;
	BOOL	m_fFirstEncounter;// only put on the handsign show in the squad's first encounter.
	int		m_cClipSize;

	int m_voicePitch;

	int		m_iShotgunShell;

	int		m_iSentence;

	static const char *pTrooperSentences[];
};

LINK_ENTITY_TO_CLASS( monster_trooper, CTrooper );

TYPEDESCRIPTION	CTrooper::m_SaveData[] =
{
	DEFINE_FIELD( CTrooper, m_flNextPainTime, FIELD_TIME ),
//	DEFINE_FIELD( CTrooper, m_flLastEnemySightTime, FIELD_TIME ), // don't save, go to zero
	DEFINE_FIELD( CTrooper, m_fStanding, FIELD_BOOLEAN ),
	DEFINE_FIELD( CTrooper, m_fFirstEncounter, FIELD_BOOLEAN ),
	DEFINE_FIELD( CTrooper, m_cClipSize, FIELD_INTEGER ),
	DEFINE_FIELD( CTrooper, m_voicePitch, FIELD_INTEGER ),
//  DEFINE_FIELD( CShotgun, m_iShotgunShell, FIELD_INTEGER ),
	DEFINE_FIELD( CTrooper, m_iSentence, FIELD_INTEGER ),
};

IMPLEMENT_SAVERESTORE( CTrooper, CSquadMonster );

const char *CTrooper::pTrooperSentences[] =
{
	"T_GREN", // grenade scared trooper
	"T_ALERT", // sees player
	"T_TAUNT", // say rude things
};

enum
{
	TROOPER_SENT_NONE = -1,
	TROOPER_SENT_GREN = 0,
	TROOPER_SENT_ALERT,
	TROOPER_SENT_TAUNT,
} TROOPER_SENTENCE_TYPES; // a variable, as in hgrunt.cpp: retail .bss 0x101d18ec (name ours)

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
void CTrooper :: SpeakSentence( void )
{
	if ( m_iSentence == TROOPER_SENT_NONE )
	{
		// no sentence cued up.
		return;
	}

	if (FOkToSpeak())
	{
		SENTENCEG_PlayRndSz( ENT(pev), pTrooperSentences[ m_iSentence ], TROOPER_SENTENCE_VOLUME, TROOPER_ATTN, 0, m_voicePitch);
		JustSpoke();
	}
}

//=========================================================
// IRelationship - overridden because human grunts are
// the trooper's nemesis.
//=========================================================
int CTrooper::IRelationship ( CBaseEntity *pTarget )
{
	if ( FClassnameIs( pTarget->pev, "monster_human_grunt" ) )
	{
		return R_NM;
	}

	return CSquadMonster :: IRelationship( pTarget );
}

//=========================================================
// GibMonster - make gun fly through the air.
//=========================================================
void CTrooper :: GibMonster ( void )
{
	Vector	vecGunPos;
	Vector	vecGunAngles;

	if ( GetBodygroup( 2 ) != 2 )
	{// throw a gun if the trooper has one
		GetAttachment( 0, vecGunPos, vecGunAngles );

		CBaseEntity *pGun;
		if (FBitSet( pev->weapons, TROOPER_SHOTGUN ))
		{
			pGun = DropItem( "weapon_shotgun", vecGunPos, vecGunAngles );
		}
		else if (FBitSet( pev->weapons, TROOPER_CROSSBOW ))
		{
			pGun = DropItem( "weapon_crossbow", vecGunPos, vecGunAngles );
		}
		if ( pGun ) // sic: unset when the trooper has neither weapon
		{
			pGun->pev->velocity = Vector (RANDOM_FLOAT(-100,100), RANDOM_FLOAT(-100,100), RANDOM_FLOAT(200,300));
			pGun->pev->avelocity = Vector ( 0, RANDOM_FLOAT( 200, 400 ), 0 );
		}
	}

	CBaseMonster :: GibMonster();
}

//=========================================================
// someone else is talking - don't speak
//=========================================================
BOOL CTrooper :: FOkToSpeak( void )
{
// if someone else is talking, don't speak
	if (gpGlobals->time <= CTalkMonster::g_talkWaitTime)
		return FALSE;

	if ( pev->spawnflags & SF_MONSTER_GAG )
	{
		if ( m_hEnemy == NULL )
		{
			// no talking without an enemy if gagged.
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
void CTrooper :: JustSpoke( void )
{
	CTalkMonster::g_talkWaitTime = gpGlobals->time + RANDOM_FLOAT(1.5, 2.0);
	m_iSentence = TROOPER_SENT_NONE;
}

//=========================================================
// PrescheduleThink - this function runs after conditions
// are collected and before scheduling code is run.
//=========================================================
void CTrooper :: PrescheduleThink ( void )
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
// CheckMeleeAttack1
//=========================================================
BOOL CTrooper :: CheckMeleeAttack1 ( float flDot, float flDist )
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
// CheckRangeAttack1 - overridden for HGrunt, cause
// FCanCheckAttacks() doesn't disqualify all attacks based
// on whether or not the enemy is occluded because unlike
// the base class, the HGrunt can attack when the enemy is
// occluded (throw grenade over wall, etc). We must
// disqualify the machine gun attack if the enemy is occluded.
//=========================================================
BOOL CTrooper :: CheckRangeAttack1 ( float flDot, float flDist )
{
	if ( !HasConditions( bits_COND_ENEMY_OCCLUDED ) && flDist <= 2048 && flDot >= 0.5 && NoFriendlyFire() )
	{
		TraceResult	tr;

		if ( !m_hEnemy->IsPlayer() && flDist <= 64 )
		{
			// kick nonclients who are close enough, but don't shoot at them.
			return FALSE;
		}

		Vector vecSrc = GetGunPosition();

		// verify that a bullet fired from the gun will hit the enemy before the world.
		UTIL_TraceLine( vecSrc, m_hEnemy->BodyTarget(vecSrc), ignore_monsters, ignore_glass, ENT(pev), &tr);

		if ( tr.flFraction == 1.0 )
		{
			return TRUE;
		}
	}

	return FALSE;
}

//=========================================================
// CheckRangeAttack2 - troopers have no grenades
//=========================================================
BOOL CTrooper :: CheckRangeAttack2 ( float flDot, float flDist )
{
	return FALSE;
}


//=========================================================
// TraceAttack - helmet shots are head shots
//=========================================================
void CTrooper :: TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType)
{
	// check for helmet shot
	if (ptr->iHitgroup == 11)
	{
		// it's head shot anyways
		ptr->iHitgroup = HITGROUP_HEAD;
	}
	CSquadMonster::TraceAttack( pevAttacker, flDamage, vecDir, ptr, bitsDamageType );
}


//=========================================================
// TakeDamage - overridden for the grunt because the grunt
// needs to forget that he is in cover if he's hurt. (Obviously
// not in a safe place anymore).
//=========================================================
int CTrooper :: TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	Forget( bits_MEMORY_INCOVER );

	return CSquadMonster :: TakeDamage ( pevInflictor, pevAttacker, flDamage, bitsDamageType );
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CTrooper :: SetYawSpeed ( void )
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

void CTrooper :: IdleSound( void )
{
	if (FOkToSpeak() && (g_fTrooperQuestion || RANDOM_LONG(0,1)))
	{
		if (!g_fTrooperQuestion)
		{
			// ask question or make statement
			switch (RANDOM_LONG(0,1))
			{
			case 0: // check in
				SENTENCEG_PlayRndSz(ENT(pev), "T_CHECK", TROOPER_SENTENCE_VOLUME, ATTN_IDLE, 0, m_voicePitch);
				g_fTrooperQuestion = 1;
				break;
			case 1: // question
				SENTENCEG_PlayRndSz(ENT(pev), "T_QUEST", TROOPER_SENTENCE_VOLUME, ATTN_IDLE, 0, m_voicePitch);
				g_fTrooperQuestion = 2;
				break;
			}
		}
		else
		{
			switch (g_fTrooperQuestion)
			{
			case 1: // check in
				SENTENCEG_PlayRndSz(ENT(pev), "T_CLEAR", TROOPER_SENTENCE_VOLUME, ATTN_IDLE, 0, m_voicePitch);
				break;
			case 2: // question
				SENTENCEG_PlayRndSz(ENT(pev), "T_ANSWER", TROOPER_SENTENCE_VOLUME, ATTN_IDLE, 0, m_voicePitch);
				break;
			}
			g_fTrooperQuestion = 0;
		}
		JustSpoke();
	}
}

//=========================================================
// CheckAmmo - overridden for the grunt because he actually
// uses ammo! (base class doesn't)
//=========================================================
void CTrooper :: CheckAmmo ( void )
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
int	CTrooper :: Classify ( void )
{
	return m_iClass?m_iClass:CLASS_FACTION_A;
}

//=========================================================
//=========================================================
CBaseEntity *CTrooper :: Kick( void )
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

Vector CTrooper :: GetGunPosition( )
{
	if (m_fStanding )
	{
		return pev->origin + Vector( 0, 0, 60 );
	}
	else
	{
		return pev->origin + Vector( 0, 0, 48 );
	}
}

//=========================================================
// ShootBolt - fire a crossbow bolt (name ours)
//=========================================================
void CTrooper :: ShootBolt ( void )
{
	if (m_hEnemy == NULL)
	{
		return;
	}

	Vector vecShootOrigin = GetGunPosition();
	Vector vecShootDir = ShootAtEnemy( vecShootOrigin );

	UTIL_MakeVectors ( pev->angles );

	CTrooperCrossbowBolt *pBolt = CTrooperCrossbowBolt::BoltCreate();
	pBolt->pev->origin = vecShootOrigin;
	pBolt->pev->angles = UTIL_VecToAngles( vecShootDir );
	pBolt->pev->owner = edict();
	pBolt->pev->velocity = vecShootDir * BOLT_AIR_VELOCITY;
	pBolt->pev->speed = BOLT_AIR_VELOCITY;

	m_cAmmoLoaded--;// take away a bullet!

	Vector angDir = UTIL_VecToAngles( vecShootDir );
	SetBlending( 0, angDir.x );
}

//=========================================================
// Shoot
//=========================================================
void CTrooper :: Shotgun ( void )
{
	if (m_hEnemy == NULL)
	{
		return;
	}

	Vector vecShootOrigin = GetGunPosition();
	Vector vecShootDir = ShootAtEnemy( vecShootOrigin );

	UTIL_MakeVectors ( pev->angles );

	Vector	vecShellVelocity = gpGlobals->v_right * RANDOM_FLOAT(40,90) + gpGlobals->v_up * RANDOM_FLOAT(75,200) + gpGlobals->v_forward * RANDOM_FLOAT(-40, 40);
	EjectBrass ( vecShootOrigin - vecShootDir * 24, vecShellVelocity, pev->angles.y, m_iShotgunShell, TE_BOUNCE_SHOTSHELL);
	FireBullets(gSkillData.hgruntShotgunPellets, vecShootOrigin, vecShootDir, VECTOR_CONE_15DEGREES, 2048, BULLET_PLAYER_BUCKSHOT, 0 ); // shoot +-7.5 degrees

	pev->effects |= EF_MUZZLEFLASH;

	m_cAmmoLoaded--;// take away a bullet!

	Vector angDir = UTIL_VecToAngles( vecShootDir );
	SetBlending( 0, angDir.x );
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CTrooper :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	Vector	vecShootDir;
	Vector	vecShootOrigin;

	switch( pEvent->event )
	{
		case TROOPER_AE_DROP_GUN:
			{
			Vector	vecGunPos;
			Vector	vecGunAngles;

			GetAttachment( 0, vecGunPos, vecGunAngles );

			// switch to body group with no gun.
			SetBodygroup( GUN_GROUP, GUN_NONE );

			// now spawn a gun.
			if (FBitSet( pev->weapons, TROOPER_SHOTGUN ))
			{
				 DropItem( "weapon_shotgun", vecGunPos, vecGunAngles );
			}
			else if (FBitSet( pev->weapons, TROOPER_CROSSBOW ))
			{
				 DropItem( "weapon_crossbow", vecGunPos, vecGunAngles );
			}

			}
			break;

		case TROOPER_AE_RELOAD:
			if (FBitSet( pev->weapons, TROOPER_SHOTGUN ))
			{
				EMIT_SOUND( ENT(pev), CHAN_WEAPON, "hgrunt/gr_reload1.wav", 1, ATTN_NORM );
			}
			else if (FBitSet( pev->weapons, TROOPER_CROSSBOW ))
			{
				EMIT_SOUND( ENT(pev), CHAN_WEAPON, "weapons/xbow_reload1.wav", 1, ATTN_NORM );
			}
			m_cAmmoLoaded = m_cClipSize;
			ClearConditions(bits_COND_NO_AMMO_LOADED);
			break;

		case TROOPER_AE_BURST1:
		{
			if ( FBitSet( pev->weapons, TROOPER_SHOTGUN ))
			{
				Shotgun( );

				int iBarrel = RANDOM_LONG(0,10);
				if ( iBarrel >= 3 )
				{
					EMIT_SOUND(ENT(pev), CHAN_WEAPON, "weapons/sbarrel1.wav", 1, ATTN_NORM );
				}
				else
				{
					EMIT_SOUND(ENT(pev), CHAN_WEAPON, "weapons/dbarrel1.wav", 1, ATTN_NORM );
				}
			}
			else if ( FBitSet( pev->weapons, TROOPER_CROSSBOW ))
			{
				ShootBolt( );

				EMIT_SOUND(ENT(pev), CHAN_WEAPON, "weapons/xbow_fire1.wav", 1, ATTN_NORM );
			}

			CSoundEnt::InsertSound ( bits_SOUND_COMBAT, pev->origin, 384, 0.3 );
		}
		break;

		case TROOPER_AE_BURST2: // sic: the burst events fall into the kick
		case TROOPER_AE_BURST3:
		case TROOPER_AE_KICK:
		{
			CBaseEntity *pHurt = Kick();

			if ( pHurt )
			{
				// SOUND HERE!
				UTIL_MakeVectors( pev->angles );
				pHurt->pev->punchangle.x = 15;
				pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_forward * 100 + gpGlobals->v_up * 50;
				pHurt->TakeDamage( pev, pev, gSkillData.hgruntDmgKick, DMG_CLUB );
			}
		}
		break;

		case TROOPER_AE_CAUGHT_ENEMY:
		{
			if ( FOkToSpeak() )
			{
				SENTENCEG_PlayRndSz(ENT(pev), "T_ALERT", TROOPER_SENTENCE_VOLUME, TROOPER_ATTN, 0, m_voicePitch);
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
void CTrooper :: Spawn()
{
	int iHead;

	Precache( );

	SET_MODEL(ENT(pev), "models/trooper.mdl");
	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= BLOOD_COLOR_RED;
	pev->effects		= 0;
	pev->health			= gSkillData.hgruntHealth;
	m_flFieldOfView		= 0.2;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState		= MONSTERSTATE_NONE;
	m_flNextPainTime	= gpGlobals->time;
	m_iSentence			= TROOPER_SENT_NONE;

	m_afCapability		= bits_CAP_SQUAD | bits_CAP_TURN_HEAD | bits_CAP_DOORS_GROUP;

	m_fEnemyEluded		= FALSE;
	m_fFirstEncounter	= TRUE;// this is true when the trooper spawns, because he hasn't encountered an enemy yet.

	m_HackedGunPos = Vector ( 0, 0, 55 );

	if (pev->weapons == 0)
	{
		// initialize to original values
		pev->weapons = TROOPER_SHOTGUN;
	}

	if (pev->body == -1)
	{
		// pick a head, not the one the last trooper got
		while ( iHead == g_iLastTrooperHead ) // sic: iHead is not initialised
		{
			iHead = RANDOM_LONG( 0, 4 );
		}
		SetBodygroup( HEAD_GROUP, iHead );
	}
	g_iLastTrooperHead = pev->body;

	if (FBitSet( pev->weapons, TROOPER_SHOTGUN ))
	{
		SetBodygroup( GUN_GROUP, GUN_SHOTGUN );
		m_cClipSize		= 8;
	}
	else if (FBitSet( pev->weapons, TROOPER_CROSSBOW ))
	{
		SetBodygroup( GUN_GROUP, GUN_CROSSBOW );
		m_cClipSize		= 4;
	}
	else
	{
		SetBodygroup( GUN_GROUP, GUN_NONE );
	}
	m_cAmmoLoaded		= m_cClipSize;

	CTalkMonster::g_talkWaitTime = 0;

	if ( GetBodygroup( HEAD_GROUP ) == 4 )
		pev->skin = 1;
	else
		pev->skin = 0;

	MonsterInit();
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CTrooper :: Precache()
{
	PRECACHE_MODEL("models/trooper.mdl");

	PRECACHE_SOUND( "hgrunt/gr_die1.wav" );
	PRECACHE_SOUND( "hgrunt/gr_die2.wav" );
	PRECACHE_SOUND( "hgrunt/gr_die3.wav" );

	PRECACHE_SOUND( "trooper/tr_pain1.wav" );
	PRECACHE_SOUND( "trooper/tr_pain2.wav" );
	PRECACHE_SOUND( "trooper/tr_pain3.wav" );
	PRECACHE_SOUND( "trooper/tr_pain4.wav" );
	PRECACHE_SOUND( "trooper/tr_pain5.wav" );
	PRECACHE_SOUND( "trooper/tr_pain6.wav" );
	PRECACHE_SOUND( "trooper/tr_pain7.wav" );
	PRECACHE_SOUND( "trooper/tr_pain8.wav" );
	PRECACHE_SOUND( "trooper/tr_pain9.wav" );
	PRECACHE_SOUND( "trooper/tr_pain10.wav" );

	PRECACHE_SOUND( "hgrunt/gr_reload1.wav" );

	PRECACHE_SOUND( "weapons/sbarrel1.wav" );
	PRECACHE_SOUND( "weapons/dbarrel1.wav" );
	PRECACHE_SOUND( "weapons/xbow_fire1.wav" );
	PRECACHE_SOUND( "weapons/xbow_reload1.wav" );

	PRECACHE_SOUND("zombie/claw_miss2.wav");// because we use the basemonster SWIPE animation event

	// get voice pitch
	if (RANDOM_LONG(0,1))
		m_voicePitch = 109 + RANDOM_LONG(0,7);
	else
		m_voicePitch = 100;

	m_iShotgunShell = PRECACHE_MODEL ("models/shotgunshell.mdl");

	UTIL_PrecacheOther( "crossbow_bolt" );
}

//=========================================================
// start task
//=========================================================
void CTrooper :: StartTask ( Task_t *pTask )
{
	m_iTaskStatus = TASKSTATUS_RUNNING;

	switch ( pTask->iTask )
	{
	case TASK_TROOPER_CHECK_FIRE:
		if ( !NoFriendlyFire() )
		{
			SetConditions( bits_COND_TROOPER_NOFIRE );
		}
		TaskComplete();
		break;

	case TASK_TROOPER_SPEAK_SENTENCE:
		SpeakSentence();
		TaskComplete();
		break;

	case TASK_WALK_PATH:
	case TASK_RUN_PATH:
		// trooper no longer assumes he is covered if he moves
		Forget( bits_MEMORY_INCOVER );
		CSquadMonster ::StartTask( pTask );
		break;

	case TASK_RELOAD:
		m_IdealActivity = ACT_RELOAD;
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
void CTrooper :: RunTask ( Task_t *pTask )
{
	CSquadMonster :: RunTask( pTask );
}

//=========================================================
// PainSound
//=========================================================
void CTrooper :: PainSound ( void )
{
	if ( gpGlobals->time > m_flNextPainTime )
	{
		switch ( RANDOM_LONG(0,9) )
		{
		case 0:
			EMIT_SOUND( ENT(pev), CHAN_VOICE, "trooper/tr_pain1.wav", 1, ATTN_NORM );
			break;
		case 1:
			EMIT_SOUND( ENT(pev), CHAN_VOICE, "trooper/tr_pain2.wav", 1, ATTN_NORM );
			break;
		case 2:
			EMIT_SOUND( ENT(pev), CHAN_VOICE, "trooper/tr_pain3.wav", 1, ATTN_NORM );
			break;
		case 3:
			EMIT_SOUND( ENT(pev), CHAN_VOICE, "trooper/tr_pain4.wav", 1, ATTN_NORM );
			break;
		case 4:
			EMIT_SOUND( ENT(pev), CHAN_VOICE, "trooper/tr_pain5.wav", 1, ATTN_NORM );
			break;
		case 5:
			EMIT_SOUND( ENT(pev), CHAN_VOICE, "trooper/tr_pain6.wav", 1, ATTN_NORM );
			break;
		case 6:
			EMIT_SOUND( ENT(pev), CHAN_VOICE, "trooper/tr_pain7.wav", 1, ATTN_NORM );
			break;
		case 7:
			EMIT_SOUND( ENT(pev), CHAN_VOICE, "trooper/tr_pain8.wav", 1, ATTN_NORM );
			break;
		case 8:
			EMIT_SOUND( ENT(pev), CHAN_VOICE, "trooper/tr_pain9.wav", 1, ATTN_NORM );
			break;
		case 9:
			EMIT_SOUND( ENT(pev), CHAN_VOICE, "trooper/tr_pain10.wav", 1, ATTN_NORM );
			break;
		}

		m_flNextPainTime = gpGlobals->time + 1;
	}
}

//=========================================================
// DeathSound
//=========================================================
void CTrooper :: DeathSound ( void )
{
	switch ( RANDOM_LONG(0,2) )
	{
	case 0:
		EMIT_SOUND( ENT(pev), CHAN_VOICE, "hgrunt/gr_die1.wav", 1, ATTN_IDLE );
		break;
	case 1:
		EMIT_SOUND( ENT(pev), CHAN_VOICE, "hgrunt/gr_die2.wav", 1, ATTN_IDLE );
		break;
	case 2:
		EMIT_SOUND( ENT(pev), CHAN_VOICE, "hgrunt/gr_die3.wav", 1, ATTN_IDLE );
		break;
	}
}

//=========================================================
// AI Schedules Specific to this monster
//=========================================================

//=========================================================
// TrooperFail
//=========================================================
Task_t	tlTrooperFail[] =
{
	{ TASK_STOP_MOVING,			0				},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE },
	{ TASK_WAIT,				(float)2		},
	{ TASK_WAIT_PVS,			(float)0		},
};

Schedule_t	slTrooperFail[] =
{
	{
		tlTrooperFail,
		ARRAYSIZE ( tlTrooperFail ),
		bits_COND_CAN_RANGE_ATTACK1 |
		bits_COND_CAN_RANGE_ATTACK2 |
		bits_COND_CAN_MELEE_ATTACK1 |
		bits_COND_CAN_MELEE_ATTACK2,
		0,
		"Trooper Fail"
	},
};

//=========================================================
// Trooper Combat Fail
//=========================================================
Task_t	tlTrooperCombatFail[] =
{
	{ TASK_STOP_MOVING,			0				},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE },
	{ TASK_WAIT_FACE_ENEMY,		(float)2		},
	{ TASK_WAIT_PVS,			(float)0		},
};

Schedule_t	slTrooperCombatFail[] =
{
	{
		tlTrooperCombatFail,
		ARRAYSIZE ( tlTrooperCombatFail ),
		bits_COND_CAN_RANGE_ATTACK1	|
		bits_COND_CAN_RANGE_ATTACK2,
		0,
		"Trooper Combat Fail"
	},
};

//=========================================================
// Establish line of fire - move to a position that allows
// the trooper to attack.
//=========================================================
Task_t tlTrooperEstablishLineOfFire[] =
{
	{ TASK_SET_FAIL_SCHEDULE,	(float)SCHED_TROOPER_ELOF_FAIL	},
	{ TASK_GET_PATH_TO_ENEMY,	(float)0						},
	{ TASK_TROOPER_SPEAK_SENTENCE,(float)0						},
	{ TASK_RUN_PATH,			(float)0						},
	{ TASK_WAIT_FOR_MOVEMENT,	(float)0						},
};

Schedule_t slTrooperEstablishLineOfFire[] =
{
	{
		tlTrooperEstablishLineOfFire,
		ARRAYSIZE ( tlTrooperEstablishLineOfFire ),
		bits_COND_NEW_ENEMY			|
		bits_COND_ENEMY_DEAD		|
		bits_COND_CAN_RANGE_ATTACK1	|
		bits_COND_CAN_MELEE_ATTACK1	|
		bits_COND_CAN_RANGE_ATTACK2	|
		bits_COND_CAN_MELEE_ATTACK2	|
		bits_COND_HEAR_SOUND,

		bits_SOUND_DANGER,
		"TrooperEstablishLineOfFire"
	},
};

//=========================================================
// TrooperFoundEnemy - trooper established sight with an enemy
// that was hiding from the squad.
//=========================================================
Task_t	tlTrooperFoundEnemy[] =
{
	{ TASK_STOP_MOVING,				0							},
	{ TASK_FACE_ENEMY,				(float)0					},
};

Schedule_t	slTrooperFoundEnemy[] =
{
	{
		tlTrooperFoundEnemy,
		ARRAYSIZE ( tlTrooperFoundEnemy ),
		bits_COND_HEAR_SOUND,

		bits_SOUND_DANGER,
		"TrooperFoundEnemy"
	},
};

//=========================================================
// TrooperCombatFace Schedule
//=========================================================
Task_t	tlTrooperCombatFace1[] =
{
	{ TASK_STOP_MOVING,				0							},
	{ TASK_SET_ACTIVITY,			(float)ACT_IDLE				},
	{ TASK_FACE_ENEMY,				(float)0					},
	{ TASK_WAIT,					(float)1.5					},
	{ TASK_SET_SCHEDULE,			(float)SCHED_TROOPER_SWEEP	},
};

Schedule_t	slTrooperCombatFace[] =
{
	{
		tlTrooperCombatFace1,
		ARRAYSIZE ( tlTrooperCombatFace1 ),
		bits_COND_NEW_ENEMY				|
		bits_COND_ENEMY_DEAD			|
		bits_COND_CAN_RANGE_ATTACK1		|
		bits_COND_CAN_RANGE_ATTACK2,
		0,
		"Combat Face"
	},
};

//=========================================================
// Suppressing fire - don't stop shooting until the clip is
// empty or trooper gets hurt.
//=========================================================
Task_t	tlTrooperSignalSuppress[] =
{
	{ TASK_STOP_MOVING,					0						},
	{ TASK_FACE_IDEAL,					(float)0				},
	{ TASK_FACE_ENEMY,					(float)0				},
	{ TASK_TROOPER_CHECK_FIRE,			(float)0				},
	{ TASK_RANGE_ATTACK1,				(float)0				},
	{ TASK_FACE_ENEMY,					(float)0				},
	{ TASK_TROOPER_CHECK_FIRE,			(float)0				},
	{ TASK_RANGE_ATTACK1,				(float)0				},
	{ TASK_FACE_ENEMY,					(float)0				},
	{ TASK_TROOPER_CHECK_FIRE,			(float)0				},
	{ TASK_RANGE_ATTACK1,				(float)0				},
	{ TASK_FACE_ENEMY,					(float)0				},
	{ TASK_TROOPER_CHECK_FIRE,			(float)0				},
	{ TASK_RANGE_ATTACK1,				(float)0				},
	{ TASK_FACE_ENEMY,					(float)0				},
	{ TASK_TROOPER_CHECK_FIRE,			(float)0				},
	{ TASK_RANGE_ATTACK1,				(float)0				},
};

Schedule_t	slTrooperSignalSuppress[] =
{
	{
		tlTrooperSignalSuppress,
		ARRAYSIZE ( tlTrooperSignalSuppress ),
		bits_COND_ENEMY_DEAD		|
		bits_COND_LIGHT_DAMAGE		|
		bits_COND_HEAVY_DAMAGE		|
		bits_COND_HEAR_SOUND		|
		bits_COND_TROOPER_NOFIRE	|
		bits_COND_NO_AMMO_LOADED,

		bits_SOUND_DANGER,
		"SignalSuppress"
	},
};

Task_t	tlTrooperSuppress[] =
{
	{ TASK_STOP_MOVING,			0							},
	{ TASK_FACE_ENEMY,			(float)0					},
	{ TASK_TROOPER_CHECK_FIRE,	(float)0					},
	{ TASK_RANGE_ATTACK1,		(float)0					},
	{ TASK_FACE_ENEMY,			(float)0					},
	{ TASK_TROOPER_CHECK_FIRE,	(float)0					},
	{ TASK_RANGE_ATTACK1,		(float)0					},
	{ TASK_FACE_ENEMY,			(float)0					},
	{ TASK_TROOPER_CHECK_FIRE,	(float)0					},
	{ TASK_RANGE_ATTACK1,		(float)0					},
	{ TASK_FACE_ENEMY,			(float)0					},
	{ TASK_TROOPER_CHECK_FIRE,	(float)0					},
	{ TASK_RANGE_ATTACK1,		(float)0					},
	{ TASK_FACE_ENEMY,			(float)0					},
	{ TASK_TROOPER_CHECK_FIRE,	(float)0					},
	{ TASK_RANGE_ATTACK1,		(float)0					},
};

Schedule_t	slTrooperSuppress[] =
{
	{
		tlTrooperSuppress,
		ARRAYSIZE ( tlTrooperSuppress ),
		bits_COND_ENEMY_DEAD		|
		bits_COND_LIGHT_DAMAGE		|
		bits_COND_HEAVY_DAMAGE		|
		bits_COND_HEAR_SOUND		|
		bits_COND_TROOPER_NOFIRE	|
		bits_COND_NO_AMMO_LOADED,

		bits_SOUND_DANGER,
		"Suppress"
	},
};


//=========================================================
// trooper wait in cover - we don't allow danger or the ability
// to attack to break a trooper's run to cover schedule, but
// when a trooper is in cover, we do want them to attack if they can.
//=========================================================
Task_t	tlTrooperWaitInCover[] =
{
	{ TASK_STOP_MOVING,				(float)0					},
	{ TASK_SET_ACTIVITY,			(float)ACT_IDLE				},
	{ TASK_WAIT_FACE_ENEMY,			(float)1					},
};

Schedule_t	slTrooperWaitInCover[] =
{
	{
		tlTrooperWaitInCover,
		ARRAYSIZE ( tlTrooperWaitInCover ),
		bits_COND_NEW_ENEMY			|
		bits_COND_HEAR_SOUND		|
		bits_COND_CAN_RANGE_ATTACK1	|
		bits_COND_CAN_RANGE_ATTACK2	|
		bits_COND_CAN_MELEE_ATTACK1	|
		bits_COND_CAN_MELEE_ATTACK2,

		bits_SOUND_DANGER,
		"TrooperWaitInCover"
	},
};

//=========================================================
// run to cover.
// !!!BUGBUG - set a decent fail schedule here.
//=========================================================
Task_t	tlTrooperTakeCover1[] =
{
	{ TASK_STOP_MOVING,				(float)0							},
	{ TASK_SET_FAIL_SCHEDULE,		(float)SCHED_TROOPER_TAKECOVER_FAILED	},
	{ TASK_WAIT,					(float)0.2							},
	{ TASK_FIND_COVER_FROM_ENEMY,	(float)0							},
	{ TASK_TROOPER_SPEAK_SENTENCE,	(float)0							},
	{ TASK_RUN_PATH,				(float)0							},
	{ TASK_WAIT_FOR_MOVEMENT,		(float)0							},
	{ TASK_REMEMBER,				(float)bits_MEMORY_INCOVER			},
	{ TASK_SET_SCHEDULE,			(float)SCHED_TROOPER_WAIT_FACE_ENEMY	},
};

Schedule_t	slTrooperTakeCover[] =
{
	{
		tlTrooperTakeCover1,
		ARRAYSIZE ( tlTrooperTakeCover1 ),
		0,
		0,
		"TakeCover"
	},
};

//=========================================================
// Trooper reload schedule
//=========================================================
Task_t	tlTrooperHideReload[] =
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

Schedule_t slTrooperHideReload[] =
{
	{
		tlTrooperHideReload,
		ARRAYSIZE ( tlTrooperHideReload ),
		bits_COND_HEAVY_DAMAGE	|
		bits_COND_HEAR_SOUND,

		bits_SOUND_DANGER,
		"TrooperHideReload"
	}
};

//=========================================================
// Do a turning sweep of the area
//=========================================================
Task_t	tlTrooperSweep[] =
{
	{ TASK_TURN_LEFT,			(float)179	},
	{ TASK_WAIT,				(float)1	},
	{ TASK_TURN_LEFT,			(float)179	},
	{ TASK_WAIT,				(float)1	},
};

Schedule_t	slTrooperSweep[] =
{
	{
		tlTrooperSweep,
		ARRAYSIZE ( tlTrooperSweep ),

		bits_COND_NEW_ENEMY		|
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE	|
		bits_COND_CAN_RANGE_ATTACK1	|
		bits_COND_CAN_RANGE_ATTACK2	|
		bits_COND_HEAR_SOUND,

		bits_SOUND_WORLD		|// sound flags
		bits_SOUND_DANGER		|
		bits_SOUND_PLAYER,

		"Trooper Sweep"
	},
};

//=========================================================
// primary range attack. Overriden because base class stops attacking when the enemy is occluded.
//=========================================================
Task_t	tlTrooperRangeAttack1A[] =
{
	{ TASK_STOP_MOVING,			(float)0		},
	{ TASK_PLAY_SEQUENCE_FACE_ENEMY,		(float)ACT_CROUCH },
	{ TASK_TROOPER_CHECK_FIRE,	(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_TROOPER_CHECK_FIRE,	(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_TROOPER_CHECK_FIRE,	(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_TROOPER_CHECK_FIRE,	(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
};

Schedule_t	slTrooperRangeAttack1A[] =
{
	{
		tlTrooperRangeAttack1A,
		ARRAYSIZE ( tlTrooperRangeAttack1A ),
		bits_COND_NEW_ENEMY			|
		bits_COND_ENEMY_DEAD		|
		bits_COND_HEAVY_DAMAGE		|
		bits_COND_ENEMY_OCCLUDED	|
		bits_COND_HEAR_SOUND		|
		bits_COND_TROOPER_NOFIRE	|
		bits_COND_NO_AMMO_LOADED,

		bits_SOUND_DANGER,
		"Range Attack1A"
	},
};


//=========================================================
// primary range attack. Overriden because base class stops attacking when the enemy is occluded.
//=========================================================
Task_t	tlTrooperRangeAttack1B[] =
{
	{ TASK_STOP_MOVING,				(float)0		},
	{ TASK_PLAY_SEQUENCE_FACE_ENEMY,(float)ACT_IDLE_ANGRY  },
	{ TASK_TROOPER_CHECK_FIRE,	(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_TROOPER_CHECK_FIRE,	(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_TROOPER_CHECK_FIRE,	(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_TROOPER_CHECK_FIRE,	(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
};

Schedule_t	slTrooperRangeAttack1B[] =
{
	{
		tlTrooperRangeAttack1B,
		ARRAYSIZE ( tlTrooperRangeAttack1B ),
		bits_COND_NEW_ENEMY			|
		bits_COND_ENEMY_DEAD		|
		bits_COND_HEAVY_DAMAGE		|
		bits_COND_ENEMY_OCCLUDED	|
		bits_COND_NO_AMMO_LOADED	|
		bits_COND_TROOPER_NOFIRE	|
		bits_COND_HEAR_SOUND,

		bits_SOUND_DANGER,
		"Range Attack1B"
	},
};


DEFINE_CUSTOM_SCHEDULES( CTrooper )
{
	slTrooperFail,
	slTrooperCombatFail,
	slTrooperEstablishLineOfFire,
	slTrooperFoundEnemy,
	slTrooperCombatFace,
	slTrooperSignalSuppress,
	slTrooperSuppress,
	slTrooperWaitInCover,
	slTrooperTakeCover,
	slTrooperHideReload,
	slTrooperSweep,
	slTrooperRangeAttack1A,
	slTrooperRangeAttack1B,
};

IMPLEMENT_CUSTOM_SCHEDULES( CTrooper, CSquadMonster );

//=========================================================
// SetActivity
//=========================================================
void CTrooper :: SetActivity ( Activity NewActivity )
{
	int	iSequence = ACTIVITY_NOT_AVAILABLE;
	void *pmodel = GET_MODEL_PTR( ENT(pev) );

	switch ( NewActivity)
	{
	case ACT_RANGE_ATTACK1:
		// trooper is either shooting standing or shooting crouched
		if ( m_fStanding )
		{
			// get aimable sequence
			iSequence = LookupSequence( "standing_shotgun" );
		}
		else
		{
			// get crouching shoot
			iSequence = LookupSequence( "crouching_shotgun" );
		}
		break;
	case ACT_RANGE_ATTACK2:
		break;
	case ACT_RUN:
		if ( pev->health <= TROOPER_LIMP_HEALTH )
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
		if ( pev->health <= TROOPER_LIMP_HEALTH )
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
		ALERT ( at_console, "%s has no sequence for act:%d\n", STRING(pev->classname), NewActivity );
		pev->sequence		= 0;	// Set to the reset anim (if it's there)
	}
}

//=========================================================
// Get Schedule!
//=========================================================
Schedule_t *CTrooper :: GetSchedule( void )
{

	// clear old sentence
	m_iSentence = TROOPER_SENT_NONE;

	// troopers place HIGH priority on running away from danger sounds.
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
					SENTENCEG_PlayRndSz( ENT(pev), "T_GREN", TROOPER_SENTENCE_VOLUME, TROOPER_ATTN, 0, m_voicePitch);
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
						if (FOkToSpeak())
						{
							if (m_hEnemy != NULL)
								SENTENCEG_PlayRndSz( ENT(pev), "T_ALERT", TROOPER_SENTENCE_VOLUME, TROOPER_ATTN, 0, m_voicePitch);

							JustSpoke();
						}

						if ( HasConditions ( bits_COND_CAN_RANGE_ATTACK1 ) )
						{
							return GetScheduleOfType ( SCHED_TROOPER_SUPPRESS );
						}
						else
						{
							return GetScheduleOfType ( SCHED_TROOPER_ESTABLISH_LINE_OF_FIRE );
						}
					}
				}
			}
// no ammo
			else if ( HasConditions ( bits_COND_NO_AMMO_LOADED ) )
			{
				return GetScheduleOfType ( SCHED_TROOPER_COVER_AND_RELOAD );
			}

// damaged just a little
			else if ( HasConditions( bits_COND_LIGHT_DAMAGE ) )
			{
				// if hurt:
				// 30% chance of taking cover
				// 70% chance of flinch.
				int iPercent = RANDOM_LONG(0,99);

				if ( iPercent <= 30 && m_hEnemy != NULL )
				{
					// only try to take cover if we actually have an enemy!
					if (FOkToSpeak())
					{
						m_iSentence = TROOPER_SENT_ALERT;
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
// can shoot
			else if ( HasConditions ( bits_COND_CAN_RANGE_ATTACK1 ) )
			{
				if ( InSquad() )
				{
					// if the enemy has eluded the squad and a squad member has just located the enemy
					// and the enemy does not see the squad member, issue a call to the squad to waste a
					// little time and give the player a chance to turn.
					if ( MySquadLeader()->m_fEnemyEluded && !HasConditions ( bits_COND_ENEMY_FACING_ME ) )
					{
						MySquadLeader()->m_fEnemyEluded = FALSE;
						return GetScheduleOfType ( SCHED_TROOPER_FOUND_ENEMY );
					}
				}

				if ( OccupySlot ( bits_SLOTS_HGRUNT_ENGAGE ) )
				{
					// try to take an available ENGAGE slot
					return GetScheduleOfType( SCHED_RANGE_ATTACK1 );
				}
				else
				{
					// hide!
					return GetScheduleOfType( SCHED_TAKE_COVER_FROM_ENEMY );
				}
			}
// can't see enemy
			else if ( HasConditions( bits_COND_ENEMY_OCCLUDED ) )
			{
				if ( OccupySlot( bits_SLOTS_HGRUNT_ENGAGE ) )
				{
					if (FOkToSpeak())
					{
						m_iSentence = TROOPER_SENT_TAUNT;
					}

					return GetScheduleOfType( SCHED_TROOPER_ESTABLISH_LINE_OF_FIRE );
				}
				else
				{
					if (FOkToSpeak() && RANDOM_LONG(0,1))
					{
						SENTENCEG_PlayRndSz( ENT(pev), "T_TAUNT", TROOPER_SENTENCE_VOLUME, TROOPER_ATTN, 0, m_voicePitch);
						JustSpoke();
					}
					return GetScheduleOfType( SCHED_STANDOFF );
				}
			}

			if ( HasConditions( bits_COND_SEE_ENEMY ) && !HasConditions ( bits_COND_CAN_RANGE_ATTACK1 ) )
			{
				return GetScheduleOfType ( SCHED_TROOPER_ESTABLISH_LINE_OF_FIRE );
			}
		}
	}

	// no special cases here, call the base class
	return CSquadMonster :: GetSchedule();
}

//=========================================================
//=========================================================
Schedule_t* CTrooper :: GetScheduleOfType ( int Type )
{
	switch	( Type )
	{
	case SCHED_TAKE_COVER_FROM_ENEMY:
		{
			return &slTrooperTakeCover[ 0 ];
		}
	case SCHED_TROOPER_TAKECOVER_FAILED:
		{
			if ( HasConditions( bits_COND_CAN_RANGE_ATTACK1 ) && OccupySlot( bits_SLOTS_HGRUNT_ENGAGE ) )
			{
				return GetScheduleOfType( SCHED_RANGE_ATTACK1 );
			}

			return GetScheduleOfType ( SCHED_FAIL );
		}
		break;
	case SCHED_TROOPER_ELOF_FAIL:
		{
			// trooper is unable to move to a position that allows him to attack the enemy.
			return GetScheduleOfType ( SCHED_TAKE_COVER_FROM_ENEMY );
		}
		break;
	case SCHED_TROOPER_ESTABLISH_LINE_OF_FIRE:
		{
			return &slTrooperEstablishLineOfFire[ 0 ];
		}
		break;
	case SCHED_RANGE_ATTACK1:
		{
			// randomly stand or crouch
			if (RANDOM_LONG(0,9) == 0)
				m_fStanding = RANDOM_LONG(0,1);

			if (m_fStanding)
				return &slTrooperRangeAttack1B[ 0 ];
			else
				return &slTrooperRangeAttack1A[ 0 ];
		}
	case SCHED_COMBAT_FACE:
		{
			return &slTrooperCombatFace[ 0 ];
		}
	case SCHED_TROOPER_WAIT_FACE_ENEMY:
		{
			return &slTrooperWaitInCover[ 0 ];
		}
	case SCHED_TROOPER_SWEEP:
		{
			return &slTrooperSweep[ 0 ];
		}
	case SCHED_TROOPER_COVER_AND_RELOAD:
		{
			return &slTrooperHideReload[ 0 ];
		}
	case SCHED_TROOPER_FOUND_ENEMY:
		{
			return &slTrooperFoundEnemy[ 0 ];
		}
	case SCHED_TROOPER_SUPPRESS:
		{
			if ( m_hEnemy->IsPlayer() && m_fFirstEncounter )
			{
				m_fFirstEncounter = FALSE;// after first encounter, leader won't issue handsigns anymore when he has a new enemy
				return &slTrooperSignalSuppress[ 0 ];
			}
			else
			{
				return &slTrooperSuppress[ 0 ];
			}
		}
	case SCHED_FAIL:
		{
			if ( m_hEnemy != NULL )
			{
				// trooper has an enemy, so pick a different default fail schedule most likely to help recover.
				return &slTrooperCombatFail[ 0 ];
			}

			return &slTrooperFail[ 0 ];
		}
	default:
		{
			return CSquadMonster :: GetScheduleOfType ( Type );
		}
	}
}

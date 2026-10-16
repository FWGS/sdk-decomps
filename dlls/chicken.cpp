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
// Wanted: the chicken (monster_chicken). Started from the SDK
// cockroach (roach.cpp): it wanders about, pecks, clucks, runs
// screaming from what it fears and leaves a puff of feathers when
// killed; a nonzero "gender" makes a rooster (body group 1) that
// crows now and then. CChick is retail's name (its MonsterThink is
// exported); the file name, the mode and event names and the member
// names are ours.
//=========================================================

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"schedule.h"
#include	"soundent.h"

// what the chicken is doing (names ours)
#define		CHICK_IDLE				0
#define		CHICK_SCARED			1	// running from what it fears
#define		CHICK_SMELL_FOOD		2	// the roach's; nothing sets it
#define		CHICK_EAT				3
#define		CHICK_ATTACK			4	// pecking at its enemy; nothing in this file starts it

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define		CHICK_AE_PECK			0	// name ours

class CChick : public CBaseMonster
{
public:
	void Spawn( void );
	void Precache( void );
	void KeyValue( KeyValueData *pkvd );
	void SetYawSpeed( void );
	void EXPORT MonsterThink ( void );
	void Move ( float flInterval );
	void PickNewDest ( int iCondition );
	void Killed( entvars_t *pevAttacker, int iGib );
	void HandleAnimEvent( MonsterEvent_t *pEvent );

	float	m_flNextSmellTime;	// set in Spawn, never read
	float	m_flLastLightLevel;	// never used
	int		Classify ( void );
	void	Look ( int iDistance );
	int		ISoundMask ( void );

	int		m_iGender;			// "gender": nonzero is a rooster
	int		m_iMode;
	float	m_flFleeTime;		// when a scared or attacking chicken calms down
	float	m_flNextScream;
};
LINK_ENTITY_TO_CLASS( monster_chicken, CChick );

//=========================================================
// Classify - indicates this monster's place in the
// relationship table.
//=========================================================
int	CChick :: Classify ( void )
{
	return CLASS_INSECT;
}

//=========================================================
// ISoundMask - returns a bit mask indicating which types
// of sounds this monster regards.
//=========================================================
int CChick :: ISoundMask ( void )
{
	return	bits_SOUND_MEAT;
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CChick :: SetYawSpeed ( void )
{
	int ys;

	ys = 120;

	pev->yaw_speed = ys;
}

//=========================================================
// Spawn
//=========================================================
void CChick :: Spawn()
{
	Precache( );

	SET_MODEL(ENT(pev), "models/chicken.mdl");
	UTIL_SetSize( pev, Vector( -16, -16, 0 ), Vector( 16, 16, 26 ) );

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= BLOOD_COLOR_RED;
	pev->effects		= 0;
	pev->health			= gSkillData.chickenHealth;
	m_flFieldOfView		= 0.5;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState		= MONSTERSTATE_NONE;

	MonsterInit();
	SetActivity ( ACT_IDLE );

	pev->takedamage		= DAMAGE_YES;
	m_iMode				= CHICK_IDLE;
	m_flNextSmellTime	= gpGlobals->time;
	SetBodygroup( 1, m_iGender );
	m_flNextScream		= 0;
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CChick :: Precache()
{
	PRECACHE_MODEL("models/chicken.mdl");
	PRECACHE_MODEL("models/feathers.mdl");

	PRECACHE_SOUND("common/null.wav");
	PRECACHE_SOUND("chicken/chick_cluck.wav");
	PRECACHE_SOUND("chicken/chick_attack.wav");

	for ( int i = 1; i <= 5; i++ )
	{
		char szSound[128];

		sprintf( szSound, "chicken/chick_scream%i.wav", i );
		PRECACHE_SOUND( (char *)STRING( ALLOC_STRING( szSound ) ) );
	}
}

//=========================================================
// Killed: the chicken turns into a puff of feathers that
// falls and is removed 20 seconds later.
//=========================================================
void CChick :: Killed( entvars_t *pevAttacker, int iGib )
{
	pev->solid = SOLID_NOT;

	EMIT_SOUND_DYN(ENT(pev), CHAN_BODY, "common/null.wav", 1, 0.8, 0, 100 );

	CBaseEntity *pOwner = CBaseEntity::Instance(pev->owner);
	if ( pOwner )
	{
		pOwner->DeathNotice( pev );
	}

	SET_MODEL(ENT(pev), "models/feathers.mdl");
	UTIL_SetSize( pev, Vector( -1, -1, -1 ), Vector( 1, 1, 1 ) );
	ClearBits( pev->flags, FL_ONGROUND | FL_MONSTER );
	pev->movetype = MOVETYPE_TOSS;
	pev->solid = SOLID_NOT;
	pev->gravity = 0.7;
	pev->sequence = 0;
	pev->frame = 0;
	pev->deadflag = DEAD_DEAD;
	ResetSequenceInfo( );
	FCheckAITrigger();

	SetThink( SUB_Remove );
	pev->nextthink = gpGlobals->time + 20;
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CChick :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	switch ( pEvent->event )
	{
	case CHICK_AE_PECK:
		{
			// peck from a little higher up
			pev->origin.z += 5;
			CBaseEntity *pHurt = CheckTraceHullAttack( 40, gSkillData.chickenDmgPeck, DMG_SLASH );
			pev->origin.z -= 5;

			if ( pHurt && FBitSet( pHurt->pev->flags, FL_MONSTER | FL_CLIENT ) )
			{
				pHurt->pev->punchangle.z = RANDOM_FLOAT( 0, 1 );
				pHurt->pev->punchangle.y = RANDOM_FLOAT( -1, 1 );
				pHurt->pev->punchangle.x = -5;
			}
		}
		break;
	}
}

//=========================================================
// MonsterThink, overridden for chickens.
//=========================================================
void CChick :: MonsterThink( void  )
{
	if ( FNullEnt( FIND_CLIENT_IN_PVS( edict() ) ) )
		pev->nextthink = gpGlobals->time + RANDOM_FLOAT(1,1.5);
	else
		pev->nextthink = gpGlobals->time + 0.1;// keep monster thinking

	float flInterval = StudioFrameAdvance( ); // animate

	Look( 100 );

	switch ( m_iMode )
	{
	case	CHICK_ATTACK:
		{
			if ( !HasConditions( bits_COND_SEE_FEAR ) && m_flFleeTime < gpGlobals->time )
			{
				m_iMode = CHICK_IDLE;
				SetActivity ( ACT_IDLE );
				m_hEnemy = NULL;
				break;
			}

			m_flFleeTime = gpGlobals->time + 10;

			if ( m_hEnemy != NULL && ( pev->origin - m_hEnemy->pev->origin ).Length2D() < 38 )
			{
				if ( m_Activity != ACT_MELEE_ATTACK1 || m_fSequenceFinished )
				{
					if ( RANDOM_LONG(0,3) == 0 )
						EMIT_SOUND(ENT(pev), CHAN_BODY, "chicken/chick_attack.wav", 0.5, 0.8 );

					Vector vecDir = ( m_vecEnemyLKP - pev->origin ).Normalize();
					pev->angles.y = UTIL_VecToAngles( vecDir ).y;
					SetActivity ( ACT_MELEE_ATTACK1 );
				}
			}
			else if ( m_Activity != ACT_RUN )
			{
				PickNewDest( CHICK_ATTACK );
				SetActivity ( ACT_RUN );
			}
			break;
		}
	case	CHICK_SCARED:
		{
			if ( !HasConditions( bits_COND_SEE_FEAR ) && m_flFleeTime < gpGlobals->time )
			{
				m_iMode = CHICK_IDLE;
				m_hEnemy = NULL;
			}
			else if ( m_flNextScream < gpGlobals->time )
			{
				char szSound[128];

				m_flNextScream = gpGlobals->time + RANDOM_FLOAT( 0, 1 ) + 1.0;
				sprintf( szSound, "chicken/chick_scream%i.wav", RANDOM_LONG( 1, 5 ) );
				EMIT_SOUND(ENT(pev), CHAN_BODY, szSound, 0.5, 0.8 );
			}
			break;
		}
	case	CHICK_IDLE:
	case	CHICK_EAT:
		{
			if ( HasConditions( bits_COND_SEE_FEAR ) )
			{
				// if see something scary
				Eat( 30 +  ( RANDOM_LONG(0,14) ) );// chicken will ignore food for 30 to 45 seconds
				PickNewDest( CHICK_SCARED );
				SetActivity ( ACT_RUN );
				m_flFleeTime = gpGlobals->time + 5;
			}
			else if ( m_fSequenceFinished )
			{
				if ( m_iMode == CHICK_IDLE && m_Activity == ACT_IDLE )
				{
					if ( RANDOM_LONG(0,3) == 0 )
					{
						PickNewDest( CHICK_IDLE );
						SetActivity ( ACT_WALK );
						ClearBits( pev->flags, FL_ONGROUND );
					}
					else
					{
						if ( RANDOM_LONG(0,3) == 0 )
							EMIT_SOUND_DYN(ENT(pev), CHAN_BODY, "chicken/chick_cluck.wav", 0.5, 0.8, 0, 100 + RANDOM_LONG(-10,10) );

						SetActivity ( ACT_IDLE );

						if ( RANDOM_LONG(0,10) == 0 && m_iGender )
						{
							// the rooster crows
							pev->sequence = LookupSequence( "chicken_crow" );
							if ( pev->sequence != -1 )
							{
								EMIT_SOUND(ENT(pev), CHAN_BODY, "common/null.wav", 1, 0.8 );
								ResetSequenceInfo( );
							}
							else
							{
								SetActivity ( ACT_IDLE );
							}
						}
					}
				}
				else if ( m_iMode == CHICK_EAT )
				{
					if ( RANDOM_LONG(0,60) == 0 )
					{
						m_iMode = CHICK_IDLE;
						PickNewDest( CHICK_IDLE );
						SetActivity ( ACT_WALK );
						Eat( 30 +  ( RANDOM_LONG(0,14) ) );
					}
					else
					{
						SetActivity ( ACT_MELEE_ATTACK1 );
					}
				}
			}
			break;
		}
	}

	if ( m_flGroundSpeed != 0 )
	{
		Move( flInterval );
	}
}

//=========================================================
// Picks a new spot for the chicken to run to.
//=========================================================
void CChick :: PickNewDest ( int iCondition )
{
	Vector	vecNewDir;
	Vector	vecDest;
	float	flDist;
	int		i;

	m_iMode = iCondition;

	switch ( m_iMode )
	{
	case CHICK_SMELL_FOOD:
		{
			// find the food and go there.
			CSound *pSound;

			pSound = CSoundEnt::SoundPointerForIndex( m_iAudibleList );

			if ( pSound )
			{
				m_Route[ 0 ].vecLocation.x = pSound->m_vecOrigin.x + ( 3 - RANDOM_LONG(0,5) );
				m_Route[ 0 ].vecLocation.y = pSound->m_vecOrigin.y + ( 3 - RANDOM_LONG(0,5) );
				m_Route[ 0 ].vecLocation.z = pSound->m_vecOrigin.z;
				m_Route[ 0 ].iType = bits_MF_TO_LOCATION;
				m_movementGoal = RouteClassify( m_Route[ 0 ].iType );
				return;
			}
		}
		// no food: wander
	case CHICK_IDLE:
		i = 0;
		do
		{
			// a random spot up to 400 units away that can be walked to;
			// after 31 tries the last one is taken anyway
			vecNewDir.x = RANDOM_FLOAT( -1, 1 );
			vecNewDir.y = RANDOM_FLOAT( -1, 1 );	// sic: z is never set (as the roach's)
			flDist		= RANDOM_LONG( 128, 400 );
			vecDest = pev->origin + vecNewDir * flDist;
			i++;
		} while ( !CheckLocalMove( pev->origin, vecDest, m_hEnemy, NULL ) && i <= 30 );
		break;

	case CHICK_SCARED:
		i = 0;
		do
		{
			// away from the enemy, give or take 80 degrees
			Vector vecDir = ( pev->origin - m_vecEnemyLKP ).Normalize();
			Vector vecAngles = UTIL_VecToAngles( vecDir );
			vecAngles.y += RANDOM_FLOAT( -80, 80 );
			MAKE_VECTORS( vecAngles );
			vecNewDir = gpGlobals->v_forward;
			flDist		= RANDOM_LONG( 128, 600 );
			vecDest = pev->origin + vecNewDir * flDist;
			i++;
		} while ( !CheckLocalMove( pev->origin, vecDest, m_hEnemy, NULL ) && i <= 30 );
		break;

	case CHICK_ATTACK:
		vecDest = m_hEnemy->pev->origin;	// sic: no check for a lost enemy
		break;
	}

	// sic: any other mode leaves vecDest unset

	m_Route[ 0 ].vecLocation.x = vecDest.x;
	m_Route[ 0 ].vecLocation.y = vecDest.y;
	m_Route[ 0 ].vecLocation.z = pev->origin.z;
	m_Route[ 0 ].iType = bits_MF_TO_LOCATION;
	m_movementGoal = RouteClassify( m_Route[ 0 ].iType );
}

//=========================================================
// chicken's move function
//=========================================================
void CChick :: Move ( float flInterval )
{
	float		flWaypointDist;

	if ( !m_Route[ m_iRouteIndex ].iType )
		return;

	// local move to waypoint.
	flWaypointDist = ( m_Route[ m_iRouteIndex ].vecLocation - pev->origin ).Length2D();
	MakeIdealYaw ( m_Route[ m_iRouteIndex ].vecLocation );

	ChangeYaw ( pev->yaw_speed );
	UTIL_MakeVectors( pev->angles );

	// blocked 30 units ahead: pick a new spot to run off to
	if ( !CheckLocalMove( pev->origin, pev->origin + gpGlobals->v_forward * 30, m_hEnemy, NULL ) )
	{
		PickNewDest( m_iMode );
	}

	WALK_MOVE( ENT(pev), pev->ideal_yaw, m_flGroundSpeed * flInterval, WALKMOVE_NORMAL );

	// if the waypoint is closer than step size, then stop after next step
	if ( flWaypointDist <= m_flGroundSpeed * flInterval )
	{
		RouteClear();

		switch ( m_iMode )
		{
		case CHICK_SCARED:
			PickNewDest( CHICK_SCARED );
			SetActivity ( ACT_RUN );
			break;
		case CHICK_ATTACK:
			PickNewDest( CHICK_ATTACK );
			SetActivity ( ACT_RUN );
			break;
		}

		if ( m_iMode == CHICK_IDLE )
		{
			SetActivity ( ACT_IDLE );
		}
	}

	if ( RANDOM_LONG(0,149) == 1 && m_iMode != CHICK_SMELL_FOOD )
	{
		// random wander while moving
		PickNewDest( CHICK_IDLE );
	}
}

//=========================================================
// Look - overriden for the chicken, which can see 360
// degrees and takes what it fears as its enemy.
//=========================================================
void CChick :: Look ( int iDistance )
{
	CBaseEntity	*pSightEnt = NULL;// the current visible entity that we're dealing with
	CBaseEntity	*pPreviousEnt;// the last entity added to the link list
	int			iSighted = 0;

	// DON'T let visibility information from last frame sit around!
	ClearConditions( bits_COND_SEE_HATE |bits_COND_SEE_DISLIKE | bits_COND_SEE_ENEMY | bits_COND_SEE_FEAR );

	// don't let monsters outside of the player's PVS act up, or most of the interesting
	// things will happen before the player gets there!
	if ( FNullEnt( FIND_CLIENT_IN_PVS( edict() ) ) )
	{
		return;
	}

	m_pLink = NULL;
	pPreviousEnt = this;

	// Examine all entities within a reasonable radius
	while ((pSightEnt = UTIL_FindEntityInSphere( pSightEnt, pev->origin, iDistance )) != NULL)
	{
		// only consider ents that can be damaged. !!!temporarily only considering other monsters and clients
		if (  pSightEnt->IsPlayer() || FBitSet ( pSightEnt->pev->flags, FL_MONSTER ) )
		{
			if ( FVisible( pSightEnt ) && !FBitSet( pSightEnt->pev->flags, FL_NOTARGET ) && pSightEnt->pev->health > 0 )
			{
				// NULL the Link pointer for each ent added to the link list. If other ents follow, the will overwrite
				// this value. If this ent happens to be the last, the list will be properly terminated.
				pPreviousEnt->m_pLink = pSightEnt;
				pSightEnt->m_pLink = NULL;
				pPreviousEnt = pSightEnt;

				switch ( IRelationship ( pSightEnt ) )
				{
				case	R_FR:
					m_vecEnemyLKP = pSightEnt->pev->origin;
					m_hEnemy = pSightEnt;
					iSighted |= bits_COND_SEE_FEAR;
					break;
				case	R_NO:
					break;
				default:
					ALERT ( at_console, "%s can't asses %s\n", STRING(pev->classname), STRING(pSightEnt->pev->classname ) );
					break;
				}
			}
		}
	}
	SetConditions( iSighted );
}

//=========================================================
// KeyValue
//=========================================================
void CChick :: KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "gender"))
	{
		m_iGender = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CBaseMonster::KeyValue( pkvd );
}

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
// fakegordon.cpp - T2DEF's fake Gordon, a copy of gman.cpp
// (file name ours)
//=========================================================
#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"schedule.h"
#include	"weapons.h"
#include	"talkmonster.h"

extern int g_iT2DefSpeaker;

//=========================================================
// Monster's Anim Events Go Here
//=========================================================

class CFakeGordon : public CBaseMonster
{
public:
	void Spawn( void );
	void SetYawSpeed( void );
	int  Classify ( void );
	void HandleAnimEvent( MonsterEvent_t *pEvent );
	int ISoundMask ( void );
	void MonsterThink( void );

	int	Save( CSave &save );
	int Restore( CRestore &restore );
	static TYPEDESCRIPTION m_SaveData[];

	void StartTask( Task_t *pTask );
	void RunTask( Task_t *pTask );

	void PlayScriptedSentence( const char *pszSentence, float duration, float volume, float attenuation, BOOL bConcurrent, CBaseEntity *pListener );

	EHANDLE m_hRealGordon;
	EHANDLE m_hTalkTarget;
	float m_flTalkTime;
};
LINK_ENTITY_TO_CLASS( monster_fake_gordon, CFakeGordon );


TYPEDESCRIPTION	CFakeGordon::m_SaveData[] =
{
	DEFINE_FIELD( CFakeGordon, m_hRealGordon, FIELD_EHANDLE ),
	DEFINE_FIELD( CFakeGordon, m_hTalkTarget, FIELD_EHANDLE ),
	DEFINE_FIELD( CFakeGordon, m_flTalkTime, FIELD_TIME ),
};
IMPLEMENT_SAVERESTORE( CFakeGordon, CBaseMonster );


//=========================================================
// Classify - indicates this monster's place in the
// relationship table.
//=========================================================
int	CFakeGordon :: Classify ( void )
{
	return m_iClass?m_iClass:CLASS_NONE;
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CFakeGordon :: SetYawSpeed ( void )
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
void CFakeGordon :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	switch( pEvent->event )
	{
	case 1:
		SetNextThink( 6 );
		break;
	default:
		CBaseMonster::HandleAnimEvent( pEvent );
		break;
	}
}

//=========================================================
// ISoundMask - generic monster can't hear.
//=========================================================
int CFakeGordon :: ISoundMask ( void )
{
	return	NULL;
}

//=========================================================
// Spawn
//=========================================================
void CFakeGordon :: Spawn()
{
	if ( FBitSet( pev->spawnflags, 8 ) )
	{
		PRECACHE_MODEL( "models/fakegordoncoat.mdl" );
		SET_MODEL( ENT(pev), "models/fakegordoncoat.mdl" );
	}
	else
	{
		PRECACHE_MODEL( "models/fakegordon.mdl" );
		SET_MODEL( ENT(pev), "models/fakegordon.mdl" );
	}
	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	pev->solid			= SOLID_NOT;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= DONT_BLEED;
	pev->health			= 800;
	pev->view_ofs		= Vector ( 0, 0, 50 );// position of the eyes relative to monster's origin.
	m_flFieldOfView		= VIEW_FIELD_FULL;
	m_MonsterState		= MONSTERSTATE_NONE;
	m_hRealGordon		= NULL;

	MonsterInit();
}


//=========================================================
// AI Schedules Specific to this monster
//=========================================================


void CFakeGordon :: StartTask( Task_t *pTask )
{
	switch( pTask->iTask )
	{
	case TASK_WAIT:
		if (m_hRealGordon == NULL)
		{
			m_hRealGordon = UTIL_FindEntityByClassname( NULL, "player" );
		}
		break;
	}
	CBaseMonster::StartTask( pTask );
}

void CFakeGordon :: RunTask( Task_t *pTask )
{
	switch( pTask->iTask )
	{
	case TASK_WAIT:
		// look at who I'm talking to
		if (m_flTalkTime > gpGlobals->time && m_hTalkTarget != NULL)
		{
			float yaw = VecToYaw(m_hTalkTarget->pev->origin - pev->origin) - pev->angles.y;

			if (yaw > 180) yaw -= 360;
			if (yaw < -180) yaw += 360;

			// turn towards vector
			SetBoneController( 0, yaw );
		}
		// look at whoever speaks a scripted sentence
		else if (CTalkMonster::g_talkWaitTime - 2 > gpGlobals->time && g_iT2DefSpeaker)
		{
			CBaseEntity *pSpeaker = CBaseEntity::Instance( g_engfuncs.pfnPEntityOfEntIndex( g_iT2DefSpeaker ) );

			if (pSpeaker)
			{
				if ((pSpeaker->pev->origin - pev->origin).Length() < 256)
				{
					float yaw = VecToYaw(pSpeaker->pev->origin - pev->origin) - pev->angles.y;

					if (yaw > 180) yaw -= 360;
					if (yaw < -180) yaw += 360;

					// turn towards vector
					SetBoneController( 0, yaw );
				}
			}
		}
		else
		{
			SetBoneController( 0, 0 );
		}
		CBaseMonster::RunTask( pTask );
		break;
	default:
		SetBoneController( 0, 0 );
		CBaseMonster::RunTask( pTask );
		break;
	}
}

//=========================================================
// MonsterThink - keeps x and y of the real Gordon when he
// is close
//=========================================================
void CFakeGordon :: MonsterThink( void )
{
	CBaseMonster::MonsterThink();

	m_hRealGordon = CBaseEntity::Instance( g_engfuncs.pfnPEntityOfEntIndex( 1 ) );

	if (m_hRealGordon != NULL && !FBitSet( pev->spawnflags, 64 ))
	{
		float flDist = (m_hRealGordon->pev->origin - pev->origin).Length();

		if (flDist <= 256)
		{
			pev->origin.x = m_hRealGordon->pev->origin.x;
			pev->origin.y = m_hRealGordon->pev->origin.y;
		}
	}
}


void CFakeGordon::PlayScriptedSentence( const char *pszSentence, float duration, float volume, float attenuation, BOOL bConcurrent, CBaseEntity *pListener )
{
	CBaseMonster::PlayScriptedSentence( pszSentence, duration, volume, attenuation, bConcurrent, pListener );

	CTalkMonster::g_talkWaitTime = gpGlobals->time + duration + 2.0;
	m_flTalkTime = gpGlobals->time + duration;
	m_hTalkTarget = pListener;
}

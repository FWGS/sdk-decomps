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
// POV: monster_hev_orangehelmet, an HEV scientist with the
// orange helmet (head 2 of the HEV scientist's model). With
// spawnflag 32 he rides the func_train "hztrain0101" (code
// from the generic monster's train following) and doesn't
// take damage. One of CBaseBarn's monsters (basebarn.h,
// pov_barn.cpp). File name ours.
//=========================================================

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"basebarn.h"

// spawnflags (names ours)
#define SF_HEVORANGE_STRONG		8		// 80000 health
#define SF_HEVORANGE_ONTRAIN	32		// rides hztrain0101, can't be hurt

class CHEVOrangeHelmet : public CBaseBarn
{
public:
	void Spawn( void );
	void Precache( void );

	int GetHead( void ) { return 2; }
	void SetGun( int iGun );
	BOOL HasGun( void );

	void TalkInit( void );

	void MonsterThink( void );
	int TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType );

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	int		m_iGotPos;
	Vector	m_vecDistance;
};

LINK_ENTITY_TO_CLASS( monster_hev_orangehelmet, CHEVOrangeHelmet );

TYPEDESCRIPTION	CHEVOrangeHelmet::m_SaveData[] =
{
	DEFINE_FIELD( CHEVOrangeHelmet, m_iGotPos, FIELD_INTEGER ),
	DEFINE_FIELD( CHEVOrangeHelmet, m_vecDistance, FIELD_VECTOR ),
};

IMPLEMENT_SAVERESTORE( CHEVOrangeHelmet, CBaseBarn );

//=========================================================
// Spawn
//=========================================================
void CHEVOrangeHelmet :: Spawn()
{
	CBaseBarn::Spawn();

	SET_MODEL(ENT(pev), "models/hev_scientist.mdl");
	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	SetupWeapon();

	if ( pev->spawnflags & SF_HEVORANGE_STRONG )
		pev->health		= 80000;
	else
		pev->health		= gSkillData.hevbarnsHealth;

	m_iGotPos			= FALSE;
	m_fUsesMeleeAttack	= TRUE;
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CHEVOrangeHelmet :: Precache()
{
	PRECACHE_MODEL("models/hev_scientist.mdl");

	CBaseBarn::Precache();

	TalkInit();
}

// Init talk data
void CHEVOrangeHelmet :: TalkInit()
{

	CTalkMonster::TalkInit();

	// scientists speach group names (group names are in sentences.txt)

	m_szGrp[TLK_ANSWER]  =	"SC_ANSWER";
	m_szGrp[TLK_QUESTION] =	"SC_QUESTION";
	m_szGrp[TLK_IDLE] =		"SC_IDLE";
	m_szGrp[TLK_SMELL] =	"SC_SMELL";

	m_szGrp[TLK_WOUND] =	"SC_WOUND";
	m_szGrp[TLK_MORTAL] =	"SC_MORTAL";

	m_szGrp[TLK_PAIN] =		"SC_PAIN";
	m_szGrp[TLK_DIE] =		"SC_PAIN";
	m_szGrp[TLK_ATTACK] =	"SC_ATTACK";
	m_szGrp[TLK_KILL] =		"SC_KILL";

	// get voice for head
	switch ( RANDOM_LONG( 0, 3 ) )
	{
	default:
	case 0:	m_voicePitch = 105; break;	//glasses
	case 1: m_voicePitch = 100; break;	//einstein
	case 2:	m_voicePitch = 95;  break;	//luther
	case 3:	m_voicePitch = 100;  break;	//slick
	}
}

//=========================================================
// SetGun - the HEV models have the head in bodygroup 1
// and the gun in bodygroup 2
//=========================================================
void CHEVOrangeHelmet :: SetGun( int iGun )
{
	pev->body = 0;
	SetBodygroup( 1, GetHead() );
	SetBodygroup( 2, iGun );
}

//=========================================================
// HasGun
//=========================================================
BOOL CHEVOrangeHelmet :: HasGun( void )
{
	if ( GetBodygroup( 2 ) == BARN_BODY_GUNGONE || GetBodygroup( 2 ) == BARN_BODY_GUNHOLSTERED )
		return FALSE;

	return TRUE;
}

//=========================================================
// MonsterThink - ride along with hztrain0101
//=========================================================
void CHEVOrangeHelmet :: MonsterThink( void )
{
	if ( pev->spawnflags & SF_HEVORANGE_ONTRAIN )
	{
		edict_t *pentTrain = FIND_ENTITY_BY_TARGETNAME( NULL, "hztrain0101" );
		if ( FNullEnt( pentTrain ) )
		{
			ALERT( at_console, "CHEVOrangeHelmet: ERROR - couldn't find func_train %s!!\n", "hztrain0101" );
		}
		else if ( !FClassnameIs( pentTrain, "func_train" ) )
		{
			ALERT( at_console, "CHEVOrangeHelmet: ERROR - %s is not a func_train!!\n", "hztrain0101" );
		}
		else if ( !m_iGotPos )
		{
			m_vecDistance = pev->origin - pentTrain->v.origin;
			m_iGotPos = TRUE;
		}
		else
		{
			pev->origin = pentTrain->v.origin + m_vecDistance;
			pev->velocity = pentTrain->v.velocity;
			pev->avelocity = pentTrain->v.avelocity;
			pev->angles = pentTrain->v.angles;
		}
	}

	CBaseMonster::MonsterThink();
}

int CHEVOrangeHelmet :: TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType )
{
	int ret = 0;

	if ( pev->spawnflags & SF_HEVORANGE_ONTRAIN )
		PainSound();
	else
		ret = CBaseBarn::TakeDamage( pevInflictor, pevAttacker, flDamage, bitsDamageType );

	return ret;
}

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
// POV: monster_worker_brave (CBrvWorker), a brave
// scientist with gus.mdl and a tool that hits like the
// zombie's claw. File name and the names retail doesn't
// give are ours.
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
#define		BRVWORKER_AE_HIT		( 4 )

#define		BRVWORKER_WEAPON_KNIFE	4	// slashes; the others club (name ours)

class CBrvWorker : public CBrvScientist
{
public:
	void Spawn( void );
	void Precache( void );
	void TalkInit( void );
	void SetActivity ( Activity NewActivity );
	void HandleAnimEvent( MonsterEvent_t *pEvent );
	void SetWeaponBody( int iBody );

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	int		m_iWeaponUsed;
};

LINK_ENTITY_TO_CLASS( monster_worker_brave, CBrvWorker );

TYPEDESCRIPTION	CBrvWorker::m_SaveData[] =
{
	DEFINE_FIELD( CBrvWorker, m_iWeaponUsed, FIELD_INTEGER ),
};

IMPLEMENT_SAVERESTORE( CBrvWorker, CBrvScientist );

//=========================================================
// Spawn - the health key picks the tool (1-4)
//=========================================================
void CBrvWorker :: Spawn( void )
{
	if ( pev->health >= 1 && pev->health <= 4 )
		m_iWeaponUsed = pev->health;
	else
		m_iWeaponUsed = RANDOM_LONG( 1, 4 );

	CBrvScientist::Spawn();

	SET_MODEL(ENT(pev), "models/gus.mdl");
	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	SetWeaponBody( m_iWeaponUsed );

	m_afCapability |= bits_CAP_MELEE_ATTACK1;
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CBrvWorker :: Precache( void )
{
	PRECACHE_MODEL("models/gus.mdl");

	CBrvScientist::Precache();

	TalkInit();
}

// Init talk data
void CBrvWorker :: TalkInit()
{
	m_voicePitch = RANDOM_LONG( 90, 94 );		// sic: CScientist::TalkInit sets it again

	CBrvScientist::TalkInit();
}

//=========================================================
// SetActivity (the human grunt's)
//=========================================================
void CBrvWorker :: SetActivity ( Activity NewActivity )
{
	int	iSequence = ACTIVITY_NOT_AVAILABLE;
	void *pmodel = GET_MODEL_PTR( ENT(pev) );

	switch ( NewActivity )
	{
	case ACT_MELEE_ATTACK1:
		iSequence = LookupSequence( "attack1" );
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
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CBrvWorker :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	switch( pEvent->event )
	{
	case BRVWORKER_AE_HIT:
		{
			// the zombie's right slash
			CBaseEntity *pHurt = CheckTraceHullAttack( 70, gSkillData.zombieDmgOneSlash, m_iWeaponUsed == BRVWORKER_WEAPON_KNIFE ? DMG_SLASH : DMG_CLUB );
			if ( pHurt )
			{
				if ( pHurt->pev->flags & (FL_MONSTER|FL_CLIENT) )
				{
					pHurt->pev->punchangle.z = -9;
					pHurt->pev->punchangle.x = 5;
					pHurt->pev->velocity = pHurt->pev->velocity - gpGlobals->v_right * 60;
				}
				AttackHitSound();
			}
			else
				AttackMissSound();
		}
		break;

	default:
		CBrvScientist::HandleAnimEvent( pEvent );
		break;
	}
}

void CBrvWorker :: SetWeaponBody( int iBody )
{
	pev->body = iBody;
}

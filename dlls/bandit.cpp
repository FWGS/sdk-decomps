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
// the bandit base: Half-Life's human grunt, reworked into
// the squad AI of Gunman's human enemies; it fires gauss
// shots and the shotgun, rolls out of the line of fire
// and throws grenades
//=========================================================

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"schedule.h"
#include	"animation.h"
#include	"weapons.h"
#include	"talkmonster.h"
#include	"soundent.h"
#include	"effects.h"
#include	"player.h"
#include	"gaussshots.h"
#include	"bandit.h"

int g_fBanditQuestion;				// true if an idle bandit asked a question. Cleared when someone answers.

extern DLL_GLOBAL int		g_iSkillLevel;
extern unsigned short g_usGaussFire;
extern int g_iGibHuman;
extern int g_iGoreHuman;

//=========================================================
// monster-specific DEFINE's
//=========================================================
#define BANDIT_SENTENCE_VOLUME		(float)0.25 // volume of grunt sentences
#define BANDIT_ATTN					ATTN_NORM	// attenutation of grunt sentences

#define BANDIT_SENT_NONE	-1
#define BANDIT_SENT_GREN	0
#define BANDIT_SENT_ALERT	1
#define BANDIT_SENT_MONSTER	2
#define BANDIT_SENT_COVER	3
#define BANDIT_SENT_THROW	4
#define BANDIT_SENT_CHARGE	5
#define BANDIT_SENT_TAUNT	6

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define		BANDIT_AE_GREN_TOSS		( 2 )
#define		BANDIT_AE_GREN_DROP		( 3 )
#define		BANDIT_AE_KICK			( 4 )

TYPEDESCRIPTION	CBaseBandit::m_SaveData[] =
{
	DEFINE_FIELD( CBaseBandit, m_flNextRollCheck, FIELD_TIME ),
	DEFINE_FIELD( CBaseBandit, m_flNextGrenadeCheck, FIELD_TIME ),
	DEFINE_FIELD( CBaseBandit, m_flNextPainTime, FIELD_TIME ),
//	DEFINE_FIELD( CBaseBandit, m_flLastEnemySightTime, FIELD_TIME ), // don't save, go to zero
	DEFINE_FIELD( CBaseBandit, m_vecTossVelocity, FIELD_VECTOR ),
	DEFINE_FIELD( CBaseBandit, m_bThrowGrenade, FIELD_BOOLEAN ),
	DEFINE_FIELD( CBaseBandit, m_bStanding, FIELD_BOOLEAN ),
	DEFINE_FIELD( CBaseBandit, m_bFirstEncounter, FIELD_BOOLEAN ),
	DEFINE_FIELD( CBaseBandit, m_cClipSize, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseBandit, m_iVoicePitch, FIELD_INTEGER ),
//  DEFINE_FIELD( CBaseBandit, m_iBrassShell, FIELD_INTEGER ),
//  DEFINE_FIELD( CBaseBandit, m_iShotgunShell, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseBandit, m_iSentence, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseBandit, m_iGunsDrawn, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseBandit, m_iBeam, FIELD_INTEGER ),
};

IMPLEMENT_SAVERESTORE( CBaseBandit, CSquadMonster );

const char *CBaseBandit::pBanditSentences[] =
{
	"HG_GREN", // grenade scared grunt
	"HG_ALERT", // sees player
	"HG_MONST", // sees monster
	"HG_COVER", // running to cover
	"HG_THROW", // about to throw grenade
	"HG_CHARGE",  // running out to get the enemy
	"HG_TAUNT", // say rude things
	"HG_IDLE",
	"HG_QUEST",
	"HG_ANSWER",
	"HG_CHECK",
};

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
void CBaseBandit :: SpeakSentence( void )
{
	if ( m_iSentence == BANDIT_SENT_NONE )
	{
		// no sentence cued up.
		return;
	}

	if (FOkToSpeak())
	{
		SENTENCEG_PlayRndSz( ENT(pev), pBanditSentences[ m_iSentence ], BANDIT_SENTENCE_VOLUME, BANDIT_ATTN, 0, m_iVoicePitch);
		JustSpoke();
	}
}

//=========================================================
// ISoundMask - Overidden for human grunts because they
// hear the DANGER sound that is made by hand grenades and
// other dangerous items.
//=========================================================
int CBaseBandit :: ISoundMask ( void )
{
	return	bits_SOUND_WORLD	|
			bits_SOUND_COMBAT	|
			bits_SOUND_PLAYER	|
			bits_SOUND_DANGER;
}

//=========================================================
// someone else is talking - don't speak
//=========================================================
BOOL CBaseBandit :: FOkToSpeak( void )
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

	// only bandits in a squad talk
	return InSquad();
}

//=========================================================
//=========================================================
void CBaseBandit :: JustSpoke( void )
{
	CTalkMonster::g_talkWaitTime = gpGlobals->time + RANDOM_FLOAT(2.0, 3.5);
	m_iSentence = BANDIT_SENT_NONE;
}

//=========================================================
// PrescheduleThink - this function runs after conditions
// are collected and before scheduling code is run.
//=========================================================
void CBaseBandit :: PrescheduleThink ( void )
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
BOOL CBaseBandit :: FCanCheckAttacks ( void )
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
BOOL CBaseBandit :: CheckMeleeAttack1 ( float flDot, float flDist )
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
BOOL CBaseBandit :: CheckRangeAttack1 ( float flDot, float flDist )
{
	if ( !HasConditions( bits_COND_ENEMY_OCCLUDED ) && flDist <= 4096 && flDot >= 0.5 && NoFriendlyFire() )
	{
		TraceResult	tr;

		if ( !m_hEnemy->IsPlayer() && flDist <= 64 )
		{
			// kick nonclients, but don't shoot at them.
			return FALSE;
		}

		// far away, try it crouched first
		if ( CanCrouch() && flDist >= 800 )
		{
			Crouch();

			Vector vecSrc = GetGunPosition();

			// verify that a bullet fired from the gun will hit the enemy before the world.
			UTIL_TraceLine( vecSrc, m_hEnemy->BodyTarget(vecSrc), dont_ignore_monsters, ignore_glass, ENT(pev), &tr);

			if ( tr.flFraction == 1.0 )
			{
				return TRUE;
			}
		}

		Stand();

		Vector vecSrc = GetGunPosition();

		// verify that a bullet fired from the gun will hit the enemy before the world.
		UTIL_TraceLine( vecSrc, m_hEnemy->BodyTarget(vecSrc), dont_ignore_monsters, ignore_glass, ENT(pev), &tr);

		if ( tr.flFraction == 1.0 )
		{
			return TRUE;
		}
	}

	return FALSE;
}

//=========================================================
// CheckRangeAttack2 - this checks the Grunt's grenade
// attack.
//=========================================================
BOOL CBaseBandit :: CheckRangeAttack2 ( float flDot, float flDist )
{
	if (! FBitSet(pev->weapons, BANDIT_HANDGRENADE))
	{
		return FALSE;
	}

	// if the grunt isn't moving, it's ok to check.
	if ( m_flGroundSpeed != 0 )
	{
		m_bThrowGrenade = FALSE;
		return m_bThrowGrenade;
	}

	// assume things haven't changed too much since last time
	if (gpGlobals->time < m_flNextGrenadeCheck )
	{
		return m_bThrowGrenade;
	}

	if ( !FBitSet ( m_hEnemy->pev->flags, FL_ONGROUND ) && m_hEnemy->pev->waterlevel == 0 && m_vecEnemyLKP.z > pev->absmax.z  )
	{
		//!!!BUGBUG - we should make this check movetype and make sure it isn't FLY? Players who jump a lot are unlikely to
		// be grenaded.
		// don't throw grenades at anything that isn't on the ground!
		m_bThrowGrenade = FALSE;
		return m_bThrowGrenade;
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
			m_bThrowGrenade = FALSE;
		}
	}

	if ( ( vecTarget - pev->origin ).Length2D() <= 256 )
	{
		// crap, I don't want to blow myself up
		m_flNextGrenadeCheck = gpGlobals->time + 1; // one full second.
		m_bThrowGrenade = FALSE;
		return m_bThrowGrenade;
	}

	Vector vecToss = VecCheckToss( pev, GetGunPosition(), vecTarget, 0.5 );

	if ( vecToss != g_vecZero )
	{
		m_bThrowGrenade = TRUE;
		m_vecTossVelocity = vecToss;

		// don't check again for a while.
		m_flNextGrenadeCheck = gpGlobals->time; // 1/3 second.
	}
	else
	{
		// don't throw
		m_bThrowGrenade = FALSE;
		// don't check again for a while.
		m_flNextGrenadeCheck = gpGlobals->time + 1; // one full second.
	}

	return m_bThrowGrenade;
}


//=========================================================
// TraceAttack - Gunman's bandits wear no helmets: a hit
// there is a head shot
//=========================================================
void CBaseBandit :: TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType)
{
	if ( ptr->iHitgroup == 11 || ptr->iHitgroup == HITGROUP_HEAD )
	{
		ptr->iHitgroup = HITGROUP_HEAD;
	}
	CSquadMonster::TraceAttack( pevAttacker, flDamage, vecDir, ptr, bitsDamageType );
}

//=========================================================
// GetDeathActivity - a head shot takes the head off
//=========================================================
Activity CBaseBandit :: GetDeathActivity ( void )
{
	BOOL fHeadGib;
	int iHeadless;
	int iGroup;

	GetHeadGib( &fHeadGib, &iGroup, &iHeadless );

	if ( m_LastHitGroup == HITGROUP_HEAD && fHeadGib && ( m_LastHitDamageType & DMG_BULLET ) && CVAR_GET_FLOAT( "violence_hgibs" ) != 0 )
	{
		SetBodygroup( iGroup, iHeadless );
		SpawnGib( "models/humanskull.mdl", 3, 0, 2 );
		SpawnGib( "models/hgibs.mdl", 1, 8, 2 );
	}

	return CSquadMonster::GetDeathActivity();
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CBaseBandit :: SetYawSpeed ( void )
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

void CBaseBandit :: IdleSound( void )
{
	if (FOkToSpeak() && (g_fBanditQuestion || RANDOM_LONG(0,1)))
	{
		if (!g_fBanditQuestion)
		{
			// ask question or make statement
			switch (RANDOM_LONG(0,2))
			{
			case 0: // check in
				SENTENCEG_PlayRndSz(ENT(pev), "HG_CHECK", BANDIT_SENTENCE_VOLUME, ATTN_NORM, 0, m_iVoicePitch);
				g_fBanditQuestion = 1;
				break;
			case 1: // question
				SENTENCEG_PlayRndSz(ENT(pev), "HG_QUEST", BANDIT_SENTENCE_VOLUME, ATTN_NORM, 0, m_iVoicePitch);
				g_fBanditQuestion = 2;
				break;
			case 2: // statement
				SENTENCEG_PlayRndSz(ENT(pev), "HG_IDLE", BANDIT_SENTENCE_VOLUME, ATTN_NORM, 0, m_iVoicePitch);
				break;
			}
		}
		else
		{
			switch (g_fBanditQuestion)
			{
			case 1: // check in
				SENTENCEG_PlayRndSz(ENT(pev), "HG_CLEAR", BANDIT_SENTENCE_VOLUME, ATTN_NORM, 0, m_iVoicePitch);
				break;
			case 2: // question
				SENTENCEG_PlayRndSz(ENT(pev), "HG_ANSWER", BANDIT_SENTENCE_VOLUME, ATTN_NORM, 0, m_iVoicePitch);
				break;
			}
			g_fBanditQuestion = 0;
		}
		JustSpoke();
	}
}

//=========================================================
// CheckAmmo - overridden for the grunt because he actually
// uses ammo! (base class doesn't)
//=========================================================
void CBaseBandit :: CheckAmmo ( void )
{
	if ( !HasUnlimitedAmmo() && m_cAmmoLoaded <= 0 )
	{
		SetConditions(bits_COND_NO_AMMO_LOADED);
	}
}

//=========================================================
// Classify - indicates this monster's place in the
// relationship table.
//=========================================================
int	CBaseBandit :: Classify ( void )
{
	return	CLASS_HUMAN_MILITARY;
}

//=========================================================
//=========================================================
CBaseEntity *CBaseBandit :: Kick( void )
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

Vector CBaseBandit :: GetGunPosition( )
{
	if (m_bStanding )
	{
		return pev->origin + Vector( 0, 0, 60 );
	}
	else
	{
		return pev->origin + Vector( 0, 0, 48 );
	}
}

//=========================================================
// Shoot - a gauss shot from the gun
//=========================================================
void CBaseBandit :: Shoot ( void )
{
	ShootGauss( GetGunPosition(), 0 );
}

//=========================================================
// ShootGauss - a gauss beam from the given spot, spread a
// little around the enemy
//=========================================================
void CBaseBandit :: ShootGauss ( Vector vecShootOrigin, int iGun )
{
	if (m_hEnemy == NULL)
	{
		return;
	}

	Vector vecShootDir = ShootAtEnemy( vecShootOrigin );

	UTIL_MakeVectors ( pev->angles );

	float x, y;
	do {
		x = RANDOM_FLOAT(-0.5,0.5) + RANDOM_FLOAT(-0.5,0.5);
		y = RANDOM_FLOAT(-0.5,0.5) + RANDOM_FLOAT(-0.5,0.5);
	} while ( x*x+y*y > 1 );

	vecShootDir = vecShootDir + x * VECTOR_CONE_8DEGREES.x * gpGlobals->v_right + y * VECTOR_CONE_8DEGREES.y * gpGlobals->v_up;

	FireGauss( vecShootOrigin, vecShootDir, gSkillData.banditDmgBolt, this, iGun );

	EMIT_SOUND( ENT(pev), CHAN_WEAPON, "weapons/gauss_fire1.wav", 1, ATTN_NORM );
	pev->effects |= EF_MUZZLEFLASH;

	if ( !HasUnlimitedAmmo() )
		m_cAmmoLoaded--;// take away a bullet!

	Vector angDir = UTIL_VecToAngles( vecShootDir );
	SetBlending( 0, angDir.x );
}

//=========================================================
// FireGauss - the gauss beam's trace; the event draws it
//=========================================================
void CBaseBandit :: FireGauss ( Vector vecOrigSrc, Vector vecDir, float flDamage, CBaseEntity *pShooter, int iGun )
{
	Vector vecSrc = vecOrigSrc;
	Vector vecDest = vecSrc + vecDir * 8192;
	TraceResult tr;

	UTIL_TraceLine( vecSrc, vecDest, dont_ignore_monsters, ENT( pShooter->pev ), &tr );

	if ( tr.fAllSolid )
		return;

	CBaseEntity *pEntity = CBaseEntity::Instance( tr.pHit );

	if ( pEntity == NULL )
		return;

	pShooter->pev->effects |= EF_MUZZLEFLASH;

	Vector vecMuzzle = vecSrc + gpGlobals->v_up * -8 + gpGlobals->v_right * 3;	// sic: not used

	if ( pEntity->pev->takedamage )
	{
		ClearMultiDamage();
		pEntity->TraceAttack( pShooter->pev, flDamage, vecDir, &tr, DMG_BULLET );
		ApplyMultiDamage( pShooter->pev, pShooter->pev );

		if ( pEntity->IsPlayer() )
			EMIT_SOUND_DYN( pEntity->edict(), CHAN_VOICE, "weapons/gauss_fire3.wav", 0.25, ATTN_NORM, 0, 120 );
	}

	UTIL_VecToAngles( vecDir.Normalize() );	// sic: thrown away

	// the event gets the trace's plane normal and end point
	PLAYBACK_EVENT_FULL( FEV_RELIABLE, edict(), g_usGaussFire, 0.0, (float *)&tr.vecPlaneNormal, (float *)&tr.vecEndPos, 0.0, 0.0, 1, iGun, 0, 1 );
}

//=========================================================
// ShootFastShot - the gauss pistol's fast shot
//=========================================================
void CBaseBandit :: ShootFastShot ( Vector vecShootOrigin )
{
	if (m_hEnemy == NULL)
	{
		return;
	}

	Vector vecShootDir = ShootAtEnemy( vecShootOrigin );
	Vector vecAngles = UTIL_VecToAngles( vecShootDir );
	vecAngles.x = vecAngles.x * -1;
	UTIL_MakeVectors( vecAngles );

	CFastShot::Shoot( vecShootOrigin, vecShootDir, this, gSkillData.banditDmgFast );	// sic: the direction, not the angles

	EMIT_SOUND( ENT(pev), CHAN_WEAPON, "weapons/gauss_fire1.wav", 1, ATTN_NORM );
	pev->effects |= EF_MUZZLEFLASH;

	if ( !HasUnlimitedAmmo() )
		m_cAmmoLoaded--;// take away a bullet!

	Vector angDir = UTIL_VecToAngles( vecShootDir );
	SetBlending( 0, angDir.x );
}

//=========================================================
// ShootBoltShot - the gauss pistol's charged bolt
//=========================================================
void CBaseBandit :: ShootBoltShot ( Vector vecShootOrigin )
{
	if (m_hEnemy == NULL)
	{
		return;
	}

	Vector vecShootDir = ShootAtEnemy( vecShootOrigin );
	Vector vecAngles = UTIL_VecToAngles( vecShootDir );
	vecAngles.x = vecAngles.x * -1;
	UTIL_MakeVectors( vecAngles );

	CBoltShot::Shoot( vecShootOrigin, vecAngles, this );

	EMIT_SOUND( ENT(pev), CHAN_WEAPON, "weapons/gauss_fire2.wav", 1, ATTN_NORM );
	EMIT_SOUND( ENT(pev), CHAN_ITEM, "weapons/gauss_charge.wav", 1, ATTN_NORM );
	pev->effects |= EF_MUZZLEFLASH;

	if ( !HasUnlimitedAmmo() )
		m_cAmmoLoaded--;// take away a bullet!

	Vector angDir = UTIL_VecToAngles( vecShootDir );
	SetBlending( 0, angDir.x );
}

//=========================================================
// Shotgun
//=========================================================
void CBaseBandit :: Shotgun ( void )
{
	if (m_hEnemy == NULL)
	{
		return;
	}

	Vector vecShootOrigin = GetGunPosition();
	Vector vecShootDir = ShootAtEnemy( vecShootOrigin );

	UTIL_MakeVectors ( pev->angles );

	Vector vecAngles = pev->angles;
	vecAngles.x += 4;

	Vector vecForward;
	g_engfuncs.pfnAngleVectors( vecAngles, vecForward, NULL, NULL );

	Vector vecMuzzle;
	GET_ATTACHMENT( ENT(pev), 0, vecMuzzle, NULL );

	PLAYBACK_EVENT_FULL( FEV_RELIABLE, ENT(pev), m_usShotgun, 0.0, (float *)&vecMuzzle, (float *)&vecShootDir, 0.0, 0.0, 0, 0, 0, 0 );	// sic: vecForward isn't used

	FireBullets( 6, vecShootOrigin, vecShootDir, VECTOR_CONE_15DEGREES, 4096, BULLET_PLAYER_BUCKSHOT, 0, gSkillData.demomanDmgShotgun );

	pev->effects |= EF_MUZZLEFLASH;

	if ( !HasUnlimitedAmmo() )
		m_cAmmoLoaded--;// take away a bullet!

	Vector angDir = UTIL_VecToAngles( vecShootDir );
	SetBlending( 0, angDir.x );
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CBaseBandit :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	switch( pEvent->event )
	{
		case BANDIT_AE_GREN_TOSS:
		{
			UTIL_MakeVectors( pev->angles );
			CGrenade::ShootTimed( pev, pev->origin + gpGlobals->v_forward * 34 + Vector (0, 0, 32), m_vecTossVelocity, 3.5 );

			m_bThrowGrenade = FALSE;
			m_flNextGrenadeCheck = gpGlobals->time + 6;// wait six seconds before even looking again to see if a grenade can be thrown.
			// !!!LATER - when in a group, only try to throw grenade if ordered.
		}
		break;

		case BANDIT_AE_GREN_DROP:
		{
			UTIL_MakeVectors( pev->angles );
			CGrenade::ShootTimed( pev, pev->origin + gpGlobals->v_forward * 17 - gpGlobals->v_right * 27 + gpGlobals->v_up * 6, g_vecZero, 3 );
		}
		// sic: falls through, and kicks

		case BANDIT_AE_KICK:
		{
			CBaseEntity *pHurt = Kick();

			if ( pHurt )
			{
				// SOUND HERE!
				UTIL_MakeVectors( pev->angles );
				pHurt->pev->punchangle.x = 15;
				pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_forward * 100 + gpGlobals->v_up * 50;
				pHurt->TakeDamage( pev, pev, gSkillData.humanDmgKick, DMG_CLUB );

				CBaseMonster *pMonster = pHurt->MyMonsterPointer();
				if ( pMonster )
				{
					TraceResult tr;
					UTIL_TraceLine( BodyTarget( g_vecZero ), pMonster->BodyTarget( g_vecZero ), dont_ignore_monsters, ENT(pev), &tr );
					pMonster->TraceBleed( gSkillData.humanDmgKick, ( pMonster->BodyTarget( g_vecZero ) - BodyTarget( g_vecZero ) ).Normalize(), &tr, DMG_SLASH, tr.vecEndPos );

					if ( pMonster->IsPlayer() )
						EMIT_SOUND_DYN( pMonster->edict(), CHAN_VOICE, "debris/bustflesh1.wav", 0.4, 2.0, 0, 120 );
				}
			}
		}
		break;

		default:
			CSquadMonster::HandleAnimEvent( pEvent );
			break;
	}
}

//=========================================================
// Spawn - what every bandit does once the derived class
// has set its model and called MonsterInit
//=========================================================
void CBaseBandit :: Spawn()
{
	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	pev->effects		= 0;
	m_flFieldOfView		= 0.2;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_flNextGrenadeCheck = gpGlobals->time + 1;
	m_flNextPainTime	= gpGlobals->time;
	m_iSentence			= BANDIT_SENT_NONE;

	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	m_afCapability		= bits_CAP_SQUAD | bits_CAP_TURN_HEAD | bits_CAP_DOORS_GROUP | bits_CAP_HEAR;

	m_fEnemyEluded		= FALSE;
	m_bFirstEncounter	= TRUE;// this is true when the grunt spawns, because he hasn't encountered an enemy yet.

	if ( HasUnlimitedAmmo() )
		m_cAmmoLoaded = 1;
	else
		m_cAmmoLoaded = m_cClipSize;

	m_bStanding = TRUE;

	if (RANDOM_LONG(0,1))
		m_iVoicePitch = 90 + RANDOM_LONG(0,7);
	else
		m_iVoicePitch = 90;

	CTalkMonster::g_talkWaitTime = 0;

	m_flDistTooFar = 4096;
	m_flNextRollCheck = gpGlobals->time;
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CBaseBandit :: Precache()
{
	m_iBeam = PRECACHE_MODEL( "sprites/zbeam5.spr" );

	PRECACHE_SOUND( "weapons/gauss_fire1.wav" );
	PRECACHE_SOUND( "weapons/gauss_fire2.wav" );
	PRECACHE_SOUND( "weapons/gauss_charge.wav" );
	PRECACHE_SOUND( "debris/bustflesh1.wav" );
	PRECACHE_SOUND( "debris/bustflesh1.wav" );
	PRECACHE_SOUND( "bandit/men_check_in.wav" );

	// get voice pitch
	if ( UTIL_PercentChance( 50 ) )
		m_iVoicePitch = 109 + RANDOM_LONG(0,7);
	else
		m_iVoicePitch = 100;

	m_iBrassShell = PRECACHE_MODEL("models/shell.mdl");// brass shell
	m_iShotgunShell = PRECACHE_MODEL ("models/shotgunshell.mdl");

	PRECACHE_MODEL( "sprites/gaussbeam1.spr" );
	PRECACHE_MODEL( "sprites/gausspark.spr" );

	g_iGoreHuman = PRECACHE_MODEL( "sprites/gorehuman.spr" );
	g_iGibHuman = PRECACHE_MODEL( "sprites/gibhuman.spr" );

	PRECACHE_MODEL( "models/humanskull.mdl" );
	PRECACHE_MODEL( "models/hgibs.mdl" );

	m_usShotgun = PRECACHE_EVENT( 1, "events/monstershotgun.sc" );

	CSquadMonster::Precache();
}

//=========================================================
// start task
//=========================================================
void CBaseBandit :: StartTask ( Task_t *pTask )
{
	m_iTaskStatus = TASKSTATUS_RUNNING;

	switch ( pTask->iTask )
	{
	case TASK_BANDIT_CHECK_FIRE:
		if ( !NoFriendlyFire() )
		{
			SetConditions( bits_COND_BANDIT_NOFIRE );
		}
		TaskComplete();
		break;

	case TASK_BANDIT_SPEAK_SENTENCE:
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
		if ( LookupActivity( ACT_RELOAD ) == ACTIVITY_NOT_AVAILABLE )
			TaskComplete();
		else
			m_IdealActivity = ACT_RELOAD;
		break;

	case TASK_BANDIT_FACE_TOSS_DIR:
		break;

	case TASK_FACE_IDEAL:
	case TASK_FACE_ENEMY:
		CSquadMonster :: StartTask( pTask );
		if (pev->movetype == MOVETYPE_FLY)
		{
			m_IdealActivity = ACT_GLIDE;
		}
		break;

	case TASK_BANDIT_ROLL_RIGHT:
		{
			UTIL_MakeVectors( pev->angles );
			Vector vecRight = pev->origin + gpGlobals->v_right * 40;
			if ( CheckLocalMove( pev->origin, vecRight, NULL, NULL ) == LOCALMOVE_VALID && MoveToLocation( ACT_ROLL_RIGHT, 0, vecRight ) )
				break;
			TaskFail();
		}
		break;

	case TASK_BANDIT_ROLL_LEFT:
		{
			UTIL_MakeVectors( pev->angles );
			Vector vecLeft = pev->origin + gpGlobals->v_right * -40;
			if ( CheckLocalMove( pev->origin, vecLeft, NULL, NULL ) == LOCALMOVE_VALID && MoveToLocation( ACT_ROLL_LEFT, 0, vecLeft ) )
				break;
			TaskFail();
		}
		break;

	case TASK_PLAY_SEQUENCE:
	case TASK_PLAY_SEQUENCE_FACE_ENEMY:
	case TASK_PLAY_SEQUENCE_FACE_TARGET:
		{
			int iActivity = (int)pTask->flData;

			if ( LookupActivity( iActivity ) != ACTIVITY_NOT_AVAILABLE )
			{
				CSquadMonster :: StartTask( pTask );
				break;
			}

			// not every bandit model has every sequence the schedules ask for
			switch ( iActivity )
			{
			case ACT_FLY:
			case ACT_LAND:
			case ACT_CROUCH:
			case ACT_RELOAD:
			case ACT_ARM:
			case ACT_GLIDE:
			case ACT_SPECIAL_ATTACK1:
			case ACT_VICTORY_DANCE:
				ALERT( at_console, "no act: %d for: %s, defaulting\n", iActivity, STRING(pev->classname) );
				TaskComplete();
				break;
			case ACT_INSPECT_FLOOR:
			case ACT_INSPECT_WALL:
			case ACT_IDLE_ANGRY:
			case ACT_COMBAT_IDLE:
				ALERT( at_console, "no act: %d for: %s, defaulting\n", iActivity, STRING(pev->classname) );
				iActivity = ACT_IDLE;
				break;
			case ACT_WALK_HURT:
				ALERT( at_console, "no act: %d for: %s, defaulting\n", iActivity, STRING(pev->classname) );
				iActivity = ACT_WALK;
				break;
			}
			m_IdealActivity = (Activity)iActivity;
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
void CBaseBandit :: RunTask ( Task_t *pTask )
{
	switch ( pTask->iTask )
	{
	case TASK_BANDIT_FACE_TOSS_DIR:
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
	case TASK_RELOAD:
		if ( m_fSequenceFinished )
		{
			m_cAmmoLoaded = m_cClipSize;
			TaskComplete();
		}
		// sic: falls through into the roll
	case TASK_BANDIT_ROLL_RIGHT:
		if ( ( m_Route[ m_iRouteIndex ].vecLocation - pev->origin ).Length2D() < 1 )
		{
			Crouch();
			RouteClear();
			TaskComplete();
		}
		if ( m_fSequenceFinished )
		{
			Crouch();
			RouteClear();
			TaskComplete();
		}
		break;
	case TASK_BANDIT_ROLL_LEFT:
		if ( ( m_Route[ m_iRouteIndex ].vecLocation - pev->origin ).Length2D() < 1 )
		{
			Crouch();
			RouteClear();
			TaskComplete();
		}
		if ( m_fSequenceFinished )
		{
			Crouch();
			RouteClear();
			TaskComplete();
		}
		break;
	default:
		{
			CSquadMonster :: RunTask( pTask );
			break;
		}
	}
}

//=========================================================
// AI Schedules Specific to this monster
//=========================================================

//=========================================================
// GruntFail
//=========================================================
Task_t	tlBanditFail[] =
{
	{ TASK_STOP_MOVING,			0				},
	{ TASK_SET_ACTIVITY_IDLE,	(float)0		},
	{ TASK_WAIT,				(float)2		},
	{ TASK_WAIT_PVS,			(float)0		},
};

Schedule_t	slBanditFail[] =
{
	{
		tlBanditFail,
		ARRAYSIZE ( tlBanditFail ),
		bits_COND_CAN_RANGE_ATTACK1 |
		bits_COND_CAN_RANGE_ATTACK2 |
		bits_COND_CAN_MELEE_ATTACK1 |
		bits_COND_CAN_MELEE_ATTACK2,
		0,
		"Grunt Fail"
	},
};

//=========================================================
// Grunt Combat Fail
//=========================================================
Task_t	tlBanditCombatFail[] =
{
	{ TASK_STOP_MOVING,			0				},
	{ TASK_SET_ACTIVITY_IDLE,	(float)0		},
	{ TASK_WAIT_FACE_ENEMY,		(float)2		},
	{ TASK_WAIT_PVS,			(float)0		},
};

Schedule_t	slBanditCombatFail[] =
{
	{
		tlBanditCombatFail,
		ARRAYSIZE ( tlBanditCombatFail ),
		bits_COND_CAN_RANGE_ATTACK1	|
		bits_COND_CAN_RANGE_ATTACK2,
		0,
		"Grunt Combat Fail"
	},
};

//=========================================================
// Victory dance!
//=========================================================
Task_t	tlBanditVictoryDance[] =
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

Schedule_t	slBanditVictoryDance[] =
{
	{
		tlBanditVictoryDance,
		ARRAYSIZE ( tlBanditVictoryDance ),
		bits_COND_NEW_ENEMY		|
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE,
		0,
		"GruntVictoryDance"
	},
};

//=========================================================
// Establish line of fire - move to a position that allows
// the grunt to attack.
//=========================================================
Task_t tlBanditEstablishLineOfFire[] =
{
	{ TASK_SET_FAIL_SCHEDULE,	(float)SCHED_BANDIT_ELOF_FAIL	},
	{ TASK_GET_PATH_TO_ENEMY,	(float)0						},
	{ TASK_BANDIT_SPEAK_SENTENCE,(float)0						},
	{ TASK_RUN_PATH,			(float)0						},
	{ TASK_WAIT_FOR_MOVEMENT,	(float)0						},
};

Schedule_t slBanditEstablishLineOfFire[] =
{
	{
		tlBanditEstablishLineOfFire,
		ARRAYSIZE ( tlBanditEstablishLineOfFire ),
		bits_COND_NEW_ENEMY			|
		bits_COND_ENEMY_DEAD		|
		bits_COND_CAN_RANGE_ATTACK1	|
		bits_COND_CAN_MELEE_ATTACK1	|
		bits_COND_CAN_RANGE_ATTACK2	|
		bits_COND_CAN_MELEE_ATTACK2	|
		bits_COND_HEAR_SOUND,

		bits_SOUND_DANGER,
		"GruntEstablishLineOfFire"
	},
};

//=========================================================
// GruntFoundEnemy - grunt established sight with an enemy
// that was hiding from the squad.
//=========================================================
Task_t	tlBanditFoundEnemy[] =
{
	{ TASK_STOP_MOVING,				0							},
	{ TASK_FACE_ENEMY,				(float)0					},
	{ TASK_WAIT_RANDOM,				(float)0.3					},
};

Schedule_t	slBanditFoundEnemy[] =
{
	{
		tlBanditFoundEnemy,
		ARRAYSIZE ( tlBanditFoundEnemy ),
		bits_COND_HEAR_SOUND,

		bits_SOUND_DANGER,
		"GruntFoundEnemy"
	},
};

//=========================================================
// GruntCombatFace Schedule
//=========================================================
Task_t	tlBanditCombatFace1[] =
{
	{ TASK_STOP_MOVING,				0							},
	{ TASK_SET_ACTIVITY_IDLE,		(float)0					},
	{ TASK_FACE_ENEMY,				(float)0					},
	{ TASK_WAIT,					(float)1.5					},
	{ TASK_SET_SCHEDULE,			(float)SCHED_BANDIT_SWEEP	},
};

Schedule_t	slBanditCombatFace[] =
{
	{
		tlBanditCombatFace1,
		ARRAYSIZE ( tlBanditCombatFace1 ),
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
// empty or grunt gets hurt.
//=========================================================
Task_t	tlBanditSignalSuppress[] =
{
	{ TASK_STOP_MOVING,					0						},
	{ TASK_FACE_IDEAL,					(float)0				},
	{ TASK_PLAY_SEQUENCE_FACE_ENEMY,	(float)ACT_COMBAT_IDLE	},
	{ TASK_WAIT_RANDOM,					(float)0.2				},
	{ TASK_FACE_ENEMY,					(float)0				},
	{ TASK_BANDIT_CHECK_FIRE,			(float)0				},
	{ TASK_RANGE_ATTACK1,				(float)0				},
	{ TASK_FACE_ENEMY,					(float)0				},
	{ TASK_BANDIT_CHECK_FIRE,			(float)0				},
	{ TASK_RANGE_ATTACK1,				(float)0				},
	{ TASK_FACE_ENEMY,					(float)0				},
	{ TASK_BANDIT_CHECK_FIRE,			(float)0				},
	{ TASK_RANGE_ATTACK1,				(float)0				},
	{ TASK_FACE_ENEMY,					(float)0				},
	{ TASK_BANDIT_CHECK_FIRE,			(float)0				},
	{ TASK_RANGE_ATTACK1,				(float)0				},
	{ TASK_FACE_ENEMY,					(float)0				},
	{ TASK_BANDIT_CHECK_FIRE,			(float)0				},
	{ TASK_RANGE_ATTACK1,				(float)0				},
};

Schedule_t	slBanditSignalSuppress[] =
{
	{
		tlBanditSignalSuppress,
		ARRAYSIZE ( tlBanditSignalSuppress ),
		bits_COND_ENEMY_DEAD		|
		bits_COND_LIGHT_DAMAGE		|
		bits_COND_HEAVY_DAMAGE		|
		bits_COND_HEAR_SOUND		|
		bits_COND_BANDIT_NOFIRE		|
		bits_COND_NO_AMMO_LOADED,

		bits_SOUND_DANGER,
		"SignalSuppress"
	},
};

Task_t	tlBanditSuppress[] =
{
	{ TASK_STOP_MOVING,			0							},
	{ TASK_FACE_ENEMY,			(float)0					},
	{ TASK_BANDIT_CHECK_FIRE,	(float)0					},
	{ TASK_RANGE_ATTACK1,		(float)0					},
	{ TASK_FACE_ENEMY,			(float)0					},
	{ TASK_BANDIT_CHECK_FIRE,	(float)0					},
	{ TASK_RANGE_ATTACK1,		(float)0					},
	{ TASK_FACE_ENEMY,			(float)0					},
	{ TASK_BANDIT_CHECK_FIRE,	(float)0					},
	{ TASK_RANGE_ATTACK1,		(float)0					},
	{ TASK_FACE_ENEMY,			(float)0					},
	{ TASK_BANDIT_CHECK_FIRE,	(float)0					},
	{ TASK_RANGE_ATTACK1,		(float)0					},
	{ TASK_FACE_ENEMY,			(float)0					},
	{ TASK_BANDIT_CHECK_FIRE,	(float)0					},
	{ TASK_RANGE_ATTACK1,		(float)0					},
};

Schedule_t	slBanditSuppress[] =
{
	{
		tlBanditSuppress,
		ARRAYSIZE ( tlBanditSuppress ),
		bits_COND_ENEMY_DEAD		|
		bits_COND_LIGHT_DAMAGE		|
		bits_COND_HEAVY_DAMAGE		|
		bits_COND_HEAR_SOUND		|
		bits_COND_BANDIT_NOFIRE		|
		bits_COND_NO_AMMO_LOADED,

		bits_SOUND_DANGER,
		"Suppress"
	},
};


//=========================================================
// grunt wait in cover - we don't allow danger or the ability
// to attack to break a grunt's run to cover schedule, but
// when a grunt is in cover, we do want them to attack if they can.
//=========================================================
Task_t	tlBanditWaitInCover[] =
{
	{ TASK_STOP_MOVING,				(float)0					},
	{ TASK_SET_ACTIVITY_IDLE,		(float)0					},
	{ TASK_WAIT_FACE_ENEMY,			(float)1					},
};

Schedule_t	slBanditWaitInCover[] =
{
	{
		tlBanditWaitInCover,
		ARRAYSIZE ( tlBanditWaitInCover ),
		bits_COND_NEW_ENEMY			|
		bits_COND_HEAR_SOUND		|
		bits_COND_CAN_RANGE_ATTACK1	|
		bits_COND_CAN_RANGE_ATTACK2	|
		bits_COND_CAN_MELEE_ATTACK1	|
		bits_COND_CAN_MELEE_ATTACK2,

		bits_SOUND_DANGER,
		"GruntWaitInCover"
	},
};

//=========================================================
// run to cover.
// !!!BUGBUG - set a decent fail schedule here.
//=========================================================
Task_t	tlBanditTakeCover1[] =
{
	{ TASK_STOP_MOVING,				(float)0							},
	{ TASK_SET_FAIL_SCHEDULE,		(float)SCHED_BANDIT_TAKECOVER_FAILED	},
	{ TASK_WAIT,					(float)0.2							},
	{ TASK_FIND_COVER_FROM_ENEMY,	(float)0							},
	{ TASK_BANDIT_SPEAK_SENTENCE,	(float)0							},
	{ TASK_RUN_PATH,				(float)0							},
	{ TASK_WAIT_FOR_MOVEMENT,		(float)0							},
	{ TASK_REMEMBER,				(float)bits_MEMORY_INCOVER			},
	{ TASK_SET_SCHEDULE,			(float)SCHED_BANDIT_WAIT_FACE_ENEMY	},
};

Schedule_t	slBanditTakeCover[] =
{
	{
		tlBanditTakeCover1,
		ARRAYSIZE ( tlBanditTakeCover1 ),
		0,
		0,
		"TakeCover"
	},
};

//=========================================================
// drop grenade then run to cover.
//=========================================================
Task_t	tlBanditGrenadeCover1[] =
{
	{ TASK_STOP_MOVING,						(float)0							},
	{ TASK_FIND_COVER_FROM_ENEMY,			(float)99							},
	{ TASK_FIND_FAR_NODE_COVER_FROM_ENEMY,	(float)384							},
	{ TASK_PLAY_SEQUENCE,					(float)ACT_SPECIAL_ATTACK1			},
	{ TASK_CLEAR_MOVE_WAIT,					(float)0							},
	{ TASK_RUN_PATH,						(float)0							},
	{ TASK_WAIT_FOR_MOVEMENT,				(float)0							},
	{ TASK_SET_SCHEDULE,					(float)SCHED_BANDIT_WAIT_FACE_ENEMY	},
};

Schedule_t	slBanditGrenadeCover[] =
{
	{
		tlBanditGrenadeCover1,
		ARRAYSIZE ( tlBanditGrenadeCover1 ),
		0,
		0,
		"GrenadeCover"
	},
};


//=========================================================
// drop grenade then run to cover.
//=========================================================
Task_t	tlBanditTossGrenadeCover1[] =
{
	{ TASK_FACE_ENEMY,						(float)0							},
	{ TASK_RANGE_ATTACK2, 					(float)0							},
	{ TASK_SET_SCHEDULE,					(float)SCHED_TAKE_COVER_FROM_ENEMY	},
};

Schedule_t	slBanditTossGrenadeCover[] =
{
	{
		tlBanditTossGrenadeCover1,
		ARRAYSIZE ( tlBanditTossGrenadeCover1 ),
		0,
		0,
		"TossGrenadeCover"
	},
};


//=========================================================
// hide from the loudest sound source (to run from grenade)
//=========================================================
Task_t	tlBanditTakeCoverFromBestSound[] =
{
	{ TASK_SET_FAIL_SCHEDULE,			(float)SCHED_COWER			},// duck and cover if cannot move from explosion
	{ TASK_STOP_MOVING,					(float)0					},
	{ TASK_FIND_COVER_FROM_BEST_SOUND,	(float)0					},
	{ TASK_RUN_PATH,					(float)0					},
	{ TASK_WAIT_FOR_MOVEMENT,			(float)0					},
	{ TASK_REMEMBER,					(float)bits_MEMORY_INCOVER	},
	{ TASK_TURN_LEFT,					(float)179					},
};

Schedule_t	slBanditTakeCoverFromBestSound[] =
{
	{
		tlBanditTakeCoverFromBestSound,
		ARRAYSIZE ( tlBanditTakeCoverFromBestSound ),
		0,
		0,
		"GruntTakeCoverFromBestSound"
	},
};

//=========================================================
// Grunt reload schedule
//=========================================================
Task_t	tlBanditHideReload[] =
{
	{ TASK_STOP_MOVING,				(float)0					},
	{ TASK_SET_FAIL_SCHEDULE,		(float)SCHED_RELOAD			},
	{ TASK_FIND_COVER_FROM_ENEMY,	(float)0					},
	{ TASK_RUN_PATH,				(float)0					},
	{ TASK_RELOAD,					(float)0					},
	{ TASK_WAIT_FOR_MOVEMENT,		(float)0					},
	{ TASK_REMEMBER,				(float)bits_MEMORY_INCOVER	},
	{ TASK_FACE_ENEMY,				(float)0					},
};

Schedule_t slBanditHideReload[] =
{
	{
		tlBanditHideReload,
		ARRAYSIZE ( tlBanditHideReload ),
		0,
		0,
		"GruntHideReload"
	}
};

//=========================================================
// reload where he stands
//=========================================================
Task_t	tlBanditReload[] =
{
	{ TASK_STOP_MOVING,				(float)0					},
	{ TASK_RELOAD,					(float)0					},
	{ TASK_FACE_ENEMY,				(float)0					},
};

Schedule_t slBanditReload[] =
{
	{
		tlBanditReload,
		ARRAYSIZE ( tlBanditReload ),
		0,
		0,
		"GruntReload"
	}
};

//=========================================================
// Do a turning sweep of the area
//=========================================================
Task_t	tlBanditSweep[] =
{
	{ TASK_TURN_LEFT,			(float)179	},
	{ TASK_WAIT,				(float)1	},
	{ TASK_TURN_LEFT,			(float)179	},
	{ TASK_WAIT,				(float)1	},
};

Schedule_t	slBanditSweep[] =
{
	{
		tlBanditSweep,
		ARRAYSIZE ( tlBanditSweep ),

		bits_COND_NEW_ENEMY		|
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE	|
		bits_COND_CAN_RANGE_ATTACK1	|
		bits_COND_CAN_RANGE_ATTACK2	|
		bits_COND_HEAR_SOUND,

		bits_SOUND_WORLD		|// sound flags
		bits_SOUND_DANGER		|
		bits_SOUND_PLAYER,

		"Grunt Sweep"
	},
};

//=========================================================
// secondary range attack. Overriden because base class stops attacking when the enemy is occluded.
// grunt's grenade toss requires the enemy be occluded.
//=========================================================
Task_t	tlBanditRangeAttack2[] =
{
	{ TASK_STOP_MOVING,				(float)0					},
	{ TASK_BANDIT_FACE_TOSS_DIR,	(float)0					},
	{ TASK_PLAY_SEQUENCE,			(float)ACT_RANGE_ATTACK2	},
	{ TASK_SET_SCHEDULE,			(float)SCHED_BANDIT_WAIT_FACE_ENEMY	},// don't run immediately after throwing grenade.
};

Schedule_t	slBanditRangeAttack2[] =
{
	{
		tlBanditRangeAttack2,
		ARRAYSIZE ( tlBanditRangeAttack2 ),
		0,
		0,
		"RangeAttack2"
	},
};


//=========================================================
// repel
//=========================================================
Task_t	tlBanditRepel[] =
{
	{ TASK_STOP_MOVING,			(float)0		},
	{ TASK_FACE_IDEAL,			(float)0		},
	{ TASK_PLAY_SEQUENCE,		(float)ACT_GLIDE 	},
};

Schedule_t	slBanditRepel[] =
{
	{
		tlBanditRepel,
		ARRAYSIZE ( tlBanditRepel ),
		bits_COND_SEE_ENEMY			|
		bits_COND_NEW_ENEMY			|
		bits_COND_LIGHT_DAMAGE		|
		bits_COND_HEAVY_DAMAGE		|
		bits_COND_HEAR_SOUND,

		bits_SOUND_DANGER			|
		bits_SOUND_COMBAT			|
		bits_SOUND_PLAYER,
		"Repel"
	},
};


//=========================================================
// repel
//=========================================================
Task_t	tlBanditRepelAttack[] =
{
	{ TASK_STOP_MOVING,			(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_PLAY_SEQUENCE,		(float)ACT_FLY 	},
};

Schedule_t	slBanditRepelAttack[] =
{
	{
		tlBanditRepelAttack,
		ARRAYSIZE ( tlBanditRepelAttack ),
		bits_COND_ENEMY_OCCLUDED,
		0,
		"Repel Attack"
	},
};

//=========================================================
// repel land
//=========================================================
Task_t	tlBanditRepelLand[] =
{
	{ TASK_STOP_MOVING,			(float)0		},
	{ TASK_PLAY_SEQUENCE,		(float)ACT_LAND	},
	{ TASK_GET_PATH_TO_LASTPOSITION,(float)0				},
	{ TASK_RUN_PATH,				(float)0				},
	{ TASK_WAIT_FOR_MOVEMENT,		(float)0				},
	{ TASK_CLEAR_LASTPOSITION,		(float)0				},
};

Schedule_t	slBanditRepelLand[] =
{
	{
		tlBanditRepelLand,
		ARRAYSIZE ( tlBanditRepelLand ),
		bits_COND_SEE_ENEMY			|
		bits_COND_NEW_ENEMY			|
		bits_COND_LIGHT_DAMAGE		|
		bits_COND_HEAVY_DAMAGE		|
		bits_COND_HEAR_SOUND,

		bits_SOUND_DANGER			|
		bits_SOUND_COMBAT			|
		bits_SOUND_PLAYER,
		"Repel Land"
	},
};

//=========================================================
// range attacks, by weapon, standing and crouched
//=========================================================
Task_t	tlBanditMachinegunStand[] =
{
	{ TASK_STOP_MOVING,				(float)0		},
	{ TASK_BANDIT_CHECK_FIRE,		(float)0		},
	{ TASK_RANGE_ATTACK1,			(float)0		},
	{ TASK_FACE_ENEMY,				(float)0		},
};

Schedule_t	slBanditMachinegunStand[] =
{
	{
		tlBanditMachinegunStand,
		ARRAYSIZE ( tlBanditMachinegunStand ),
		bits_COND_NEW_ENEMY			|
		bits_COND_ENEMY_DEAD		|
		bits_COND_HEAVY_DAMAGE		|
		bits_COND_ENEMY_OCCLUDED	|
		bits_COND_NO_AMMO_LOADED	|
		bits_COND_BANDIT_NOFIRE,
		0,
		"Machinegun range stand"
	},
};

Task_t	tlBanditMachinegunCrouch[] =
{
	{ TASK_STOP_MOVING,				(float)0		},
	{ TASK_PLAY_SEQUENCE_FACE_ENEMY,(float)ACT_CROUCH },
	{ TASK_BANDIT_CHECK_FIRE,		(float)0		},
	{ TASK_RANGE_ATTACK1,			(float)0		},
	{ TASK_FACE_ENEMY,				(float)0		},
};

Schedule_t	slBanditMachinegunCrouch[] =
{
	{
		tlBanditMachinegunCrouch,
		ARRAYSIZE ( tlBanditMachinegunCrouch ),
		bits_COND_NEW_ENEMY			|
		bits_COND_ENEMY_DEAD		|
		bits_COND_HEAVY_DAMAGE		|
		bits_COND_ENEMY_OCCLUDED	|
		bits_COND_NO_AMMO_LOADED	|
		bits_COND_BANDIT_NOFIRE,
		0,
		"Range machinegun crouch"
	},
};

Task_t	tlBanditRocketStand[] =
{
	{ TASK_STOP_MOVING,				(float)0		},
	{ TASK_BANDIT_CHECK_FIRE,		(float)0		},
	{ TASK_RANGE_ATTACK1,			(float)0		},
	{ TASK_FIND_COVER_FROM_ORIGIN,	(float)0		},
};

Schedule_t	slBanditRocketStand[] =
{
	{
		tlBanditRocketStand,
		ARRAYSIZE ( tlBanditRocketStand ),
		bits_COND_NEW_ENEMY			|
		bits_COND_HEAVY_DAMAGE		|
		bits_COND_ENEMY_OCCLUDED	|
		bits_COND_NO_AMMO_LOADED	|
		bits_COND_BANDIT_NOFIRE,
		0,
		"Rocket range stand"
	},
};

Task_t	tlBanditRocketCrouch[] =
{
	{ TASK_STOP_MOVING,				(float)0		},
	{ TASK_PLAY_SEQUENCE_FACE_ENEMY,(float)ACT_CROUCH },
	{ TASK_BANDIT_CHECK_FIRE,		(float)0		},
	{ TASK_RANGE_ATTACK1,			(float)0		},
	{ TASK_FIND_COVER_FROM_ORIGIN,	(float)0		},
};

Schedule_t	slBanditRocketCrouch[] =
{
	{
		tlBanditRocketCrouch,
		ARRAYSIZE ( tlBanditRocketCrouch ),
		bits_COND_NEW_ENEMY			|
		bits_COND_ENEMY_DEAD		|
		bits_COND_HEAVY_DAMAGE		|
		bits_COND_ENEMY_OCCLUDED	|
		bits_COND_NO_AMMO_LOADED	|
		bits_COND_BANDIT_NOFIRE,
		0,
		"rocketlauncher crouch"
	},
};

Task_t	tlBanditShotgunStand[] =
{
	{ TASK_STOP_MOVING,				(float)0		},
	{ TASK_FACE_ENEMY,				(float)0		},
	{ TASK_BANDIT_CHECK_FIRE,		(float)0		},
	{ TASK_RANGE_ATTACK1,			(float)0		},
};

Schedule_t	slBanditShotgunStand[] =
{
	{
		tlBanditShotgunStand,
		ARRAYSIZE ( tlBanditShotgunStand ),
		bits_COND_NEW_ENEMY			|
		bits_COND_HEAVY_DAMAGE		|
		bits_COND_ENEMY_OCCLUDED	|
		bits_COND_NO_AMMO_LOADED	|
		bits_COND_BANDIT_NOFIRE,
		0,
		"Shotgun range stand"
	},
};

Task_t	tlBanditShotgunCrouch[] =
{
	{ TASK_STOP_MOVING,				(float)0		},
	{ TASK_PLAY_SEQUENCE_FACE_ENEMY,(float)ACT_CROUCH },
	{ TASK_BANDIT_CHECK_FIRE,		(float)0		},
	{ TASK_RANGE_ATTACK1,			(float)0		},
};

Schedule_t	slBanditShotgunCrouch[] =
{
	{
		tlBanditShotgunCrouch,
		ARRAYSIZE ( tlBanditShotgunCrouch ),
		bits_COND_NEW_ENEMY			|
		bits_COND_ENEMY_DEAD		|
		bits_COND_HEAVY_DAMAGE		|
		bits_COND_ENEMY_OCCLUDED	|
		bits_COND_NO_AMMO_LOADED	|
		bits_COND_BANDIT_NOFIRE,
		0,
		"shotgun crouch"
	},
};

//=========================================================
// draw the guns
//=========================================================
Task_t	tlBanditDraw[] =
{
	{ TASK_STOP_MOVING,				(float)0		},
	{ TASK_PLAY_SEQUENCE_FACE_ENEMY,(float)ACT_ARM	},
	{ TASK_FACE_ENEMY,				(float)0		},
};

Schedule_t	slBanditDraw[] =
{
	{
		tlBanditDraw,
		ARRAYSIZE ( tlBanditDraw ),
		0,
		0,
		"Bandit Draw"
	},
};

//=========================================================
// roll aside and shoot
//=========================================================
Task_t	tlBanditRollLeft[] =
{
	{ TASK_WAIT,					(float)0.2		},
	{ TASK_BANDIT_ROLL_LEFT,		(float)0		},
	{ TASK_BANDIT_CHECK_FIRE,		(float)0		},
	{ TASK_RANGE_ATTACK1,			(float)0		},
};

Schedule_t	slBanditRollLeft[] =
{
	{
		tlBanditRollLeft,
		ARRAYSIZE ( tlBanditRollLeft ),
		0,
		0,
		"Bandit Roll Left"
	},
};

Task_t	tlBanditRollRight[] =
{
	{ TASK_WAIT,					(float)0.2		},
	{ TASK_BANDIT_ROLL_RIGHT,		(float)0		},
	{ TASK_BANDIT_CHECK_FIRE,		(float)0		},
	{ TASK_RANGE_ATTACK1,			(float)0		},
};

Schedule_t	slBanditRollRight[] =
{
	{
		tlBanditRollRight,
		ARRAYSIZE ( tlBanditRollRight ),
		0,
		0,
		"Bandit Roll Right"
	},
};

//=========================================================
// roll out of the enemy's line of fire
//=========================================================
Task_t	tlBanditDodgeLeft[] =
{
	{ TASK_SET_FAIL_SCHEDULE,		(float)SCHED_RANGE_ATTACK1	},
	{ TASK_STOP_MOVING,				(float)0		},
	{ TASK_BANDIT_ROLL_LEFT,		(float)0		},
	{ TASK_FACE_ENEMY,				(float)0		},
};

Schedule_t	slBanditDodgeLeft[] =
{
	{
		tlBanditDodgeLeft,
		ARRAYSIZE ( tlBanditDodgeLeft ),
		0,
		0,
		"Bandit Roll Left"
	},
};

Task_t	tlBanditDodgeRight[] =
{
	{ TASK_SET_FAIL_SCHEDULE,		(float)SCHED_RANGE_ATTACK1	},
	{ TASK_STOP_MOVING,				(float)0		},
	{ TASK_BANDIT_ROLL_RIGHT,		(float)0		},
	{ TASK_FACE_ENEMY,				(float)0		},
};

Schedule_t	slBanditDodgeRight[] =
{
	{
		tlBanditDodgeRight,
		ARRAYSIZE ( tlBanditDodgeRight ),
		0,
		0,
		"Bandit Roll Right"
	},
};

DEFINE_CUSTOM_SCHEDULES( CBaseBandit )
{
	slBanditFail,
	slBanditCombatFail,
	slBanditVictoryDance,
	slBanditEstablishLineOfFire,
	slBanditFoundEnemy,
	slBanditCombatFace,
	slBanditSignalSuppress,
	slBanditSuppress,
	slBanditWaitInCover,
	slBanditTakeCover,
	slBanditGrenadeCover,
	slBanditTossGrenadeCover,
	slBanditTakeCoverFromBestSound,
	slBanditHideReload,
	slBanditReload,
	slBanditSweep,
	slBanditRangeAttack2,
	slBanditRepel,
	slBanditRepelAttack,
	slBanditRepelLand,
	slBanditMachinegunStand,
	slBanditMachinegunCrouch,
	slBanditRocketStand,
	slBanditRocketCrouch,
	slBanditShotgunStand,
	slBanditShotgunCrouch,
	slBanditDraw,
	slBanditRollRight,
	slBanditRollLeft,
	slBanditDodgeRight,
	slBanditDodgeLeft,
};

IMPLEMENT_CUSTOM_SCHEDULES( CBaseBandit, CSquadMonster );

//=========================================================
// Get Schedule!
//=========================================================
Schedule_t *CBaseBandit :: GetSchedule( void )
{

	// clear old sentence
	m_iSentence = BANDIT_SENT_NONE;

	// flying? If PRONE, barnacle has me. IF not, it's assumed I am rapelling.
	if ( pev->movetype == MOVETYPE_FLY && m_MonsterState != MONSTERSTATE_PRONE )
	{
		if (pev->flags & FL_ONGROUND)
		{
			// just landed
			pev->movetype = MOVETYPE_STEP;
			return GetScheduleOfType ( SCHED_BANDIT_REPEL_LAND );
		}
		else
		{
			// repel down a rope,
			if ( m_MonsterState == MONSTERSTATE_COMBAT )
				return GetScheduleOfType ( SCHED_BANDIT_REPEL_ATTACK );
			else
				return GetScheduleOfType ( SCHED_BANDIT_REPEL );
		}
	}

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
				// and the grunt sees it and knows it's a grenade
				if (FOkToSpeak())
				{
					m_iSentence = BANDIT_SENT_GREN;
					SpeakSentence();
				}
				return GetScheduleOfType( SCHED_TAKE_COVER_FROM_BEST_SOUND );
			}

			if (!HasConditions( bits_COND_SEE_ENEMY ) && ( pSound->m_iType & (bits_SOUND_PLAYER | bits_SOUND_COMBAT) ))
			{
				MakeIdealYaw( pSound->m_vecOrigin );
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
							{
								// player
								m_iSentence = BANDIT_SENT_ALERT;
								SpeakSentence();
							}
							else if ((m_hEnemy != NULL) &&
									(m_hEnemy->Classify() == CLASS_ALIEN_BEAK ||
									m_hEnemy->Classify() != CLASS_ALIEN_TUBE ||
									m_hEnemy->Classify() != CLASS_ALIEN_CRITTER))	// sic: always true
							{
								// monster
								m_iSentence = BANDIT_SENT_MONSTER;
								SpeakSentence();
							}
						}

						if ( HasConditions ( bits_COND_CAN_RANGE_ATTACK1 ) && pev->weapons != BANDIT_ROCKETLAUNCHER )
						{
							return GetScheduleOfType ( SCHED_BANDIT_SUPPRESS );
						}
						else
						{
							return GetScheduleOfType ( SCHED_BANDIT_ESTABLISH_LINE_OF_FIRE );
						}
					}
				}
			}
// no ammo
			else if ( HasConditions ( bits_COND_NO_AMMO_LOADED ) )
			{
				//!!!KELLY - this individual just realized he's out of bullet ammo.
				// He's going to try to find cover to run to and reload, but rarely, if
				// none is available, he'll drop and reload in the open here.
				return GetScheduleOfType ( SCHED_BANDIT_COVER_AND_RELOAD );
			}

// damaged just a little
			else if ( HasConditions( bits_COND_LIGHT_DAMAGE ) )
			{
				// flinch the first time, then think about cover
				if ( !HasMemory( bits_MEMORY_FLINCHED ) )
				{
					Remember( bits_MEMORY_FLINCHED );
					return GetScheduleOfType( SCHED_SMALL_FLINCH );
				}

				if ( UTIL_PercentChance( 50 ) && m_hEnemy != NULL && InSquad() && FOkToSpeak() )
				{
					//!!!KELLY - this grunt was hit and is going to run to cover.
					m_iSentence = BANDIT_SENT_COVER;
					JustSpoke();	// sic: clears the sentence it just cued
					return GetScheduleOfType( SCHED_TAKE_COVER_FROM_ENEMY );
				}
			}
// can kick
			else if ( HasConditions ( bits_COND_CAN_MELEE_ATTACK1 ) )
			{
				return GetScheduleOfType ( SCHED_MELEE_ATTACK1 );
			}
// can grenade launch
			else if ( HasGrenades() && HasConditions ( bits_COND_CAN_RANGE_ATTACK2 ) )
			{
				// shoot a grenade if you can
				return GetScheduleOfType( SCHED_RANGE_ATTACK2 );
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
						return GetScheduleOfType ( SCHED_BANDIT_FOUND_ENEMY );
					}
				}

				UTIL_MakeVectors( pev->angles );

				// roll out from behind cover, toward the side a gun is on
				if ( HasConditions( bits_COND_ENEMY_OCCLUDED ) && m_hEnemy != NULL && GetGunsFiring() && UTIL_PercentChance( 50 ) )
				{
					Vector vecDir = ( pev->origin - m_hEnemy->pev->origin ).Normalize();

					if ( DotProduct( vecDir, gpGlobals->v_forward ) <= -0.9 )
					{
						switch ( GetGunsFiring() )
						{
						case 1:
							if ( CheckLocalMove( pev->origin, pev->origin + gpGlobals->v_right * 40, NULL, NULL ) == LOCALMOVE_VALID )
								return slBanditDodgeRight;
							break;
						case 2:
							if ( CheckLocalMove( pev->origin, pev->origin + gpGlobals->v_right * -40, NULL, NULL ) == LOCALMOVE_VALID )
								return slBanditDodgeLeft;
							break;
						}
					}
				}

				// roll out of the enemy's aim
				if ( m_flNextRollCheck < gpGlobals->time && GetGunsFiring() && HasConditions( bits_COND_ENEMY_FACING_ME ) )
				{
					UTIL_MakeVectors( pev->angles );

					Vector vecRight = pev->origin + gpGlobals->v_right * 40;
					Vector vecLeft = pev->origin + gpGlobals->v_right * -40;

					float flDotRight = DotProduct( ( vecRight - m_hEnemy->pev->origin ).Normalize(), gpGlobals->v_forward );
					float flDotLeft = DotProduct( ( vecLeft - m_hEnemy->pev->origin ).Normalize(), gpGlobals->v_forward );

					if ( flDotRight <= -0.98 && flDotRight < flDotLeft && CheckLocalMove( pev->origin, vecRight, NULL, NULL ) == LOCALMOVE_VALID )
					{
						m_flNextRollCheck = gpGlobals->time + RANDOM_LONG( 6, 18 );
						return slBanditDodgeRight;
					}

					if ( flDotLeft <= -0.98 && flDotLeft < flDotRight && CheckLocalMove( pev->origin, vecLeft, NULL, NULL ) == LOCALMOVE_VALID )
					{
						m_flNextRollCheck = gpGlobals->time + RANDOM_LONG( 6, 18 );
						return slBanditDodgeLeft;
					}

					m_flNextRollCheck = gpGlobals->time + 1;
				}

				if ( OccupySlot ( bits_SLOTS_HGRUNT_ENGAGE ) )
				{
					// try to take an available ENGAGE slot
					return GetScheduleOfType( SCHED_RANGE_ATTACK1 );
				}
				else if ( HasConditions ( bits_COND_CAN_RANGE_ATTACK2 ) && OccupySlot( bits_SLOTS_HGRUNT_GRENADE ) && HasGrenades() )
				{
					// throw a grenade if can and no engage slots are available
					return GetScheduleOfType( SCHED_RANGE_ATTACK2 );
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
				if ( HasConditions( bits_COND_CAN_RANGE_ATTACK2 ) && OccupySlot( bits_SLOTS_HGRUNT_GRENADE ) )
				{
					//!!!KELLY - this grunt is about to throw or fire a grenade at the player. Great place for "fire in the hole"  "frag out" etc
					if (FOkToSpeak())
					{
						m_iSentence = BANDIT_SENT_THROW;
						SpeakSentence();
					}
					return GetScheduleOfType( SCHED_RANGE_ATTACK2 );
				}
				else if ( OccupySlot( bits_SLOTS_HGRUNT_ENGAGE ) )
				{
					//!!!KELLY - grunt cannot see the enemy and has just decided to
					// charge the enemy's position.
					if (FOkToSpeak() && UTIL_PercentChance( 50 ))
					{
						m_iSentence = BANDIT_SENT_CHARGE;
					}

					return GetScheduleOfType( SCHED_BANDIT_ESTABLISH_LINE_OF_FIRE );
				}
				else
				{
					//!!!KELLY - grunt is going to stay put for a couple seconds to see if
					// the enemy wanders back out into the open, or approaches the
					// grunt's covered position. Good place for a taunt, I guess?
					if (FOkToSpeak() && UTIL_PercentChance( 50 ))
					{
						m_iSentence = BANDIT_SENT_TAUNT;
						SpeakSentence();
					}
					return GetScheduleOfType( SCHED_STANDOFF );
				}
			}

			if ( HasConditions( bits_COND_SEE_ENEMY ) && !HasConditions ( bits_COND_CAN_RANGE_ATTACK1 ) )
			{
				return GetScheduleOfType ( SCHED_BANDIT_ESTABLISH_LINE_OF_FIRE );
			}
		}
	}

	// no special cases here, call the base class
	return CSquadMonster :: GetSchedule();
}

//=========================================================
//=========================================================
Schedule_t* CBaseBandit :: GetScheduleOfType ( int Type )
{
	switch	( Type )
	{
	case SCHED_BANDIT_DRAW:
		{
			return &slBanditDraw[ 0 ];
		}
	case SCHED_TAKE_COVER_FROM_ENEMY:
		{
			if ( InSquad() )
			{
				if ( g_iSkillLevel == SKILL_HARD && HasConditions( bits_COND_CAN_RANGE_ATTACK2 ) && OccupySlot( bits_SLOTS_HGRUNT_GRENADE ) && HasGrenades() )
				{
					if (FOkToSpeak())
					{
						m_iSentence = BANDIT_SENT_THROW;
						SpeakSentence();
					}
					return slBanditTossGrenadeCover;
				}
				else
				{
					return &slBanditTakeCover[ 0 ];
				}
			}
			else
			{
				if ( UTIL_PercentChance( 75 ) && HasGrenades() )
				{
					return &slBanditGrenadeCover[ 0 ];
				}
				else
				{
					return &slBanditTakeCover[ 0 ];
				}
			}
		}
	case SCHED_TAKE_COVER_FROM_BEST_SOUND:
		{
			return &slBanditTakeCoverFromBestSound[ 0 ];
		}
	case SCHED_COWER:
		{
			// don't cower from a player in a vehicle, shoot at it
			if ( m_hEnemy != NULL && FClassnameIs( m_hEnemy->pev, "player" ) )
			{
				CBasePlayer *pPlayer = (CBasePlayer *)GET_PRIVATE( m_hEnemy->edict() );
				if ( pPlayer->m_pVehicle != NULL )
					return GetScheduleOfType( SCHED_RANGE_ATTACK1 );
			}
			return CBaseMonster :: GetScheduleOfType( SCHED_COWER );
		}
	case SCHED_BANDIT_TAKECOVER_FAILED:
		{
			if ( HasConditions( bits_COND_CAN_RANGE_ATTACK1 ) && !HasConditions( bits_COND_ENEMY_OCCLUDED ) && OccupySlot( bits_SLOTS_HGRUNT_ENGAGE ) )
			{
				return GetScheduleOfType( SCHED_RANGE_ATTACK1 );
			}

			return GetScheduleOfType ( SCHED_FAIL );
		}
		break;
	case SCHED_BANDIT_ESTABLISH_LINE_OF_FIRE:
		{
			return &slBanditEstablishLineOfFire[ 0 ];
		}
		break;
	case SCHED_RANGE_ATTACK1:
		{
			switch ( pev->weapons )
			{
			case BANDIT_MACHINEGUN2:
				return GetScheduleOfType( SCHED_BANDIT_RANGE_MACHINEGUN );
			case BANDIT_MACHINEGUN:
			case BANDIT_PISTOLS_CLOSE:
				return GetScheduleOfType( SCHED_BANDIT_RANGE_MACHINEGUN );
			case BANDIT_ROCKETLAUNCHER:
				return GetScheduleOfType( SCHED_BANDIT_RANGE_ROCKET );
			case BANDIT_SHOTGUN:
				return GetScheduleOfType( SCHED_BANDIT_RANGE_SHOTGUN );
			case BANDIT_ROCKETLAUNCHER2:
				return GetScheduleOfType( SCHED_BANDIT_RANGE_ROCKET );
			default:
				ALERT( at_console, "No firing for this type of weapon\n" );
				break;
			}
		}
		// sic: falls through
	case SCHED_BANDIT_ELOF_FAIL:
		{
			// human grunt is unable to move to a position that allows him to attack the enemy.
			return GetScheduleOfType ( SCHED_TAKE_COVER_FROM_ENEMY );
		}
		break;
	case SCHED_BANDIT_RANGE_MACHINEGUN:
		{
			if ( m_bStanding )
				return &slBanditMachinegunStand[ 0 ];
			else
				return &slBanditMachinegunCrouch[ 0 ];
		}
	case SCHED_BANDIT_RANGE_ROCKET:
		{
			if ( m_bStanding )
				return &slBanditRocketStand[ 0 ];
			else
				return &slBanditRocketCrouch[ 0 ];
		}
	case SCHED_BANDIT_RANGE_SHOTGUN:
		{
			if ( m_bStanding )
				return &slBanditShotgunStand[ 0 ];
			else
				return &slBanditShotgunCrouch[ 0 ];
		}
	case SCHED_RANGE_ATTACK2:
		{
			return &slBanditRangeAttack2[ 0 ];
		}
	case SCHED_COMBAT_FACE:
		{
			return &slBanditCombatFace[ 0 ];
		}
	case SCHED_BANDIT_WAIT_FACE_ENEMY:
		{
			return &slBanditWaitInCover[ 0 ];
		}
	case SCHED_BANDIT_SWEEP:
		{
			return &slBanditSweep[ 0 ];
		}
	case SCHED_BANDIT_COVER_AND_RELOAD:
		{
			return &slBanditHideReload[ 0 ];
		}
	case SCHED_RELOAD:
		{
			return &slBanditReload[ 0 ];
		}
	case SCHED_BANDIT_FOUND_ENEMY:
		{
			return &slBanditFoundEnemy[ 0 ];
		}
	case SCHED_VICTORY_DANCE:
		{
			if ( InSquad() )
			{
				if ( !IsLeader() )
				{
					return &slBanditFail[ 0 ];
				}
			}

			return &slBanditVictoryDance[ 0 ];
		}
	case SCHED_BANDIT_SUPPRESS:
		{
			if ( m_hEnemy->IsPlayer() && m_bFirstEncounter )
			{
				m_bFirstEncounter = FALSE;// after first encounter, leader won't issue handsigns anymore when he has a new enemy
				return &slBanditSignalSuppress[ 0 ];
			}
			else
			{
				return &slBanditSuppress[ 0 ];
			}
		}
	case SCHED_FAIL:
		{
			if ( m_hEnemy != NULL )
			{
				// grunt has an enemy, so pick a different default fail schedule most likely to help recover.
				return &slBanditCombatFail[ 0 ];
			}

			return &slBanditFail[ 0 ];
		}
	case SCHED_BANDIT_REPEL:
		{
			if (pev->velocity.z > -128)
				pev->velocity.z -= 32;
			return &slBanditRepel[ 0 ];
		}
	case SCHED_BANDIT_REPEL_ATTACK:
		{
			if (pev->velocity.z > -128)
				pev->velocity.z -= 32;
			return &slBanditRepelAttack[ 0 ];
		}
	case SCHED_BANDIT_REPEL_LAND:
		{
			return &slBanditRepelLand[ 0 ];
		}
	default:
		{
			return CSquadMonster :: GetScheduleOfType ( Type );
		}
	}
}

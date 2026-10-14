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
// the bandit: two gauss pistols, one in each hand, or the
// mechanical gun; monster_human_grunt is the same bandit
//=========================================================

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"schedule.h"
#include	"animation.h"
#include	"weapons.h"
#include	"soundent.h"
#include	"bandit.h"

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define		BANDIT_AE_SHOOT_LEFT		( 1 )	// a gauss beam from each gun
#define		BANDIT_AE_SHOOT_RIGHT		( 2 )
#define		BANDIT_AE_DRAW				( 3 )
#define		BANDIT_AE_HOLSTER			( 4 )
#define		BANDIT_AE_FASTSHOT_LEFT		( 5 )	// a fast shot from each gun
#define		BANDIT_AE_FASTSHOT_RIGHT	( 6 )
#define		BANDIT_AE_MECHAGUN			( 8 )
#define		BANDIT_AE_PUNCH				( 11 )

class CBanditGrunt : public CBaseBandit
{
public:
	CBanditGrunt( void );
	void Spawn( void );
	void Precache( void );
	void PrecacheSounds( void );
	void KeyValue( KeyValueData *pkvd );
	void SetYawSpeed( void );
	int  Classify( void ) { return CLASS_HUMAN_MILITARY; }
	int  MonsterType( void )
	{
		if ( g_Language == LANGUAGE_GERMAN || CVAR_GET_FLOAT( "violence_germany" ) != 0 )
			return 4;
		return 3;
	}
	void GetHeadGib( BOOL *pfHeadGib, int *piGroup, int *piHeadless )
	{
		*pfHeadGib = TRUE;
		*piGroup = 2;
		*piHeadless = 4;
	}
	int GetGunsFiring( void ) { return m_iGunsFiring; }
	BOOL CanCrouch( void ) { return TRUE; }
	void Crouch( void )
	{
		m_bStanding = FALSE;
		pev->view_ofs = Vector( 0, 0, 32 );
	}
	void Stand( void )
	{
		m_bStanding = TRUE;
		pev->view_ofs = Vector( 0, 0, 60 );
	}
	BOOL HasUnlimitedAmmo( void ) { return FALSE; }
	void HandleAnimEvent( MonsterEvent_t *pEvent );
	BOOL FCanCheckAttacks( void );
	BOOL CheckRangeAttack1( float flDot, float flDist );
	void RunAI( void );
	void RunTask( Task_t *pTask );
	void SetActivity( Activity NewActivity );
	void PainSound( void );
	void AttackHitSound( CBaseMonster *pVictim );
	void Killed( entvars_t *pevAttacker, int iGib );
	Vector GunPosition( int iGun );
	Schedule_t *GetSchedule( void );

	int	Save( CSave &save );
	int Restore( CRestore &restore );
	static TYPEDESCRIPTION m_SaveData[];

	static const char *pBanditSounds[];

	int		m_iHead;		// "Head", read but not used
	BOOL	m_bHasAmmo;
	int		m_iWeapon;		// "MechaGunBandit" makes this the mechanical gun
	int		m_iGunsFiring;	// which guns can hit the enemy
	int		m_usMechaGun;
};

LINK_ENTITY_TO_CLASS( monster_human_bandit, CBanditGrunt );
LINK_ENTITY_TO_CLASS( monster_human_grunt, CBanditGrunt );

TYPEDESCRIPTION	CBanditGrunt::m_SaveData[] =
{
	DEFINE_FIELD( CBanditGrunt, m_bHasAmmo, FIELD_BOOLEAN ),
	DEFINE_FIELD( CBanditGrunt, m_iWeapon, FIELD_INTEGER ),
};

IMPLEMENT_SAVERESTORE( CBanditGrunt, CBaseBandit );

const char *CBanditGrunt::pBanditSounds[] =
{
	"bandit/bandit_flinch1.wav",
	"bandit/bandit_flinch2.wav",
	"bandit/bandit_flinch3.wav",
	"bandit/bandit_walkcasual_stepright.wav",
	"bandit/bandit_walkcasual_stepleft.wav",
	"bandit/bandit_run_stepright.wav",
	"bandit/bandit_run_stepleft.wav",
	"bandit/bandit_diesimple.wav",
	"bandit/bandit_dieviolent.wav",
	"bandit/bandit_dieforward.wav",
	"bandit/bandit_diebackwards.wav",
	"bandit/bandit_dieheadshot.wav",
	"bandit/bandit_footdrag.wav",
	"bandit/bandit_draw.wav",
	"weapons/hks1.wav",
	"weapons/hks2.wav",
	"debris/bustflesh2.wav",
};

void CBanditGrunt::PrecacheSounds( void )
{
	for ( int i = 0; i < ARRAYSIZE( pBanditSounds ); i++ )
		PRECACHE_SOUND( (char *)pBanditSounds[i] );
}

void CBanditGrunt::PainSound( void )
{
	switch ( RANDOM_LONG( 0, 2 ) )
	{
	case 0:
		EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "bandit/bandit_flinch1.wav", 1, ATTN_NORM, 0, m_iVoicePitch );
		break;
	case 1:
		EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "bandit/bandit_flinch2.wav", 1, ATTN_NORM, 0, m_iVoicePitch );
		break;
	case 2:
		EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "bandit/bandit_flinch3.wav", 1, ATTN_NORM, 0, m_iVoicePitch );
		break;
	}
}

void CBanditGrunt::AttackHitSound( CBaseMonster *pVictim )
{
	EMIT_SOUND_DYN( pVictim->edict(), CHAN_VOICE, "debris/bustflesh2.wav", 0.5, ATTN_NORM, 0, 50 );
}

void CBanditGrunt::KeyValue( KeyValueData *pkvd )
{
	if ( FStrEq( pkvd->szKeyName, "Head" ) )
	{
		m_iHead = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}

	if ( FStrEq( pkvd->szKeyName, "StartDrawn" ) )
	{
		m_iGunsDrawn = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}

	if ( FStrEq( pkvd->szKeyName, "MechaGunBandit" ) )
	{
		if ( atoi( pkvd->szValue ) )
			m_iWeapon = BANDIT_MACHINEGUN2;
		pkvd->fHandled = TRUE;
	}
	else
		CBaseBandit::KeyValue( pkvd );
}

void CBanditGrunt::Killed( entvars_t *pevAttacker, int iGib )
{
	if ( m_bHasAmmo )
	{
		Vector vecGunAngles;	// sic: never set

		m_bHasAmmo = FALSE;
		SetBodygroup( 1, 3 );

		if ( m_iWeapon == BANDIT_MACHINEGUN2 )
		{
			CBaseEntity *pGun = DropItem( "weapon_minigun", GunPosition( 1 ), vecGunAngles );
			if ( pGun )
			{
				pGun->pev->velocity = Vector( RANDOM_FLOAT( -48, 48 ), RANDOM_FLOAT( -48, 48 ), RANDOM_FLOAT( 100, 200 ) );
				pGun->pev->avelocity = Vector( 0, RANDOM_FLOAT( 100, 200 ), 0 );
			}
		}
		else
		{
			CBaseEntity *pGun = DropItem( "weapon_gausspistol", GunPosition( 0 ), vecGunAngles );
			if ( pGun )
			{
				pGun->pev->velocity = Vector( RANDOM_FLOAT( -48, 48 ), RANDOM_FLOAT( -48, 48 ), RANDOM_FLOAT( 100, 200 ) );
				pGun->pev->avelocity = Vector( 0, RANDOM_FLOAT( 200, 400 ), 0 );
			}

			pGun = DropItem( "weapon_gausspistol", GunPosition( 1 ), vecGunAngles );
			if ( pGun )
			{
				pGun->pev->velocity = Vector( RANDOM_FLOAT( -48, 48 ), RANDOM_FLOAT( -48, 48 ), RANDOM_FLOAT( 100, 200 ) );
				pGun->pev->avelocity = Vector( 0, RANDOM_FLOAT( 200, 400 ), 0 );
			}
		}
	}

	CBaseBandit::Killed( pevAttacker, iGib );
}

BOOL CBanditGrunt::FCanCheckAttacks( void )
{
	if ( !HasConditions( bits_COND_ENEMY_TOOFAR ) )
		return TRUE;
	else
		return FALSE;
}

//=========================================================
// GunPosition - the muzzle of gun 0 or 1; the mechanical
// gun is at the shoulder
//=========================================================
Vector CBanditGrunt::GunPosition( int iGun )
{
	Vector vecPos;
	Vector vecAngles;

	if ( pev->weapons == BANDIT_MACHINEGUN2 )
	{
		vecPos = pev->origin + gpGlobals->v_up * 54 + gpGlobals->v_right * 10;
	}
	else
	{
		GetAttachment( iGun, vecPos, vecAngles );

		// sic: the attachment is thrown away
		switch ( iGun )
		{
		case 0:
			vecPos = pev->origin + gpGlobals->v_up * 54 + gpGlobals->v_right * 10;
			break;
		case 1:
			vecPos = pev->origin + gpGlobals->v_up * 54 + gpGlobals->v_right * -10;
			break;
		}
	}

	return vecPos;
}

//=========================================================
// CheckRangeAttack1 - crouched far away, then standing;
// remembers which guns are clear
//=========================================================
BOOL CBanditGrunt::CheckRangeAttack1( float flDot, float flDist )
{
	BOOL fClear = FALSE;

	if ( ( !HasUnlimitedAmmo() && HasConditions( bits_COND_NO_AMMO_LOADED ) ) || flDist >= 4096 || flDot <= 0.1 || !NoFriendlyFire() )
		return FALSE;

	TraceResult tr;

	if ( CanCrouch() && m_hEnemy != NULL && ( flDist > 512 || !m_bStanding ) )
	{
		Vector vecSrc;
		vecSrc = GunPosition( 0 );
		vecSrc.z -= 20;

		UTIL_TraceLine( vecSrc, m_hEnemy->BodyTarget( GunPosition( 0 ) ), dont_ignore_monsters, ignore_glass, ENT(pev), &tr );

		if ( tr.flFraction == 1 || tr.pHit == m_hEnemy->edict() )
		{
			m_iGunsFiring = 1;
			fClear = TRUE;
		}

		if ( pev->weapons != BANDIT_MACHINEGUN2 )
		{
			vecSrc = GunPosition( 1 );
			vecSrc.z -= 20;

			UTIL_TraceLine( vecSrc, m_hEnemy->BodyTarget( GunPosition( 1 ) ), dont_ignore_monsters, ignore_glass, ENT(pev), &tr );

			if ( tr.flFraction == 1 || tr.pHit == m_hEnemy->edict() )
			{
				m_iGunsFiring = 2;

				if ( pev->weapons == BANDIT_MACHINEGUN2 )
				{
					m_bStanding = FALSE;
					return TRUE;
				}

				if ( fClear )
				{
					m_bStanding = FALSE;
					return TRUE;
				}
			}
		}
	}

	m_bStanding = TRUE;
	fClear = FALSE;

	UTIL_TraceLine( GunPosition( 0 ), m_hEnemy->BodyTarget( GunPosition( 0 ) ), dont_ignore_monsters, ignore_glass, ENT(pev), &tr );

	if ( tr.flFraction == 1 || tr.pHit == m_hEnemy->edict() )
	{
		m_iGunsFiring = 1;
		fClear = TRUE;
	}

	if ( pev->weapons == BANDIT_MACHINEGUN2 )
		return fClear;

	UTIL_TraceLine( GunPosition( 1 ), m_hEnemy->BodyTarget( GunPosition( 1 ) ), dont_ignore_monsters, ignore_glass, ENT(pev), &tr );

	if ( tr.flFraction == 1 || tr.pHit == m_hEnemy->edict() )
	{
		m_iGunsFiring = 1;

		if ( !fClear )
			return FALSE;

		// close up, the two guns fire fast
		if ( flDist <= 512 && !UTIL_PercentChance( 10 ) )
		{
			pev->weapons = BANDIT_PISTOLS_CLOSE;
			return TRUE;
		}

		pev->weapons = BANDIT_MACHINEGUN;
		return TRUE;
	}

	if ( !fClear )
		return FALSE;

	pev->weapons = BANDIT_MACHINEGUN;
	return TRUE;
}

void CBanditGrunt::SetYawSpeed( void )
{
	int ys;

	switch ( m_Activity )
	{
	case ACT_IDLE:
		ys = 100;
		break;
	case ACT_RUN:
		ys = 100;
		break;
	case ACT_WALK:
	case ACT_TURN_LEFT:
	case ACT_TURN_RIGHT:
		ys = 130;
		break;
	case ACT_RANGE_ATTACK1:
	case ACT_RANGE_ATTACK2:
	case ACT_MELEE_ATTACK1:
	case ACT_MELEE_ATTACK2:
		ys = 80;
		break;
	case ACT_GLIDE:
	case ACT_FLY:
		ys = 20;
		break;
	default:
		ys = 120;
		break;
	}

	pev->yaw_speed = ys;
}

//=========================================================
// RunAI - rolls play faster
//=========================================================
void CBanditGrunt::RunAI( void )
{
	if ( m_Activity >= ACT_ROLL_LEFT && m_Activity <= ACT_ROLL_RIGHT )
		pev->framerate = 1.5;
	else
		pev->framerate = 1;

	CBaseBandit::RunAI();
}

//=========================================================
// GetSchedule - draw the guns before a fight
//=========================================================
Schedule_t *CBanditGrunt::GetSchedule( void )
{
	if ( m_iGunsDrawn == 1 && GetBodygroup( 1 ) == 1 )
		SetBodygroup( 1, 0 );

	if ( m_MonsterState >= MONSTERSTATE_COMBAT && m_MonsterState <= MONSTERSTATE_ALERT && m_iGunsDrawn == 2 )
		return GetScheduleOfType( SCHED_BANDIT_DRAW );

	return CBaseBandit::GetSchedule();
}

void CBanditGrunt::HandleAnimEvent( MonsterEvent_t *pEvent )
{
	switch ( pEvent->event )
	{
	case BANDIT_AE_SHOOT_LEFT:
		{
			Vector vecGunPos;
			Vector vecGunAngles;
			GetAttachment( 0, vecGunPos, vecGunAngles );
			ShootGauss( vecGunPos, 0 );
		}
		break;

	case BANDIT_AE_SHOOT_RIGHT:
		{
			Vector vecGunPos;
			Vector vecGunAngles;
			GetAttachment( 1, vecGunPos, vecGunAngles );
			ShootGauss( vecGunPos, 1 );
		}
		break;

	case BANDIT_AE_DRAW:
		m_iGunsDrawn = 1;
		SetBodygroup( 1, 0 );
		break;

	case BANDIT_AE_HOLSTER:
		m_iGunsDrawn = 2;
		SetBodygroup( 1, 1 );
		break;

	case BANDIT_AE_FASTSHOT_LEFT:
		{
			Vector vecGunPos;
			Vector vecGunAngles;
			GetAttachment( 0, vecGunPos, vecGunAngles );
			ShootFastShot( vecGunPos );
		}
		break;

	case BANDIT_AE_FASTSHOT_RIGHT:
		{
			Vector vecGunPos;
			Vector vecGunAngles;
			GetAttachment( 1, vecGunPos, vecGunAngles );
			ShootFastShot( vecGunPos );
		}
		break;

	case BANDIT_AE_MECHAGUN:
		{
			Vector vecShootOrigin = GunPosition( 0 );
			Vector vecShootDir = ShootAtEnemy( vecShootOrigin );

			Vector vecMuzzle;
			GET_ATTACHMENT( ENT(pev), 0, vecMuzzle, NULL );

			PLAYBACK_EVENT_FULL( FEV_RELIABLE, ENT(pev), m_usMechaGun, 0.0, (float *)&vecMuzzle, (float *)&vecShootDir, 0.0, 0.0, 0, 0, 0, 0 );

			FireBullets( 1, vecShootOrigin, vecShootDir, VECTOR_CONE_3DEGREES, 4096, BULLET_PLAYER_9MM, 4, gSkillData.banditDmgMech );

			m_cAmmoLoaded--;// take away a bullet!

			if ( RANDOM_LONG( 0, 1 ) )
				EMIT_SOUND( ENT(pev), CHAN_WEAPON, "weapons/hks1.wav", 0.5, 0.6 );
			else
				EMIT_SOUND( ENT(pev), CHAN_WEAPON, "weapons/hks2.wav", 0.5, 0.6 );

			pev->effects |= EF_MUZZLEFLASH;

			Vector angDir = UTIL_VecToAngles( vecShootDir );
			SetBlending( 0, angDir.x );
		}
		break;

	case BANDIT_AE_PUNCH:
		{
			CBaseEntity *pHurt = CheckTraceHullAttack( 128, gSkillData.banditDmgMelee, DMG_SLASH );

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
					pMonster->TraceBleed( gSkillData.rustbitDmgClaw, ( pMonster->BodyTarget( g_vecZero ) - BodyTarget( g_vecZero ) ).Normalize(), &tr, DMG_SLASH, tr.vecEndPos );	// sic: the rustbit's claw damage

					if ( pMonster->IsPlayer() )
						AttackHitSound( pMonster );
				}
			}
		}
		break;

	default:
		CBaseBandit::HandleAnimEvent( pEvent );
		break;
	}
}

void CBanditGrunt::RunTask( Task_t *pTask )
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
	default:
		{
			CBaseBandit::RunTask( pTask );
			break;
		}
	}
}

void CBanditGrunt::SetActivity( Activity NewActivity )
{
	int	iSequence = ACTIVITY_NOT_AVAILABLE;
	void *pmodel = GET_MODEL_PTR( ENT(pev) );

	if ( pev->weapons == BANDIT_MACHINEGUN2 )
	{
		switch ( NewActivity )
		{
		case ACT_IDLE:
			if ( UTIL_PercentChance( 15 ) )
				iSequence = LookupSequence( "mechaidleactive" );
			else
				iSequence = LookupSequence( "mechaidle" );
			break;
		case ACT_WALK:
			iSequence = LookupSequence( "mechawalk" );
			break;
		case ACT_RUN:
			iSequence = LookupSequence( "mecharun" );
			break;
		case ACT_CROUCH:
			iSequence = LookupSequence( "mcrouch#mechacrouching" );
			break;
		case ACT_STAND:
			iSequence = LookupSequence( "mechastanding" );
			break;
		case ACT_RANGE_ATTACK1:
			if ( m_bStanding )
				iSequence = LookupSequence( "mechashooting" );
			else
				iSequence = LookupSequence( "mechacrouchshooting" );
			break;
		case ACT_RELOAD:
			if ( m_bStanding )
				iSequence = LookupSequence( "mechareload" );
			else
				iSequence = LookupSequence( "mechacrouchreload" );
			break;
		case ACT_MELEE_ATTACK1:
			iSequence = LookupSequence( "mechapunch" );
			break;
		default:
			CBaseBandit::SetActivity( NewActivity );
			return;
		}
	}
	else
	{
		switch ( NewActivity )
		{
		case ACT_RANGE_ATTACK1:
			switch ( pev->weapons )
			{
			case BANDIT_MACHINEGUN:
				if ( m_bStanding )
					iSequence = LookupSequence( "shooting" );
				else
					iSequence = LookupSequence( "crouchfire" );
				break;
			case BANDIT_PISTOLS_CLOSE:
				if ( m_bStanding )
					iSequence = LookupSequence( "rapidfire" );
				else
					iSequence = LookupSequence( "crouchrapidfire" );
				break;
			}
			break;
		case ACT_WALK:
			if ( pev->health <= gSkillData.banditLimpHealth )
			{
				// limp!
				iSequence = LookupActivity( ACT_WALK_HURT );
			}
			else
			{
				iSequence = LookupActivity( NewActivity );
			}
			break;
		case ACT_RUN:
			if ( pev->health <= gSkillData.banditLimpHealth )
			{
				// limp!
				iSequence = LookupActivity( ACT_RUN_HURT );
			}
			else
			{
				iSequence = LookupActivity( NewActivity );
			}
			break;
		case ACT_RELOAD:
			if ( m_bStanding )
				iSequence = LookupSequence( "reload" );
			else
				iSequence = LookupSequence( "crouchreload" );
			break;
		case ACT_IDLE:
			if ( m_bStanding )
			{
				CBaseBandit::SetActivity( NewActivity );
				return;
			}
			iSequence = LookupSequence( "crouch#idle" );
			break;
		default:
			CBaseBandit::SetActivity( NewActivity );
			return;
		}
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

CBanditGrunt::CBanditGrunt( void )
{
	m_iGunsDrawn = 1;
	m_iWeapon = BANDIT_MACHINEGUN;
}

void CBanditGrunt::Precache( void )
{
	PRECACHE_MODEL( "models/bandit.mdl" );
	UTIL_PrecacheOther( "ammo_gaussclip" );
	UTIL_PrecacheOther( "ammo_minigunClip" );
	PRECACHE_MODEL( "models/shell.mdl" );

	m_usMechaGun = PRECACHE_EVENT( 1, "events/monstermechagun.sc" );

	CBaseBandit::Precache( );
}

void CBanditGrunt::Spawn( void )
{
	Precache( );

	SET_MODEL( ENT(pev), "models/bandit.mdl" );
	pev->health = gSkillData.banditHealth;
	m_flFieldOfView = 0.2;

	CBaseBandit::Spawn( );
	MonsterInit( );

	pev->view_ofs = Vector( 0, 0, 60 );
	m_afCapability &= ~bits_CAP_RANGE_ATTACK2;

	// the head: the level's body number, or any of three
	SetBodygroup( 2, pev->body == 0 ? RANDOM_LONG( 0, 2 ) : pev->body - 1 );

	if ( UTIL_PercentChance( 50 ) )
		pev->skin = 0;
	else
		pev->skin = 1;

	pev->weapons = m_iWeapon;

	if ( m_iWeapon == BANDIT_MACHINEGUN2 )
	{
		pev->weapons = BANDIT_MACHINEGUN2;
		m_cClipSize = 24;
		SetBodygroup( 1, 2 );
		SetBodygroup( 3, 1 );
	}
	else
	{
		if ( m_iGunsDrawn != 1 )
			SetBodygroup( 1, 1 );
		else
			SetBodygroup( 1, 0 );
		m_cClipSize = 18;
	}

	m_bHasAmmo = TRUE;
	m_cAmmoLoaded = m_cClipSize;
}

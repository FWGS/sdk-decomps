/***
*
*	Counter-Strike Beta 1.1, hostage.cpp: the hostage (hostage_entity), the
*	map's spawn entity.
*
*	A scientist model that stands where the map put it until a player uses
*	it. Used by a player it follows him (counter-terrorists hear it answer),
*	used again it stays; a hostage told to stay backs away from a player who
*	comes closer than 200 units. Shot, it flinches by hit group; killed, it
*	costs its killer 850 dollars and its body is thrown away from him, then
*	hidden three seconds later. ClientCommand's "hostage" rescues it,
*	RestartRound puts it back. File, class and method names are Beta 3.1's
*	(Linux symbols); member names are ours (hostage.h).
*
****/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"
#include "gamerules.h"
#include "hostage.h"
#include "../engine/studio.h"

LINK_ENTITY_TO_CLASS( hostage_entity, CHostage );

void CHostage::Spawn( void )
{
	Precache();

	pev->classname	= MAKE_STRING( "hostage_entity" );
	pev->movetype	= MOVETYPE_STEP;
	pev->solid		= SOLID_SLIDEBOX;
	pev->takedamage	= DAMAGE_YES;
	pev->flags		|= FL_MONSTER;
	pev->deadflag	= DEAD_NO;
	pev->health		= 100;
	pev->gravity	= 1.0;

	SET_MODEL( ENT(pev), "models/scientist.mdl" );
	SetActivity( ACT_IDLE );
	SetSequenceBox();

	m_flNextUse = 0;
	m_iUnused12C = 0;
	m_flFollowRange = 1024;
	m_fStopped = TRUE;
	m_hTargetEnt = NULL;
	m_flFollowDistance = 128;
	m_flFlinchTime = 0;

	UTIL_SetSize( pev, Vector( -16, -16, 0 ), Vector( 16, 16, 72 ) );
	UTIL_MakeVectors( pev->v_angle );
	float flYaw = UTIL_VecToYaw( gpGlobals->v_forward );
	SetBoneController( 0, flYaw );

	SetThink( IdleThink );
	pev->nextthink = gpGlobals->time + 0.1;

	m_vecStart = pev->origin;
}

void CHostage::Precache( void )
{
	PRECACHE_MODEL( "models/scientist.mdl" );
	PRECACHE_SOUND( "hostage/hos1.wav" );
	PRECACHE_SOUND( "hostage/hos2.wav" );
	PRECACHE_SOUND( "hostage/hos3.wav" );
	PRECACHE_SOUND( "hostage/hos4.wav" );
	PRECACHE_SOUND( "hostage/hos5.wav" );
}

int CHostage::Classify( void )
{
	return CLASS_HUMAN_PASSIVE;
}

void CHostage::SetActivity( int activity )
{
	int iSequence = LookupActivity( activity );

	if ( pev->sequence != iSequence )
	{
		pev->gaitsequence = iSequence;
		pev->sequence = iSequence;
		m_iActivity = activity;
		pev->frame = 0;
		ResetSequenceInfo();
	}
}

// the size of the current sequence's box. Both new's leak: the pointers are
// overwritten at once (sic).
void CHostage::SetCollisionBox( void )
{
	studiohdr_t *pstudiohdr = new studiohdr_t;
	pstudiohdr = (studiohdr_t *)GET_MODEL_PTR( ENT(pev) );

	mstudioseqdesc_t *pseqdesc = new mstudioseqdesc_t;
	pseqdesc = (mstudioseqdesc_t *)((byte *)pstudiohdr + pstudiohdr->seqindex);

	Vector vecMins, vecMaxs;
	vecMins = pseqdesc[ pev->sequence ].bbmin;
	vecMaxs = pseqdesc[ pev->sequence ].bbmax;

	UTIL_SetSize( pev, vecMins, vecMaxs );
}

void CHostage::IdleThink( void )
{
	float flInterval = StudioFrameAdvance();
	DispatchAnimEvents( flInterval );
	pev->nextthink = gpGlobals->time + 0.1;

	if ( pev->deadflag != DEAD_DEAD )
	{
		// the leader is dead: nobody to follow
		if ( m_hTargetEnt != NULL && m_hTargetEnt->pev->deadflag != DEAD_NO )
			m_hTargetEnt = NULL;

		if ( m_hTargetEnt != NULL && ( pev->origin - m_hTargetEnt->pev->origin ).Length() < 1024 && m_fStopped == FALSE )
		{
			// follow: turn to the leader and run to him
			TraceResult tr;
			Vector vecDir = m_hTargetEnt->pev->origin - pev->origin;

			pev->angles = UTIL_VecToAngles( vecDir );
			pev->angles.x = 0;
			pev->angles.z = 0;

			if ( vecDir.Length() > m_flFollowDistance )
				pev->velocity = vecDir / vecDir.Length() * 265;

			// blocked at the feet but free higher up: jump
			UTIL_MakeVectors( pev->angles );
			UTIL_TraceLine( pev->origin + gpGlobals->v_up * 2 - gpGlobals->v_forward * 32, pev->origin + gpGlobals->v_forward * 100 + gpGlobals->v_up * 2, ignore_monsters, ENT( pev ), &tr );
			if ( tr.flFraction != 1.0 )
			{
				UTIL_TraceLine( pev->origin + gpGlobals->v_up * 25 - gpGlobals->v_forward * 32, pev->origin + gpGlobals->v_forward * 100 + gpGlobals->v_up * 25, ignore_monsters, ENT( pev ), &tr );
				if ( tr.flFraction == 1.0 )
				{
					ALERT( at_console, "There's something at my feet!\n" );
					pev->velocity.z = 150;
				}
			}
		}
		else if ( m_hTargetEnt != NULL && ( pev->origin - m_hTargetEnt->pev->origin ).Length() < 200 && m_fStopped == TRUE )
		{
			// told to stay, but the leader is too close: back away from him
			TraceResult tr;
			Vector vecDir = pev->origin - m_hTargetEnt->pev->origin;

			pev->angles = UTIL_VecToAngles( vecDir );
			pev->angles.x = 0;
			pev->angles.z = 0;

			pev->velocity = vecDir / vecDir.Length() * 145;

			UTIL_MakeVectors( pev->angles );
			UTIL_TraceLine( pev->origin + gpGlobals->v_up * 2 - gpGlobals->v_forward * 32, pev->origin + gpGlobals->v_forward * 100 + gpGlobals->v_up * 2, ignore_monsters, ENT( pev ), &tr );
			if ( tr.flFraction != 1.0 )
			{
				UTIL_TraceLine( pev->origin + gpGlobals->v_up * 25 - gpGlobals->v_forward * 32, pev->origin + gpGlobals->v_forward * 100 + gpGlobals->v_up * 25, ignore_monsters, ENT( pev ), &tr );
				if ( tr.flFraction == 1.0 )
				{
					ALERT( at_console, "There's something at my feet!\n" );
					pev->velocity.z = 150;
				}
			}
		}

		if ( gpGlobals->time >= m_flFlinchTime )
		{
			if ( pev->velocity.Length() > 120 )
				SetActivity( ACT_RUN );
			else if ( pev->velocity.Length() > 15 )
				SetActivity( ACT_WALK );
			else
				SetActivity( ACT_IDLE );
		}
	}
	else
		UTIL_SetSize( pev, Vector( 0, 0, 0 ), Vector( 0, 0, 0 ) );
}

void CHostage::Remove( void )
{
	pev->movetype = MOVETYPE_NONE;
	pev->solid = SOLID_NOT;
	pev->takedamage = DAMAGE_NO;
	pev->effects |= EF_NODRAW;

	UTIL_SetSize( pev, Vector( 0, 0, 0 ), Vector( 0, 0, 0 ) );
	pev->nextthink = -1;
}

// back to where Spawn found it, alive again (its health is not restored)
void CHostage::RePosition( void )
{
	pev->movetype = MOVETYPE_STEP;
	pev->solid = SOLID_SLIDEBOX;
	pev->takedamage = DAMAGE_YES;
	pev->deadflag = DEAD_NO;
	pev->effects &= ~EF_NODRAW;

	m_hTargetEnt = NULL;
	m_fTouched = FALSE;

	UTIL_SetOrigin( pev, m_vecStart );
	UTIL_SetSize( pev, Vector( -16, -16, 0 ), Vector( 16, 16, 72 ) );

	pev->velocity.z = 50;
	pev->nextthink = 0.1;	// sic: not gpGlobals->time + 0.1
	SetThink( IdleThink );
}

int CHostage::TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType )
{
	int iForce;	// how hard the body is thrown; not set for HITGROUP_GENERIC (sic)

	switch ( m_LastHitGroup )
	{
	case HITGROUP_GENERIC:
		break;
	case HITGROUP_HEAD:
		if ( RANDOM_LONG( 0, 1 ) )
			EMIT_SOUND( ENT(pev), CHAN_VOICE, "player/headshot1.wav", 1, ATTN_NORM );
		else
			EMIT_SOUND( ENT(pev), CHAN_VOICE, "player/headshot2.wav", 1, ATTN_NORM );
		SetActivity( ACT_SMALL_FLINCH );
		flDamage *= 2.5;
		break;
	case HITGROUP_CHEST:
		SetActivity( ACT_SMALL_FLINCH );
		flDamage *= 1.5;
		break;
	case HITGROUP_STOMACH:
		SetActivity( ACT_SMALL_FLINCH );
		flDamage *= 1.75;
		break;
	case HITGROUP_LEFTARM:
		SetActivity( ACT_FLINCH_LEFTARM );
		flDamage *= 0.75;
		break;
	case HITGROUP_RIGHTARM:
		SetActivity( ACT_FLINCH_RIGHTARM );
		flDamage *= 0.75;
		break;
	case HITGROUP_LEFTLEG:
		SetActivity( ACT_FLINCH_LEFTLEG );
		flDamage *= 0.6;
		break;
	case HITGROUP_RIGHTLEG:
		SetActivity( ACT_FLINCH_RIGHTLEG );
		flDamage *= 0.6;
		break;
	}

	m_flFlinchTime = gpGlobals->time + 0.75;
	pev->health -= flDamage;

	if ( pev->health <= 0 )
	{
		pev->health = 0;
		pev->movetype = MOVETYPE_TOSS;
		pev->flags &= ~FL_ONGROUND;

		switch ( m_LastHitGroup )
		{
		case HITGROUP_GENERIC:
			break;
		case HITGROUP_HEAD:
			SetActivity( ACT_DIE_HEADSHOT );
			iForce = 2;
			break;
		case HITGROUP_CHEST:
			SetActivity( ACT_DIESIMPLE );
			iForce = 2;
			break;
		case HITGROUP_STOMACH:
			SetActivity( ACT_DIEFORWARD );
			iForce = 1;
			break;
		case HITGROUP_LEFTARM:
			SetActivity( ACT_DIEBACKWARD );
			iForce = 2;
			break;
		case HITGROUP_RIGHTARM:
			SetActivity( ACT_DIESIMPLE );
			iForce = 2;
			break;
		case HITGROUP_LEFTLEG:
			SetActivity( ACT_DIEBACKWARD );
			iForce = 1;
			break;
		case HITGROUP_RIGHTLEG:
			SetActivity( ACT_DIEFORWARD );
			iForce = 1;
			break;
		}

		// killing a hostage costs the killer 850 dollars
		if ( GetClassPtr( (CBaseEntity *)pevAttacker )->IsPlayer() )
		{
			CBasePlayer *pPlayer = GetClassPtr( (CBasePlayer *)pevAttacker );

			pPlayer->m_iAccount -= 850;
			if ( pPlayer->m_iAccount < 0 )
				pPlayer->m_iAccount = 0;
		}

		// the body flies away from the killer
		pev->velocity = pev->origin - pevAttacker->origin;
		pev->velocity = pev->velocity / pev->velocity.Length();
		pev->velocity = pev->velocity * iForce * RANDOM_FLOAT( 100, 150 );
		pev->velocity.z += RANDOM_FLOAT( 100, 200 ) * iForce;

		if ( pev->velocity.Length() > 350 )
			pev->velocity = pev->velocity / pev->velocity.Length() * 350;

		pev->takedamage = DAMAGE_NO;
		pev->deadflag = DEAD_DEAD;
		pev->nextthink = gpGlobals->time + 3;
		SetThink( Remove );
		return 0;
	}

	return 1;
}

void CHostage::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if ( gpGlobals->time < m_flNextUse )
		return;

	if ( pev->takedamage == DAMAGE_NO )
		return;

	// a third state is treated as TRUE; retail's code keeps the test against
	// TRUE without a jump, the two branches being the same
	if ( m_fStopped == FALSE )
	{
		m_flNextUse = gpGlobals->time + 1;
		m_fStopped = TRUE;
	}
	else if ( m_fStopped == TRUE )
	{
		m_flNextUse = gpGlobals->time + 1;
		m_fStopped = FALSE;
	}
	else
	{
		m_flNextUse = gpGlobals->time + 1;
		m_fStopped = FALSE;
	}

	if ( pActivator->IsPlayer() )
	{
		// a new leader is followed at once
		if ( pActivator != m_hTargetEnt )
			m_fStopped = FALSE;

		if ( ((CBasePlayer *)pActivator)->m_iTeam == TEAM_TERRORIST )
		{
			m_hTargetEnt = pActivator;
		}
		else if ( ((CBasePlayer *)pActivator)->m_iTeam == TEAM_CT )
		{
			m_hTargetEnt = pActivator;

			if ( m_fStopped == FALSE )
			{
				switch ( RANDOM_LONG( 0, 4 ) )
				{
				case 0:	EMIT_SOUND( ENT(pev), CHAN_VOICE, "hostage/hos1.wav", 1, ATTN_NORM ); break;
				case 1:	EMIT_SOUND( ENT(pev), CHAN_VOICE, "hostage/hos2.wav", 1, ATTN_NORM ); break;
				case 2:	EMIT_SOUND( ENT(pev), CHAN_VOICE, "hostage/hos3.wav", 1, ATTN_NORM ); break;
				case 3:	EMIT_SOUND( ENT(pev), CHAN_VOICE, "hostage/hos4.wav", 1, ATTN_NORM ); break;
				case 4:	EMIT_SOUND( ENT(pev), CHAN_VOICE, "hostage/hos5.wav", 1, ATTN_NORM ); break;
				}
			}

			// the first counter-terrorist to lead it pays himself and his team
			if ( !m_fTouched )
			{
				m_fTouched = TRUE;
				((CHalfLifeMultiplay *)g_pGameRules)->m_iAccountCT += 150;
				((CBasePlayer *)pActivator)->m_iAccount += 150;
			}
		}
	}

	if ( m_fStopped == TRUE )
		m_flFollowRange = 300;
	else if ( m_fStopped == FALSE )
		m_flFollowRange = 1024;
}

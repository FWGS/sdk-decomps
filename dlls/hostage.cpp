/***
*
*	Counter-Strike Beta 2.0, hostage.cpp: the hostage (hostage_entity), the
*	map's spawn entity.
*
*	A scientist model that stands where the map put it until a player uses
*	it. Used by a player it follows him (counter-terrorists hear it answer),
*	used again it stays; a hostage told to stay backs away from a player who
*	comes closer than 200 units. Shot, it flinches by hit group; killed, it
*	costs its killer 850 dollars and its body is thrown away from him, then
*	hidden three seconds later. ClientCommand's "hostage" rescues it,
*	RestartRound puts it back. Beta 2.0: it keeps 82 units from its leader
*	(36 when backing away), is pushed down while it runs, and a hostage whose
*	leader is out of reach is in a third state (m_fStopped 2); the money goes
*	through AddAccount. File, class and method names are Beta 3.1's
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
	m_vecPath[0] = Vector( 0, 0, 0 );	// Beta 3.0

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

	if ( iSequence != -1 && pev->sequence != iSequence )	// Beta 3.0: a missing activity changes nothing
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
		{
			m_fStopped = 2;
			m_hTargetEnt = NULL;
		}

		if ( m_hTargetEnt != NULL && ( pev->origin - m_hTargetEnt->pev->origin ).Length() < 1024 && m_fStopped == FALSE )
		{
			// follow: turn to the leader and run to him
			TraceResult tr;
			Vector vecDir = m_hTargetEnt->pev->origin - pev->origin;

			pev->angles = UTIL_VecToAngles( vecDir );
			pev->angles.x = 0;
			pev->angles.z = 0;

			// Beta 3.0: with a wall between, follow the leader's trail (m_fStopped 6)
			UTIL_TraceLine( pev->origin + gpGlobals->v_up * 25, pev->origin + vecDir * 2048 + gpGlobals->v_up * 25, dont_ignore_monsters, ENT( pev ), &tr );
			if ( VARS( tr.pHit )->solid == SOLID_BSP )
			{
				m_fStopped = 6;
				return;
			}

			if ( vecDir.Length() > m_flFollowDistance )
			{
				pev->velocity = vecDir / vecDir.Length() * 310;
				pev->velocity.z -= 40;
			}

			// blocked at the feet but free higher up: jump
			UTIL_MakeVectors( pev->angles );
			UTIL_TraceLine( pev->origin + gpGlobals->v_up * 2 - gpGlobals->v_forward * 32, pev->origin + gpGlobals->v_forward * 100 + gpGlobals->v_up * 2, ignore_monsters, ENT( pev ), &tr );
			if ( tr.flFraction != 1.0 )
			{
				UTIL_TraceLine( pev->origin + gpGlobals->v_up * 25 - gpGlobals->v_forward * 32, pev->origin + gpGlobals->v_forward * 100 + gpGlobals->v_up * 25, ignore_monsters, ENT( pev ), &tr );
				if ( tr.flFraction == 1.0 )
					pev->velocity.z = 150;
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

			if ( vecDir.Length() > m_flFollowDistance )
			{
				pev->velocity = vecDir / vecDir.Length() * 110;
				pev->velocity.z -= 40;
			}

			UTIL_MakeVectors( pev->angles );
			UTIL_TraceLine( pev->origin + gpGlobals->v_up * 2 - gpGlobals->v_forward * 32, pev->origin + gpGlobals->v_forward * 100 + gpGlobals->v_up * 2, ignore_monsters, ENT( pev ), &tr );
			if ( tr.flFraction != 1.0 )
			{
				UTIL_TraceLine( pev->origin + gpGlobals->v_up * 25 - gpGlobals->v_forward * 32, pev->origin + gpGlobals->v_forward * 100 + gpGlobals->v_up * 25, ignore_monsters, ENT( pev ), &tr );
				if ( tr.flFraction == 1.0 )
					pev->velocity.z = 150;
			}
		}
		else if ( m_fStopped == 6 && m_hTargetEnt != NULL )
		{
			// Beta 3.0: the leader is out of sight; walk along his trail
			if ( m_hTargetEnt->IsPlayer() && m_vecPath[0] == Vector( 0, 0, 0 ) )
			{
				// take over the leader's last 20 positions and start at the nearest
				CBasePlayer *pPlayer = GetClassPtr( (CBasePlayer *)m_hTargetEnt->pev );

				for ( int i = 0; i < 20; i++ )
					m_vecPath[i] = pPlayer->m_vecOldOrigins[i];

				TraceResult tr;
				Vector vecDir;
				float flNearest = 99999;	// name ours

				for ( i = 0; i <= 19; i++ )	// "<= 19": retail compares with 19 here, with 20 in the loop above
				{
					vecDir = m_vecPath[i] - pev->origin;
					if ( vecDir.Length() < flNearest )
					{
						flNearest = vecDir.Length();
						m_iPathIndex = i;
					}
				}
			}
			else
			{
				if ( !m_hTargetEnt->IsPlayer() )
				{
					m_fStopped = 2;
					return;
				}

				UTIL_MakeVectors( pev->angles );

				Vector vecDir = m_vecPath[ m_iPathIndex ] + gpGlobals->v_up * -15 - pev->origin;
				Vector vecLeader = m_hTargetEnt->pev->origin - pev->origin;	// name ours
				TraceResult tr;

				// the leader in sight again: follow him
				UTIL_TraceLine( pev->origin + gpGlobals->v_up * 25, pev->origin + vecLeader * 2048 + gpGlobals->v_up * 25, dont_ignore_monsters, ENT( pev ), &tr );
				if ( VARS( tr.pHit )->solid != SOLID_BSP )
				{
					m_fStopped = FALSE;
					m_vecPath[0] = Vector( 0, 0, 0 );
					return;
				}

				if ( vecDir.Length() > 32 )
				{
					pev->angles = UTIL_VecToAngles( vecDir );
					pev->angles.x = 0;
					pev->angles.z = 0;
					pev->velocity = vecDir / vecDir.Length() * 310;
					pev->velocity.z -= 40;
				}
				else
				{
					// this point reached: on to the next (newer) one
					m_iPathIndex--;
					if ( m_iPathIndex < 0 )
					{
						// the end of the trail: take the leader's trail afresh
						m_vecPath[0] = Vector( 0, 0, 0 );
						m_fStopped = 6;
						return;
					}
				}

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
		}
		else
			m_fStopped = 2;	// no leader near: neither following nor told to stay

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

	pev->velocity.z = -80;	// Beta 3.0 (was 50: up)
	pev->nextthink = 0.1;	// sic: not gpGlobals->time + 0.1
	SetThink( IdleThink );
}

int CHostage::TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType )
{
	int iForce;	// how hard the body is thrown

	// Beta 3.0: a hit of no particular group counts as a stomach hit, and
	// any other as a chest hit (each written as a case of its own: the
	// object's constants come in the order of first use, 1.75 before 2.5,
	// the code in the order of the later copy of equal cases)
	switch ( m_LastHitGroup )
	{
	case HITGROUP_GENERIC:
		SetActivity( ACT_SMALL_FLINCH );
		flDamage *= 1.75;
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
	default:
		SetActivity( ACT_SMALL_FLINCH );
		flDamage *= 1.5;
		break;
	}

	m_flFlinchTime = gpGlobals->time + 0.75;
	pev->health -= flDamage;

	if ( pev->health <= 0 )
	{
		pev->health = 0;
		pev->movetype = MOVETYPE_TOSS;
		pev->flags &= ~FL_ONGROUND;

		// Beta 3.0: the counter-terrorists' commander radios it
		if ( ((CHalfLifeMultiplay *)g_pGameRules)->m_pCTCommander != NULL )
			((CHalfLifeMultiplay *)g_pGameRules)->m_pCTCommander->Radio( "hosdown" );

		// Beta 3.0: a hit of no particular group, and any other, dies as from a head shot
		switch ( m_LastHitGroup )
		{
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
		case HITGROUP_GENERIC:
		case HITGROUP_HEAD:
			SetActivity( ACT_DIE_HEADSHOT );
			iForce = 2;
			break;
		default:	// written apart, as the jump table shows (a shared label drops cases 0 and 1 from it)
			SetActivity( ACT_DIE_HEADSHOT );
			iForce = 2;
			break;
		}

		// killing a hostage costs the killer 850 dollars
		if ( GetClassPtr( (CBaseEntity *)pevAttacker )->IsPlayer() )
		{
			CBasePlayer *pPlayer = GetClassPtr( (CBasePlayer *)pevAttacker );

			pPlayer->AddAccount( -850 );
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

	m_flNextUse = gpGlobals->time + 1;

	if ( pActivator->IsPlayer() )
	{
		// Beta 3.0: the leader's use makes a following hostage stay; any
		// other use, or a new leader, makes it follow
		if ( pActivator == m_hTargetEnt && m_fStopped == FALSE )
			m_fStopped = TRUE;
		else
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
				((CBasePlayer *)pActivator)->AddAccount( 150 );
			}
		}
	}

	if ( m_fStopped == TRUE )
	{
		m_flFollowDistance = 36;
		m_flFollowRange = 300;
	}
	else if ( m_fStopped == FALSE )
	{
		m_flFollowDistance = 128;
		m_flFollowRange = 1024;
	}
}

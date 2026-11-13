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
*	Beta 4.1: ClientCommand's "hostage" is gone; a following hostage that
*	comes within 200 units of a rescue point rescues itself (IdleThink), the
*	leader pushes it away by walking into it (Touch), it has to be faced to be
*	used, follows from 2048 units and backs away from 256.
*
*	Counter-Strike 1.0: Half-Life's monster_scientist is a hostage too; the
*	map's spawn flag 4 makes it collide with func_monsterclip (Spawn); in the
*	training course (rules that are not multiplayer) IdleThink stops it at a
*	func_monsterclip and TakeDamage tells the rules of a killed hostage.
*
****/

#include "extdll.h"
#include "util.h"
// The include of studio.h (needed since Beta 6.5 for studiohdr_t) stands
// ahead of cbase.h: the place is ours, but the image pins that this file's
// list does not begin extdll.h util.h cbase.h. In every server from Beta 6.5
// to 1.3 hostage.obj has GetClassPtr<CBaseEntity> ahead of
// CHostage::BloodColor, the order of a source that writes its own .pch, which
// under /YX a list beginning with those three headers never does in a
// Rebuild All (it reuses the three-header .pch). Counter-Strike 1.0's PDB
// has one link, so no later build can explain it (measured: compiled after
// the three-header .pch this text gives retail's object; with the include
// behind hostage.h, 79 bytes differ)
#include "../engine/studio.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"
#include "gamerules.h"
#include "hostage.h"
#include "trains.h"	// Beta 7.1: CFuncVehicle (TakeDamage); the include's place is ours

extern void Broadcast( const char *sentence );	// multiplay_gamerules.cpp (Beta 3.0)
extern void ShowMenu( CBasePlayer *pPlayer, int bitsValidSlots, int nDisplayTime, BOOL fNeedMore, char *pszText );	// client.cpp (Beta 5.0 leak)

LINK_ENTITY_TO_CLASS( hostage_entity, CHostage );
LINK_ENTITY_TO_CLASS( monster_scientist, CHostage );	// Counter-Strike 1.0: Half-Life's scientist is a hostage

void CHostage::Spawn( void )
{
	Precache();

	pev->classname	= MAKE_STRING( "hostage_entity" );
	pev->movetype	= MOVETYPE_STEP;
	pev->solid		= SOLID_SLIDEBOX;
	pev->takedamage	= DAMAGE_YES;
	pev->flags		|= FL_MONSTER;
	pev->deadflag	= DEAD_NO;
	pev->max_health	= 100;			// Beta 5.0 (leak)
	pev->health		= pev->max_health;
	pev->gravity	= 1.0;
	pev->view_ofs	= Vector( 0, 0, 12 );	// Beta 6.0

	// Counter-Strike 1.0: the monsters' spawn flag (the SDK's monsters.h:
	// SF_MONSTER_HITMONSTERCLIP, 4; CBaseMonster::MonsterInit's lines)
	if ( pev->spawnflags & 4 )
		pev->flags |= FL_MONSTERCLIP;

	// Beta 5.0: the map may give the hostage a model and a skin
	if ( pev->skin < 0 )
		pev->skin = 0;

	SET_MODEL( ENT(pev), STRING( pev->model ) );
	SetActivity( ACT_IDLE );
	SetSequenceBox();

	m_flNextUse = 0;
	m_flNextRescueCheck = 0;
	m_flFollowRange = 1024;
	m_fStopped = TRUE;
	m_hTargetEnt = NULL;
	m_flFollowDistance = 128;
	m_flFlinchTime = 0;
	m_vecPath[0] = Vector( 0, 0, 0 );	// Beta 3.0
	m_fRescued = FALSE;					// Beta 5.0 (leak)

	UTIL_SetSize( pev, Vector( -10, -10, 0 ), Vector( 10, 10, 62 ) );	// Beta 6.0: smaller (was 32 by 32 by 72)
	UTIL_MakeVectors( pev->v_angle );
	float flYaw = UTIL_VecToYaw( gpGlobals->v_forward );
	SetBoneController( 0, flYaw );

	SetThink( IdleThink );
	pev->nextthink = gpGlobals->time + 0.1;
	// Beta 5.0 (leak): no SetTouch( Touch ) (Beta 4.1's): the engine calls the
	// Touch override anyway

	m_vecStart = pev->origin;
}

void CHostage::Precache( void )
{
	// Beta 5.0: the scientist unless the map names another model
	if ( !pev->model )
		pev->model = MAKE_STRING( "models/scientist.mdl" );

	PRECACHE_MODEL( (char *)STRING( pev->model ) );
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
	studiohdr_t *pstudiohdr;
	pstudiohdr = (studiohdr_t *)GET_MODEL_PTR( ENT(pev) );

	mstudioseqdesc_t *pseqdesc;
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

	// sic: a stand-in for text the image cannot give. Two facts ask for it.
	// Every Linux library from Beta 3.1 on has CHostage::GetActivity written
	// out, which egcs does for the inline functions a file calls, yet no code
	// reads the member: the call is in text that compiles to nothing. And
	// IdleThink's inline expansion (which Vector operators and constructors
	// are called, which expanded) follows the inliner's budget, twice the
	// function's size as written: Beta 6.5's code needs 40 to 56 units more
	// than the visible text has, Beta 6.6's (the same text plus one log
	// line) exactly 44 (11 unused int locals; 10 and 12 do not fit). The
	// Betas' block, "if ( GetActivity() ) SetActivity( ACT_WALK ); else
	// SetActivity( ACT_IDLE );" ahead of the function's other inline calls,
	// with the unused bContinue below, gives retail's bytes in both versions
	// (Beta 6.5's "== ACT_IDLE" form is 4 units too long for 6.6); the real
	// text is not known.
	// Counter-Strike 1.0 (Visual C++ 6.0 SP4, the monster clip block added
	// below): still needed. Measured in its place: 8 to 12 dead stores to one
	// int give retail's bytes, 7 and 13 do not, none or unused uninitialised
	// locals do not; without bContinue 73 lines differ. In Counter-Strike 1.1
	// (the rescue's log line is longer there) the window is one wide: exactly
	// 9 dead stores give retail's bytes (8 and 10 do not, nor 1 to 7 and 11
	// to 30), with any form of the new log line's arguments. The Betas'
	// block is worth 12, the top of 1.0's window, and does not fit 1.1; this
	// one-armed block is worth the 9 and gives retail's bytes in 1.0, 1.0b
	// and 1.1 alike (each compiled and compared; so does the same test with
	// != ACT_IDLE and SetActivity( ACT_IDLE )), and not in Betas 6.6, 7.0 and
	// 7.1. So the form changes once, in Counter-Strike 1.0; whether the text
	// really changed then or in 1.1 is not shown. "if ( GetActivity() )"
	// alone, a bare call, or a local set from it do not fit.
	if ( 0 )
	{
		if ( GetActivity() == ACT_IDLE )
			SetActivity( ACT_WALK );
	}

	if ( pev->deadflag != DEAD_DEAD )
	{
		// the leader is dead: nobody to follow
		if ( m_hTargetEnt != NULL && m_hTargetEnt->pev->deadflag != DEAD_NO )
		{
			m_fStopped = 2;
			m_hTargetEnt = NULL;
		}

		if ( m_hTargetEnt != NULL && ( pev->origin - m_hTargetEnt->pev->origin ).Length() < 3024 && m_fStopped == FALSE )	// Beta 6.0: 3024 (Beta 4.1: 2048, Beta 5.0 leak: 1024)
		{
			TraceResult tr;

			// Beta 5.0 (leak): a hostage that follows a counter-terrorist is rescued in a
			// func_hostage_rescue (m_fRescued), or on a map without one within 256 units of
			// an info_hostage_rescue (without those: of an info_player_start); checked at
			// every think, worth 1000 dollars to him (Beta 4.1: once a second, 200 units, 900)
			CBasePlayer *pPlayer = GetClassPtr( (CBasePlayer *)m_hTargetEnt->pev );	// names ours

			// Beta 5.0: only while a counter-terrorist leads it; anybody else (or
			// nobody) leading it clears the rescue (names ours)
			if ( pPlayer )
			{
				if ( !((CHalfLifeMultiplay *)g_pGameRules)->m_fMapHasRescueZone )
				{
					if ( pPlayer->m_iTeam == TEAM_CT )
					{
					// the three locals as Counter-Strike 1.6 still has them (its debug
					// information: names, the lines, bContinue never used): the first
					// search goes to pSpot, the loops are while loops
					CBaseEntity *pSpot;
					BOOL bContinue = FALSE, bResHostagePt = FALSE;

					pSpot = UTIL_FindEntityByClassname( NULL, "info_hostage_rescue" );
					if ( pSpot )
						bResHostagePt = TRUE;

					pSpot = UTIL_FindEntityByClassname( NULL, "info_hostage_rescue" );
					while ( pSpot != NULL )
					{
						if ( ( pSpot->pev->origin - pev->origin ).Length() < 256 )
						{
							m_fRescued = TRUE;	// Beta 6.0: no console line any more
							break;
						}

						pSpot = UTIL_FindEntityByClassname( pSpot, "info_hostage_rescue" );
					}

					if ( !bResHostagePt )
					{
						pSpot = UTIL_FindEntityByClassname( NULL, "info_player_start" );
						while ( pSpot != NULL )
						{
							if ( ( pSpot->pev->origin - pev->origin ).Length() < 256 )
							{
								m_fRescued = TRUE;
								break;
							}

							pSpot = UTIL_FindEntityByClassname( pSpot, "info_player_start" );
						}
					}
					}
					else
						m_fRescued = FALSE;
				}

				// Beta 5.0: no "Hostage rescued." on the leader's screen any more
				if ( m_fRescued && pPlayer->m_iTeam == TEAM_CT )
				{
					pev->deadflag = DEAD_RESPAWNABLE;	// rescued
					pPlayer->AddAccount( 1000, true );
					UTIL_LogPrintf( "%s has rescued a hostage\n", STRING( pPlayer->pev->netname ) );	// Beta 6.6
					Remove();
					((CHalfLifeMultiplay *)g_pGameRules)->m_iHostagesRescued++;
					((CHalfLifeMultiplay *)g_pGameRules)->CheckWinConditions();
					Broadcast( "rescued" );
				}
			}
			else
				m_fRescued = FALSE;

			// follow: turn to the leader and run to him
			Vector vecDir = m_hTargetEnt->pev->origin - pev->origin;

			pev->angles = UTIL_VecToAngles( vecDir );
			pev->angles.x = 0;
			pev->angles.z = 0;

			// Beta 6.0: the leader out of sight (a straight line 12 units up,
			// through glass) sends it along his trail (m_fStopped 6); Beta 3.0
			// to 5.2 looked 2048 units ahead for a wall
			UTIL_TraceLine( pev->origin + Vector( 0, 0, 12 ), m_hTargetEnt->pev->origin + Vector( 0, 0, 12 ), ignore_monsters, ignore_glass, ENT( pev ), &tr );
			if ( tr.flFraction != 1.0 )
			{
				m_fStopped = 6;
				m_flNextStuckCheck = gpGlobals->time + 3;
				return;
			}

			if ( vecDir.Length() > m_flFollowDistance )
			{
				pev->velocity = vecDir / vecDir.Length() * 370;	// Beta 4.0 (was 310)
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
		else if ( m_hTargetEnt != NULL && ( pev->origin - m_hTargetEnt->pev->origin ).Length() < 200 && m_fStopped == TRUE )	// Beta 4.1: 256 (Beta 5.0 leak: 200 again)
		{
			// told to stay, but the leader is too close: back away from him
			TraceResult tr;
			Vector vecDir = pev->origin - m_hTargetEnt->pev->origin;

			pev->angles = UTIL_VecToAngles( vecDir );
			pev->angles.x = 0;
			pev->angles.z = 0;

			if ( vecDir.Length() > m_flFollowDistance )
			{
				pev->velocity = vecDir / vecDir.Length() * 110;	// Beta 4.1: 200 (Beta 5.0 leak: 110 again)
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
		else if ( m_hTargetEnt != NULL && m_fStopped == 6 )
		{
			// Beta 3.0: the leader is out of sight; walk along his trail
			// (Beta 6.0: no console lines here any more)
			if ( m_hTargetEnt->IsPlayer() && m_vecPath[0] == Vector( 0, 0, 0 ) )
			{
				// take over the leader's last 20 positions and start at the
				// nearest one in sight (Beta 6.0: a straight line 12 units up)
				CBasePlayer *pPlayer = GetClassPtr( (CBasePlayer *)m_hTargetEnt->pev );

				for ( int i = 0; i < 20; i++ )
					m_vecPath[i] = pPlayer->m_vecOldOrigins[i];

				TraceResult tr;
				Vector vecDir;
				float flNearest = 9999;	// name ours (Beta 5.0 to 5.2: 99999)

				for ( i = 0; i <= 19; i++ )	// "<= 19": retail compares with 19 here, with 20 in the loop above
				{
					vecDir = m_vecPath[i] - pev->origin;
					float flDistance = ( m_vecPath[i] - pev->origin ).Length();	// name ours

					UTIL_TraceLine( m_vecPath[i] + Vector( 0, 0, 12 ), pev->origin + Vector( 0, 0, 12 ), ignore_monsters, ignore_glass, ENT( pev ), &tr );
					if ( flDistance < flNearest && tr.flFraction == 1.0 )
					{
						m_iPathIndex = i;
						flNearest = flDistance;
					}
				}
			}
			else
			{
				if ( m_hTargetEnt != NULL && !m_hTargetEnt->IsPlayer() )
				{
					m_fStopped = 2;
					return;
				}

				if ( m_hTargetEnt == NULL )
				{
					m_fStopped = 2;
					return;
				}

				UTIL_MakeVectors( pev->angles );

				Vector vecDir = m_vecPath[ m_iPathIndex ] + gpGlobals->v_up * -15 - pev->origin;
				Vector vecLeader = m_hTargetEnt->pev->origin - pev->origin;	// name ours; Beta 6.0: not used any more (sic)
				TraceResult tr;

				// the leader in sight again: follow him
				UTIL_TraceLine( pev->origin + Vector( 0, 0, 12 ), m_hTargetEnt->pev->origin + Vector( 0, 0, 12 ), ignore_monsters, ignore_glass, ENT( pev ), &tr );
				if ( tr.flFraction == 1.0 )
				{
					m_fStopped = FALSE;
					m_vecPath[0] = Vector( 0, 0, 0 );
					return;
				}

				// Beta 6.0: not 4 units away from where it was at the last
				// check: stuck, so take the leader's trail afresh
				if ( ( m_vecStuckCheckPos - pev->origin ).Length() <= 4 )
				{
					m_vecStuckCheckPos = Vector( 0, 0, 0 );
					m_vecPath[0] = Vector( 0, 0, 0 );
					m_fStopped = 6;
					m_flNextStuckCheck = gpGlobals->time + 3;
					return;
				}

				if ( vecDir.Length() > 16 )	// Beta 6.0: 16 (was 32)
				{
					pev->angles = UTIL_VecToAngles( vecDir );
					pev->angles.x = 0;
					pev->angles.z = 0;
					pev->velocity = vecDir / vecDir.Length() * 380;	// Beta 4.0 (was 310)
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
						m_flNextStuckCheck = gpGlobals->time + 3;
						return;
					}
				}

				UTIL_TraceLine( pev->origin + gpGlobals->v_up * 2 - gpGlobals->v_forward * 32, pev->origin + gpGlobals->v_forward * 100 + gpGlobals->v_up * 2, ignore_monsters, ENT( pev ), &tr );
				if ( tr.flFraction != 1.0 )
				{
					UTIL_TraceLine( pev->origin + gpGlobals->v_up * 25 - gpGlobals->v_forward * 32, pev->origin + gpGlobals->v_forward * 100 + gpGlobals->v_up * 25, ignore_monsters, ENT( pev ), &tr );
					if ( tr.flFraction == 1.0 )
						pev->velocity.z = 150;	// Beta 6.0: no "There's something at my feet!" any more
				}

				// Beta 6.0: every 3 seconds the position is noted for the stuck check
				if ( m_flNextStuckCheck < gpGlobals->time )
				{
					m_flNextStuckCheck = gpGlobals->time + 3;
					m_vecStuckCheckPos = pev->origin;
				}
			}
		}
		else
			m_fStopped = 2;	// no leader near: neither following nor told to stay

		// Counter-Strike 1.0: in the training course a hostage that cannot
		// walk 32 units ahead is stopped where its box, 10 units wider,
		// meets a func_monsterclip, and put on the floor when in the air
		// (from the binary; names ours)
		Vector vecOrigin = pev->origin;

		if ( !g_pGameRules->IsMultiplayer() )
		{
			if ( !WALK_MOVE( ENT( pev ), pev->angles.y, 32, WALKMOVE_CHECKONLY ) )
			{
				pev->absmax.x += 10;
				pev->absmax.y += 10;
				pev->absmin.x -= 10;
				pev->absmin.y -= 10;

				CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "func_monsterclip" );
				while ( pEntity != NULL )
				{
					if ( Intersects( pEntity ) )
					{
						pev->velocity = g_vecZero;

						if ( !( pev->flags & FL_ONGROUND ) )
						{
							pev->origin = vecOrigin;
							DROP_TO_FLOOR( ENT( pev ) );
							vecOrigin = pev->origin;
						}
					}

					pEntity = UTIL_FindEntityByClassname( pEntity, "func_monsterclip" );
				}

				pev->absmax.x -= 10;
				pev->absmax.y -= 10;
				pev->absmin.x += 10;
				pev->absmin.y += 10;
			}
		}

		pev->origin = vecOrigin;

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

// back to where Spawn found it, alive again (Beta 5.0 leak: and healed)
void CHostage::RePosition( void )
{
	pev->health = pev->max_health;	// Beta 5.0 (leak): healed at every new round
	pev->movetype = MOVETYPE_STEP;
	pev->solid = SOLID_SLIDEBOX;
	pev->takedamage = DAMAGE_YES;
	pev->deadflag = DEAD_NO;
	pev->effects &= ~EF_NODRAW;

	m_hTargetEnt = NULL;
	m_fTouched = FALSE;
	m_fRescued = FALSE;	// Beta 5.0: a rescued hostage is back in play at a new round

	UTIL_SetOrigin( pev, m_vecStart );
	UTIL_SetSize( pev, Vector( -16, -16, 0 ), Vector( 16, 16, 72 ) );

	pev->velocity.z = -80;	// Beta 3.0 (was 50: up)
	pev->nextthink = 0.1;	// sic: not gpGlobals->time + 0.1
	SetThink( IdleThink );
}

int CHostage::TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType )
{
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

		// Beta 3.0: a hit of no particular group, and any other, dies as from a head shot
		// (Beta 6.5: the body is not thrown any more)
		switch ( m_LastHitGroup )
		{
		case HITGROUP_CHEST:
			SetActivity( ACT_DIESIMPLE );
			break;
		case HITGROUP_STOMACH:
			SetActivity( ACT_DIEFORWARD );
			break;
		case HITGROUP_LEFTARM:
			SetActivity( ACT_DIEBACKWARD );
			break;
		case HITGROUP_RIGHTARM:
			SetActivity( ACT_DIESIMPLE );
			break;
		case HITGROUP_LEFTLEG:
			SetActivity( ACT_DIEBACKWARD );
			break;
		case HITGROUP_RIGHTLEG:
			SetActivity( ACT_DIEFORWARD );
			break;
		case HITGROUP_GENERIC:
		case HITGROUP_HEAD:
			SetActivity( ACT_DIE_HEADSHOT );
			break;
		default:
			SetActivity( ACT_DIE_HEADSHOT );
			break;
		}

		// Beta 7.1: run over by a vehicle, its driver is the attacker
		// (sic: before the test for an attacker; GetClassPtr makes an entity for none)
		CBaseEntity *pAttackingEnt = GetClassPtr( (CBaseEntity *)pevAttacker );	// Counter-Strike 1.6's name

		if ( pAttackingEnt->Classify() == CLASS_VEHICLE )
		{
			pAttackingEnt = ((CFuncVehicle *)pAttackingEnt)->m_pDriver;

			if ( pAttackingEnt )
				pevAttacker = pAttackingEnt->pev;
		}

		// Beta 6.0: only with an attacker (a hostage killed by nobody pays nothing)
		if ( pevAttacker )
		{
			// killing a hostage costs the killer 2250 dollars (Beta 3.1: 850, Beta 5.0: 1500)
			if ( GetClassPtr( (CBaseEntity *)pevAttacker )->IsPlayer() )
			{
				CBasePlayer *pPlayer = GetClassPtr( (CBasePlayer *)pevAttacker );

				pPlayer->AddAccount( -2250, true );
				ClientPrint( pPlayer->pev, HUD_PRINTCENTER, "#Killed_Hostage" );	// Beta 5.0 (leak); Beta 7.1: a titles.txt name
				if ( !(pPlayer->m_flDisplayHistory & 0x40) )	// hostage killed
				{
					pPlayer->HintMessage( "You have lost money for killing a hostage.", FALSE, FALSE );
					pPlayer->m_flDisplayHistory |= 0x40;
				}

				// Counter-Strike 1.0: the training course is told
				if ( !g_pGameRules->IsMultiplayer() )
					((CHalfLifeTraining *)g_pGameRules)->HostageDied();

				// Beta 6.5: mp_hostagepenalty, the number of hostages a terrorist may
				// kill; one before it he is warned, at it the server drops him
				int iHostagePenalty = CVAR_GET_FLOAT( "mp_hostagepenalty" );	// name ours

				if ( pPlayer->m_iTeam == TEAM_TERRORIST && iHostagePenalty != 0 )
				{
					pPlayer->m_iHostagesKilled++;

					if ( pPlayer->m_iHostagesKilled == iHostagePenalty - 1 )
						pPlayer->HintMessage( "If you kill one more hostage, you will be removed from the server.", TRUE, FALSE );
					else if ( pPlayer->m_iHostagesKilled >= iHostagePenalty )
						CLIENT_COMMAND( pPlayer->edict(), "disconnect\n" );
				}

				// Beta 7.1: the server log
				char szText[256];	// name ours

				sprintf( szText, "%s killed a hostage\n", STRING( pPlayer->pev->netname ) );
				UTIL_LogPrintf( "%s", szText );
			}
		}

		pev->takedamage = DAMAGE_NO;
		pev->deadflag = DEAD_DEAD;
		pev->solid = SOLID_NOT;	// Beta 6.5
		pev->nextthink = gpGlobals->time + 3;
		SetThink( Remove );

		// Beta 4.0: a dead hostage may decide the round; then the counter-terrorists'
		// commander radios it (Beta 3.0: before the death animation)
		((CHalfLifeMultiplay *)g_pGameRules)->CheckWinConditions();

		if ( ((CHalfLifeMultiplay *)g_pGameRules)->m_pCTCommander != NULL )
			((CHalfLifeMultiplay *)g_pGameRules)->m_pCTCommander->Radio( "%!MRAD_HOSDOWN", "Hostage down.\n" );

		return 0;
	}

	// Beta 6.0: no attacker, no damage reported
	if ( !pevAttacker )
		return 0;

	// Beta 5.0 (leak): a player who hurts a hostage is told so, and the first
	// time given a hint
	if ( GetClassPtr( (CBaseEntity *)pevAttacker )->IsPlayer() )
	{
		CBasePlayer *pPlayer = GetClassPtr( (CBasePlayer *)pevAttacker );	// name ours

		ClientPrint( pPlayer->pev, HUD_PRINTCENTER, "#Injured_Hostage" );
		if ( !(pPlayer->m_flDisplayHistory & 0x20) )	// hostage injured
		{
			pPlayer->HintMessage( "Be careful around hostages.\nYou will lose money if you kill a hostage.", FALSE, FALSE );
			pPlayer->m_flDisplayHistory |= 0x20;
		}
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
		// Beta 6.0: his path is recorded for the hostage (CBasePlayer::UpdatePath)
		((CBasePlayer *)pActivator)->m_bLeadsHostage = true;

		// Beta 3.0: the leader's use makes a following hostage stay; any
		// other use, or a new leader, makes it follow
		// Beta 7.0: it stays by losing its leader (state 2: no leader within
		// reach) rather than by the stopped state
		if ( pActivator == m_hTargetEnt && m_fStopped == FALSE )
		{
			m_hTargetEnt = NULL;
			m_fStopped = 2;
		}
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
			// (Beta 4.0: 500 each, was 150; Beta 5.0 leak: 150 to him, 100 to the team)
			if ( !m_fTouched )
			{
				m_fTouched = TRUE;
				((CHalfLifeMultiplay *)g_pGameRules)->m_iAccountCT += 100;
				((CBasePlayer *)pActivator)->AddAccount( 150, true );
				UTIL_LogPrintf( "%s touched a hostage\n", STRING( pActivator->pev->netname ) );	// Beta 6.6
			}
		}

		((CBasePlayer *)pActivator)->HostageUsed();	// Beta 5.0 (leak): the hint
	}

	if ( m_fStopped == TRUE )
	{
		m_flFollowDistance = 60;	// Beta 5.0 (leak) (was 36)
		m_flFollowRange = 300;
	}
	else if ( m_fStopped == FALSE )
	{
		m_flFollowDistance = 128;
		m_flFollowRange = 1024;
	}
}

// Beta 5.0 (leak): the hostage is used on and off (FCAP_ONOFF_USE)
int CHostage::ObjectCaps( void )
{
	return CBaseMonster::ObjectCaps() | FCAP_ONOFF_USE;
}

// Beta 4.1: the player the hostage follows pushes it away when he runs into
// it, at 225 units a second
void CHostage::Touch( CBaseEntity *pOther )
{
	// Beta 5.0 (leak): any player pushes it away, at 200 units a second (Beta 4.1:
	// the one it follows, at 225)
	if ( pOther->IsPlayer() == TRUE )
	{
		Vector vecPush;		// name ours

		vecPush = pev->origin - pOther->pev->origin;
		vecPush = vecPush / vecPush.Length();
		vecPush = vecPush * 200;
		pev->velocity = vecPush;
	}
}

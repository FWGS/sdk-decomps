/***
*
*	Counter-Strike Beta 2.0, hostage.cpp: the hostage (hostage_entity), the
*	map's spawn entity.
*
*	A scientist model that stands where the map put it until a
*	counter-terrorist uses it; then it follows him, and used again it stays.
*	Shot, it flinches by hit group; killed, it costs its killer money and is
*	hidden three seconds later. A following hostage that comes into a rescue
*	zone (or, on a map without one, within 256 units of a rescue point) is
*	rescued; RestartRound puts it back. File, class and method names are the
*	Linux libraries' symbols.
*
*	Counter-Strike 1.4: the hostage finds its way. It no longer walks the
*	trail of its leader's last positions: every tenth of a second (IdleThink;
*	the animation runs thirty times a second) DoFollow asks its CLocalNav
*	(hostage_localnav.cpp) for a path to the leader, walks to the furthest
*	node of the path it can reach in a straight line (MoveToward), and
*	wiggles free where it stands still (Wiggle); PreThink, called for every
*	hostage each frame, lifts it over steps. Only counter-terrorists can use
*	it; what the attacker loses is counted from the damage done; the
*	counter-terrorists' radar is sent its place once a second and its death
*	or rescue (messages HostagePos and HostageK). Written from the Windows
*	DLL, the 1.4 Linux library (it has the functions the Windows compiler
*	expanded inline: GetModifiedDamage, PlayPainSound, SetFlinchActivity,
*	SetDeathActivity, GiveCTTouchBonus, PointAt, IsOnLadder, and one nothing
*	calls, ApplyHostagePenalty) and Counter-Strike 1.6's debug information
*	(the names of members, parameters and locals, the order of the
*	definitions).
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
#include "hostage_localnav.h"	// Counter-Strike 1.4: CLocalNav


extern void Broadcast( const char *sentence );	// multiplay_gamerules.cpp (Beta 3.0)
extern char *GetTeam( int teamNo );	// multiplay_gamerules.cpp (Counter-Strike 1.3)

extern int gmsgHostagePos;	// player.cpp (Counter-Strike 1.4)
extern int gmsgHostageK;
extern cvar_t *g_psv_gravity;	// game.cpp

int g_iHostageNumber = 0;	// Counter-Strike 1.4: counts the hostages spawned (m_iHostageIndex); the Linux library's name

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
	pev->velocity	= Vector( 0, 0, 0 );	// Counter-Strike 1.4

	// Counter-Strike 1.0: the monsters' spawn flag (the SDK's monsters.h:
	// SF_MONSTER_HITMONSTERCLIP, 4; CBaseMonster::MonsterInit's lines)
	if ( pev->spawnflags & 4 )
		pev->flags |= FL_MONSTERCLIP;

	// Beta 5.0: the map may give the hostage a model and a skin
	if ( pev->skin < 0 )
		pev->skin = 0;

	SET_MODEL( ENT(pev), STRING( pev->model ) );
	SetActivity( ACT_IDLE );

	m_flNextChange = 0;
	m_State = STAND;
	m_hTargetEnt = NULL;
	m_hStoppedTargetEnt = NULL;
	m_flFlinchTime = 0;
	m_vPathToFollow[0] = Vector( 0, 0, 0 );	// Beta 3.0
	m_bRescueMe = FALSE;					// Beta 5.0 (leak)

	UTIL_SetSize( pev, Vector( -10, -10, 0 ), Vector( 10, 10, 62 ) );	// Beta 6.0: smaller (was 32 by 32 by 72)
	UTIL_MakeVectors( pev->v_angle );
	SetBoneController( 0, UTIL_VecToYaw( gpGlobals->v_forward ) );
	DROP_TO_FLOOR( ENT( pev ) );	// Counter-Strike 1.4

	SetThink( IdleThink );
	pev->nextthink = gpGlobals->time + RANDOM_FLOAT( 0.1, 0.2 );	// Counter-Strike 1.4: the hostages do not all think in one frame
	m_flNextFullThink = gpGlobals->time + RANDOM_FLOAT( 0.1, 0.2 );

	m_vStart = pev->origin;
	m_vStartAngles = pev->angles;
	m_vOldPos = Vector( 9999, 9999, 9999 );
	m_iHostageIndex = ++g_iHostageNumber;

	m_flNextRadarTime = gpGlobals->time + RANDOM_FLOAT( 0, 1 );
	nTargetNode = NODE_INVALID_EMPTY;
	m_fHasPath = FALSE;
	m_flLastPathCheck = -1;
	m_flPathAcquired = -1;
	m_flPathCheckInterval = 0.1;

	m_LocalNav = new CLocalNav( this );
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
	PRECACHE_MODEL( "sprites/smoke.spr" );	// Counter-Strike 1.4: UTIL_DrawBeamPoints's sprite
}

// Counter-Strike 1.4: nothing to do when the activity is the running one;
// walking and running pass into each other without starting the frames anew
void CHostage::SetActivity( int act )
{
	if ( m_Activity != act )
	{
		int sequence = LookupActivity( act );

		if ( sequence != -1 )	// ACTIVITY_NOT_AVAILABLE (animation.h)
		{
			if ( pev->sequence != sequence )
			{
				if ( ( m_Activity != ACT_WALK && m_Activity != ACT_RUN ) || ( act != ACT_WALK && act != ACT_RUN ) )
					pev->frame = 0;

				pev->sequence = sequence;
			}

			m_Activity = act;
			ResetSequenceInfo();
		}
	}
}

void CHostage::IdleThink( void )
{
	pev->nextthink = gpGlobals->time + ( 1.0 / 30 );

	float flInterval = StudioFrameAdvance();
	DispatchAnimEvents( flInterval );

	if ( m_flNextFullThink > gpGlobals->time )
		return;

	m_flNextFullThink = gpGlobals->time + 0.1;

	if ( pev->deadflag == DEAD_DEAD )
	{
		UTIL_SetSize( pev, Vector( 0, 0, 0 ), Vector( 0, 0, 0 ) );
		return;
	}

	// the leader is dead: nobody to follow
	if ( m_hTargetEnt != NULL && m_hTargetEnt->pev->deadflag != DEAD_NO )
	{
		m_State = STAND;
		m_hTargetEnt = NULL;
	}

	if ( m_hTargetEnt != NULL )
	{
		// Beta 5.0 (leak): a hostage that follows a counter-terrorist is rescued in a
		// func_hostage_rescue (m_bRescueMe), or on a map without one within 256 units of
		// an info_hostage_rescue (without those: of an info_player_start); worth 1000
		// dollars to him
		CBasePlayer *pPlayer = GetClassPtr( (CBasePlayer *)m_hTargetEnt->pev );

		if ( pPlayer->m_iTeam == TEAM_CT )
		{
			if ( !((CHalfLifeMultiplay *)g_pGameRules)->m_fMapHasRescueZone )
			{
				// the three locals as Counter-Strike 1.6 still has them (its debug
				// information: names, the lines, bContinue never used)
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
						m_bRescueMe = TRUE;
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
							m_bRescueMe = TRUE;
							break;
						}

						pSpot = UTIL_FindEntityByClassname( pSpot, "info_player_start" );
					}
				}
			}

			if ( m_bRescueMe )
			{
				pev->deadflag = DEAD_RESPAWNABLE;	// rescued
				pPlayer->AddAccount( 1000, true );
				UTIL_LogPrintf( "\"%s<%i><%s><CT>\" triggered \"Rescued_A_Hostage\"\n", STRING( pPlayer->pev->netname ), GETPLAYERUSERID( pPlayer->edict() ), GETPLAYERAUTHID( pPlayer->edict() ) );

				SendHostageEventMsg();

				// tell director about it
				MESSAGE_BEGIN( MSG_SPEC, SVC_DIRECTOR );
					WRITE_BYTE ( 9 );	// command length in bytes
					WRITE_BYTE ( DRC_CMD_EVENT );
					WRITE_SHORT( ENTINDEX( pPlayer->edict() ) );
					WRITE_SHORT( ENTINDEX( this->edict() ) );
					WRITE_LONG( 15 );
				MESSAGE_END();

				pev->effects |= EF_NODRAW;
				Remove();

				((CHalfLifeMultiplay *)g_pGameRules)->m_iHostagesRescued++;
				((CHalfLifeMultiplay *)g_pGameRules)->CheckWinConditions();
				Broadcast( "rescued" );
			}
		}
	}

	DoFollow();

	// once a second the counter-terrorists' radar is told where it is, when it has moved
	if ( pev->deadflag != DEAD_DEAD && !( pev->effects & EF_NODRAW ) )
	{
		if ( gpGlobals->time >= m_flNextRadarTime )
		{
			if ( ( m_vOldPos - pev->origin ).Length() > 1.0 )
			{
				m_vOldPos = pev->origin;

				if ( !((CHalfLifeMultiplay *)g_pGameRules)->m_flRestartRoundTime )
					SendHostagePositionMsg();
			}

			m_flNextRadarTime = gpGlobals->time + 1.0;
		}
	}

	if ( gpGlobals->time >= m_flFlinchTime )
	{
		if ( pev->velocity.Length() > 160 )
			SetActivity( ACT_RUN );
		else if ( pev->velocity.Length() > 15 )
			SetActivity( ACT_WALK );
		else
			SetActivity( ACT_IDLE );
	}
}

void CHostage::Remove( void )
{
	pev->movetype = MOVETYPE_NONE;
	pev->solid = SOLID_NOT;
	pev->takedamage = DAMAGE_NO;

	UTIL_SetSize( pev, Vector( 0, 0, 0 ), Vector( 0, 0, 0 ) );
	pev->nextthink = -1;
	m_flNextFullThink = -1;
}

// back to where Spawn found it, alive again (Beta 5.0 leak: and healed)
void CHostage::RePosition( void )
{
	pev->health = pev->max_health;
	pev->movetype = MOVETYPE_STEP;
	pev->solid = SOLID_SLIDEBOX;
	pev->takedamage = DAMAGE_YES;
	pev->deadflag = DEAD_NO;
	pev->velocity = Vector( 0, 0, 0 );
	pev->angles = m_vStartAngles;
	pev->effects &= ~EF_NODRAW;

	m_hTargetEnt = NULL;
	m_hStoppedTargetEnt = NULL;

	m_bTouched = FALSE;
	m_bRescueMe = FALSE;
	m_flNextRadarTime = 0;
	m_vOldPos = Vector( 9999, 9999, 9999 );

	UTIL_SetOrigin( pev, m_vStart );
	UTIL_SetSize( pev, Vector( -10, -10, 0 ), Vector( 10, 10, 62 ) );
	DROP_TO_FLOOR( ENT( pev ) );
	SetActivity( ACT_IDLE );

	SetThink( IdleThink );
	pev->nextthink = gpGlobals->time + RANDOM_FLOAT( 0.1, 0.2 );
	m_flNextFullThink = gpGlobals->time + RANDOM_FLOAT( 0.1, 0.2 );

	nTargetNode = NODE_INVALID_EMPTY;
	m_fHasPath = FALSE;
	m_flLastPathCheck = -1;
	m_flPathAcquired = -1;
	m_flPathCheckInterval = 0.1;
}

int CHostage::TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType )
{
	float flActualDamage;
	CBasePlayer *pAttacker = NULL;

	flActualDamage = GetModifiedDamage( flDamage, m_LastHitGroup );

	if ( flActualDamage > pev->health )
		flActualDamage = pev->health;

	pev->health -= flActualDamage;

	PlayPainSound();

	if ( pevAttacker != NULL )
	{
		// Beta 7.1: run over by a vehicle, its driver is the attacker
		CBaseEntity *pAttackingEnt = GetClassPtr( (CBaseEntity *)pevAttacker );

		if ( pAttackingEnt->Classify() == CLASS_VEHICLE )
		{
			CBaseEntity *pDriver = ((CFuncVehicle *)pAttackingEnt)->m_pDriver;

			if ( pDriver )
				pevAttacker = pDriver->pev;
		}

		if ( pAttackingEnt->IsPlayer() )
			pAttacker = GetClassPtr( (CBasePlayer *)pevAttacker );
	}

	if ( pev->health <= 0 )
	{
		pev->health = 0;
		pev->movetype = MOVETYPE_TOSS;
		pev->flags &= ~FL_ONGROUND;
		SetDeathActivity();

		// Counter-Strike 1.4: the killer pays 500 dollars and 20 for each point of the damage
		if ( pAttacker )
		{
			pAttacker->AddAccount( -20 * (int)flActualDamage - 500, true );
			AnnounceDeath( pAttacker );
			ApplyHostagePenalty( pAttacker );	// Counter-Strike 1.5: mp_hostagepenalty counts again
		}

		pev->takedamage = DAMAGE_NO;
		pev->deadflag = DEAD_DEAD;
		pev->solid = SOLID_NOT;

		// Beta 4.0: a dead hostage may decide the round
		((CHalfLifeMultiplay *)g_pGameRules)->CheckWinConditions();

		if ( !((CHalfLifeMultiplay *)g_pGameRules)->m_flRestartRoundTime )
			SendHostageEventMsg();

		pev->nextthink = gpGlobals->time + 3;
		SetThink( Remove );

		return 0;
	}

	SetFlinchActivity();

	if ( !pAttacker )
		return 0;

	// Counter-Strike 1.4: a player who hurts a hostage pays 20 dollars for each point of the damage
	pAttacker->AddAccount( -20 * (int)flActualDamage, true );
	ClientPrint( pAttacker->pev, HUD_PRINTCENTER, "#Injured_Hostage" );

	if ( !( pAttacker->m_flDisplayHistory & 0x20 ) )	// hostage injured
	{
		pAttacker->HintMessage( "#Hint_careful_around_hostages", FALSE, FALSE );
		pAttacker->m_flDisplayHistory |= 0x20;
	}

	return 1;
}

// Beta 3.0: a hit of no particular group counts as a stomach hit, and any
// other as a chest hit (each written as a case of its own)
float CHostage::GetModifiedDamage( float flDamage, int nHitGroup )
{
	switch ( nHitGroup )
	{
	case HITGROUP_GENERIC:	flDamage *= 1.75; break;
	case HITGROUP_HEAD:		flDamage *= 2.5; break;
	case HITGROUP_CHEST:	flDamage *= 1.5; break;
	case HITGROUP_STOMACH:	flDamage *= 1.75; break;
	case HITGROUP_LEFTARM:	flDamage *= 0.75; break;
	case HITGROUP_RIGHTARM:	flDamage *= 0.75; break;
	case HITGROUP_LEFTLEG:	flDamage *= 0.6; break;
	case HITGROUP_RIGHTLEG:	flDamage *= 0.6; break;
	default:				flDamage *= 1.5; break;
	}

	return flDamage;
}

void CHostage::PlayPainSound( void )
{
	if ( m_LastHitGroup == HITGROUP_HEAD )
	{
		switch ( RANDOM_LONG( 0, 1 ) )
		{
		case 0:	EMIT_SOUND( ENT(pev), CHAN_VOICE, "player/headshot1.wav", VOL_NORM, ATTN_NORM ); break;
		case 1:	EMIT_SOUND( ENT(pev), CHAN_VOICE, "player/headshot2.wav", VOL_NORM, ATTN_NORM ); break;
		}
	}
}

void CHostage::SetFlinchActivity( void )
{
	m_flFlinchTime = gpGlobals->time + 0.75;

	switch ( m_LastHitGroup )
	{
	case HITGROUP_GENERIC:
	case HITGROUP_HEAD:
	case HITGROUP_CHEST:
	case HITGROUP_STOMACH:
		SetActivity( ACT_SMALL_FLINCH );
		break;
	case HITGROUP_LEFTARM:
		SetActivity( ACT_FLINCH_LEFTARM );
		break;
	case HITGROUP_RIGHTARM:
		SetActivity( ACT_FLINCH_RIGHTARM );
		break;
	case HITGROUP_LEFTLEG:
		SetActivity( ACT_FLINCH_LEFTLEG );
		break;
	case HITGROUP_RIGHTLEG:
		SetActivity( ACT_FLINCH_RIGHTLEG );
		break;
	default:
		SetActivity( ACT_SMALL_FLINCH );
		break;
	}
}

// Beta 3.0: a hit of no particular group, and any other, dies as from a head shot
void CHostage::SetDeathActivity( void )
{
	switch ( m_LastHitGroup )
	{
	case HITGROUP_GENERIC:
	case HITGROUP_HEAD:
		SetActivity( ACT_DIE_HEADSHOT );
		break;
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
	default:
		SetActivity( ACT_DIE_HEADSHOT );
		break;
	}
}

void CHostage::AnnounceDeath( CBasePlayer *pAttacker )
{
	ClientPrint( pAttacker->pev, HUD_PRINTCENTER, "#Killed_Hostage" );	// Beta 5.0 (leak); Beta 7.1: a titles.txt name

	if ( !( pAttacker->m_flDisplayHistory & 0x40 ) )	// hostage killed
	{
		pAttacker->HintMessage( "#Hint_lost_money", FALSE, FALSE );	// Counter-Strike 1.1: the hints are titles of the client's
		pAttacker->m_flDisplayHistory |= 0x40;
	}

	// Counter-Strike 1.0: the training course is told
	if ( !g_pGameRules->IsMultiplayer() )
		((CHalfLifeTraining *)g_pGameRules)->HostageDied();

	UTIL_LogPrintf( "\"%s<%i><%s><%s>\" triggered \"Killed_A_Hostage\"\n", STRING( pAttacker->pev->netname ), GETPLAYERUSERID( pAttacker->edict() ), GETPLAYERAUTHID( pAttacker->edict() ), GetTeam( pAttacker->m_iTeam ) );

	// tell director about it
	MESSAGE_BEGIN( MSG_SPEC, SVC_DIRECTOR );
		WRITE_BYTE ( 9 );	// command length in bytes
		WRITE_BYTE ( DRC_CMD_EVENT );
		WRITE_SHORT( ENTINDEX( pAttacker->edict() ) );
		WRITE_SHORT( ENTINDEX( this->edict() ) );
		WRITE_LONG( 15 );
	MESSAGE_END();
}

// Beta 6.5: mp_hostagepenalty, the number of hostages a terrorist may kill;
// one before it he is warned, at it the server drops him. In Counter-Strike 1.4
// nothing called it (text from the Linux library); 1.5's TakeDamage calls it again
void CHostage::ApplyHostagePenalty( CBasePlayer *pAttacker )
{
	if ( pAttacker->m_iTeam == TEAM_TERRORIST )
	{
		int iHostagePenalty = CVAR_GET_FLOAT( "mp_hostagepenalty" );

		if ( iHostagePenalty != 0 )
		{
			pAttacker->m_iHostagesKilled++;

			if ( pAttacker->m_iHostagesKilled == iHostagePenalty - 1 )
				pAttacker->HintMessage( "#Hint_removed_for_next_hostage_killed", TRUE, FALSE );
			else if ( pAttacker->m_iHostagesKilled >= iHostagePenalty )
				CLIENT_COMMAND( pAttacker->edict(), "disconnect\n" );
		}
	}
}

void CHostage::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if ( !pActivator->IsPlayer() )
		return;

	if ( pev->takedamage == DAMAGE_NO )
		return;

	CBasePlayer *pPlayer = (CBasePlayer *)pActivator;

	// Counter-Strike 1.4: the terrorists cannot move it any more
	if ( pPlayer->m_iTeam != TEAM_CT )
	{
		if ( !( pPlayer->m_flDisplayHistory & 0x200000 ) )	// Counter-Strike 1.4: only counter-terrorists move hostages
		{
			pPlayer->m_flDisplayHistory |= 0x200000;
			pPlayer->HintMessage( "#Only_CT_Can_Move_Hostages", FALSE, TRUE );
		}

		return;
	}

	if ( gpGlobals->time < m_flNextChange )
		return;

	m_flNextChange = gpGlobals->time + 1;
	m_flPathAcquired = gpGlobals->time;

	// Beta 3.0: the leader's use makes a following hostage stay; any other
	// use, or a new leader, makes it follow
	if ( pActivator != m_hTargetEnt )
	{
		m_State = FOLLOW;
		m_hTargetEnt = pActivator;
		m_hStoppedTargetEnt = NULL;
	}
	else if ( m_State == FOLLOW )
	{
		m_State = STAND;
		m_hTargetEnt = NULL;
		m_hStoppedTargetEnt = pActivator;
	}
	else
		m_State = FOLLOW;

	if ( m_State == FOLLOW )
		PlayFollowRescueSound();

	GiveCTTouchBonus( pPlayer );
	pPlayer->HostageUsed();	// Beta 5.0 (leak): the hint
}

void CHostage::PlayFollowRescueSound( void )
{
	switch ( RANDOM_LONG( 0, 4 ) )
	{
	case 0:	EMIT_SOUND( ENT(pev), CHAN_VOICE, "hostage/hos1.wav", VOL_NORM, ATTN_NORM ); break;
	case 1:	EMIT_SOUND( ENT(pev), CHAN_VOICE, "hostage/hos2.wav", VOL_NORM, ATTN_NORM ); break;
	case 2:	EMIT_SOUND( ENT(pev), CHAN_VOICE, "hostage/hos3.wav", VOL_NORM, ATTN_NORM ); break;
	case 3:	EMIT_SOUND( ENT(pev), CHAN_VOICE, "hostage/hos4.wav", VOL_NORM, ATTN_NORM ); break;
	case 4:	EMIT_SOUND( ENT(pev), CHAN_VOICE, "hostage/hos5.wav", VOL_NORM, ATTN_NORM ); break;
	}
}

// the first counter-terrorist to lead it pays himself and his team
// (Beta 5.0 leak: 150 to him, 100 to the team)
void CHostage::GiveCTTouchBonus( CBasePlayer *pPlayer )
{
	if ( m_bTouched != TRUE )
	{
		m_bTouched = TRUE;

		// the rules through a local: with the member taken straight from g_pGameRules
		// Use (which has this function expanded inline) loads the string base ahead of
		// the name in the log line below, and retail the name first (the local's name is ours)
		CHalfLifeMultiplay *mp = (CHalfLifeMultiplay *)g_pGameRules;
		mp->m_iAccountCT += 100;
		pPlayer->AddAccount( 150, true );
		UTIL_LogPrintf( "\"%s<%i><%s><CT>\" triggered \"Touched_A_Hostage\"\n", STRING( pPlayer->pev->netname ), GETPLAYERUSERID( pPlayer->edict() ), GETPLAYERAUTHID( pPlayer->edict() ) );
	}
}

// Beta 5.0 (leak): the hostage is used on and off (FCAP_ONOFF_USE).
// Counter-Strike 1.4: and spawned anew when a level is loaded (FCAP_MUST_SPAWN)
int CHostage::ObjectCaps( void )
{
	return CBaseMonster::ObjectCaps() | FCAP_MUST_SPAWN | FCAP_ONOFF_USE;
}

// Counter-Strike 1.4: a counter-terrorist or another hostage that walks into
// it pushes it away, 50 units a second more at each touch
void CHostage::Touch( CBaseEntity *pOther )
{
	Vector vPush;
	Vector vecSrc;	// name ours

	if ( pOther->IsPlayer() == TRUE )
	{
		if ( ((CBasePlayer *)pOther)->m_iTeam != TEAM_CT )
			return;
	}
	else
	{
		if ( !FClassnameIs( pOther->pev, "hostage_entity" ) )
			return;
	}

	vecSrc = pOther->pev->origin;
	vecSrc.z = pev->origin.z;

	vPush = pev->origin - vecSrc;
	vPush = vPush / vPush.Length();
	vPush = vPush * 50;

	pev->velocity.x += vPush.x;
	pev->velocity.y += vPush.y;
	pev->velocity.z += vPush.z;

	UTIL_DrawBeamPoints( pev->origin, pev->origin + pev->velocity.Normalize() * 500, 10, 0, 255, 0 );
}

void CHostage::DoFollow( void )
{
	CBaseEntity *pFollowing;
	Vector vecDest;
	float flRadius = 0;
	float flDistToDest;

	if ( m_hTargetEnt == NULL )
		return;

	pFollowing = GetClassPtr( (CBaseEntity *)m_hTargetEnt->pev );
	m_LocalNav->SetTargetEnt( pFollowing );

	vecDest = pFollowing->pev->origin;
	vecDest.z += pFollowing->pev->mins.z;

	flDistToDest = ( vecDest - pev->origin ).Length();

	// near the leader, with a way to him: nothing to do
	if ( flDistToDest < 80 && ( m_fHasPath || m_LocalNav->PathTraversable( pev->origin, vecDest, TRUE ) != PATH_TRAVERSABLE_EMPTY ) )
		return;

	if ( pev->flags & FL_ONGROUND )
	{
		if ( m_flPathCheckInterval + m_flLastPathCheck < gpGlobals->time )
		{
			if ( !m_fHasPath || pFollowing->pev->velocity.Length2D() > 1 )
			{
				m_flLastPathCheck = gpGlobals->time;
				m_LocalNav->RequestNav( this );
			}
		}
	}

	if ( m_fHasPath )
	{
		nTargetNode = m_LocalNav->GetFurthestTraversableNode( pev->origin, vecNodes, m_nPathNodes, TRUE );

		if ( !nTargetNode )
		{
			if ( ( vecNodes[ nTargetNode ] - pev->origin ).Length2D() < HOSTAGE_STEPSIZE )
				nTargetNode = NODE_INVALID_EMPTY;
		}

		if ( nTargetNode == NODE_INVALID_EMPTY )
		{
			m_fHasPath = FALSE;
			m_flPathCheckInterval = 0.1;
		}
	}

	if ( gpGlobals->time < m_flFlinchTime )
		return;

	if ( nTargetNode != NODE_INVALID_EMPTY )
	{
		if ( pev->flags & FL_ONGROUND )
			PointAt( vecNodes[ nTargetNode ] );

		if ( IsOnLadder() )
			pev->v_angle.x = -60;

		MoveToward( vecNodes[ nTargetNode ] );
	}

	// two seconds after it got its leader or a path, standing still or with nowhere to go
	if ( pev->flags & FL_ONGROUND )
	{
		if ( m_flPathAcquired != -1 && gpGlobals->time < m_flPathAcquired + 2.0 )
		{
			if ( pev->velocity.Length2D() < 1.0 || nTargetNode == NODE_INVALID_EMPTY )
				Wiggle();
		}
	}
}

void CHostage::PointAt( Vector &vecLoc )
{
	pev->angles.x = 0;
	pev->angles.y = UTIL_VecToAngles( vecLoc - pev->origin ).y;
	pev->angles.z = 0;
}

void CHostage::MoveToward( Vector &vecLoc )
{
	int nFwdMove;
	Vector vecFwd;
	Vector vecbigDest;
	Vector vecMove;

	CBaseEntity *pFollowing = GetClassPtr( (CBaseEntity *)m_hTargetEnt->pev );

	Vector vecAng;

	vecAng.x = 0;
	vecAng.y = UTIL_VecToAngles( vecLoc - pev->origin ).y;
	vecAng.z = 0;

	UTIL_MakeVectorsPrivate( vecAng, vecFwd, NULL, NULL );

	// one step ahead, or less when the goal is nearer
	float flDist = min( ( vecLoc - pev->origin ).Length2D(), ( vecFwd * CLocalNav::s_flStepSize ).Length2D() );

	vecMove = vecFwd * flDist;
	vecbigDest = pev->origin + vecMove;
	nFwdMove = m_LocalNav->PathTraversable( pev->origin, vecbigDest, FALSE );

	if ( nFwdMove != PATH_TRAVERSABLE_EMPTY )
	{
		float flSpeed;

		Vector vecDest;

		vecDest = pFollowing->pev->origin;
		vecDest.z += pFollowing->pev->mins.z;

		if ( !( pev->flags & FL_ONGROUND ) )
		{
			flSpeed = 250;
		}
		else
		{
			float flDist = ( vecDest - pev->origin ).Length();

			if ( flDist < 110 )
				flSpeed = flDist;
			else if ( flDist < 250 )
				flSpeed = 300;
			else
				flSpeed = 400;
		}

		pev->velocity.x = vecFwd.x * flSpeed;
		pev->velocity.y = vecFwd.y * flSpeed;

		UTIL_DrawBeamPoints( pev->origin, pev->origin + pev->velocity.Normalize() * 500, 10, 255, 0, 0 );

		// sic: the step that needs no jump has its own branch, with a test that
		// does nothing. Retail compares with 2, leaves, and then with 3; and the
		// inline expansion of this function (the Vector operators and
		// constructors expanded up to "vecFwd * flDist", calls from there on)
		// asks for text worth 6 to 12 dead stores more than the visible code has
		// (measured: 5 and 13 do not give retail's 817 bytes). An on-ground test
		// with an empty body here does; an empty branch alone does not. What the
		// test once guarded is not known.
		if ( nFwdMove == PATH_TRAVERSABLE_STEP )
		{
			if ( pev->flags & FL_ONGROUND )
			{
			}
		}
		else if ( nFwdMove == PATH_TRAVERSABLE_STEPJUMPABLE )
		{
			if ( pev->flags & FL_ONGROUND )
				pev->velocity.z = 270;
		}
	}
}

BOOL CHostage::IsOnLadder( void )
{
	return pev->movetype == MOVETYPE_FLY;
}

// CLocalNav calls it when the hostage's search may run (RequestNav, Think)
void CHostage::NavReady( void )
{
	CBaseEntity *pFollowing;
	Vector vecDest;
	float flRadius = 40;
	node_index_t nindexPath;

	if ( m_hTargetEnt == NULL )
		return;

	pFollowing = GetClassPtr( (CBaseEntity *)m_hTargetEnt->pev );
	vecDest = pFollowing->pev->origin;

	// a leader in the air: where he will land
	if ( !( pFollowing->pev->flags & FL_ONGROUND ) )
	{
		TraceResult tr;
		Vector vecDropDest;

		vecDropDest = vecDest;
		vecDropDest.z -= 300;

		UTIL_TraceHull( vecDest, vecDropDest, ignore_monsters, human_hull, pFollowing->edict(), &tr );

		if ( tr.fStartSolid || tr.flFraction == 1.0 )
			return;

		vecDest = tr.vecEndPos;
	}

	vecDest.z += pFollowing->pev->mins.z;
	m_LocalNav->SetTargetEnt( pFollowing );

	nindexPath = m_LocalNav->FindPath( pev->origin, vecDest, flRadius, TRUE );

	if ( nindexPath != NODE_INVALID_EMPTY )
	{
		m_fHasPath = TRUE;
		nTargetNode = NODE_INVALID_EMPTY;
		m_flPathAcquired = gpGlobals->time;
		m_flPathCheckInterval = 0.5;

		m_nPathNodes = m_LocalNav->SetupPathNodes( nindexPath, vecNodes, TRUE );
	}
	else
	{
		// no path: ask more rarely, up to twice a second
		if ( !m_fHasPath )
			m_flPathCheckInterval = min( m_flPathCheckInterval + 0.1, 0.5 );
	}
}

// the hostage's place, for the radar of every living counter-terrorist
void CHostage::SendHostagePositionMsg( void )
{
	CBaseEntity *pEntity = NULL;

	pEntity = UTIL_FindEntityByClassname( pEntity, "player" );
	while ( pEntity != NULL && !FNullEnt( pEntity->edict() ) )
	{
		if ( pEntity->IsPlayer() && pEntity->pev->flags != FL_DORMANT )
		{
			CBasePlayer *pTempPlayer = GetClassPtr( (CBasePlayer *)pEntity->pev );

			if ( pTempPlayer->pev->deadflag == DEAD_NO && pTempPlayer->m_iTeam == TEAM_CT )
			{
				MESSAGE_BEGIN( MSG_ONE, gmsgHostagePos, NULL, pTempPlayer->pev );
					WRITE_BYTE( 0 );
					WRITE_BYTE( m_iHostageIndex );
					WRITE_COORD( pev->origin.x );
					WRITE_COORD( pev->origin.y );
					WRITE_COORD( pev->origin.z );
				MESSAGE_END();
			}
		}

		pEntity = UTIL_FindEntityByClassname( pEntity, "player" );
	}
}

// the hostage is dead or rescued: off their radar
void CHostage::SendHostageEventMsg( void )
{
	CBaseEntity *pEntity = NULL;

	pEntity = UTIL_FindEntityByClassname( pEntity, "player" );
	while ( pEntity != NULL && !FNullEnt( pEntity->edict() ) )
	{
		if ( pEntity->IsPlayer() && pEntity->pev->flags != FL_DORMANT )
		{
			CBasePlayer *pTempPlayer = GetClassPtr( (CBasePlayer *)pEntity->pev );

			if ( pTempPlayer->pev->deadflag == DEAD_NO && pTempPlayer->m_iTeam == TEAM_CT )
			{
				MESSAGE_BEGIN( MSG_ONE, gmsgHostageK, NULL, pTempPlayer->pev );
					WRITE_BYTE( m_iHostageIndex );
				MESSAGE_END();
			}
		}

		pEntity = UTIL_FindEntityByClassname( pEntity, "player" );
	}
}

// pushed away from every one of the eight directions in which it cannot go
// 50 units, and a little at random
void CHostage::Wiggle( void )
{
	TraceResult tr;
	Vector vec( 0, 0, 0 );

	if ( !m_LocalNav->PathTraversable( pev->origin, pev->origin + Vector( 1, 0, 0 ) * 50, TRUE ) )
		vec = vec - Vector( 1, 0, 0 );

	if ( !m_LocalNav->PathTraversable( pev->origin, pev->origin + Vector( -1, 0, 0 ) * 50, TRUE ) )
		vec = vec - Vector( -1, 0, 0 );

	if ( !m_LocalNav->PathTraversable( pev->origin, pev->origin + Vector( 0, 1, 0 ) * 50, TRUE ) )
		vec = vec - Vector( 0, 1, 0 );

	if ( !m_LocalNav->PathTraversable( pev->origin, pev->origin + Vector( 0, -1, 0 ) * 50, TRUE ) )
		vec = vec - Vector( 0, -1, 0 );

	if ( !m_LocalNav->PathTraversable( pev->origin, pev->origin + Vector( 1, 1, 0 ) * 50, TRUE ) )
		vec = vec - Vector( 1, 1, 0 );

	if ( !m_LocalNav->PathTraversable( pev->origin, pev->origin + Vector( 1, -1, 0 ) * 50, TRUE ) )
		vec = vec - Vector( 1, -1, 0 );

	if ( !m_LocalNav->PathTraversable( pev->origin, pev->origin + Vector( -1, 1, 0 ) * 50, TRUE ) )
		vec = vec - Vector( -1, 1, 0 );

	if ( !m_LocalNav->PathTraversable( pev->origin, pev->origin + Vector( -1, -1, 0 ) * 50, TRUE ) )
		vec = vec - Vector( -1, -1, 0 );

	vec = vec + Vector( RANDOM_FLOAT( -3, 3 ), RANDOM_FLOAT( -3, 3 ), 0 );
	pev->velocity = pev->velocity + ( vec.Normalize() * 100 );

	UTIL_DrawBeamPoints( pev->origin, pev->origin + pev->velocity.Normalize() * 500, 10, 0, 0, 255 );
}

// every frame (CLocalNav::HostagePrethink): a hostage that walks into a
// step it could stand on is put on top of it
void CHostage::PreThink( void )
{
	Vector vecSrc;
	Vector vecDest;
	TraceResult tr;
	float flOrigDist;
	float flRaisedDist;
	float flInterval = gpGlobals->frametime;

	if ( !( pev->flags & FL_ONGROUND ) )
		return;

	if ( pev->velocity.Length2D() < 1 )
		return;

	vecSrc = pev->origin;
	vecDest = vecSrc + pev->velocity * flInterval;
	vecDest.z = vecSrc.z;

	TRACE_MONSTER_HULL( edict(), vecSrc, vecDest, dont_ignore_monsters, edict(), &tr );

	if ( tr.fStartSolid )
		return;

	if ( tr.flFraction == 1.0 )
		return;

	if ( tr.vecPlaneNormal.z > 0.7 )
		return;

	flOrigDist = ( tr.vecEndPos - pev->origin ).Length2D();
	vecSrc.z += CLocalNav::s_flStepSize;
	vecDest = vecSrc + ( pev->velocity.Normalize() * 0.1 );
	vecDest.z = vecSrc.z;

	TRACE_MONSTER_HULL( edict(), vecSrc, vecDest, dont_ignore_monsters, edict(), &tr );

	if ( tr.fStartSolid )
		return;

	vecSrc = tr.vecEndPos;
	vecDest = tr.vecEndPos;
	vecDest.z -= CLocalNav::s_flStepSize;

	TRACE_MONSTER_HULL( edict(), vecSrc, vecDest, dont_ignore_monsters, edict(), &tr );

	if ( tr.vecPlaneNormal.z < 0.7 )
		return;

	flRaisedDist = ( tr.vecEndPos - pev->origin ).Length2D();

	if ( flRaisedDist > flOrigDist )
	{
		Vector vecNewOrigin;

		vecNewOrigin = pev->origin;
		vecNewOrigin.z = tr.vecEndPos.z;

		UTIL_SetOrigin( pev, vecNewOrigin );
		pev->velocity.z += pev->gravity * g_psv_gravity->value * flInterval;
	}
}

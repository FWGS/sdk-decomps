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
#ifndef TALKMONSTER_H
#define TALKMONSTER_H

#ifndef MONSTERS_H
#include "monsters.h"
#endif

//=========================================================
// Talking monster base class
// Used for scientists and barneys
//=========================================================

#define TALKRANGE_MIN 500.0				// don't talk to anyone farther away than this

#define TLK_STARE_DIST	128				// anyone closer than this and looking at me is probably staring at me.

#define bit_saidDamageLight		(1<<0)	// bits so we don't repeat key sentences
#define bit_saidDamageMedium	(1<<1)
#define bit_saidDamageHeavy		(1<<2)
#define bit_saidHelloPlayer		(1<<3)
#define bit_saidWoundLight		(1<<4)
#define bit_saidWoundHeavy		(1<<5)
#define bit_saidHeard			(1<<6)
#define bit_saidSmelled			(1<<7)

#define TLK_CFRIENDS		16

typedef enum
{
	TLK_ANSWER = 0,
	TLK_QUESTION,
	TLK_IDLE,
	TLK_PIDLE,
	TLK_PQUESTION,
	TLK_SMELL,
	TLK_WOUND,
	TLK_MORTAL,
	TLK_ATTACK,		// names from here on ours, after the sentence groups
	TLK_KILL,
	TLK_UNUSED1,	// no TalkInit sets these three
	TLK_UNUSED2,
	TLK_UNUSED3,
	TLK_PAIN,
	TLK_DIE,

	TLK_CGROUPS,					// MUST be last entry
} TALKGROUPNAMES;


enum
{
	SCHED_CANT_FOLLOW = LAST_COMMON_SCHEDULE + 1,
	SCHED_MOVE_AWAY,		// Try to get out of the player's way
	SCHED_MOVE_AWAY_FOLLOW,	// same, but follow afterward
	SCHED_MOVE_AWAY_FAIL,	// Turn back toward player

	LAST_TALKMONSTER_SCHEDULE,		// MUST be last
};

enum
{
	TASK_MOVE_AWAY_PATH = LAST_COMMON_TASK + 1,
	TASK_WALK_PATH_FOR_UNITS,

	TASK_TLK_RESPOND,		// say my response
	TASK_TLK_SPEAK,			// question or remark
	TASK_TLK_HEADRESET,		// reset head position
	TASK_TLK_EYECONTACT,	// maintain eyecontact with person who I'm talking to
	TASK_TLK_IDEALYAW,		// set ideal yaw to face who I'm talking to

	LAST_TALKMONSTER_TASK,			// MUST be last
};

class CTalkMonster : public CBaseMonster
{
public:
	void			TalkInit( void );				
	CBaseEntity		*FindNearestFriend(BOOL fPlayer);
	float			TargetDistance( void );
	void			StopTalking( void ) { SentenceStop(); }
	
	// Base Monster functions
	void			Precache( void );
	int				TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType);
	void			Killed( entvars_t *pevAttacker, int iGib );
	int				IRelationship ( CBaseEntity *pTarget );
	virtual int		CanPlaySentence( BOOL fDisregardState );
	virtual void	PlaySentence( const char *pszSentence, float duration, float volume, float attenuation );
	void			PlayScriptedSentence( const char *pszSentence, float duration, float volume, float attenuation, BOOL bConcurrent, CBaseEntity *pListener );
	void			KeyValue( KeyValueData *pkvd );

	// AI functions
	void			SetActivity ( Activity newActivity );
	Schedule_t		*GetScheduleOfType ( int Type );
	void			StartTask( Task_t *pTask );
	void			RunTask( Task_t *pTask );
	void			HandleAnimEvent( MonsterEvent_t *pEvent );
	void			PrescheduleThink( void );
	

	// Conversations / communication
	int				GetVoicePitch( void );
	void			IdleRespond( void );
	int				FIdleSpeak( void );
	void			IdleHeadTurn( Vector &vecFriend );
	int				FOkToSpeak( void );
	void			TrySmellTalk( void );
	CBaseEntity		*EnumFriends( CBaseEntity *pentPrevious, int listNumber, BOOL bTrace );
	void			ShutUpFriends( void );
	BOOL			IsTalking( void );
	void			Talk( float flDuration );	
	
	virtual void	SetAnswerQuestion( CTalkMonster *pSpeaker );
	virtual int		FriendNumber( int arrayNumber )	{ return arrayNumber; }

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	
	static char *m_szFriends[TLK_CFRIENDS];		// array of friend names
	static float g_talkWaitTime;
	
	int			m_bitsSaid;						// set bits for sentences we don't want repeated
	int			m_nSpeak;						// number of times initiated talking
	int			m_voicePitch;					// pitch of voice for this head
	const char	*m_szGrp[TLK_CGROUPS];			// sentence group names
	float		m_useTime;						// Don't allow +USE until this time
	int			m_iszUse;						// Custom +USE sentence group (follow)
	int			m_iszUnUse;						// Custom +USE sentence group (stop following)

	float		m_flLastSaidSmelled;// last time we talked about something that stinks
	float		m_flStopTalkTime;// when in the future that I'll be done saying this sentence.

	EHANDLE		m_hTalkTarget;	// who to look at while talking
	CUSTOM_SCHEDULES;
};


// Clients can push talkmonsters out of their way
#define		bits_COND_CLIENT_PUSH		( bits_COND_SPECIAL1 )
// Don't see a client right now.
#define		bits_COND_CLIENT_UNSEEN		( bits_COND_SPECIAL2 )


#endif		//TALKMONSTER_H

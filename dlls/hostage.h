/***
*
*	Counter-Strike Beta 1.1, hostage.h: the hostage (hostage_entity).
*
*	client.cpp (ClientCommand) and multiplay_gamerules.cpp (RestartRound) call
*	CHostage's non-virtual Remove and RePosition, so its declaration was
*	shared; where it stood the image cannot say. This header and its name
*	are ours. Class and method names are retail's: the export
*	?IdleThink@CHostage@@QAEXXZ, the rest from Beta 3.1's symbols. The
*	layout is retail's (336 bytes, 56 of its own, two Vector members by the
*	constructor); the member names are ours.
*
****/
#ifndef HOSTAGE_H
#define HOSTAGE_H

class CHostage : public CBaseMonster
{
public:
	void Spawn( void );
	void Precache( void );
	int  Classify ( void );
	void SetActivity( int activity );
	void SetCollisionBox( void );
	void EXPORT IdleThink( void );
	void EXPORT Remove( void );
	void RePosition( void );
	int  TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType );
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	int  BloodColor( void ) { return BLOOD_COLOR_RED; }

	// names ours; offsets are retail's. m_hTargetEnt (CBaseMonster's) is the
	// player the hostage follows.
	int		m_iActivity;		// 0x118 the activity SetActivity last started
	BOOL	m_fTouched;			// 0x11c new in Beta 1.1: a counter-terrorist has used it since the last RePosition (the first use pays)
	float	m_flFollowDistance;	// 0x120 Spawn: 128; a following hostage stops closer than this
	float	m_flFlinchTime;		// 0x124 TakeDamage: now + 0.75; IdleThink leaves the activity alone until then
	float	m_flNextUse;		// 0x128 Use answers once a second
	int		m_iUnused12C;		// 0x12c Spawn: 0; nothing else refers to it
	float	m_flFollowRange;	// 0x130 only written: Spawn 1024, Use 300 when told to stay, 1024 when following
	BOOL	m_fStopped;			// 0x134 TRUE: stays where it is (Spawn); FALSE: follows m_hTargetEnt (Use toggles it)
	Vector	m_vecUnused138;		// 0x138 constructed, never used
	Vector	m_vecStart;			// 0x144 where Spawn stood; RePosition moves the hostage back there
};

#endif // HOSTAGE_H

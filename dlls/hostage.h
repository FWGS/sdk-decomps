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
	void Touch( CBaseEntity *pOther );
	int  BloodColor( void ) { return BLOOD_COLOR_RED; }

	// names ours; offsets are retail's. m_hTargetEnt (CBaseMonster's) is the
	// player the hostage follows.
	int		m_iActivity;		// 0x118 the activity SetActivity last started
	BOOL	m_fTouched;			// 0x11c new in Beta 1.1: a counter-terrorist has used it since the last RePosition (the first use pays)
	float	m_flFollowDistance;	// 0x120 Spawn: 128; Beta 2.0's Use: 82 when following, 36 when told to stay; IdleThink moves the hostage only when farther than this
	float	m_flFlinchTime;		// 0x124 TakeDamage: now + 0.75; IdleThink leaves the activity alone until then
	float	m_flNextUse;		// 0x128 Use answers once a second
	float	m_flNextRescueCheck;	// 0x12c Beta 4.1: IdleThink looks for a rescue point once a second (Spawn: 0; Beta 1.1 to 4.0 nothing else referred to it)
	float	m_flFollowRange;	// 0x130 only written: Spawn 1024, Use 300 when told to stay, 1024 when following
	BOOL	m_fStopped;			// 0x134 TRUE: stays where it is (Spawn); FALSE: follows m_hTargetEnt (Use toggles it); 2 (Beta 2.0's IdleThink): no leader within reach
	Vector	m_vecStart;			// 0x138 where Spawn stood; RePosition moves the hostage back there (Beta 3.0 drops the unused Vector before it)
	Vector	m_vecPath[20];		// 0x144 new in Beta 3.0: the positions of the leader's trail the hostage walks along (IdleThink)
	int		m_iPathIndex;		// 0x234 the point of m_vecPath it walks to
};

#endif // HOSTAGE_H

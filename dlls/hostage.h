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
	int  ObjectCaps( void );	// Beta 5.0 (leak) (Linux symbol): the hostage is used on and off

	// names ours; offsets are Beta 6.0's (CHostage is 0x258 bytes, GetClassPtr<CHostage>).
	// m_hTargetEnt (CBaseMonster's) is the player the hostage follows.
	int		m_iActivity;		// 0x118 the activity SetActivity last started
	BOOL	m_fTouched;			// 0x11c new in Beta 1.1: a counter-terrorist has used it since the last RePosition (the first use pays)
	BOOL	m_fRescued;			// 0x120 new in Beta 5.0 (leak): it reached a rescue zone (CHostageRescue::HostageRescueTouch, IdleThink); Spawn clears it
	float	m_flFollowDistance;	// 0x124 Spawn: 128; Use: 128 when following, 60 when told to stay; IdleThink moves the hostage only when farther than this
	float	m_flFlinchTime;		// 0x128 TakeDamage: now + 0.75; IdleThink leaves the activity alone until then
	float	m_flNextUse;		// 0x12c Use answers once a second
	float	m_flNextRescueCheck;	// 0x130 Beta 4.1: IdleThink looked for a rescue point once a second; from Beta 5.0 (leak) on only Spawn sets it (0)
	float	m_flFollowRange;	// 0x134 only written: Spawn 1024, Use 300 when told to stay, 1024 when following
	float	m_flNextStuckCheck;	// 0x138 new in Beta 5.0, unreferenced there; Beta 6.0: when IdleThink next notes the position for the stuck check (every 3 seconds on the trail; set when the trail is taken)
	int		m_iUnused13C;		// 0x13c new in Beta 5.0: no function refers to it
	int		m_iUnused140;		// 0x140 new in Beta 6.0: no function refers to it (m_fStopped and the rest are 4 bytes later than in Beta 5.2)
	BOOL	m_fStopped;			// 0x144 TRUE: stays where it is (Spawn); FALSE: follows m_hTargetEnt (Use toggles it); 2: no leader within reach; 6 (Beta 3.0): the leader is out of sight, it walks his trail
	Vector	m_vecStart;			// 0x148 where Spawn stood; RePosition moves the hostage back there (Beta 3.0 drops the unused Vector before it)
	Vector	m_vecPath[20];		// 0x154 new in Beta 3.0: the positions of the leader's trail the hostage walks along (IdleThink copies his m_vecOldOrigins)
	Vector	m_vecStuckCheckPos;	// 0x244 new in Beta 6.0: the position noted every 3 seconds on the trail; not 4 units from it means stuck: the trail is taken afresh (IdleThink)
	int		m_iPathIndex;		// 0x250 the point of m_vecPath it walks to
	int		m_iUnused23C;		// 0x254 new in Beta 5.0 (leak): no function refers to it
};

#endif // HOSTAGE_H

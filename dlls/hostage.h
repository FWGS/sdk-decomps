/***
*
*	Counter-Strike Beta 1.1, hostage.h: the hostage (hostage_entity).
*
*	client.cpp (ClientCommand) and multiplay_gamerules.cpp (RestartRound) call
*	CHostage's non-virtual Remove and RePosition, so its declaration was
*	shared; where it stood the image cannot say. Class and method names are
*	retail's: the export ?IdleThink@CHostage@@QAEXXZ, the rest from the Linux
*	libraries' symbols.
*
*	Counter-Strike 1.4: the hostage finds its way (hostage_localnav.h). The
*	class is 0x7a0 bytes (GetClassPtr<CHostage>). The header's name, the
*	members' names, types and order and the enum are Counter-Strike 1.6's
*	(debug information of its Linux library: its class has every member of
*	1.4's at the same place, 0x14 higher, and four more behind
*	m_iHostageIndex); the offsets are where 1.4's code reads or writes the
*	members. Five it does not touch (m_flMarkPosition, m_iModel, m_iSkin,
*	m_iWaypoint, m_target): they fill the gaps in 1.6's order.
*	The methods are the 1.4 Linux library's symbols, declared in the order
*	of 1.6's class.
*
****/
#ifndef HOSTAGE_H
#define HOSTAGE_H

#define MAX_NODES			100		// CLocalNav's nodes (hostage_localnav.h); name ours
#define MAX_HOSTAGES		12		// name ours

enum state { FOLLOW, STAND, DUCK, SCARED, IDLE, FOLLOWPATH };

class CLocalNav;

class CHostage : public CBaseMonster
{
public:
	void Spawn( void );
	void Precache( void );
	void EXPORT IdleThink( void );
	void EXPORT Remove( void );
	void RePosition( void );
	void SetActivity( int act );
	int  ObjectCaps( void );
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	int  TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType );
	void Touch( CBaseEntity *pOther );
	int  Classify ( void ) { return CLASS_HUMAN_PASSIVE; }
	int  GetActivity( void ) { return m_Activity; }
	int  BloodColor( void ) { return BLOOD_COLOR_RED; }
	float GetModifiedDamage( float flDamage, int nHitGroup );
	void SetFlinchActivity( void );
	void SetDeathActivity( void );
	void PlayPainSound( void );
	void PlayFollowRescueSound( void );
	void AnnounceDeath( CBasePlayer *pAttacker );
	void ApplyHostagePenalty( CBasePlayer *pAttacker );
	void GiveCTTouchBonus( CBasePlayer *pPlayer );
	void SendHostagePositionMsg( void );
	void SendHostageEventMsg( void );
	void DoFollow( void );
	BOOL IsOnLadder( void );
	void PointAt( Vector &vecLoc );
	void MoveToward( Vector &vecLoc );
	void NavReady( void );
	void Wiggle( void );
	void PreThink( void );

	int		m_Activity;				// 0x180 the activity SetActivity last started
	BOOL	m_bTouched;				// 0x184 a counter-terrorist has used it since the last RePosition (the first use pays)
	BOOL	m_bRescueMe;			// 0x188 it reached a rescue zone (CHostageRescue::HostageRescueTouch, IdleThink)
	float	m_flFlinchTime;			// 0x18c TakeDamage: now + 0.75; the activity is left alone until then
	float	m_flNextChange;			// 0x190
	float	m_flMarkPosition;		// 0x194
	int		m_iModel;				// 0x198
	int		m_iSkin;				// 0x19c
	float	m_flNextRadarTime;		// 0x1a0
	state	m_State;				// 0x1a4
	Vector	m_vStart;				// 0x1a8 where Spawn stood; RePosition moves the hostage back there
	Vector	m_vStartAngles;			// 0x1b4
	Vector	m_vPathToFollow[20];	// 0x1c0
	int		m_iWaypoint;			// 0x2b0
	CBasePlayer *m_target;			// 0x2b4
	CLocalNav *m_LocalNav;			// 0x2b8
	int		nTargetNode;			// 0x2bc
	Vector	vecNodes[MAX_NODES];	// 0x2c0
	EHANDLE	m_hStoppedTargetEnt;	// 0x770
	float	m_flNextFullThink;		// 0x778
	float	m_flPathCheckInterval;	// 0x77c
	float	m_flLastPathCheck;		// 0x780
	int		m_nPathNodes;			// 0x784
	BOOL	m_fHasPath;				// 0x788
	float	m_flPathAcquired;		// 0x78c
	Vector	m_vOldPos;				// 0x790
	int		m_iHostageIndex;		// 0x79c
};

#endif // HOSTAGE_H

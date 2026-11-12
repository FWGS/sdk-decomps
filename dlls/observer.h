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
//
// observer.h: Counter-Strike Beta 5.0 (leak)'s observer (observer.cpp). The
// class, its nested CGhosts and their methods are Beta 5.0's Linux symbols
// (Beta 5.0's SetChaseMode takes a char, its GetChaseOrigin a third parameter;
// both return a static Vector); the exports give ChaseMoveThink, ChaseMoveDelayThink and
// FreeMoveThink as protected, Spawn as public. The file name of the header
// and every member name are ours. CObserver is 0x40 bytes (GetClassPtr).
//
#ifndef OBSERVER_H
#define OBSERVER_H

class CObserver : public CBaseEntity
{
public:
	CObserver();
	static CObserver *Create( void );
	void Precache( void );
	virtual void EXPORT Spawn( void );	// exported (retail's export table)
	void SetOwner( CBasePlayer *pOwner );

	// the status bar's (names ours; inline: player.obj has the bodies)
	CBasePlayer *GetTarget( void ) { return m_iTarget ? (CBasePlayer *)UTIL_PlayerByIndex( m_iTarget ) : NULL; }
	BOOL IsChasing( void ) { return m_pfnThink == (BASEPTR)&CObserver::ChaseMoveThink; }

	// the ghosts: an observer's position shown to the other observers as a
	// sprite of his team's colour, every mp_ghostfrequency seconds; and the
	// observer's hints
	class CGhosts
	{
	public:
		void Reset( void );
		void SetHintMessage( void );
		void Update( CBasePlayer *pOwner, const Vector &vecOrigin );

		float	m_flNextUpdate;		// 0x00 when Update sends the ghost next
		float	m_flHintTime;		// 0x04 when Update shows the observer hints
		BOOL	m_fShowHint;		// 0x08 SetHintMessage asks for them
	};

protected:
	void NextClient( void );
	void EXPORT ChaseMoveThink( void );
	void EXPORT ChaseMoveDelayThink( void );
	void EXPORT FreeMoveThink( void );
	int FindNextClient( int iStart, BOOL bSameTeam );
	void HandleButtons( void );
	void SetChaseMode( char cMode );	// Beta 5.0: 0 free look, 1 locked (Linux symbol: a char)
	void SetFreeMode( void );
	const Vector &GetChaseOrigin( const Vector &angles, entvars_t *pevTarget, Vector &vecOffset );	// Beta 5.0: the offset (Linux symbol)

	int			m_iTarget;			// 0x20 the client index of the player chased (0: none)
	CBasePlayer	*m_pOwner;			// 0x24 the dead player this observer is (SetOwner)
	CBaseEntity	*m_pLastTarget;		// 0x28 the player whose view the owner last got (ChaseMoveThink)
	int			m_iLastButtons;		// 0x2c the owner's buttons at the last HandleButtons
	float		m_flNextChase;		// 0x30 ChaseMoveDelayThink looks for a new player from then on
	char		m_cChaseMode;		// 0x34 Beta 5.0: SetChaseMode's mode, 0 free look (behind the target, the observer's own view), 1 locked (behind the target, looking his way); HandleButtons, ChaseMoveThink (name ours)
	CGhosts		m_ghosts;			// 0x38 [0x34]
};

#endif // OBSERVER_H

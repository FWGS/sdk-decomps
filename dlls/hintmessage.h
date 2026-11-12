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
// hintmessage.h: Counter-Strike Beta 5.0 (leak)'s hint messages. The class
// and its three functions are Beta 5.0's Linux symbols (hintmessage.cpp:
// CHintMessageQueue::Reset, Update(CBaseEntity &), AddMessage(const char *));
// the file name and the members are ours. CBasePlayer has one at 0x2ac (48
// bytes): Spawn empties it, CBasePlayer::HintMessage adds, PreThink shows.
//
#ifndef HINTMESSAGE_H
#define HINTMESSAGE_H

class CHintMessageQueue
{
public:
	void Reset( void );
	void Update( CBaseEntity &player );
	bool AddMessage( const char *message );

	int			m_iMessages;			// 0x00 messages waiting, 8 at most (AddMessage refuses more)
	int			m_iShown;				// 0x04 the slot Update empties before it steps to the next and shows that
	int			m_iFree;				// 0x08 the slot AddMessage fills next (Reset: 1)
	float		m_flNextMessage;		// 0x0c Update shows nothing before this time, and sets it on when it shows one
	const char	*m_rgpszMessages[8];	// 0x10 the ring of waiting messages
};

#endif // HINTMESSAGE_H

/***
*
*	Counter-Strike Beta 5.0 (leak), hintmessage.cpp: the queue of hint messages
*	a player is shown (CHintMessageQueue, hintmessage.h; class, method and
*	the static tp are Beta 5.0's Linux symbols; the member names are ours).
*
*	Up to eight hints wait in a ring; every five seconds at most PreThink's
*	Update shows the next on the HUD (channel 1, green, 4 seconds, typed
*	out; Beta 6.0: a second per 25 characters, at least one, and the static
*	tp is a local set at each call, no symbol in Beta 6.0's Linux library;
*	Counter-Strike 1.1: sent as a HudText message, the client draws it).
*	Reset empties the ring at Spawn.
*
****/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"

void CHintMessageQueue::Reset( void )
{
	m_iMessages = 0;
	m_iShown = 0;
	m_iFree = 1;
	m_flNextMessage = 0;
	for ( int i = 0; i < 8; i++ )
		m_rgpszMessages[ i ] = NULL;
}

void CHintMessageQueue::Update( CBaseEntity &player )
{
	// Counter-Strike 1.1: the hint goes out as a HudText message of its own
	// making, the string (a "#Hint_..." title of the client's now) and a byte 1
	// (UTIL_ShowMessage writes 0 there), cut at 499 characters. Beta 6.0 to
	// 1.0b drew it here with UTIL_HudMessage: green, typed out, a second per
	// 25 characters.
	if ( gpGlobals->time > m_flNextMessage && m_iMessages )
	{
		m_rgpszMessages[ m_iShown ] = NULL;
		m_iMessages--;
		m_iShown = ( m_iShown + 1 ) % 8;
		m_flNextMessage = gpGlobals->time + 6;	// Beta 5.0 (the leak: 5)

		if ( m_rgpszMessages[ m_iShown ] )
		{
			MESSAGE_BEGIN( MSG_ONE, gmsgHudText, NULL, player.pev );
				if ( strlen( m_rgpszMessages[ m_iShown ] ) < 500 )
				{
					WRITE_STRING( m_rgpszMessages[ m_iShown ] );
				}
				else
				{
					char tmp[500];
					strncpy( tmp, m_rgpszMessages[ m_iShown ], 499 );
					tmp[499] = 0;
					WRITE_STRING( tmp );
				}
				WRITE_BYTE( 1 );
			MESSAGE_END();
		}
	}
}

bool CHintMessageQueue::AddMessage( const char *message )
{
	if ( m_iMessages < 8 )
	{
		m_rgpszMessages[ m_iFree ] = message;
		m_iFree = ( m_iFree + 1 ) % 8;
		m_iMessages++;
		return true;
	}

	return false;
}

/***
*
*	Counter-Strike Beta 5.0 (leak), hintmessage.cpp: the queue of hint messages
*	a player is shown (CHintMessageQueue, hintmessage.h; class, method and
*	the static tp are Beta 5.0's Linux symbols; the member names are ours).
*
*	Up to eight hints wait in a ring; every five seconds at most PreThink's
*	Update shows the next on the HUD (channel 1, green, 4 seconds, typed
*	out). Reset empties the ring at Spawn.
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
	static hudtextparms_t tp = { -1, 0.7, 2, 40, 255, 40, 200, 0, 255, 0, 200, 0.01, 0.7, 4.0, 0.07, 1 };

	if ( gpGlobals->time > m_flNextMessage && m_iMessages )
	{
		m_rgpszMessages[ m_iShown ] = NULL;
		m_iMessages--;
		m_iShown = ( m_iShown + 1 ) % 8;
		m_flNextMessage = gpGlobals->time + 5;

		if ( m_rgpszMessages[ m_iShown ] )
			UTIL_HudMessage( &player, tp, m_rgpszMessages[ m_iShown ] );
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

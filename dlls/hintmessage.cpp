/***
*
*	Counter-Strike Beta 5.0 (leak), hintmessage.cpp: the queue of hint messages
*	a player is shown (CHintMessageQueue, hintmessage.h; class, method and
*	the static tp are Beta 5.0's Linux symbols; the member names are ours).
*
*	Up to eight hints wait in a ring; every five seconds at most PreThink's
*	Update shows the next on the HUD (channel 1, green, 4 seconds, typed
*	out; Beta 6.0: a second per 25 characters, at least one, and the static
*	tp is a local set at each call, no symbol in Beta 6.0's Linux library).
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
	// Beta 6.0: the parameters are set at each call (Beta 5.2: a static table),
	// and a hint stays a second per 25 characters, at least one
	hudtextparms_t tp;	// name ours

	tp.x = -1;
	tp.y = 0.7;
	tp.effect = 2;
	tp.r1 = 40;
	tp.g1 = 255;
	tp.b1 = 40;
	tp.a1 = 200;
	tp.r2 = 0;
	tp.g2 = 255;
	tp.b2 = 0;
	tp.a2 = 200;
	tp.fadeinTime = 0.01;
	tp.fadeoutTime = 0.7;
	tp.holdTime = 5;
	tp.fxTime = 0.07;
	tp.channel = 1;

	if ( gpGlobals->time > m_flNextMessage && m_iMessages )
	{
		m_rgpszMessages[ m_iShown ] = NULL;
		m_iMessages--;
		m_iShown = ( m_iShown + 1 ) % 8;
		m_flNextMessage = gpGlobals->time + 6;	// Beta 5.0 (the leak: 5)

		if ( m_rgpszMessages[ m_iShown ] )
		{
			tp.holdTime = strlen( m_rgpszMessages[ m_iShown ] ) / 25;

			if ( tp.holdTime < 1 )
				tp.holdTime = 1;

			UTIL_HudMessage( &player, tp, m_rgpszMessages[ m_iShown ] );
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

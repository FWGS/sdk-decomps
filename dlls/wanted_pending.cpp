/***
*
*	Wanted! code that retail defines in its own source files, which the
*	reconstruction hasn't written yet. Each item moves to its file when that
*	file is reconstructed; this file then goes away.
*
****/

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"

// monster_horse's file: horses and pumas get big boxes while a bullet is
// traced, so that the trace hits them, and their own sizes back afterwards
// (horses are not solid otherwise; dead pumas aren't either). Name ours.
void ExpandHorseHulls( int fExpand )
{
	char *pszClassname;

	for ( int i = 0; i < 2; i++ )
	{
		if ( i == 0 )
			pszClassname = "monster_horse";
		else if ( i == 1 )
			pszClassname = "monster_puma";

		CBaseEntity *pEntity = NULL;
		while ( ( pEntity = UTIL_FindEntityByClassname( pEntity, pszClassname ) ) != NULL )
		{
			if ( fExpand )
			{
				UTIL_SetSize( pEntity->pev, Vector( -76, -76, 0 ), Vector( 76, 76, 128 ) );
				pEntity->pev->solid = SOLID_SLIDEBOX;
			}
			else
			{
				UTIL_SetSize( pEntity->pev, pEntity->pev->startpos, pEntity->pev->endpos );
				if ( i == 0 )
					pEntity->pev->solid = SOLID_NOT;
				else if ( i == 1 )
				{
					if ( pEntity->IsAlive() )
						pEntity->pev->solid = SOLID_SLIDEBOX;
					else
						pEntity->pev->solid = SOLID_NOT;
				}
			}
			UTIL_SetOrigin( pEntity->pev, pEntity->pev->origin );
		}
	}
}

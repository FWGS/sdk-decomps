/***
*
*	Scientist Slaughterhouse: the linked entity (entity_link.cpp).
*	The file name and the names marked "ours" are ours.
*
*	Not in cbase.h: declared there it changes how VC6 compiles other files
*	(CGraph::LinkVisibleNodes in nodes.cpp no longer matches retail).
*
****/
#ifndef ENTITY_LINK_H
#define ENTITY_LINK_H

//
// CBaseLinked: follows the entity named by "link_to_this", keeping its
// offset and angles relative to it. info_target_linked; base of info_camera,
// the func_turret shoot points and fire_trail.
//
#define SF_LINKED_FAKEMODEL	1024	// "Fake Model": an invisible sprite, so the client knows the entity

class CBaseLinked : public CBaseEntity
{
public:
	void	Spawn( void );
	void	KeyValue( KeyValueData *pkvd );
	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	void	InitLink( void );	// name ours
	void	EXPORT LinkThink( void );

	string_t	m_iszLinkEnt;		// "link_to_this"
	EHANDLE		m_hLinked;
	Vector		m_vecRelativePos;	// the linked entity's origin minus ours, at InitLink
	Vector		m_vecAngles;		// our angles at InitLink
};

#endif // ENTITY_LINK_H

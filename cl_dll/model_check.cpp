//
// model_check.cpp (file name ours)
//
// Counter-Strike's check of the player and weapon models (names ours): the bounding
// box of each model's vertices and of its animated bones is measured from the file on
// disk; a model larger than the shipped ones means a modified model, and CHud::Init and
// CHud::MsgFunc_ResetHUD disconnect the player.
//

#include "hud.h"
#include "cl_util.h"
#include "studio.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_MODEL_BOUNDS	128		// name ours

class CModelBounds	// name ours
{
public:
	CModelBounds() { Clear(); }

	void Clear( void );
	void AddPoint( const Vector &vecPoint );
	void AddBonePoint( const Vector &vecPoint );
	void AddBoneBox( void );
	void Load( const char *pszName );

	const char *m_pszName;
	Vector m_vecMins;
	Vector m_vecMaxs;
	int m_iPoints;
	Vector m_vecBoneMins;
	Vector m_vecBoneMaxs;
	int m_iBonePoints;
};

// the count is a global of its own: VC6 keeps a store to it apart from the array's
// loads only when the two are different symbols (LoadModelBounds' order shows it)
CModelBounds g_ModelBounds[MAX_MODEL_BOUNDS];	// name ours
int g_iModelBounds;	// name ours (hashes after g_ModelBounds: retail's .bss order)

static char *g_szCheckModels[] =	// name ours
{
	"models/player.mdl",
	"models/player/arab/arab.mdl",
	"models/player/arctic/arctic.mdl",
	"models/player/gign/gign.mdl",
	"models/player/gsg9/gsg9.mdl",
	"models/player/sas/sas.mdl",
	"models/player/terror/terror.mdl",
	"models/player/urban/urban.mdl",
	"models/player/vip/vip.mdl",
	"models/player/guerilla/guerilla.mdl",
	"models/p_ak47.mdl",
	"models/p_aug.mdl",
	"models/p_awp.mdl",
	"models/p_c4.mdl",
	"models/p_deagle.mdl",
	"models/p_flashbang.mdl",
	"models/p_smokegrenade.mdl",
	"models/p_g3sg1.mdl",
	"models/p_glock18.mdl",
	"models/p_hegrenade.mdl",
	"models/p_knife.mdl",
	"models/p_m249.mdl",
	"models/p_m3.mdl",
	"models/p_m4a1.mdl",
	"models/p_mac10.mdl",
	"models/p_mp5.mdl",
	"models/p_p228.mdl",
	"models/p_p90.mdl",
	"models/p_scout.mdl",
	"models/p_sg552.mdl",
	"models/p_tmp.mdl",
	"models/p_usp.mdl",
	"models/p_xm1014.mdl",
	NULL
};

void CModelBounds :: Clear( void )
{
	m_pszName = NULL;
	m_vecMins = m_vecMaxs = Vector( 0, 0, 0 );
	m_iPoints = 0;
	m_vecBoneMins = m_vecBoneMaxs = Vector( 0, 0, 0 );
	m_iBonePoints = 0;
}

void CModelBounds :: AddPoint( const Vector &vecPoint )
{
	if ( !m_iPoints )
	{
		m_vecMaxs = vecPoint;
		m_vecMins = vecPoint;
	}
	else
	{
		m_vecMins.x = min( m_vecMins.x, vecPoint.x );
		m_vecMins.y = min( m_vecMins.y, vecPoint.y );
		m_vecMins.z = min( m_vecMins.z, vecPoint.z );
		m_vecMaxs.x = max( m_vecMaxs.x, vecPoint.x );
		m_vecMaxs.y = max( m_vecMaxs.y, vecPoint.y );
		m_vecMaxs.z = max( m_vecMaxs.z, vecPoint.z );
	}
	m_iPoints++;
}

void CModelBounds :: AddBonePoint( const Vector &vecPoint )
{
	if ( !m_iBonePoints )
	{
		m_vecBoneMaxs = vecPoint;
		m_vecBoneMins = vecPoint;
	}
	else
	{
		m_vecBoneMins.x = min( m_vecBoneMins.x, vecPoint.x );
		m_vecBoneMins.y = min( m_vecBoneMins.y, vecPoint.y );
		m_vecBoneMins.z = min( m_vecBoneMins.z, vecPoint.z );
		m_vecBoneMaxs.x = max( m_vecBoneMaxs.x, vecPoint.x );
		m_vecBoneMaxs.y = max( m_vecBoneMaxs.y, vecPoint.y );
		m_vecBoneMaxs.z = max( m_vecBoneMaxs.z, vecPoint.z );
	}
	m_iBonePoints++;
}

// the bones' box, centred on the origin, goes into the model's box
void CModelBounds :: AddBoneBox( void )
{
	if ( m_iBonePoints <= 0 )
		return;

	Vector vecSize = m_vecBoneMaxs - m_vecBoneMins;
	AddPoint( vecSize * 0.5 );
	AddPoint( -( vecSize * 0.5 ) );

	m_vecBoneMins = m_vecBoneMaxs = Vector( 0, 0, 0 );
	m_iBonePoints = 0;
}

static void CalcBonePosition( int frame, mstudiobone_t *pbone, mstudioanim_t *panim, float *pos )
{
	int					j, k;
	mstudioanimvalue_t	*panimvalue;
	float s = 0;	// no frame interpolation here

	for (j = 0; j < 3; j++)
	{
		pos[j] = pbone->value[j]; // default;
		if (panim->offset[j] != 0)
		{
			panimvalue = (mstudioanimvalue_t *)((byte *)panim + panim->offset[j]);
			
			k = frame;
			// find span of values that includes the frame we want
			while (panimvalue->num.total <= k)
			{
				k -= panimvalue->num.total;
				panimvalue += panimvalue->num.valid + 1;
			}
			// if we're inside the span
			if (panimvalue->num.valid > k)
			{
				// and there's more data in the span
				if (panimvalue->num.valid > k + 1)
				{
					pos[j] += (panimvalue[k+1].value * (1.0 - s) + s * panimvalue[k+2].value) * pbone->scale[j];
				}
				else
				{
					pos[j] += panimvalue[k+1].value * pbone->scale[j];
				}
			}
			else
			{
				// are we at the end of the repeating values section and there's another section with data?
				if (panimvalue->num.total <= k + 1)
				{
					pos[j] += (panimvalue[panimvalue->num.valid].value * (1.0 - s) + s * panimvalue[panimvalue->num.valid + 2].value) * pbone->scale[j];
				}
				else
				{
					pos[j] += panimvalue[panimvalue->num.valid].value * pbone->scale[j];
				}
			}
		}
	}
}

void CModelBounds :: Load( const char *pszName )
{
	char szPath[_MAX_PATH];	// name ours

	if ( strchr( pszName, ':' ) )
		strcpy( szPath, pszName );
	else
		sprintf( szPath, "%s/%s", "cstrike", pszName );

	FILE *fp = fopen( szPath, "rb" );
	if ( !fp )
		return;

	fseek( fp, 0, SEEK_END );
	int size = ftell( fp );
	fseek( fp, 0, SEEK_SET );

	byte *pBuffer = (byte *)malloc( size );
	if ( pBuffer )
	{
		if ( fread( pBuffer, 1, size, fp ) == size )
		{
			studiohdr_t *phdr = (studiohdr_t *)pBuffer;
			mstudiobodyparts_t *pbodyparts = (mstudiobodyparts_t *)( pBuffer + phdr->bodypartindex );
			int i, j, k;
			int nummodels = 0;

			for ( i = 0; i < phdr->numbodyparts; i++ )
				nummodels += pbodyparts[i].nummodels;

			mstudiomodel_t *pmodel = (mstudiomodel_t *)&pbodyparts[phdr->numbodyparts];
			for ( i = 0; i < nummodels; i++, pmodel++ )
			{
				int numverts = pmodel->numverts;
				vec3_t *pverts = (vec3_t *)( pBuffer + pmodel->vertindex );
				for ( j = 0; j < numverts; j++ )
					AddPoint( pverts[j] );
			}

			mstudiobone_t *pbones = (mstudiobone_t *)( pBuffer + phdr->boneindex );
			mstudioseqdesc_t *pseqdesc = (mstudioseqdesc_t *)( pBuffer + phdr->seqindex );
			int numbones = phdr->numbones;
			int numseq = phdr->numseq;

			for ( i = 0; i < numseq; i++, pseqdesc++ )
			{
				int numframes = pseqdesc->numframes;
				mstudioanim_t *panim = (mstudioanim_t *)( pBuffer + pseqdesc->animindex );
				for ( j = 0; j < numbones; j++ )
				{
					for ( k = 0; k < numframes; k++ )
					{
						float pos[3];
						CalcBonePosition( k, &pbones[j], panim, pos );
						AddBonePoint( pos );
					}
				}

				AddBoneBox();
			}
		}
		free( pBuffer );
	}
	fclose( fp );
}

void ClearModelBounds( void )	// name ours
{
	g_iModelBounds = 0;
	for ( int i = 0; i < MAX_MODEL_BOUNDS; i++ )
		g_ModelBounds[i].Clear();
}

void LoadModelBounds( void )	// name ours
{
	ClearModelBounds();

	for ( int i = 0; g_szCheckModels[i]; i++ )
	{
		g_ModelBounds[g_iModelBounds].m_pszName = g_szCheckModels[i];
		g_ModelBounds[g_iModelBounds].Load( g_szCheckModels[i] );

		// sic: retail reads the loaded count of points and counts the model either way
		if ( g_ModelBounds[g_iModelBounds].m_iPoints > 0 )
			g_iModelBounds++;
		else
			g_iModelBounds++;
	}
}

// true when no model is larger than the shipped ones
bool CheckModelBounds( void )	// name ours
{
	if ( !g_iModelBounds )
	{
		LoadModelBounds();
		if ( !g_iModelBounds )
			return false;
	}

	bool bOk = true;
	for ( int i = 0; i < g_iModelBounds; i++ )
	{
		Vector vecSize = g_ModelBounds[i].m_vecMaxs - g_ModelBounds[i].m_vecMins;
		Vector vecLimit;

		if ( !strnicmp( g_ModelBounds[i].m_pszName, "models/p_", 9 ) )
			vecLimit = Vector( 50, 60, 65 );
		else
			vecLimit = Vector( 95, 72, 115 );

		if ( vecSize.x > vecLimit.x || vecSize.y > vecLimit.y || vecSize.z > vecLimit.z )
			bOk = false;
	}

	return bOk;
}

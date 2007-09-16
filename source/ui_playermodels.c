/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#include "ui_local.h"

/*
=============================================================================

PLAYER MODELS

=============================================================================
*/

static const char *baseWeaponNames[] = {
	"w_bfg.md2",
	"w_blaster.md2",
	"w_chaingun.md2",
	"w_glauncher.md2",
	"w_hyperblaster.md2",
	"w_machinegun.md2",
	"w_railgun.md2",
	"w_rlauncher.md2",
	"w_shotgun.md2",
	"w_sshotgun.md2",
	NULL
};

static qboolean IconOfSkinExists( char *skin, char **pcxfiles, int npcxfiles ) {
	int i;
	char scratch[MAX_OSPATH];

	COM_StripExtension( skin, scratch, sizeof( scratch ) );
	Q_strcat( scratch, sizeof( scratch ), "_i.pcx" );

	for( i = 0 ; i < npcxfiles ; i++ ) {
		if( strcmp( pcxfiles[i], scratch ) == 0 )
			return qtrue;
	}

	return qfalse;
}

static int QDECL pmicmpfnc( const void *_a, const void *_b ) {
	const playerModelInfo_t *a = (const playerModelInfo_t *)_a;
	const playerModelInfo_t *b = (const playerModelInfo_t *)_b;

	/*
	** sort by male, female, then alphabetical
	*/
	if( strcmp( a->directory, "male" ) == 0 )
		return -1;
	else if( strcmp( b->directory, "male" ) == 0 )
		return 1;

	if( strcmp( a->directory, "female" ) == 0 )
		return -1;
	else if( strcmp( b->directory, "female" ) == 0 )
		return 1;

	return strcmp( a->directory, b->directory );
}

void PlayerModel_Load( void ) {
	char scratch[MAX_QPATH];
	int ndirs = 0;
	char *dirnames[MAX_PLAYERMODELS];
	int i, j;
	char **list;
	char *p;
	int numFiles;
	playerModelInfo_t *pmi;

	uis.numPlayerModels = 0;

	/*
	** get a list of directories
	*/
	if( !( list = fs.ListFiles( NULL, "players/*/*", FS_SEARCH_BYFILTER|FS_SEARCH_SAVEPATH, &numFiles ) ) ) {
		return;
	}

	for( i = 0; i < numFiles; i++ ) {
		Q_strncpyz( scratch, list[i] + 8, sizeof( scratch ) );
		if( ( p = strchr( scratch, '/' ) ) ) {
			*p = 0;
		}

		for( j = 0; j < ndirs; j++ ) {
			if( !strcmp( dirnames[j], scratch ) ) {
				break;
			}
		}

		if( j != ndirs ) {
			continue;
		}
		
		dirnames[ndirs++] = UI_CopyString( scratch );
		if( ndirs == MAX_PLAYERMODELS ) {
			break;
		}
	}

	fs.FreeFileList( list );

	if( !ndirs ) {
		return;
	}

	/*
	** go through the subdirectories
	*/

	for( i = 0; i < ndirs; i++ ) {
		int k, s;
		char **pcxnames;
		char **skinnames;
		int npcxfiles;
		int nskins = 0;
		int numWeapons;
		char **weaponNames;

		// verify the existence of tris.md2
		Com_sprintf( scratch, sizeof( scratch ), "players/%s/tris.md2", dirnames[i] );
		if( fs.LoadFile( scratch, NULL ) < 1 ) {
			continue;
		}

		// verify the existence of at least one pcx skin
		Com_sprintf( scratch, sizeof( scratch ), "players/%s", dirnames[i] );
		pcxnames = fs.ListFiles( scratch, ".pcx", 0, &npcxfiles );
		if( !pcxnames ) {
			continue;
		}

		// count valid skins, which consist of a skin with a matching "_i" icon
		for( k = 0; k < npcxfiles; k++ ) {
			if( !strstr( pcxnames[k], "_i.pcx" ) ) {
				if( IconOfSkinExists( pcxnames[k], pcxnames, npcxfiles ) ) {
					nskins++;
				}
			}
		}

		if( !nskins ) {
			fs.FreeFileList( pcxnames );
			continue;
		}

		skinnames = UI_Malloc( sizeof( char * ) * ( nskins + 1 ) );
        skinnames[nskins] = NULL;

		// copy the valid skins
		for( s = 0, k = 0; k < npcxfiles; k++ ) {
			if( !strstr( pcxnames[k], "_i.pcx" ) ) {
				if( IconOfSkinExists( pcxnames[k], pcxnames, npcxfiles ) ) {
					COM_StripExtension( pcxnames[k], scratch, sizeof( scratch ) );
					skinnames[s++] = UI_CopyString( scratch );
				}
			}
		}

		fs.FreeFileList( pcxnames );

		// load vweap models
		Com_sprintf( scratch, sizeof( scratch ), "players/%s/w_*.md2", dirnames[i] );
		weaponNames = fs.ListFiles( NULL, scratch, FS_SEARCH_BYFILTER, &numWeapons );

		pmi = &uis.pmi[uis.numPlayerModels++];
		pmi->numWeapons = 0;

		if( weaponNames ) {
			pmi->weaponNames = UI_Malloc( sizeof( char * ) * numWeapons );

			for( j = 0; j < numWeapons ; j++ ) {
				for( k = 0; baseWeaponNames[k] ; k++ ) {
					if( !strcmp( weaponNames[j], baseWeaponNames[k] ) ) {
						break;
					}
				}
				if( !baseWeaponNames[k] ) {
					continue;
				}

				pmi->weaponNames[pmi->numWeapons++] = UI_CopyString( weaponNames[j] );
			}

			fs.FreeFileList( weaponNames );
		}

		// at this point we have a valid player model
		pmi->nskins = nskins;
		pmi->skindisplaynames = skinnames;

		// make short name for the model
		strcpy( pmi->directory, dirnames[i] );
	}

	for( i = 0; i < ndirs; i++ ) {
		com.Free( dirnames[i] );
	}

	qsort( uis.pmi, uis.numPlayerModels, sizeof( uis.pmi[0] ), pmicmpfnc );
}



void PlayerModel_Free( void ) {
	playerModelInfo_t *pmi;
	int i, j;

	for( i = 0, pmi = uis.pmi; i < uis.numPlayerModels; i++, pmi++ ) {
		if( pmi->skindisplaynames ) {
			for( j = 0; j < pmi->nskins; j++ ) {		
				com.Free( pmi->skindisplaynames[j] );
			}
			com.Free( pmi->skindisplaynames );
		}
		if( pmi->weaponNames ) {
			for( j = 0; j < pmi->numWeapons; j++ ) {		
				com.Free( pmi->weaponNames[j] );
			}
			com.Free( pmi->weaponNames );
		}
		memset( pmi, 0, sizeof( *pmi ) );
	}

	uis.numPlayerModels = 0;
}
	
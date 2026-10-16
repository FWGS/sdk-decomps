/***
*
*	Copyright (c) 1999, 2000 Valve LLC. All rights reserved.
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
#include "extdll.h"
#include "eiface.h"
#include "util.h"
#include "game.h"


cvar_t	displaysoundlist = {"displaysoundlist","0"};
cvar_t  mapcyclefile = {"mapcyclefile","mapcycle.txt"};
cvar_t  servercfgfile = {"servercfgfile","server.cfg"};
cvar_t  lservercfgfile = {"lservercfgfile","listenserver.cfg"};

// multiplayer server rules
cvar_t	fragsleft	= {"mp_fragsleft","0", FCVAR_SERVER | FCVAR_UNLOGGED };	  // Don't spam console/log files/users with this changing
cvar_t	timeleft	= {"mp_timeleft","0" , FCVAR_SERVER | FCVAR_UNLOGGED };	  // "      "

// multiplayer server rules
cvar_t	teamplay	= {"mp_teamplay","0", FCVAR_SERVER };
cvar_t	fraglimit	= {"mp_fraglimit","0", FCVAR_SERVER };
cvar_t	timelimit	= {"mp_timelimit","0", FCVAR_SERVER };
cvar_t	friendlyfire= {"mp_friendlyfire","0", FCVAR_SERVER };
cvar_t	falldamage	= {"mp_falldamage","0", FCVAR_SERVER };
cvar_t	weaponstay	= {"mp_weaponstay","0", FCVAR_SERVER };
cvar_t	forcerespawn= {"mp_forcerespawn","1", FCVAR_SERVER };
cvar_t	flashlight	= {"mp_flashlight","0", FCVAR_SERVER };
cvar_t	aimcrosshair= {"mp_autocrosshair","1", FCVAR_SERVER };
cvar_t	decalfrequency = {"decalfrequency","30", FCVAR_SERVER };
cvar_t	teamlist = {"mp_teamlist","hgrunt;scientist", FCVAR_SERVER };
cvar_t	teamoverride = {"mp_teamoverride","1" };
cvar_t	defaultteam = {"mp_defaultteam","0" };
cvar_t	allowmonsters={"mp_allowmonsters","0", FCVAR_SERVER };

// Cow-Bots (C names ours)
cvar_t	sv_defaultbots = {"sv_defaultbots","0", FCVAR_SERVER };
cvar_t	sv_adjbotskill = {"sv_adjbotskill","0", FCVAR_SERVER };
cvar_t	sv_botskill = {"sv_botskill","0", FCVAR_EXTDLL };

cvar_t 	*g_psv_gravity = NULL;
cvar_t	*g_psv_aim = NULL;
cvar_t	*g_footsteps = NULL;

//CVARS FOR SKILL LEVEL SETTINGS
// Hoss
cvar_t	sk_hoss_health1 = {"sk_hoss_health1","0"};
cvar_t	sk_hoss_health2 = {"sk_hoss_health2","0"};
cvar_t	sk_hoss_health3 = {"sk_hoss_health3","0"};

// Annie
cvar_t	sk_annie_health1 = {"sk_annie_health1","0"};
cvar_t	sk_annie_health2 = {"sk_annie_health2","0"};
cvar_t	sk_annie_health3 = {"sk_annie_health3","0"};

// Big miner
cvar_t	sk_bigminer_health1 = {"sk_bigminer_health1","0"};
cvar_t	sk_bigminer_health2 = {"sk_bigminer_health2","0"};
cvar_t	sk_bigminer_health3 = {"sk_bigminer_health3","0"};

cvar_t	sk_bigminer_punch1 = {"sk_bigminer_punch1","0"};
cvar_t	sk_bigminer_punch2 = {"sk_bigminer_punch2","0"};
cvar_t	sk_bigminer_punch3 = {"sk_bigminer_punch3","0"};

// Small miner
cvar_t	sk_smallminer_health1 = {"sk_smallminer_health1","0"};
cvar_t	sk_smallminer_health2 = {"sk_smallminer_health2","0"};
cvar_t	sk_smallminer_health3 = {"sk_smallminer_health3","0"};

cvar_t	sk_smallminer_pick1 = {"sk_smallminer_pick1","0"};
cvar_t	sk_smallminer_pick2 = {"sk_smallminer_pick2","0"};
cvar_t	sk_smallminer_pick3 = {"sk_smallminer_pick3","0"};

// Cowboy
cvar_t	sk_cowboy_health1 = {"sk_cowboy_health1","0"};
cvar_t	sk_cowboy_health2 = {"sk_cowboy_health2","0"};
cvar_t	sk_cowboy_health3 = {"sk_cowboy_health3","0"};

cvar_t	sk_cowboy_punch1 = {"sk_cowboy_punch1","0"};
cvar_t	sk_cowboy_punch2 = {"sk_cowboy_punch2","0"};
cvar_t	sk_cowboy_punch3 = {"sk_cowboy_punch3","0"};

cvar_t	sk_cowboy_dynamitespeed1 = {"sk_cowboy_dynamitespeed1","0"};
cvar_t	sk_cowboy_dynamitespeed2 = {"sk_cowboy_dynamitespeed2","0"};
cvar_t	sk_cowboy_dynamitespeed3 = {"sk_cowboy_dynamitespeed3","0"};

// Ramone
cvar_t	sk_ramone_health1 = {"sk_ramone_health1","0"};
cvar_t	sk_ramone_health2 = {"sk_ramone_health2","0"};
cvar_t	sk_ramone_health3 = {"sk_ramone_health3","0"};

cvar_t	sk_ramone_punch1 = {"sk_ramone_punch1","0"};
cvar_t	sk_ramone_punch2 = {"sk_ramone_punch2","0"};
cvar_t	sk_ramone_punch3 = {"sk_ramone_punch3","0"};

cvar_t	sk_ramone_dynamitespeed1 = {"sk_ramone_dynamitespeed1","0"};
cvar_t	sk_ramone_dynamitespeed2 = {"sk_ramone_dynamitespeed2","0"};
cvar_t	sk_ramone_dynamitespeed3 = {"sk_ramone_dynamitespeed3","0"};

// Mexican bandit
cvar_t	sk_mexbandit_health1 = {"sk_mexbandit_health1","0"};
cvar_t	sk_mexbandit_health2 = {"sk_mexbandit_health2","0"};
cvar_t	sk_mexbandit_health3 = {"sk_mexbandit_health3","0"};

cvar_t	sk_mexbandit_kick1 = {"sk_mexbandit_kick1","0"};
cvar_t	sk_mexbandit_kick2 = {"sk_mexbandit_kick2","0"};
cvar_t	sk_mexbandit_kick3 = {"sk_mexbandit_kick3","0"};

cvar_t	sk_mexbandit_dynamitespeed1 = {"sk_mexbandit_dynamitespeed1","0"};
cvar_t	sk_mexbandit_dynamitespeed2 = {"sk_mexbandit_dynamitespeed2","0"};
cvar_t	sk_mexbandit_dynamitespeed3 = {"sk_mexbandit_dynamitespeed3","0"};

// Leech
cvar_t	sk_leech_health1 = {"sk_leech_health1","0"};
cvar_t	sk_leech_health2 = {"sk_leech_health2","0"};
cvar_t	sk_leech_health3 = {"sk_leech_health3","0"};

cvar_t	sk_leech_dmg_bite1 = {"sk_leech_dmg_bite1","0"};
cvar_t	sk_leech_dmg_bite2 = {"sk_leech_dmg_bite2","0"};
cvar_t	sk_leech_dmg_bite3 = {"sk_leech_dmg_bite3","0"};

// Kaiewi (Indian)
cvar_t	sk_kaiewi_health1 = {"sk_kaiewi_health1","0"};
cvar_t	sk_kaiewi_health2 = {"sk_kaiewi_health2","0"};
cvar_t	sk_kaiewi_health3 = {"sk_kaiewi_health3","0"};

cvar_t	sk_kaiewi_punch1 = {"sk_kaiewi_punch1","0"};
cvar_t	sk_kaiewi_punch2 = {"sk_kaiewi_punch2","0"};
cvar_t	sk_kaiewi_punch3 = {"sk_kaiewi_punch3","0"};

// Colonel
cvar_t	sk_colonel_health1 = {"sk_colonel_health1","0"};
cvar_t	sk_colonel_health2 = {"sk_colonel_health2","0"};
cvar_t	sk_colonel_health3 = {"sk_colonel_health3","0"};

// Townsman A
cvar_t	sk_wtowna_health1 = {"sk_wtowna_health1","0"};
cvar_t	sk_wtowna_health2 = {"sk_wtowna_health2","0"};
cvar_t	sk_wtowna_health3 = {"sk_wtowna_health3","0"};

// Townsman B
cvar_t	sk_wtownb_health1 = {"sk_wtownb_health1","0"};
cvar_t	sk_wtownb_health2 = {"sk_wtownb_health2","0"};
cvar_t	sk_wtownb_health3 = {"sk_wtownb_health3","0"};

// Mexican townsman
cvar_t	sk_townmex_health1 = {"sk_townmex_health1","0"};
cvar_t	sk_townmex_health2 = {"sk_townmex_health2","0"};
cvar_t	sk_townmex_health3 = {"sk_townmex_health3","0"};

// Masala
cvar_t	sk_masala_health1 = {"sk_masala_health1","0"};
cvar_t	sk_masala_health2 = {"sk_masala_health2","0"};
cvar_t	sk_masala_health3 = {"sk_masala_health3","0"};

// Nagatow
cvar_t	sk_nagatow_health1 = {"sk_nagatow_health1","0"};
cvar_t	sk_nagatow_health2 = {"sk_nagatow_health2","0"};
cvar_t	sk_nagatow_health3 = {"sk_nagatow_health3","0"};

// Crispen
cvar_t	sk_crispen_health1 = {"sk_crispen_health1","0"};
cvar_t	sk_crispen_health2 = {"sk_crispen_health2","0"};
cvar_t	sk_crispen_health3 = {"sk_crispen_health3","0"};

// Dave
cvar_t	sk_dave_health1 = {"sk_dave_health1","0"};
cvar_t	sk_dave_health2 = {"sk_dave_health2","0"};
cvar_t	sk_dave_health3 = {"sk_dave_health3","0"};

// Scorpion
cvar_t	sk_scorpion_health1 = {"sk_scorpion_health1","0"};
cvar_t	sk_scorpion_health2 = {"sk_scorpion_health2","0"};
cvar_t	sk_scorpion_health3 = {"sk_scorpion_health3","0"};

cvar_t	sk_scorpion_dmg_sting1 = {"sk_scorpion_dmg_sting1","0"};
cvar_t	sk_scorpion_dmg_sting2 = {"sk_scorpion_dmg_sting2","0"};
cvar_t	sk_scorpion_dmg_sting3 = {"sk_scorpion_dmg_sting3","0"};

// Snake
cvar_t	sk_snake_health1 = {"sk_snake_health1","0"};
cvar_t	sk_snake_health2 = {"sk_snake_health2","0"};
cvar_t	sk_snake_health3 = {"sk_snake_health3","0"};

cvar_t	sk_snake_dmg_bite1 = {"sk_snake_dmg_bite1","0"};
cvar_t	sk_snake_dmg_bite2 = {"sk_snake_dmg_bite2","0"};
cvar_t	sk_snake_dmg_bite3 = {"sk_snake_dmg_bite3","0"};

// Puma
cvar_t	sk_puma_health1 = {"sk_puma_health1","0"};
cvar_t	sk_puma_health2 = {"sk_puma_health2","0"};
cvar_t	sk_puma_health3 = {"sk_puma_health3","0"};

cvar_t	sk_puma_dmg_claw1 = {"sk_puma_dmg_claw1","0"};
cvar_t	sk_puma_dmg_claw2 = {"sk_puma_dmg_claw2","0"};
cvar_t	sk_puma_dmg_claw3 = {"sk_puma_dmg_claw3","0"};

cvar_t	sk_puma_dmg_pounce1 = {"sk_puma_dmg_pounce1","0"};
cvar_t	sk_puma_dmg_pounce2 = {"sk_puma_dmg_pounce2","0"};
cvar_t	sk_puma_dmg_pounce3 = {"sk_puma_dmg_pounce3","0"};

// Bear
cvar_t	sk_bear_health1 = {"sk_bear_health1","0"};
cvar_t	sk_bear_health2 = {"sk_bear_health2","0"};
cvar_t	sk_bear_health3 = {"sk_bear_health3","0"};

cvar_t	sk_bear_dmg_claw1 = {"sk_bear_dmg_claw1","0"};
cvar_t	sk_bear_dmg_claw2 = {"sk_bear_dmg_claw2","0"};
cvar_t	sk_bear_dmg_claw3 = {"sk_bear_dmg_claw3","0"};

cvar_t	sk_bear_dmg_pounce1 = {"sk_bear_dmg_pounce1","0"};
cvar_t	sk_bear_dmg_pounce2 = {"sk_bear_dmg_pounce2","0"};
cvar_t	sk_bear_dmg_pounce3 = {"sk_bear_dmg_pounce3","0"};

// Chicken
cvar_t	sk_chicken_health1 = {"sk_chicken_health1","0"};
cvar_t	sk_chicken_health2 = {"sk_chicken_health2","0"};
cvar_t	sk_chicken_health3 = {"sk_chicken_health3","0"};

cvar_t	sk_chicken_dmg_peck1 = {"sk_chicken_dmg_peck1","0"};
cvar_t	sk_chicken_dmg_peck2 = {"sk_chicken_dmg_peck2","0"};
cvar_t	sk_chicken_dmg_peck3 = {"sk_chicken_dmg_peck3","0"};

// Horse
cvar_t	sk_horse_health1 = {"sk_horse_health1","0"};
cvar_t	sk_horse_health2 = {"sk_horse_health2","0"};
cvar_t	sk_horse_health3 = {"sk_horse_health3","0"};

cvar_t	sk_horse_dmg_kick1 = {"sk_horse_dmg_kick1","0"};
cvar_t	sk_horse_dmg_kick2 = {"sk_horse_dmg_kick2","0"};
cvar_t	sk_horse_dmg_kick3 = {"sk_horse_dmg_kick3","0"};

// Player weapons
cvar_t	sk_plr_knife1 = {"sk_plr_knife1","0"};
cvar_t	sk_plr_knife2 = {"sk_plr_knife2","0"};
cvar_t	sk_plr_knife3 = {"sk_plr_knife3","0"};

cvar_t	sk_plr_pick1 = {"sk_plr_pick1","0"};
cvar_t	sk_plr_pick2 = {"sk_plr_pick2","0"};
cvar_t	sk_plr_pick3 = {"sk_plr_pick3","0"};

cvar_t	sk_plr_pistol_bullet1 = {"sk_plr_pistol_bullet1","0"};
cvar_t	sk_plr_pistol_bullet2 = {"sk_plr_pistol_bullet2","0"};
cvar_t	sk_plr_pistol_bullet3 = {"sk_plr_pistol_bullet3","0"};

cvar_t	sk_plr_gattlinggun_bullet1 = {"sk_plr_gattlinggun_bullet1","0"};
cvar_t	sk_plr_gattlinggun_bullet2 = {"sk_plr_gattlinggun_bullet2","0"};
cvar_t	sk_plr_gattlinggun_bullet3 = {"sk_plr_gattlinggun_bullet3","0"};

cvar_t	sk_plr_shotgun1 = {"sk_plr_shotgun1","0"};
cvar_t	sk_plr_shotgun2 = {"sk_plr_shotgun2","0"};
cvar_t	sk_plr_shotgun3 = {"sk_plr_shotgun3","0"};

cvar_t	sk_plr_buffalo1 = {"sk_plr_buffalo1","0"};
cvar_t	sk_plr_buffalo2 = {"sk_plr_buffalo2","0"};
cvar_t	sk_plr_buffalo3 = {"sk_plr_buffalo3","0"};

cvar_t	sk_plr_colts_bullet1 = {"sk_plr_colts_bullet1","0"};
cvar_t	sk_plr_colts_bullet2 = {"sk_plr_colts_bullet2","0"};
cvar_t	sk_plr_colts_bullet3 = {"sk_plr_colts_bullet3","0"};

cvar_t	sk_plr_bow_arrow1 = {"sk_plr_bow_arrow1","0"};
cvar_t	sk_plr_bow_arrow2 = {"sk_plr_bow_arrow2","0"};
cvar_t	sk_plr_bow_arrow3 = {"sk_plr_bow_arrow3","0"};

cvar_t	sk_plr_cannon_ball1 = {"sk_plr_cannon_ball1","0"};
cvar_t	sk_plr_cannon_ball2 = {"sk_plr_cannon_ball2","0"};
cvar_t	sk_plr_cannon_ball3 = {"sk_plr_cannon_ball3","0"};

cvar_t	sk_plr_beartrap1 = {"sk_plr_beartrap1","0"};
cvar_t	sk_plr_beartrap2 = {"sk_plr_beartrap2","0"};
cvar_t	sk_plr_beartrap3 = {"sk_plr_beartrap3","0"};

cvar_t	sk_plr_rpg1 = {"sk_plr_rpg1","0"};
cvar_t	sk_plr_rpg2 = {"sk_plr_rpg2","0"};
cvar_t	sk_plr_rpg3 = {"sk_plr_rpg3","0"};

cvar_t	sk_plr_dynamite1 = {"sk_plr_dynamite1","0"};
cvar_t	sk_plr_dynamite2 = {"sk_plr_dynamite2","0"};
cvar_t	sk_plr_dynamite3 = {"sk_plr_dynamite3","0"};

cvar_t	sk_plr_winchester_bullet1 = {"sk_plr_winchester_bullet1","0"};
cvar_t	sk_plr_winchester_bullet2 = {"sk_plr_winchester_bullet2","0"};
cvar_t	sk_plr_winchester_bullet3 = {"sk_plr_winchester_bullet3","0"};

// Monster weapons
cvar_t	sk_pistol_bullet1 = {"sk_pistol_bullet1","0"};
cvar_t	sk_pistol_bullet2 = {"sk_pistol_bullet2","0"};
cvar_t	sk_pistol_bullet3 = {"sk_pistol_bullet3","0"};

cvar_t	sk_shotgun_bullet1 = {"sk_shotgun_bullet1","0"};
cvar_t	sk_shotgun_bullet2 = {"sk_shotgun_bullet2","0"};
cvar_t	sk_shotgun_bullet3 = {"sk_shotgun_bullet3","0"};

cvar_t	sk_gattlinggun_bullet1 = {"sk_gattlinggun_bullet1","0"};
cvar_t	sk_gattlinggun_bullet2 = {"sk_gattlinggun_bullet2","0"};
cvar_t	sk_gattlinggun_bullet3 = {"sk_gattlinggun_bullet3","0"};

cvar_t	sk_winchester_bullet1 = {"sk_winchester_bullet1","0"};
cvar_t	sk_winchester_bullet2 = {"sk_winchester_bullet2","0"};
cvar_t	sk_winchester_bullet3 = {"sk_winchester_bullet3","0"};

cvar_t	sk_bow_arrow1 = {"sk_bow_arrow1","0"};
cvar_t	sk_bow_arrow2 = {"sk_bow_arrow2","0"};
cvar_t	sk_bow_arrow3 = {"sk_bow_arrow3","0"};

// Health/suit charge
cvar_t	sk_suitcharger1 = {"sk_suitcharger1","0"};
cvar_t	sk_suitcharger2 = {"sk_suitcharger2","0"};
cvar_t	sk_suitcharger3 = {"sk_suitcharger3","0"};

cvar_t	sk_battery1 = {"sk_battery1","0"};
cvar_t	sk_battery2 = {"sk_battery2","0"};
cvar_t	sk_battery3 = {"sk_battery3","0"};

cvar_t	sk_healthcharger1 = {"sk_healthcharger1","0"};
cvar_t	sk_healthcharger2 = {"sk_healthcharger2","0"};
cvar_t	sk_healthcharger3 = {"sk_healthcharger3","0"};

cvar_t	sk_herbs1 = {"sk_herbs1","0"};
cvar_t	sk_herbs2 = {"sk_herbs2","0"};
cvar_t	sk_herbs3 = {"sk_herbs3","0"};

cvar_t	sk_elixer1 = {"sk_elixer1","0"};
cvar_t	sk_elixer2 = {"sk_elixer2","0"};
cvar_t	sk_elixer3 = {"sk_elixer3","0"};

cvar_t	sk_healthkit1 = {"sk_healthkit1","0"};
cvar_t	sk_healthkit2 = {"sk_healthkit2","0"};
cvar_t	sk_healthkit3 = {"sk_healthkit3","0"};

// Healing by the townsfolk
cvar_t	sk_colonel_heal1 = {"sk_colonel_heal1","0"};
cvar_t	sk_colonel_heal2 = {"sk_colonel_heal2","0"};
cvar_t	sk_colonel_heal3 = {"sk_colonel_heal3","0"};

cvar_t	sk_wtowna_heal1 = {"sk_wtowna_heal1","0"};
cvar_t	sk_wtowna_heal2 = {"sk_wtowna_heal2","0"};
cvar_t	sk_wtowna_heal3 = {"sk_wtowna_heal3","0"};

cvar_t	sk_wtownb_heal1 = {"sk_wtownb_heal1","0"};
cvar_t	sk_wtownb_heal2 = {"sk_wtownb_heal2","0"};
cvar_t	sk_wtownb_heal3 = {"sk_wtownb_heal3","0"};

cvar_t	sk_townmex_heal1 = {"sk_townmex_heal1","0"};
cvar_t	sk_townmex_heal2 = {"sk_townmex_heal2","0"};
cvar_t	sk_townmex_heal3 = {"sk_townmex_heal3","0"};

cvar_t	sk_masala_heal1 = {"sk_masala_heal1","0"};
cvar_t	sk_masala_heal2 = {"sk_masala_heal2","0"};
cvar_t	sk_masala_heal3 = {"sk_masala_heal3","0"};

cvar_t	sk_nagatow_heal1 = {"sk_nagatow_heal1","0"};
cvar_t	sk_nagatow_heal2 = {"sk_nagatow_heal2","0"};
cvar_t	sk_nagatow_heal3 = {"sk_nagatow_heal3","0"};

cvar_t	sk_crispen_heal1 = {"sk_crispen_heal1","0"};
cvar_t	sk_crispen_heal2 = {"sk_crispen_heal2","0"};
cvar_t	sk_crispen_heal3 = {"sk_crispen_heal3","0"};

cvar_t	sk_dave_heal1 = {"sk_dave_heal1","0"};
cvar_t	sk_dave_heal2 = {"sk_dave_heal2","0"};
cvar_t	sk_dave_heal3 = {"sk_dave_heal3","0"};

// Monster damage adjusters
cvar_t	sk_monster_head1 = {"sk_monster_head1","2"};
cvar_t	sk_monster_head2 = {"sk_monster_head2","2"};
cvar_t	sk_monster_head3 = {"sk_monster_head3","2"};

cvar_t	sk_monster_chest1 = {"sk_monster_chest1","1"};
cvar_t	sk_monster_chest2 = {"sk_monster_chest2","1"};
cvar_t	sk_monster_chest3 = {"sk_monster_chest3","1"};

cvar_t	sk_monster_stomach1 = {"sk_monster_stomach1","1"};
cvar_t	sk_monster_stomach2 = {"sk_monster_stomach2","1"};
cvar_t	sk_monster_stomach3 = {"sk_monster_stomach3","1"};

cvar_t	sk_monster_arm1 = {"sk_monster_arm1","1"};
cvar_t	sk_monster_arm2 = {"sk_monster_arm2","1"};
cvar_t	sk_monster_arm3 = {"sk_monster_arm3","1"};

cvar_t	sk_monster_leg1 = {"sk_monster_leg1","1"};
cvar_t	sk_monster_leg2 = {"sk_monster_leg2","1"};
cvar_t	sk_monster_leg3 = {"sk_monster_leg3","1"};

// Player damage adjusters
cvar_t	sk_player_head1 = {"sk_player_head1","2"};
cvar_t	sk_player_head2 = {"sk_player_head2","2"};
cvar_t	sk_player_head3 = {"sk_player_head3","2"};

cvar_t	sk_player_chest1 = {"sk_player_chest1","1"};
cvar_t	sk_player_chest2 = {"sk_player_chest2","1"};
cvar_t	sk_player_chest3 = {"sk_player_chest3","1"};

cvar_t	sk_player_stomach1 = {"sk_player_stomach1","1"};
cvar_t	sk_player_stomach2 = {"sk_player_stomach2","1"};
cvar_t	sk_player_stomach3 = {"sk_player_stomach3","1"};

cvar_t	sk_player_arm1 = {"sk_player_arm1","1"};
cvar_t	sk_player_arm2 = {"sk_player_arm2","1"};
cvar_t	sk_player_arm3 = {"sk_player_arm3","1"};

cvar_t	sk_player_leg1 = {"sk_player_leg1","1"};
cvar_t	sk_player_leg2 = {"sk_player_leg2","1"};
cvar_t	sk_player_leg3 = {"sk_player_leg3","1"};

// END Cvars for Skill Level settings

// Register your console variables here
// This gets called one time when the game is initialied
void GameDLLInit( void )
{
	// Register cvars here:
	g_psv_gravity = CVAR_GET_POINTER( "sv_gravity" );
	g_psv_aim = CVAR_GET_POINTER( "sv_aim" );
	g_footsteps = CVAR_GET_POINTER( "mp_footsteps" );

	CVAR_REGISTER (&displaysoundlist);
	CVAR_REGISTER (&mapcyclefile);
	CVAR_REGISTER (&servercfgfile);
	CVAR_REGISTER (&lservercfgfile);

	CVAR_REGISTER (&teamplay);
	CVAR_REGISTER (&fraglimit);
	CVAR_REGISTER (&timelimit);

	CVAR_REGISTER (&fragsleft);
	CVAR_REGISTER (&timeleft);

	CVAR_REGISTER (&friendlyfire);
	CVAR_REGISTER (&falldamage);
	CVAR_REGISTER (&weaponstay);
	CVAR_REGISTER (&forcerespawn);
	CVAR_REGISTER (&flashlight);
	CVAR_REGISTER (&aimcrosshair);
	CVAR_REGISTER (&decalfrequency);
	CVAR_REGISTER (&teamlist);
	CVAR_REGISTER (&teamoverride);
	CVAR_REGISTER (&defaultteam);
	CVAR_REGISTER (&allowmonsters);

// REGISTER CVARS FOR SKILL LEVEL STUFF
	// Hoss
	CVAR_REGISTER ( &sk_hoss_health1 );
	CVAR_REGISTER ( &sk_hoss_health2 );
	CVAR_REGISTER ( &sk_hoss_health3 );

	// Annie
	CVAR_REGISTER ( &sk_annie_health1 );
	CVAR_REGISTER ( &sk_annie_health2 );
	CVAR_REGISTER ( &sk_annie_health3 );

	// Big miner
	CVAR_REGISTER ( &sk_bigminer_health1 );
	CVAR_REGISTER ( &sk_bigminer_health2 );
	CVAR_REGISTER ( &sk_bigminer_health3 );

	CVAR_REGISTER ( &sk_bigminer_punch1 );
	CVAR_REGISTER ( &sk_bigminer_punch2 );
	CVAR_REGISTER ( &sk_bigminer_punch3 );

	// Small miner
	CVAR_REGISTER ( &sk_smallminer_health1 );
	CVAR_REGISTER ( &sk_smallminer_health2 );
	CVAR_REGISTER ( &sk_smallminer_health3 );

	CVAR_REGISTER ( &sk_smallminer_pick1 );
	CVAR_REGISTER ( &sk_smallminer_pick2 );
	CVAR_REGISTER ( &sk_smallminer_pick3 );

	// Cowboy
	CVAR_REGISTER ( &sk_cowboy_health1 );
	CVAR_REGISTER ( &sk_cowboy_health2 );
	CVAR_REGISTER ( &sk_cowboy_health3 );

	CVAR_REGISTER ( &sk_cowboy_punch1 );
	CVAR_REGISTER ( &sk_cowboy_punch2 );
	CVAR_REGISTER ( &sk_cowboy_punch3 );

	CVAR_REGISTER ( &sk_cowboy_dynamitespeed1 );
	CVAR_REGISTER ( &sk_cowboy_dynamitespeed2 );
	CVAR_REGISTER ( &sk_cowboy_dynamitespeed3 );

	// Ramone
	CVAR_REGISTER ( &sk_ramone_health1 );
	CVAR_REGISTER ( &sk_ramone_health2 );
	CVAR_REGISTER ( &sk_ramone_health3 );

	CVAR_REGISTER ( &sk_ramone_punch1 );
	CVAR_REGISTER ( &sk_ramone_punch2 );
	CVAR_REGISTER ( &sk_ramone_punch3 );

	CVAR_REGISTER ( &sk_ramone_dynamitespeed1 );
	CVAR_REGISTER ( &sk_ramone_dynamitespeed2 );
	CVAR_REGISTER ( &sk_ramone_dynamitespeed3 );

	// Mexican bandit
	CVAR_REGISTER ( &sk_mexbandit_health1 );
	CVAR_REGISTER ( &sk_mexbandit_health2 );
	CVAR_REGISTER ( &sk_mexbandit_health3 );

	CVAR_REGISTER ( &sk_mexbandit_kick1 );
	CVAR_REGISTER ( &sk_mexbandit_kick2 );
	CVAR_REGISTER ( &sk_mexbandit_kick3 );

	CVAR_REGISTER ( &sk_mexbandit_dynamitespeed1 );
	CVAR_REGISTER ( &sk_mexbandit_dynamitespeed2 );
	CVAR_REGISTER ( &sk_mexbandit_dynamitespeed3 );

	// Leech
	CVAR_REGISTER ( &sk_leech_health1 );
	CVAR_REGISTER ( &sk_leech_health2 );
	CVAR_REGISTER ( &sk_leech_health3 );

	CVAR_REGISTER ( &sk_leech_dmg_bite1 );
	CVAR_REGISTER ( &sk_leech_dmg_bite2 );
	CVAR_REGISTER ( &sk_leech_dmg_bite3 );

	// Kaiewi (Indian)
	CVAR_REGISTER ( &sk_kaiewi_health1 );
	CVAR_REGISTER ( &sk_kaiewi_health2 );
	CVAR_REGISTER ( &sk_kaiewi_health3 );

	CVAR_REGISTER ( &sk_kaiewi_punch1 );
	CVAR_REGISTER ( &sk_kaiewi_punch2 );
	CVAR_REGISTER ( &sk_kaiewi_punch3 );

	// Colonel
	CVAR_REGISTER ( &sk_colonel_health1 );
	CVAR_REGISTER ( &sk_colonel_health2 );
	CVAR_REGISTER ( &sk_colonel_health3 );

	// Townsman A
	CVAR_REGISTER ( &sk_wtowna_health1 );
	CVAR_REGISTER ( &sk_wtowna_health2 );
	CVAR_REGISTER ( &sk_wtowna_health3 );

	// Townsman B
	CVAR_REGISTER ( &sk_wtownb_health1 );
	CVAR_REGISTER ( &sk_wtownb_health2 );
	CVAR_REGISTER ( &sk_wtownb_health3 );

	// Mexican townsman
	CVAR_REGISTER ( &sk_townmex_health1 );
	CVAR_REGISTER ( &sk_townmex_health2 );
	CVAR_REGISTER ( &sk_townmex_health3 );

	// Masala
	CVAR_REGISTER ( &sk_masala_health1 );
	CVAR_REGISTER ( &sk_masala_health2 );
	CVAR_REGISTER ( &sk_masala_health3 );

	// Nagatow
	CVAR_REGISTER ( &sk_nagatow_health1 );
	CVAR_REGISTER ( &sk_nagatow_health2 );
	CVAR_REGISTER ( &sk_nagatow_health3 );

	// Crispen
	CVAR_REGISTER ( &sk_crispen_health1 );
	CVAR_REGISTER ( &sk_crispen_health2 );
	CVAR_REGISTER ( &sk_crispen_health3 );

	// Dave
	CVAR_REGISTER ( &sk_dave_health1 );
	CVAR_REGISTER ( &sk_dave_health2 );
	CVAR_REGISTER ( &sk_dave_health3 );

	// Scorpion
	CVAR_REGISTER ( &sk_scorpion_health1 );
	CVAR_REGISTER ( &sk_scorpion_health2 );
	CVAR_REGISTER ( &sk_scorpion_health3 );

	CVAR_REGISTER ( &sk_scorpion_dmg_sting1 );
	CVAR_REGISTER ( &sk_scorpion_dmg_sting2 );
	CVAR_REGISTER ( &sk_scorpion_dmg_sting3 );

	// Snake
	CVAR_REGISTER ( &sk_snake_health1 );
	CVAR_REGISTER ( &sk_snake_health2 );
	CVAR_REGISTER ( &sk_snake_health3 );

	CVAR_REGISTER ( &sk_snake_dmg_bite1 );
	CVAR_REGISTER ( &sk_snake_dmg_bite2 );
	CVAR_REGISTER ( &sk_snake_dmg_bite3 );

	// Puma
	CVAR_REGISTER ( &sk_puma_health1 );
	CVAR_REGISTER ( &sk_puma_health2 );
	CVAR_REGISTER ( &sk_puma_health3 );

	CVAR_REGISTER ( &sk_puma_dmg_claw1 );
	CVAR_REGISTER ( &sk_puma_dmg_claw2 );
	CVAR_REGISTER ( &sk_puma_dmg_claw3 );

	CVAR_REGISTER ( &sk_puma_dmg_pounce1 );
	CVAR_REGISTER ( &sk_puma_dmg_pounce2 );
	CVAR_REGISTER ( &sk_puma_dmg_pounce3 );

	// Bear
	CVAR_REGISTER ( &sk_bear_health1 );
	CVAR_REGISTER ( &sk_bear_health2 );
	CVAR_REGISTER ( &sk_bear_health3 );

	CVAR_REGISTER ( &sk_bear_dmg_claw1 );
	CVAR_REGISTER ( &sk_bear_dmg_claw2 );
	CVAR_REGISTER ( &sk_bear_dmg_claw3 );

	CVAR_REGISTER ( &sk_bear_dmg_pounce1 );
	CVAR_REGISTER ( &sk_bear_dmg_pounce2 );
	CVAR_REGISTER ( &sk_bear_dmg_pounce3 );

	// Chicken
	CVAR_REGISTER ( &sk_chicken_health1 );
	CVAR_REGISTER ( &sk_chicken_health2 );
	CVAR_REGISTER ( &sk_chicken_health3 );

	CVAR_REGISTER ( &sk_chicken_dmg_peck1 );
	CVAR_REGISTER ( &sk_chicken_dmg_peck2 );
	CVAR_REGISTER ( &sk_chicken_dmg_peck3 );

	// Horse
	CVAR_REGISTER ( &sk_horse_health1 );
	CVAR_REGISTER ( &sk_horse_health2 );
	CVAR_REGISTER ( &sk_horse_health3 );

	CVAR_REGISTER ( &sk_horse_dmg_kick1 );
	CVAR_REGISTER ( &sk_horse_dmg_kick2 );
	CVAR_REGISTER ( &sk_horse_dmg_kick3 );

	// Player weapons
	CVAR_REGISTER ( &sk_plr_knife1 );
	CVAR_REGISTER ( &sk_plr_knife2 );
	CVAR_REGISTER ( &sk_plr_knife3 );

	CVAR_REGISTER ( &sk_plr_pick1 );
	CVAR_REGISTER ( &sk_plr_pick2 );
	CVAR_REGISTER ( &sk_plr_pick3 );

	CVAR_REGISTER ( &sk_plr_pistol_bullet1 );
	CVAR_REGISTER ( &sk_plr_pistol_bullet2 );
	CVAR_REGISTER ( &sk_plr_pistol_bullet3 );

	CVAR_REGISTER ( &sk_plr_gattlinggun_bullet1 );
	CVAR_REGISTER ( &sk_plr_gattlinggun_bullet2 );
	CVAR_REGISTER ( &sk_plr_gattlinggun_bullet3 );

	CVAR_REGISTER ( &sk_plr_shotgun1 );
	CVAR_REGISTER ( &sk_plr_shotgun2 );
	CVAR_REGISTER ( &sk_plr_shotgun3 );

	CVAR_REGISTER ( &sk_plr_buffalo1 );
	CVAR_REGISTER ( &sk_plr_buffalo2 );
	CVAR_REGISTER ( &sk_plr_buffalo3 );

	CVAR_REGISTER ( &sk_plr_colts_bullet1 );
	CVAR_REGISTER ( &sk_plr_colts_bullet2 );
	CVAR_REGISTER ( &sk_plr_colts_bullet3 );

	CVAR_REGISTER ( &sk_plr_bow_arrow1 );
	CVAR_REGISTER ( &sk_plr_bow_arrow2 );
	CVAR_REGISTER ( &sk_plr_bow_arrow3 );

	CVAR_REGISTER ( &sk_plr_cannon_ball1 );
	CVAR_REGISTER ( &sk_plr_cannon_ball2 );
	CVAR_REGISTER ( &sk_plr_cannon_ball3 );

	CVAR_REGISTER ( &sk_plr_beartrap1 );
	CVAR_REGISTER ( &sk_plr_beartrap2 );
	CVAR_REGISTER ( &sk_plr_beartrap3 );

	CVAR_REGISTER ( &sk_plr_dynamite1 );
	CVAR_REGISTER ( &sk_plr_dynamite2 );
	CVAR_REGISTER ( &sk_plr_dynamite3 );

	CVAR_REGISTER ( &sk_plr_winchester_bullet1 );
	CVAR_REGISTER ( &sk_plr_winchester_bullet2 );
	CVAR_REGISTER ( &sk_plr_winchester_bullet3 );

	CVAR_REGISTER ( &sk_plr_rpg1 );
	CVAR_REGISTER ( &sk_plr_rpg2 );
	CVAR_REGISTER ( &sk_plr_rpg3 );

	// Monster weapons
	CVAR_REGISTER ( &sk_pistol_bullet1 );
	CVAR_REGISTER ( &sk_pistol_bullet2 );
	CVAR_REGISTER ( &sk_pistol_bullet3 );

	CVAR_REGISTER ( &sk_shotgun_bullet1 );
	CVAR_REGISTER ( &sk_shotgun_bullet2 );
	CVAR_REGISTER ( &sk_shotgun_bullet3 );

	CVAR_REGISTER ( &sk_gattlinggun_bullet1 );
	CVAR_REGISTER ( &sk_gattlinggun_bullet2 );
	CVAR_REGISTER ( &sk_gattlinggun_bullet3 );

	CVAR_REGISTER ( &sk_winchester_bullet1 );
	CVAR_REGISTER ( &sk_winchester_bullet2 );
	CVAR_REGISTER ( &sk_winchester_bullet3 );

	CVAR_REGISTER ( &sk_bow_arrow1 );
	CVAR_REGISTER ( &sk_bow_arrow2 );
	CVAR_REGISTER ( &sk_bow_arrow3 );

	// Health/suit charge
	CVAR_REGISTER ( &sk_suitcharger1 );
	CVAR_REGISTER ( &sk_suitcharger2 );
	CVAR_REGISTER ( &sk_suitcharger3 );

	CVAR_REGISTER ( &sk_battery1 );
	CVAR_REGISTER ( &sk_battery2 );
	CVAR_REGISTER ( &sk_battery3 );

	CVAR_REGISTER ( &sk_healthcharger1 );
	CVAR_REGISTER ( &sk_healthcharger2 );
	CVAR_REGISTER ( &sk_healthcharger3 );

	CVAR_REGISTER ( &sk_healthkit1 );
	CVAR_REGISTER ( &sk_healthkit2 );
	CVAR_REGISTER ( &sk_healthkit3 );

	CVAR_REGISTER ( &sk_herbs1 );
	CVAR_REGISTER ( &sk_herbs2 );
	CVAR_REGISTER ( &sk_herbs3 );

	CVAR_REGISTER ( &sk_elixer1 );
	CVAR_REGISTER ( &sk_elixer2 );
	CVAR_REGISTER ( &sk_elixer3 );

	// Healing by the townsfolk
	CVAR_REGISTER ( &sk_colonel_heal1 );
	CVAR_REGISTER ( &sk_colonel_heal2 );
	CVAR_REGISTER ( &sk_colonel_heal3 );

	CVAR_REGISTER ( &sk_wtowna_heal1 );
	CVAR_REGISTER ( &sk_wtowna_heal2 );
	CVAR_REGISTER ( &sk_wtowna_heal3 );

	CVAR_REGISTER ( &sk_wtownb_heal1 );
	CVAR_REGISTER ( &sk_wtownb_heal2 );
	CVAR_REGISTER ( &sk_wtownb_heal3 );

	CVAR_REGISTER ( &sk_townmex_heal1 );
	CVAR_REGISTER ( &sk_townmex_heal2 );
	CVAR_REGISTER ( &sk_townmex_heal3 );

	CVAR_REGISTER ( &sk_masala_heal1 );
	CVAR_REGISTER ( &sk_masala_heal2 );
	CVAR_REGISTER ( &sk_masala_heal3 );

	CVAR_REGISTER ( &sk_nagatow_heal1 );
	CVAR_REGISTER ( &sk_nagatow_heal2 );
	CVAR_REGISTER ( &sk_nagatow_heal3 );

	CVAR_REGISTER ( &sk_crispen_heal1 );
	CVAR_REGISTER ( &sk_crispen_heal2 );
	CVAR_REGISTER ( &sk_crispen_heal3 );

	CVAR_REGISTER ( &sk_dave_heal1 );
	CVAR_REGISTER ( &sk_dave_heal2 );
	CVAR_REGISTER ( &sk_dave_heal3 );

	// Monster damage adjusters
	CVAR_REGISTER ( &sk_monster_head1 );
	CVAR_REGISTER ( &sk_monster_head2 );
	CVAR_REGISTER ( &sk_monster_head3 );

	CVAR_REGISTER ( &sk_monster_chest1 );
	CVAR_REGISTER ( &sk_monster_chest2 );
	CVAR_REGISTER ( &sk_monster_chest3 );

	CVAR_REGISTER ( &sk_monster_stomach1 );
	CVAR_REGISTER ( &sk_monster_stomach2 );
	CVAR_REGISTER ( &sk_monster_stomach3 );

	CVAR_REGISTER ( &sk_monster_arm1 );
	CVAR_REGISTER ( &sk_monster_arm2 );
	CVAR_REGISTER ( &sk_monster_arm3 );

	CVAR_REGISTER ( &sk_monster_leg1 );
	CVAR_REGISTER ( &sk_monster_leg2 );
	CVAR_REGISTER ( &sk_monster_leg3 );

	// Player damage adjusters
	CVAR_REGISTER ( &sk_player_head1 );
	CVAR_REGISTER ( &sk_player_head2 );
	CVAR_REGISTER ( &sk_player_head3 );

	CVAR_REGISTER ( &sk_player_chest1 );
	CVAR_REGISTER ( &sk_player_chest2 );
	CVAR_REGISTER ( &sk_player_chest3 );

	CVAR_REGISTER ( &sk_player_stomach1 );
	CVAR_REGISTER ( &sk_player_stomach2 );
	CVAR_REGISTER ( &sk_player_stomach3 );

	CVAR_REGISTER ( &sk_player_arm1 );
	CVAR_REGISTER ( &sk_player_arm2 );
	CVAR_REGISTER ( &sk_player_arm3 );

	CVAR_REGISTER ( &sk_player_leg1 );
	CVAR_REGISTER ( &sk_player_leg2 );
	CVAR_REGISTER ( &sk_player_leg3 );
// END REGISTER CVARS FOR SKILL LEVEL STUFF

	// Cow-Bots
	CVAR_REGISTER ( &sv_defaultbots );
	CVAR_REGISTER ( &sv_adjbotskill );
	CVAR_REGISTER ( &sv_botskill );

	SERVER_COMMAND( "exec skill.cfg\n" );
}


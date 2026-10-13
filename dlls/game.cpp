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
cvar_t	teamplay	= {"mp_teamplay","0" };
cvar_t	fraglimit	= {"mp_fraglimit","0", FCVAR_SERVER };
cvar_t	timelimit	= {"mp_timelimit","0", FCVAR_SERVER };
cvar_t	friendlyfire= {"mp_friendlyfire","0" };
cvar_t	falldamage	= {"mp_falldamage","0", FCVAR_SERVER };
cvar_t	weaponstay	= {"mp_weaponstay","0", FCVAR_SERVER };
cvar_t	forcerespawn= {"mp_forcerespawn","1", FCVAR_SERVER };
cvar_t	flashlight	= {"mp_flashlight","0", FCVAR_SERVER };
cvar_t	aimcrosshair= {"mp_autocrosshair","1", FCVAR_SERVER };
cvar_t	decalfrequency = {"decalfrequency","30", FCVAR_SERVER };
cvar_t	teamlist = {"mp_teamlist","hgrunt;scientist" };
cvar_t	teamoverride = {"mp_teamoverride","1" };
cvar_t	defaultteam = {"mp_defaultteam","0" };
cvar_t	allowmonsters={"mp_allowmonsters","0", FCVAR_SERVER };

cvar_t 	*g_psv_gravity = NULL;
cvar_t	*g_psv_aim = NULL;
cvar_t	*g_footsteps = NULL;

//CVARS FOR SKILL LEVEL SETTINGS
cvar_t	sk_percent_random_h1 = {"sk_percent_random_h1","0"};
cvar_t	sk_percent_random_h2 = {"sk_percent_random_h2","0"};
cvar_t	sk_percent_random_h3 = {"sk_percent_random_h3","0"};

cvar_t	sk_human_kick_d1 = {"sk_human_kick_d1","0"};
cvar_t	sk_human_kick_d2 = {"sk_human_kick_d2","0"};
cvar_t	sk_human_kick_d3 = {"sk_human_kick_d3","0"};

cvar_t	sk_gren_speed1 = {"sk_gren_speed1","0"};
cvar_t	sk_gren_speed2 = {"sk_gren_speed2","0"};
cvar_t	sk_gren_speed3 = {"sk_gren_speed3","0"};

cvar_t	sk_bandit_h1 = {"sk_bandit_h1","0"};
cvar_t	sk_bandit_h2 = {"sk_bandit_h2","0"};
cvar_t	sk_bandit_h3 = {"sk_bandit_h3","0"};

cvar_t	sk_bandit_limp_h1 = {"sk_bandit_limp_h1","0"};
cvar_t	sk_bandit_limp_h2 = {"sk_bandit_limp_h2","0"};
cvar_t	sk_bandit_limp_h3 = {"sk_bandit_limp_h3","0"};

cvar_t	sk_bandit_melee_d1 = {"sk_bandit_melee_d1","0"};
cvar_t	sk_bandit_melee_d2 = {"sk_bandit_melee_d2","0"};
cvar_t	sk_bandit_melee_d3 = {"sk_bandit_melee_d3","0"};

cvar_t	sk_bandit_bolt_d1 = {"sk_bandit_bolt_d1","0"};
cvar_t	sk_bandit_bolt_d2 = {"sk_bandit_bolt_d2","0"};
cvar_t	sk_bandit_bolt_d3 = {"sk_bandit_bolt_d3","0"};

cvar_t	sk_bandit_fast_d1 = {"sk_bandit_fast_d1","0"};
cvar_t	sk_bandit_fast_d2 = {"sk_bandit_fast_d2","0"};
cvar_t	sk_bandit_fast_d3 = {"sk_bandit_fast_d3","0"};

cvar_t	sk_bandit_mech_d1 = {"sk_bandit_mech_d1","0"};
cvar_t	sk_bandit_mech_d2 = {"sk_bandit_mech_d2","0"};
cvar_t	sk_bandit_mech_d3 = {"sk_bandit_mech_d3","0"};

cvar_t	sk_demoman_h1 = {"sk_demoman_h1","0"};
cvar_t	sk_demoman_h2 = {"sk_demoman_h2","0"};
cvar_t	sk_demoman_h3 = {"sk_demoman_h3","0"};

cvar_t	sk_demoman_limp_h1 = {"sk_demoman_limp_h1","0"};
cvar_t	sk_demoman_limp_h2 = {"sk_demoman_limp_h2","0"};
cvar_t	sk_demoman_limp_h3 = {"sk_demoman_limp_h3","0"};

cvar_t	sk_demoman_rocket_d1 = {"sk_demoman_rocket_d1","0"};
cvar_t	sk_demoman_rocket_d2 = {"sk_demoman_rocket_d2","0"};
cvar_t	sk_demoman_rocket_d3 = {"sk_demoman_rocket_d3","0"};

cvar_t	sk_demoman_sg_d1 = {"sk_demoman_sg_d1","0"};
cvar_t	sk_demoman_sg_d2 = {"sk_demoman_sg_d2","0"};
cvar_t	sk_demoman_sg_d3 = {"sk_demoman_sg_d3","0"};

cvar_t	sk_gator_h1 = {"sk_gator_h1","0"};
cvar_t	sk_gator_h2 = {"sk_gator_h2","0"};
cvar_t	sk_gator_h3 = {"sk_gator_h3","0"};

cvar_t	sk_gator_bite_d1 = {"sk_gator_bite_d1","0"};
cvar_t	sk_gator_bite_d2 = {"sk_gator_bite_d2","0"};
cvar_t	sk_gator_bite_d3 = {"sk_gator_bite_d3","0"};

cvar_t	sk_ourano_h1 = {"sk_ourano_h1","0"};
cvar_t	sk_ourano_h2 = {"sk_ourano_h2","0"};
cvar_t	sk_ourano_h3 = {"sk_ourano_h3","0"};

cvar_t	sk_ourano_d1 = {"sk_ourano_d1","0"};
cvar_t	sk_ourano_d2 = {"sk_ourano_d2","0"};
cvar_t	sk_ourano_d3 = {"sk_ourano_d3","0"};

cvar_t	sk_raptor_h1 = {"sk_raptor_h1","0"};
cvar_t	sk_raptor_h2 = {"sk_raptor_h2","0"};
cvar_t	sk_raptor_h3 = {"sk_raptor_h3","0"};

cvar_t	sk_raptor_bite_d1 = {"sk_raptor_bite_d1","0"};
cvar_t	sk_raptor_bite_d2 = {"sk_raptor_bite_d2","0"};
cvar_t	sk_raptor_bite_d3 = {"sk_raptor_bite_d3","0"};

cvar_t	sk_raptor_jump_d1 = {"sk_raptor_jump_d1","0"};
cvar_t	sk_raptor_jump_d2 = {"sk_raptor_jump_d2","0"};
cvar_t	sk_raptor_jump_d3 = {"sk_raptor_jump_d3","0"};

cvar_t	sk_smlscorp_h1 = {"sk_smlscorp_h1","0"};
cvar_t	sk_smlscorp_h2 = {"sk_smlscorp_h2","0"};
cvar_t	sk_smlscorp_h3 = {"sk_smlscorp_h3","0"};

cvar_t	sk_smlscorp_claw_d1 = {"sk_smlscorp_claw_d1","0"};
cvar_t	sk_smlscorp_claw_d2 = {"sk_smlscorp_claw_d2","0"};
cvar_t	sk_smlscorp_claw_d3 = {"sk_smlscorp_claw_d3","0"};

cvar_t	sk_smlscorp_sting_d1 = {"sk_smlscorp_sting_d1","0"};
cvar_t	sk_smlscorp_sting_d2 = {"sk_smlscorp_sting_d2","0"};
cvar_t	sk_smlscorp_sting_d3 = {"sk_smlscorp_sting_d3","0"};

cvar_t	sk_lrgscorp_h1 = {"sk_lrgscorp_h1","0"};
cvar_t	sk_lrgscorp_h2 = {"sk_lrgscorp_h2","0"};
cvar_t	sk_lrgscorp_h3 = {"sk_lrgscorp_h3","0"};

cvar_t	sk_lrgscorp_claw_d1 = {"sk_lrgscorp_claw_d1","0"};
cvar_t	sk_lrgscorp_claw_d2 = {"sk_lrgscorp_claw_d2","0"};
cvar_t	sk_lrgscorp_claw_d3 = {"sk_lrgscorp_claw_d3","0"};

cvar_t	sk_lrgscorp_sting_d1 = {"sk_lrgscorp_sting_d1","0"};
cvar_t	sk_lrgscorp_sting_d2 = {"sk_lrgscorp_sting_d2","0"};
cvar_t	sk_lrgscorp_sting_d3 = {"sk_lrgscorp_sting_d3","0"};

cvar_t	sk_rustbit_h1 = {"sk_rustbit_h1","0"};
cvar_t	sk_rustbit_h2 = {"sk_rustbit_h2","0"};
cvar_t	sk_rustbit_h3 = {"sk_rustbit_h3","0"};

cvar_t	sk_rustbit_shot_d1 = {"sk_rustbit_shot_d1","0"};
cvar_t	sk_rustbit_shot_d2 = {"sk_rustbit_shot_d2","0"};
cvar_t	sk_rustbit_shot_d3 = {"sk_rustbit_shot_d3","0"};

cvar_t	sk_rustbit_claw_d1 = {"sk_rustbit_claw_d1","0"};
cvar_t	sk_rustbit_claw_d2 = {"sk_rustbit_claw_d2","0"};
cvar_t	sk_rustbit_claw_d3 = {"sk_rustbit_claw_d3","0"};

cvar_t	sk_rustbot_h1 = {"sk_rustbot_h1","0"};
cvar_t	sk_rustbot_h2 = {"sk_rustbot_h2","0"};
cvar_t	sk_rustbot_h3 = {"sk_rustbot_h3","0"};

cvar_t	sk_rustbot_melee_d1 = {"sk_rustbot_melee_d1","0"};
cvar_t	sk_rustbot_melee_d2 = {"sk_rustbot_melee_d2","0"};
cvar_t	sk_rustbot_melee_d3 = {"sk_rustbot_melee_d3","0"};

cvar_t	sk_rustbot_long_d1 = {"sk_rustbot_long_d1","0"};
cvar_t	sk_rustbot_long_d2 = {"sk_rustbot_long_d2","0"};
cvar_t	sk_rustbot_long_d3 = {"sk_rustbot_long_d3","0"};

cvar_t	sk_rustgunner_h1 = {"sk_rustgunner_h1","0"};
cvar_t	sk_rustgunner_h2 = {"sk_rustgunner_h2","0"};
cvar_t	sk_rustgunner_h3 = {"sk_rustgunner_h3","0"};

cvar_t	sk_rustgunner_r_d1 = {"sk_rustgunner_r_d1","0"};
cvar_t	sk_rustgunner_r_d2 = {"sk_rustgunner_r_d2","0"};
cvar_t	sk_rustgunner_r_d3 = {"sk_rustgunner_r_d3","0"};

cvar_t	sk_rustgunner_mr_d1 = {"sk_rustgunner_mr_d1","0"};
cvar_t	sk_rustgunner_mr_d2 = {"sk_rustgunner_mr_d2","0"};
cvar_t	sk_rustgunner_mr_d3 = {"sk_rustgunner_mr_d3","0"};

cvar_t	sk_scientist_h1 = {"sk_scientist_h1","0"};
cvar_t	sk_scientist_h2 = {"sk_scientist_h2","0"};
cvar_t	sk_scientist_h3 = {"sk_scientist_h3","0"};

cvar_t	sk_tube_h1 = {"sk_tube_h1","0"};
cvar_t	sk_tube_h2 = {"sk_tube_h2","0"};
cvar_t	sk_tube_h3 = {"sk_tube_h3","0"};

cvar_t	sk_beak_h1 = {"sk_beak_h1","0"};
cvar_t	sk_beak_h2 = {"sk_beak_h2","0"};
cvar_t	sk_beak_h3 = {"sk_beak_h3","0"};

cvar_t	sk_beak_bite_d1 = {"sk_beak_bite_d1","0"};
cvar_t	sk_beak_bite_d2 = {"sk_beak_bite_d2","0"};
cvar_t	sk_beak_bite_d3 = {"sk_beak_bite_d3","0"};

cvar_t	sk_beak_scrape_d1 = {"sk_beak_scrape_d1","0"};
cvar_t	sk_beak_scrape_d2 = {"sk_beak_scrape_d2","0"};
cvar_t	sk_beak_scrape_d3 = {"sk_beak_scrape_d3","0"};

cvar_t	sk_critter_h1 = {"sk_critter_h1","0"};
cvar_t	sk_critter_h2 = {"sk_critter_h2","0"};
cvar_t	sk_critter_h3 = {"sk_critter_h3","0"};

cvar_t	sk_critter_spit_d1 = {"sk_critter_spit_d1","0"};
cvar_t	sk_critter_spit_d2 = {"sk_critter_spit_d2","0"};
cvar_t	sk_critter_spit_d3 = {"sk_critter_spit_d3","0"};

cvar_t	sk_critter_melee1_d1 = {"sk_critter_melee1_d1","0"};
cvar_t	sk_critter_melee1_d2 = {"sk_critter_melee1_d2","0"};
cvar_t	sk_critter_melee1_d3 = {"sk_critter_melee1_d3","0"};

cvar_t	sk_maggot_d1 = {"sk_maggot_d1","0"};
cvar_t	sk_maggot_d2 = {"sk_maggot_d2","0"};
cvar_t	sk_maggot_d3 = {"sk_maggot_d3","0"};

cvar_t	sk_maggot_h1 = {"sk_maggot_h1","0"};
cvar_t	sk_maggot_h2 = {"sk_maggot_h2","0"};
cvar_t	sk_maggot_h3 = {"sk_maggot_h3","0"};

cvar_t	sk_hatchetfish_h1 = {"sk_hatchetfish_h1","0"};
cvar_t	sk_hatchetfish_h2 = {"sk_hatchetfish_h2","0"};
cvar_t	sk_hatchetfish_h3 = {"sk_hatchetfish_h3","0"};

cvar_t	sk_hatchetfish_d1 = {"sk_hatchetfish_d1","0"};
cvar_t	sk_hatchetfish_d2 = {"sk_hatchetfish_d2","0"};
cvar_t	sk_hatchetfish_d3 = {"sk_hatchetfish_d3","0"};

cvar_t	sk_chopper_h1 = {"sk_chopper_h1","0"};
cvar_t	sk_chopper_h2 = {"sk_chopper_h2","0"};
cvar_t	sk_chopper_h3 = {"sk_chopper_h3","0"};

cvar_t	sk_xenome_h1 = {"sk_xenome_h1","0"};
cvar_t	sk_xenome_h2 = {"sk_xenome_h2","0"};
cvar_t	sk_xenome_h3 = {"sk_xenome_h3","0"};

cvar_t	sk_xenomemelee_d1 = {"sk_xenomemelee_d1","0"};
cvar_t	sk_xenomemelee_d2 = {"sk_xenomemelee_d2","0"};
cvar_t	sk_xenomemelee_d3 = {"sk_xenomemelee_d3","0"};

cvar_t	sk_xenomedart_d1 = {"sk_xenomedart_d1","0"};
cvar_t	sk_xenomedart_d2 = {"sk_xenomedart_d2","0"};
cvar_t	sk_xenomedart_d3 = {"sk_xenomedart_d3","0"};

cvar_t	sk_9mmAR_bullet1 = {"sk_9mmAR_bullet1","0"};
cvar_t	sk_9mmAR_bullet2 = {"sk_9mmAR_bullet2","0"};
cvar_t	sk_9mmAR_bullet3 = {"sk_9mmAR_bullet3","0"};

cvar_t	sk_buckshot1 = {"sk_buckshot1","0"};
cvar_t	sk_buckshot2 = {"sk_buckshot2","0"};
cvar_t	sk_buckshot3 = {"sk_buckshot3","0"};

cvar_t	sk_gaussshot_d1 = {"sk_gaussshot_d1","0"};
cvar_t	sk_gaussshot_d2 = {"sk_gaussshot_d2","0"};
cvar_t	sk_gaussshot_d3 = {"sk_gaussshot_d3","0"};

cvar_t	sk_gaussfast_d1 = {"sk_gaussfast_d1","0"};
cvar_t	sk_gaussfast_d2 = {"sk_gaussfast_d2","0"};
cvar_t	sk_gaussfast_d3 = {"sk_gaussfast_d3","0"};

cvar_t	sk_gaussslow_d1 = {"sk_gaussslow_d1","0"};
cvar_t	sk_gaussslow_d2 = {"sk_gaussslow_d2","0"};
cvar_t	sk_gaussslow_d3 = {"sk_gaussslow_d3","0"};

cvar_t	sk_chemgun_d1 = {"sk_chemgun_d1","0"};
cvar_t	sk_chemgun_d2 = {"sk_chemgun_d2","0"};
cvar_t	sk_chemgun_d3 = {"sk_chemgun_d3","0"};

cvar_t	sk_onehandpunch_d1 = {"sk_onehandpunch_d1","0"};
cvar_t	sk_onehandpunch_d2 = {"sk_onehandpunch_d2","0"};
cvar_t	sk_onehandpunch_d3 = {"sk_onehandpunch_d3","0"};

cvar_t	sk_twohandpunch_d1 = {"sk_twohandpunch_d1","0"};
cvar_t	sk_twohandpunch_d2 = {"sk_twohandpunch_d2","0"};
cvar_t	sk_twohandpunch_d3 = {"sk_twohandpunch_d3","0"};

cvar_t	sk_knife1_d1 = {"sk_knife1_d1","0"};
cvar_t	sk_knife1_d2 = {"sk_knife1_d2","0"};
cvar_t	sk_knife1_d3 = {"sk_knife1_d3","0"};

cvar_t	sk_hand_grenade_d1 = {"sk_hand_grenade_d1","0"};
cvar_t	sk_hand_grenade_d2 = {"sk_hand_grenade_d2","0"};
cvar_t	sk_hand_grenade_d3 = {"sk_hand_grenade_d3","0"};

cvar_t	sk_mine_d1 = {"sk_mine_d1","0"};
cvar_t	sk_mine_d2 = {"sk_mine_d2","0"};
cvar_t	sk_mine_d3 = {"sk_mine_d3","0"};

cvar_t	sk_dml_rocket_d1 = {"sk_dml_rocket_d1","0"};
cvar_t	sk_dml_rocket_d2 = {"sk_dml_rocket_d2","0"};
cvar_t	sk_dml_rocket_d3 = {"sk_dml_rocket_d3","0"};

cvar_t	sk_suitcharger1 = {"sk_suitcharger1","0"};
cvar_t	sk_suitcharger2 = {"sk_suitcharger2","0"};
cvar_t	sk_suitcharger3 = {"sk_suitcharger3","0"};

cvar_t	sk_battery1 = {"sk_battery1","0"};
cvar_t	sk_battery2 = {"sk_battery2","0"};
cvar_t	sk_battery3 = {"sk_battery3","0"};

cvar_t	sk_armorsuit1 = {"sk_armorsuit1","0"};
cvar_t	sk_armorsuit2 = {"sk_armorsuit2","0"};
cvar_t	sk_armorsuit3 = {"sk_armorsuit3","0"};

cvar_t	sk_healthcharger1 = {"sk_healthcharger1","0"};
cvar_t	sk_healthcharger2 = {"sk_healthcharger2","0"};
cvar_t	sk_healthcharger3 = {"sk_healthcharger3","0"};

cvar_t	sk_healthkit1 = {"sk_healthkit1","0"};
cvar_t	sk_healthkit2 = {"sk_healthkit2","0"};
cvar_t	sk_healthkit3 = {"sk_healthkit3","0"};

cvar_t	sk_scientist_heal1 = {"sk_scientist_heal1","0"};
cvar_t	sk_scientist_heal2 = {"sk_scientist_heal2","0"};
cvar_t	sk_scientist_heal3 = {"sk_scientist_heal3","0"};

// monster damage adjusters
cvar_t	sk_monster_head1 = {"sk_monster_head1","2"};
// monster damage adjusters
cvar_t	sk_monster_head2 = {"sk_monster_head2","2"};
// monster damage adjusters
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

// player damage adjusters
cvar_t	sk_player_head1 = {"sk_player_head1","2"};
// player damage adjusters
cvar_t	sk_player_head2 = {"sk_player_head2","2"};
// player damage adjusters
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
	CVAR_REGISTER ( &sk_percent_random_h1 );
	CVAR_REGISTER ( &sk_percent_random_h2 );
	CVAR_REGISTER ( &sk_percent_random_h3 );

	CVAR_REGISTER ( &sk_human_kick_d1 );
	CVAR_REGISTER ( &sk_human_kick_d2 );
	CVAR_REGISTER ( &sk_human_kick_d3 );

	CVAR_REGISTER ( &sk_gren_speed1 );
	CVAR_REGISTER ( &sk_gren_speed2 );
	CVAR_REGISTER ( &sk_gren_speed3 );

	CVAR_REGISTER ( &sk_bandit_h1 );
	CVAR_REGISTER ( &sk_bandit_h2 );
	CVAR_REGISTER ( &sk_bandit_h3 );

	CVAR_REGISTER ( &sk_bandit_limp_h1 );
	CVAR_REGISTER ( &sk_bandit_limp_h2 );
	CVAR_REGISTER ( &sk_bandit_limp_h3 );

	CVAR_REGISTER ( &sk_bandit_melee_d1 );
	CVAR_REGISTER ( &sk_bandit_melee_d2 );
	CVAR_REGISTER ( &sk_bandit_melee_d3 );

	CVAR_REGISTER ( &sk_bandit_bolt_d1 );
	CVAR_REGISTER ( &sk_bandit_bolt_d2 );
	CVAR_REGISTER ( &sk_bandit_bolt_d3 );

	CVAR_REGISTER ( &sk_bandit_fast_d1 );
	CVAR_REGISTER ( &sk_bandit_fast_d2 );
	CVAR_REGISTER ( &sk_bandit_fast_d3 );

	CVAR_REGISTER ( &sk_bandit_mech_d1 );
	CVAR_REGISTER ( &sk_bandit_mech_d2 );
	CVAR_REGISTER ( &sk_bandit_mech_d3 );

	CVAR_REGISTER ( &sk_demoman_h1 );
	CVAR_REGISTER ( &sk_demoman_h2 );
	CVAR_REGISTER ( &sk_demoman_h3 );

	CVAR_REGISTER ( &sk_demoman_limp_h1 );
	CVAR_REGISTER ( &sk_demoman_limp_h2 );
	CVAR_REGISTER ( &sk_demoman_limp_h3 );

	CVAR_REGISTER ( &sk_demoman_rocket_d1 );
	CVAR_REGISTER ( &sk_demoman_rocket_d2 );
	CVAR_REGISTER ( &sk_demoman_rocket_d3 );

	CVAR_REGISTER ( &sk_demoman_sg_d1 );
	CVAR_REGISTER ( &sk_demoman_sg_d2 );
	CVAR_REGISTER ( &sk_demoman_sg_d3 );

	CVAR_REGISTER ( &sk_gator_h1 );
	CVAR_REGISTER ( &sk_gator_h2 );
	CVAR_REGISTER ( &sk_gator_h3 );

	CVAR_REGISTER ( &sk_gator_bite_d1 );
	CVAR_REGISTER ( &sk_gator_bite_d2 );
	CVAR_REGISTER ( &sk_gator_bite_d3 );

	CVAR_REGISTER ( &sk_ourano_h1 );
	CVAR_REGISTER ( &sk_ourano_h2 );
	CVAR_REGISTER ( &sk_ourano_h3 );

	CVAR_REGISTER ( &sk_ourano_d1 );
	CVAR_REGISTER ( &sk_ourano_d2 );
	CVAR_REGISTER ( &sk_ourano_d3 );

	CVAR_REGISTER ( &sk_raptor_h1 );
	CVAR_REGISTER ( &sk_raptor_h2 );
	CVAR_REGISTER ( &sk_raptor_h3 );

	CVAR_REGISTER ( &sk_raptor_bite_d1 );
	CVAR_REGISTER ( &sk_raptor_bite_d2 );
	CVAR_REGISTER ( &sk_raptor_bite_d3 );

	CVAR_REGISTER ( &sk_raptor_jump_d1 );
	CVAR_REGISTER ( &sk_raptor_jump_d2 );
	CVAR_REGISTER ( &sk_raptor_jump_d3 );

	CVAR_REGISTER ( &sk_smlscorp_h1 );
	CVAR_REGISTER ( &sk_smlscorp_h2 );
	CVAR_REGISTER ( &sk_smlscorp_h3 );

	CVAR_REGISTER ( &sk_smlscorp_claw_d1 );
	CVAR_REGISTER ( &sk_smlscorp_claw_d2 );
	CVAR_REGISTER ( &sk_smlscorp_claw_d3 );

	CVAR_REGISTER ( &sk_smlscorp_sting_d1 );
	CVAR_REGISTER ( &sk_smlscorp_sting_d2 );
	CVAR_REGISTER ( &sk_smlscorp_sting_d3 );

	CVAR_REGISTER ( &sk_lrgscorp_h1 );
	CVAR_REGISTER ( &sk_lrgscorp_h2 );
	CVAR_REGISTER ( &sk_lrgscorp_h3 );

	CVAR_REGISTER ( &sk_lrgscorp_claw_d1 );
	CVAR_REGISTER ( &sk_lrgscorp_claw_d2 );
	CVAR_REGISTER ( &sk_lrgscorp_claw_d3 );

	CVAR_REGISTER ( &sk_lrgscorp_sting_d1 );
	CVAR_REGISTER ( &sk_lrgscorp_sting_d2 );
	CVAR_REGISTER ( &sk_lrgscorp_sting_d3 );

	CVAR_REGISTER ( &sk_rustbit_h1 );
	CVAR_REGISTER ( &sk_rustbit_h2 );
	CVAR_REGISTER ( &sk_rustbit_h3 );

	CVAR_REGISTER ( &sk_rustbit_shot_d1 );
	CVAR_REGISTER ( &sk_rustbit_shot_d2 );
	CVAR_REGISTER ( &sk_rustbit_shot_d3 );

	CVAR_REGISTER ( &sk_rustbit_claw_d1 );
	CVAR_REGISTER ( &sk_rustbit_claw_d2 );
	CVAR_REGISTER ( &sk_rustbit_claw_d3 );

	CVAR_REGISTER ( &sk_rustbot_h1 );
	CVAR_REGISTER ( &sk_rustbot_h2 );
	CVAR_REGISTER ( &sk_rustbot_h3 );

	CVAR_REGISTER ( &sk_rustbot_melee_d1 );
	CVAR_REGISTER ( &sk_rustbot_melee_d2 );
	CVAR_REGISTER ( &sk_rustbot_melee_d3 );

	CVAR_REGISTER ( &sk_rustbot_long_d1 );
	CVAR_REGISTER ( &sk_rustbot_long_d2 );
	CVAR_REGISTER ( &sk_rustbot_long_d3 );

	CVAR_REGISTER ( &sk_rustgunner_h1 );
	CVAR_REGISTER ( &sk_rustgunner_h2 );
	CVAR_REGISTER ( &sk_rustgunner_h3 );

	CVAR_REGISTER ( &sk_rustgunner_r_d1 );
	CVAR_REGISTER ( &sk_rustgunner_r_d2 );
	CVAR_REGISTER ( &sk_rustgunner_r_d3 );

	CVAR_REGISTER ( &sk_rustgunner_mr_d1 );
	CVAR_REGISTER ( &sk_rustgunner_mr_d2 );
	CVAR_REGISTER ( &sk_rustgunner_mr_d3 );

	CVAR_REGISTER ( &sk_scientist_h1 );
	CVAR_REGISTER ( &sk_scientist_h2 );
	CVAR_REGISTER ( &sk_scientist_h3 );

	CVAR_REGISTER ( &sk_tube_h1 );
	CVAR_REGISTER ( &sk_tube_h2 );
	CVAR_REGISTER ( &sk_tube_h3 );

	CVAR_REGISTER ( &sk_beak_h1 );
	CVAR_REGISTER ( &sk_beak_h2 );
	CVAR_REGISTER ( &sk_beak_h3 );

	CVAR_REGISTER ( &sk_beak_bite_d1 );
	CVAR_REGISTER ( &sk_beak_bite_d2 );
	CVAR_REGISTER ( &sk_beak_bite_d3 );

	CVAR_REGISTER ( &sk_beak_scrape_d1 );
	CVAR_REGISTER ( &sk_beak_scrape_d2 );
	CVAR_REGISTER ( &sk_beak_scrape_d3 );

	CVAR_REGISTER ( &sk_critter_h1 );
	CVAR_REGISTER ( &sk_critter_h2 );
	CVAR_REGISTER ( &sk_critter_h3 );

	CVAR_REGISTER ( &sk_critter_spit_d1 );
	CVAR_REGISTER ( &sk_critter_spit_d2 );
	CVAR_REGISTER ( &sk_critter_spit_d3 );

	CVAR_REGISTER ( &sk_critter_melee1_d1 );
	CVAR_REGISTER ( &sk_critter_melee1_d2 );
	CVAR_REGISTER ( &sk_critter_melee1_d3 );

	CVAR_REGISTER ( &sk_maggot_d1 );
	CVAR_REGISTER ( &sk_maggot_d2 );
	CVAR_REGISTER ( &sk_maggot_d3 );

	CVAR_REGISTER ( &sk_maggot_h1 );
	CVAR_REGISTER ( &sk_maggot_h2 );
	CVAR_REGISTER ( &sk_maggot_h3 );

	CVAR_REGISTER ( &sk_hatchetfish_h1 );
	CVAR_REGISTER ( &sk_hatchetfish_h2 );
	CVAR_REGISTER ( &sk_hatchetfish_h3 );

	CVAR_REGISTER ( &sk_hatchetfish_d1 );
	CVAR_REGISTER ( &sk_hatchetfish_d2 );
	CVAR_REGISTER ( &sk_hatchetfish_d3 );

	CVAR_REGISTER ( &sk_chopper_h1 );
	CVAR_REGISTER ( &sk_chopper_h2 );
	CVAR_REGISTER ( &sk_chopper_h3 );

	CVAR_REGISTER ( &sk_xenome_h1 );
	CVAR_REGISTER ( &sk_xenome_h2 );
	CVAR_REGISTER ( &sk_xenome_h3 );

	CVAR_REGISTER ( &sk_xenomemelee_d1 );
	CVAR_REGISTER ( &sk_xenomemelee_d2 );
	CVAR_REGISTER ( &sk_xenomemelee_d3 );

	CVAR_REGISTER ( &sk_xenomedart_d1 );
	CVAR_REGISTER ( &sk_xenomedart_d2 );
	CVAR_REGISTER ( &sk_xenomedart_d3 );

	CVAR_REGISTER ( &sk_9mmAR_bullet1 );
	CVAR_REGISTER ( &sk_9mmAR_bullet2 );
	CVAR_REGISTER ( &sk_9mmAR_bullet3 );

	CVAR_REGISTER ( &sk_buckshot1 );
	CVAR_REGISTER ( &sk_buckshot2 );
	CVAR_REGISTER ( &sk_buckshot3 );

	CVAR_REGISTER ( &sk_gaussshot_d1 );
	CVAR_REGISTER ( &sk_gaussshot_d2 );
	CVAR_REGISTER ( &sk_gaussshot_d3 );

	CVAR_REGISTER ( &sk_gaussfast_d1 );
	CVAR_REGISTER ( &sk_gaussfast_d2 );
	CVAR_REGISTER ( &sk_gaussfast_d3 );

	CVAR_REGISTER ( &sk_gaussslow_d1 );
	CVAR_REGISTER ( &sk_gaussslow_d2 );
	CVAR_REGISTER ( &sk_gaussslow_d3 );

	CVAR_REGISTER ( &sk_chemgun_d1 );
	CVAR_REGISTER ( &sk_chemgun_d2 );
	CVAR_REGISTER ( &sk_chemgun_d3 );

	CVAR_REGISTER ( &sk_hand_grenade_d1 );
	CVAR_REGISTER ( &sk_hand_grenade_d2 );
	CVAR_REGISTER ( &sk_hand_grenade_d3 );

	CVAR_REGISTER ( &sk_mine_d1 );
	CVAR_REGISTER ( &sk_mine_d2 );
	CVAR_REGISTER ( &sk_mine_d3 );

	CVAR_REGISTER ( &sk_dml_rocket_d1 );
	CVAR_REGISTER ( &sk_dml_rocket_d2 );
	CVAR_REGISTER ( &sk_dml_rocket_d3 );

	CVAR_REGISTER ( &sk_onehandpunch_d1 );
	CVAR_REGISTER ( &sk_onehandpunch_d2 );
	CVAR_REGISTER ( &sk_onehandpunch_d3 );

	CVAR_REGISTER ( &sk_twohandpunch_d1 );
	CVAR_REGISTER ( &sk_twohandpunch_d2 );
	CVAR_REGISTER ( &sk_twohandpunch_d3 );

	CVAR_REGISTER ( &sk_knife1_d1 );
	CVAR_REGISTER ( &sk_knife1_d2 );
	CVAR_REGISTER ( &sk_knife1_d3 );

	CVAR_REGISTER ( &sk_suitcharger1 );
	CVAR_REGISTER ( &sk_suitcharger2 );
	CVAR_REGISTER ( &sk_suitcharger3 );

	CVAR_REGISTER ( &sk_battery1 );
	CVAR_REGISTER ( &sk_battery2 );
	CVAR_REGISTER ( &sk_battery3 );

	CVAR_REGISTER ( &sk_armorsuit1 );
	CVAR_REGISTER ( &sk_armorsuit2 );
	CVAR_REGISTER ( &sk_armorsuit3 );

	CVAR_REGISTER ( &sk_healthcharger1 );
	CVAR_REGISTER ( &sk_healthcharger2 );
	CVAR_REGISTER ( &sk_healthcharger3 );

	CVAR_REGISTER ( &sk_healthkit1 );
	CVAR_REGISTER ( &sk_healthkit2 );
	CVAR_REGISTER ( &sk_healthkit3 );

	CVAR_REGISTER ( &sk_scientist_heal1 );
	CVAR_REGISTER ( &sk_scientist_heal2 );
	CVAR_REGISTER ( &sk_scientist_heal3 );

// monster damage adjusters
	CVAR_REGISTER ( &sk_monster_head1 );
// monster damage adjusters
	CVAR_REGISTER ( &sk_monster_head2 );
// monster damage adjusters
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

// player damage adjusters
	CVAR_REGISTER ( &sk_player_head1 );
// player damage adjusters
	CVAR_REGISTER ( &sk_player_head2 );
// player damage adjusters
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

	SERVER_COMMAND( "exec skill.cfg\n" );
}


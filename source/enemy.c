// ***************************************************************************
// Vectrex C programming file template
// ***************************************************************************

#include <vectrex.h>
#include "generic.h"
#include "draw.h"
#include "platform.h"
#include "vulture.h"
#include "knight.h"
#include "ptery.h"
#include "egg.h"
#include "player.h"
#include "enemy.h"

// ---------------------------------------------------------------------------

extern const signed char* const spiral[];
extern struct player player_1;

static const struct character_anim enemy_anims[] =
{
	/* Vulture */
	{
		10,						// h
		4,						// w
		3,						// treshold
		VULTURE_LEFT,
		VULTURE_RIGHT,
		VULTURE_WALK_LEFT_END - VULTURE_WALK_LEFT_START + 1, // max_frames
		VULTURE_WALK_LEFT_START,
		VULTURE_WALK_RIGHT_START,
		VULTURE_BRAKE_LEFT,
		VULTURE_BRAKE_RIGHT,
		vulture					// vectorlists
	},

	/* Ptery */
	{
		8,						// h
		12,						// w
		5,						// treshold
		PTERY_LEFT,
		PTERY_RIGHT,
		PTERY_FLAP_LEFT - PTERY_LEFT + 1,
		PTERY_LEFT,
		PTERY_RIGHT,
		PTERY_DIVE_LEFT,
		PTERY_DIVE_RIGHT,
		ptery
	}
};

const struct enemy_race enemy_races[] =
{
	/*	type					speed	speed_treshold	flap_treshold		gravity_treshold	rise_treshold	reaction_treshold	bounce_treshold	anim	*/
	{	ENEMY_TYPE_BOUNCER,	2,		6,				24,				3,				2,			24,				12,				&enemy_anims[0]	},
	{	ENEMY_TYPE_PTERY,		3,		2,				255,				4,				4,			56,				0,				&enemy_anims[1]	}
};

unsigned int enemy_status = 0;
struct object *enemy_list = 0;
struct object *enemy_free_list = 0;

static void set_dir_enemy(
	struct enemy *enemy,
	unsigned int dir
	)
{
	enemy->ch.dir = dir;

	if (dir == DIR_LEFT)
	{
		enemy->ch.base_frame = enemy->ch.anim->frame_left;
		enemy->ch.dy = 0;
		enemy->target_speed = -enemy->ch.move_speed;
	}
	else if (dir == DIR_RIGHT)
	{
		enemy->ch.base_frame = enemy->ch.anim->frame_right;
		enemy->ch.dy = 0;
		enemy->target_speed = enemy->ch.move_speed;
	}
}

void init_enemy(
	struct enemy *enemy,
	signed int y,
	signed int x,
	unsigned int dir,
	unsigned int state,
	const struct enemy_race *race
	)
{
	take_object(&enemy->ch.obj, &enemy_free_list);
	init_character(&enemy->ch, y, x, race->speed, race->anim, &enemy_list);
	init_character_0(&enemy->ch_0, y, x, 1, race->anim);

	enemy->race			= race;
	enemy->speed_counter	= 0;
	enemy->gravity_counter	= 0;
	enemy->rise_counter	= 0;
	enemy->spawn_counter	= 0;
	enemy->window_counter	= 0;
	enemy->number_counter	= 0;

	enemy->state = state;
	enemy->state_counter = 0;

	set_dir_enemy(enemy, dir);
}

void deinit_enemy(
	struct enemy *enemy
	)
{
	deinit_object(&enemy->ch.obj, &enemy_list);
	give_object(&enemy->ch.obj, &enemy_free_list);
}

void move_enemies(void)
{
	struct enemy *enemy;
	unsigned int enemy_type;
	struct enemy *rem_enemy = 0;

	enemy_status = 0;

	enemy = (struct enemy *) enemy_list;
	while(enemy != 0)
	{
		if (++enemy->window_counter == ENEMY_TIME_WINDOW)
		{
			enemy->window_counter = 0;
			enemy->number_counter = 0;
		}

		enemy_type = enemy->race->type;

		if (enemy->state == ENEMY_STATE_MOVE)
		{
			if (++enemy->speed_counter == enemy->race->speed_treshold)
			{
				enemy->speed_counter = 0;
				if (enemy->ch.dx < enemy->target_speed)
				{
					enemy->ch.dx++;
				}
				else if (enemy->ch.dx > enemy->target_speed)
				{
					enemy->ch.dx--;
				}
			}

			if (++enemy->gravity_counter == enemy->race->gravity_treshold)
			{
				enemy->gravity_counter = 0;
				enemy->ch.dy = -1;
			}
			else
			{
				enemy->ch.dy = 0;
			}

			if (enemy_type == ENEMY_TYPE_BOUNCER)
			{
				if (hit_over_platform(&enemy->ch.obj, &enemy->ch.dy, enemy->ch.dx))
				{
					if (enemy->ch.dir == DIR_LEFT)
					{
						enemy->ch.base_frame = enemy->ch.anim->frame_walk_left;
					}
					else if (enemy->ch.dir == DIR_RIGHT)
					{
						enemy->ch.base_frame = enemy->ch.anim->frame_walk_right;
					}
					enemy->state_counter = 0;
					enemy->state = ENEMY_STATE_WALK;
				}
				else if (!hit_platform(&enemy->ch.obj, &enemy->ch.dy, &enemy->ch.dx))
				{
					if (move_character(&enemy->ch) == 2)
					{
						enemy->state_counter = 0;
						enemy->state = ENEMY_STATE_REMOVE;
					}

					if (++enemy->state_counter == enemy->race->reaction_treshold)
					{
						if (enemy->ch.obj.y < player_1.ch.obj.y)
						{
							enemy->state = ENEMY_STATE_FLAP;
						}
						enemy->state_counter = 0;
					}
				}
				else
				{
					enemy->state_counter = 0;
					enemy->number_counter++;
					enemy->state = ENEMY_STATE_BOUNCE;
				}
			}
			else if (enemy_type == ENEMY_TYPE_PTERY)
			{
				enemy->ch.frame = 2;
				unsigned int ret = move_character(&enemy->ch);
				if (ret == 2)
				{
					enemy->state_counter = 0;
					enemy->state = ENEMY_STATE_REMOVE;
				}
				else if (ret == 3)
				{
					enemy->ch.frame = 0;
					enemy->state_counter = 0;
					enemy->state = ENEMY_STATE_STOP;
				}
			}
		}
		else if (enemy->state == ENEMY_STATE_FLAP)
		{
			if (++enemy->speed_counter == enemy->race->speed_treshold)
			{
				enemy->speed_counter = 0;
				if (enemy->ch.dx < enemy->target_speed)
				{
					enemy->ch.dx++;
				}
				else if (enemy->ch.dx > enemy->target_speed)
				{
					enemy->ch.dx--;
				}
			}

			if (++enemy->rise_counter == enemy->race->rise_treshold)
			{
				enemy->rise_counter = 0;
				enemy->ch.dy = 1;
			}
			else
			{
				enemy->ch.dy = 0;
			}

			if (enemy_type == ENEMY_TYPE_BOUNCER)
			{
				animate_character_limit(&enemy->ch, 2);

				if (!hit_platform(&enemy->ch.obj, &enemy->ch.dy, &enemy->ch.dx))
				{
					if (move_character(&enemy->ch) == 2)
					{
						enemy->state_counter = 0;
						enemy->state = ENEMY_STATE_REMOVE;
					}

					if (++enemy->state_counter == enemy->race->flap_treshold)
					{
						enemy->ch.frame = 0;
						enemy->state_counter = 0;
						enemy->state = ENEMY_STATE_MOVE;
					}
				}
				else
				{
					enemy->ch.frame = 0;
					enemy->state_counter = 0;
					enemy->number_counter++;
					enemy->state = ENEMY_STATE_BOUNCE;
				}
			}
			else if (enemy_type == ENEMY_TYPE_PTERY)
			{
				animate_character(&enemy->ch);

				unsigned int ret = move_character(&enemy->ch);
				if (ret == 2)
				{
					enemy->state_counter = 0;
					enemy->state = ENEMY_STATE_REMOVE;
				}
				else if (ret == 3)
				{
					enemy->state_counter = 0;
					enemy->state = ENEMY_STATE_STOP;
				}
			}
		}
		else if (enemy->state == ENEMY_STATE_BOUNCE)
		{
			if (++enemy->gravity_counter == enemy->race->gravity_treshold)
			{
				enemy->gravity_counter = 0;
				enemy->ch.dy = -1;
			}
			else
			{
				enemy->ch.dy = 0;
			}

			if (!hit_platform(&enemy->ch.obj, &enemy->ch.dy, &enemy->ch.dx))
			{
				if (move_character(&enemy->ch) == 2)
				{
					enemy->state_counter = 0;
					enemy->state = ENEMY_STATE_REMOVE;
				}
			}

			if (++enemy->state_counter == enemy->race->bounce_treshold)
			{
				if (enemy->number_counter >= ENEMY_MAX_BOUNCES)
				{
					enemy->window_counter = 0;
					enemy->number_counter = 0;
					if (enemy->ch.dir == DIR_LEFT)
					{
						set_dir_enemy(enemy, DIR_RIGHT);
					}
					else if (enemy->ch.dir == DIR_RIGHT)
					{
						set_dir_enemy(enemy, DIR_LEFT);
					}
				}

				enemy->state_counter = 0;

				if (enemy->ch.obj.y > player_1.ch.obj.y)
				{
					enemy->state = ENEMY_STATE_MOVE;
				}
				else
				{
					enemy->state = ENEMY_STATE_FLAP;
				}
			}
		}
		else if (enemy->state == ENEMY_STATE_WALK)
		{
			animate_character(&enemy->ch);

			if (++enemy->state_counter == enemy->race->reaction_treshold)
			{
				if (enemy->ch.dir == DIR_LEFT)
				{
					enemy->ch.base_frame = enemy->ch.anim->frame_left;
				}
				else if (enemy->ch.dir == DIR_RIGHT)
				{
					enemy->ch.base_frame = enemy->ch.anim->frame_right;
				}
				enemy->ch.frame = 0;
				enemy->state_counter = 0;
				enemy->state = ENEMY_STATE_FLAP;
			}
			else if (!hit_over_platform(&enemy->ch.obj, &enemy->ch.dy, enemy->ch.dx))
			{
				if (enemy->ch.dir == DIR_LEFT)
				{
					enemy->ch.base_frame = enemy->ch.anim->frame_left;
				}
				else if (enemy->ch.dir == DIR_RIGHT)
				{
					enemy->ch.base_frame = enemy->ch.anim->frame_right;
				}
				enemy->ch.frame = 0;
				enemy->state_counter = 0;
				enemy->state = ENEMY_STATE_MOVE;
			}
			else if (!hit_platform(&enemy->ch.obj, &enemy->ch.dy, &enemy->ch.dx))
			{
				if (move_character(&enemy->ch) == 2)
				{
					enemy->state_counter = 0;
					enemy->state = ENEMY_STATE_REMOVE;
				}
			}
		}
		else if (enemy->state == ENEMY_STATE_EGG)
		{
			if (enemy->ch.dy != 0)
			{
				if (move_character(&enemy->ch) == 2)
				{
					enemy->state_counter = 0;
					enemy->state = ENEMY_STATE_REMOVE;
				}
				else if (hit_over_platform(&enemy->ch.obj, &enemy->ch.dy, enemy->ch.dx))
				{
					enemy->ch.dy = 0;
					enemy->ch.dx = 0;
				}
			}
			else
			{
				if (++enemy->state_counter >= ENEMY_EGG_TRESHOLD)
				{
					enemy->state = ENEMY_STATE_KNIGHT;
					enemy->state_counter = 0;
				}
			}
		}
		else if (enemy->state == ENEMY_STATE_KNIGHT)
		{
			if (++enemy->state_counter == ENEMY_KNIGHT_TRESHOLD)
			{
				enemy->ch_0.obj.y = enemy->ch.obj.y + ENEMY_CALL_BIRD_DY;

				if (enemy->ch.obj.x < 0)
				{
					enemy->ch_0.obj.x = CHARACTER_MIN_X;
					set_dir_character(&enemy->ch_0, DIR_RIGHT);
				}
				else
				{
					enemy->ch_0.obj.x = CHARACTER_MAX_X;
					set_dir_character(&enemy->ch_0, DIR_LEFT);
				}

				enemy->ch_0.obj.active = 1;

				enemy->state = ENEMY_STATE_CALL_BIRD;
				enemy->state_counter = 0;
			}
		}
		else if (enemy->state == ENEMY_STATE_CALL_BIRD)
		{
			unsigned int match = 0;

			enemy->state_counter = 0;

			if (enemy->ch_0.obj.y > enemy->ch.obj.y)
			{
				if (++enemy->gravity_counter == enemy->race->gravity_treshold)
				{
					enemy->gravity_counter = 0;
					enemy->ch_0.dy = -1;
				}
				else
				{
					enemy->ch_0.dy = 0;
				}
			}
			else
			{
				enemy->ch_0.dy = 0;
				match = 1;
			}

			if (enemy->ch_0.obj.x < enemy->ch.obj.x)
			{
				set_dir_character(&enemy->ch_0, DIR_RIGHT);
			}
			else if (enemy->ch_0.obj.x > enemy->ch.obj.x)
			{
				set_dir_character(&enemy->ch_0, DIR_LEFT);
			}
			else if (match)
			{
				enemy->state_counter = 0;
				set_dir_enemy(enemy, enemy->ch_0.dir);
				enemy->state = ENEMY_STATE_MOVE;
			}

			move_character(&enemy->ch_0);
		}
		else if (enemy->state == ENEMY_STATE_SPAWN)
		{
			if (++enemy->ch.counter >= ENEMY_SPAWN_ANIM_TRESHOLD)
			{
				enemy->ch.counter = 0;
				if (++enemy->ch.frame >= ENEMY_SPAWN_ANIM_FRAMES)
				{
					enemy->ch.frame = 0;
				}
			}

			if (++enemy->state_counter >= ENEMY_SPAWN_TRESHOLD)
			{
				enemy->ch.counter = 0;
				enemy->ch.frame = 0;
				enemy->state = ENEMY_STATE_MOVE;
				enemy->state_counter = 0;
			}
		}
		else if (enemy->state == ENEMY_STATE_STOP)
		{
			if (++enemy->state_counter >= enemy->race->reaction_treshold)
			{
				if (enemy->ch.obj.x < player_1.ch.obj.x)
				{
					set_dir_enemy(enemy, DIR_RIGHT);
				}
				else if (enemy->ch.obj.x > player_1.ch.obj.x)
				{
					set_dir_enemy(enemy, DIR_LEFT);
				}

				if (enemy->ch.obj.y < player_1.ch.obj.y)
				{
					enemy->state = ENEMY_STATE_FLAP;
				}
				else
				{
					enemy->state = ENEMY_STATE_MOVE;
				}
				enemy->state_counter = 0;
			}
		}
		else if (enemy->state == ENEMY_STATE_REMOVE)
		{
			rem_enemy = enemy;
		}
		enemy = (struct enemy *) enemy->ch.obj.next;

		if (rem_enemy != 0)
		{
			rem_enemy->state_counter = 0;
			enemy->state = ENEMY_STATE_REMOVED;
			deinit_enemy(rem_enemy);
			rem_enemy = 0;
		}
	}
}

void retreat_enemy(
	struct enemy *enemy
	)
{
	enemy->ch.dx = -enemy->ch.dx;
}

void hit_enemy(
	struct enemy *enemy
	)
{
	enemy->ch.dy = -1;
	enemy->state_counter = 0;
	enemy->state = ENEMY_STATE_EGG;
}

void draw_enemies(void)
{
	struct enemy *enemy;

	enemy = (struct enemy *) enemy_list;
	while (enemy != 0)
	{
		if (enemy->state >= ENEMY_STATE_MOVE && enemy->state <= ENEMY_STATE_WALK)
		{
			// ZERO
			dp_VIA_cntl=0xcc;

			signed int y = enemy->ch.obj.y;
			signed int x = enemy->ch.obj.x;			
			
			dp_VIA_t1_cnt_lo = OBJECT_MOVE_SCALE;
            	dp_VIA_port_a = y;			// y pos to dac
            	dp_VIA_cntl = 0xce;	// disable zero, disable all blank
            	dp_VIA_port_b = 0;			// mux enable, dac to -> integrator y (and x)
            	dp_VIA_shift_reg = 0;		// all output is BLANK
            	dp_VIA_port_b++;			// mux disable, dac only to x
            	dp_VIA_port_a = x;			// dac -> x
            	dp_VIA_t1_cnt_hi=0;		// start timer
			dp_VIA_t1_cnt_lo = ENEMY_DRAW_SCALE;
            	while ((dp_VIA_int_flags & 0x40) == 0); // wait till timer finishes

			draw_vlp_1(enemy->ch.anim->shapes[enemy->ch.base_frame + enemy->ch.frame]);
						
/*						
			draw_synced_list_c(
				enemy->ch.anim->shapes[enemy->ch.base_frame + enemy->ch.frame],
				enemy->ch.obj.y,
				enemy->ch.obj.x,
				OBJECT_MOVE_SCALE,
				enemy->ch.obj.scale
				);
*/			

			if (enemy->race->type < ENEMY_TYPE_PTERY)
			{
				// ZERO
				dp_VIA_cntl=0xcc;

				dp_VIA_t1_cnt_lo = OBJECT_MOVE_SCALE;
            		dp_VIA_port_a = y;			// y pos to dac
            		dp_VIA_cntl = 0xce;	// disable zero, disable all blank
            		dp_VIA_port_b = 0;			// mux enable, dac to -> integrator y (and x)
            		dp_VIA_shift_reg = 0;		// all output is BLANK
            		dp_VIA_port_b++;			// mux disable, dac only to x
            		dp_VIA_port_a = x;			// dac -> x
            		dp_VIA_t1_cnt_hi=0;		// start timer
				dp_VIA_t1_cnt_lo = KNIGHT_DRAW_SCALE;
            		while ((dp_VIA_int_flags & 0x40) == 0); // wait till timer finishes
				draw_vlp_2(knight[enemy->ch.dir]);			
			
/*
				draw_synced_list_c(
					knight[enemy->ch.dir],
					enemy->ch.obj.y,
					enemy->ch.obj.x,
					OBJECT_MOVE_SCALE,
					0x18/KNIGHT_SCALE
					);
*/
			}			
		}
		else if (enemy->state == ENEMY_STATE_EGG)
		{
			// ZERO
			dp_VIA_cntl=0xcc;

			signed int y = enemy->ch.obj.y - enemy->ch.obj.h_2;
			signed int x = enemy->ch.obj.x;			
			
			dp_VIA_t1_cnt_lo = OBJECT_MOVE_SCALE;
            	dp_VIA_port_a = y;			// y pos to dac
            	dp_VIA_cntl = 0xce;	// disable zero, disable all blank
            	dp_VIA_port_b = 0;			// mux enable, dac to -> integrator y (and x)
            	dp_VIA_shift_reg = 0;		// all output is BLANK
            	dp_VIA_port_b++;			// mux disable, dac only to x
            	dp_VIA_port_a = x;			// dac -> x
            	dp_VIA_t1_cnt_hi=0;		// start timer
			dp_VIA_t1_cnt_lo = ENEMY_DRAW_SCALE;
            	while ((dp_VIA_int_flags & 0x40) == 0); // wait till timer finishes

			draw_vlp_1(egg);
		}
		else if (enemy->state == ENEMY_STATE_KNIGHT)
		{
			signed int y = enemy->ch.obj.y - enemy->ch.obj.h_2;;
			signed int x = enemy->ch.obj.x;			

			// ZERO
			dp_VIA_cntl=0xcc;

			dp_VIA_t1_cnt_lo = OBJECT_MOVE_SCALE;
            	dp_VIA_port_a = y;			// y pos to dac
            	dp_VIA_cntl = 0xce;	// disable zero, disable all blank
            	dp_VIA_port_b = 0;			// mux enable, dac to -> integrator y (and x)
            	dp_VIA_shift_reg = 0;		// all output is BLANK
            	dp_VIA_port_b++;			// mux disable, dac only to x
            	dp_VIA_port_a = x;			// dac -> x
            	dp_VIA_t1_cnt_hi=0;		// start timer
			dp_VIA_t1_cnt_lo = KNIGHT_DRAW_SCALE;
            	while ((dp_VIA_int_flags & 0x40) == 0); // wait till timer finishes
			draw_vlp_2(knight[2]);
		}
		else if (enemy->state == ENEMY_STATE_CALL_BIRD)
		{
			signed int y = enemy->ch.obj.y - enemy->ch.obj.h_2;;
			signed int x = enemy->ch.obj.x;			

			// ZERO
			dp_VIA_cntl=0xcc;

			dp_VIA_t1_cnt_lo = OBJECT_MOVE_SCALE;
            	dp_VIA_port_a = y;			// y pos to dac
            	dp_VIA_cntl = 0xce;	// disable zero, disable all blank
            	dp_VIA_port_b = 0;			// mux enable, dac to -> integrator y (and x)
            	dp_VIA_shift_reg = 0;		// all output is BLANK
            	dp_VIA_port_b++;			// mux disable, dac only to x
            	dp_VIA_port_a = x;			// dac -> x
            	dp_VIA_t1_cnt_hi=0;		// start timer
			dp_VIA_t1_cnt_lo = KNIGHT_DRAW_SCALE;
            	while ((dp_VIA_int_flags & 0x40) == 0); // wait till timer finishes
			draw_vlp_2(knight[2]);

			if (enemy->ch_0.obj.active)
			{
				// ZERO
				dp_VIA_cntl=0xcc;

				y = enemy->ch_0.obj.y;
				x = enemy->ch_0.obj.x;			
			
				dp_VIA_t1_cnt_lo = OBJECT_MOVE_SCALE;
            		dp_VIA_port_a = y;			// y pos to dac
            		dp_VIA_cntl = 0xce;	// disable zero, disable all blank
            		dp_VIA_port_b = 0;			// mux enable, dac to -> integrator y (and x)
            		dp_VIA_shift_reg = 0;		// all output is BLANK
            		dp_VIA_port_b++;			// mux disable, dac only to x
            		dp_VIA_port_a = x;			// dac -> x
            		dp_VIA_t1_cnt_hi=0;		// start timer
				dp_VIA_t1_cnt_lo = ENEMY_DRAW_SCALE;
            		while ((dp_VIA_int_flags & 0x40) == 0); // wait till timer finishes

				draw_vlp_1(enemy->ch.anim->shapes[enemy->ch.base_frame + enemy->ch.frame]);
			}
		}
		else if (enemy->state == ENEMY_STATE_SPAWN)
		{
			draw_synced_list_c(
				spiral[enemy->ch.frame],
				enemy->ch.obj.y,
				enemy->ch.obj.x,
				OBJECT_MOVE_SCALE,
				0x01 + (enemy->state_counter >> 2)
				);
		}

		enemy = (struct enemy *) enemy->ch.obj.next;
	}
}

// ***************************************************************************
// end of file
// ***************************************************************************

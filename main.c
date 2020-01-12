#include "libs.h"
#include "helpers.c"
#include "consts.c"
#include "maps.c"

struct v2 TILE_CENTER = { 3*PIXEL_SIZE, 4*PIXEL_SIZE };

struct view {
	int top, left;
	struct v2 camera_target_tile;
	bool zoom_view;
};

struct v2 eyes_target_tile = { ARENA_WIDTH_IN_TILES / 2 - 1, 11 };

typedef char dot_map_t[ARENA_HEIGHT_IN_TILES][ARENA_WIDTH_IN_TILES];

dot_map_t new_dot_map;

const struct level new_level;

// pairs for colors
enum {
	BG_PAIR = 1,
	PLAYER_PAIR,
	WALL_PAIR,
	FOOD_PAIR,
};

enum dir {
	UP,
	LEFT,
	DOWN,
	RIGHT
};

char get_arena(int row, int col) {
	if (col < 0) {
		col += ARENA_WIDTH_IN_TILES;
	} else if (col >= ARENA_WIDTH_IN_TILES) {
		col -= ARENA_WIDTH_IN_TILES;
	}
	return arena_map[row*ARENA_WIDTH_IN_TILES + col];
}

#define DRAW_SIZE 8

struct v2 draw_pos(int row, int col, struct view* view) {
	struct v2 ret = {
		3 * (col - view->camera_target_tile.x) + view->left,
		(row - view->camera_target_tile.y) + view->top,
	};
	return ret;
}

struct v2 draw_pos_v2(struct v2 p, struct view* view) {
	return draw_pos(p.y, p.x, view);
}

void draw_tile(int row, int col, char ch, struct view* view, char fill_ch) {
	if (view->zoom_view) {
		int tile_size = DRAW_SIZE;
		int i;
		for (i = 0; i < tile_size; ++i) {
			int j;
			for (j = 0; j < tile_size; ++j) {
				mvaddch(tile_size * (row - view->camera_target_tile.y) + view->top + j, tile_size * (col - view->camera_target_tile.x) + view->left + i, fill_ch);
			}
		}
	} else {
		struct v2 pos = draw_pos(row, col, view);
		mvaddch(pos.y, pos.x, fill_ch);
		mvaddch(pos.y, pos.x + 1, ch);
		mvaddch(pos.y, pos.x + 2, fill_ch);
	}
}

void draw_tile_v2(struct v2 tile_pos, char ch, struct view* view, char fill_ch) {
	draw_tile(tile_pos.y, tile_pos.x, ch, view, fill_ch);
}

bool is_h_dir(enum dir dir) {
	return dir == LEFT || dir == RIGHT;
}

void move_in_dir(enum dir dir, struct v2* pos, int amt) {
	switch (dir) {
		case RIGHT:
			pos->x += amt;
			break;
		case LEFT:
			pos->x -= amt;
			break;
		case UP:
			pos->y -= amt;
			break;
		case DOWN:
			pos->y += amt;
			break;
	}
}

struct v2 pos_to_tile(struct v2* p) {
	return div_scale(*p, TILE_SIZE);
}

int num_blocked_tiles = 0;
struct v2 blocked_tiles[20];

bool test_player_dir_blocked(enum dir dir, struct v2* next_pos) {
	int mv_amt = TILE_SIZE / 2;
	if (dir == RIGHT || dir == UP) {
		mv_amt += PIXEL_SIZE;
	}
	move_in_dir(dir, next_pos, mv_amt);
	struct v2 next_tile = pos_to_tile(next_pos);
	return !!strchr("/`[]-|+_", get_arena(next_tile.y, next_tile.x));
}

int get_speed(int percentage) {
	return (percentage * PIXEL_SIZE / 100) * 6 / 5;
}

int seconds_to_frames(int seconds) {
	return SECONDS(seconds) / MS(16);
}

struct game_data {
	/* Data that persists between levels, but not between games. */
	int num_extra_lives;
	int current_level;
	int score;
	enum {
		PAUSED_BEFORE_PLAYING,
		GAME_OVER,
		PLAYING,
		LOSING_A_LIFE,
		LEVEL_TRANSITION,
	} mode;

	/* Data that describes the current state of a level */
	struct level {
		int player_chomp_timer, player_eat_timer,
					player_powerup_timer,
					fruit_score_timer,
					extra_life_timer;
		bool fruit_is_visible, player_blocked, player_turning, is_chasing,
					should_flash_extra_life;
		int dots_eaten; 

		struct v2 player_pos;
		enum dir player_dir, next_dir;
		int dot_timer;

		dot_map_t dot_map;
	} level;

	/* Data that describes the constants for the current level */
	struct level_constants {
		int player_speed, player_powerup_speed;
		int player_powerup_time;
		int elroy_v1_dots_left, elroy_v1_speed;

		int scatter_times[4];
		int chase_times[3];

	} level_constants;
};

void
set_level_constants(struct level_constants* level_constants, int level) {
	level_constants->player_speed = get_speed(80);
	level_constants->player_powerup_speed = get_speed(90);
}

void 
set_player_to_start_position(struct level* level) {
	struct v2 player_start_pos = { ARENA_WIDTH / 2, (ARENA_HEIGHT_IN_TILES - 8)*TILE_SIZE + TILE_CENTER.y };

	level->player_pos = player_start_pos;
	level->player_dir = LEFT;
	level->player_blocked = FALSE;
	level->player_turning = FALSE;
	level->next_dir = level->player_dir;
	level->dot_timer = 0;
}

void
start_new_level(struct game_data* game_data) {
	game_data->level = new_level;
	memcpy(game_data->level.dot_map, new_dot_map, sizeof(new_dot_map));
	set_player_to_start_position(&game_data->level);
}

const
struct game_data new_game = {
	.num_extra_lives = 1
};

struct game_data
create_new_game() {
	struct game_data game_data = new_game;

	set_level_constants(&game_data.level_constants, 0);

	start_new_level(&game_data);
	game_data.level.should_flash_extra_life = TRUE;
	return game_data;
}

int main(int argc, char** argv) {
	UNUSED(argc);
	UNUSED(argv);

	// Init curses
	initscr();
	cbreak();
	keypad(stdscr, TRUE);
	noecho();
	curs_set(FALSE);
	timeout(0);

	{ // Izveidot ēdiena karti
		int row, col;
		for (row = 0; row < ARENA_HEIGHT_IN_TILES; ++ row) {
			for (col = 0; col < ARENA_WIDTH_IN_TILES; ++ col) {
				char ch = food_map[row*ARENA_WIDTH_IN_TILES + col];
				if (ch == '.' || ch == '*' || ch == '@') {
					new_dot_map[row][col] = ch;
				}
			}
		}
	}

	int transition_timer = 0;
	bool running = TRUE;

	int last_update = time_in_us();
	int frame_timer = 0;
	int high_score = 0;
	int next_input = -1;
	int current_player = 0;
	struct view view = { 0 };

	start_color();

	// init_pair(index, foreground, background);
	init_pair(BG_PAIR, COLOR_WHITE, COLOR_BLACK_DARK);
	init_pair(PLAYER_PAIR, COLOR_YELLOW_BRIGHT, COLOR_GRAY);
	init_pair(FOOD_PAIR, COLOR_ORANGE, COLOR_GRAY);
	init_pair(WALL_PAIR, COLOR_WHITE, COLOR_GRAY);

	bkgd(COLOR_PAIR(BG_PAIR));

	struct game_data games[2] = { create_new_game(), create_new_game() };
	struct v2 player_turn_tile;

	games[current_player].mode = PAUSED_BEFORE_PLAYING;

	int frame_times[] = { 8000, 16667, 50000, 100000, 150000 };
	int frame_time_index = 1;
	while (running) {
		// Iegūt input
		int ch;
		while ((ch = getch()) != -1) {
			if (games[current_player].mode == GAME_OVER) {
				current_player = 0;
				transition_timer = 0;
				games[0] = create_new_game();
			}
			switch (ch) {
				case 'q':
				case 'Q':
					running = FALSE;
					continue;
				case 'r':
				case 'R':
					games[current_player].mode = GAME_OVER;
					break;
			}
			switch (ch) {
				case KEY_LEFT:
					next_input = LEFT;
					break;
				case KEY_RIGHT:
					next_input = RIGHT;
					break;
				case KEY_UP:
					next_input = UP;
					break;
				case KEY_DOWN:
					next_input = DOWN;
					break;
			}
		}

		if (!games[current_player].level.player_turning && next_input != -1) {
			games[current_player].level.next_dir = next_input;
			next_input = -1;
		}

		frame_timer += time_in_us() - last_update;
		last_update = time_in_us();

		if (frame_timer > frame_times[frame_time_index]) {
			++games[current_player].level.player_chomp_timer;

			switch (games[current_player].mode) {
				case PAUSED_BEFORE_PLAYING: {
					int time = seconds_to_frames(2);
					transition_timer++;
					if (transition_timer >= time) {
						transition_timer = 0;
						games[current_player].mode = PLAYING;
					}
				} break;
				case PLAYING: {
					int player_speed;
					int old_score = games[current_player].score;
					num_blocked_tiles = 0;
					
					player_speed = games[current_player].level_constants.player_speed;
					if (games[current_player].level.player_dir != games[current_player].level.next_dir) {
						if (is_h_dir(games[current_player].level.player_dir) == is_h_dir(games[current_player].level.next_dir)) {
							games[current_player].level.player_dir = games[current_player].level.next_dir;
						} else {
							struct v2 blocked_pos_s = games[current_player].level.player_pos;
							bool blocked = test_player_dir_blocked(games[current_player].level.next_dir, &blocked_pos_s);
							if (blocked) {
								blocked_tiles[num_blocked_tiles++] = pos_to_tile(&blocked_pos_s);
							} else {
								games[current_player].level.player_turning = TRUE;
								player_turn_tile = pos_to_tile(&games[current_player].level.player_pos);
							}
						}
					}

					// kustība
					if (games[current_player].level.player_turning) {
						move_in_dir(games[current_player].level.next_dir, &games[current_player].level.player_pos, player_speed);
						struct v2 tile_center = add(scale(player_turn_tile, TILE_SIZE), TILE_CENTER);
						bool reached_centerline = FALSE;
						if (is_h_dir(games[current_player].level.player_dir)) {
							int mv_amt = tile_center.x - games[current_player].level.player_pos.x;
							if (mv_amt < 0) {
								games[current_player].level.player_pos.x -= MIN(-mv_amt, player_speed);
								reached_centerline = -mv_amt < player_speed;
							} else {
								games[current_player].level.player_pos.x += MIN(mv_amt, player_speed);
								reached_centerline = mv_amt < player_speed;
							}
						} else {
							int mv_amt = tile_center.y - games[current_player].level.player_pos.y;
							if (mv_amt < 0) {
								games[current_player].level.player_pos.y -= MIN(-mv_amt, player_speed);
								reached_centerline = -mv_amt < player_speed;
							} else {
								games[current_player].level.player_pos.y += MIN(mv_amt, player_speed);
								reached_centerline = mv_amt < player_speed;
							}
						}

						if (reached_centerline) { 
							games[current_player].level.player_turning = FALSE;
							games[current_player].level.player_dir = games[current_player].level.next_dir;
						}
					} else {
						struct v2 blocked_pos_s = games[current_player].level.player_pos;
						games[current_player].level.player_blocked = test_player_dir_blocked(games[current_player].level.player_dir, &blocked_pos_s);
						if (games[current_player].level.player_blocked) {
							blocked_tiles[num_blocked_tiles++] = pos_to_tile(&blocked_pos_s);
						} else {
							move_in_dir(games[current_player].level.player_dir, &games[current_player].level.player_pos, player_speed);
							// if (games[current_player].level.player_pos.x < -TUNNEL_WIDTH / 2) {
							// 	games[current_player].level.player_pos.x += ARENA_WIDTH + TUNNEL_WIDTH;
							// } else if (games[current_player].level.player_pos.x >= ARENA_WIDTH + TUNNEL_WIDTH / 2) {
							// 	games[current_player].level.player_pos.x -= ARENA_WIDTH + TUNNEL_WIDTH;
							// }
						}
					}

					{
						struct v2 player_tile = pos_to_tile(&games[current_player].level.player_pos);
						char ch = games[current_player].level.dot_map[player_tile.y][player_tile.x];
						if (ch == '.') {
							games[current_player].level.player_eat_timer = 1;
							games[current_player].score += 10;
						} else if (ch == '*') {
							games[current_player].level.player_eat_timer = 3;
							games[current_player].level.player_powerup_timer = games[current_player].level_constants.player_powerup_time;
							games[current_player].score += 50;
						}
						if (ch == '.' || ch == '*') {
							int first_fruit_dot_counter = 10, second_fruit_dot_counter = 20;
							games[current_player].level.dot_timer = 0;
							++games[current_player].level.dots_eaten;
						}
						games[current_player].level.dot_map[player_tile.y][player_tile.x] = 0;
					}
				} break;
				case GAME_OVER: {
				} break;
				case LEVEL_TRANSITION: {
					++transition_timer;
					if (transition_timer > seconds_to_frames(1)) {
						set_level_constants(&games[current_player].level_constants, ++games[current_player].current_level);
						start_new_level(&games[current_player]);
						transition_timer = 0;
						games[current_player].mode = PAUSED_BEFORE_PLAYING;
					}
				} break;
			}

			erase();
			{
				view.left = 2;
				view.top = 3;

				if (view.zoom_view) {
					struct v2 offset = { -8, -2 };
				} else {
					struct v2 zero = { 0 };
					view.camera_target_tile = zero;
				}
				
				{
					// Uzzīmēt karti
					int row, col;

					for (row = 0; row < ARENA_HEIGHT_IN_TILES; ++row) {
						for (col = 0; col < ARENA_WIDTH_IN_TILES; ++col) {
							char ch = get_arena(row, col), draw_ch = ch, fill_ch = ' ';
							switch (ch) {
								case 'x':
									draw_ch = ' ';
									attron(COLOR_PAIR(WALL_PAIR));
									break;
								case ' ':
									attron(COLOR_PAIR(COLOR_BLACK));
									break;
								case '-':
								case '|':
								case '+':
									attron(COLOR_PAIR(WALL_PAIR));
									break;
							}
							// ja '-', ieliek vēlvienu '-', lai labāk izskatās
							if (ch == '-') {
								fill_ch = ch;
							}

							draw_tile(row, col, draw_ch, &view, fill_ch);
						}
					}
				}

				{
					attron(COLOR_PAIR(FOOD_PAIR));
					int row, col;
					for (row = 0; row < ARENA_HEIGHT_IN_TILES; ++row) {
						for (col = 0; col < ARENA_WIDTH_IN_TILES; ++col) {
							char ch = games[current_player].level.dot_map[row][col];
							if (ch == '.' || ch == '*') {
								draw_tile(row, col, ch == '.' ? ch : '@', &view, ' ');
							}
						}
					}
				}

				attron(COLOR_PAIR(PLAYER_PAIR));

				char player_symbol = 'A';

				// uzzīmē spēlētāja simbolu uz kartes
				struct v2 player_tile = pos_to_tile(&games[current_player].level.player_pos);
				if (player_tile.x >= 0 && player_tile.x < ARENA_WIDTH_IN_TILES) {
					draw_tile_v2(player_tile, player_symbol, &view, ' ');
				}

				// statuss
				if (games[current_player].mode == PAUSED_BEFORE_PLAYING) {
					attron(COLOR_PAIR(PLAYER_PAIR));
					struct v2 pos = draw_pos(14, 16, &view);
					mvprintw(pos.y, pos.x, "R E A D Y !");
				}

				// spēles statuss
				attron(COLOR_PAIR(BG_PAIR));
				mvprintw(view.top - 2, view.left + 2, "Score");
				mvprintw(view.top - 2, view.left + 7, "%4d",  games[0].score);

				attron(COLOR_PAIR(BG_PAIR));
				int diag_row = view.top + ARENA_HEIGHT_IN_TILES + 1;
			}

			refresh();

			frame_timer = 0;
		}
		usleep(1);
	}

	endwin();

	return 0;
}

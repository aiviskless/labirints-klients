#include "libs.h"
#include "helpers.c"
#include "consts.h"
#include "maps.c"

struct pair TILE_CENTER = { 3*PIXEL_SIZE, 4*PIXEL_SIZE };

struct window {
	int top, left;
	struct pair camera_target_tile;
};


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

struct pair draw_pos(int row, int col, struct window* window) {
	struct pair ret = {
		3 * (col - window->camera_target_tile.x) + window->left,
		(row - window->camera_target_tile.y) + window->top,
	};
	return ret;
}

struct pair draw_pos_pair(struct pair p, struct window* window) {
	return draw_pos(p.y, p.x, window);
}

void draw_tile(int row, int col, char ch, struct window* window, char fill_ch) {
	struct pair pos = draw_pos(row, col, window);
	mvaddch(pos.y, pos.x, fill_ch);
	mvaddch(pos.y, pos.x + 1, ch);
	mvaddch(pos.y, pos.x + 2, fill_ch);
}

void draw_tile_pair(struct pair tile_pos, char ch, struct window* window, char fill_ch) {
	draw_tile(tile_pos.y, tile_pos.x, ch, window, fill_ch);
}

bool is_direction_horizontal(enum dir dir) {
	return dir == LEFT || dir == RIGHT;
}

void move_in_dir(enum dir dir, struct pair* pos, int amt) {
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

struct pair pos_to_tile(struct pair* p) {
	return div_scale(*p, TILE_SIZE);
}

bool test_player_dir_blocked(enum dir dir, struct pair* next_pos) {
	int mv_amt = TILE_SIZE / 2;
	if (dir == RIGHT || dir == UP) {
		mv_amt += PIXEL_SIZE;
	}
	move_in_dir(dir, next_pos, mv_amt);
	struct pair next_tile = pos_to_tile(next_pos);
	// move chars to const
	return !!strchr(ARENA_WALLS, get_arena(next_tile.y, next_tile.x));
}

int seconds_to_frames(int seconds) {
	return SECONDS(seconds) / MS(16);
}

struct game_data {
	int score;
	enum {
		PAUSED_BEFORE_PLAYING,
		PLAYING,
	} mode;

	struct level {
		bool player_blocked, player_turning;

		struct pair player_pos;
		enum dir player_dir, next_dir;
	} level;
};

void set_player_to_start_position(struct level* level) {
	struct pair player_start_pos = { ARENA_WIDTH / 2, (ARENA_HEIGHT_IN_TILES - 2)*TILE_SIZE + TILE_CENTER.y };

	level->player_pos = player_start_pos;
	level->player_dir = LEFT;
	level->player_blocked = FALSE;
	level->player_turning = FALSE;
	level->next_dir = level->player_dir;
}

void start_new_level(struct game_data* game_data) {
	game_data->level = new_level;
	set_player_to_start_position(&game_data->level);
}

struct game_data create_new_game() {
	struct game_data game_data = {};

	start_new_level(&game_data);

	return game_data;
}

int start_game() {
	// Init curses
	initscr();
	cbreak();
	keypad(stdscr, TRUE);
	noecho();
	curs_set(FALSE);
	timeout(0);

	int row, col;

	// kontrolē PAUSED_BEFORE_PLAYING ilgumu
	int transition_timer = 0;

	bool running = TRUE;

	int last_update = time_in_us();
	int frame_timer = 0;
	int next_input = -1;
	struct window window = { 0 };

	// uzstāda krāsas jeb krāsu pārus
	start_color();

	// init_pair(index, foreground, background);
	init_pair(BG_PAIR, COLOR_WHITE, COLOR_BLACK_DARK);
	init_pair(PLAYER_PAIR, COLOR_YELLOW_BRIGHT, COLOR_GRAY);
	init_pair(FOOD_PAIR, COLOR_ORANGE, COLOR_GRAY);
	init_pair(WALL_PAIR, COLOR_WHITE, COLOR_GRAY);

	bkgd(COLOR_PAIR(BG_PAIR));

	struct game_data game = create_new_game();
	struct pair player_turn_tile;

	game.mode = PAUSED_BEFORE_PLAYING;

	int frame_time = 16667;
	while (running) {
		// Iegūt input
		int ch;
		while ((ch = getch()) != -1) {
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

		if (!game.level.player_turning && next_input != -1) {
			game.level.next_dir = next_input;
			next_input = -1;
		}

		frame_timer += time_in_us() - last_update;
		last_update = time_in_us();

		if (frame_timer > frame_time) {
			switch (game.mode) {
				case PAUSED_BEFORE_PLAYING: {
					int time = seconds_to_frames(2);
					transition_timer++;
					if (transition_timer >= time) {
						transition_timer = 0;
						game.mode = PLAYING;
					}
				} break;
				case PLAYING: {
					int old_score = game.score;
					
					if (game.level.player_dir != game.level.next_dir) {
						if (is_direction_horizontal(game.level.player_dir) == is_direction_horizontal(game.level.next_dir)) {
							game.level.player_dir = game.level.next_dir;
						} else {
							struct pair blocked_pos_s = game.level.player_pos;
							bool blocked = test_player_dir_blocked(game.level.next_dir, &blocked_pos_s);
							if (!blocked) {
								game.level.player_turning = TRUE;
								player_turn_tile = pos_to_tile(&game.level.player_pos);
							}
						}
					}

					// kustība
					if (game.level.player_turning) {
						move_in_dir(game.level.next_dir, &game.level.player_pos, PLAYER_SPEED);
						struct pair tile_center = add(scale(player_turn_tile, TILE_SIZE), TILE_CENTER);
						bool reached_centerline = FALSE;
						if (is_direction_horizontal(game.level.player_dir)) {
							int mv_amt = tile_center.x - game.level.player_pos.x;
							if (mv_amt < 0) {
								game.level.player_pos.x -= MIN(-mv_amt, PLAYER_SPEED);
								reached_centerline = -mv_amt < PLAYER_SPEED;
							} else {
								game.level.player_pos.x += MIN(mv_amt, PLAYER_SPEED);
								reached_centerline = mv_amt < PLAYER_SPEED;
							}
						} else {
							int mv_amt = tile_center.y - game.level.player_pos.y;
							if (mv_amt < 0) {
								game.level.player_pos.y -= MIN(-mv_amt, PLAYER_SPEED);
								reached_centerline = -mv_amt < PLAYER_SPEED;
							} else {
								game.level.player_pos.y += MIN(mv_amt, PLAYER_SPEED);
								reached_centerline = mv_amt < PLAYER_SPEED;
							}
						}

						if (reached_centerline) { 
							game.level.player_turning = FALSE;
							game.level.player_dir = game.level.next_dir;
						}
					} else {
						struct pair blocked_pos_s = game.level.player_pos;
						game.level.player_blocked = test_player_dir_blocked(game.level.player_dir, &blocked_pos_s);
						if (!game.level.player_blocked) {
							move_in_dir(game.level.player_dir, &game.level.player_pos, PLAYER_SPEED);
						}
					}

					// pārbauda, vai uzkāpj uz ēdiena
					struct pair player_tile = pos_to_tile(&game.level.player_pos);
					char ch = get_arena(player_tile.y, player_tile.x);
					if (ch == '.') {
						// noņem ēdienu
						arena_map[player_tile.y*ARENA_WIDTH_IN_TILES + player_tile.x] = ' ';
						
						game.score += 10;
					}
				} break;
			}

			erase();

			// nobīde no malām
			window.left = 3;
			window.top = 3;

			// uzzīmēt karti
			for (row = 0; row < ARENA_HEIGHT_IN_TILES; ++row) {
				for (col = 0; col < ARENA_WIDTH_IN_TILES; ++col) {
					char ch = get_arena(row, col), draw_ch = ch, fill_ch = ' ';
					switch (ch) {
						case '.':
							attron(COLOR_PAIR(FOOD_PAIR));
							break;
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
					// ja '-', ieliek vēl vienu '-', lai labāk izskatās
					if (ch == '-') {
						fill_ch = ch;
					}

					draw_tile(row, col, draw_ch, &window, fill_ch);
				}
			}

			attron(COLOR_PAIR(PLAYER_PAIR));

			char player_symbol = 'A';

			// uzzīmē spēlētāja simbolu uz kartes
			struct pair player_tile = pos_to_tile(&game.level.player_pos);
			if (player_tile.x >= 0 && player_tile.x < ARENA_WIDTH_IN_TILES) {
				draw_tile_pair(player_tile, player_symbol, &window, ' ');
			}

			// spēles sākuma statuss
			if (game.mode == PAUSED_BEFORE_PLAYING) {
				struct pair pos = draw_pos(14, 16, &window);
				mvprintw(pos.y, pos.x, "R E A D Y !");
			}

			// spēles statuss
			attron(COLOR_PAIR(BG_PAIR));
			mvprintw(window.top - 2, window.left + 2, "Score");
			mvprintw(window.top - 2, window.left + 7, "%4d",  game.score);

			int diag_row = window.top + ARENA_HEIGHT_IN_TILES + 1;

			refresh();

			frame_timer = 0;
		}
		usleep(1);
	}

	endwin();

	return 0;
}
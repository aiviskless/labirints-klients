#include "libs.h"

void* memset(void* s, int c, size_t n);
void* memcpy(void* d, void* s, size_t n);
char* strchr(const char* s, int c);

#define internal static
#define global_variable static
#define local_persist static
#define UNUSED(x) (void)(x);
#define MIN(x,y) ((x) < (y) ? (x) : (y))
#define MAX(x,y) ((x) < (y) ? (y) : (x))

#define ARRAY_LENGTH(arr)\
	(sizeof(arr) / sizeof(arr[0]))

#define for_array(index, arr)\
	for (index = 0; index < ARRAY_LENGTH(arr); ++index)

#define MS(x) (1000*(x))
#define SECONDS(x) (MS(1000*(x)))

typedef int8_t int8;
typedef uint8_t uint8;
typedef int16_t int16;
typedef uint16_t uint16;
typedef int32_t int32;
typedef uint32_t uint32;
typedef int64_t int64;
typedef uint64_t uint64;

typedef float real32;
typedef double real64;
typedef int32_t bool32;
typedef char* c_str;

struct v2 {
	int32 x, y;
};

inline internal struct v2
add(struct v2 a, struct v2 b) {
	a.x += b.x;
	a.y += b.y;
	return a;
}

inline internal struct v2
sub(struct v2 a, struct v2 b) {
	a.x -= b.x;
	a.y -= b.y;
	return a;
}

inline internal bool32
eql(struct v2* a, struct v2* b) {
	return a->x == b->x && a->y == b->y;
}

inline internal struct v2
scale(struct v2 a, int32 amt) {
	a.x *= amt;
	a.y *= amt;
	return a;
}

inline internal int32
floor_div(int32 num, int32 den) {
	double fresult = (double)num / (double)den;

	int32 result = (int32)fresult;
	if (fresult < (double)result) {
		--result;
	}

	return result;
}

inline internal struct v2
div_scale(struct v2 a, int32 amt) {

	a.x = floor_div(a.x, amt);
	a.y = floor_div(a.y, amt);

	return a;
}

uint32
time_in_us() {
	struct timeval t;
	gettimeofday(&t, NULL);
	return t.tv_sec * 1000000 + t.tv_usec;
}

#define SQUARE(x) ((x)*(x))
#define DIST_SQUARE(x, y) (SQUARE(x) + SQUARE(y))

#define PIXEL_SIZE 40 
#define TILE_SIZE_IN_PIXELS 8
#define TILE_SIZE (PIXEL_SIZE * TILE_SIZE_IN_PIXELS)

#define LIFE_LOST_PAUSE_TIME 30
#define LIFE_LOST_TURN_TIME 12

struct v2 TILE_CENTER_IN_PIXELS = { 3, 4 };
struct v2 TILE_CENTER = { 3*PIXEL_SIZE, 4*PIXEL_SIZE };

#define NUM_DIRS 4

struct view {
	int top, left;
	struct v2 camera_target_tile;
	bool32 zoom_view;
};

#define ARENA_WIDTH_IN_TILES 28
#define ARENA_HEIGHT_IN_TILES 31

#define HOUSE_CENTER (ARENA_WIDTH_IN_TILES / 2)
#define HOUSE_LEFT (HOUSE_CENTER - 3)
#define HOUSE_RIGHT (HOUSE_CENTER + 2)
#define HOUSE_TOP 13
#define HOUSE_BOTTOM 15

#define ARENA_HEIGHT (ARENA_HEIGHT_IN_TILES*TILE_SIZE)
#define ARENA_WIDTH (ARENA_WIDTH_IN_TILES*TILE_SIZE)

global_variable
struct v2 top_house_targets[] = {
	{ 0 },
	{ HOUSE_CENTER, HOUSE_TOP },
	{ HOUSE_LEFT, HOUSE_TOP },
	{ HOUSE_RIGHT, HOUSE_TOP },
};
global_variable
struct v2 bottom_house_targets[] = {
	{ HOUSE_CENTER, HOUSE_BOTTOM },
	{ HOUSE_CENTER, HOUSE_BOTTOM },
	{ HOUSE_LEFT, HOUSE_BOTTOM },
	{ HOUSE_RIGHT, HOUSE_BOTTOM },
};
global_variable
struct v2 forbidden_upward_tiles[] = {
	{ 12, 10 },
	{ 15, 10 },
	{ 12, 22 },
	{ 15, 22 },
};
global_variable
struct v2 fruit_tile = { HOUSE_CENTER, HOUSE_BOTTOM + 2 };

global_variable
struct v2 eyes_target_tile = { ARENA_WIDTH_IN_TILES / 2 - 1, 11 };

typedef char dot_map_t[ARENA_HEIGHT_IN_TILES][ARENA_WIDTH_IN_TILES];

global_variable
dot_map_t new_dot_map;


/* Corners:
 * top-left: /
 * top-right: `
 * bottom-left: [
 * bottom-right: ]
 */
const
char arena[ARENA_HEIGHT_IN_TILES*ARENA_WIDTH_IN_TILES] = 
"/------------`/------------`"
"|            ||            |"
"| /--` /---` || /---` /--` |"
"| |xx| |xxx| || |xxx| |xx| |"
"| [--] [---] [] [---] [--] |"
"|                          |"
"| /--` /` /------` /` /--` |"
"| [--] || [--`/--] || [--] |"
"|      ||    ||    ||      |"
"[----` |[--` || /--]| /----]"
"xxxxx| |/--] [] [--`| |xxxxx"
"xxxxx| ||          || |xxxxx"
"xxxxx| || +------+ || |xxxxx"
"xxxxx| [] |xxxxxx| [] |xxxxx"
"xxxxx|    |xxxxxx|    |xxxxx"
"xxxxx| /` |xxxxxx| /` |xxxxx"
"xxxxx| || +------+ || |xxxxx"
"xxxxx| ||          || |xxxxx"
"xxxxx| || /------` || |xxxxx"
"/----] [] [--`/--] [] [----`"
"|            ||            |"
"| /--` /---` || /---` /--` |"
"| [-`| [---] [] [---] |/-] |"
"|   ||                ||   |"
"[-` || /` /------` /` || /-]"
"/-] [] || [--`/--] || [] [-`"
"|      ||    ||    ||      |"
"| /----][--` || /--][----` |"
"| [--------] [] [--------] |"
"|                          |"
"[--------------------------]"
;

const
char dot_placement_map[ARENA_HEIGHT_IN_TILES*ARENA_WIDTH_IN_TILES] = 
"+------------++------------+"
"|   .     .  ||     .      |"
"| +--+ +---+ || +---+ +--+ |"
"| |  | |   | || |   |.|  |.|"
"| +--+ +---+ ++ +---+ +--+ |"
"|   .                     .|"
"| +--+ ++ +------+ ++ +--+ |"
"| +--+.|| +--++--+ || +--+ |"
"|   .  ||    || .  ||      |"
"+----+ |+--+.|| +--+| +----+"
"     |.|+--+ ++ +--+|.|     "
"     | ||  .       || |     "
"     | || +------+ || |     "
"     | ++ |      | ++ |     "
"     |    |      |.   |     "
"     | ++ |      | ++ |     "
"     | || +------+.|| |     "
"     |.||  .       || |     "
"     | || +------+ || |     "
"+----+ ++ +--++--+ ++ +----+"
"|    .       ||      .     |"
"| +--+.+---+ || +---+ +--+ |"
"| +-+| +---+ ++ +---+ |+-+ |"
"|.  ||         .      ||   |"
"+-+ || ++ +------+ ++ || +-+"
"+-+ ++ || +--++--+.|| ++ +-+"
"|   .  ||    ||    ||      |"
"| +----++--+ || +--++----+ |"
"| +--------+.++ +--------+ |"
"|.           .      .      |"
"+--------------------------+"
;

const
struct level new_level;

enum {
	BG_PAIR = 1,
	PACMAN_PAIR,
	DOT_PAIR,
	GAME_OVER_TEXT_PAIR,
	BLINKY_PAIR,
	INKY_PAIR,
	PINKY_PAIR,
	CLYDE_PAIR,
	FRIGHT_PAIR,
	FRIGHT_FLASH_PAIR,
	EYES_PAIR,
	EMPTY_PAIR,
	DARK_WALL_PAIR,
	LIGHT_WALL_PAIR,
	DOOR_PAIR,

	CHERRIES_PAIR,
	STRAWBERRY_PAIR,
	PEACH_PAIR,
	APPLE_PAIR,
	GRAPES_PAIR,
	GALAXIAN_PAIR,
	BELL_PAIR,
	KEY_PAIR,

	CHERRIES_STEM_PAIR,
	METAL_PAIR,
	GALAXIAN_WING_PAIR,

	PLAYER_TEXT_PAIR,
};

internal char
arena_get(int row, int col) {
	if (col < 0) {
		col += ARENA_WIDTH_IN_TILES;
	} else if (col >= ARENA_WIDTH_IN_TILES) {
		col -= ARENA_WIDTH_IN_TILES;
	}
	return arena[row*ARENA_WIDTH_IN_TILES + col];
}

enum dir {
	UP,
	LEFT,
	DOWN,
	RIGHT
};
char* dir_names[] = {
	"UP",
	"LEFT",
	"DOWN",
	"RIGHT"
};

#define DRAW_SIZE 8

internal struct v2
draw_pos(int row, int col, struct view* view) {
	struct v2 ret = {
		3 * (col - view->camera_target_tile.x) + view->left,
		(row - view->camera_target_tile.y) + view->top,
	};
	return ret;
}

internal struct v2
draw_pos_v2(struct v2 p, struct view* view) {
	return draw_pos(p.y, p.x, view);
}

internal void
draw_tile(int row, int col, char ch, struct view* view, char fill_ch) {
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

internal void
draw_tile_v2(struct v2 tile_pos, char ch, struct view* view, char fill_ch) {
	draw_tile(tile_pos.y, tile_pos.x, ch, view, fill_ch);
}

enum {
	COLOR_LIGHT_BLUE,
	COLOR_PINK,
	COLOR_ORANGE,
	COLOR_DARK_RED,
	COLOR_BRIGHT_RED,
	COLOR_GRAPE_GREEN,
	COLOR_DOOR_RED,
	COLOR_PACMAN_YELLOW,
	COLOR_WALL_BLUE,
	COLOR_DARK_BACKGROUND,
	COLOR_GREY,
	COLOR_LIGHT_GREY,
	COLOR_STEM,
	NUM_COLORS
};

const char* DIR_NAMES[] = {
	"Up", "Down", "Left", "Right"
};

internal bool32
is_h_dir(enum dir dir) {
	return dir == LEFT || dir == RIGHT;
}

internal void
move_in_dir(enum dir dir, struct v2* pos, int amt) {
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

internal struct v2
pos_to_tile(struct v2* p) {
	return div_scale(*p, TILE_SIZE);
}

int num_blocked_tiles = 0;
struct v2 blocked_tiles[20];

internal bool32
test_pacman_dir_blocked(enum dir dir, struct v2* next_pos) {
	int mv_amt = TILE_SIZE / 2;
	if (dir == RIGHT || dir == UP) {
		mv_amt += PIXEL_SIZE;
	}
	move_in_dir(dir, next_pos, mv_amt);
	struct v2 next_tile = pos_to_tile(next_pos);
	return !!strchr("/`[]-|+_", arena_get(next_tile.y, next_tile.x));
}

inline internal uint32
get_speed(int percentage) {
	return (percentage * PIXEL_SIZE / 100) * 6 / 5;
}

enum bonus_symbol {
	CHERRIES,
	STRAWBERRY,
	PEACH,
	APPLE,
	GRAPES,
	GALAXIAN,
	BELL,
	KEY
};

inline internal uint32
seconds_to_frames(uint32 seconds) {
	return SECONDS(seconds) / MS(16);
}

uint32 symbol_points[] = {
	100, 300, 500, 700, 1000, 2000, 3000, 5000
};

internal enum bonus_symbol
get_symbol_for_level(uint32 level) {
	enum bonus_symbol level_symbols[] = {
		CHERRIES, STRAWBERRY, PEACH, PEACH, APPLE, APPLE, GRAPES, GRAPES,
		GALAXIAN, GALAXIAN, BELL, BELL
	};
	if (level < ARRAY_LENGTH(level_symbols)) {
		return level_symbols[level];
	} else {
		return KEY;
	}
}

struct game_data {
	/* Data that persists between levels, but not between games. */
	uint32 num_extra_lives;
	uint32 current_level;
	uint32 score;
	enum {
		PAUSED_BEFORE_PLAYING,
		GAME_OVER,
		PLAYING,
		LOSING_A_LIFE,
		LEVEL_TRANSITION,
	} mode;

	/* Data that describes the current state of a level */
	struct level {
		uint32 pacman_chomp_timer, pacman_eat_timer,
					pacman_powerup_timer,
					fruit_timer,
					fruit_score_timer,
					extra_life_timer;
		bool32 fruit_is_visible, pacman_blocked, pacman_turning, is_chasing,
					should_flash_extra_life;
		uint32 dots_eaten; 

		struct v2 pacman_pos;
		enum dir pacman_dir, next_dir;
		uint32 dot_timer;

		dot_map_t dot_map;
	} level;

	/* Data that describes the constants for the current level */
	struct level_constants {
		enum bonus_symbol bonus_symbol;
		uint32 pacman_speed, pacman_powerup_speed;
		uint32 pacman_powerup_time;
		uint32 elroy_v1_dots_left, elroy_v1_speed;

		uint32 scatter_times[4];
		uint32 chase_times[3];

	} level_constants;
};

internal void
set_level_constants(struct level_constants* level_constants, uint32 level) {
	level_constants->bonus_symbol = get_symbol_for_level(level);

	level_constants->pacman_speed = get_speed(80);
	level_constants->pacman_powerup_speed = get_speed(90);
}



#define TUNNEL_WIDTH (4*TILE_SIZE)

#define NUM_DOTS 244

internal void 
set_player_to_start_position(struct level* level) {
	struct v2 pacman_start_pos = { ARENA_WIDTH / 2, (ARENA_HEIGHT_IN_TILES - 8)*TILE_SIZE + TILE_CENTER.y };

	level->pacman_pos = pacman_start_pos;
	level->pacman_dir = LEFT;
	level->pacman_blocked = FALSE;
	level->pacman_turning = FALSE;
	level->next_dir = level->pacman_dir;
	level->dot_timer = 0;
}

internal void
start_new_level(struct game_data* game_data) {
	game_data->level = new_level;
	memcpy(game_data->level.dot_map, new_dot_map, sizeof(new_dot_map));
	set_player_to_start_position(&game_data->level);
}

#ifdef MOM
#define NUM_LIVES 5
#else
#define NUM_LIVES 2
#endif

const
struct game_data new_game = {
	.num_extra_lives = NUM_LIVES
};

internal struct game_data
create_new_game() {
	struct game_data game_data = new_game;

	set_level_constants(&game_data.level_constants, 0);

	start_new_level(&game_data);
	game_data.level.should_flash_extra_life = TRUE;
	return game_data;
}

global_variable
bool32 has_color_terminal;

internal void
safe_attron(int color_pair) {
	if (has_color_terminal) {
		attron(color_pair);
	}
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
				char ch = dot_placement_map[row*ARENA_WIDTH_IN_TILES + col];
				if (ch == '.' || ch == '*' || ch == '@') {
					new_dot_map[row][col] = ch;
				}
			}
		}
	}

	uint32 transition_timer = 0;
	bool32 running = TRUE;

	uint32 last_update = time_in_us();
	uint32 frame_timer = 0;
	uint32 high_score = 0;
	int32 next_input = -1;
	bool32 is_two_player = FALSE;
	int current_player = 0;
	struct view view = { 0 };

	has_color_terminal = has_colors();
	uint32 fruit_time = 0;
	start_color();
	int term_colors[NUM_COLORS] = { 0 };
	if (!has_color_terminal) {
		mvprintw(0, 0, "Warning: Colors not enabled for this terminal.\nProgrammer's suggestion: try running in the unicode-rxvt terminal with 256 color support.\nPress any key to continue.");
		timeout(-1);
		getch();
		timeout(0);
	} else if (COLORS == 256) {
		term_colors[COLOR_LIGHT_BLUE] = 51;
		term_colors[COLOR_PINK] = 197;
		term_colors[COLOR_ORANGE] = 172;
		term_colors[COLOR_DARK_RED] = 88;
		term_colors[COLOR_BRIGHT_RED] = 196;
		term_colors[COLOR_GRAPE_GREEN] = 76;
		term_colors[COLOR_DOOR_RED] = 124;
		term_colors[COLOR_PACMAN_YELLOW] = 226;
		term_colors[COLOR_WALL_BLUE] = 21;
		term_colors[COLOR_DARK_BACKGROUND] = 232;
		term_colors[COLOR_GREY] = 233;
		term_colors[COLOR_LIGHT_GREY] = 252;
		term_colors[COLOR_STEM] = 94;
	} else {
		mvprintw(0, 0, "WARNING: %u colors available in this terminal. This game currently looks best in 256 colors.", COLORS);
		timeout(-1);
		getch();
		timeout(0);

		term_colors[COLOR_LIGHT_BLUE] = COLOR_WHITE;
		term_colors[COLOR_PINK] = COLOR_MAGENTA;
		term_colors[COLOR_ORANGE] = COLOR_YELLOW;
		term_colors[COLOR_DARK_RED] = COLOR_RED;
		term_colors[COLOR_BRIGHT_RED] = COLOR_RED;
		term_colors[COLOR_GRAPE_GREEN] = COLOR_GREEN;
		term_colors[COLOR_DOOR_RED] = COLOR_RED;
		term_colors[COLOR_PACMAN_YELLOW] = COLOR_YELLOW;
		term_colors[COLOR_WALL_BLUE] = COLOR_BLUE;
		term_colors[COLOR_DARK_BACKGROUND] = COLOR_BLACK;
		term_colors[COLOR_GREY] = COLOR_BLACK;
		term_colors[COLOR_LIGHT_GREY] = COLOR_WHITE;
		term_colors[COLOR_STEM] = COLOR_RED;
	}
	if (has_color_terminal) {
		init_pair(BG_PAIR, COLOR_WHITE, term_colors[COLOR_DARK_BACKGROUND]);

		int bkgd_color = term_colors[COLOR_GREY];

		init_pair(EMPTY_PAIR, COLOR_WHITE, bkgd_color);
		init_pair(DARK_WALL_PAIR, term_colors[COLOR_WALL_BLUE], term_colors[COLOR_DARK_BACKGROUND]);
		init_pair(LIGHT_WALL_PAIR, term_colors[COLOR_LIGHT_BLUE], term_colors[COLOR_DARK_BACKGROUND]);
		init_pair(DOOR_PAIR, term_colors[COLOR_DOOR_RED], term_colors[COLOR_DARK_BACKGROUND]);
		init_pair(PACMAN_PAIR, term_colors[COLOR_PACMAN_YELLOW], bkgd_color);
		init_pair(DOT_PAIR, COLOR_WHITE, bkgd_color);

		init_pair(GAME_OVER_TEXT_PAIR, term_colors[COLOR_DOOR_RED], bkgd_color);

		init_pair(BLINKY_PAIR, COLOR_WHITE, term_colors[COLOR_DOOR_RED]);
		init_pair(INKY_PAIR, COLOR_WHITE, term_colors[COLOR_LIGHT_BLUE]);
		init_pair(PINKY_PAIR, COLOR_WHITE, term_colors[COLOR_PINK]);
		init_pair(CLYDE_PAIR, COLOR_WHITE, term_colors[COLOR_ORANGE]);
		init_pair(FRIGHT_PAIR, COLOR_WHITE, term_colors[COLOR_WALL_BLUE]);
		init_pair(FRIGHT_FLASH_PAIR, term_colors[COLOR_WALL_BLUE], COLOR_WHITE);
		init_pair(EYES_PAIR, COLOR_WHITE, bkgd_color);

		init_pair(CHERRIES_PAIR, term_colors[COLOR_DARK_RED], bkgd_color);
		init_pair(STRAWBERRY_PAIR, term_colors[COLOR_BRIGHT_RED], bkgd_color);
		init_pair(PEACH_PAIR, term_colors[COLOR_ORANGE], bkgd_color);
		init_pair(APPLE_PAIR, term_colors[COLOR_DOOR_RED], bkgd_color);
		init_pair(GRAPES_PAIR, term_colors[COLOR_GRAPE_GREEN], bkgd_color);
		init_pair(GALAXIAN_PAIR, term_colors[COLOR_ORANGE], bkgd_color);
		init_pair(BELL_PAIR, term_colors[COLOR_PACMAN_YELLOW], bkgd_color);
		init_pair(KEY_PAIR, term_colors[COLOR_LIGHT_BLUE], bkgd_color);

		init_pair(CHERRIES_STEM_PAIR, term_colors[COLOR_STEM], bkgd_color);
		init_pair(METAL_PAIR, term_colors[COLOR_LIGHT_GREY], bkgd_color);
		init_pair(GALAXIAN_WING_PAIR, term_colors[COLOR_WALL_BLUE], bkgd_color);

		init_pair(PLAYER_TEXT_PAIR, term_colors[COLOR_LIGHT_BLUE], bkgd_color);


		bkgd(COLOR_PAIR(BG_PAIR));
	}

	struct game_data games[2] = { create_new_game(), create_new_game() };
	struct v2 pacman_turn_tile;

	games[current_player].mode = GAME_OVER;

	uint32 frame_times[] = { 8000, 16667, 50000, 100000, 150000 };
	int frame_time_index = 1;
	while (running) {
		// Iegūt input
		int ch;
		while ((ch = getch()) != -1) {
			if (games[current_player].mode == GAME_OVER) {
				current_player = 0;
				transition_timer = 0;
				games[0] = create_new_game();
				is_two_player = FALSE;
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

		if (!games[current_player].level.pacman_turning && next_input != -1) {
			games[current_player].level.next_dir = next_input;
			next_input = -1;
		}

		frame_timer += time_in_us() - last_update;
		last_update = time_in_us();

		if (frame_timer > frame_times[frame_time_index]) {
			++games[current_player].level.pacman_chomp_timer;
			if (games[current_player].level.fruit_is_visible) {
				++games[current_player].level.fruit_timer;
				if (games[current_player].level.fruit_timer > fruit_time) {
					games[current_player].level.fruit_is_visible = FALSE;
				}
			}

			switch (games[current_player].mode) {
				case PAUSED_BEFORE_PLAYING: {
					uint32 time = is_two_player ? seconds_to_frames(4) : seconds_to_frames(2);
					transition_timer++;
					if (transition_timer >= time) {
						transition_timer = 0;
						games[current_player].mode = PLAYING;
					}
				} break;
				case PLAYING: {
					uint32 pacman_speed;
					uint32 old_score = games[current_player].score;
					num_blocked_tiles = 0;
					
					pacman_speed = games[current_player].level_constants.pacman_speed;
					if (games[current_player].level.pacman_dir != games[current_player].level.next_dir) {
						if (is_h_dir(games[current_player].level.pacman_dir) == is_h_dir(games[current_player].level.next_dir)) {
							games[current_player].level.pacman_dir = games[current_player].level.next_dir;
						} else {
							struct v2 blocked_pos_s = games[current_player].level.pacman_pos;
							bool32 blocked = test_pacman_dir_blocked(games[current_player].level.next_dir, &blocked_pos_s);
							if (blocked) {
								blocked_tiles[num_blocked_tiles++] = pos_to_tile(&blocked_pos_s);
							} else {
								games[current_player].level.pacman_turning = TRUE;
								pacman_turn_tile = pos_to_tile(&games[current_player].level.pacman_pos);
							}
						}
					}

					// kustība
					if (games[current_player].level.pacman_turning) {
						move_in_dir(games[current_player].level.next_dir, &games[current_player].level.pacman_pos, pacman_speed);
						struct v2 tile_center = add(scale(pacman_turn_tile, TILE_SIZE), TILE_CENTER);
						bool32 reached_centerline = FALSE;
						if (is_h_dir(games[current_player].level.pacman_dir)) {
							int mv_amt = tile_center.x - games[current_player].level.pacman_pos.x;
							if (mv_amt < 0) {
								games[current_player].level.pacman_pos.x -= MIN(-mv_amt, pacman_speed);
								reached_centerline = -mv_amt < pacman_speed;
							} else {
								games[current_player].level.pacman_pos.x += MIN(mv_amt, pacman_speed);
								reached_centerline = mv_amt < pacman_speed;
							}
						} else {
							int mv_amt = tile_center.y - games[current_player].level.pacman_pos.y;
							if (mv_amt < 0) {
								games[current_player].level.pacman_pos.y -= MIN(-mv_amt, pacman_speed);
								reached_centerline = -mv_amt < pacman_speed;
							} else {
								games[current_player].level.pacman_pos.y += MIN(mv_amt, pacman_speed);
								reached_centerline = mv_amt < pacman_speed;
							}
						}

						if (reached_centerline) { 
							games[current_player].level.pacman_turning = FALSE;
							games[current_player].level.pacman_dir = games[current_player].level.next_dir;
						}
					} else {
						struct v2 blocked_pos_s = games[current_player].level.pacman_pos;
						games[current_player].level.pacman_blocked = test_pacman_dir_blocked(games[current_player].level.pacman_dir, &blocked_pos_s);
						if (games[current_player].level.pacman_blocked) {
							blocked_tiles[num_blocked_tiles++] = pos_to_tile(&blocked_pos_s);
						} else {
							move_in_dir(games[current_player].level.pacman_dir, &games[current_player].level.pacman_pos, pacman_speed);
							if (games[current_player].level.pacman_pos.x < -TUNNEL_WIDTH / 2) {
								games[current_player].level.pacman_pos.x += ARENA_WIDTH + TUNNEL_WIDTH;
							} else if (games[current_player].level.pacman_pos.x >= ARENA_WIDTH + TUNNEL_WIDTH / 2) {
								games[current_player].level.pacman_pos.x -= ARENA_WIDTH + TUNNEL_WIDTH;
							}
						}
					}

					{
						struct v2 pacman_tile = pos_to_tile(&games[current_player].level.pacman_pos);
						char ch = games[current_player].level.dot_map[pacman_tile.y][pacman_tile.x];
						if (ch == '.') {
							games[current_player].level.pacman_eat_timer = 1;
							games[current_player].score += 10;
						} else if (ch == '*') {
							games[current_player].level.pacman_eat_timer = 3;
							games[current_player].level.pacman_powerup_timer = games[current_player].level_constants.pacman_powerup_time;
							games[current_player].score += 50;
						}
						if (ch == '.' || ch == '*') {
							uint32 first_fruit_dot_counter = 10, second_fruit_dot_counter = 20;
							games[current_player].level.dot_timer = 0;
							++games[current_player].level.dots_eaten;

							// nosaka kad spawno ēdienu
							if (1 == 1) {
								games[current_player].level.fruit_is_visible = TRUE;
								games[current_player].level.fruit_timer = 0;
								fruit_time = seconds_to_frames(9) + (rand() % seconds_to_frames(1));
							}
						}
						games[current_player].level.dot_map[pacman_tile.y][pacman_tile.x] = 0;
					}

					{
						struct v2 pacman_tile = pos_to_tile(&games[current_player].level.pacman_pos);
						// ja spēlētājs ir uzkāpis uz ēdiena
						if (games[current_player].level.fruit_is_visible && (eql(&pacman_tile, &fruit_tile))) {
							games[current_player].level.fruit_is_visible = FALSE;
							games[current_player].level.fruit_score_timer = seconds_to_frames(2);
							games[current_player].score += symbol_points[games[current_player].level_constants.bonus_symbol];
						}
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
				uint32 flash_time = 10;
				int wall_pair;
				if (games[current_player].level.extra_life_timer && games[current_player].level.extra_life_timer ||
						(games[current_player].mode == LEVEL_TRANSITION && transition_timer / flash_time % 2 == 1)) {
					wall_pair = LIGHT_WALL_PAIR;
				} else {
					wall_pair = DARK_WALL_PAIR;
				}
				{
					// Uzzīmēt karti
					int row, col;

					for (row = 0; row < ARENA_HEIGHT_IN_TILES; ++row) {
						for (col = 0; col < ARENA_WIDTH_IN_TILES; ++col) {
							char ch = arena_get(row, col), draw_ch = ch, fill_ch = ' ';
							switch (ch) {
								case '`':
								case '[':
								case ']':
								case '/':
									draw_ch = '+';
									safe_attron(COLOR_PAIR(wall_pair));
									break;
								case 'x':
									draw_ch = ' ';
									safe_attron(COLOR_PAIR(wall_pair));
									break;
								case ' ':
									safe_attron(COLOR_PAIR(EMPTY_PAIR));
									break;
								case '-':
								case '|':
								case '+':
									safe_attron(COLOR_PAIR(wall_pair));
									break;
							}
							if (ch == '-') {
								fill_ch = ch;
							}
							draw_tile(row, col, draw_ch, &view, fill_ch);
						}
					}
				}

				{
					safe_attron(COLOR_PAIR(DOT_PAIR));
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

				safe_attron(COLOR_PAIR(PACMAN_PAIR));

				char player_symbol = 'A';

				// uzzīmē spēlētāja simbolu uz kartes
				struct v2 pacman_tile = pos_to_tile(&games[current_player].level.pacman_pos);
				if (pacman_tile.x >= 0 && pacman_tile.x < ARENA_WIDTH_IN_TILES) {
					draw_tile_v2(pacman_tile, player_symbol, &view, ' ');
				}

				// statuss
				if (games[current_player].mode == GAME_OVER) {
					safe_attron(COLOR_PAIR(GAME_OVER_TEXT_PAIR));
					struct v2 pos = draw_pos(HOUSE_BOTTOM + 2, HOUSE_LEFT, &view);
					mvprintw(pos.y, pos.x, "G A M E    O V E R");
				} else if (games[current_player].mode == PAUSED_BEFORE_PLAYING) {
					if (is_two_player) {
						safe_attron(COLOR_PAIR(PLAYER_TEXT_PAIR));
						struct v2 pos = draw_pos(HOUSE_TOP - 2, HOUSE_LEFT, &view);
						mvprintw(pos.y, pos.x, "  P L A Y E R   %d", current_player + 1);
					}
					{
						safe_attron(COLOR_PAIR(PACMAN_PAIR));
						struct v2 pos = draw_pos(HOUSE_BOTTOM + 2, HOUSE_LEFT + 1, &view);
						mvprintw(pos.y, pos.x, "R E A D Y !");
					}
				}

				// spēles statuss
				safe_attron(COLOR_PAIR(BG_PAIR));
				mvprintw(view.top - 2, view.left + 2, "Score");
				mvprintw(view.top - 2, view.left + 7, "%4d",  games[0].score);

				safe_attron(COLOR_PAIR(BG_PAIR));
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

#define PIXEL_SIZE 40 
#define TILE_SIZE_IN_PIXELS 8
#define TILE_SIZE (PIXEL_SIZE * TILE_SIZE_IN_PIXELS)

#define NUM_DIRS 4

// arena map params
#define ARENA_WIDTH_IN_TILES 36
#define ARENA_HEIGHT_IN_TILES 30
#define ARENA_HEIGHT (ARENA_HEIGHT_IN_TILES*TILE_SIZE)
#define ARENA_WIDTH (ARENA_WIDTH_IN_TILES*TILE_SIZE)

// more colors in 256
#define COLOR_BLACK_DARK 16
#define COLOR_GRAY 233
#define COLOR_YELLOW_BRIGHT 11
#define COLOR_ORANGE 173
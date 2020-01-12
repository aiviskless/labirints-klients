#include "libs.h"

void* memset(void* s, int c, size_t n);
void* memcpy(void* d, void* s, size_t n);
char* strchr(const char* s, int c);

#define MIN(x,y) ((x) < (y) ? (x) : (y))
#define MAX(x,y) ((x) < (y) ? (y) : (x))

#define MS(x) (1000*(x))
#define SECONDS(x) (MS(1000*(x)))

typedef char* c_str;

struct pair {
	int x, y;
};

struct pair add(struct pair a, struct pair b) {
	a.x += b.x;
	a.y += b.y;
	return a;
}

struct pair sub(struct pair a, struct pair b) {
	a.x -= b.x;
	a.y -= b.y;
	return a;
}

bool eql(struct pair* a, struct pair* b) {
	return a->x == b->x && a->y == b->y;
}

struct pair scale(struct pair a, int amt) {
	a.x *= amt;
	a.y *= amt;
	return a;
}

int floor_div(int num, int den) {
	double fresult = (double)num / (double)den;

	int result = (int)fresult;
	if (fresult < (double)result) {
		--result;
	}

	return result;
}

struct pair div_scale(struct pair a, int amt) {

	a.x = floor_div(a.x, amt);
	a.y = floor_div(a.y, amt);

	return a;
}

int time_in_us() {
	struct timeval t;
	gettimeofday(&t, NULL);
	return t.tv_sec * 1000000 + t.tv_usec;
}
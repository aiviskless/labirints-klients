#include "libs.h"

void* memset(void* s, int c, size_t n);
void* memcpy(void* d, void* s, size_t n);
char* strchr(const char* s, int c);

#define static
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

typedef char* c_str;

struct v2 {
	int x, y;
};

struct v2 add(struct v2 a, struct v2 b) {
	a.x += b.x;
	a.y += b.y;
	return a;
}

struct v2 sub(struct v2 a, struct v2 b) {
	a.x -= b.x;
	a.y -= b.y;
	return a;
}

bool eql(struct v2* a, struct v2* b) {
	return a->x == b->x && a->y == b->y;
}

struct v2 scale(struct v2 a, int amt) {
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

struct v2 div_scale(struct v2 a, int amt) {

	a.x = floor_div(a.x, amt);
	a.y = floor_div(a.y, amt);

	return a;
}

int time_in_us() {
	struct timeval t;
	gettimeofday(&t, NULL);
	return t.tv_sec * 1000000 + t.tv_usec;
}
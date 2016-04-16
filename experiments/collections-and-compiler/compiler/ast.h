#pragma once

#include <stdint.h>
#include "../collections/array.h"
#include "scanner.h"

typedef struct node_s node_t, *node_p;

struct node_s {
	uint64_t type;
	union {
		int64_t sint;
		array_p list;
		slice_t str;
	};
};

#define T_SINT  1
#define T_LIST  2
#define T_STR   3
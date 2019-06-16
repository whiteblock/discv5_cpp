#pragma once
#ifndef DV5_UTILS_H
#define DV5_UTILS_H

#include "types.h"
namespace dv5{
	int log_distance(hash_t a, hash_t b);

	uint_fast64_t random(uint_fast64_t max);
};


#endif
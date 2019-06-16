#pragma once
#ifndef DV5_TABLE_H
#define DV5_TABLE_H

#include "bucket.h"
#include "node.h"
#include "types.h"

namespace dv5
{
class table
{
	std::size_t _count;
	std::array<bucket, max_number_of_buckets> _buckets;
	node self;
  public:
  	//TODO constructor

  	//returns contested
  	std::vector<node>::iterator add_node(node n);
};
} // namespace dv5

#endif
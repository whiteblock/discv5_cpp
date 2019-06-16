#pragma once
#ifndef DV5_BUCKET_H
#define DV5_BUCKET_H

#include "node.h"
#include "types.h"
#include <vector>

namespace dv5
{
class bucket
{
	std::vector<node> _entries;
	std::vector<node> _back_log;

  public:
	bucket() { _back_log.reserve(max_bucket_size); }

	template <typename... Args> void insert(Args&&... args)
	{
		if (entries.size() >= max_bucket_size) {
			_back_log.emplace_back(std::forward<Args>(args)...);
		} else {
			_entries.emplace(_entries.begin(), std::forward<Args>(args)...)
		}
	}
	const std::vector<node>& get_entries() {return _entries;}

	const std::vector<node>& get_back_log() {return _back_log;}
};
} // namespace dv5
#endif
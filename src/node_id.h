#pragma once
#ifndef DV5_NODE_ID_H
#define DV5_NODE_ID_H

#include <string_view>
#include "types.h"

namespace dv5
{

class node_id
{
  private:
	std::array<byte_t, byte_size> _data;

  public:
	node_id();													// TODO
	explicit node_id(const std::string& hex);					// TODO
	explicit node_id(const std::string_view hex);				// TODO
	explicit node_id(const public_key_t& pk);					// TODO
	node_id(const hash_t& hash, const std::vector<byte_t>& sig); // TODO

	public_key_t get_pub_key() const;

	std::string get_string() const;
	operator std::string() const;
};

} // namespace dv5
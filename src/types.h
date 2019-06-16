#pragma once
#ifndef DV5_TYPES_H
#define DV5_TYPES_H

#include <array>
#include <boost/asio.hpp>
#include <chrono>
#include "constants.h"
#include <cstdint>
#include <string>
#include <vector>

namespace dv5
{
/**TYPES**/
typedef uint8_t byte_t;
typedef node_id typedef std::string key_t;
typedef std::chrono::high_resolution_clock::time_point time_t;
typedef std::vector<byte_t> public_key_t; // tmp until crypto lib is integrated
typedef std::array<byte_t, hash_size> hash_t;
typedef boost::asio::ip::address ip_addr_t;
typedef uint16_t port_t;
typedef boost::asio::ip::udp::endpoint udp_addr_t;
typedef boost::asio::ip::tcp::endpoint tcp_addr_t;

} // namespace dv5

#endif
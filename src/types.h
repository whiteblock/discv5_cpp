#ifndef TYPES_H
#define TYPES_H

#include <boost/asio.hpp>
#include <vector>
#include <cstdint>
#include <string>
#include <chrono>
namespace dv5{
	typedef uint8_t byte_t;
	typedef std::string db_key_t;
	typedef std::chrono::high_resolution_clock::time_point time_t;
	typedef std::vector<byte_t> public_key_t;//tmp until crypto lib is integrated
	typedef std::vector<byte_t> hash_t;
	typedef boost::asio::ip::udp::endpoint udp_addr_t;
}
#endif
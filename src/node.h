#pragma once
#ifndef DV5_NODE_H
#define DV5_NODE_H

#include "types.h"
#include <string>

namespace dv5{

struct node{
	std::string _id;
	
	ip_addr_t _ip;
	port_t _udp_port;
	port_t _tcp_port;

};
}

#endif
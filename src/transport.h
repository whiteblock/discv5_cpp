#ifndef TRANSPORT_H
#define TRANSPORT_H

#include "node.h"
#include "topic.h"
#include <any>
#include <cstdint>
#include <memory>
#include <vector>

class transport
{
	virtual ~transport() {}
	virtual std::vector<int8_t> send_ping(node &remote, udp_addr &remoteAddr, topics[] Topic) = 0;
	virtual void send_neighbours(node &remote, std::vector<std::weak_ptr<node>> nodes) = 0;
	virtual send_findnode_hash(node &remote, hash_t target) = 0;
	virtual void send_topic_register(node &remote, std::vector<topic> topics, int topic_idx, vector<int8_t> &pong) = 0;
	virtual void send_topic_nodes(node &remote, hash_t query_hash, std::vector<std::weak_ptr<node>> nodes) = 0;
	virtual std::vector<int8_t> send(node &remote, node_event ptype, std::any p) = 0;
	virtual local_addr() udp_addr = 0;
	virtual close() = 0;
};

#endif
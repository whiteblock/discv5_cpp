#ifndef NETWORK_H
#define NETWORK_H

#include "topic.h"
#include <boost/lockfree/spsc_queue.hpp>
#include <chrono>

namespace dv5
{

struct topic_register_req {
	add bool dv5::topic topic;
};

struct topic_search_req {
	dv5::topic topic;
	boost::lockfree::queue<std::weak_ptr<node>> found;
	boost::lockfree::queue<bool> lookup;
	std::chrono::nanoseconds delay;
};

struct timeout_event {
	node_event ev;
	std::weak_ptr<dv5::node> node;
};

struct topic_search_result {
	lookup_info target nodes std::vector<std::weak_ptr<node>>;
};

// Network manages the table and all protocol interaction.
type Network struct {
	db          *nodeDB // database of known nodes
	conn        transport
	netrestrict *netutil.Netlist

	closed           chan struct{}          // closed when loop is done
	closeReq         chan struct{}          // 'request to close'
	refreshReq       chan []*Node           // lookups ask for refresh on this channel
	refreshResp      chan (<-chan struct{}) // ...and get the channel to block on from this one
	read             chan ingressPacket     // ingress packets arrive here
	timeout          chan timeoutEvent
	queryReq         chan *findnodeQuery // lookups submit findnode queries on this channel
	tableOpReq       chan func()
	tableOpResp      chan struct{}
	topicRegisterReq chan topicRegisterReq
	topicSearchReq   chan topicSearchReq

	// State of the main loop.
	tab           *Table
	topictab      *topicTable
	ticketStore   *ticketStore
	nursery       []*Node
	nodes         map[NodeID]*Node // tracks active nodes with state != known
	timeoutTimers map[timeoutEvent]*time.Timer

	// Revalidation queues.
	// Nodes put on these queues will be pinged eventually.
	slowRevalidateQueue []*Node
	fastRevalidateQueue []*Node

	// Buffers for state transition.
	sendBuf []*ingressPacket
}


} // namespace dv5
#endif
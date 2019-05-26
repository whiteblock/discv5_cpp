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
	boost::lockfree::spsc_queue<std::weak_ptr<node>> found;
	boost::lockfree::spsc_queue<bool> lookup;
	std::chrono::nanoseconds delay;
};

struct timeout_event {
	node_event ev;
	std::weak_ptr<dv5::node> node;
};

struct topic_search_result {
	lookup_info target nodes std::vector<std::weak_ptr<node>>;
};


} // namespace dv5
#endif
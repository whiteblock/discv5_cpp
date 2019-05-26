#include "node_state.h"


dv5::node_state::unknown =
	dv5::node_state("unknown",
					[&](network &net, node &n, node_event ev, ingress_packet &pkt) -> node_state & { // handle, on ex
						switch (ev) {
						case node_event::ping_packet:
							net.handle_ping(n, pkt);
							net.ping(n, pkt.remote_addr);
							return dv5::node_state::verifywait;
						default:
							throw error::invalid_event;
						}
					},
					[](network &net, node &n) {
						net.tab.delete(n);
						n.ping_echo.clear();
						auto empty_response = vector<std::weak_ptr<node>>();
						for (auto &q : n.guts.deferred_queries) {
							q.reply.push(empty_response);
						}
						n.guts.deferred_queries.clear();
						if (n.guts.pending_neighbours) {
							n.guts.pending_neighbours.reply.push(empty_response);
							n.guts.pending_neighbours.reset();
						}
						n.query_timeouts = 0;
					});

dv5::node_state::verifyinit =
	node_state("verifyinit",
			   [&](network &net, node &n, node_event ev, ingress_packet &pkt) -> node_state & {
				   switch (ev) {
				    case node_event::ping_packet:
					   net.handle_ping(n, pkt);
					   return verifywait;
					case node_event::pong_packet: 
						net.handle_known_pong(n, pkt);
					   return remoteverifywait;
				    case node_event::pong_timeout:
					   return unknown;
				   default:
					   throw error::invalid_event;
				   }
			   },[](network &net, node &n) { net.ping(n, n.addr()); });

dv5::node_state::verifywait = node_state("verifywait",[&](network &net, node &n, node_event ev, ingress_packet &pkt) -> node_state& {
	switch(ev){
		case node_event::ping_packet:
			net.handle_ping(n, pkt);
			return verifywait;
		case node_event::pong_packet: 
			net.handleKnownPong(n, pkt); 
			return known;
		case node_event::pong_timeout :
			return unknown;
		default:
			return verifywait;
		throw error::invalid_event;
	}
});

dv5::node_state::remoteverifywait = node_state("remoteverifywait",[&](network &net, node &n, node_event ev, ingress_packet &pkt) -> node_state& {
	switch(ev){
		case node_event::ping_packet:
			net.handlePing(n, pkt); 
			return remoteverifywait;
		case node_event::ping_timeout:
			return known;
		default:
			throw error::invalid_event;
	}
},
[](network &net, node &n){
	net.timed_event(resp_timeout, n, ping_timeout);
});

dv5::node_state::known = node_state("known",[&](network &net, node &n, node_event ev, ingress_packet &pkt) -> node_state& {
	switch (ev){
		case node_event::ping_packet: 
			net.handle_ping(n, pkt);
			return known;
		case node_event::pong_packet: 
			net.handle_known_pong(n, pkt)
			return known;
		default: 
		return net.handle_query_event(n, ev, pkt)
	}
},[](network &net, node &n){
	n.query_timeouts = 0; 
	n.start_next_query(net);

  // Insert into the table and start revalidation of the last node
  // in the bucket if it is full.
	auto last = net.tab.add(n);
	if (last != nil && last.state == known){// TODO: do this asynchronously; TODO: Fix this when implemented
		net.transition(last, contested)
	}
},true);

dv5::node_state::contested = node_state("contested",[&](network &net, node &n, node_event ev, ingress_packet &pkt) -> node_state& {
	switch(ev){
			case node_event::pong_packet:
				// Node is still alive.
				net.handle_known_pong(n, pkt);
				return known;
			case node_event::pong_timeout: 
				net.tab.delete_replace(n); 
				return unresponsive;
			case node_event::ping_packet: 
				net.handle_ping(n, pkt); 
				return contested;
			default: 
				return net.handle_query_event(n, ev, pkt)
		}
},[](network &net, node &n){
	net.ping(n, n.addr());
},true);


dv5::node_state::unresponsive = node_state("unresponsive",[&](network &net, node &n, node_event ev, ingress_packet &pkt) -> node_state& {
	switch(ev){
			case node_event::ping_packet:
				net.handle_ping(n, pkt);
				return known;
			case node_event::pong_packet:
				net.handle_known_pong(n, pkt);
				return known;
			default: 
				return net.handle_query_event(n, ev, pkt)
			}
},dv5::node_state::enter_t(),true);
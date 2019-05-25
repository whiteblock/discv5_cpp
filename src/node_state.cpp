#include "node_state.h"


dv5::node_state::unknown = dv5::node_state("unknown",
   [&](network &net, node &n, node_event ev,ingress_packet pkt &) -> node_state& { // handle, on ex
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
		   n.guts.pending_neighbours.reset()
	   }
	   n.query_timeouts = 0
   },
   false);

dv5::node_state::verifyinit =
	node_state("verifyinit", [&](network &net, node &n, node_event ev, ingress_packet pkt &) -> node_state &);
{
	name :,
	enter : func(net *Network, n *Node){net.ping(n, n.addr())},
	handle : func(net *Network, n *Node, ev nodeEvent, pkt *ingressPacket)(*nodeState, error){switch ev{
		case pingPacket : net.handlePing(n, pkt) return verifywait,
		nil case pongPacket : err : = net.handleKnownPong(n, pkt) return remoteverifywait,
		err case pongTimeout : return unknown,
		nil default : return verifyinit,
		errInvalidEvent
	}},
}

dv5::node_state::verifywait = &nodeState{
	name : "verifywait",
	handle : func(net *Network, n *Node, ev nodeEvent, pkt *ingressPacket)(*nodeState, error){switch ev{
		case pingPacket : net.handlePing(n, pkt) return verifywait,
		nil case pongPacket : err : = net.handleKnownPong(n, pkt) return known,
		err case pongTimeout : return unknown,
		nil default : return verifywait,
		errInvalidEvent
	}},
}

							  dv5::node_state::remoteverifywait = &nodeState{
	name : "remoteverifywait",
	enter : func(net *Network, n *Node){net.timedEvent(respTimeout, n, pingTimeout)},
	handle : func(net *Network, n *Node, ev nodeEvent, pkt *ingressPacket)(*nodeState, error){switch ev{
		case pingPacket : net.handlePing(n, pkt) return remoteverifywait,
		nil case pingTimeout : return known,
		nil default : return remoteverifywait,
		errInvalidEvent
	}},
}

																  dv5::node_state::known = &nodeState{
	name : "known",
	canQuery : true,
	enter : func(net *Network, n *Node){
		n.queryTimeouts = 0 n.startNextQuery(net)
						  // Insert into the table and start revalidation of the last node
						  // in the bucket if it is full.
						  last :
			= net.tab.add(n) if last != nil && last.state ==
												   known{// TODO: do this asynchronously
														 net.transition(last, contested)}
	},
	handle : func(net *Network, n *Node, ev nodeEvent, pkt *ingressPacket)(*nodeState, error){switch ev{
		case pingPacket : net.handlePing(n, pkt) return known,
		nil case pongPacket : err : = net.handleKnownPong(n, pkt) return known,
		err default : return net.handleQueryEvent(n, ev, pkt)
	}},
}

																						   dv5::node_state::contested =
	&nodeState{
		name : "contested",
		canQuery : true,
		enter : func(net *Network, n *Node){net.ping(n, n.addr())},
		handle : func(net *Network, n *Node, ev nodeEvent, pkt *ingressPacket)(*nodeState, error){switch ev{
			case pongPacket :
				// Node is still alive.
			err : = net.handleKnownPong(n, pkt) return known,
			err case pongTimeout : net.tab.deleteReplace(n) return unresponsive,
			nil case pingPacket : net.handlePing(n, pkt) return contested,
			nil default : return net.handleQueryEvent(n, ev, pkt)
		}},
	}

	dv5::node_state::unresponsive = &nodeState
{
name:
	"unresponsive",
		canQuery : true, handle : func(net * Network, n * Node, ev nodeEvent, pkt * ingressPacket)(*nodeState, error)
	{
		switch
			ev
			{
			case pingPacket:
				net.handlePing(n, pkt) return known,
					nil case pongPacket : err : = net.handleKnownPong(n, pkt) return known,
											  err default : return net.handleQueryEvent(n, ev, pkt)
			}
	}
	,
}
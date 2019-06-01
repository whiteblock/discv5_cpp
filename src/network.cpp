#include "network.h"


func newNetwork(conn transport, ourPubkey ecdsa.PublicKey, dbPath string, netrestrict *netutil.Netlist) (*Network, error) {
	ourID := PubkeyID(&ourPubkey)

	var db *nodeDB
	if dbPath != "<no database>" {
		var err error
		if db, err = newNodeDB(dbPath, Version, ourID); err != nil {
			return nil, err
		}
	}

	tab := newTable(ourID, conn.localAddr())
	net := &Network{
		db:               db,
		conn:             conn,
		netrestrict:      netrestrict,
		tab:              tab,
		topictab:         newTopicTable(db, tab.self),
		ticketStore:      newTicketStore(),
		refreshReq:       make(chan []*Node),
		refreshResp:      make(chan (<-chan struct{})),
		closed:           make(chan struct{}),
		closeReq:         make(chan struct{}),
		read:             make(chan ingressPacket, 100),
		timeout:          make(chan timeoutEvent),
		timeoutTimers:    make(map[timeoutEvent]*time.Timer),
		tableOpReq:       make(chan func()),
		tableOpResp:      make(chan struct{}),
		queryReq:         make(chan *findnodeQuery),
		topicRegisterReq: make(chan topicRegisterReq),
		topicSearchReq:   make(chan topicSearchReq),
		nodes:            make(map[NodeID]*Node),
	}
	go net.loop()
	return net, nil
}

// Close terminates the network listener and flushes the node database.
func (net *Network) Close() {
	net.conn.Close()
	select {
	case <-net.closed:
	case net.closeReq <- struct{}{}:
		<-net.closed
	}
}

// Self returns the local node.
// The returned node should not be modified by the caller.
func (net *Network) Self() *Node {
	return net.tab.self
}

// ReadRandomNodes fills the given slice with random nodes from the
// table. It will not write the same node more than once. The nodes in
// the slice are copies and can be modified by the caller.
func (net *Network) ReadRandomNodes(buf []*Node) (n int) {
	net.reqTableOp(func() { n = net.tab.readRandomNodes(buf) })
	return n
}

// SetFallbackNodes sets the initial points of contact. These nodes
// are used to connect to the network if the table is empty and there
// are no known nodes in the database.
func (net *Network) SetFallbackNodes(nodes []*Node) error {
	nursery := make([]*Node, 0, len(nodes))
	for _, n := range nodes {
		if err := n.validateComplete(); err != nil {
			return fmt.Errorf("bad bootstrap/fallback node %q (%v)", n, err)
		}
		// Recompute cpy.sha because the node might not have been
		// created by NewNode or ParseNode.
		cpy := *n
		cpy.sha = crypto.Keccak256Hash(n.ID[:])
		nursery = append(nursery, &cpy)
	}
	net.reqRefresh(nursery)
	return nil
}

// Resolve searches for a specific node with the given ID.
// It returns nil if the node could not be found.
func (net *Network) Resolve(targetID NodeID) *Node {
	result := net.lookup(crypto.Keccak256Hash(targetID[:]), true)
	for _, n := range result {
		if n.ID == targetID {
			return n
		}
	}
	return nil
}

// Lookup performs a network search for nodes close
// to the given target. It approaches the target by querying
// nodes that are closer to it on each iteration.
// The given target does not need to be an actual node
// identifier.
//
// The local node may be included in the result.
func (net *Network) Lookup(targetID NodeID) []*Node {
	return net.lookup(crypto.Keccak256Hash(targetID[:]), false)
}

func (net *Network) lookup(target common.Hash, stopOnMatch bool) []*Node {
	var (
		asked          = make(map[NodeID]bool)
		seen           = make(map[NodeID]bool)
		reply          = make(chan []*Node, alpha)
		result         = nodesByDistance{target: target}
		pendingQueries = 0
	)
	// Get initial answers from the local node.
	result.push(net.tab.self, bucketSize)
	for {
		// Ask the α closest nodes that we haven't asked yet.
		for i := 0; i < len(result.entries) && pendingQueries < alpha; i++ {
			n := result.entries[i]
			if !asked[n.ID] {
				asked[n.ID] = true
				pendingQueries++
				net.reqQueryFindnode(n, target, reply)
			}
		}
		if pendingQueries == 0 {
			// We have asked all closest nodes, stop the search.
			break
		}
		// Wait for the next reply.
		select {
		case nodes := <-reply:
			for _, n := range nodes {
				if n != nil && !seen[n.ID] {
					seen[n.ID] = true
					result.push(n, bucketSize)
					if stopOnMatch && n.sha == target {
						return result.entries
					}
				}
			}
			pendingQueries--
		case <-time.After(respTimeout):
			// forget all pending requests, start new ones
			pendingQueries = 0
			reply = make(chan []*Node, alpha)
		}
	}
	return result.entries
}

func (net *Network) RegisterTopic(topic Topic, stop <-chan struct{}) {
	select {
	case net.topicRegisterReq <- topicRegisterReq{true, topic}:
	case <-net.closed:
		return
	}
	select {
	case <-net.closed:
	case <-stop:
		select {
		case net.topicRegisterReq <- topicRegisterReq{false, topic}:
		case <-net.closed:
		}
	}
}

func (net *Network) SearchTopic(topic Topic, setPeriod <-chan time.Duration, found chan<- *Node, lookup chan<- bool) {
	for {
		select {
		case <-net.closed:
			return
		case delay, ok := <-setPeriod:
			select {
			case net.topicSearchReq <- topicSearchReq{topic: topic, found: found, lookup: lookup, delay: delay}:
			case <-net.closed:
				return
			}
			if !ok {
				return
			}
		}
	}
}

func (net *Network) reqRefresh(nursery []*Node) <-chan struct{} {
	select {
	case net.refreshReq <- nursery:
		return <-net.refreshResp
	case <-net.closed:
		return net.closed
	}
}

func (net *Network) reqQueryFindnode(n *Node, target common.Hash, reply chan []*Node) bool {
	q := &findnodeQuery{remote: n, target: target, reply: reply}
	select {
	case net.queryReq <- q:
		return true
	case <-net.closed:
		return false
	}
}

func (net *Network) reqReadPacket(pkt ingressPacket) {
	select {
	case net.read <- pkt:
	case <-net.closed:
	}
}

func (net *Network) reqTableOp(f func()) (called bool) {
	select {
	case net.tableOpReq <- f:
		<-net.tableOpResp
		return true
	case <-net.closed:
		return false
	}
}
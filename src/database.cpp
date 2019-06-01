#include "database.h"

using namespace std;
using namespace dv5;



dv5::node_db::node_db(node_id id) : _id(id),_quit(false) //TODO
{
	//open the db to operate in memory
}

dv5::node_db::node_db(std::string path,int version,node_id id) :  _id(id),_quit(false) //TODO
{
	opts := &opt.Options{OpenFilesCacheCapacity: 5}
	db, err := leveldb.OpenFile(path, opts)
	if _, iscorrupted := err.(*errors.ErrCorrupted); iscorrupted {
		db, err = leveldb.RecoverFile(path, nil)
	}
	if err != nil {
		return nil, err
	}
	// The nodes contained in the cache correspond to a certain protocol version.
	// Flush all nodes if the version doesn't match.
	currentVer := make([]byte, binary.MaxVarintLen64)
	currentVer = currentVer[:binary.PutVarint(currentVer, int64(version))]

	blob, err := db.Get(nodeDBVersionKey, nil)
	switch err {
	case leveldb.ErrNotFound:
		// Version not found (i.e. empty cache), insert it
		if err := db.Put(nodeDBVersionKey, currentVer, nil); err != nil {
			db.Close()
			return nil, err
		}

	case nil:
		// Version present, flush if different
		if !bytes.Equal(blob, currentVer) {
			db.Close()
			if err = os.RemoveAll(path); err != nil {
				return nil, err
			}
			return newPersistentNodeDB(path, version, self)
		}
	}
	return &nodeDB{
		lvl:  db,
		self: self,
		quit: make(chan struct{}),
	}, nil
}\
// makeKey generates the leveldb key-blob from a node id and its particular
// field of interest.
db_key_t dv5::make_key(node_id id, std::string field)
{
	if(node_db::null_node_id.get_string() == id.get_string()){
		return field;//TODO
	}
	return node_db::item_prefix + id.get_string() + field;
}

// splitKey tries to split a database key into a node id and a field part.
//ret id,field
std::tuple<node_id,string> dv5::split_key(db_key_t key)
{
		// If the key is not of a node, return it plainly
	if !bytes.HasPrefix(key, nodeDBItemPrefix) {
		return NodeID{}, string(key)
	}
	// Otherwise split the id and field
	item := key[len(nodeDBItemPrefix):]
	copy(id[:], item[:len(id)])
	field = string(item[len(id):])

	return id, field
}

int64_t dv5::node_db::fetch_int64(const db_key_t& key)
{
	blob, err := db.lvl.Get(key, nil)
	if err != nil {
		return 0
	}
	val, read := binary.Varint(blob)
	if read <= 0 {
		return 0
	}
	return val
}

void dv5::node_db::store_int64(const db_key_t& key,int64_t n)
{
	blob := make([]byte, binary.MaxVarintLen64)
	blob = blob[:binary.PutVarint(blob, n)]
	return db.lvl.Put(key, blob, nil)
}

// node retrieves a node with a given id from the database.
Node dv5::node_db::node(node_id id)
{//TODO
	var node Node
	if err := db.fetchRLP(makeKey(id, nodeDBDiscoverRoot), &node); err != nil {
		return nil
	}
	node.sha = crypto.Keccak256Hash(node.ID[:])
	return &node
}

void dv5::node_db::update_node(const Node& node)
{//TODO db ->this
	db.storeRLP(makeKey(node.ID, nodeDBDiscoverRoot), node)
}


// deleteNode deletes all information/keys associated with a node.

void dv5::node_db::delete_node(const node_id& id)
{
	deleter := db.lvl.NewIterator(util.BytesPrefix(makeKey(id, "")), nil)
	for deleter.Next() {
		if err := db.lvl.Delete(deleter.Key(), nil); err != nil {
			return
		}
	}
	return
}

// ensureExpirer is a small helper method ensuring that the data expiration
// mechanism is running. If the expiration goroutine is already running, this
// method simply returns.
//
// The goal is to start the data evacuation only after the network successfully
// bootstrapped itself (to prevent dumping potentially useful seed nodes). Since
// it would require significant overhead to exactly trace the first successful
// convergence, it's simpler to "ensure" the correct state when an appropriate
// condition occurs (i.e. a successful bonding), and discard further events.
void dv5::node_db::ensure_expirer()
{
	db.runner.Do(func() { go db.expirer() })
}

// expirer should be started in a go routine, and is responsible for looping ad
// infinitum and dropping stale data from the database.
void dv5::node_db::expirer()
{
	tick := time.NewTicker(nodeDBCleanupCycle)
	defer tick.Stop()
	for {
		select {
		case <-tick.C:
			if err := db.expireNodes(); err != nil {
				log.Error(fmt.Sprintf("Failed to expire nodedb items: %v", err))
			}
		case <-db.quit:
			return
		}
	}
}

// expireNodes iterates over the database and deletes all nodes that have not
// been seen (i.e. received a pong from) for some allotted time.
void dv5::node_db::expire_nodes() {
	threshold := time.Now().Add(-nodeDBNodeExpiration)

	// Find discovered nodes that are older than the allowance
	it := db.lvl.NewIterator(nil, nil)
	defer it.Release()

	for it.Next() {
		// Skip the item if not a discovery node
		id, field := splitKey(it.Key())
		if field != nodeDBDiscoverRoot {
			continue
		}
		// Skip the node if not expired yet (and not self)
		if !bytes.Equal(id[:], db.self[:]) {
			if seen := db.lastPong(id); seen.After(threshold) {
				continue
			}
		}
		// Otherwise delete all associated information
		db.deleteNode(id)
	}
	return nil
}

// lastPing retrieves the time of the last ping packet send to a remote node,
// requesting binding.
dv5::time_t dv5::node_db::last_ping(const node_id& id)
{
	return time.Unix(db.fetchInt64(makeKey(id, nodeDBDiscoverPing)), 0)
}

// updateLastPing updates the last time we tried contacting a remote node.
void dv5::node_db::update_last_ping(const node_id& id,dv5::time_t instance)
{
	return db.storeInt64(makeKey(id, nodeDBDiscoverPing), instance.Unix())
}


// lastPong retrieves the time of the last successful contact from remote node.
dv5::time_t dv5::node_db::last_pong(const node_id& id)
{
	return time.Unix(db.fetchInt64(makeKey(id, nodeDBDiscoverPong)), 0)
}

// updateLastPong updates the last time a remote node successfully contacted.
void dv5::node_db::update_last_pong(const node_id& id,dv5::time_t instance)
{
	return db.storeInt64(makeKey(id, nodeDBDiscoverPong), instance.Unix())
}

// findFails retrieves the number of findnode failures since bonding.
int dv5::node_db::find_fails(const node_id& id)
{
	return int(db.fetchInt64(makeKey(id, nodeDBDiscoverFindFails)))
}

// updateFindFails updates the number of findnode failures since bonding.
void dv5::node_db::update_find_fails(const node_id& id,int fails)
{
	return db.storeInt64(makeKey(id, nodeDBDiscoverFindFails), int64(fails))
}

// localEndpoint returns the last local endpoint communicated to the
// given remote node.
rpc_endpoint_t dv5::node_db::local_endpoint(const node_id& id)
{
	var ep rpcEndpoint
	if err := db.fetchRLP(makeKey(id, nodeDBDiscoverLocalEndpoint), &ep); err != nil {
		return nil
	}
	return &ep
}

void dv5::node_db::update_local_endpoint(const node_id&,rpc_endpoint_t ep)
{
	return db.storeRLP(makeKey(id, nodeDBDiscoverLocalEndpoint), &ep)
}

std::tuple<uint32_t,uint32_t> fetch_topic_reg_tickets(const node_id& id)
{
	key := makeKey(id, nodeDBTopicRegTickets)
	blob, _ := db.lvl.Get(key, nil)
	if len(blob) != 8 {
		return 0, 0
	}
	issued = binary.BigEndian.Uint32(blob[0:4])
	used = binary.BigEndian.Uint32(blob[4:8])
	return
}

void update_topic_reg_tickets(const node_id& id,uint32_t issued,uint32_t used)
{
	key := makeKey(id, nodeDBTopicRegTickets)
	blob := make([]byte, 8)
	binary.BigEndian.PutUint32(blob[0:4], issued)
	binary.BigEndian.PutUint32(blob[4:8], used)
	return db.lvl.Put(key, blob, nil)
}

// reads the next node record from the iterator, skipping over other
// database entries.
func nextNode(it iterator.Iterator) *Node {
	for end := false; !end; end = !it.Next() {
		id, field := splitKey(it.Key())
		if field != nodeDBDiscoverRoot {
			continue
		}
		var n Node
		if err := rlp.DecodeBytes(it.Value(), &n); err != nil {
			log.Warn(fmt.Sprintf("invalid node %x: %v", id, err))
			continue
		}
		return &n
	}
	return nil
}
#ifndef DATABASE_H
#define DATABASE_H

#include "types.h"
#include "node.h"
#include <string>
#include <mutex>
#include <atomic>
#include <chrono>
#include <tuple>
#include <thread>

namespace dv5{
	nodeDBDiscoverLocalEndpoint = nodeDBDiscoverRoot + ":localendpoint"
	nodeDBTopicRegTickets       = ":tickets"
class node_db {
public:
	static std::chrono::seconds node_expiration = 24 * 60 * 60;
	static std::chrono::seconds cleanup_cycle = 60 * 60;
	static node_id null_node_id = node_id();

	// Schema layout for the node database
	static std::string version_key = "version";// Version of the database to flush if changes
	static std::string item_prefix = "n:";// Identifier to prefix node entries with
	static std::string discover_root = ":discover";
	static std::string discover_pint = discover_root + ":lastping";
	static std::string discover_pong = discover_root + ":lastpong";
	static std::string discover_find_fails = discover_root + ":findfail";
	static std::string discover_local_endpoint = discover_root + ":localendpoint";
	static std::string topic_reg_tickets = ":tickets";
private:

	//leveldb _lvl;
	node_id _id;//self
	std::once_flag _runner;
	std::atomic_bool _quit;
public:
	explicit node_db(node_id id);
	explicit node_db(std::string path,int version,node_id id);

	int64_t fetch_int64(const db_key_t& key);
	void store_int64(const db_key_t& key,int64_t n);

	template<typename T>
	void store_rpl(const db_key_t& key,T val)
	{ //TODO
		blob, err := rlp.EncodeToBytes(val)
		if err != nil {
			return err
		}
		return db.lvl.Put(key, blob, nil)
	}

	template<typename T>
	T fetch_rpl(const db_key_t& key)
	{//TODO
		blob, err := db.lvl.Get(key, nil)
		if err != nil {
			return err
		}
		err = rlp.DecodeBytes(blob, val)
		if err != nil {
			log.Warn(fmt.Sprintf("key %x (%T) %v", key, val, err))
		}
		return err
	}

	Node node(node_id id);
	void update_node(const Node& node);
	void delete_node(const node_id& id);

	void expirer();
	void ensure_expirer();
	void expire_nodes();

	dv5::time_t last_ping(const node_id& id);
	void update_last_ping(const node_id& id,dv5::time_t instance);
	dv5::time_t last_pong(const node_id& id);
	void update_last_pong(const node_id& id,dv5::time_t instance);
	int find_fails(const node_id& id);
	void update_find_fails(const node_id& id,int fails);

	}
};
	db_key_t make_key(node_id id, std::string field);
	//ret id,field
	std::tuple<node_id,string> split_key(db_key_t key);
}

#endif
#ifndef NODE_H
#define NODE_H

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>
#include <functional>
#include <cstdint.h>
#include <boost/asio.hpp>
#include <boost/asio/ip/address.hpp>
#include <boost/lockfree/spsc_queue.hpp>

using boost::asio::ip::address;
namespace dv5
{	
	typedef vector<int8_t> hash_t;
	typedef boost::asio::ip::udp::endpoint udp_addr;

	enum class node_event = {
		// Packet type events.
		// These correspond to packet types in the UDP protocol.
		ping_packet,
		pong_packet,
		findnode_packet,
		neighbors_packet,
		findnode_hash_packet,
		topic_register_packet,
		topic_query_packet,
		topic_nodes_packet,

		// Non-packet events.
		pong_timeout = topic_nodes_packet + 256,
		ping_timeout = pong_timeout + 1,
		neighbours_timeout = ping_timeout + 1
	};

	struct find_node_query{
		std::weak_ptr<node> remote;
		hash_t target;
		boost::lockfree::spsc_queue<vector<std::weak_ptr<node>>> reply;
		int nresults;
	};

	struct node_net_guts {
		hash_t sha;
		node_state& state;
		std::vector<int8_t> ping_echo;
		std::vector<topic> ping_topics;
		std::vector<find_node_query> deferred_queries;
		std::optional<find_node_query> pending_neighbours
		int query_timeouts;

		void defer_query(find_node_query q); //TODO
		void defer_query(find_node_query&& q); //TODO
		void start_next_query(network& net);//TODO
		bool start(network& net); //TODO


	}; 
	
	class node_id {
	public:
		static int bits = 512;
		typedef std::vector<uint8_t> public_key_t;//tmp until crypto lib is integrated

		explicit node_id(const std::string& hex); //TODO
		explicit node_id(const std::string_view hex); //TODO
		explicit node_id(const public_key_t& pk); //TODO
		explicit node_id(const std::vector<uint8_t>& hash,const std::vector<uint8_t> sig); //TODO

		public_key_t get_pub_key() const;

		std::string get_string() const;
		operator std::string() const;
	};


	class node{//ip::udp::endpoint
	public:
		boost::asio::ip::address ip;
		uint16_t udp_port;
		uint16_t tcp_port;
		node_id id;
		node_net_guts guts;
		//TODO
	};
}
#endif
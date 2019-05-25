#ifndef NODE_STATE_H
#define NODE_STATE_H

#include "node.h"
#include <string>
#include <utility>
#include <tuple>
#include "error.h"

namespace dv5
{
class node_state
{
  public:
	typedef std::function< node_state& (network &, node &, node_event, ingress_packet &)> handle_t;
	typedef std::function<void(node &, network &)> enter_t;

	std::string name;
	handle_t handle;
	enter_t enter;
	bool can_query;

	node_state(std::string name, handle_t handle, enter_t enter, bool can_query)
		: name(std::move(name)), handle(std::move(handle)), enter(std::move(enter)), can_query(can_query)
	{}
	node_state(std::string&& name, handle_t&& handle, enter_t&& enter, bool&& can_query)
		: name(std::move(name)), handle(std::move(handle)), enter(std::move(enter)), can_query(can_query)
	{}

	const std::string &get_name() const noexcept { return _name; }


	static node_state unknown;
	static node_state verify_init;
	static node_state verify_wait;
	static node_state remote_verify_wait;
	static node_state known;
	static node_state contested;
	static node_state unresponsive;
};
} // namespace dv5
#endif
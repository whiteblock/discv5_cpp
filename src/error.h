#ifndef ERROR_H
#define ERROR_H

#include <stdexcept>
#include <utility>

namespace dv5{

	class error : public std::exception {
	private:
		std::string info;
		bool empty;
	public:
		explicit error(std::string what_arg) : info(std::move(what_arg)),empty(false) {}
		explicit error(const char *what_arg) : info(what_arg),empty(false) {}
		error() : info(),empty(true){}
		operator bool() const noexcept {return this->empty;}
		const char *what() const noexcept { return this->info.c_str(); }

		//Some constant errors
		static error invalid_event = error("invalid in current state");
		static error no_query = error("no pending query");
	}
};

#endif
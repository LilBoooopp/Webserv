#pragma once
#include "IHandler.hpp"

class	Router {
	IHandler*	fallback_;
public:
	Router(IHandler* fd): fallback_(fd) {}
	IHandler*	route(const std::string& /*path*/) { return (fallback_); }
};

/*
 *  This file is part of DORPC. Please see README for details.
 *  Copyright (C) 2022 Marek Zalewski aka Drwalin
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef RPC_DEBUG_HPP
#define RPC_DEBUG_HPP

#include <cstdio>
#include <regex>
#include <string>
#include <mutex>
#include <cstdarg>

namespace net_debug {
	extern std::mutex MUTEX;
	extern char** ARGV;

	std::string PrettyFunction(const char* pretty);

	void PRINT(int ARG0, ...);
}

#define DEBUG(...) { \
	std::lock_guard<std::mutex> lock(net_debug::MUTEX); \
	fprintf(stderr, " DEBUG [%s]:", net_debug::ARGV[1]); \
	net_debug::PRINT(0, __VA_ARGS__); \
	fprintf(stderr, " ; %s %s:%i\n", \
			net_debug::PrettyFunction(__PRETTY_FUNCTION__).c_str(), __FILE__, __LINE__); \
	fflush(stderr); }

#endif


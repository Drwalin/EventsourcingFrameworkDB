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

#include "Debug.hpp"

namespace net_debug {
	std::mutex MUTEX;
	char** ARGV = []()->char**{
		static char* v[2];
		v[0] = (char*)"program";
		v[1] = (char*)"native";
		return v;}();

	std::string PrettyFunction(const char* pretty) {
		std::string name = pretty;
		name = std::regex_replace(name, std::regex("::__cxx11::basic_string<[>]*>"),
				"::string");
		name = std::regex_replace(name, std::regex("::__cxx11::basic_"), "::");
		name = std::regex_replace(name, std::regex("::__cxx11::"), "::");
		return name;
	}

	void PRINT(int ARG0, ...) {
		va_list l;
		va_start(l, ARG0);
		vfprintf(stderr, va_arg(l, char*), l);
		va_end(l);
	}
}


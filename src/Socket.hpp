/*
 *  This file is part of DORPC. Please see README for details.
 *  Copyright (C) 2021-2022 Marek Zalewski aka Drwalin
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

#ifndef DORPC_NETWORKING_SOCKET_HPP
#define DORPC_NETWORKING_SOCKET_HPP

#include <cinttypes>
#include <functional>
#include <memory>
#include <libusockets.h>
#include <string>
#include <atomic>

#include "Buffer.hpp"

namespace net {
	class Socket {
	private:
		Socket() = default;
	public:
		
		enum Method {
			// TODO test all following methods:
			ANY_BYTES,
			EXACT_BYTES,
// 			ATLEAST_BYTES,
// 			ATMOST_BYTES,
			END_WITH_CHARACTER,
// 			END_WITH_CRLF,
// 			END_WITH_CRLF_CRLF,
// 			HTML_HEADER,
// 			HTML_BODY,
// 			HTML_HEADER_BODY,
// 			HTML_HEADER_DONE_BODY,
// 			HTML_HEADER_ONE_LINE
		};
		
		struct ReceivingMethod {
			Method method = ANY_BYTES;
			int32_t value = 0;
		};
		
		~Socket();
		
		struct us_socket_t* socket;
		std::weak_ptr<Socket> self;
		std::shared_ptr<class Context> context;
		std::shared_ptr<class Loop> loop;
		std::string remoteIp;
		int ssl;
		union {
			void* userData;
			uint64_t userData64;
			uint32_t userData32;
			uint16_t userData16;
			uint8_t userData8;
		};

		Buffer buffer;
		ReceivingMethod receivingMethod;

		
		void IncRefs();
		void DecRefs();
		
		void Init(struct us_socket_t* socket, int ssl);


		void Send(Buffer& sendBuffer);


		void OnOpen(char* ip, int ipLength);
		void OnData(uint8_t* data, int length);
		void OnEnd();
		void OnClose(int code, void* reason);
		void OnTimeout();
		void OnWritable();

		void InternalSend(Buffer& buffer);
		void InternalClose();
		
		void InternalOnDataBufferDone();
		
		friend class net::Context;
	};
}

#endif


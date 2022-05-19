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

#include <algorithm>

#include <cinttypes>
#include <cstring>

#include <libusockets.h>
#include <memory>

#include "Loop.hpp"
#include "Context.hpp"
#include "Event.hpp"

#include "Socket.hpp"

#include "Debug.hpp"

namespace net {
	
	void Socket::IncRefs() {
		std::shared_ptr<Socket> s = self.lock();
		if(s == NULL)
			return;
		uint8_t bytes[sizeof(std::shared_ptr<Socket>)];
		new (bytes) std::shared_ptr<Socket>(s);
	}
	
	template<typename T>
	struct Destructor {
		Destructor(T* ptr) {
			ptr->~T();
		}
	};
	
	void Socket::DecRefs() {
		std::shared_ptr<Socket> s = self.lock();
		if(s == NULL)
			return;
		uint8_t bytes[sizeof(std::shared_ptr<Socket>)];
		memcpy(bytes, &s, sizeof(std::shared_ptr<Socket>));
		// TODO it may be problematic
		Destructor tmp((std::shared_ptr<Socket>*)bytes);
	}
	
	
	Socket::~Socket() {
	}
	
	void Socket::OnOpen(char* ip, int ipLength) {
		remoteIp = std::string(TranslateIp(ip, ipLength));
		context->sockets.insert(self.lock());
		bytes_to_receive = 0;
		received_bytes_of_size = 0;
	}

	void Socket::OnEnd() {
		buffer.Destroy();
		context->sockets.erase(self.lock());
	}

	void Socket::OnClose(int code, void* reason) {
		remoteIp.clear();
		buffer.Destroy();
		delete *(std::shared_ptr<Socket>**)us_socket_ext(1, socket);
		*(std::shared_ptr<Socket>**)us_socket_ext(1, socket) = NULL;
		context->sockets.erase(self.lock());
	}

	void Socket::OnTimeout() {
		// TODO
	}

	void Socket::OnWritable() {
		// TODO
	}

	void Socket::OnData(uint8_t* data, int length) {
		while(length) {
			if(received_bytes_of_size < 4) {
				int bytes_to_copy = std::min(4-received_bytes_of_size, length);
				memcpy(received_size+received_bytes_of_size, data,
						bytes_to_copy);
				length -= bytes_to_copy;
				data += bytes_to_copy;
				received_bytes_of_size += bytes_to_copy;
				if(received_bytes_of_size == 4) {
					bytes_to_receive =
						(int(received_size[0]))
						| (int(received_size[1]) << 8)
						| (int(received_size[2]) << 16)
						| (int(received_size[3]) << 24);
				}
			} else {
				int32_t bytes_to_copy = std::min(bytes_to_receive, length);
				buffer.Write(data, bytes_to_copy);
				data += bytes_to_copy;
				length -= bytes_to_copy;
				bytes_to_receive -= bytes_to_copy;
				if(bytes_to_receive == 0) {
					if(context->onReceiveMessage) {
						context->onReceiveMessage(buffer, self.lock());
					}
					buffer.Clear();
					received_bytes_of_size = 0;
				}
			}
		}
	}

	void Socket::Send(Buffer& sendBuffer) {
		Event* event = Event::Allocate();
		event->after = NULL;
		event->buffer_or_ip = std::move(sendBuffer);
		event->socket = self.lock();
		event->listenSocket = NULL;
		event->type = Event::SOCKET_SEND;
		if(loop == Loop::ThisThreadLoop())
			event->Run();
		else
			loop->PushEvent(event);
	}

	void Socket::InternalSend(Buffer& buffer) {
		int32_t length = buffer.Size();
		uint8_t b[4];
		b[0] = (length)&0xFF;
		b[1] = (length>>8)&0xFF;
		b[2] = (length>>16)&0xFF;
		b[3] = (length>>24)&0xFF;
		us_socket_write(ssl, socket, (char*)b, 4, length);
		us_socket_write(ssl, socket, (char*)buffer.Data(), length, 0);
	}

	void Socket::InternalClose() {
		// TODO does it work
		if(us_socket_is_established(ssl, socket)) {
			us_socket_close(ssl, socket, 0, NULL);
		} else {
			us_socket_close_connecting(ssl, socket);
		}
		socket = NULL;
	}
}


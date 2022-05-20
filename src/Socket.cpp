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
		receivingMethod.method = ANY_BYTES;
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
			switch(receivingMethod.method) {
				case ANY_BYTES:
					buffer.Write(data, length);
					InternalOnDataBufferDone();
					return;
				case EXACT_BYTES:
					if(length + buffer.Size() < receivingMethod.value) {
						buffer.Write(data, length);
						return;
					} else if(length + buffer.Size() >= receivingMethod.value) {
						int32_t c = receivingMethod.value - buffer.Size();
						buffer.Write(data, c);
						InternalOnDataBufferDone();
						length -= c;
						data += c;
					}
					break;
				case END_WITH_CHARACTER:
					{
						int i=0;
						for(i=0; i<length; ++i) {
							if(data[i] == (uint8_t)(char)receivingMethod.value)
								break;	
						}
						if(i < length) {
							++i;
							buffer.Write(data, i);
							length -= i;
							data += i;
							InternalOnDataBufferDone();
						} else {
							buffer.Write(data, length);
							return;
						}
					}
					break;
				default:
					// TODO error = not implemented
					throw "Error: Socket::OnData default: receivinMethod not implemented.";
			}
		}
	}
	
	void Socket::InternalOnDataBufferDone() {
		if(context->onReceiveMessage) {
			receivingMethod
				= context->onReceiveMessage(buffer, self.lock());
		}
		buffer.Clear();
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


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

#include <libusockets.h>
#include <mpmc_pool.hpp>
#include <concurrent.hpp>

#include "Buffer.hpp"
#include "Socket.hpp"
#include "Loop.hpp"
#include "Context.hpp"

#include "Event.hpp"

#include "Debug.hpp"

namespace net {
	namespace impl {
		concurrent::mpmc::pool<Event> eventPool;
	}

	void Event::Run() {
		switch(type) {
		case LISTEN_SOCKET_START:
			if(context) {
				context->InternalStartListening(
						(const char*)buffer_or_ip.Data(), port);
			} else {
				// TODO error
			}
			break;
		case LISTEN_SOCKET_STOP:
			if(context) {
				context->listenSockets.erase(listenSocket);
				us_listen_socket_close(context->ssl, listenSocket);
			} else {
				// TODO error
			}
			break;

		case SOCKET_CONNECT:
			if(context) {
				context->InternalConnect((char*)buffer_or_ip.Data(), port);
			} else {
				// TODO error
			}
			break;
		case SOCKET_CLOSE:
			if(socket) {
				socket->InternalClose();
			} else {
				// TODO error
			}
			break;
		case SOCKET_SEND:
			if(socket) {
				socket->InternalSend(buffer_or_ip);
			} else {
				// TODO error
			}
			break;
		
		case ALLCAST_LOOP:
			if(loop) {
				for(std::shared_ptr<Context> c : loop->contexts) {
					for(std::shared_ptr<Socket> s : c->sockets) {
						if(s != socket) {
							s->InternalSend(buffer_or_ip);
						}
					}
				}
			} else {
				// TODO error
			}
			break;
		case ALLCAST_CONTEXT:
			if(context) {
				for(std::shared_ptr<Socket> s : context->sockets) {
					if(s != socket) {
						s->InternalSend(buffer_or_ip);
					}
				}
			} else {
				// TODO error
			}
			break;

		default:
			break;
		}
		if(after) {
			after(*this);
		}
	}

	Event* Event::Allocate() {
		return impl::eventPool.acquire();
	}

	void Event::Free(Event* event) {
		if(event) {
			event->socket = NULL;
			event->context = NULL;
			event->loop = NULL;
			impl::eventPool.release(event);
		}
	}
}


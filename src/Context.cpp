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

#include "Context.hpp"
#include "Loop.hpp"

#include <sstream>
#include <iostream>
#include <functional>
#include <cstring>

#include "Debug.hpp"

namespace net {
	Context::~Context() {
		loop->contexts.erase(self.lock());
		CloseAll();
		delete *(std::shared_ptr<Context>**)us_socket_context_ext(ssl, context);
		*(std::shared_ptr<Context>**)us_socket_context_ext(ssl, context) = NULL;
	}
	
	void Context::CloseAll() {
		for(auto* s : listenSockets) {
			// TODO stop listening -> kill listen socket ?? does it work
			us_listen_socket_close(ssl, s);
		}
		listenSockets.clear();
		for(std::shared_ptr<Socket> s : sockets) {
			s->InternalClose();
		}
		sockets.clear();
	}
	
	void Context::StartListening(const char* ip, int port) {
		Event* event = Event::Allocate();
		event->buffer_or_ip.Write(ip, strlen(ip)+1);
		event->port = port;
		event->context = self.lock();
		event->type = Event::LISTEN_SOCKET_START;
		loop->PushEvent(event);
	}
	
	
	void Context::AllcastSend(Buffer& sendBuffer, std::shared_ptr<Socket> ignore) {
		Event* event = Event::Allocate();
		event->after = NULL;
		event->buffer_or_ip = std::move(sendBuffer);
		event->socket = ignore;
		event->context = self.lock();
		event->type = Event::ALLCAST_CONTEXT;
		if(loop == Loop::ThisThreadLoop())
			event->Run();
		else
			loop->PushEvent(event);
	}
	

	struct us_listen_socket_t* Context::InternalStartListening(const char* host,
			int port) {
		us_listen_socket_t* socket = us_socket_context_listen(ssl, context,
				host, port, 0, sizeof(Socket));
		listenSockets.insert(socket);
		return socket;
	}
	
	void Context::Connect(const char* ip, int port) {
		Event* event = Event::Allocate();
		event->buffer_or_ip.Write(ip, strlen(ip)+1);
		event->port = port;
		event->context = self.lock();
		event->type = Event::SOCKET_CONNECT;
		loop->PushEvent(event);
	}
	
	void Context::InternalConnect(const char* ip, int port) {
		us_socket_context_connect(ssl, context, ip, port, NULL, 0,
				sizeof(std::shared_ptr<Socket>*));
	}

	struct us_socket_t* Context::InternalOnDataSsl(struct us_socket_t* socket,
			char* data, int length) {
		std::shared_ptr<Socket> s = **(std::shared_ptr<Socket>**)
			us_socket_ext(1, socket);
		s->OnData((uint8_t*)data, length);
		return socket;
	}

	struct us_socket_t* Context::InternalOnOpenSsl(struct us_socket_t* socket,
			int isClient, char* ip, int ipLength) {
		std::shared_ptr<Socket> s(new Socket());
		*(std::shared_ptr<Socket>**)us_socket_ext(1, socket) =
			new std::shared_ptr<Socket>(s);
		
		s->ssl = 1;
		s->self = s;
		s->socket = socket;
		s->context = **(std::shared_ptr<Context>**)
			us_socket_context_ext(1, us_socket_context(1, socket));
		s->loop = s->context->loop;

		s->OnOpen(ip, ipLength);
		s->context->onNewSocket(s, isClient, s->remoteIp);

		return socket;
	}

	struct us_socket_t* Context::InternalOnEndSsl(struct us_socket_t* socket) {
		std::shared_ptr<Socket> s = **(std::shared_ptr<Socket>**)
			us_socket_ext(1, socket);
		s->OnEnd();
		return socket;
	}

	struct us_socket_t* Context::InternalOnCloseSsl(struct us_socket_t* socket,
			int code, void* reason) {
		std::shared_ptr<Socket> s = **(std::shared_ptr<Socket>**)
			us_socket_ext(1, socket);
		if(s->context->onCloseSocket)
			s->context->onCloseSocket(s, code, reason);
		s->OnClose(code, reason);
		return socket;
	}

	struct us_socket_t* Context::InternalOnTimeoutSsl(
			struct us_socket_t* socket) {
		std::shared_ptr<Socket> s = **(std::shared_ptr<Socket>**)
			us_socket_ext(1, socket);
		s->OnTimeout();
		return socket;
	}

	struct us_socket_t* Context::InternalOnWritableSsl(
			struct us_socket_t* socket) {
		std::shared_ptr<Socket> s = **(std::shared_ptr<Socket>**)
			us_socket_ext(1, socket);
		s->OnWritable();
		return socket;
	}

	std::shared_ptr<Context> Context::Make(std::shared_ptr<Loop> loop,
			std::function<void(std::shared_ptr<Socket>, bool, std::string)>
				onNewSocket,
			std::function<void(std::shared_ptr<Socket>, int, void*)>
				onCloseSocket,
			std::function<void(Buffer&, std::shared_ptr<Socket>)>
				onReceiveMessage,
			const char* keyFileName, const char* certFileName,
			const char* caFileName, const char* passphrase) {
		if(keyFileName == NULL || certFileName == NULL || caFileName == NULL) {
			DEBUG(" WARNING: Currently no-ssl tcp sockets are not suported! Do not use this context with listening sockets");
			
			fflush(stderr);
// 			return NULL;
		}
		struct us_socket_context_options_t options;
		options.cert_file_name = certFileName;
		options.key_file_name = keyFileName;
		options.passphrase = passphrase;
		options.ca_file_name = caFileName;
		us_socket_context_t* context = us_create_socket_context(1, loop->loop,
				sizeof(std::shared_ptr<Context>*), options);

		us_socket_context_on_open(1, context, Context::InternalOnOpenSsl);
		us_socket_context_on_data(1, context, Context::InternalOnDataSsl);
		us_socket_context_on_writable(1, context,
				Context::InternalOnWritableSsl);
		us_socket_context_on_close(1, context, Context::InternalOnCloseSsl);
		us_socket_context_on_timeout(1, context, Context::InternalOnTimeoutSsl);
		us_socket_context_on_end(1, context, Context::InternalOnEndSsl);

		std::shared_ptr<Context> c(new Context());
		*((std::shared_ptr<Context>**)us_socket_context_ext(1, context)) =
			new std::shared_ptr<Context>(c);
		c->self = c;

		c->context = context;
		c->loop = loop;
		c->userData = NULL;
		c->onNewSocket = onNewSocket;
		c->onReceiveMessage = onReceiveMessage;
		c->onCloseSocket = onCloseSocket;
		c->ssl = 1;

		loop->contexts.insert(c);

		return c;
	}
	
	
	
	std::string TranslateIp(const char* ip, int ipLength) {
		std::stringstream ss;
		if(ipLength == 4) {
			ss << std::dec;
			for(int i=0; i<ipLength; ++i) {
				if(i)
					ss << '.';
				ss << (unsigned)ip[i];
			}
		} else {
			ss << std::hex;
			for(int i=0; i<ipLength; i+=2) {
				if(i)
					ss << '.';
				ss << (((unsigned)ip[i]) | (((unsigned)ip[i+1])<<8));
			}
		}
		return ss.str();
	}
}


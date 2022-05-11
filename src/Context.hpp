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

#ifndef DORPC_NETWORKING_CONTEXT_HPP
#define DORPC_NETWORKING_CONTEXT_HPP

#include <functional>
#include <set>
#include <memory>
#include <libusockets.h>

#include "Buffer.hpp"
#include "Socket.hpp"

namespace net {
	class Context {
	private:
		Context() = default;
	public:
		
		~Context();
		
		struct us_socket_context_t* context;
		std::shared_ptr<Loop> loop;
		void* userData;
		std::function<void(std::shared_ptr<Socket>, bool, std::string)>
			onNewSocket;
		std::function<void(std::shared_ptr<Socket>, int, void*)> onCloseSocket;
		std::function<void(Buffer&, std::shared_ptr<Socket>)> onReceiveMessage;
		int ssl;
		std::weak_ptr<Context> self;
		std::set<std::shared_ptr<Socket>> sockets;
		std::set<struct us_listen_socket_t*> listenSockets;

		
		void CloseAll();

		void StartListening(const char* host, int port);
		struct us_listen_socket_t* InternalStartListening(const char* host,
				int port);
		
		void InternalConnect(const char* ip, int port);
		void Connect(const char* ip, int port);
		
		
		void AllcastSend(Buffer& sendBuffer, std::shared_ptr<Socket> ignore=NULL);


		static struct us_socket_t* InternalOnOpenSsl(struct us_socket_t* socket,
				int isClient, char* ip, int ipLength);
		static struct us_socket_t* InternalOnConnectioErrorOpenSsl(
				struct us_socket_t* socket, int code);
		static struct us_socket_t* InternalOnDataSsl(struct us_socket_t* socket,
				char* data, int length);
		static struct us_socket_t* InternalOnEndSsl(struct us_socket_t* socket);
		static struct us_socket_t* InternalOnCloseSsl(
				struct us_socket_t* socket, int code, void* reason);
		static struct us_socket_t* InternalOnTimeoutSsl(
				struct us_socket_t* socket);
		static struct us_socket_t* InternalOnWritableSsl(
				struct us_socket_t* socket);

		static std::shared_ptr<Context> Make(std::shared_ptr<Loop> loop,
				std::function<void(std::shared_ptr<Socket>, bool, std::string)>
					onNewSocket,
				std::function<void(std::shared_ptr<Socket>, int, void*)>
					onCloseSocket,
				std::function<void(Buffer&, std::shared_ptr<Socket>)>
					onReceiveMessage,
				const char* keyFileName, const char* certFileName,
				const char* caFileName, const char* passphrase);
	};
	
	std::string TranslateIp(const char* ip, int ipLength);
}

#endif


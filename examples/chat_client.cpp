
#include <Context.hpp>
#include <Loop.hpp>
#include <Socket.hpp>

#include <iostream>
#include <thread>
#include <cstring>
#include <string_view>

#include <Debug.hpp>

std::atomic<int> received_counter = 0;

int main(int argc, char** argv) {
	std::shared_ptr<net::Loop> loop = net::Loop::Make();
	std::shared_ptr<net::Context> context = net::Context::Make(loop, [=](
				std::shared_ptr<net::Socket> socket,
				bool isClient, std::string ip) {
				DEBUG(" Connected from: %s\n", ip.c_str());
			},
			[=](std::shared_ptr<net::Socket> socket, int ec, void* edata) {
				DEBUG(" Close connection");
			},
			[=](net::Buffer& buffer, std::shared_ptr<net::Socket> socket) {
				std::string_view v((char*)buffer.Data(), buffer.Size()-1);
				printf(" received: %s\n", v.data());
				fflush(stdout);
			},
			"cert/user.key", "cert/user.crt", "cert/rootca.crt", NULL);
	
	context->InternalConnect(argc==3 ? argv[1] : "127.0.0.1",
			argc>=2 ? atoi(argv[argc==3?2:1]) : 32367);
	
	std::thread thread_loop = std::thread([=]() {
			try {
				printf(" loop->Run();\n");
				loop->Run();
			} catch (...) {
				printf(" EXCEPTION... FAILED\n\n");
				fflush(stdout);
				exit(1);
			}
		});
	
	try {
		while(true) {
			std::string line;
			std::getline(std::cin, line);
			if(line != "") {
				net::Buffer buffer;
				buffer.Write(line.c_str(), line.size()+1);
				loop->AllcastSend(buffer);
			}
		}
	} catch (...) {
		printf(" EXCEPTION... FAILED\n\n");
		fflush(stdout);
		exit(2);
	}
	thread_loop.join();
	return 0;
}


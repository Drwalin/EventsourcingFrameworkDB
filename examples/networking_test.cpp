
#include <Context.hpp>
#include <Loop.hpp>
#include <Socket.hpp>

#include <thread>
#include <cstring>
#include <string_view>

const uint16_t ports[2] = {12345, 12346};

std::atomic<int> received_counter = 0;

void process(int portOpen, int portOther, int id) {
	std::shared_ptr<net::Loop> loop = net::Loop::Make();
	std::shared_ptr<net::Context> context = net::Context::Make(loop, [=](
				std::shared_ptr<net::Socket> socket,
				bool isClient, std::string ip) {
				net::Buffer buffer;
				char str[1024];
				sprintf(str, "Hello from %i to %i, has been sent", portOpen,
						portOther);
				buffer.Write(str, strlen(str)+1);
				socket->Send(buffer);
				socket->receivingMethod = {net::Socket::END_WITH_CHARACTER, 0};
			},
			[=](std::shared_ptr<net::Socket> socket, int ec, void* edata) {
			},
			[=](net::Buffer& buffer, std::shared_ptr<net::Socket> socket)
			-> net::Socket::ReceivingMethod {
				std::string_view v((char*)buffer.Data(), buffer.Size()-1);
				bool valid = v.starts_with("Hello from ")
						&& v.ends_with(", has been sent");
				if(valid == false) {
					std::this_thread::yield();
					printf(" tests ... FAILED\n\n");
					fflush(stdout);
					exit(1);
				} else {
					if(++received_counter == 4) {
						printf(" tests 4/4 ... OK\n\n");
						fflush(stdout);
						exit(0);
					}
				}
				return {net::Socket::END_WITH_CHARACTER, 0};
			},
			"cert/user.key", "cert/user.crt", "cert/rootca.crt", NULL);
	
	try {
		context->InternalStartListening("127.0.0.1", portOpen);
		context->Connect("127.0.0.1", portOther);
		loop->Run();
	} catch (...) {
		printf(" EXCEPTION... FAILED\n\n");
		fflush(stdout);
		exit(1);
	}
}

int main(int argc, char** argv) {
	std::thread thread = std::thread(process, ports[0], ports[1], 0);
	process(ports[1], ports[0], 1);
	
	thread.join();
	return 0;
}



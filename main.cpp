#include <iostream>
#include "inc/http_client.hpp"
#include "inc/http_service.hpp"

using namespace ericahttp;

int main(int argc, char **argv) {
	http_client client{std::string(argv[1])};
	client.set_op(GET);
	client.add_header_line("Cache-Control", "public");
	client.send_packet();
	auto t = client.recv_packet();
	std::cout << t.first << "\r\n" << t.second;
	return 0;
}

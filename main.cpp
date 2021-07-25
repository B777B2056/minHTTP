#include <iostream>
// #include "inc/http_client.hpp"
#include "inc/http_server.hpp"

using namespace ericahttp;

int main(int argc, char **argv) {
    int port;
    std::stringstream ss;
    ss << std::string(argv[1]);
    ss >> port;
	http_server<MultiPlexing, 100> hsm(port);
    hsm.event_loop();
	return 0;
}

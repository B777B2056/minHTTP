#include <iostream>
// #include "inc/http_client.hpp"
#include "inc/http_server.hpp"

using namespace ericahttp;

int main(int argc, char **argv) {
    int port;
    std::stringstream ss;
    ss << std::string(argv[1]);
    ss >> port;
	http_server<MultiProcess> hsm(port, 100);
    hsm.event_loop();
	return 0;
}

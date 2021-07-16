#include <iostream>
// #include "inc/http_client.hpp"
#include "inc/http_service.hpp"

using namespace ericahttp;

int main(int argc, char **argv) {
    int port;
    std::stringstream ss;
    ss << std::string(argv[1]);
    ss >> port;
	http_service_multiprocess hsm(port, 100);
    hsm.request_handler();
	return 0;
}

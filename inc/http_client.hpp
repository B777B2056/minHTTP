#ifndef HTTP_CLIENT
#define HTTP_CLIENT

#include <iostream>
#include <ctime>
#include <vector>
#include <string>
#include <utility>
#include <cstring>
#include <cstdlib>
#include <sstream>
#include "utils.hpp"

extern "C" {
	#include <netdb.h>
	#include <unistd.h>
	#include <arpa/inet.h>
	#include <sys/socket.h>
}

namespace ericahttp {
	enum OPERATE {GET, POST};

	// RAII HTTP Client
	class http_client {
		protected:
			// Operate type
			OPERATE _op;

			bool _is_set_op;

			// Service URL
			std::string _host;

			std::string _path;

			// HTTP version
			std::string _version;

			// Header line
			std::string _request_header;

			// Packet
			std::string _packet;

			// Client TCP socket
			int _client_socket;

			// Build header line
			virtual void _build_request_header();

			// Raise a TCP connection(Create a TCP socket)
			virtual void _create_tcp_conn();

			// Make a packet
			virtual void _make_packet(const std::string &body); 

			// Close HTTP client
			virtual void _client_close();

		public:
			http_client() = delete;

			http_client(const std::string &url);

			http_client(std::string &&url);

			virtual ~http_client();

			// Set operation
			virtual void set_op(OPERATE op);

			// Add header line
			virtual void add_header_line(const std::string &command,
										 const std::string &value);
			
			// Send a packet into HTTP Service(Send request message)
			virtual size_t send_packet(const std::string &body = "");

			// Get a packet which is returned by service, then return status code and body
			virtual std::pair<int, std::string> recv_packet(int buffer_len = 4096);
	};
}

#endif

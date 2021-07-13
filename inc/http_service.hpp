#include <string>
#include <vector>
#include <sstream>
#include <iostream>
#include "utils.hpp"

extern "C" {
    #include <sys/wait.h>
    #include <sys/socket.h>
    #include <string.h>
    #include <unistd.h>
    #include <signal.h>
    #include <arpa/inet.h>    
}

namespace ericahttp {
    // Base interface
    class _http_service_base {
        protected:
            // Welcome socket
            int _wel_socket = -1;
            // Create TCP socket, binding and listening port.
            virtual void _startup(int port, int max_client_num);
            // Accept TCP connection which is requested by client.
            // Return connection socket.
            virtual int _accept() = 0;
            // Create status line and header line
            virtual void _build_header(int status_code) = 0;
            // Make response packet
            virtual std::string _make_packet(const std::string &body) = 0;
            // Send data to client.
            virtual void _send_response(const std::string &body) = 0;
            // Receive data from client.
            virtual std::vector<std::string> _recv_request() = 0;
            // Run CGI script
            // virtual void execute_cgi() = 0;
            // Error handler.
            virtual void _error_handler(std::string &&msg);

        public:
            virtual ~_http_service_base() {}
            // Accept TCP connection and handle request
            virtual void request_handler() = 0;
    };

    // Multi-processing Service
    class http_service_multiprocess 
        : public _http_service_base {
        private:
            // Response packet status line and header line
            std::string _header;
            // Client info
            sockaddr_in _client_msg;
            // Signal when process is dead
            void _pdead(int pid);
            // Register the signal for preventing zombie process
            void _pinit();
            
            virtual int _accept();
            virtual void _build_header(int status_code);
            virtual std::string _make_packet(const std::string &body);
            virtual void _send_response(const std::string &body);
            virtual std::vector<std::string> _recv_request();
        
        public:
            http_service_multiprocess(int port, int max_client_num = 100);
            virtual ~http_service_multiprocess();
            virtual void request_handler();
    };
}



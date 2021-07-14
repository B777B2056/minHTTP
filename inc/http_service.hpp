#ifndef HTTP_SERVICE
#define HTTP_SERVICE

#include <string>
#include <vector>
#include <sstream>
#include <utility>
#include <iostream>
#include <unordered_map>
#include "utils.hpp"

extern "C" {
    #include <sys/types.h>
    #include <sys/wait.h>
    #include <sys/socket.h>
    #include <string.h>
    #include <unistd.h>
    #include <signal.h>
    #include <arpa/inet.h>    
}

namespace ericahttp {
    // Error handler.
    void _error_handler(std::string &&msg);

    // Base interface
    class _http_service_base {
        protected:
            // Welcome socket
            int _wel_socket = -1;
            // Create TCP socket, binding and listening port.
            virtual void _startup(int port, int max_client_num);
            // Accept TCP connection which is requested by client.
            // Return connection socket.
            virtual int _accept(sockaddr_in &client_msg);
            // Create status line and header line
            virtual std::string _build_header(int status_code) const;
            // Make response packet
            virtual std::string _make_packet(const std::string &header, 
                                             const std::string &body);
            // Receive packet from client's request
            std::pair<int, std::string> _recv_packet(int client);
            // GET method.
            virtual std::string _get(const std::string &path);
            // POST method(Excute CGI file).
            virtual void _post(const std::string &path,
                               const std::string &para);
            // Analyze status line
            virtual std::pair<int, std::string> _extract_status_line(const std::string &status_line);
            // Analyze header lines
            virtual void _extract_header_line(const std::vector<std::string> &header_lines);
            // Analyze body
            virtual void _extract_body(const std::string &body);

        public:
            // Status code-description table
            static std::unordered_map<int, std::string> _status_table;

            virtual ~_http_service_base() {}

            // Accept TCP connection and handle request
            virtual void request_handler() = 0;
    };

    // Signal when process is dead
    void _pdead(int pid);

    // Multi-processing Service
    class http_service_multiprocess 
        : public _http_service_base {
        private:
            // Client info
            sockaddr_in _client_msg;
            // Register the signal for preventing zombie process
            void _pinit();
        
        public:
            http_service_multiprocess(int port, int max_client_num = 100);
            virtual ~http_service_multiprocess();
            virtual void request_handler() override;
    };
}

#endif



#ifndef HTTP_SERVER
#define HTTP_SERVER

#include <fstream>
#include <string>
#include <vector>
#include <sstream>
#include <utility>
#include <iostream>
#include <unordered_map>
#include "utils.hpp"

extern "C" {
    #include <errno.h>
    #include <sys/types.h>
    #include <sys/wait.h>
    #include <sys/socket.h>
    #include <string.h>
    #include <unistd.h>
    #include <signal.h>
    #include <arpa/inet.h>    
}

namespace ericahttp {
    // Enum 
    enum server_type {MultiProcess, MultiThread, MultiPlexing};
    // Base interface
    class _http_server_base {
        protected:
            // Welcome socket
            int _wel_socket = -1;
            // Create TCP socket, binding and listening port.
            virtual void _startup(int port, int max_client_num);
            // Accept TCP connection which is requested by client.
            // Return connection socket.
            virtual int _accept(sockaddr_in &client_msg);
            // Make response packet
            virtual void _send_packet(int client, int status_code,
                                      const std::string &body);
            // Receive packet from client's request
            std::string _recv_packet(int client);
            // GET method.
            virtual void _get(int client, 
                              std::string &path,
                              const std::string &para);
            // POST method.
            virtual void _post(int client, 
                               std::string &path,
                               const std::string &body);
            // Execute CGI program.
            virtual void _execute_cgi(int client, int method,
                                      const std::string &path,
                                      const std::string &para);
            // Analyze status line
            virtual void _request_handler(int client,
                                          const std::string &status_line, 
                                          const std::string &body);

        public:
            // Status code-description table
            static std::unordered_map<int, std::string> _status_table;

            virtual ~_http_server_base() {}

            // Accept TCP connection and handle request
            virtual void event_loop() = 0;
    };

    template<server_type T>
    class http_server {};

    // Multi-process Server
    template< >
    class http_server<MultiProcess> 
        : public _http_server_base {
        private:
            // Client info
            sockaddr_in _client_msg;
        
        public:
            http_server(int port, int max_client_num = 100) {
                this->_startup(port, max_client_num);
                std::cout << "Listening..." << std::endl;
            }

            virtual ~http_server() {
                // Close welcome socket 
                close(_wel_socket);
            }

            virtual void event_loop() override {
                while(true) {
                    int client = this->_accept(_client_msg);
                    std::cout << "Accepted..." << std::endl;
                    if(client != -1) {
                        // Create new process
                        pid_t child = fork();
                        if(child < 0) {
                           this->_send_packet(client, 503, "");
                        } else if(child == 0) {
                            std::cout << "Ready to receive packet..." << std::endl;
                            // Handle request in child process
                            // Extract request packet
                            std::string msg = this->_recv_packet(client);
                            std::cout << "Received packet..." << std::endl;
                            auto lines = split(msg, "\r\n\r\n");
                            if(lines.size() < 1) {
                                this->_send_packet(client, 400, "");
                        } else {
                            std::string body{lines[1].begin(), lines[1].end()};    // request packet body
                            lines = split(lines[0], "\r\n");    
                            std::string status_line = lines[0];    // request status_line
                            std::vector<std::string> header_lines{lines.begin() + 1, lines.end()};    // request header lines
                            // Analyze statue_line and do event
                            this->_request_handler(client, status_line, body);
                        }
                        std::cout << "Sended response packet..." << std::endl;
                        std::cout << "Connection closed..." << std::endl;
                    } else {
                        // Close child process
                        wait(nullptr);
                    }
                    // Close client TCP connection
                    close(client);
                } 
            }
        }
    };
    
    // Multi-thread Server

    // Multi-plexing Server

}

#endif



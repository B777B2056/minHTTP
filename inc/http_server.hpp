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
    #include <sys/select.h>
    #include <sys/epoll.h>
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
                                          const std::string &msg);

        public:
            // Status code-description table
            static std::unordered_map<int, std::string> _status_table;

            virtual ~_http_server_base() {}

            // Accept TCP connection and handle request
            virtual void event_loop() = 0;
    };

    template <server_type T, int MAX_CONN>
    class http_server {};

    // Multi-process Server
    template <int MAX_CONN>
    class http_server<MultiProcess, MAX_CONN> 
        : public _http_server_base {
        public:
            http_server(int port) {
                this->_startup(port, MAX_CONN);
            }

            virtual ~http_server() {
                // Close welcome socket 
                close(_wel_socket);
            }

            virtual void event_loop() override {
                for(;;) {
                    sockaddr_in client_msg;
                    int client = this->_accept(client_msg);
                    if(client != -1) {
                        // Create new process
                        pid_t child = fork();
                        if(child < 0) {
                           this->_send_packet(client, 503, "");
                        } else if(child == 0) {
                            // Handle request in child process
                            std::string msg = this->_recv_packet(client);
                            this->_request_handler(client, msg);
                        } else {
                            // Close child process
                            wait(nullptr);
                        }
                        // Close client TCP connection
                        close(client);
                    } else {
                        std::cout << "Client error" << std::endl;
                    }
                }
            }
        };
    
    // Multi-thread Server

    // Multi-plexing Server
    template <int MAX_CONN>
    class http_server<MultiPlexing, MAX_CONN> 
        : public _http_server_base {
        private:
            bool _is_select;

            // Select
            void _select_loop() {
                // select info
                int fd_max = _wel_socket;
                fd_set rfds, rfds_copy;
                timeval tv;
                // Unset fd_set all pos
                FD_ZERO(&rfds);
                // Set pos for server socket
                FD_SET(_wel_socket, &rfds);
                // Event loop
                for(;;) {
                    // Save rfds
                    rfds_copy = rfds;
                    // Set timeout
                    tv.tv_sec = 5;
                    tv.tv_usec = 5000;
                    // Select
                    int ret = select(fd_max + 1, &rfds_copy, nullptr, nullptr, &tv);
                    if(ret == -1) {
                        std::cout << "Select error" << std::endl;
                    } else if(ret == 0) {
                        continue;    // Time out
                    } else {
                        // Scan all pos, find state changed pos and handler it
                        for(int i = 0; i <= fd_max; ++i) {
                            if(FD_ISSET(i, &rfds_copy)) {
                                if(i == _wel_socket) {
                                    sockaddr_in client_msg;
                                    int client = this->_accept(client_msg);
                                    if(client == -1) {
                                        std::cout << "Client error" << std::endl;
                                        continue;
                                    }
                                    FD_SET(client, &rfds);    // register client file descriptor
                                    if(client > fd_max)    // update max fd
                                        fd_max = client;
                                } else {
                                    std::string msg = this->_recv_packet(i);
                                    if(msg.empty()) {
                                        // EOF flag
                                        FD_CLR(i, &rfds);   // logout client file descriptor
                                        close(i);
                                    } else {
                                        // Handle request
                                        this->_request_handler(i, msg);
                                    }
                                }
                            }
                        }
                    }
                }
            }
            
            // Epoll
            void _epoll_loop() {
                int epfd = epoll_create(MAX_CONN);
                epoll_event *ep_events = new epoll_event[MAX_CONN];
                epoll_event event;
                event.events = EPOLLIN;
                event.data.fd = _wel_socket;
                epoll_ctl(epfd, EPOLL_CTL_ADD, _wel_socket, &event);
                for(;;) {
                    int event_cnt = epoll_wait(epfd, ep_events, MAX_CONN, -1);
                    if(event_cnt == -1) {
                        std::cout << "Epoll error" << std::endl;
                        break;
                    }
                    for(int i = 0; i < event_cnt; ++i) {
                        int cur_fd = ep_events[i].data.fd;
                        if(cur_fd == _wel_socket) {
                            sockaddr_in client_msg;
                            int client = this->_accept(client_msg);
                            if(client == -1) {
                                std::cout << "Client error" << std::endl;
                                continue;
                            }
                            event.events = EPOLLIN;
                            event.data.fd = client;
                            epoll_ctl(epfd, EPOLL_CTL_ADD, client, &event);    // register client file descriptor
                        } else {
                            std::string msg = this->_recv_packet(cur_fd);
                            if(msg.empty()) {
                                // EOF flag
                                epoll_ctl(epfd, EPOLL_CTL_DEL, cur_fd, nullptr);   // logout client file descriptor
                                close(cur_fd);
                            } else {
                                // Handle request
                                this->_request_handler(cur_fd, msg);
                            }
                        }
                    }
                }
                close(epfd);
                delete[] ep_events;
            }

        public:
            http_server(int port, bool is_select = false)
                : _is_select(is_select) {
                this->_startup(port, MAX_CONN);
            }

            virtual ~http_server() {
                // Close welcome socket 
                close(_wel_socket);
            }

            virtual void event_loop() override {
                if(this->_is_select) 
                    this->_select_loop();
                else
                    this->_epoll_loop();
            }
        };
}

#endif



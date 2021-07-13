#include "../inc/http_service.hpp"

namespace ericahttp {
    // Interface class
    void _http_service_base::_startup(int port, int max_client_num) {
        _wel_socket = socket(PF_INET, SOCK_STREAM, 0);
        if(_wel_socket == -1)
            this->_error_handler("socket");
        sockaddr_in serv_addr;
        memset(&serv_addr, 0, sizeof(serv_addr));
        _wel_socket.sin_family = AF_INET;
        _wel_socket.sin_addr.s_addr = htonl(INADDR_ANY);
        _wel_socket.sin_port = htons(port);
        if(bind(_wel_socket, 
                reinterpret_cast<sockaddr*>(&serv_addr), 
                sizeof(serv_addr)) == -1)
            this->_error_handler("bind");
        if(listen(_wel_socket, max_client_num) == -1)
            this->_error_handler("listen");
    }

    void _http_service_base::_error_handler(std::string &&msg) {
        switch(msg) {
            case "socket":
                throw socketCreateException();
                break;
            case "bind":
                break;
            case "listen":
                break;
            case "fork":
                break;
            case "signal":
                break;
            case "waitpid":
                break;
        }
    }

    // Multi-process service
    http_service_multiprocess::http_service_multiprocess(int port, int max_client_num)
        : _status_line(""), _header_line("") {
        this->_pinit();
        this->_startup(port, max_client_num);
    }

    http_service_multiprocess::~http_service_multiprocess() {
        close(_wel_socket);
    }

    void http_service_multiprocess::_pdead(int pid) {
        int wstatus;
        if(waitpid(pid, &wstatus, WNOHANG) == -1)
            this->_error_handler("waitpid");
    }

    void http_service_multiprocess::_pinit() {
        sigaction sa;
        sa.sa_handler = _pdead;
        sa.sa_flags = 0;
        sigemptyset(&sa.sa_mask);
        if(sigaction(SIGCHLD, &sa, 0) == -1)
            this->_error_handler("signal");
    }

    int http_service_multiprocess::_accept() {
        auto csize = sizeof(_client_msg);
        return accept(_wel_socket, reinterpret_cast<sockaddr*>(&_client_msg), &csize);
    }

    void http_service_multiprocess::_build_header(int status_code) {
        
    }

    std::string http_service_multiprocess::_make_packet(const std::string &body) {
        std::stringstream ss;
        ss << _header << "\r\n" << body;
        return ss.str();
    }

    void http_service_multiprocess::_send_response(const std::String &body) {
        
    }

    std::vector<std::string> http_service_multiprocess::_recv_request() {
    
    }

    void http_service_multiprocess::request_handler() {
        while(true) {
            int client = this->_accept();
            if(client != -1) {
                // Create new process
                pid_t child = fork();
                if(child == -1) {
                    close(client);
                    this->_error_handler("fork");
                }
                else if(child == 0) {
                    // Close welcome socket 
                    close(_wel_socket);
                    // Handle request in child process
                    
                } else {
                    // Client connection socket is moved to child process, so close redundant socket
                    close(client);
                }
            } 
        }
    } 
}



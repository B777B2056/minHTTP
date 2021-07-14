#include "../inc/http_service.hpp"

namespace ericahttp {
    void _error_handler(std::string &&msg) {
        if(msg == "socket") {
            throw socketCreateException();
        } else if(msg == "bind") {

        } else if(msg == "listen") {

        } else if(msg == "fork") {

        } else if(msg == "signal") {

        } else if(msg == "waitpid") {

        }
    }

    // Interface class
    std::unordered_map<int, std::string> 
    _http_service_base::_status_table{{100, "Continue"}, {101, "Switching Protocols"}, {102, "Processing"},
                                      {200, "OK"}, {201, "Created"}, {202, "Accepted"}, {203, "Non-Authoritative Information"}, {204, "No Content"},
                                      {300, "Multiple Choices"}, {301, "Moved Permanently"}, {302, "Move temporarily"}, {304, "Not Modified"},
                                      {400, "Bad Request"}, {403, "Forbidden"}, {404, "Not Found"},
                                      {500, "Internal Server Error"}, {501, "Not Implemented"}, {502, "Bad Gateway"}, {503, "Service Unavailable"}, {505, "HTTP Version Not Supported"}
                                     };
    
    void _http_service_base::_startup(int port, int max_client_num) {
        _wel_socket = socket(PF_INET, SOCK_STREAM, 0);
        if(_wel_socket == -1)
            _error_handler("socket");
        sockaddr_in serv_addr;
        memset(&serv_addr, 0, sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
        serv_addr.sin_port = htons(port);
        if(bind(_wel_socket, 
                reinterpret_cast<sockaddr*>(&serv_addr), 
                sizeof(serv_addr)) == -1)
            _error_handler("bind");
        if(listen(_wel_socket, max_client_num) == -1)
            _error_handler("listen");
    }

    int _http_service_base::_accept(sockaddr_in &client_msg) {
        socklen_t csize = sizeof(client_msg);
        return accept(_wel_socket, reinterpret_cast<sockaddr*>(&client_msg), &csize);
    }

    std::string _http_service_base::_build_header(int status_code) const {
        std::stringstream ss;
        ss << "HTTP/1.1" << " " << status_code << _status_table[status_code] << "\r\n"
           << "";
        return ss.str();
    }

    std::string _http_service_base::_make_packet(const std::string &header, 
                                                 const std::string &body) {
        std::stringstream ss;
        ss << header << "\r\n" << body;
        return ss.str();
    }

    std::pair<int, std::string> _http_service_base::_recv_packet(int client) {
        int content_len = 0;
		std::string data_packet("");
		char tmp[4096];
		while(true) {
			ssize_t len = recv(client, tmp, 4096, 0);
			if(len <= 0)
				break;
			data_packet.append(tmp, 4096);
			auto index = data_packet.find("Content-Length");
			if(index != std::string::npos) {
				std::string str = "";
				for(; data_packet[index] != ':'; ++index);
				++index;
				for(; data_packet[index] != '\r'; ++index)
					str += data_packet[index];
				std::stringstream ss(str);
				ss >> content_len;
				break;
			}
			memset(tmp, 0, 4096);
		}
        return std::make_pair(content_len, data_packet);
    }

    std::string _http_service_base::_get(const std::string &path) {
        
    }

    void _http_service_base::_post(const std::string &path,
                                   const std::string &para) {
    
    }

    std::pair<int, std::string> _http_service_base::_extract_status_line(const std::string &status_line) {
        int status_code = -1;
        std::string rbody = "";
        auto status_words = split(status_line);
        // HTTP Version Not Supported
        if(status_words[2] != "HTTP/1.1") {
            status_code = 505;
        }
        // GET
        if(status_words[0] == "GET") {
            try {
                rbody = _get(status_words[1]);
                status_code = 200;
            } catch(...) {
                status_code = 404;
            }
        // POST
        } else if(status_words[0] == "POST") {
            try {
                // Extract file's name and paraments.
                auto fp = split(status_words[1], "?");
                _post(fp[0], fp[1]);
                status_code = 200;
            } catch(...) {
                status_code = 502;
            }
        }
        return std::make_pair(status_code, rbody);
    }

    void _http_service_base::_extract_header_line(const std::vector<std::string> &header_lines) {

    }
    
    void _http_service_base::_extract_body(const std::string &body) {
        if(body.empty())
            return;
    }

    // Multi-process service
    void _pdead(int pid) {
        int wstatus;
        if(waitpid(pid, &wstatus, WNOHANG) == -1)
            _error_handler("waitpid");
    }

    http_service_multiprocess::http_service_multiprocess(int port, int max_client_num) {
        this->_pinit();
        this->_startup(port, max_client_num);
    }

    http_service_multiprocess::~http_service_multiprocess() {
        // Close welcome socket 
        close(_wel_socket);

    }

    void http_service_multiprocess::_pinit() {
        struct sigaction sa;
        sa.sa_handler = _pdead;
        sa.sa_flags = 0;
        sigemptyset(&sa.sa_mask);
        if(sigaction(SIGCHLD, &sa, 0) == -1)
            _error_handler("signal");
    }

    void http_service_multiprocess::request_handler() {
        while(true) {
            int client = this->_accept(_client_msg);
            if(client != -1) {
                // Create new process
                pid_t child = fork();
                if(child == -1) {
                    close(client);
                    _error_handler("fork");
                }
                else if(child == 0) {
                    // Handle request in child process
                    int status_code;    // status code
                    // Extract request packet
                    auto mc = this->_recv_packet(client);
                    int content_len = mc.first;
                    std::string msg = mc.second;
                    auto lines = split(msg, "\r\n\r\n");
                    if(lines.size() < 1) {
                        status_code = 400;
                        close(client);
                        continue;
                    }
                    std::string body{lines[1].begin(), lines[1].begin() + content_len};    // request packet body
                    lines = split(lines[0], "\r\n");    
                    std::string status_line = lines[0];    // request status_line
                    std::vector<std::string> header_lines{lines.begin() + 1, lines.end()};    // request header lines
                    // Analyze statue_line
                    auto sl = _extract_status_line(status_line);
                    status_code = sl.first;
                    if(status_code == -1) {
                        close(client);
                        continue;
                    }
                    if(status_code == 200) {
                        // Analyze header line(Except "Content-Length")
                        this->_extract_header_line(header_lines);
                        // Analyze body
                        this->_extract_body(body);
                    }
                    // Send response packet
                    std::string response = this->_make_packet(this->_build_header(status_code), sl.second);
                    send(client, response.c_str(), response.length(), 0);
                    // Close client connection
                    close(client);
                } else {
                    // Client connection socket is moved to child process, so close redundant socket
                    close(client);
                }
            } 
        }
    } 
}


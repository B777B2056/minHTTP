#include "../inc/http_service.hpp"

namespace ericahttp {
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
            throw socketCreateException();
        sockaddr_in serv_addr;
        memset(&serv_addr, 0, sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
        serv_addr.sin_port = htons(port);
        if(bind(_wel_socket, 
                reinterpret_cast<sockaddr*>(&serv_addr), 
                sizeof(serv_addr)) == -1)
            throw bindException();
        if(listen(_wel_socket, max_client_num) == -1)
            throw listenException();
    }

    int _http_service_base::_accept(sockaddr_in &client_msg) {
        socklen_t csize = sizeof(client_msg);
        return accept(_wel_socket, reinterpret_cast<sockaddr*>(&client_msg), &csize);
    }

    std::string _http_service_base::_build_header(int status_code) const {
        std::stringstream ss;
        ss << "HTTP/1.1" << " " << status_code << " " << _status_table[status_code];
        return ss.str();
    }

    std::string _http_service_base::_make_packet(const std::string &header, 
                                                 const std::string &body) {
        std::stringstream ss;
        ss << header << "\r\n" 
           << "Content-Length:" << body.length() << "\r\n"
           << "\r\n"
           << body;
        return ss.str();
    }

    std::pair<int, std::string> _http_service_base::_recv_packet(int client) {
		std::string data_packet("");
		char tmp[4096];
		ssize_t content_len = recv(client, tmp, 4096, 0);
		data_packet.append(tmp, 4096);
        return std::make_pair(content_len, data_packet);
    }

    std::string _http_service_base::_get(std::string &path,
                                         const std::string &para) {
        std::string content;
        if(path[0] == '/')
            path.erase(path.begin());
        if(path.find("/cgi") == std::string::npos) {
            // Static file(read only)
            std::ifstream target(path);
            if(!target)
                throw notFoundException();
            std::stringstream buf;
            buf << target.rdbuf();
            content = buf.str();
            target.close();
        } else {
            // Dynamic file
            try {
                content = this->_execute_cgi(path, para);
            } catch(...) {
                throw cgiException();
            }
        }
        std::cout << "Content:\n" << content << std::endl;
        return content;
    }

    void _http_service_base::_post(std::string &path,
                                   const std::string &body) {
        if(path.find("/cgi") == std::string::npos) {
            // Static file(write into)
            std::ofstream target(path, std::ios::app);
            if(!target)
                throw notFoundException();
            auto p = split(body, "&");    // split para
            for(std::string s : p)
                target << s << "\r\n";
            target.close();
        } else {
             // Dynamic file
            try {
                this->_execute_cgi(path, body);
            } catch(...) {
                throw cgiException();
            }
        }
    }

    std::string _http_service_base::_execute_cgi(const std::string &path,
                                                 const std::string &para) {
        auto p = split(para, "&");    // split para

    }

    std::pair<int, std::string> _http_service_base::_extract_status_line(const std::string &status_line, 
                                                                         const std::string &body) {
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
                // Extract file's name and paraments.
                auto fp = split(status_words[1], "?");
                rbody = _get(fp[0], fp[1]);
                status_code = 200;
            } catch(notFoundException ne) {
                status_code = 404;
            } catch(cgiException ce) {
                status_code = 502;
            }
        // POST
        } else if(status_words[0] == "POST") {
            try {
                _post(status_words[1], body);
                status_code = 200;
            } catch(notFoundException ne) {
                status_code = 404;
            } catch(cgiException ce) {
                status_code = 502;
            }

        }
        return std::make_pair(status_code, rbody);
    }

    // Multi-process service
    void _pdead(int pid) {
        int wstatus;
        if(waitpid(pid, &wstatus, WNOHANG) == -1)
            std::cout << "Warning: kill zombie process failed, pid is " << pid << std::endl;
    }

    http_service_multiprocess::http_service_multiprocess(int port, int max_client_num) {
        this->_pinit();
        this->_startup(port, max_client_num);

        std::cout << "Listening..." << std::endl;
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
            std::cout << "Warning: signal register failed, maybe rise zombie process" << std::endl;
    }

    void http_service_multiprocess::request_handler() {
        while(true) {
            int client = this->_accept(_client_msg);

            std::cout << "Accepted..." << std::endl;

            if(client != -1) {
                int status_code;    // status code
                std::string response;    // response packet
                // Create new process
                pid_t child = fork();
                if(child == -1) {
                    std::cout << "Child process create failed..." << std::endl;

                    status_code = 503;
                    response = this->_make_packet(this->_build_header(status_code), "");
                    // Send response packet
                    send(client, response.c_str(), response.length(), 0);
                    close(client);
                }
                else if(child == 0) {
                    std::cout << "Ready to receive packet..." << std::endl; 
                    // Handle request in child process
                    // Extract request packet
                    auto mc = this->_recv_packet(client);
                    int content_len = mc.first;
                    std::string msg = mc.second;

                    std::cout << "Recv msg's len is " << content_len << " bytes" << std::endl;
                    std::cout << msg << std::endl;

                    auto lines = split(msg, "\r\n\r\n");
                    if(lines.size() < 1) {
                        std::cout << "400 Bad request" << std::endl;

                        status_code = 400;
                        response = this->_make_packet(this->_build_header(status_code), "");
                    } else {
                        std::string body{lines[1].begin(), lines[1].begin() + content_len};    // request packet body
                        lines = split(lines[0], "\r\n");    
                        std::string status_line = lines[0];    // request status_line
                        std::vector<std::string> header_lines{lines.begin() + 1, lines.end()};    // request header lines

                        std::cout << "Status line:\n" << status_line << std::endl;
                        std::cout << "Header line:\n";
                        for(auto h : header_lines)
                            std::cout << h << std::endl;
                        std::cout << "Body:\n" << body << std::endl;

                        // Analyze statue_line
                        auto sl = _extract_status_line(status_line, body);
                        status_code = sl.first;
                        if(status_code == -1) {
                            close(client);
                            continue;
                        } else if(status_code == 200) {
                            response = this->_make_packet(this->_build_header(status_code), sl.second);
                        } else {
                            response = this->_make_packet(this->_build_header(status_code), "");
                        }
                    }
                    // Send response packet
                    std::cout << "Response:\n" << response << std::endl;
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


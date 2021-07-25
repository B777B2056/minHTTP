#include "../inc/http_server.hpp"

namespace ericahttp {
    // Interface class
    std::unordered_map<int, std::string> 
    _http_server_base::_status_table{{100, "Continue"}, {101, "Switching Protocols"}, {102, "Processing"},
                                      {200, "OK"}, {201, "Created"}, {202, "Accepted"}, {203, "Non-Authoritative Information"}, {204, "No Content"},
                                      {300, "Multiple Choices"}, {301, "Moved Permanently"}, {302, "Move temporarily"}, {304, "Not Modified"},
                                      {400, "Bad Request"}, {403, "Forbidden"}, {404, "Not Found"},
                                      {500, "Internal Server Error"}, {501, "Not Implemented"}, {502, "Bad Gateway"}, {503, "Service Unavailable"}, {505, "HTTP Version Not Supported"}
                                     };
    
    void _http_server_base::_startup(int port, int max_client_num) {
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

    int _http_server_base::_accept(sockaddr_in &client_msg) {
        socklen_t csize = sizeof(client_msg);
        return accept(_wel_socket, reinterpret_cast<sockaddr*>(&client_msg), &csize);
    }

    void _http_server_base::_send_packet(int client, int status_code,
                                          const std::string &body) {
        std::stringstream ss;
        ss << "HTTP/1.1" << " " << status_code << " " << _status_table[status_code] << "\r\n"
           << "Content-Length:" << body.length() << "\r\n"
           << "\r\n"
           << body;
        send(client, ss.str().c_str(), ss.str().length(), 0);
    }

    std::string _http_server_base::_recv_packet(int client) {
		std::string data_packet("");
        /*
		char ch;
        while(read(client, &ch, 1) > 0) {
            data_packet += ch;
        }
        */
        char tmp[8092];
		recv(client, tmp, sizeof(tmp), 0);
		data_packet.append(tmp, sizeof(tmp));
        return data_packet;
    }

    void _http_server_base::_get(int client, 
                                  std::string &path,
                                  const std::string &para) {
        if(path[0] == '/')
            path.erase(path.begin());
        std::ifstream target(path);
        if(!target) {
            this->_send_packet(client, 404, "");
            return;
        }
        if(path.find("cgi-bin") == std::string::npos) {
            // Static file(read only)
            std::stringstream buf;
            buf << target.rdbuf();
            target.close();
            this->_send_packet(client, 200, buf.str());
        } else {
            // Dynamic file
            this->_execute_cgi(client, 0, path, para);
        }
    }

    void _http_server_base::_post(int client,
                                   std::string &path,
                                   const std::string &body) {
        if(path[0] == '/')
            path.erase(path.begin());
        if(path.find("cgi-bin") == std::string::npos) {
            // Static file(write into)
            std::string p;
            std::ofstream target(path, std::ios::app);
            if(!target) {
                this->_send_packet(client, 404, "");
            } else {
                auto str = split(body, "&");    // split para
                for(std::string s : str)
                    target << s << "\r\n";
                target.close();
                this->_send_packet(client, 200, "");
            }
        } else {
            // Dynamic file
            this->_execute_cgi(client, 1, path, body);
        }
    }

    // REMEMBER: PIPE FIRST, FORK AFTER!!!!!!!!!!!
    void _http_server_base::_execute_cgi(int client, int method,
                                          const std::string &path,
                                          const std::string &para) {
        int to_cgi[2];    // Parent process to CGI process, input para to stdin for POST
        int from_cgi[2];    // CGI process to parent process, output result from stdout
        // Send header line
        if(pipe(from_cgi) == -1) {
            this->_send_packet(client, 502, "pipe error"); 
            return;
        }
        if(pipe(to_cgi) == -1) {
            this->_send_packet(client, 502, "pipe error");
            return;
        }
        pid_t pid = fork();
        if(pid < 0) {
            this->_send_packet(client, 502, "fork error");
        } else if(pid == 0) {
            // Close input pipe write channel
            close(to_cgi[1]);
            // Close output pipe read channel
            close(from_cgi[0]);
            // Redirect
            dup2(to_cgi[0], STDIN_FILENO);
            dup2(from_cgi[1], STDOUT_FILENO);
            switch(method) {
                // GET method
                case 0:
                    // Set environment variable
                    setenv("REQUEST_METHOD", "GET", 1);
                    setenv("QUERY_STRING", para.c_str(), 1);
                    break;
                // POST method
                case 1:
                    // Set environment variable
                    std::stringstream ss;
                    ss << para.length();
                    setenv("REQUEST_METHOD", "POST", 1);
                    setenv("CONTENT_LENGTH", ss.str().c_str(), 1);
                    break;
            }
            // Run CGI program
            execl(path.c_str(), path.c_str(), nullptr);
            exit(0);
        } else {
            // Close output pipe write channel
            close(from_cgi[1]);
            // Close input pipe read channel
            close(to_cgi[0]);
            // POST: send para to CGI
            if(method == 1)
                write(to_cgi[1], para.c_str(), para.length());
            // Receive data from CGI
            char ch;
            std::string content("");
            while(read(from_cgi[0], &ch, 1) > 0) {
                content += ch;
            }
            // Close other pipe
            close(from_cgi[0]);
            close(to_cgi[1]);
            // Send packet
            this->_send_packet(client, 200, content);
            // Close CGI process
            wait(nullptr);
        }
    }

    void _http_server_base::_request_handler(int client,
                                             const std::string &msg) {
        auto lines = split(msg, "\r\n\r\n");
	    if(lines.size() < 1) {
		    this->_send_packet(client, 400, "");
	    	return;
	    } 
	    std::string body{lines[1]};    // request packet body
	    lines = split(lines[0], "\r\n");    
	    std::string status_line = lines[0];    // request status_line
	    std::vector<std::string> header_lines{lines.begin() + 1, lines.end()};    // request header lines
        std::string rbody = "";
        auto status_words = split(status_line);
        // HTTP Version Not Supported
        if(status_words[2] != "HTTP/1.1") {
            this->_send_packet(client, 505, "");
        } else {
            // GET
            if(status_words[0] == "GET") {
                // Extract file's name and paraments.
                auto fp = split(status_words[1], "?");
                _get(client, fp[0], fp[1]);
            // POST
            } else if(status_words[0] == "POST") {
                _post(client, status_words[1], body);
            }
        }
    }
}


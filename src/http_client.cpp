#include "../inc/http_client.hpp"

namespace ericahttp {
	http_client::http_client(const std::string &url) 
		: _is_set_op(false), _host(""), _path(""),
		  _version("HTTP/1.1"), _request_header(""),  
		  _packet(""), _client_socket(-1) {
		ericahttp::split_url(url, _host, _path);
		this->_create_tcp_conn();
	}

	http_client::http_client(std::string &&url)
		: _is_set_op(false), _host(""), _path(""),
		  _version("HTTP/1.1"), _request_header(""),  
		  _packet(""), _client_socket(-1) {
		ericahttp::split_url(std::forward<std::string&&>(url), _host, _path);
		this->_create_tcp_conn();
	}


	http_client::~http_client() {
		std::cout << "Close connection..." << std::endl;
		this->_client_close();
	}

	void http_client::_build_request_header() {
		char local_host[60];
		char date[60];
		time_t date_t = time(nullptr);
		strftime(date, 60, "%a, %d %b %Y %T", gmtime(&date_t));
		gethostname(local_host, sizeof(local_host));
		std::stringstream ss;
		ss << "Host:" + _host + "\r\n"
		   << "Connection:keep-alive\r\n"
		   << "Date:" + std::string(date) + "\r\n"
		   << "Accept:text/html\r\n"
		   << "User-agent:" + std::string(local_host) + "\r\n"
		   ;
		_request_header = ss.str();
	}

	void http_client::_create_tcp_conn() {
		// create client TCP socket
		_client_socket = socket(AF_INET, SOCK_STREAM, 0);
		if(_client_socket == -1)
			throw ericahttp::socketCreateException();
		// init struct
		sockaddr_in addr;
		memset(&addr, 0, sizeof(addr));
		// find host ip by name through DNS service
		auto hpk = gethostbyname(_host.c_str()); 
		if(!hpk)
			throw ericahttp::DNSException(_host.data());
		// fill the struct
		addr.sin_family = AF_INET;		// protocol
		addr.sin_addr.s_addr = inet_addr(inet_ntoa(*(in_addr *)(hpk->h_addr_list[0])));		// service IP
		addr.sin_port = htons(80);		// target process port number
	    std::cout << "HOST IP is " << inet_ntoa(addr.sin_addr) << std::endl;	
		// connect
		int conn_flag = connect(_client_socket, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
		if(conn_flag == -1)
			throw ericahttp::connectException();
		std::cout << "TCP connection creates successed." << std::endl;
	}

	void http_client::_make_packet(const std::string& body) {
		std::stringstream ss;
		switch(_op) {
			case GET:
				ss << "GET";
				break;
			case POST:
				ss << "POST";
				break;
		}
		ss << " " << _path << " " << _version << "\r\n"
		   << _request_header << "\r\n"
		   << body;
		std::cout << "Packet: " << std::endl;
		std::cout << ss.str() << std::endl;
		_packet = ss.str();
	}

	void http_client::_client_close() {
		if(_client_socket == -1) 
			return;
		close(_client_socket);
	}

	void http_client::set_op(OPERATE op) {
		_op = op;
		_is_set_op = true;
		this->_build_request_header();
	}

	void http_client::add_header_line(const std::string &command,
									  const std::string &value) {
		if(command != "Host" && command != "Connection" 
		&& command != "Date" && command != "Accept"
		&& command != "User-agent") {
			_request_header += (command + ":" + value + "\r\n");
		}
	}

	size_t http_client::send_packet(const std::string& body) {
		if(!_is_set_op) {
			std::cout << "Operation NOT set." << std::endl;
			return -1;
		}
		this->_make_packet(body);
		return send(_client_socket, _packet.c_str(), _packet.length(), 0);
	}

	std::pair<int, std::string> http_client::recv_packet(int buffer_len) {
		if(_client_socket == -1) 
			return std::make_pair(0, "");
		// Receive HTTP response packet
		int content_len = 0;
		std::string data_packet("");
		char tmp[buffer_len];
		while(true) {
			ssize_t len = recv(_client_socket, tmp, buffer_len, 0);
			if(len <= 0)
				break;
			data_packet.append(tmp, buffer_len);
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
			memset(tmp, 0, buffer_len);
		}
		// Spilt packet by lines
		auto line = ericahttp::split(data_packet, "\r\n");
		// Spilt first line, get the staus code
		std::string s = ericahttp::split(line[0])[1];
		int status_code = (s[0]-'0')*100 + (s[1]-'0')*10 + (s[2]-'0');
		// Find body
		std::string body("");
		int body_start = data_packet.find("\r\n\r\n");
		body_start += 4;
		body.append(data_packet, body_start, content_len);
		return std::make_pair(status_code, body);
	}
}

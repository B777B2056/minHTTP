#include <vector>
#include <string>
#include <cstring>
#include <exception>
#include <algorithm>

namespace ericahttp {
	// String split by other string
	std::vector<std::string> split(const std::string &str, 
		 	   		   			   const std::string &sp = " ");
	// Spilt URL to host name and file path
	void split_url(const std::string &url, std::string &host, std::string &path);

	void split_url(std::string &&url, std::string &host, std::string &path);

	// TCP socket creation ERROR
	class socketCreateException : public std::exception {
		public:
		    socketCreateException() : message("Error: TCP socket creates failed.") {}
		    ~socketCreateException() throw () {}

		    virtual const char* what() const throw () {
		        return message;
		    }

		private:
		    const char *message;
	};

	// DNS service ERROR
	class DNSException : public std::exception {
		public:
		    DNSException(const char *host)
				: message("Error: DNS service failed to turn host name to IP address, which host name is ") {
                strcat(message, host);
			}

		    ~DNSException() throw () {}

		    virtual const char* what() const throw () {
		        return message;
		    }

		private:
		    char message[256];
	};

	// TCP connection ERROR
	class connectException : public std::exception {
		public:
		    connectException() : message("Error: TCP connects failed.") {}
		    ~connectException() throw () {}

		    virtual const char* what() const throw () {
		        return message;
		    }

		private:
		    const char *message;
	};
}

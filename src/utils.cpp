#include "../inc/utils.hpp"

namespace ericahttp {
	std::vector<std::string> split(const std::string &str, 
		 	   		   			   const std::string &sp) {
		std::vector<std::string> m;
		for(auto it = str.begin(); it != str.end(); ) {
			auto t = std::search(it, str.end(), sp.begin(), sp.end());
			if(t != std::end(str)) {
				m.push_back(std::string(it, t));
				std::advance(t, sp.length());
				it = t;
			} else {
				m.push_back(std::string(it, str.end()));
				break;
			}
		}
		return m;
	}

	void split_url(const std::string &url, std::string &host, std::string &path) {
		int cnt = 0;
		auto it = url.begin();
		for( ; it != url.end(); ++it) {
			if(*it == '/')
				++cnt;
			if(cnt == 2 && *it != '/')
				host += *it;
			if(cnt == 3)
				break;
		}
		while(it != url.end())
			path += (*it++);
	}

	void split_url(std::string &&url, std::string &host, std::string &path) {
		int cnt = 0;
		auto it = url.begin();
		for( ; it != url.end(); ++it) {
			if(*it == '/')
				++cnt;
			if(cnt == 2 && *it != '/')
				host += *it;
			if(cnt == 3)
				break;
		}
		while(it != url.end())
			path += (*it++);
	}

}

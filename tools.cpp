
#include "tools.hpp"

// TOKENIZE STRING
std::vector<std::string> tokenize_string(const std::string &str, char delim)
{
	std::vector<std::string> cont;
	std::size_t current, previous= 0;
	current = str.find(delim);
	while (current != std::string::npos)
	{
		cont.push_back(str.substr(previous, current - previous));
		previous = current + 1;
		current = str.find(delim, previous);
	}
	cont.push_back(str.substr(previous, current - previous));
	
	return cont;
}


 
#ifndef _H_EXCEPTION
#define _H_EXCEPTION

#include <exception>
#include <string>

class Exception : public std::exception
{
protected:
	std::string description;
public:
	Exception(const std::string &description = "") : description(description) {}
	Exception(const Exception&) = default;
	std::string what(void) { return description; }
};

#endif

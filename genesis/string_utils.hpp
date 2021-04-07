#ifndef __STRING_UTILS_HPP__
#define __STRING_UTILS_HPP__

#include <sstream>
#include <string>

// string utils
namespace su
{

template <class T>
std::string hex_str(T t, size_t wide = sizeof(T) * 2)
{
	std::stringstream ss;
	ss << "0x";
	ss << std::hex << std::setfill('0') << std::setw(wide);
	ss << t;
	return ss.str();
}


inline void trim(std::string& str)
{
	auto not_space = [](auto c) { return !std::isspace(c); };

	// ltrim
	str.erase(str.begin(), std::find_if(str.begin(), str.end(), not_space));
	// rtrim
	str.erase(std::find_if(str.rbegin(), str.rend(), not_space).base(), str.end());
}

} // namespace su

#endif // __STRING_UTILS_HPP__

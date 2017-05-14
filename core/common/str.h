#ifndef _STR_H
#define _STR_H

#include <string>
#include <vector>

namespace str
{
    std::string ascii_dump(const std::string& in, const std::string& replacement = ".");
    std::string capitalize(const std::string&);
    std::string codepoint_to_utf8(uint32_t);
    bool        contains(const std::string& haystack, const std::string& needle);
    std::string downcase(const std::string& input);
    std::string format(const char* fmt, ...);
    std::string group_digits(const std::string& in, const std::string& separator = ",");
    std::string hexdump(const std::string& in);
    std::string inspect(const std::string str);
    bool        is_prefix_of(const std::string&, const std::string&);
    std::string join(const std::string& delimiter, const std::vector<std::string>& vec);
    std::string repeat(const std::string&, int n);
    std::string replace_prefix(const std::string& s, const std::string& prefix, const std::string& replacement);
    std::string replace_suffix(const std::string& s, const std::string& suffix, const std::string& replacement);
    std::vector<std::string> split(const std::string& in, const std::string& separator);
    std::string upcase(const std::string& input);
}

#endif

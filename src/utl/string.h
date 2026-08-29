#pragma once

#include <string>
#include <vector>


namespace wlf::utl {

    struct icomp {
        bool operator() (const std::string& lhs, const std::string& rhs) const noexcept;
    };

    // Splits a string into multiple parts which are separated by a delimiter. The function
    // adds the parts to the specified vector and returns the number of added parts.
    size_t split(const char* str, const char delim, std::vector<std::string>& parts);

    // Converts a string to an integer. The function returns true if the conversion
    // succeeds. The value parameter remains untouched if an error was detected.
    bool str2num(const std::string& numstr, const int radix, const long minval, const long maxval, long& value);
    bool str2i(const std::string& numstr, int& value);

    // Trims string.
    std::wstring trimright(const std::wstring& str);
    std::wstring trimleft(const std::wstring& str);
    std::wstring trim(const std::wstring& str);

    std::string trimright(const std::string& str);
    std::string trimleft(const std::string& str);
    std::string trim(const std::string& str);

    // Converts string to lower case.
    std::string lower(const std::string& str);

    // Converts string to upper case.
    std::string upper(const std::string& str);

    // Performs a case insensitive string comparison.
    bool iequal(std::string const& s1, std::string const& s2);

    // Converts an ascii string to a widestring
    std::wstring str2wstr(const std::string& str);
}
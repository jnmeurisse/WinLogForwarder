#pragma once

#include <string>

namespace wlf::utl {

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

}
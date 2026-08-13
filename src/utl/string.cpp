#include "string.h"

#include <cstdlib>
#include <cerrno>

namespace wlf::utl {

    bool str2num(const std::string& numstr, const int radix, const long minval, const long maxval, long& value)
    {
        if (minval > maxval) {
            errno = EINVAL;

        }
        else {
            errno = 0;
            long l = std::strtol(numstr.c_str(), nullptr, radix);

            if (errno == 0) {
                if (l < minval || l > maxval) {
                    errno = ERANGE;
                }
                else {
                    value = l;
                }
            }
        }

        return errno == 0;
    }


    bool str2i(const std::string& str, int& value)
    {
        long tmp;
        const bool ok = str2num(str, 10, INT_MIN, INT_MAX, tmp);
        if (ok)
            value = tmp;

        return ok;
    }


    std::wstring trimright(const std::wstring& str)
    {
        return str.length() == 0 ? str : str.substr(0, str.find_last_not_of(L" \t") + 1);
    }


    std::wstring trimleft(const std::wstring& str)
    {
        return str.length() == 0 ? str : str.substr(str.find_first_not_of(L" \t"));
    }


    std::wstring trim(const std::wstring& str)
    {
        return str.length() == 0 ? str : trimleft(trimright(str));
    }


    std::string trimright(const std::string& str)
    {
        return str.length() == 0 ? str : str.substr(0, str.find_last_not_of(" \t") + 1);
    }


    std::string trimleft(const std::string& str)
    {
        return str.length() == 0 ? str : str.substr(str.find_first_not_of(" \t"));
    }


    std::string trim(const std::string& str)
    {
        return str.length() == 0 ? str : trimleft(trimright(str));
    }

}
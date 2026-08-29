#include "string.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <cstdlib>
#include <cerrno>
#include <vector>
#include <algorithm>
#include <cctype>
#include <cstring>


namespace wlf::utl {

    bool icomp::operator()(const std::string& lhs, const std::string& rhs) const noexcept
    {
        return ::_stricmp(lhs.c_str(), rhs.c_str()) < 0;
    }


    size_t split(const char* str, const char delim, std::vector<std::string>& parts)
    {
        const size_t count = parts.size();

        do {
            const char* const begin = str;

            while (*str != delim && *str)
                str++;

            parts.emplace_back(begin, str);
        } while (0 != *str++);

        return parts.size() - count;
    }


    size_t split(const std::string& str, const char delim, std::vector<std::string>& parts)
    {
        return split(str.c_str(), delim, parts);
    }


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


    std::string lower(const std::string& str)
    {
        std::string tmp = str;
        std::transform(tmp.begin(), tmp.end(), tmp.begin(), tolower);

        return tmp;
    }


    std::string upper(const std::string& str)
    {
        std::string tmp = str;
        std::transform(tmp.begin(), tmp.end(), tmp.begin(), toupper);

        return tmp;
    }


    static bool icheq(unsigned char a, unsigned char b) noexcept
    {
        return std::tolower(a) == std::tolower(b);
    }


    bool iequal(std::string const& s1, std::string const& s2)
    {
        return (s1.length() == s2.length()) && std::equal(s2.begin(), s2.end(), s1.begin(), icheq);
    }


    std::wstring str2wstr(const std::string& str)
    {
        const int size = ::MultiByteToWideChar(
            CP_UTF8, 0,
            str.data(), (int)str.size(),
            nullptr, 0);
        std::vector<wchar_t> result(size, 0);

        ::MultiByteToWideChar(CP_UTF8, 0,
            str.data(), (int)str.size(),
            result.data(), (int)result.size());

        return std::wstring(result.data(), result.size());
    }


}
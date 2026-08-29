#include "user_cache.h"

#include <string_view>
#include "utl/exception.h"
#include <mutex>


namespace wlf::utl {

    UserSID::UserSID(const::PSID psid)
    {
        if (psid && ::IsValidSid(psid)) {
            _sid_size = ::GetLengthSid(psid);
            
            if (_sid_size > _sid.size())
                throw utl::os_error("UserSID::UserSID - buffer size", ERROR_INSUFFICIENT_BUFFER);

            if (!CopySid(_sid_size, _sid.data(), psid))
                throw utl::os_error("UserSID::UserSID - copy sid", ::GetLastError());

            _hash = std::hash<std::string_view>()({
                        reinterpret_cast<char *>(_sid.data()),
                        _sid_size
                    });
        }
    }


    bool UserSID::operator==(const UserSID& other) const
    {
        return
            (this->valid() && other.valid()) &&
            (::EqualSid(sid(), other.sid()));
    }


    size_t UserSID::HashFunction::operator()(const UserSID& sid) const
    {
        return sid._hash;
    }


	static bool lookup_account_sid(const UserSID& user_sid, UserAccountInfo& account_info)
	{
		if (!user_sid.valid()) 
            return false;

		::DWORD name_size = static_cast<DWORD>(account_info.user_name.size());
		::DWORD domain_size = static_cast<DWORD>(account_info.domain.size());

		// Zero-initialize arrays for safety
		account_info.user_name.fill(L'\0');
		account_info.domain.fill(L'\0');

		BOOL success = ::LookupAccountSidW(
			nullptr,
			user_sid.sid(),
			account_info.user_name.data(),
			&name_size,
			account_info.domain.data(),
			&domain_size,
			&account_info.account_type
		);

		return success != FALSE;
	}


	UserCache::UserCache(size_t cache_size) noexcept
		: _max_cache_size(cache_size)
	{
	}


	const UserAccountInfoPtr UserCache::get_account_info(const UserSID& user_sid)
	{
        if (!user_sid.valid())
			return nullptr;

		std::lock_guard<std::mutex> lock(_mutex);

		// Check cache first.
		auto it = _sid_cache.find(user_sid);
		if (it != _sid_cache.end()) {
			// it->second is the iterator. We move it to the front.
			_sid_lru_list.splice(_sid_lru_list.begin(), _sid_lru_list, it->second);

			// Dereference the iterator to get the pair, then return the user information
			return it->second->second;
		}

		// Cache miss, resolve the sid.
		auto account_info = std::make_shared<UserAccountInfo>();
		if (!lookup_account_sid(user_sid, *account_info))
			return nullptr;

		// Enforce cache size limit.
		if (_max_cache_size > 0 && _sid_cache.size() > _max_cache_size) {
			_sid_cache.erase(_sid_lru_list.back().first);
			_sid_lru_list.pop_back();
		}

		// Insert a new entry at the front of the LRU list.
		_sid_lru_list.push_front({ user_sid, account_info });
		_sid_cache[user_sid] = _sid_lru_list.begin();

		// Return the newly cached account
		return _sid_lru_list.front().second;
	}

}
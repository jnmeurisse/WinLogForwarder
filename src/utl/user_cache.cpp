#include "user_cache.h"

#include <sddl.h>


namespace wlf::utl {

	static std::wstring sid_to_string(const ::PSID psid)
	{
		std::wstring sid_string;

		::LPWSTR buffer = nullptr;
		if (psid && ::ConvertSidToStringSidW(psid, &buffer)) {
			sid_string.assign(buffer);
			::LocalFree(buffer);
		}

		return sid_string;
	}


	static bool lookup_account_sid(::PSID psid, UserAccountInfo& account_info)
	{
		if (!psid || !::IsValidSid(psid))
			return false;

		::DWORD name_size = static_cast<DWORD>(account_info.user_name.size());
		::DWORD domain_size = static_cast<DWORD>(account_info.domain.size());

		// Zero-initialize arrays for safety
		account_info.user_name.fill(L'\0');
		account_info.domain.fill(L'\0');

		BOOL success = ::LookupAccountSidW(
			nullptr,
			psid,
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


	const UserAccountInfoPtr UserCache::get_account_info(const ::PSID psid)
	{
		std::wstring sid_key{ sid_to_string(psid) };
		if (sid_key.empty())
			return nullptr;

		std::lock_guard<std::mutex> lock(_mutex);

		// Check cache first.
		auto it = _sid_cache.find(sid_key);
		if (it != _sid_cache.end()) {
			// it->second is the iterator. We move it to the front.
			_sid_lru_list.splice(_sid_lru_list.begin(), _sid_lru_list, it->second);

			// Dereference the iterator to get the pair, then return the user information
			return it->second->second;
		}

		// Cache miss, resolve the sid.
		auto account_info = std::make_shared<UserAccountInfo>();
		if (!lookup_account_sid(psid, *account_info))
			return nullptr;

		// Enforce cache size limit.
		if (_max_cache_size > 0 && _sid_cache.size() > _max_cache_size) {
			_sid_cache.erase(_sid_lru_list.back().first);
			_sid_lru_list.pop_back();
		}

		// Insert a new entry at the front of the LRU list.
		_sid_lru_list.push_front({ sid_key, account_info });
		_sid_cache[sid_key] = _sid_lru_list.begin();

		// Return the newly cached account
		return _sid_lru_list.front().second;
	}

}
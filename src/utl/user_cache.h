#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <lmcons.h>


#include <array>
#include <list>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>



namespace wlf::utl {
	constexpr unsigned int MAX_USERNAME_LENGTH = UNLEN;
	constexpr unsigned int MAX_DOMAIN_LENGTH = 256;


	struct UserAccountInfo {
		std::array<wchar_t, MAX_USERNAME_LENGTH> user_name;
		std::array<wchar_t, MAX_DOMAIN_LENGTH> domain;
		::SID_NAME_USE account_type;
	};


    using UserAccountInfoPtr = std::shared_ptr<const UserAccountInfo>;


    /**
     * A LRU cache for user information based on Security Identifiers (SIDs).
     *
    */
    class UserCache {
    public:
        UserCache() = delete;

		
		UserCache(size_t cache_size) noexcept;
        ~UserCache() = default;

		// Prevent copying and moving to ensure thread safety and preserve internal cache state.
		UserCache(const UserCache&) = delete;
		UserCache& operator=(const UserCache&) = delete;
		UserCache(UserCache&&) = delete;
		UserCache& operator=(UserCache&&) = delete;
 
		/**
         * @brief Retrieves user information for a given SID.
         *
         * If the user information is already cached, it returns the cached
         * value. Otherwise, it performs a lookup and caches the result.
         *
         * @param psid The SID for which to retrieve user information.
         * @return A pointer to the user information, or null pointer if the SID
         *        if not found.
         */
        const UserAccountInfoPtr get_account_info(const ::PSID psid);

    private:
        std::mutex _mutex;

        // Maximum number of entries in the cache.
        const size_t _max_cache_size;

        // Tracks the access order of SIDs (Most recently used at the front).
        using sid_lru_list = std::list<std::pair<std::wstring, UserAccountInfoPtr>>;
        sid_lru_list _sid_lru_list;

        // Maps the SID string to a pair containing the Account Name and the
        // iterator pointing to the LRU list.
        std::unordered_map<std::wstring, sid_lru_list::iterator> _sid_cache;
    };

}
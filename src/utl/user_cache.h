/*!
* This file is part of WindowsLogForwarder
*
* Copyright (C) 2026 Jean-Noel Meurisse
* SPDX-License-Identifier: GPL-3.0-only
*/
#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <lmcons.h>

#include <array>
#include <list>
#include <memory>
#include <mutex>
#include <unordered_map>


namespace wlf::utl {
	constexpr unsigned int MAX_USERNAME_LENGTH = UNLEN;
	constexpr unsigned int MAX_DOMAIN_LENGTH = 256;
    constexpr unsigned int MAX_SID_LENGTH = SECURITY_MAX_SID_SIZE;

    class UserSID {
    public:
        UserSID() = delete;
        explicit UserSID(const ::PSID psid);

        bool operator==(const UserSID& other) const;

        struct HashFunction {
            size_t operator()(const UserSID& sid) const;
        };

        inline PSID sid() const noexcept { return valid() ? (PSID)_sid.data() : nullptr; }
        inline bool valid() const noexcept { return _sid_size > 0; }
    private:
        // A buffer storing the security ID of a user
        std::array<::BYTE, MAX_SID_LENGTH> _sid;

        // The size of the Security ID stored in the buffer
        ::DWORD _sid_size = 0;

        // A hash of the security ID
        size_t _hash = 0;
    };


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
		explicit UserCache(size_t cache_size) noexcept;
        ~UserCache() = default;

		// Prevent copying and moving to ensure thread safety and 
        // preserve internal cache state.
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
         * @param user_sid The user SID for which to retrieve user information.
         * @return A pointer to the user information, or null pointer if the SID
         *         if not found.
         */
        const UserAccountInfoPtr get_account_info(const UserSID& user_sid);

    private:
        std::mutex _mutex;

        // Maximum number of entries in the cache.
        const size_t _max_cache_size;

        // Tracks the access order of SIDs (Most recently used at the front).
        using sid_lru_list = std::list<std::pair<UserSID, UserAccountInfoPtr>>;
        sid_lru_list _sid_lru_list;

        // Maps the SID string to a pair containing the Account Name and the
        // iterator pointing to the LRU list.
        std::unordered_map<UserSID, sid_lru_list::iterator, UserSID::HashFunction> _sid_cache;
    };

}
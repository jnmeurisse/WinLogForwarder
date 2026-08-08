#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <cstdint>
#include <memory>
#include <string>

#include "evt/event_data.h"
#include "evt/event_message.h"
#include "utl/pugixml.hpp"

namespace wlf::evt {

	class EventFormatter {
	public:

		virtual ~EventFormatter() = default;
		virtual bool format(EventMessageBuilder& emb, const EventData& event_data) const noexcept = 0;
	};

	using IEventFormatter = std::shared_ptr<EventFormatter>;


	class RFC5424EventFormatter : public EventFormatter
	{
	public:
        struct Options {
			unsigned int facility;
			std::wstring hostname;
			std::wstring sdid;
		};

		explicit RFC5424EventFormatter(const Options& options) noexcept;
		bool format(EventMessageBuilder& emb, const EventData& event_data) const noexcept override;

	private:
		const Options _options;

        bool append_header(EventMessageBuilder& emb, const evt::EventData& event_data) const noexcept;
        bool append_space(EventMessageBuilder& emb) const noexcept;
        bool append_priority(EventMessageBuilder& emb, unsigned int priority) const noexcept;
        bool append_version(EventMessageBuilder& emb, unsigned int version) const noexcept;
        bool append_timestamp(EventMessageBuilder& emb) const noexcept;
        bool append_id(EventMessageBuilder& emb, unsigned int id, size_t count) const noexcept;
        bool append_sd(EventMessageBuilder& emb, const evt::EventData& event_data) const noexcept;

		static bool append_sd_param(EventMessageBuilder& emb, const wchar_t* name, const wchar_t* value) noexcept;
        static bool append_sd_param(EventMessageBuilder& emb, const wchar_t* name, const ::FILETIME& ft) noexcept;
        static bool append_sd_param(EventMessageBuilder& emb, const wchar_t* name, uint64_t value) noexcept;
        static bool append_sd_param(EventMessageBuilder& emb, pugi::xml_document& xml_doc) noexcept;
        static bool append_sd_name(EventMessageBuilder& emb, const wchar_t* name) noexcept;
        static bool append_sd_value(EventMessageBuilder& emb, const wchar_t* value, size_t count) noexcept;
		//bool append_xml(EventMessageBuilder& message, const evt::EventData& event_data) const noexcept;
    
        static bool append_ascii(EventMessageBuilder& emb, const wchar_t* str, size_t count, const wchar_t *excluded, wchar_t replacement) noexcept;
    };

}

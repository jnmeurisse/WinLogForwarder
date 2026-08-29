#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <cstdint>
#include <memory>

#include "evt/event_data.h"
#include "evt/event_message.h"
#include "evt/event_types.h"


namespace wlf::evt {

	class EventFormatter abstract {
	public:
        EventFormatter() = default;
		virtual ~EventFormatter() = default;

        virtual size_t max_message_size() const noexcept = 0;
		virtual bool format(const EventData& evd, EventMessageBuilder& emb) const noexcept = 0;
    };

	using EventFormatterPtr = std::shared_ptr<EventFormatter>;


    /**
     * A RFC5424 Structured Data Parameter Name.
     */
    class SDParamName abstract
    {
    public:
        virtual ~SDParamName() = default;
        virtual bool append(EventMessageBuilder& emb) const noexcept = 0;
        virtual explicit operator bool() const noexcept = 0;
    };


	class RFC5424EventFormatter : public EventFormatter
	{
	public:
		explicit RFC5424EventFormatter(const EventFormatterConfig& config) noexcept;

        size_t max_message_size() const noexcept override;
        bool format(const EventData& event_data, EventMessageBuilder& emb) const noexcept override;

	private:
		const EventFormatterConfig _config;

        bool append_header(EventMessageBuilder& emb, const evt::EventData& event_data) const noexcept;
        bool append_space(EventMessageBuilder& emb) const noexcept;
        bool append_priority(EventMessageBuilder& emb, unsigned int priority) const noexcept;
        bool append_version(EventMessageBuilder& emb, unsigned int version) const noexcept;
        bool append_timestamp(EventMessageBuilder& emb) const noexcept;
        bool append_id(EventMessageBuilder& emb, unsigned int id, size_t count) const noexcept;
        bool append_system_sd(EventMessageBuilder& emb, const EventData& event_data) const noexcept;
        void append_event_sd(EventMessageBuilder& emb, const EventData& event_data) const noexcept;

		bool append_sd_param(EventMessageBuilder& emb, const SDParamName& name, const wchar_t* value, size_t max_chars) const noexcept;
        bool append_sd_param(EventMessageBuilder& emb, const SDParamName& name, const ::FILETIME& ft) const noexcept;
        bool append_sd_param(EventMessageBuilder& emb, const SDParamName& name, uint64_t value) const noexcept;
        bool append_sd_param(EventMessageBuilder& emb, const SDParamName& name, const ::GUID* value) const noexcept;

        bool append_sd_value(EventMessageBuilder& emb, const wchar_t* value, size_t max_code_units) const noexcept;
        void append_user_data(EventMessageBuilder& emb, const evt::EventData& event_data) const noexcept;
    };

}

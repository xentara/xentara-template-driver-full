// Copyright (c) embedded ocean GmbH
#pragma once

#include "AbstractTemplateOutputHandler.hpp"
#include "AbstractInput.hpp"
#include "AbstractOutput.hpp"

#include <xentara/process/Task.hpp>
#include <xentara/skill/DataPoint.hpp>
#include <xentara/skill/EnableSharedFromThis.hpp>
#include <xentara/utils/json/decoder/Value.hpp>

#include <functional>
#include <string_view>

namespace xentara::plugins::templateDriver
{

using namespace std::literals;

class TemplateIoComponent;
class TemplateBatchTransaction;

/// @brief A class representing a specific type of output.
/// @note This class derived from AbstractInput as well as AbstractOutput, so that we can read back the currently set value
/// from the I/O component.
/// @todo rename this class to something more descriptive
class TemplateOutput final :
	public skill::DataPoint,
	public AbstractInput,
	public AbstractOutput,
	public skill::EnableSharedFromThis<TemplateOutput>
{
private:
	/// @brief A structure used to store the class specific attributes within an element's configuration
	struct Config final
	{
		/// @todo Add custom config attributes
	};
	
public:
	/// @brief The class object containing meta-information about this element type
	class Class final : public skill::Element::Class
	{
	public:
		/// @brief Gets the global object
		static auto instance() -> Class&
		{
			return _instance;
		}

	    /// @brief Returns the array handle for the class specific attributes within an element's configuration
	    auto configHandle() const -> const auto &
        {
            return _configHandle;
        }

		/// @name Virtual Overrides for skill::Element::Class
		/// @{

		auto name() const -> std::string_view final
		{
			/// @todo change class name
			return "TemplateOutput"sv;
		}
	
		auto uuid() const -> utils::core::Uuid final
		{
			/// @todo assign a unique UUID
			return "deadbeef-dead-beef-dead-beefdeadbeef"_uuid;
		}

		/// @}

	private:
	    /// @brief The array handle for the class specific attributes within an element's configuration
		memory::Array::ObjectHandle<Config> _configHandle { config().appendObject<Config>() };

		/// @brief The global object that represents the class
		static Class _instance;
	};

	/// @brief This constructor attaches the output to its I/O component
	TemplateOutput(std::reference_wrapper<TemplateIoComponent> ioComponent) :
		_ioComponent(ioComponent)
	{
	}
	
	/// @name Virtual Overrides for skill::DataPoint
	/// @{

	auto dataType() const -> const data::DataType & final;

	auto directions() const -> io::Directions final;

	auto forEachAttribute(const model::ForEachAttributeFunction &function) const -> bool final;

	auto forEachEvent(const model::ForEachEventFunction &function) -> bool final;

	auto makeReadHandle(const model::Attribute &attribute) const noexcept -> std::optional<data::ReadHandle> final;

	auto makeWriteHandle(const model::Attribute &attribute) noexcept -> std::optional<data::WriteHandle> final;
	
	/// @}

	/// @name Virtual Overrides for AbstractInput
	/// @{

	auto ioComponent() const -> const TemplateIoComponent & final
	{
		return _ioComponent;
	}
	
	auto attachInput(memory::Array &dataArray, std::size_t &eventCount) -> void final;

	auto updateReadState(WriteSentinel &writeSentinel,
		std::chrono::system_clock::time_point timeStamp,
		const utils::eh::expected<std::reference_wrapper<const ReadCommand::Payload>, std::error_code> &payloadOrError,
		const CommonReadState::Changes &commonChanges,
		PendingEventList &eventsToFire) -> void final;
	
	/// @}

	/// @name Virtual Overrides for AbstractOutput
	/// @{

	auto addToWriteCommand(WriteCommand &command) -> bool final;

	auto attachOutput(memory::Array &dataArray, std::size_t &eventCount) -> void final;

	auto updateWriteState(
		WriteSentinel &writeSentinel,
		std::chrono::system_clock::time_point timeStamp,
		std::error_code error,
		PendingEventList &eventsToFire) -> void final;
	
	/// @}

protected:
	/// @name Virtual Overrides for skill::DataPoint
	/// @{

	auto loadConfig(const ConfigIntializer &initializer,
		utils::json::decoder::Object &jsonObject,
		config::Resolver &resolver,
		const config::FallbackHandler &fallbackHandler) -> void final;

	/// @}

private:
	/// @brief Creates an output handler based on a configuration value
	auto createHandler(utils::json::decoder::Value &value) -> std::unique_ptr<AbstractTemplateOutputHandler>;

	/// @brief The I/O component this output belongs to
	/// @todo give this a more descriptive name, e.g. "_device"
	std::reference_wrapper<TemplateIoComponent> _ioComponent;
	
	/// @brief The batch transaction this input belongs to, or nullptr if it hasn't been loaded yet.
	/// @todo give this a more descriptive name, e.g. "_poll"
	TemplateBatchTransaction *_batchTransaction { nullptr };

	/// @brief The handler for data type specific functionality, or nullptr, if the data type hans not been loaded yet
	std::unique_ptr<AbstractTemplateOutputHandler> _handler;

	/// @class xentara::plugins::templateDriver::TemplateOutput
	/// @todo add information needed to decode the value from the payload of a read command, like e.g. a data offset.
};

} // namespace xentara::plugins::templateDriver
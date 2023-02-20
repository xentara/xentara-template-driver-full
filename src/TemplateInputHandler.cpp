// Copyright (c) embedded ocean GmbH
#include "TemplateInputHandler.hpp"

#include "Attributes.hpp"
#include "TemplateIoBatch.hpp"

#include <xentara/data/DataType.hpp>
#include <xentara/data/ReadHandle.hpp>
#include <xentara/memory/WriteSentinel.hpp>
#include <xentara/model/Attribute.hpp>
#include <xentara/utils/tools/Concepts.hpp>

namespace xentara::plugins::templateDriver
{
	
using namespace std::literals;

template <typename ValueType>
const model::Attribute TemplateInputHandler<ValueType>::kValueAttribute { model::Attribute::kValue, model::Attribute::Access::ReadOnly, staticDataType() };

template <typename ValueType>
constexpr auto TemplateInputHandler<ValueType>::staticDataType() -> const data::DataType &
{
	// Return the correct type
	if constexpr (std::same_as<ValueType, bool>)
	{
	    return data::DataType::kBoolean;
	}
	// To determine if a type is an integer type, we use xentara::utils::Tools::Integral instead of std::integral,
	// because std::integral is true for bool, char, wchar_t, char8_t, char16_t, and char32_t, which we don't want.
	else if constexpr (utils::tools::Integral<ValueType>)
	{
	    return data::DataType::kInteger;
	}
	else if constexpr (std::floating_point<ValueType>)
	{
	    return data::DataType::kFloatingPoint;
	}
}

template <typename ValueType>
auto TemplateInputHandler<ValueType>::dataType() const -> const data::DataType &
{
	return kValueAttribute.dataType();
}

template <typename ValueType>
auto TemplateInputHandler<ValueType>::resolveAttribute(std::string_view name, TemplateIoBatch &ioBatch) -> const model::Attribute *
{
	// Handle the value attribute separately
	if (name == kValueAttribute)
	{
		return &kValueAttribute;
	}

	// Check the state attributes
	if (auto attribute = _state.resolveAttribute(name))
	{
		return attribute;
	}
	// Also check the common read state attributes from the I/O batch
	if (auto attribute = ioBatch.resolveReadStateAttribute(name))
	{
		return attribute;
	}

	return nullptr;
}

template <typename ValueType>
auto TemplateInputHandler<ValueType>::resolveEvent(std::string_view name, TemplateIoBatch &ioBatch, std::shared_ptr<void> parent) -> std::shared_ptr<process::Event>
{
	// Check the state events
	if (auto event = _state.resolveEvent(name, parent))
	{
		return event;
	}
	// Also check the common read state events from the I/O batch
	if (auto event = ioBatch.resolveReadStateEvent(name))
	{
		return event;
	}

	return nullptr;
}

template <typename ValueType>
auto TemplateInputHandler<ValueType>::readHandle(const model::Attribute &attribute, TemplateIoBatch &ioBatch) const noexcept -> std::optional<data::ReadHandle>
{
	// Get the data block
	const auto &dataBlock = ioBatch.readDataBlock();
	
	// Handle the value attribute separately
	if (attribute == kValueAttribute)
	{
		return _state.valueReadHandle(dataBlock);
	}
	
	// Check the state attributes
	if (auto handle = _state.readHandle(dataBlock, attribute))
	{
		return *handle;
	}
	// Also check the common read state attributes from the I/O batch
	if (auto handle = ioBatch.readStateReadHandle(attribute))
	{
		return *handle;
	}

	return std::nullopt;
}

template <typename ValueType>
auto TemplateInputHandler<ValueType>::attachReadState(memory::Array &dataArray, std::size_t &eventCount) -> void
{
	_state.attach(dataArray, eventCount);
}

template <typename ValueType>
auto TemplateInputHandler<ValueType>::updateReadState(WriteSentinel &writeSentinel,
	std::chrono::system_clock::time_point timeStamp,
	const utils::eh::Failable<std::reference_wrapper<const ReadCommand::Payload>> &payloadOrError,
	const CommonReadState::Changes &commonChanges,
	PendingEventList &eventsToFire) -> void
{
	// Check if we have a valid payload
	if (const auto payload = payloadOrError.value())
	{
		/// @todo decode the value from the payload data
		ValueType value = {};
		
		/// @todo it may be advantageous to split the decoding of the value up according to value type, either by using helper functions,
		/// or using if constexpr().
		//
		// For example, you could create a function decodeValue(), which would call helper functions named decodeBoolean(), decodeInteger(), and decodeFloatingPoint().
		// This functions could be implemnted like this:
		// 
		// template <typename ValueType>
		// auto TemplateInputHandler<ValueType>decodeValue(const ReadCommand::Payload &payload) -> dectype(auto)
		// {
		//     if constexpr (std::same_as<ValueType, bool>)
		//     {
		//         return decodeBoolean(payload);
		//     }
		//     else if constexpr (utils::Tools::Integral<ValueType>)
		//     {
		//         return decodeInteger(payload);
		//     }
		//     else if constexpr (std::floating_point<ValueType>)
		//     {
		//         return decodeFloatingPoint(payload);
		//     }
		// }
		// 
		// To determine if a type is an integer type, you should use xentara::utils::Tools::Integral instead of std::integral,
		// because std::integral is true for *bool*, *char*, *wchar_t*, *char8_t*, *char16_t*, and *char32_t*, which is generally not desirable.

		// Update the read state
		_state.update(writeSentinel, timeStamp, value, commonChanges, eventsToFire);	
	}
	// We have an error
	else
	{
		// Update the state with the error
		_state.update(writeSentinel, timeStamp, payloadOrError.error(), commonChanges, eventsToFire);
	}
}

/// @class xentara::plugins::templateDriver::TemplateInputHandler
/// @todo change list of template instantiations to the supported types
template class TemplateInputHandler<bool>;
template class TemplateInputHandler<std::uint8_t>;
template class TemplateInputHandler<std::uint16_t>;
template class TemplateInputHandler<std::uint32_t>;
template class TemplateInputHandler<std::uint64_t>;
template class TemplateInputHandler<std::int8_t>;
template class TemplateInputHandler<std::int16_t>;
template class TemplateInputHandler<std::int32_t>;
template class TemplateInputHandler<std::int64_t>;
template class TemplateInputHandler<float>;
template class TemplateInputHandler<double>;

} // namespace xentara::plugins::templateDriver
// Copyright (c) embedded ocean GmbH
#include "TemplateOutputHandler.hpp"

#include "Attributes.hpp"
#include "TemplateIoBatch.hpp"

#include <xentara/data/DataType.hpp>
#include <xentara/data/ReadHandle.hpp>
#include <xentara/data/WriteHandle.hpp>
#include <xentara/memory/WriteSentinel.hpp>
#include <xentara/model/Attribute.hpp>
#include <xentara/utils/tools/Concepts.hpp>

namespace xentara::plugins::templateDriver
{
	
using namespace std::literals;

template <typename ValueType>
const model::Attribute TemplateOutputHandler<ValueType>::kValueAttribute { model::Attribute::kValue, model::Attribute::Access::ReadWrite, staticDataType() };

template <typename ValueType>
constexpr auto TemplateOutputHandler<ValueType>::staticDataType() -> const data::DataType &
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
	    return data::DataType::kBoolean;
	}
	else if constexpr (std::floating_point<ValueType>)
	{
	    return data::DataType::kFloatingPoint;
	}
}

template <typename ValueType>
auto TemplateOutputHandler<ValueType>::dataType() const -> const data::DataType &
{
	return kValueAttribute.dataType();
}

template <typename ValueType>
auto TemplateOutputHandler<ValueType>::resolveAttribute(std::u16string_view name, TemplateIoBatch &ioBatch) -> const model::Attribute *
{
	// Handle the value attribute separately
	if (name == kValueAttribute)
	{
		return &kValueAttribute;
	}

	// Check the read state attributes
	if (auto attribute = _readState.resolveAttribute(name))
	{
		return attribute;
	}
	// Also check the common read state attributes from the I/O batch
	if (auto attribute = ioBatch.resolveReadStateAttribute(name))
	{
		return attribute;
	}

	// Check the write state attributes
	if (auto attribute = _writeState.resolveAttribute(name))
	{
		return attribute;
	}

	return nullptr;
}

template <typename ValueType>
auto TemplateOutputHandler<ValueType>::resolveEvent(std::u16string_view name, TemplateIoBatch &ioBatch, std::shared_ptr<void> parent) -> std::shared_ptr<process::Event>
{
	// Check the read state events
	if (auto event = _readState.resolveEvent(name, parent))
	{
		return event;
	}
	// Also check the common read state events from the I/O batch
	if (auto event = ioBatch.resolveReadStateEvent(name))
	{
		return event;
	}

	// Check the write state events
	if (auto event = _writeState.resolveEvent(name, parent))
	{
		return event;
	}

	return nullptr;
}

template <typename ValueType>
auto TemplateOutputHandler<ValueType>::readHandle(const model::Attribute &attribute, TemplateIoBatch &ioBatch) const noexcept -> std::optional<data::ReadHandle>
{
	// Get the data blocks
	const auto &readDataBlock = ioBatch.readDataBlock();
	const auto &writeDataBlock = ioBatch.writeDataBlock();
	
	// Handle the value attribute separately
	if (attribute == kValueAttribute)
	{
		return _readState.valueReadHandle(readDataBlock);
	}
	
	// Check the read state attributes
	if (auto handle = _readState.readHandle(readDataBlock, attribute))
	{
		return *handle;
	}
	// Also check the common read state attributes from the I/O batch
	if (auto handle = ioBatch.readStateReadHandle(attribute))
	{
		return *handle;
	}

	// Check the write state attributes
	if (auto handle = _writeState.readHandle(writeDataBlock, attribute))
	{
		return *handle;
	}

	return std::nullopt;
}

template <typename ValueType>
auto TemplateOutputHandler<ValueType>::writeHandle(const model::Attribute &attribute, TemplateIoBatch &ioBatch, std::shared_ptr<void> parent) noexcept -> std::optional<data::WriteHandle>
{
	// Handle the value attribute, which is the only writable attribute
	if (attribute == kValueAttribute)
	{
		// Make a shared pointer that refers to this handler
		std::shared_ptr<TemplateOutputHandler<ValueType>> sharedThis(parent, this);

		// This magic code creates a write handle of type ValueType that calls scheduleWrite() on sharedThis.
		// (There are two sets of braces needed here: one for data::WriteHandle, and one for std::optional)
		return {{ std::in_place_type<ValueType>, &TemplateOutputHandler<ValueType>::scheduleOutputValue, sharedThis }};
	}

	return std::nullopt;
}

template <typename ValueType>
auto TemplateOutputHandler<ValueType>::attachReadState(memory::Array &dataArray, std::size_t &eventCount) -> void
{
	_readState.attach(dataArray, eventCount);
}

template <typename ValueType>
auto TemplateOutputHandler<ValueType>::updateReadState(WriteSentinel &writeSentinel,
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
		// auto TemplateOutputHandler<ValueType>decodeValue(const ReadCommand::Payload &payload) -> dectype(auto)
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
		_readState.update(writeSentinel, timeStamp, value, commonChanges, eventsToFire);
	}
	// We have an error
	else
	{
		// Update the read state with the error
		_readState.update(writeSentinel, timeStamp, payloadOrError.error(), commonChanges, eventsToFire);
	}
}

template <typename ValueType>
auto TemplateOutputHandler<ValueType>::attachWriteState(memory::Array &dataArray, std::size_t &eventCount) -> void
{
	_writeState.attach(dataArray, eventCount);
}

template <typename ValueType>
auto TemplateOutputHandler<ValueType>::addToWriteCommand(WriteCommand &command) -> bool
{
	// Get the value
	auto pendingValue = _pendingOutputValue.dequeue();
	// If there was no pending value, do nothing
	if (!pendingValue)
	{
		return false;
	}

	/// @todo add the value to the command

	/// @todo it may be advantageous to split this function up according to value type, either using explicit 
	/// template specialization, or using if constexpr().
	//
	// For example, this function could be split into addBooleanToWriteCommand(), addIntegerToWriteCommand(), and addFloatingPointToWriteCommand() functions.
	// These functions could then be called like this:
	//
	// if constexpr (std::same_as<ValueType, bool>)
	// {
	//     addBooleanToWriteCommand(command, *pendingValue);
	// }
	// else if constexpr (utils::Tools::Integral<ValueType>)
	// {
	//     addIntegerToWriteCommand(command, *pendingValue);
	// }
	// else if constexpr (std::floating_point<ValueType>)
	// {
	//     addFloatingPointToWriteCommand(command, *pendingValue);
	// }
	//
	// To determine if a type is an integer type, you should use xentara::utils::Tools::Integral instead of std::integral,
	// because std::integral is true for *bool*, *char*, *wchar_t*, *char8_t*, *char16_t*, and *char32_t*, which is generally not desirable.

	return true;
}

template <typename ValueType>
auto TemplateOutputHandler<ValueType>::updateWriteState(WriteSentinel &writeSentinel,
	std::chrono::system_clock::time_point timeStamp,
	std::error_code error,
	PendingEventList &eventsToFire) -> void
{
	// Update the write state
	_writeState.update(writeSentinel, timeStamp, error, eventsToFire);
}

/// @class xentara::plugins::templateDriver::TemplateOutputHandler
/// @todo change list of template instantiations to the supported types
template class TemplateOutputHandler<bool>;
template class TemplateOutputHandler<std::uint8_t>;
template class TemplateOutputHandler<std::uint16_t>;
template class TemplateOutputHandler<std::uint32_t>;
template class TemplateOutputHandler<std::uint64_t>;
template class TemplateOutputHandler<std::int8_t>;
template class TemplateOutputHandler<std::int16_t>;
template class TemplateOutputHandler<std::int32_t>;
template class TemplateOutputHandler<std::int64_t>;
template class TemplateOutputHandler<float>;
template class TemplateOutputHandler<double>;

} // namespace xentara::plugins::templateDriver
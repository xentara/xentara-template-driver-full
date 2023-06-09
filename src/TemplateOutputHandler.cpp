// Copyright (c) embedded ocean GmbH
#include "TemplateOutputHandler.hpp"

#include "Attributes.hpp"
#include "TemplateBatchTransaction.hpp"

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
	    return data::DataType::kInteger;
	}
	else if constexpr (std::floating_point<ValueType>)
	{
	    return data::DataType::kFloatingPoint;
	}
	else if constexpr (utils::tools::StringType<ValueType>)
	{
	    return data::DataType::kString;
	}
}

template <typename ValueType>
auto TemplateOutputHandler<ValueType>::dataType() const -> const data::DataType &
{
	return kValueAttribute.dataType();
}

template <typename ValueType>
auto TemplateOutputHandler<ValueType>::forEachAttribute(const model::ForEachAttributeFunction &function, TemplateBatchTransaction &batchTransaction) const -> bool
{
	return
		// Handle the value attribute separately
		function(kValueAttribute) ||

		// Handle the read state attributes
		_readState.forEachAttribute(function) ||
		// Also handle the common read state attributes from the batch transaction
		batchTransaction.forEachReadStateAttribute(function) ||

		// Handle the write state attributes
		_writeState.forEachAttribute(function);
}

template <typename ValueType>
auto TemplateOutputHandler<ValueType>::forEachEvent(const model::ForEachEventFunction &function, TemplateBatchTransaction &batchTransaction, std::shared_ptr<void> parent) -> bool
{
	return
		// Handle the read state events
		_readState.forEachEvent(function, parent) ||
		// Also handle the common read state events from the batch transaction
		batchTransaction.forEachReadStateEvent(function) ||

		// Handle the write state events
		_writeState.forEachEvent(function, parent);
}

template <typename ValueType>
auto TemplateOutputHandler<ValueType>::makeReadHandle(const model::Attribute &attribute, TemplateBatchTransaction &batchTransaction) const noexcept -> std::optional<data::ReadHandle>
{
	// Get the data blocks
	const auto &readDataBlock = batchTransaction.readDataBlock();
	const auto &writeDataBlock = batchTransaction.writeDataBlock();
	
	// Handle the value attribute separately
	if (attribute == kValueAttribute)
	{
		return _readState.valueReadHandle(readDataBlock);
	}
	
	// Handle the read state attributes
	if (auto handle = _readState.makeReadHandle(readDataBlock, attribute))
	{
		return handle;
	}
	// Also handle the common read state attributes from the batch transaction
	if (auto handle = batchTransaction.makeReadStateReadHandle(attribute))
	{
		return handle;
	}

	// Handle the write state attributes
	if (auto handle = _writeState.makeReadHandle(writeDataBlock, attribute))
	{
		return handle;
	}

	return std::nullopt;
}

template <typename ValueType>
auto TemplateOutputHandler<ValueType>::makeWriteHandle(const model::Attribute &attribute, TemplateBatchTransaction &batchTransaction, std::shared_ptr<void> parent) noexcept -> std::optional<data::WriteHandle>
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
	const utils::eh::expected<std::reference_wrapper<const ReadCommand::Payload>, std::error_code> &payloadOrError,
	const CommonReadState::Changes &commonChanges,
	PendingEventList &eventsToFire) -> void
{
	// Check if we have a valid payload
	if (payloadOrError)
	{
		/// @todo decode the value from the payload data
		ValueType value = {};
		
		/// @todo it may be advantageous to split the decoding of the value up according to value type, either by using helper functions,
		/// or using if constexpr().
		//
		// For example, you could create a function decodeValue(), which would call helper functions named decodeBoolean(), decodeInteger(),
		// decodeFloatingPoint(), and decodeString(). This function could be implemented like this:
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
		//     else if constexpr (utils::tools::StringType<ValueType>)
		//     {
		//         return decodeString(payload);
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
		_readState.update(writeSentinel, timeStamp, utils::eh::unexpected(payloadOrError.error()), commonChanges, eventsToFire);
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
	// For example, this function could be split into addBooleanToWriteCommand(), addIntegerToWriteCommand(),
	// addFloatingPointToWriteCommand(), and addStringToWriteCommand() functions. These functions could then
	// be called like this:
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
	// else if constexpr (utils::tools::StringType<ValueType>)
	// {
	//     addStringToWriteCommand(command, *pendingValue);
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
template class TemplateOutputHandler<std::string>;

} // namespace xentara::plugins::templateDriver
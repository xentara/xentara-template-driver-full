// Copyright (c) embedded ocean GmbH
#include "TemplateInputHandler.hpp"

#include "Attributes.hpp"
#include "TemplateIoTransaction.hpp"

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
	else if constexpr (utils::tools::StringType<ValueType>)
	{
	    return data::DataType::kString;
	}
}

template <typename ValueType>
auto TemplateInputHandler<ValueType>::dataType() const -> const data::DataType &
{
	return kValueAttribute.dataType();
}

template <typename ValueType>
auto TemplateInputHandler<ValueType>::forEachAttribute(const model::ForEachAttributeFunction &function, TemplateIoTransaction &ioTransaction) const -> bool
{
	return
		// Handle the value attribute separately
		function(kValueAttribute) ||

		// Handle the state attributes
		_state.forEachAttribute(function) ||
		// Also handle the common read state attributes from the I/O transaction
		ioTransaction.forEachReadStateAttribute(function);
}

template <typename ValueType>
auto TemplateInputHandler<ValueType>::forEachEvent(const model::ForEachEventFunction &function, TemplateIoTransaction &ioTransaction, std::shared_ptr<void> parent) -> bool
{
	return
		// Handle the state events
		_state.forEachEvent(function, parent) ||
		// Also handle the common read state events from the I/O transaction
		ioTransaction.forEachReadStateEvent(function);
}

template <typename ValueType>
auto TemplateInputHandler<ValueType>::makeReadHandle(const model::Attribute &attribute, TemplateIoTransaction &ioTransaction) const noexcept -> std::optional<data::ReadHandle>
{
	// Get the data block
	const auto &dataBlock = ioTransaction.readDataBlock();
	
	// Handle the value attribute separately
	if (attribute == kValueAttribute)
	{
		return _state.valueReadHandle(dataBlock);
	}
	
	// Handle the state attributes
	if (auto handle = _state.makeReadHandle(dataBlock, attribute))
	{
		return handle;
	}
	// Also handle the common read state attributes from the I/O transaction
	if (auto handle = ioTransaction.makeReadStateReadHandle(attribute))
	{
		return handle;
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
	const utils::eh::expected<std::reference_wrapper<const ReadCommand::Payload>, std::error_code> &payloadOrError,
	const CommonReadState::Changes &commonChanges,
	PendingEventList &eventsToRaise) -> void
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
		//     else if constexpr (utils::tools::StringType<ValueType>)
		//     {
		//         return decodeString(payload);
		//     }
		// }
		// 
		// To determine if a type is an integer type, you should use xentara::utils::Tools::Integral instead of std::integral,
		// because std::integral is true for *bool*, *char*, *wchar_t*, *char8_t*, *char16_t*, and *char32_t*, which is generally not desirable.

		// Update the read state
		_state.update(writeSentinel, timeStamp, value, commonChanges, eventsToRaise);	
	}
	// We have an error
	else
	{
		// Update the state with the error
		_state.update(writeSentinel, timeStamp, utils::eh::unexpected(payloadOrError.error()), commonChanges, eventsToRaise);
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
template class TemplateInputHandler<std::string>;

} // namespace xentara::plugins::templateDriver
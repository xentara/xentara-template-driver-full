// Copyright (c) embedded ocean GmbH
#pragma once

#include "AbstractTemplateOutputHandler.hpp"
#include "PerValueReadState.hpp"
#include "WriteState.hpp"
#include "SingleValueQueue.hpp"

#include <xentara/model/Attribute.hpp>

#include <string>

namespace xentara::plugins::templateDriver
{

using namespace std::literals;

class TemplateBatchTransaction;

/// @brief Data type specific functionality for TemplateOutput.
///
/// @todo rename this class to something more descriptive
///
/// @todo split this class into several classes for different value types or classes of value type, if necessary.
/// For example, this class could be split into TemplateBooleanOutputHandler, TemplateIntegerOutputHandler,
/// and TemplateFloatingPointOutputHandler classes.
template <typename ValueType>
class TemplateOutputHandler final : public AbstractTemplateOutputHandler
{
public:
	/// @name Virtual Overrides for AbstractTemplateOutputHandler
	/// @{

	auto dataType() const -> const data::DataType & final;

	auto forEachAttribute(const model::ForEachAttributeFunction &function, TemplateBatchTransaction &batchTransaction) const -> bool final;

	auto forEachEvent(const model::ForEachEventFunction &function, TemplateBatchTransaction &batchTransaction, std::shared_ptr<void> parent) -> bool final;

	auto makeReadHandle(const model::Attribute &attribute, TemplateBatchTransaction &batchTransaction) const noexcept -> std::optional<data::ReadHandle> final;

	auto makeWriteHandle(const model::Attribute &attribute, TemplateBatchTransaction &batchTransaction, std::shared_ptr<void> parent) noexcept -> std::optional<data::WriteHandle> final;
	
	auto attachReadState(memory::Array &dataArray, std::size_t &eventCount) -> void final;

	auto updateReadState(WriteSentinel &writeSentinel,
		std::chrono::system_clock::time_point timeStamp,
		const utils::eh::expected<std::reference_wrapper<const ReadCommand::Payload>, std::error_code> &payloadOrError,
		const CommonReadState::Changes &commonChanges,
		PendingEventList &eventsToRaise) -> void final;
	
	auto addToWriteCommand(WriteCommand &command) -> bool final;

	auto attachWriteState(memory::Array &dataArray, std::size_t &eventCount) -> void final;

	auto updateWriteState(
		WriteSentinel &writeSentinel,
		std::chrono::system_clock::time_point timeStamp,
		std::error_code error,
		PendingEventList &eventsToRaise) -> void final;
	
	/// @}

	/// @brief A Xentara attribute containing the current value.
	/// @note This is a member of this class rather than of the attributes namespace, because the access flags
	/// and type may differ from class to class
	static const model::Attribute kValueAttribute;

private:
	/// @brief Determines the correct data type based on the *ValueType* template parameter
	///
	/// This function returns the same value as dataType(), but is static and constexpr.
	static constexpr auto staticDataType() -> const data::DataType &;

	/// @brief Schedules a value to be written.
	///
	/// This function is called by the value write handle.
	auto scheduleOutputValue(ValueType value) noexcept
	{
		_pendingOutputValue.enqueue(value);
	}

	/// @brief The read state
	PerValueReadState<ValueType> _readState;
	/// @brief The write state
	WriteState _writeState;

	/// @brief The queue for the pending output value
	SingleValueQueue<ValueType> _pendingOutputValue;
};

/// @class xentara::plugins::templateDriver::TemplateOutputHandler
/// @todo change list of extern template statements to the supported types
extern template class TemplateOutputHandler<bool>;
extern template class TemplateOutputHandler<std::uint8_t>;
extern template class TemplateOutputHandler<std::uint16_t>;
extern template class TemplateOutputHandler<std::uint32_t>;
extern template class TemplateOutputHandler<std::uint64_t>;
extern template class TemplateOutputHandler<std::int8_t>;
extern template class TemplateOutputHandler<std::int16_t>;
extern template class TemplateOutputHandler<std::int32_t>;
extern template class TemplateOutputHandler<std::int64_t>;
extern template class TemplateOutputHandler<float>;
extern template class TemplateOutputHandler<double>;
extern template class TemplateOutputHandler<std::string>;

} // namespace xentara::plugins::templateDriver